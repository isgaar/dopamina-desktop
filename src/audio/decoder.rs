// Location: src/audio/decoder.rs
// Function: Pure Rust Symphonia audio decoder supporting local files and HTTP
// 206 byte-range streaming with instant seeking and interleaved f32 conversion.
// All Rights Reserved isgaar

use std::collections::BTreeMap;
use std::fs::File;
use std::io::{self, Read, Seek, SeekFrom};
use std::path::Path;
use std::sync::{Arc, Mutex};
use std::time::Duration;

use symphonia::core::audio::SampleBuffer;
use symphonia::core::codecs::{Decoder, DecoderOptions, CODEC_TYPE_NULL};
use symphonia::core::errors::Error as SymphoniaError;
use symphonia::core::formats::{FormatOptions, FormatReader, SeekMode, SeekTo};
use symphonia::core::io::{MediaSource, MediaSourceStream};
use symphonia::core::meta::MetadataOptions;
use symphonia::core::probe::Hint;
use symphonia::core::units::Time;

const HTTP_CHUNK_BYTES: u64 = 262_144;

struct HttpRangeSource {
    client: reqwest::blocking::Client,
    url: String,
    token: Option<String>,
    pos: u64,
    total_len: Option<u64>,
    supports_range: bool,
    chunks: Arc<Mutex<BTreeMap<u64, Vec<u8>>>>,
    full_body: Option<Vec<u8>>,
}

impl HttpRangeSource {
    fn open(url: &str, token: Option<&str>) -> io::Result<Self> {
        let client = reqwest::blocking::Client::builder()
            .connect_timeout(Duration::from_secs(6))
            .timeout(Duration::from_secs(30))
            .build()
            .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;

        let mut req = client.get(url).header("Range", format!("bytes=0-{}", HTTP_CHUNK_BYTES - 1));
        if let Some(tok) = token.filter(|t| !t.trim().is_empty()) {
            req = req.bearer_auth(tok);
        }
        let resp = req
            .send()
            .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;

        if !resp.status().is_success() {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                format!("HTTP {}", resp.status().as_u16()),
            ));
        }

        let status = resp.status().as_u16();
        let total_len = parse_content_range_total(resp.headers())
            .or_else(|| {
                resp.headers()
                    .get(reqwest::header::CONTENT_LENGTH)
                    .and_then(|v| v.to_str().ok())
                    .and_then(|s| s.parse::<u64>().ok())
            });
        let bytes = resp
            .bytes()
            .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?
            .to_vec();

        if status == 206 {
            let mut map = BTreeMap::new();
            map.insert(0, bytes);
            Ok(Self {
                client,
                url: url.to_string(),
                token: token.map(str::to_string),
                pos: 0,
                total_len,
                supports_range: true,
                chunks: Arc::new(Mutex::new(map)),
                full_body: None,
            })
        } else {
            let len = bytes.len() as u64;
            Ok(Self {
                client,
                url: url.to_string(),
                token: token.map(str::to_string),
                pos: 0,
                total_len: Some(len),
                supports_range: false,
                chunks: Arc::new(Mutex::new(BTreeMap::new())),
                full_body: Some(bytes),
            })
        }
    }

    fn fetch_chunk(&self, chunk_index: u64) -> io::Result<Vec<u8>> {
        if let Some(cached) = self.chunks.lock().unwrap().get(&chunk_index).cloned() {
            return Ok(cached);
        }
        let start = chunk_index * HTTP_CHUNK_BYTES;
        let mut end = start + HTTP_CHUNK_BYTES - 1;
        if let Some(total) = self.total_len {
            if start >= total {
                return Ok(Vec::new());
            }
            end = end.min(total - 1);
        }
        let mut req = self
            .client
            .get(&self.url)
            .header("Range", format!("bytes={start}-{end}"));
        if let Some(tok) = self.token.as_deref().filter(|t| !t.trim().is_empty()) {
            req = req.bearer_auth(tok);
        }
        let resp = req
            .send()
            .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;
        if !resp.status().is_success() {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                format!("HTTP {}", resp.status().as_u16()),
            ));
        }
        let data = resp
            .bytes()
            .map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?
            .to_vec();

        let mut guard = self.chunks.lock().unwrap();
        if guard.len() > 64 {
            if let Some(&first_key) = guard.keys().next() {
                guard.remove(&first_key);
            }
        }
        guard.insert(chunk_index, data.clone());
        Ok(data)
    }
}

impl Read for HttpRangeSource {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        if buf.is_empty() {
            return Ok(0);
        }
        if let Some(full) = &self.full_body {
            let idx = self.pos as usize;
            if idx >= full.len() {
                return Ok(0);
            }
            let n = buf.len().min(full.len() - idx);
            buf[..n].copy_from_slice(&full[idx..idx + n]);
            self.pos += n as u64;
            return Ok(n);
        }
        if let Some(total) = self.total_len {
            if self.pos >= total {
                return Ok(0);
            }
        }
        let chunk_idx = self.pos / HTTP_CHUNK_BYTES;
        let offset = (self.pos % HTTP_CHUNK_BYTES) as usize;
        let chunk = self.fetch_chunk(chunk_idx)?;
        if offset >= chunk.len() {
            return Ok(0);
        }
        let n = buf.len().min(chunk.len() - offset);
        buf[..n].copy_from_slice(&chunk[offset..offset + n]);
        self.pos += n as u64;
        Ok(n)
    }
}

impl Seek for HttpRangeSource {
    fn seek(&mut self, style: SeekFrom) -> io::Result<u64> {
        let len = self.total_len.unwrap_or(0) as i64;
        let next = match style {
            SeekFrom::Start(p) => p as i64,
            SeekFrom::Current(delta) => self.pos as i64 + delta,
            SeekFrom::End(delta) => {
                if self.total_len.is_none() {
                    return Err(io::Error::new(
                        io::ErrorKind::Unsupported,
                        "unknown stream length",
                    ));
                }
                len + delta
            }
        };
        if next < 0 {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "negative seek position",
            ));
        }
        self.pos = next as u64;
        Ok(self.pos)
    }
}

impl MediaSource for HttpRangeSource {
    fn is_seekable(&self) -> bool {
        self.supports_range || self.full_body.is_some()
    }

    fn byte_len(&self) -> Option<u64> {
        self.total_len
    }
}

fn parse_content_range_total(headers: &reqwest::header::HeaderMap) -> Option<u64> {
    let raw = headers.get(reqwest::header::CONTENT_RANGE)?.to_str().ok()?;
    let (_, total_str) = raw.split_once('/')?;
    total_str.trim().parse::<u64>().ok()
}

pub struct DecodedStream {
    format: Box<dyn FormatReader>,
    decoder: Box<dyn Decoder>,
    track_id: u32,
    pub sample_rate: u32,
    pub channels: usize,
    pub bit_depth: u32,
    pub duration_sec: f64,
    sample_buf: Option<SampleBuffer<f32>>,
}

impl DecodedStream {
    pub fn open(path_or_url: &str, token: Option<&str>, format_hint: &str) -> Result<Self, String> {
        let mut hint = Hint::new();
        if !format_hint.trim().is_empty() {
            hint.with_extension(format_hint.trim());
        }

        let mss = if path_or_url.starts_with("http://") || path_or_url.starts_with("https://") {
            let source = HttpRangeSource::open(path_or_url, token).map_err(|e| e.to_string())?;
            MediaSourceStream::new(Box::new(source), Default::default())
        } else {
            let local = path_or_url.strip_prefix("file://").unwrap_or(path_or_url);
            if let Some(ext) = Path::new(local).extension().and_then(|s| s.to_str()) {
                hint.with_extension(ext);
            }
            let file = File::open(local).map_err(|e| e.to_string())?;
            MediaSourceStream::new(Box::new(file), Default::default())
        };

        let probed = symphonia::default::get_probe()
            .format(
                &hint,
                mss,
                &FormatOptions {
                    enable_gapless: true,
                    ..Default::default()
                },
                &MetadataOptions::default(),
            )
            .map_err(|e| e.to_string())?;

        let format = probed.format;
        let track = format
            .tracks()
            .iter()
            .find(|t| t.codec_params.codec != CODEC_TYPE_NULL)
            .ok_or_else(|| String::from("no supported audio track found"))?;

        let track_id = track.id;
        let params = &track.codec_params;
        let sample_rate = params.sample_rate.unwrap_or(44_100);
        let channels = params.channels.map(|c| c.count()).unwrap_or(2).clamp(1, 2);
        let bit_depth = params.bits_per_sample.unwrap_or(16);
        let duration_sec = match (params.n_frames, params.time_base) {
            (Some(frames), Some(tb)) => {
                let t = tb.calc_time(frames);
                t.seconds as f64 + t.frac
            }
            (Some(frames), None) => frames as f64 / sample_rate.max(1) as f64,
            _ => 0.0,
        };

        let decoder = symphonia::default::get_codecs()
            .make(params, &DecoderOptions::default())
            .map_err(|e| e.to_string())?;

        Ok(Self {
            format,
            decoder,
            track_id,
            sample_rate,
            channels,
            bit_depth,
            duration_sec,
            sample_buf: None,
        })
    }

    pub fn seek_to_sec(&mut self, seconds: f64) -> Result<(), String> {
        let clamped = seconds.max(0.0);
        let whole = clamped.trunc() as u64;
        let frac = clamped.fract();
        self.format
            .seek(
                SeekMode::Accurate,
                SeekTo::Time {
                    time: Time::new(whole, frac),
                    track_id: Some(self.track_id),
                },
            )
            .map_err(|e| e.to_string())?;
        self.decoder.reset();
        Ok(())
    }

    pub fn next_pcm_packet(&mut self, out: &mut Vec<f32>) -> Result<bool, String> {
        out.clear();
        loop {
            let packet = match self.format.next_packet() {
                Ok(pkt) => pkt,
                Err(SymphoniaError::IoError(err))
                    if err.kind() == io::ErrorKind::UnexpectedEof =>
                {
                    return Ok(false);
                }
                Err(SymphoniaError::ResetRequired) => {
                    self.decoder.reset();
                    continue;
                }
                Err(err) => return Err(err.to_string()),
            };

            if packet.track_id() != self.track_id {
                continue;
            }

            match self.decoder.decode(&packet) {
                Ok(decoded) => {
                    let spec = *decoded.spec();
                    let capacity = decoded.capacity() as u64;
                    let buf = self
                        .sample_buf
                        .get_or_insert_with(|| SampleBuffer::new(capacity, spec));
                    if buf.capacity() < decoded.capacity() * spec.channels.count() {
                        *buf = SampleBuffer::new(capacity, spec);
                    }
                    buf.copy_interleaved_ref(decoded);
                    let raw = buf.samples();
                    let src_ch = spec.channels.count();
                    if src_ch == self.channels {
                        out.extend_from_slice(raw);
                    } else if src_ch == 1 && self.channels == 2 {
                        out.reserve(raw.len() * 2);
                        for &s in raw {
                            out.push(s);
                            out.push(s);
                        }
                    } else if src_ch >= 2 && self.channels == 1 {
                        out.reserve(raw.len() / src_ch);
                        for frame in raw.chunks_exact(src_ch) {
                            out.push((frame[0] + frame[1]) * 0.5);
                        }
                    } else {
                        out.extend_from_slice(raw);
                    }
                    if !out.is_empty() {
                        return Ok(true);
                    }
                }
                Err(SymphoniaError::DecodeError(_)) => continue,
                Err(SymphoniaError::IoError(err))
                    if err.kind() == io::ErrorKind::UnexpectedEof =>
                {
                    return Ok(false);
                }
                Err(err) => return Err(err.to_string()),
            }
        }
    }
}


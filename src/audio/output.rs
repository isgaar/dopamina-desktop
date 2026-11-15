// Location: src/audio/output.rs
// Function: Pure Rust dynamic PulseAudio/PipeWire audio output sink loading
// libpulse-simple.so.0 at runtime without C header build dependencies.
// All Rights Reserved isgaar

use std::ffi::{c_char, c_int, c_void, CString};

const RTLD_LAZY: c_int = 1;
const PA_SAMPLE_FLOAT32LE: c_int = 5;
const PA_STREAM_PLAYBACK: c_int = 1;

#[repr(C)]
struct PaSampleSpec {
    format: c_int,
    rate: u32,
    channels: u8,
}

#[repr(C)]
struct PaBufferAttr {
    maxlength: u32,
    tlength: u32,
    prebuf: u32,
    minreq: u32,
    fragsize: u32,
}

type FnPaSimpleNew = unsafe extern "C" fn(
    server: *const c_char,
    name: *const c_char,
    dir: c_int,
    dev: *const c_char,
    stream_name: *const c_char,
    ss: *const PaSampleSpec,
    map: *const c_void,
    attr: *const PaBufferAttr,
    error: *mut c_int,
) -> *mut c_void;

type FnPaSimpleWrite =
    unsafe extern "C" fn(s: *mut c_void, data: *const c_void, bytes: usize, error: *mut c_int) -> c_int;
type FnPaSimpleDrain = unsafe extern "C" fn(s: *mut c_void, error: *mut c_int) -> c_int;
type FnPaSimpleFlush = unsafe extern "C" fn(s: *mut c_void, error: *mut c_int) -> c_int;
type FnPaSimpleFree = unsafe extern "C" fn(s: *mut c_void);
type FnPaSimpleGetLatency = unsafe extern "C" fn(s: *mut c_void, error: *mut c_int) -> u64;

#[cfg(unix)]
extern "C" {
    fn dlopen(filename: *const c_char, flags: c_int) -> *mut c_void;
    fn dlsym(handle: *mut c_void, symbol: *const c_char) -> *mut c_void;
    fn dlclose(handle: *mut c_void) -> c_int;
}

struct PulseLib {
    handle: *mut c_void,
    pa_simple_new: FnPaSimpleNew,
    pa_simple_write: FnPaSimpleWrite,
    pa_simple_drain: FnPaSimpleDrain,
    pa_simple_flush: FnPaSimpleFlush,
    pa_simple_free: FnPaSimpleFree,
    pa_simple_get_latency: Option<FnPaSimpleGetLatency>,
}

unsafe impl Send for PulseLib {}

impl PulseLib {
    fn load() -> Option<Self> {
        #[cfg(unix)]
        unsafe {
            let candidates = [
                b"libpulse-simple.so.0\0".as_slice(),
                b"libpulse-simple.so\0".as_slice(),
            ];
            let mut handle = std::ptr::null_mut();
            for lib in candidates {
                handle = dlopen(lib.as_ptr() as *const c_char, RTLD_LAZY);
                if !handle.is_null() {
                    break;
                }
            }
            if handle.is_null() {
                return None;
            }
            let sym = |name: &[u8]| -> Option<*mut c_void> {
                let p = dlsym(handle, name.as_ptr() as *const c_char);
                if p.is_null() {
                    None
                } else {
                    Some(p)
                }
            };
            let pa_simple_new = std::mem::transmute(sym(b"pa_simple_new\0")?);
            let pa_simple_write = std::mem::transmute(sym(b"pa_simple_write\0")?);
            let pa_simple_drain = std::mem::transmute(sym(b"pa_simple_drain\0")?);
            let pa_simple_flush = std::mem::transmute(sym(b"pa_simple_flush\0")?);
            let pa_simple_free = std::mem::transmute(sym(b"pa_simple_free\0")?);
            let pa_simple_get_latency = sym(b"pa_simple_get_latency\0")
                .map(|p| std::mem::transmute::<*mut c_void, FnPaSimpleGetLatency>(p));
            Some(Self {
                handle,
                pa_simple_new,
                pa_simple_write,
                pa_simple_drain,
                pa_simple_flush,
                pa_simple_free,
                pa_simple_get_latency,
            })
        }
        #[cfg(not(unix))]
        {
            None
        }
    }
}

impl Drop for PulseLib {
    fn drop(&mut self) {
        #[cfg(unix)]
        unsafe {
            if !self.handle.is_null() {
                dlclose(self.handle);
            }
        }
    }
}

pub struct AudioSink {
    lib: Option<PulseLib>,
    stream: *mut c_void,
    pub sample_rate: u32,
    pub channels: u8,
    pub buffer_ms: u32,
}

unsafe impl Send for AudioSink {}

impl AudioSink {
    pub fn open(sample_rate: u32, channels: u8, buffer_ms: u32) -> Self {
        let lib = PulseLib::load();
        let mut sink = Self {
            lib,
            stream: std::ptr::null_mut(),
            sample_rate: sample_rate.max(8_000),
            channels: channels.clamp(1, 2),
            buffer_ms: buffer_ms.clamp(100, 3_000),
        };
        sink.reopen(sink.sample_rate, sink.channels, sink.buffer_ms);
        sink
    }

    pub fn reopen(&mut self, sample_rate: u32, channels: u8, buffer_ms: u32) -> bool {
        self.close_stream();
        self.sample_rate = sample_rate.max(8_000);
        self.channels = channels.clamp(1, 2);
        self.buffer_ms = buffer_ms.clamp(100, 3_000);

        let Some(lib) = &self.lib else {
            return false;
        };
        let app_name = CString::new("Dopamine").unwrap();
        let stream_name = CString::new("Hi-Fi Playback").unwrap();
        let ss = PaSampleSpec {
            format: PA_SAMPLE_FLOAT32LE,
            rate: self.sample_rate,
            channels: self.channels,
        };
        let bytes_per_sec = self.sample_rate as u64 * self.channels as u64 * 4;
        let tlength = ((bytes_per_sec * self.buffer_ms as u64) / 1000).clamp(4096, 1_048_576) as u32;
        let attr = PaBufferAttr {
            maxlength: tlength.saturating_mul(2),
            tlength,
            prebuf: u32::MAX,
            minreq: u32::MAX,
            fragsize: u32::MAX,
        };
        let mut err: c_int = 0;
        let stream = unsafe {
            (lib.pa_simple_new)(
                std::ptr::null(),
                app_name.as_ptr(),
                PA_STREAM_PLAYBACK,
                std::ptr::null(),
                stream_name.as_ptr(),
                &ss,
                std::ptr::null(),
                &attr,
                &mut err,
            )
        };
        if stream.is_null() {
            return false;
        }
        self.stream = stream;
        true
    }

    pub fn write_f32(&mut self, samples: &[f32]) -> bool {
        if samples.is_empty() {
            return true;
        }
        if self.stream.is_null() {
            let frames = samples.len() / self.channels.max(1) as usize;
            let us = (frames as u64 * 1_000_000) / self.sample_rate.max(8000) as u64;
            std::thread::sleep(std::time::Duration::from_micros(us.min(50_000)));
            return true;
        }
        let Some(lib) = &self.lib else {
            return false;
        };
        let mut err: c_int = 0;
        let bytes = std::mem::size_of_val(samples);
        let rc = unsafe {
            (lib.pa_simple_write)(
                self.stream,
                samples.as_ptr() as *const c_void,
                bytes,
                &mut err,
            )
        };
        rc >= 0
    }

    pub fn flush(&mut self) {
        if let (Some(lib), false) = (&self.lib, self.stream.is_null()) {
            let mut err: c_int = 0;
            unsafe {
                (lib.pa_simple_flush)(self.stream, &mut err);
            }
        }
    }

    pub fn drain(&mut self) {
        if let (Some(lib), false) = (&self.lib, self.stream.is_null()) {
            let mut err: c_int = 0;
            unsafe {
                (lib.pa_simple_drain)(self.stream, &mut err);
            }
        }
    }

    pub fn latency_sec(&self) -> f64 {
        if let (Some(lib), false) = (&self.lib, self.stream.is_null()) {
            if let Some(get_lat) = lib.pa_simple_get_latency {
                let mut err: c_int = 0;
                let us = unsafe { get_lat(self.stream, &mut err) };
                if err == 0 {
                    return us as f64 / 1_000_000.0;
                }
            }
        }
        0.0
    }

    fn close_stream(&mut self) {
        if let (Some(lib), false) = (&self.lib, self.stream.is_null()) {
            unsafe {
                (lib.pa_simple_free)(self.stream);
            }
            self.stream = std::ptr::null_mut();
        }
    }
}

impl Drop for AudioSink {
    fn drop(&mut self) {
        self.close_stream();
    }
}


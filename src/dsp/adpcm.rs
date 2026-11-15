// Location: dopamina-android/dopamine-dsp/src/adpcm.rs
// Function: Zero-allocation high-performance Microsoft & IMA ADPCM audio decoders.
// All Rights Reserved isgaar

pub const ADAPTATION_TABLE: [i32; 16] = [
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230,
];

#[inline(always)]
fn clamp_i16(val: i32) -> i16 {
    val.clamp(-32768, 32767) as i16
}

pub struct MsAdpcmDecoder;

impl MsAdpcmDecoder {
    pub fn decode_stereo_block(
        input: &[u8],
        coefs: &[(i16, i16)],
        output: &mut [i16],
    ) -> usize {
        if input.len() < 14 {
            return 0;
        }

        let pred_l = (input[0] as usize).min(coefs.len().saturating_sub(1));
        let pred_r = (input[1] as usize).min(coefs.len().saturating_sub(1));

        let mut delta_l = i16::from_le_bytes([input[2], input[3]]) as i32;
        let mut delta_r = i16::from_le_bytes([input[4], input[5]]) as i32;

        let mut samp1_l = i16::from_le_bytes([input[6], input[7]]) as i32;
        let mut samp1_r = i16::from_le_bytes([input[8], input[9]]) as i32;

        let mut samp2_l = i16::from_le_bytes([input[10], input[11]]) as i32;
        let mut samp2_r = i16::from_le_bytes([input[12], input[13]]) as i32;

        let (coef1_l, coef2_l) = (coefs[pred_l].0 as i32, coefs[pred_l].1 as i32);
        let (coef1_r, coef2_r) = (coefs[pred_r].0 as i32, coefs[pred_r].1 as i32);

        if output.len() < 4 {
            return 0;
        }

        output[0] = samp2_l as i16;
        output[1] = samp2_r as i16;
        output[2] = samp1_l as i16;
        output[3] = samp1_r as i16;

        let mut out_idx = 4;
        let in_len = input.len();

        for i in 14..in_len {
            if out_idx + 2 > output.len() {
                break;
            }
            let byte = input[i];

            // Left channel (high nibble)
            let nib_l_raw = ((byte >> 4) & 0x0F) as usize;
            let nib_l = if nib_l_raw >= 8 { (nib_l_raw as i32) - 16 } else { nib_l_raw as i32 };
            let pred_val_l = (samp1_l * coef1_l + samp2_l * coef2_l) >> 8;
            let new_samp_l = clamp_i16(pred_val_l + nib_l * delta_l);
            samp2_l = samp1_l;
            samp1_l = new_samp_l as i32;
            delta_l = (delta_l * ADAPTATION_TABLE[nib_l_raw]) >> 8;
            if delta_l < 16 { delta_l = 16; }

            // Right channel (low nibble)
            let nib_r_raw = (byte & 0x0F) as usize;
            let nib_r = if nib_r_raw >= 8 { (nib_r_raw as i32) - 16 } else { nib_r_raw as i32 };
            let pred_val_r = (samp1_r * coef1_r + samp2_r * coef2_r) >> 8;
            let new_samp_r = clamp_i16(pred_val_r + nib_r * delta_r);
            samp2_r = samp1_r;
            samp1_r = new_samp_r as i32;
            delta_r = (delta_r * ADAPTATION_TABLE[nib_r_raw]) >> 8;
            if delta_r < 16 { delta_r = 16; }

            output[out_idx] = new_samp_l;
            output[out_idx + 1] = new_samp_r;
            out_idx += 2;
        }

        out_idx
    }

    pub fn decode_mono_block(
        input: &[u8],
        coefs: &[(i16, i16)],
        output: &mut [i16],
    ) -> usize {
        if input.len() < 7 {
            return 0;
        }

        let pred = (input[0] as usize).min(coefs.len().saturating_sub(1));
        let mut delta = i16::from_le_bytes([input[1], input[2]]) as i32;
        let mut samp1 = i16::from_le_bytes([input[3], input[4]]) as i32;
        let mut samp2 = i16::from_le_bytes([input[5], input[6]]) as i32;

        let (coef1, coef2) = (coefs[pred].0 as i32, coefs[pred].1 as i32);

        if output.len() < 2 {
            return 0;
        }

        output[0] = samp2 as i16;
        output[1] = samp1 as i16;
        let mut out_idx = 2;

        for i in 7..input.len() {
            if out_idx + 2 > output.len() {
                break;
            }
            let byte = input[i];

            // Sample 1 (high nibble)
            let nib1_raw = ((byte >> 4) & 0x0F) as usize;
            let nib1 = if nib1_raw >= 8 { (nib1_raw as i32) - 16 } else { nib1_raw as i32 };
            let pred1 = (samp1 * coef1 + samp2 * coef2) >> 8;
            let s1 = clamp_i16(pred1 + nib1 * delta);
            samp2 = samp1;
            samp1 = s1 as i32;
            delta = (delta * ADAPTATION_TABLE[nib1_raw]) >> 8;
            if delta < 16 { delta = 16; }
            output[out_idx] = s1;
            out_idx += 1;

            // Sample 2 (low nibble)
            let nib2_raw = (byte & 0x0F) as usize;
            let nib2 = if nib2_raw >= 8 { (nib2_raw as i32) - 16 } else { nib2_raw as i32 };
            let pred2 = (samp1 * coef1 + samp2 * coef2) >> 8;
            let s2 = clamp_i16(pred2 + nib2 * delta);
            samp2 = samp1;
            samp1 = s2 as i32;
            delta = (delta * ADAPTATION_TABLE[nib2_raw]) >> 8;
            if delta < 16 { delta = 16; }
            output[out_idx] = s2;
            out_idx += 1;
        }

        out_idx
    }
}

import sys
import wave
import math
import struct

def generate_wav(input_bin, output_wav, filename, load_addr, exec_addr):
    with open(input_bin, 'rb') as f:
        data = f.read()

    file_size = len(data)
    sample_rate = 44100
    
    # MSX 1200 baud standard:
    # Bit 0: 1 cycle of 1200 Hz (833 us)
    # Bit 1: 2 cycles of 2400 Hz (833 us)
    
    def gen_tone(freq, cycles):
        n = int(cycles * sample_rate / freq)
        return [int(127 * math.sin(2 * math.pi * i * freq / sample_rate) + 128) for i in range(n)]

    bit0_samples = gen_tone(1200, 1)   # Bit 0: 1 cycle at 1200 Hz
    bit1_samples = gen_tone(2400, 2)   # Bit 1: 2 cycles at 2400 Hz
    
    def encode_byte(byte):
        samples = bytearray()
        # Start bit (0)
        samples.extend(bit0_samples)
        # 8 data bits (LSB first)
        for i in range(8):
            if byte & (1 << i):
                samples.extend(bit1_samples)
            else:
                samples.extend(bit0_samples)
        # Stop bit (1)
        samples.extend(bit1_samples)
        return samples
    
    wav_data = bytearray()
    
    SYNC = [0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74]
    end_addr = (load_addr + file_size - 1) & 0xFFFF
    
    # Leader: 3 seconds of 2400 Hz carrier (0xFF bytes)
    for _ in range(360):
        wav_data.extend(encode_byte(0xFF))
    
    # ---- Block 1: Header Block ----
    # Sync
    for b in SYNC:
        wav_data.extend(encode_byte(b))
    # Binary marker: 10 bytes of 0xD0
    for _ in range(10):
        wav_data.extend(encode_byte(0xD0))
    # Filename (6 bytes, space-padded)
    fname = filename.encode('ascii').ljust(6, b' ')[:6]
    for b in fname:
        wav_data.extend(encode_byte(b))
    
    # ---- Block 2: Data Block ----
    # Sync
    for b in SYNC:
        wav_data.extend(encode_byte(b))
    # Binary data block marker: 0xFE
    wav_data.extend(encode_byte(0xFE))
    # Start address
    for b in struct.pack('<H', load_addr):
        wav_data.extend(encode_byte(b))
    # End address
    for b in struct.pack('<H', end_addr):
        wav_data.extend(encode_byte(b))
    # Exec address
    for b in struct.pack('<H', exec_addr):
        wav_data.extend(encode_byte(b))
    # Data
    for byte in data:
        wav_data.extend(encode_byte(byte))
    
    # Gap at end
    gap_samples = [128] * sample_rate
    wav_data.extend(gap_samples)
    
    with wave.open(output_wav, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(1)
        wf.setframerate(sample_rate)
        wf.writeframes(bytes(wav_data))
        
    duration = len(wav_data) / sample_rate
    print(f"WAV file created: {output_wav}")
    print(f"Duration: {duration:.1f} seconds ({duration/60:.1f} minutes)")
    print(f"Load: 0x{load_addr:04X}, End: 0x{end_addr:04X}, Exec: 0x{exec_addr:04X}")

if __name__ == "__main__":
    if len(sys.argv) < 6:
        print("Usage: cas2wav.py <input.bin> <output.wav> <filename> <load_addr> <exec_addr>")
        sys.exit(1)
    generate_wav(sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4], 0), int(sys.argv[5], 0))

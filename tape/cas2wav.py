import sys
import wave
import math
import struct

def generate_wav(input_bin, output_wav, filename, load_addr, exec_addr):
    with open(input_bin, 'rb') as f:
        data = f.read()

    sample_rate = 44100
    
    # MSX 1200 baud standard:
    # Bit 0: 1 cycle of 1200 Hz (833 us)
    # Bit 1: 2 cycles of 2400 Hz (833 us)
    
    def gen_tone(freq, cycles):
        n = int(cycles * sample_rate / freq)
        return [int(127 * math.sin(2 * math.pi * i * freq / sample_rate) + 128) for i in range(n)]

    bit0_samples = gen_tone(1200, 1)   # Bit 0: 1 cycle at 1200 Hz
    bit1_samples = gen_tone(2400, 2)   # Bit 1: 2 cycles at 2400 Hz
    
    # Leader: 3 seconds of 1200 Hz tone
    leader_samples = gen_tone(1200, 3600)
    
    # Gap: 1 second of silence
    gap_samples = [128] * sample_rate
    
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
    
    # Add leader
    wav_data.extend(leader_samples)
    
    # Create MSX tape header (128 bytes)
    header = bytearray(128)
    header[0:3] = b'CAS'
    header[3] = 0x00
    header[8:8+len(filename)] = filename.encode('ascii').ljust(16, b' ')
    header[24:28] = struct.pack('<I', 0)  # File type: 0 = Binary
    header[28:32] = struct.pack('<I', load_addr)
    header[32:36] = struct.pack('<I', len(data))
    header[36:40] = struct.pack('<I', exec_addr)
    
    # Encode header
    for byte in header:
        wav_data.extend(encode_byte(byte))
    
    # Encode data
    for byte in data:
        wav_data.extend(encode_byte(byte))
    
    # Add gap at end
    wav_data.extend(gap_samples)
    
    with wave.open(output_wav, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(1)
        wf.setframerate(sample_rate)
        wf.writeframes(bytes(wav_data))
        
    duration = len(wav_data) / sample_rate
    print(f"WAV file created: {output_wav}")
    print(f"Duration: {duration:.1f} seconds ({duration/60:.1f} minutes)")

if __name__ == "__main__":
    if len(sys.argv) < 6:
        print("Usage: cas2wav.py <input.bin> <output.wav> <filename> <load_addr> <exec_addr>")
        sys.exit(1)
    generate_wav(sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4], 0), int(sys.argv[5], 0))

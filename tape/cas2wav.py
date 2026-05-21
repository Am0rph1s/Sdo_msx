import sys
import wave
import struct
import math

def generate_wav(input_cas, output_wav):
    with open(input_cas, 'rb') as f:
        data = f.read()

    sample_rate = 44100
    
    # MSX Cassette Encoding (1200 baud)
    # 1 bit duration = 1/1200 sec = 0.833ms = 36.75 samples at 44100 Hz
    # Bit 0: 2 cycles of 2400 Hz (36.75 samples)
    # Bit 1: 1 cycle of 1200 Hz (36.75 samples)
    
    def gen_tone(freq, cycles):
        n = int(cycles * sample_rate / freq)
        return [int(127 * math.sin(2 * math.pi * i * freq / sample_rate) + 128) for i in range(n)]

    bit0_samples = gen_tone(2400, 2)  # 1200 baud '0'
    bit1_samples = gen_tone(1200, 1)  # 1200 baud '1'
    
    # Leader tone: 2 seconds of 1200 Hz
    leader_samples = gen_tone(1200, 2400)
    
    # Gap: 1 second of silence
    gap_samples = [128] * sample_rate
    
    wav_data = bytearray()
    
    # Add leader
    wav_data.extend(leader_samples)
    
    # Encode data bytes
    for byte in data:
        # Start bit (0)
        wav_data.extend(bit0_samples)
        # 8 data bits (LSB first)
        for i in range(8):
            if byte & (1 << i):
                wav_data.extend(bit1_samples)
            else:
                wav_data.extend(bit0_samples)
        # Stop bit (1)
        wav_data.extend(bit1_samples)
    
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
    if len(sys.argv) < 3:
        print("Usage: cas2wav.py <input.cas> <output.wav>")
        sys.exit(1)
    generate_wav(sys.argv[1], sys.argv[2])

import sys
import struct

def create_cas(input_bin, output_cas, filename, load_addr, exec_addr):
    with open(input_bin, 'rb') as f:
        data = f.read()

    file_size = len(data)

    # MSX Standard CAS Header (128 bytes)
    header = bytearray(128)
    header[0:3] = b'CAS'
    header[3] = 0x00
    # 4-7: Reserved (0x00)
    header[8:8+len(filename)] = filename.encode('ascii').ljust(16, b' ')
    header[24:28] = struct.pack('<I', 0) # File type: 0 = Binary
    header[28:32] = struct.pack('<I', load_addr)
    header[32:36] = struct.pack('<I', file_size)
    header[36:40] = struct.pack('<I', exec_addr)
    # 40-127: Padding (0x00)

    with open(output_cas, 'wb') as f:
        f.write(header)
        f.write(data)
    print(f"CAS file created: {output_cas} ({file_size} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: bin2cas.py <input.bin> <output.cas> <filename> <load_addr> [exec_addr]")
        sys.exit(1)
        
    inp = sys.argv[1]
    out = sys.argv[2]
    fname = sys.argv[3]
    load = int(sys.argv[4], 0)
    exec_addr = int(sys.argv[5], 0) if len(sys.argv) > 5 else load
    
    create_cas(inp, out, fname, load, exec_addr)

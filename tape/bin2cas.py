import sys
import struct

def create_cas(input_bin, output_cas, filename, load_addr, exec_addr):
    with open(input_bin, 'rb') as f:
        data = f.read()

    file_size = len(data)
    
    # MSX CAS Header sequence
    HEADER_SEQ = bytes([0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74])
    
    # Create the complete CAS file
    cas_data = bytearray()
    
    # Add header sequence
    cas_data.extend(HEADER_SEQ)
    
    # File type: 10 bytes of 0xD0 for Binary files
    cas_data.extend(b'\xD0' * 10)
    
    # Filename (6 bytes, padded with spaces)
    fname_bytes = filename.encode('ascii').ljust(6, b' ')[:6]
    cas_data.extend(fname_bytes)
    
    # Starting address (2 bytes, little-endian)
    cas_data.extend(struct.pack('<H', load_addr))
    
    # Ending address (2 bytes, little-endian)
    # Note: If file size causes wrap around 64K, we just use the wrapped value
    end_addr = (load_addr + file_size - 1) & 0xFFFF
    cas_data.extend(struct.pack('<H', end_addr))
    
    # Execution address (2 bytes, little-endian)
    cas_data.extend(struct.pack('<H', exec_addr))
    
    # File data
    cas_data.extend(data)
    
    # Ensure total length is multiple of 8 bytes (pad with 0x00 if needed)
    padding = (8 - (len(cas_data) % 8)) % 8
    if padding > 0:
        cas_data.extend(b'\x00' * padding)
    
    with open(output_cas, 'wb') as f:
        f.write(cas_data)
        
    print(f"CAS file created: {output_cas}")
    print(f"File size: {len(cas_data)} bytes")
    print(f"Load address: 0x{load_addr:04X}")
    print(f"End address: 0x{end_addr:04X}")
    print(f"Exec address: 0x{exec_addr:04X}")

if __name__ == "__main__":
    if len(sys.argv) < 6:
        print("Usage: bin2cas.py <input.bin> <output.cas> <filename> <load_addr> <exec_addr>")
        sys.exit(1)
        
    inp = sys.argv[1]
    out = sys.argv[2]
    fname = sys.argv[3]
    load = int(sys.argv[4], 0)
    exec_addr = int(sys.argv[5], 0)
    
    create_cas(inp, out, fname, load, exec_addr)

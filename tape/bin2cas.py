import sys
import struct

def pad8(data):
    """Pad data to multiple of 8 bytes with 0x00"""
    padding = (8 - (len(data) % 8)) % 8
    if padding:
        data.extend(b'\x00' * padding)
    return data

def create_cas(input_bin, output_cas, filename, load_addr, exec_addr):
    with open(input_bin, 'rb') as f:
        data = f.read()

    file_size = len(data)
    end_addr = (load_addr + file_size - 1) & 0xFFFF
    
    SYNC = bytes([0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74])
    
    cas_data = bytearray()
    
    # Leader: ~3 seconds of 2400 Hz carrier (0xFF bytes)
    # 1200 baud = 120 bytes/sec, x 3 sec = 360 bytes
    cas_data.extend(b'\xFF' * 360)
    
    # ---- Block 1: Header Block (binary file marker + filename) ----
    # Sync must be at position divisible by 8
    cas_data = pad8(cas_data)
    cas_data.extend(SYNC)       # 8 bytes sync
    cas_data.extend(b'\xD0' * 10)  # 10 bytes binary marker
    fname = filename.encode('ascii').ljust(6, b' ')[:6]
    cas_data.extend(fname)      # 6 bytes filename
    
    # ---- Block 2: Data Block (addresses + binary data) ----
    # NOTE: NO sync before data block! Binary data immediately follows filename.
    cas_data.extend(b'\xFE')    # 1 byte binary data block marker
    cas_data.extend(struct.pack('<H', load_addr))  # start address
    cas_data.extend(struct.pack('<H', end_addr))   # end address
    cas_data.extend(struct.pack('<H', exec_addr))  # exec address
    cas_data.extend(data)       # binary data
    
    # Final padding to 8-byte boundary
    cas_data = pad8(cas_data)
    
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

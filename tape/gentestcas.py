import sys
import struct

def create_test_cas(output_cas, load_addr=0xC000, exec_addr=0xC000):
    # Tiny program that sets border to red and hangs
    code = bytes([
        0x3E, 0x07,       # ld a, 7
        0xD3, 0x99,       # out (0x99), a
        0x3E, 0x04,       # ld a, 4 (red)
        0xD3, 0x99,       # out (0x99), a
        0x18, 0xFE,       # jr $ (infinite loop)
    ])
    
    HEADER_SEQ = bytes([0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74])
    
    cas_data = bytearray()
    cas_data.extend(HEADER_SEQ)                           # 8 sync bytes
    cas_data.extend(b'\xD0' * 10)                         # 10 filler bytes
    cas_data.extend(b'TEST  ')                             # 6-byte filename
    cas_data.extend(struct.pack('<H', load_addr))          # start
    end_addr = (load_addr + len(code) - 1) & 0xFFFF
    cas_data.extend(struct.pack('<H', end_addr))           # end
    cas_data.extend(struct.pack('<H', exec_addr))          # exec
    cas_data.extend(code)                                  # data
    
    padding = (8 - (len(cas_data) % 8)) % 8
    if padding:
        cas_data.extend(b'\x00' * padding)
    
    with open(output_cas, 'wb') as f:
        f.write(cas_data)
    
    print(f"Created test CAS: {output_cas}")
    print(f"Size: {len(cas_data)} bytes")
    print(f"Load: 0x{load_addr:04X}, End: 0x{end_addr:04X}, Exec: 0x{exec_addr:04X}")
    print(f"Code size: {len(code)} bytes")

if __name__ == "__main__":
    create_test_cas(sys.argv[1] if len(sys.argv) > 1 else "test.cas")

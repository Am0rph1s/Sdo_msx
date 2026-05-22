# Mini test with TWO syncs (correct MSX binary CAS format)
import struct

code = bytes([
    0xF3,             # di
    0x31, 0x00, 0xD0, # ld sp, 0xD000
    0x3E, 0x07,       # ld a, 7
    0xD3, 0x99,       # out (0x99), a
    0x3E, 0x04,       # ld a, 4 (red)
    0xD3, 0x99,       # out (0x99), a
    0x18, 0xFE,       # jr $
])

SYNC = bytes([0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74])

def pad8(data):
    p = (8 - (len(data) % 8)) % 8
    if p: data.extend(b'\x00' * p)
    return data

cas = bytearray()
cas.extend(b'\xFF' * 360)       # leader
cas = pad8(cas)
cas.extend(SYNC)                 # sync 1 (header)
cas.extend(b'\xD0' * 10)         # binary marker
cas.extend(b'MINI  ')            # filename (6 bytes)
cas = pad8(cas)
cas.extend(SYNC)                 # sync 2 (data block)
cas.extend(b'\xFE')              # binary data marker
cas.extend(struct.pack('<H', 0x4000))  # load
cas.extend(struct.pack('<H', 0x400D))  # end
cas.extend(struct.pack('<H', 0x4000))  # exec
cas.extend(code)                 # data
cas = pad8(cas)

with open('C:\\msxgl\\projects\\nau_dx\\tape\\mini4000.cas', 'wb') as f:
    f.write(cas)

print(f'Mini test (2 syncs): {len(cas)} bytes')
with open('C:\\msxgl\\projects\\nau_dx\\tape\\mini4000.cas', 'rb') as f:
    d = f.read()
print(f'Sync1 at {d.find(SYNC)}')
print(f'Sync2 at {d.find(SYNC, d.find(SYNC)+1)}')
print(f'After sync2: 0x{d[d.find(SYNC, d.find(SYNC)+1)+8]:02X} (expected FE)')

# Create minimal test: tiny program at 0x8000 that changes border to red and loops
import sys
import struct

code = bytes([
    0x3E, 0x07,       # ld a, 7
    0xD3, 0x99,       # out (0x99), a
    0x3E, 0x04,       # ld a, 4 (red)
    0xD3, 0x99,       # out (0x99), a
    0x18, 0xFE,       # jr $
])

SYNC = bytes([0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74])

cas = bytearray()
cas.extend(b'\xFF' * 360)      # leader
cas.extend(SYNC)                # header block sync
cas.extend(b'\xD0' * 10)        # binary marker
cas.extend(b'TEST  ')           # filename

# pad to 8
while len(cas) % 8 != 0:
    cas.extend(b'\x00')

cas.extend(SYNC)                # data block sync
cas.extend(b'\xFE')             # binary data marker
load = 0x8000
end = (load + len(code) - 1) & 0xFFFF
exec_addr = 0x8000
cas.extend(struct.pack('<H', load))
cas.extend(struct.pack('<H', end))
cas.extend(struct.pack('<H', exec_addr))
cas.extend(code)

while len(cas) % 8 != 0:
    cas.extend(b'\x00')

with open('C:\\msxgl\\projects\\nau_dx\\tape\\tiny_test.cas', 'wb') as f:
    f.write(cas)

print(f'Tiny test CAS: {len(cas)} bytes, load=0x{load:04X}, end=0x{end:04X}, exec=0x{exec_addr:04X}')

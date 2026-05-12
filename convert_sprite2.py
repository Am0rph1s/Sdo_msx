#!/usr/bin/env python3
"""Convert ASCII sprite patterns to MSX TMS9918 format"""

# Pattern BLANC (W+B)
white_ascii = [
    ".....11.....",  # row 0
    "....1111....",  # row 1
    "....1111....",  # row 2
    "....1..1....",  # row 3
    "............",  # row 4
    "............",  # row 5
    "..1.1111.1..",  # row 6
    "....1111....",  # row 7
    ".1..1111..1.",  # row 8
    ".1111111111.",  # row 9
    "1..111111..1",  # row 10
    "1..11..11..1",  # row 11
    "...1....1...",  # row 12
    "1..1....1..1",  # row 13
    "1..........1",  # row 14
    "............",  # row 15
]

# Pattern VERMELL (R)
red_ascii = [
    "............",  # row 0
    "............",  # row 1
    "............",  # row 2
    "............",  # row 3
    "....1111....",  # row 4
    "...111111...",  # row 5
    "...1....1...",  # row 6
    "..11....11..",  # row 7
    "..11....11..",  # row 8
    ".1........1.",  # row 9
    ".11......11.",  # row 10
    "11........11",  # row 11
    "111......111",  # row 12
    ".11......11.",  # row 13
    "..11....11..",  # row 14
    "..11....11..",  # row 15
]

def row_to_bytes(row):
    """Convert row to 2 bytes: left (cols 0-7) + right (cols 8-15)"""
    # Pad to 16 chars if needed
    row = row.ljust(16, '.')
    
    left = 0
    right = 0
    
    # Left byte: cols 0-7, bit7=col0, bit0=col7
    for i in range(8):
        if row[i] == '1':
            left |= (1 << (7 - i))
    
    # Right byte: cols 8-15, bit7=col8, bit0=col15
    for i in range(8, 16):
        if row[i] == '1':
            right |= (1 << (15 - i))
    
    return left, right

print("White sprite (BLANC):")
white_data = []
for row in white_ascii:
    left, right = row_to_bytes(row)
    white_data.extend([left, right])
    print(f"  0x{left:02X}, 0x{right:02X},")

print(f"\nstatic const u8 g_SpriteWhite[32] = {{")
for i in range(0, 32, 2):
    print(f"    0x{white_data[i]:02X}, 0x{white_data[i+1]:02X},")
print("};")

print("\n" + "="*50)
print("\nRed sprite (VERMELL):")
red_data = []
for row in red_ascii:
    left, right = row_to_bytes(row)
    red_data.extend([left, right])
    print(f"  0x{left:02X}, 0x{right:02X},")

print(f"\nstatic const u8 g_SpriteRed[32] = {{")
for i in range(0, 32, 2):
    print(f"    0x{red_data[i]:02X}, 0x{red_data[i+1]:02X},")
print("};")

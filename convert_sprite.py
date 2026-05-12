#!/usr/bin/env python3
"""Convert CPC ship sprite to MSX TMS9918 format"""

# CPC sprite definition - use rows 1-16 (16 rows for MSX 16x16 sprite)
ship_rows = [
    ".....WW.....",  # row 1 -> MSX row 0
    "....WWWW....",  # row 2 -> MSX row 1
    "....WWWW....",  # row 3 -> MSX row 2
    "....W..W....",  # row 4 -> MSX row 3
    "....BBBB....",  # row 5 -> MSX row 4
    "....BBBB....",  # row 6 -> MSX row 5
    "...R.BB.R...",  # row 7 -> MSX row 6
    "...RRBBRR...",  # row 8 -> MSX row 7
    "..WRWWWWRW..",  # row 9 -> MSX row 8
    "..RRWWWWRR..",  # row 10 -> MSX row 9
    ".WRRWWWWRRW.",  # row 11 -> MSX row 10
    ".RWWWWWWWWR.",  # row 12 -> MSX row 11
    "WRRWWWWWWRRW",  # row 13 -> MSX row 12
    "WRRWW..WWRRW",  # row 14 -> MSX row 13
    "RRRW....WRRR",  # row 15 -> MSX row 14
    "WRRW....WRRW",  # row 16 -> MSX row 15
]

# Colors
W = 1  # white pixel (in white sprite)
R = 0  # red pixel (not in white sprite)
B = 0  # blue pixel (not in white sprite)
T = 0  # transparent

def row_to_bytes(row, is_white_sprite):
    """Convert a row to 2 bytes for MSX TMS9918 (bit7=left, bit0=right)"""
    left = 0
    right = 0
    
    for i, c in enumerate(row):
        if c == '.':
            continue  # transparent
        
        if is_white_sprite:
            # White sprite: W=white, B=blue (both visible)
            if c == 'W' or c == 'B':
                if i < 6:  # left 6 pixels -> cols 0-5
                    left |= (1 << (5 - i))
                else:  # right 6 pixels -> cols 6-11
                    right |= (1 << (11 - i))
        else:
            # Red sprite: only R=red
            if c == 'R':
                if i < 6:
                    left |= (1 << (5 - i))
                else:
                    right |= (1 << (11 - i))
    
    return left, right

print("White sprite (W+B):")
white_data = []
for row in ship_rows:
    left, right = row_to_bytes(row, is_white_sprite=True)
    white_data.extend([left, right])
    print(f"  0x{left:02X}, 0x{right:02X},")

print(f"\nstatic const u8 g_SpriteWhite[32] = {{")
for i in range(0, 32, 2):
    print(f"    0x{white_data[i]:02X}, 0x{white_data[i+1]:02X},")
print("};")

print("\n" + "="*50)
print("\nRed sprite (R only):")
red_data = []
for row in ship_rows:
    left, right = row_to_bytes(row, is_white_sprite=False)
    red_data.extend([left, right])
    print(f"  0x{left:02X}, 0x{right:02X},")

print(f"\nstatic const u8 g_SpriteRed[32] = {{")
for i in range(0, 32, 2):
    print(f"    0x{red_data[i]:02X}, 0x{red_data[i+1]:02X},")
print("};")

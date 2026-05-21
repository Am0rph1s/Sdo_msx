import sys
import struct

def create_cas_two_blocks(loader_bin, data_bin, output_cas, loader_name, data_name, loader_load, loader_exec, data_load):
    with open(loader_bin, 'rb') as f:
        loader_data = f.read()
    with open(data_bin, 'rb') as f:
        data_data = f.read()

    HEADER_SEQ = bytes([0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74])
    
    def make_block(data, name, load_addr, exec_addr):
        block = bytearray()
        block.extend(HEADER_SEQ)
        block.extend(b'\xD0' * 10)
        block.extend(name.encode('ascii').ljust(6, b' ')[:6])
        block.extend(struct.pack('<H', load_addr))
        end_addr = (load_addr + len(data) - 1) & 0xFFFF
        block.extend(struct.pack('<H', end_addr))
        block.extend(struct.pack('<H', exec_addr))
        block.extend(data)
        padding = (8 - (len(block) % 8)) % 8
        if padding > 0:
            block.extend(b'\x00' * padding)
        return block

    cas_data = bytearray()
    cas_data.extend(make_block(loader_data, loader_name, loader_load, loader_exec))
    cas_data.extend(make_block(data_data, data_name, data_load, data_load))

    with open(output_cas, 'wb') as f:
        f.write(cas_data)
        
    print(f"CAS file created: {output_cas}")
    print(f"Loader: {len(loader_data)} bytes at 0x{loader_load:04X}")
    print(f"Data: {len(data_data)} bytes at 0x{data_load:04X}")

if __name__ == "__main__":
    if len(sys.argv) < 8:
        print("Usage: bin2cas.py <loader.bin> <data.bin> <output.cas> <loader_name> <data_name> <loader_load> <loader_exec> <data_load>")
        sys.exit(1)
        
    create_cas_two_blocks(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], 
                          int(sys.argv[6], 0), int(sys.argv[7], 0), int(sys.argv[8], 0))

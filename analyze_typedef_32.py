import struct
import sys

def read_string(f, strings_base, idx):
    if idx == 0xFFFFFFFF: return "NULL"
    f.seek(strings_base + idx)
    chars = []
    while True:
        b = f.read(1)
        if not b or b == b'\0': break
        chars.append(b.decode('utf-8', errors='ignore'))
    return "".join(chars)

def analyze(path):
    strings_base = 0xBCFE15
    typedef_base = 0x825CE8
    
    with open(path, 'rb') as f:
        for i in range(10):
            offset = typedef_base + (i * 32)
            f.seek(offset)
            data = struct.unpack('<IIIIIIII', f.read(32))
            name = read_string(f, strings_base, data[0])
            print(f"Type {i} @ 0x{offset:X}:")
            print(f"    NameIdx: {data[0]} ('{name}')")
            print(f"    Raw: {[hex(x) for x in data]}")

if __name__ == "__main__":
    analyze(sys.argv[1])

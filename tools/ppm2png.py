#!/usr/bin/env python
# Convert QEMU screendump PPM (P6) to a viewable PNG.
import struct, zlib, sys

def ppm_to_png(src, dst):
    with open(src, 'rb') as f:
        data = f.read()
    assert data[:2] == b'P6', "not a P6 PPM"
    header_end = data.find(b'\n255\n')
    assert header_end > 0
    dims = data[2:header_end].split()
    w, h = int(dims[0]), int(dims[1])
    pos = header_end + 5
    pixels = data[pos:pos + w * h * 3]

    def chunk(tag, payload):
        c = struct.pack('>I', len(payload)) + tag + payload
        return c + struct.pack('>I', zlib.crc32(tag + payload) & 0xffffffff)

    ihdr = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
    raw = b''.join(b'\x00' + pixels[y*w*3:(y+1)*w*3] for y in range(h))
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) +
           chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
    with open(dst, 'wb') as f:
        f.write(png)
    print(f"{src}: {w}x{h} PPM -> {dst}: {len(png)} bytes PNG")

if __name__ == '__main__':
    ppm_to_png(sys.argv[1], sys.argv[2])

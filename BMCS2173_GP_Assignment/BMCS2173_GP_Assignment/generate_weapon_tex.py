import struct
import random
import math

def save_bmp(filename, width, height, pixels):
    row_size = (width * 3 + 3) & ~3
    image_size = row_size * height
    file_size = 54 + image_size
    with open(filename, "wb") as f:
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(b'\x00\x00\x00\x00')
        f.write(struct.pack('<I', 54))
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<i', width))
        f.write(struct.pack('<i', height))
        f.write(struct.pack('<H', 1)) 
        f.write(struct.pack('<H', 24))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', image_size))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', 0))
        
        idx = 0
        for y in range(height):
            for x in range(width):
                r, g, b = pixels[idx]
                idx += 1
                f.write(struct.pack('<BBB', b, g, r))
            padding = row_size - (width * 3)
            f.write(b'\x00' * padding)
    print(f"Generated {filename}")

def generate_metal_texture(size=256):
    pixels = []
    for y in range(size):
        for x in range(size):
            # Very simple plain metal
            val = 150 + random.randint(-2, 2)
            pixels.append((val, val, val))
    save_bmp("metal.bmp", size, size, pixels)

def generate_wood_texture(size=256):
    pixels = []
    for y in range(size):
        for x in range(size):
            # Very simple plain wood
            v = random.randint(-2, 2)
            pixels.append((139 + v, 69 + v, 19 + v))
    save_bmp("wood.bmp", size, size, pixels)

if __name__ == "__main__":
    generate_metal_texture()
    generate_wood_texture()

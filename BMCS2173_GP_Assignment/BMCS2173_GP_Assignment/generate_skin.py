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
                r = max(0, min(255, int(r)))
                g = max(0, min(255, int(g)))
                b = max(0, min(255, int(b)))
                f.write(struct.pack('<BBB', b, g, r))
            padding = row_size - (width * 3)
            f.write(b'\x00' * padding)
    print(f"Generated {filename}")

def generate_textures(size=256):
    # 1. Skin (Very soft, plain color)
    pixels_skin = []
    for y in range(size):
        for x in range(size):
            r, g, b = 255, 220, 190
            noise = random.randint(-1, 1)
            pixels_skin.append((r+noise, g+noise, b+noise))
    save_bmp("skin.bmp", size, size, pixels_skin)

    # 2. Fabric (Plain dark blue)
    pixels_fabric = []
    for y in range(size):
        for x in range(size):
            val = 45 + random.randint(-2, 2)
            pixels_fabric.append((val-15, val-10, val+35)) 
    save_bmp("fabric.bmp", size, size, pixels_fabric)

    # 3. Gold (Smooth metallic yellow, without harsh strokes)
    pixels_gold = []
    for y in range(size):
        for x in range(size):
            r, g, b = 220, 185, 60
            noise = random.randint(-1, 1)
            pixels_gold.append((r+noise, g+noise, b+noise/2))
    save_bmp("gold.bmp", size, size, pixels_gold)

    # 4. White Leather (Almost pure plain white)
    pixels_white = []
    for y in range(size):
        for x in range(size):
            v = 240 + random.randint(-2, 2)
            pixels_white.append((v, v, v))
    save_bmp("white_leather.bmp", size, size, pixels_white)

    # 5. Dark Leather / Rough Rubber (Plain Dark brown)
    pixels_dark = []
    for y in range(size):
        for x in range(size):
            noise = random.randint(-2, 2)
            pixels_dark.append((45+noise, 35+noise, 30+noise))
    save_bmp("dark_leather.bmp", size, size, pixels_dark)

if __name__ == "__main__":
    generate_textures()

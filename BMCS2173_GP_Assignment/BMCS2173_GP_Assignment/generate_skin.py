import struct
import random
import math

def generate_skin_bmp(filename="skin.bmp", size=256):
    width = size
    height = size
    # Base skin color BGR (Windows Bitmap format uses BGR)
    # R: 235, G: 185, B: 160
    base_r, base_g, base_b = 215, 165, 140
    
    row_size = (width * 3 + 3) & ~3 # 4-byte padding
    image_size = row_size * height
    file_size = 54 + image_size
    
    with open(filename, "wb") as f:
        # BMP Header
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(b'\x00\x00')
        f.write(b'\x00\x00')
        f.write(struct.pack('<I', 54))
        
        # DIB Header (BITMAPINFOHEADER)
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<i', width))
        f.write(struct.pack('<i', height))
        f.write(struct.pack('<H', 1))  # Color planes
        f.write(struct.pack('<H', 24)) # 24 bpp
        f.write(struct.pack('<I', 0))  # BI_RGB (No compression)
        f.write(struct.pack('<I', image_size))
        f.write(struct.pack('<i', 2835)) # Print resolution
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<I', 0)) # Palette colors
        f.write(struct.pack('<I', 0)) # Important colors
        
        # Pixel Data
        for y in range(height):
            for x in range(width):
                # Generate procedural noise
                nx = x / 20.0
                ny = y / 20.0
                noise = math.sin(nx)*math.cos(ny)*10 + random.randint(-5, 5)
                
                # Darker pore marks
                pore_factor = 0
                if random.random() < 0.05:
                    pore_factor = -15
                    
                r = max(0, min(255, int(base_r + noise + pore_factor)))
                g = max(0, min(255, int(base_g + noise + pore_factor)))
                b = max(0, min(255, int(base_b + noise + pore_factor)))
                
                # Write BGR format
                f.write(struct.pack('<BBB', b, g, r))
            
            # Write row padding
            padding = row_size - (width * 3)
            f.write(b'\x00' * padding)
    print(f"Generated {filename}")

if __name__ == "__main__":
    generate_skin_bmp()

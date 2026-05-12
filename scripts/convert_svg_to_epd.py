#!/usr/bin/env python3
"""
Convert SVG logo to e-ink display byte array format.
Generates C code compatible with the EPD display.
"""

from PIL import Image, ImageDraw
import cairosvg
import io
import sys

def svg_to_bitmap(svg_path, width, height):
    """Convert SVG to monochrome bitmap."""
    # Convert SVG to PNG in memory with white background
    png_data = cairosvg.svg2png(
        url=svg_path, 
        output_width=width, 
        output_height=height,
        background_color='white'
    )
    
    # Open as PIL Image
    img = Image.open(io.BytesIO(png_data))
    
    # Convert to RGB first (to handle transparency), then grayscale
    if img.mode == 'RGBA':
        # Create white background
        background = Image.new('RGB', img.size, (255, 255, 255))
        background.paste(img, mask=img.split()[3])  # Use alpha channel as mask
        img = background
    
    # Convert to grayscale
    img = img.convert('L')
    
    # Save debug image to check what we're getting
    img.save(f'debug_{width}x{height}.png')
    
    # Convert to pure black and white (1-bit)
    # Using threshold - anything darker than 128 becomes black (0), lighter becomes white (255)
    threshold = 128
    img = img.point(lambda x: 0 if x < threshold else 255, '1')
    
    return img

def bitmap_to_byte_array(img):
    """Convert bitmap image to byte array for e-ink display.
    
    Format for EPD_ShowPicture with Color=BLACK:
    - Each byte represents 8 horizontal pixels
    - Bit 7 (0x80, MSB) = leftmost pixel
    - When bit is 1: draws WHITE (because !BLACK = WHITE)
    - When bit is 0: draws BLACK (because BLACK = BLACK)
    So: 1 = white pixel, 0 = black pixel
    """
    width, height = img.size
    pixels = img.load()
    
    # Calculate number of bytes needed per row (width must be multiple of 8)
    bytes_per_row = (width + 7) // 8
    
    byte_array = []
    
    for y in range(height):
        for x_byte in range(bytes_per_row):
            byte_val = 0
            for bit in range(8):
                x = x_byte * 8 + bit
                if x < width:
                    # Get pixel value (0 = black, 255 = white in source image)
                    pixel = pixels[x, y]
                    # For e-ink: bit=1 for white, bit=0 for black
                    if pixel > 128:  # white pixel in source
                        byte_val |= (1 << (7 - bit))
                # If x >= width, leave bit as 0 (black)
            byte_array.append(byte_val)
    
    return byte_array

def generate_c_array(byte_array, array_name, width, height):
    """Generate C code for the byte array."""
    code = f"// Spotify logo - {width}x{height} pixels\n"
    code += f"// Total bytes: {len(byte_array)}\n"
    code += f"const unsigned char {array_name}[{len(byte_array)}] = {{\n"
    
    # Format bytes in rows of 16 for readability
    for i in range(0, len(byte_array), 16):
        chunk = byte_array[i:i+16]
        hex_values = ','.join(f'0x{b:02X}' for b in chunk)
        code += f"    {hex_values}"
        if i + 16 < len(byte_array):
            code += ","
        code += "\n"
    
    code += "};\n"
    return code

def main():
    # Configuration
    svg_file = "spotify_logo.svg"
    
    # Common sizes for the logo
    sizes = [
        (50, 50, "spotify_logo_50x50"),
        (80, 80, "spotify_logo_80x80"),
        (100, 100, "spotify_logo_100x100"),
    ]
    
    output_file = "include/spotify_logo.h"
    
    print(f"Converting {svg_file} to e-ink display format...")
    
    with open(output_file, 'w') as f:
        f.write("#ifndef SPOTIFY_LOGO_H\n")
        f.write("#define SPOTIFY_LOGO_H\n\n")
        f.write("// Spotify logo bitmaps for e-ink display\n")
        f.write("// Generated from SVG file\n")
        f.write("// Format: Monochrome bitmap, 1 bit per pixel\n")
        f.write("//         Each byte = 8 horizontal pixels (MSB = leftmost)\n")
        f.write("//         0 = black, 1 = white\n\n")
        
        for width, height, array_name in sizes:
            print(f"  Generating {width}x{height}...")
            
            # Convert SVG to bitmap
            img = svg_to_bitmap(svg_file, width, height)
            
            # Convert to byte array
            byte_array = bitmap_to_byte_array(img)
            
            # Generate C code
            c_code = generate_c_array(byte_array, array_name, width, height)
            f.write(c_code)
            f.write("\n")
            
            print(f"    Generated {len(byte_array)} bytes")
        
        f.write("#endif // SPOTIFY_LOGO_H\n")
    
    print(f"\nDone! Generated {output_file}")
    print("\nUsage in your code:")
    print('  #include "spotify_logo.h"')
    print('  EPD_Display_Part(x, y, 50, 50, spotify_logo_50x50);')

if __name__ == "__main__":
    main()

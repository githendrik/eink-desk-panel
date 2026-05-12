#!/usr/bin/env python3
"""
Render the EPD mock display to a PNG image.
Uses Pillow for image generation.
"""

import json
import sys
from PIL import Image, ImageDraw, ImageFont

EPD_W = 400
EPD_H = 300

def load_font(size):
    """Load a font, falling back to default if needed."""
    try:
        return ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", size)
    except:
        try:
            return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size)
        except:
            return ImageFont.load_default()

def render_display(data, output_path):
    """Render the display data to a PNG image."""
    # Create white background
    img = Image.new('RGB', (EPD_W, EPD_H), 'white')
    draw = ImageDraw.Draw(img)
    
    # Draw picture placeholders (weather icons)
    for pic in data.get('pictureElements', []):
        x, y, w, h = pic['x'], pic['y'], pic['w'], pic['h']
        icon_index = pic.get('iconIndex', 0)
        
        # Draw rectangle placeholder
        draw.rectangle([x, y, x + w, y + h], outline='black', width=2)
        
        # Add icon label
        icons = ['Mist', 'Cloudy', 'Thunder', 'Clear', 'Snow', 'Rain']
        label = icons[icon_index] if icon_index < len(icons) else f'Icon {icon_index}'
        
        # Draw simple icon symbol
        center_x, center_y = x + w // 2, y + h // 2
        if icon_index == 0:  # Mist - horizontal lines
            for i in range(-2, 3):
                draw.line([(center_x - 30, center_y + i * 10), 
                          (center_x + 30, center_y + i * 10)], fill='black', width=2)
        elif icon_index == 1:  # Cloudy - cloud
            draw.ellipse([center_x - 30, center_y - 20, center_x + 30, center_y + 20], 
                        outline='black', width=2)
        elif icon_index == 2:  # Thunder - cloud with lightning
            draw.ellipse([center_x - 30, center_y - 20, center_x + 30, center_y + 20], 
                        outline='black', width=2)
            draw.line([(center_x, center_y - 10), (center_x - 10, center_y + 10), 
                      (center_x + 10, center_y + 10), (center_x, center_y + 30)], 
                     fill='black', width=2)
        elif icon_index == 3:  # Clear - sun
            draw.ellipse([center_x - 25, center_y - 25, center_x + 25, center_y + 25], 
                        outline='black', width=2)
            for i in range(8):
                angle = i * 45 * 3.14159 / 180
                sx = center_x + int(35 * (angle ** 0))
                sy = center_y + int(35 * (angle ** 0))
                ex = center_x + int(45 * (angle ** 0))
                ey = center_y + int(45 * (angle ** 0))
        elif icon_index == 4:  # Snow - snowflake
            for i in range(-30, 31, 20):
                draw.line([(center_x + i, center_y - 30), (center_x + i, center_y + 30)], 
                         fill='black', width=2)
                draw.line([(center_x - 30, center_y + i), (center_x + 30, center_y + i)], 
                         fill='black', width=2)
        elif icon_index == 5:  # Rain - cloud with drops
            draw.ellipse([center_x - 30, center_y - 20, center_x + 30, center_y + 20], 
                        outline='black', width=2)
            for i in range(-20, 21, 20):
                draw.line([(center_x + i, center_y + 25), (center_x + i, center_y + 40)], 
                         fill='black', width=2)
        
        # Draw label below icon
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 10)
        except:
            font = ImageFont.load_default()
        bbox = draw.textbbox((0, 0), label, font=font)
        text_w = bbox[2] - bbox[0]
        draw.text((x + (w - text_w) // 2, y + h + 5), label, fill='black', font=font)
    
    # Draw lines
    for line in data.get('lineElements', []):
        draw.line([(line['x1'], line['y1']), (line['x2'], line['y2'])], 
                 fill='black', width=1)
    
    # Draw rectangles
    for rect in data.get('rectElements', []):
        x, y, w, h = rect['x'], rect['y'], rect['w'], rect['h']
        if rect.get('filled', False):
            draw.rectangle([x, y, x + w, y + h], fill='black')
        else:
            draw.rectangle([x, y, x + w, y + h], outline='black', width=1)
    
    # Draw text
    for text_elem in data.get('textElements', []):
        x, y = text_elem['x'], text_elem['y']
        text = text_elem['text']
        font_size = text_elem['fontSize']
        
        font = load_font(font_size)
        draw.text((x, y), text, fill='black', font=font)
    
    # Save image
    img.save(output_path)
    print(f"Rendered to: {output_path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: render.py <data.json> [output.png]")
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else 'display_preview.png'
    
    with open(input_path, 'r') as f:
        data = json.load(f)
    
    render_display(data, output_path)

if __name__ == '__main__':
    main()

import os
from PIL import Image, ImageOps

def compile_8_graphs_to_grid(output_pdf_name="compiled_8_grid_report.pdf"):
    valid_extensions = ('.png', '.jpg', '.jpeg', '.bmp', '.tiff')
    
    image_files = sorted([
        f for f in os.listdir('.') 
        if f.lower().endswith(valid_extensions)
    ])
    
    if not image_files:
        print("No images found in the current folder!")
        return

    print(f"Found {len(image_files)} images. Compiling 8 per page (2x4 grid)...")

    # Standard A4 Canvas Size scaled up to 300 DPI for maximum sharpness
    page_width, page_height = 2480, 3508
    
    # Grid spacing configurations for 4 rows
    margin_x = 80
    margin_y = 90
    gap_x = 60
    gap_y = 50  # Tightened slightly to accommodate the 4th row comfortably
    
    # Calculate exact slot allocations for a 2x4 layout
    available_width = (page_width - (2 * margin_x) - gap_x) // 2
    available_height = (page_height - (2 * margin_y) - (3 * gap_y)) // 4

    pages = []
    current_page_images = []

    for img_path in image_files:
        try:
            with Image.open(img_path) as img:
                img_rgb = img.convert('RGB')
                
                # Resize keeping aspect ratio sharp using LANCZOS
                img_rgb.thumbnail((available_width, available_height), Image.Resampling.LANCZOS)
                
                # Elegant crisp border
                img_bordered = ImageOps.expand(img_rgb, border=3, fill='#E8E8E8')
                
                current_page_images.append(img_bordered)
                
            if len(current_page_images) == 8:
                pages.append(create_8_plot_page(current_page_images, page_width, page_height, margin_x, margin_y, gap_x, gap_y, available_width, available_height))
                current_page_images = [] 
                
        except Exception as e:
            print(f"Skipping image {img_path}: {e}")

    # Handle leftover images on the final page
    if current_page_images:
        pages.append(create_8_plot_page(current_page_images, page_width, page_height, margin_x, margin_y, gap_x, gap_y, available_width, available_height))

    # Save with explicit high DPI parameters
    if pages:
        pages[0].save(
            output_pdf_name, 
            save_all=True, 
            append_images=pages[1:],
            dpi=(300, 300)
        )
        print(f"Success! Created 8-pack grid document: '{output_pdf_name}'")
    else:
        print("No pages could be generated.")

def create_8_plot_page(images, width, height, mx, my, gx, gy, slot_w, slot_h):
    canvas = Image.new('RGB', (width, height), 'white')
    
    # Define the 8 coordinates for a 2x4 layout (Left column, Right column for 4 rows)
    positions = [
        # Row 1
        (mx, my), (mx + slot_w + gx, my),
        # Row 2
        (mx, my + slot_h + gy), (mx + slot_w + gx, my + slot_h + gy),
        # Row 3
        (mx, my + 2*(slot_h + gy)), (mx + slot_w + gx, my + 2*(slot_h + gy)),
        # Row 4
        (mx, my + 3*(slot_h + gy)), (mx + slot_w + gx, my + 3*(slot_h + gy))
    ]
    
    for img, pos in zip(images, positions):
        # Center image within its specific grid slot allocation
        x_offset = pos[0] + (slot_w - img.width) // 2
        y_offset = pos[1] + (slot_h - img.height) // 2
        canvas.paste(img, (x_offset, y_offset))
        
    return canvas

if __name__ == "__main__":
    compile_8_graphs_to_grid()
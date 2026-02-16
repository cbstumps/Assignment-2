import serial
import time
from PIL import Image
import os

# --- CONFIGURATION ---
PORT = 'COM4'  # <--- DOUBLE CHECK THIS matches Arduino IDE
BAUD_RATE = 115200
IMG_FOLDER = './images_2' 
WIDTH = 240 
HEIGHT = 135

def process_and_send(file_path, ser_connection):
    # Odef process_and_send(file_path, ser_connection):
    img = Image.open(file_path).resize((WIDTH, HEIGHT)).convert('RGB')
    print(f"Sending {file_path}...")
    ser_connection.write(b'START\n')
    
    for y in range(HEIGHT):
        # WAIT for M5 to say it's ready for this specific row
        while True:
            if ser_connection.in_waiting > 0:
                line = ser_connection.readline().decode().strip()
                if line == "NEXT_ROW":
                    break

        row_data = []
        for x in range(WIDTH):
            r, g, b = img.getpixel((x, y))
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            # Try this order first:
            row_data.append(rgb565 & 0xFF)        # Low Byte
            row_data.append((rgb565 >> 8) & 0xFF) # High Byte
            
        ser_connection.write(bytearray(row_data))
        ser_connection.flush() # Forces the laptop to empty the buffer

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    if not os.path.exists(IMG_FOLDER):
        print(f"Error: Folder '{IMG_FOLDER}' not found!")
    else:
        try:
            # Initialize Serial
            ser = serial.Serial(PORT, BAUD_RATE, timeout=2)
            time.sleep(2) # Wait for M5 to reboot after connecting

            # Get list of images
            files = [f for f in os.listdir(IMG_FOLDER) if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
            
            if not files:
                print("No images found in folder.")
            
            for filename in files:
                full_path = os.path.join(IMG_FOLDER, filename)
                process_and_send(full_path, ser)
                print("Waiting 3 seconds...")
                time.sleep(3) 
                
            ser.close()
            print("Done!")

        except Exception as e:
            print(f"Serial Error: {e}")
            print("Make sure the Arduino Serial Monitor is CLOSED.")
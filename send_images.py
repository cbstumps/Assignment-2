import serial
import time
from PIL import Image
import os

# --- CONFIGURATION ---
PORT = 'COM4'                  # Confirm this in Device Manager / Arduino IDE
BAUD_RATE = 115200
IMG_FOLDER = './images_2'      # ← your images folder
WIDTH = 240
HEIGHT = 135
TIMEOUT_SEC = 10               # How long to wait for each "NEXT_ROW" before giving up

def process_and_send(file_path, ser):
    try:
        img = Image.open(file_path).resize((WIDTH, HEIGHT)).convert('RGB')
        print(f"Preparing and sending {os.path.basename(file_path)}...")

        ser.write(b'START\n')
        ser.flush()
        print("Sent START command")

        for y in range(HEIGHT):
            print(f"  Waiting for NEXT_ROW (row {y+1}/{HEIGHT})...", end="", flush=True)

            start_wait = time.time()
            received = False

            while time.time() - start_wait < TIMEOUT_SEC:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    print(f" received: '{line}'")
                    if line == "NEXT_ROW":
                        received = True
                        break
                time.sleep(0.005)  # small sleep to avoid CPU spin

            if not received:
                print(f"\nTIMEOUT waiting for NEXT_ROW on row {y+1} after {TIMEOUT_SEC}s!")
                return False  # Stop sending this image

            # Send row data
            row_data = bytearray()
            for x in range(WIDTH):
                r, g, b = img.getpixel((x, y))
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                row_data.append((rgb565 >> 8) & 0xFF)   # high byte FIRST
                row_data.append(rgb565 & 0xFF)          # low byte second

            ser.write(row_data)
            ser.flush()
            print(f"  Sent row {y+1}")

        print(f"Finished sending {os.path.basename(file_path)}")
        return True

    except Exception as e:
        print(f"Error during send: {e}")
        return False

# --- MAIN ---
if __name__ == "__main__":
    if not os.path.exists(IMG_FOLDER):
        print(f"Folder not found: {IMG_FOLDER}")
        print("Current dir:", os.getcwd())
        exit(1)

    files = [f for f in os.listdir(IMG_FOLDER) if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
    if not files:
        print("No images in folder.")
        exit(1)

    try:
        ser = serial.Serial(PORT, BAUD_RATE, timeout=2)
        print(f"Opened {PORT}")
        time.sleep(3.5)  # Increased — give M5 time to boot after open

        # Optional: flush any garbage from reset
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        for filename in sorted(files):
            full_path = os.path.join(IMG_FOLDER, filename)
            success = process_and_send(full_path, ser)
            if not success:
                print("Aborting remaining images due to error.")
                break
            print("Pausing 3 seconds...\n")
            time.sleep(3)

        ser.close()
        print("Done!")

    except serial.SerialException as e:
        print(f"Cannot open serial port: {e}")
        print("→ Close Arduino Serial Monitor / other terminals")
        print("→ Check correct COM port")
    except Exception as e:
        print(f"Unexpected error: {e}")
import serial
import time
from PIL import Image
import os

# --- CONFIGURATION ---
PORT = 'COM4'
BAUD_RATE = 115200
IMG_FOLDER = './images_2'
WIDTH = 240
HEIGHT = 135
TIMEOUT_SEC = 10               # timeout per row
HOLD_WAIT_TIMEOUT = 120        # max seconds to wait for button hold (safety)

def process_and_send(file_path, ser):
    try:
        img = Image.open(file_path).resize((WIDTH, HEIGHT)).convert('RGB')
        filename = os.path.basename(file_path)
        print(f"Preparing and sending {filename}...")

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
                time.sleep(0.005)

            if not received:
                print(f"\nTIMEOUT waiting for NEXT_ROW on row {y+1}!")
                return False

            row_data = bytearray()
            for x in range(WIDTH):
                r, g, b = img.getpixel((x, y))
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                row_data.append((rgb565 >> 8) & 0xFF)   # high byte first
                row_data.append(rgb565 & 0xFF)

            ser.write(row_data)
            ser.flush()
            print(f"  Sent row {y+1}")

        print(f"Finished sending {filename}")
        return True

    except Exception as e:
        print(f"Error during send: {e}")
        return False


def wait_for_button_hold(ser):
    print("\nWaiting for you to hold M5 button for 2 seconds...")
    print("(Holding the button will trigger the next image)\n")

    start_wait = time.time()
    while time.time() - start_wait < HOLD_WAIT_TIMEOUT:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line == "BUTTON_HELD_2S":
                print("Button held for 2 seconds → proceeding to next image!")
                return True
            else:
                print(f"Received: '{line}' (ignoring)")
        time.sleep(0.05)

    print("Timeout waiting for button hold! (120 seconds)")
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
        time.sleep(3.5)

        ser.reset_input_buffer()
        ser.reset_output_buffer()

        for i, filename in enumerate(sorted(files), 1):
            full_path = os.path.join(IMG_FOLDER, filename)
            success = process_and_send(full_path, ser)
            if not success:
                print("Aborting remaining images.")
                break

            # After image sent → wait for button hold (except after last image)
            if i < len(files):
                if not wait_for_button_hold(ser):
                    print("No button hold detected → stopping.")
                    break
            else:
                print("Last image sent. No more waiting.")

        ser.close()
        print("Done!")

    except serial.SerialException as e:
        print(f"Serial port error: {e}")
        print("→ Close any other programs using COM4")
    except Exception as e:
        print(f"Unexpected error: {e}")
#include <M5StickCPlus2.h>

const int screenWidth  = 240;
const int screenHeight = 135;

// Buffer to hold one full row of pixels
uint16_t rowBuffer[screenWidth];

// Progress bar settings
const int PROGRESS_BAR_HEIGHT = 10;
const int PROGRESS_BAR_Y = screenHeight - PROGRESS_BAR_HEIGHT;
const uint16_t PROGRESS_BG_COLOR   = BLACK;
const uint16_t PROGRESS_FILL_COLOR = GREENYELLOW;

// Transition type (change as needed)
#define TRANSITION_TYPE 2   // 0=none, 1=fade, 2=horiz wipe, 3=vert wipe

// Button timing
const unsigned long LONG_PRESS_MS = 3000;  // 3 seconds for next image

// State variables
bool showImage = true;                     // Toggle: true = picture visible, false = black screen
bool hasImage = false;                     // Whether an image is currently loaded
uint16_t* currentImageBuffer = nullptr;    // Pointer to last received image (for redrawing)
bool waitingForNext = false;               // After last image, waiting for more or show "no more"

// ────────────────────────────────────────────────
// Progress bar function
// ────────────────────────────────────────────────
void drawProgressBar(int currentRow, int totalRows) {
    StickCP2.Display.fillRect(0, PROGRESS_BAR_Y, screenWidth, PROGRESS_BAR_HEIGHT, PROGRESS_BG_COLOR);
    int fillWidth = (currentRow * screenWidth) / totalRows;
    if (fillWidth > 0) {
        StickCP2.Display.fillRect(0, PROGRESS_BAR_Y, fillWidth, PROGRESS_BAR_HEIGHT, PROGRESS_FILL_COLOR);
    }
    StickCP2.Display.drawRect(0, PROGRESS_BAR_Y, screenWidth, PROGRESS_BAR_HEIGHT, WHITE);
}

// ────────────────────────────────────────────────
// Transition functions (your existing ones - shortened for brevity)
// ────────────────────────────────────────────────
void horizontalWipe(uint16_t* imgBuf) {
    const int steps = 1;  // wider steps = faster
    for (int col = 0; col <= screenWidth; col += steps) {
        for (int y = 0; y < screenHeight; y++) {
            int drawWidth = min(steps, screenWidth - col);
            if (drawWidth > 0) {
                memcpy(rowBuffer, imgBuf + y * screenWidth + col, drawWidth * 2);
                StickCP2.Display.pushImage(col, y, drawWidth, 1, rowBuffer);
            }
        }
        delay(5);  // Adjust speed
    }
}
void verticalWipe(uint16_t* imgBuf) {
    const int delayPerRowMs = 32;   // speed (higher = slower)

    for (int y = 0; y < screenHeight; y++) {
        memcpy(rowBuffer, imgBuf + y * screenWidth, screenWidth * 2);
        StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
        delay(delayPerRowMs);   // Controls how fast each row appears
    }
}
void fadeIn(uint16_t* imgBuf) {
    const int FADE_STEPS = 8;           // higher = smoother but slower
    const int delayPerStep = 30;        // ms between steps (~240 ms total)

    for (int step = 0; step <= FADE_STEPS; step++) {
        float factor = (float)step / FADE_STEPS;

        for (int y = 0; y < screenHeight; y++) {
            for (int x = 0; x < screenWidth; x++) {
                uint16_t color = imgBuf[y * screenWidth + x];

                // Extract RGB565 channels
                uint8_t r = (color >> 11) & 0x1F;
                uint8_t g = (color >> 5)  & 0x3F;
                uint8_t b =  color        & 0x1F;

                // Scale brightness
                r = (uint8_t)(r * factor);
                g = (uint8_t)(g * factor);
                b = (uint8_t)(b * factor);

                rowBuffer[x] = (r << 11) | (g << 5) | b;
            }
            StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
        }
        delay(delayPerStep);
    }
}
void applyTransition(uint16_t* imgBuf) {
    switch (TRANSITION_TYPE) {
        case 0: StickCP2.Display.pushImage(0, 0, screenWidth, screenHeight, imgBuf); break;
        case 1: fadeIn(imgBuf); break;
        case 2: horizontalWipe(imgBuf); break;
        case 3: verticalWipe(imgBuf); break;
        default: StickCP2.Display.pushImage(0, 0, screenWidth, screenHeight, imgBuf); break;
    }
}

// (Include your fadeIn / horizontalWipe / verticalWipe functions here - unchanged)

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.fillScreen(BLACK);

    Serial.begin(115200);
    Serial.setRxBufferSize(2048);  // Important for reliable row transfer

    StickCP2.Display.drawCenterString("Ready for Serial Images", 120, 60);
}

void loop() {
    StickCP2.update();  // MUST call this regularly to read buttons!

    // Button handling
    static unsigned long pressStart = 0;
    static bool wasPressed = false;

    if (StickCP2.BtnA.isPressed()) {
        if (!wasPressed) {
            pressStart = millis();
            wasPressed = true;
        }

        unsigned long heldTime = millis() - pressStart;
        if (heldTime >= LONG_PRESS_MS && !waitingForNext) {
            // Long press → next image
            if (currentImageBuffer != nullptr) {
                delete[] currentImageBuffer;
                currentImageBuffer = nullptr;
            }
            hasImage = false;
            showImage = true;
            waitingForNext = true;  // Wait for new image via serial
            StickCP2.Display.fillScreen(BLACK);
            StickCP2.Display.drawCenterString("Waiting for next image...", 120, 60);
            pressStart = 0;  // Prevent repeat
        }
    } else {
        if (wasPressed) {
            // Released → check if it was short press
            if (millis() - pressStart < LONG_PRESS_MS) {
                // Short press → toggle visibility
                if (hasImage) {
                    showImage = !showImage;
                    if (showImage) {
                        // Redraw current image (if we have it buffered)
                        if (currentImageBuffer != nullptr) {
                            applyTransition(currentImageBuffer);
                        }
                    } else {
                        StickCP2.Display.fillScreen(BLACK);
                    }
                }
            }
            wasPressed = false;
            pressStart = 0;
        }
    }

    // Serial image receiving
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "START") {
            waitingForNext = false;  // We got a new image

            uint16_t* imageBuffer = new uint16_t[screenWidth * screenHeight];
            if (!imageBuffer) {
                Serial.println("Memory alloc failed!");
                return;
            }

            StickCP2.Display.fillScreen(BLACK);
            StickCP2.Display.drawCenterString("Receiving image...", 120, 50);

            for (int y = 0; y < screenHeight; y++) {
                Serial.println("NEXT_ROW");

                uint32_t timeout = millis();
                while (Serial.available() < screenWidth * 2) {
                    if (millis() - timeout > 500) {
                        Serial.println("Row timeout!");
                        delete[] imageBuffer;
                        return;
                    }
                    StickCP2.update();  // Keep button responsive during receive
                }

                Serial.readBytes((uint8_t*)(imageBuffer + y * screenWidth), screenWidth * 2);
                drawProgressBar(y + 1, screenHeight);
            }

            Serial.println("OK");

            // Free old image if exists
            if (currentImageBuffer != nullptr) {
                delete[] currentImageBuffer;
            }
            currentImageBuffer = imageBuffer;
            hasImage = true;
            showImage = true;

            // Display with transition
            applyTransition(currentImageBuffer);

            // Clear progress bar after done
            StickCP2.Display.fillRect(0, PROGRESS_BAR_Y, screenWidth, PROGRESS_BAR_HEIGHT, BLACK);
        }
    }

    // If no more pictures and long press happened
    if (waitingForNext && !hasImage) {
        if (millis() % 2000 < 1000) {  // Blink message
            StickCP2.Display.drawCenterString("No more pictures", 120, 60);
        } else {
            StickCP2.Display.fillScreen(BLACK);
        }
    }
}
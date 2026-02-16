#include <M5StickCPlus2.h>

const int screenWidth  = 240;
const int screenHeight = 135;

// Buffer to hold one full row of pixels
uint16_t rowBuffer[screenWidth];

// Choose transition here (0 = none / instant, 1 = fade-in, 2 = horizontal wipe, 3 = vertical wipe)
#define TRANSITION_TYPE 1   // <-- Change this number to try different effects

// For fade-in: number of steps (higher = smoother but slower)
const int FADE_STEPS = 8;

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.fillScreen(BLACK);

    Serial.begin(115200);
    Serial.setRxBufferSize(1024);

    StickCP2.Display.drawCenterString("Ready for Serial Images", 120, 60);
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "START") {
            // We'll collect the full image first into a temporary buffer
            // (135 rows × 240 pixels × 2 bytes = ~64 KB — fits comfortably in PSRAM/heap)
            uint16_t* imageBuffer = new uint16_t[screenWidth * screenHeight];

            if (!imageBuffer) {
                Serial.println("Memory allocation failed!");
                return;
            }

            for (int y = 0; y < screenHeight; y++) {
                Serial.println("NEXT_ROW");  // Tell Python we're ready

                uint32_t timeout = millis();
                while (Serial.available() < screenWidth * 2) {
                    if (millis() - timeout > 500) {
                        Serial.println("Timeout waiting for row!");
                        delete[] imageBuffer;
                        return;
                    }
                }

                Serial.readBytes((uint8_t*)(imageBuffer + y * screenWidth), screenWidth * 2);
            }

            Serial.println("OK");  // Image fully received

            // Now apply the chosen transition
            applyTransition(imageBuffer);

            // Clean up
            delete[] imageBuffer;
        }
    }
}

// ────────────────────────────────────────────────
// Transition functions
// ────────────────────────────────────────────────

void applyTransition(uint16_t* imgBuf) {
    switch (TRANSITION_TYPE) {
        case 0:  // Instant (original behavior)
            StickCP2.Display.pushImage(0, 0, screenWidth, screenHeight, imgBuf);
            break;

        case 1:  // Fade-in from black
            fadeIn(imgBuf);
            break;

        case 2:  // Horizontal wipe (left to right)
            horizontalWipe(imgBuf);
            break;

        case 3:  // Vertical wipe (top to bottom – smooth version)
            verticalWipe(imgBuf);
            break;

        default:
            StickCP2.Display.pushImage(0, 0, screenWidth, screenHeight, imgBuf);
            break;
    }
}

void fadeIn(uint16_t* imgBuf) {
    // Simple brightness scaling fade (not true alpha, but looks decent)
    for (int step = 0; step <= FADE_STEPS; step++) {
        float factor = (float)step / FADE_STEPS;

        for (int y = 0; y < screenHeight; y++) {
            for (int x = 0; x < screenWidth; x++) {
                uint16_t color = imgBuf[y * screenWidth + x];

                uint8_t r = (color >> 11) & 0x1F;
                uint8_t g = (color >> 5)  & 0x3F;
                uint8_t b =  color        & 0x1F;

                r = (uint8_t)(r * factor);
                g = (uint8_t)(g * factor);
                b = (uint8_t)(b * factor);

                rowBuffer[x] = (r << 11) | (g << 5) | b;
            }
            StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
        }
        delay(30);  // Adjust for speed (total ~240 ms at 8 steps)
    }
}

void horizontalWipe(uint16_t* imgBuf) {
    const int steps = 32;  // wider steps = faster
    for (int col = 0; col <= screenWidth; col += steps) {
        for (int y = 0; y < screenHeight; y++) {
            int drawWidth = min(steps, screenWidth - col);
            if (drawWidth > 0) {
                memcpy(rowBuffer, imgBuf + y * screenWidth + col, drawWidth * 2);
                StickCP2.Display.pushImage(col, y, drawWidth, 1, rowBuffer);
            }
        }
        delay(15);  // Adjust speed
    }
}

void verticalWipe(uint16_t* imgBuf) {
    // Similar to your original row-by-row, but controlled speed
    for (int y = 0; y < screenHeight; y++) {
        memcpy(rowBuffer, imgBuf + y * screenWidth, screenWidth * 2);
        StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
        delay(8);  // ~1 second total – feels smooth
    }
}
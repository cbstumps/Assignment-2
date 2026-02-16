#include <M5StickCPlus2.h>

const int screenWidth  = 240;
const int screenHeight = 135;
const int PROGRESS_BAR_HEIGHT = 10;
const int PROGRESS_BAR_Y = screenHeight - PROGRESS_BAR_HEIGHT;  // bottom of screen
const uint16_t PROGRESS_BG_COLOR   = BLACK;
const uint16_t PROGRESS_FILL_COLOR = GREENYELLOW;

// Buffer to hold one full row of pixels
uint16_t rowBuffer[screenWidth];

// Choose transition here (0 = none / instant, 1 = fade-in, 2 = horizontal wipe, 3 = vertical wipe)
#define TRANSITION_TYPE 2   // <-- Change this number to try different effects

// For fade-in: number of steps (higher = smoother but slower)
const int FADE_STEPS = 32;

void setup() {
    Serial.setRxBufferSize(2048);  // ← Add / increase this
    Serial.begin(115200);

    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.fillScreen(BLACK);

    StickCP2.Display.drawCenterString("Ready for Serial Images", 120, 60);
}

void drawProgressBar(int currentRow, int totalRows) {
    // Background (clear previous bar)
    StickCP2.Display.fillRect(0, PROGRESS_BAR_Y, screenWidth, PROGRESS_BAR_HEIGHT, PROGRESS_BG_COLOR);

    // Calculate fill width
    int fillWidth = (currentRow * screenWidth) / totalRows;

    // Draw filled portion
    if (fillWidth > 0) {
        StickCP2.Display.fillRect(0, PROGRESS_BAR_Y, fillWidth, PROGRESS_BAR_HEIGHT, PROGRESS_FILL_COLOR);
    }

    // Optional: thin white border
    StickCP2.Display.drawRect(0, PROGRESS_BAR_Y, screenWidth, PROGRESS_BAR_HEIGHT, WHITE);
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "START") {
            uint16_t* imageBuffer = new uint16_t[screenWidth * screenHeight];
            if (!imageBuffer) {
            Serial.println("Memory allocation failed!");
            return;
        }

        // Optional: clear screen or show "Receiving..." message
        StickCP2.Display.fillScreen(BLACK);
        StickCP2.Display.drawCenterString("Receiving image...", 120, 50);

        for (int y = 0; y < screenHeight; y++) {
            Serial.println("NEXT_ROW");

            uint32_t timeout = millis();
            while (Serial.available() < screenWidth * 2) {
                if (millis() - timeout > 500) {
                    Serial.println("Timeout waiting for row!");
                    delete[] imageBuffer;
                    return;
                }
            }

            Serial.readBytes((uint8_t*)(imageBuffer + y * screenWidth), screenWidth * 2);

            // Update progress bar after receiving this row
            drawProgressBar(y + 1, screenHeight);   // y+1 because we just finished this row
        }

        Serial.println("OK");

        // Now apply transition (your existing code)
        applyTransition(imageBuffer);

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
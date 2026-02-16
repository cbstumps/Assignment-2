#include <M5StickCPlus2.h>

const int screenWidth = 240;
const int screenHeight = 135;
// Buffer to hold one full row of pixels to speed up drawing
uint16_t rowBuffer[screenWidth]; 

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.fillScreen(BLACK);
    
    Serial.begin(115200);
    // Increase the serial buffer size for smoother image transfer
    Serial.setRxBufferSize(1024); 

    StickCP2.Display.drawCenterString("Ready for Serial Images", 120, 60);
}

int transitionType = 2; 

void loop() {
    StickCP2.update();

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        
        if (cmd == "START") {
            // Clear buffer to prevent rainbow/offset issues
            while(Serial.available() > 0) { Serial.read(); }
            
            transitionType = (transitionType + 1) % 3; // 0: Top-Down, 1: Bottom-Up, 2: Interlaced

            for (int y = 0; y < screenHeight; y++) {
                Serial.println("NEXT_ROW"); 

                for (int x = 0; x < screenWidth; x++) {
                    uint8_t bytes[2];
                    uint32_t timeout = millis();
                    while(Serial.available() < 2) {
                        if(millis() - timeout > 500) break; 
                    }
                    Serial.readBytes(bytes, 2);
                    rowBuffer[x] = (bytes[0] << 8) | bytes[1];
                }

                // --- REFINED TRANSITIONS ---
                if (transitionType == 0) {
                    // Standard Top-Down Wipe
                    StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
                } 
                else if (transitionType == 1) {
                    // Bottom-Up Wipe (Corrected: Data still goes to matching y)
                    // This will look like the image is appearing from bottom to top
                    // but the pixels will be in their correct final positions.
                    StickCP2.Display.pushImage(0, screenHeight - 1 - y, screenWidth, 1, rowBuffer);
                }
                else if (transitionType == 2) {
                    // Interlaced (TV Style)
                    // Draws every other line first, then fills in the gaps
                    // This creates a "Venetian Blind" effect
                    int interlacedY;
                    if (y < screenHeight / 2) {
                        interlacedY = y * 2; // Even rows
                    } else {
                        interlacedY = ((y - screenHeight / 2) * 2) + 1; // Odd rows
                    }
                    StickCP2.Display.pushImage(0, interlacedY, screenWidth, 1, rowBuffer);
                }
            }
            Serial.println("OK"); 
        }
    }
}
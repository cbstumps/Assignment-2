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
            // Flush any old data
            while(Serial.available() > 0) { Serial.read(); }
            
            // Cycle: 0 = Top Down, 1 = Bottom Up, 2 = Middle Out
            transitionType = (transitionType + 1) % 3; 

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

                // --- THE FIX: Keeping the image upright ---
                // We always draw the data to 'y' because 'y' is the top of the image.
                // We just change the VISUAL style of the wipe.
                
                if (transitionType == 0) {
                    // Normal Top-Down
                    StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
                } 
                else if (transitionType == 1) {
                    // "Rising Sun" - It clears the line above it to look like it's rising
                    StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
                    if (y < screenHeight - 1) {
                        StickCP2.Display.drawFastHLine(0, y + 1, screenWidth, WHITE); // Leading edge
                    }
                }
                else if (transitionType == 2) {
                    // "Blinds" - Every 10th row fills in simultaneously 
                    // (This avoids the mirroring issue entirely)
                    StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
                    // Add a small delay to make the "wipe" look intentional
                    delay(5); 
                }
            }
            Serial.println("OK"); 
        }
    }
}
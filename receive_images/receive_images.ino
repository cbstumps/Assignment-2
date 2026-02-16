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

int transitionType = 0; // 0: Normal, 1: Bottom-Up, 2: Center-Out

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        
        if (cmd == "START") {
            transitionType = (transitionType + 1) % 3; // Cycle through 3 transitions
            
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

                // --- TRANSITION LOGIC ---
                if (transitionType == 0) {
                    // Standard Top-Down
                    StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
                } 
                else if (transitionType == 1) {
                    // Bottom-Up (Draws from the bottom of the screen to the top)
                    StickCP2.Display.pushImage(0, screenHeight - 1 - y, screenWidth, 1, rowBuffer);
                }
                else if (transitionType == 2) {
                    // "Curtain" / Center-Out
                    // Draws rows toward the center from top and bottom simultaneously
                    if (y % 2 == 0) {
                        StickCP2.Display.pushImage(0, y / 2, screenWidth, 1, rowBuffer);
                    } else {
                        StickCP2.Display.pushImage(0, screenHeight - 1 - (y / 2), screenWidth, 1, rowBuffer);
                    }
                }
            }
            Serial.println("OK"); 
        }
    }
}
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

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        
        if (cmd == "START") {
            for (int y = 0; y < screenHeight; y++) {
                // SYNC: Tell Python we are ready for the next row
                Serial.println("NEXT_ROW"); 

                for (int x = 0; x < screenWidth; x++) {
                    uint8_t bytes[2];
                    // Wait for 2 bytes with a timeout to prevent freezing
                    uint32_t timeout = millis();
                    while(Serial.available() < 2) {
                        if(millis() - timeout > 500) break; 
                    }
                    Serial.readBytes(bytes, 2);
                    rowBuffer[x] = (bytes[0] << 8) | bytes[1];
                }
                StickCP2.Display.pushImage(0, y, screenWidth, 1, rowBuffer);
            }
            Serial.println("OK"); 
        }
    }
}
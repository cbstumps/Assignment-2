#include <M5StickCPlus2.h>

const int screenWidth = 240;
const int screenHeight = 135;
uint16_t rowBuffer[screenWidth];
M5Canvas canvas(&StickCP2.Display); // The "Ghost" screen

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    
    // Create the memory space for the image
    canvas.createSprite(screenWidth, screenHeight);
    
    Serial.begin(115200);
    Serial.setRxBufferSize(2048); 
    
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.drawCenterString("Ready for Upload", 120, 60);
}

int transitionType = 0;

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        
        if (cmd == "START") {
            // 1. Receive data into Canvas
            for (int y = 0; y < screenHeight; y++) {
                Serial.println("NEXT_ROW"); // Handshake
                
                for (int x = 0; x < screenWidth; x++) {
                    uint8_t bytes[2];
                    while(Serial.available() < 2); // Wait for pixel
                    Serial.readBytes(bytes, 2);
                    uint16_t color = (bytes[0] << 8) | bytes[1];
                    canvas.drawPixel(x, y, color);
                }
            }

            // 2. Perform Transition
            transitionType = (transitionType + 1) % 3;
            
            if (transitionType == 0) {
                // Slide in from Right
                for (int i = screenWidth; i >= 0; i -= 10) {
                    canvas.pushSprite(&StickCP2.Display, i, 0);
                }
            } 
            else if (transitionType == 1) {
                // Drop from Top
                for (int i = -screenHeight; i <= 0; i += 5) {
                    canvas.pushSprite(&StickCP2.Display, 0, i);
                }
            }
            else {
                // Instant Pop
                canvas.pushSprite(&StickCP2.Display, 0, 0);
            }

            Serial.println("OK"); 
        }
    }
}
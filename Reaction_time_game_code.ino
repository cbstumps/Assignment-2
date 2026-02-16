#include <M5StickCPlus2.h>
#include <Preferences.h>
#include <vector>
#include <algorithm>

enum State { IDLE, WAIT, REACTION, RESULT, LEADERBOARD, FOUL };
State currentState = IDLE;

unsigned long startTime;
unsigned long targetTime;
std::vector<int> highScores;
Preferences prefs;

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    
    // Load High Scores from Flash Memory
    prefs.begin("scores", false);
    for (int i = 0; i < 5; i++) {
        // Default high score is 9999ms until someone beats it
        int s = prefs.getInt(("s" + String(i)).c_str(), 9999);
        highScores.push_back(s);
    }
    prefs.end();

    resetToIdle();
}

void resetToIdle() {
    currentState = IDLE;
    StickCP2.Display.fillScreen(BLACK);
    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.setTextSize(2);
    
    // Split into two lines for better readability
    StickCP2.Display.drawCenterString("M5: Start", 120, 45);
    StickCP2.Display.drawCenterString("Side: Leaderboard", 120, 75);
}

void showLeaderboard() {
    currentState = LEADERBOARD;
    StickCP2.Display.fillScreen(BLUE);
    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.drawCenterString("-- TOP 5 TIMES --", 120, 10);
    
    for (int i = 0; i < 5; i++) {
        String scoreText = String(i + 1) + ". " + (highScores[i] == 9999 ? "---" : String(highScores[i]) + " ms");
        StickCP2.Display.setCursor(65, 35 + (i * 15));
        StickCP2.Display.print(scoreText);
    }
    StickCP2.Display.drawCenterString("Press M5 to Return", 120, 115);
}

void updateLeaderboard(int newTime) {
    highScores.push_back(newTime);
    std::sort(highScores.begin(), highScores.end());
    highScores.pop_back(); // Keep only top 5 fastest

    // Save back to Flash Memory
    prefs.begin("scores", false);
    for (int i = 0; i < 5; i++) {
        prefs.putInt(("s" + String(i)).c_str(), highScores[i]);
    }
    prefs.end();
}

void loop() {
    StickCP2.update();

    switch (currentState) {
        case IDLE:
            if (StickCP2.BtnA.wasPressed()) { // Front M5 Button
                currentState = WAIT;
                StickCP2.Display.fillScreen(RED);
                StickCP2.Display.setTextColor(WHITE);
                StickCP2.Display.drawCenterString("WAIT...", 120, 60);
                targetTime = millis() + random(2000, 5000);
            }
            if (StickCP2.BtnB.wasPressed()) { // Side Button
                showLeaderboard();
            }
            break;

        case WAIT:
            if (StickCP2.BtnA.wasPressed()) {
                currentState = FOUL;
                StickCP2.Display.fillScreen(ORANGE);
                StickCP2.Display.drawCenterString("Too Early!", 120, 60);
                delay(1500);
                resetToIdle();
            } else if (millis() >= targetTime) {
                currentState = REACTION;
                startTime = millis();
                StickCP2.Display.fillScreen(GREEN);
                StickCP2.Display.setTextColor(BLACK);
            }
            break;

        case REACTION:
            // LIVE TIMER: Redraws the time as it counts up
            // Note: fillRect is used here to prevent text overlapping/flicker
            StickCP2.Display.fillRect(60, 55, 120, 25, GREEN); 
            StickCP2.Display.drawCenterString(String(millis() - startTime) + " ms", 120, 60);
            
            if (StickCP2.BtnA.wasPressed()) {
                int finalTime = millis() - startTime;
                updateLeaderboard(finalTime);
                currentState = RESULT;
                StickCP2.Display.fillScreen(BLACK);
                StickCP2.Display.setTextColor(WHITE);
                StickCP2.Display.drawCenterString("TIME: " + String(finalTime) + "ms", 120, 45);
                StickCP2.Display.drawCenterString("Press M5 for Menu", 120, 75);
            }
            break;

        case RESULT:
        case LEADERBOARD:
            if (StickCP2.BtnA.wasPressed()) resetToIdle();
            break;
    }
}
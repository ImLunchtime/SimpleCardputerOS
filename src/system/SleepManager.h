#pragma once
#include <M5Cardputer.h>
#include "system/BatteryManager.h"

class SleepManager {
private:
    bool sleeping;
    BatteryManager battery;
    unsigned long lastRedrawMs;

public:
    SleepManager() : sleeping(false), lastRedrawMs(0) {}

    bool isSleeping() const {
        return sleeping;
    }

    void enterSleep() {
        sleeping = true;
        battery.forceUpdate();
        drawLockScreen(true);
    }

    void leaveSleep() {
        sleeping = false;
    }

    void tick() {
        if (!sleeping) return;
        unsigned long now = millis();
        if (now - lastRedrawMs >= 1000) {
            battery.update();
            drawLockScreen(false);
            lastRedrawMs = now;
        }
    }

private:
    void drawLockScreen(bool clearAll) {
        auto& d = M5Cardputer.Display;
        int16_t w = d.width();
        int16_t h = d.height();
        if (w <= 0) w = 240;
        if (h <= 0) h = 135;

        if (clearAll) {
            d.fillScreen(TFT_BLACK);
        } else {
            d.fillRect(0, 0, w, h, TFT_BLACK);
        }

        d.setTextColor(TFT_WHITE);
        d.setTextSize(1);
        d.setFont(&fonts::efontCN_12);

        String text = battery.getBatteryInfo();

        int16_t x = 5;
        int16_t y = h / 2;

        d.setCursor(x, y);
        d.print(text);
    }
};

extern SleepManager globalSleepManager;


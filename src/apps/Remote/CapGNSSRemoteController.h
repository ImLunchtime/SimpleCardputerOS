#pragma once

#include "IRemoteController.h"
#include <M5Cardputer.h>
#include "MultipleSatellite.h"

class CapGNSSRemoteController : public IRemoteController {
public:
    CapGNSSRemoteController()
        : window(nullptr),
          statusLabel(nullptr),
          satInfoLabel(nullptr),
          positionLabel(nullptr),
          timeLabel(nullptr),
          uiManager(nullptr),
          lastUpdateMs(0),
          hasFix(false) {
    }

    const char* getMenuText() const override {
        return "Cap GNSS";
    }

    int getMenuItemId() const override {
        return 102;
    }

    void ensureCreated(UIManager* manager) override {
        if (window) {
            return;
        }
        uiManager = manager;
        window = uiManager->createWindow(WINDOW_ID, 20, 20, 200, 100, "Cap GNSS", "CapGNSSWindow");
        statusLabel = uiManager->createLabel(STATUS_LABEL_ID, 8, 25, "GNSS: initializing...", "CapGNSSStatus", window);
        satInfoLabel = uiManager->createLabel(SAT_INFO_LABEL_ID, 8, 40, "Sat: -- Mode: --", "CapGNSSSatInfo", window);
        positionLabel = uiManager->createLabel(POSITION_LABEL_ID, 8, 55, "Lat: ---.------  Lon: ---.------", "CapGNSSPos", window);
        timeLabel = uiManager->createLabel(TIME_LABEL_ID, 8, 70, "Time: --:--:--", "CapGNSSTime", window);
        uiManager->setWidgetTreeVisible(window, false);
    }

    UIWindow* getWindow() const override {
        return window;
    }

    void handleKeyEvent(const KeyEvent& event, void (*sendCommand)(const char* cmd, void* ctx), void* ctx) override {
        (void)event;
        (void)sendCommand;
        (void)ctx;
    }

    void update() override {
        if (!uiManager || !window || !window->isVisible()) {
            return;
        }
        initGnss();
        gps.updateGPS();
        bool locationUpdated = gps.location.isUpdated();
        bool timeUpdated = gps.time.isUpdated();
        int satCount = gps.satellites.value();
        String mode = gps.getSatelliteMode();
        char satBuf[48];
        snprintf(satBuf, sizeof(satBuf), "Sat: %d %s", satCount, mode.c_str());
        satInfoLabel->setText(satBuf);
        unsigned long now = millis();
        if (!locationUpdated && !timeUpdated && (now - lastUpdateMs < 500)) {
            return;
        }
        lastUpdateMs = now;
        if (gps.location.isValid()) {
            hasFix = true;
            double lat = gps.location.lat();
            double lon = gps.location.lng();
            char posBuf[64];
            snprintf(posBuf, sizeof(posBuf), "Lat: %.6f  Lon: %.6f", lat, lon);
            positionLabel->setText(posBuf);
        } else {
            hasFix = false;
            positionLabel->setText("Lat: ---.------  Lon: ---.------");
        }
        if (gps.time.isValid()) {
            int hour = (gps.time.hour() + 8) % 24; // UTC+8 Using a simple calculation now. Will add proper timezone logic according to GPS coords later.
            int minute = gps.time.minute();
            int second = gps.time.second();
            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "Time: %02d:%02d:%02d", hour, minute, second);
            timeLabel->setText(timeBuf);
        } else {
            timeLabel->setText("Time: --:--:--");
        }
        if (hasFix) {
            statusLabel->setText("GNSS: fix");
        } else {
            statusLabel->setText("GNSS: searching...");
        }
        Serial.printf("GNSS sats=%d mode=%s locValid=%d timeValid=%d\n",
                      satCount,
                      mode.c_str(),
                      gps.location.isValid() ? 1 : 0,
                      gps.time.isValid() ? 1 : 0);
    }

private:
    enum ControlIds {
        WINDOW_ID = 20,
        STATUS_LABEL_ID = 21,
        SAT_INFO_LABEL_ID = 22,
        POSITION_LABEL_ID = 23,
        TIME_LABEL_ID = 24
    };

    UIWindow* window;
    UILabel* statusLabel;
    UILabel* satInfoLabel;
    UILabel* positionLabel;
    UILabel* timeLabel;
    UIManager* uiManager;
    unsigned long lastUpdateMs;
    bool hasFix;

    inline static MultipleSatellite gps{Serial1, 115200, SERIAL_8N1, 15, 13};
    inline static bool gnssInitialized;
    inline static bool gnssConfigured;
    inline static bool serialInitialized;

    void initGnss() {
        if (!serialInitialized) {
            serialInitialized = true;
            Serial.begin(115200);
        }
        if (!gnssInitialized) {
            gnssInitialized = true;
            gps.begin();
        }
        if (!gnssConfigured) {
            gnssConfigured = true;
            Serial.print(F("<----------Cap GNSS Remote---------->"));
            Serial.print(F(" TinyGPSPlus v"));
            Serial.println(TinyGPSPlus::libraryVersion());
            String version = gps.getGNSSVersion();
            Serial.printf(" GNSS SW=%s\r\n", version.c_str());
        }
    }
};


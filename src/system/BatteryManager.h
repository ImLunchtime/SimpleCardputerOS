#pragma once
#include <M5Cardputer.h>

class BatteryManager {
private:
    int batteryLevel;      // 电池电量百分比 (0-100)
    int batteryVoltage;    // 电池电压 (mV)
    bool isCharging;       // 是否正在充电
    unsigned long lastUpdateTime;  // 上次更新时间
    static const unsigned long UPDATE_INTERVAL = 1000;  // 更新间隔1秒

public:
    BatteryManager() : batteryLevel(0), batteryVoltage(0), isCharging(false), lastUpdateTime(0) {}

    // 更新电池信息
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
            batteryLevel = M5Cardputer.Power.getBatteryLevel();      // 0 - 100 %
            batteryVoltage = M5Cardputer.Power.getBatteryVoltage();  // unit: mV
            isCharging = M5Cardputer.Power.isCharging();
            lastUpdateTime = currentTime;
        }
    }

    // 获取电池电量百分比
    int getBatteryLevel() const {
        return batteryLevel;
    }

    // 获取电池电压 (mV)
    int getBatteryVoltage() const {
        return batteryVoltage;
    }

    // 获取充电状态
    bool getChargingStatus() const {
        return isCharging;
    }

    // 获取格式化的电池信息字符串
    String getBatteryInfo() const {
        String info = String(batteryLevel) + "% " + String(batteryVoltage) + "mV";
        if (isCharging) {
            info += " CHG";
        }
        return info;
    }

    // 获取简短的电池信息字符串（仅百分比）
    String getBatteryLevelString() const {
        String info = String(batteryLevel) + "%";
        if (isCharging) {
            info += "+";
        }
        return info;
    }

    // 强制立即更新
    void forceUpdate() {
        batteryLevel = M5Cardputer.Power.getBatteryLevel();
        batteryVoltage = M5Cardputer.Power.getBatteryVoltage();
        isCharging = M5Cardputer.Power.isCharging();
        lastUpdateTime = millis();
    }
};

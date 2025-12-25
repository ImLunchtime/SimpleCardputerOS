#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
class UISlider : public UIWidget {
private:
    int minValue;
    int maxValue;
    int currentValue;
    uint16_t trackColor;
    uint16_t thumbColor;
    uint16_t focusColor;
    String label;
    bool showValue;
public:
    UISlider(int id, int x, int y, int width, int height, int min, int max, int initial, const String& _label = "", const String& name = "")
        : UIWidget(id, WIDGET_SLIDER, x, y, width, height, name, true),
          minValue(min), maxValue(max), currentValue(initial),
          trackColor(TFT_DARKGREY), thumbColor(TFT_WHITE), focusColor(TFT_YELLOW),
          label(_label), showValue(true) {
        if (currentValue < minValue) currentValue = minValue;
        if (currentValue > maxValue) currentValue = maxValue;
    }
    void setValue(int value) {
        if (value < minValue) value = minValue;
        if (value > maxValue) value = maxValue;
        if (currentValue != value) {
            currentValue = value;
            onValueChanged(currentValue);
        }
    }
    int getValue() const { return currentValue; }
    void setRange(int min, int max) {
        minValue = min;
        maxValue = max;
        if (currentValue < minValue) setValue(minValue);
        if (currentValue > maxValue) setValue(maxValue);
    }
    void setColors(uint16_t track, uint16_t thumb, uint16_t focus) {
        trackColor = track;
        thumbColor = thumb;
        focusColor = focus;
    }
    void setShowValue(bool show) { showValue = show; }
    void setLabel(const String& newLabel) { label = newLabel; }
    void draw(LGFX_Device* display) override {
        if (!visible) return;
        Theme* currentTheme = getCurrentTheme();
        if (currentTheme) {
            SliderDrawParams params;
            params.display = display;
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            params.x = absX;
            params.y = absY;
            params.width = width;
            params.height = height;
            params.minValue = minValue;
            params.maxValue = maxValue;
            params.currentValue = currentValue;
            params.label = label;
            params.showValue = showValue;
            params.focused = focused;
            params.trackColor = trackColor;
            params.thumbColor = thumbColor;
            currentTheme->drawSlider(params);
        } else {
            if (label.length() > 0) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                int absX = getAbsoluteX();
                int absY = getAbsoluteY();
                display->setCursor(absX, absY - 12);
                display->print(label);
            }
            int absX2 = getAbsoluteX();
            int absY2 = getAbsoluteY();
            int trackY = absY2 + (height - 4) / 2;
            int trackHeight = 4;
            uint16_t borderColor = focused ? focusColor : TFT_WHITE;
            display->drawRect(absX2, trackY, width, trackHeight, borderColor);
            display->fillRect(absX2 + 1, trackY + 1, width - 2, trackHeight - 2, trackColor);
            int range = maxValue - minValue;
            int thumbX = absX2;
            if (range > 0) {
                thumbX = absX2 + ((currentValue - minValue) * (width - 8)) / range;
            }
            uint16_t currentThumbColor = focused ? focusColor : thumbColor;
            display->fillRect(thumbX, absY2, 8, height, currentThumbColor);
            display->drawRect(thumbX, absY2, 8, height, TFT_BLACK);
            if (showValue) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                String valueText = String(currentValue);
                int textWidth = valueText.length() * 6;
                display->setCursor(absX2 + width - textWidth, absY2 + height + 2);
                display->print(valueText);
            }
        }
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        if (!focused) return false;
        bool handled = false;
        int step = (maxValue - minValue) / 20;
        if (step < 1) step = 1;
        if (event.left) {
            setValue(currentValue - step);
            handled = true;
        } else if (event.right) {
            setValue(currentValue + step);
            handled = true;
        }
        return handled;
    }
    virtual void onValueChanged(int newValue) {}
};

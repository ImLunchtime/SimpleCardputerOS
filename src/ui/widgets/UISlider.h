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
            invalidate();
            onValueChanged(currentValue);
        }
    }
    int getValue() const { return currentValue; }
    void setRange(int min, int max) {
        if (minValue == min && maxValue == max) return;
        minValue = min;
        maxValue = max;
        invalidate();
        if (currentValue < minValue) setValue(minValue);
        if (currentValue > maxValue) setValue(maxValue);
    }
    void setColors(uint16_t track, uint16_t thumb, uint16_t focus) {
        if (trackColor == track && thumbColor == thumb && focusColor == focus) return;
        trackColor = track;
        thumbColor = thumb;
        focusColor = focus;
        invalidate();
    }
    void setShowValue(bool show) { if (showValue != show) { showValue = show; invalidate(); } }
    void setLabel(const String& newLabel) { if (label != newLabel) { label = newLabel; invalidate(); } }
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
            int absX2 = getAbsoluteX();
            int absY2 = getAbsoluteY();
            display->setFont(&fonts::efontCN_12);
            display->setTextColor(TFT_WHITE);
            display->setTextSize(1);

            int headerH = (label.length() > 0 || showValue) ? 10 : 0;
            int trackAreaY = absY2 + headerH;
            int trackAreaH = height - headerH;
            if (trackAreaH < 6) {
                headerH = 0;
                trackAreaY = absY2;
                trackAreaH = height;
            }

            if (headerH > 0) {
                int headerY = absY2 + 1;
                if (label.length() > 0) {
                    display->setCursor(absX2 + 1, headerY);
                    display->print(label);
                }
                if (showValue) {
                    String valueText = String(currentValue);
                    int textWidth = valueText.length() * 6;
                    display->setCursor(absX2 + width - textWidth - 2, headerY);
                    display->print(valueText);
                }
            }

            int trackY = trackAreaY + (trackAreaH - 4) / 2;
            int trackHeight = 4;
            uint16_t borderColor = focused ? focusColor : TFT_WHITE;
            display->drawRect(absX2, trackY, width, trackHeight, borderColor);
            display->fillRect(absX2 + 1, trackY + 1, width - 2, trackHeight - 2, trackColor);
            int range = maxValue - minValue;
            int thumbX = absX2;
            if (range > 0) {
                thumbX = absX2 + ((currentValue - minValue) * (width - 8)) / range;
            }
            if (thumbX < absX2) thumbX = absX2;
            if (thumbX > absX2 + width - 8) thumbX = absX2 + width - 8;
            uint16_t currentThumbColor = focused ? focusColor : thumbColor;
            display->fillRect(thumbX, trackAreaY, 8, trackAreaH, currentThumbColor);
            display->drawRect(thumbX, trackAreaY, 8, trackAreaH, TFT_BLACK);
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

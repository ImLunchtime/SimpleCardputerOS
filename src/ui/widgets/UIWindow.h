#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
#include <smooth_ui_toolkit.hpp>
class UIWindow : public UIWidget {
private:
    String title;
    uint16_t borderColor;
    int childOffsetX;
    int childOffsetY;
    smooth_ui_toolkit::AnimateValue animProgress;
    bool animating;
    bool opening;
    bool closing;
    bool contentVisible;
    bool lastVisibleState;
    void computeAnimatedRect(int& outX, int& outY, int& outW, int& outH) {
        int targetX = getAbsoluteX();
        int targetY = getAbsoluteY();
        int targetW = width;
        int targetH = height;
        if (!animating) {
            outX = targetX;
            outY = targetY;
            outW = targetW;
            outH = targetH;
            return;
        }
        float p = animProgress.directValue();
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
        int startW = targetW / 4;
        int startH = targetH / 4;
        if (startW < 10) startW = targetW / 2;
        if (startH < 10) startH = targetH / 2;
        int startX = 0;
        int startY = 0;
        int currentW = startW + (int)((float)(targetW - startW) * p);
        int currentH = startH + (int)((float)(targetH - startH) * p);
        int currentX = startX + (int)((float)(targetX - startX) * p);
        int currentY = startY + (int)((float)(targetY - startY) * p);
        outW = currentW;
        outH = currentH;
        outX = currentX;
        outY = currentY;
    }
    void startOpenAnimation() {
        animating = true;
        opening = true;
        closing = false;
        contentVisible = false;
        animProgress.teleport(0.0f);
        auto& spring = animProgress.springOptions();
        spring.visualDuration = 1.2f; 
        spring.bounce = 0.3f;
        animProgress.move(1.0f);
    }
    void startCloseAnimation() {
        animating = true;
        opening = false;
        closing = true;
        contentVisible = false;
        animProgress.teleport(1.0f);
        auto& spring = animProgress.springOptions();
        spring.visualDuration = 0.15f;
        spring.bounce = 0.8f;
        animProgress.move(0.0f);
    }
public:
    UIWindow(int id, int x, int y, int width, int height, const String& title = "", const String& name = "")
        : UIWidget(id, WIDGET_WINDOW, x, y, width, height, name, false),
          title(title), borderColor(TFT_WHITE), childOffsetX(-6), childOffsetY(-6),
          animProgress(0.0f), animating(false), opening(false), closing(false), contentVisible(false), lastVisibleState(false) {}
    int getChildOffsetX() const override { return childOffsetX; }
    int getChildOffsetY() const override { return childOffsetY; }
    void setChildOffset(int ox, int oy) { childOffsetX = ox; childOffsetY = oy; }
    void setTitle(const String& newTitle) { title = newTitle; }
    void setBorderColor(uint16_t color) { borderColor = color; }
    bool shouldDrawContent() const { return contentVisible; }
    bool isAnimating() const { return animating; }
    bool shouldDrawWindow() const { return visible || animating; }
    void draw(LovyanGFX* display) override {
        if (!display) return;
        if (!visible && !animating) return;
        if (visible && !animating && !lastVisibleState) return;
        int drawX;
        int drawY;
        int drawW;
        int drawH;
        computeAnimatedRect(drawX, drawY, drawW, drawH);
        Theme* theme = getCurrentTheme();
        if (theme) {
            ThemeDrawParams params;
            params.display = display;
            params.x = drawX;
            params.y = drawY;
            params.width = drawW;
            params.height = drawH;
            params.visible = visible;
            params.text = title;
            params.textColor = TFT_WHITE;
            params.borderColor = borderColor;
            params.backgroundColor = TFT_BLACK;
            theme->drawWindow(params);
        } else {
            display->fillRect(drawX, drawY, drawW, drawH, TFT_BLACK);
            display->drawRect(drawX, drawY, drawW, drawH, borderColor);
            if (!title.isEmpty()) {
                display->setFont(&fonts::efontCN_12);
                display->setTextColor(TFT_WHITE);
                display->setTextSize(1);
                display->setCursor(drawX + 5, drawY + 3);
                display->print(title);
            }
        }
    }
    void drawPartial(LovyanGFX* display) override {
        if (!display) return;
        if (!visible && !animating) return;
        draw(display);
    }
    void clearAppArea(LovyanGFX* display) override {
        if (!display) return;
        return;
    }
    void drawAppPartial(LovyanGFX* display) override {
        if (!display) return;
        if (!visible && !animating) return;
        draw(display);
    }
    bool handleKeyEvent(const KeyEvent& event) override {
        return false;
    }
    bool update(uint32_t nowMs) override {
        bool currentVisible = visible;
        if (!lastVisibleState && currentVisible && !animating) {
            startOpenAnimation();
        } else if (lastVisibleState && !currentVisible && !animating) {
            startCloseAnimation();
        }
        lastVisibleState = currentVisible;
        if (!animating) return false;
        float t = (float)nowMs / 1000.0f;
        animProgress.update(t);
        float p = animProgress.directValue();
        if (opening) {
            if (p >= 0.99f || animProgress.currentPlayingState() == smooth_ui_toolkit::AnimateState::Completed) {
                animating = false;
                opening = false;
                contentVisible = true;
            }
        } else if (closing) {
            if (p <= 0.01f || animProgress.currentPlayingState() == smooth_ui_toolkit::AnimateState::Completed) {
                animating = false;
                closing = false;
                contentVisible = false;
            }
        } else {
            if (animProgress.currentPlayingState() == smooth_ui_toolkit::AnimateState::Completed) {
                animating = false;
            }
        }
        return true;
    }
};

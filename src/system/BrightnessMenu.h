#pragma once
#include <M5Cardputer.h>
#include "ui/UIManager.h"

extern int g_displayBrightness;

class BrightnessMenu {
private:
    UIManager* uiManager;
    static constexpr int BRIGHTNESS_PANEL_ID = -2000;
    static constexpr int BRIGHTNESS_SLIDER_ID = -2001;
    class BrightnessSlider : public UISlider {
    public:
        BrightnessSlider(int id, int x, int y, int width, int height, const String& name)
            : UISlider(id, x, y, width, height, 5, 100, 50, "Brightness", name) {}
        void onValueChanged(int newValue) override {
            int brightness = (newValue * 255) / 100;
            if (brightness < 0) brightness = 0;
            if (brightness > 255) brightness = 255;
            M5Cardputer.Display.setBrightness(brightness);
            g_displayBrightness = brightness;
        }
    };
    void ensureWidgets() {
        if (!uiManager) return;
        UIWidget* panelWidget = uiManager->getWidget(BRIGHTNESS_PANEL_ID);
        UIPanel* panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        UIWidget* sliderWidget = uiManager->getWidget(BRIGHTNESS_SLIDER_ID);
        UISlider* sliderBase = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
        if (!panel) {
            int screenW = M5Cardputer.Display.width();
            int screenH = M5Cardputer.Display.height();
            if (screenW <= 0) screenW = 240;
            if (screenH <= 0) screenH = 135;
            int panelW = screenW - 60;
            if (panelW < 100) panelW = screenW - 20;
            int panelH = 40;
            int panelX = (screenW - panelW) / 2;
            int panelY = (screenH - panelH) / 2;
            panel = uiManager->createPanel(BRIGHTNESS_PANEL_ID, panelX, panelY, panelW, panelH, "BrightnessPanel");
            panel->setBackgroundColor(TFT_BLACK);
            panel->setBorderColor(TFT_WHITE);
        }
        if (!sliderBase && panel) {
            int panelX, panelY, panelW, panelH;
            panel->getBounds(panelX, panelY, panelW, panelH);
            int sliderW = panelW - 20;
            int sliderH = panelH - 16;
            if (sliderH < 12) sliderH = panelH - 8;
            if (sliderH < 8) sliderH = panelH;
            int sliderX = 10;
            int sliderY = (panelH - sliderH) / 2;
            BrightnessSlider* slider = new BrightnessSlider(BRIGHTNESS_SLIDER_ID, sliderX, sliderY, sliderW, sliderH, "BrightnessSlider");
            slider->setParent(panel);
            uiManager->addWidget(slider);
            slider->onValueChanged(slider->getValue());
        }
        panelWidget = uiManager->getWidget(BRIGHTNESS_PANEL_ID);
        sliderWidget = uiManager->getWidget(BRIGHTNESS_SLIDER_ID);
        panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        sliderBase = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
        if (panel) {
            uiManager->ensureForeground(panel);
        }
        if (sliderBase) {
            uiManager->ensureForeground(sliderBase);
        }
        if (panel) {
            uiManager->setWidgetTreeVisible(panel, false);
        }
    }
public:
    explicit BrightnessMenu(UIManager* manager) : uiManager(manager) {}
    void setUIManager(UIManager* manager) {
        uiManager = manager;
    }
    bool handleKeyEvent(const KeyEvent& event) {
        if (!uiManager) return false;
        UIWidget* panelWidget = uiManager->getWidget(BRIGHTNESS_PANEL_ID);
        UIPanel* panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        UIWidget* sliderWidget = uiManager->getWidget(BRIGHTNESS_SLIDER_ID);
        UISlider* slider = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
        bool panelVisible = panel && panel->isVisible();
        if (event.opt) {
            if (!panel || !slider) {
                ensureWidgets();
                panelWidget = uiManager->getWidget(BRIGHTNESS_PANEL_ID);
                panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
                sliderWidget = uiManager->getWidget(BRIGHTNESS_SLIDER_ID);
                slider = sliderWidget ? static_cast<UISlider*>(sliderWidget) : nullptr;
            }
            if (panel) {
                uiManager->ensureForeground(panel);
            }
            if (slider) {
                uiManager->ensureForeground(slider);
            }
            if (panel) {
                bool currentlyVisible = panel->isVisible();
                uiManager->setWidgetTreeVisible(panel, !currentlyVisible);
                uiManager->rebuildForegroundFocus();
                uiManager->smartRefresh();
            }
            return true;
        }
        if (panelVisible && slider) {
            bool handled = false;
            int value = slider->getValue();
            if (event.left) {
                value -= 5;
                if (value < 5) value = 5;
                slider->setValue(value);
                handled = true;
            } else if (event.right) {
                value += 5;
                if (value > 100) value = 100;
                slider->setValue(value);
                handled = true;
            }
            if (event.enter || event.esc) {
                uiManager->setWidgetTreeVisible(panel, false);
                uiManager->rebuildForegroundFocus();
                uiManager->smartRefresh();
                handled = true;
            }
            if (handled) {
                return true;
            }
        }
        return false;
    }
};

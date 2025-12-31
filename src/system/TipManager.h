#pragma once
#include <M5Cardputer.h>
#include "ui/UIManager.h"

class TipManager {
private:
    UIManager* uiManager;
    static constexpr int TIP_PANEL_ID = -2100;
    static constexpr int TIP_LABEL1_ID = -2101;
    static constexpr int TIP_LABEL2_ID = -2102;
    static constexpr int TIP_PROGRESS_ID = -2103;
    static constexpr int TIP_BUTTON_ID = -2104;
    enum TipMode {
        TIP_NONE,
        TIP_TWO_LABELS,
        TIP_PROGRESS,
        TIP_ERROR
    };
    class TipProgressSlider : public UISlider {
    public:
        TipProgressSlider(int id, int x, int y, int width, int height, const String& name)
            : UISlider(id, x, y, width, height, 0, 100, 0, "", name) {}
        bool handleKeyEvent(const KeyEvent& event) override {
            return false;
        }
    };
    TipMode currentMode;
    bool visible;
    bool userClosable;
    uint32_t autoCloseMs;
    uint32_t showTimeMs;
    void ensurePanel() {
        if (!uiManager) return;
        UIWidget* panelWidget = uiManager->getWidget(TIP_PANEL_ID);
        UIPanel* panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        if (!panel) {
            int screenW = M5Cardputer.Display.width();
            int screenH = M5Cardputer.Display.height();
            if (screenW <= 0) screenW = 240;
            if (screenH <= 0) screenH = 135;
            int panelW = screenW - 60;
            if (panelW < 120) panelW = screenW - 20;
            int panelH = 70;
            int panelX = (screenW - panelW) / 2;
            int panelY = (screenH - panelH) / 2;
            panel = uiManager->createPanel(TIP_PANEL_ID, panelX, panelY, panelW, panelH, "TipPanel");
            panel->setBackgroundColor(TFT_BLACK);
            panel->setBorderColor(TFT_WHITE);
        }
        panelWidget = uiManager->getWidget(TIP_PANEL_ID);
        panel = panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
        if (panel) {
            uiManager->ensureForeground(panel);
            uiManager->setWidgetTreeVisible(panel, false);
        }
    }
    UIPanel* getPanel() {
        if (!uiManager) return nullptr;
        UIWidget* panelWidget = uiManager->getWidget(TIP_PANEL_ID);
        return panelWidget ? static_cast<UIPanel*>(panelWidget) : nullptr;
    }
    UILabel* ensureLabel1() {
        if (!uiManager) return nullptr;
        UIWidget* labelWidget = uiManager->getWidget(TIP_LABEL1_ID);
        UILabel* label = labelWidget ? static_cast<UILabel*>(labelWidget) : nullptr;
        UIPanel* panel = getPanel();
        if (!label && panel) {
            int panelX, panelY, panelW, panelH;
            panel->getBounds(panelX, panelY, panelW, panelH);
            int labelX = 10;
            int labelY = 10;
            label = uiManager->createLabel(TIP_LABEL1_ID, labelX, labelY, "", "TipLabel1", panel);
        }
        return label;
    }
    UILabel* ensureLabel2() {
        if (!uiManager) return nullptr;
        UIWidget* labelWidget = uiManager->getWidget(TIP_LABEL2_ID);
        UILabel* label = labelWidget ? static_cast<UILabel*>(labelWidget) : nullptr;
        UIPanel* panel = getPanel();
        if (!label && panel) {
            int panelX, panelY, panelW, panelH;
            panel->getBounds(panelX, panelY, panelW, panelH);
            int labelX = 10;
            int labelY = panelH / 2;
            label = uiManager->createLabel(TIP_LABEL2_ID, labelX, labelY, "", "TipLabel2", panel);
        }
        return label;
    }
    TipProgressSlider* ensureProgress() {
        if (!uiManager) return nullptr;
        UIWidget* sliderWidget = uiManager->getWidget(TIP_PROGRESS_ID);
        TipProgressSlider* slider = sliderWidget ? static_cast<TipProgressSlider*>(sliderWidget) : nullptr;
        UIPanel* panel = getPanel();
        if (!slider && panel) {
            int panelX, panelY, panelW, panelH;
            panel->getBounds(panelX, panelY, panelW, panelH);
            int sliderW = panelW - 20;
            int sliderH = 18;
            int sliderX = 10;
            int sliderY = panelH - sliderH - 10;
            slider = new TipProgressSlider(TIP_PROGRESS_ID, sliderX, sliderY, sliderW, sliderH, "TipProgress");
            slider->setParent(panel);
            slider->setShowValue(false);
            uiManager->addWidget(slider);
        }
        return slider;
    }
    UIButton* ensureButton() {
        if (!uiManager) return nullptr;
        UIWidget* buttonWidget = uiManager->getWidget(TIP_BUTTON_ID);
        UIButton* button = buttonWidget ? static_cast<UIButton*>(buttonWidget) : nullptr;
        UIPanel* panel = getPanel();
        if (!button && panel) {
            int panelX, panelY, panelW, panelH;
            panel->getBounds(panelX, panelY, panelW, panelH);
            int btnW = 60;
            int btnH = 20;
            int btnX = (panelW - btnW) / 2;
            int btnY = panelH - btnH - 10;
            button = uiManager->createButton(TIP_BUTTON_ID, btnX, btnY, btnW, btnH, "OK", "TipOk", panel);
        }
        return button;
    }
    void internalShowCommon(TipMode mode, bool closable, uint32_t timeoutMs) {
        if (!uiManager) return;
        ensurePanel();
        UIPanel* panel = getPanel();
        if (!panel) return;
        currentMode = mode;
        visible = true;
        userClosable = closable;
        autoCloseMs = timeoutMs;
        showTimeMs = autoCloseMs > 0 ? millis() : 0;
        uiManager->setWidgetTreeVisible(panel, true);
        uiManager->ensureForeground(panel);
        uiManager->rebuildForegroundFocus();
        uiManager->smartRefresh();
    }
public:
    explicit TipManager(UIManager* manager)
        : uiManager(manager), currentMode(TIP_NONE), visible(false), userClosable(false), autoCloseMs(0), showTimeMs(0) {}
    void setUIManager(UIManager* manager) {
        uiManager = manager;
    }
    void showTwoLabel(const String& line1, const String& line2, uint32_t timeoutMs = 0, bool closable = false) {
        if (!uiManager) return;
        ensurePanel();
        UILabel* label1 = ensureLabel1();
        UILabel* label2 = ensureLabel2();
        if (label1) {
            label1->setText(line1);
            label1->setVisible(true);
        }
        if (label2) {
            label2->setText(line2);
            label2->setVisible(true);
        }
        UIWidget* progressWidget = uiManager->getWidget(TIP_PROGRESS_ID);
        if (progressWidget) {
            progressWidget->setVisible(false);
        }
        UIWidget* buttonWidget = uiManager->getWidget(TIP_BUTTON_ID);
        if (buttonWidget) {
            buttonWidget->setVisible(false);
        }
        internalShowCommon(TIP_TWO_LABELS, closable, timeoutMs);
    }
    void showProgress(const String& text, int min, int max, int value, uint32_t timeoutMs = 0, bool closable = false) {
        if (!uiManager) return;
        ensurePanel();
        UILabel* label1 = ensureLabel1();
        if (label1) {
            label1->setText(text);
            label1->setVisible(true);
        }
        UILabel* label2 = ensureLabel2();
        if (label2) {
            label2->setVisible(false);
        }
        TipProgressSlider* slider = ensureProgress();
        if (slider) {
            slider->setRange(min, max);
            slider->setValue(value);
            slider->setVisible(true);
        }
        UIWidget* buttonWidget = uiManager->getWidget(TIP_BUTTON_ID);
        if (buttonWidget) {
            buttonWidget->setVisible(false);
        }
        internalShowCommon(TIP_PROGRESS, closable, timeoutMs);
    }
    void updateProgress(int value) {
        if (!uiManager) return;
        if (!visible || currentMode != TIP_PROGRESS) return;
        UIWidget* sliderWidget = uiManager->getWidget(TIP_PROGRESS_ID);
        TipProgressSlider* slider = sliderWidget ? static_cast<TipProgressSlider*>(sliderWidget) : nullptr;
        if (slider) {
            slider->setValue(value);
        }
    }
    void showError(const String& text, uint32_t timeoutMs = 0, bool closable = true) {
        if (!uiManager) return;
        ensurePanel();
        UILabel* label1 = ensureLabel1();
        if (label1) {
            label1->setText(text);
            label1->setVisible(true);
        }
        UILabel* label2 = ensureLabel2();
        if (label2) {
            label2->setVisible(false);
        }
        UIWidget* progressWidget = uiManager->getWidget(TIP_PROGRESS_ID);
        if (progressWidget) {
            progressWidget->setVisible(false);
        }
        UIButton* button = ensureButton();
        if (button) {
            button->setVisible(true);
        }
        internalShowCommon(TIP_ERROR, closable, timeoutMs);
    }
    void closeTip() {
        if (!uiManager) return;
        UIPanel* panel = getPanel();
        if (!panel) {
            visible = false;
            currentMode = TIP_NONE;
            return;
        }
        uiManager->setWidgetTreeVisible(panel, false);
        uiManager->rebuildForegroundFocus();
        uiManager->smartRefresh();
        visible = false;
        currentMode = TIP_NONE;
        autoCloseMs = 0;
        showTimeMs = 0;
    }
    bool handleKeyEvent(const KeyEvent& event) {
        if (!visible) return false;
        if (currentMode == TIP_ERROR) {
            if (event.enter || event.esc) {
                closeTip();
                return true;
            }
            return true;
        }
        if (userClosable && (event.enter || event.esc)) {
            closeTip();
            return true;
        }
        return true;
    }
    void update() {
        if (!visible) return;
        if (autoCloseMs == 0) return;
        uint32_t now = millis();
        if (now - showTimeMs >= autoCloseMs) {
            closeTip();
        }
    }
};


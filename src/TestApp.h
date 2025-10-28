#pragma once
#include "App.h"
#include "Graphics.h"
#include "EventSystem.h"
#include "FocusManager.h"

class TestApp : public App {
private:
    String inputText;
    Graphics* graphics;
    EventSystem* eventSystem;
    FocusManager focusManager;
    
    // 控件ID定义
    enum ControlIds {
        BUTTON1_ID = 1,
        BUTTON2_ID = 2,
        BUTTON3_ID = 3,
        INPUT_ID = 4
    };

public:
    TestApp(Graphics* gfx, EventSystem* events) 
        : graphics(gfx), eventSystem(events), inputText("") {}

    void setup() override {
        // 初始化焦点管理器
        focusManager.clear();
        focusManager.addItem(BUTTON1_ID, FOCUSABLE_BUTTON, 20, 55, 60, 20, "Button1");
        focusManager.addItem(BUTTON2_ID, FOCUSABLE_BUTTON, 90, 55, 60, 20, "Button2");
        focusManager.addItem(BUTTON3_ID, FOCUSABLE_BUTTON, 160, 55, 60, 20, "Button3");
        focusManager.addItem(INPUT_ID, FOCUSABLE_INPUT, 20, 85, 200, 35, "Input");
        
        drawInterface();
    }

    void loop() override {
        // 主循环逻辑
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 处理Tab键切换焦点
        if (event.tab) {
            focusManager.nextFocus();
            drawInterface();
            return;
        }
        
        // 处理回车键（按钮点击）
        if (event.enter) {
            if (focusManager.getCurrentFocusType() == FOCUSABLE_BUTTON) {
                handleButtonClick(focusManager.getCurrentFocusId());
                return;
            }
            // 如果在输入框中，回车键添加换行
            else if (focusManager.getCurrentFocusType() == FOCUSABLE_INPUT) {
                inputText += "\n";
                drawInterface();
                return;
            }
        }
        
        // 只有在输入框聚焦时才处理文本输入
        if (focusManager.getCurrentFocusType() == FOCUSABLE_INPUT) {
            if (event.del) {
                // 删除最后一个字符
                if (inputText.length() > 0) {
                    inputText.remove(inputText.length() - 1);
                }
            } else if (event.text.length() > 0) {
                // 添加输入的文本
                inputText += event.text;
            }
            drawInterface();
        }
    }

private:
    void handleButtonClick(int buttonId) {
        switch (buttonId) {
            case BUTTON1_ID:
                inputText += "[Button1 Clicked] ";
                break;
            case BUTTON2_ID:
                inputText += "[Button2 Clicked] ";
                break;
            case BUTTON3_ID:
                inputText += "[Button3 Clicked] ";
                break;
        }
        drawInterface();
    }
    
    void drawInterface() {
        graphics->clear();
        
        // 绘制主窗口
        graphics->drawWindow(10, 10, 220, 120, TFT_WHITE);
        
        // 绘制标题
        graphics->drawLabel(20, 20, "M5Stack Cardputer OS");
        graphics->drawLabel(20, 35, "Focus Demo - Tab to switch");
        
        // 绘制按钮示例（带焦点状态）
        graphics->drawButton(20, 55, 60, 20, "Button1", TFT_BLUE, focusManager.isFocused(BUTTON1_ID));
        graphics->drawButton(90, 55, 60, 20, "Button2", TFT_GREEN, focusManager.isFocused(BUTTON2_ID));
        graphics->drawButton(160, 55, 60, 20, "Button3", TFT_RED, focusManager.isFocused(BUTTON3_ID));
        
        // 绘制输入区域窗口（带焦点状态）
        graphics->drawWindow(20, 85, 200, 35, TFT_YELLOW, focusManager.isFocused(INPUT_ID));
        graphics->drawLabel(25, 90, "Input:");
        
        // 显示输入的文本（限制显示长度）
        String displayText = inputText;
        if (displayText.length() > 25) {
            displayText = displayText.substring(displayText.length() - 25);
        }
        graphics->drawLabel(25, 105, displayText);
        
        // 绘制状态栏，显示当前焦点
        String statusText = "Focus: " + focusManager.getCurrentFocusName();
        if (focusManager.getCurrentFocusType() == FOCUSABLE_INPUT) {
            statusText += " (Type enabled)";
        } else {
            statusText += " (Enter to click)";
        }
        graphics->drawLabel(10, 140, statusText);
    }
};
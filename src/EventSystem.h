#pragma once
#include <M5Cardputer.h>

struct KeyEvent {
    String text;
    bool enter;
    bool del;
    bool tab;  // 添加Tab键检测
};

class EventSystem {
public:
    bool hasKeyEvent(KeyEvent& event) {
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            // 关键：必须先更新键盘状态
            M5Cardputer.Keyboard.updateKeysState();
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            
            // 初始化事件
            event.text = "";
            event.enter = status.enter;
            event.del = status.del;
            event.tab = status.tab;  // 直接使用KeysState中的tab字段
            
            // 构建文本字符串
            for (char c : status.word) {
                event.text += c;
            }
            
            return true;
        }
        return false;
    }
};
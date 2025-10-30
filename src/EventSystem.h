#pragma once
#include <M5Cardputer.h>

struct KeyEvent {
    String text;
    bool enter;
    bool del;
    bool tab;
    bool up;      // ";" 键
    bool down;    // "." 键
    bool left;    // "," 键
    bool right;   // "/" 键
    bool esc;     // ESC 键
};

class EventSystem {
public:
    bool hasKeyEvent(KeyEvent& event) {
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            // 关键：先更新键盘状态
            M5Cardputer.Keyboard.updateKeysState();
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            
            // 初始化事件
            event.text = "";
            event.enter = status.enter;
            event.del = status.del;
            event.tab = status.tab;
            event.up = false;
            event.down = false;
            event.left = false;
            event.right = false;
            event.esc = false;
            
            // 构建文本字符串并检测方向键和ESC键
            for (char c : status.word) {
                if (c == ';') {
                    event.up = true;
                } else if (c == '.') {
                    event.down = true;
                } else if (c == ',') {
                    event.left = true;
                } else if (c == '/') {
                    event.right = true;
                } else if (c == '`') {
                    event.esc = true;  // 使用"`"字符作为ESC键
                } else {
                    event.text += c;  // 只有非方向键和非ESC键字符才加入text
                }
            }
            
            return true;
        }
        return false;
    }
};
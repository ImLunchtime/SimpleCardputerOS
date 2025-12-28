#pragma once
#include <M5Cardputer.h>

struct KeyEvent {
    String text;
    bool enter;
    bool opt;
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
        if (!M5Cardputer.Keyboard.isChange()) {
            return false;
        }
        M5Cardputer.Keyboard.updateKeysState();
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        event.text = "";
        event.enter = status.enter;
        event.opt = status.opt;
        event.del = status.del;
        event.tab = status.tab;
        event.up = false;
        event.down = false;
        event.left = false;
        event.right = false;
        event.esc = false;

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
                event.esc = true;
            } else {
                event.text += c;
            }
        }

        return true;
    }
};

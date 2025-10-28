#pragma once

enum FocusableType {
    FOCUSABLE_BUTTON,
    FOCUSABLE_INPUT
};

struct FocusableItem {
    int id;
    FocusableType type;
    int x, y, width, height;
    String name;
    
    FocusableItem(int _id, FocusableType _type, int _x, int _y, int _w, int _h, const String& _name)
        : id(_id), type(_type), x(_x), y(_y), width(_w), height(_h), name(_name) {}
};

class FocusManager {
private:
    FocusableItem* items[10];  // 最多支持10个可聚焦控件
    int itemCount;
    int currentFocus;

public:
    FocusManager() : itemCount(0), currentFocus(0) {
        for (int i = 0; i < 10; i++) {
            items[i] = nullptr;
        }
    }
    
    ~FocusManager() {
        clear();
    }
    
    void clear() {
        for (int i = 0; i < itemCount; i++) {
            if (items[i]) {
                delete items[i];
                items[i] = nullptr;
            }
        }
        itemCount = 0;
        currentFocus = 0;
    }
    
    void addItem(int id, FocusableType type, int x, int y, int width, int height, const String& name) {
        if (itemCount < 10) {
            items[itemCount] = new FocusableItem(id, type, x, y, width, height, name);
            itemCount++;
        }
    }
    
    void nextFocus() {
        if (itemCount > 0) {
            currentFocus = (currentFocus + 1) % itemCount;
        }
    }
    
    void previousFocus() {
        if (itemCount > 0) {
            currentFocus = (currentFocus - 1 + itemCount) % itemCount;
        }
    }
    
    int getCurrentFocusId() {
        if (itemCount > 0 && currentFocus < itemCount) {
            return items[currentFocus]->id;
        }
        return -1;
    }
    
    FocusableType getCurrentFocusType() {
        if (itemCount > 0 && currentFocus < itemCount) {
            return items[currentFocus]->type;
        }
        return FOCUSABLE_BUTTON;
    }
    
    bool isFocused(int id) {
        return getCurrentFocusId() == id;
    }
    
    String getCurrentFocusName() {
        if (itemCount > 0 && currentFocus < itemCount) {
            return items[currentFocus]->name;
        }
        return "";
    }
};
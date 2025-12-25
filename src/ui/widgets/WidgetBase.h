#pragma once
#include <M5Cardputer.h>
#include "system/EventSystem.h"
#include "themes/ThemeManager.h"
enum UIWidgetType {
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_WINDOW,
    WIDGET_MENU_LIST,
    WIDGET_MENU_GRID,
    WIDGET_SLIDER,
    WIDGET_IMAGE,
    WIDGET_SCREEN
};
class UIWidget {
protected:
    int id;
    UIWidgetType type;
    int x, y, width, height;
    String name;
    bool visible;
    bool focusable;
    bool focused;
    UIWidget* parent;
public:
    UIWidget(int _id, UIWidgetType _type, int _x, int _y, int _w, int _h, const String& _name, bool _focusable = false)
        : id(_id), type(_type), x(_x), y(_y), width(_w), height(_h), name(_name),
          visible(true), focusable(_focusable), focused(false), parent(nullptr) {}
    virtual ~UIWidget() {}
    int getId() const { return id; }
    UIWidgetType getType() const { return type; }
    String getName() const { return name; }
    bool isVisible() const { return visible; }
    bool isFocusable() const { return focusable; }
    bool isFocused() const { return focused; }
    void setVisible(bool _visible) { visible = _visible; }
    void setFocused(bool _focused) { focused = _focused; }
    void setParent(UIWidget* p) { parent = p; }
    UIWidget* getParent() const { return parent; }
    void setPosition(int _x, int _y) { x = _x; y = _y; }
    void setSize(int _w, int _h) { width = _w; height = _h; }
    void getBounds(int& _x, int& _y, int& _w, int& _h) const {
        _x = x; _y = y; _w = width; _h = height;
    }
    virtual int getChildOffsetX() const { return 0; }
    virtual int getChildOffsetY() const { return 0; }
    int getAbsoluteX() const { return parent ? parent->getAbsoluteX() + parent->getChildOffsetX() + x : x; }
    int getAbsoluteY() const { return parent ? parent->getAbsoluteY() + parent->getChildOffsetY() + y : y; }
    void getAbsoluteBounds(int& _x, int& _y, int& _w, int& _h) const {
        _x = getAbsoluteX();
        _y = getAbsoluteY();
        _w = width;
        _h = height;
    }
    virtual void draw(LGFX_Device* display) = 0;
    virtual bool handleKeyEvent(const KeyEvent& event) = 0;
    virtual void onFocusChanged(bool hasFocus) {}
    virtual void clearArea(LGFX_Device* display) {
        int ax, ay, aw, ah;
        getAbsoluteBounds(ax, ay, aw, ah);
        display->fillRect(ax, ay, aw, ah, TFT_BLACK);
    }
    virtual void drawPartial(LGFX_Device* display) {
        clearArea(display);
        draw(display);
    }
    virtual void clearAppArea(LGFX_Device* display) {
        if (type == WIDGET_WINDOW) {
            int ax, ay, aw, ah;
            getAbsoluteBounds(ax, ay, aw, ah);
            display->fillRect(ax, ay, aw, ah, TFT_BLACK);
        } else {
            clearArea(display);
        }
    }
    virtual void drawAppPartial(LGFX_Device* display) {
        clearAppArea(display);
        draw(display);
    }
    virtual bool hasSecondaryFocus() const { return false; }
    virtual bool handleSecondaryKeyEvent(const KeyEvent& event) { return false; }
};

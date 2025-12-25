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
    bool dirty;
    bool hasLastDrawBounds;
    int lastDrawX;
    int lastDrawY;
    int lastDrawW;
    int lastDrawH;
public:
    UIWidget(int _id, UIWidgetType _type, int _x, int _y, int _w, int _h, const String& _name, bool _focusable = false)
        : id(_id), type(_type), x(_x), y(_y), width(_w), height(_h), name(_name),
          visible(true), focusable(_focusable), focused(false), parent(nullptr),
          dirty(true), hasLastDrawBounds(false), lastDrawX(0), lastDrawY(0), lastDrawW(0), lastDrawH(0) {}
    virtual ~UIWidget() {}
    int getId() const { return id; }
    UIWidgetType getType() const { return type; }
    String getName() const { return name; }
    bool isVisible() const { return visible; }
    bool isFocusable() const { return focusable; }
    bool isFocused() const { return focused; }
    void setVisible(bool _visible) { if (visible != _visible) { visible = _visible; invalidate(); } }
    void setFocused(bool _focused) { if (focused != _focused) { focused = _focused; invalidate(); } }
    void setParent(UIWidget* p) { parent = p; }
    UIWidget* getParent() const { return parent; }
    void setPosition(int _x, int _y) { if (x != _x || y != _y) { x = _x; y = _y; invalidate(); } }
    void setSize(int _w, int _h) { if (width != _w || height != _h) { width = _w; height = _h; invalidate(); } }
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
    bool isDirty() const { return dirty; }
    void invalidate() { dirty = true; }
    void markDrawn() {
        dirty = false;
        hasLastDrawBounds = true;
        getAbsoluteBounds(lastDrawX, lastDrawY, lastDrawW, lastDrawH);
    }
    void getDirtyBounds(int& outX, int& outY, int& outW, int& outH) const {
        int cx, cy, cw, ch;
        getAbsoluteBounds(cx, cy, cw, ch);
        if (!hasLastDrawBounds) {
            outX = cx;
            outY = cy;
            outW = cw;
            outH = ch;
            return;
        }
        int nx = min(cx, lastDrawX);
        int ny = min(cy, lastDrawY);
        int rx = max(cx + cw, lastDrawX + lastDrawW);
        int by = max(cy + ch, lastDrawY + lastDrawH);
        outX = nx;
        outY = ny;
        outW = rx - nx;
        outH = by - ny;
    }
    virtual void draw(LGFX_Device* display) = 0;
    virtual bool handleKeyEvent(const KeyEvent& event) = 0;
    virtual void onFocusChanged(bool hasFocus) {}
    virtual bool update(uint32_t nowMs) { return false; }
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

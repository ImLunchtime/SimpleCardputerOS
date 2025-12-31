#include "UIManager.h"
#include <lgfx/v1/LGFX_Sprite.hpp>
static bool rectIntersects(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    if (aw <= 0 || ah <= 0 || bw <= 0 || bh <= 0) return false;
    int ar = ax + aw;
    int ab = ay + ah;
    int br = bx + bw;
    int bb = by + bh;
    return !(ar <= bx || br <= ax || ab <= by || bb <= ay);
}

static bool intersectRects(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh, int& outX, int& outY, int& outW, int& outH) {
    int nx = max(ax, bx);
    int ny = max(ay, by);
    int rx = min(ax + aw, bx + bw);
    int by2 = min(ay + ah, by + bh);
    int nw = rx - nx;
    int nh = by2 - ny;
    if (nw <= 0 || nh <= 0) return false;
    outX = nx;
    outY = ny;
    outW = nw;
    outH = nh;
    return true;
}

bool UIManager::computeClipRect(UIWidget* widget, int& outX, int& outY, int& outW, int& outH) {
    if (!widget) return false;
    int ax, ay, aw, ah;
    widget->getAbsoluteBounds(ax, ay, aw, ah);
    int cx = ax;
    int cy = ay;
    int cw = aw;
    int ch = ah;
    UIWidget* p = widget->getParent();
    while (p) {
        int px, py, pw, ph;
        p->getAbsoluteBounds(px, py, pw, ph);
        int nx = max(cx, px);
        int ny = max(cy, py);
        int rx = min(cx + cw, px + pw);
        int by = min(cy + ch, py + ph);
        int nw = rx - nx;
        int nh = by - ny;
        if (nw <= 0 || nh <= 0) return false;
        cx = nx;
        cy = ny;
        cw = nw;
        ch = nh;
        p = p->getParent();
    }
    outX = cx;
    outY = cy;
    outW = cw;
    outH = ch;
    return true;
}

static bool isWidgetAndParentsVisibleForDraw(UIWidget* widget) {
    if (!widget) return false;
    bool selfVisible = false;
    if (widget->getType() == WIDGET_WINDOW) {
        UIWindow* window = static_cast<UIWindow*>(widget);
        selfVisible = window->shouldDrawWindow();
    } else {
        selfVisible = widget->isVisible();
    }
    if (!selfVisible) return false;
    UIWidget* p = widget->getParent();
    while (p) {
        if (!p->isVisible()) return false;
        if (p->getType() == WIDGET_WINDOW) {
            UIWindow* pw = static_cast<UIWindow*>(p);
            if (!pw->shouldDrawWindow() || !pw->shouldDrawContent()) return false;
        }
        p = p->getParent();
    }
    return true;
}

void UIManager::drawWidgetClipped(UIWidget* widget, bool partial) {
    if (!display || !widget) return;
    int cx, cy, cw, ch;
    if (!computeClipRect(widget, cx, cy, cw, ch)) return;
    display->setClipRect(cx, cy, cw, ch);
    if (partial) widget->drawPartial(display);
    else widget->draw(display);
    display->clearClipRect();
    widget->markDrawn();
}

void UIManager::drawWidgetClippedWithExtra(UIWidget* widget, bool partial, int clipX, int clipY, int clipW, int clipH) {
    if (!display || !widget) return;
    int wx, wy, ww, wh;
    if (!computeClipRect(widget, wx, wy, ww, wh)) return;
    int cx, cy, cw, ch;
    if (!intersectRects(wx, wy, ww, wh, clipX, clipY, clipW, clipH, cx, cy, cw, ch)) return;
    display->setClipRect(cx, cy, cw, ch);
    if (partial) widget->drawPartial(display);
    else widget->draw(display);
    display->clearClipRect();
    widget->markDrawn();
}

bool UIManager::flushDirtyInAppArea() {
    if (!hasBackgroundLayer || foregroundWidgetCount <= 0) return false;

    int dirtyX = 0, dirtyY = 0, dirtyW = 0, dirtyH = 0;
    bool hasDirty = false;
    for (int i = 0; i < foregroundWidgetCount; i++) {
        UIWidget* w = foregroundWidgets[i];
        if (!w || !isWidgetAndParentsVisibleForDraw(w) || !w->isDirty()) continue;
        int x, y, ww, hh;
        w->getDirtyBounds(x, y, ww, hh);
        if (!hasDirty) {
            dirtyX = x; dirtyY = y; dirtyW = ww; dirtyH = hh;
            hasDirty = true;
        } else {
            int nx = min(dirtyX, x);
            int ny = min(dirtyY, y);
            int rx = max(dirtyX + dirtyW, x + ww);
            int by = max(dirtyY + dirtyH, y + hh);
            dirtyX = nx;
            dirtyY = ny;
            dirtyW = rx - nx;
            dirtyH = by - ny;
        }
    }
    if (!hasDirty) return false;

    if (display) display->startWrite();

    UIWindow* appWindow = nullptr;
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i] && foregroundWidgets[i]->getType() == WIDGET_WINDOW) {
            appWindow = static_cast<UIWindow*>(foregroundWidgets[i]);
            break;
        }
    }
    if (appWindow && isWidgetAndParentsVisibleForDraw(appWindow)) {
        drawWidgetClippedWithExtra(appWindow, false, dirtyX, dirtyY, dirtyW, dirtyH);
    }

    for (int i = 0; i < foregroundWidgetCount; i++) {
        UIWidget* w = foregroundWidgets[i];
        if (!w || w == appWindow || !isWidgetAndParentsVisibleForDraw(w)) continue;
        int wx, wy, ww, wh;
        w->getDirtyBounds(wx, wy, ww, wh);
        if (!rectIntersects(wx, wy, ww, wh, dirtyX, dirtyY, dirtyW, dirtyH)) continue;
        drawWidgetClippedWithExtra(w, false, dirtyX, dirtyY, dirtyW, dirtyH);
    }

    for (int i = 0; i < foregroundWidgetCount; i++) {
        UIWidget* w = foregroundWidgets[i];
        if (w && w->isDirty()) {
            w->markDrawn();
        }
    }

    if (display) display->endWrite();
    return true;
}

bool UIManager::flushDirtyInRoot() {
    if (hasBackgroundLayer) return false;
    int dirtyX = 0, dirtyY = 0, dirtyW = 0, dirtyH = 0;
    bool hasDirty = false;
    for (int i = 0; i < widgetCount; i++) {
        UIWidget* w = widgets[i];
        if (!w || !isWidgetAndParentsVisibleForDraw(w) || !w->isDirty()) continue;
        int x, y, ww, hh;
        w->getDirtyBounds(x, y, ww, hh);
        if (!hasDirty) {
            dirtyX = x; dirtyY = y; dirtyW = ww; dirtyH = hh;
            hasDirty = true;
        } else {
            int nx = min(dirtyX, x);
            int ny = min(dirtyY, y);
            int rx = max(dirtyX + dirtyW, x + ww);
            int by = max(dirtyY + dirtyH, y + hh);
            dirtyX = nx;
            dirtyY = ny;
            dirtyW = rx - nx;
            dirtyH = by - ny;
        }
    }
    if (!hasDirty) return false;

    if (display) display->startWrite();

    for (int i = 0; i < widgetCount; i++) {
        UIWidget* w = widgets[i];
        if (!w || !isWidgetAndParentsVisibleForDraw(w)) continue;
        int wx, wy, ww, wh;
        w->getDirtyBounds(wx, wy, ww, wh);
        if (!rectIntersects(wx, wy, ww, wh, dirtyX, dirtyY, dirtyW, dirtyH)) continue;
        drawWidgetClippedWithExtra(w, false, dirtyX, dirtyY, dirtyW, dirtyH);
    }

    for (int i = 0; i < widgetCount; i++) {
        UIWidget* w = widgets[i];
        if (w && w->isDirty()) {
            w->markDrawn();
        }
    }

    if (display) display->endWrite();
    return true;
}

UIManager::UIManager() : display(&M5Cardputer.Display), widgetCount(0), currentFocus(-1), focusableCount(0),
                  backgroundWidgetCount(0), foregroundWidgetCount(0), hasBackgroundLayer(false), rootScreen(nullptr), lastAnimationRedrawMs(0), windowCanvas(nullptr) {
    for (int i = 0; i < 20; i++) {
        widgets[i] = nullptr;
        focusableWidgets[i] = -1;
        backgroundWidgets[i] = nullptr;
        foregroundWidgets[i] = nullptr;
    }
    int dw = display ? display->width() : 0;
    int dh = display ? display->height() : 0;
    if (dw <= 0) dw = 240;
    if (dh <= 0) dh = 135;
    rootScreen = new UIScreen(-1000, dw, dh, "RootScreen");
}

UIManager::~UIManager() {
    clear();
    if (rootScreen) { delete rootScreen; rootScreen = nullptr; }
    if (windowCanvas) {
        windowCanvas->deleteSprite();
        delete windowCanvas;
        windowCanvas = nullptr;
    }
}

void UIManager::drawAllOn(LovyanGFX* target) {
    if (!target) return;
    if (hasBackgroundLayer) {
        for (int i = 0; i < backgroundWidgetCount; i++) {
            if (backgroundWidgets[i] && isWidgetAndParentsVisibleForDraw(backgroundWidgets[i])) {
                drawWidgetClippedOn(target, backgroundWidgets[i], false);
            }
        }
    }
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i] && isWidgetAndParentsVisibleForDraw(foregroundWidgets[i])) {
            drawWidgetClippedOn(target, foregroundWidgets[i], false);
        }
    }
    if (!hasBackgroundLayer && foregroundWidgetCount == 0) {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && isWidgetAndParentsVisibleForDraw(widgets[i])) {
                drawWidgetClippedOn(target, widgets[i], false);
            }
        }
    }
}

void UIManager::drawWidgetClippedOn(LovyanGFX* target, UIWidget* widget, bool partial) {
    if (!target || !widget) return;
    int cx, cy, cw, ch;
    if (!computeClipRect(widget, cx, cy, cw, ch)) return;
    target->setClipRect(cx, cy, cw, ch);
    if (partial) widget->drawPartial(target);
    else widget->draw(target);
    target->clearClipRect();
    widget->markDrawn();
}

void UIManager::addWidget(UIWidget* widget) {
    if (widgetCount < 20 && widget != nullptr) {
        if (widget->getParent() == nullptr) {
            widget->setParent(rootScreen);
        }
        widgets[widgetCount] = widget;
        if (hasBackgroundLayer && foregroundWidgetCount < 20) {
            foregroundWidgets[foregroundWidgetCount] = widget;
            foregroundWidgetCount++;
            if (widget->isFocusable()) {
                bool hasForegroundFocusable = false;
                for (int i = 0; i < foregroundWidgetCount - 1; i++) {
                    if (foregroundWidgets[i] && foregroundWidgets[i]->isFocusable()) {
                        hasForegroundFocusable = true;
                        break;
                    }
                }
                if (!hasForegroundFocusable) {
                    for (int i = 0; i < backgroundWidgetCount; i++) {
                        if (backgroundWidgets[i]) {
                            backgroundWidgets[i]->setFocused(false);
                        }
                    }
                    focusableCount = 1;
                    focusableWidgets[0] = widgetCount;
                    currentFocus = 0;
                    widget->setFocused(true);
                } else {
                    focusableWidgets[focusableCount] = widgetCount;
                    focusableCount++;
                }
            }
        } else {
            if (widget->isFocusable()) {
                focusableWidgets[focusableCount] = widgetCount;
                focusableCount++;
                if (currentFocus == -1) {
                    currentFocus = 0;
                    widget->setFocused(true);
                }
            }
        }
        widgetCount++;
    }
}

UIWidget* UIManager::getWidget(int id) {
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i] && widgets[i]->getId() == id) {
            return widgets[i];
        }
    }
    return nullptr;
}

UIScreen* UIManager::getRootScreen() const { return rootScreen; }

void UIManager::removeWidget(int id) {
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i] && widgets[i]->getId() == id) {
            if (widgets[i]->isFocusable()) {
                removeFocusableWidget(i);
            }
            delete widgets[i];
            for (int j = i; j < widgetCount - 1; j++) {
                widgets[j] = widgets[j + 1];
            }
            widgets[widgetCount - 1] = nullptr;
            widgetCount--;
            break;
        }
    }
}

void UIManager::clear() {
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i]) {
            delete widgets[i];
            widgets[i] = nullptr;
        }
    }
    widgetCount = 0;
    focusableCount = 0;
    currentFocus = -1;
    for (int i = 0; i < 20; i++) {
        focusableWidgets[i] = -1;
    }
}

void UIManager::clearForeground() {
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i]) {
            removeFromMainList(foregroundWidgets[i]);
            delete foregroundWidgets[i];
            foregroundWidgets[i] = nullptr;
        }
    }
    foregroundWidgetCount = 0;
    rebuildFocusListForBackground();
}

void UIManager::saveToBackground() {
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i] && backgroundWidgetCount < 20) {
            backgroundWidgets[backgroundWidgetCount] = widgets[i];
            backgroundWidgetCount++;
        }
    }
    hasBackgroundLayer = true;
}

void UIManager::nextFocus() {
    if (focusableCount == 0) return;
    if (currentFocus >= 0 && currentFocus < focusableCount) {
        int widgetIndex = focusableWidgets[currentFocus];
        if (widgetIndex >= 0 && widgets[widgetIndex]) {
            widgets[widgetIndex]->setFocused(false);
            widgets[widgetIndex]->onFocusChanged(false);
        }
    }
    currentFocus = (currentFocus + 1) % focusableCount;
    int widgetIndex = focusableWidgets[currentFocus];
    if (widgetIndex >= 0 && widgets[widgetIndex]) {
        widgets[widgetIndex]->setFocused(true);
        widgets[widgetIndex]->onFocusChanged(true);
    }
}

void UIManager::previousFocus() {
    if (focusableCount == 0) return;
    if (currentFocus >= 0 && currentFocus < focusableCount) {
        int widgetIndex = focusableWidgets[currentFocus];
        if (widgetIndex >= 0 && widgets[widgetIndex]) {
            widgets[widgetIndex]->setFocused(false);
            widgets[widgetIndex]->onFocusChanged(false);
        }
    }
    currentFocus = (currentFocus - 1 + focusableCount) % focusableCount;
    int widgetIndex = focusableWidgets[currentFocus];
    if (widgetIndex >= 0 && widgets[widgetIndex]) {
        widgets[widgetIndex]->setFocused(true);
        widgets[widgetIndex]->onFocusChanged(true);
    }
}

UIWidget* UIManager::getCurrentFocusedWidget() {
    if (currentFocus >= 0 && currentFocus < focusableCount) {
        int widgetIndex = focusableWidgets[currentFocus];
        if (widgetIndex >= 0 && widgets[widgetIndex]) {
            return widgets[widgetIndex];
        }
    }
    return nullptr;
}

bool UIManager::handleKeyEvent(const KeyEvent& event) {
    UIWidget* focusedWidget = getCurrentFocusedWidget();
    if (event.tab) {
        nextFocus();
        return true;
    }
    if (focusedWidget && focusedWidget->hasSecondaryFocus()) {
        if (event.up || event.down || event.left || event.right || event.enter) {
            return focusedWidget->handleKeyEvent(event);
        }
    }
    if (focusedWidget) {
        return focusedWidget->handleKeyEvent(event);
    }
    return false;
}

void UIManager::clearScreen() {
    display->fillScreen(TFT_BLACK);
}

void UIManager::drawAll() {
    if (!display) return;
    display->startWrite();
    for (int i = 0; i < widgetCount; i++) {
        UIWidget* w = widgets[i];
        if (w && isWidgetAndParentsVisibleForDraw(w)) {
            drawWidgetClipped(w, false);
        }
    }
    display->endWrite();
}

void UIManager::refresh() {
    if (!display) return;
    int dw = display->width();
    int dh = display->height();
    if (dw <= 0 || dh <= 0) return;
    if (!windowCanvas) {
        windowCanvas = new LGFX_Sprite(display);
    }
    if (windowCanvas->width() != dw || windowCanvas->height() != dh) {
        windowCanvas->deleteSprite();
        windowCanvas->createSprite(dw, dh);
    }
    Theme* theme = getCurrentTheme();
    if (theme) {
        theme->clearArea(windowCanvas, 0, 0, dw, dh);
    } else {
        windowCanvas->fillScreen(TFT_BLACK);
    }
    windowCanvas->startWrite();
    drawAllOn(windowCanvas);
    windowCanvas->endWrite();
    windowCanvas->pushSprite(0, 0);
}

void UIManager::switchToApp() {
    if (!hasBackgroundLayer && widgetCount > 0) {
        saveToBackground();
    }
    if (foregroundWidgetCount > 0) {
        clearForeground();
    }
}

void UIManager::switchToLauncher() {
    if (foregroundWidgetCount > 0) {
        clearForeground();
    }
    smartRefresh();
}

void UIManager::finishAppSetup() {
    if (hasBackgroundLayer && foregroundWidgetCount > 0) {
        rebuildFocusListForForeground();
    }
    smartRefresh();
}

void UIManager::drawWidget(int id) {
    UIWidget* widget = getWidget(id);
    if (widget && isWidgetAndParentsVisibleForDraw(widget)) {
        drawWidgetClipped(widget, false);
    }
}

void UIManager::drawWidgetPartial(int id) {
    UIWidget* widget = getWidget(id);
    if (widget && isWidgetAndParentsVisibleForDraw(widget)) {
        drawWidgetClipped(widget, true);
    }
}

void UIManager::drawForegroundPartial() {
    if (hasBackgroundLayer && foregroundWidgetCount > 0) {
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i] && isWidgetAndParentsVisibleForDraw(foregroundWidgets[i])) {
                drawWidgetClipped(foregroundWidgets[i], true);
            }
        }
    }
}

void UIManager::refreshAppArea() {
    refresh();
}

void UIManager::smartRefresh() {
    refresh();
}

void UIManager::tick() {
    uint32_t nowMs = millis();
    if (display && rootScreen) {
        int w = display->width();
        int h = display->height();
        if (w > 0 && h > 0) {
            rootScreen->setSize(w, h);
        }
    }
    bool anyUpdateRequested = false;
    if (hasBackgroundLayer) {
        for (int i = 0; i < backgroundWidgetCount; i++) {
            if (backgroundWidgets[i]) {
                if (backgroundWidgets[i]->update(nowMs)) {
                    backgroundWidgets[i]->invalidate();
                    anyUpdateRequested = true;
                }
            }
        }
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i]) {
                if (foregroundWidgets[i]->update(nowMs)) {
                    foregroundWidgets[i]->invalidate();
                    anyUpdateRequested = true;
                }
            }
        }
    } else {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i]) {
                if (widgets[i]->update(nowMs)) {
                    widgets[i]->invalidate();
                    anyUpdateRequested = true;
                }
            }
        }
    }
    bool anyDirty = false;
    if (hasBackgroundLayer) {
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i] && isWidgetAndParentsVisibleForDraw(foregroundWidgets[i]) && foregroundWidgets[i]->isDirty()) {
                anyDirty = true;
                break;
            }
        }
    } else {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && isWidgetAndParentsVisibleForDraw(widgets[i]) && widgets[i]->isDirty()) {
                anyDirty = true;
                break;
            }
        }
    }
    if (!anyUpdateRequested && !anyDirty) return;
    if (nowMs - lastAnimationRedrawMs < 16) return;
    lastAnimationRedrawMs = nowMs;
    smartRefresh();
}

void UIManager::setWidgetTreeVisible(UIWidget* root, bool visible) {
    if (!root) return;
    for (int i = 0; i < widgetCount; i++) {
        UIWidget* w = widgets[i];
        if (!w) continue;
        UIWidget* p = w;
        while (p) {
            if (p == root) {
                w->setVisible(visible);
                break;
            }
            p = p->getParent();
        }
    }
}

void UIManager::showPage(UIWidget* root) {
    setWidgetTreeVisible(root, true);
}

void UIManager::hidePage(UIWidget* root) {
    setWidgetTreeVisible(root, false);
}

UILabel* UIManager::createLabel(int id, int x, int y, const String& text, const String& name, UIWidget* parent) {
    UILabel* label = new UILabel(id, x, y, text, name);
    label->setParent(parent ? parent : rootScreen);
    addWidget(label);
    return label;
}

UIButton* UIManager::createButton(int id, int x, int y, int width, int height, const String& text, const String& name, UIWidget* parent) {
    UIButton* button = new UIButton(id, x, y, width, height, text, name);
    button->setParent(parent ? parent : rootScreen);
    addWidget(button);
    return button;
}

UIButton* UIManager::createImageButton(int id, int x, int y, int width, int height, const uint8_t* imageData, size_t dataSize, const String& name, UIWidget* parent) {
    UIButton* button = new UIButton(id, x, y, width, height, String(""), name);
    button->setImageData(imageData, dataSize);
    button->setParent(parent ? parent : rootScreen);
    addWidget(button);
    return button;
}

UIButton* UIManager::createImageButtonFromFile(int id, int x, int y, int width, int height, const String& filePath, const String& name, UIWidget* parent) {
    UIButton* button = new UIButton(id, x, y, width, height, String(""), name);
    button->setImageFile(filePath);
    button->setParent(parent ? parent : rootScreen);
    addWidget(button);
    return button;
}

UIWindow* UIManager::createWindow(int id, int x, int y, int width, int height, const String& title, const String& name, UIWidget* parent) {
    UIWindow* window = new UIWindow(id, x, y, width, height, title, name);
    window->setParent(parent ? parent : rootScreen);
    addWidget(window);
    return window;
}

UIPanel* UIManager::createPanel(int id, int x, int y, int width, int height, const String& name, UIWidget* parent) {
    UIPanel* panel = new UIPanel(id, x, y, width, height, name);
    panel->setParent(parent ? parent : rootScreen);
    addWidget(panel);
    return panel;
}

UIMenuList* UIManager::createMenuList(int id, int x, int y, int width, int height, const String& name, UIWidget* parent) {
    UIMenuList* menu = new UIMenuList(id, x, y, width, height, name);
    menu->setParent(parent ? parent : rootScreen);
    addWidget(menu);
    return menu;
}

UIMenuGrid* UIManager::createMenuGrid(int id, int x, int y, int width, int height, int columns, int rows, const String& name, UIWidget* parent) {
    UIMenuGrid* menu = new UIMenuGrid(id, x, y, width, height, columns, rows, name);
    menu->setParent(parent ? parent : rootScreen);
    addWidget(menu);
    return menu;
}

UIImage* UIManager::createImage(int id, int x, int y, int width, int height, const uint8_t* imageData, size_t dataSize, const String& name, UIWidget* parent) {
    UIImage* image = new UIImage(id, x, y, width, height, imageData, dataSize, name);
    image->setParent(parent ? parent : rootScreen);
    addWidget(image);
    return image;
}

void UIManager::removeFocusableWidget(int widgetIndex) {
    for (int i = 0; i < focusableCount; i++) {
        if (focusableWidgets[i] == widgetIndex) {
            for (int j = i; j < focusableCount - 1; j++) {
                focusableWidgets[j] = focusableWidgets[j + 1];
            }
            focusableWidgets[focusableCount - 1] = -1;
            focusableCount--;
            if (currentFocus >= focusableCount) {
                currentFocus = focusableCount > 0 ? 0 : -1;
            }
            break;
        }
    }
    for (int i = 0; i < focusableCount; i++) {
        if (focusableWidgets[i] > widgetIndex) {
            focusableWidgets[i]--;
        }
    }
}

void UIManager::removeFromMainList(UIWidget* widget) {
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i] == widget) {
            if (widget->isFocusable()) {
                removeFocusableWidget(i);
            }
            for (int j = i; j < widgetCount - 1; j++) {
                widgets[j] = widgets[j + 1];
            }
            widgets[widgetCount - 1] = nullptr;
            widgetCount--;
            break;
        }
    }
}

void UIManager::rebuildFocusListForBackground() {
    focusableCount = 0;
    currentFocus = -1;
    for (int i = 0; i < backgroundWidgetCount; i++) {
        if (backgroundWidgets[i] && backgroundWidgets[i]->isFocusable()) {
            for (int j = 0; j < widgetCount; j++) {
                if (widgets[j] == backgroundWidgets[i]) {
                    focusableWidgets[focusableCount] = j;
                    focusableCount++;
                    if (currentFocus == -1) {
                        currentFocus = 0;
                        backgroundWidgets[i]->setFocused(true);
                    }
                    break;
                }
            }
        }
    }
}

void UIManager::ensureForeground(UIWidget* widget) {
    if (!widget) return;
    if (!hasBackgroundLayer) return;
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i] == widget) {
            rebuildFocusListForForeground();
            return;
        }
    }
    if (foregroundWidgetCount >= 20) return;
    foregroundWidgets[foregroundWidgetCount] = widget;
    foregroundWidgetCount++;
    rebuildFocusListForForeground();
}

void UIManager::rebuildFocusListForForeground() {
    focusableCount = 0;
    currentFocus = -1;
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i]) {
            widgets[i]->setFocused(false);
        }
    }
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i] && foregroundWidgets[i]->isFocusable() && foregroundWidgets[i]->isVisible()) {
            for (int j = 0; j < widgetCount; j++) {
                if (widgets[j] == foregroundWidgets[i]) {
                    focusableWidgets[focusableCount] = j;
                    focusableCount++;
                    if (currentFocus == -1) {
                        currentFocus = 0;
                        foregroundWidgets[i]->setFocused(true);
                    }
                    break;
                }
            }
        }
    }
}

void UIManager::rebuildForegroundFocus() {
    if (hasBackgroundLayer) {
        rebuildFocusListForForeground();
        return;
    }
    focusableCount = 0;
    currentFocus = -1;
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i]) {
            widgets[i]->setFocused(false);
        }
    }
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i] && widgets[i]->isFocusable() && widgets[i]->isVisible()) {
            focusableWidgets[focusableCount] = i;
            focusableCount++;
            if (currentFocus == -1) {
                currentFocus = 0;
                widgets[i]->setFocused(true);
            }
        }
    }
}

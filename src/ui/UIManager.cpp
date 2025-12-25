#include "UIManager.h"

UIManager::UIManager() : display(&M5Cardputer.Display), widgetCount(0), currentFocus(-1), focusableCount(0),
                  backgroundWidgetCount(0), foregroundWidgetCount(0), hasBackgroundLayer(false), rootScreen(nullptr) {
    for (int i = 0; i < 20; i++) {
        widgets[i] = nullptr;
        focusableWidgets[i] = -1;
        backgroundWidgets[i] = nullptr;
        foregroundWidgets[i] = nullptr;
    }
    int dw = display ? display->width() : 240;
    int dh = display ? display->height() : 135;
    rootScreen = new UIScreen(-1000, dw, dh, "RootScreen");
}

UIManager::~UIManager() {
    clear();
    if (rootScreen) { delete rootScreen; rootScreen = nullptr; }
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
    if (hasBackgroundLayer) {
        for (int i = 0; i < backgroundWidgetCount; i++) {
            if (backgroundWidgets[i] && backgroundWidgets[i]->isVisible()) {
                backgroundWidgets[i]->draw(display);
            }
        }
    }
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i] && foregroundWidgets[i]->isVisible()) {
            foregroundWidgets[i]->draw(display);
        }
    }
    if (!hasBackgroundLayer && foregroundWidgetCount == 0) {
        for (int i = 0; i < widgetCount; i++) {
            if (widgets[i] && widgets[i]->isVisible()) {
                widgets[i]->draw(display);
            }
        }
    }
}

void UIManager::refresh() {
    clearScreen();
    drawAll();
}

void UIManager::switchToApp() {
    if (!hasBackgroundLayer && widgetCount > 0) {
        saveToBackground();
    }
    if (foregroundWidgetCount > 0) {
        clearForeground();
    }
    clearScreen();
}

void UIManager::switchToLauncher() {
    if (foregroundWidgetCount > 0) {
        clearForeground();
    }
    clearScreen();
    drawAll();
}

void UIManager::finishAppSetup() {
    if (hasBackgroundLayer && foregroundWidgetCount > 0) {
        rebuildFocusListForForeground();
        drawForegroundPartial();
    } else {
        drawAll();
    }
}

void UIManager::drawWidget(int id) {
    UIWidget* widget = getWidget(id);
    if (widget && widget->isVisible()) {
        widget->draw(display);
    }
}

void UIManager::drawWidgetPartial(int id) {
    UIWidget* widget = getWidget(id);
    if (widget && widget->isVisible()) {
        widget->drawPartial(display);
    }
}

void UIManager::drawForegroundPartial() {
    if (hasBackgroundLayer && foregroundWidgetCount > 0) {
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i] && foregroundWidgets[i]->isVisible()) {
                foregroundWidgets[i]->drawPartial(display);
            }
        }
    }
}

void UIManager::refreshAppArea() {
    if (hasBackgroundLayer && foregroundWidgetCount > 0) {
        UIWindow* appWindow = nullptr;
        for (int i = 0; i < foregroundWidgetCount; i++) {
            if (foregroundWidgets[i] && foregroundWidgets[i]->getType() == WIDGET_WINDOW) {
                appWindow = static_cast<UIWindow*>(foregroundWidgets[i]);
                break;
            }
        }
        if (appWindow) {
            appWindow->clearAppArea(display);
            for (int i = 0; i < foregroundWidgetCount; i++) {
                if (foregroundWidgets[i] && foregroundWidgets[i]->isVisible()) {
                    foregroundWidgets[i]->draw(display);
                }
            }
        }
    } else {
        refresh();
    }
}

void UIManager::smartRefresh() {
    if (hasBackgroundLayer && foregroundWidgetCount > 0) {
        refreshAppArea();
    } else {
        refresh();
    }
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

UIMenuList* UIManager::createMenuList(int id, int x, int y, int width, int height, const String& name, int itemHeight, UIWidget* parent) {
    UIMenuList* menu = new UIMenuList(id, x, y, width, height, name, itemHeight);
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

void UIManager::rebuildFocusListForForeground() {
    focusableCount = 0;
    currentFocus = -1;
    for (int i = 0; i < widgetCount; i++) {
        if (widgets[i]) {
            widgets[i]->setFocused(false);
        }
    }
    for (int i = 0; i < foregroundWidgetCount; i++) {
        if (foregroundWidgets[i] && foregroundWidgets[i]->isFocusable()) {
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

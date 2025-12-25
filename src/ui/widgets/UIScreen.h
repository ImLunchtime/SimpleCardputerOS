#pragma once
#include <M5Cardputer.h>
#include "WidgetBase.h"
class UIScreen : public UIWidget {
public:
    UIScreen(int id, int width, int height, const String& name = "")
        : UIWidget(id, WIDGET_SCREEN, 0, 0, width, height, name, false) {}
    void draw(LGFX_Device* display) override {}
    bool handleKeyEvent(const KeyEvent& event) override { return false; }
};

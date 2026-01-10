#pragma once

#include "ITool.h"
#include "assets/panel_intercom.h"

class IntercomTool : public ImagePanelTool {
public:
    const char* getMenuText() const override {
        return "Intercom";
    }

    int getMenuItemId() const override {
        return 104;
    }

private:
    int getWindowWidgetId() const override { return WINDOW_ID; }
    int getImageWidgetId() const override { return IMAGE_ID; }
    const char* getWindowTitle() const override { return "Intercom"; }
    const char* getWindowName() const override { return "IntercomWindow"; }
    const uint8_t* getImageData() const override { return panel_intercom; }
    size_t getImageDataSize() const override { return panel_intercom_size; }
    const char* getImageName() const override { return "IntercomImage"; }

    enum ControlIds {
        WINDOW_ID = 300,
        IMAGE_ID = 301
    };
};

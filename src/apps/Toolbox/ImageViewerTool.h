#pragma once

#include "ITool.h"
#include "system/SDFileManager.h"

class ImageViewerTool : public ToolBase {
public:
    ImageViewerTool()
        : menu(nullptr),
          statusLabel(nullptr),
          imageView(nullptr),
          showingImage(false),
          imageCount(0),
          currentZoom(1.0f),
          panOffsetX(0),
          panOffsetY(0) {
    }

    const char* getMenuText() const override {
        return "Image Viewer";
    }

    int getMenuItemId() const override {
        return 105;
    }

    const char* getEnterTipLine1() const override {
        return "Loading images";
    }

    const char* getEnterTipLine2() const override {
        return "Please wait...";
    }

    void onEnter(const ToolServices& services) override {
        (void)services;
        printf("[ImageViewer] onEnter\n");
        loadImageList();
        showListPage();
    }

    void handleKeyEvent(const KeyEvent& event, const ToolServices& services) override {
        (void)services;
        if (!uiManager) {
            printf("[ImageViewer] handleKeyEvent: uiManager is null\n");
            return;
        }
        if (showingImage) {
            printf("[ImageViewer] handleKeyEvent in image page\n");
            if (!imageView) {
                printf("[ImageViewer] handleKeyEvent: imageView is null in image page\n");
                return;
            }
            bool transformed = false;
            const int panStep = 16;
            if (event.up) {
                panOffsetY -= panStep;
                imageView->panBy(0, -panStep);
                transformed = true;
            } else if (event.down) {
                panOffsetY += panStep;
                imageView->panBy(0, panStep);
                transformed = true;
            } else if (event.left) {
                panOffsetX -= panStep;
                imageView->panBy(-panStep, 0);
                transformed = true;
            } else if (event.right) {
                panOffsetX += panStep;
                imageView->panBy(panStep, 0);
                transformed = true;
            }
            if (!event.text.isEmpty()) {
                if (event.text.indexOf('-') != -1) {
                    float newZoom = currentZoom * 0.8f;
                    if (newZoom < 0.25f) newZoom = 0.25f;
                    currentZoom = newZoom;
                    imageView->setZoom(currentZoom);
                    transformed = true;
                } else if (event.text.indexOf('=') != -1) {
                    float newZoom = currentZoom * 1.25f;
                    if (newZoom > 4.0f) newZoom = 4.0f;
                    currentZoom = newZoom;
                    imageView->setZoom(currentZoom);
                    transformed = true;
                } else if (event.text.indexOf('1') != -1) {
                    int imgW = 0;
                    int imgH = 0;
                    if (imageView->getImageSize(imgW, imgH) && imgW > 0 && imgH > 0) {
                        int bx = 0;
                        int by = 0;
                        int bw = 0;
                        int bh = 0;
                        imageView->getBounds(bx, by, bw, bh);
                        float baseScaleX = (float)bw / (float)imgW;
                        float baseScaleY = (float)bh / (float)imgH;
                        float baseScale = baseScaleX < baseScaleY ? baseScaleX : baseScaleY;
                        if (baseScale > 0.0f) {
                            float newZoom = 1.0f / baseScale;
                            currentZoom = newZoom;
                            panOffsetX = 0;
                            panOffsetY = 0;
                            imageView->setZoom(currentZoom);
                            imageView->setOffset(0, 0);
                            transformed = true;
                        }
                    }
                }
            }
            if (event.enter) {
                printf("[ImageViewer] returning to list page\n");
                showListPage();
                return;
            }
            if (transformed && uiManager) {
                uiManager->refresh();
            }
            return;
        }
        if (menu) {
            printf("[ImageViewer] handleKeyEvent forwarding to menu\n");
            menu->handleSecondaryKeyEvent(event);
        } else {
            printf("[ImageViewer] handleKeyEvent: menu is null\n");
        }
    }

private:
    static const int WINDOW_ID = 350;
    static const int MENU_ID = 351;
    static const int STATUS_LABEL_ID = 352;
    static const int IMAGE_ID = 353;
    static const int kMaxImages = 64;
    static const int kBaseItemId = 4000;

    class ImageMenuList : public UIMenuList {
    public:
        ImageMenuList(int id, int x, int y, int width, int height, const char* name, ImageViewerTool* tool)
            : UIMenuList(id, x, y, width, height, name), parent(tool) {}

        void onItemSelected(MenuItem* item) override {
            if (parent) {
                parent->onImageItemSelected(item);
            }
        }

    private:
        ImageViewerTool* parent;
    };

    ImageMenuList* menu;
    UILabel* statusLabel;
    UIImage* imageView;
    bool showingImage;
    FileInfo imageFiles[kMaxImages];
    int imageCount;
    float currentZoom;
    int panOffsetX;
    int panOffsetY;

    int getWindowWidgetId() const override {
        return WINDOW_ID;
    }

    void resetPointers() override {
        window = nullptr;
        menu = nullptr;
        statusLabel = nullptr;
        imageView = nullptr;
        showingImage = false;
        imageCount = 0;
        currentZoom = 1.0f;
        panOffsetX = 0;
        panOffsetY = 0;
    }

    void bindWidgets(UIManager* manager) override {
        UIWidget* existingWindow = manager->getWidget(WINDOW_ID);
        window = existingWindow ? static_cast<UIWindow*>(existingWindow) : nullptr;
        UIWidget* existingMenu = manager->getWidget(MENU_ID);
        menu = existingMenu ? static_cast<ImageMenuList*>(existingMenu) : nullptr;
        UIWidget* existingStatus = manager->getWidget(STATUS_LABEL_ID);
        statusLabel = existingStatus ? static_cast<UILabel*>(existingStatus) : nullptr;
        UIWidget* existingImage = manager->getWidget(IMAGE_ID);
        imageView = existingImage ? static_cast<UIImage*>(existingImage) : nullptr;
        showingImage = false;
        printf("[ImageViewer] bindWidgets window=%p menu=%p status=%p image=%p\n",
               window, menu, statusLabel, imageView);
    }

    void resetImageTransform() {
        currentZoom = 1.0f;
        panOffsetX = 0;
        panOffsetY = 0;
        if (imageView) {
            imageView->resetTransform();
        }
    }

    void createWidgets(UIManager* manager) override {
        printf("[ImageViewer] createWidgets\n");
        window = manager->createWindow(WINDOW_ID, 30, 20, 180, 100, "Image Viewer", "ImageViewerWindow");
        statusLabel = manager->createLabel(STATUS_LABEL_ID, 10, 25, "Select an image:", "ImageViewerStatus", window);
        menu = new ImageMenuList(MENU_ID, 10, 40, 160, 50, "ImageViewerMenu", this);
        menu->setParent(window);
        manager->addWidget(menu);
        imageView = new UIImage(IMAGE_ID, 10, 25, 160, 70, String(""), "ImageViewerImage");
        imageView->setParent(window);
        imageView->setVisible(false);
        manager->addWidget(imageView);
        printf("[ImageViewer] createWidgets done window=%p menu=%p status=%p image=%p\n",
               window, menu, statusLabel, imageView);
    }

    void loadImageList() {
        if (!appManager || !statusLabel || !menu) {
            printf("[ImageViewer] loadImageList early exit appManager=%p statusLabel=%p menu=%p\n",
                   appManager, statusLabel, menu);
            return;
        }
        SDFileManager* fm = appManager->getSDFileManager();
        if (!fm) {
            statusLabel->setText("SD manager not available");
            menu->clear();
            imageCount = 0;
            printf("[ImageViewer] SDFileManager is null\n");
            return;
        }
        if (!fm->isInitialized()) {
            printf("[ImageViewer] SD not initialized, calling initialize()\n");
            if (!fm->initialize()) {
                statusLabel->setText("Failed to init SD card");
                menu->clear();
                imageCount = 0;
                printf("[ImageViewer] SD initialize failed\n");
                return;
            }
        } else {
            printf("[ImageViewer] SD already initialized\n");
        }
        menu->clear();
        imageCount = 0;
        const String picturesRoot = "/pictures";
        bool ok = fm->scanDirectoryRecursive(picturesRoot, imageFiles, imageCount, kMaxImages, ".png");
        printf("[ImageViewer] scanDirectoryRecursive(\"%s\") ok=%d count=%d\n",
               picturesRoot.c_str(), ok ? 1 : 0, imageCount);
        if (!ok || imageCount == 0) {
            statusLabel->setText("No PNG images found in /pictures");
            imageCount = 0;
            printf("[ImageViewer] no PNG images found in /pictures\n");
            return;
        }
        statusLabel->setText("Select an image:");
        for (int i = 0; i < imageCount; i++) {
            String name = imageFiles[i].name;
            if (name.length() == 0) {
                name = imageFiles[i].path;
            }
            printf("[ImageViewer] image[%d] name=\"%s\" path=\"%s\"\n",
                   i, name.c_str(), imageFiles[i].path.c_str());
            menu->addItem(name, kBaseItemId + i);
        }
    }

    void onImageItemSelected(MenuItem* item) {
        if (!item) {
            printf("[ImageViewer] onImageItemSelected: item is null\n");
            return;
        }
        int index = item->id - kBaseItemId;
        if (index < 0 || index >= imageCount) {
            printf("[ImageViewer] onImageItemSelected: invalid index %d (count=%d)\n", index, imageCount);
            return;
        }
        if (!imageView) {
            printf("[ImageViewer] onImageItemSelected: imageView is null\n");
            return;
        }
        printf("[ImageViewer] onImageItemSelected: id=%d index=%d path=\"%s\"\n",
               item->id, index, imageFiles[index].path.c_str());
        resetImageTransform();
        imageView->setImageFile(imageFiles[index].path);
        showImagePage();
    }

    void showListPage() {
        if (!window) {
            printf("[ImageViewer] showListPage: window is null\n");
            return;
        }
        printf("[ImageViewer] showListPage\n");
        showingImage = false;
        if (statusLabel) {
            statusLabel->setVisible(true);
        }
        if (menu) {
            menu->setVisible(true);
        }
        if (imageView) {
            imageView->setVisible(false);
        }
        if (uiManager) {
            uiManager->rebuildForegroundFocus();
            uiManager->refresh();
        }
    }

    void showImagePage() {
        if (!window) {
            printf("[ImageViewer] showImagePage: window is null\n");
            return;
        }
        printf("[ImageViewer] showImagePage\n");
        showingImage = true;
        resetImageTransform();
        if (statusLabel) {
            statusLabel->setVisible(false);
        }
        if (menu) {
            menu->setVisible(false);
        }
        if (imageView) {
            imageView->setVisible(true);
        }
        if (uiManager) {
            uiManager->rebuildForegroundFocus();
            uiManager->refresh();
        }
    }
};


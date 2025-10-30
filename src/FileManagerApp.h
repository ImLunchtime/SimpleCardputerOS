#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"
#include "SDFileManager.h"

class FileManagerApp : public App {
private:
    EventSystem* eventSystem;
    AppManager* appManager;
    SDFileManager fileManager;
    
    // UI控件ID
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        PATH_LABEL_ID = 2,
        FILE_LIST_ID = 3,
        STATUS_LABEL_ID = 4,
        WINDOW_ID = 5
    };
    
    // UI控件
    UILabel* titleLabel;
    UILabel* pathLabel;
    UIMenuList* fileList;
    UILabel* statusLabel;
    UIWindow* mainWindow;
    
    // 文件数据
    static const int MAX_FILES = 100;
    FileInfo files[MAX_FILES];
    int fileCount;
    bool sdInitialized;
    
public:
    FileManagerApp(EventSystem* events, AppManager* manager) 
        : eventSystem(events), appManager(manager), fileCount(0), sdInitialized(false) {
        uiManager = appManager->getUIManager();
    }
    
    void setup() override {
        // 创建主窗口
        mainWindow = new UIWindow(WINDOW_ID, 0, 0, 240, 135);
        uiManager->addWidget(mainWindow);
        
        // 创建标题标签
        titleLabel = new UILabel(TITLE_LABEL_ID, 5, 5, "File Manager");
        titleLabel->setTextColor(TFT_WHITE);
        uiManager->addWidget(titleLabel);
        
        // 创建路径标签
        pathLabel = new UILabel(PATH_LABEL_ID, 5, 20, "/");
        pathLabel->setTextColor(TFT_YELLOW);
        uiManager->addWidget(pathLabel);
        
        // 创建文件列表
        fileList = new UIMenuList(FILE_LIST_ID, 5, 35, 230, 80);
        fileList->setColors(TFT_WHITE, TFT_BLUE, TFT_WHITE, TFT_DARKGREY);
        uiManager->addWidget(fileList);
        
        // 创建状态标签
        statusLabel = new UILabel(STATUS_LABEL_ID, 5, 120, "Press ` to exit");
        statusLabel->setTextColor(TFT_GREEN);
        uiManager->addWidget(statusLabel);
        
        // 设置焦点到文件列表 - 使用nextFocus方法
        uiManager->nextFocus();
        
        // 初始化SD卡
        initializeSD();
        
        // 刷新文件列表
        refreshFileList();
        
        drawInterface();
    }
    
    void loop() override {
        // 文件管理器App主要通过事件驱动，这里可以添加定期更新逻辑
    }
    
    void onKeyEvent(const KeyEvent& event) override {
        // 处理Enter键 - 进入目录或显示文件信息
        if (event.enter) {
            handleFileSelection();
            drawInterface();
            return;
        }
        
        // 处理其他按键 - 传递给UI管理器
        if (uiManager->handleKeyEvent(event)) {
            drawInterface();
        }
    }
    
private:
    void initializeSD() {
        statusLabel->setText("Initializing SD card...");
        drawInterface();
        
        sdInitialized = fileManager.initialize();
        
        if (sdInitialized) {
            statusLabel->setText("SD card ready. Use arrows to navigate, Enter to select");
            refreshFileList();
        } else {
            statusLabel->setText("SD card initialization failed!");
        }
    }
    
    void refreshFileList() {
        fileList->clear();
        fileCount = 0;
        
        if (!fileManager.isInitialized()) {
            statusLabel->setText("SD card not initialized");
            return;
        }
        
        String currentPath = fileManager.getCurrentPath();
        pathLabel->setText("Path: " + currentPath);
        
        // 获取文件列表
        bool success = fileManager.listCurrentDirectory(files, fileCount, MAX_FILES);
        
        if (!success) {
            statusLabel->setText("Failed to read directory: " + currentPath);
            fileList->addItem("Error reading directory", -1, "");
            return;
        }
        
        if (fileCount == 0) {
            statusLabel->setText("Directory is empty: " + currentPath);
            return;
        }
        
        // 添加文件到列表
        for (int i = 0; i < fileCount; i++) {
            String displayName;
            
            if (files[i].isDirectory) {
                if (files[i].name == "..") {
                    displayName = "[..] (Up)";
                } else {
                    displayName = "[" + files[i].name + "]";
                }
            } else {
                displayName = files[i].name;
                if (fileManager.isAudioFile(files[i].name)) {
                    displayName = "♪ " + displayName;
                }
            }
            
            fileList->addItem(displayName, i, "");
        }
        
        statusLabel->setText("Loaded " + String(fileCount) + " items from " + currentPath);
    }
    
    void handleFileSelection() {
        if (!fileManager.isInitialized()) {
            statusLabel->setText("SD card not initialized");
            return;
        }
        
        MenuItem* selectedItem = fileList->getSelectedItem();
        if (!selectedItem) {
            statusLabel->setText("No item selected");
            return;
        }
        
        int selectedIndex = selectedItem->id;
        if (selectedIndex < 0 || selectedIndex >= fileCount) {
            statusLabel->setText("Invalid selection index");
            return;
        }
        
        FileInfo& selectedFile = files[selectedIndex];
        
        if (selectedFile.isDirectory) {
            bool success = fileManager.enterDirectory(selectedFile.name);
            
            if (success) {
                String newPath = fileManager.getCurrentPath();
                statusLabel->setText("Entered: " + newPath);
                refreshFileList();
            } else {
                statusLabel->setText("Failed to enter: " + selectedFile.name);
            }
        } else {
            String info = "File: " + selectedFile.name + " (" + String(selectedFile.size) + " bytes)";
            statusLabel->setText(info);
        }
    }
    
    String formatFileSize(size_t size) {
        if (size < 1024) {
            return String(size) + "B";
        } else if (size < 1024 * 1024) {
            return String(size / 1024) + "KB";
        } else {
            return String(size / (1024 * 1024)) + "MB";
        }
    }
    
    void drawInterface() {
        uiManager->refresh();
    }
};
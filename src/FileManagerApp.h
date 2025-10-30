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
        } else {
            statusLabel->setText("SD card initialization failed!");
        }
    }
    
    void refreshFileList() {
        if (!sdInitialized) {
            fileList->clear();
            fileList->addItem("SD card not available", -1);
            return;
        }
        
        // 清空列表
        fileList->clear();
        
        // 获取当前路径
        String currentPath = fileManager.getCurrentPath();
        pathLabel->setText(currentPath);
        
        // 获取文件列表
        if (fileManager.listCurrentDirectory(files, fileCount, MAX_FILES)) {
            for (int i = 0; i < fileCount; i++) {
                String displayName = files[i].name;
                
                if (files[i].isDirectory) {
                    if (files[i].name == "..") {
                        displayName = "[..] (Parent Directory)";
                    } else {
                        displayName = "[DIR] " + files[i].name;
                    }
                } else {
                    // 显示文件大小
                    String sizeStr = formatFileSize(files[i].size);
                    displayName = files[i].name + " (" + sizeStr + ")";
                    
                    // 标记音频文件
                    if (fileManager.isAudioFile(files[i].name)) {
                        displayName = "♪ " + displayName;
                    }
                }
                
                fileList->addItem(displayName, i);
            }
        } else {
            fileList->addItem("Error reading directory", -1);
        }
        
        // 重置选择 - 通过访问protected成员selectedIndex
        // 注意：这需要添加一个公共方法来设置选择索引
    }
    
    void handleFileSelection() {
        if (!sdInitialized || fileCount == 0) return;
        
        MenuItem* selectedItem = fileList->getSelectedItem();
        if (!selectedItem) return;
        
        int selectedIndex = selectedItem->id;
        if (selectedIndex < 0 || selectedIndex >= fileCount) return;
        
        FileInfo& selectedFile = files[selectedIndex];
        
        if (selectedFile.isDirectory) {
            // 进入目录
            if (fileManager.enterDirectory(selectedFile.name)) {
                refreshFileList();
                statusLabel->setText("Entered directory: " + selectedFile.name);
            } else {
                statusLabel->setText("Failed to enter directory");
            }
        } else {
            // 显示文件信息
            String info = "File: " + selectedFile.name + " Size: " + formatFileSize(selectedFile.size);
            statusLabel->setText(info);
            
            // 如果是音频文件，可以在这里添加播放逻辑
            if (fileManager.isAudioFile(selectedFile.name)) {
                // TODO: 集成音频播放功能
            }
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
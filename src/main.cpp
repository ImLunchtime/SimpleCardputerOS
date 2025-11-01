#include "M5Cardputer.h"
#include "EventSystem.h"
#include "AppManager.h"
#include "LauncherApp.h"
#include "MusicApp.h"
#include "SettingsApp.h"
#include "TestApp.h"
#include "FileManagerApp.h"
#include "ThemeApp.h"
#include "ThemeManager.h"

// 全局对象
EventSystem globalEventSystem;
AppManager globalAppManager(&globalEventSystem);
ThemeManager globalThemeManagerInstance;
ThemeManager* globalThemeManager = &globalThemeManagerInstance;

// 应用实例
LauncherApp launcherApp(&globalEventSystem);
MusicApp musicApp(&globalEventSystem, &globalAppManager);
SettingsApp settingsApp(&globalEventSystem);
TestApp testApp(&globalEventSystem);
FileManagerApp fileManagerApp(&globalEventSystem, &globalAppManager);
ThemeApp themeApp(&globalEventSystem);

void setup() {
  // 初始化M5Cardputer
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);  // 启用键盘
  
  // 设置显示器
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);
  
  // 注册应用到应用管理器
  globalAppManager.registerApp("launcher", "Launcher", &launcherApp, true);  // 启动器
  globalAppManager.registerApp("theme", "Theme", &themeApp);
  globalAppManager.registerApp("music", "Music", &musicApp);
  //globalAppManager.registerApp("settings", "Settings", &settingsApp);
  globalAppManager.registerApp("filemanager", "File Manager", &fileManagerApp);
  globalAppManager.registerApp("test", "Test", &testApp);
  
  // 初始化应用管理器（启动启动器）
  globalAppManager.initialize();
}

void loop() {
  // 更新键盘状态
  M5Cardputer.Keyboard.updateKeyList();
  
  // 检查键盘事件
  KeyEvent event;
  if (globalEventSystem.hasKeyEvent(event)) {
    globalAppManager.handleKeyEvent(event);
  }
  
  // 更新当前App
  globalAppManager.update();
  
  delay(50);  // 减少CPU使用
}
#include "M5Cardputer.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "apps/LauncherApp.h"
#include "apps/MusicApp.h"
#include "apps/SettingsApp.h"
#include "apps/TestApp.h"
#include "apps/FileManagerApp.h"
#include "apps/ThemeApp.h"
#include "themes/ThemeManager.h"
#include "themes/PrototypeTheme.h"
#include "themes/DarkTheme.h"
#include "themes/Windows98Theme.h"
#include "themes/WatercolorTheme.h"

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
  
  // 初始化主题系统并设置默认主题
  if (globalThemeManager) {
    globalThemeManager->registerTheme(new PrototypeTheme());
    globalThemeManager->registerTheme(new DarkTheme());
    globalThemeManager->registerTheme(new Windows98Theme());
    globalThemeManager->registerTheme(new WatercolorTheme());
    // 设置Dark主题为默认主题
    globalThemeManager->setCurrentTheme(1);
  }
  
  // 注册应用到应用管理器
  globalAppManager.registerApp("launcher", "Launcher", &launcherApp, true);  // 启动器
  globalAppManager.registerApp("theme", "Theme", &themeApp);
  globalAppManager.registerApp("music", "Music", &musicApp);
  //globalAppManager.registerApp("settings", "Settings", &settingsApp);
  globalAppManager.registerApp("filemanager", "Files", &fileManagerApp);
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
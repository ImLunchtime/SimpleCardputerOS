#include "M5Cardputer.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "apps/Launcher/LauncherApp.h"
#include "apps/Music/MusicApp.h"
#include "apps/Settings/SettingsApp.h"
#include "apps/Test/TestApp.h"
#include "apps/FileManager/FileManagerApp.h"
#include "apps/Theme/ThemeApp.h"
#include "apps/Remote/RemoteApp.h"
#include "themes/ThemeManager.h"
#include "themes/PrototypeTheme.h"
#include "themes/DarkTheme.h"
#include "themes/Windows98Theme.h"
#include "themes/WatercolorTheme.h"

int g_displayBrightness = 128;
bool g_isSleeping = false;

/*
 *                        _oo0oo_
 *                       o8888888o
 *                       88" . "88
 *                       (| -_- |)
 *                       0\  =  /0
 *                     ___/`---'\___
 *                   .' \\|     |// '.
 *                  / \\|||  :  |||// \
 *                 / _||||| -:- |||||- \
 *                |   | \\\  - /// |   |
 *                | \_|  ''\---/''  |_/ |
 *                \  .-\__  '-'  ___/-. /
 *              ___'. .'  /--.--\  `. .'___
 *           ."" '<  `.___\_<|>_/___.' >' "".
 *          | | :  `- \`.;`\ _ /`;.`/ - ` : | |
 *          \  \ `_.   \_ __\ /__ _/   .-` /  /
 *      =====`-.____`.___ \_____/___.-`___.-'=====
 *                        `=---='
 * 
 * 
 *      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 *            佛祖保佑     永不宕机     永无BUG
 */


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
RemoteApp remoteApp(&globalEventSystem);

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setBrightness(g_displayBrightness);

  globalAppManager.initializeSD();

  if (globalThemeManager) {
    globalThemeManager->registerTheme(new PrototypeTheme());
    globalThemeManager->registerTheme(new DarkTheme());
    globalThemeManager->registerTheme(new Windows98Theme());
    globalThemeManager->registerTheme(new WatercolorTheme());
    globalThemeManager->setCurrentTheme(0);
  }

  globalAppManager.registerApp("launcher", "Launcher", &launcherApp, true);
  globalAppManager.registerApp("theme", "Theme", &themeApp);
  globalAppManager.registerApp("music", "Music", &musicApp);
  //globalAppManager.registerApp("settings", "Settings", &settingsApp);
  globalAppManager.registerApp("filemanager", "Files", &fileManagerApp);
  globalAppManager.registerApp("test", "Test", &testApp);
  globalAppManager.registerApp("remote", "Remote", &remoteApp);

  globalAppManager.initialize();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.BtnA.wasPressed()) {
    if (g_isSleeping) {
      M5Cardputer.Display.setBrightness(g_displayBrightness);
      g_isSleeping = false;
    } else {
      M5Cardputer.Display.setBrightness(0);
      g_isSleeping = true;
    }
  }

  M5Cardputer.Keyboard.updateKeyList();

  KeyEvent event;
  if (globalEventSystem.hasKeyEvent(event)) {
    globalAppManager.handleKeyEvent(event);
  }

  globalAppManager.update();

  delay(10);
}

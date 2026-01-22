#include "M5Cardputer.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "apps/Launcher/LauncherApp.h"
#include "apps/GNSS/GNSSApp.h"
#include "apps/Music/MusicApp.h"
#include "apps/Settings/SettingsApp.h"
#include "apps/Toolbox/ToolboxApp.h"
#include "themes/ThemeManager.h"
#include "themes/PrototypeTheme.h"
#include "themes/DarkTheme.h"
#include "themes/Windows98Theme.h"
#include "themes/WatercolorTheme.h"
#include "themes/OrangeTheme.h"
#include "system/SleepManager.h"

int g_displayBrightness = 128;
SleepManager globalSleepManager;

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
GNSSApp gnssApp(&globalEventSystem);
MusicApp musicApp(&globalEventSystem, &globalAppManager);
SettingsApp settingsApp(&globalEventSystem);
ToolboxApp toolboxApp(&globalEventSystem);

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
    globalThemeManager->registerTheme(new OrangeTheme());
    globalThemeManager->setCurrentTheme("Orange");
  }

  globalAppManager.loadSystemConfig();
  M5Cardputer.Display.setBrightness(g_displayBrightness);

  globalAppManager.registerApp("launcher", "Launcher", &launcherApp, true);
  globalAppManager.registerApp("music", "Music", &musicApp);
  globalAppManager.registerApp("settings", "Settings", &settingsApp);
  globalAppManager.registerApp("toolbox", "Toolbox", &toolboxApp);
  globalAppManager.registerApp("gnss", "GNSS", &gnssApp);

  globalAppManager.initialize();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.BtnA.wasPressed()) {
    if (globalSleepManager.isSleeping()) {
      M5Cardputer.Display.setBrightness(g_displayBrightness);
      globalSleepManager.leaveSleep();
    } else {
      int sleepBrightness = (255 * 5) / 100;
      if (sleepBrightness < 1) sleepBrightness = 1;
      M5Cardputer.Display.setBrightness(sleepBrightness);
      globalSleepManager.enterSleep();
    }
  }

  M5Cardputer.Keyboard.updateKeyList();

  KeyEvent event;
  if (globalEventSystem.hasKeyEvent(event)) {
    globalAppManager.handleKeyEvent(event);
  }

  globalAppManager.update();
  globalSleepManager.tick();

  delay(10);
}

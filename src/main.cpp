#include "M5Cardputer.h"
#include "NewTestApp.h"
#include "EventSystem.h"

EventSystem eventSystem;
NewTestApp appSystem(&eventSystem);

void setup() {
  // 初始化M5Cardputer
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);  // 启用键盘
  
  // 设置显示器
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);
  
  // 初始化应用
  appSystem.setup();
}

void loop() {
  // 更新键盘状态
  M5Cardputer.Keyboard.updateKeyList();
  
  // 检查键盘事件
  KeyEvent event;
  if (eventSystem.hasKeyEvent(event)) {
    appSystem.onKeyEvent(event);
  }
  
  // 更新当前App
  appSystem.loop();
  
  delay(50);  // 减少CPU使用
}
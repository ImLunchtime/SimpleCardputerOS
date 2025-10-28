#include "M5Cardputer.h"
#include "Graphics.h"
#include "EventSystem.h"
#include "App.h"
#include "TestApp.h"

// 系统组件
Graphics* graphics;
EventSystem eventSystem;
AppSystem appSystem;
TestApp* testApp;

void setup() {
  // 初始化M5Cardputer
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);  // 启用键盘
  
  // 设置显示器
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);
  
  // 初始化图形系统
  graphics = new Graphics();
  
  // 创建并运行测试App
  testApp = new TestApp(graphics, &eventSystem);
  appSystem.runApp(testApp);
}

void loop() {
  // 更新键盘状态
  M5Cardputer.Keyboard.updateKeyList();
  
  // 检查键盘事件
  KeyEvent event;
  if (eventSystem.hasKeyEvent(event)) {
    appSystem.handleKeyEvent(event);
  }
  
  // 更新当前App
  appSystem.update();
  
  delay(50);  // 减少CPU使用
}
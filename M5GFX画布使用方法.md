# M5GFX画布使用方法

- 设备与画布初始化

```
#include <M5GFX.h>
#include <lgfx/v1/LGFX_Sprite.hpp>

M5GFX* display = nullptr;
LGFX_Sprite* canvas = nullptr;
LGFX_Sprite* canvasSpaceBar = 
nullptr;
LGFX_Sprite* canvasSystemBar = 
nullptr;

void initDisplay() {
  display = new M5GFX;
  display->init();
  canvas = new LGFX_Sprite
  (display);
  canvas->createSprite
  (display->width() - 18, 
  display->height() - 26);
  canvasSpaceBar = new LGFX_Sprite
  (display);
  canvasSpaceBar->createSprite
  (display->width() - canvas->width
  (), display->height());
  canvasSystemBar = new LGFX_Sprite
  (display);
  canvasSystemBar->createSprite
  (canvas->width(), display->height
  () - canvas->height());
}
```
- 合成推送
```
void pushSystemBar() {
  canvasSystemBar->pushSprite
  (canvasSpaceBar->width(), 0);
}
void pushSpaceBar() {
  canvasSpaceBar->pushSprite(0, 0);
}
void pushCanvas() {
  canvas->pushSprite
  (canvasSpaceBar->width(), 
  canvasSystemBar->height());
}
```
- 菜单区域绘制示例
```
void renderMenu() {
  canvas->fillScreen(0x101010);
  canvas->fillSmoothRoundRect(40, 
  20, 64, 64, 8, 0x555555);
  canvas->setFont(&
  fonts::efontEN_16);
  canvas->setTextColor(0xFFFFFF, 
  0x101010);
  canvas->drawCenterString("APP", 
  72, 90);
}
```
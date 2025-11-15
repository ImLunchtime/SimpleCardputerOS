#pragma once
#include "ui/NinePatch.h"

// 由 ninepatch_gui.py 生成的9-Patch数据
// 原始前缀: watercolor_button

static const int NP_watercolor_button_TOP_LEFT_W = 3;
static const int NP_watercolor_button_TOP_LEFT_H = 3;
static const uint16_t NP_watercolor_button_TOP_LEFT_PIXELS[9] = {
  0x1082, 0x1082, 0x1082, 0x1082, 0xFFDF, 0xFFDF, 0x1082, 0xFFDF, 0xF7BE
};

static const int NP_watercolor_button_TOP_W = 1;
static const int NP_watercolor_button_TOP_H = 3;
static const uint16_t NP_watercolor_button_TOP_PIXELS[3] = {
  0x1082, 0xFFDF, 0xF7BE
};

static const int NP_watercolor_button_TOP_RIGHT_W = 3;
static const int NP_watercolor_button_TOP_RIGHT_H = 3;
static const uint16_t NP_watercolor_button_TOP_RIGHT_PIXELS[9] = {
  0x1082, 0x1082, 0x1082, 0xFFDF, 0xFFDF, 0x1082, 0xF7BE, 0xFFDF, 0x1082
};

static const int NP_watercolor_button_LEFT_W = 3;
static const int NP_watercolor_button_LEFT_H = 1;
static const uint16_t NP_watercolor_button_LEFT_PIXELS[3] = {
  0x1082, 0xFFBE, 0xF79D
};

static const int NP_watercolor_button_CENTER_W = 3;
static const int NP_watercolor_button_CENTER_H = 3;
static const uint16_t NP_watercolor_button_CENTER_PIXELS[9] = {
  0xF79D, 0xF79D, 0xF79D, 0xF79D, 0xF79D, 0xF79D, 0xF79D, 0xF79D, 0xF79D
};

static const int NP_watercolor_button_RIGHT_W = 3;
static const int NP_watercolor_button_RIGHT_H = 1;
static const uint16_t NP_watercolor_button_RIGHT_PIXELS[3] = {
  0xF79D, 0xFFBE, 0x1082
};

static const int NP_watercolor_button_BOTTOM_LEFT_W = 3;
static const int NP_watercolor_button_BOTTOM_LEFT_H = 3;
static const uint16_t NP_watercolor_button_BOTTOM_LEFT_PIXELS[9] = {
  0x1082, 0xE71C, 0xEF7D, 0x1082, 0xE71C, 0xE71C, 0x1082, 0x1082, 0x1082
};

static const int NP_watercolor_button_BOTTOM_W = 1;
static const int NP_watercolor_button_BOTTOM_H = 3;
static const uint16_t NP_watercolor_button_BOTTOM_PIXELS[3] = {
  0xEF7D, 0xE71C, 0x1082
};

static const int NP_watercolor_button_BOTTOM_RIGHT_W = 3;
static const int NP_watercolor_button_BOTTOM_RIGHT_H = 3;
static const uint16_t NP_watercolor_button_BOTTOM_RIGHT_PIXELS[9] = {
  0xEF7D, 0xE71C, 0x1082, 0xE71C, 0xE71C, 0x1082, 0x1082, 0x1082, 0x1082
};

static inline NinePatchSet makeNinePatch_watercolor_button() {
  NinePatchSet s;
  s.tl = NinePatchImage{ NP_watercolor_button_TOP_LEFT_PIXELS, NP_watercolor_button_TOP_LEFT_W, NP_watercolor_button_TOP_LEFT_H };
  s.t  = NinePatchImage{ NP_watercolor_button_TOP_PIXELS, NP_watercolor_button_TOP_W, NP_watercolor_button_TOP_H };
  s.tr = NinePatchImage{ NP_watercolor_button_TOP_RIGHT_PIXELS, NP_watercolor_button_TOP_RIGHT_W, NP_watercolor_button_TOP_RIGHT_H };
  s.l  = NinePatchImage{ NP_watercolor_button_LEFT_PIXELS, NP_watercolor_button_LEFT_W, NP_watercolor_button_LEFT_H };
  s.c  = NinePatchImage{ NP_watercolor_button_CENTER_PIXELS, NP_watercolor_button_CENTER_W, NP_watercolor_button_CENTER_H };
  s.r  = NinePatchImage{ NP_watercolor_button_RIGHT_PIXELS, NP_watercolor_button_RIGHT_W, NP_watercolor_button_RIGHT_H };
  s.bl = NinePatchImage{ NP_watercolor_button_BOTTOM_LEFT_PIXELS, NP_watercolor_button_BOTTOM_LEFT_W, NP_watercolor_button_BOTTOM_LEFT_H };
  s.b  = NinePatchImage{ NP_watercolor_button_BOTTOM_PIXELS, NP_watercolor_button_BOTTOM_W, NP_watercolor_button_BOTTOM_H };
  s.br = NinePatchImage{ NP_watercolor_button_BOTTOM_RIGHT_PIXELS, NP_watercolor_button_BOTTOM_RIGHT_W, NP_watercolor_button_BOTTOM_RIGHT_H };
  return s;
}

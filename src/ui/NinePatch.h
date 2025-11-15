#pragma once
#include <M5Cardputer.h>
#include <stdint.h>
#include <stddef.h>

// 九宫格单图数据（RGB565 像素数组）
// 与 png_to_array.py 生成的数组一致：行优先连续像素，尺寸为 width x height。
struct NinePatchImage {
    const uint16_t* pixels;
    int width;
    int height;

    NinePatchImage() : pixels(nullptr), width(0), height(0) {}
    NinePatchImage(const uint16_t* p, int w, int h) : pixels(p), width(w), height(h) {}
};

// 九宫格集合：按顺序为 左上/上/右上/左/中/右/左下/下/右下
struct NinePatchSet {
    NinePatchImage tl; // top-left
    NinePatchImage t;  // top
    NinePatchImage tr; // top-right
    NinePatchImage l;  // left
    NinePatchImage c;  // center
    NinePatchImage r;  // right
    NinePatchImage bl; // bottom-left
    NinePatchImage b;  // bottom
    NinePatchImage br; // bottom-right
};

// 重复/拉伸模式
enum class NinePatchFillMode {
    Tile,     // 平铺重复
    Stretch   // 最近邻拉伸
};

// 尺寸度量：用于指定角落尺寸与边框厚度（若不指定则默认取对应图块尺寸）
struct NinePatchMetrics {
    // 边框厚度（通常与角落尺寸一致，作为默认来自图块）
    int leftWidth;    // 左边框宽度
    int rightWidth;   // 右边框宽度
    int topHeight;    // 上边框高度
    int bottomHeight; // 下边框高度

    // 各角落的宽高（可选，未设置时使用对应的边框厚度）
    int topLeftWidth;
    int topLeftHeight;
    int topRightWidth;
    int topRightHeight;
    int bottomLeftWidth;
    int bottomLeftHeight;
    int bottomRightWidth;
    int bottomRightHeight;

    NinePatchMetrics()
        : leftWidth(0), rightWidth(0), topHeight(0), bottomHeight(0),
          topLeftWidth(0), topLeftHeight(0), topRightWidth(0), topRightHeight(0),
          bottomLeftWidth(0), bottomLeftHeight(0), bottomRightWidth(0), bottomRightHeight(0) {}

    // 从图块集合推导默认度量（边框厚度与角落尺寸取对应图块宽高）
    static NinePatchMetrics fromSet(const NinePatchSet& set) {
        NinePatchMetrics m;
        m.leftWidth = set.l.width;
        m.rightWidth = set.r.width;
        m.topHeight = set.t.height;
        m.bottomHeight = set.b.height;

        m.topLeftWidth = set.tl.width;
        m.topLeftHeight = set.tl.height;
        m.topRightWidth = set.tr.width;
        m.topRightHeight = set.tr.height;
        m.bottomLeftWidth = set.bl.width;
        m.bottomLeftHeight = set.bl.height;
        m.bottomRightWidth = set.br.width;
        m.bottomRightHeight = set.br.height;
        return m;
    }
};

// 简易矩形
struct NinePatchRect {
    int x;
    int y;
    int width;
    int height;
};

// 9-Patch 渲染器：将九宫格图块绘制到指定窗口区域
class NinePatchRenderer {
public:
    // 绘制窗口（九宫格），x/y/width/height 为目标区域
    // 边框与角落尺寸由 metrics 指定；未指定时使用集合中图块自身尺寸。
    // edgeMode 控制边框的填充模式（平铺/拉伸），centerMode 控制中心区域填充模式。
    static void drawWindow(
        LGFX_Device* display,
        const NinePatchSet& set,
        int x, int y, int width, int height,
        const NinePatchMetrics& metrics,
        NinePatchFillMode edgeMode = NinePatchFillMode::Tile,
        NinePatchFillMode centerMode = NinePatchFillMode::Tile);

    // 计算内容区域（去除边框厚度），用于子控件布局。
    static NinePatchRect getContentRect(
        int x, int y, int width, int height,
        const NinePatchMetrics& metrics);

private:
    // 将图块以指定模式绘制到目标矩形
    static void drawPatch(LGFX_Device* display,
                          const NinePatchImage& img,
                          const NinePatchRect& dst,
                          NinePatchFillMode mode);

    // 辅助：平铺复制生成临时缓冲并 pushImage
    static void blitTile(LGFX_Device* display,
                         const NinePatchImage& img,
                         const NinePatchRect& dst);

    // 辅助：最近邻拉伸生成临时缓冲并 pushImage
    static void blitStretch(LGFX_Device* display,
                            const NinePatchImage& img,
                            const NinePatchRect& dst);
};

/*
用法示例（与 windowdesign1_ninepatch.h 搭配）：

  #include "windowdesign1_ninepatch.h"

  NinePatchSet set = makeNinePatch_windowdesign1();
  NinePatchMetrics m = NinePatchMetrics::fromSet(set);
  // 若需自定义角落/边框尺寸，可修改 m 的各字段。

  // 在某处绘制窗口：
  NinePatchRenderer::drawWindow(&M5Cardputer.Display, set,
                                10, 10, 200, 100,
                                m,
                                NinePatchFillMode::Tile,
                                NinePatchFillMode::Tile);

  // 获取内容区域（用于子控件的起始布局范围）
  NinePatchRect content = NinePatchRenderer::getContentRect(10, 10, 200, 100, m);

*/

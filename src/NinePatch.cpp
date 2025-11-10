#include "NinePatch.h"

static inline int clampPositive(int v) { return v < 0 ? 0 : v; }

void NinePatchRenderer::drawWindow(
    LGFX_Device* display,
    const NinePatchSet& set,
    int x, int y, int width, int height,
    const NinePatchMetrics& m,
    NinePatchFillMode edgeMode,
    NinePatchFillMode centerMode) {

    if (!display || width <= 0 || height <= 0) return;

    // 取度量（允许使用默认推导）
    int leftW   = (m.leftWidth    > 0 ? m.leftWidth    : set.l.width);
    int rightW  = (m.rightWidth   > 0 ? m.rightWidth   : set.r.width);
    int topH    = (m.topHeight    > 0 ? m.topHeight    : set.t.height);
    int bottomH = (m.bottomHeight > 0 ? m.bottomHeight : set.b.height);

    int tlW = (m.topLeftWidth     > 0 ? m.topLeftWidth     : leftW);
    int tlH = (m.topLeftHeight    > 0 ? m.topLeftHeight    : topH);
    int trW = (m.topRightWidth    > 0 ? m.topRightWidth    : rightW);
    int trH = (m.topRightHeight   > 0 ? m.topRightHeight   : topH);
    int blW = (m.bottomLeftWidth  > 0 ? m.bottomLeftWidth  : leftW);
    int blH = (m.bottomLeftHeight > 0 ? m.bottomLeftHeight : bottomH);
    int brW = (m.bottomRightWidth > 0 ? m.bottomRightWidth : rightW);
    int brH = (m.bottomRightHeight> 0 ? m.bottomRightHeight: bottomH);

    // 最小尺寸校正，避免负值
    if (width  < (tlW + trW))   width  = tlW + trW;
    if (width  < (leftW + rightW)) width = leftW + rightW;
    if (height < (tlH + blH))   height = tlH + blH;
    if (height < (topH + bottomH)) height = topH + bottomH;

    // 计算各区域矩形
    NinePatchRect dstTL { x, y, tlW, tlH };
    NinePatchRect dstTR { x + width - trW, y, trW, trH };
    NinePatchRect dstBL { x, y + height - blH, blW, blH };
    NinePatchRect dstBR { x + width - brW, y + height - brH, brW, brH };

    NinePatchRect dstT  { x + tlW, y, clampPositive(width - tlW - trW), topH };
    NinePatchRect dstB  { x + blW, y + height - bottomH, clampPositive(width - blW - brW), bottomH };
    NinePatchRect dstL  { x, y + tlH, leftW, clampPositive(height - tlH - blH) };
    NinePatchRect dstR  { x + width - rightW, y + trH, rightW, clampPositive(height - trH - brH) };

    NinePatchRect dstC  { x + leftW, y + topH, clampPositive(width - leftW - rightW), clampPositive(height - topH - bottomH) };

    // 绘制：角落直接按原图尺寸铺设；边框与中心根据模式选择 Tile 或 Stretch
    if (set.tl.pixels && dstTL.width > 0 && dstTL.height > 0) drawPatch(display, set.tl, dstTL, NinePatchFillMode::Stretch);
    if (set.tr.pixels && dstTR.width > 0 && dstTR.height > 0) drawPatch(display, set.tr, dstTR, NinePatchFillMode::Stretch);
    if (set.bl.pixels && dstBL.width > 0 && dstBL.height > 0) drawPatch(display, set.bl, dstBL, NinePatchFillMode::Stretch);
    if (set.br.pixels && dstBR.width > 0 && dstBR.height > 0) drawPatch(display, set.br, dstBR, NinePatchFillMode::Stretch);

    if (set.t.pixels && dstT.width > 0 && dstT.height > 0)   drawPatch(display, set.t,  dstT, edgeMode);
    if (set.b.pixels && dstB.width > 0 && dstB.height > 0)   drawPatch(display, set.b,  dstB, edgeMode);
    if (set.l.pixels && dstL.width > 0 && dstL.height > 0)   drawPatch(display, set.l,  dstL, edgeMode);
    if (set.r.pixels && dstR.width > 0 && dstR.height > 0)   drawPatch(display, set.r,  dstR, edgeMode);

    if (set.c.pixels && dstC.width > 0 && dstC.height > 0)   drawPatch(display, set.c,  dstC, centerMode);
}

NinePatchRect NinePatchRenderer::getContentRect(
    int x, int y, int width, int height,
    const NinePatchMetrics& m) {
    int leftW   = m.leftWidth;
    int rightW  = m.rightWidth;
    int topH    = m.topHeight;
    int bottomH = m.bottomHeight;

    NinePatchRect r;
    r.x = x + leftW;
    r.y = y + topH;
    r.width  = clampPositive(width  - leftW - rightW);
    r.height = clampPositive(height - topH  - bottomH);
    return r;
}

void NinePatchRenderer::drawPatch(LGFX_Device* display,
                                  const NinePatchImage& img,
                                  const NinePatchRect& dst,
                                  NinePatchFillMode mode) {
    if (!display || !img.pixels || dst.width <= 0 || dst.height <= 0 || img.width <= 0 || img.height <= 0) return;
    if (mode == NinePatchFillMode::Tile) {
        blitTile(display, img, dst);
    } else {
        blitStretch(display, img, dst);
    }
}

void NinePatchRenderer::blitTile(LGFX_Device* display,
                                 const NinePatchImage& img,
                                 const NinePatchRect& dst) {
    const int outW = dst.width;
    const int outH = dst.height;
    uint16_t* buffer = new (std::nothrow) uint16_t[outW * outH];
    if (!buffer) return;

    for (int yy = 0; yy < outH; ++yy) {
        int sy = yy % img.height;
        const uint16_t* srcRow = img.pixels + sy * img.width;
        for (int xx = 0; xx < outW; ++xx) {
            int sx = xx % img.width;
            buffer[yy * outW + xx] = srcRow[sx];
        }
    }

    // LGFX 部分面板在 pushImage 时需要字节交换以正确显示颜色
    // 这里临时启用 swapBytes，避免蓝色显示为偏黄/偏绿的问题。
    display->setSwapBytes(true);
    display->pushImage(dst.x, dst.y, outW, outH, buffer);
    display->setSwapBytes(false);
    delete[] buffer;
}

void NinePatchRenderer::blitStretch(LGFX_Device* display,
                                    const NinePatchImage& img,
                                    const NinePatchRect& dst) {
    const int outW = dst.width;
    const int outH = dst.height;
    uint16_t* buffer = new (std::nothrow) uint16_t[outW * outH];
    if (!buffer) return;

    // 最近邻拉伸：将目标坐标按比例映射到源图块
    for (int yy = 0; yy < outH; ++yy) {
        int sy = (yy * img.height) / outH;
        const uint16_t* srcRow = img.pixels + sy * img.width;
        for (int xx = 0; xx < outW; ++xx) {
            int sx = (xx * img.width) / outW;
            buffer[yy * outW + xx] = srcRow[sx];
        }
    }

    display->setSwapBytes(true);
    display->pushImage(dst.x, dst.y, outW, outH, buffer);
    display->setSwapBytes(false);
    delete[] buffer;
}
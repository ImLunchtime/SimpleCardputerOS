#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
PyQt5 9-Patch 可视化裁剪与头文件生成工具

功能摘要：
- 载入图片，设置四边边框厚度（left/right/top/bottom），自动计算四角与中心区域。
- 在图像上叠加九宫格覆盖层，实时预览裁剪效果。
- 为边框与中心区域设置“采样尺寸”与“采样位置”，用于生成紧凑的平铺/拉伸素材（如 1xH 的上边框、30x1 的左边框、3x3 的中心平铺单元）。
- 输出 C/C++ 头文件，与项目中的 NinePatch.h / NinePatchRenderer 兼容（RGB565 uint16_t 数组）。

使用：
  python tools/ninepatch_gui.py

依赖：
  pip install PyQt5 Pillow
"""

import sys
import os
from pathlib import Path
from typing import Tuple

from PyQt5.QtCore import Qt, QRectF
from PyQt5.QtGui import QImage, QPixmap, QColor, QBrush, QPen, QPainter
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QFormLayout,
    QPushButton, QLabel, QLineEdit, QSpinBox, QFileDialog, QMessageBox,
    QGraphicsView, QGraphicsScene, QGraphicsRectItem, QSplitter, QGroupBox,
    QCheckBox
)


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5


def image_region_to_rgb565_array(img: QImage, x: int, y: int, w: int, h: int) -> Tuple[str, int, int]:
    """将 QImage 的指定区域转换为 RGB565 数组，返回 (数组文本, 宽, 高)。"""
    if img.isNull() or w <= 0 or h <= 0:
        return "", 0, 0

    # 确保访问有效像素格式
    # 使用 img.pixel(x,y) 获取 ARGB，转 QColor 取 RGB
    lines = []
    values_per_line = 16
    total_values = w * h
    count = 0
    buffer = []
    for yy in range(h):
        for xx in range(w):
            qr = img.pixel(x + xx, y + yy)
            qc = QColor(qr)
            v = rgb888_to_rgb565(qc.red(), qc.green(), qc.blue())
            buffer.append(f"0x{v:04X}")
            count += 1
            if count % values_per_line == 0:
                lines.append("  " + ", ".join(buffer))
                buffer = []
    if buffer:
        lines.append("  " + ", ".join(buffer))
    array_text = ",\n".join(lines)
    return array_text, w, h


class NinePatchScene(QGraphicsScene):
    def __init__(self):
        super().__init__()
        self.image_item = None
        self.overlay_items = {}
        self.sample_items = {}
        self.img_w = 0
        self.img_h = 0

    def set_image(self, img: QImage):
        self.clear()
        self.overlay_items.clear()
        self.sample_items.clear()
        self.image_item = self.addPixmap(QPixmap.fromImage(img))
        self.img_w = img.width()
        self.img_h = img.height()

    def ensure_rect_item(self, key: str, color: QColor, alpha: int = 60) -> QGraphicsRectItem:
        if key in self.overlay_items:
            return self.overlay_items[key]
        item = QGraphicsRectItem()
        item.setBrush(QBrush(QColor(color.red(), color.green(), color.blue(), alpha)))
        item.setPen(QPen(QColor(color.red(), color.green(), color.blue(), 200), 1, Qt.SolidLine))
        item.setZValue(10)
        self.addItem(item)
        self.overlay_items[key] = item
        return item

    def ensure_sample_item(self, key: str, color: QColor) -> QGraphicsRectItem:
        if key in self.sample_items:
            return self.sample_items[key]
        item = QGraphicsRectItem()
        item.setBrush(QBrush(QColor(color.red(), color.green(), color.blue(), 80)))
        item.setPen(QPen(QColor(color.red(), color.green(), color.blue(), 255), 1, Qt.DashLine))
        item.setZValue(20)
        self.addItem(item)
        self.sample_items[key] = item
        return item

    def update_overlay(self, tlW, trW, blW, brW, topH, bottomH,
                        top_sample_x, top_sample_w,
                        left_sample_y, left_sample_h,
                        right_sample_y, right_sample_h,
                        bottom_sample_x, bottom_sample_w,
                        center_sample_x, center_sample_y, center_sample_w, center_sample_h):
        if self.image_item is None:
            return

        W = self.img_w
        H = self.img_h
        # 区域计算
        tl_rect = QRectF(0, 0, tlW, topH)
        tr_rect = QRectF(W - trW, 0, trW, topH)
        bl_rect = QRectF(0, H - bottomH, blW, bottomH)
        br_rect = QRectF(W - brW, H - bottomH, brW, bottomH)
        t_rect = QRectF(tlW, 0, max(0, W - tlW - trW), topH)
        b_rect = QRectF(blW, H - bottomH, max(0, W - blW - brW), bottomH)
        l_rect = QRectF(0, topH, tlW, max(0, H - topH - bottomH))
        r_rect = QRectF(W - trW, topH, trW, max(0, H - topH - bottomH))
        c_rect = QRectF(tlW, topH, max(0, W - tlW - trW), max(0, H - topH - bottomH))

        # 覆盖层绘制
        self.ensure_rect_item('tl', QColor(255, 0, 0)).setRect(tl_rect)
        self.ensure_rect_item('t', QColor(255, 128, 0)).setRect(t_rect)
        self.ensure_rect_item('tr', QColor(255, 0, 255)).setRect(tr_rect)
        self.ensure_rect_item('l', QColor(0, 255, 0)).setRect(l_rect)
        self.ensure_rect_item('c', QColor(0, 128, 255)).setRect(c_rect)
        self.ensure_rect_item('r', QColor(0, 255, 128)).setRect(r_rect)
        self.ensure_rect_item('bl', QColor(128, 0, 255)).setRect(bl_rect)
        self.ensure_rect_item('b', QColor(128, 128, 0)).setRect(b_rect)
        self.ensure_rect_item('br', QColor(0, 0, 255)).setRect(br_rect)

        # 采样指示（在各边和中心区域内显示实际导出的采样矩形）
        top_sample_x = max(0, min(int(t_rect.width()) - max(1, top_sample_w), top_sample_x))
        left_sample_y = max(0, min(int(l_rect.height()) - max(1, left_sample_h), left_sample_y))
        right_sample_y = max(0, min(int(r_rect.height()) - max(1, right_sample_h), right_sample_y))
        bottom_sample_x = max(0, min(int(b_rect.width()) - max(1, bottom_sample_w), bottom_sample_x))
        center_sample_x = max(0, min(int(c_rect.width()) - max(1, center_sample_w), center_sample_x))
        center_sample_y = max(0, min(int(c_rect.height()) - max(1, center_sample_h), center_sample_y))

        self.ensure_sample_item('t_sample', QColor(255, 255, 255)).setRect(
            QRectF(t_rect.left() + top_sample_x, t_rect.top(), max(1, top_sample_w), t_rect.height()))
        self.ensure_sample_item('l_sample', QColor(255, 255, 255)).setRect(
            QRectF(l_rect.left(), l_rect.top() + left_sample_y, l_rect.width(), max(1, left_sample_h)))
        self.ensure_sample_item('r_sample', QColor(255, 255, 255)).setRect(
            QRectF(r_rect.left(), r_rect.top() + right_sample_y, r_rect.width(), max(1, right_sample_h)))
        self.ensure_sample_item('b_sample', QColor(255, 255, 255)).setRect(
            QRectF(b_rect.left() + bottom_sample_x, b_rect.top(), max(1, bottom_sample_w), b_rect.height()))
        self.ensure_sample_item('c_sample', QColor(255, 255, 255)).setRect(
            QRectF(c_rect.left() + center_sample_x, c_rect.top() + center_sample_y, max(1, center_sample_w), max(1, center_sample_h)))


class NinePatchGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("9-Patch 裁剪工具")
        self.resize(1100, 700)

        self.img: QImage = QImage()
        self.prefix: str = "windowdesign1"

        # 默认参数（类似示例）
        self.leftW = 30
        self.rightW = 31
        self.topH = 28
        self.bottomH = 33

        # 角尺寸默认与边框相同
        self.tlW = self.leftW
        self.trW = self.rightW
        self.blW = self.leftW
        self.brW = self.rightW

        # 采样设置（默认边条1px、中心3x3）
        self.topSampleX = 0
        self.topSampleW = 1
        self.leftSampleY = 0
        self.leftSampleH = 1
        self.rightSampleY = 0
        self.rightSampleH = 1
        self.bottomSampleX = 0
        self.bottomSampleW = 1
        self.centerSampleX = 0
        self.centerSampleY = 0
        self.centerSampleW = 3
        self.centerSampleH = 3

        self.scene = NinePatchScene()
        self.view = QGraphicsView(self.scene)
        # 启用抗锯齿与平滑像素变换；修复 setRenderHint 传入 RenderHints 的类型错误
        self.view.setRenderHints(self.view.renderHints() | QPainter.Antialiasing | QPainter.SmoothPixmapTransform)
        self.view.setDragMode(QGraphicsView.ScrollHandDrag)

        # 控件区
        controls = QWidget()
        controls_layout = QVBoxLayout(controls)

        # 文件与前缀
        file_box = QGroupBox("图片与输出")
        file_form = QFormLayout()
        self.le_path = QLineEdit()
        self.le_prefix = QLineEdit(self.prefix)
        btn_open = QPushButton("打开图片…")
        btn_open.clicked.connect(self.on_open_image)
        file_form.addRow(QLabel("图片路径"), self.le_path)
        file_form.addRow(QLabel("前缀名"), self.le_prefix)
        file_form.addRow(btn_open)
        file_box.setLayout(file_form)

        # 边框参数
        border_box = QGroupBox("边框厚度（像素）")
        border_form = QFormLayout()
        self.sb_leftW = QSpinBox(); self.sb_leftW.setRange(0, 4096); self.sb_leftW.setValue(self.leftW)
        self.sb_rightW = QSpinBox(); self.sb_rightW.setRange(0, 4096); self.sb_rightW.setValue(self.rightW)
        self.sb_topH = QSpinBox(); self.sb_topH.setRange(0, 4096); self.sb_topH.setValue(self.topH)
        self.sb_bottomH = QSpinBox(); self.sb_bottomH.setRange(0, 4096); self.sb_bottomH.setValue(self.bottomH)
        border_form.addRow(QLabel("左宽 left"), self.sb_leftW)
        border_form.addRow(QLabel("右宽 right"), self.sb_rightW)
        border_form.addRow(QLabel("上高 top"), self.sb_topH)
        border_form.addRow(QLabel("下高 bottom"), self.sb_bottomH)
        border_box.setLayout(border_form)

        # 采样参数
        sample_box = QGroupBox("采样设置（导出单元尺寸与位置）")
        sample_form = QFormLayout()
        self.sb_topSampleX = QSpinBox(); self.sb_topSampleX.setRange(0, 4096); self.sb_topSampleX.setValue(self.topSampleX)
        self.sb_topSampleW = QSpinBox(); self.sb_topSampleW.setRange(1, 256); self.sb_topSampleW.setValue(self.topSampleW)
        self.sb_leftSampleY = QSpinBox(); self.sb_leftSampleY.setRange(0, 4096); self.sb_leftSampleY.setValue(self.leftSampleY)
        self.sb_leftSampleH = QSpinBox(); self.sb_leftSampleH.setRange(1, 256); self.sb_leftSampleH.setValue(self.leftSampleH)
        self.sb_rightSampleY = QSpinBox(); self.sb_rightSampleY.setRange(0, 4096); self.sb_rightSampleY.setValue(self.rightSampleY)
        self.sb_rightSampleH = QSpinBox(); self.sb_rightSampleH.setRange(1, 256); self.sb_rightSampleH.setValue(self.rightSampleH)
        self.sb_bottomSampleX = QSpinBox(); self.sb_bottomSampleX.setRange(0, 4096); self.sb_bottomSampleX.setValue(self.bottomSampleX)
        self.sb_bottomSampleW = QSpinBox(); self.sb_bottomSampleW.setRange(1, 256); self.sb_bottomSampleW.setValue(self.bottomSampleW)
        self.sb_centerSampleX = QSpinBox(); self.sb_centerSampleX.setRange(0, 4096); self.sb_centerSampleX.setValue(self.centerSampleX)
        self.sb_centerSampleY = QSpinBox(); self.sb_centerSampleY.setRange(0, 4096); self.sb_centerSampleY.setValue(self.centerSampleY)
        self.sb_centerSampleW = QSpinBox(); self.sb_centerSampleW.setRange(1, 512); self.sb_centerSampleW.setValue(self.centerSampleW)
        self.sb_centerSampleH = QSpinBox(); self.sb_centerSampleH.setRange(1, 512); self.sb_centerSampleH.setValue(self.centerSampleH)

        sample_form.addRow(QLabel("Top 采样X"), self.sb_topSampleX)
        sample_form.addRow(QLabel("Top 采样宽"), self.sb_topSampleW)
        sample_form.addRow(QLabel("Left 采样Y"), self.sb_leftSampleY)
        sample_form.addRow(QLabel("Left 采样高"), self.sb_leftSampleH)
        sample_form.addRow(QLabel("Right 采样Y"), self.sb_rightSampleY)
        sample_form.addRow(QLabel("Right 采样高"), self.sb_rightSampleH)
        sample_form.addRow(QLabel("Bottom 采样X"), self.sb_bottomSampleX)
        sample_form.addRow(QLabel("Bottom 采样宽"), self.sb_bottomSampleW)
        sample_form.addRow(QLabel("Center X"), self.sb_centerSampleX)
        sample_form.addRow(QLabel("Center Y"), self.sb_centerSampleY)
        sample_form.addRow(QLabel("Center 宽"), self.sb_centerSampleW)
        sample_form.addRow(QLabel("Center 高"), self.sb_centerSampleH)
        sample_box.setLayout(sample_form)

        # 选项
        options_box = QGroupBox("选项")
        options_layout = QVBoxLayout(options_box)
        self.cb_lock_corners = QCheckBox("角尺寸与边框厚度一致（推荐）")
        self.cb_lock_corners.setChecked(True)
        options_layout.addWidget(self.cb_lock_corners)

        # 生成按钮
        btn_generate = QPushButton("生成 9-Patch 头文件…")
        btn_generate.clicked.connect(self.on_generate_header)

        # 装配布局
        controls_layout.addWidget(file_box)
        controls_layout.addWidget(border_box)
        controls_layout.addWidget(sample_box)
        controls_layout.addWidget(options_box)
        controls_layout.addStretch(1)
        controls_layout.addWidget(btn_generate)

        splitter = QSplitter()
        splitter.addWidget(self.view)
        splitter.addWidget(controls)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 0)

        container = QWidget()
        root_layout = QVBoxLayout(container)
        root_layout.addWidget(splitter)
        self.setCentralWidget(container)

        # 信号绑定
        for sb in [self.sb_leftW, self.sb_rightW, self.sb_topH, self.sb_bottomH,
                   self.sb_topSampleX, self.sb_topSampleW, self.sb_leftSampleY, self.sb_leftSampleH,
                   self.sb_rightSampleY, self.sb_rightSampleH, self.sb_bottomSampleX, self.sb_bottomSampleW,
                   self.sb_centerSampleX, self.sb_centerSampleY, self.sb_centerSampleW, self.sb_centerSampleH]:
            sb.valueChanged.connect(self.on_params_changed)
        self.cb_lock_corners.stateChanged.connect(self.on_params_changed)

    def on_open_image(self):
        path, _ = QFileDialog.getOpenFileName(self, "选择图片", os.getcwd(), "Images (*.png *.jpg *.jpeg *.bmp)" )
        if not path:
            return
        img = QImage(path)
        if img.isNull():
            QMessageBox.critical(self, "错误", "无法加载图片：%s" % path)
            return
        self.img = img
        self.le_path.setText(path)
        self.scene.set_image(img)
        self.view.fitInView(self.scene.itemsBoundingRect(), Qt.KeepAspectRatio)
        self.on_params_changed()

    def on_params_changed(self):
        self.prefix = self.le_prefix.text().strip() or "windowdesign1"
        self.leftW = self.sb_leftW.value()
        self.rightW = self.sb_rightW.value()
        self.topH = self.sb_topH.value()
        self.bottomH = self.sb_bottomH.value()
        lock = self.cb_lock_corners.isChecked()
        if lock:
            self.tlW = self.leftW
            self.trW = self.rightW
            self.blW = self.leftW
            self.brW = self.rightW
        # 采样
        self.topSampleX = self.sb_topSampleX.value()
        self.topSampleW = self.sb_topSampleW.value()
        self.leftSampleY = self.sb_leftSampleY.value()
        self.leftSampleH = self.sb_leftSampleH.value()
        self.rightSampleY = self.sb_rightSampleY.value()
        self.rightSampleH = self.sb_rightSampleH.value()
        self.bottomSampleX = self.sb_bottomSampleX.value()
        self.bottomSampleW = self.sb_bottomSampleW.value()
        self.centerSampleX = self.sb_centerSampleX.value()
        self.centerSampleY = self.sb_centerSampleY.value()
        self.centerSampleW = self.sb_centerSampleW.value()
        self.centerSampleH = self.sb_centerSampleH.value()

        # 更新覆盖层
        if not self.img.isNull():
            self.scene.update_overlay(
                self.tlW, self.trW, self.blW, self.brW, self.topH, self.bottomH,
                self.topSampleX, self.topSampleW,
                self.leftSampleY, self.leftSampleH,
                self.rightSampleY, self.rightSampleH,
                self.bottomSampleX, self.bottomSampleW,
                self.centerSampleX, self.centerSampleY, self.centerSampleW, self.centerSampleH)

    def _validate_geometry(self) -> bool:
        if self.img.isNull():
            QMessageBox.warning(self, "提示", "请先选择图片")
            return False
        W = self.img.width(); H = self.img.height()
        if self.leftW + self.rightW > W or self.topH + self.bottomH > H:
            QMessageBox.warning(self, "提示", "边框厚度之和超过图片尺寸，请调整！")
            return False
        return True

    def _compute_rects(self):
        W = self.img.width(); H = self.img.height()
        tl = (0, 0, self.tlW, self.topH)
        tr = (W - self.trW, 0, self.trW, self.topH)
        bl = (0, H - self.bottomH, self.blW, self.bottomH)
        br = (W - self.brW, H - self.bottomH, self.brW, self.bottomH)
        t = (self.tlW, 0, max(0, W - self.tlW - self.trW), self.topH)
        b = (self.blW, H - self.bottomH, max(0, W - self.blW - self.brW), self.bottomH)
        l = (0, self.topH, self.tlW, max(0, H - self.topH - self.bottomH))
        r = (W - self.trW, self.topH, self.trW, max(0, H - self.topH - self.bottomH))
        c = (self.tlW, self.topH, max(0, W - self.tlW - self.trW), max(0, H - self.topH - self.bottomH))
        return tl, t, tr, l, c, r, bl, b, br

    def _sample_rects(self, t, l, r, b, c):
        # 边与中心的导出采样区域
        tx, ty, tw, th = t
        lx, ly, lw, lh = l
        rx, ry, rw, rh = r
        bx, by, bw, bh = b
        cx, cy, cw, ch = c

        sx_t = min(max(0, self.topSampleX), max(0, tw - self.topSampleW))
        sx_b = min(max(0, self.bottomSampleX), max(0, bw - self.bottomSampleW))
        sy_l = min(max(0, self.leftSampleY), max(0, lh - self.leftSampleH))
        sy_r = min(max(0, self.rightSampleY), max(0, rh - self.rightSampleH))
        sx_c = min(max(0, self.centerSampleX), max(0, cw - self.centerSampleW))
        sy_c = min(max(0, self.centerSampleY), max(0, ch - self.centerSampleH))

        t_sample = (tx + sx_t, ty, max(1, self.topSampleW), th)
        b_sample = (bx + sx_b, by, max(1, self.bottomSampleW), bh)
        l_sample = (lx, ly + sy_l, lw, max(1, self.leftSampleH))
        r_sample = (rx, ry + sy_r, rw, max(1, self.rightSampleH))
        c_sample = (cx + sx_c, cy + sy_c, max(1, self.centerSampleW), max(1, self.centerSampleH))
        return t_sample, l_sample, r_sample, b_sample, c_sample

    def on_generate_header(self):
        if not self._validate_geometry():
            return

        tl, t, tr, l, c, r, bl, b, br = self._compute_rects()
        t_sample, l_sample, r_sample, b_sample, c_sample = self._sample_rects(t, l, r, b, c)

        # 生成数组
        def region_to_array(name: str, rect: Tuple[int, int, int, int]):
            x, y, w, h = rect
            arr_text, aw, ah = image_region_to_rgb565_array(self.img, x, y, w, h)
            return (name, aw, ah, arr_text)

        prefix = self.prefix
        arrays = []
        arrays.append(region_to_array(f"NP_{prefix}_TOP_LEFT", tl))
        arrays.append(region_to_array(f"NP_{prefix}_TOP", t_sample))
        arrays.append(region_to_array(f"NP_{prefix}_TOP_RIGHT", tr))
        arrays.append(region_to_array(f"NP_{prefix}_LEFT", l_sample))
        arrays.append(region_to_array(f"NP_{prefix}_CENTER", c_sample))
        arrays.append(region_to_array(f"NP_{prefix}_RIGHT", r_sample))
        arrays.append(region_to_array(f"NP_{prefix}_BOTTOM_LEFT", bl))
        arrays.append(region_to_array(f"NP_{prefix}_BOTTOM", b_sample))
        arrays.append(region_to_array(f"NP_{prefix}_BOTTOM_RIGHT", br))

        # 选择输出路径
        default_name = f"{prefix}_ninepatch.h"
        out_path, _ = QFileDialog.getSaveFileName(self, "保存头文件", os.path.join(os.getcwd(), default_name), "C/C++ Header (*.h)")
        if not out_path:
            return

        try:
            with open(out_path, 'w', encoding='utf-8') as f:
                f.write("#pragma once\n")
                f.write("#include \"NinePatch.h\"\n\n")
                f.write("// 由 ninepatch_gui.py 生成的9-Patch数据\n")
                f.write(f"// 原始前缀: {prefix}\n\n")

                # 常量与数组
                for name, w, h, text in arrays:
                    base = name
                    f.write(f"static const int {base}_W = {w};\n")
                    f.write(f"static const int {base}_H = {h};\n")
                    f.write(f"static const uint16_t {base}_PIXELS[{w*h}] = {{\n")
                    f.write(text + "\n" if text else "\n")
                    f.write("};\n\n")

                # 构造 NinePatchSet 的便捷函数
                f.write(f"static inline NinePatchSet makeNinePatch_{prefix}() {{\n")
                f.write("  NinePatchSet s;\n")
                f.write(f"  s.tl = NinePatchImage{{ {arrays[0][0]}_PIXELS, {arrays[0][0]}_W, {arrays[0][0]}_H }};\n")
                f.write(f"  s.t  = NinePatchImage{{ {arrays[1][0]}_PIXELS, {arrays[1][0]}_W, {arrays[1][0]}_H }};\n")
                f.write(f"  s.tr = NinePatchImage{{ {arrays[2][0]}_PIXELS, {arrays[2][0]}_W, {arrays[2][0]}_H }};\n")
                f.write(f"  s.l  = NinePatchImage{{ {arrays[3][0]}_PIXELS, {arrays[3][0]}_W, {arrays[3][0]}_H }};\n")
                f.write(f"  s.c  = NinePatchImage{{ {arrays[4][0]}_PIXELS, {arrays[4][0]}_W, {arrays[4][0]}_H }};\n")
                f.write(f"  s.r  = NinePatchImage{{ {arrays[5][0]}_PIXELS, {arrays[5][0]}_W, {arrays[5][0]}_H }};\n")
                f.write(f"  s.bl = NinePatchImage{{ {arrays[6][0]}_PIXELS, {arrays[6][0]}_W, {arrays[6][0]}_H }};\n")
                f.write(f"  s.b  = NinePatchImage{{ {arrays[7][0]}_PIXELS, {arrays[7][0]}_W, {arrays[7][0]}_H }};\n")
                f.write(f"  s.br = NinePatchImage{{ {arrays[8][0]}_PIXELS, {arrays[8][0]}_W, {arrays[8][0]}_H }};\n")
                f.write("  return s;\n")
                f.write("}\n")

            QMessageBox.information(self, "成功", f"已生成头文件：\n{out_path}")
        except Exception as e:
            QMessageBox.critical(self, "错误", f"写入失败：{e}")


def main():
    app = QApplication(sys.argv)
    gui = NinePatchGUI()
    gui.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
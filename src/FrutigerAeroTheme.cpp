#include "FrutigerAeroTheme.h"

// 全局Frutiger Aero主题实例
FrutigerAeroTheme frutigerAeroTheme;

FrutigerAeroTheme::FrutigerAeroTheme() {
    // 初始化九宫格资源与度量信息
    windowNP = makeNinePatch_windowdesign1();
    windowMetrics = NinePatchMetrics::fromSet(windowNP);
}
#pragma once
#include <M5Cardputer.h>

// 主题绘制参数结构
struct ThemeDrawParams {
    LGFX_Device* display;
    int x, y, width, height;
    bool focused;
    bool visible;
    String text;
    uint16_t textColor;
    uint16_t borderColor;
    uint16_t backgroundColor;
    uint16_t selectedColor;
    uint16_t disabledColor;
    const uint8_t* imageData;
    size_t imageDataSize;
    String filePath;
    bool useFile;
    
    ThemeDrawParams() : display(nullptr), x(0), y(0), width(0), height(0), 
                       focused(false), visible(true), text(""),
                       textColor(TFT_WHITE), borderColor(TFT_WHITE), 
                       backgroundColor(TFT_BLACK), selectedColor(TFT_YELLOW),
                       disabledColor(TFT_DARKGREY), imageData(nullptr), imageDataSize(0),
                       filePath(""), useFile(false) {}
};

// 滑块绘制参数
struct SliderDrawParams : public ThemeDrawParams {
    int minValue;
    int maxValue;
    int currentValue;
    uint16_t trackColor;
    uint16_t thumbColor;
    String label;
    bool showValue;
    
    SliderDrawParams() : minValue(0), maxValue(100), currentValue(0),
                        trackColor(TFT_DARKGREY), thumbColor(TFT_WHITE),
                        label(""), showValue(true) {}
};

// 菜单项绘制参数
struct MenuItemDrawParams {
    LGFX_Device* display;
    int x, y, width, height;
    String text;
    bool selected;
    bool enabled;
    uint16_t textColor;
    uint16_t selectedColor;
    uint16_t disabledColor;
    uint16_t backgroundColor;
    
    MenuItemDrawParams() : display(nullptr), x(0), y(0), width(0), height(0),
                          text(""), selected(false), enabled(true),
                          textColor(TFT_WHITE), selectedColor(TFT_YELLOW),
                          disabledColor(TFT_DARKGREY), backgroundColor(TFT_BLACK) {}
};

// 网格菜单项绘制参数
struct GridMenuItemDrawParams {
    LGFX_Device* display;
    int x, y, width, height;
    String text;
    bool selected;
    bool enabled;
    bool focused;  // 整个网格菜单是否有焦点
    uint16_t textColor;
    uint16_t selectedColor;
    uint16_t disabledColor;
    uint16_t backgroundColor;
    uint16_t borderColor;
    const uint8_t* imageData;
    size_t imageDataSize;
    String filePath;
    bool useFile;
    
    GridMenuItemDrawParams() : display(nullptr), x(0), y(0), width(0), height(0),
                              text(""), selected(false), enabled(true), focused(false),
                              textColor(TFT_WHITE), selectedColor(TFT_YELLOW),
                              disabledColor(TFT_DARKGREY), backgroundColor(TFT_BLACK),
                              borderColor(TFT_DARKGREY), imageData(nullptr), imageDataSize(0),
                              filePath(""), useFile(false) {}
};

#include <SD.h>
static inline bool pngGetSize(const uint8_t* data, size_t len, int& w, int& h) {
    if (!data || len < 24) return false;
    if (data[0] != 0x89 || data[1] != 'P' || data[2] != 'N' || data[3] != 'G') return false;
    w = (int)((uint32_t)data[16] << 24 | (uint32_t)data[17] << 16 | (uint32_t)data[18] << 8 | (uint32_t)data[19]);
    h = (int)((uint32_t)data[20] << 24 | (uint32_t)data[21] << 16 | (uint32_t)data[22] << 8 | (uint32_t)data[23]);
    return true;
}

static inline bool pngFileGetSize(const String& path, int& w, int& h) {
    File f = SD.open(path.c_str());
    if (!f) return false;
    uint8_t buf[24];
    size_t n = f.read(buf, sizeof(buf));
    f.close();
    return pngGetSize(buf, n, w, h);
}

// 主题接口 - 所有主题都必须实现这些函数
class Theme {
public:
    virtual ~Theme() {}
    
    // 基础UI元素绘制函数
    virtual void drawLabel(const ThemeDrawParams& params) = 0;
    virtual void drawButton(const ThemeDrawParams& params) = 0;
    virtual void drawWindow(const ThemeDrawParams& params) = 0;
    virtual void drawSlider(const SliderDrawParams& params) = 0;
    
    // 菜单相关绘制函数
    virtual void drawMenuBorder(const ThemeDrawParams& params) = 0;
    virtual void drawMenuItem(const MenuItemDrawParams& params) = 0;
    virtual void drawGridMenuItem(const GridMenuItemDrawParams& params) = 0;
    
    // 清除区域函数
    virtual void clearArea(LGFX_Device* display, int x, int y, int width, int height) = 0;
    
    // 主题信息
    virtual String getThemeName() const = 0;
    virtual String getThemeDescription() const = 0;
};

// 主题管理器
class ThemeManager {
private:
    Theme* themes[10];  // 最多支持10个主题
    int themeCount;
    int currentThemeIndex;
    String currentThemeName;
    
public:
    ThemeManager() : themeCount(0), currentThemeIndex(0), currentThemeName("Default") {
        for (int i = 0; i < 10; i++) {
            themes[i] = nullptr;
        }
    }
    
    ~ThemeManager() {
        // 注意：不要删除主题实例，因为它们可能是静态的或由其他地方管理
    }
    
    // 注册主题
    bool registerTheme(Theme* theme) {
        if (themeCount >= 10 || theme == nullptr) {
            return false;
        }
        
        themes[themeCount] = theme;
        themeCount++;
        return true;
    }
    
    // 设置当前主题
    bool setCurrentTheme(const String& themeName) {
        for (int i = 0; i < themeCount; i++) {
            if (themes[i] && themes[i]->getThemeName() == themeName) {
                currentThemeIndex = i;
                currentThemeName = themeName;
                return true;
            }
        }
        return false;
    }
    
    // 设置当前主题（通过索引）
    bool setCurrentTheme(int index) {
        if (index >= 0 && index < themeCount && themes[index]) {
            currentThemeIndex = index;
            currentThemeName = themes[index]->getThemeName();
            return true;
        }
        return false;
    }
    
    // 获取当前主题
    Theme* getCurrentTheme() {
        if (currentThemeIndex >= 0 && currentThemeIndex < themeCount) {
            return themes[currentThemeIndex];
        }
        return nullptr;
    }
    
    // 获取主题数量
    int getThemeCount() const {
        return themeCount;
    }
    
    // 获取主题列表
    void getThemeList(String* themeNames, int maxCount) const {
        int count = (maxCount < themeCount) ? maxCount : themeCount;
        for (int i = 0; i < count; i++) {
            if (themes[i]) {
                themeNames[i] = themes[i]->getThemeName();
            }
        }
    }
    
    // 获取当前主题名称
    String getCurrentThemeName() const {
        return currentThemeName;
    }
    
    // 获取当前主题索引
    int getCurrentThemeIndex() const {
        return currentThemeIndex;
    }
    
    // 获取指定索引的主题
    Theme* getTheme(int index) {
        if (index >= 0 && index < themeCount) {
            return themes[index];
        }
        return nullptr;
    }
    
    // 下一个主题
    void nextTheme() {
        if (themeCount > 1) {
            currentThemeIndex = (currentThemeIndex + 1) % themeCount;
            if (themes[currentThemeIndex]) {
                currentThemeName = themes[currentThemeIndex]->getThemeName();
            }
        }
    }
    
    // 上一个主题
    void previousTheme() {
        if (themeCount > 1) {
            currentThemeIndex = (currentThemeIndex - 1 + themeCount) % themeCount;
            if (themes[currentThemeIndex]) {
                currentThemeName = themes[currentThemeIndex]->getThemeName();
            }
        }
    }
};

// 全局主题管理器实例声明
extern ThemeManager* globalThemeManager;

// 便捷函数 - 获取当前主题
inline Theme* getCurrentTheme() {
    return globalThemeManager ? globalThemeManager->getCurrentTheme() : nullptr;
}

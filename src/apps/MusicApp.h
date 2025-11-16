#pragma once
#include "system/App.h"
#include "ui/UIManager.h"
#include "system/EventSystem.h"
#include "system/AppManager.h"
#include "system/SDFileManager.h"
#include <cstring>  // 为 memset 添加
#include <vector>
#include <algorithm>

// ESP8266Audio 库
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>

// FreeRTOS 组件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// 音频命令枚举
enum AudioCommand {
    AUDIO_CMD_PLAY,
    AUDIO_CMD_PAUSE,
    AUDIO_CMD_STOP,
    AUDIO_CMD_NEXT,
    AUDIO_CMD_PREV,
    AUDIO_CMD_VOLUME,
    AUDIO_CMD_SHUTDOWN
};

// 音频任务命令结构
struct AudioTaskCommand {
    AudioCommand cmd;
    int param;
    char filePath[256];
};

// 音频状态结构
struct MusicAudioStatus {
    bool isPlaying;
    bool isPaused;
    int currentFileIndex;
    int currentVolume;
    char currentSongName[128];
    bool hasError;
    char errorMessage[128];
};

// 音乐分类结构
struct MusicTrack {
    String artist;
    String album;
    String title;
    String filePath;
    String fileName;
};

struct LyricLine {
    uint32_t timeMs;
    String text;
};

struct Album {
    String name;
    String artist;
    std::vector<MusicTrack*> tracks;
};

struct Artist {
    String name;
    std::vector<Album*> albums;
    std::vector<MusicTrack*> singleTracks; // 没有专辑信息的单曲
};

// 菜单导航状态
enum MenuLevel {
    MENU_MAIN,      // 主菜单：Albums, Artists, Uncategorized
    MENU_ARTISTS,   // 艺术家列表
    MENU_ALBUMS,    // 专辑列表（可能是某个艺术家的专辑或所有专辑）
    MENU_TRACKS     // 曲目列表
};

struct MenuState {
    MenuLevel level;
    String currentArtist;
    String currentAlbum;
    int selectedIndex;
};

// M5Speaker 音频输出类 - 与参考实现保持一致
class AudioOutputM5Speaker : public AudioOutput {
public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0) {
        _m5sound = m5sound;
        _virtual_ch = virtual_sound_channel;
    }
    
    virtual ~AudioOutputM5Speaker(void) {
    }
    
    virtual bool begin(void) override { 
        return true;
    }
    
    virtual bool ConsumeSample(int16_t sample[2]) override {
        if (_tri_buffer_index < tri_buf_size) {
            _tri_buffer[_tri_index][_tri_buffer_index] = sample[0];
            _tri_buffer[_tri_index][_tri_buffer_index + 1] = sample[1];
            _tri_buffer_index += 2;
            return true;
        }
        
        flush();
        return false;
    }
    
    virtual void flush(void) override {
        if (_tri_buffer_index) {
            // 使用基类的 hertz 变量，而不是自定义的采样率
            _m5sound->playRaw(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch);
            _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
            _tri_buffer_index = 0;
        }
    }
    
    virtual bool stop(void) override {
        flush();
        _m5sound->stop(_virtual_ch);
        return true;
    }

protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t tri_buf_size = 640;
    int16_t _tri_buffer[3][tri_buf_size];
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
};

class MusicApp : public App {
private:
    EventSystem* eventSystem;
    AppManager* appManager;
    
    
    // 双核心音频系统组件
    TaskHandle_t audioTaskHandle;
    QueueHandle_t audioCommandQueue;
    SemaphoreHandle_t audioStatusMutex;
    MusicAudioStatus audioStatus;
    
    // 自定义音量滑块类
    class VolumeSlider : public UISlider {
    public:
        VolumeSlider(int id, int x, int y, int width, int height, const String& name)
            : UISlider(id, x, y, width, height, 0, 100, 50, "Volume", name) {
        }
        
        void draw(LGFX_Device* display) override {
            // 绘制滑块背景
            int absX = getAbsoluteX();
            int absY = getAbsoluteY();
            display->fillRect(absX, absY, width, height, TFT_DARKGREY);
            
            // 绘制滑块边框
            if (focused) {
                display->drawRect(absX - 1, absY - 1, width + 2, height + 2, TFT_YELLOW);
            } else {
                display->drawRect(absX, absY, width, height, TFT_WHITE);
            }
            
            // 计算滑块位置（硬编码范围0-100）
            int sliderPos = absX + (getValue() * (width - 8)) / 100;
            
            // 绘制滑块指示器
            display->fillRect(sliderPos, absY + 2, 8, height - 4, TFT_WHITE);
            
            // 绘制音量文本
            String volumeText = "Vol: " + String(getValue()) + "%";
            display->setFont(&fonts::efontCN_12);
            display->setTextColor(TFT_WHITE);
            display->drawString(volumeText, absX + width + 5, absY + 2);
        }
    };

    // 自定义音乐菜单列表类
    class MusicMenuList : public UIMenuList {
    public:
        MusicMenuList(int id, int x, int y, int width, int height, const String& name, int itemHeight, MusicApp* app)
            : UIMenuList(id, x, y, width, height, name, 14), parentApp(app) {}
        
        void onItemSelected(MenuItem* item) override {
            parentApp->handleMenuSelection(item);
        }
        
    private:
        MusicApp* parentApp;
    };

    // UI 控件 ID 枚举
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        SONG_LABEL_ID = 2,
        LYRICS_CURRENT_LABEL_ID = 3,
        PLAYLIST_ID = 4,
        VOLUME_SLIDER_ID = 5,
        VOLUME_LABEL_ID = 6,
        LYRICS_NEXT_LABEL_ID = 7,
        WINDOW_ID = 8
    };
    
    // UI 组件
    UILabel* titleLabel;
    UILabel* songLabel;
    UILabel* lyricsCurrentLabel;
    UILabel* lyricsNextLabel;
    MusicMenuList* playList;  // 改为自定义菜单列表
    VolumeSlider* volumeSlider;  // 音量滑块
    UIWindow* mainWindow;
    
    // 音频组件（仅在主线程使用）
    static constexpr uint8_t m5spk_virtual_channel = 0;
    AudioFileSourceSD* audioFile;
    AudioOutputM5Speaker* audioOutput;
    AudioGeneratorMP3* mp3Generator;
    AudioFileSourceID3* id3Source;
    
    // 播放状态（主线程）
    bool isPlaying;
    bool isPaused;
    bool isInitialized;
    uint32_t pausedPosition;
    int currentVolume;
    
    // 音乐文件列表和分类
    static const int MAX_MUSIC_FILES = 100;
    FileInfo musicFiles[MAX_MUSIC_FILES];
    int musicFileCount;
    int currentFileIndex;
    
    // 音乐分类数据
    std::vector<Artist*> artists;
    std::vector<Album*> allAlbums;
    std::vector<MusicTrack*> uncategorizedTracks;
    std::vector<MusicTrack*> allTracks;
    
    // 菜单导航状态
    MenuState menuState;

    std::vector<LyricLine> lyricLines;
    bool lyricsAvailable;
    int currentLyricIndex;
    int lastLyricsFileIndex;
    uint32_t playbackStartMillis;
    uint32_t pauseStartMillis;
    uint32_t pausedAccumulatedMillis;
    bool lastIsPlayingFlag;
    bool lastIsPausedFlag;
    String lastDisplayedCurrent;
    String lastDisplayedNext;

public:
    MusicApp(EventSystem* events, AppManager* manager);
    ~MusicApp();
    
    void setup() override;
    void loop() override;
    void onKeyEvent(const KeyEvent& event) override;

private:
    // 双核心音频系统方法
    void initializeDualCoreAudio();
    void sendAudioCommand(AudioCommand cmd, int param = 0, const char* filePath = nullptr);
    void updateAudioStatus();
    void handleNextPrevRequests();
    void playNextSong();
    void playPreviousSong();
    void updateUIFromAudioStatus();
    void updateLyricsDisplay();
    
    // 音频任务方法
    static void audioTaskFunction(void* parameter);
    void processAudioCommands();
    void playAudioFile(const char* filePath);
    void pauseAudioPlayback();
    void resumeAudioPlayback();
    void stopAudioPlayback();
    void playNextAudio();
    void playPreviousAudio();
    void setAudioVolume(int volume);
    void updateAudioStatus(bool playing, bool paused, const char* songPath);
    void updateAudioError(const char* errorMsg);
    void cleanupAudioTask();
    
    // 音乐文件和UI方法
    void scanMusicFiles();
    void playCurrentSong();
    void playSelectedSong();
    void adjustVolume(int delta);
    void setVolume(int volume);
    void updateSongInfo();
    void cleanup();
    void drawInterface();
    void prepareLyricsForCurrentSong();
    void clearLyrics();
    void loadLyricsForFile(const String& mp3Path);
    String computeLrcPath(const String& mp3Path);
    
    // 音乐分类和菜单导航方法
    void categorizeMusic();
    MusicTrack* parseFileName(const String& fileName, const String& filePath);
    Artist* findOrCreateArtist(const String& artistName);
    Album* findOrCreateAlbum(Artist* artist, const String& albumName);
    void clearMusicData();
    void buildMainMenu();
    void buildArtistsMenu();
    void buildAlbumsMenu(const String& artistName = "");
    void buildTracksMenu(const String& artistName = "", const String& albumName = "");
    void navigateBack();
    void navigateForward();
    void updateMenuDisplay();
    void handleMenuSelection(MenuItem* item);  // 处理菜单项选择
    
    // 静态回调函数
    static void metadataCallback(void *cbData, const char *type, bool isUnicode, const char *string);
};
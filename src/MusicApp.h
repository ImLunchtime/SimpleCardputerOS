#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"
#include "SDFileManager.h"
#include <cstring>  // 为 memset 添加

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

// M5Speaker 音频输出类
class AudioOutputM5Speaker : public AudioOutput {
public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0) {
        _m5sound = m5sound;
        _virtual_ch = virtual_sound_channel;
        _buffer_index = 0;
    }
    
    virtual ~AudioOutputM5Speaker(void) {
    }
    
    virtual bool begin(void) override { 
        _buffer_index = 0;
        return true;
    }
    
    virtual bool ConsumeSample(int16_t sample[2]) override {
        if (_buffer_index < BUFFER_SIZE) {
            _buffer[_buffer_index++] = sample[0];
            _buffer[_buffer_index++] = sample[1];
        }
        
        if (_buffer_index >= BUFFER_SIZE) {
            _m5sound->playRaw(_buffer, BUFFER_SIZE, 44100, true, 1, _virtual_ch);
            _buffer_index = 0;
        }
        return true;
    }
    
    virtual void flush(void) override {
        if (_buffer_index > 0) {
            _m5sound->playRaw(_buffer, _buffer_index, 44100, true, 1, _virtual_ch);
            _buffer_index = 0;
        }
    }
    
    virtual bool stop(void) override {
        _m5sound->stop(_virtual_ch);
        _buffer_index = 0;
        return true;
    }

protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    static constexpr size_t BUFFER_SIZE = 512;
    int16_t _buffer[BUFFER_SIZE];
    size_t _buffer_index;
};

class MusicApp : public App {
private:
    EventSystem* eventSystem;
    AppManager* appManager;
    SDFileManager fileManager;
    
    // 双核心音频系统组件
    TaskHandle_t audioTaskHandle;
    QueueHandle_t audioCommandQueue;
    SemaphoreHandle_t audioStatusMutex;
    MusicAudioStatus audioStatus;
    
    // 音量滑块类
    class VolumeSlider : public UISlider {
    private:
        MusicApp* musicApp;
    public:
        VolumeSlider(int id, int x, int y, int width, int height, int min, int max, int initial, const String& label, MusicApp* app)
            : UISlider(id, x, y, width, height, min, max, initial, label), musicApp(app) {}
        
        void onValueChanged(int newValue) override {
            if (musicApp) {
                musicApp->setVolume(newValue);
            }
        }
    };
    
    // UI 控件 ID 枚举
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        SONG_LABEL_ID = 3,
        CONTROL_LABEL_ID = 4,
        VOLUME_LABEL_ID = 5,
        PLAYLIST_ID = 6,
        WINDOW_ID = 7,
        PLAY_BUTTON_ID = 8,
        STOP_BUTTON_ID = 9,
        PREV_BUTTON_ID = 10,
        NEXT_BUTTON_ID = 11,
        VOLUME_SLIDER_ID = 12
    };
    
    // UI 组件
    UILabel* titleLabel;
    UILabel* statusLabel;
    UILabel* songLabel;
    UILabel* controlLabel;
    UILabel* volumeLabel;
    UIMenuList* playList;
    UIWindow* mainWindow;
    
    // 控制按钮
    UIButton* playButton;
    UIButton* stopButton;
    UIButton* prevButton;
    UIButton* nextButton;
    VolumeSlider* volumeSlider;
    
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
    
    // 音乐文件列表
    static const int MAX_MUSIC_FILES = 100;
    FileInfo musicFiles[MAX_MUSIC_FILES];
    int musicFileCount;
    int currentFileIndex;

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
    
    // 主线程方法
    void scanMusicFiles();
    void playCurrentSong();
    void playSelectedSong();
    void adjustVolume(int delta);
    void setVolume(int volume);
    void updateSongInfo();
    void cleanup();
    void drawInterface();
    
    // 静态回调函数
    static void metadataCallback(void *cbData, const char *type, bool isUnicode, const char *string);
};
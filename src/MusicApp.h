#pragma once
#include "App.h"
#include "UIManager.h"
#include "EventSystem.h"
#include "AppManager.h"
#include "SDFileManager.h"
#include <cstring>  // 为 memset 添加

// 音频库
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>

// 简化的音频输出类，避免复杂的缓冲区管理
class AudioOutputM5Speaker : public AudioOutput {
public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0) {
        _m5sound = m5sound;
        _virtual_ch = virtual_sound_channel;
        _buffer_index = 0;
        memset(_buffer, 0, sizeof(_buffer));
    }
    
    virtual ~AudioOutputM5Speaker(void) {
        stop();
    }
    
    virtual bool begin(void) override { 
        if (!_m5sound) return false;
        return true; 
    }
    
    virtual bool ConsumeSample(int16_t sample[2]) override {
        if (!_m5sound || _buffer_index >= BUFFER_SIZE - 1) {
            flush();
            if (_buffer_index >= BUFFER_SIZE - 1) return false;
        }
        
        _buffer[_buffer_index++] = sample[0];
        _buffer[_buffer_index++] = sample[1];
        
        if (_buffer_index >= BUFFER_SIZE) {
            flush();
        }
        
        return true;
    }
    
    virtual void flush(void) override {
        if (_buffer_index > 0 && _m5sound) {
            _m5sound->playRaw(_buffer, _buffer_index, hertz, true, 1, _virtual_ch);
            _buffer_index = 0;
        }
    }
    
    virtual bool stop(void) override {
        if (_m5sound) {
            _m5sound->stop(_virtual_ch);
        }
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
    
    // 控件ID定义
    enum ControlIds {
        TITLE_LABEL_ID = 1,
        STATUS_LABEL_ID = 2,
        SONG_LABEL_ID = 3,
        CONTROL_LABEL_ID = 4,
        VOLUME_LABEL_ID = 5,
        PLAYLIST_ID = 6,
        WINDOW_ID = 7
    };
    
    // UI控件
    UILabel* titleLabel;
    UILabel* statusLabel;
    UILabel* songLabel;
    UILabel* controlLabel;
    UILabel* volumeLabel;
    UIMenuList* playList;
    UIWindow* mainWindow;
    
    // 音频相关
    static constexpr uint8_t m5spk_virtual_channel = 0;
    AudioFileSourceSD* audioFile;
    AudioOutputM5Speaker* audioOutput;
    AudioGeneratorMP3* mp3Generator;
    AudioFileSourceID3* id3Source;
    
    // 播放状态
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
    MusicApp(EventSystem* events, AppManager* manager) 
        : eventSystem(events), appManager(manager), 
          audioFile(nullptr), audioOutput(nullptr), mp3Generator(nullptr), id3Source(nullptr),
          isPlaying(false), isPaused(false), isInitialized(false), pausedPosition(0),
          currentVolume(50), musicFileCount(0), currentFileIndex(0) {
        uiManager = appManager->getUIManager();
    }
    
    ~MusicApp() {
        cleanup();
    }

    void setup() override {
        // 创建主窗口
        mainWindow = new UIWindow(WINDOW_ID, 0, 0, 240, 135);
        uiManager->addWidget(mainWindow);
        
        // 创建标题
        titleLabel = new UILabel(TITLE_LABEL_ID, 5, 5, "Music Player");
        titleLabel->setTextColor(TFT_WHITE);
        uiManager->addWidget(titleLabel);
        
        // 创建歌曲信息标签
        songLabel = new UILabel(SONG_LABEL_ID, 5, 20, "No song loaded");
        songLabel->setTextColor(TFT_YELLOW);
        uiManager->addWidget(songLabel);
        
        // 创建播放列表
        playList = new UIMenuList(PLAYLIST_ID, 5, 35, 230, 60);
        playList->setColors(TFT_WHITE, TFT_BLUE, TFT_WHITE, TFT_DARKGREY);
        uiManager->addWidget(playList);
        
        // 创建状态标签
        statusLabel = new UILabel(STATUS_LABEL_ID, 5, 100, "Initializing...");
        statusLabel->setTextColor(TFT_GREEN);
        uiManager->addWidget(statusLabel);
        
        // 创建控制说明
        controlLabel = new UILabel(CONTROL_LABEL_ID, 5, 115, "Space:Play P:Pause N:Next B:Prev");
        controlLabel->setTextColor(TFT_CYAN);
        uiManager->addWidget(controlLabel);
        
        // 创建音量标签
        volumeLabel = new UILabel(VOLUME_LABEL_ID, 5, 125, "Volume: 50");
        volumeLabel->setTextColor(TFT_MAGENTA);
        uiManager->addWidget(volumeLabel);
        
        // 设置焦点
        uiManager->nextFocus();
        
        // 初始化音频系统
        initializeAudio();
        
        // 扫描音乐文件
        scanMusicFiles();
        
        drawInterface();
    }

    void loop() override {
        // 处理音频播放
        if (mp3Generator && mp3Generator->isRunning()) {
            if (!mp3Generator->loop()) {
                stopPlayback();
                playNext(); // 自动播放下一首
            }
        }
    }

    void onKeyEvent(const KeyEvent& event) override {
        // 处理文本输入
        if (!event.text.isEmpty()) {
            char key = event.text.charAt(0);
            switch (key) {
                case ' ': // 空格键 - 播放/暂停
                    if (isPlaying) {
                        pausePlayback();
                    } else if (isPaused) {
                        resumePlayback();
                    } else {
                        playCurrentSong();
                    }
                    break;
                    
                case 'p': // P键 - 暂停
                    if (isPlaying) {
                        pausePlayback();
                    }
                    break;
                    
                case 'n': // N键 - 下一首
                    playNext();
                    break;
                    
                case 'b': // B键 - 上一首
                    playPrevious();
                    break;
                    
                case ';': // 分号 - 音量+ (也是up键)
                    adjustVolume(10);
                    break;
                    
                case '.': // 句号 - 音量- (也是down键)
                    adjustVolume(-10);
                    break;
            }
        }
        
        // 处理特殊键
        if (event.enter) { // 回车 - 播放选中的歌曲
            playSelectedSong();
        }
        
        if (event.up || event.down) { // 上下箭头 - 列表导航
            // 将事件传递给UI管理器处理列表导航
            if (uiManager->handleKeyEvent(event)) {
                drawInterface();
            }
        }
        
        // 将事件传递给UI管理器处理
        if (uiManager->handleKeyEvent(event)) {
            drawInterface();
        }
    }

private:
    void initializeAudio() {
        // 清理之前的资源
        cleanup();
        
        // 初始化SD卡
        if (!fileManager.initialize()) {
            statusLabel->setText("SD card initialization failed!");
            return;
        }
        
        // 创建音频组件，添加错误检查
        try {
            audioFile = new AudioFileSourceSD();
            if (!audioFile) {
                statusLabel->setText("Failed to create audio file source");
                return;
            }
            
            audioOutput = new AudioOutputM5Speaker(&M5Cardputer.Speaker, m5spk_virtual_channel);
            if (!audioOutput) {
                statusLabel->setText("Failed to create audio output");
                cleanup();
                return;
            }
            
            if (!audioOutput->begin()) {
                statusLabel->setText("Failed to initialize audio output");
                cleanup();
                return;
            }
            
            mp3Generator = new AudioGeneratorMP3();
            if (!mp3Generator) {
                statusLabel->setText("Failed to create MP3 generator");
                cleanup();
                return;
            }
            
            // 设置音量
            M5Cardputer.Speaker.setVolume(currentVolume);
            
            isInitialized = true;
            statusLabel->setText("Audio system ready");
            
        } catch (...) {
            statusLabel->setText("Audio initialization failed");
            cleanup();
            return;
        }
    }
    
    void scanMusicFiles() {
        if (!isInitialized) return;
        
        statusLabel->setText("Scanning for music files...");
        drawInterface();
        
        // 扫描所有MP3文件
        musicFileCount = 0;
        fileManager.scanAllFiles(musicFiles, musicFileCount, MAX_MUSIC_FILES, ".mp3");
        
        // 更新播放列表
        playList->clear();
        for (int i = 0; i < musicFileCount; i++) {
            String displayName = musicFiles[i].name;
            // 移除.mp3扩展名
            if (displayName.endsWith(".mp3")) {
                displayName = displayName.substring(0, displayName.length() - 4);
            }
            playList->addItem("♪ " + displayName, i, "");
        }
        
        if (musicFileCount > 0) {
            statusLabel->setText("Found " + String(musicFileCount) + " music files");
            currentFileIndex = 0;
            updateSongInfo();
        } else {
            statusLabel->setText("No MP3 files found");
        }
    }
    
    void playCurrentSong() {
        if (!isInitialized || musicFileCount == 0 || !audioFile || !audioOutput || !mp3Generator) {
            statusLabel->setText("Audio system not ready");
            return;
        }
        
        stopPlayback(); // 停止当前播放
        
        String filePath = musicFiles[currentFileIndex].path;
        
        // 打开音频文件
        if (!audioFile->open(filePath.c_str())) {
            statusLabel->setText("Failed to open: " + musicFiles[currentFileIndex].name);
            return;
        }
        
        // 创建ID3源，添加错误检查
        try {
            id3Source = new AudioFileSourceID3(audioFile);
            if (!id3Source) {
                statusLabel->setText("Failed to create ID3 source");
                audioFile->close();
                return;
            }
            
            id3Source->RegisterMetadataCB(metadataCallback, this);
            
            // 开始播放
            if (mp3Generator->begin(id3Source, audioOutput)) {
                isPlaying = true;
                isPaused = false;
                statusLabel->setText("Playing: " + musicFiles[currentFileIndex].name);
                updateSongInfo();
            } else {
                statusLabel->setText("Failed to start playback");
                if (id3Source) {
                    delete id3Source;
                    id3Source = nullptr;
                }
                audioFile->close();
            }
        } catch (...) {
            statusLabel->setText("Playback initialization failed");
            if (id3Source) {
                delete id3Source;
                id3Source = nullptr;
            }
            audioFile->close();
        }
    }
    
    void playSelectedSong() {
        MenuItem* selectedItem = playList->getSelectedItem();
        if (selectedItem && selectedItem->id >= 0 && selectedItem->id < musicFileCount) {
            currentFileIndex = selectedItem->id;
            playCurrentSong();
        }
    }
    
    void pausePlayback() {
        if (isPlaying && id3Source) {
            pausedPosition = id3Source->getPos();
            mp3Generator->stop();
            isPlaying = false;
            isPaused = true;
            statusLabel->setText("Paused: " + musicFiles[currentFileIndex].name);
        }
    }
    
    void resumePlayback() {
        if (isPaused && !isPlaying) {
            // 重新开始播放（从暂停位置）
            playCurrentSong();
            if (id3Source && pausedPosition > 0) {
                id3Source->seek(pausedPosition, 1);
            }
        }
    }
    
    void stopPlayback() {
        if (mp3Generator && isPlaying) {
            mp3Generator->stop();
        }
        
        if (audioOutput) {
            audioOutput->stop();
        }
        
        if (id3Source) {
            id3Source->close();
            delete id3Source;
            id3Source = nullptr;
        }
        
        if (audioFile) {
            audioFile->close();
        }
        
        isPlaying = false;
        isPaused = false;
        pausedPosition = 0;
    }
    
    void playNext() {
        if (musicFileCount == 0) return;
        
        currentFileIndex = (currentFileIndex + 1) % musicFileCount;
        playCurrentSong();
    }
    
    void playPrevious() {
        if (musicFileCount == 0) return;
        
        currentFileIndex = (currentFileIndex - 1 + musicFileCount) % musicFileCount;
        playCurrentSong();
    }
    
    void adjustVolume(int delta) {
        currentVolume += delta;
        if (currentVolume < 0) currentVolume = 0;
        if (currentVolume > 255) currentVolume = 255;
        
        M5Cardputer.Speaker.setVolume(currentVolume);
        volumeLabel->setText("Volume: " + String(currentVolume));
    }
    
    void updateSongInfo() {
        if (musicFileCount > 0 && currentFileIndex >= 0 && currentFileIndex < musicFileCount) {
            String info = "(" + String(currentFileIndex + 1) + "/" + String(musicFileCount) + ") ";
            info += musicFiles[currentFileIndex].name;
            songLabel->setText(info);
            
            // 注意：UIMenuList会自动管理选中项，不需要手动设置
        }
    }
    
    void cleanup() {
        // 首先停止播放
        stopPlayback();
        
        // 清理音频组件，按正确顺序
        if (mp3Generator) {
            delete mp3Generator;
            mp3Generator = nullptr;
        }
        
        if (audioOutput) {
            audioOutput->stop();
            delete audioOutput;
            audioOutput = nullptr;
        }
        
        if (audioFile) {
            delete audioFile;
            audioFile = nullptr;
        }
        
        // 重置状态
        isInitialized = false;
        isPlaying = false;
        isPaused = false;
        pausedPosition = 0;
    }
    
    void drawInterface() {
        uiManager->refresh();
    }
    
    // 静态回调函数
    static void metadataCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
        // 可以在这里处理ID3标签信息
        // 暂时不做处理，避免复杂化
    }
};
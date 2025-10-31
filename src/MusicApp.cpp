#include "MusicApp.h"

MusicApp::MusicApp(EventSystem* events, AppManager* manager) 
    : eventSystem(events), appManager(manager), 
      audioFile(nullptr), audioOutput(nullptr), mp3Generator(nullptr), id3Source(nullptr),
      isPlaying(false), isPaused(false), isInitialized(false), pausedPosition(0),
      currentVolume(50), musicFileCount(0), currentFileIndex(0),
      audioTaskHandle(nullptr), audioCommandQueue(nullptr), audioStatusMutex(nullptr) {
    uiManager = appManager->getUIManager();
    
    // 初始化音频状态
    memset(&audioStatus, 0, sizeof(audioStatus));
    audioStatus.currentVolume = currentVolume;
    audioStatus.currentFileIndex = -1;
}

MusicApp::~MusicApp() {
    // 停止音频任务
    if (audioTaskHandle) {
        sendAudioCommand(AUDIO_CMD_SHUTDOWN);
        vTaskDelay(pdMS_TO_TICKS(100)); // 等待任务结束
        vTaskDelete(audioTaskHandle);
        audioTaskHandle = nullptr;
    }
    
    // 清理队列和信号量
    if (audioCommandQueue) {
        vQueueDelete(audioCommandQueue);
        audioCommandQueue = nullptr;
    }
    
    if (audioStatusMutex) {
        vSemaphoreDelete(audioStatusMutex);
        audioStatusMutex = nullptr;
    }
    
    cleanup();
}

void MusicApp::setup() {
    // 配置 M5Cardputer 扬声器 - 使用更高的采样率提升音质
    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = 128000; // 使用 128kHz 采样率，与参考实现一致
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5Cardputer.Speaker.config(spk_cfg);
    
    // 创建主窗口 - 更小的窗口，类似设置窗口
    mainWindow = new UIWindow(WINDOW_ID, 30, 20, 180, 95);
    uiManager->addWidget(mainWindow);
    
    // 创建标题
    titleLabel = new UILabel(TITLE_LABEL_ID, 35, 25, "Music Player");
    titleLabel->setTextColor(TFT_WHITE);
    uiManager->addWidget(titleLabel);
    
    // 创建歌曲信息标签
    songLabel = new UILabel(SONG_LABEL_ID, 35, 40, "No song loaded");
    songLabel->setTextColor(TFT_YELLOW);
    uiManager->addWidget(songLabel);
    
    // 创建播放列表 - 调整大小和位置
    playList = new UIMenuList(PLAYLIST_ID, 35, 55, 140, 30);
    playList->setColors(TFT_WHITE, TFT_BLUE, TFT_WHITE, TFT_DARKGREY);
    uiManager->addWidget(playList);
    
    // 创建音量滑块 - 调整位置
    volumeSlider = new VolumeSlider(VOLUME_SLIDER_ID, 35, 90, 120, 15, 0, 100, currentVolume, "Volume", this);
    volumeSlider->setColors(TFT_DARKGREY, TFT_WHITE, TFT_YELLOW);
    uiManager->addWidget(volumeSlider);
    
    // 创建状态标签 - 调整位置
    statusLabel = new UILabel(STATUS_LABEL_ID, 35, 105, "Ready");
    statusLabel->setTextColor(TFT_GREEN);
    uiManager->addWidget(statusLabel);
    
    // 设置焦点
    uiManager->nextFocus();
    
    // 初始化双核心音频系统
    initializeDualCoreAudio();
    
    // 扫描音乐文件
    scanMusicFiles();
    
    drawInterface();
}

void MusicApp::loop() {
    // 更新音频状态（从音频任务获取状态）
    updateAudioStatus();
    
    // 检查是否有下一首/上一首的请求
    handleNextPrevRequests();
    
    // 更新UI显示
    updateUIFromAudioStatus();
}

void MusicApp::onKeyEvent(const KeyEvent& event) {
    // 处理按钮点击
    if (event.enter) {
        UIWidget* focusedWidget = uiManager->getCurrentFocusedWidget();
        if (focusedWidget) {
            switch (focusedWidget->getId()) {
                case PLAYLIST_ID:
                    playSelectedSong();
                    break;
            }
        }
    }
    
    // 处理文本输入（保留一些快捷键）
    if (!event.text.isEmpty()) {
        char key = event.text.charAt(0);
        switch (key) {
            case ' ': // 空格键 - 播放/暂停
                if (audioStatus.isPlaying) {
                    sendAudioCommand(AUDIO_CMD_PAUSE);
                } else if (audioStatus.isPaused) {
                    sendAudioCommand(AUDIO_CMD_PLAY);
                } else {
                    playCurrentSong();
                }
                break;
                
            case 'p': // P键 - 暂停
                if (audioStatus.isPlaying) {
                    sendAudioCommand(AUDIO_CMD_PAUSE);
                }
                break;
                
            case 's': // S键 - 停止
                sendAudioCommand(AUDIO_CMD_STOP);
                break;
                
            case 'n': // N键 - 下一首
                sendAudioCommand(AUDIO_CMD_NEXT);
                break;
                
            case 'b': // B键 - 上一首
                sendAudioCommand(AUDIO_CMD_PREV);
                break;
                
            case ';': // 分号 - 音量+ (也是up键)
                adjustVolume(10);
                break;
                
            case '.': // 句号 - 音量- (也是down键)
                adjustVolume(-10);
                break;
        }
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

// 初始化双核心音频系统
void MusicApp::initializeDualCoreAudio() {
    // 清理之前的资源
    cleanup();
    
    // 初始化SD卡
    if (!fileManager.initialize()) {
        statusLabel->setText("SD card initialization failed!");
        return;
    }
    
    // 创建命令队列
    audioCommandQueue = xQueueCreate(10, sizeof(AudioTaskCommand));
    if (!audioCommandQueue) {
        statusLabel->setText("Failed to create audio command queue");
        return;
    }
    
    // 创建状态互斥锁
    audioStatusMutex = xSemaphoreCreateMutex();
    if (!audioStatusMutex) {
        statusLabel->setText("Failed to create audio status mutex");
        vQueueDelete(audioCommandQueue);
        audioCommandQueue = nullptr;
        return;
    }
    
    // 创建音频任务，运行在Core 0上，增加堆栈大小
    BaseType_t result = xTaskCreatePinnedToCore(
        audioTaskFunction,      // 任务函数
        "AudioTask",           // 任务名称
        16384,                 // 堆栈大小 (增加到16KB)
        this,                  // 传递给任务的参数
        1,                     // 任务优先级 (降低优先级)
        &audioTaskHandle,      // 任务句柄
        0                      // 运行在Core 0
    );
    
    if (result != pdPASS) {
        statusLabel->setText("Failed to create audio task");
        vQueueDelete(audioCommandQueue);
        vSemaphoreDelete(audioStatusMutex);
        audioCommandQueue = nullptr;
        audioStatusMutex = nullptr;
        return;
    }
    
    // 设置音量
    M5Cardputer.Speaker.setVolume(currentVolume);
    
    isInitialized = true;
    statusLabel->setText("Dual-core audio system ready");
}

// 发送音频命令到音频任务
void MusicApp::sendAudioCommand(AudioCommand cmd, int param, const char* filePath) {
    if (!audioCommandQueue) return;
    
    AudioTaskCommand command;
    command.cmd = cmd;
    command.param = param;
    
    if (filePath) {
        strncpy(command.filePath, filePath, sizeof(command.filePath) - 1);
        command.filePath[sizeof(command.filePath) - 1] = '\0';
    } else {
        command.filePath[0] = '\0';
    }
    
    xQueueSend(audioCommandQueue, &command, 0);
}

// 更新音频状态（从音频任务获取）
void MusicApp::updateAudioStatus() {
    if (!audioStatusMutex) return;
    
    if (xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // 复制状态到本地变量
        isPlaying = audioStatus.isPlaying;
        isPaused = audioStatus.isPaused;
        currentFileIndex = audioStatus.currentFileIndex;
        currentVolume = audioStatus.currentVolume;
        
        xSemaphoreGive(audioStatusMutex);
    }
}

// 根据音频状态更新UI
void MusicApp::handleNextPrevRequests() {
    // 检查音频状态中的错误消息，看是否有下一首/上一首的请求
    if (xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
        if (audioStatus.hasError) {
            if (strstr(audioStatus.errorMessage, "Next requested") || strstr(audioStatus.errorMessage, "Song finished")) {
                // 处理下一首请求或歌曲结束
                playNextSong();
                audioStatus.hasError = false;
                strcpy(audioStatus.errorMessage, "");
            } else if (strstr(audioStatus.errorMessage, "Previous requested")) {
                // 处理上一首请求
                playPreviousSong();
                audioStatus.hasError = false;
                strcpy(audioStatus.errorMessage, "");
            }
        }
        xSemaphoreGive(audioStatusMutex);
    }
}

void MusicApp::playNextSong() {
    if (musicFileCount == 0) return;
    
    currentFileIndex = (currentFileIndex + 1) % musicFileCount;
    sendAudioCommand(AUDIO_CMD_STOP); // 先停止当前播放
    vTaskDelay(pdMS_TO_TICKS(100)); // 等待停止完成
    sendAudioCommand(AUDIO_CMD_PLAY, 0, musicFiles[currentFileIndex].path.c_str());
    updateSongInfo();
}

void MusicApp::playPreviousSong() {
    if (musicFileCount == 0) return;
    
    currentFileIndex = (currentFileIndex - 1 + musicFileCount) % musicFileCount;
    sendAudioCommand(AUDIO_CMD_STOP); // 先停止当前播放
    vTaskDelay(pdMS_TO_TICKS(100)); // 等待停止完成
    sendAudioCommand(AUDIO_CMD_PLAY, 0, musicFiles[currentFileIndex].path.c_str());
    updateSongInfo();
}

// 根据音频状态更新UI
void MusicApp::updateUIFromAudioStatus() {
    if (!audioStatusMutex) return;
    
    if (xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        // 更新歌曲信息
        if (strlen(audioStatus.currentSongName) > 0) {
            String info = "(" + String(audioStatus.currentFileIndex + 1) + "/" + String(musicFileCount) + ") ";
            info += String(audioStatus.currentSongName);
            songLabel->setText(info);
        }
        
        // 更新状态信息
        if (audioStatus.hasError && strlen(audioStatus.errorMessage) > 0) {
            statusLabel->setText(String(audioStatus.errorMessage));
        }
        
        xSemaphoreGive(audioStatusMutex);
    }
}

// 静态音频任务函数
void MusicApp::audioTaskFunction(void* parameter) {
    MusicApp* app = static_cast<MusicApp*>(parameter);
    app->processAudioCommands();
}

// 音频任务主循环
void MusicApp::processAudioCommands() {
    AudioTaskCommand command;
    
    // 在音频任务中初始化音频组件，添加更多安全检查
    audioFile = new(std::nothrow) AudioFileSourceSD();
    if (!audioFile) {
        updateAudioError("Failed to create AudioFileSourceSD");
        cleanupAudioTask();
        vTaskDelete(nullptr);
        return;
    }
    
    // 初始化扬声器，使用更安全的参数
    audioOutput = new(std::nothrow) AudioOutputM5Speaker(&M5Cardputer.Speaker, m5spk_virtual_channel);
    if (!audioOutput) {
        updateAudioError("Failed to create AudioOutputM5Speaker");
        cleanupAudioTask();
        vTaskDelete(nullptr);
        return;
    }
    
    mp3Generator = new(std::nothrow) AudioGeneratorMP3();
    if (!mp3Generator) {
        updateAudioError("Failed to create AudioGeneratorMP3");
        cleanupAudioTask();
        vTaskDelete(nullptr);
        return;
    }
    
    id3Source = nullptr;
    
    // 尝试初始化音频输出
    if (!audioOutput->begin()) {
        updateAudioError("Audio output init failed");
        cleanupAudioTask();
        vTaskDelete(nullptr);
        return;
    }
    
    // 设置音频输出参数 - 使用标准的 44.1kHz 采样率
    audioOutput->SetRate(44100);        // MP3 标准采样率
    audioOutput->SetBitsPerSample(16);  // 16位采样
    audioOutput->SetChannels(2);        // 立体声
    
    while (true) {
        // 处理音频播放循环 - 高优先级，减少延迟
        if (mp3Generator && mp3Generator->isRunning()) {
            if (!mp3Generator->loop()) {
                // 歌曲播放完毕，停止播放并通知主线程
                stopAudioPlayback();
                // 通知主线程歌曲结束，让主线程决定是否播放下一首
                updateAudioError("Song finished");
            }
        }
        
        // 检查命令队列 - 使用较短的超时时间
        if (xQueueReceive(audioCommandQueue, &command, pdMS_TO_TICKS(1)) == pdTRUE) {
            switch (command.cmd) {
                case AUDIO_CMD_PLAY:
                    if (strlen(command.filePath) > 0) {
                        playAudioFile(command.filePath);
                    } else {
                        resumeAudioPlayback();
                    }
                    break;
                case AUDIO_CMD_PAUSE:
                    pauseAudioPlayback();
                    break;
                case AUDIO_CMD_STOP:
                    stopAudioPlayback();
                    break;
                case AUDIO_CMD_NEXT:
                    // 不在音频任务中处理，而是通知主线程
                    updateAudioError("Next requested");
                    break;
                case AUDIO_CMD_PREV:
                    // 不在音频任务中处理，而是通知主线程
                    updateAudioError("Previous requested");
                    break;
                case AUDIO_CMD_VOLUME:
                    setAudioVolume(command.param);
                    break;
                case AUDIO_CMD_SHUTDOWN:
                    stopAudioPlayback();
                    cleanupAudioTask();
                    vTaskDelete(nullptr);
                    return;
            }
        }
        
        // 减少延迟，提高音频处理性能
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// 音频任务内部方法
void MusicApp::playAudioFile(const char* filePath) {
    // 先停止当前播放
    stopAudioPlayback();
    
    // 添加延迟确保停止完成
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 检查文件路径
    if (!filePath || strlen(filePath) == 0) {
        updateAudioError("Invalid file path");
        return;
    }
    
    // 尝试打开文件
    if (!audioFile->open(filePath)) {
        updateAudioError("Failed to open file");
        return;
    }
    
    // 创建ID3源，使用更安全的方式
    id3Source = new(std::nothrow) AudioFileSourceID3(audioFile);
    if (!id3Source) {
        updateAudioError("Failed to create ID3 source");
        audioFile->close();
        return;
    }
    
    // 设置ID3回调
    id3Source->RegisterMetadataCB(metadataCallback, this);
    
    // 尝试开始播放
    if (mp3Generator && mp3Generator->begin(id3Source, audioOutput)) {
        updateAudioStatus(true, false, filePath);
    } else {
        updateAudioError("Failed to start playback");
        // 清理资源
        if (id3Source) {
            delete id3Source;
            id3Source = nullptr;
        }
        audioFile->close();
    }
}

void MusicApp::pauseAudioPlayback() {
    if (mp3Generator && mp3Generator->isRunning()) {
        mp3Generator->stop();
        updateAudioStatus(false, true, "");
    }
}

void MusicApp::resumeAudioPlayback() {
    // 安全地获取当前文件信息
    int fileIndex = -1;
    
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        fileIndex = audioStatus.currentFileIndex;
        xSemaphoreGive(audioStatusMutex);
    }
    
    // 检查文件索引的有效性（需要从主线程获取musicFileCount）
    // 为了安全起见，我们不直接访问musicFiles数组
    // 而是通过发送命令让主线程处理
    if (fileIndex >= 0) {
        // 发送播放当前歌曲的命令到主线程
        AudioTaskCommand cmd;
        cmd.cmd = AUDIO_CMD_PLAY;
        cmd.param = fileIndex;
        cmd.filePath[0] = '\0';
        
        // 这里我们需要一个更安全的方式来获取文件路径
        // 暂时使用简单的重新开始播放
        updateAudioStatus(true, false, "");
    }
}

void MusicApp::stopAudioPlayback() {
    // 按正确顺序停止和清理
    if (mp3Generator && mp3Generator->isRunning()) {
        mp3Generator->stop();
        // 等待停止完成
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    if (audioOutput) {
        audioOutput->flush();
        audioOutput->stop();
    }
    
    // 清理ID3源
    if (id3Source) {
        delete id3Source;
        id3Source = nullptr;
    }
    
    // 关闭文件
    if (audioFile && audioFile->isOpen()) {
        audioFile->close();
    }
    
    // 更新状态
    updateAudioStatus(false, false, "");
}

void MusicApp::playNextAudio() {
    // 这个函数现在只是一个占位符，实际逻辑在主线程处理
    updateAudioError("Use main thread for next/prev");
}

void MusicApp::playPreviousAudio() {
    // 这个函数现在只是一个占位符，实际逻辑在主线程处理  
    updateAudioError("Use main thread for next/prev");
}

void MusicApp::setAudioVolume(int volume) {
    M5Cardputer.Speaker.setVolume((volume * 255) / 100);
    
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        audioStatus.currentVolume = volume;
        xSemaphoreGive(audioStatusMutex);
    }
}

void MusicApp::updateAudioStatus(bool playing, bool paused, const char* songPath) {
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        audioStatus.isPlaying = playing;
        audioStatus.isPaused = paused;
        audioStatus.hasError = false;
        audioStatus.errorMessage[0] = '\0';
        
        if (songPath && strlen(songPath) > 0) {
            // 从路径中提取文件名
            const char* fileName = strrchr(songPath, '/');
            if (fileName) {
                fileName++; // 跳过 '/'
            } else {
                fileName = songPath;
            }
            
            strncpy(audioStatus.currentSongName, fileName, sizeof(audioStatus.currentSongName) - 1);
            audioStatus.currentSongName[sizeof(audioStatus.currentSongName) - 1] = '\0';
            
            // 查找文件索引
            for (int i = 0; i < musicFileCount; i++) {
                if (strcmp(musicFiles[i].path.c_str(), songPath) == 0) {
                    audioStatus.currentFileIndex = i;
                    break;
                }
            }
        }
        
        xSemaphoreGive(audioStatusMutex);
    }
}

void MusicApp::updateAudioError(const char* errorMsg) {
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        audioStatus.hasError = true;
        strncpy(audioStatus.errorMessage, errorMsg, sizeof(audioStatus.errorMessage) - 1);
        audioStatus.errorMessage[sizeof(audioStatus.errorMessage) - 1] = '\0';
        xSemaphoreGive(audioStatusMutex);
    }
}

void MusicApp::cleanupAudioTask() {
    if (mp3Generator) {
        delete mp3Generator;
        mp3Generator = nullptr;
    }
    
    if (audioOutput) {
        delete audioOutput;
        audioOutput = nullptr;
    }
    
    if (audioFile) {
        delete audioFile;
        audioFile = nullptr;
    }
}

// UI相关方法（在主线程中运行）
void MusicApp::scanMusicFiles() {
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

void MusicApp::playCurrentSong() {
    if (!isInitialized || musicFileCount == 0 || currentFileIndex < 0 || currentFileIndex >= musicFileCount) {
        statusLabel->setText("No song selected");
        return;
    }
    
    String filePath = musicFiles[currentFileIndex].path;
    sendAudioCommand(AUDIO_CMD_PLAY, 0, filePath.c_str());
}

void MusicApp::playSelectedSong() {
    MenuItem* selectedItem = playList->getSelectedItem();
    if (selectedItem && selectedItem->id >= 0 && selectedItem->id < musicFileCount) {
        currentFileIndex = selectedItem->id;
        playCurrentSong();
    }
}

void MusicApp::adjustVolume(int delta) {
    currentVolume += delta;
    if (currentVolume < 0) currentVolume = 0;
    if (currentVolume > 100) currentVolume = 100;
    
    sendAudioCommand(AUDIO_CMD_VOLUME, currentVolume);
}

void MusicApp::setVolume(int volume) {
    currentVolume = volume;
    if (currentVolume < 0) currentVolume = 0;
    if (currentVolume > 100) currentVolume = 100;
    
    sendAudioCommand(AUDIO_CMD_VOLUME, currentVolume);
}

void MusicApp::updateSongInfo() {
    if (musicFileCount > 0 && currentFileIndex >= 0 && currentFileIndex < musicFileCount) {
        String info = "(" + String(currentFileIndex + 1) + "/" + String(musicFileCount) + ") ";
        info += musicFiles[currentFileIndex].name;
        songLabel->setText(info);
    }
}

void MusicApp::cleanup() {
    // 重置状态
    isInitialized = false;
    isPlaying = false;
    isPaused = false;
    pausedPosition = 0;
}

void MusicApp::drawInterface() {
    uiManager->refresh();
}

// 静态回调函数
void MusicApp::metadataCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
    // 可以在这里处理ID3标签信息
    // 暂时不做处理，避免复杂化
}
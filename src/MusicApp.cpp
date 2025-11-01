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
    
    // 初始化菜单状态
    menuState.level = MENU_MAIN;
    menuState.currentArtist = "";
    menuState.currentAlbum = "";
    menuState.selectedIndex = 0;
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
    
    // 清理音乐分类数据
    clearMusicData();
    
    cleanup();
}

void MusicApp::setup() {
    // 配置 M5Cardputer 扬声器 - 使用更高的采样率提升音质
    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = 128000; // 使用 128kHz 采样率，与参考实现一致
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5Cardputer.Speaker.config(spk_cfg);
    
    // 创建主窗口 - 扩大窗口尺寸以容纳底部UI
    mainWindow = new UIWindow(WINDOW_ID, 20, 15, 200, 130);
    uiManager->addWidget(mainWindow);
    
    // 创建标题
    titleLabel = new UILabel(TITLE_LABEL_ID, 25, 20, "Music Player");
    titleLabel->setTextColor(TFT_WHITE);
    uiManager->addWidget(titleLabel);
    
    // 创建歌曲信息标签
    songLabel = new UILabel(SONG_LABEL_ID, 25, 30, "No song loaded");
    songLabel->setTextColor(TFT_YELLOW);
    uiManager->addWidget(songLabel);
    
    // 创建播放列表 - 调整高度为底部UI留出空间
    playList = new MusicMenuList(PLAYLIST_ID, 25, 45, 190, 50, "playlist", 10, this);
    playList->setColors(TFT_WHITE, TFT_BLUE, TFT_WHITE, TFT_DARKGREY);
    uiManager->addWidget(playList);
    
    // 创建当前播放曲名标签 - 位于底部
    nowPlayingLabel = new UILabel(NOW_PLAYING_LABEL_ID, 25, 100, "♪ Not Playing");
    nowPlayingLabel->setTextColor(TFT_CYAN);
    uiManager->addWidget(nowPlayingLabel);
    
    // 创建音量滑块 - 位于底部
    volumeSlider = new VolumeSlider(VOLUME_SLIDER_ID, 25, 115, 120, 12, "volume");
    uiManager->addWidget(volumeSlider);
    
    // 创建状态标签 - 调整位置到右下角
    statusLabel = new UILabel(STATUS_LABEL_ID, 150, 115, "Ready");
    statusLabel->setTextColor(TFT_GREEN);
    uiManager->addWidget(statusLabel);
    
    // 设置焦点
    uiManager->nextFocus();
    
    // 初始化双核心音频系统
    initializeDualCoreAudio();
    
    // 扫描音乐文件并分类
    scanMusicFiles();
    
    // 构建主菜单
    buildMainMenu();
    
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
    // 处理Esc键 - 返回上一级菜单
    if (event.esc) {
        navigateBack();
        return;
    }
    
    // 只保留基本的播放控制快捷键，其他按键交给UI系统处理
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
                return; // 处理完毕，直接返回
        }
    }
    
    // 左右箭头键控制音量
    if (event.left || event.right) {
        int volumeDelta = event.right ? 5 : -5;
        adjustVolume(volumeDelta);
        return; // 处理完毕，直接返回
    }
    
    // 将其他所有按键事件（包括上下键、Enter等）交给UI管理器统一处理
    if (uiManager->handleKeyEvent(event)) {
        uiManager->refreshAppArea();
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
        
        // 更新当前播放曲名显示
        if (nowPlayingLabel) {
            if (audioStatus.isPlaying && strlen(audioStatus.currentSongName) > 0) {
                String nowPlaying = "♪ " + String(audioStatus.currentSongName);
                // 如果曲名太长，截断显示
                if (nowPlaying.length() > 25) {
                    nowPlaying = nowPlaying.substring(0, 22) + "...";
                }
                nowPlayingLabel->setText(nowPlaying);
                nowPlayingLabel->setTextColor(TFT_CYAN);
            } else if (audioStatus.isPaused) {
                nowPlayingLabel->setText("⏸ Paused");
                nowPlayingLabel->setTextColor(TFT_ORANGE);
            } else {
                nowPlayingLabel->setText("♪ Not Playing");
                nowPlayingLabel->setTextColor(TFT_DARKGREY);
            }
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
    
    // 清理之前的数据
    clearMusicData();
    
    // 扫描所有MP3文件
    musicFileCount = 0;
    fileManager.scanAllFiles(musicFiles, musicFileCount, MAX_MUSIC_FILES, ".mp3");
    
    if (musicFileCount > 0) {
        statusLabel->setText("Categorizing music files...");
        drawInterface();
        
        // 分类音乐文件
        categorizeMusic();
        
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
    if (!selectedItem) return;
    
    if (menuState.level == MENU_TRACKS) {
        // 在曲目列表中，播放选中的曲目
        std::vector<MusicTrack*> tracksToPlay;
        
        if (menuState.currentArtist.isEmpty() && menuState.currentAlbum.isEmpty()) {
            // 未分类曲目
            tracksToPlay = uncategorizedTracks;
        } else if (!menuState.currentArtist.isEmpty() && menuState.currentAlbum.isEmpty()) {
            // 艺术家的所有曲目
            Artist* artist = findOrCreateArtist(menuState.currentArtist);
            for (Album* album : artist->albums) {
                for (MusicTrack* track : album->tracks) {
                    tracksToPlay.push_back(track);
                }
            }
            for (MusicTrack* track : artist->singleTracks) {
                tracksToPlay.push_back(track);
            }
        } else {
            // 特定专辑的曲目
            Artist* artist = findOrCreateArtist(menuState.currentArtist);
            for (Album* album : artist->albums) {
                if (album->name.equalsIgnoreCase(menuState.currentAlbum)) {
                    tracksToPlay = album->tracks;
                    break;
                }
            }
        }
        
        if (selectedItem->id >= 0 && selectedItem->id < (int)tracksToPlay.size()) {
            MusicTrack* track = tracksToPlay[selectedItem->id];
            
            // 找到对应的文件索引
            for (int i = 0; i < musicFileCount; i++) {
                if (String(musicFiles[i].path) == track->filePath) {
                    currentFileIndex = i;
                    playCurrentSong();
                    return;
                }
            }
        }
    }
}

void MusicApp::adjustVolume(int delta) {
    currentVolume += delta;
    if (currentVolume < 0) currentVolume = 0;
    if (currentVolume > 100) currentVolume = 100;
    
    // 更新音量滑块显示
    if (volumeSlider) {
        volumeSlider->setValue(currentVolume);
    }
    
    sendAudioCommand(AUDIO_CMD_VOLUME, currentVolume);
    
    // 刷新UI显示
    uiManager->refreshAppArea();
}

void MusicApp::setVolume(int volume) {
    currentVolume = volume;
    if (currentVolume < 0) currentVolume = 0;
    if (currentVolume > 100) currentVolume = 100;
    
    // 更新音量滑块显示
    if (volumeSlider) {
        volumeSlider->setValue(currentVolume);
    }
    
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
    // 使用智能刷新，根据是否有前景层选择合适的刷新方式
    uiManager->smartRefresh();
}

// 静态回调函数
void MusicApp::metadataCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
    // 可以在这里处理ID3标签信息
    // 暂时不做处理，避免复杂化
}

// 音乐分类和菜单导航方法实现
void MusicApp::categorizeMusic() {
    // 解析所有音乐文件
    for (int i = 0; i < musicFileCount; i++) {
        MusicTrack* track = parseFileName(musicFiles[i].name, musicFiles[i].path);
        if (track) {
            allTracks.push_back(track);
            
            if (track->artist.isEmpty() || track->album.isEmpty()) {
                // 无法分类的文件
                uncategorizedTracks.push_back(track);
            } else {
                // 找到或创建艺术家
                Artist* artist = findOrCreateArtist(track->artist);
                
                // 找到或创建专辑
                Album* album = findOrCreateAlbum(artist, track->album);
                
                // 添加曲目到专辑
                album->tracks.push_back(track);
            }
        }
    }
}

MusicTrack* MusicApp::parseFileName(const String& fileName, const String& filePath) {
    MusicTrack* track = new MusicTrack();
    track->fileName = fileName;
    track->filePath = filePath;
    
    // 移除.mp3扩展名
    String nameWithoutExt = fileName;
    if (nameWithoutExt.endsWith(".mp3")) {
        nameWithoutExt = nameWithoutExt.substring(0, nameWithoutExt.length() - 4);
    }
    
    // 尝试解析 "艺术家-专辑-曲名" 格式
    int firstDash = nameWithoutExt.indexOf('-');
    if (firstDash > 0) {
        int secondDash = nameWithoutExt.indexOf('-', firstDash + 1);
        if (secondDash > firstDash + 1) {
            // 找到两个破折号，按格式解析
            track->artist = nameWithoutExt.substring(0, firstDash);
            track->album = nameWithoutExt.substring(firstDash + 1, secondDash);
            track->title = nameWithoutExt.substring(secondDash + 1);
            
            // 去除前后空格
            track->artist.trim();
            track->album.trim();
            track->title.trim();
        } else {
            // 只有一个破折号，可能是 "艺术家-曲名" 格式
            track->artist = nameWithoutExt.substring(0, firstDash);
            track->title = nameWithoutExt.substring(firstDash + 1);
            track->artist.trim();
            track->title.trim();
            track->album = ""; // 没有专辑信息
        }
    } else {
        // 没有破折号，整个文件名作为曲名
        track->title = nameWithoutExt;
        track->artist = "";
        track->album = "";
    }
    
    return track;
}

Artist* MusicApp::findOrCreateArtist(const String& artistName) {
    // 查找现有艺术家
    for (Artist* artist : artists) {
        if (artist->name.equalsIgnoreCase(artistName)) {
            return artist;
        }
    }
    
    // 创建新艺术家
    Artist* newArtist = new Artist();
    newArtist->name = artistName;
    artists.push_back(newArtist);
    return newArtist;
}

Album* MusicApp::findOrCreateAlbum(Artist* artist, const String& albumName) {
    // 在艺术家的专辑中查找
    for (Album* album : artist->albums) {
        if (album->name.equalsIgnoreCase(albumName)) {
            return album;
        }
    }
    
    // 创建新专辑
    Album* newAlbum = new Album();
    newAlbum->name = albumName;
    newAlbum->artist = artist->name;
    artist->albums.push_back(newAlbum);
    allAlbums.push_back(newAlbum);
    return newAlbum;
}

void MusicApp::clearMusicData() {
    // 清理所有动态分配的内存
    for (MusicTrack* track : allTracks) {
        delete track;
    }
    allTracks.clear();
    uncategorizedTracks.clear();
    
    for (Album* album : allAlbums) {
        delete album;
    }
    allAlbums.clear();
    
    for (Artist* artist : artists) {
        delete artist;
    }
    artists.clear();
}

void MusicApp::buildMainMenu() {
    playList->clear();
    menuState.level = MENU_MAIN;
    menuState.currentArtist = "";
    menuState.currentAlbum = "";
    
    // 添加主菜单项
    playList->addItem("📁 Albums (" + String(allAlbums.size()) + ")", 0, "");
    playList->addItem("🎤 Artists (" + String(artists.size()) + ")", 1, "");
    playList->addItem("📄 Uncategorized (" + String(uncategorizedTracks.size()) + ")", 2, "");
    
    titleLabel->setText("Music Library");
    songLabel->setText("Select a category");
}

void MusicApp::buildArtistsMenu() {
    playList->clear();
    menuState.level = MENU_ARTISTS;
    
    // 添加返回选项
    playList->addItem("📁 ../", -1, "");
    
    for (size_t i = 0; i < artists.size(); i++) {
        Artist* artist = artists[i];
        int totalTracks = 0;
        for (Album* album : artist->albums) {
            totalTracks += album->tracks.size();
        }
        totalTracks += artist->singleTracks.size();
        
        playList->addItem("🎤 " + artist->name + " (" + String(totalTracks) + ")", i, "");
    }
    
    titleLabel->setText("Artists");
    songLabel->setText("Select an artist");
}

void MusicApp::buildAlbumsMenu(const String& artistName) {
    playList->clear();
    menuState.level = MENU_ALBUMS;
    menuState.currentArtist = artistName;
    
    // 添加返回选项
    playList->addItem("../", -1, "");
    
    if (artistName.isEmpty()) {
        // 显示所有专辑
        for (size_t i = 0; i < allAlbums.size(); i++) {
            Album* album = allAlbums[i];
            playList->addItem("" + album->name + " - " + album->artist + " (" + String(album->tracks.size()) + ")", i, "");
        }
        titleLabel->setText("All Albums");
    } else {
        // 显示特定艺术家的专辑
        Artist* artist = findOrCreateArtist(artistName);
        int index = 0;
        for (Album* album : artist->albums) {
            playList->addItem("" + album->name + " (" + String(album->tracks.size()) + ")", index++, "");
        }
        titleLabel->setText(artistName + " - Albums");
    }
    
    songLabel->setText("Select an album");
}

void MusicApp::buildTracksMenu(const String& artistName, const String& albumName) {
    playList->clear();
    menuState.level = MENU_TRACKS;
    menuState.currentArtist = artistName;
    menuState.currentAlbum = albumName;
    
    // 添加返回选项
    playList->addItem("../", -1, "");
    
    std::vector<MusicTrack*> tracksToShow;
    
    if (artistName.isEmpty() && albumName.isEmpty()) {
        // 显示未分类的曲目
        tracksToShow = uncategorizedTracks;
        titleLabel->setText("Uncategorized");
    } else if (!artistName.isEmpty() && albumName.isEmpty()) {
        // 显示艺术家的所有曲目
        Artist* artist = findOrCreateArtist(artistName);
        for (Album* album : artist->albums) {
            for (MusicTrack* track : album->tracks) {
                tracksToShow.push_back(track);
            }
        }
        for (MusicTrack* track : artist->singleTracks) {
            tracksToShow.push_back(track);
        }
        titleLabel->setText(artistName + " - All Tracks");
    } else {
        // 显示特定专辑的曲目
        Artist* artist = findOrCreateArtist(artistName);
        for (Album* album : artist->albums) {
            if (album->name.equalsIgnoreCase(albumName)) {
                tracksToShow = album->tracks;
                break;
            }
        }
        titleLabel->setText(albumName);
    }
    
    // 添加曲目到列表
    for (size_t i = 0; i < tracksToShow.size(); i++) {
        MusicTrack* track = tracksToShow[i];
        String displayName = "♪ " + track->title;
        if (!track->artist.isEmpty() && artistName.isEmpty()) {
            displayName += " - " + track->artist;
        }
        playList->addItem(displayName, i, "");
    }
    
    songLabel->setText("Select a track to play");
}

void MusicApp::navigateBack() {
    switch (menuState.level) {
        case MENU_MAIN:
            // 已经在主菜单，无法返回
            break;
            
        case MENU_ARTISTS:
        case MENU_ALBUMS:
            buildMainMenu();
            break;
            
        case MENU_TRACKS:
            if (!menuState.currentArtist.isEmpty() && !menuState.currentAlbum.isEmpty()) {
                // 从专辑曲目返回到专辑列表
                buildAlbumsMenu(menuState.currentArtist);
            } else if (!menuState.currentArtist.isEmpty()) {
                // 从艺术家曲目返回到艺术家列表
                buildArtistsMenu();
            } else {
                // 从未分类曲目返回到主菜单
                buildMainMenu();
            }
            break;
    }
    
    updateMenuDisplay();
}

void MusicApp::navigateForward() {
    MenuItem* selectedItem = playList->getSelectedItem();
    if (!selectedItem) return;
    
    switch (menuState.level) {
        case MENU_MAIN:
            switch (selectedItem->id) {
                case 0: // Albums
                    buildAlbumsMenu();
                    break;
                case 1: // Artists
                    buildArtistsMenu();
                    break;
                case 2: // Uncategorized
                    buildTracksMenu();
                    break;
            }
            break;
            
        case MENU_ARTISTS:
            if (selectedItem->id >= 0 && selectedItem->id < (int)artists.size()) {
                Artist* artist = artists[selectedItem->id];
                if (artist->albums.size() > 1) {
                    // 艺术家有多个专辑，显示专辑列表
                    buildAlbumsMenu(artist->name);
                } else {
                    // 艺术家只有一个专辑或只有单曲，直接显示曲目
                    buildTracksMenu(artist->name);
                }
            }
            break;
            
        case MENU_ALBUMS:
            if (!menuState.currentArtist.isEmpty()) {
                // 在艺术家的专辑列表中
                Artist* artist = findOrCreateArtist(menuState.currentArtist);
                if (selectedItem->id >= 0 && selectedItem->id < (int)artist->albums.size()) {
                    Album* album = artist->albums[selectedItem->id];
                    buildTracksMenu(artist->name, album->name);
                }
            } else {
                // 在所有专辑列表中
                if (selectedItem->id >= 0 && selectedItem->id < (int)allAlbums.size()) {
                    Album* album = allAlbums[selectedItem->id];
                    buildTracksMenu(album->artist, album->name);
                }
            }
            break;
            
        case MENU_TRACKS:
            // 播放选中的曲目
            playSelectedSong();
            break;
    }
    
    updateMenuDisplay();
}

void MusicApp::updateMenuDisplay() {
    uiManager->refreshAppArea();
}

void MusicApp::handleMenuSelection(MenuItem* item) {
    if (!item) return;
    
    // 处理返回选项（ID为-1）
    if (item->id == -1) {
        navigateBack();
        return;
    }
    
    switch (menuState.level) {
        case MENU_MAIN:
            switch (item->id) {
                case 0: // Albums
                    buildAlbumsMenu();
                    break;
                case 1: // Artists
                    buildArtistsMenu();
                    break;
                case 2: // Uncategorized
                    buildTracksMenu();
                    break;
            }
            break;
            
        case MENU_ARTISTS:
            if (item->id >= 0 && item->id < (int)artists.size()) {
                Artist* artist = artists[item->id];
                if (artist->albums.size() > 1) {
                    // 艺术家有多个专辑，显示专辑列表
                    buildAlbumsMenu(artist->name);
                } else {
                    // 艺术家只有一个专辑或只有单曲，直接显示曲目
                    buildTracksMenu(artist->name);
                }
            }
            break;
            
        case MENU_ALBUMS:
            if (!menuState.currentArtist.isEmpty()) {
                // 在艺术家的专辑列表中
                Artist* artist = findOrCreateArtist(menuState.currentArtist);
                if (item->id >= 0 && item->id < (int)artist->albums.size()) {
                    Album* album = artist->albums[item->id];
                    buildTracksMenu(artist->name, album->name);
                }
            } else {
                // 在所有专辑列表中
                if (item->id >= 0 && item->id < (int)allAlbums.size()) {
                    Album* album = allAlbums[item->id];
                    buildTracksMenu(album->artist, album->name);
                }
            }
            break;
            
        case MENU_TRACKS:
            // 播放选中的曲目
            playSelectedSong();
            break;
    }
    
    updateMenuDisplay();
}
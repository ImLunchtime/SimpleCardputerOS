#include "apps/Music/MusicApp.h"
#include "apps/Music/MusicAppPlayback.h"
#include "apps/Music/MusicAppLyrics.h"
#include "apps/Music/MusicAppUI.h"

MusicApp::MusicApp(EventSystem* events, AppManager* manager) 
    : eventSystem(events), appManager(manager), 
      audioFile(nullptr), audioOutput(nullptr), mp3Generator(nullptr), id3Source(nullptr),
      isPlaying(false), isPaused(false), isInitialized(false), pausedPosition(0),
      currentVolume(10), musicFileCount(0), currentFileIndex(0),
      audioTaskHandle(nullptr), audioCommandQueue(nullptr), audioStatusMutex(nullptr),
      scanTaskHandle(nullptr), scanInProgress(false), scanCompleted(false), scanFailed(false), menuBuilt(false) {
    uiManager = appManager->getUIManager();
    exitRequested = false;
    
    memset(&audioStatus, 0, sizeof(audioStatus));
    audioStatus.currentVolume = currentVolume;
    audioStatus.currentFileIndex = -1;
    
    menuState.level = MENU_MAIN;
    menuState.currentArtist = "";
    menuState.currentAlbum = "";
    menuState.selectedIndex = 0;

    lyricsAvailable = false;
    currentLyricIndex = -1;
    lastLyricsFileIndex = -1;
    playbackStartMillis = 0;
    pauseStartMillis = 0;
    pausedAccumulatedMillis = 0;
    lastIsPlayingFlag = false;
    lastIsPausedFlag = false;
    lastDisplayedCurrent = "";
    lastDisplayedNext = "";

    mainWindow = nullptr;
    playerWindow = nullptr;
    titleLabel = nullptr;
    songLabel = nullptr;
    playerSongLabel = nullptr;
    playList = nullptr;
    lyricsCurrentLabel = nullptr;
    lyricsNextLabel = nullptr;
    volumeSlider = nullptr;

    viewMode = VIEW_LIST;
}

MusicApp::~MusicApp() {
    if (audioTaskHandle) {
        sendAudioCommand(AUDIO_CMD_SHUTDOWN);
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(audioTaskHandle);
        audioTaskHandle = nullptr;
    }
    
    if (audioCommandQueue) {
        vQueueDelete(audioCommandQueue);
        audioCommandQueue = nullptr;
    }
    
    if (audioStatusMutex) {
        vSemaphoreDelete(audioStatusMutex);
        audioStatusMutex = nullptr;
    }
    
    if (scanTaskHandle) {
        vTaskDelete(scanTaskHandle);
        scanTaskHandle = nullptr;
    }
    
    clearMusicData();
    
    cleanup();
}

void MusicApp::setup() {
    if (scanTaskHandle) {
        exitRequested = true;
        uint32_t startMs = millis();
        while (scanTaskHandle && (millis() - startMs < 3000)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    exitRequested = false;
    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = 128000;
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5Cardputer.Speaker.config(spk_cfg);

    if (appManager) {
        appManager->setGlobalEscEnabled(false);
    }
    
    mainWindow = new UIWindow(WINDOW_ID, 20, 15, 200, 116);
    uiManager->addWidget(mainWindow);
    
    mainWindow->setChildOffset(-20, -16);
    
    titleLabel = new UILabel(TITLE_LABEL_ID, 25, 20, "Music");
    titleLabel->setParent(mainWindow);
    titleLabel->setTextColor(TFT_WHITE);
    uiManager->addWidget(titleLabel);
    
    songLabel = new UILabel(SONG_LABEL_ID, 25, 30, "No song loaded");
    songLabel->setParent(mainWindow);
    songLabel->setTextColor(TFT_YELLOW);
    uiManager->addWidget(songLabel);
    
    playList = new MusicMenuList(PLAYLIST_ID, 25, 45, 190, 65, "playlist", this);
    playList->setParent(mainWindow);
    playList->setColors(TFT_WHITE, TFT_BLUE, TFT_WHITE, TFT_DARKGREY);
    uiManager->addWidget(playList);
    
    UILabel* listHintLabel = new UILabel(11, 25, 115, "[P] Open Player");
    listHintLabel->setParent(mainWindow);
    listHintLabel->setTextColor(TFT_DARKGREY);
    uiManager->addWidget(listHintLabel);
    
    playerWindow = new UIWindow(9, 20, 15, 200, 116);
    uiManager->addWidget(playerWindow);
    playerWindow->setChildOffset(-20, -16);
    
    playerSongLabel = new UILabel(10, 25, 20, "No song loaded");
    playerSongLabel->setParent(playerWindow);
    playerSongLabel->setTextColor(TFT_YELLOW);
    uiManager->addWidget(playerSongLabel);
    
    lyricsCurrentLabel = new UILabel(LYRICS_CURRENT_LABEL_ID, 25, 40, "");
    lyricsCurrentLabel->setParent(playerWindow);
    lyricsCurrentLabel->setTextColor(TFT_CYAN);
    uiManager->addWidget(lyricsCurrentLabel);

    lyricsNextLabel = new UILabel(LYRICS_NEXT_LABEL_ID, 25, 55, "");
    lyricsNextLabel->setParent(playerWindow);
    lyricsNextLabel->setTextColor(TFT_DARKGREY);
    uiManager->addWidget(lyricsNextLabel);
    
    volumeSlider = new VolumeSlider(VOLUME_SLIDER_ID, 25, 90, 120, 12, "volume");
    volumeSlider->setParent(playerWindow);
    uiManager->addWidget(volumeSlider);
    
    UILabel* playerHintLabel = new UILabel(12, 25, 110, "[+] [-] Vol. [Space] Stop");
    playerHintLabel->setParent(playerWindow);
    playerHintLabel->setTextColor(TFT_DARKGREY);
    uiManager->addWidget(playerHintLabel);
    
    uiManager->nextFocus();
    
    initializeDualCoreAudio();
    
    scanMusicFiles();
    
    showListView();
}

void MusicApp::loop() {
    updateAudioStatus();
    
    handleNextPrevRequests();
    
    updateUIFromAudioStatus();
    updateLyricsDisplay();
    
    if (!menuBuilt && !scanInProgress && (scanCompleted || scanFailed)) {
        TipManager* tip = appManager ? appManager->getTipManager() : nullptr;
        if (tip) {
            tip->closeTip();
        }
        if (scanFailed) {
            songLabel->setText("Scan failed");
        } else if (musicFileCount <= 0) {
            songLabel->setText("No MP3 files found");
        } else {
            songLabel->setText("Found " + String(musicFileCount) + " music files");
            currentFileIndex = 0;
            updateSongInfo();
        }
        buildMainMenu();
        menuBuilt = true;
        uiManager->refreshAppArea();
    }
}

void MusicApp::onKeyEvent(const KeyEvent& event) {
    if (event.esc) {
        if (viewMode == VIEW_PLAYER) {
            showListView();
        } else {
            shutdownForExit();
            if (appManager) {
                appManager->returnToLauncher();
            }
        }
        return;
    }
    
    if (!event.text.isEmpty()) {
        char key = event.text.charAt(0);
        switch (key) {
            case ' ':
                if (viewMode == VIEW_PLAYER) {
                    if (audioStatus.isPlaying) {
                        sendAudioCommand(AUDIO_CMD_PAUSE);
                    } else if (audioStatus.isPaused) {
                        sendAudioCommand(AUDIO_CMD_PLAY);
                    }
                    return;
                }
                break;
            case 'p':
            case 'P':
                if (viewMode == VIEW_LIST) {
                    if (audioStatus.isPlaying || audioStatus.isPaused) {
                        showPlayerView();
                        return;
                    }
                }
                break;
        }
    }
    
    if (viewMode == VIEW_PLAYER && (event.left || event.right)) {
        int volumeDelta = event.right ? 5 : -5;
        adjustVolume(volumeDelta);
        return;
    }
    
    if (uiManager->handleKeyEvent(event)) {
        uiManager->refreshAppArea();
    }
}

void MusicApp::showListView() {
    viewMode = VIEW_LIST;
    if (!uiManager) return;
    if (playerWindow) {
        uiManager->hidePage(playerWindow);
    }
    if (mainWindow) {
        uiManager->showPage(mainWindow);
    }
    uiManager->refresh();
}

void MusicApp::showPlayerView() {
    viewMode = VIEW_PLAYER;
    if (!uiManager) return;
    if (mainWindow) {
        uiManager->hidePage(mainWindow);
    }
    if (playerWindow) {
        uiManager->showPage(playerWindow);
    }
    uiManager->refresh();
}

void MusicApp::shutdownForExit() {
    exitRequested = true;

    if (audioCommandQueue) {
        AudioTaskCommand command;
        command.cmd = AUDIO_CMD_SHUTDOWN;
        command.param = 0;
        command.filePath[0] = '\0';
        xQueueSend(audioCommandQueue, &command, pdMS_TO_TICKS(200));
    }

    uint32_t startMs = millis();
    while (audioTaskHandle && (millis() - startMs < 800)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (audioTaskHandle) {
        vTaskDelete(audioTaskHandle);
        audioTaskHandle = nullptr;
    }

    if (audioCommandQueue) {
        vQueueDelete(audioCommandQueue);
        audioCommandQueue = nullptr;
    }
    if (audioStatusMutex) {
        vSemaphoreDelete(audioStatusMutex);
        audioStatusMutex = nullptr;
    }

    clearMusicData();
    cleanup();
    mainWindow = nullptr;
    playerWindow = nullptr;
    titleLabel = nullptr;
    songLabel = nullptr;
    playerSongLabel = nullptr;
    playList = nullptr;
    lyricsCurrentLabel = nullptr;
    lyricsNextLabel = nullptr;
    volumeSlider = nullptr;
    menuBuilt = false;
    scanCompleted = false;
    scanFailed = false;
}

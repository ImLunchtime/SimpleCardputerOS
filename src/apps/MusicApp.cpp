#include "apps/MusicApp.h"
#include "apps/MusicAppPlayback.h"
#include "apps/MusicAppLyrics.h"
#include "apps/MusicAppUI.h"

MusicApp::MusicApp(EventSystem* events, AppManager* manager) 
    : eventSystem(events), appManager(manager), 
      audioFile(nullptr), audioOutput(nullptr), mp3Generator(nullptr), id3Source(nullptr),
      isPlaying(false), isPaused(false), isInitialized(false), pausedPosition(0),
      currentVolume(50), musicFileCount(0), currentFileIndex(0),
      audioTaskHandle(nullptr), audioCommandQueue(nullptr), audioStatusMutex(nullptr) {
    uiManager = appManager->getUIManager();
    
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
    
    clearMusicData();
    
    cleanup();
}

void MusicApp::setup() {
    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = 128000;
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5Cardputer.Speaker.config(spk_cfg);
    
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
    
    playList = new MusicMenuList(PLAYLIST_ID, 25, 45, 190, 50, "playlist", 10, this);
    playList->setParent(mainWindow);
    playList->setColors(TFT_WHITE, TFT_BLUE, TFT_WHITE, TFT_DARKGREY);
    uiManager->addWidget(playList);
    
    lyricsCurrentLabel = new UILabel(LYRICS_CURRENT_LABEL_ID, 25, 87, "");
    lyricsCurrentLabel->setParent(mainWindow);
    lyricsCurrentLabel->setTextColor(TFT_CYAN);
    uiManager->addWidget(lyricsCurrentLabel);

    lyricsNextLabel = new UILabel(LYRICS_NEXT_LABEL_ID, 25, 100, "");
    lyricsNextLabel->setParent(mainWindow);
    lyricsNextLabel->setTextColor(TFT_DARKGREY);
    uiManager->addWidget(lyricsNextLabel);
    
    volumeSlider = new VolumeSlider(VOLUME_SLIDER_ID, 25, 115, 120, 12, "volume");
    volumeSlider->setParent(mainWindow);
    uiManager->addWidget(volumeSlider);
    
    uiManager->nextFocus();
    
    initializeDualCoreAudio();
    
    scanMusicFiles();
    
    buildMainMenu();
    
    drawInterface();
}

void MusicApp::loop() {
    updateAudioStatus();
    
    handleNextPrevRequests();
    
    updateUIFromAudioStatus();
    updateLyricsDisplay();
}

void MusicApp::onKeyEvent(const KeyEvent& event) {
    if (event.esc) {
        navigateBack();
        return;
    }
    
    if (!event.text.isEmpty()) {
        char key = event.text.charAt(0);
        switch (key) {
            case ' ':
                if (audioStatus.isPlaying) {
                    sendAudioCommand(AUDIO_CMD_PAUSE);
                } else if (audioStatus.isPaused) {
                    sendAudioCommand(AUDIO_CMD_PLAY);
                } else {
                    playCurrentSong();
                }
                return;
        }
    }
    
    if (event.left || event.right) {
        int volumeDelta = event.right ? 5 : -5;
        adjustVolume(volumeDelta);
        return;
    }
    
    if (uiManager->handleKeyEvent(event)) {
        uiManager->refreshAppArea();
    }
}

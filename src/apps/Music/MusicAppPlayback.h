#pragma once
#include "apps/Music/MusicApp.h"

inline void MusicApp::initializeDualCoreAudio() {
    cleanup();
    
    SDFileManager* fm = appManager->getSDFileManager();
    if (!fm || !fm->isInitialized()) {
        songLabel->setText("SD card not ready!");
        return;
    }
    
    audioCommandQueue = xQueueCreate(10, sizeof(AudioTaskCommand));
    if (!audioCommandQueue) {
        songLabel->setText("Failed to create audio command queue");
        return;
    }
    
    audioStatusMutex = xSemaphoreCreateMutex();
    if (!audioStatusMutex) {
        songLabel->setText("Failed to create audio status mutex");
        vQueueDelete(audioCommandQueue);
        audioCommandQueue = nullptr;
        return;
    }
    
    BaseType_t result = xTaskCreatePinnedToCore(
        audioTaskFunction,
        "AudioTask",
        16384,
        this,
        1,
        &audioTaskHandle,
        0
    );
    
    if (result != pdPASS) {
        songLabel->setText("Failed to create audio task");
        vQueueDelete(audioCommandQueue);
        vSemaphoreDelete(audioStatusMutex);
        audioCommandQueue = nullptr;
        audioStatusMutex = nullptr;
        return;
    }
    
    M5Cardputer.Speaker.setVolume(currentVolume);
    
    isInitialized = true;
    songLabel->setText("Ready");
}

inline void MusicApp::sendAudioCommand(AudioCommand cmd, int param, const char* filePath) {
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

inline void MusicApp::updateAudioStatus() {
    if (!audioStatusMutex) return;
    
    if (xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        isPlaying = audioStatus.isPlaying;
        isPaused = audioStatus.isPaused;
        currentFileIndex = audioStatus.currentFileIndex;
        currentVolume = audioStatus.currentVolume;
        
        xSemaphoreGive(audioStatusMutex);
    }
}

inline void MusicApp::handleNextPrevRequests() {
    if (xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
        if (audioStatus.hasError) {
            if (strstr(audioStatus.errorMessage, "Next requested") || strstr(audioStatus.errorMessage, "Song finished")) {
                playNextSong();
                audioStatus.hasError = false;
                strcpy(audioStatus.errorMessage, "");
            } else if (strstr(audioStatus.errorMessage, "Previous requested")) {
                playPreviousSong();
                audioStatus.hasError = false;
                strcpy(audioStatus.errorMessage, "");
            }
        }
        xSemaphoreGive(audioStatusMutex);
    }
}

inline void MusicApp::playNextSong() {
    if (musicFileCount == 0) return;
    
    currentFileIndex = (currentFileIndex + 1) % musicFileCount;
    sendAudioCommand(AUDIO_CMD_STOP);
    vTaskDelay(pdMS_TO_TICKS(100));
    sendAudioCommand(AUDIO_CMD_PLAY, 0, musicFiles[currentFileIndex].path.c_str());
    updateSongInfo();
    prepareLyricsForCurrentSong();
}

inline void MusicApp::playPreviousSong() {
    if (musicFileCount == 0) return;
    
    currentFileIndex = (currentFileIndex - 1 + musicFileCount) % musicFileCount;
    sendAudioCommand(AUDIO_CMD_STOP);
    vTaskDelay(pdMS_TO_TICKS(100));
    sendAudioCommand(AUDIO_CMD_PLAY, 0, musicFiles[currentFileIndex].path.c_str());
    updateSongInfo();
    prepareLyricsForCurrentSong();
}

inline void MusicApp::updateUIFromAudioStatus() {
    if (!audioStatusMutex) return;
    
    if (xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (strlen(audioStatus.currentSongName) > 0) {
            String info = "(" + String(audioStatus.currentFileIndex + 1) + "/" + String(musicFileCount) + ") ";
            info += String(audioStatus.currentSongName);
            songLabel->setText(info);
        }
        
        xSemaphoreGive(audioStatusMutex);
    }
}

inline void MusicApp::audioTaskFunction(void* parameter) {
    MusicApp* app = static_cast<MusicApp*>(parameter);
    app->processAudioCommands();
}

inline void MusicApp::processAudioCommands() {
    AudioTaskCommand command;
    
    audioFile = new(std::nothrow) AudioFileSourceSD();
    if (!audioFile) {
        updateAudioError("Failed to create AudioFileSourceSD");
        cleanupAudioTask();
        vTaskDelete(nullptr);
        return;
    }
    
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
    
    if (!audioOutput->begin()) {
        updateAudioError("Audio output init failed");
        cleanupAudioTask();
        vTaskDelete(nullptr);
        return;
    }
    
    audioOutput->SetRate(44100);
    audioOutput->SetBitsPerSample(16);
    audioOutput->SetChannels(2);
    
    while (true) {
        if (mp3Generator && mp3Generator->isRunning()) {
            if (!mp3Generator->loop()) {
                stopAudioPlayback();
                updateAudioError("Song finished");
            }
        }
        
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
                    updateAudioError("Next requested");
                    break;
                case AUDIO_CMD_PREV:
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
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

inline void MusicApp::playAudioFile(const char* filePath) {
    stopAudioPlayback();
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (!filePath || strlen(filePath) == 0) {
        updateAudioError("Invalid file path");
        return;
    }
    
    if (!audioFile->open(filePath)) {
        updateAudioError("Failed to open file");
        return;
    }
    
    id3Source = new(std::nothrow) AudioFileSourceID3(audioFile);
    if (!id3Source) {
        updateAudioError("Failed to create ID3 source");
        audioFile->close();
        return;
    }
    
    id3Source->RegisterMetadataCB(metadataCallback, this);
    
    if (mp3Generator && mp3Generator->begin(id3Source, audioOutput)) {
        updateAudioStatus(true, false, filePath);
    } else {
        updateAudioError("Failed to start playback");
        if (id3Source) {
            delete id3Source;
            id3Source = nullptr;
        }
        audioFile->close();
    }
}

inline void MusicApp::pauseAudioPlayback() {
    if (mp3Generator && mp3Generator->isRunning()) {
        mp3Generator->stop();
        updateAudioStatus(false, true, "");
    }
}

inline void MusicApp::resumeAudioPlayback() {
    int fileIndex = -1;
    
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        fileIndex = audioStatus.currentFileIndex;
        xSemaphoreGive(audioStatusMutex);
    }
    
    if (fileIndex >= 0) {
        updateAudioStatus(true, false, "");
    }
}

inline void MusicApp::stopAudioPlayback() {
    if (mp3Generator && mp3Generator->isRunning()) {
        mp3Generator->stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    if (audioOutput) {
        audioOutput->flush();
        audioOutput->stop();
    }
    
    if (id3Source) {
        delete id3Source;
        id3Source = nullptr;
    }
    
    if (audioFile && audioFile->isOpen()) {
        audioFile->close();
    }
    
    updateAudioStatus(false, false, "");
}

inline void MusicApp::playNextAudio() {
    updateAudioError("Use main thread for next/prev");
}

inline void MusicApp::playPreviousAudio() {
    updateAudioError("Use main thread for next/prev");
}

inline void MusicApp::setAudioVolume(int volume) {
    M5Cardputer.Speaker.setVolume((volume * 255) / 100);
    
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        audioStatus.currentVolume = volume;
        xSemaphoreGive(audioStatusMutex);
    }
}

inline void MusicApp::updateAudioStatus(bool playing, bool paused, const char* songPath) {
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        audioStatus.isPlaying = playing;
        audioStatus.isPaused = paused;
        audioStatus.hasError = false;
        audioStatus.errorMessage[0] = '\0';
        
        if (songPath && strlen(songPath) > 0) {
            const char* fileName = strrchr(songPath, '/');
            if (fileName) {
                fileName++;
            } else {
                fileName = songPath;
            }
            
            strncpy(audioStatus.currentSongName, fileName, sizeof(audioStatus.currentSongName) - 1);
            audioStatus.currentSongName[sizeof(audioStatus.currentSongName) - 1] = '\0';
            
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

inline void MusicApp::updateAudioError(const char* errorMsg) {
    if (xSemaphoreTake(audioStatusMutex, portMAX_DELAY) == pdTRUE) {
        audioStatus.hasError = true;
        strncpy(audioStatus.errorMessage, errorMsg, sizeof(audioStatus.errorMessage) - 1);
        audioStatus.errorMessage[sizeof(audioStatus.errorMessage) - 1] = '\0';
        xSemaphoreGive(audioStatusMutex);
    }
}

inline void MusicApp::cleanupAudioTask() {
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

inline void MusicApp::metadataCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
}

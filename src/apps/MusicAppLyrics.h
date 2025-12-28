#pragma once
#include "apps/MusicApp.h"

inline void MusicApp::prepareLyricsForCurrentSong() {
    clearLyrics();
    if (musicFileCount > 0 && currentFileIndex >= 0 && currentFileIndex < musicFileCount) {
        String mp3Path = musicFiles[currentFileIndex].path;
        String lrcPath = computeLrcPath(mp3Path);
        SDFileManager* fm = appManager->getSDFileManager();
        if (fm && fm->exists(lrcPath)) {
            loadLyricsForFile(mp3Path);
        } else {
            lyricsAvailable = false;
            lyricsCurrentLabel->setText("Can't locate lyrics file");
            lyricsNextLabel->setText("");
            uiManager->refreshAppArea();
        }
        lastLyricsFileIndex = currentFileIndex;
    }
    playbackStartMillis = millis();
    pausedAccumulatedMillis = 0;
    lastIsPlayingFlag = true;
    lastIsPausedFlag = false;
    currentLyricIndex = -1;
    lastDisplayedCurrent = "";
    lastDisplayedNext = "";
}

inline void MusicApp::clearLyrics() {
    lyricLines.clear();
    lyricsAvailable = false;
    currentLyricIndex = -1;
    lastDisplayedCurrent = "";
    lastDisplayedNext = "";
}

inline String MusicApp::computeLrcPath(const String& mp3Path) {
    int dot = mp3Path.lastIndexOf('.');
    if (dot >= 0) return mp3Path.substring(0, dot) + ".lrc";
    return mp3Path + ".lrc";
}

inline void MusicApp::loadLyricsForFile(const String& mp3Path) {
    String lrcPath = computeLrcPath(mp3Path);
    SDFileManager* fm = appManager->getSDFileManager();
    String content = fm ? fm->readFile(lrcPath) : "";
    lyricLines.clear();
    if (content.length() == 0) {
        lyricsAvailable = false;
        return;
    }
    int pos = 0;
    while (pos < content.length()) {
        int next = content.indexOf('\n', pos);
        if (next == -1) next = content.length();
        String line = content.substring(pos, next);
        pos = next + 1;
        line.replace("\r", "");
        if (line.length() == 0) continue;
        String rem = line;
        std::vector<uint32_t> times;
        bool parsedAny = false;
        while (rem.length() > 0 && rem.charAt(0) == '[') {
            int close = rem.indexOf(']');
            if (close <= 0) break;
            String tag = rem.substring(1, close);
            int colon = tag.indexOf(':');
            if (colon < 0) break;
            String mmStr = tag.substring(0, colon);
            String secStr = tag.substring(colon + 1);
            if (mmStr.length() == 0 || secStr.length() == 0) break;
            char c0 = mmStr.charAt(0);
            char c1 = secStr.charAt(0);
            if (c0 < '0' || c0 > '9' || c1 < '0' || c1 > '9') break;
            int mm = 0;
            int ss = 0;
            int fracMs = 0;
            int dotPos = secStr.indexOf('.');
            if (dotPos >= 0) {
                String sPart = secStr.substring(0, dotPos);
                String fracPart = secStr.substring(dotPos + 1);
                ss = sPart.toInt();
                int frac = fracPart.toInt();
                if (fracPart.length() == 2) {
                    fracMs = frac * 10;
                } else if (fracPart.length() == 3) {
                    fracMs = frac;
                } else {
                    fracMs = 0;
                }
            } else {
                ss = secStr.toInt();
            }
            mm = mmStr.toInt();
            uint32_t ms = (uint32_t)mm * 60000u + (uint32_t)ss * 1000u + (uint32_t)fracMs;
            times.push_back(ms);
            parsedAny = true;
            rem = rem.substring(close + 1);
        }
        if (!parsedAny) continue;
        String text = rem;
        text.trim();
        if (text.length() == 0) continue;
        for (size_t i = 0; i < times.size(); i++) {
            LyricLine ll;
            ll.timeMs = times[i];
            ll.text = text;
            lyricLines.push_back(ll);
        }
    }
    if (!lyricLines.empty()) {
        std::sort(lyricLines.begin(), lyricLines.end(), [](const LyricLine& a, const LyricLine& b){ return a.timeMs < b.timeMs; });
        lyricsAvailable = true;
    } else {
        lyricsAvailable = false;
    }
}

inline void MusicApp::updateLyricsDisplay() {
    if (audioStatusMutex && xSemaphoreTake(audioStatusMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
        isPlaying = audioStatus.isPlaying;
        isPaused = audioStatus.isPaused;
        int idx = audioStatus.currentFileIndex;
        xSemaphoreGive(audioStatusMutex);
        if (isPlaying && idx >= 0 && idx != lastLyricsFileIndex) {
            currentFileIndex = idx;
            prepareLyricsForCurrentSong();
        }
    }
    if (isPlaying && !lastIsPlayingFlag) {
        playbackStartMillis = millis();
        pausedAccumulatedMillis = 0;
        lastIsPlayingFlag = true;
        lastIsPausedFlag = false;
    }
    if (isPaused && !lastIsPausedFlag) {
        pauseStartMillis = millis();
        lastIsPausedFlag = true;
    }
    if (!isPaused && lastIsPausedFlag) {
        pausedAccumulatedMillis += millis() - pauseStartMillis;
        lastIsPausedFlag = false;
    }
    if (!isPlaying) {
        lastIsPlayingFlag = false;
    }
    if (!isPlaying) return;
    if (!lyricsAvailable) {
        if (lastDisplayedCurrent != "Can't locate lyrics file" || lastDisplayedNext.length() > 0) {
            lyricsCurrentLabel->setText("Can't locate lyrics file");
            lyricsNextLabel->setText("");
            lastDisplayedCurrent = "Can't locate lyrics file";
            lastDisplayedNext = "";
            uiManager->refreshAppArea();
        }
        return;
    }
    uint32_t nowMs = millis();
    uint32_t elapsed = nowMs >= playbackStartMillis ? nowMs - playbackStartMillis : 0;
    if (elapsed >= pausedAccumulatedMillis) {
        elapsed -= pausedAccumulatedMillis;
    } else {
        elapsed = 0;
    }
    if (currentLyricIndex < 0) {
        int i = 0;
        while (i + 1 < (int)lyricLines.size() && lyricLines[i + 1].timeMs <= elapsed) i++;
        currentLyricIndex = i;
    } else {
        while (currentLyricIndex + 1 < (int)lyricLines.size() && lyricLines[currentLyricIndex + 1].timeMs <= elapsed) {
            currentLyricIndex++;
        }
    }
    String cur = currentLyricIndex >= 0 && currentLyricIndex < (int)lyricLines.size() ? lyricLines[currentLyricIndex].text : "";
    String nxt = currentLyricIndex + 1 < (int)lyricLines.size() ? lyricLines[currentLyricIndex + 1].text : "";
    if (cur != lastDisplayedCurrent || nxt != lastDisplayedNext) {
        lyricsCurrentLabel->setText(cur);
        lyricsNextLabel->setText(nxt);
        lastDisplayedCurrent = cur;
        lastDisplayedNext = nxt;
        uiManager->refreshAppArea();
    }
}


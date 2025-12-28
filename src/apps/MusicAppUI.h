#pragma once
#include "apps/MusicApp.h"

inline void MusicApp::scanMusicFiles() {
    if (!isInitialized) return;
    
    songLabel->setText("Scanning for music files...");
    drawInterface();
    
    clearMusicData();
    
    musicFileCount = 0;
    {
        SDFileManager* fm = appManager->getSDFileManager();
        if (fm) fm->scanAllFiles(musicFiles, musicFileCount, MAX_MUSIC_FILES, ".mp3");
    }
    
    if (musicFileCount > 0) {
        songLabel->setText("Categorizing music files...");
        drawInterface();
        
        categorizeMusic();
        
        songLabel->setText("Found " + String(musicFileCount) + " music files");
        currentFileIndex = 0;
        updateSongInfo();
    } else {
        songLabel->setText("No MP3 files found");
    }
}

inline void MusicApp::playCurrentSong() {
    if (!isInitialized || musicFileCount == 0 || currentFileIndex < 0 || currentFileIndex >= musicFileCount) {
        songLabel->setText("No song selected");
        return;
    }
    
    String filePath = musicFiles[currentFileIndex].path;
    sendAudioCommand(AUDIO_CMD_PLAY, 0, filePath.c_str());
    prepareLyricsForCurrentSong();
}

inline void MusicApp::playSelectedSong() {
    MenuItem* selectedItem = playList->getSelectedItem();
    if (!selectedItem) return;
    
    if (menuState.level == MENU_TRACKS) {
        std::vector<MusicTrack*> tracksToPlay;
        
        if (menuState.currentArtist.isEmpty() && menuState.currentAlbum.isEmpty()) {
            tracksToPlay = uncategorizedTracks;
        } else if (!menuState.currentArtist.isEmpty() && menuState.currentAlbum.isEmpty()) {
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

inline void MusicApp::adjustVolume(int delta) {
    currentVolume += delta;
    if (currentVolume < 0) currentVolume = 0;
    if (currentVolume > 100) currentVolume = 100;
    
    if (volumeSlider) {
        volumeSlider->setValue(currentVolume);
    }
    
    sendAudioCommand(AUDIO_CMD_VOLUME, currentVolume);
    
    uiManager->refreshAppArea();
}

inline void MusicApp::setVolume(int volume) {
    currentVolume = volume;
    if (currentVolume < 0) currentVolume = 0;
    if (currentVolume > 100) currentVolume = 100;
    
    if (volumeSlider) {
        volumeSlider->setValue(currentVolume);
    }
    
    sendAudioCommand(AUDIO_CMD_VOLUME, currentVolume);
}

inline void MusicApp::updateSongInfo() {
    if (musicFileCount > 0 && currentFileIndex >= 0 && currentFileIndex < musicFileCount) {
        String info = "(" + String(currentFileIndex + 1) + "/" + String(musicFileCount) + ") ";
        info += musicFiles[currentFileIndex].name;
        songLabel->setText(info);
    }
}

inline void MusicApp::cleanup() {
    isInitialized = false;
    isPlaying = false;
    isPaused = false;
    pausedPosition = 0;
}

inline void MusicApp::drawInterface() {
    uiManager->smartRefresh();
}

inline void MusicApp::categorizeMusic() {
    for (int i = 0; i < musicFileCount; i++) {
        MusicTrack* track = parseFileName(musicFiles[i].name, musicFiles[i].path);
        if (track) {
            allTracks.push_back(track);
            
            if (track->artist.isEmpty() || track->album.isEmpty()) {
                uncategorizedTracks.push_back(track);
            } else {
                Artist* artist = findOrCreateArtist(track->artist);
                
                Album* album = findOrCreateAlbum(artist, track->album);
                
                album->tracks.push_back(track);
            }
        }
    }
}

inline MusicTrack* MusicApp::parseFileName(const String& fileName, const String& filePath) {
    MusicTrack* track = new MusicTrack();
    track->fileName = fileName;
    track->filePath = filePath;
    
    String nameWithoutExt = fileName;
    if (nameWithoutExt.endsWith(".mp3")) {
        nameWithoutExt = nameWithoutExt.substring(0, nameWithoutExt.length() - 4);
    }
    
    int firstDash = nameWithoutExt.indexOf('-');
    if (firstDash > 0) {
        int secondDash = nameWithoutExt.indexOf('-', firstDash + 1);
        if (secondDash > firstDash + 1) {
            track->artist = nameWithoutExt.substring(0, firstDash);
            track->album = nameWithoutExt.substring(firstDash + 1, secondDash);
            track->title = nameWithoutExt.substring(secondDash + 1);
            
            track->artist.trim();
            track->album.trim();
            track->title.trim();
        } else {
            track->artist = nameWithoutExt.substring(0, firstDash);
            track->title = nameWithoutExt.substring(firstDash + 1);
            track->artist.trim();
            track->title.trim();
            track->album = "";
        }
    } else {
        track->title = nameWithoutExt;
        track->artist = "";
        track->album = "";
    }
    
    return track;
}

inline Artist* MusicApp::findOrCreateArtist(const String& artistName) {
    for (Artist* artist : artists) {
        if (artist->name.equalsIgnoreCase(artistName)) {
            return artist;
        }
    }
    
    Artist* newArtist = new Artist();
    newArtist->name = artistName;
    artists.push_back(newArtist);
    return newArtist;
}

inline Album* MusicApp::findOrCreateAlbum(Artist* artist, const String& albumName) {
    for (Album* album : artist->albums) {
        if (album->name.equalsIgnoreCase(albumName)) {
            return album;
        }
    }
    
    Album* newAlbum = new Album();
    newAlbum->name = albumName;
    newAlbum->artist = artist->name;
    artist->albums.push_back(newAlbum);
    allAlbums.push_back(newAlbum);
    return newAlbum;
}

inline void MusicApp::clearMusicData() {
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

inline void MusicApp::buildMainMenu() {
    playList->clear();
    menuState.level = MENU_MAIN;
    menuState.currentArtist = "";
    menuState.currentAlbum = "";
    
    playList->addItem("Albums (" + String(allAlbums.size()) + ")", 0, "");
    playList->addItem("Artists (" + String(artists.size()) + ")", 1, "");
    playList->addItem("Uncategorized (" + String(uncategorizedTracks.size()) + ")", 2, "");
    
    titleLabel->setText("Music Library");
    songLabel->setText("Select a category");
}

inline void MusicApp::buildArtistsMenu() {
    playList->clear();
    menuState.level = MENU_ARTISTS;
    
    playList->addItem("../", -1, "");
    
    for (size_t i = 0; i < artists.size(); i++) {
        Artist* artist = artists[i];
        int totalTracks = 0;
        for (Album* album : artist->albums) {
            totalTracks += album->tracks.size();
        }
        totalTracks += artist->singleTracks.size();
        
        playList->addItem(artist->name + " (" + String(totalTracks) + ")", i, "");
    }
    
    titleLabel->setText("Artists");
    songLabel->setText("Select an artist");
}

inline void MusicApp::buildAlbumsMenu(const String& artistName) {
    playList->clear();
    menuState.level = MENU_ALBUMS;
    menuState.currentArtist = artistName;
    
    playList->addItem("../", -1, "");
    
    if (artistName.isEmpty()) {
        for (size_t i = 0; i < allAlbums.size(); i++) {
            Album* album = allAlbums[i];
            playList->addItem("" + album->name + " - " + album->artist + " (" + String(album->tracks.size()) + ")", i, "");
        }
        titleLabel->setText("All Albums");
    } else {
        Artist* artist = findOrCreateArtist(artistName);
        int index = 0;
        for (Album* album : artist->albums) {
            playList->addItem("" + album->name + " (" + String(album->tracks.size()) + ")", index++, "");
        }
        titleLabel->setText(artistName + " - Albums");
    }
    
    songLabel->setText("Select an album");
}

inline void MusicApp::buildTracksMenu(const String& artistName, const String& albumName) {
    playList->clear();
    menuState.level = MENU_TRACKS;
    menuState.currentArtist = artistName;
    menuState.currentAlbum = albumName;
    
    playList->addItem("../", -1, "");
    
    std::vector<MusicTrack*> tracksToShow;
    
    if (artistName.isEmpty() && albumName.isEmpty()) {
        tracksToShow = uncategorizedTracks;
        titleLabel->setText("Uncategorized");
    } else if (!artistName.isEmpty() && albumName.isEmpty()) {
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
        Artist* artist = findOrCreateArtist(artistName);
        for (Album* album : artist->albums) {
            if (album->name.equalsIgnoreCase(albumName)) {
                tracksToShow = album->tracks;
                break;
            }
        }
        titleLabel->setText(albumName);
    }
    
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

inline void MusicApp::navigateBack() {
    switch (menuState.level) {
        case MENU_MAIN:
            break;
            
        case MENU_ARTISTS:
        case MENU_ALBUMS:
            buildMainMenu();
            break;
            
        case MENU_TRACKS:
            if (!menuState.currentArtist.isEmpty() && !menuState.currentAlbum.isEmpty()) {
                buildAlbumsMenu(menuState.currentArtist);
            } else if (!menuState.currentArtist.isEmpty()) {
                buildArtistsMenu();
            } else {
                buildMainMenu();
            }
            break;
    }
    
    updateMenuDisplay();
}

inline void MusicApp::navigateForward() {
    MenuItem* selectedItem = playList->getSelectedItem();
    if (!selectedItem) return;
    
    switch (menuState.level) {
        case MENU_MAIN:
            switch (selectedItem->id) {
                case 0:
                    buildAlbumsMenu();
                    break;
                case 1:
                    buildArtistsMenu();
                    break;
                case 2:
                    buildTracksMenu();
                    break;
            }
            break;
            
        case MENU_ARTISTS:
            if (selectedItem->id >= 0 && selectedItem->id < (int)artists.size()) {
                Artist* artist = artists[selectedItem->id];
                if (artist->albums.size() > 1) {
                    buildAlbumsMenu(artist->name);
                } else {
                    buildTracksMenu(artist->name);
                }
            }
            break;
            
        case MENU_ALBUMS:
            if (!menuState.currentArtist.isEmpty()) {
                Artist* artist = findOrCreateArtist(menuState.currentArtist);
                if (selectedItem->id >= 0 && selectedItem->id < (int)artist->albums.size()) {
                    Album* album = artist->albums[selectedItem->id];
                    buildTracksMenu(artist->name, album->name);
                }
            } else {
                if (selectedItem->id >= 0 && selectedItem->id < (int)allAlbums.size()) {
                    Album* album = allAlbums[selectedItem->id];
                    buildTracksMenu(album->artist, album->name);
                }
            }
            break;
            
        case MENU_TRACKS:
            playSelectedSong();
            break;
    }
    
    updateMenuDisplay();
}

inline void MusicApp::updateMenuDisplay() {
    uiManager->refreshAppArea();
}

inline void MusicApp::handleMenuSelection(MenuItem* item) {
    if (!item) return;
    
    if (item->id == -1) {
        navigateBack();
        return;
    }
    
    switch (menuState.level) {
        case MENU_MAIN:
            switch (item->id) {
                case 0:
                    buildAlbumsMenu();
                    break;
                case 1:
                    buildArtistsMenu();
                    break;
                case 2:
                    buildTracksMenu();
                    break;
            }
            break;
            
        case MENU_ARTISTS:
            if (item->id >= 0 && item->id < (int)artists.size()) {
                Artist* artist = artists[item->id];
                if (artist->albums.size() > 1) {
                    buildAlbumsMenu(artist->name);
                } else {
                    buildTracksMenu(artist->name);
                }
            }
            break;
            
        case MENU_ALBUMS:
            if (!menuState.currentArtist.isEmpty()) {
                Artist* artist = findOrCreateArtist(menuState.currentArtist);
                if (item->id >= 0 && item->id < (int)artist->albums.size()) {
                    Album* album = artist->albums[item->id];
                    buildTracksMenu(artist->name, album->name);
                }
            } else {
                if (item->id >= 0 && item->id < (int)allAlbums.size()) {
                    Album* album = allAlbums[item->id];
                    buildTracksMenu(album->artist, album->name);
                }
            }
            break;
            
        case MENU_TRACKS:
            playSelectedSong();
            break;
    }
    
    updateMenuDisplay();
}


#pragma once

#include "ITool.h"
#include <M5Cardputer.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <vector>
#include <AudioOutput.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include "system/AudioOutputM5Speaker.h"

// SD Card Pins
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

class BusSimulatorTool : public ToolBase {
private:
    struct Station {
        String id;
        String name;
        String audio;
    };

    struct RouteInfo {
        String id;
        String name;
    };

    enum State {
        SELECTING_ROUTE,
        STOPPED,
        RUNNING
    };
    
    // Audio Components
    AudioGeneratorMP3 *mp3;
    AudioFileSourceSD *audioFile;
    AudioOutputM5Speaker *audioOut;
    std::vector<String> audioPlaylist;
    std::vector<String> audioTemplate;
    std::vector<String> arrivalAudioTemplate;
    String currentLineAudio;
    int volumePercent;

    UIMenuList* routeMenu;
    UILabel* statusLabel;
    UILabel* currentStationLabel;
    UILabel* nextStationLabel;
    UILabel* guideLabel;
    
    std::vector<RouteInfo> routes;
    std::vector<Station> stations;
    int currentStationIndex;
    State state;
    
    // IDs for widgets
    static const int WINDOW_ID = 200;
    static const int MENU_LIST_ID = 201;
    static const int STATUS_LABEL_ID = 202;
    static const int CURR_STATION_LABEL_ID = 203;
    static const int NEXT_STATION_LABEL_ID = 204;
    static const int GUIDE_LABEL_ID = 205;

    class RouteMenuCallback : public UIMenuList {
        BusSimulatorTool* tool;
    public:
        RouteMenuCallback(int id, int x, int y, int w, int h, const char* name, BusSimulatorTool* t)
            : UIMenuList(id, x, y, w, h, name), tool(t) {}
        void onItemSelected(MenuItem* item) override {
            tool->onRouteSelected(item);
        }
    };

public:
    BusSimulatorTool() 
        : routeMenu(nullptr), statusLabel(nullptr),
          currentStationLabel(nullptr), nextStationLabel(nullptr), guideLabel(nullptr),
          currentStationIndex(0), state(SELECTING_ROUTE),
          mp3(nullptr), audioFile(nullptr), audioOut(nullptr), volumePercent(10) {}
          
    ~BusSimulatorTool() {
        cleanupAudio();
    }

    const char* getMenuText() const override { return "Bus Simulator"; }
    int getMenuItemId() const override { return 103; } // Unique ID

    const char* getEnterTipLine1() const override { return "Loading route data"; }
    const char* getEnterTipLine2() const override { return "Please wait..."; }

    void onExit(const ToolServices& services) override {
        (void)services;
        stopAudio();
    }

private:
    int getWindowWidgetId() const override { return WINDOW_ID; }

    void resetPointers() override {
        window = nullptr;
        routeMenu = nullptr;
        statusLabel = nullptr;
        currentStationLabel = nullptr;
        nextStationLabel = nullptr;
        guideLabel = nullptr;
    }

    void bindWidgets(UIManager* manager) override {
        UIWidget* existingWindow = manager->getWidget(WINDOW_ID);
        window = existingWindow ? static_cast<UIWindow*>(existingWindow) : nullptr;
        UIWidget* m = manager->getWidget(MENU_LIST_ID);
        routeMenu = m ? static_cast<UIMenuList*>(m) : nullptr;
        UIWidget* l1 = manager->getWidget(STATUS_LABEL_ID);
        UIWidget* l2 = manager->getWidget(CURR_STATION_LABEL_ID);
        UIWidget* l3 = manager->getWidget(NEXT_STATION_LABEL_ID);
        UIWidget* l4 = manager->getWidget(GUIDE_LABEL_ID);
        statusLabel = l1 ? static_cast<UILabel*>(l1) : nullptr;
        currentStationLabel = l2 ? static_cast<UILabel*>(l2) : nullptr;
        nextStationLabel = l3 ? static_cast<UILabel*>(l3) : nullptr;
        guideLabel = l4 ? static_cast<UILabel*>(l4) : nullptr;
    }

    void createWidgets(UIManager* manager) override {
        uiManager = manager;
        setupAudio();
        window = manager->createWindow(WINDOW_ID, 30, 20, 180, 100, "Bus Simulator", "BusSimWindow");
        routeMenu = new RouteMenuCallback(MENU_LIST_ID, 15, 35, 150, 50, "RouteList", this);
        routeMenu->setParent(window);
        manager->addWidget(routeMenu);
        statusLabel = manager->createLabel(STATUS_LABEL_ID, 15, 30, "Stopped", "BusStatus", window);
        currentStationLabel = manager->createLabel(CURR_STATION_LABEL_ID, 15, 50, "Curr: --", "BusCurrStation", window);
        nextStationLabel = manager->createLabel(NEXT_STATION_LABEL_ID, 15, 65, "Next: --", "BusNextStation", window);
        guideLabel = manager->createLabel(GUIDE_LABEL_ID, 15, 85, "Select Route", "BusGuide", window);
        loadRoutes();
        updateUIState();
    }

public:

    void setupAudio() {
        if (!audioOut) {
            audioOut = new AudioOutputM5Speaker(&M5Cardputer.Speaker);
        }
        if (!mp3) {
            mp3 = new AudioGeneratorMP3();
        }
        int spkVol = volumePercent;
        if (spkVol < 0) spkVol = 0;
        if (spkVol > 100) spkVol = 100;
        M5Cardputer.Speaker.setVolume((spkVol * 255) / 100);
    }

    void cleanupAudio() {
        stopAudio();
        if (mp3) { delete mp3; mp3 = nullptr; }
        if (audioOut) { delete audioOut; audioOut = nullptr; }
    }

    void stopAudio() {
        if (mp3 && mp3->isRunning()) {
            mp3->stop();
        }
        if (audioFile) {
            audioFile->close();
            delete audioFile;
            audioFile = nullptr;
        }
        audioPlaylist.clear();
    }

    void playAudio(String path) {
        if (mp3 && mp3->isRunning()) {
             mp3->stop();
        }
        if (audioFile) {
             delete audioFile;
             audioFile = nullptr;
        }
        
        String fullPath = "/bus_routes/audios/" + path;
        audioFile = new AudioFileSourceSD(fullPath.c_str());
        if (mp3 && audioOut) {
            mp3->begin(audioFile, audioOut);
        }
    }
    
    void playNextInQueue() {
        if (audioPlaylist.empty()) return;
        String next = audioPlaylist.front();
        audioPlaylist.erase(audioPlaylist.begin());
        playAudio(next);
    }

    void loadRoutes() {
        if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
            // Try again
        }
        
        File file = SD.open("/bus_routes/routes.json");
        if (!file) {
            guideLabel->setText("Err: routes.json");
            return;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            guideLabel->setText("Err: Json Parse");
            return;
        }
        
        // Parse audio template
        audioTemplate.clear();
        JsonArray templateArray = doc["audio_template"];
        for (JsonVariant v : templateArray) {
            audioTemplate.push_back(v.as<String>());
        }

        arrivalAudioTemplate.clear();
        JsonArray arrivalTemplateArray = doc["arrival_audio_template"];
        for (JsonVariant v : arrivalTemplateArray) {
            arrivalAudioTemplate.push_back(v.as<String>());
        }

        JsonArray routesArray = doc["routes"];
        routes.clear();
        routeMenu->clear();
        
        for (JsonObject r : routesArray) {
            RouteInfo info;
            info.id = r["id"].as<String>();
            info.name = r["name"].as<String>();
            routes.push_back(info);
            routeMenu->addItem(info.name.c_str(), routes.size() - 1); 
        }
        
        if (routes.empty()) {
             guideLabel->setText("No Routes Found");
        } else {
             guideLabel->setText("Select Route (Enter)");
        }
    }

    void onRouteSelected(MenuItem* item) {
        int index = item->id;
        if (index >= 0 && index < routes.size()) {
            loadRouteDetails(routes[index].id);
        }
    }

    void loadRouteDetails(String routeId) {
        String path = "/bus_routes/" + routeId + ".json";
        File file = SD.open(path);
        if (!file) {
            guideLabel->setText(("Err: " + routeId).c_str());
            return;
        }

        JsonDocument doc; // Larger buffer for stations
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            guideLabel->setText("Err: Line Json");
            return;
        }

        currentLineAudio = doc["line_audio"].as<String>();

        stations.clear();
        JsonArray stArray = doc["stations"];
        for (JsonObject s : stArray) {
            Station st;
            st.id = s["id"].as<String>();
            st.name = s["name"].as<String>();
            st.audio = s["audio"].as<String>();
            stations.push_back(st);
        }

        if (stations.empty()) {
            guideLabel->setText("No Stations");
            return;
        }

        state = STOPPED;
        currentStationIndex = 0;
        updateUIState();
    }

    void updateUIState() {
        if (state == SELECTING_ROUTE) {
            if (routeMenu) {
                routeMenu->setParent(window);
                routeMenu->setVisible(true);
            }
            
            // Detach station labels to prevent them from being forced visible by UIManager
            if (statusLabel) {
                statusLabel->setVisible(false);
                statusLabel->setParent(nullptr);
            }
            if (currentStationLabel) {
                currentStationLabel->setVisible(false);
                currentStationLabel->setParent(nullptr);
            }
            if (nextStationLabel) {
                nextStationLabel->setVisible(false);
                nextStationLabel->setParent(nullptr);
            }
            
            if (guideLabel) guideLabel->setText("Select Route (Enter)");
            // if (uiManager) uiManager->setFocus(routeMenu);
        } else {
            if (routeMenu) {
                routeMenu->setVisible(false);
                routeMenu->setParent(nullptr);
            }
            
            if (window) {
                window->invalidate();
            }

            if (statusLabel) {
                statusLabel->setParent(window);
                statusLabel->setVisible(true);
            }
            if (currentStationLabel) {
                currentStationLabel->setParent(window);
                currentStationLabel->setVisible(true);
            }
            if (nextStationLabel) {
                nextStationLabel->setParent(window);
                nextStationLabel->setVisible(true);
            }
            
            updateStationLabels();
            
            String statusText;
            if (state == STOPPED) {
                 statusText = "Status: Stopped";
                 statusLabel->setTextColor(TFT_YELLOW);
                 if (guideLabel) guideLabel->setText("Press G to Start");
            } else {
                 statusText = "Status: Running";
                 statusLabel->setTextColor(TFT_GREEN);
                 if (guideLabel) guideLabel->setText("Press G to Arrive");
            }
            statusText += " Vol:";
            statusText += String(volumePercent);
            statusText += "%";
            statusLabel->setText(statusText.c_str());
        }
    }

    void updateStationLabels() {
        if (stations.empty()) return;
        
        if (currentStationIndex < stations.size()) {
            String curr = "Curr: " + stations[currentStationIndex].name;
            currentStationLabel->setText(curr.c_str());
        } else {
             currentStationLabel->setText("Curr: Terminal");
        }
        
        String next = "Next: --";
        if (currentStationIndex + 1 < stations.size()) {
            next = "Next: " + stations[currentStationIndex + 1].name;
        } else {
            next = "Next: Terminal";
        }
        nextStationLabel->setText(next.c_str());
    }

    void playAnnouncement(bool departure) {
        stopAudio();
        
        if (departure) {
            int nextIndex = currentStationIndex + 1;
            if (nextIndex >= stations.size()) return;
            
            for (const String& item : audioTemplate) {
                if (item == "$线路") {
                    if (currentLineAudio.length() > 0) audioPlaylist.push_back(currentLineAudio);
                } else if (item == "$本线路终点站") {
                    if (!stations.empty()) audioPlaylist.push_back(stations.back().audio);
                } else if (item == "$下一站") {
                    if (nextIndex < stations.size()) audioPlaylist.push_back(stations[nextIndex].audio);
                } else {
                    audioPlaylist.push_back(item);
                }
            }
            playNextInQueue();
        } else {
            if (currentStationIndex >= stations.size()) return;

            if (arrivalAudioTemplate.empty()) {
                playAudio(stations[currentStationIndex].audio);
                return;
            }

            for (const String& item : arrivalAudioTemplate) {
                if (item == "$站点") {
                    audioPlaylist.push_back(stations[currentStationIndex].audio);
                } else if (item == "$线路") {
                    if (currentLineAudio.length() > 0) audioPlaylist.push_back(currentLineAudio);
                } else if (item == "$本线路终点站") {
                    if (!stations.empty()) audioPlaylist.push_back(stations.back().audio);
                } else if (item == "$下一站") {
                    int nextIndex = currentStationIndex + 1;
                    if (nextIndex < stations.size()) audioPlaylist.push_back(stations[nextIndex].audio);
                } else {
                    audioPlaylist.push_back(item);
                }
            }
            playNextInQueue();
        }
    }

    void handleKeyEvent(const KeyEvent& event, const ToolServices& services) override {
        (void)services;
        if (state == SELECTING_ROUTE) {
             if (routeMenu) {
                 routeMenu->handleSecondaryKeyEvent(event);
             }
        } else {
            if (event.text.indexOf('g') != -1 || event.text.indexOf('G') != -1) {
                if (state == STOPPED) {
                    state = RUNNING;
                    playAnnouncement(true);
                } else if (state == RUNNING) {
                    state = STOPPED;
                    if (currentStationIndex + 1 < stations.size()) {
                        currentStationIndex++;
                    }
                    playAnnouncement(false);
                }
                updateUIState();
            } else if (event.text.indexOf('-') != -1) {
                volumePercent -= 10;
                if (volumePercent < 0) volumePercent = 0;
                int spkVol = volumePercent;
                if (spkVol > 100) spkVol = 100;
                M5Cardputer.Speaker.setVolume((spkVol * 255) / 100);
                updateUIState();
            } else if (event.text.indexOf('=') != -1) {
                volumePercent += 10;
                if (volumePercent > 100) volumePercent = 100;
                int spkVol = volumePercent;
                if (spkVol < 0) spkVol = 0;
                M5Cardputer.Speaker.setVolume((spkVol * 255) / 100);
                updateUIState();
            } else if (event.text.indexOf('t') != -1 || event.text.indexOf('T') != -1) {
                stopAudio();
                state = STOPPED;
                currentStationIndex = 0;
                updateUIState();
            }
            if (event.esc) {
                stopAudio();
            }
        }
    }
    
    void update(const ToolServices& services) override {
        (void)services;
        if (mp3 && mp3->isRunning()) {
            if (!mp3->loop()) {
                mp3->stop();
                if (audioFile) { delete audioFile; audioFile = nullptr; }
                playNextInQueue();
            }
        } else if (!audioPlaylist.empty()) {
            playNextInQueue();
        }

        if (routeMenu && routeMenu->isVisible()) {
            routeMenu->update(millis());
        }
    }
};

#pragma once
#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>

// SD卡SPI引脚定义
#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

// 文件信息结构体
struct FileInfo {
    String name;        // 文件名
    String path;        // 完整路径
    bool isDirectory;   // 是否为目录
    size_t size;        // 文件大小（目录为0）
    
    FileInfo() : name(""), path(""), isDirectory(false), size(0) {}
    FileInfo(const String& n, const String& p, bool isDir, size_t s = 0) 
        : name(n), path(p), isDirectory(isDir), size(s) {}
};

// SD卡文件管理器类
class SDFileManager {
private:
    bool initialized;
    String currentPath;
    
public:
    SDFileManager() : initialized(false), currentPath("/") {}
    
    // 初始化SD卡
    bool initialize() {
        if (initialized) return true;
        
        // 初始化SPI
        SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
        
        // 初始化SD卡
        if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
            Serial.println("SD卡初始化失败");
            return false;
        }
        
        initialized = true;
        Serial.println("SD卡初始化成功");
        return true;
    }
    
    // 检查是否已初始化
    bool isInitialized() const {
        return initialized;
    }
    
    // 获取当前路径
    String getCurrentPath() const {
        return currentPath;
    }
    
    // 设置当前路径
    bool setCurrentPath(const String& path) {
        if (!initialized) return false;
        
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) {
            dir.close();
            return false;
        }
        dir.close();
        
        currentPath = path;
        // 确保路径以/结尾
        if (!currentPath.endsWith("/")) {
            currentPath += "/";
        }
        return true;
    }
    
    // 列出指定目录下的文件和文件夹
    bool listDirectory(const String& dirPath, FileInfo* fileList, int& fileCount, int maxFiles) {
        if (!initialized) return false;
        
        File dir = SD.open(dirPath);
        if (!dir || !dir.isDirectory()) {
            dir.close();
            return false;
        }
        
        fileCount = 0;
        
        // 如果不是根目录，添加".."返回上级目录项
        if (dirPath != "/") {
            if (fileCount < maxFiles) {
                fileList[fileCount] = FileInfo("..", "", true, 0);
                fileCount++;
            }
        }
        
        File file = dir.openNextFile();
        while (file && fileCount < maxFiles) {
            String fileName = file.name();
            String filePath = file.path();
            bool isDir = file.isDirectory();
            size_t fileSize = isDir ? 0 : file.size();
            
            fileList[fileCount] = FileInfo(fileName, filePath, isDir, fileSize);
            fileCount++;
            
            file.close();
            file = dir.openNextFile();
        }
        
        dir.close();
        return true;
    }
    
    // 列出当前目录下的文件和文件夹
    bool listCurrentDirectory(FileInfo* fileList, int& fileCount, int maxFiles) {
        return listDirectory(currentPath, fileList, fileCount, maxFiles);
    }
    
    // 读取文件内容
    String readFile(const String& filePath) {
        if (!initialized) return "";
        
        File file = SD.open(filePath);
        if (!file || file.isDirectory()) {
            file.close();
            return "";
        }
        
        String content = "";
        while (file.available()) {
            content += (char)file.read();
        }
        
        file.close();
        return content;
    }
    
    // 检查文件是否存在
    bool fileExists(const String& filePath) {
        if (!initialized) return false;
        
        File file = SD.open(filePath);
        bool exists = file && !file.isDirectory();
        file.close();
        return exists;
    }
    
    // 检查目录是否存在
    bool directoryExists(const String& dirPath) {
        if (!initialized) return false;
        
        File dir = SD.open(dirPath);
        bool exists = dir && dir.isDirectory();
        dir.close();
        return exists;
    }
    
    // 递归扫描所有文件
    bool scanAllFiles(FileInfo* fileList, int& fileCount, int maxFiles, bool mp3Only = false) {
        if (!initialized) return false;
        
        fileCount = 0;
        return scanDirectoryRecursive("/", fileList, fileCount, maxFiles, mp3Only);
    }
    
    // 递归扫描指定目录
    bool scanDirectoryRecursive(const String& dirPath, FileInfo* fileList, int& fileCount, int maxFiles, bool mp3Only = false) {
        if (!initialized || fileCount >= maxFiles) return false;
        
        File dir = SD.open(dirPath);
        if (!dir || !dir.isDirectory()) {
            dir.close();
            return false;
        }
        
        File file = dir.openNextFile();
        while (file && fileCount < maxFiles) {
            String fileName = file.name();
            String filePath = file.path();
            bool isDir = file.isDirectory();
            
            if (isDir) {
                // 递归扫描子目录
                scanDirectoryRecursive(filePath, fileList, fileCount, maxFiles, mp3Only);
            } else {
                // 检查是否为MP3文件（如果需要过滤）
                bool shouldAdd = true;
                if (mp3Only) {
                    String lowerName = fileName;
                    lowerName.toLowerCase();
                    shouldAdd = lowerName.endsWith(".mp3");
                }
                
                if (shouldAdd) {
                    size_t fileSize = file.size();
                    fileList[fileCount] = FileInfo(fileName, filePath, false, fileSize);
                    fileCount++;
                }
            }
            
            file.close();
            file = dir.openNextFile();
        }
        
        dir.close();
        return true;
    }
    
    // 获取文件扩展名
    String getFileExtension(const String& fileName) {
        int lastDot = fileName.lastIndexOf('.');
        if (lastDot == -1) return "";
        return fileName.substring(lastDot + 1);
    }
    
    // 检查是否为音频文件
    bool isAudioFile(const String& fileName) {
        String ext = getFileExtension(fileName);
        ext.toLowerCase();
        return ext == ".mp3" || ext == ".wav" || ext == ".m4a" || ext == ".aac";
    }
    
    // 进入目录
    bool enterDirectory(const String& dirName) {
        if (!initialized) return false;
        
        String newPath;
        if (dirName == "..") {
            // 返回上级目录
            if (currentPath == "/") return false; // 已经在根目录
            
            // 移除最后的/
            String temp = currentPath;
            if (temp.endsWith("/")) {
                temp = temp.substring(0, temp.length() - 1);
            }
            
            // 找到上一个/的位置
            int lastSlash = temp.lastIndexOf('/');
            if (lastSlash == -1) {
                newPath = "/";
            } else {
                newPath = temp.substring(0, lastSlash + 1);
            }
        } else {
            // 进入子目录
            newPath = currentPath + dirName + "/";
        }
        
        return setCurrentPath(newPath);
    }
    
    // 获取父目录路径
    String getParentPath(const String& path) {
        if (path == "/") return "/";
        
        String temp = path;
        if (temp.endsWith("/")) {
            temp = temp.substring(0, temp.length() - 1);
        }
        
        int lastSlash = temp.lastIndexOf('/');
        if (lastSlash == -1) {
            return "/";
        } else {
            return temp.substring(0, lastSlash + 1);
        }
    }
};
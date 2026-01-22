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
    
    // 规范化路径
    String normalizePath(const String& path) {
        if (path.isEmpty() || path == "/") return "/";
        
        String normalized = path;
        if (!normalized.startsWith("/")) normalized = "/" + normalized;
        if (normalized.endsWith("/") && normalized.length() > 1) {
            normalized = normalized.substring(0, normalized.length() - 1);
        }
        return normalized;
    }
    
public:
    SDFileManager() : initialized(false), currentPath("/") {}
    
    // 初始化SD卡
    bool initialize() {
        if (initialized) return true;
        
        // M5Cardputer SD pins
        // SCK: 40, MISO: 39, MOSI: 14, CS: 12
        // Use HSPI (SPI3) to avoid conflict with Display (usually FSPI/SPI2)
        // SPI2 is FSPI, SPI3 is HSPI. 
        // We create a new SPIClass instance for HSPI.
        
        // Use a pointer to avoid object destruction issues if declared on stack
        // But better to have it as member. For now, let's use member.
        // Wait, if I change it to member, I need to update class definition.
        // Let's use a static pointer or just leak it (it's a singleton manager anyway)
        static SPIClass* sdSPI = new SPIClass(HSPI);
        
        sdSPI->begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
        
        // Try to initialize SD card
        // Note: For ESP32-S3 and M5Cardputer, we should ensure SPI frequency is appropriate.
        if (!SD.begin(SD_SPI_CS_PIN, *sdSPI, 20000000)) { // Reduced to 20MHz for stability
            // If failed, try to disable LoRa module if present (pin 5) to avoid conflict?
            const int loraDisablePin = 5;
            pinMode(loraDisablePin, OUTPUT);
            digitalWrite(loraDisablePin, HIGH);
            
            // Retry with lower frequency
            if (!SD.begin(SD_SPI_CS_PIN, *sdSPI, 10000000)) {
                return false;
            }
        }
        
        initialized = true;
        currentPath = "/";
        return true;
    }
    
    // 检查是否已初始化
    bool isInitialized() const { return initialized; }
    
    // 获取当前路径
    String getCurrentPath() const { return currentPath; }
    
    // 设置当前路径
    bool setCurrentPath(const String& path) {
        if (!initialized) return false;
        
        String newPath = normalizePath(path);
        if (!exists(newPath) || !isDirectory(newPath)) return false;
        
        currentPath = newPath;
        return true;
    }
    
    // 检查文件/目录是否存在
    bool exists(const String& path) {
        if (!initialized) return false;
        File file = SD.open(path);
        bool result = (bool)file;
        file.close();
        return result;
    }
    
    // 检查是否为目录
    bool isDirectory(const String& path) {
        if (!initialized) return false;
        File file = SD.open(path);
        bool result = file && file.isDirectory();
        file.close();
        return result;
    }
    
    // 列出目录内容
    bool listDirectory(const String& dirPath, FileInfo* fileList, int& fileCount, int maxFiles) {
        if (!initialized) return false;
        
        fileCount = 0;
        String targetPath = dirPath.isEmpty() ? currentPath : normalizePath(dirPath);
        
        File dir = SD.open(targetPath);
        if (!dir || !dir.isDirectory()) {
            dir.close();
            return false;
        }
        
        // 添加返回上级目录项（非根目录）
        if (targetPath != "/" && fileCount < maxFiles) {
            fileList[fileCount++] = FileInfo("..", "", true, 0);
        }
        
        dir.rewindDirectory();
        File file = dir.openNextFile();
        
        while (file && fileCount < maxFiles) {
            String fileName = file.name();
            String displayName = fileName;
            
            // 提取文件名
            int lastSlash = displayName.lastIndexOf('/');
            if (lastSlash != -1) {
                displayName = displayName.substring(lastSlash + 1);
            }
            
            // 跳过隐藏文件和无效文件名
            if (displayName.length() == 0 || displayName.startsWith(".")) {
                file.close();
                file = dir.openNextFile();
                continue;
            }
            
            // 构建完整路径
            String fullPath = targetPath;
            if (!fullPath.endsWith("/")) fullPath += "/";
            fullPath += displayName;
            
            bool isDir = file.isDirectory();
            size_t fileSize = isDir ? 0 : file.size();
            
            fileList[fileCount++] = FileInfo(displayName, fullPath, isDir, fileSize);
            
            file.close();
            file = dir.openNextFile();
        }
        
        dir.close();
        return true;
    }
    
    // 列出当前目录内容
    bool listCurrentDirectory(FileInfo* fileList, int& fileCount, int maxFiles) {
        return listDirectory(currentPath, fileList, fileCount, maxFiles);
    }
    
    // 进入目录
    bool enterDirectory(const String& dirName) {
        if (!initialized) return false;
        
        String newPath;
        if (dirName == "..") {
            // 返回上级目录
            if (currentPath == "/") return false;
            
            int lastSlash = currentPath.lastIndexOf('/');
            if (lastSlash <= 0) {
                newPath = "/";
            } else {
                newPath = currentPath.substring(0, lastSlash);
            }
        } else {
            // 进入子目录
            newPath = currentPath;
            if (!newPath.endsWith("/")) newPath += "/";
            newPath += dirName;
        }
        
        return setCurrentPath(newPath);
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

    // 写入文件内容（覆盖）
    bool writeFile(const String& filePath, const String& content) {
        if (!initialized) return false;
        
        File file = SD.open(filePath, FILE_WRITE);
        if (!file) {
            return false;
        }
        
        file.print(content);
        file.close();
        return true;
    }
    
    // 扫描所有文件（递归）
    bool scanAllFiles(FileInfo* fileList, int& fileCount, int maxFiles, const String& extension = "") {
        if (!initialized) return false;
        fileCount = 0;
        return scanDirectoryRecursive("/", fileList, fileCount, maxFiles, extension);
    }
    
    // 递归扫描目录
    bool scanDirectoryRecursive(const String& dirPath, FileInfo* fileList, int& fileCount, int maxFiles, const String& extension = "") {
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
                scanDirectoryRecursive(filePath, fileList, fileCount, maxFiles, extension);
            } else {
                // 检查文件扩展名
                bool shouldAdd = extension.isEmpty();
                if (!extension.isEmpty()) {
                    String lowerName = fileName;
                    lowerName.toLowerCase();
                    String lowerExt = extension;
                    lowerExt.toLowerCase();
                    shouldAdd = lowerName.endsWith(lowerExt);
                }
                
                if (shouldAdd) {
                    size_t fileSize = file.size();
                    fileList[fileCount++] = FileInfo(fileName, filePath, false, fileSize);
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
        return fileName.substring(lastDot);
    }
    
    // 检查是否为音频文件
    bool isAudioFile(const String& fileName) {
        String ext = getFileExtension(fileName);
        ext.toLowerCase();
        return ext == ".mp3" || ext == ".wav" || ext == ".m4a" || ext == ".aac";
    }
};

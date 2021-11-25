#pragma once

#include "globals.h"
#include <thread>

struct FileRef {
  std::string path;
  std::string fileName;
  int sizeMB{0};
};

struct FileManagerParams {
  std::string recordDir;
  int recordDirSizeLimitMB{0};
  int recordDirSizeCheckIntervalSec{0};
  bool useLocalTime{false};
};

constexpr char FILENAME_DELIMITIER = '#';

class FileManager {
public:
  ~FileManager();
  
  static FileManager &instance();

  bool init(const FileManagerParams &params);

  void update(const uint64_t delta);

  FileManagerParams &params();

  std::string generateRecordFile(const std::string &capturerName,
                                 const std::string &fileExtension,
                                 uint32_t chunkLengthSec, time_t t = 0) const;

private:
  FileManager() = default;

  FileManager(const FileManager &) = delete;

  FileManager operator=(const FileManager &) = delete;

  FileManager operator=(const FileManager &&) = delete;

  FileManagerParams mParams;
  bool mExitFlag = false;
  std::thread mGarbageCollectorThread;
  time_t mLastCheckTime = 0;
};

#pragma once

#include "globals.h"
#include <thread>
#include <mutex>
#include <atomic>

struct FileRef
{
  std::string path;
  std::string fileName;
  int sizeMB = 0;
};

class FileManager {
public:
  ~FileManager();
  static FileManager &instance();

  bool init(const std::string &recordingDir = std::string(),
            int directorySizeLimitMB = 0);

  void update(const uint64_t delta);

  std::string getRecordingDir() const;

  int getDirectorySizeLimitMB() const;

  void setDirectorySizeLimitMB(int directorySizeLimitMB);

  std::string generateRecordFile(const std::string &capturerName, const std::string &fileExtension, time_t t = 0) const;

private:
  FileManager();
  FileManager(const FileManager &) = delete;
  FileManager operator=(const FileManager &) = delete;
  FileManager operator=(const FileManager &&) = delete;

  std::string mRecordingDir;
  std::atomic_int mDirectorySizeLimitMB = 0;
  std::thread mFileOrgThread;
  std::mutex mFileOrgMutex;
  std::atomic_bool mFileOrgFree = false;
};

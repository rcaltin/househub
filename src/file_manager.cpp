#include "file_manager.h"
#include <experimental/filesystem>
#include <fstream>
#include <list>
#include <sstream>

namespace fs = std::experimental::filesystem;

FileManager::FileManager() {}

FileManager::~FileManager() {
  if (mFileOrgThread.joinable()) {
    mFileOrgThread.join();
  }
}

FileManager &FileManager::instance() {
  static FileManager fm;
  return fm;
}

bool FileManager::init(const std::string &recordingDir,
                       int directorySizeLimitMB) {
  std::lock_guard<std::mutex> lock(mFileOrgMutex);

  mRecordingDir = recordingDir;
  setDirectorySizeLimitMB(directorySizeLimitMB);

  fs::create_directories(mRecordingDir);

  std::ofstream f(mRecordingDir + ".init");
  return f.is_open();
}

void FileManager::update(const uint64_t delta) {

  if (mDirectorySizeLimitMB && !mFileOrgThread.joinable()) {

    mFileOrgThread = std::thread([this]() {
      std::lock_guard<std::mutex> lock(mFileOrgMutex);

      mFileOrgFree = false;

      int totalSizeMB = 0;
      std::list<FileRef> mFiles;
      for (const auto &i : fs::recursive_directory_iterator(mRecordingDir)) {
        if (!fs::is_directory(i.path())) {
          const auto &p = i.path();

          FileRef ref;
          ref.path = p;
          ref.fileName = p.filename();
          ref.sizeMB = fs::file_size(p) / 1048576; // B to MB

          totalSizeMB += ref.sizeMB;

          mFiles.emplace_back(std::move(ref));
        }
      }

      auto FileRefComp = [](const FileRef &a, const FileRef &b) {
        // return a.time > b.time;
        return a.fileName > b.fileName;
      };
      mFiles.sort(FileRefComp);

      while (totalSizeMB >= mDirectorySizeLimitMB) {

        const FileRef &f = mFiles.back();

        if (fs::remove(f.path)) {
          totalSizeMB -= f.sizeMB;
        }

        mFiles.pop_back();
      }

      mFileOrgFree = true;
    });
  } else if (mFileOrgFree) {
    mFileOrgThread.join();
  }
}

std::string FileManager::getRecordingDir() const { return mRecordingDir; }

int FileManager::getDirectorySizeLimitMB() const {
  return mDirectorySizeLimitMB;
}

void FileManager::setDirectorySizeLimitMB(int directorySizeLimitMB) {
  mDirectorySizeLimitMB = directorySizeLimitMB;
}

std::string FileManager::generateRecordFile(const std::string &capturerName,
                                            const std::string &fileExtension,
                                            time_t t) const {
  if (!t) {
    t = std::time(nullptr);
  }

  std::stringstream path;
  path << mRecordingDir << capturerName << "/";

  fs::create_directories(path.str());
  path << timeString(t, false) << fileExtension;
  return path.str();
}
#include "file_manager.h"
#include <fstream>
#include <list>
#include <sstream>

// TODO check compiler c++ version here to determine include experimental or not
#include <experimental/filesystem>
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

bool FileManager::init(const FileManagerParams &params) {
  std::lock_guard<std::mutex> lock(mFileOrgMutex);
  mParams = params;

  std::ofstream f(mParams.recordDir + ".init");
  return fs::create_directories(mParams.recordDir) && f.is_open();
}

void FileManager::update(const uint64_t delta) {

  if (mParams.recordDirSizeLimitMB && !mFileOrgThread.joinable()) {

    mFileOrgThread = std::thread([this]() {
      std::lock_guard<std::mutex> lock(mFileOrgMutex);

      mFileOrgFree = false;

      int totalSizeMB = 0;
      std::list<FileRef> mFiles;
      for (const auto &i :
           fs::recursive_directory_iterator(mParams.recordDir)) {
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
        // TODO compare file creation time to sort instead file name
        return a.fileName > b.fileName;
      };
      mFiles.sort(FileRefComp);

      while (totalSizeMB >= mParams.recordDirSizeLimitMB) {

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

std::string FileManager::generateRecordFile(const std::string &capturerName,
                                            const std::string &fileExtension,
                                            time_t t) const {
  if (!t) {
    t = std::time(nullptr);
  }

  std::stringstream path;
  path << mParams.recordDir << capturerName << "/";

  fs::create_directories(path.str());
  path << timeString(t, mParams.useLocalTime) << fileExtension;
  return path.str();
}
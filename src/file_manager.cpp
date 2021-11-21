#include "file_manager.h"
#include <chrono>
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

  // create dir if not exist
  fs::create_directories(mParams.recordDir);

  // trial file
  std::ofstream f(mParams.recordDir + ".init");
  return f.is_open();
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
        const auto &p = i.path();
        if (!fs::is_directory(p) && p.has_extension()) {
          FileRef ref;
          ref.path = p;
          ref.fileName = p.filename();
          ref.sizeMB = fs::file_size(p) / 1048576; // B to MB

          totalSizeMB += ref.sizeMB;

          mFiles.emplace_back(std::move(ref));
        }
      }

      auto FileRefComp = [](const FileRef &l, const FileRef &r) {
        const Strings &stringsL = split(l.fileName, FILENAME_DELIMITIER);
        const Strings &stringsR = split(r.fileName, FILENAME_DELIMITIER);

        const bool isVidL = stringsL.size() == 3;
        const bool isVidR = stringsR.size() == 3;
        if (isVidL && isVidR) {
          // both sides are video records
          return stringTime(stringsL.at(1)) > stringTime(stringsR.at(1));
        } else if (isVidL || isVidR) {
          // one side is video record
          return isVidR;
        } else {
          // both sides are not video record
          return l.fileName > r.fileName;
        }
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
                                            uint32_t chunkLengthSec,
                                            time_t t) const {
  auto tStart = t ? std::chrono::system_clock::from_time_t(t)
                  : std::chrono::system_clock::now();
  auto tEnd = tStart + std::chrono::seconds(chunkLengthSec);

  std::stringstream path;
  path << mParams.recordDir << "/" << capturerName << "/";

  fs::create_directories(path.str());

  path << capturerName << FILENAME_DELIMITIER
       << timeString(std::chrono::system_clock::to_time_t(tStart),
                     mParams.useLocalTime)
       << FILENAME_DELIMITIER
       << timeString(std::chrono::system_clock::to_time_t(tEnd),
                     mParams.useLocalTime)
       << fileExtension;

  return path.str();
}
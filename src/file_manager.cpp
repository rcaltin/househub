#include "file_manager.h"
#include "file_system.h"
#include <chrono>
#include <fstream>
#include <list>
#include <sstream>

FileManager::~FileManager() {
  mExitFlag = true;
  if (mGarbageCollectorThread.joinable()) {
    mGarbageCollectorThread.join();
  }
}

FileManager &FileManager::instance() {
  static FileManager fm;
  return fm;
}

bool FileManager::init(const FileManagerParams &params) {
  mParams = params;

  // create dir if not exist
  fs::create_directories(mParams.recordDir);

  // trial file
  std::ofstream f(mParams.recordDir + "_rw_test_file");
  if (!f.is_open()) {
    return false;
  }
  f.close();
  fs::remove(mParams.recordDir + "_rw_test_file");

  // run garbage collector thread
  if (mParams.recordDirSizeLimitMB) {
    mGarbageCollectorThread = std::thread([this]() {
      while (!mExitFlag) {

        const time_t t = std::time(nullptr);

        if (t - mLastCheckTime >= mParams.recordDirSizeCheckIntervalSec) {
          mLastCheckTime = t;

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

          while (!mFiles.empty() &&
                 totalSizeMB >= mParams.recordDirSizeLimitMB) {

            const FileRef &f = mFiles.back();

            if (fs::remove(f.path)) {
              totalSizeMB -= f.sizeMB;
              LOG(INFO) << "chunk removed due to file size limit: " << f.path;
            }

            mFiles.pop_back();
          }
        }
      }
    });
  }

  return true;
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
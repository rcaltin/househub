#include "app.h"
#include "INIReader.h"
#include "capturer_factory.h"
#include "file_manager.h"
#include "globals.h"
#include <csignal>
#include <iostream>

void signalHandler(int signum) {
  std::cout << "!! Signal " << signum << " received. Terminating... !!"
            << std::endl;

  App::ExitFlag = true;
}

bool App::ExitFlag = false;

App::App() {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGKILL, signalHandler);
}

App::~App() {}

int App::exec(int argc, char *argv[]) {

  std::cout << "househub preparing... " << std::endl;

#ifdef WIN32
  std::string iniFile = "C:/househub.ini";
#else
  std::string iniFile = "/etc/househub/househub.ini";
#endif
  if (argc == 2) {
    iniFile = argv[1];
  }

  // read ini
  INIReader ini(iniFile);
  if (ini.ParseError() != 0) {
    std::cout << "ini file could not load: " << iniFile << std::endl;
    return ExitCode::INI_LOAD_ERROR;
  }

  // init file-manager
  FileManagerParams fmp;
  fmp.recordDir = ini.Get("file_manager", "record_dir", "./rec/");
  fmp.recordDirSizeLimitMB =
      ini.GetInteger("file_manager", "record_dir_size_limit_mb", 8192);
  fmp.useLocalTime = ini.GetBoolean("file_manager", "use_localtime", false);
  if (!FileManager::instance().init(fmp)) {
    std::cout << "file directory r/w error: " << fmp.recordDir << std::endl;
    return ExitCode::RW_ERROR;
  }

  // pre-check capturer count
  const int capturerCount =
      ini.GetInteger("app_settings", "capturer_count", -1);
  if (capturerCount <= 0) {
    std::cout << "invalid or not defined capturer count." << std::endl;
    return ExitCode::BAD_CAPTURER_COUNT;
  }

  // create capturers
  for (int i = 1; i <= capturerCount; ++i) {
    const std::string capN = "capturer" + std::to_string(i);
    if (ini.Get(capN.c_str(), "name", "___N/A") != "___N/A") {
      CapturerParams cp;
      cp.name = ini.Get(capN.c_str(), "name", capN.c_str());
      cp.type = ini.Get(capN.c_str(), "type", "default");
      cp.streamUri = ini.Get(capN.c_str(), "stream_uri", "localhost/stream");
      cp.filterK = ini.GetInteger(capN.c_str(), "filter_k", 0);
      cp.videoOutStreamParams.name = cp.name;

      cp.videoOutStreamParams.fps =
          ini.GetInteger(capN.c_str(), "output_fps", 10);

      cp.videoOutStreamParams.outputSize =
          cv::Size(ini.GetInteger(capN.c_str(), "output_width", 1024),
                   ini.GetInteger(capN.c_str(), "output_height", 768));

      cp.videoOutStreamParams.chunkLengthSec =
          ini.GetInteger(capN.c_str(), "chunk_length_sec", 60);

      cp.videoOutStreamParams.fileExtension =
          ini.Get(capN.c_str(), "file_extension", ".avi");

      cp.videoOutStreamParams.watermark =
          ini.GetBoolean(capN.c_str(), "watermark", true);

      cp.videoOutStreamParams.useLocaltime =
          ini.GetBoolean(capN.c_str(), "use_localtime", true);

      const std::string fourcc = ini.Get(capN.c_str(), "fourcc", "mjpg");
      if (fourcc.length() != 4) {
        std::cout << "bad fourcc value." << std::endl;
        return ExitCode::BAD_FOURCC;
      }
      std::copy(fourcc.c_str(), fourcc.c_str() + 4,
                cp.videoOutStreamParams.fourcc);

      auto cap = CapturerFactory::createCapturer(cp);
      if (!cap) {
        std::cout << "unknown capturer type: " << cp.type << std::endl;
        return ExitCode::UNKNOWN_CAPTURER_TYPE;
      }

      // init
      if (cap->init(cp)) {
        cap->startCapture();
        mCapturers.push_back(std::move(cap));
      } else {
        std::cout << "capturer: " << cp.name
                  << " initialization error. skipped." << std::endl;
      }
    } else {
      std::cout
          << "capturer (" << capN
          << ") expected but definition not found in the ini file. skipped."
          << std::endl;
    }
  }

  // post-check capturer count
  if (mCapturers.size() == 0) {
    std::cout << "no capturer loaded. check ini definitions." << std::endl;
    return ExitCode::NO_CAPTURER;
  }

  std::cout << "househub started." << std::endl;

  // main loop
  int i = 0;
  uint64_t tLast = timeSinceEpochMs();
  while (!ExitFlag) {
    const uint64_t delta = (timeSinceEpochMs() - tLast);

    for (auto &c : mCapturers) {
      c->update(delta);
    }

    FileManager::instance().update(delta);
  }

  return ExitCode::NORMAL_EXIT;
}

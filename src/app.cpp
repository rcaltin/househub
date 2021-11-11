#include "app.h"
#include "INIReader.h"
#include "capturer_factory.h"
#include "file_manager.h"
#include "globals.h"
#include <csignal>
#include <iostream>

static std::string usage = "usage:"
                           "esp32cam-hub <name 1> <type 1> <uri 1> <name ...> "
                           "<type ...> <uri ...> <name N> <type N> <uri N>";

void signalHandler(int signum) {
  std::cout << "\n\n!! INTERRUPTED !!\n\nTerminating..." << std::endl;

  App::ExitFlag = true;
}

bool App::ExitFlag = false;

App::App() {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
}

App::~App() {}

int App::exec(int argc, char *argv[]) {

  std::string iniFile = "app.ini";
  if (argc == 2) {
    iniFile = argv[1];
  }

  // read ini
  INIReader ini(iniFile);
  if (ini.ParseError() != 0) {
    std::cout << "ini file could not load: " << iniFile << std::endl;
    return -1;
  }

  // init file-manager
  if (!FileManager::instance().init(
          ini.Get("file_manager", "record_dir", "./rec/"),
          ini.GetInteger("file_manager", "record_dir_size_limit_mb", 8192))) {
    std::cout << "file directory r/w error: "
              << FileManager::instance().getRecordingDir() << std::endl;
    return -2;
  }

  // pre-check capturer count
  const int capturerCount =
      ini.GetInteger("app_settings", "capturer_count", -1);
  if (capturerCount <= 0) {
    std::cout << "invalid or not defined capturer count." << std::endl;
    return -3;
  }

  // create capturers
  for (int i = 1; i <= capturerCount; ++i) {
    const std::string capN = "capturer" + std::to_string(i);
    if (ini.Get(capN.c_str(), "name", "_X_X_X_") != "_X_X_X_") {
      auto cap = CapturerFactory::createCapturer(
          ini.Get(capN.c_str(), "name", capN.c_str()),
          ini.Get(capN.c_str(), "type", "default"),
          ini.Get(capN.c_str(), "stream_uri", "localhost/stream"),
          ini.GetInteger(capN.c_str(), "output_fps", 10),
          ini.GetInteger(capN.c_str(), "output_width", 1024),
          ini.GetInteger(capN.c_str(), "output_height", 768),
          ini.GetInteger(capN.c_str(), "filter_k", 0),
          ini.GetInteger(capN.c_str(), "chunk_length_sec", 60),
          ini.Get(capN.c_str(), "fourcc", "mjpg").c_str(),
          ini.Get(capN.c_str(), "file_extension", ".avi"));

      if (cap) {
        mCapturers.push_back(std::move(cap));
      } else {
        std::cout << "unknown capturer type." << std::endl;
        return -3;
      }
    } else {
      std::cout << capN << " expected but not found in the ini file. skipped."
                << std::endl;
    }
  }

  // post-check capturer count
  if (mCapturers.size() == 0) {
    std::cout << "no capturer loaded. check ini definitions." << std::endl;
    return -4;
  }

  // init capturers
  for (auto &c : mCapturers) {
    if (c->init()) {
      c->startCapture();
    } else {
      std::cout << "capturer: " << c->getName() << " initialization error. "
                << std::endl;
    }
  }

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

  return 0;
}

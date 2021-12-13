#pragma once

#include "icapturer.h"
#include <atomic>
#include <memory>
#include <vector>

class App final {
public:
  App();

  App(const App &) = delete;

  App &operator=(const App &) = delete;

  ~App();

  ExitCode exec(int argc, char *argv[]);

  static void exit();

private:
  ExitCode initConfigManager(const std::string &iniFile);

  ExitCode initLogging();

  ExitCode initFileManager();

  ExitCode initCapturers();

  static void signalHandler(int signum);

  std::vector<std::unique_ptr<ICapturer>> mCapturers;
  static std::atomic_bool sExitFlag;
};

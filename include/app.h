#pragma once

#include "icapturer.h"
#include <memory>
#include <vector>

enum ExitCode : int {
  NORMAL_EXIT = 0,
  INI_LOAD_ERROR = -1,
  RW_ERROR = -2,
  BAD_CAPTURER_COUNT = -3,
  BAD_FOURCC = -4,
  UNKNOWN_CAPTURER_TYPE = -5,
  NO_CAPTURER = -6
};

class App {
public:
  App();
  ~App();

  int exec(int argc, char *argv[]);

  static bool ExitFlag;

private:
  std::vector<std::unique_ptr<ICapturer>> mCapturers;
};

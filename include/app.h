#pragma once

#include "icapturer.h"
#include <memory>
#include <vector>

class App {
public:
  App();
  ~App();

  int exec(int argc, char *argv[]);

  static bool ExitFlag;

private:
  std::vector<std::unique_ptr<ICapturer>> mCapturers;
};

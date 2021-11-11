#pragma once

#include "opencv2/opencv.hpp"
#include <ctime>
#include <chrono>
#include <string>

static std::string timeString(time_t t, bool localTime = true) {
  if (t == 0) {
    t = std::time(nullptr);
  }
  char tmp[20]; // "%F %T" + '\0' = 20 char
  std::strftime(&tmp[0], sizeof(tmp), "%F %T",
                localTime ? std::localtime(&t) : std::gmtime(&t));

  return std::string(tmp);
}

static uint64_t timeSinceEpochMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
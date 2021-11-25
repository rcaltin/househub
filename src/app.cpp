#include "app.h"
#include "capturer_factory.h"
#include "config_manager.h"
#include "file_manager.h"
#include "file_system.h"
#include "globals.h"
#include <csignal>

void signalHandler(int signum) {
  LOG(INFO) << "!! Signal " << signum << " received. Terminating... !!";

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
  google::InitGoogleLogging(argv[0]);
  google::SetStderrLogging(google::GLOG_INFO);

  LOG(INFO) << "househub preparing... ";

#ifdef WIN32
  std::string iniFile = "C:/househub.ini";
#else
  std::string iniFile = "/etc/househub/househub.ini";
#endif
  if (argc == 2) {
    iniFile = argv[1];
  }

  // init config-manager
  ConfigManager &cm = ConfigManager::instance();
  if (!cm.init(iniFile)) {
    LOG(FATAL) << "bad ini format: " << iniFile;
    return ExitCode::BAD_INI_FORMAT;
  }

  // ini version check
  if (cm.getInt("app_settings", "ini_version") < REQUIRED_INI_VERSION) {
    LOG(FATAL) << "bad ini version: " << iniFile;
    return ExitCode::BAD_INI_VERSION;
  }

  // create log dir
  const std::string logDir =
      cm.getString("app_settings", "log_dir", "./househub-logs/");
  if (!fs::exists(logDir) && !fs::create_directories(logDir)) {
    LOG(ERROR) << "log directory could not created: " << logDir;
  } else {
    for (int severity = 0; severity < google::NUM_SEVERITIES; ++severity) {
      google::SetLogDestination(severity, logDir.c_str());
      google::SetLogSymlink(severity, "");
    }
  }

  // set log overdue days
  google::EnableLogCleaner(cm.getInt("app_settings", "log_overdue_days", 30));

  // init file-manager
  FileManagerParams fmp;
  fmp.recordDir =
      cm.getString("file_manager", "record_dir", "./househub-records/");
  fmp.recordDirSizeLimitMB =
      cm.getInt("file_manager", "record_dir_size_limit_mb", 8192);
  fmp.recordDirSizeCheckIntervalSec =
      cm.getInt("file_manager", "record_dir_size_check_interval_sec", 10);
  fmp.useLocalTime = cm.getBool("file_manager", "use_localtime", false);
  if (!FileManager::instance().init(fmp)) {
    LOG(FATAL) << "file directory r/w error: " << fmp.recordDir;
    return ExitCode::RW_ERROR;
  }

  // pre-check capturer count
  const int capturerCount = cm.getInt("app_settings", "capturer_count", -1);
  if (capturerCount <= 0) {
    LOG(FATAL) << "invalid or not defined capturer count.";
    return ExitCode::BAD_CAPTURER_COUNT;
  }

  // create capturers
  for (int i = 1; i <= capturerCount; ++i) {
    const std::string capN = "capturer" + std::to_string(i);
    if (cm.getString(capN.c_str(), "name", "___N/A") != "___N/A") {
      CapturerParams cp;
      cp.name = cm.getString(capN.c_str(), "name", capN.c_str());
      cp.type = cm.getString(capN.c_str(), "type", "default");
      cp.streamUri =
          cm.getString(capN.c_str(), "stream_uri", "localhost/stream");
      cp.filterK = cm.getInt(capN.c_str(), "filter_k", 0);
      cp.videoOutStreamParams.name = cp.name;

      cp.videoOutStreamParams.fps = cm.getInt(capN.c_str(), "output_fps", 10);

      cp.videoOutStreamParams.outputSize =
          cv::Size(cm.getInt(capN.c_str(), "output_width", 1024),
                   cm.getInt(capN.c_str(), "output_height", 768));

      cp.videoOutStreamParams.chunkLengthSec =
          cm.getInt(capN.c_str(), "chunk_length_sec", 60);

      cp.videoOutStreamParams.uniformChunks =
          cm.getBool(capN.c_str(), "uniform_chunks", true);

      cp.videoOutStreamParams.fileExtension =
          cm.getString(capN.c_str(), "file_extension", ".avi");

      cp.videoOutStreamParams.watermark =
          cm.getBool(capN.c_str(), "watermark", true);

      cp.videoOutStreamParams.useLocaltime =
          cm.getBool(capN.c_str(), "use_localtime", true);

      const std::string fourcc = cm.getString(capN.c_str(), "fourcc", "mjpg");
      if (fourcc.length() != 4) {
        LOG(FATAL) << "bad fourcc value.";
        return ExitCode::BAD_FOURCC;
      }
      std::copy(fourcc.c_str(), fourcc.c_str() + 4,
                cp.videoOutStreamParams.fourcc);

      auto cap = CapturerFactory::createCapturer(cp);
      if (!cap) {
        LOG(FATAL) << "bad capturer type: " << cp.type;
        return ExitCode::BAD_CAPTURER_TYPE;
      }

      // init
      if (cap->init(cp)) {
        cap->startCapture();
        mCapturers.push_back(std::move(cap));
      } else {
        LOG(ERROR) << "capturer: " << cp.name
                   << " initialization error. skipped.";
      }
    } else {
      LOG(WARNING)
          << "capturer (" << capN
          << ") expected but definition not found in the ini file. skipped.";
    }
  }

  // post-check capturer count
  if (mCapturers.size() == 0) {
    LOG(FATAL) << "no capturer loaded. check ini definitions.";
    return ExitCode::NO_CAPTURER;
  }

  LOG(INFO) << "househub started.";

  // main loop
  using namespace std::chrono_literals;
  while (!ExitFlag) {
    // TODO command line interface
    std::this_thread::sleep_for(100ms);
  }

  LOG(INFO) << "househub stopped.";

  return ExitCode::NORMAL_EXIT;
}

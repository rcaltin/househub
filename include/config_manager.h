#pragma once

#include <map>
#include <string>

using KeyValueMap = std::map<std::string, std::string>;

class ConfigManager {
public:
  ~ConfigManager();

  static ConfigManager &instance();

  bool init(const std::string &iniFile);

  bool hasSection(const std::string &section);

  bool hasSectionKey(const std::string &section, const std::string &key);

  bool getBool(const std::string &section, const std::string &key,
               bool defaultValue = false) const;

  long getInt(const std::string &section, const std::string &key,
              long defaultValue = 0L) const;

  double getDouble(const std::string &section, const std::string &key,
                   double defaultValue = 0.0) const;

  std::string getString(const std::string &section, const std::string &key,
                        const std::string &defaultValue = "") const;

private:
  ConfigManager() = default;

  ConfigManager(const ConfigManager &) = delete;

  ConfigManager operator=(const ConfigManager &) = delete;

  ConfigManager operator=(const ConfigManager &&) = delete;

  std::string getRaw(const std::string &section, const std::string &key) const;

  static int iniHandler(void *userPtr, const char *section, const char *key,
                        const char *value);

  KeyValueMap mMap;
};

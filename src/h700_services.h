#pragma once

#include <string>

struct H700Status {
  int battery_percent = -1;
  bool charging = false;
  int brightness = -1;
  int volume = -1;
  bool autostart = false;
};

class H700Services {
 public:
  H700Services(std::string app_dir, std::string state_dir);

  H700Status ReadStatus() const;
  int ChangeBrightness(int delta) const;
  bool RestoreBrightness() const;
  int ChangeVolume(int delta) const;
  bool RestoreVolume() const;
  int HallState() const;
  // Retained for source compatibility. Frontend power paths must exit and let
  // launch.sh invoke the vendor script only after SDL has been destroyed.
  bool Suspend(bool automatic) const;
  bool SetAutostart(bool enabled) const;
  bool AutostartEnabled() const;
  bool SetPegasusSplash(bool enabled) const;
  bool SetRecommendedControls(bool enabled) const;

 private:
  int ReadInt(const std::string &path, int fallback) const;
  bool WriteInt(const std::string &path, int value) const;

  std::string app_dir_;
  std::string state_dir_;
};

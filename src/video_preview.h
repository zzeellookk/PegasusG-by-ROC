#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class VideoPreview {
 public:
  VideoPreview() = default;
  ~VideoPreview();

  void Start(const std::string &path, int width, int height);
  void StartVideo(const std::string &path, int width, int height, bool loop = false);
  void StopVideo();
  void SetAudio(const std::string &path, bool loop);
  void Stop();
  void ReleaseDevices();
  bool CopyLatestFrame(std::vector<std::uint8_t> *pixels, std::uint64_t *version);
  bool ConsumeAudioFinished() { return audio_finished_.exchange(false); }
  bool Running() const { return running_.load(); }

 private:
  void Decode(std::string path, int width, int height, bool loop);
  void AudioWorker();
  void StopAudio();
  void ShutdownAudio();
  void InterruptVideoDecoder();

  std::thread thread_;
  std::thread audio_thread_;
  std::mutex video_process_mutex_;
  std::int64_t video_decoder_pid_ = -1;
  std::mutex audio_control_mutex_;
  std::condition_variable audio_control_cv_;
  std::condition_variable audio_ack_cv_;
  std::string requested_audio_path_;
  bool requested_audio_loop_ = false;
  std::uint64_t audio_request_version_ = 0;
  std::uint64_t audio_ack_version_ = 0;
  bool audio_shutdown_ = false;
  std::atomic<bool> audio_finished_{false};
  std::atomic<bool> stop_{false};
  std::atomic<bool> running_{false};
  std::mutex mutex_;
  std::vector<std::uint8_t> frame_;
  std::uint64_t version_ = 0;
};

#include "video_preview.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#ifndef _WIN32
#include <alsa/asoundlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
#endif

namespace {

#ifdef _WIN32
#define ROC_POPEN _popen
#define ROC_PCLOSE _pclose
#else
#define ROC_POPEN popen
#define ROC_PCLOSE pclose
#endif

std::string ShellQuote(const std::string &value) {
#ifdef _WIN32
  std::string result = "\"";
  for (char ch : value) result += ch == '"' ? "\\\"" : std::string(1, ch);
  return result + "\"";
#else
  std::string result = "'";
  for (char ch : value) result += ch == '\'' ? "'\\''" : std::string(1, ch);
  return result + "'";
#endif
}

#ifndef _WIN32
void MarkOpenDescriptorsCloseOnExec() {
  DIR *directory = opendir("/proc/self/fd");
  if (!directory) return;
  const int directory_fd = dirfd(directory);
  while (dirent *entry = readdir(directory)) {
    int value = 0;
    bool numeric = entry->d_name[0] != '\0';
    for (const char *cursor = entry->d_name; *cursor; ++cursor) {
      if (*cursor < '0' || *cursor > '9') { numeric = false; break; }
      value = value * 10 + (*cursor - '0');
    }
    if (!numeric || value < 3 || value == directory_fd) continue;
    const int flags = fcntl(value, F_GETFD);
    if (flags >= 0) fcntl(value, F_SETFD, flags | FD_CLOEXEC);
  }
  closedir(directory);
}

struct AudioDecoder {
  pid_t pid = -1;
  int output_fd = -1;
};

struct VideoDecoder {
  pid_t pid = -1;
  int output_fd = -1;
};

bool SpawnVideoDecoder(const std::string &command, VideoDecoder *decoder) {
  if (!decoder) return false;
  int pipe_fds[2] = {-1, -1};
  if (pipe(pipe_fds) != 0) return false;
  for (int fd : pipe_fds) {
    const int flags = fcntl(fd, F_GETFD);
    if (flags >= 0) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
  }

  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return false;
  }
  posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);
  posix_spawn_file_actions_addopen(&actions, STDERR_FILENO,
                                   "/tmp/pegasus-gba-ffmpeg.log",
                                   O_WRONLY | O_CREAT | O_TRUNC, 0644);

  posix_spawnattr_t attributes;
  if (posix_spawnattr_init(&attributes) != 0) {
    posix_spawn_file_actions_destroy(&actions);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return false;
  }
  posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
  posix_spawnattr_setpgroup(&attributes, 0);

  char *arguments[] = {
      const_cast<char *>("sh"), const_cast<char *>("-c"),
      const_cast<char *>(command.c_str()), nullptr,
  };
  pid_t child = -1;
  const int result = posix_spawn(&child, "/bin/sh", &actions, &attributes,
                                 arguments, environ);
  posix_spawnattr_destroy(&attributes);
  posix_spawn_file_actions_destroy(&actions);
  close(pipe_fds[1]);
  if (result != 0) {
    close(pipe_fds[0]);
    return false;
  }
  decoder->pid = child;
  decoder->output_fd = pipe_fds[0];
  return true;
}

void StopVideoDecoder(VideoDecoder *decoder) {
  if (!decoder) return;
  if (decoder->output_fd >= 0) close(decoder->output_fd);
  decoder->output_fd = -1;
  if (decoder->pid <= 0) return;
  kill(-decoder->pid, SIGTERM);
  for (int attempt = 0; attempt < 20; ++attempt) {
    const pid_t result = waitpid(decoder->pid, nullptr, WNOHANG);
    if (result == decoder->pid || (result < 0 && errno == ECHILD)) {
      decoder->pid = -1;
      return;
    }
    usleep(1000);
  }
  kill(-decoder->pid, SIGKILL);
  while (waitpid(decoder->pid, nullptr, 0) < 0 && errno == EINTR) {}
  decoder->pid = -1;
}

bool SpawnAudioDecoder(const std::string &path, bool loop, AudioDecoder *decoder) {
  if (!decoder) return false;
  int pipe_fds[2] = {-1, -1};
  if (pipe(pipe_fds) != 0) return false;
  for (int fd : pipe_fds) {
    const int flags = fcntl(fd, F_GETFD);
    if (flags >= 0) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
  }

  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return false;
  }
  posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);
  posix_spawn_file_actions_addopen(&actions, STDERR_FILENO,
                                   "/tmp/pegasus-gba-audio.log",
                                   O_WRONLY | O_CREAT | O_TRUNC, 0644);

  posix_spawnattr_t attributes;
  if (posix_spawnattr_init(&attributes) != 0) {
    posix_spawn_file_actions_destroy(&actions);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return false;
  }
  posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
  posix_spawnattr_setpgroup(&attributes, 0);

  std::ostringstream command;
  command << "export LD_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu:/usr/lib:/lib; "
          << "if [ -f /usr/lib/aarch64-linux-gnu/libfontconfig.so.1.12.0 ]; then "
          << "export LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libfontconfig.so.1.12.0; "
          << "else unset LD_PRELOAD; fi; "
          << "exec ffmpeg -nostdin -loglevel error ";
  if (loop) command << "-stream_loop -1 ";
  command << "-i " << ShellQuote(path)
          << " -map 0:a:0? -vn -acodec pcm_s16le -f s16le -ac 2 -ar 44100 -";
  const std::string command_text = command.str();
  char *arguments[] = {
      const_cast<char *>("sh"), const_cast<char *>("-c"),
      const_cast<char *>(command_text.c_str()), nullptr,
  };
  pid_t child = -1;
  const int result = posix_spawn(&child, "/bin/sh", &actions, &attributes,
                                 arguments, environ);
  posix_spawnattr_destroy(&attributes);
  posix_spawn_file_actions_destroy(&actions);
  close(pipe_fds[1]);
  if (result != 0) {
    close(pipe_fds[0]);
    return false;
  }
  const int status_flags = fcntl(pipe_fds[0], F_GETFL);
  if (status_flags >= 0) fcntl(pipe_fds[0], F_SETFL, status_flags | O_NONBLOCK);
  decoder->pid = child;
  decoder->output_fd = pipe_fds[0];
  return true;
}

void StopAudioDecoder(AudioDecoder *decoder) {
  if (!decoder) return;
  if (decoder->output_fd >= 0) close(decoder->output_fd);
  decoder->output_fd = -1;
  if (decoder->pid <= 0) return;
  kill(-decoder->pid, SIGTERM);
  for (int attempt = 0; attempt < 20; ++attempt) {
    const pid_t result = waitpid(decoder->pid, nullptr, WNOHANG);
    if (result == decoder->pid || (result < 0 && errno == ECHILD)) {
      decoder->pid = -1;
      return;
    }
    usleep(1000);
  }
  kill(-decoder->pid, SIGKILL);
  while (waitpid(decoder->pid, nullptr, 0) < 0 && errno == EINTR) {}
  decoder->pid = -1;
}

bool WritePcmFrames(snd_pcm_t *pcm, const std::int16_t *samples, size_t frames) {
  constexpr size_t kChannels = 2;
  size_t offset = 0;
  while (offset < frames) {
    const snd_pcm_sframes_t written = snd_pcm_writei(
        pcm, samples + offset * kChannels, frames - offset);
    if (written < 0) {
      if (snd_pcm_recover(pcm, static_cast<int>(written), 1) < 0) return false;
      continue;
    }
    offset += static_cast<size_t>(written);
  }
  return true;
}

void WriteFadeToSilence(snd_pcm_t *pcm, std::int16_t left, std::int16_t right) {
  constexpr size_t kChannels = 2;
  constexpr size_t kFadeFrames = 882;
  std::array<std::int16_t, kFadeFrames * kChannels> fade{};
  for (size_t frame = 0; frame < kFadeFrames; ++frame) {
    const std::int32_t remaining = static_cast<std::int32_t>(kFadeFrames - frame);
    fade[frame * kChannels] = static_cast<std::int16_t>(
        static_cast<std::int32_t>(left) * remaining /
        static_cast<std::int32_t>(kFadeFrames));
    fade[frame * kChannels + 1] = static_cast<std::int16_t>(
        static_cast<std::int32_t>(right) * remaining /
        static_cast<std::int32_t>(kFadeFrames));
  }
  WritePcmFrames(pcm, fade.data(), kFadeFrames);
}
#endif

}  // namespace

VideoPreview::~VideoPreview() {
  ReleaseDevices();
}

void VideoPreview::Start(const std::string &path, int width, int height) {
  Stop();
  if (path.empty() || width <= 0 || height <= 0) return;
  StartVideo(path, width, height, false);
  SetAudio(path, false);
}

void VideoPreview::StartVideo(const std::string &path, int width, int height, bool loop) {
  StopVideo();
  if (path.empty() || width <= 0 || height <= 0) return;
  stop_ = false;
  thread_ = std::thread(&VideoPreview::Decode, this, path, width, height, loop);
}

void VideoPreview::StopVideo() {
  stop_ = true;
  InterruptVideoDecoder();
  if (thread_.joinable()) thread_.join();
  running_ = false;
  std::lock_guard<std::mutex> lock(mutex_);
  frame_.clear();
  ++version_;
}

void VideoPreview::InterruptVideoDecoder() {
#ifndef _WIN32
  pid_t pid = -1;
  {
    std::lock_guard<std::mutex> lock(video_process_mutex_);
    pid = static_cast<pid_t>(video_decoder_pid_);
  }
  if (pid > 0) kill(-pid, SIGTERM);
#endif
}

void VideoPreview::Stop() {
  StopVideo();
  StopAudio();
}

void VideoPreview::ReleaseDevices() {
  Stop();
  ShutdownAudio();
}

void VideoPreview::SetAudio(const std::string &path, bool loop) {
#ifndef _WIN32
  std::lock_guard<std::mutex> lock(audio_control_mutex_);
  if (!audio_thread_.joinable() && path.empty()) return;
  if (!audio_thread_.joinable()) {
    audio_shutdown_ = false;
    audio_thread_ = std::thread(&VideoPreview::AudioWorker, this);
  }
  if (requested_audio_path_ == path && requested_audio_loop_ == loop) return;
  audio_finished_.store(false);
  requested_audio_path_ = path;
  requested_audio_loop_ = loop;
  ++audio_request_version_;
  audio_control_cv_.notify_one();
#else
  (void)path;
  (void)loop;
#endif
}

void VideoPreview::StopAudio() {
#ifndef _WIN32
  audio_finished_.store(false);
  std::unique_lock<std::mutex> lock(audio_control_mutex_);
  if (!audio_thread_.joinable()) return;
  if (requested_audio_path_.empty() && audio_ack_version_ == audio_request_version_) return;
  requested_audio_path_.clear();
  requested_audio_loop_ = false;
  const std::uint64_t request = ++audio_request_version_;
  audio_control_cv_.notify_one();
  audio_ack_cv_.wait_for(lock, std::chrono::milliseconds(500), [this, request] {
    return audio_ack_version_ >= request || audio_shutdown_;
  });
#endif
}

void VideoPreview::ShutdownAudio() {
#ifndef _WIN32
  {
    std::lock_guard<std::mutex> lock(audio_control_mutex_);
    if (!audio_thread_.joinable()) return;
    requested_audio_path_.clear();
    requested_audio_loop_ = false;
    audio_shutdown_ = true;
    ++audio_request_version_;
    audio_control_cv_.notify_one();
  }
  audio_thread_.join();
#endif
}

void VideoPreview::AudioWorker() {
#ifndef _WIN32
  MarkOpenDescriptorsCloseOnExec();
  snd_pcm_t *pcm = nullptr;
  int result = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (result >= 0) {
    result = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE,
                                SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 1, 40000);
  }
  if (result < 0) {
    std::cerr << "[audio] ALSA initialization failed: " << snd_strerror(result) << '\n';
    if (pcm) snd_pcm_close(pcm);
    std::unique_lock<std::mutex> lock(audio_control_mutex_);
    while (!audio_shutdown_) {
      audio_ack_version_ = audio_request_version_;
      audio_ack_cv_.notify_all();
      const std::uint64_t version = audio_request_version_;
      audio_control_cv_.wait(lock, [this, version] {
        return audio_shutdown_ || audio_request_version_ != version;
      });
    }
    return;
  }
  MarkOpenDescriptorsCloseOnExec();

  constexpr size_t kChannels = 2;
  constexpr size_t kFramesPerChunk = 441;
  constexpr size_t kFadeFrames = 882;
  constexpr size_t kChunkBytes = kFramesPerChunk * kChannels * sizeof(std::int16_t);
  std::array<std::int16_t, kFramesPerChunk * kChannels> samples{};
  std::array<std::int16_t, kFramesPerChunk * kChannels> silence{};
  std::array<std::uint8_t, kChunkBytes> decoded_bytes{};
  size_t decoded_size = 0;
  std::uint64_t fade_in_frame = 0;
  std::int16_t last_left = 0;
  std::int16_t last_right = 0;
  std::uint64_t handled_version = 0;
  AudioDecoder decoder;
  bool pcm_ok = true;

  while (pcm_ok) {
    std::string requested_path;
    bool requested_loop = false;
    std::uint64_t requested_version = 0;
    bool shutdown = false;
    {
      std::lock_guard<std::mutex> lock(audio_control_mutex_);
      requested_path = requested_audio_path_;
      requested_loop = requested_audio_loop_;
      requested_version = audio_request_version_;
      shutdown = audio_shutdown_;
    }

    if (requested_version != handled_version) {
      if (decoder.pid > 0 || last_left != 0 || last_right != 0) {
        WriteFadeToSilence(pcm, last_left, last_right);
      }
      last_left = 0;
      last_right = 0;
      StopAudioDecoder(&decoder);
      decoded_size = 0;
      fade_in_frame = 0;
      if (!requested_path.empty() && !shutdown &&
          !SpawnAudioDecoder(requested_path, requested_loop, &decoder)) {
        std::cerr << "[audio] ffmpeg spawn failed\n";
      }
      handled_version = requested_version;
      {
        std::lock_guard<std::mutex> lock(audio_control_mutex_);
        audio_ack_version_ = handled_version;
        audio_ack_cv_.notify_all();
      }
    }
    if (shutdown) break;

    bool have_audio = false;
    bool decoder_finished = false;
    if (decoder.output_fd >= 0) {
      while (decoded_size < decoded_bytes.size()) {
        const ssize_t read_bytes = read(decoder.output_fd,
                                        decoded_bytes.data() + decoded_size,
                                        decoded_bytes.size() - decoded_size);
        if (read_bytes > 0) {
          decoded_size += static_cast<size_t>(read_bytes);
          continue;
        }
        if (read_bytes == 0) decoder_finished = true;
        else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
          decoder_finished = true;
        }
        break;
      }
      if (decoded_size == decoded_bytes.size()) {
        std::memcpy(samples.data(), decoded_bytes.data(), decoded_bytes.size());
        decoded_size = 0;
        have_audio = true;
        for (size_t frame = 0; frame < kFramesPerChunk && fade_in_frame < kFadeFrames;
             ++frame, ++fade_in_frame) {
          const std::int32_t gain = static_cast<std::int32_t>(fade_in_frame);
          for (size_t channel = 0; channel < kChannels; ++channel) {
            const size_t index = frame * kChannels + channel;
            samples[index] = static_cast<std::int16_t>(
                static_cast<std::int32_t>(samples[index]) * gain /
                static_cast<std::int32_t>(kFadeFrames));
          }
        }
      } else if (decoder_finished) {
        WriteFadeToSilence(pcm, last_left, last_right);
        last_left = 0;
        last_right = 0;
        decoded_size = 0;
        StopAudioDecoder(&decoder);
        bool completed_current_request = false;
        {
          std::lock_guard<std::mutex> lock(audio_control_mutex_);
          completed_current_request = !requested_loop &&
              requested_version == audio_request_version_ &&
              requested_path == requested_audio_path_;
        }
        if (completed_current_request) audio_finished_.store(true);
      }
    }

    if (have_audio) {
      pcm_ok = WritePcmFrames(pcm, samples.data(), kFramesPerChunk);
      if (pcm_ok) {
        last_left = samples[(kFramesPerChunk - 1) * kChannels];
        last_right = samples[(kFramesPerChunk - 1) * kChannels + 1];
      }
    } else {
      pcm_ok = WritePcmFrames(pcm, silence.data(), kFramesPerChunk);
    }
  }

  WriteFadeToSilence(pcm, last_left, last_right);
  StopAudioDecoder(&decoder);
  WritePcmFrames(pcm, silence.data(), kFramesPerChunk);
  snd_pcm_drain(pcm);
  snd_pcm_close(pcm);
#endif
}

bool VideoPreview::CopyLatestFrame(std::vector<std::uint8_t> *pixels,
                                   std::uint64_t *version) {
  if (!pixels || !version) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  if (frame_.empty() || *version == version_) return false;
  *pixels = frame_;
  *version = version_;
  return true;
}

void VideoPreview::Decode(std::string path, int width, int height, bool loop) {
#ifndef _WIN32
  MarkOpenDescriptorsCloseOnExec();
#endif
  std::ostringstream command;
#ifndef _WIN32
  command << "export LD_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu:/usr/lib:/lib; "
          << "if [ -f /usr/lib/aarch64-linux-gnu/libfontconfig.so.1.12.0 ]; then "
          << "export LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libfontconfig.so.1.12.0; fi; ";
#endif
  command << "exec ffmpeg -nostdin -loglevel error ";
  if (loop) command << "-stream_loop -1 ";
  command << "-i " << ShellQuote(path)
          << " -an -vf scale=" << width << ':' << height
          << ":force_original_aspect_ratio=decrease,pad=" << width << ':' << height
          << ":-1:-1:color=black -r 15 -pix_fmt rgba -f rawvideo -";
#ifdef _WIN32
  command << " 2>NUL";
  FILE *pipe = ROC_POPEN(command.str().c_str(), "rb");
#else
  VideoDecoder decoder;
  if (!SpawnVideoDecoder(command.str(), &decoder)) {
    std::cerr << "[video] ffmpeg spawn failed\n";
    return;
  }
  {
    std::lock_guard<std::mutex> lock(video_process_mutex_);
    video_decoder_pid_ = decoder.pid;
  }
  FILE *pipe = fdopen(decoder.output_fd, "r");
  if (!pipe) {
    StopVideoDecoder(&decoder);
    std::lock_guard<std::mutex> lock(video_process_mutex_);
    video_decoder_pid_ = -1;
    std::cerr << "[video] fdopen failed: " << std::strerror(errno) << '\n';
    return;
  }
#endif
  if (!pipe) {
    std::cerr << "[video] popen failed: " << std::strerror(errno) << '\n';
    return;
  }
  running_ = true;
  const size_t bytes = static_cast<size_t>(width) * height * 4;
  std::vector<std::uint8_t> buffer(bytes);
  constexpr auto kFramePeriod = std::chrono::microseconds(66667);
  auto next_frame_at = std::chrono::steady_clock::now();
  while (!stop_) {
    size_t offset = 0;
    while (offset < bytes && !stop_) {
      const size_t read = std::fread(buffer.data() + offset, 1, bytes - offset, pipe);
      if (read == 0) break;
      offset += read;
    }
    if (offset != bytes) break;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      frame_ = buffer;
      ++version_;
    }
    next_frame_at += kFramePeriod;
    while (!stop_) {
      const auto now = std::chrono::steady_clock::now();
      if (now >= next_frame_at) break;
      const auto remaining = next_frame_at - now;
      std::this_thread::sleep_for(std::min(
          remaining, std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                         std::chrono::milliseconds(5))));
    }
    const auto now = std::chrono::steady_clock::now();
    if (now > next_frame_at + kFramePeriod * 2) next_frame_at = now;
  }
  int result = 0;
#ifdef _WIN32
  result = ROC_PCLOSE(pipe);
#else
  std::fclose(pipe);
  decoder.output_fd = -1;
  StopVideoDecoder(&decoder);
  {
    std::lock_guard<std::mutex> lock(video_process_mutex_);
    video_decoder_pid_ = -1;
  }
#endif
  if (!stop_ && result != 0) std::cerr << "[video] ffmpeg exited status=" << result << '\n';
  running_ = false;
}

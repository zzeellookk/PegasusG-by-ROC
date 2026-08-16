# 天马 G（Pegasus Frontend）移植到 H700 Linux 掌机的分析

## 结论摘要

“天马 G”对应的前端本体是 [mmatyas/pegasus-frontend](https://github.com/mmatyas/pegasus-frontend)，不是某个 H700 专用仓库。以当前上游 `master`（提交 `6b322063a036db60cba5810fda82a3ce38f1e62f`，2026-07-13）和 RG34XXSP 实机探测为基线，结论需要分成两层：

- C++11 + Qt Quick/QML，源码没有 x86 汇编或 x86 ABI 假设。
- 上游明确覆盖 Linux、Raspberry Pi、Odroid 和嵌入式设备；SDL2 手柄实现可直接使用 Linux `/dev/input`/evdev 路径。
- 当前官方连续构建提供 Android64，但没有 AArch64 Linux/H700 发布包；Raspberry Pi 包是独立的旧 ARM 平台包，不能直接当作 H700 二进制。
- **源码移植到 AArch64 Linux 可行**，这部分仍是中等工作量。
- **完整 Pegasus 运行在当前 RG34XXSP 原厂固件上为中高风险**：实机内核明确关闭 DRM，没有 `/dev/dri`、X11 或 Wayland，显示是厂商 `/dev/fb0 + /dev/disp + /dev/ion`。
- 上游 CMake 将 Linux `aarch64/arm` 自动标记为 `PEGASUS_ON_EGLFS`，但不能再把它等同于“本机可用 EGLFS/KMS”。原厂固件需要 Qt linuxfb 软件渲染、定制 EGLFS fbdev/vendor EGL，或替换为具备 DRM/Panfrost 的固件。
- 原厂前端独占 fb/disp/ion/input/ALSA。启动模拟器前后的显示所有权释放/恢复是首要问题；Raspberry Pi KMS patch 只能参考生命周期思路，不能原样解决 Sunxi fbdev。

完整实机事实以 [RG34XXSP H700 系统与硬件开发基线](./RG34XXSP_H700_SYSTEM_HARDWARE_BASELINE.md) 为准。

## Git 仓库与许可证

### 前端本体

- [mmatyas/pegasus-frontend](https://github.com/mmatyas/pegasus-frontend)：C++/QML，GPLv3（仓库内 `LICENSE.md` 为完整许可证及附加条款）。
- [mmatyas/pegasus-frontend-translations](https://github.com/mmatyas/pegasus-frontend-translations)：翻译子模块。
- [mmatyas/pegasus-theme-grid](https://github.com/mmatyas/pegasus-theme-grid)：默认主题子模块。
- [mmatyas/SortFilterProxyModel](https://github.com/mmatyas/SortFilterProxyModel)：QML 模型依赖子模块。

### 天马 G 相关配套工具（不是前端本体）

- [jiangxincode/GameToolBox](https://github.com/jiangxincode/GameToolBox)：Go 编写的 ROM 管理工具，仓库描述明确写着面向“天马G/Batocera”等前端系统。
- [zhyohaGit/ForPegasus](https://github.com/zhyohaGit/ForPegasus)：Python 图鉴/元数据辅助工具。
- [jinws1993/TMGManager](https://github.com/jinws1993/TMGManager)：Python ROM 管理、刮削和入库工具。
- [mrwangyu2/pegasus-game-filter](https://github.com/mrwangyu2/pegasus-game-filter)：Python 桌面 ROM 筛选工具。

这些工具可作为 PC 端资源准备工具，不应作为 H700 掌机运行时的核心依赖。

## 代码结构与可移植性

### 图形栈

`src/app/main.cpp` 创建 `QGuiApplication`，`src/backend/FrontendLayer.cpp` 使用 `QQmlApplicationEngine` 加载 `qrc:/frontend/main.qml`。核心依赖为 Qt Core/QML/Quick/Multimedia/SQL/SVG，UI 大部分在 QML 中完成。

`cmake/PegasusTargetPlatform.cmake` 的逻辑是：Linux + `arm|aarch64` -> `PEGASUS_ON_EGLFS`，其他 Linux -> `PEGASUS_ON_X11`。这是编译期平台分类，不是目标机 DRM 能力探测。当前 RG34XXSP 原厂内核为 `CONFIG_DRM=n`，所以标准 Qt EGLFS KMS backend 不可用。可验证的候选是 `linuxfb + QT_QUICK_BACKEND=software`，或针对厂商 `libmali`/fbdev native window 构建 Qt EGLFS device integration。

### 输入

`src/backend/model/CMakeLists.txt` 默认选择 `GamepadManagerSDL2`；实现使用 SDL2 GameController/Joystick API，并内置/加载 controller mapping。对 H700 内置按键，只要内核暴露稳定的 evdev 设备并提供 SDL2，通常无需改动 C++ 输入层，主要工作是提供正确的 SDL mapping 和按键语义。

Qt Gamepad 是可选替代路径，但在精简掌机系统上 SDL2 更容易控制依赖和设备枚举，建议保留 SDL2。

### 游戏启动

`src/backend/ProcessLauncher.cpp` 通过 `QProcess` 启动命令，支持 `{file.path}`、工作目录和脚本钩子；EmulationStation 兼容 provider 读取 `es_systems.cfg` 与 `gamelist.xml`/元数据。这使 Pegasus 能复用现有 Batocera/EmulationStation 风格 ROM 目录和启动命令。

但是 `Backend::onProcessLaunched()` 会先销毁 QML 前端，游戏退出后再重建。当前原厂前端实际持有 `/dev/fb0`、`/dev/disp`、`/dev/ion`、输入 event 和 ALSA PCM；Pegasus 与模拟器也必须执行同等级别的释放/重建。仓库中的 [`etc/rpi4/kms_launch_fix.diff`](https://github.com/mmatyas/pegasus-frontend/blob/master/etc/rpi4/kms_launch_fix.diff) 可参考状态机和延迟启动思路，但其 EGLFS/KMS integration 操作不适用于本机专有 Sunxi fbdev 栈。

### 电源与系统控制

`src/backend/platform/PowerCommands_linux.cpp` 依次尝试 logind、ConsoleKit 和 `reboot`/`poweroff`/`suspend` 命令。很多掌机发行版没有完整 logind/ConsoleKit，建议在系统侧提供受控的 power helper 或脚本，并通过 Pegasus 的 `scripts/{reboot,shutdown,...}` 接口接入，避免让前端直接依赖 root 权限。

### 资源与运行时目录

`src/backend/Paths.cpp` 支持 portable 模式和 `$HOME/.config`/缓存目录。掌机镜像建议启用 `--portable` 或放置 `portable.txt`，把主题、配置、脚本和缓存固定在 ROM/数据分区，减少只读 rootfs 与多用户路径问题。

## H700 目标平台判断

实机为 RG34XXSP-class：四核 Cortex-A53 480-1512 MHz、Mali-G31、约 973 MiB 可用内存、720x480 内屏、Ubuntu 22.04 用户态、Linux 4.9.170 厂商 BSP。需要特别区分“用户态源码可移植”和“目标显示栈可运行”：公开的 [mporrato/alpine-h700](https://github.com/mporrato/alpine-h700) 也记录了 H700 镜像通常仍需从原厂 SD 卡提取 SPL、U-Boot、内核、模块及固件。

| 项目 | 判断 | 备注 |
|---|---|---|
| AArch64 编译 | 可行 | C++/Qt 代码无 x86 专属指令；需要 AArch64 Qt/交叉工具链 |
| 原厂固件 DRM/KMS | 不可用 | `CONFIG_DRM=n`、无 `/dev/dri`；不能靠安装 Mesa/libdrm 补齐 |
| 原厂 framebuffer 软件渲染 | 可做 PoC | 720x480；需自带 Qt linuxfb，验证 Qt Quick software backend 性能 |
| 原厂 vendor EGL | 条件可行 | Mali r20p0/ES 3.2；需要 Qt EGLFS fbdev/native-window 适配和显示独占 |
| SDL2 手柄 | 高可行 | 依赖 evdev 权限、SDL2 编译选项和 controller mapping |
| QML 性能 | 未验证，需裁剪 | 无 swap；视频、模糊、动画和大图缓存会放大 CPU/GPU/内存压力 |
| 外部模拟器切换 | 高风险 | 需释放/恢复 fb/disp/ion、EGL、输入和 ALSA，不是标准 DRM lease/VT |
| 电源键/关机 | 条件可行 | AXP2202 厂商 sysfs + `pwr_new.sh`/`forceOS.sh`，建议平台 helper 封装 |
| 直接使用官方二进制 | 不可取 | 没有官方 AArch64 Linux 包，必须自行构建和打包 |

## 推荐移植路线

### 阶段 0：实机探针（已完成）

本次已经在 RG34XXSP 实机确认：

```sh
uname -m                       # aarch64
test -e /dev/dri/card0         # false
cat /sys/class/graphics/fb0/modes
# U:720x480p-59
grep CONFIG_DRM /proc/config.gz
# CONFIG_DRM is not set
```

厂商 Mali 库报告 OpenGL ES 3.2，但 EGL window surface 必须在原前端释放显示后单独验证。已有 ROC 日志证明 KMSDRM、Wayland、X11、fbcon 和 DirectFB 均不能在本机提供真实 LCD 输出。

### 阶段 1：最小 AArch64 构建

1. 优先使用 Qt 5.15 LTS，以目标 glibc 2.35/AArch64 sysroot 构建 Core/Qml/Quick/Multimedia/Sql/Svg；首个 PoC 同时准备 linuxfb software 和 EGLFS fbdev 两个配置。
2. `git clone --recurse-submodules`，固定三个子模块版本；不要把 PC 主题工具链带入掌机运行时。
3. 先关闭可选 provider、APNG、视频和高成本主题特效，只保留 Pegasus metadata + EmulationStation provider + SDL2 gamepad。
4. Qt/QML 可随包分发；EGL/GLES 必须使用目标固件的 `libmali` 配套库，禁止优先加载 Mesa GBM/Panfrost。

### 阶段 2：H700 运行时适配

- PoC A：`QT_QPA_PLATFORM=linuxfb QT_QUICK_BACKEND=software`，先证明 720x480 主界面、输入和内存预算成立。
- PoC B：在停止原前端的维护窗口验证厂商 `mali-fbdev` EGL surface，再决定是否实现 Qt EGLFS fbdev hooks。
- 为 H700 内置按键加入 SDL mapping；校验 A/B、X/Y、L1/L2、R1/R2、Start/Select、音量/菜单键。
- 为 framebuffer/disp/ion、输入与 ALSA 实现显式 `Release -> Launch -> Reacquire` 状态机，连续测试恢复。
- 用 `systemd-logind`、发行版脚本或一个最小 setuid/polkit helper 实现关机/重启/挂起。
- 对视频预览、模糊背景、实时 shader、透明层做默认关闭或按设备能力降级；保留静态封面和轻量转场。

### 阶段 3：与 ROC Shell 的集成决策

建议不要把 Pegasus 的 Qt/QML 前端直接嵌进当前 ROC SDL2 壳层。更稳妥的选择是二选一：

1. **独立 Pegasus 运行模式**：ROC 负责系统级启动和设备探针，Pegasus 作为一个可替换的前端进程运行；通过配置文件/脚本共享 ROM 根目录和模拟器命令。
2. **复用数据/启动协议**：借鉴 Pegasus 的 metadata 格式、EmulationStation provider 和启动变量，把 UI 保持在 ROC 的 SDL2 渲染器中。这样避免同一设备上同时维护 SDL2 + Qt EGLFS 两套显示生命周期。

如果首要目标是尽快在 H700 上得到稳定、低内存的单一 Shell，方案 2 风险更低；如果需要 Pegasus 主题生态和现成 QML UI，方案 1 的复用率更高。

## 必须完成的验收清单

- 冷启动、切换主题、扫描 5k/10k ROM 的内存峰值和耗时。
- 连续启动/退出同一模拟器 20 次，确认 fb/disp/ion、EGL、输入、音频和前端重建不泄漏。
- 断开/重新连接 USB 手柄，确认 SDL 热插拔和 mapping 持久化。
- 设备横竖屏/旋转、睡眠唤醒、关机重启、低电量状态。
- 只读 rootfs、无网络、无 `$HOME` 写权限时的降级行为。
- GPLv3 及主题/图标/元数据资产的许可证清单，尤其是商业固件分发时不能把非 GPL 资产默认打包进去。

## 参考链接

- [Pegasus README](https://github.com/mmatyas/pegasus-frontend/blob/master/README.md)
- [Pegasus CMake platform detection](https://github.com/mmatyas/pegasus-frontend/blob/master/cmake/PegasusTargetPlatform.cmake)
- [Pegasus gamepad CMake selection](https://github.com/mmatyas/pegasus-frontend/blob/master/src/backend/model/CMakeLists.txt)
- [Pegasus process launcher](https://github.com/mmatyas/pegasus-frontend/blob/master/src/backend/ProcessLauncher.cpp)
- [Raspberry Pi KMS launch fix](https://github.com/mmatyas/pegasus-frontend/blob/master/etc/rpi4/kms_launch_fix.diff)
- [Pegasus continuous release assets](https://github.com/mmatyas/pegasus-frontend/releases/tag/continuous)
- [Alpine Linux for Allwinner H700](https://github.com/mporrato/alpine-h700)

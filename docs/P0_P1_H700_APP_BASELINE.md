# P0/P1 H700 独立应用基线

日期：2026-08-03

状态：P0/P1 工程基线已落地。当前产物是可由 `.sh` 启动的 H700 官方系统应用包，
不是 Knulli 系统包，也不依赖 Knulli 运行时。Knulli 源码仅作为 H700 可用系统形态的参考。

> 2026-08-07 实机补充：目标机确认为 RG34XXSP-class，原厂 Linux 4.9 内核关闭 DRM，
> 只有 Sunxi framebuffer/disp/ion 与厂商 Mali 栈。完整环境见
> [RG34XXSP H700 系统与硬件开发基线](./RG34XXSP_H700_SYSTEM_HARDWARE_BASELINE.md)。

## P0：官方系统 APPS 应用包

H700 应用打包入口：

```text
H700/build_app.ps1
H700/build_app.sh
H700/launcher/ROCFrontend.sh
H700/launcher/launch.sh
```

默认复用现有 ROCreader H700 构建资产：

```text
H700\sysroot
<external-runtime-reference>\APPS\ROCreader
```

默认输出结构：

```text
Roms/APPS/ROCFrontend.sh
Roms/APPS/ROCFrontend/
  roc_frontend
  launch.sh
  config.json
  version.txt
  assets/fonts/ui_font_02.ttf
  assets/schema/roc_library_schema.sql
  data/cache/
  data/conf/
  data/home/
  lib/
  lib_system_sdl/
```

构建命令：

```powershell
.\H700\build_app.ps1 -Output Stage -Version 0.03
.\H700\build_app.ps1 -Output Zip -Version 0.03
```

本次已验证生成：

```text
H700/dist_app/release_stage
H700/Downloads/ROCFrontend ver0.03 for H700 app.zip
```

`launch.sh` 默认先尝试原厂 SDL2 的非上游 `mali` 后端，再尝试通用后端：

```text
mali -> KMSDRM -> kmsdrm -> wayland -> x11 -> fbcon -> directfb
```

RG34XXSP 实测中，KMSDRM/Wayland/X11/fbcon/DirectFB 均不能输出到 LCD；`offscreen`
仅在 `--screenshot` 模式成功。原厂前端仍占用显示时，`mali` 报
`mali-fbdev: Can't create EGL window surface`。因此当前 P0 只证明 AArch64 二进制、依赖和
离屏渲染成立，尚未证明真机独占 LCD 渲染；下一项必须是在维护窗口释放原前端后单独验证
`SDL_VIDEODRIVER=mali`。

运行日志位置：

```text
Roms/APPS/ROCFrontend/data/logs/ROCFrontend.log
```

默认扫描根路径使用 Linux 冒号分隔，覆盖方式：

```sh
ROC_FRONTEND_SCAN_ROOTS=/mnt/mmc/Books:/mnt/sdcard/Books:/storage/roms
ROC_FRONTEND_MAX_SCAN_ITEMS=10000
```

## P1：多分辨率逻辑画布

当前布局以 `720x720` 作为 4 寸级设备的标准基准。低于 720 高的物理屏不再整体缩小，
而是按真实像素作为逻辑像素，让 `720x480` 表现为 `720x720` 的纵向裁切，
`640x480` 表现为 `720x480` 的左右缩短。

```text
framebuffer_height < 720:
  logical_width = framebuffer_width
  logical_height = framebuffer_height
  window_scale = 1.0

framebuffer_height >= 720:
  logical_height = 720
  window_scale = framebuffer_height / 720
  logical_width = framebuffer_width / window_scale
```

默认 density 固定为 1.0，高密屏不再压缩 UI。用户或机型适配脚本仍可覆盖：

```sh
ROC_FRONTEND_DENSITY=1.0
ROC_FRONTEND_SAFE_LEFT=0
ROC_FRONTEND_SAFE_TOP=0
ROC_FRONTEND_SAFE_RIGHT=0
ROC_FRONTEND_SAFE_BOTTOM=0
ROC_FRONTEND_DIAGNOSTICS=1
```

当前关键布局结果：

```text
640x480    logical=640x480  scale=1.000  density=1.00  compact  columns=4  card=138x207
720x480    logical=720x480  scale=1.000  density=1.00  compact  columns=4  card=156x234
720x720    logical=720x720  scale=1.000  density=1.00  square   columns=4  card=144x216
1600x1440  logical=800x720  scale=2.000  density=1.00  square   columns=4  card=163x245
```

这保证 `1600x1440` 近似 `720x720` 的 2x 像素展开，布局比例保持同类；
`720x480` 与 `640x480` 则优先保留小屏可读性，而不是显示更多但更小的按钮和图标。

## P1：白底 UI 与库数据雏形

视觉基调已从 SteamOS 深色方向切换为 wiliwili/IUX 更接近的白底、浅灰分区、粉色强调色、
蓝色焦点描边。当前截图矩阵：

```text
screenshots/roc-shell-640x480.png
screenshots/roc-shell-720x480.png
screenshots/roc-shell-720x720.png
screenshots/roc-shell-1600x1440.png
```

代码已按模块拆开，避免 `app.cpp` 继续膨胀：

```text
src/app.cpp                  生命周期、SDL 初始化、字体、截图、路径查找
src/app_state.cpp            导航、搜索、扫描线程、状态和诊断
src/app_render.cpp           顶栏、筛选栏、库网格、侧栏、底栏、toast
src/ui_draw.*                主题色与绘制基础函数
src/layout.*                 分辨率归一和布局求解
src/input.*                  输入到 UiAction 的语义映射
src/library_catalog.*        内存库、筛选、最近、收藏、演示数据
src/cover_grid_data_source.* wiliwili RecyclingGridDataSource 思路的可见索引层
src/library_scanner.*        本地目录扫描，按模块识别文件和游戏目录
src/online_sources.*         在线书源 ini 读取、设置页选择和在线小说演示目录
assets/schema/roc_library_schema.sql  后续 SQLite 持久化表结构
```

在线书源配置：

```text
assets/config/online_sources.ini
Roms/APPS/ROCFrontend/data/conf/online_sources.ini
```

小说页现在提供 `本地小说库 / 在线小说库` 分段切换；设置页显示 ini 中配置好的书源，A 键选择
当前在线网址并写回 ini。

当前扫描规则：

```text
小说：txt, epub, pdf, mobi, azw3
漫画：cbz, cbr, cb7, zip, rar, 7z
视频：mp4, mkv, avi, mov, webm, flv, m4v
音乐：mp3, flac, ogg, wav, m4a, aac, opus
ONS：目录内存在 nscript.dat 或 0.txt
KRKR：目录内存在 data.xp3 或任意 xp3
```

SQLite 代码暂未链接，因为当前 H700 sysroot 可见 `libsqlite3.so`，但没有确认可用的
`sqlite3.h` 开发头。P1 先交付 schema 与内存 Catalog/DataSource，等工具链补齐后再接入持久化。

## 当前验证

桌面验证在 WSL 中完成：

```powershell
wsl.exe --cd "/mnt/d/Works/PegasusG by ROC" make test
wsl.exe --cd "/mnt/d/Works/PegasusG by ROC" make all
wsl.exe --cd "/mnt/d/Works/PegasusG by ROC" make screenshots ratio-screenshots
```

H700 构建验证：

```text
roc_frontend: ELF 64-bit LSB pie executable, ARM aarch64
interpreter: /lib/ld-linux-aarch64.so.1
type: PIE
package: H700/Downloads/ROCFrontend ver0.03 for H700 app.zip
```

## 尚未完成

- 尚未在 H700 官方系统真机启动验证。
- 目前仍是 SDL2 直接绘制壳，还未形成 retained widget tree。
- Router/Intent、Presenter 异步生命周期保护、页面返回栈仍是下一轮架构目标。
- 业务模块仍未接入 ROCreader / ROCgalgame 的真实阅读、播放、游戏启动边界。
- 包内运行库先复用 ROCreader staging，后续应抽成共享 H700 runtime。

## 顶栏头像槽

顶部胶囊搜索框左侧已经预留圆形头像框。默认显示浅色占位头像；如果存在头像图片，会按中心裁切为圆形纹理并绘制到该位置。

查找顺序：
```text
data/conf/avatar.png
data/conf/avatar.jpg
data/conf/avatar.jpeg
assets/config/avatar.png
assets/config/avatar.jpg
assets/config/avatar.jpeg
avatar.png
avatar.jpg
avatar.jpeg
```

也可以通过运行参数或环境变量指定：
```sh
ROC_FRONTEND_AVATAR=/path/to/avatar.png
./roc_frontend --avatar /path/to/avatar.jpg
```

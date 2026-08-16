# ROC Shell 第一阶段实现记录

日期：2026-08-03

状态：桌面交互原型、P0 H700 应用包、P1 多分辨率布局和本地库数据雏形已可构建验证；
尚未完成 H700 官方系统真机启动验证。

## 1. 本阶段目标

本阶段先建立一个独立于 `ROCreader` 和 `ROCgalgame` 的 SDL2/C++ 前端壳，用真实运行结果验证：

1. 所有机型共用同一套基于约束的布局算法，不按机型名或精确分辨率选择 UI。
2. 十字键、左摇杆和 ABXY 先转为语义动作，再交给页面处理。
3. 小说、漫画、视频、音乐、ONS 游戏、KRKR 游戏等入口统一放在侧栏和库网格中。
4. 当前产品是可由 `.sh` 启动的独立应用，不是 Knulli 系统包。
5. UI 基调切换为 wiliwili/IUX 接近的白底浅色风格，不继续走 SteamOS 深色方向。

本阶段不启动阅读器或游戏核心，也不修改旧项目。

## 2. 已实现内容

```text
src/layout.cpp               多分辨率逻辑画布和布局约束
src/input.cpp                SDL 键盘/GameController 到 UiAction 的映射
src/app.cpp                  SDL 生命周期、字体、截图和资源查找
src/app_state.cpp            交互状态、搜索、扫描线程、诊断日志
src/app_render.cpp           顶栏、筛选栏、封面网格、侧栏、toast、底栏
src/ui_draw.cpp              主题色和基础绘制
src/library_catalog.cpp      演示库、查询、最近、收藏
src/library_scanner.cpp      本地文件和 ONS/KRKR 目录扫描
src/cover_grid_data_source.cpp 可见项索引数据源
src/online_sources.cpp       在线书源 ini 读取、保存和在线书库演示数据
```

测试与打包：

```text
tests/layout_test.cpp
tests/catalog_test.cpp
Makefile
H700/build_app.ps1
H700/build_app.sh
H700/launcher/*.sh
assets/schema/roc_library_schema.sql
```

左侧模块顺序：

```text
主页
小说
漫画
视频
音乐
ONS 游戏
KRKR 游戏
设置
电源
```

侧栏在所有屏幕比例下默认隐藏，不占用内容宽度。按 Menu/Start 或在库根页面返回后，
侧栏以带文字的覆盖层出现；选择模块或再次返回后收起。

## 3. 自适应规则

`720x720` 是当前标准布局。`1600x1440` 按 2x 级高密屏处理为 `800x720` 逻辑空间，
保持 4 列 square 布局。`720x480` 和 `640x480` 不再放大到 720 逻辑高，
而是保持物理像素逻辑尺寸，让小屏按钮和卡片不被整体缩小。

```text
framebuffer_height < 720:
  logical = framebuffer
  scale = 1.0

framebuffer_height >= 720:
  logical_height = 720
  scale = framebuffer_height / 720
```

当前关键矩阵：

```text
640x480    logical=640x480  compact  columns=4  card=138x207
720x480    logical=720x480  compact  columns=4  card=156x234
720x720    logical=720x720  square   columns=4  card=144x216
1600x1440  logical=800x720  square   columns=4  card=163x245
```

这些是连续的内容约束断点，不是设备表。代码中不得用 H700、A133P、RK3566、RK3568、
RK3576 或具体掌机名称决定 UI。

## 4. 控制器约定

| 输入 | 语义动作 | 当前行为 |
| --- | --- | --- |
| 十字键 / 左摇杆 | Up/Down/Left/Right | 侧栏、筛选和网格导航 |
| A | Confirm | 选择项目，并记录最近打开 |
| B | Back | 退出搜索、清空搜索或召回/关闭侧栏 |
| X | ContextPrimary | 收藏当前项目 |
| Y | ContextSecondary | 取消收藏当前项目 |
| LB / RB | TabPrevious/TabNext | 切换全部、最近、收藏筛选 |
| Start/Menu | Menu | 从任意区域召回侧栏 |
| Select/View | Search | 进入搜索输入 |

键盘开发映射：方向键、Enter/Space、Esc/Backspace、X/Y、Q/E 或方括号、PageUp/PageDown、
M 和 F。

小说模块额外支持 `本地小说库 / 在线小说库` 来源切换。焦点在筛选栏时按 Up 进入来源切换，
Left/Right 选择来源，粉色滑块底板表示当前库。在线小说按 A 进入全屏二级菜单，菜单中预留
“下载到本地 / 在线阅读 / 返回书库”。

## 5. 构建与验证

桌面验证在 WSL 中完成：

```powershell
wsl.exe --cd "/mnt/d/Works/PegasusG by ROC" make test
wsl.exe --cd "/mnt/d/Works/PegasusG by ROC" make all
wsl.exe --cd "/mnt/d/Works/PegasusG by ROC" make screenshots ratio-screenshots
```

截图矩阵：

```text
640x480   720x480   720x720   854x480
1024x768  1280x720  1280x800  1600x1440
```

H700 应用包验证：

```powershell
.\H700\build_app.ps1 -Output Stage -Version 0.03
.\H700\build_app.ps1 -Output Zip -Version 0.03
```

输出：

```text
H700/dist_app/release_stage
H700/Downloads/ROCFrontend ver0.03 for H700 app.zip
```

二进制校验：

```text
ELF 64-bit LSB pie executable, ARM aarch64
interpreter /lib/ld-linux-aarch64.so.1
```

## 6. 架构参考

wiliwili 的结构比 ROCreader 更适合前端扩展，当前已参考但未直接迁移：

```text
Activity    页面容器
Fragment    页面内子区域
Presenter   异步请求和业务逻辑
View        AutoTabFrame、RecyclingGrid 等复用控件
Intent      统一页面跳转入口
DataSource  列表可见项和 cell 生成边界
```

当前项目已先吸收 `DataSource` 分离、生命周期/状态/渲染拆文件、扫描线程取消这几件事。
下一阶段继续补 `Router/Intent`、`Screen/Fragment`、`Presenter/JobToken` 和通用网格数据源。

## 7. 当前限制

- 尚未在 H700 官方系统真机启动验证。
- 仍是 SDL2 直接绘制壳，还不是 retained widget tree。
- SQLite schema 已有，但 H700 sqlite 开发头尚未确认，当前使用内存 Catalog。
- 业务模块尚未接入 ROCreader / ROCgalgame 的真实数据、进度和启动边界。
- 电池图标仍是视觉样板，只有时间来自系统。
- 字体和纹理缓存还没有低内存淘汰策略。
- 顶栏已经加入圆形头像槽，支持 `data/conf/avatar.png|jpg|jpeg`、`ROC_FRONTEND_AVATAR` 和 `--avatar`；真实图片会在加载时中心裁切为圆形纹理。

## 8. 下一施工顺序

1. 建立 `Router/Intent` 和页面返回栈，替换散落在 `HandleAction()` 中的页面动作。
2. 继续拆出 `platform / ui / shell / library / modules` 目录边界。
3. 把扫描、封面生成、外部启动探测改成 Presenter/JobToken 风格的可取消任务。
4. 接入 SQLite repository，落地最近、收藏、进度和扫描源。
5. 分模块定义 Novel、Comic、Video、Music、ONS、KRKR 的 provider 与 launch contract。
6. 在 H700 官方系统真机上验证 SDL video driver、输入、字体、日志和退出行为。

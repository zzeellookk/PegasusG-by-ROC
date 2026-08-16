# P0/P1 架构说明

日期：2026-08-03

目标：让当前原型从一坨 `app.cpp` 走向可长期扩展的前端应用。短期继续使用 SDL2 直接绘制，
但代码边界按 wiliwili 更成熟的做法预留。

## wiliwili 架构观察

本地参考源码位于：

```text
research/external/wiliwili/wiliwili
```

wiliwili 的主结构比 ROCreader 更适合前端壳扩展：

```text
include/activity   页面容器，类似 Android Activity
include/fragment   页面内子区域，复杂页面拆成多个 Fragment
include/presenter  请求和业务逻辑层，负责异步数据回调
include/view       可复用 UI 控件，例如 AutoTabFrame、RecyclingGrid
include/api        外部服务和数据结构
include/utils      Intent、配置、对话框、线程等通用工具
```

最值得学习的点：

- `Intent` 集中处理页面跳转，业务代码不直接到处 push Activity。
- `Presenter` 有异步回调生命周期保护，页面销毁后不再更新 UI。
- `RecyclingGridDataSource` 把列表数据、可见 cell、选择动作从页面渲染中分离。
- `AutoTabFrame` 把侧栏/标签/焦点切换封装为控件，页面只注册 tab 和内容创建函数。
- XML 视图负责结构声明，C++ 负责行为绑定，能降低复杂页面的耦合。

当前项目不直接迁移 borealis/XML，但会吸收这些边界。

## 当前模块边界

```text
src/main.cpp
```

解析命令行和环境变量，创建 `AppOptions`。它不持有 UI 状态，不扫描文件，也不绘制。

```text
src/app.cpp
```

应用生命周期层：SDL/IMG/TTF 初始化、窗口、渲染器、字体、截图、资源查找、析构清理。

```text
src/app_state.cpp
```

状态和交互层：焦点区域、侧栏/筛选/网格导航、搜索输入、最近/收藏动作、扫描线程和诊断。

```text
src/app_render.cpp
```

绘制层：只读取当前状态和布局，绘制顶栏、筛选栏、库网格、侧栏、toast、底栏。

```text
src/ui_draw.*
```

主题色和 SDL 基础图元，避免渲染文件里反复散落低层绘制细节。

```text
src/layout.*
```

分辨率归一、safe area、布局模式、网格列数和卡片尺寸。

```text
src/input.*
```

输入适配层，把键盘和手柄事件转为 `UiAction`。

```text
src/library_catalog.*
```

内存库模型，提供查询、计数、最近、收藏和演示数据。

```text
src/cover_grid_data_source.*
```

可见索引数据源，当前对应 wiliwili `RecyclingGridDataSource` 的最小版本。

```text
src/library_scanner.*
```

本地内容扫描服务，按扩展名和目录标记生成不同模块的 `LibraryItem`。

## 下一步演进

P2 起建议继续拆成这些目录：

```text
src/platform/   SDL 窗口、渲染器、输入、路径、进程启动、日志
src/ui/         layout、theme、draw、widgets、focus
src/shell/      app shell、router、screens、module registry
src/library/    catalog、scanner、sqlite repository、artwork cache
src/modules/    novel、comic、video、music、ons、krkr 的 handler/provider
```

对应 wiliwili 的学习路线：

- 用 `Router/Intent` 替代散落在 `HandleAction()` 里的页面跳转和模块打开逻辑。
- 用 `Screen/Fragment` 拆开库首页、搜索页、设置页、启动确认页。
- 用 `Presenter/JobToken` 包装扫描、封面生成、外部进程探测，避免异步回调打到已销毁页面。
- 把 `CoverGridDataSource` 扩展成通用 `GridDataSource<T>`，支持分页、空状态、错误状态和刷新。
- 等 UI 复杂度上来后，再决定是否引入 retained widget tree 或轻量 XML/JSON 视图声明。

## 在线书架入口建议

在线书架不要作为全局侧边栏的新一级模块。侧边栏仍负责内容大类：小说、漫画、视频、音乐、ONS、KRKR。
在线/本地应该是某个内容大类内部的数据源模式，否则未来每个大类都会膨胀出“本地小说、在线小说、本地漫画、在线漫画”。

推荐交互：

```text
侧边栏选择 小说
  -> 小说库页面
     标题行切换：本地小说库 / 在线小说库
     本地：显示扫描到的本地书架
     在线：显示已添加的网址书架和在线目录
```

进入在线模式的入口：

- 首选：小说页标题行放一个 `本地小说库 / 在线小说库` 分段切换，粉色滑块底板表示当前来源。
- 次选：按 Menu 打开侧边栏时，在小说条目右侧显示当前来源状态，但不在侧边栏里完成切换。
- 后续设置页提供“在线书架源管理”，用于添加、编辑、删除网址，不承担日常阅读入口。

退出在线模式：

- B 从在线书架详情页返回在线书架列表。
- B 在在线书架列表返回小说库根页面，但保留 `在线` 模式。
- 在分段切换中选择 `本地` 回到本地书架。
- 全局 Back 不应静默退出前端，除非已经处于根页面且明确触发退出确认。

对应数据模型预留：

```text
LibrarySource
  module_id: novel/comic/...
  source_type: local/online
  root_path_or_url
  display_name
  enabled
```

当前 P1.5 已落地的过渡实现：

- 默认配置文件：`assets/config/online_sources.ini`。
- H700 包内运行配置：`Roms/APPS/ROCFrontend/data/conf/online_sources.ini`。
- 设置页显示已配置书源，A 键选择当前书源，并写回 ini 中的 `[online] selected=...`。
- 小说页可在 `本地小说库 / 在线小说库` 之间用十字键切换。
- 在线书籍按 A 进入全屏二级菜单，当前菜单预留“下载到本地 / 在线阅读 / 返回书库”。
- 真实在线解析、下载任务队列和本地入库仍是后续模块工作。

## P0/P1 冻结约束

- 不做 Knulli 系统包，只做官方系统可执行 `.sh` 应用。
- 不把 SteamOS 深色风格作为视觉目标，当前基调为 wiliwili/IUX 方向的白底前端。
- 小屏优先：720x720 是标准布局，720x480/640x480 保持可读尺寸并允许内容被裁切。
- 不再让 `app.cpp` 承担业务、渲染、扫描、数据源的全部职责。
- wiliwili 可作为开源架构参考，但本项目当前代码按自己的 SDL2 壳实现。

## 顶栏与个人化资源

顶栏现在拆成三个稳定区域：左侧圆形头像槽、中间胶囊搜索框、右侧状态图标。头像资源属于运行配置，不写死在 UI 中；默认从包内 `data/conf/avatar.png|jpg|jpeg` 查找，也可以用 `ROC_FRONTEND_AVATAR` 或 `--avatar` 指定任意图片路径。渲染层只持有预处理后的圆形纹理，资源查找和像素裁切仍放在 `src/app.cpp` 的生命周期层。

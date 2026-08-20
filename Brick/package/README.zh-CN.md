# 天马G by ROC — TrimUI Brick 安装说明

本包是 `dev/brick` 自动构建的 TrimUI Brick 测试包，不是 TrimUI 或
Pegasus Frontend 官方发布。

## 完整安装包

1. 先在天马G设置中关闭开机自启。
2. 备份存储卡上的 `Apps/PegasusG`，不要直接覆盖唯一的可运行版本。
3. 将完整包解压到存储卡根目录，确认存在
   `Apps/PegasusG/config.json`。
4. 从 Brick 的“应用”页面启动并完成实机检查。

完整包不包含 ROM、BIOS、存档、FFmpeg 或实验性核心。它调用固件和存储卡
现有的 `Emus/GBA` 启动脚本，不会覆盖 RetroArch 配置。

## 更新包

更新包只包含新编译的前端和三个稳定启动脚本。它只能覆盖在一个已经完整安装
且确认可运行的 `Apps/PegasusG` 目录上，不能用于空白存储卡安装。

如果新版本无法进入应用，请恢复备份，并保留
`Apps/PegasusG/data/brick-port.log` 供排查。

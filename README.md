# 服务端已迁移到[castserver-go](https://github.com/zwcway/castserver-go)


# Castspeaker
Castspeaker 是一个应用于基于以太网/WIFI局域网，C/S 架构，在带宽允许内支持更多数量扬声器客户端的数字扬声器管理系统，使用时钟同步保证所有扬声器客户端高质量的同步播放。Castspeaker可以自动发现局域网内的扬声器客户端，自动匹配采样率和位宽。支持设备分组，可以为每个分组单独指定播放源，通过在移动设备或者电脑端安装音频驱动以播放到指定的设备分组。可应用在智能家居中全屋音响，支持使用 homeassitant 作为管理端。

![overview](https://raw.githubusercontent.com/zwcway/castspeaker/main/doc/overview.png)

# 功能特色

- [x] *自动发现播放设备*。使用UDP Multicast协议。最多支持255个设备。255个设备在96K采样率、32位格式下需要带宽约800Mbps。
- [ ] *设备分组*。最多支持255个分组。
- [x] *声道路由*。最多支持17声道。声道和播放设备映射关系。
- [ ] *分组路由*。
- [ ] *DSP*。
- [ ] *同步模式*。通过调整扬声器设备的时钟使传输速率与Sample Frame信号保持同步。缺点是会有少量Jitter，依赖设备时钟。
- [ ] *低延迟模式*。最小缓存，实时输出。适合游戏等场景。
- [ ] *高品质模式*。高延迟，大缓存，杜绝丢包，零Jitter。适合听音乐等场景。
- [ ] *web端控制*。
- [ ] *命令行控制*。
- [ ] *音量控制*。
- [ ] *分组控制*。
- [ ] *支持音箱自动调音*

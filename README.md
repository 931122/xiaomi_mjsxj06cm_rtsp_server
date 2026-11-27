# xiaomi_mjsxj06cm_rtsp_server
[@hatsujouki](https://github.com/hatsujouki/xiaomi-camera-MJSXJ09CM-hack) 程序分析

[@bxinquan](https://github.com/bxinquan/rtsp_demo) rtsp服务器

[@Troll338cz](https://github.com/Troll338cz/mini_telnetd) telnetd程序

[@SungurLabs](https://github.com/SungurLabs/sungurlabs.github.io/blob/6043366d497943e0a246a6a420ba8fb2adfcef31/_posts/2021-07-14-Xiaomi-Smart-Camera---Recovering-Firmware-and-Backdooring.md) telned开启方法

- 理论适用所有创米的摄像头

- MJSXJ06CM 我没有找到串口，使用spi编程器将flash拷贝出后添加了telnetd


视频格式：前96字节为固定头 第81个字节起4个字节为视频流（音频流）长度。

使用方法：

1. 修改Makefile配置交叉编译

	`cd mini_telnetd; make`

	`cd rtsp_demo; make`

2. 参考[@SungurLabs](https://github.com/SungurLabs/sungurlabs.github.io/blob/6043366d497943e0a246a6a420ba8fb2adfcef31/_posts/2021-07-14-Xiaomi-Smart-Camera---Recovering-Firmware-and-Backdooring.md) 修改/etc/init.d/rcS, 在最后添加

	`/usr/bin/telnetd &`

	`/mnt/sdcard/hacks/run.sh &`

	`# cat /mnt/sdcard/hacks/run.sh
#!/bin/sh
CONFIG_PATH=/mnt/sdcard/hacks/etc
BIN_PATH=/mnt/sdcard/hacks/bin

sleep 90 && $BIN_PATH/demo`

3. 本地控制云台

	仓库内提供了一个基于 `libmortox` 的本地测试程序 `mortox_lib_demo.c`，不依赖 `miio token`。

	编译：

	`/opt/toolchain/arm-sigmastar-linux-uclibcgnueabihf-9.1.0/bin/arm-sigmastar-linux-uclibcgnueabihf-9.1.0-gcc -Wall -Wextra -O2 mortox_lib_demo.c -ldl -o mortox_lib_demo`

	使用：

	`./mortox_lib_demo left`

	`./mortox_lib_demo right`

	`./mortox_lib_demo up`

	`./mortox_lib_demo down`

	默认从当前目录加载 `./libmortox.so.0.0.0`，默认连接 `/tmp/mortox.socket`，也可以手动指定：

	`./mortox_lib_demo left ./libmortox.so.0.0.0 /tmp/mortox.socket`

## ONVIF 适配与双向语音对讲

已基于 `zry98/MJSXJ05CM-ONVIF` 增加独立 ONVIF 服务并完成群晖 Surveillance Station 实测适配（视频、低底噪音频、PTZ 云台控制、ONVIF Backchannel 双向语音对讲）：

- `rtsp_demo/`：RTSP 服务，主/子码流（`/main` 2304x1296 H265、`/sub` 640x360 H265）均支持真实共享内存时间戳与音频流；内置 21 抽头抗混叠 FIR 低通降采样滤波（支持 `--audio-rate 8000|16000`，默认 `8000`）以及 ONVIF Backchannel（`Require: www.onvif.org/ver20/backchannel`，支持 `PCMU/8000` / `PCMA/8000` 自动转码上采样至 16kHz A-law 播放）。
- `onvif_srvd/`：ONVIF Device/Media/PTZ SOAP 服务（Profile S / Profile T，含视频/音频/音频输出解码配置、PTZ 控制、WS-Security `mustUnderstand` 兼容），构建时使用 gSOAP 生成绑定代码。
- `onvif_wsdd/`：本项目新增的轻量 WS-Discovery responder，用于 ONVIF 客户端自动发现。
- `onvif_srvd/mi_ptz_control`：把 ONVIF PTZ 上下左右命令落到 `libmortox` 的 `motor_cmd`。
- `scripts/start_onvif_stack.sh`：同时启动 RTSP、ONVIF SOAP、WS-Discovery。
- `scripts/stop_onvif_stack.sh`：停止上述进程。

一键编译与打包：

```sh
make            # 交叉编译 mi_rtsp_server / onvif_srvd / mi_ptz_control / onvif_wsdd，并同步到 bin/mi_rtsp_server
make package    # 生成可部署压缩包 dist/mjsxj06cm_onvif-<timestamp>.tar.gz
```

也可以分模块编译：

```sh
cd rtsp_demo && make
cd ../onvif_wsdd && make
cd ../onvif_srvd && make release
```

设备端解压与启动（摄像头 BusyBox `tar` 不支持 `-z` 且 SD 卡为 FAT32）：

```sh
gunzip -c mjsxj06cm_onvif-*.tar.gz | tar -xf - -C /mnt/sdcard/hacks/
sh /mnt/sdcard/hacks/mjsxj06cm_onvif/scripts/start_onvif_stack.sh
```

常用环境变量：

```sh
ONVIF_IFACE=wlan0         # 摄像头网卡（默认 wlan0）
RTSP_PORT=554             # 当前 RTSP 端口
ONVIF_PORT=1000           # ONVIF SOAP HTTP 端口
START_RTSP=1              # 如果 RTSP 已由系统脚本单独启动，设为 0
MORTOX_SOCKET=/tmp/mortox.socket
MORTOX_LIB=/mnt/sdcard/hacks/mjsxj06cm_onvif/lib/libmortox.so.0.0.0
```

默认暴露两路 Media profile：

- `rtsp://<ip>:554/main`（2304x1296 H265 + 音频 + ONVIF Backchannel 对讲）
- `rtsp://<ip>:554/sub`（640x360 H265 + 音频）

注意：参考 ONVIF 项目使用 Media1 WSDL，profile 编码枚举原生只有 `JPEG/MPEG4/H264`。当前 RTSP 代码实际声明为 H265，客户端（含群晖 Surveillance Station、go2rtc 等）最终会按 RTSP SDP 正常解码播放；如果遇到客户端严格校验 ONVIF profile 编码，需要把 RTSP 流切到 H264 或后续升级到 Media2/H265 描述。

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

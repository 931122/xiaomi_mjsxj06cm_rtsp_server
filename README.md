# xiaomi_mjsxj06cm_rtsp_server
[@hatsujouki](https://github.com/hatsujouki/xiaomi-camera-MJSXJ09CM-hack) 程序分析

[@bxinquan](https://github.com/bxinquan/rtsp_demo) rtsp服务器

[@Troll338cz](https://github.com/Troll338cz/mini_telnetd) telnetd程序

[@SungurLabs](https://github.com/SungurLabs/sungurlabs.github.io/blob/6043366d497943e0a246a6a420ba8fb2adfcef31/_posts/2021-07-14-Xiaomi-Smart-Camera---Recovering-Firmware-and-Backdooring.md) telned开启方法

- 理论适用所有创米的摄像头

- MJSXJ06CM 我没有找到串口，使用spi编程器将flash拷贝出后添加了telnetd

- 音频格式g711，期待大佬分析(应该是两路音频，共640字节，只发320字节，应该是没有降噪)

视频格式：前96字节为固定头 第81个字节起4个字节为视频流（音频流）长度。

使用方法：

1. 修改Makefile配置交叉编译

	`cd mini_telnetd; make`

	`cd rtsp_demo; make`
2. 参考[@SungurLabs](https://github.com/SungurLabs/sungurlabs.github.io/blob/6043366d497943e0a246a6a420ba8fb2adfcef31/_posts/2021-07-14-Xiaomi-Smart-Camera---Recovering-Firmware-and-Backdooring.md) 修改/etc/init.d/rcS, 在最后添加

	`/usr/bin/telnetd &`

	`/mnt/sdcard/hacks/run.sh &`

# Architecture

当前代码先按“媒体入口”和“设备桥接”分开，目的是给后续 ONVIF 留稳定接口。

## 1. 已拆开的模块

### `rtsp_demo/rtsp_demo.c`

负责 RTSP/RTP 协议面：

- RTSP server
- `DESCRIBE / SETUP / PLAY / RECORD`
- RTP over UDP/TCP
- 音频接收回调 `rtsp_set_audio_rx_hook()`

这一层不关心小米设备内部的 `/run/audio_*` 细节。

### `rtsp_demo/mi_unix.c`

负责设备侧 Unix `SOCK_SEQPACKET` 连接。

后面如果要接：

- `mortox.socket`
- `/run/audio_play/control`
- `/run/audio_in/control`

都应该优先复用这里，而不是在业务文件里重复写 `socket/connect`。

### `rtsp_demo/mi_audio_play.c`

负责把入向音频桥接到 `/run/audio_play/control`：

- RTP payload 拼成 `640-byte` 对讲帧
- 写入 `4-byte header + 640-byte payload`
- 失败后自动重连一次

后续 ONVIF Talk 也应直接复用这一层。

### `rtsp_demo/mi_audio_codec.c`

负责轻量音频格式处理。

当前只放了：

- `16k A-law -> 8k A-law` 下采样

后续如果要补：

- `PCMU -> PCMA`
- `8k <-> 16k`
- PCM/A-law 转换

建议继续放这里。

## 2. 后续 ONVIF 建议结构

建议把后续功能按下面拆：

### `onvif/control`

控制面：

- PTZ Move / Stop / Preset
- Talk capability
- Profile / Media / PTZ service discovery

### `onvif/media`

媒体入口：

- ONVIF 音频对讲入口
- RTP/RTSP 接收或发送适配

### `device/bridge`

设备桥接：

- `mi_audio_play`
- `mortox` PTZ bridge
- 设备内部 socket / shared memory / control node

这样 ONVIF 只负责“标准协议”，设备适配只负责“怎么落到这台机子上”。

## 3. 当前新增骨架

### `device/mi_ptz_bridge.*`

已经把 PTZ 设备侧接口抽成独立 bridge：

- 动态加载 `libmortox`
- 自动读取当前 `motor_cmd`
- 自动生成下一个序号
- 提供 `left/right/up/down` 统一入口

后面 ONVIF PTZ 直接调这一层，不再重复碰 `mortox` 细节。

### `onvif/onvif_service.*`

这是一个很轻的 service skeleton：

- `ptz_move` 回调
- `talk_push` 回调

后面真正实现 ONVIF SOAP / WS-Discovery / Media / PTZ / TwoWayAudio 时，
协议层只需要把请求翻译成这两个回调即可。

## 4. 当前可复用的落点

### 云台

已经有独立桥：

- `device/mi_ptz_bridge.*`

### 对讲

已经有现成桥：

- `mi_audio_play_rtsp_rx_hook()`
- `mi_audio_play_bridge_push()`

后面 ONVIF 只要把音频帧送到这里即可。

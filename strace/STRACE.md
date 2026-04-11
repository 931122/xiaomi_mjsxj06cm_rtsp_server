# 云台控制分析

本文记录 `MJSXJ09CM / MJSXJ06CM` 在本地云台控制上的分析过程，重点基于 `mortoxd`、`miio_miss`、`strace` 和 `libmortox.so.0.0.0`。

## 1. 目标

目标不是走局域网 `miio token` 控制，而是直接在摄像头本地找到云台控制链路，做到：

- 不依赖 token
- 直接本地控制
- 确认方向码
- 做出可执行的本地 C demo

## 2. 初始判断

参考项目：

- `hatsujouki/xiaomi-camera-MJSXJ09CM-hack`

项目中已经提到：

- `mortoxd` 会输出 `miio_token`
- `miiocli chuangmicamera rotate ...` 可以控制云台

这说明云台控制一定和系统内部的某条控制链有关，但是否直接由 `mortoxd` 驱动，需要进一步确认。

## 3. 对 mortoxd 做 strace

使用：

```sh
strace -ff -s 256 -xx -p $(pidof mortoxd) \
  -e trace=recv,recvfrom,recvmsg,sendto,sendmsg,write
```

在手机 App 中按云台方向键后，可以稳定观察到这些键：

- `mem/default/motor_cmd`
- `mem/default/motor_position`
- `mem/default/time_motor`
- `nvram/default/motor`

典型日志中可以看到：

```text
motor_cmd = "[49,2,52]"
motor_cmd = "[49,1,53]"
motor_cmd = "[49,3,54]"
motor_cmd = "[49,4,55]"
```

结合实际按键顺序：

- 右
- 左
- 上
- 下

可得到方向码映射：

- `1 = left`
- `2 = right`
- `3 = up`
- `4 = down`

这与 `python-miio` 中 `set_motor` 的方向定义一致。

## 4. 从 strace 推断 mortox 的角色

单次方向动作的典型链路是：

1. 有客户端向 `mortoxd` 写入 `mem/default/motor_cmd`
2. `mortoxd` 返回 ACK
3. `mortoxd` 向其他 fd 广播同样的更新
4. 随后出现 `motor_position`
5. 更新时间 `time_motor`
6. 再写入 `nvram/default/motor`
7. 通过 `FLSH...` 刷新到 flash

由此可以推断：

- `mortoxd` 更像“状态总线 + 持久化中心”
- 它不是最底层的电机驱动本体
- 真正执行电机动作的逻辑在订阅该状态的其他模块中

## 5. 为什么最开始手搓 socket 包只能 ACK 但不一定生效

最开始直接连接 `/tmp/mortox.socket`，重放抓到的 `motor_cmd` 包：

```text
reply (42 bytes):
4d 4f 52 54 4f 58 ...
```

可以拿到稳定 ACK，说明：

- socket 路径正确
- 外层协议基本正确

但是云台并不总是动作。原因是：

- `motor_cmd` 的第三个数字不是随便写的
- 它更像一个事务号 / 序号
- 如果重复发送同样的序号，下游执行者会忽略

例如抓到的真实命令值：

- `left  -> [49,1,53]`
- `right -> [49,2,52]`
- `up    -> [49,3,54]`
- `down  -> [49,4,55]`

第一次能动，连续重复同一条不动，就是这个序号机制造成的。

## 6. 为什么要盯 miio_miss

对 `miio_miss` 做静态字符串分析：

```sh
strings /mnt/data/bin/miio_miss | grep -E 'mortox|motor|set_motor|motor_cmd|motor_position'
```

可以直接看到：

- `libmortox.so.0`
- `libmortox_config_get`
- `libmortox_reconnect`
- `libmortox_close`
- `libmortox_config_open`
- `libmortox_get_value`
- `libmortox_register_notification`
- `libmortox_set_value`
- `libmortox_sync`
- `panoramic_motor_cmd`
- `motor_cmd`
- `motor_position`
- `time_motor`
- `Connected to mortox.`

这说明：

- `miio_miss` 明确通过 `libmortox` 使用 `mortoxd`
- 云台控制链路不是猜测，确实经过 `libmortox_*`

## 7. 改走 libmortox，而不是继续硬猜协议

既然已经拿到了 `libmortox.so.0.0.0`，就没必要继续手搓全部 wire protocol。

导出符号里有：

- `libmortox_open`
- `libmortox_simple_get`
- `libmortox_simple_set`
- `libmortox_close`
- `xdata_value_as_char`
- `xdata_free`

进一步反汇编确认了两件事：

### 7.1 `libmortox_open`

`libmortox_open(1, path, 0)` 会走 Unix socket：

- `type == 1` 对应 `AF_UNIX`
- 直接 `connect()` 到传入路径

所以本地直连可以写成：

```c
client = libmortox_open(1, "/tmp/mortox.socket", 0);
```

### 7.2 `libmortox_simple_set`

这是一个 6 参数函数：

```c
libmortox_simple_set(client, namespace, group, key, value_type, value)
```

对于字符串值，`value_type` 用 `1`。

所以写 `motor_cmd` 的最小调用是：

```c
libmortox_simple_set(client, "mem", "default", "motor_cmd", 1, "[49,1,53]");
```

### 7.3 `libmortox_simple_get`

`libmortox_simple_get()` 会返回一个 `xdata *`，然后通过：

```c
xdata_value_as_char(xdata)
```

转成字符串值。

因此可以先读当前：

```c
mem/default/motor_cmd
```

再解析最后一个序号，生成下一条命令。

## 8. 最终策略

最终采用的本地控制方案是：

1. `dlopen("./libmortox.so.0.0.0")`
2. `dlsym()` 获取：
   - `libmortox_open`
   - `libmortox_simple_get`
   - `libmortox_simple_set`
   - `libmortox_close`
   - `xdata_value_as_char`
   - `xdata_free`
3. `libmortox_open(1, "/tmp/mortox.socket", 0)`
4. 读取 `mem/default/motor_cmd`
5. 解析当前最后一个序号
6. 构造新命令：
   - `left  -> [49,1,next_seq]`
   - `right -> [49,2,next_seq]`
   - `up    -> [49,3,next_seq]`
   - `down  -> [49,4,next_seq]`
7. 调用 `libmortox_simple_set(...)`

这个方案已经验证可用。

## 9. 当前仓库里的结果

当前仓库内已经加入本地测试程序：

- `mortox_lib_demo.c`
- `mortox_lib_demo`

该工具特点：

- 不依赖 `miio token`
- 直接使用本地 `libmortox`
- 自动读取当前 `motor_cmd`
- 自动生成下一个有效序号

使用方法：

```sh
./mortox_lib_demo left
./mortox_lib_demo right
./mortox_lib_demo up
./mortox_lib_demo down
```

也可以显式指定库和 socket：

```sh
./mortox_lib_demo left ./libmortox.so.0.0.0 /tmp/mortox.socket
```

## 10. 关键结论

结论如下：

1. 云台方向码已经确认：
   - `1 = left`
   - `2 = right`
   - `3 = up`
   - `4 = down`

2. `mortoxd` 是状态总线，不是唯一执行者。

3. 真正可靠的本地控制方式不是手搓裸包，而是调用 `libmortox`。

4. `motor_cmd` 的最后一个数字是有效序号的一部分，重复值可能被忽略。

5. 正确做法是先读当前 `motor_cmd`，再生成下一条命令。

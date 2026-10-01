# nRF54L15 DK Build Notes

## 环境

| 项目 | 值 |
|------|-----|
| SDK | `/opt/nordic/ncs/v3.3.0` |
| Toolchain | `0c0f19d91c` (`/opt/nordic/ncs/toolchains/0c0f19d91c`) |
| Board target | `nrf54l15dk/nrf54l15/cpuapp` |
| Board serial | `<YOUR_DK_SERIAL>` (PCA10156) |
| Host | macOS arm64 |

## 常用命令

```bash
# 环境变量（每次新终端必须设置）
export PATH="/opt/nordic/ncs/toolchains/0c0f19d91c/bin:$PATH"
export ZEPHYR_SDK_INSTALL_DIR="/opt/nordic/ncs/toolchains/0c0f19d91c/opt/zephyr-sdk"
export ZEPHYR_BASE="/opt/nordic/ncs/v3.3.0/zephyr"
cd /opt/nordic/ncs/v3.3.0

# Build（-p 强制清理重建）
west build -p -b nrf54l15dk/nrf54l15/cpuapp <sample_path> -d <build_dir>

# Flash（必须用 --runner jlink，见下方说明）
west flash -d <build_dir> --dev-id <YOUR_DK_SERIAL> --runner jlink

# Reset & Run（flash 后如果没自动启动）
JLinkExe -USB <YOUR_DK_SERIAL> -nogui 1 -if swd -speed 4000 -device nRF54L15_M33 \
  -CommanderScript /dev/stdin <<'EOF'
r
g
q
EOF

# 查连接的板子 serial
"$HOME/Library/Application Support/nrfconnect/nrfutil-sandboxes/arm64/device/2.17.5/bin/nrfutil-device" list

# 调试：halt + 读寄存器（排查崩溃用）
JLinkExe -USB <YOUR_DK_SERIAL> -nogui 1 -if swd -speed 4000 -device nRF54L15_M33 \
  -CommanderScript /dev/stdin <<'EOF'
halt
regs
q
EOF
```

## 踩坑记录

### 1. `--runner jlink` 是必须的

**现象：** `west flash` 默认用 nrfutil runner，报错：
```
Unable to find worker executable: Failed to find file plugin-probe-worker
```

**原因：** nrfutil-device sandbox 里的 `plugin-probe-worker` 路径与 west 期望的不一致。

**解法：** 始终加 `--runner jlink`，依赖系统已安装的 JLinkExe (`/usr/local/bin/JLinkExe`)。

---

### 2. Flash 后需要手动 Reset

**现象：** `west flash` 完成，LED 没变化。

**原因：** JLink runner 的 reset 序列（`writeDP 1 0`）在某些情况下无法可靠地启动应用。

**解法：** Flash 后执行 `r` + `g` 命令，或者直接重新上电。

---

### 3. 线程栈 512 字节不够

**现象：** 多线程程序 flash 后 LED 完全没反应，串口也无输出。

**诊断：**
```
XPSR: IPSR = 006 (UsageFault)
PSP = 20001528 = PSPLIM   ← PSP == PSPLIM，栈溢出
```

**原因：** `K_THREAD_STACK_DEFINE` 设为 512 字节，不够 `pwm_set_dt` + `printk` 的调用栈深度。

**解法：** 调用了 PWM / GPIO driver 函数的线程，栈至少设 **1024 字节**。

**诊断方法：**
```bash
# halt 后看 IPSR 和 PSP vs PSPLIM
JLinkExe ... halt; regs
# IPSR=6 = UsageFault，PSP==PSPLIM = 栈溢出
```

---

### 4. JLink 直接操作必须用 `nRF54L15_M33` 设备名

**现象：** 用 `-device CORTEX-M33` 运行 `loadfile` 时，`Writing target memory failed`，即使先执行了 `erase` 也一样。

**原因：** `CORTEX-M33` 是通用设备名，JLink 没有对应的 nRF54L15 flash 算法（secure domain / RRAMC 需要芯片专属初始化脚本）。

**解法：** 直接调用 JLinkExe 时，设备名必须用 `nRF54L15_M33`：
```bash
JLinkExe -device nRF54L15_M33 -if SWD -speed 4000 -autoconnect 1
```

`west flash --runner jlink` 内部已自动使用正确设备名（从 `runners.yaml` 读取），不受此影响。

---

### 5. nrf-bm SDK ≠ 标准 NCS

`/opt/nordic/ncs/nrf-bm/v1.0.0` 是 **Bare Metal SDK**（无 Zephyr，基于 SoftDevice），专用于纯 BLE 应用。Matter / Thread / OpenThread 必须用标准 NCS（`/opt/nordic/ncs/v3.3.0`）。用 nrf-bm 编 Matter 样例会触发 `image_signing_softdevice.cmake` 报错。

---

## PWM 硬件约束

nRF54L15 的 PWM domain 限制：**只有 GPIO Port P1 的引脚才能接硬件 PWM**（PWM20/21/22 与 P1 同域）。

| LED | GPIO | PWM 可用 |
|-----|------|----------|
| LED0 | P2.9 | ❌ 只能 GPIO |
| LED1 | P1.10 | ✅ PWM20 ch0（板子默认配置） |
| LED2 | P2.7 | ❌ 只能 GPIO |
| LED3 | P1.14 | ✅ PWM21 ch0（需 overlay 添加） |

PWM21 overlay 写法见 `blinky_pwm/boards/nrf54l15dk_nrf54l15_cpuapp.overlay`。

---

## 已验证的样例

| 样例 | 路径 | 状态 |
|------|------|------|
| blinky_pwm（4-LED 改版） | `lab/board-configs/ncs/blinky_pwm` | ✅ 运行正常 |
| button + k_work 消抖 | `lab/board-configs/ncs/button` | ✅ 运行正常 |
| synchronization（sem + mutex） | `lab/board-configs/ncs/synchronization` | ✅ 运行正常（VCOM1） |
| shell_module | `lab/board-configs/ncs/shell_module` | ✅ 运行正常（VCOM1，`uart:~$` 提示符） |
| peripheral_lbs | `lab/board-configs/ncs/peripheral_lbs` | ✅ 运行正常（BLE 广播 Nordic_LBS，LED 控制 + Button notify 均验证） |
| matter_light_bulb | `lab/board-configs/ncs/matter_light_bulb` | 🔄 固件运行正常（shell 可见），待 commission |
| matter_light_switch | `lab/board-configs/ncs/matter_light_switch` | 🔄 固件运行正常（shell 可见），待 commission |

---

## 串口映射（VCOM0 vs VCOM1）

nRF54L15 DK 通过 J-Link OB 暴露两个 CDC-ACM 虚拟串口。macOS 下编号规则：

| 端口 | macOS 路径（<SERIAL>）| 用途 |
|------|-------------------------------|------|
| VCOM0 | `/dev/tty.usbmodem<SERIAL12>1` | 无输出（空闲） |
| VCOM1 | `/dev/tty.usbmodem<SERIAL12>3` | **Zephyr shell + log**（`uart:~$`） |

`<SERIAL12>` 是 J-Link 序列号（`nrfutil device list` 查）补零到 12 位。

VCOM1 端口号 = VCOM0 + 2（最后一位）。macOS 的 `pyserial` 默认 `dtr=True` 时 VCOM0 没有输出，原因未明；直接用原始 TTY 读 VCOM1 可靠。

**读 shell 最小代码：**
```python
import os, time, select, termios

fd = os.open('/dev/tty.usbmodem<SERIAL12>3', os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
attrs = termios.tcgetattr(fd)
attrs[0]=0; attrs[1]=0; attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL
attrs[3]=0; attrs[4]=termios.B115200; attrs[5]=termios.B115200
termios.tcsetattr(fd, termios.TCSANOW, attrs)

os.write(fd, b'matter onboardingcodes ble\r\n')
time.sleep(2)
r,_,_ = select.select([fd],[],[],1)
if r: print(os.read(fd, 4096).decode())
os.close(fd)
```

---

## fw_info 固件标识系统

所有样例通过 `common/fw_info/` 公共模块自动嵌入构建标识：

| 文件 | 作用 |
|------|------|
| `fw_info.cmake` | CMake include，自动计算 CRC、生成 `fw_info_generated.c` |
| `fw_info.c.in` | 结构体模板，32 字节，含 magic/crc/git_hash/build_time/version/builder_id |
| `fw_info.ld` | `KEEP(*(.fw_info_block))` 防止 gc-sections 裁掉 |
| `fw_info_extract.py` | post-build：从 ELF 提取符号地址，写 `build/fw_info.txt` |
| `verify_fw_info.py` | 验证工具：JLink 读设备内存，对比 CRC 和期望值 |

**使用：** 在 `CMakeLists.txt` 末尾加一行：
```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../common/fw_info/fw_info.cmake)
```

**验证：**
```bash
python3 common/fw_info/verify_fw_info.py <YOUR_DK_SERIAL> \
    build/matter_light_switch/fw_info.txt
```

---

## Matter 固件踩坑：.matter IDL 与静态 cluster 配置必须同步

### 症状

`west flash` 后固件完全无 UART 输出，JLink `halt; regs` 显示 PC 卡在 `fatal.c` 里的 `for(;;){}`，IPSR=003（HardFault）或普通异常。

### 根因

NCS v3.3.0 的 Matter 层有两套"endpoint 数量"来源：

1. `.zap` → 代码生成 → `endpoint_config.h`（`FIXED_ENDPOINT_COUNT=4`）
2. `.matter` IDL → `zap-generated` 目录 → `static-cluster-config/*.h`（`kFixedClusterConfig` 数组）

两套必须 **完全一致**。如果 `.matter` 只声明了 EP0+EP1，而 `.zap` 生成了 4 个端点，启动时 `attribute-storage.cpp` 里的 `VerifyOrDie` 会在 EP2/EP3 初始化时触发 abort — 此时 UART 尚未初始化，没有任何输出。

### 修复

在 `.matter` IDL 里补齐 EP2、EP3 的完整声明（binding cluster + server cluster），然后 `west build -p` 触发重新代码生成。`static-cluster-config/*.h` 里的 `kFixedClusterConfig` 数组会自动扩展到 4 个端点。

---

## Matter T-7 Debug 经验（Thread 直连 binding）

### 串口 Shell 命令前缀

自定义命令通过 `chip::Shell::Engine::Root().RegisterCommands()` 注册，在 Matter shell 里，不在 Zephyr 根 shell。

正确前缀：
```
matter switch bind 0xABCDEF 1     ✅
matter switch nodes               ✅
matter nodeid                     ✅（DK#2）
matter acl grant 0xABCDEF        ✅（DK#2）

switch bind ...                   ❌ command not found
```

### `FabricInfo::GetNodeId()` 不可靠

`matter nodeid` 内部调用 `FabricInfo::GetNodeId()` 返回的值 **与 mDNS operational node ID 不一致**，不能直接用于 binding。

正确做法：在 DK#1 上运行 `matter dns browse operational`，找到对方实际广播的 node ID。

DK#2 在本 session 里的 Fabric 1 node ID = `0xa1e29908`（每次重新 commission 后会变）。

### node ID 映射关系

```
Fabric A6BD9F564E9CFA57 (Apple Home primary):
  DK#1: 0x352CC03C  （matter switch nodes 的 Fabric 1）
  DK#2: 0xa1e29908  （matter nodeid 的 Fabric 1，commission 后确认）
  HomePod mini: 0x6D547BCF

Fabric 5DBCDF992175A4EA (Apple Home secondary):
  DK#1: 0x7b9e2b
  DK#2: 0xb60328de
```

注：node ID 在每次重新 commission 后会变，但 fabric ID 不变。

### ACL 问题：直连 binding 默认被拒绝

Matter 直连 binding（DK#1 → DK#2）时，DK#2 的 Access Control List 只允许 Apple Home hub 发命令，不允许 peer 设备直接访问。

**现象：** DK#1 发 Toggle，DK#2 返回 `status 0x7e (AccessControl: denied)`，LED 无反应。

**解法：** 在 DK#2 上运行 `matter acl grant 0x352CC03C`，添加 Operate 权限给 DK#1。

代码实现（`app_task.cpp`）：
```cpp
chip::Access::AccessControl::Entry entry;
chip::Access::GetAccessControl().PrepareEntry(entry);
entry.SetFabricIndex(fabricIndex);
entry.SetPrivilege(chip::Access::Privilege::kOperate);
entry.SetAuthMode(chip::Access::AuthMode::kCase);
entry.AddSubject(nullptr, nodeId);
chip::Access::GetAccessControl().CreateEntry(nullptr, entry);
```

**重要：ACL 存储在 flash 里，重新 commission 后会被 Apple Home 覆盖，需要重新运行 `matter acl grant`。**

### Flash 不要用 `--erase`

```bash
# ✅ 保留 commissioning 数据
west flash --build-dir build --snr <serial>

# ❌ 会擦除所有 NVM，Matter commissioning 数据丢失，需重新配对
west flash --build-dir build --snr <serial> --erase
```

### nRF54L15 DK 按键编号（0-indexed）

板子上物理标注的 "Button 0" / "Button 1" 对应 Zephyr 的 DK_BTN1/DK_BTN2（注意 Nordic 宏是 1-indexed）：

| 物理标注 | Zephyr 别名 | 代码宏 | `BIT()` |
|----------|-------------|--------|---------|
| Button 0 | sw0 | DK_BTN1_MSK | BIT(0) |
| Button 1 | sw1 | DK_BTN2_MSK | BIT(1) |
| Button 2 | sw2 | DK_BTN3_MSK | BIT(2) |
| Button 3 | sw3 | DK_BTN4_MSK | BIT(3) |

`light_switch` 用 `DK_BTN2_MSK`（BIT(1)）= 物理 **Button 1**。

### Build 环境（macOS，无 nrfutil bootstrap）

```bash
export TOOLCHAIN=/opt/nordic/ncs/toolchains/0c0f19d91c
export PATH="$TOOLCHAIN/Cellar/ccache/4.3_1/bin:$TOOLCHAIN/Cellar/ninja/1.13.2/bin:$TOOLCHAIN/Cellar/cmake/4.2.1/bin:$TOOLCHAIN/bin:$TOOLCHAIN/opt/zephyr-sdk/arm-zephyr-eabi/bin:$TOOLCHAIN/nrfutil/home/bin:$TOOLCHAIN/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin:$PATH"
export ZEPHYR_SDK_INSTALL_DIR=$TOOLCHAIN/opt/zephyr-sdk
export NRFUTIL_HOME=$TOOLCHAIN/nrfutil/home
export ZEPHYR_BASE=/opt/nordic/ncs/v3.3.0/zephyr
```

west 在 `$TOOLCHAIN/Cellar/python@3.12/.../bin/west`，不在 `$TOOLCHAIN/bin/`。

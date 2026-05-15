# lab

Personal hardware lab repository — firmware experiments, driver examples, and board configurations for Nordic Semiconductor development kits and custom hardware.

## Repository Structure

```
lab/
├── board-configs/
│   ├── ncs/          # Zephyr / nRF Connect SDK samples (NCS v3.3.0)
│   └── nrf5-sdk/     # Legacy nRF5 SDK samples (SDK 17.x, armgcc / SES / Keil / IAR)
├── lib/
│   └── nrf5-sdk/     # nRF5 SDK submodule (read-only mirror)
├── instruments/      # Lab instrument configs and scripts
├── measurements/     # Raw measurement data
├── waveforms/        # Captured waveforms
└── reports/          # Lab reports and write-ups
```

## board-configs/ncs

Zephyr-based samples targeting the **nRF54L15 DK** (`nrf54l15dk/nrf54l15/cpuapp`). Each sample is a self-contained west project with board overlays for the DK.

| Sample | Description | Status |
|--------|-------------|--------|
| `blinky_pwm` | 4-LED PWM blink (multi-thread, custom overlay) | ✅ |
| `button` | Button input with `k_work` debounce | ✅ |
| `synchronization` | Semaphore + mutex demo | ✅ |
| `shell_module` | Zephyr shell over VCOM1 | ✅ |
| `peripheral_lbs` | BLE peripheral — LED Button Service | ✅ |
| `matter_light_bulb` | Matter over Thread, Apple Home, PWM dimming | ✅ |
| `matter_light_switch` | Matter over Thread, direct binding | ✅ |
| `openthread_cli` | OpenThread CLI shell | ✅ |

See [`board-configs/ncs/build-notes.md`](board-configs/ncs/build-notes.md) for build environment setup, flash commands, and hardware-specific gotchas (PWM domain constraints, stack sizes, JLink runner requirement, Matter ACL / binding workflow).

### Quick Start (NCS)

```bash
# Set up environment (every new terminal)
export TOOLCHAIN=/opt/nordic/ncs/toolchains/0c0f19d91c
export PATH="$TOOLCHAIN/Cellar/cmake/4.2.1/bin:$TOOLCHAIN/Cellar/ninja/1.13.2/bin:$TOOLCHAIN/bin:$TOOLCHAIN/opt/zephyr-sdk/arm-zephyr-eabi/bin:$TOOLCHAIN/nrfutil/home/bin:$TOOLCHAIN/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin:$PATH"
export ZEPHYR_SDK_INSTALL_DIR=$TOOLCHAIN/opt/zephyr-sdk
export ZEPHYR_BASE=/opt/nordic/ncs/v3.3.0/zephyr

# Build
west build -p -b nrf54l15dk/nrf54l15/cpuapp board-configs/ncs/<sample> -d board-configs/ncs/build

# Flash (JLink runner required on this host)
west flash -d board-configs/ncs/build --runner jlink
```

## board-configs/nrf5-sdk

Legacy nRF5 SDK examples targeting **nRF52840 DK** (PCA10056), **nRF52 DK** (PCA10040), and **kik0001** (custom nRF52840-based board). All IDEs are supported: armgcc, SES, Keil (arm5_no_packs), and IAR.

| Module | IC / Peripheral | Boards |
|--------|-----------------|--------|
| `eeprom/at24c256` | AT24C256 I²C EEPROM (TWI) | pca10040, pca10056, kik0001 |
| `hall-effect/tmag5170` | TMAG5170 single-axis SPI hall sensor | pca10040, pca10056, kik0001 |
| `hall-effect/tmag5170x4` | TMAG5170 ×4 multiplexed | pca10040, pca10056, kik0001 |
| `projects/bootloader` | Custom DFU bootloader | kik0001 |
| `projects/hid` | USB HID (mouse + keyboard composite) | kik0001 |
| `projects/uncategorized` | Miscellaneous experiments | various |

The nRF5 SDK itself lives in `lib/nrf5-sdk` (git submodule). All Makefiles and `.emProject` files reference it via relative path.

### Quick Start (nRF5 SDK)

```bash
# Initialize the submodule after cloning
git submodule update --init lib/nrf5-sdk

# armgcc build example
cd board-configs/nrf5-sdk/eeprom/at24c256/pca10056/blank/armgcc
make

# Flash via nrfjprog
nrfjprog --program _build/nrf52840_xxaa.hex --sectorerase --verify --reset
```

## Cloning

```bash
git clone --recurse-submodules git@github.com:KefanZhg/lab.git
# or, if already cloned without submodules:
git submodule update --init
```

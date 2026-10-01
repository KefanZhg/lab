#!/usr/bin/env python3
"""
verify_fw_info.py — read fw_info block from device via JLink and verify CRC.

Usage:
    python3 verify_fw_info.py <serial> <fw_info.txt>

    serial      J-Link device serial number (from `nrfutil device list`)
    fw_info.txt path to fw_info.txt written by post-build step

Example:
    python3 verify_fw_info.py <YOUR_DK_SERIAL> \
        ~/dev/lab/board-configs/ncs/matter_light_switch/build/fw_info.txt
"""

import sys, struct, zlib, subprocess, tempfile, os, re, datetime

FW_INFO_MAGIC       = 0x46574946
FW_INFO_STRUCT_SIZE = 32
FW_INFO_STRUCT_FMT  = '<IIIIBBBBHHII'  # 32 bytes little-endian


def read_device_memory(serial: str, addr: int, length: int) -> bytes:
    """Use JLinkExe to read <length> bytes from <addr> on the connected device."""
    script = (
        f"h\n"
        f"mem {addr:#010x},{length}\n"
        f"q\n"
    )
    with tempfile.NamedTemporaryFile(mode='w', suffix='.jlink', delete=False) as f:
        f.write(script)
        script_path = f.name

    try:
        result = subprocess.run(
            ['JLinkExe', '-USB', serial, '-nogui', '1',
             '-if', 'swd', '-speed', '4000', '-device', 'nRF54L15_M33',
             '-CommanderScript', script_path],
            capture_output=True, text=True, timeout=20
        )
    finally:
        os.unlink(script_path)

    output = result.stdout
    # JLink mem output format: "000C9C38 = 46 49 57 46 D4 BC 90 88  D1 97 DF A1 ..."
    raw = bytearray()
    for line in output.splitlines():
        m = re.match(r'[0-9A-Fa-f]{8,}\s*=\s*((?:[0-9A-Fa-f]{2}\s*)+)', line)
        if m:
            for byte_hex in m.group(1).split():
                raw.append(int(byte_hex, 16))
    return bytes(raw[:length])


def parse_block(data: bytes) -> dict:
    if len(data) < FW_INFO_STRUCT_SIZE:
        raise ValueError(f"Short read: got {len(data)} bytes, need {FW_INFO_STRUCT_SIZE}")
    fields = struct.unpack(FW_INFO_STRUCT_FMT, data[:FW_INFO_STRUCT_SIZE])
    return {
        'magic':       fields[0],
        'crc32':       fields[1],
        'git_hash':    fields[2],
        'build_time':  fields[3],
        'ver_major':   fields[4],
        'ver_minor':   fields[5],
        'ver_patch':   fields[6],
        'ver_build':   fields[7],
        'builder_id':  fields[8],
        'struct_size': fields[9],
        'reserved0':   fields[10],
        'reserved1':   fields[11],
    }


def compute_crc(block: dict) -> int:
    d = struct.pack(FW_INFO_STRUCT_FMT,
        block['magic'], 0,
        block['git_hash'], block['build_time'],
        block['ver_major'], block['ver_minor'],
        block['ver_patch'], block['ver_build'],
        block['builder_id'], block['struct_size'],
        block['reserved0'], block['reserved1'])
    return zlib.crc32(d) & 0xFFFFFFFF


def load_fw_info_txt(path: str) -> dict:
    info = {}
    with open(path) as f:
        for line in f:
            k, _, v = line.strip().partition('=')
            info[k.strip()] = v.strip()
    return info


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    serial = sys.argv[1]
    info_txt = sys.argv[2]

    ref = load_fw_info_txt(info_txt)
    addr = int(ref['fw_info_addr'], 16)
    expected_crc = int(ref['crc'], 16)

    print(f"[verify_fw_info] Reading from device {serial} at {addr:#010x} ...")
    data = read_device_memory(serial, addr, FW_INFO_STRUCT_SIZE)

    if len(data) < FW_INFO_STRUCT_SIZE:
        print(f"  FAIL: could not read {FW_INFO_STRUCT_SIZE} bytes (got {len(data)})")
        sys.exit(2)

    block = parse_block(data)

    print(f"  magic      : {block['magic']:#010x}  ({'OK' if block['magic'] == FW_INFO_MAGIC else 'BAD — not fw_info block!'})")

    if block['magic'] != FW_INFO_MAGIC:
        print("  FAIL: magic mismatch, wrong address or firmware not flashed")
        sys.exit(3)

    actual_crc = compute_crc(block)
    crc_ok = (actual_crc == block['crc32'])
    match  = (block['crc32'] == expected_crc)

    ts = datetime.datetime.utcfromtimestamp(block['build_time']).strftime('%Y-%m-%d %H:%M:%S UTC') \
         if block['build_time'] else 'unknown'

    ver_str = f"{block['ver_major']}.{block['ver_minor']}.{block['ver_patch']}"
    if block['ver_build']:
        ver_str += f".{block['ver_build']}"

    print(f"  version    : {ver_str}")
    print(f"  git_hash   : {block['git_hash']:#010x}")
    print(f"  build_time : {block['build_time']} ({ts})")
    print(f"  builder_id : {block['builder_id']}")
    print(f"  struct_size: {block['struct_size']}")
    print(f"  crc (device) : {block['crc32']:#010x}")
    print(f"  crc (recomputed) : {actual_crc:#010x}  ({'OK' if crc_ok else 'CORRUPT'})")
    print(f"  crc (expected)   : {expected_crc:#010x}  ({'MATCH' if match else 'MISMATCH — different build!'})")
    print()

    if not crc_ok:
        print("RESULT: FAIL — flash contents corrupt (CRC mismatch with recomputed)")
        sys.exit(4)
    if not match:
        print("RESULT: FAIL — flash has DIFFERENT build from expected (CRC mismatch with fw_info.txt)")
        sys.exit(5)

    print("RESULT: PASS — device has exactly the expected firmware build.")


if __name__ == '__main__':
    main()

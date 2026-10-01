#!/usr/bin/env python3
"""
fw_info_extract.py — post-build helper: finds fw_info symbol address in ELF via nm,
writes fw_info.txt with address and build metadata.

Usage:
    python3 fw_info_extract.py <nm_tool> <elf_path> <output_txt>
                               <crc> <git_hash> <build_time>
                               <version> <builder_id>
"""
import subprocess, re, sys, os

def main():
    if len(sys.argv) != 9:
        print(f"Usage: {sys.argv[0]} <nm> <elf> <out_txt> <crc> <hash> <time> <ver> <builder>",
              file=sys.stderr)
        sys.exit(1)

    nm_tool, elf_path, out_txt, crc, git_hash, build_time, version, builder_id = sys.argv[1:]

    if not os.path.exists(elf_path):
        print(f"fw_info_extract: ELF not found: {elf_path}", file=sys.stderr)
        sys.exit(0)

    nm = subprocess.run([nm_tool, elf_path], capture_output=True, text=True)
    m = re.search(r'^([0-9a-fA-F]+) . fw_info$', nm.stdout, re.MULTILINE)
    if not m:
        print('fw_info symbol not found in ELF — section may be stripped', file=sys.stderr)
        sys.exit(0)

    addr = int(m.group(1), 16)
    info = (
        f'fw_info_addr=0x{addr:08X}\n'
        f'crc=0x{crc}\n'
        f'git_hash=0x{git_hash}\n'
        f'build_time={build_time}\n'
        f'version={version}\n'
        f'builder_id={builder_id}\n'
    )
    with open(out_txt, 'w') as f:
        f.write(info)
    print(f'[fw_info] addr=0x{addr:08X}  crc=0x{crc}  ver={version}  hash={git_hash}')

if __name__ == '__main__':
    main()

#pragma once
#include <stdint.h>

#define FW_INFO_MAGIC       0x46574946U   /* ASCII 'FWIF', little-endian */
#define FW_INFO_STRUCT_SIZE 32U

/*
 * Firmware identification block — embedded in flash at link time.
 * Layout (28 bytes, no padding needed, all naturally aligned):
 *
 * offset  size  field
 * 0x00     4    magic        = FW_INFO_MAGIC
 * 0x04     4    crc32        CRC32 of all 28 bytes with this field = 0
 * 0x08     4    git_hash     lower 32 bits of HEAD commit SHA
 * 0x0C     4    build_time   Unix timestamp (UTC, seconds)
 * 0x10     1    ver_major
 * 0x11     1    ver_minor
 * 0x12     1    ver_patch
 * 0x13     1    ver_build    0 = not used
 * 0x14     2    builder_id   configurable via CMake -DFW_BUILDER_ID=N
 * 0x16     2    struct_size  = FW_INFO_STRUCT_SIZE (compatibility guard)
 * 0x18     8    reserved     = 0 (pad to 32 bytes)
 */
struct fw_info_t {
	uint32_t magic;
	uint32_t crc32;
	uint32_t git_hash;
	uint32_t build_time;
	uint8_t  ver_major;
	uint8_t  ver_minor;
	uint8_t  ver_patch;
	uint8_t  ver_build;
	uint16_t builder_id;
	uint16_t struct_size;
	uint32_t reserved[2];   /* 8 bytes pad → total 32 bytes */
};

extern const struct fw_info_t fw_info;

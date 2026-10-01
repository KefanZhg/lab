# =============================================================
# fw_info.cmake — embeds build identity block into firmware
#
# Usage in CMakeLists.txt:
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../common/fw_info/fw_info.cmake)
#
# Optional cache variable:
#   -DFW_BUILDER_ID=<uint16>   default 0
# =============================================================

set(_FW_INFO_DIR ${CMAKE_CURRENT_LIST_DIR})

# ── Builder ID ──────────────────────────────────────────────
set(FW_BUILDER_ID "0" CACHE STRING "Firmware builder ID (uint16, 0-65535)")

# ── Git hash (lower 32 bits of HEAD SHA) ────────────────────
find_package(Git QUIET)
set(FW_INFO_GIT_HASH "00000000")
set(FW_INFO_GIT_HASH_STR "unknown")

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _sha_full
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _sha_result
    )
    if(_sha_result EQUAL 0 AND _sha_full)
        string(SUBSTRING "${_sha_full}" 0 8 FW_INFO_GIT_HASH)
        set(FW_INFO_GIT_HASH_STR "${_sha_full}")
    endif()
endif()

# ── Version from git tag (vX.Y.Z or X.Y.Z) ──────────────────
set(FW_INFO_VER_MAJOR 0)
set(FW_INFO_VER_MINOR 0)
set(FW_INFO_VER_PATCH 0)
set(FW_INFO_VER_BUILD 0)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _git_tag
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _tag_result
    )
    if(_tag_result EQUAL 0 AND _git_tag MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(FW_INFO_VER_MAJOR ${CMAKE_MATCH_1})
        set(FW_INFO_VER_MINOR ${CMAKE_MATCH_2})
        set(FW_INFO_VER_PATCH ${CMAKE_MATCH_3})
        if(_git_tag MATCHES "\\.([0-9]+)$" AND _git_tag MATCHES "^v?[0-9]+\\.[0-9]+\\.[0-9]+\\.")
            set(FW_INFO_VER_BUILD ${CMAKE_MATCH_1})
        endif()
        message(STATUS "fw_info: version from tag ${_git_tag} -> "
            "${FW_INFO_VER_MAJOR}.${FW_INFO_VER_MINOR}.${FW_INFO_VER_PATCH}.${FW_INFO_VER_BUILD}")
    else()
        message(STATUS "fw_info: no exact git tag on HEAD, version = 0.0.0.0")
    endif()
endif()

# ── Build timestamp (Unix seconds UTC) ──────────────────────
string(TIMESTAMP FW_INFO_BUILD_TIME "%s" UTC)
# CMake < 3.14 or some platforms return literal "%s"; fall back to YYYYMMDD
if(NOT FW_INFO_BUILD_TIME MATCHES "^[0-9]+$")
    string(TIMESTAMP FW_INFO_BUILD_TIME "%Y%m%d" UTC)
endif()

# ── Find Python for CRC calculation ─────────────────────────
# Try toolchain python first, then system python3
set(_py_candidates
    "$ENV{TOOLCHAIN}/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/python3.12"
    "/opt/nordic/ncs/toolchains/0c0f19d91c/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/python3.12"
    "/opt/nordic/ncs/toolchains/0c0f19d91c/opt/python@3.12/bin/python3.12"
)
set(_fw_python "")
foreach(_py ${_py_candidates})
    if(EXISTS "${_py}")
        set(_fw_python "${_py}")
        break()
    endif()
endforeach()
if(NOT _fw_python)
    find_program(_fw_python NAMES python3 python)
endif()

# ── Compute CRC32 (struct packed, crc field = 0) ────────────
#   Format: <IIIIBBBBHHII  (little-endian, 32 bytes)
#     I magic, I crc(=0), I git_hash, I build_time,
#     B major, B minor, B patch, B build,
#     H builder_id, H struct_size, I reserved[0], I reserved[1]
set(FW_INFO_CRC "00000000")

if(_fw_python)
    execute_process(
        COMMAND ${_fw_python} -c
"import struct, zlib
d = struct.pack('<IIIIBBBBHHII',
    0x46574946, 0,
    int('${FW_INFO_GIT_HASH}', 16),
    int('${FW_INFO_BUILD_TIME}'),
    ${FW_INFO_VER_MAJOR}, ${FW_INFO_VER_MINOR},
    ${FW_INFO_VER_PATCH}, ${FW_INFO_VER_BUILD},
    ${FW_BUILDER_ID}, 32, 0, 0)
print(format(zlib.crc32(d) & 0xFFFFFFFF, '08X'))"
        OUTPUT_VARIABLE FW_INFO_CRC
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _crc_result
        ERROR_VARIABLE _crc_err
    )
    if(NOT _crc_result EQUAL 0)
        message(WARNING "fw_info: CRC computation failed: ${_crc_err}, using 00000000")
        set(FW_INFO_CRC "00000000")
    endif()
else()
    message(WARNING "fw_info: Python not found, CRC will be 00000000")
endif()

message(STATUS "fw_info: hash=${FW_INFO_GIT_HASH} time=${FW_INFO_BUILD_TIME} "
    "ver=${FW_INFO_VER_MAJOR}.${FW_INFO_VER_MINOR}.${FW_INFO_VER_PATCH}.${FW_INFO_VER_BUILD} "
    "builder=${FW_BUILDER_ID} crc=0x${FW_INFO_CRC}")

# ── Generate fw_info.c ───────────────────────────────────────
configure_file(
    ${_FW_INFO_DIR}/fw_info.c.in
    ${CMAKE_CURRENT_BINARY_DIR}/fw_info_generated.c
    @ONLY
)

target_sources(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/fw_info_generated.c)
target_include_directories(app PRIVATE ${_FW_INFO_DIR})

# Prevent --gc-sections from discarding the fw_info block
zephyr_linker_sources(RODATA ${_FW_INFO_DIR}/fw_info.ld)

# ── Post-build: extract fw_info address and write info file ──
set(_nm_tool ${CMAKE_NM})
if(NOT _nm_tool)
    find_program(_nm_tool NAMES arm-zephyr-eabi-nm
        PATHS "$ENV{ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin"
              "/opt/nordic/ncs/toolchains/0c0f19d91c/opt/zephyr-sdk/arm-zephyr-eabi/bin"
    )
endif()

if(_nm_tool)
    # zephyr_final lives in a different CMake directory so we can't use TARGET.
    # Instead, declare fw_info.txt as an output that depends on zephyr.elf —
    # cmake's file-level dependency tracking ensures the ELF is linked first.
    set(_elf_path "${CMAKE_BINARY_DIR}/zephyr/zephyr.elf")
    add_custom_command(
        OUTPUT  ${CMAKE_BINARY_DIR}/fw_info.txt
        COMMAND ${_fw_python}
            ${_FW_INFO_DIR}/fw_info_extract.py
            ${_nm_tool}
            ${_elf_path}
            ${CMAKE_BINARY_DIR}/fw_info.txt
            ${FW_INFO_CRC}
            ${FW_INFO_GIT_HASH}
            ${FW_INFO_BUILD_TIME}
            ${FW_INFO_VER_MAJOR}.${FW_INFO_VER_MINOR}.${FW_INFO_VER_PATCH}.${FW_INFO_VER_BUILD}
            ${FW_BUILDER_ID}
        DEPENDS ${_elf_path}
        COMMENT "Extracting fw_info block address"
        VERBATIM
    )
    add_custom_target(fw_info_post_build ALL
        DEPENDS ${CMAKE_BINARY_DIR}/fw_info.txt
    )
endif()

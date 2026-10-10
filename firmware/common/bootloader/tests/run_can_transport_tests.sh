#!/usr/bin/env bash
set -euo pipefail

bootloader_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -I"${bootloader_dir}" \
  -DFW_UPDATE_CAN_TX_FRAME_SIZE=FW_UPDATE_CAN_MAX_FRAME_SIZE \
  "${bootloader_dir}/tests/test_fw_update_can_transport.c" \
  "${bootloader_dir}/fw_update_protocol.c" \
  -o "${test_dir}/test_fw_update_can_transport"

"${test_dir}/test_fw_update_can_transport"

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -DFW_UPDATE_TARGET_ID=0x54454C45U \
  -DFW_UPDATE_FLASH_POINTER=TestEngineFlashPointer \
  -I"${bootloader_dir}/tests/include" \
  -I"${bootloader_dir}" \
  "${bootloader_dir}/tests/test_fw_update_bootloader_engine.c" \
  "${bootloader_dir}/fw_update_bootloader_engine.c" \
  "${bootloader_dir}/fw_update_protocol.c" \
  -o "${test_dir}/test_fw_update_bootloader_engine"

"${test_dir}/test_fw_update_bootloader_engine"


"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -I"${bootloader_dir}/tests/include" \
  -I"${bootloader_dir}" \
  "${bootloader_dir}/tests/test_bootloader_boot_request.c" \
  "${bootloader_dir}/bootloader_boot_request.c" \
  -o "${test_dir}/test_bootloader_boot_request"

"${test_dir}/test_bootloader_boot_request"

"${CC:-cc}" -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror \
  -DBOOTLOADER_APP_MAX_SIZE_BYTES=225280U \
  -DFW_UPDATE_TARGET_ID=0x54454C45U \
  -DFW_UPDATE_FLASH_POINTER=TestStoragePointer \
  -I"${bootloader_dir}/tests/storage_include" \
  -I"${bootloader_dir}" \
  "${bootloader_dir}/tests/test_bootloader_storage.c" \
  "${bootloader_dir}/bootloader_storage.c" \
  "${bootloader_dir}/fw_update_bootloader_engine.c" \
  "${bootloader_dir}/fw_update_protocol.c" \
  -o "${test_dir}/test_bootloader_storage"

"${test_dir}/test_bootloader_storage" "$@"

#!/usr/bin/env bash
set -euo pipefail

bootloader_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
monocypher_dir="${bootloader_dir}/third_party/monocypher"
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -I"${bootloader_dir}" \
  "${bootloader_dir}/tests/test_fw_update_can_transport.c" \
  "${bootloader_dir}/fw_update_can_transport.c" \
  "${bootloader_dir}/fw_update_protocol.c" \
  -o "${test_dir}/test_fw_update_can_transport"

"${test_dir}/test_fw_update_can_transport"

for bench in 0 1; do
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -DFW_UPDATE_TARGET_ID=0x54454C45U \
  -DFW_UPDATE_HARDWARE_REVISION=1U \
  -DDEBUG -DFW_UPDATE_ALLOW_UNSIGNED_BENCH=${bench} \
  -DFW_UPDATE_PUBLIC_KEY_CONFIGURED=$((1-bench)) \
  -I"${bootloader_dir}/tests/include" \
  -I"${bootloader_dir}" \
  -I"${monocypher_dir}" \
  "${bootloader_dir}/tests/test_fw_update_bootloader_engine.c" \
  "${bootloader_dir}/fw_update_bootloader_engine.c" \
  "${bootloader_dir}/fw_update_protocol.c" \
  "${bootloader_dir}/bootloader_sha256.c" \
  "${monocypher_dir}/monocypher.c" \
  "${monocypher_dir}/monocypher-ed25519.c" \
  -o "${test_dir}/test_fw_update_bootloader_engine"

"${test_dir}/test_fw_update_bootloader_engine"

done

"${CC:-cc}" -std=c11 -Wall -Wextra -Werror \
  -I"${bootloader_dir}/tests/include" \
  -I"${bootloader_dir}" \
  "${bootloader_dir}/tests/test_bootloader_boot_request.c" \
  "${bootloader_dir}/bootloader_boot_request.c" \
  -o "${test_dir}/test_bootloader_boot_request"

"${test_dir}/test_bootloader_boot_request"

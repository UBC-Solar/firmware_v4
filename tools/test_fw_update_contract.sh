#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bootloader_dir="${repo_root}/firmware/common/bootloader"
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
  -I"${bootloader_dir}" \
  "${bootloader_dir}/tests/test_fw_update_contract.c" \
  "${bootloader_dir}/fw_update_protocol.c" \
  -o "${test_dir}/test_fw_update_contract"

"${test_dir}/test_fw_update_contract"

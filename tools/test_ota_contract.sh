#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bootloader_dir="${repo_root}/firmware/common/bootloader"
monocypher_dir="${bootloader_dir}/third_party/monocypher"
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
  -I"${bootloader_dir}" \
  -I"${monocypher_dir}" \
  "${bootloader_dir}/tests/test_ota_contract.c" \
  "${bootloader_dir}/sunlite_ota_protocol.c" \
  "${bootloader_dir}/bootloader_sha256.c" \
  "${monocypher_dir}/monocypher.c" \
  "${monocypher_dir}/monocypher-ed25519.c" \
  -o "${test_dir}/test_ota_contract"

"${test_dir}/test_ota_contract"

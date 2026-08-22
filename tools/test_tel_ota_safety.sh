#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tel_app_dir="${repo_root}/firmware/components/tel/app"
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
  -I"${tel_app_dir}" \
  "${repo_root}/firmware/components/tel/tests/test_ota_safety.c" \
  "${tel_app_dir}/tel_ota_safety.c" \
  -o "${test_dir}/test_tel_ota_safety"

"${test_dir}/test_tel_ota_safety"

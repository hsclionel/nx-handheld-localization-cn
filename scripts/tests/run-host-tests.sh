#!/bin/bash
# Host-only test run: no device, no adb, no cross toolchain. Used by .github/workflows/ci.yaml; run it locally the same way:
#   bash scripts/tests/run-host-tests.sh
# 1. workspace/all/common/tests/run_tests.sh (C unit tests, including test_i18n against the shipped zh_CN.lang)
# 2. the scripts/tests/*.sh listed in HOST_SHELL_TESTS below
#
# HOST_SHELL_TESTS is an explicit list of the shell tests that pass on a plain Linux host today. Not listed:
# - device/adb tests: artfetch-cli, deep-sleep-e2e, music-service-e2e, n64-core-device, rumble-switch-e2e,
#   wifi-connect-e2e
# - fail to build under gcc 14 -Werror (format-truncation in their own cc lines): archive-cache, art-style,
#   media-tools-unit, rewind-budget, scraper-variants
# - button-layout-sh: sources the helper under `sh -c 'set -o pipefail'` like the device's busybox; Ubuntu
#   24.04's dash has no pipefail (Debian 13's does), so it fails on the CI runner only
# - fail on a host for other reasons, to be looked at separately: boot-update-zip-check, cheatdb-data,
#   dc-netplay-sh, extras-gen1recomp-install, extras-portmaster-catalogs, extras-portmaster-install,
#   menu-animations, patch-osdd-layout, portmaster-launch
# Add a test here once it passes on a host.
set -u
cd "$(dirname "$0")/../.."

HOST_SHELL_TESTS="
test-apply-patches.sh
test-artfetch-sh.sh
test-audiomon-bt-disconnect.sh
test-audiomon-spk-guard.sh
test-cheat-match.sh
test-cheat-mem.sh
test-cpu-policy.sh
test-extras-cheatdb-install.sh
test-gamelistart.sh
test-gba-classic-nes.sh
test-gpu-shader-boost.sh
test-minarch-bench.sh
test-minarch-cpu-profile.sh
test-n64-pak-sh.sh
test-netplay-paks.sh
test-netplay-prelaunch.sh
test-osdmusic-launch.sh
test-osd-overlay-tg5050.sh
test-osd-temperature.sh
test-portmaster-compat.sh
test-rgba-colors.sh
test-search-hint.sh
test-sync-rsync-fat32.sh
test-tls-verify.sh
test-turbo-shaper.sh
test-update-btmgr-cleanup.sh
test-update-n64-standalone-cleanup.sh
test-update-portmaster-refresh.sh
test-update-psp-standalone-cleanup.sh
"

failed=""
run() {
	name=$1
	shift
	echo "::group::$name"
	if "$@"; then
		echo "::endgroup::"
		echo "ok   $name"
	else
		echo "::endgroup::"
		echo "FAIL $name"
		failed="$failed $name"
	fi
}

run run_tests.sh sh workspace/all/common/tests/run_tests.sh
# zh_CN.lang vs the sources: fails on stale entries; untranslated candidates are listed, never fail
run i18n_dict_check python3 scripts/tests/i18n_dict_check.py --untranslated
for t in $HOST_SHELL_TESTS; do
	run "$t" bash "scripts/tests/$t"
done

if [ -n "$failed" ]; then
	echo "Failed:$failed"
	exit 1
fi
echo "All host tests passed."

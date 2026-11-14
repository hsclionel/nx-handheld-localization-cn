#!/usr/bin/env bash
# Host tests for the PSP Ad-Hoc shell side: nx_psp_netplay.sh (sourced by
# PSP.pak/launch.sh) and the netplay markers. A fake netplay.elf on PATH
# stands in for the wizard. Busybox-compatible constructs only in the script
# under test; the fake wizard and the assertions here may use bash.
set -euo pipefail
cd "$(dirname "$0")/../.."
ROOT="$PWD"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
FAIL=0
fail() { echo "FAIL: $*" >&2; FAIL=1; }

PSP40="skeleton/SYSTEM/tg5040/paks/Emus/PSP.pak"
PSP50="skeleton/SYSTEM/tg5050/paks/Emus/PSP.pak"

# --- syntax + platform parity ---------------------------------------------
sh -n "$PSP40/nx_psp_netplay.sh" || fail "nx_psp_netplay.sh fails sh -n"
sh -n "$PSP50/nx_psp_netplay.sh" || fail "tg5050 nx_psp_netplay.sh fails sh -n"
sh -n "$PSP40/launch.sh" || fail "tg5040 PSP launch.sh fails sh -n"
sh -n "$PSP50/launch.sh" || fail "tg5050 PSP launch.sh fails sh -n"
cmp -s "$PSP40/nx_psp_netplay.sh" "$PSP50/nx_psp_netplay.sh" \
	|| fail "tg5040/tg5050 nx_psp_netplay.sh differ"
[ -f "$PSP40/netplay" ] || fail "tg5040 PSP.pak has no netplay marker"
[ -f "$PSP50/netplay" ] || fail "tg5050 PSP.pak has no netplay marker"
grep -q 'nx_psp_netplay\.sh' "$PSP40/launch.sh" || fail "tg5040 launch.sh does not source nx_psp_netplay.sh"
grep -q 'nx_psp_netplay\.sh' "$PSP50/launch.sh" || fail "tg5050 launch.sh does not source nx_psp_netplay.sh"

# --- fake netplay.elf -------------------------------------------------------
mkdir -p "$TMP/bin" "$TMP/logs"
cat > "$TMP/bin/netplay.elf" <<'EOF'
#!/bin/sh
# NX_FAKE_WIZARD: host | openhost | client | badpeer | fail. --cleanup only
# leaves a mark (NX_FAKE_CLEANUP_MARK) so a test can see it ran; the wizard
# arguments go to NX_FAKE_ARGS_FILE, one per line.
if [ "$1" = "--cleanup" ]; then
	[ -n "${NX_FAKE_CLEANUP_MARK:-}" ] && touch "$NX_FAKE_CLEANUP_MARK"
	exit 0
fi
[ -n "${NX_FAKE_ARGS_FILE:-}" ] && printf '%s\n' "$@" > "$NX_FAKE_ARGS_FILE"
SESSION=""
while [ $# -gt 0 ]; do
	case "$1" in
		--session-file) SESSION="$2"; shift ;;
	esac
	shift
done
case "${NX_FAKE_WIZARD:-host}" in
	host) printf 'NETPLAY_ROLE=host\nNETPLAY_PEER_IP=192.168.0.7\nNETPLAY_MODE=wifi\nNETPLAY_PLAYER=1\nNETPLAY_NUM_PLAYERS=2\n' > "$SESSION" ;;
	openhost) printf 'NETPLAY_ROLE=host\nNETPLAY_PEER_IP=\nNETPLAY_MODE=wifi\nNETPLAY_PLAYER=1\nNETPLAY_NUM_PLAYERS=2\n' > "$SESSION" ;;
	client) printf 'NETPLAY_ROLE=client\nNETPLAY_PEER_IP=192.168.0.5\nNETPLAY_MODE=wifi\nNETPLAY_PLAYER=2\nNETPLAY_NUM_PLAYERS=2\n' > "$SESSION" ;;
	badpeer) printf 'NETPLAY_ROLE=client\nNETPLAY_PEER_IP=999.1.1.1\nNETPLAY_MODE=wifi\n' > "$SESSION" ;;
	fail) exit 1 ;;
esac
exit 0
EOF
chmod +x "$TMP/bin/netplay.elf"
export PATH="$TMP/bin:$PATH"

# run_case <name> <wizard> [extra env...]: sources the script, runs
# nx_psp_netplay, then prints the session exports (or nothing, when the
# script exits the shell first, as on wizard failure). Prints to stdout.
run_case() {
	name="$1"; wizard="$2"; shift 2
	FLAG="$TMP/$name.flag"; SESSION_F="$TMP/$name.session"
	touch "$FLAG"
	env -i PATH="$PATH" HOME="$TMP" ROM="$TMP/Tag Force.iso" LOGS_PATH="$TMP/logs" \
		NETPLAY_LAUNCH_FLAG="$FLAG" NETPLAY_SESSION_FILE="$SESSION_F" \
		NX_FAKE_WIZARD="$wizard" "$@" \
		sh -c '. "$1"; nx_psp_netplay; echo "REACHED"; echo "CORE_NETPLAY=${NX_CORE_NETPLAY:-}"; echo "GGPO_SERVER=${NX_GGPO_SERVER:-}"; echo "SESSION_OPTIONS=${NX_SESSION_OPTIONS:-}"; echo "ROLE_LEFT=${NETPLAY_ROLE:-}"' \
		_ "$ROOT/$PSP40/nx_psp_netplay.sh"
}

# --- helpers ---------------------------------------------------------------
out=$(sh -c '. "$1"; echo "$(nx_psp_ip_digits 192.168.0.5)"' _ "$ROOT/$PSP40/nx_psp_netplay.sh")
[ "$out" = "192168000005" ] || fail "ip_digits 192.168.0.5: got '$out'"
out=$(sh -c '. "$1"; echo "$(nx_psp_ip_digits 10.0.0.1)"' _ "$ROOT/$PSP40/nx_psp_netplay.sh")
[ "$out" = "010000000001" ] || fail "ip_digits 10.0.0.1: got '$out'"
for bad in 999.1.1.1 1.2.3 1.2.3.4.5 notanip 192.168.0.256 ""; do
	if sh -c '. "$1"; nx_psp_ip_digits "$2" >/dev/null' _ "$ROOT/$PSP40/nx_psp_netplay.sh" "$bad"; then
		fail "ip_digits accepted '$bad'"
	fi
done

# --- plain launch: no flag, nothing exported -------------------------------
out=$(env -i PATH="$PATH" HOME="$TMP" ROM="$TMP/Tag Force.iso" LOGS_PATH="$TMP/logs" \
	sh -c '. "$1"; nx_psp_netplay; echo "CORE_NETPLAY=${NX_CORE_NETPLAY:-unset}"; echo "SESSION_OPTIONS=${NX_SESSION_OPTIONS:-unset}"' \
	_ "$ROOT/$PSP40/nx_psp_netplay.sh")
echo "$out" | grep -q 'CORE_NETPLAY=unset' || fail "plain launch exported NX_CORE_NETPLAY: $out"
echo "$out" | grep -q 'SESSION_OPTIONS=unset' || fail "plain launch exported NX_SESSION_OPTIONS: $out"

# --- host -------------------------------------------------------------------
out=$(run_case host host NX_PSP_NETPLAY_MAC_HEX=0123456789ab NX_PSP_NETPLAY_OWN_IP=192.168.0.2 \
	NX_FAKE_ARGS_FILE="$TMP/host.args")
echo "$out" | grep -q '^REACHED$' || fail "host case did not reach the core launch: $out"
echo "$out" | grep -q '^CORE_NETPLAY=1$' || fail "host: NX_CORE_NETPLAY != 1: $out"
echo "$out" | grep -q '^GGPO_SERVER=192.168.0.7$' || fail "host: GGPO_SERVER wrong: $out"
echo "$out" | grep -q '^ROLE_LEFT=$' || fail "host: NETPLAY_ROLE not unset: $out"
opts=$(echo "$out" | sed -n 's/^SESSION_OPTIONS=//p')
for kv in ppsspp_enable_wlan=enabled ppsspp_enable_builtin_pro_ad_hoc_server=enabled \
		ppsspp_change_pro_ad_hoc_server_address="IP address" ppsspp_port_offset=10000 \
		ppsspp_pro_ad_hoc_server_address01=1 ppsspp_pro_ad_hoc_server_address03=2 \
		ppsspp_pro_ad_hoc_server_address06=8 ppsspp_pro_ad_hoc_server_address09=0 \
		ppsspp_pro_ad_hoc_server_address12=2 \
		ppsspp_change_mac_address01=0 ppsspp_change_mac_address02=0 \
		ppsspp_change_mac_address11=a ppsspp_change_mac_address12=b; do
	# note: override MAC 0123456789ab has its second nibble masked 1 -> 0
	case ";$opts;" in *";$kv;"*) ;; *) fail "host: missing $kv in '$opts'" ;; esac
done
# never localhost: PPSSPP would register the host as 127.0.0.1 (device test 2026-11-14)
case "$opts" in *"=localhost"*) fail "host: server address must not be localhost: '$opts'" ;; esac
grep -qx -- '--open-host' "$TMP/host.args" || fail "wizard not offered --open-host: $(cat "$TMP/host.args")"

# --- host for PC/phone (--open-host): no peer, server = own IP --------------
out=$(run_case openhost openhost NX_PSP_NETPLAY_OWN_IP=10.0.0.12)
echo "$out" | grep -q '^REACHED$' || fail "open host did not reach the core launch: $out"
echo "$out" | grep -q '^CORE_NETPLAY=1$' || fail "open host: NX_CORE_NETPLAY != 1: $out"
echo "$out" | grep -q '^GGPO_SERVER=$' || fail "open host: GGPO_SERVER should be empty: $out"
opts=$(echo "$out" | sed -n 's/^SESSION_OPTIONS=//p')
for kv in ppsspp_enable_builtin_pro_ad_hoc_server=enabled ppsspp_change_pro_ad_hoc_server_address="IP address" \
		ppsspp_pro_ad_hoc_server_address01=0 ppsspp_pro_ad_hoc_server_address02=1 \
		ppsspp_pro_ad_hoc_server_address03=0 ppsspp_pro_ad_hoc_server_address10=0 \
		ppsspp_pro_ad_hoc_server_address11=1 ppsspp_pro_ad_hoc_server_address12=2; do
	case ";$opts;" in *";$kv;"*) ;; *) fail "open host: missing $kv in '$opts'" ;; esac
done

# --- host with no usable LAN address: back to the list, WiFi restored ------
rm -f "$TMP/noip.cleanup"
out=$(run_case noip host NX_PSP_NETPLAY_OWN_IP=not.an.ip NX_FAKE_CLEANUP_MARK="$TMP/noip.cleanup"); rc=$?
echo "$out" | grep -q '^REACHED$' && fail "host with no LAN address still reached the core launch"
[ "$rc" = 0 ] || fail "host with no LAN address: exit code $rc, expected 0"
[ -f "$TMP/noip.cleanup" ] || fail "host with no LAN address: netplay.elf --cleanup not run"

# --- join -------------------------------------------------------------------
out=$(run_case client client NX_PSP_NETPLAY_MAC_HEX=0123456789ab)
echo "$out" | grep -q '^REACHED$' || fail "join case did not reach the core launch: $out"
echo "$out" | grep -q '^CORE_NETPLAY=1$' || fail "join: NX_CORE_NETPLAY != 1: $out"
echo "$out" | grep -q '^GGPO_SERVER=192.168.0.5$' || fail "join: GGPO_SERVER wrong: $out"
opts=$(echo "$out" | sed -n 's/^SESSION_OPTIONS=//p')
for kv in ppsspp_enable_wlan=enabled ppsspp_enable_builtin_pro_ad_hoc_server=disabled \
		ppsspp_change_pro_ad_hoc_server_address="IP address" ppsspp_port_offset=10000 \
		ppsspp_pro_ad_hoc_server_address01=1 ppsspp_pro_ad_hoc_server_address02=9 \
		ppsspp_pro_ad_hoc_server_address03=2 ppsspp_pro_ad_hoc_server_address04=1 \
		ppsspp_pro_ad_hoc_server_address05=6 ppsspp_pro_ad_hoc_server_address06=8 \
		ppsspp_pro_ad_hoc_server_address07=0 ppsspp_pro_ad_hoc_server_address08=0 \
		ppsspp_pro_ad_hoc_server_address09=0 ppsspp_pro_ad_hoc_server_address10=0 \
		ppsspp_pro_ad_hoc_server_address11=0 ppsspp_pro_ad_hoc_server_address12=5; do
	case ";$opts;" in *";$kv;"*) ;; *) fail "join: missing $kv in '$opts'" ;; esac
done

# --- MAC first-byte mask: ffffff... -> second nibble c ----------------------
out=$(run_case mask host NX_PSP_NETPLAY_MAC_HEX=ffffffffffff)
opts=$(echo "$out" | sed -n 's/^SESSION_OPTIONS=//p')
case ";$opts;" in
	*";ppsspp_change_mac_address01=f;"*) ;;
	*) fail "MAC mask: expected 01=f in '$opts'" ;;
esac
case ";$opts;" in
	*";ppsspp_change_mac_address02=c;"*) ;;
	*) fail "MAC mask: expected 02=c in '$opts'" ;;
esac

# --- random MAC (no override): 12 nibbles, second nibble 0/4/8/c -----------
out=$(run_case rand host)
opts=$(echo "$out" | sed -n 's/^SESSION_OPTIONS=//p')
n=$(printf '%s' "$opts" | tr ';' '\n' | grep -c '^ppsspp_change_mac_address[0-9][0-9]=[0-9a-f]$' || true)
[ "$n" = 12 ] || fail "random MAC: expected 12 nibble options, got $n in '$opts'"
n2=$(printf '%s' "$opts" | tr ';' '\n' | sed -n 's/^ppsspp_change_mac_address02=//p')
case "$n2" in 0|4|8|c) ;; *) fail "random MAC: nibble 02 '$n2' not masked" ;; esac

# --- wizard failure / bad peer IP: exit before the core launch --------------
out=$(run_case wfail fail); rc=$?
echo "$out" | grep -q '^REACHED$' && fail "wizard failure still reached the core launch"
[ "$rc" = 0 ] || fail "wizard failure exit code $rc, expected 0"
out=$(run_case badpeer badpeer); rc=$?
echo "$out" | grep -q '^REACHED$' && fail "bad peer IP still reached the core launch"
[ "$rc" = 0 ] || fail "bad peer IP exit code $rc, expected 0"

[ "$FAIL" = 0 ] || exit 1
echo "test-psp-adhoc-sh ok"

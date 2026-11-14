# Sourced by PSP.pak's launch.sh. PSP Ad-Hoc multiplayer is run inside the
# PPSSPP core itself (its built-in PRO Ad Hoc Server for the host, the host's
# IP for the joiner), so this follows the Dreamcast / flycast GGPO precedent
# (nx_dc_netplay.sh), not the generic lockstep path: the pre-launch wizard
# (netplay.elf) finds the other player, then this script hands the session to
# the core as PPSSPP core options and tells minarch with NX_CORE_NETPLAY=1.
# NETPLAY_ROLE is unset before the core starts: the generic netplay-prelaunch
# path would fail loudly (ppsspp has no link support in netplay_boot.c).
# A cancelled or failed wizard exits the sourcing launch.sh (back to the game
# list), never falling through to a one-player launch. The flag and session
# paths are overridable for tests only.
# Saves are NOT synced: each player keeps their own Saves/PSP/SAVEDATA/
# (NETPLAY_SAVES_DIR is never set, so SavePaths_netplayMode is NONE).
nx_psp_netplay() {
	NP_FLAG="${NETPLAY_LAUNCH_FLAG:-/tmp/netplay_launch}"
	[ -f "$NP_FLAG" ] || return 0
	rm -f "$NP_FLAG"
	NP_SESSION="${NETPLAY_SESSION_FILE:-/tmp/netplay_session}"

	NP_ROMFILE=$(basename "$ROM")
	NP_STEM="${NP_ROMFILE%.*}"

	netplay.elf --game "$NP_STEM" --session-file "$NP_SESSION" \
		> "$LOGS_PATH/netplay-wizard.txt" 2>&1 || { nx_psp_netplay_cleanup; exit 0; }
	[ -f "$NP_SESSION" ] || { nx_psp_netplay_cleanup; exit 0; }
	. "$NP_SESSION"
	NP_ROLE="$NETPLAY_ROLE"
	NP_PEER="$NETPLAY_PEER_IP"
	unset NETPLAY_ROLE NETPLAY_PEER_IP NETPLAY_MODE NETPLAY_PLAYER NETPLAY_NUM_PLAYERS NETPLAY_PEER_CAPS

	# A unique MAC per device: the core generates a random one when all
	# twelve nibbles are 0, but it seeds from the wall clock, so two
	# handhelds starting in the same second would collide. Generate one
	# here (NX_PSP_NETPLAY_MAC_HEX overrides it, for tests only).
	NP_MAC_HEX="${NX_PSP_NETPLAY_MAC_HEX:-}"
	if [ ${#NP_MAC_HEX} -ne 12 ]; then
		NP_MAC_HEX=$(od -An -tx1 -N6 /dev/urandom 2>/dev/null | tr -d ' \n')
	fi
	if [ ${#NP_MAC_HEX} -ne 12 ]; then
		NP_MAC_HEX=$(printf '%s%s' "$$" "$(date +%s 2>/dev/null)" | md5sum 2>/dev/null | cut -c1-12)
	fi
	case "$NP_MAC_HEX" in
		*[!0-9a-fA-F]*) NP_MAC_HEX="" ;;
	esac
	if [ ${#NP_MAC_HEX} -eq 12 ]; then
		# First byte with its low two bits clear, as the core does
		# (CreateRandMAC masks with 0xfc): the second nibble keeps only 0/4/8/c.
		NP_N2=$(printf '%s' "$NP_MAC_HEX" | cut -c2)
		case "$NP_N2" in
			0|1|2|3) NP_N2=0 ;;
			4|5|6|7) NP_N2=4 ;;
			8|9|a|b|A|B) NP_N2=8 ;;
			*) NP_N2=c ;;
		esac
		NP_MAC_HEX="$(printf '%s' "$NP_MAC_HEX" | cut -c1)$NP_N2$(printf '%s' "$NP_MAC_HEX" | cut -c3-12)"
	fi

	# The session's PPSSPP core options (see checkpoints/phase1-options.md):
	# WLAN on and Port Offset 10000 on both sides; the host also runs the
	# built-in PRO Ad Hoc Server and points its own game at it via
	# localhost, while the joiner points at the host's IP, digit by digit.
	NP_OPTS="ppsspp_enable_wlan=enabled;ppsspp_port_offset=10000"
	if [ "$NP_ROLE" = host ]; then
		NP_OPTS="$NP_OPTS;ppsspp_enable_builtin_pro_ad_hoc_server=enabled;ppsspp_change_pro_ad_hoc_server_address=localhost"
	else
		NP_IP_DIGITS=$(nx_psp_ip_digits "$NP_PEER") || { nx_psp_netplay_cleanup; exit 0; }
		NP_OPTS="$NP_OPTS;ppsspp_enable_builtin_pro_ad_hoc_server=disabled;ppsspp_change_pro_ad_hoc_server_address=IP address"
		NP_I=1
		while [ "$NP_I" -le 12 ]; do
			NP_OPTS="$NP_OPTS;ppsspp_pro_ad_hoc_server_address$(nx_psp_pad2 "$NP_I")=$(printf '%s' "$NP_IP_DIGITS" | cut -c"$NP_I")"
			NP_I=$((NP_I + 1))
		done
	fi
	if [ ${#NP_MAC_HEX} -eq 12 ]; then
		NP_I=1
		while [ "$NP_I" -le 12 ]; do
			NP_OPTS="$NP_OPTS;ppsspp_change_mac_address$(nx_psp_pad2 "$NP_I")=$(printf '%s' "$NP_MAC_HEX" | cut -c"$NP_I")"
			NP_I=$((NP_I + 1))
		done
	fi

	export NX_CORE_NETPLAY=1 NX_GGPO_SERVER="$NP_PEER" NX_SESSION_OPTIONS="$NP_OPTS"
}

# The host IP as the twelve digits the core's server-address options take:
# each octet zero-padded to three digits ("192.168.0.5" -> "192168000005").
# Prints the digits; returns non-zero for anything that is not a dotted quad.
nx_psp_ip_digits() {
	NP_IP="$1"
	case "$NP_IP" in
		*.*.*.*) ;;
		*) return 1 ;;
	esac
	case "$NP_IP" in
		*[!0-9.]*) return 1 ;;
	esac
	NP_OUT=""
	NP_N=1
	while [ "$NP_N" -le 4 ]; do
		NP_OCT=$(printf '%s' "$NP_IP" | cut -d. -f"$NP_N")
		case "$NP_OCT" in
			""|*[!0-9]*|????*) return 1 ;;
		esac
		[ "$NP_OCT" -gt 255 ] 2>/dev/null && return 1
		case "$NP_OCT" in
			[0-9]) NP_OCT="00$NP_OCT" ;;
			[0-9][0-9]) NP_OCT="0$NP_OCT" ;;
		esac
		NP_OUT="$NP_OUT$NP_OCT"
		NP_N=$((NP_N + 1))
	done
	[ -n "$(printf '%s' "$NP_IP" | cut -d. -f5)" ] && return 1
	printf '%s' "$NP_OUT"
}

# 1 -> 01, ... 9 -> 09, 10 -> 10: the core option suffixes are two digits.
nx_psp_pad2() {
	case "$1" in
		[0-9]) printf '0%s' "$1" ;;
		*) printf '%s' "$1" ;;
	esac
}

# After the game (or a cancelled wizard): PSP stages nothing in /tmp, so
# there is nothing to remove here; netplay.elf --cleanup (from launch.sh)
# restores WiFi/hotspot from the session file. Kept for symmetry with DC.
nx_psp_netplay_cleanup() {
	:
}

# PSP Ad-Hoc — first device test (2026-11-14)

Branch `feature/psp-adhoc-netplay` (PR 4) at `a029fa56`, tg5040 build.
Result: **PC PPSSPP ↔ Brick Pro played a full Tag Force SP duel**, but only
after manual workarounds. The wizard path in PR 4 did not work for a PC peer.

## Setup

| Side | Details |
|---|---|
| Handheld | TrimUI Brick Pro, NX Redux v2.5.1 + CN build, 192.168.0.2 (Wi-Fi), PPSSPP core `2a1700a` |
| PC | Windows PPSSPP, 192.168.0.9, same Wi-Fi, nickname `leo` |
| Game | 游戏王 卡片力量SP [v1.00][PLAY汉化组] (`NPJH00142`), same ISO on both sides |

Deployed to the handheld (md5-checked, `.prev` backups kept):
`bin/minarch.elf` (`eaa605ae`), `PSP.pak/launch.sh`, `PSP.pak/nx_psp_netplay.sh`,
`PSP.pak/netplay` marker, `res/lang/zh_CN.lang`. No reboot needed.

## What happened

1. **Wizard Host (Y → Host)** sat on "Waiting for player…" forever. The
   wizard waits for a second NX handheld to finish its own handshake on
   TCP 55440; PC PPSSPP cannot, so the game never started and no Ad-Hoc
   server ran. → finding 2.
2. **Manual fallback**: plain launch, in-game Core Options set to WLAN on,
   built-in PRO Ad Hoc Server on, server `localhost`, port offset 10000,
   saved per game. Changed mid-game, so only the server address applied
   (PPSSPP reads port offset and the built-in server only at boot: socket on
   UDP 1, nothing on 27312). After a full relaunch the server listened on
   27312 and the PC connected.
3. **PC joined the server but could only create its own group.** With
   `localhost` the handheld registered at the server as `127.0.0.1` and bound
   its game socket to `127.0.0.1:10001`, so the PC was told to reach the host
   at its own loopback. → finding 1.
4. **Fix by hand**: the per-game cfg was edited over SSH to
   `ppsspp_change_pro_ad_hoc_server_address = IP address` with digits
   `192168000002` (backup: `….iso-brickpro.cfg.bak-adhoc`). The digit options
   are not shown in minarch's menu. → finding 4.
5. **Relaunch → success.** Server log: handheld registered as
   `192.168.0.2`, PC (`192.168.0.9`) joined group `YuGiTF6`. The first PTP
   connect attempt did not hold (PC left the group); the game then created
   group `GCADHGD`, the PC joined it, and the duel was played to the end.
   The game saved normally to the handheld's own
   `Saves/PSP/SAVEDATA/NPJH001420001/` (22:11:09); nothing was synced.
6. **On quit minarch aborted**: `terminate called without an active exception`
   / `Aborted`, after the core had shut down the Ad-Hoc server and joysticks.
   The earlier run with no peer connected exited cleanly. → finding 3.

## Findings and improvements

### Must fix before PR 4 can work

1. **Host must advertise its own LAN IP, not `localhost`.**
   `nx_psp_netplay.sh` sets `ppsspp_change_pro_ad_hoc_server_address=localhost`
   for the host. Verified on device: this registers the host as 127.0.0.1 and
   binds its socket to loopback, so *no* remote peer (PC, phone or another
   handheld) can reach it. Use `IP address` plus the host's own IP digits —
   the wlan0 address on shared Wi-Fi, the hotspot address in hotspot mode.
2. **Hosting for a non-NX peer (PC/phone PPSSPP).** The wizard Host step
   blocks until another NX handheld joins. Add a "Host for PPSSPP (PC/phone)"
   choice that skips the peer wait, starts the game with the host options and
   shows the IP the other side must enter. A matching "Join by IP" (enter the
   PC host's IP) would cover the reverse case.
3. **minarch aborts on exit after an Ad-Hoc session.** Likely a PPSSPP
   `std::thread` (friend finder / matching / server) still joinable when the
   core is torn down. Needs a look at the core's `retro_deinit` / Ad-Hoc
   shutdown at `2a1700a`, and a check that minarch's own exit work (resume
   state, cfg) finishes before the abort.

### Should fix

4. **Ad-Hoc server IP digits hidden in the minarch menu.** The twelve
   `ppsspp_pro_ad_hoc_server_addressNN` options exist in the core but are not
   shown, so the manual fallback can't be done on the device. Either show them
   or offer one frontend "Ad-Hoc server IP" entry.
5. **Log spam**: 30,502 `sceKernelPollMbx(-1…): invalid mbx id` lines in one
   session (PSP.txt 2.2 MB, 32,680 lines) on the SD card. Lower PPSSPP's
   sceKernel log level for Ad-Hoc sessions or cap the log.
6. **Wizard screen is English on the zh_CN build** ("Other device must be on
   the same WiFi", "Waiting for player…"). netplay.elf strings need adding to
   `zh_CN.lang`.
7. **Options only apply at boot.** Changing Ad-Hoc options in-game half-applies
   them. The wizard path sets them before boot, so it is unaffected; for the
   manual path add a "restart the game to apply" note.

### Notes

- The game's own first matching attempt can fail; re-creating the room from
  the game menu worked. Worth a line in user-facing help.
- PPSSPP tries to fetch `adhoc-servers.json` over IPv6 and fails offline;
  harmless, it falls back to the bundled copy.
- Windows Firewall did not block PPSSPP on the Public Wi-Fi profile here.

## Still to test

From the PR 4 matrix, not yet run: two handhelds via the wizard (after
fixes 1–2), hotspot mode, phone PPSSPP, MENU → Leave Ad-Hoc (peer ends
immediately), sleep during a session, the session-option leak check
("Save changes" with no PSP cfg, then a plain launch), and other games
(MHFU, Tekken 6).

## Cleanup pending

The handheld's Tag Force SP per-game cfg still has the manual Ad-Hoc
settings (WLAN on, built-in server, server 192.168.0.2, port offset 10000).
Restore it from `….iso-brickpro.cfg.bak-adhoc` once testing is done, so plain
launches stop running an Ad-Hoc server.

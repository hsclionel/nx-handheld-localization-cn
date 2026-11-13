# Tech debt: PSP Ad-Hoc Co-op — design notes and remaining implementation

> Status (2026-11-13): design note only — **no code is implemented yet**.
> An automated implementation attempt was started the same day and stalled:
> the worker was interrupted during exploration and a resume did not restart
> it (no activity after the exploration step). No code was changed and no
> feature branch was created at the time. This note preserves the verified
> findings and the plan so the work can be picked up manually.
>
> Note: GitHub Issues are disabled in this repository, so this note is kept
> as a plan document instead of an issue.

## Goal

Add PSP Ad-Hoc multiplayer to NX Redux with the same press-Y → Host / Join
pre-launch wizard experience as existing Netplay, so users never have to dig
through PPSSPP core options.

- **Host:** enable WLAN + built-in PRO Ad Hoc Server, advertise the local IP
  via the existing discovery.
- **Join:** enable WLAN, set the Ad-Hoc Server IP to the host IP, match the
  Port Offset.
- Force the same Port Offset on both sides (default `10000`); generate /
  ensure unique MAC addresses where the core allows it.
- Apply the settings via the existing pre-launch options mechanism before
  launching the core.
- Phase 4 (public packet-relay servers) is explicitly deferred.

## Verified from the repo (main @ `62b5c81`)

### 1. The PPSSPP core is an external checkout, pinned by hash

- `workspace/tg5040/cores/Makefile:230-231` and
  `workspace/tg5050/cores/Makefile:230-231`:
  `ppsspp_REPO = https://github.com/hrydgard/ppsspp`,
  `ppsspp_HASH = d4e3c7eb2bf0ec9241621b3752cd3e5406178c78`.
- `workspace/all/cores/ppsspp/` holds only `build-libretro.sh` +
  `outline_atomics.c`; the core options list lives in the PPSSPP checkout,
  **not** in this repo.
- The core-option override mechanism itself is proven:
  `skeleton/SYSTEM/tg5040/paks/Emus/PSP.pak/default.cfg` already sets
  `ppsspp_*` keys, and `PSP.pak/options.sh` dumps the core option list via
  `minarch.elf --dump-options`.

### 2. Netplay capability is a marker file, not a table

- `workspace/all/nextui/gamelist.c:1110-1116` (`entryNetplayCapable`): a pak
  is netplay-capable iff it ships a `netplay` marker file beside its
  `launch.sh`. That one probe drives the Y NETPLAY hint (`gamelist.c:2558`,
  `home.c:1995`) and the Y-press launch (`gamelist.c:1572, 1812, 2161`).
- `skeleton/SYSTEM/tg5040/paks/Emus/PSP.pak/` and
  `.../tg5050/.../PSP.pak/` contain `launch.sh`, `options.sh`,
  `default.cfg` — and **no** `netplay` marker. 20 other paks ship the
  marker; PSP is not among them.
- So step 2.1 is: add an (empty) `netplay` marker to both PSP.pak dirs.
  But the marker alone is not sufficient — see §3.

### 3. The generic lockstep path does NOT fit PSP — use the Dreamcast / GGPO precedent

- `workspace/all/netplay/netplay.c:195-210` (`Netplay_checkCoreSupport`):
  the generic lockstep engine supports 10 cores only (fbneo, fceumm,
  snes9x, mednafen_supafaust, picodrive, genesis_plus_gx, pcsx_rearmed,
  swanstation, mupen64plus_next). `ppsspp` is not in it.
- `workspace/all/netplay/netplay_boot.c:77-83` (`NetplayBoot_startFromEnv`):
  if `NETPLAY_ROLE` is set but the core has no link support, it logs
  `"NETPLAY_ROLE=... but core ... has no link support"` and returns -1 —
  it fails loudly by design. Routing PSP through the generic
  `netplay-prelaunch.sh` path would hit this.
- Precedent that fits PSP: Dreamcast / flycast GGPO, documented in
  `workspace/all/netplay/core_netplay.h` — "a netplay session the core runs
  itself". The pak `nx_dc_netplay.sh` runs the wizard, then **unsets**
  `NETPLAY_ROLE` / `NETPLAY_PEER_IP` / `NETPLAY_MODE` and exports
  `NX_CORE_NETPLAY=1` + `NX_GGPO_*`; minarch runs no link engine, it only
  behaves like a netplay frontend (no save states / fast-forward / rewind /
  reset; menu offers "Leave netplay?").
- PSP Ad-Hoc is the same shape (the core runs the Ad-Hoc server/client
  itself). Implementation should be a per-pak `nx_psp_netplay.sh` modelled
  on `nx_dc_netplay.sh`, **not** the generic `netplay-prelaunch.sh`. It must
  unset `NETPLAY_ROLE` and export `NX_CORE_NETPLAY=1` (or a PSP-specific
  equivalent) plus the session PPSSPP core options.

### 4. Session core options: the forcing hook exists, but it is static per core

- `workspace/all/minarch/ma_config.c:1306-1319`
  (`Config_forceNetplayOptions`, called from `Config_readOptions`): when
  `NETPLAY_ROLE` is set, applies `NetplayRules_forcedOptions(core.name)`
  and locks them; user config untouched.
- `workspace/all/netplay/netplay_core_rules.c:41-48`: the table currently
  has entries for `mupen64plus_next` only.
- Problem for PSP: Host vs Join need **different** values (host enables the
  built-in server; join sets Server IP to the host IP), and the join IP is
  only known at wizard time. A static per-core table cannot express that,
  so PSP options must be applied from session env / files (e.g. the pak
  script writing a session override, or extending the forced-options path to
  read `NX_PSP_*` env) — not just a new table row.

### 5. Saves: PSP should be excluded from save sync, like the link-cable cores

- `skeleton/SYSTEM/shared/bin/netplay-prelaunch.sh`: lockstep cores sync
  saves; `gambatte|gpsp|mgba` get `NETPLAY_SYNC_SAVES=0` because link play
  needs distinct saves. PSP Ad-Hoc is the same — each player keeps their
  own `Saves/PSP/SAVEDATA/`; the Dreamcast pattern (host serves its save,
  client plays on a copy) would be wrong for PSP.
- `workspace/all/minarch/ma_saves.c:449` already treats `NX_CORE_NETPLAY`
  as a netplay session for save handling, so the DC-style flag should carry
  the session restrictions through.

## Remaining work

### Phase 1 — Core options verification & exposure

- [ ] Check out the pinned PPSSPP hash (`d4e3c7eb…`) and read
  `libretro/libretro_core_options.h` for the exact Ad-Hoc key names: Enable
  Networking / WLAN, Enable Built-in PRO Ad Hoc Server, Ad-Hoc Server IP /
  Address, Port Offset, MAC address, Nickname. Web sources confirm the
  settings exist in PPSSPP, but the exact libretro option keys are **not
  yet confirmed**.
- [ ] If any critical option is missing or hidden, patch the core options
  list so it appears and can be set programmatically before launch.
- [ ] Confirm the minarch launch path can override core options for a single
  session (the pre-launch options editor already does this for other cores).
- [ ] Check whether the pinned build has the 1.21+ packet-relay options
  (ppsspp.org docs say relay options are PPSSPP 1.21+) — decides Phase 4
  feasibility. Do not assume.

### Phase 2 — UI integration

- [ ] Add the `netplay` marker to both PSP.pak dirs so the Y NETPLAY hint
  shows on PSP games.
- [ ] Add a per-pak `nx_psp_netplay.sh` modelled on `nx_dc_netplay.sh`:
  run the wizard, unset `NETPLAY_ROLE` / `NETPLAY_PEER_IP` /
  `NETPLAY_MODE`, export `NX_CORE_NETPLAY=1` plus the session PPSSPP
  options.
- [ ] Wizard Host: enable WLAN + built-in Ad Hoc Server, advertise local
  IP. Wizard Join: enable WLAN, set Ad-Hoc Server IP to host IP, match
  Port Offset (default `10000`). Generate / ensure unique MACs where the
  core allows.
- [ ] Optional fallback if wizard integration is delayed: a simple
  "PSP Multiplayer" page (game context menu or Emulator Options) with Mode
  (Local Host / Local Join / Public Server), Host IP field (auto-filled on
  join), Nickname, Apply & Launch — writing the same core options.

### Phase 3 — Launch & session handling

- [ ] Launch both cores with the prepared core options; reuse existing save
  handling with per-player `Saves/PSP/SAVEDATA/` (no save sync).
- [ ] During the session, disable / limit save states, fast-forward and
  reset, as in other netplay modes; provide a "Leave Ad-Hoc" option in the
  in-game menu (see `core_netplay.h` / the bye-over-UDP mechanism on port
  55442).
- [ ] Host unit tests that can run without a device (cf. existing
  `workspace/all/netplay-wizard/tests/` and `workspace/all/netplay/tests/`),
  e.g. Host/Join option generation, Port Offset / MAC handling, wizard
  branching. Verify against the real code, not fixtures.

### Manual test matrix (needs real hardware — not done)

- [ ] Two NX Redux devices over Wi-Fi and over Hotspot.
- [ ] One NX Redux device + a phone running standalone PPSSPP (manual IP join).
- [ ] Games: Tag Force series, Monster Hunter Freedom Unite, Tekken 6, etc.

## Sources

- `workspace/tg5040/cores/Makefile:230-231`,
  `workspace/tg5050/cores/Makefile:230-231`
- `workspace/all/nextui/gamelist.c:1110-1116, 1572, 1812, 2161, 2558`;
  `home.c:1995`
- `workspace/all/netplay/netplay.c:195-210`; `netplay_boot.c:77-83`;
  `core_netplay.h`; `netplay_core_rules.c:41-48`
- `workspace/all/minarch/ma_config.c:1306-1319`; `ma_saves.c:449`
- `skeleton/SYSTEM/tg5040/paks/Emus/PSP.pak/{launch.sh,options.sh,default.cfg}`;
  `skeleton/SYSTEM/tg5040/paks/Emus/DC.pak/{launch.sh,nx_dc_netplay.sh}`;
  `skeleton/SYSTEM/shared/bin/netplay-prelaunch.sh`
- https://www.ppsspp.org/docs/settings/network/ (PPSSPP networking settings)

#ifndef SCRAPER_SERIAL_H
#define SCRAPER_SERIAL_H

#include <stdbool.h>

// Game serials read from the ROM itself. A Chinese-patched ROM changes its CRC and is named in Chinese, but the
// cartridge header or disc still carries the official serial, which ScreenScraper can look up (serialnum=).
//
//   GBA  game code at 0xAC            "AGB-AXVE-USA", "AXVE"
//   NDS  game code at 0x0C            "NTR-AY9E-USA", "AY9E"
//   PS1  SYSTEM.CNF BOOT line         "SLUS-00594"      (.iso / .bin / .img, or the .bin a .cue / .m3u points at)
//   PSP  PSP_GAME/PARAM.SFO DISC_ID   "ULUS-10041"      (.iso)
//
// .chd, .cso, .pbp and archives are not read. The GBA and NDS forms are the cartridge-label spelling; the bare
// code is always offered second.

#define SCRAPER_SERIAL_MAX 40
#define SCRAPER_SERIAL_CANDIDATES 3

// Fill out[] with up to `max` candidate serials, best first. Returns how many (0 when the system has no serial
// scheme or the file cannot be read).
int ScraperSerial_candidates(const char* rom_path, int system_id, char out[][SCRAPER_SERIAL_MAX], int max);

// The ScreenScraper system ids this module reads (the scraper_systems.c values).
bool ScraperSerial_supports(int system_id);

#endif

#ifndef SCRAPER_SEARCH_H
#define SCRAPER_SEARCH_H

#include <stdbool.h>
#include <stdint.h>

#include "scraper_api.h"

// The order of lookups for one ROM, split out of scraper_api.c so it can be host-tested against canned responses.
// Each step runs only when the ones before it found nothing, because every request counts against the account's
// daily quota:
//   1. jeuInfos romnom=<file name>          (skipped for a name with non-ASCII characters: it cannot match)
//   2. jeuInfos crc + size                  (skipped over crc_max_bytes: hashing a disc image takes minutes)
//   3. jeuInfos serialnum=<header serial>   (GBA, NDS, PS1, PSP: survives translation patches and renames)
//   4. jeuRecherche by cleaned title, then jeuInfos gameid=<first hit>   (only when name_search is set)

typedef struct {
	const char* api_base; // "https://api.screenscraper.fr/api2"
	const char* auth;	  // URL-encoded "devid=..&devpassword=..&softname=..[&ssid=..&sspassword=..]"
	const char* filename; // basename, extension included
	const char* rom_path; // NULL when unknown
	int system_id;
	bool name_search;
	long crc_max_bytes;

	// transport, replaceable in tests
	char* (*http_get)(const char* url, void* ctx); // body of a 200 response (malloc'd), else NULL
	void (*rate_limit)(void);					   // may be NULL
	bool (*file_crc)(const char* path, uint32_t* crc, long* size);
	char* (*url_encode)(const char* s); // malloc'd
	void (*note)(const char* how);		// "romnom", "crc", "serial", "search"; may be NULL
	void* ctx;
} ScraperSearchEnv;

// True when a step found the game; info is then filled as ScraperParse_gameInfo does.
bool ScraperSearch_find(const ScraperSearchEnv* env, ScraperGameInfo* info);

#endif

#ifndef SCRAPER_PARSE_H
#define SCRAPER_PARSE_H

#include <stdbool.h>
#include <stddef.h>

#include "cjson/cJSON.h"
#include "scraper_api.h"

// ScreenScraper jeuInfos.php JSON parsing, split out of scraper_api.c so it can be host-tested
// without HTTP.

// The first media URL in `medias` whose type is types[0], else types[1], ...; within a type the
// region goes us > wor > eu > any. out_url is "" when none matches.
void ScraperParse_mediaURL(const cJSON* medias, const char** types, int type_count, char* out_url, int url_size);

// Prefer the Chinese regions (cn > tw > asi > jp > wor > us > eu) over the default us > wor > eu. Set once at start
// from the UI language; any region is still taken when the preferred ones have no media.
void ScraperParse_setPreferChinese(bool on);

// The id of the first game in a jeuRecherche.php response (response.jeux[]); false when there is none (ScreenScraper
// answers a miss with an empty object in the array).
bool ScraperParse_firstGameId(const char* json, char* id, size_t id_size);

// Fill info from a jeuInfos.php response body: the us/wor/eu name (else the first one) and the
// screenshot, box art (3D, else 2D), 2D box art and wheel URLs. Sets info->found and returns true
// when the body has a response.jeu object; false otherwise (info untouched).
bool ScraperParse_gameInfo(const char* json, ScraperGameInfo* info);

#endif

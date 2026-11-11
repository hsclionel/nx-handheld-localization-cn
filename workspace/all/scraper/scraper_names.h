#ifndef SCRAPER_NAMES_H
#define SCRAPER_NAMES_H

#include <stdbool.h>
#include <stddef.h>

#include "scraper_scan.h"

// Name helpers of the Artwork Manager's system and ROM lists, split out of scraper.c for host tests.

// "Game Boy (GB)" -> "GB": the text inside the last (...). "" when there is none, it is empty, or
// it does not fit tag_size.
void Scraper_extractTag(const char* dirname, char* tag_out, int tag_size);

// "01) Game Boy (GB)" -> "Game Boy": the folder name before its last " (", without nextui's
// numeric sort prefix. The whole name when it has no tag.
void Scraper_extractDisplayName(const char* dirname, char* name_out, int name_size);

// Fill dir + key with the parent directory and basename nextui uses to alias this game in a
// map.txt (see mapAliasFor): a folder game keys on its folder name in the folder's parent.
void Scraper_mapKeyForGame(const ScanGame* g, char* dir, size_t dir_size, char* key, size_t key_size);

// True when s holds a byte >= 0x80 (a name ScreenScraper's romnom lookup cannot match).
bool Scraper_hasNonAscii(const char* s);

// The title to search by, from a ROM file name: no directory or extension, no leading sort number, no bracketed
// tags ([..] (..) {..} and the full-width forms), no trailing version or translation tag (v1.2, 汉化版, 简体中文).
// "025_恶魔城-晓月圆舞曲[完美汉化](v1.2).gba" -> "恶魔城-晓月圆舞曲". The plain stem when cleaning leaves nothing.
void Scraper_searchTerm(const char* filename, char* out, size_t out_size);

#endif

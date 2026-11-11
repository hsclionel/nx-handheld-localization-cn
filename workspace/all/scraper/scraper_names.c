#include "scraper_names.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "utils.h" // trimSortingMeta
#include "../common/folder_tag.h"

void Scraper_extractTag(const char* dirname, char* tag_out, int tag_size) {
	// up to the name's last ')'; a tag too long for tag_out is dropped
	folderTag(dirname, tag_out, (size_t)tag_size, FOLDER_TAG_LAST_CLOSE);
}

void Scraper_extractDisplayName(const char* dirname, char* name_out, int name_size) {
	const char* open = strrchr(dirname, '(');
	if (open && open > dirname) {
		int len = (int)(open - dirname);
		while (len > 0 && dirname[len - 1] == ' ')
			len--;
		if (len > 0 && len < name_size) {
			strncpy(name_out, dirname, len);
			name_out[len] = '\0';
		} else {
			snprintf(name_out, name_size, "%s", dirname);
		}
	} else {
		snprintf(name_out, name_size, "%s", dirname);
	}

	// Drop the numeric sort prefix nextui folders use ("1) Game Boy" ->
	// "Game Boy"), the same way the main menu title does (trimSortingMeta).
	char* trimmed = name_out;
	trimSortingMeta(&trimmed);
	if (trimmed != name_out)
		memmove(name_out, trimmed, strlen(trimmed) + 1);
}

void Scraper_mapKeyForGame(const ScanGame* g, char* dir, size_t dir_size,
						   char* key, size_t key_size) {
	if (g->folder_game) {
		// g->path is <parent>/<folder>/<folder>.cue; the listed entry is the
		// folder, so key on the folder name and look in <parent>/map.txt.
		char folder[SCAN_PATH_MAX];
		snprintf(folder, sizeof(folder), "%s", g->path);
		char* slash = strrchr(folder, '/');
		if (slash)
			*slash = '\0'; // -> <parent>/<folder>
		slash = strrchr(folder, '/');
		snprintf(key, key_size, "%s", slash ? slash + 1 : folder);
		if (slash)
			*slash = '\0'; // -> <parent>
		snprintf(dir, dir_size, "%s", folder);
	} else {
		snprintf(key, key_size, "%s", g->filename);
		snprintf(dir, dir_size, "%s", g->path);
		char* slash = strrchr(dir, '/');
		if (slash)
			*slash = '\0'; // -> parent dir of the ROM file
	}
}

bool Scraper_hasNonAscii(const char* s) {
	for (; *s; s++)
		if ((unsigned char)*s >= 0x80)
			return true;
	return false;
}

// One UTF-8 character's length; 1 for a stray byte.
static int u8len(const unsigned char* p) {
	if (p[0] < 0x80)
		return 1;
	if ((p[0] & 0xE0) == 0xC0)
		return 2;
	if ((p[0] & 0xF0) == 0xE0)
		return 3;
	if ((p[0] & 0xF8) == 0xF0)
		return 4;
	return 1;
}

// A bracket pair opens at s: returns the length of its opening character, else 0. *close gets the closing text.
static int bracketOpen(const char* s, const char** close) {
	static const struct {
		const char *open, *close;
	} pairs[] = {{"(", ")"},	  {"[", "]"},	  {"{", "}"},	  {"\xEF\xBC\x88", "\xEF\xBC\x89"}, // （ ）
				 {"\xE3\x80\x90", "\xE3\x80\x91"}, {"\xEF\xBC\xBB", "\xEF\xBC\xBD"}, {"\xEF\xBD\x9B", "\xEF\xBD\x9D"}, // 【】 ［］ ｛｝
				 {"\xE3\x80\x8A", "\xE3\x80\x8B"}, {"\xE3\x80\x8C", "\xE3\x80\x8D"}};					// 《》 「」
	for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++) {
		size_t n = strlen(pairs[i].open);
		if (strncmp(s, pairs[i].open, n) == 0) {
			*close = pairs[i].close;
			return (int)n;
		}
	}
	return 0;
}

// Tag words translation groups append to a title; removed from the end of the search term.
static const char* const TRAILING_TAGS[] = {"\xE5\xAE\x8C\xE7\xBE\x8E\xE6\xB1\x89\xE5\x8C\x96", // 完美汉化
											"\xE7\xAE\x80\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87", // 简体中文
											"\xE7\xB9\x81\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87", // 繁体中文
											"\xE6\xB1\x89\xE5\x8C\x96\xE7\x89\x88",			// 汉化版
											"\xE4\xB8\xAD\xE6\x96\x87\xE7\x89\x88",			// 中文版
											"\xE6\xB1\x89\xE5\x8C\x96",						// 汉化
											"\xE7\xAE\x80\xE4\xBD\x93",						// 简体
											"\xE7\xB9\x81\xE4\xBD\x93",						// 繁体
											"\xE4\xB8\xAD\xE6\x96\x87",						// 中文
											NULL};

static void trimEnds(char* s) {
	size_t len = strlen(s);
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '-' || s[len - 1] == '_' || s[len - 1] == '.'))
		s[--len] = '\0';
	size_t lead = 0;
	while (s[lead] == ' ' || s[lead] == '-' || s[lead] == '_')
		lead++;
	if (lead)
		memmove(s, s + lead, strlen(s + lead) + 1);
}

// True when the text after the last space is a version such as v1.2, V2, ver1.0 or Ver.1.1a.
static size_t trailingVersion(const char* s) {
	const char* sp = strrchr(s, ' ');
	const char* tok = sp ? sp + 1 : s;
	const char* p = tok;
	if (*p != 'v' && *p != 'V')
		return 0;
	p++;
	if (strncasecmp(p, "er", 2) == 0)
		p += 2;
	if (*p == '.')
		p++;
	if (!isdigit((unsigned char)*p))
		return 0;
	while (isdigit((unsigned char)*p) || *p == '.')
		p++;
	if (isalpha((unsigned char)*p) && p[1] == '\0')
		p++;
	return *p == '\0' ? strlen(tok) : 0;
}

void Scraper_searchTerm(const char* filename, char* out, size_t out_size) {
	if (!out || out_size == 0)
		return;
	out[0] = '\0';
	if (!filename)
		return;
	const char* base = strrchr(filename, '/');
	base = base ? base + 1 : filename;

	char stem[256];
	snprintf(stem, sizeof(stem), "%s", base);
	// drop a ROM extension: 2-4 letters/digits with at least one letter ("gba", "chd", "7z"; not ".2" of v1.2)
	char* dot = strrchr(stem, '.');
	if (dot && dot != stem) {
		size_t el = strlen(dot + 1);
		bool alnum = el >= 2 && el <= 4, letter = false;
		for (size_t i = 0; alnum && i < el; i++) {
			if (!isalnum((unsigned char)dot[1 + i]))
				alnum = false;
			if (isalpha((unsigned char)dot[1 + i]))
				letter = true;
		}
		if (alnum && letter)
			*dot = '\0';
	}

	// a leading sort number: "001_", "025 - ", "12." (not a bare number such as 1942)
	const char* s = stem;
	const char* q = s;
	while (isdigit((unsigned char)*q))
		q++;
	if (q > s && q - s <= 4 && (*q == '_' || *q == '.' || *q == ')' || strncmp(q, " - ", 3) == 0)) {
		const char* rest = q + (*q == ' ' ? 3 : 1);
		if (strlen(rest) >= 2)
			s = rest;
	}

	// drop every bracketed part
	char buf[256];
	size_t o = 0;
	for (const char* p = s; *p && o + 4 < sizeof(buf);) {
		const char* close = NULL;
		int n = bracketOpen(p, &close);
		if (n) {
			const char* end = strstr(p + n, close);
			if (end) {
				p = end + strlen(close);
				continue;
			}
		}
		int l = u8len((const unsigned char*)p);
		for (int i = 0; i < l && p[i]; i++)
			buf[o++] = p[i];
		p += l;
	}
	buf[o] = '\0';
	for (char* c = buf; *c; c++)
		if (*c == '_')
			*c = ' ';
	// collapse runs of spaces
	size_t w = 0;
	for (size_t i = 0; buf[i]; i++)
		if (!(buf[i] == ' ' && w > 0 && buf[w - 1] == ' '))
			buf[w++] = buf[i];
	buf[w] = '\0';
	trimEnds(buf);

	// trailing version / translation tags, repeatedly
	for (bool again = true; again;) {
		again = false;
		size_t v = trailingVersion(buf);
		if (v && v < strlen(buf)) {
			buf[strlen(buf) - v] = '\0';
			trimEnds(buf);
			again = true;
			continue;
		}
		for (int i = 0; TRAILING_TAGS[i]; i++) {
			size_t tl = strlen(TRAILING_TAGS[i]), bl = strlen(buf);
			if (bl > tl && strcmp(buf + bl - tl, TRAILING_TAGS[i]) == 0) {
				buf[bl - tl] = '\0';
				trimEnds(buf);
				again = true;
				break;
			}
		}
	}
	if (buf[0] == '\0') // cleaning removed everything: the plain stem is better than nothing
		snprintf(buf, sizeof(buf), "%s", stem);
	snprintf(out, out_size, "%s", buf);
}

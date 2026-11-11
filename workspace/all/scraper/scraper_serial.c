#define _GNU_SOURCE // strcasestr
#include "scraper_serial.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define SS_ID_GBA 12
#define SS_ID_NDS 15
#define SS_ID_PS1 57
#define SS_ID_PSP 61

bool ScraperSerial_supports(int system_id) {
	return system_id == SS_ID_GBA || system_id == SS_ID_NDS || system_id == SS_ID_PS1 || system_id == SS_ID_PSP;
}

static const char* regionName(char c) {
	switch (c) {
	case 'E':
		return "USA";
	case 'J':
		return "JPN";
	case 'P':
		return "EUR";
	case 'D':
		return "GER";
	case 'F':
		return "FRA";
	case 'I':
		return "ITA";
	case 'S':
		return "ESP";
	case 'K':
		return "KOR";
	case 'C':
		return "CHN";
	case 'U':
		return "AUS";
	default:
		return NULL;
	}
}

static bool validCode(const unsigned char* p) {
	for (int i = 0; i < 4; i++)
		if (!((p[i] >= 'A' && p[i] <= 'Z') || (p[i] >= '0' && p[i] <= '9')))
			return false;
	return true;
}

// Cartridge header: code at `offset`, label prefix "AGB" / "NTR".
static int cartridgeCandidates(FILE* f, long offset, const char* prefix, char out[][SCRAPER_SERIAL_MAX], int max) {
	unsigned char hdr[0xC0];
	size_t n = fread(hdr, 1, sizeof(hdr), f);
	if (n < (size_t)offset + 4 || !validCode(hdr + offset))
		return 0;
	char code[5];
	memcpy(code, hdr + offset, 4);
	code[4] = '\0';
	int count = 0;
	const char* region = regionName(code[3]);
	if (region && count < max)
		snprintf(out[count++], SCRAPER_SERIAL_MAX, "%s-%s-%s", prefix, code, region);
	if (count < max)
		snprintf(out[count++], SCRAPER_SERIAL_MAX, "%s", code);
	return count;
}

static int gbaCandidates(FILE* f, char out[][SCRAPER_SERIAL_MAX], int max) {
	unsigned char hdr[0xC0];
	if (fread(hdr, 1, sizeof(hdr), f) < sizeof(hdr) || hdr[0xB2] != 0x96) // the fixed byte of a GBA header
		return 0;
	fseek(f, 0, SEEK_SET);
	return cartridgeCandidates(f, 0xAC, "AGB", out, max);
}

static int ndsCandidates(FILE* f, char out[][SCRAPER_SERIAL_MAX], int max) {
	return cartridgeCandidates(f, 0x0C, "NTR", out, max);
}

// ---------------------------------------------------------------- ISO 9660

typedef struct {
	FILE* f;
	long sector; // bytes per sector in the file: 2048, or 2352 for a raw image
	long skip;	 // bytes before the 2048 data bytes: 0, 16 (mode 1) or 24 (mode 2 form 1)
} Iso;

static bool isoRead(const Iso* iso, uint32_t lba, unsigned char* buf) {
	if (fseek(iso->f, (long)lba * iso->sector + iso->skip, SEEK_SET) != 0)
		return false;
	return fread(buf, 1, 2048, iso->f) == 2048;
}

static bool isoOpen(Iso* iso, FILE* f) {
	static const long layouts[][2] = {{2048, 0}, {2352, 16}, {2352, 24}};
	for (size_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); i++) {
		iso->f = f;
		iso->sector = layouts[i][0];
		iso->skip = layouts[i][1];
		unsigned char pvd[2048];
		if (isoRead(iso, 16, pvd) && pvd[0] == 1 && memcmp(pvd + 1, "CD001", 5) == 0)
			return true;
	}
	return false;
}

static uint32_t le32(const unsigned char* p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Find `name` (case-insensitive, ";1" ignored) in the directory at dir_lba. True with its extent and size.
static bool isoFind(const Iso* iso, uint32_t dir_lba, uint32_t dir_size, const char* name, uint32_t* lba, uint32_t* size) {
	unsigned char buf[2048];
	size_t want = strlen(name);
	for (uint32_t off = 0; off < dir_size; off += 2048) {
		if (!isoRead(iso, dir_lba + off / 2048, buf))
			return false;
		uint32_t p = 0;
		while (p < 2048) {
			unsigned rec = buf[p];
			if (rec == 0)
				break; // the rest of this sector is padding
			if (p + rec > 2048 || rec < 34)
				return false;
			unsigned nlen = buf[p + 32];
			if (p + 33 + nlen > 2048)
				return false;
			const unsigned char* n = buf + p + 33;
			unsigned cmp = nlen;
			for (unsigned i = 0; i < nlen; i++)
				if (n[i] == ';') {
					cmp = i;
					break;
				}
			if (cmp == want && strncasecmp((const char*)n, name, want) == 0) {
				*lba = le32(buf + p + 2);
				*size = le32(buf + p + 10);
				return true;
			}
			p += rec;
		}
	}
	return false;
}

static bool isoRoot(const Iso* iso, uint32_t* lba, uint32_t* size) {
	unsigned char pvd[2048];
	if (!isoRead(iso, 16, pvd))
		return false;
	*lba = le32(pvd + 156 + 2);
	*size = le32(pvd + 156 + 10);
	return true;
}

// "SLUS_005.94" / "SLUS-00594" / "slus00594" -> "SLUS-00594". False when it is not four letters plus digits.
static bool normalizePsSerial(const char* raw, char* out, size_t out_size) {
	char letters[8], digits[16];
	int nl = 0, nd = 0;
	for (const char* p = raw; *p && *p != ';' && !isspace((unsigned char)*p); p++) {
		if (isalpha((unsigned char)*p) && nd == 0 && nl < 4)
			letters[nl++] = (char)toupper((unsigned char)*p);
		else if (isdigit((unsigned char)*p) && nd < 9)
			digits[nd++] = *p;
		else if (*p != '_' && *p != '.' && *p != '-')
			return false;
	}
	if (nl != 4 || nd < 3)
		return false;
	letters[nl] = digits[nd] = '\0';
	snprintf(out, out_size, "%s-%s", letters, digits);
	return true;
}

static int ps1Candidates(const Iso* iso, char out[][SCRAPER_SERIAL_MAX], int max) {
	uint32_t root_lba, root_size, lba, size;
	if (!isoRoot(iso, &root_lba, &root_size) || !isoFind(iso, root_lba, root_size, "SYSTEM.CNF", &lba, &size))
		return 0;
	unsigned char buf[2049];
	if (!isoRead(iso, lba, buf))
		return 0;
	buf[2048] = '\0';
	if (size < 2048)
		buf[size] = '\0';
	const char* boot = strcasestr((const char*)buf, "BOOT");
	const char* eq = boot ? strchr(boot, '=') : NULL;
	if (!eq)
		return 0;
	const char* p = eq + 1;
	while (*p == ' ' || *p == '\t')
		p++;
	if (strncasecmp(p, "cdrom", 5) == 0) {
		p += 5;
		while (isdigit((unsigned char)*p))
			p++;
		if (*p == ':')
			p++;
	}
	while (*p == '\\' || *p == '/')
		p++;
	char serial[SCRAPER_SERIAL_MAX];
	if (!normalizePsSerial(p, serial, sizeof(serial)) || max < 1)
		return 0;
	snprintf(out[0], SCRAPER_SERIAL_MAX, "%s", serial);
	return 1;
}

static int pspCandidates(const Iso* iso, char out[][SCRAPER_SERIAL_MAX], int max) {
	uint32_t root_lba, root_size, dir_lba, dir_size, lba, size;
	if (!isoRoot(iso, &root_lba, &root_size) || !isoFind(iso, root_lba, root_size, "PSP_GAME", &dir_lba, &dir_size) ||
		!isoFind(iso, dir_lba, dir_size, "PARAM.SFO", &lba, &size))
		return 0;
	unsigned char sfo[2048];
	if (!isoRead(iso, lba, sfo) || memcmp(sfo, "\0PSF", 4) != 0)
		return 0;
	uint32_t key_off = le32(sfo + 8), data_off = le32(sfo + 12), count = le32(sfo + 16);
	for (uint32_t i = 0; i < count && 20 + i * 16 + 16 <= sizeof(sfo); i++) {
		const unsigned char* e = sfo + 20 + i * 16;
		uint32_t koff = key_off + (uint32_t)(e[0] | (e[1] << 8)), len = le32(e + 4), doff = data_off + le32(e + 12);
		if (koff + 8 > sizeof(sfo) || doff + len > sizeof(sfo))
			continue;
		if (strncmp((const char*)sfo + koff, "DISC_ID", 8) != 0)
			continue;
		char id[24];
		snprintf(id, sizeof(id), "%.*s", (int)(len < 23 ? len : 23), (const char*)sfo + doff);
		char serial[SCRAPER_SERIAL_MAX];
		if (!normalizePsSerial(id, serial, sizeof(serial)) || max < 1)
			return 0;
		snprintf(out[0], SCRAPER_SERIAL_MAX, "%s", serial);
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------- file selection

static bool hasExt(const char* path, const char* ext) {
	size_t pl = strlen(path), el = strlen(ext);
	return pl > el && strcasecmp(path + pl - el, ext) == 0;
}

// The first FILE "x.bin" of a .cue, or the first entry of an .m3u, resolved next to the list file.
static bool firstReferenced(const char* list_path, bool cue, char* out, size_t out_size) {
	FILE* f = fopen(list_path, "r");
	if (!f)
		return false;
	char line[512], name[512] = "";
	while (fgets(line, sizeof(line), f)) {
		if (cue) {
			char* q = strcasestr(line, "FILE");
			char* a = q ? strchr(q, '"') : NULL;
			char* b = a ? strchr(a + 1, '"') : NULL;
			if (b) {
				*b = '\0';
				snprintf(name, sizeof(name), "%s", a + 1);
				break;
			}
		} else {
			size_t l = strlen(line);
			while (l && (line[l - 1] == '\n' || line[l - 1] == '\r' || line[l - 1] == ' '))
				line[--l] = '\0';
			if (l && line[0] != '#') {
				snprintf(name, sizeof(name), "%s", line);
				break;
			}
		}
	}
	fclose(f);
	if (!name[0])
		return false;
	if (name[0] == '/') {
		snprintf(out, out_size, "%s", name);
	} else {
		snprintf(out, out_size, "%s", list_path);
		char* slash = strrchr(out, '/');
		size_t dir = slash ? (size_t)(slash - out) + 1 : 0;
		snprintf(out + dir, out_size - dir, "%s", name);
	}
	return true;
}

int ScraperSerial_candidates(const char* rom_path, int system_id, char out[][SCRAPER_SERIAL_MAX], int max) {
	if (!rom_path || !ScraperSerial_supports(system_id) || max < 1)
		return 0;
	char resolved[1024];
	snprintf(resolved, sizeof(resolved), "%s", rom_path);
	for (int hop = 0; hop < 2; hop++) {
		if (hasExt(resolved, ".cue") || hasExt(resolved, ".m3u")) {
			char next[1024];
			if (!firstReferenced(resolved, hasExt(resolved, ".cue"), next, sizeof(next)))
				return 0;
			snprintf(resolved, sizeof(resolved), "%s", next);
		}
	}
	if (hasExt(resolved, ".chd") || hasExt(resolved, ".cso") || hasExt(resolved, ".pbp") || hasExt(resolved, ".zip") ||
		hasExt(resolved, ".7z") || hasExt(resolved, ".cue") || hasExt(resolved, ".m3u"))
		return 0;
	FILE* f = fopen(resolved, "rb");
	if (!f)
		return 0;
	int n = 0;
	if (system_id == SS_ID_GBA) {
		n = gbaCandidates(f, out, max);
	} else if (system_id == SS_ID_NDS) {
		n = ndsCandidates(f, out, max);
	} else {
		Iso iso;
		if (isoOpen(&iso, f))
			n = system_id == SS_ID_PSP ? pspCandidates(&iso, out, max) : ps1Candidates(&iso, out, max);
	}
	fclose(f);
	return n;
}

// Host test for scraper_serial.c: header serials of GBA / NDS ROMs and disc serials of PS1 / PSP images,
// built from synthetic files.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../scraper/scraper_serial.h"

static void put32(unsigned char* p, uint32_t v) {
	p[0] = v & 255;
	p[1] = (v >> 8) & 255;
	p[2] = (v >> 16) & 255;
	p[3] = (v >> 24) & 255;
}

// One ISO 9660 directory record; returns its (even) length.
static int dirRec(unsigned char* b, uint32_t lba, uint32_t size, int is_dir, const char* name) {
	int nlen = (int)strlen(name);
	int len = 33 + nlen;
	if (len & 1)
		len++;
	memset(b, 0, len);
	b[0] = (unsigned char)len;
	put32(b + 2, lba);
	put32(b + 10, size);
	b[25] = is_dir ? 2 : 0;
	b[32] = (unsigned char)nlen;
	memcpy(b + 33, name, nlen);
	return len;
}

// Image writer: sector `lba` lands at lba*sector + skip, 2048 data bytes (a raw image has 2352-byte sectors).
typedef struct {
	FILE* f;
	long sector, skip;
} Img;

static void sectorWrite(Img* im, uint32_t lba, const unsigned char* data, size_t n) {
	unsigned char buf[2048] = {0};
	memcpy(buf, data, n);
	fseek(im->f, (long)lba * im->sector + im->skip, SEEK_SET);
	fwrite(buf, 1, 2048, im->f);
}

// A PS1 or PSP image: PVD at 16, root at 18; PS1: SYSTEM.CNF at 20; PSP: PSP_GAME at 22 holding PARAM.SFO at 24.
static void makeDisc(const char* path, long sector, long skip, int psp, const char* boot_or_discid) {
	Img im = {fopen(path, "wb"), sector, skip};
	assert(im.f);
	unsigned char pvd[2048] = {0};
	pvd[0] = 1;
	memcpy(pvd + 1, "CD001", 5);
	pvd[6] = 1;
	dirRec(pvd + 156, 18, 2048, 1, "\0");
	sectorWrite(&im, 16, pvd, sizeof(pvd));

	unsigned char root[2048] = {0};
	int o = dirRec(root, 18, 2048, 1, "\0");
	o += dirRec(root + o, 18, 2048, 1, "\1");
	if (psp) {
		dirRec(root + o, 22, 2048, 1, "PSP_GAME");
	} else {
		dirRec(root + o, 20, (uint32_t)strlen(boot_or_discid), 0, "SYSTEM.CNF;1");
	}
	sectorWrite(&im, 18, root, sizeof(root));

	if (psp) {
		unsigned char dir[2048] = {0};
		int p = dirRec(dir, 22, 2048, 1, "\0");
		p += dirRec(dir + p, 18, 2048, 1, "\1");
		dirRec(dir + p, 24, 2048, 0, "PARAM.SFO;1");
		sectorWrite(&im, 22, dir, sizeof(dir));

		// PSF: header, one index entry, key table "DISC_ID\0", data table
		unsigned char sfo[2048] = {0};
		memcpy(sfo, "\0PSF", 4);
		put32(sfo + 4, 0x101);
		put32(sfo + 8, 0x24);	// key table
		put32(sfo + 12, 0x30);	// data table
		put32(sfo + 16, 1);		// entries
		sfo[20] = 0;			// key offset 0
		sfo[22] = 4;			// fmt: utf8 string
		sfo[23] = 0;
		put32(sfo + 24, (uint32_t)strlen(boot_or_discid) + 1);
		put32(sfo + 28, 16);
		put32(sfo + 32, 0);		// data offset 0
		memcpy(sfo + 0x24, "DISC_ID", 8);
		memcpy(sfo + 0x30, boot_or_discid, strlen(boot_or_discid) + 1);
		sectorWrite(&im, 24, sfo, sizeof(sfo));
	} else {
		sectorWrite(&im, 20, (const unsigned char*)boot_or_discid, strlen(boot_or_discid));
	}
	fclose(im.f);
}

static void expect(const char* path, int sys, int n, const char* a, const char* b) {
	char out[SCRAPER_SERIAL_CANDIDATES][SCRAPER_SERIAL_MAX];
	int got = ScraperSerial_candidates(path, sys, out, SCRAPER_SERIAL_CANDIDATES);
	if (got != n || (a && strcmp(out[0], a) != 0) || (b && strcmp(out[1], b) != 0)) {
		printf("FAIL %s sys %d: got %d [%s] [%s]\n", path, sys, got, got > 0 ? out[0] : "", got > 1 ? out[1] : "");
		assert(0);
	}
}

int main(void) {
	assert(ScraperSerial_supports(12) && ScraperSerial_supports(15) && ScraperSerial_supports(57) &&
		   ScraperSerial_supports(61) && !ScraperSerial_supports(4) && !ScraperSerial_supports(-1));

	// GBA: fixed byte 0x96 at 0xB2, game code at 0xAC
	unsigned char gba[0x200] = {0};
	memcpy(gba + 0xAC, "BPEE", 4);
	gba[0xB2] = 0x96;
	FILE* f = fopen("/tmp/nx_t_a.gba", "wb");
	fwrite(gba, 1, sizeof(gba), f);
	fclose(f);
	expect("/tmp/nx_t_a.gba", 12, 2, "AGB-BPEE-USA", "BPEE");
	memcpy(gba + 0xAC, "AXVJ", 4);
	f = fopen("/tmp/nx_t_b.gba", "wb");
	fwrite(gba, 1, sizeof(gba), f);
	fclose(f);
	expect("/tmp/nx_t_b.gba", 12, 2, "AGB-AXVJ-JPN", "AXVJ");
	memcpy(gba + 0xAC, "AXV?", 4); // not a game code
	f = fopen("/tmp/nx_t_c.gba", "wb");
	fwrite(gba, 1, sizeof(gba), f);
	fclose(f);
	expect("/tmp/nx_t_c.gba", 12, 0, NULL, NULL);
	gba[0xB2] = 0; // not a GBA header
	memcpy(gba + 0xAC, "BPEE", 4);
	f = fopen("/tmp/nx_t_d.gba", "wb");
	fwrite(gba, 1, sizeof(gba), f);
	fclose(f);
	expect("/tmp/nx_t_d.gba", 12, 0, NULL, NULL);
	memcpy(gba + 0xAC, "BPEX", 4); // unknown region letter: only the bare code
	gba[0xB2] = 0x96;
	f = fopen("/tmp/nx_t_e.gba", "wb");
	fwrite(gba, 1, sizeof(gba), f);
	fclose(f);
	expect("/tmp/nx_t_e.gba", 12, 1, "BPEX", NULL);

	// NDS: game code at 0x0C
	unsigned char nds[0x200] = {0};
	memcpy(nds + 0x0C, "AY9E", 4);
	f = fopen("/tmp/nx_t_a.nds", "wb");
	fwrite(nds, 1, sizeof(nds), f);
	fclose(f);
	expect("/tmp/nx_t_a.nds", 15, 2, "NTR-AY9E-USA", "AY9E");

	// PS1: SYSTEM.CNF in a cooked 2048 image, and in raw 2352-byte mode 1 / mode 2 images
	makeDisc("/tmp/nx_t_a.iso", 2048, 0, 0, "BOOT = cdrom:\\SLUS_005.94;1\r\nTCB = 4\r\nEVENT = 10\r\n");
	expect("/tmp/nx_t_a.iso", 57, 1, "SLUS-00594", NULL);
	makeDisc("/tmp/nx_t_b.bin", 2352, 16, 0, "BOOT=cdrom:SLPS_012.34;1\r\n");
	expect("/tmp/nx_t_b.bin", 57, 1, "SLPS-01234", NULL);
	makeDisc("/tmp/nx_t_c.bin", 2352, 24, 0, "BOOT = cdrom0:\\SCES-00311;1\n");
	expect("/tmp/nx_t_c.bin", 57, 1, "SCES-00311", NULL);
	makeDisc("/tmp/nx_t_d.iso", 2048, 0, 0, "BOOT = cdrom:\\PSX.EXE;1\n"); // homebrew: no serial
	expect("/tmp/nx_t_d.iso", 57, 0, NULL, NULL);

	// a .cue and an .m3u lead to the image they list
	f = fopen("/tmp/nx_t_e.cue", "w");
	fputs("FILE \"nx_t_b.bin\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n", f);
	fclose(f);
	expect("/tmp/nx_t_e.cue", 57, 1, "SLPS-01234", NULL);
	f = fopen("/tmp/nx_t_f.m3u", "w");
	fputs("#EXTM3U\nnx_t_e.cue\n", f);
	fclose(f);
	expect("/tmp/nx_t_f.m3u", 57, 1, "SLPS-01234", NULL);

	// PSP: PSP_GAME/PARAM.SFO DISC_ID
	makeDisc("/tmp/nx_t_p.iso", 2048, 0, 1, "ULJM05151");
	expect("/tmp/nx_t_p.iso", 61, 1, "ULJM-05151", NULL);

	// unsupported containers, systems, and missing files
	expect("/tmp/nx_t_a.iso", 4, 0, NULL, NULL); // SNES has no serial scheme
	expect("/tmp/nx_t_none.iso", 57, 0, NULL, NULL);
	f = fopen("/tmp/nx_t_a.chd", "wb");
	fputs("MComprHD", f);
	fclose(f);
	expect("/tmp/nx_t_a.chd", 57, 0, NULL, NULL);
	expect(NULL, 12, 0, NULL, NULL);

	puts("scraper_serial: OK");
	return 0;
}

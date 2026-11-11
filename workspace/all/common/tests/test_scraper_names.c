// Host test for scraper_names.c: the Artwork Manager's system tag, display name and map.txt key.
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../scraper/scraper_names.h"

static void tag(const char* dir, const char* want) {
	char out[16];
	Scraper_extractTag(dir, out, sizeof(out));
	if (strcmp(out, want) != 0) {
		printf("FAIL: tag of '%s' = '%s', want '%s'\n", dir, out, want);
		assert(0);
	}
}

static void display(const char* dir, const char* want) {
	char out[64];
	Scraper_extractDisplayName(dir, out, sizeof(out));
	if (strcmp(out, want) != 0) {
		printf("FAIL: name of '%s' = '%s', want '%s'\n", dir, out, want);
		assert(0);
	}
}

static void term(const char* file, const char* want) {
	char out[128];
	Scraper_searchTerm(file, out, sizeof(out));
	if (strcmp(out, want) != 0) {
		fprintf(stderr, "FAIL: search term of '%s' = '%s', want '%s'\n", file, out, want);
		assert(0);
	}
}

static void test_search_term(void) {
	term("Advance Wars.gba", "Advance Wars");
	term("/Roms/GBA/Advance Wars (USA).gba", "Advance Wars");
	term("025_恶魔城-晓月圆舞曲[完美汉化](v1.2).gba", "恶魔城-晓月圆舞曲");
	term("001_口袋妖怪_绿宝石[汉化v1.2].gba", "口袋妖怪 绿宝石");
	term("最终幻想7 (简).chd", "最终幻想7");
	term("三国志 孔明传 汉化版.gba", "三国志 孔明传");
	term("宝可梦解放 v2.1.1 汉化修订版.gba", "宝可梦解放 v2.1.1 汉化修订版"); // a tag inside the title is left alone
	term("【中文】超级马里奥 Ver.1.2.gba", "超级马里奥");
	term("热血物语EX 加强版.gba", "热血物语EX 加强版"); // 加强版 belongs to the title
	term("1942.gba", "1942");						   // a bare number is not a sort prefix
	term("12. Sonic.gba", "Sonic");
	term("Game v1.2.gba", "Game");
	term("Game.v1.2", "Game.v1.2"); // ".2" is not an extension
	term("Hack [v1.0] (Hack).gba", "Hack");
	term("[Hack].gba", "[Hack]"); // cleaning removed everything, so the plain stem
	term("(a).gba", "(a)");
	term("", "");
	assert(Scraper_hasNonAscii("恶魔城.gba") && !Scraper_hasNonAscii("Castlevania.gba") && Scraper_hasNonAscii("Pok\xC3\xA9mon"));
}

int main(void) {
	tag("Game Boy (GB)", "GB");
	tag("01) Game Boy (GB)", "GB");
	tag("Arcade (FBN) (MAME)", "MAME");
	tag("Game Boy", "");
	tag("Game Boy ()", "");
	tag("Game Boy (GB", "");
	tag("Game Boy GB)", "");
	tag("X (THISTAGISWAYTOOLONG)", ""); // does not fit: none rather than a cut tag

	display("Game Boy (GB)", "Game Boy");
	display("01) Game Boy (GB)", "Game Boy");
	display("1)   Sega Genesis   (MD)", "Sega Genesis");
	display("Game Boy", "Game Boy");
	display("(GB)", "(GB)");		 // nothing in front of the tag: the whole name
	display("2048 (PORTS)", "2048"); // digits without ')' are not a sort prefix
	char small[5];
	Scraper_extractDisplayName("Game Boy (GB)", small, sizeof(small));
	assert(strcmp(small, "Game") == 0); // too long for out: the whole name, cut

	// a ROM keys on its file name in its own folder
	ScanGame g;
	memset(&g, 0, sizeof(g));
	snprintf(g.path, sizeof(g.path), "/sd/Roms/Game Boy (GB)/Tetris.gb");
	snprintf(g.filename, sizeof(g.filename), "Tetris.gb");
	char dir[256], key[128];
	Scraper_mapKeyForGame(&g, dir, sizeof(dir), key, sizeof(key));
	assert(strcmp(dir, "/sd/Roms/Game Boy (GB)") == 0 && strcmp(key, "Tetris.gb") == 0);

	// a folder game keys on the folder name, in the folder's parent map.txt
	memset(&g, 0, sizeof(g));
	snprintf(g.path, sizeof(g.path), "/sd/Roms/PlayStation (PS)/FF7/FF7.m3u");
	snprintf(g.filename, sizeof(g.filename), "FF7.m3u");
	g.folder_game = true;
	Scraper_mapKeyForGame(&g, dir, sizeof(dir), key, sizeof(key));
	assert(strcmp(dir, "/sd/Roms/PlayStation (PS)") == 0 && strcmp(key, "FF7") == 0);

	printf("test_scraper_names: all tests passed\n");
	test_search_term();
	return 0;
}

// Host test for scraper_search.c: the order of lookups for one ROM, against canned responses.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../scraper/scraper_search.h"

#define HIT "{\"response\":{\"jeu\":{\"noms\":[{\"region\":\"us\",\"text\":\"Hit\"}],\"medias\":[{\"type\":\"ss\",\"region\":\"wor\",\"url\":\"http://x/ss.png\"}]}}}"
#define SEARCH_HIT "{\"response\":{\"jeux\":[{},{\"id\":\"1234\"}]}}"
#define SEARCH_MISS "{\"response\":{\"jeux\":[{}]}}"

typedef struct {
	const char* needle; // a URL containing this gets body
	const char* body;
} Route;

static Route routes[8];
static int nroutes;
static char urls[16][512];
static int nurls, nrate;
static char how[16];
static long fake_size = 1000;

static void reset(void) {
	nroutes = nurls = nrate = 0;
	how[0] = '\0';
	fake_size = 1000;
}
static void route(const char* needle, const char* body) {
	routes[nroutes++] = (Route){needle, body};
}
static char* getBody(const char* url, void* ctx) {
	(void)ctx;
	snprintf(urls[nurls++], sizeof(urls[0]), "%s", url);
	for (int i = 0; i < nroutes; i++)
		if (strstr(url, routes[i].needle))
			return strdup(routes[i].body);
	return NULL;
}
static void rate(void) {
	nrate++;
}
static bool crc(const char* path, uint32_t* c, long* size) {
	(void)path;
	*c = 0xDEADBEEF;
	*size = fake_size;
	return true;
}
static char* enc(const char* s) {
	return strdup(s);
}
static void note(const char* h) {
	snprintf(how, sizeof(how), "%s", h);
}

static ScraperSearchEnv env(const char* file, const char* path, int sys, bool name_search) {
	return (ScraperSearchEnv){.api_base = "B", .auth = "A", .filename = file, .rom_path = path, .system_id = sys,
							  .name_search = name_search, .crc_max_bytes = 5000, .http_get = getBody, .rate_limit = rate,
							  .file_crc = crc, .url_encode = enc, .note = note};
}

static void writeGba(const char* path, const char* code) {
	unsigned char h[0x200] = {0};
	memcpy(h + 0xAC, code, 4);
	h[0xB2] = 0x96;
	FILE* f = fopen(path, "wb");
	fwrite(h, 1, sizeof(h), f);
	fclose(f);
}

int main(void) {
	ScraperGameInfo info;
	ScraperSearchEnv e;

	// 1. a plain name: found by romnom, one request
	reset();
	route("romnom=Foo.gba", HIT);
	e = env("Foo.gba", "/tmp/nx_s_a.gba", 12, true);
	assert(ScraperSearch_find(&e, &info) && info.found && strcmp(info.game_name, "Hit") == 0);
	assert(nurls == 1 && nrate == 1 && strcmp(how, "romnom") == 0);
	assert(strstr(urls[0], "B/jeuInfos.php?A&romnom=Foo.gba&systemeid=12&output=json"));

	// 2. romnom misses: checksum and size, with the name along
	reset();
	route("crc=deadbeef", HIT);
	e = env("Foo.gba", "/tmp/nx_s_a.gba", 12, true);
	assert(ScraperSearch_find(&e, &info) && nurls == 2 && strcmp(how, "crc") == 0);
	assert(strstr(urls[1], "romnom=Foo.gba") && strstr(urls[1], "romtaille=1000"));

	// 3. a Chinese name skips romnom (it cannot match) and starts with the checksum, without romnom
	reset();
	route("crc=deadbeef", HIT);
	e = env("口袋妖怪 绿宝石.gba", "/tmp/nx_s_a.gba", 12, true);
	assert(ScraperSearch_find(&e, &info) && nurls == 1 && strcmp(how, "crc") == 0);
	assert(!strstr(urls[0], "romnom"));

	// 4. a disc image over the size limit is not hashed: the serial is next
	reset();
	fake_size = 6000; // over crc_max_bytes 5000
	writeGba("/tmp/nx_s_b.gba", "BPEE");
	route("serialnum=AGB-BPEE-USA", HIT);
	e = env("口袋妖怪 绿宝石.gba", "/tmp/nx_s_b.gba", 12, true);
	assert(ScraperSearch_find(&e, &info) && nurls == 1 && strcmp(how, "serial") == 0);
	assert(strstr(urls[0], "serialnum=AGB-BPEE-USA") && strstr(urls[0], "systemeid=12"));

	// 5. the label spelling misses, the bare code hits: two serial requests, then stop
	reset();
	route("serialnum=BPEE", HIT);
	e = env("口袋妖怪 绿宝石.gba", "/tmp/nx_s_b.gba", 12, false);
	assert(ScraperSearch_find(&e, &info) && strcmp(how, "serial") == 0);
	assert(nurls == 3 && strstr(urls[0], "crc=") && strstr(urls[1], "serialnum=AGB-BPEE-USA") && strstr(urls[2], "serialnum=BPEE"));

	// 6. nothing by file or serial: title search finds an id, then the game by id
	reset();
	route("jeuRecherche.php", SEARCH_HIT);
	route("gameid=1234", HIT);
	e = env("025_恶魔城-晓月圆舞曲[完美汉化](v1.2).gba", "/tmp/nx_s_c.gba", 12, true);
	assert(ScraperSearch_find(&e, &info) && strcmp(how, "search") == 0);
	assert(strstr(urls[nurls - 2], "recherche=恶魔城-晓月圆舞曲&systemeid=12"));
	assert(strstr(urls[nurls - 1], "gameid=1234"));

	// 7. title search off: the same ROM stays unfound and no recherche request goes out
	reset();
	route("jeuRecherche.php", SEARCH_HIT);
	route("gameid=1234", HIT);
	e = env("025_恶魔城-晓月圆舞曲[完美汉化](v1.2).gba", "/tmp/nx_s_c.gba", 12, false);
	assert(!ScraperSearch_find(&e, &info));
	for (int i = 0; i < nurls; i++)
		assert(!strstr(urls[i], "jeuRecherche"));

	// 8. the title search answers an empty array element: not found, and no gameid request
	reset();
	route("jeuRecherche.php", SEARCH_MISS);
	e = env("Foo.gba", NULL, 12, true);
	assert(!ScraperSearch_find(&e, &info));
	assert(nurls == 2 && strstr(urls[1], "recherche=Foo")); // romnom, then the search; no crc without a path
	assert(nrate == nurls);

	// 9. no network answer at all
	reset();
	e = env("Foo.gba", "/tmp/nx_s_a.gba", 12, true);
	assert(!ScraperSearch_find(&e, &info) && !info.found);

	puts("scraper_search: OK");
	return 0;
}

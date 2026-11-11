// Host test for scraper_parse.c: which ScreenScraper media URL and game name the Artwork Manager
// takes from a jeuInfos.php response (type order first, then region us > wor > eu > any).
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../scraper/scraper_parse.h"

static void media(const char* json, const char** types, int n, const char* want) {
	cJSON* arr = cJSON_Parse(json);
	assert(arr);
	char url[64];
	ScraperParse_mediaURL(arr, types, n, url, sizeof(url));
	if (strcmp(url, want) != 0) {
		printf("FAIL: got '%s' want '%s' for %s\n", url, want, json);
		assert(0);
	}
	cJSON_Delete(arr);
}

static void test_regions_and_search(void) {
	const char* json = "[{\"type\":\"box-2D\",\"region\":\"us\",\"url\":\"US\"},{\"type\":\"box-2D\",\"region\":\"jp\",\"url\":\"JP\"},"
					   "{\"type\":\"box-2D\",\"region\":\"cn\",\"url\":\"CN\"},{\"type\":\"box-2D\",\"region\":\"fr\",\"url\":\"FR\"}]";
	const char* box[] = {"box-2D"};
	ScraperParse_setPreferChinese(false);
	media(json, box, 1, "US");
	ScraperParse_setPreferChinese(true);
	media(json, box, 1, "CN");
	// Chinese preferred but only a Japanese and a French art exist: jp comes before the rest
	media("[{\"type\":\"box-2D\",\"region\":\"fr\",\"url\":\"FR\"},{\"type\":\"box-2D\",\"region\":\"jp\",\"url\":\"JP\"}]", box, 1, "JP");
	// no preferred region at all: the first one
	media("[{\"type\":\"box-2D\",\"region\":\"fr\",\"url\":\"FR\"},{\"type\":\"box-2D\",\"region\":\"de\",\"url\":\"DE\"}]", box, 1, "FR");
	ScraperParse_setPreferChinese(false);
	media("[{\"type\":\"box-2D\",\"region\":\"jp\",\"url\":\"JP\"},{\"type\":\"box-2D\",\"region\":\"eu\",\"url\":\"EU\"}]", box, 1, "EU");

	char id[16];
	assert(ScraperParse_firstGameId("{\"response\":{\"jeux\":[{},{\"id\":\"1234\"},{\"id\":\"9\"}]}}", id, sizeof(id)) && strcmp(id, "1234") == 0);
	assert(ScraperParse_firstGameId("{\"response\":{\"jeux\":[{\"id\":5678}]}}", id, sizeof(id)) && strcmp(id, "5678") == 0);
	assert(!ScraperParse_firstGameId("{\"response\":{\"jeux\":[{}]}}", id, sizeof(id)));
	assert(!ScraperParse_firstGameId("{\"response\":{}}", id, sizeof(id)));
	assert(!ScraperParse_firstGameId("not json", id, sizeof(id)) && !ScraperParse_firstGameId(NULL, id, sizeof(id)));
	assert(!ScraperParse_firstGameId("{\"response\":{\"jeux\":[{\"id\":\"123456789012345678\"}]}}", id, 8)); // does not fit
}

int main(void) {
	const char* box[] = {"box-3D", "box-2D"};
	const char* wheel[] = {"wheel-hd", "wheel"};
	const char* ss[] = {"ss"};

	// region priority within one type: us > wor > eu > any
	media("[{\"type\":\"ss\",\"region\":\"jp\",\"url\":\"jp\"},{\"type\":\"ss\",\"region\":\"eu\",\"url\":\"eu\"},"
		  "{\"type\":\"ss\",\"region\":\"wor\",\"url\":\"wor\"},{\"type\":\"ss\",\"region\":\"us\",\"url\":\"us\"}]",
		  ss, 1, "us");
	media("[{\"type\":\"ss\",\"region\":\"jp\",\"url\":\"jp\"},{\"type\":\"ss\",\"region\":\"eu\",\"url\":\"eu\"},"
		  "{\"type\":\"ss\",\"region\":\"wor\",\"url\":\"wor\"}]",
		  ss, 1, "wor");
	media("[{\"type\":\"ss\",\"region\":\"jp\",\"url\":\"jp\"},{\"type\":\"ss\",\"region\":\"eu\",\"url\":\"eu\"}]", ss,
		  1, "eu");
	// no preferred region (or none at all): the first of the type
	media("[{\"type\":\"ss\",\"region\":\"jp\",\"url\":\"jp\"},{\"type\":\"ss\",\"url\":\"noregion\"}]", ss, 1, "jp");
	media("[{\"type\":\"ss\",\"url\":\"noregion\"}]", ss, 1, "noregion");

	// type order beats region: a jp box-3D wins over a us box-2D; wheel-hd over wheel
	media("[{\"type\":\"box-2D\",\"region\":\"us\",\"url\":\"2d-us\"},"
		  "{\"type\":\"box-3D\",\"region\":\"jp\",\"url\":\"3d-jp\"}]",
		  box, 2, "3d-jp");
	media("[{\"type\":\"box-2D\",\"region\":\"us\",\"url\":\"2d-us\"}]", box, 2, "2d-us");
	media("[{\"type\":\"wheel\",\"region\":\"us\",\"url\":\"w\"},{\"type\":\"wheel-hd\",\"region\":\"eu\","
		  "\"url\":\"whd\"}]",
		  wheel, 2, "whd");

	// non-string type/url/region entries are skipped; nothing matching gives ""
	media("[{\"type\":5,\"region\":\"us\",\"url\":\"bad-type\"},{\"type\":\"ss\",\"region\":\"us\",\"url\":7},"
		  "{\"type\":\"ss\",\"region\":1,\"url\":\"bad-region\"}]",
		  ss, 1, "bad-region");
	media("[{\"type\":\"fanart\",\"url\":\"f\"}]", ss, 1, "");
	media("[]", ss, 1, "");

	// the URL is cut to url_size
	{
		cJSON* arr = cJSON_Parse("[{\"type\":\"ss\",\"url\":\"0123456789\"}]");
		char url[5];
		ScraperParse_mediaURL(arr, ss, 1, url, sizeof(url));
		assert(strcmp(url, "0123") == 0);
		cJSON_Delete(arr);
	}

	// full response: us/wor/eu name preferred, else the first; every media slot filled
	ScraperGameInfo info;
	memset(&info, 0, sizeof(info));
	assert(ScraperParse_gameInfo(
		"{\"response\":{\"jeu\":{\"noms\":[{\"region\":\"jp\",\"text\":\"Rockman\"},{\"region\":\"ss\",\"text\":\"X\"},"
		"{\"region\":\"wor\",\"text\":\"Mega Man\"},{\"region\":\"us\",\"text\":\"Mega Man US\"}],"
		"\"medias\":[{\"type\":\"sstitle\",\"region\":\"us\",\"url\":\"title\"},"
		"{\"type\":\"box-2D\",\"region\":\"eu\",\"url\":\"b2d\"},{\"type\":\"wheel\",\"url\":\"wh\"}]}}}",
		&info));
	assert(info.found);
	assert(strcmp(info.game_name, "Mega Man") == 0);
	assert(strcmp(info.screenshot_url, "title") == 0); // sstitle when there is no ss
	assert(strcmp(info.boxart_url, "b2d") == 0);	   // 2D when there is no 3D
	assert(strcmp(info.boxart2d_url, "b2d") == 0);
	assert(strcmp(info.wheel_url, "wh") == 0);

	memset(&info, 0, sizeof(info));
	assert(ScraperParse_gameInfo("{\"response\":{\"jeu\":{\"noms\":[{\"region\":\"jp\",\"text\":\"Rockman\"},"
								 "{\"region\":\"kr\",\"text\":\"Rokeumaen\"},{\"text\":3}]}}}",
								 &info));
	assert(strcmp(info.game_name, "Rockman") == 0);
	assert(info.screenshot_url[0] == '\0' && info.boxart_url[0] == '\0');

	// not a game: no response.jeu, broken JSON, NULL
	memset(&info, 0, sizeof(info));
	assert(!ScraperParse_gameInfo("{\"response\":{}}", &info) && !info.found);
	assert(!ScraperParse_gameInfo("{\"header\":{}}", &info));
	assert(!ScraperParse_gameInfo("Erreur : Jeu non trouvée !", &info));
	assert(!ScraperParse_gameInfo(NULL, &info));

	printf("test_scraper_parse: all tests passed\n");
	test_regions_and_search();
	return 0;
}

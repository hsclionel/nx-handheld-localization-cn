#include "scraper_search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scraper_names.h"
#include "scraper_parse.h"
#include "scraper_serial.h"

#define URL_MAX 2304

static bool tryInfo(const ScraperSearchEnv* env, const char* url, const char* how, ScraperGameInfo* info) {
	if (env->rate_limit)
		env->rate_limit();
	char* body = env->http_get(url, env->ctx);
	if (!body)
		return false;
	bool found = ScraperParse_gameInfo(body, info);
	free(body);
	if (found && env->note)
		env->note(how);
	return found;
}

static char* enc(const ScraperSearchEnv* env, const char* s) {
	return env->url_encode(s);
}

static bool bySerial(const ScraperSearchEnv* env, ScraperGameInfo* info) {
	if (!env->rom_path || !ScraperSerial_supports(env->system_id))
		return false;
	char serials[SCRAPER_SERIAL_CANDIDATES][SCRAPER_SERIAL_MAX];
	int n = ScraperSerial_candidates(env->rom_path, env->system_id, serials, SCRAPER_SERIAL_CANDIDATES);
	// at most two requests: the label spelling, then the bare code
	for (int i = 0; i < n && i < 2; i++) {
		char* e = enc(env, serials[i]);
		if (!e)
			return false;
		char url[URL_MAX];
		snprintf(url, sizeof(url), "%s/jeuInfos.php?%s&serialnum=%s&systemeid=%d&output=json", env->api_base, env->auth,
				 e, env->system_id);
		free(e);
		if (tryInfo(env, url, "serial", info))
			return true;
	}
	return false;
}

static bool byName(const ScraperSearchEnv* env, ScraperGameInfo* info) {
	char term[256];
	Scraper_searchTerm(env->filename, term, sizeof(term));
	if (strlen(term) < 2)
		return false;
	char* e = enc(env, term);
	if (!e)
		return false;
	char url[URL_MAX];
	snprintf(url, sizeof(url), "%s/jeuRecherche.php?%s&recherche=%s&systemeid=%d&output=json", env->api_base, env->auth, e,
			 env->system_id);
	free(e);
	if (env->rate_limit)
		env->rate_limit();
	char* body = env->http_get(url, env->ctx);
	if (!body)
		return false;
	char gameid[32];
	bool hit = ScraperParse_firstGameId(body, gameid, sizeof(gameid));
	free(body);
	if (!hit)
		return false;
	snprintf(url, sizeof(url), "%s/jeuInfos.php?%s&gameid=%s&systemeid=%d&output=json", env->api_base, env->auth, gameid,
			 env->system_id);
	return tryInfo(env, url, "search", info);
}

bool ScraperSearch_find(const ScraperSearchEnv* env, ScraperGameInfo* info) {
	if (!env || !env->filename || !info || !env->http_get || !env->url_encode)
		return false;
	memset(info, 0, sizeof(*info));

	char* name = enc(env, env->filename);
	if (!name)
		return false;
	char url[URL_MAX];
	size_t base = 0;
	bool ascii = !Scraper_hasNonAscii(env->filename);

	// 1. the file name
	if (ascii) {
		snprintf(url, sizeof(url), "%s/jeuInfos.php?%s&romnom=%s&systemeid=%d&output=json", env->api_base, env->auth, name,
				 env->system_id);
		if (tryInfo(env, url, "romnom", info)) {
			free(name);
			return true;
		}
	}

	// 2. checksum and size (the name is sent along when it is plain ASCII)
	if (env->rom_path && env->file_crc) {
		uint32_t crc;
		long size;
		if (env->file_crc(env->rom_path, &crc, &size) && (env->crc_max_bytes <= 0 || size <= env->crc_max_bytes)) {
			base = (size_t)snprintf(url, sizeof(url), "%s/jeuInfos.php?%s", env->api_base, env->auth);
			if (ascii)
				snprintf(url + base, sizeof(url) - base, "&romnom=%s", name);
			size_t len = strlen(url);
			snprintf(url + len, sizeof(url) - len, "&systemeid=%d&crc=%08x&romtaille=%ld&output=json", env->system_id, crc,
					 size);
			if (tryInfo(env, url, "crc", info)) {
				free(name);
				return true;
			}
		}
	}
	free(name);

	// 3. the serial in the header
	if (bySerial(env, info))
		return true;

	// 4. the title
	return env->name_search && byName(env, info);
}

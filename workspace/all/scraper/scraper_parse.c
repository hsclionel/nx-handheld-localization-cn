#include "scraper_parse.h"

#include <stdio.h>
#include <string.h>

// Region priority: us > wor > eu > any, or with Chinese preferred cn > tw > asi > jp > wor > us > eu > any
static const char* const region_priority_default[] = {"us", "wor", "eu", NULL};
static const char* const region_priority_chinese[] = {"cn", "tw", "asi", "jp", "wor", "us", "eu", NULL};
static bool prefer_chinese = false;

void ScraperParse_setPreferChinese(bool on) {
	prefer_chinese = on;
}

void ScraperParse_mediaURL(const cJSON* medias, const char** types, int type_count,
						   char* out_url, int url_size) {
	out_url[0] = '\0';
	const char* const* region_priority = prefer_chinese ? region_priority_chinese : region_priority_default;

	for (int t = 0; t < type_count; t++) {
		// First pass: try each preferred region in priority order
		for (int r = 0; region_priority[r] != NULL; r++) {
			cJSON* media;
			cJSON_ArrayForEach(media, medias) {
				cJSON* type = cJSON_GetObjectItem(media, "type");
				if (!type || !cJSON_IsString(type))
					continue;
				if (strcmp(type->valuestring, types[t]) != 0)
					continue;
				cJSON* region = cJSON_GetObjectItem(media, "region");
				cJSON* url = cJSON_GetObjectItem(media, "url");
				if (!url || !cJSON_IsString(url))
					continue;
				if (region && cJSON_IsString(region) &&
					strcmp(region->valuestring, region_priority[r]) == 0) {
					snprintf(out_url, url_size, "%s", url->valuestring);
					return;
				}
			}
		}

		// Second pass: take any available for this type
		cJSON* media;
		cJSON_ArrayForEach(media, medias) {
			cJSON* type = cJSON_GetObjectItem(media, "type");
			if (!type || !cJSON_IsString(type))
				continue;
			if (strcmp(type->valuestring, types[t]) != 0)
				continue;
			cJSON* url = cJSON_GetObjectItem(media, "url");
			if (!url || !cJSON_IsString(url))
				continue;
			snprintf(out_url, url_size, "%s", url->valuestring);
			return;
		}
	}
}

bool ScraperParse_firstGameId(const char* json, char* id, size_t id_size) {
	if (!json || !id || id_size == 0)
		return false;
	cJSON* root = cJSON_Parse(json);
	if (!root)
		return false;
	bool found = false;
	cJSON* response = cJSON_GetObjectItem(root, "response");
	cJSON* jeux = response ? cJSON_GetObjectItem(response, "jeux") : NULL;
	cJSON* game;
	cJSON_ArrayForEach(game, jeux) {
		cJSON* gid = cJSON_GetObjectItem(game, "id");
		if (gid && cJSON_IsString(gid) && gid->valuestring[0] && strlen(gid->valuestring) < id_size) {
			snprintf(id, id_size, "%s", gid->valuestring);
			found = true;
			break;
		}
		if (gid && cJSON_IsNumber(gid) && gid->valuedouble > 0) {
			snprintf(id, id_size, "%.0f", gid->valuedouble);
			found = true;
			break;
		}
	}
	cJSON_Delete(root);
	return found;
}

bool ScraperParse_gameInfo(const char* json, ScraperGameInfo* info) {
	if (!json)
		return false;
	cJSON* root = cJSON_Parse(json);
	if (!root)
		return false;

	cJSON* response = cJSON_GetObjectItem(root, "response");
	if (!response) {
		cJSON_Delete(root);
		return false;
	}

	cJSON* jeu = cJSON_GetObjectItem(response, "jeu");
	if (!jeu) {
		cJSON_Delete(root);
		return false;
	}

	info->found = true;

	// Extract game name (prefer English, fallback to any)
	cJSON* noms = cJSON_GetObjectItem(jeu, "noms");
	if (noms && cJSON_IsArray(noms)) {
		cJSON* nom;
		bool got_name = false;
		cJSON_ArrayForEach(nom, noms) {
			cJSON* region = cJSON_GetObjectItem(nom, "region");
			cJSON* text = cJSON_GetObjectItem(nom, "text");
			if (!text || !cJSON_IsString(text))
				continue;
			if (!got_name) {
				snprintf(info->game_name, sizeof(info->game_name), "%s", text->valuestring);
				got_name = true;
			}
			if (region && cJSON_IsString(region) &&
				(strcmp(region->valuestring, "us") == 0 ||
				 strcmp(region->valuestring, "wor") == 0 ||
				 strcmp(region->valuestring, "eu") == 0)) {
				snprintf(info->game_name, sizeof(info->game_name), "%s", text->valuestring);
				break;
			}
		}
	}

	// Extract media URLs
	cJSON* medias = cJSON_GetObjectItem(jeu, "medias");
	if (medias && cJSON_IsArray(medias)) {
		const char* screenshot_types[] = {"ss", "sstitle"};
		ScraperParse_mediaURL(medias, screenshot_types, 2,
							  info->screenshot_url, sizeof(info->screenshot_url));

		const char* boxart_types[] = {"box-3D", "box-2D"};
		ScraperParse_mediaURL(medias, boxart_types, 2,
							  info->boxart_url, sizeof(info->boxart_url));

		const char* boxart2d_types[] = {"box-2D"};
		ScraperParse_mediaURL(medias, boxart2d_types, 1,
							  info->boxart2d_url, sizeof(info->boxart2d_url));

		const char* wheel_types[] = {"wheel-hd", "wheel"};
		ScraperParse_mediaURL(medias, wheel_types, 2,
							  info->wheel_url, sizeof(info->wheel_url));
	}

	cJSON_Delete(root);
	return true;
}

// session_options.c - see session_options.h.
#include "session_options.h"

#include <string.h>

int SessionOptions_parse(const char* env, char keys[][SESSION_OPTION_KEY_MAX],
	char values[][SESSION_OPTION_VALUE_MAX], size_t max, size_t* count) {
	*count = 0;
	if (!env || !env[0])
		return 0;
	const char* p = env;
	while (*p) {
		const char* seg_end = strchr(p, ';');
		size_t seg_len = seg_end ? (size_t)(seg_end - p) : strlen(p);
		if (seg_len == 0) {
			// empty segment (leading or trailing ';'): skip it
			p = seg_end ? seg_end + 1 : p + seg_len;
			continue;
		}
		const char* eq = memchr(p, '=', seg_len);
		if (!eq)
			return -1;
		size_t key_len = (size_t)(eq - p);
		size_t val_len = seg_len - key_len - 1;
		if (key_len == 0 || val_len == 0 || key_len >= SESSION_OPTION_KEY_MAX ||
			val_len >= SESSION_OPTION_VALUE_MAX)
			return -1;
		if (*count >= max)
			return -1;
		memcpy(keys[*count], p, key_len);
		keys[*count][key_len] = '\0';
		memcpy(values[*count], eq + 1, val_len);
		values[*count][val_len] = '\0';
		(*count)++;
		p = seg_end ? seg_end + 1 : p + seg_len;
	}
	return (int)*count;
}

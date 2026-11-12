// i18n: switchable UI language by string lookup.
//
// The UI source keeps its English literals. Text is translated where it is
// drawn or measured (api.c's GFX text primitives), so app files stay free of
// translation edits and upstream rebases stay clean.
//
// Dictionary format (UTF-8, one entry per line, TAB separated):
//     # comment
//     English text<TAB>translation[<TAB>ignored source note]
// Both columns use C escapes: \n \t \\ and \" .
// Entry kinds (picked from the English key):
//     plain      "Back"                 exact match
//     pattern    "Added: %s"            the key contains printf conversions. The text drawn is
//                                       already formatted ("Added: Mix"), so the key is matched
//                                       against it and the captured pieces are put back into the
//                                       translation: "已添加：%s". The translation may reorder or
//                                       skip pieces with %2$s style positions.
//     prefix     "^Disc <TAB>碟片 "     text that starts with "Disc " keeps its tail
//
// Single-header: declarations here; the implementation is compiled into the one
// translation unit that defines I18N_IMPL before including this file (api.c, and
// the host test). That keeps all 26 per-app Makefiles unchanged.

#ifndef I18N_H
#define I18N_H

#include <stddef.h>

// Language ids stored in the settings file (language=N).
enum {
	I18N_LANG_EN = 0,
	I18N_LANG_ZH_CN = 1,
	I18N_LANG_COUNT
};

// "zh_CN" for I18N_LANG_ZH_CN, NULL for English / unknown.
const char* I18N_langCode(int lang);

// Load the dictionary at `path` as the active one; NULL or an unreadable file
// leaves English (no dictionary). Returns the number of entries loaded
// (plain + pattern). Safe to call while other threads call I18N_tr: the old
// table is retired, not freed.
int I18N_loadFile(const char* path);

// Translation of `en`, or `en` itself when inactive or unknown. Plain hits point
// into the dictionary (valid until exit). Pattern and prefix results live in a
// per-thread ring of 8 buffers.
const char* I18N_tr(const char* en);

// 1 when a non-empty dictionary is active.
int I18N_isActive(void);

#define T(s) I18N_tr(s)

#endif // I18N_H

#ifdef I18N_IMPL
#ifndef I18N_IMPL_DONE
#define I18N_IMPL_DONE

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char* key;
	const char* val;
	uint32_t hash;
} I18NEntry;

#define I18N_MAX_SPECS 8

typedef struct {
	const char* lit; // literal text (kind 0); NULL for a conversion
	size_t len;
	char conv; // conversion letter for kind 1
} I18NSeg;

typedef struct {
	I18NSeg* segs;
	int nseg;
	int nspec;
	size_t lit_total; // sort key: longer literal text = more specific
	const char* val;  // translation, still holding its conversions
} I18NPattern;

typedef struct {
	I18NEntry* slots; // open addressing, power-of-two size, key == NULL is empty
	uint32_t mask;
	int count;
	I18NEntry* prefixes; // longest prefix first
	int prefix_count;
	I18NPattern* patterns; // most specific first
	int pattern_count;
	char* blob; // owns every key/val string
} I18NTable;

static I18NTable* i18n_active = NULL;
static I18NTable* i18n_retired[8]; // replaced tables stay reachable: another thread may still be reading one
static int i18n_retired_n;

static void i18n_set(I18NTable* t) {
	if (i18n_active)
		i18n_retired[i18n_retired_n++ % 8] = i18n_active;
	i18n_active = t;
}

static uint32_t i18n_hash(const char* s) {
	uint32_t h = 2166136261u;
	for (; *s; s++) {
		h ^= (unsigned char)*s;
		h *= 16777619u;
	}
	return h;
}

const char* I18N_langCode(int lang) {
	return lang == I18N_LANG_ZH_CN ? "zh_CN" : NULL;
}

// Unescape in place; returns the new length.
static size_t i18n_unescape(char* s) {
	char* w = s;
	for (const char* r = s; *r; r++) {
		if (*r == '\\' && r[1]) {
			r++;
			switch (*r) {
			case 'n':
				*w++ = '\n';
				break;
			case 't':
				*w++ = '\t';
				break;
			case '\\':
				*w++ = '\\';
				break;
			case '"':
				*w++ = '"';
				break;
			default:
				*w++ = '\\';
				*w++ = *r;
				break;
			}
		} else {
			*w++ = *r;
		}
	}
	*w = '\0';
	return (size_t)(w - s);
}

static void i18n_insert(I18NTable* t, const char* k, const char* v) {
	uint32_t h = i18n_hash(k);
	uint32_t i = h & t->mask;
	while (t->slots[i].key) {
		if (t->slots[i].hash == h && strcmp(t->slots[i].key, k) == 0) {
			t->slots[i].val = v; // later line wins
			return;
		}
		i = (i + 1) & t->mask;
	}
	t->slots[i] = (I18NEntry){k, v, h};
	t->count++;
}

static int i18n_prefix_cmp(const void* a, const void* b) {
	size_t la = strlen(((const I18NEntry*)a)->key), lb = strlen(((const I18NEntry*)b)->key);
	return la < lb ? 1 : (la > lb ? -1 : 0); // longest first
}

static int i18n_pattern_cmp(const void* a, const void* b) {
	size_t la = ((const I18NPattern*)a)->lit_total, lb = ((const I18NPattern*)b)->lit_total;
	return la < lb ? 1 : (la > lb ? -1 : 0);
}

// Parse a printf conversion starting at s (s[0] == '%', s[1] != '%'). Returns its length, 0 if unsupported.
static size_t i18n_conv_len(const char* s, char* conv) {
	const char* p = s + 1;
	while (isdigit((unsigned char)*p))
		p++;
	if (*p == '$') // %2$s
		p++;
	else
		p = s + 1;
	while (*p && strchr("-+ #0", *p))
		p++;
	while (isdigit((unsigned char)*p))
		p++;
	if (*p == '.') {
		p++;
		while (isdigit((unsigned char)*p))
			p++;
	}
	while (*p && strchr("hlzjtL", *p))
		p++;
	if (!*p || !strchr("diouxXfeEgGcs", *p))
		return 0;
	*conv = *p;
	return (size_t)(p - s) + 1;
}

// Split a key holding conversions into literal and conversion segments.
// Returns NULL when it cannot be matched (unknown conversion, a %s directly followed by another
// conversion, too many conversions, or almost no literal text to anchor on).
static I18NPattern i18n_parse_pattern(const char* key, const char* val, int* ok) {
	I18NPattern p = {0};
	*ok = 0;
	size_t kl = strlen(key);
	char* lits = malloc(kl + 2);
	I18NSeg* segs = calloc(kl + 2, sizeof(I18NSeg));
	if (!lits || !segs) {
		free(lits);
		free(segs);
		return p;
	}
	char* w = lits;
	int n = 0;
	const char* lit_start = w;
	for (const char* r = key; *r;) {
		if (r[0] == '%' && r[1] == '%') {
			*w++ = '%';
			r += 2;
			continue;
		}
		if (r[0] == '%') {
			char conv;
			size_t cl = i18n_conv_len(r, &conv);
			if (!cl) {
				free(lits);
				free(segs);
				return p;
			}
			if (w > lit_start) {
				*w++ = '\0';
				segs[n++] = (I18NSeg){lit_start, (size_t)(w - 1 - lit_start), 0};
				p.lit_total += (size_t)(w - 1 - lit_start);
				lit_start = w;
			}
			segs[n++] = (I18NSeg){NULL, 0, conv};
			p.nspec++;
			r += cl;
			continue;
		}
		*w++ = *r++;
	}
	if (w > lit_start) {
		*w++ = '\0';
		segs[n++] = (I18NSeg){lit_start, (size_t)(w - 1 - lit_start), 0};
		p.lit_total += (size_t)(w - 1 - lit_start);
	}
	// One literal char still anchors a match ("%ds" -> "%d秒"; %d only takes digits);
	// a bare conversion with no literal text is still rejected.
	int bad = p.nspec > I18N_MAX_SPECS || p.lit_total < 1;
	for (int i = 0; i + 1 < n; i++)
		if (!segs[i].lit && segs[i].conv == 's' && !segs[i + 1].lit)
			bad = 1; // ambiguous split between two captures
	if (bad) {
		free(lits);
		free(segs);
		return p;
	}
	p.segs = segs;
	p.nseg = n;
	p.val = val;
	*ok = 1;
	return p;
}

int I18N_loadFile(const char* path) {
	if (!path) {
		i18n_set(NULL);
		return 0;
	}
	FILE* f = fopen(path, "rb");
	if (!f) {
		i18n_set(NULL);
		return 0;
	}
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (len <= 0 || len > (8 << 20)) {
		fclose(f);
		i18n_set(NULL);
		return 0;
	}
	char* blob = malloc((size_t)len + 1);
	if (!blob || fread(blob, 1, (size_t)len, f) != (size_t)len) {
		free(blob);
		fclose(f);
		i18n_set(NULL);
		return 0;
	}
	fclose(f);
	blob[len] = '\0';

	// First pass: count lines to size the tables.
	int lines = 0;
	for (long i = 0; i < len; i++)
		if (blob[i] == '\n')
			lines++;
	lines++;
	uint32_t size = 16;
	while (size < (uint32_t)lines * 2)
		size <<= 1;

	I18NTable* t = calloc(1, sizeof(*t));
	if (t) {
		t->slots = calloc(size, sizeof(I18NEntry));
		t->prefixes = calloc((size_t)lines, sizeof(I18NEntry));
		t->patterns = calloc((size_t)lines, sizeof(I18NPattern));
	}
	if (!t || !t->slots || !t->prefixes || !t->patterns) {
		if (t) {
			free(t->slots);
			free(t->prefixes);
			free(t->patterns);
		}
		free(t);
		free(blob);
		i18n_set(NULL);
		return 0;
	}
	t->mask = size - 1;
	t->blob = blob;

	for (char *p = blob, *next; p && *p; p = next) {
		next = strchr(p, '\n');
		if (next)
			*next++ = '\0';
		size_t ll = strlen(p);
		if (ll && p[ll - 1] == '\r')
			p[ll - 1] = '\0';
		if (!*p || *p == '#')
			continue;
		char* tab = strchr(p, '\t');
		if (!tab)
			continue;
		*tab = '\0';
		char* key = p;
		char* val = tab + 1;
		char* tab2 = strchr(val, '\t'); // optional 3rd column (source file) is ignored
		if (tab2)
			*tab2 = '\0';
		int is_prefix = (key[0] == '^');
		if (is_prefix)
			key++;
		i18n_unescape(key);
		i18n_unescape(val);
		if (!*key)
			continue;
		if (is_prefix) {
			t->prefixes[t->prefix_count++] = (I18NEntry){key, val, 0};
		} else if (strchr(key, '%')) {
			int ok;
			I18NPattern pat = i18n_parse_pattern(key, val, &ok);
			if (ok)
				t->patterns[t->pattern_count++] = pat;
			// a pattern that cannot be matched is dropped; its text stays English
		} else {
			i18n_insert(t, key, val);
		}
	}
	if (t->prefix_count > 1)
		qsort(t->prefixes, (size_t)t->prefix_count, sizeof(I18NEntry), i18n_prefix_cmp);
	if (t->pattern_count > 1)
		qsort(t->patterns, (size_t)t->pattern_count, sizeof(I18NPattern), i18n_pattern_cmp);
	if (t->count == 0 && t->prefix_count == 0 && t->pattern_count == 0) {
		free(t->slots);
		free(t->prefixes);
		free(t->patterns);
		free(t);
		free(blob);
		i18n_set(NULL);
		return 0;
	}
	i18n_set(t);
	return t->count + t->pattern_count;
}

int I18N_isActive(void) {
	return i18n_active != NULL;
}

#define I18N_RING 8
#define I18N_RING_LEN 512

static char* i18n_ring_slot(void) {
	static __thread char ring[I18N_RING][I18N_RING_LEN];
	static __thread int slot;
	return ring[slot++ % I18N_RING];
}

// Match `text` against a pattern; fills capture start/length per conversion. Returns 1 on a full match.
static int i18n_match(const I18NPattern* p, const char* text, const char** cs, size_t* cl) {
	const char* t = text;
	int si = 0;
	for (int i = 0; i < p->nseg; i++) {
		const I18NSeg* s = &p->segs[i];
		if (s->lit) {
			if (strncmp(t, s->lit, s->len) != 0)
				return 0;
			t += s->len;
			continue;
		}
		const char* start = t;
		switch (s->conv) {
		case 'd':
		case 'i':
		case 'u':
		case 'o':
		case 'x':
		case 'X': {
			const char* q = t;
			if (*q == '-' || *q == '+')
				q++;
			const char* d = q;
			while (isdigit((unsigned char)*q) || ((s->conv == 'x' || s->conv == 'X') && isxdigit((unsigned char)*q)))
				q++;
			if (q == d)
				return 0;
			t = q;
			break;
		}
		case 'f':
		case 'e':
		case 'E':
		case 'g':
		case 'G': {
			const char* q = t;
			if (*q == '-' || *q == '+')
				q++;
			const char* d = q;
			while (isdigit((unsigned char)*q) || *q == '.')
				q++;
			if (q == d)
				return 0;
			t = q;
			break;
		}
		case 'c':
			if (!*t)
				return 0;
			t++;
			break;
		default: { // 's'
			if (i + 1 < p->nseg) {
				const char* f = strstr(t, p->segs[i + 1].lit);
				if (!f)
					return 0;
				t = f;
			} else {
				t += strlen(t);
			}
			break;
		}
		}
		cs[si] = start;
		cl[si] = (size_t)(t - start);
		si++;
	}
	return *t == '\0';
}

// Build the translation: copy literals, replace each conversion by the captured text (or the Nth with %N$).
static const char* i18n_fill(const I18NPattern* p, const char** cs, size_t* cl) {
	char* out = i18n_ring_slot();
	size_t o = 0;
	int seq = 0;
	for (const char* r = p->val; *r && o + 1 < I18N_RING_LEN;) {
		if (r[0] == '%' && r[1] == '%') {
			out[o++] = '%';
			r += 2;
			continue;
		}
		if (r[0] == '%') {
			char conv;
			size_t n = i18n_conv_len(r, &conv);
			if (!n) {
				out[o++] = *r++;
				continue;
			}
			int idx = -1; // -1 = next capture in order
			const char* q = r + 1;
			if (isdigit((unsigned char)*q)) {
				int num = 0;
				const char* d = q;
				while (isdigit((unsigned char)*d))
					num = num * 10 + (*d++ - '0');
				if (*d == '$')
					idx = num - 1; // %2$s: explicit position, leaves the running count alone
			}
			if (idx < 0)
				idx = seq++;
			if (idx >= 0 && idx < p->nspec) {
				size_t k = cl[idx];
				if (o + k >= I18N_RING_LEN)
					k = I18N_RING_LEN - 1 - o;
				memcpy(out + o, cs[idx], k);
				o += k;
			}
			r += n;
			continue;
		}
		out[o++] = *r++;
	}
	out[o] = '\0';
	return out;
}

const char* I18N_tr(const char* en) {
	const I18NTable* t = i18n_active;
	if (!t || !en || !en[0])
		return en;
	uint32_t h = i18n_hash(en);
	uint32_t i = h & t->mask;
	while (t->slots[i].key) {
		if (t->slots[i].hash == h && strcmp(t->slots[i].key, en) == 0)
			return t->slots[i].val;
		i = (i + 1) & t->mask;
	}
	for (int p = 0; p < t->pattern_count; p++) {
		const char* cs[I18N_MAX_SPECS];
		size_t cl[I18N_MAX_SPECS];
		if (i18n_match(&t->patterns[p], en, cs, cl))
			return i18n_fill(&t->patterns[p], cs, cl);
	}
	for (int p = 0; p < t->prefix_count; p++) {
		size_t kl = strlen(t->prefixes[p].key);
		if (strncmp(en, t->prefixes[p].key, kl) == 0) {
			char* out = i18n_ring_slot();
			snprintf(out, I18N_RING_LEN, "%s%s", t->prefixes[p].val, en + kl);
			return out;
		}
	}
	return en;
}

#endif // I18N_IMPL_DONE
#endif // I18N_IMPL

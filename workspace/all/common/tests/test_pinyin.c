// Host test for common/pinyin.h (no SDL).
#define PINYIN_IMPL
#include "../pinyin.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void eq(const char* got, const char* want) {
	if (strcmp(got, want) != 0) {
		fprintf(stderr, "got  [%s]\nwant [%s]\n", got, want);
		assert(0);
	}
}

int main(void) {
	uint32_t cp;
	assert(Pinyin_decode("a", &cp) == 1 && cp == 'a');
	assert(Pinyin_decode("\xC3\x89", &cp) == 2 && cp == 0xC9);
	assert(Pinyin_decode("吞", &cp) == 3 && cp == 0x541E);
	assert(Pinyin_decode("\xF0\x9F\x98\x80", &cp) == 4 && cp == 0x1F600);
	assert(Pinyin_decode("\xFF", &cp) == 1);

	assert(Pinyin_initial(0x541E) == 't');
	assert(Pinyin_initial('a') == 0 && Pinyin_initial(0x3042) == 0); // not an ideograph (hiragana)
	assert(Pinyin_initial(0x4E00) == 'y' && Pinyin_initial(0x9FFF - 1) != 'A');

	char key[64];
	assert(Pinyin_initials("吞食天地", key, sizeof(key)));
	eq(key, "tstd");
	assert(Pinyin_initials("三国志 孔明传", key, sizeof(key)));
	assert(strncmp(key, "sgz", 3) == 0 && strlen(key) == 6);
	assert(Pinyin_initials("仙剑奇侠传", key, sizeof(key)) && key[0] == 'x' && key[1] == 'j');
	assert(Pinyin_initials("Tetris DX 2", key, sizeof(key)));
	eq(key, "tetrisdx2"); // letters lowercased, digits kept, spaces dropped
	assert(Pinyin_initials("魂斗罗 Contra", key, sizeof(key)) && strstr(key, "contra"));
	assert(!Pinyin_initials("!!! ---", key, sizeof(key)) && key[0] == '\0');
	assert(!Pinyin_initials(NULL, key, sizeof(key)));
	assert(Pinyin_initials("吞食天地", key, 3) && strcmp(key, "ts") == 0); // truncates, stays terminated

	// sort key: ideographs fold to their initial, everything else stays
	char sk[128];
	size_t n = Pinyin_sortKey("吞食天地 II", sk, sizeof(sk));
	eq(sk, "tstd ii");
	assert(n == strlen(sk));
	Pinyin_sortKey("Zelda: 时之笛", sk, sizeof(sk));
	assert(strncmp(sk, "zelda: ", 7) == 0 && strlen(sk) == 7 + 3); // 时之笛 -> three letters
	Pinyin_sortKey("\xC3\x89tude", sk, sizeof(sk));
	eq(sk, "\xC3\x89tude"); // a non-CJK multibyte char is kept as it is
	Pinyin_sortKey("", sk, sizeof(sk));
	eq(sk, "");
	assert(Pinyin_hasMultibyte("魂") && !Pinyin_hasMultibyte("Contra"));

	// every table entry is a-z or '.' (no reading); count the ideographs that have none
	int none = 0;
	for (uint32_t c = 0x4E00; c < 0x4E00 + 20992; c++) {
		char i = Pinyin_initial(c);
		assert(i == 0 || (i >= 'a' && i <= 'z'));
		none += i == 0;
	}
	assert(none < 200);

	puts("pinyin ok");
	return 0;
}

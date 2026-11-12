// Host test for common/i18n.h (no SDL).
#define I18N_IMPL
#include "../i18n.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void write_file(const char* path, const char* text) {
	FILE* f = fopen(path, "wb");
	assert(f);
	fputs(text, f);
	fclose(f);
}

// Entries the loader should keep from a .lang file: every non-empty, non-comment line with a tab.
static int count_entries(const char* path) {
	FILE* f = fopen(path, "rb");
	assert(f);
	char line[4096];
	int n = 0;
	while (fgets(line, sizeof(line), f))
		if (line[0] != '#' && line[0] != '\n' && line[0] != '\r' && strchr(line, '\t'))
			n++;
	fclose(f);
	return n;
}

static void eq(const char* got, const char* want) {
	if (strcmp(got, want) != 0) {
		fprintf(stderr, "got  [%s]\nwant [%s]\n", got, want);
		assert(0);
	}
}

int main(void) {
	const char* path = "/tmp/nx_test_i18n.lang";

	// inactive: strings pass through untouched, same pointer
	const char* s = "Brightness";
	assert(!I18N_isActive());
	assert(I18N_tr(s) == s);
	assert(I18N_tr(NULL) == NULL);

	assert(I18N_loadFile("/nonexistent/zh.lang") == 0);
	assert(!I18N_isActive());

	write_file(path,
			   "# comment line\n"
			   "BRIGHTNESS\t亮度\tapi.c\n"
			   "Quicksave created,\\npowering off\t已创建即时存档，\\n即将关机\n"
			   "Back\\\\slash\tx\\ty\r\n"
			   "^Disc \t碟片 \n"
			   "^Resume: \t继续播放：\n"
			   "^Resume: Radio\t继续电台：\n"
			   // patterns: the key holds printf conversions
			   "Added: %s\t已添加：%s\n"
			   "Game Time: %s\t游玩时长：%s\n"
			   "%d games\t%d 个游戏\n"
			   "%s - %s remaining\t%s - 剩余 %s\n"
			   "Added %d track%s to %s\t已添加 %1$d 首曲目到 %3$s\n"
			   "Slot %02d of %d\t第 %2$d 个中的 %1$02d\n"
			   "%.2f%% unlock rate\t解锁率 %.2f%%\n"
			   "Progress %d%%\t进度 %d%%\n"
			   "Unlocked %B %d %Y\t解锁于 %Y-%m-%d\n"
			   "%s%s\tignored\n"
			   "%d\t%d\n"
			   "no tab line\n"
			   "\n"
			   "BRIGHTNESS\t亮度2\n");
	int n = I18N_loadFile(path);
	assert(n == 3 + 8); // 3 plain (one repeated key counts once) + 8 usable patterns
	assert(I18N_isActive());

	eq(I18N_tr("BRIGHTNESS"), "亮度2"); // later line wins
	eq(I18N_tr("Quicksave created,\npowering off"), "已创建即时存档，\n即将关机");
	eq(I18N_tr("Back\\slash"), "x\ty"); // CRLF stripped, escapes decoded
	eq(I18N_tr("brightness"), "brightness"); // case sensitive
	eq(I18N_tr("Unknown text"), "Unknown text");
	eq(I18N_tr("你好"), "你好");

	// prefix rules keep the tail; the longest prefix wins
	eq(I18N_tr("Disc 2"), "碟片 2");
	eq(I18N_tr("Resume: Game"), "继续播放：Game");
	const char* r = I18N_tr("Resume: Radio One");
	assert(strncmp(r, "继续电台：", strlen("继续电台：")) == 0);

	// patterns: captured text is put back into the translation
	eq(I18N_tr("Added: Mix"), "已添加：Mix");
	eq(I18N_tr("Added: "), "已添加："); // %s may capture an empty string
	eq(I18N_tr("Game Time: 1h 5m"), "游玩时长：1h 5m");
	eq(I18N_tr("12 games"), "12 个游戏");
	eq(I18N_tr("-3 games"), "-3 个游戏");
	eq(I18N_tr("x games"), "x games"); // %d needs digits
	eq(I18N_tr("Foo - 3m remaining"), "Foo - 剩余 3m");
	eq(I18N_tr("Added 3 tracks to Chill"), "已添加 3 首曲目到 Chill"); // plural suffix capture skipped by %3$s
	eq(I18N_tr("Added 1 track to Chill"), "已添加 1 首曲目到 Chill");
	eq(I18N_tr("Slot 07 of 12"), "第 12 个中的 07");
	eq(I18N_tr("12.50% unlock rate"), "解锁率 12.50%");
	eq(I18N_tr("Progress 40%"), "进度 40%");
	eq(I18N_tr("Unlocked November 10 2026"), "Unlocked November 10 2026"); // %B is not printf: pattern dropped
	eq(I18N_tr("5"), "5");
	// a plain exact entry always beats a pattern, and extra text after a pattern fails the match
	eq(I18N_tr("Added: Mix extra"), "已添加：Mix extra");

	// ring buffer: several composed results stay valid together
	const char* a = I18N_tr("Disc 1");
	const char* b = I18N_tr("Disc 2");
	const char* c = I18N_tr("Added: x");
	assert(strcmp(a, "碟片 1") == 0 && strcmp(b, "碟片 2") == 0 && strcmp(c, "已添加：x") == 0);

	// the shipped dictionary: every entry loads (a key the pattern parser rejects, such as a stray
	// "%" or two adjacent %s, is dropped silently and its text stays English on the device)
	const char* real = "../../../../skeleton/SYSTEM/res/lang/zh_CN.lang";
	int want = count_entries(real), got = I18N_loadFile(real);
	if (got != want) {
		fprintf(stderr, "zh_CN.lang: loaded %d of %d entries\n", got, want);
		assert(0);
	}
	eq(I18N_tr("45s"), "45秒");
	eq(I18N_tr("Unlocked 2026-11-12 (pending sync)"), "已解锁 2026-11-12（待同步）");
	eq(I18N_tr("Select the fan speed percentage (Quiet/Normal/Performance or 0-100%)"),
	   "选择风扇速度百分比（安静/标准/高性能或 0-100%）");

	// reload with an empty/garbage file goes back to English
	write_file(path, "just words\nno tabs here\n");
	assert(I18N_loadFile(path) == 0);
	assert(!I18N_isActive());
	eq(I18N_tr("BRIGHTNESS"), "BRIGHTNESS");

	assert(I18N_langCode(I18N_LANG_EN) == NULL);
	eq(I18N_langCode(I18N_LANG_ZH_CN), "zh_CN");

	puts("i18n ok");
	return 0;
}

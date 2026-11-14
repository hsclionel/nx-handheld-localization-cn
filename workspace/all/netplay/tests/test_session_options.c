// Host-compiled tests: NX_SESSION_OPTIONS parsing (the "key=value;..."
// list a pak launcher exports for a core-run session such as PSP Ad-Hoc).
// Build & run: workspace/all/netplay/tests/run_tests.sh
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../session_options.h"

static int parse(const char* env, char keys[][SESSION_OPTION_KEY_MAX],
	char values[][SESSION_OPTION_VALUE_MAX], size_t* count) {
	return SessionOptions_parse(env, keys, values, SESSION_OPTIONS_MAX, count);
}

static void test_empty_and_null(void) {
	char k[SESSION_OPTIONS_MAX][SESSION_OPTION_KEY_MAX];
	char v[SESSION_OPTIONS_MAX][SESSION_OPTION_VALUE_MAX];
	size_t n = 99;
	assert(parse(NULL, k, v, &n) == 0 && n == 0);
	assert(parse("", k, v, &n) == 0 && n == 0);
	assert(parse(";", k, v, &n) == 0 && n == 0);
}

static void test_single_and_many(void) {
	char k[SESSION_OPTIONS_MAX][SESSION_OPTION_KEY_MAX];
	char v[SESSION_OPTIONS_MAX][SESSION_OPTION_VALUE_MAX];
	size_t n = 0;
	assert(parse("ppsspp_enable_wlan=enabled", k, v, &n) == 1 && n == 1);
	assert(strcmp(k[0], "ppsspp_enable_wlan") == 0);
	assert(strcmp(v[0], "enabled") == 0);
	assert(parse("a=1;b=22;ccc=333", k, v, &n) == 3 && n == 3);
	assert(strcmp(k[2], "ccc") == 0 && strcmp(v[2], "333") == 0);
	// a trailing ';' from the builder is tolerated
	assert(parse("a=1;b=2;", k, v, &n) == 2 && n == 2);
}

static void test_host_join_shapes(void) {
	// the exact strings nx_psp_netplay.sh builds (MAC nibbles elided)
	char k[SESSION_OPTIONS_MAX][SESSION_OPTION_KEY_MAX];
	char v[SESSION_OPTIONS_MAX][SESSION_OPTION_VALUE_MAX];
	size_t n = 0;
	int rc = parse("ppsspp_enable_wlan=enabled;"
		       "ppsspp_enable_builtin_pro_ad_hoc_server=enabled;"
		       "ppsspp_change_pro_ad_hoc_server_address=localhost;"
		       "ppsspp_port_offset=10000",
		k, v, &n);
	assert(rc == 4 && n == 4);
	assert(strcmp(v[2], "localhost") == 0 && strcmp(v[3], "10000") == 0);
	rc = parse("ppsspp_enable_wlan=enabled;"
		   "ppsspp_enable_builtin_pro_ad_hoc_server=disabled;"
		   "ppsspp_change_pro_ad_hoc_server_address=IP address;"
		   "ppsspp_pro_ad_hoc_server_address01=1;"
		   "ppsspp_pro_ad_hoc_server_address12=5;"
		   "ppsspp_port_offset=10000",
		k, v, &n);
	assert(rc == 6 && n == 6);
	assert(strcmp(k[2], "ppsspp_change_pro_ad_hoc_server_address") == 0);
	assert(strcmp(v[2], "IP address") == 0);
	assert(strcmp(v[4], "5") == 0);
}

static void test_malformed(void) {
	char k[SESSION_OPTIONS_MAX][SESSION_OPTION_KEY_MAX];
	char v[SESSION_OPTIONS_MAX][SESSION_OPTION_VALUE_MAX];
	size_t n = 0;
	// no '=' at all, empty key, empty value
	assert(parse("ppsspp_enable_wlan", k, v, &n) == -1);
	assert(parse("=enabled", k, v, &n) == -1);
	assert(parse("ppsspp_enable_wlan=", k, v, &n) == -1);
	assert(parse("a=1;broken;b=2", k, v, &n) == -1);
	// overlong key / value do not fit the buffers
	char big[128];
	memset(big, 'x', sizeof(big) - 1);
	big[sizeof(big) - 1] = '\0';
	char env[160];
	snprintf(env, sizeof(env), "%s=1", big);
	assert(parse(env, k, v, &n) == -1);
	snprintf(env, sizeof(env), "k=%s", big);
	assert(parse(env, k, v, &n) == -1);
	// more pairs than the table holds
	char many[1024];
	size_t o = 0;
	for (int i = 0; i < 40 && o + 6 < sizeof(many); i++)
		o += (size_t)snprintf(many + o, sizeof(many) - o, "k%d=v;", i);
	assert(parse(many, k, v, &n) == -1);
}

int main(void) {
	test_empty_and_null();
	test_single_and_many();
	test_host_join_shapes();
	test_malformed();
	printf("test_session_options ok\n");
	return 0;
}

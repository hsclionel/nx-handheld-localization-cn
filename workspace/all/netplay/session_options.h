#ifndef SESSION_OPTIONS_H
#define SESSION_OPTIONS_H

#include <stddef.h>

// Session core options from the environment, for a session the core runs
// itself (PSP Ad-Hoc): the pak's launcher exports NX_SESSION_OPTIONS as
// "key=value;key=value;..." and minarch applies each pair for the session
// only, locked, leaving the user's cfg untouched. Host and join need
// different values (the joiner's server IP is only known at wizard time),
// so the static per-core table in netplay_core_rules.c cannot express them.
//
// Libc only, so the host unit test links this directly.

#define SESSION_OPTIONS_MAX 32
#define SESSION_OPTION_KEY_MAX 64
#define SESSION_OPTION_VALUE_MAX 64

// Parses `env` into parallel key/value arrays (at most `max` pairs).
// Returns the pair count, or -1 when `env` is malformed: a segment with no
// '=', an empty key or value, a key/value that does not fit, or more pairs
// than `max`. Empty segments (a trailing ';') are skipped; `env` NULL or
// empty parses as 0 pairs. On -1 the arrays are left untouched.
int SessionOptions_parse(const char* env, char keys[][SESSION_OPTION_KEY_MAX],
	char values[][SESSION_OPTION_VALUE_MAX], size_t max, size_t* count);

#endif

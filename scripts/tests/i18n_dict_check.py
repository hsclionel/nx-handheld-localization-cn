#!/usr/bin/env python3
"""Cross-check skeleton/SYSTEM/res/lang/zh_CN.lang against the English text in the sources.

Translation happens where text is drawn (api.c's GFX text primitives call I18N_tr on every string),
so the code has no T("...") markers. The dictionary is keyed by the English text itself, which
means an upstream rewording silently turns the old entry dead and leaves the new text in English.

Stale entries (fail): an entry whose English text appears nowhere in the sources any more.
  "Appears" means every literal piece of the key (the text between printf conversions, with a
  leading ^ of a prefix key dropped) is found inside some C string literal (adjacent literals
  joined, C escapes decoded) or, for shell/JSON/text files, in the raw file. Keys that are only
  ever built at run time go in scripts/tests/i18n_allowlist.txt with a one-line reason.

Untranslated candidates (report only, never fails): sentence-like C string literals in the UI
  apps that no dictionary entry matches. Nothing marks which literals get drawn, so this list is
  a heuristic for review, not a rule. Shown with --untranslated.

  python3 scripts/tests/i18n_dict_check.py                  # stale check (exit 1 on stale)
  python3 scripts/tests/i18n_dict_check.py --untranslated   # also list untranslated candidates
"""
import argparse
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LANG = os.path.join(ROOT, "skeleton", "SYSTEM", "res", "lang", "zh_CN.lang")
ALLOW = os.path.join(ROOT, "scripts", "tests", "i18n_allowlist.txt")
SKIP_DIRS = {".git", "build", "tests", "node_modules", "cores", "toolchains"}
C_EXT = (".c", ".h", ".cpp", ".cc", ".hpp", ".m")
RAW_EXT = (".sh", ".json", ".txt", ".cfg", ".ini", ".py", ".lua")
# Apps whose literals are UI text; used only by the untranslated report.
UI_DIRS = ("common", "nextui", "settings", "minarch", "ratools", "sync", "scraper", "netplay-wizard",
           "gameinfo", "extras", "battery", "clock", "minput")

# printf conversion, as i18n.h understands it (%% is a literal percent)
CONV = re.compile(r"%(?:\d+\$)?[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|z|j|t)?[diouxXeEfgGcsp]")
ESC = {"n": "\n", "t": "\t", "\\": "\\", '"': '"', "'": "'", "r": "\r", "0": "\0", "a": "\a"}


def unescape(s):
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n in "01234567" and n != "0" or (n == "0" and i + 2 < len(s) and s[i + 2] in "01234567"):
                m = re.match(r"[0-7]{1,3}", s[i + 1:])
                out.append(chr(int(m.group(0), 8)))
                i += 1 + len(m.group(0))
                continue
            if n == "x":
                m = re.match(r"[0-9a-fA-F]+", s[i + 2:])
                if m:
                    out.append(chr(int(m.group(0), 16) & 0xFF))
                    i += 2 + len(m.group(0))
                    continue
            out.append(ESC.get(n, n))
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def load_dict():
    entries = []
    with open(LANG, encoding="utf-8") as f:
        for no, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            cols = line.split("\t")
            if len(cols) < 2:
                continue
            entries.append({"line": no, "raw": cols[0], "key": unescape(cols[0]),
                            "src": cols[2] if len(cols) > 2 else ""})
    return entries


def load_allow():
    allow = {}
    if os.path.exists(ALLOW):
        with open(ALLOW, encoding="utf-8") as f:
            for line in f:
                line = line.rstrip("\n")
                if not line.strip() or line.startswith("#"):
                    continue
                key, _, why = line.partition("\t")
                if not why.strip():
                    sys.exit(f"{ALLOW}: entry without a reason: {key!r}")
                allow[unescape(key)] = why.strip()
    return allow


C_TOKENS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|\S', re.S)


def c_literals(text):
    """String literals of a C file, adjacent literals joined ("a" "b" -> "ab"), escapes decoded."""
    out, cur = [], None
    for m in C_TOKENS.finditer(text):
        t = m.group(0)
        if t.startswith("//") or t.startswith("/*"):
            continue  # comments don't break adjacency
        if t.startswith('"'):
            s = unescape(t[1:-1])
            cur = s if cur is None else cur + s
            continue
        if cur is not None:
            out.append(cur)
            cur = None
    if cur is not None:
        out.append(cur)
    return out


def scan_sources():
    lits, ui_lits, raw = [], [], []
    for top in ("workspace", "skeleton"):
        for d, dirs, files in os.walk(os.path.join(ROOT, top)):
            dirs[:] = [x for x in dirs if x not in SKIP_DIRS]
            rel = os.path.relpath(d, ROOT).replace(os.sep, "/")
            for name in files:
                p = os.path.join(d, name)
                if name.endswith(C_EXT):
                    with open(p, encoding="utf-8", errors="ignore") as f:
                        found = c_literals(f.read())
                    lits += found
                    parts = rel.split("/")
                    if len(parts) >= 3 and parts[1] == "all" and parts[2] in UI_DIRS:
                        ui_lits += [(s, f"{rel}/{name}") for s in found]
                elif name.endswith(RAW_EXT):
                    with open(p, encoding="utf-8", errors="ignore") as f:
                        raw.append(f.read())
    return "\x00".join(lits), ui_lits, "\x00".join(raw)


def pieces(key):
    k = key[1:] if key.startswith("^") else key
    return [p.replace("%%", "%") for p in CONV.split(k) if p.strip()]


def key_regex(key):
    """The drawn text an entry matches (plain: exact; pattern: conversions as wildcards; ^prefix)."""
    prefix = key.startswith("^")
    k = key[1:] if prefix else key
    out, last = [], 0
    for m in CONV.finditer(k):
        out.append(re.escape(k[last:m.start()].replace("%%", "%")))
        out.append(".*?")
        last = m.end()
    out.append(re.escape(k[last:].replace("%%", "%")))
    return re.compile("".join(out) + (".*" if prefix else "") + r"\Z", re.S)


SENTENCE = re.compile(r"[A-Z][A-Za-z]*[a-z][A-Za-z]*( [A-Za-z0-9'(),./:&+-]+)+[.!?:]?\Z")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--untranslated", action="store_true", help="also list untranslated candidates")
    a = ap.parse_args()

    entries, allow = load_dict(), load_allow()
    lits, ui_lits, raw = scan_sources()

    stale, allowed_used = [], set()
    for e in entries:
        ps = pieces(e["key"])
        if all(p in lits or p in raw for p in ps):
            continue
        if e["key"] in allow:
            allowed_used.add(e["key"])
            continue
        stale.append(e)

    unused_allow = [k for k in allow if k not in allowed_used]
    print(f"zh_CN.lang: {len(entries)} entries, {len(stale)} stale, {len(allowed_used)} allow-listed")
    for e in stale:
        print(f"  STALE line {e['line']} ({e['src'] or '?'}): {e['raw']}")
    for k in unused_allow:
        print(f"  WARN allow-list entry not needed (key found in sources, or no such dictionary entry): {k!r}")

    if a.untranslated:
        plain = {e["key"] for e in entries if not e["key"].startswith("^") and not CONV.search(e["key"])}
        pats = [key_regex(e["key"]) for e in entries if e["key"].startswith("^") or CONV.search(e["key"])]
        seen, cand = set(), []
        for s, where in ui_lits:
            if s in seen or s in plain or not SENTENCE.match(s) or "%" in s:
                continue
            seen.add(s)
            if any(r.match(s) for r in pats):
                continue
            cand.append((where, s))
        print(f"untranslated candidates (heuristic, report only): {len(cand)}")
        for where, s in sorted(cand):
            print(f"  {where}: {s}")

    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())

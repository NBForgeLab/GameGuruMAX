#!/usr/bin/env python3
"""Compute product Lua line coverage from the unit-test coverage dump.

Usage: python compute_lua_coverage.py [coverage_dump.txt]

Reads the "file<TAB>line" pairs dumped by the test binary (GG_LUA_COVERAGE=1),
walks the product Scripts/ tree, counts executable lines (non-blank,
non-comment-only) per file, and prints total coverage plus the files with
the most uncovered executable lines.
"""
import os
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
SCRIPTS_ROOT = os.path.normpath(os.path.join(REPO_ROOT, "Scripts"))

def is_executable(line: str) -> bool:
    stripped = line.strip()
    if not stripped:
        return False
    if stripped.startswith("--"):
        return False
    return True

def collect_script_files():
    files = {}
    for dirpath, _dirs, names in os.walk(SCRIPTS_ROOT):
        for name in names:
            if name.lower().endswith(".lua"):
                full = os.path.normpath(os.path.join(dirpath, name))
                key = full.lower().replace("\\", "/")
                try:
                    with open(full, "r", encoding="utf-8", errors="replace") as f:
                        lines = f.readlines()
                except OSError:
                    continue
                executable = {i + 1 for i, text in enumerate(lines) if is_executable(text)}
                if executable:
                    files[key] = executable
    return files

def main():
    dump_path = sys.argv[1] if len(sys.argv) > 1 else "lua_coverage.txt"
    executed = {}
    with open(dump_path, "r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            raw = raw.rstrip("\n")
            if "\t" not in raw:
                continue
            file_part, line_part = raw.rsplit("\t", 1)
            try:
                line = int(line_part)
            except ValueError:
                continue
            key = os.path.normpath(file_part).lower().replace("\\", "/")
            if key not in SCRIPTS_ROOT.lower():
                # only count product scripts
                if not key.startswith(SCRIPTS_ROOT.lower().replace("\\", "/")):
                    continue
            executed.setdefault(key, set()).add(line)

    files = collect_script_files()
    total_exec = 0
    total_covered = 0
    rows = []
    for key, executable in files.items():
        covered = len(executable & executed.get(key, set()))
        total = len(executable)
        total_exec += total
        total_covered += covered
        rows.append((total - covered, key, covered, total))

    pct = (100.0 * total_covered / total_exec) if total_exec else 0.0
    print(f"Product Lua files: {len(files)}")
    print(f"Executable lines:  {total_exec}")
    print(f"Covered lines:     {total_covered}")
    print(f"LINE COVERAGE:     {pct:.2f} %")
    print()
    print("Top uncovered files (uncovered executable lines):")
    rows.sort(reverse=True)
    for uncovered, key, covered, total in rows[:25]:
        if uncovered == 0:
            break
        rel = os.path.relpath(key, SCRIPTS_ROOT.lower().replace("\\", "/"))
        print(f"  {uncovered:6d} uncovered / {total:6d} total  ({100.0*covered/total:5.1f} %)  {rel}")

if __name__ == "__main__":
    main()

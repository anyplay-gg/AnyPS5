import argparse
import fnmatch
import re
import subprocess
import sys
from pathlib import Path

CELL = re.compile(r"`([^`]+)`")
DEFAULT_DOC = "docs/dev/FORK.md"
DEFAULT_UPSTREAM = "upstream/main"


def git(repo, *args):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, check=False)
    if result.returncode:
        raise RuntimeError(f"git {' '.join(args[:2])} failed: {result.stderr.decode('utf-8', 'replace').strip()}")
    return result.stdout.decode("utf-8", "replace")


def diverging_files(repo, upstream, head):
    base = git(repo, "merge-base", upstream, head).strip()
    return base, sorted(name for name in git(repo, "diff", "-z", "--name-only", base, head, "--").split("\0") if name)


def read_rows(text):
    rows = []
    for line in text.splitlines():
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) < 2:
            continue
        patterns = CELL.findall(cells[0])
        if patterns:
            rows.append({"patterns": patterns, "why": cells[1]})
    return rows


def matches(pattern, path):
    if pattern.endswith("/"):
        return path.startswith(pattern)
    return fnmatch.fnmatchcase(path, pattern)


def analyze(repo, upstream, head, doc):
    base, files = diverging_files(repo, upstream, head)
    path = Path(repo) / doc
    text = path.read_text(encoding="utf-8") if head == "HEAD" and path.exists() else git(repo, "show", f"{head}:{doc}")
    rows = read_rows(text)
    covered = set()
    stale = []
    for row in rows:
        row["files"] = []
        for pattern in row["patterns"]:
            found = [path for path in files if matches(pattern, path)]
            if not found:
                stale.append(pattern)
            row["files"] += [path for path in found if path not in row["files"]]
            covered.update(found)
    return {
        "base": base,
        "files": files,
        "rows": rows,
        "undocumented": [path for path in files if path not in covered],
        "stale": stale,
    }


def render(data, listing=False):
    out = [f"{len(data['files'])} files differ from upstream since {data['base'][:9]}, {len(data['rows'])} documented groups"]
    if listing:
        for row in data["rows"]:
            out.append(f"- {', '.join(row['patterns'])}: {len(row['files'])} files. {row['why']}")
            out += [f"    {path}" for path in row["files"]]
    if data["undocumented"]:
        out.append(f"{len(data['undocumented'])} diverging files are in no row of the table, add them to the table or remove the change:")
        out += [f"  {path}" for path in data["undocumented"]]
    if data["stale"]:
        out.append("table entries that match no diverging file, delete them or fix the pattern:")
        out += [f"  {pattern}" for pattern in data["stale"]]
    return "\n".join(out)


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="backslashreplace")
    parser = argparse.ArgumentParser(description="check that every place the fork differs from upstream is documented in the divergence table")
    parser.add_argument("--repo", type=Path, default=Path("."))
    parser.add_argument("--upstream", default=DEFAULT_UPSTREAM)
    parser.add_argument("--head", default="HEAD")
    parser.add_argument("--doc", default=DEFAULT_DOC)
    parser.add_argument("--list", action="store_true", help="print the files of every group")
    args = parser.parse_args(argv)
    try:
        data = analyze(args.repo, args.upstream, args.head, args.doc)
    except RuntimeError as error:
        parser.error(str(error))
    print(render(data, args.list))
    return 1 if data["undocumented"] or data["stale"] else 0


if __name__ == "__main__":
    sys.exit(main())

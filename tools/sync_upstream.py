import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_main_sync
import fork_divergence

MARKER_SUBJECT = "chore(sync): merge upstream with unresolved conflicts"
STRIP_SUBJECT = "ci(sync): keep the fork's workflow files, the sync token cannot push upstream's changes"
BOT_EMAIL = "41898282+github-actions[bot]@users.noreply.github.com"
PULL = re.compile(r"^Merge pull request #(\d+) from \S+")
PREFIX = re.compile(r"^(feat|fix|docs|style|refactor|perf|test|build|ci|chore|revert)(?:\([^)]*\))?!?:")
LIBRARY = re.compile(r"^core/libs/prx/([^/]+)/")
BODY_LIMIT = 60000
PULL_LIMIT = 60
FILE_LIMIT = 60

CHECKLIST = """### Checklist
- [x] Based on current `main`; no other open PR implements the same functions
- [x] One topic per PR; follow-ups go in a new PR
- [x] No comments in code except technical debt
- [x] Unimplemented paths throw (`NotImplemented_nid_no_patch`); silent stubs are listed under Silent stubs in their library's technical debt file
- [x] No notes, investigation `.md` files or images added to the repository
- [x] New third-party code is a submodule built from source
- [x] Generic behaviour, not specific to one title
- [x] Depends on: none
- [x] Fixes: none
- [x] Unblocks: none
- [x] GPUs tested: n/a
- [x] AI-assisted: no
"""


def run(repo, *args, check=True):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if check and result.returncode:
        raise RuntimeError(f"git {' '.join(args[:2])} failed: {result.stderr.strip()}")
    return result


def out(repo, *args):
    return run(repo, *args).stdout.strip()


def contains(repo, ancestor, ref):
    return run(repo, "merge-base", "--is-ancestor", ancestor, ref, check=False).returncode == 0


def quote(text):
    return "`" + text.replace("`", "'").strip() + "`"


def upstream_pulls(repo, base, upstream):
    text = out(repo, "log", "--first-parent", "--merges", "--format=%s%x1f%b%x1e", f"{base}..{upstream}")
    found = []
    for record in text.split("\x1e"):
        subject, _, body = record.strip().partition("\x1f")
        match = PULL.match(subject)
        if match:
            title = next((line.strip() for line in body.splitlines() if line.strip()), subject)
            found.append((int(match.group(1)), title))
    return found


def upstream_summary(repo, base, upstream):
    subjects = out(repo, "log", "--no-merges", "--format=%s", f"{base}..{upstream}").splitlines()
    kinds = Counter(match.group(1) if (match := PREFIX.match(subject)) else "other" for subject in subjects)
    files = [name for name in out(repo, "diff", "--name-only", base, upstream).splitlines() if name]
    libraries = Counter(match.group(1) if (match := LIBRARY.match(name)) else "elsewhere" for name in files)
    return {
        "commits": int(out(repo, "rev-list", "--count", f"{base}..{upstream}")),
        "kinds": kinds,
        "files": len(files),
        "libraries": libraries,
        "pulls": upstream_pulls(repo, base, upstream),
    }


def group_of(rows, path):
    for row in rows:
        if any(fork_divergence.matches(pattern, path) for pattern in row["patterns"]):
            return row["name"]
    return None


def human_commits(repo, branch, main, upstream):
    emails = out(repo, "log", "--no-merges", "--format=%ae", branch, f"^{main}", f"^{upstream}").splitlines()
    return [email for email in emails if email != BOT_EMAIL]


def merge(repo, ref, message):
    result = run(repo, "merge", "--no-ff", "--no-edit", "-m", message, ref, check=False)
    if result.returncode == 0:
        return []
    unmerged = [name for name in out(repo, "diff", "--name-only", "--diff-filter=U").splitlines() if name]
    if not unmerged:
        raise RuntimeError(f"git merge {ref} failed: {result.stderr.strip() or result.stdout.strip()}")
    return unmerged


def strip_workflows(args):
    repo = args.repo
    changed = [line.split("\t", 1) for line in out(repo, "diff", "--no-renames", "--name-status", args.main, "HEAD", "--", ".github/workflows").splitlines()]
    if not changed:
        return []
    for status, path in changed:
        if status == "A":
            run(repo, "rm", "-q", "-f", "--", path)
        else:
            run(repo, "checkout", args.main, "--", path)
    run(repo, "commit", "-q", "-m", STRIP_SUBJECT)
    return [path for _, path in changed]


def stripped_note(paths):
    lines = ["", "### Workflow files not included", "The workflow token cannot push changes to `.github/workflows`, so this branch keeps the fork's version of every workflow file. Upstream changed these; apply what the fork needs by hand with `git checkout upstream/main -- <file>` and a normal commit:"]
    return "\n".join(lines + [f"- `{path}`" for path in paths])


def build(args):
    repo = args.repo
    main, upstream, remote = args.main, args.upstream, args.remote
    state = {"status": "none", "branch": args.existing or f"{args.prefix}-{args.date}", "push": False, "force": False, "old": "", "conflicts": [], "title": "", "draft": False, "behind": 0, "sha": ""}
    state["behind"] = int(out(repo, "rev-list", "--count", f"{main}..{upstream}"))
    state["sha"] = out(repo, "rev-parse", "--short=9", upstream)
    if state["behind"] == 0:
        return state, ""
    before = check_main_sync.report(repo, upstream, main)
    branch = state["branch"]
    remote_branch = f"{remote}/{branch}"
    existing = bool(args.existing)
    old = out(repo, "rev-parse", remote_branch) if existing else ""
    owned_by_bot = existing and not human_commits(repo, remote_branch, main, upstream)
    unresolved = []
    message = f"chore(sync): merge upstream boykopovar/AnyPS5 up to {state['sha']}"
    if existing and contains(repo, main, remote_branch) and contains(repo, upstream, remote_branch):
        run(repo, "checkout", "-q", "-B", branch, remote_branch)
    elif existing and not owned_by_bot:
        run(repo, "checkout", "-q", "-B", branch, remote_branch)
        if not contains(repo, main, "HEAD"):
            unresolved = merge(repo, main, f"chore(sync): merge main into {branch}")
        if not unresolved and not contains(repo, upstream, "HEAD"):
            unresolved = merge(repo, upstream, message)
        if unresolved:
            run(repo, "merge", "--abort")
    else:
        run(repo, "checkout", "-q", "-B", branch, main)
        unresolved = merge(repo, upstream, message)
        if unresolved:
            run(repo, "add", "-A")
            run(repo, "commit", "-q", "-m", MARKER_SUBJECT)
    head = out(repo, "rev-parse", "HEAD")
    state["old"] = old
    if unresolved:
        state["status"] = "conflict"
        state["conflicts"] = unresolved
        state["draft"] = True
        state["push"] = head != old and (owned_by_bot or not existing)
    else:
        state["status"] = "clean" if head != out(repo, "rev-parse", main) else "none"
        state["push"] = head != old
    state["force"] = bool(existing and owned_by_bot and state["push"])
    suffix = " (conflicts)" if state["draft"] else ""
    state["title"] = f"chore(sync): merge upstream boykopovar/AnyPS5 up to {state['sha']}{suffix}"
    return state, body(repo, args, state, before)


def body(repo, args, state, before):
    summary = upstream_summary(repo, before["mergebase"], args.upstream)
    rows = fork_divergence.read_rows(run(repo, "show", f"{args.main}:{args.doc}", check=False).stdout)
    pulls = summary["pulls"]
    lines = ["### What"]
    lines.append(f"Merges boykopovar/AnyPS5 main (up to {quote(state['sha'])}) into the fork with a merge commit. Upstream is {summary['commits']} commits ahead of the fork's merge base: {len(pulls)} merged pull requests, {summary['files']} files. The fork is {before['ahead']} commits ahead of upstream. Opened by the sync workflow on {args.date}; the workflow refreshes this pull request while it is open and never merges it.")
    if state["status"] == "conflict":
        held = "The branch holds the merge with the conflict markers still in the files, so the build fails until a person resolves them." if state["push"] else "The branch is unchanged because a person has already pushed to it; merge upstream into it by hand."
        lines += ["", f"**Draft: upstream does not merge cleanly.** {len(state['conflicts'])} files have conflicts. {held}", "", "### Conflicts"]
        for name in state["conflicts"]:
            group = group_of(rows, name)
            lines.append(f"- `{name}`" + (f" ({group})" if group else ""))
        lines += ["", "To resolve: check out this branch, fix the files, commit and push. The next run merges new upstream commits on top of your commits."]
    if summary["kinds"]:
        kinds = ", ".join(f"{count} {kind}" for kind, count in summary["kinds"].most_common())
        places = ", ".join(f"{name} ({count})" for name, count in summary["libraries"].most_common(12))
        lines += ["", "### Upstream changes", f"{kinds} (commit subjects, merges excluded).", "", f"Most changed places: {places}."]
    shared = before["shared"]
    conflicting = set(before["conflicts"])
    lines += ["", "### Changed on both sides"]
    if shared:
        lines.append(f"{len(shared)} files were changed by upstream and by the fork since the merge base. These are the fork's hot spots; read the bridge calls in them after the merge.")
        for name in shared[:FILE_LIMIT]:
            group = group_of(rows, name)
            lines.append(f"- `{name}`" + (" (conflict)" if name in conflicting else "") + (f" - {group}" if group else ""))
        if len(shared) > FILE_LIMIT:
            lines.append(f"- and {len(shared) - FILE_LIMIT} more")
        if before["pulls"]:
            numbers = ", ".join(f"`{number}`" for number in sorted(before["pulls"])[:PULL_LIMIT])
            lines += ["", f"Upstream pull requests that touched them: {numbers}."]
    else:
        lines.append("None.")
    lines += ["", "### Upstream pull requests", f"<details><summary>{len(pulls)} merged upstream pull requests</summary>", ""]
    lines += [f"- {quote(str(number))} {quote(title)}" for number, title in pulls[:PULL_LIMIT]]
    if len(pulls) > PULL_LIMIT:
        lines.append(f"- and {len(pulls) - PULL_LIMIT} more")
    lines += ["", "</details>", "", "### Tested"]
    if state["status"] == "conflict":
        lines.append("Not built: the branch has unresolved conflicts.")
    else:
        lines.append("Not tested when opened. The workflow starts the Linux build (hosted runner) and the Windows build for this pull request; read both before merging. A pull request opened with the workflow token starts no other workflows, so close and reopen it to run the conventions check.")
    lines += ["", CHECKLIST]
    return "\n".join(lines)[:BODY_LIMIT]


def main(argv=None):
    parser = argparse.ArgumentParser(description="merge upstream main into a sync branch off main and write the pull request report")
    parser.add_argument("--repo", type=Path, default=Path("."))
    parser.add_argument("--upstream", default="upstream/main")
    parser.add_argument("--main", default="origin/main")
    parser.add_argument("--remote", default="origin")
    parser.add_argument("--existing", default="", help="head branch of the open sync pull request, if there is one")
    parser.add_argument("--prefix", default="sync/upstream", help="branch name prefix, the date is appended")
    parser.add_argument("--date", required=True)
    parser.add_argument("--doc", default=fork_divergence.DEFAULT_DOC)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--strip-workflows", action="store_true", help="restore the fork's workflow files on the checked-out sync branch and add the note to the report in --out")
    args = parser.parse_args(argv)
    if args.strip_workflows:
        try:
            paths = strip_workflows(args)
        except RuntimeError as error:
            parser.error(str(error))
        if paths:
            report = args.out / "body.md"
            text = report.read_text(encoding="utf-8")
            marker = "\n### Tested"
            report.write_text(text.replace(marker, stripped_note(paths) + "\n" + marker, 1), encoding="utf-8")
        print(json.dumps({"stripped": paths}))
        return 0
    try:
        state, text = build(args)
    except RuntimeError as error:
        parser.error(str(error))
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "state.json").write_text(json.dumps(state, indent=2), encoding="utf-8")
    (args.out / "body.md").write_text(text, encoding="utf-8")
    print(json.dumps(state))
    return 0


if __name__ == "__main__":
    sys.exit(main())

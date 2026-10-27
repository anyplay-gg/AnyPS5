from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import fork_divergence
import sync_upstream

IDENTITY = {
    "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@example.com",
    "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@example.com",
}
DOC = (
    "| Group | Paths | Why | Sync risk |\n|---|---|---|\n"
    "| Bridge | `bridge/` | the bridge | Low |\n"
    "| Hot spot | `lib/Export.cpp`<br>`docs/dev/FORK.md` | hot spot | High |\n"
)


class Repo:
    def __init__(self, path):
        self.path = path
        self.git("init", "-q", "-b", "main")
        self.write("lib/Export.cpp", "one\ntwo\nthree\n")
        self.write("lib/Other.cpp", "x\n")
        self.write("docs/dev/FORK.md", DOC)
        self.commit("initial")
        self.git("branch", "upstream")

    def git(self, *args):
        result = subprocess.run(["git", "-C", str(self.path), *args], capture_output=True, text=True, env={**os.environ, **IDENTITY})
        if result.returncode:
            raise RuntimeError(result.stderr)
        return result.stdout.strip()

    def write(self, name, text):
        target = self.path / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding="utf-8")

    def commit(self, message):
        self.git("add", "-A")
        self.git("commit", "-q", "-m", message)

    def on(self, branch):
        self.git("checkout", "-q", branch)

    def upstream_pull(self, number, name, path, text):
        self.on("upstream")
        self.git("checkout", "-q", "-b", name)
        self.write(path, text)
        self.commit(f"fix(x): {name}")
        self.on("upstream")
        self.git("merge", "--no-ff", "-q", "-m", f"Merge pull request #{number} from someone/{name}\n\n{name} title", name)
        self.on("main")


def arguments(repo, out, existing=""):
    return sync_upstream.argparse.Namespace(repo=repo.path, upstream="upstream", main="main", remote="origin", existing=existing, prefix="sync/upstream", date="2026-10-27", doc="docs/dev/FORK.md", out=out)


class SyncUpstreamTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.repo = Repo(Path(self.directory.name))

    def tearDown(self):
        self.directory.cleanup()

    def test_nothing_to_merge(self):
        state, text = sync_upstream.build(arguments(self.repo, None))
        self.assertEqual((state["status"], state["push"], text), ("none", False, ""))

    def test_clean_merge_makes_a_conventional_merge_commit_and_a_template_body(self):
        self.repo.upstream_pull(11, "feature", "lib/Other.cpp", "y\n")
        self.repo.on("main")
        self.repo.write("bridge/a.cpp", "a\n")
        self.repo.commit("feat(bridge): add a")
        state, text = sync_upstream.build(arguments(self.repo, None))
        self.assertEqual((state["status"], state["push"], state["draft"]), ("clean", True, False))
        self.assertTrue(state["title"].startswith("chore(sync): merge upstream boykopovar/AnyPS5 up to "))
        self.assertEqual(self.repo.git("log", "-1", "--format=%s").split(" up to ")[0], "chore(sync): merge upstream boykopovar/AnyPS5")
        for heading in ("### What", "### Tested", "### Checklist", "### Changed on both sides"):
            self.assertIn(heading, text)
        self.assertIn("`11` `feature title`", text)
        self.assertNotIn("- [ ]", text)

    def test_hot_spots_are_listed_with_their_group(self):
        self.repo.upstream_pull(12, "touch", "lib/Export.cpp", "one\ntwo\nthree\nfour\n")
        self.repo.write("lib/Export.cpp", "zero\none\ntwo\nthree\n")
        self.repo.commit("feat(bridge): hook")
        state, text = sync_upstream.build(arguments(self.repo, None))
        self.assertEqual(state["status"], "clean")
        self.assertIn("`lib/Export.cpp` - Hot spot", text)

    def test_conflict_commits_the_markers_and_marks_the_pull_request_draft(self):
        self.repo.upstream_pull(13, "clash", "lib/Export.cpp", "one\ntheirs\nthree\n")
        self.repo.write("lib/Export.cpp", "one\nmine\nthree\n")
        self.repo.commit("feat(bridge): mine")
        state, text = sync_upstream.build(arguments(self.repo, None))
        self.assertEqual((state["status"], state["draft"], state["push"]), ("conflict", True, True))
        self.assertEqual(state["conflicts"], ["lib/Export.cpp"])
        self.assertTrue(state["title"].endswith("(conflicts)"))
        self.assertIn("- `lib/Export.cpp` (Hot spot)", text)
        self.assertEqual(self.repo.git("log", "-1", "--format=%s"), sync_upstream.MARKER_SUBJECT)
        self.assertIn("<<<<<<<", (self.repo.path / "lib/Export.cpp").read_text(encoding="utf-8"))

    def test_workflow_changes_of_upstream_are_taken_out_for_the_token(self):
        self.repo.write(".github/workflows/b.yml", "name: b\n")
        self.repo.commit("ci: b")
        self.repo.git("branch", "-f", "upstream", "main")
        self.repo.upstream_pull(15, "ci", ".github/workflows/new.yml", "name: new\n")
        self.repo.on("upstream")
        self.repo.write(".github/workflows/b.yml", "name: changed\n")
        self.repo.commit("ci: change b")
        self.repo.on("main")
        self.repo.write("bridge/a.cpp", "a\n")
        self.repo.commit("feat(bridge): add a")
        out = self.repo.path.parent / (self.repo.path.name + "-strip")
        sync_upstream.main(["--repo", str(self.repo.path), "--upstream", "upstream", "--main", "main", "--date", "2026-10-27", "--out", str(out)])
        names = self.repo.git("ls-files", ".github")
        self.assertIn("new.yml", names)
        sync_upstream.main(["--repo", str(self.repo.path), "--main", "main", "--date", "2026-10-27", "--out", str(out), "--strip-workflows"])
        self.assertEqual(self.repo.git("ls-files", ".github"), ".github/workflows/b.yml")
        self.assertEqual((self.repo.path / ".github/workflows/b.yml").read_text(encoding="utf-8"), "name: b\n")
        self.assertEqual(self.repo.git("log", "-1", "--format=%s"), sync_upstream.STRIP_SUBJECT)
        self.assertEqual(sync_upstream.stripped_paths(self.repo.path, "HEAD", "main"), [".github/workflows/b.yml", ".github/workflows/new.yml"])
        self.assertIn("### Workflow files not included", (out / "body.md").read_text(encoding="utf-8"))
        self.assertIn("`.github/workflows/new.yml`", (out / "body.md").read_text(encoding="utf-8"))

    def test_main_runs_and_writes_the_state_files(self):
        self.repo.upstream_pull(14, "feature", "lib/Other.cpp", "y\n")
        self.repo.on("main")
        out = self.repo.path.parent / (self.repo.path.name + "-out")
        code = sync_upstream.main(["--repo", str(self.repo.path), "--upstream", "upstream", "--main", "main", "--date", "2026-10-27", "--out", str(out)])
        self.assertEqual(code, 0)
        self.assertEqual(json.loads((out / "state.json").read_text(encoding="utf-8"))["status"], "clean")
        self.assertTrue((out / "body.md").read_text(encoding="utf-8").startswith("### What"))


class ForkDivergenceTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.repo = Repo(Path(self.directory.name))
        self.repo.on("main")

    def tearDown(self):
        self.directory.cleanup()

    def analyze(self):
        return fork_divergence.analyze(self.repo.path, "upstream", "HEAD", "docs/dev/FORK.md")

    def test_documented_divergence_passes(self):
        self.repo.write("bridge/a.cpp", "a\n")
        self.repo.write("lib/Export.cpp", "changed\n")
        self.repo.commit("feat(bridge): add")
        data = self.analyze()
        self.assertEqual((data["undocumented"], data["stale"]), ([], ["docs/dev/FORK.md"]))

    def test_undocumented_file_and_stale_row_are_reported(self):
        self.repo.write("elsewhere/b.cpp", "b\n")
        self.repo.commit("feat(bridge): add")
        data = self.analyze()
        self.assertEqual(data["undocumented"], ["elsewhere/b.cpp"])
        self.assertEqual(sorted(data["stale"]), ["bridge/", "docs/dev/FORK.md", "lib/Export.cpp"])
        self.assertEqual(fork_divergence.main(["--repo", str(self.repo.path), "--upstream", "upstream"]), 1)

    def test_patterns(self):
        self.assertTrue(fork_divergence.matches("a/b/", "a/b/c/d.cpp"))
        self.assertTrue(fork_divergence.matches("a/*/tests/N*.cpp", "a/x/tests/NetBridge.cpp"))
        self.assertFalse(fork_divergence.matches("a/b/", "a/bc/d.cpp"))


if __name__ == "__main__":
    unittest.main()

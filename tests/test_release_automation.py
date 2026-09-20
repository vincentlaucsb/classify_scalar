"""Release gating must never publish PR/fork runs or incomplete CI results."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('release', Path(__file__).resolve().parents[1] / 'tools/publish_release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseGateTests(unittest.TestCase):
    def setUp(self):
        self.sha = 'a' * 40
        self.pr = {'number': 1, 'merged_at': 'today', 'merge_commit_sha': self.sha,
                   'base': {'ref': 'main'}, 'labels': [{'name': 'release'}]}
        self.runs = [{'path': '.github/workflows/' + name, 'head_sha': self.sha,
                      'event': 'push', 'head_branch': 'main', 'id': index,
                      'status': 'completed', 'conclusion': 'success'}
                     for index, name in enumerate(sorted(release.REQUIRED_WORKFLOWS))]

    def test_successful_release(self):
        self.assertEqual(release.select_release([self.pr], self.runs, self.sha), self.pr)

    def test_missing_or_failing_ci(self):
        self.assertIsNone(release.select_release([self.pr], self.runs[:1], self.sha))
        self.runs[1]['conclusion'] = 'failure'
        self.assertIsNone(release.select_release([self.pr], self.runs, self.sha))

    def test_unmerged_or_unlabeled_pr(self):
        self.pr['merged_at'] = None
        self.assertIsNone(release.select_release([self.pr], self.runs, self.sha))
        self.pr['merged_at'] = 'today'
        self.pr['labels'] = []
        self.assertIsNone(release.select_release([self.pr], self.runs, self.sha))

    def test_wrong_commit_and_pr_runs(self):
        self.assertIsNone(release.select_release([self.pr], self.runs, 'b' * 40))
        self.runs[0]['event'] = 'pull_request'
        self.assertIsNone(release.select_release([self.pr], self.runs, self.sha))

    def test_latest_run_must_pass(self):
        newer = dict(self.runs[0], id=100, status='in_progress', conclusion=None)
        self.assertIsNone(release.select_release([self.pr], self.runs + [newer], self.sha))


if __name__ == '__main__':
    unittest.main()

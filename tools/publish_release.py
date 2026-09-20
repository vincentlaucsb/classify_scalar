#!/usr/bin/env python3
"""Publish a merged PR labeled 'release' only after both CI workflows pass."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

REQUIRED_WORKFLOWS = {'cmake-multi-platform.yml', 'gcc-compatibility.yml'}


def gh(*args):
    return subprocess.check_output(['gh', *args], text=True)


def api(path):
    return json.loads(gh('api', path))


def select_release(prs, runs, sha):
    candidates = [pr for pr in prs if pr.get('merged_at')
                  and pr.get('merge_commit_sha') == sha
                  and pr['base']['ref'] == 'main'
                  and any(label['name'] == 'release' for label in pr['labels'])]
    if len(candidates) != 1:
        return None
    for workflow in REQUIRED_WORKFLOWS:
        matching = [run for run in runs if run['path'] == '.github/workflows/' + workflow
                    and run['head_sha'] == sha and run['event'] == 'push'
                    and run['head_branch'] == 'main']
        if not matching:
            return None
        latest = max(matching, key=lambda run: (run['id'], run.get('run_attempt', 1)))
        if latest['status'] != 'completed' or latest['conclusion'] != 'success':
            return None
    return candidates[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    event = json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text(encoding='utf-8'))
    run = event['workflow_run']
    repository = os.environ['GITHUB_REPOSITORY']
    if (run['event'] != 'push' or run['head_branch'] != 'main'
            or run['head_repository']['full_name'] != repository):
        raise SystemExit('Releases require a main-branch push in this repository')
    sha = run['head_sha']
    if not re.fullmatch(r'[0-9a-f]{40}', sha):
        raise SystemExit('Invalid release commit')
    prs = api(f'repos/{repository}/commits/{sha}/pulls?per_page=100')
    runs = api(f'repos/{repository}/actions/runs?head_sha={sha}&event=push&per_page=100')['workflow_runs']
    pr = select_release(prs, runs, sha)
    if pr is None:
        print('No merged release PR with all required workflows passing yet.')
        return
    header = Path('include/classify_scalar.hpp').read_text(encoding='utf-8')
    version = re.search(r'^classify_scalar, version (\d+\.\d+\.\d+)$', header, re.M).group(1)
    tag = 'v' + version
    # Do not move a published version to a new commit on retry or version reuse.
    refs = gh('api', f'repos/{repository}/git/matching-refs/tags/{tag}')
    existing = [ref for ref in json.loads(refs) if ref['ref'] == 'refs/tags/' + tag]
    if existing:
        obj = existing[0]['object']
        while obj['type'] == 'tag':
            obj = api(f"repos/{repository}/git/tags/{obj['sha']}")['object']
        if obj['type'] != 'commit' or obj['sha'] != sha:
            raise SystemExit(f'{tag} already points to a different commit; bump the version')
    print(f"Release {tag} from PR #{pr['number']} at {sha}")
    if args.dry_run:
        return
    if existing:
        releases = json.loads(gh('api', '--paginate', '--slurp', f'repos/{repository}/releases?per_page=100'))
        if any(release['tag_name'] == tag for page in releases for release in page):
            print('Release already published; leaving its description unchanged.')
            return
    with tempfile.TemporaryDirectory() as directory:
        notes = Path(directory) / 'release-notes.md'
        notes.write_text(pr.get('body') or '', encoding='utf-8', newline='\n')
        gh('release', 'create', tag, '--repo', repository, '--target', sha,
           '--title', tag, '--notes-file', str(notes))


if __name__ == '__main__':
    main()

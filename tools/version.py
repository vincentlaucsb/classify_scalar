#!/usr/bin/env python3
"""Check release metadata, or bump every version location with --patch."""
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--patch', action='store_true')
    args = parser.parse_args()
    header = ROOT / 'include/classify_scalar.hpp'
    text = header.read_text(encoding='utf-8')
    version = tuple(int(re.search(r'^#define CLASSIFY_SCALAR_VERSION_' + part + r' (\d+)$',
                                  text, re.M).group(1)) for part in ('MAJOR', 'MINOR', 'PATCH'))
    major, minor, patch = version
    number = major * 10000 + minor * 100 + patch
    dotted = '.'.join(map(str, version))
    locations = {
        header: [(f'classify_scalar, version {dotted}', 1),
                 (f'#if CLASSIFY_SCALAR_VERSION >= {number}', 1),
                 (f'#define CLASSIFY_SCALAR_VERSION {number}', 1)],
        ROOT / 'CMakeLists.txt': [(f'VERSION {dotted}', 1)],
        ROOT / 'tests/test_macro_hygiene.cpp': [
            (f'CLASSIFY_SCALAR_VERSION_MAJOR == {major}', 1),
            (f'CLASSIFY_SCALAR_VERSION_MINOR == {minor}', 1),
            (f'CLASSIFY_SCALAR_VERSION_PATCH == {patch}', 1),
            (f'CLASSIFY_SCALAR_VERSION == {number}', 1)],
    }
    contents = {path: path.read_text(encoding='utf-8') for path in locations}
    for path, checks in locations.items():
        for expected, count in checks:
            if contents[path].count(expected) != count:
                parser.error(f'{path.relative_to(ROOT)}: inconsistent version: expected {expected!r}')
    if args.patch:
        if patch >= 99:
            parser.error('Patch version would overflow the numeric version format')
        updated = f'{major}.{minor}.{patch + 1}'
        contents[header] = contents[header].replace(f'version {dotted}', f'version {updated}')
        contents[header] = contents[header].replace(f'VERSION >= {number}', f'VERSION >= {number + 1}')
        contents[header] = contents[header].replace(f'VERSION {number}', f'VERSION {number + 1}')
        contents[header] = contents[header].replace(f'VERSION_PATCH {patch}', f'VERSION_PATCH {patch + 1}')
        cmake = ROOT / 'CMakeLists.txt'
        contents[cmake] = contents[cmake].replace(f'VERSION {dotted}', f'VERSION {updated}')
        tests = ROOT / 'tests/test_macro_hygiene.cpp'
        contents[tests] = contents[tests].replace(f'VERSION_PATCH == {patch}', f'VERSION_PATCH == {patch + 1}')
        contents[tests] = contents[tests].replace(f'VERSION == {number}', f'VERSION == {number + 1}')
        for path, content in contents.items():
            path.write_text(content, encoding='utf-8', newline='\n')
        print(updated)
    else:
        print(f'Version metadata agrees: {dotted}')


if __name__ == '__main__':
    main()

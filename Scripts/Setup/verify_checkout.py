"""Read-only checkout checks; run after git lfs pull. No Unreal/Python packages required."""
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, help='Optional Unreal installation root containing Engine/')
    args = parser.parse_args()
    problems = []
    for command in (['git', '--version'], ['git', 'lfs', 'version']):
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        if result.returncode:
            problems.append('Unavailable: ' + ' '.join(command))
    result = subprocess.run(['git', 'ls-files', '-z'], cwd=ROOT, capture_output=True)
    if result.returncode:
        problems.append('This directory is not a Git checkout')
    paths = [s.decode('utf-8') for s in result.stdout.split(b'\0') if s]
    seen = {}
    for name in paths:
        folded = name.casefold()
        if folded in seen and seen[folded] != name:
            problems.append('Case collision: ' + name + ' / ' + seen[folded])
        seen[folded] = name
        file = ROOT / name
        if not file.is_file():
            problems.append('Missing tracked file: ' + name)
        elif file.stat().st_size < 1024:
            if file.read_bytes().startswith(b'version https://git-lfs.github.com/spec/v1\n'):
                problems.append('LFS content not downloaded: ' + name)
    project = json.loads((ROOT / 'Altai.uproject').read_text(encoding='utf-8'))
    for module in project['Modules']:
        name = module['Name']
        if not (ROOT / 'Source' / name / (name + '.Build.cs')).is_file():
            problems.append('Missing module: ' + name)
    if args.engine:
        engine = args.engine.expanduser().resolve() / 'Engine'
        version_file = engine / 'Build/Build.version'
        if not version_file.is_file():
            problems.append('Engine/Build/Build.version not found')
        else:
            v = json.loads(version_file.read_text(encoding='utf-8'))
            actual = tuple(v[k] for k in ('MajorVersion', 'MinorVersion', 'PatchVersion'))
            if actual != (5, 8, 2):
                problems.append('Expected UE 5.8.2, found ' + '.'.join(map(str, actual)))
        plugins = {p.stem for p in (engine / 'Plugins').rglob('*.uplugin')}
        plugins.update(p.stem for p in (ROOT / 'Plugins').rglob('*.uplugin'))
        for plugin in project['Plugins']:
            if plugin.get('Enabled') and plugin['Name'] not in plugins:
                problems.append('Missing enabled plugin: ' + plugin['Name'])
    if problems:
        print('\n'.join('ERROR: ' + p for p in problems))
        return 1
    print(f'OK: {len(paths)} tracked files; no missing files, LFS pointers or case collisions.')
    print('This checks checkout integrity, not C++ compilation or runtime behavior.')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError) as error:
        print('ERROR:', error)
        raise SystemExit(1)

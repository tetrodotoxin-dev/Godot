#!/usr/bin/env python3
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Build a local /demo package with Web providers, source snapshots and notices.

The editor stages the native addon while importing the project. The exported
pack contains the Web modules built from those same owners. Nothing is deployed.
"""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

REPO = Path(__file__).resolve().parents[1]
WEB_CONFIG = ['--config=release', '--config=web']


def bazel_files(targets, configuration):
    """Read declared artifacts from Bazel instead of duplicating output paths."""
    root = Path(subprocess.check_output(
        ['bazel', 'info', 'execution_root'], cwd=REPO, text=True).strip())
    paths = subprocess.check_output(
        ['bazel', 'cquery', 'set(' + ' '.join(targets) + ')', '--output=files',
         *configuration], cwd=REPO, text=True)
    # Source repositories live outside the action symlink forest. A query
    # alone need not repopulate those symlinks after another configuration ran.
    external = Path(subprocess.check_output(
        ['bazel', 'info', 'output_base'], cwd=REPO, text=True).strip())
    return [(external if path.startswith('external/') else root) / path
            for path in paths.splitlines()]


def write_archive(target, entries):
    # Normalize container metadata so identical source bytes produce identical
    # archives, regardless of checkout timestamps and the exporting user's ID.
    with target.open('wb') as output:
        with gzip.GzipFile(filename='', mode='wb', fileobj=output, mtime=0) as zipped:
            with tarfile.open(fileobj=zipped, mode='w') as archive:
                for name, data, executable in entries:
                    info = tarfile.TarInfo(name)
                    info.size = len(data)
                    info.mode = 0o755 if executable else 0o644
                    archive.addfile(info, io.BytesIO(data))


def publish_sources(demo, engine_source, template):
    # Only product inputs belong in the public source download. Git supplies
    # tracked and new source paths, while this allowlist excludes local notes,
    # credentials, build outputs and unrelated toolchain applications.
    roots = {
        'godot': ('extension', 'demo'),
        'tetrodotoxin': ('perimortem', 'ttx', 'toolchain', 'validation'),
        'cuda': ('cuda', 'build', 'validation'),
    }
    root_files = {'.bazelrc', '.bazelversion', 'BUILD', 'BUILD.bazel',
                  'MODULE.bazel', 'MODULE.bazel.lock', 'LICENSE'}
    entries = []
    manifest = {'repositories': {}, 'template_sha256': hashlib.sha256(
        template.read_bytes()).hexdigest()}
    snapshot = None
    if not (REPO / '.git').exists():
        snapshot = json.loads((REPO.parent / 'manifest.json').read_text())
    for name, directories in roots.items():
        repo = REPO.parent / name
        if snapshot:
            paths = [item['path'] for item in snapshot['repositories'][name]['files']]
            head = snapshot['repositories'][name]['head']
        else:
            paths = subprocess.check_output(
                ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
                cwd=repo).decode().split('\0')
            head = subprocess.check_output(
                ['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip()
        files = []
        for name_in_repo in sorted(set(paths) - {''}):
            path = repo / name_in_repo
            if (name_in_repo not in root_files and
                    Path(name_in_repo).parts[0] not in directories):
                continue
            if not path.is_file():
                continue
            data = path.read_bytes()
            entries.append((f'{name}/{name_in_repo}', data, bool(path.stat().st_mode & 0o111)))
            files.append({'path': name_in_repo, 'sha256': hashlib.sha256(data).hexdigest()})
        manifest['repositories'][name] = {
            'head': head,
            'files': files,
        }

    source = demo / 'source'
    source.mkdir()
    metadata = (json.dumps(manifest, indent=2) + '\n').encode()
    # The archive can rebuild and re-export without carrying private Git state.
    # Its manifest supplies that snapshot's file inventory and original revisions.
    entries.append(('manifest.json', metadata, False))
    write_archive(source / 'lab-source.tar.gz', entries)
    (source / 'manifest.json').write_bytes(metadata)

    pocketfft = bazel_files([
        '@pocketfft//:LICENSE.md', '@pocketfft//:pocketfft_hdronly.h',
    ], WEB_CONFIG)
    sdk_license, = bazel_files(['@godot_cpp_web//:LICENSE.md'], WEB_CONFIG)
    write_archive(source / 'pocketfft.tar.gz', [
        ('pocketfft/' + path.name, path.read_bytes(), False)
        for path in sorted(pocketfft)
    ])
    notices = {
        'Lab-MIT.txt': REPO / 'LICENSE',
        'TTX-MIT.txt': REPO.parent / 'tetrodotoxin/LICENSE',
        'Godot-MIT.txt': engine_source / 'LICENSE.txt',
        'Godot.txt': engine_source / 'COPYRIGHT.txt',
        'Godot-CPP-MIT.txt': sdk_license,
        'PocketFFT-BSD.txt': next(path for path in pocketfft if path.name == 'LICENSE.md'),
    }
    licenses = demo / 'licenses'
    licenses.mkdir()
    for name, path in notices.items():
        shutil.copyfile(path, licenses / name)


def export_project(project, demo, template, modules, native_addon):
    shutil.copytree(REPO / 'demo', project, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('.godot', 'addons', 'export_presets.cfg'))
    addon = project / 'addons/godot_ttx'
    addon.mkdir(parents=True)
    subprocess.run(['tar', '-xf', str(native_addon), '-C', str(addon)], check=True)
    for module in modules:
        shutil.copyfile(module, addon / module.name)

    # Imports owns alias resolution on both hosts. This deployment publishes
    # CPU implementations. CUDA requests reach the ordinary unavailable result.
    imports = {name: 'lib' + artifact + '.wasm' for name, artifact in {
        'counter': 'counter_extension', 'sampler': 'sampler_extension',
        'cpu': 'cpu_provider',
    }.items()}
    settings = project / 'project.godot'
    settings.write_text(settings.read_text().replace('[ttx]\n',
        '[ttx]\nimports.web=' + json.dumps(imports) + '\n'))
    (project / 'export_presets.cfg').write_text('''[preset.0]
name="Web"
platform="Web"
runnable=true
export_filter="all_resources"
include_filter=""
exclude_filter="addons/godot_ttx/*.so,preview.gif"
[preset.0.options]
custom_template/release=''' + json.dumps(str(template)) + '''
variant/extensions_support=true
variant/thread_support=false
html/export_icon=false
html/canvas_resize_policy=2
progressive_web_app/enabled=false
''')
    subprocess.run(['godot', '--headless', '--editor', '--path', str(project),
                    '--frame-delay', '1000', '--quit'], check=True)
    subprocess.run(['godot', '--headless', '--path', str(project),
                    '--export-release', 'Web', str(demo / 'index.html')], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--template', required=True, type=Path,
                        help='Web template built with web_engine.py, threads=no and dlink_enabled=yes')
    parser.add_argument('--engine-source', required=True, type=Path,
                        help='Matching Godot source tree, supplying engine and dependency notices')
    parser.add_argument('--output', required=True, type=Path,
                        help='New asset root containing demo/ and _headers')
    parser.add_argument('--work-dir', required=True, type=Path)
    args = parser.parse_args()
    template = args.template.resolve(strict=True)
    engine_source = args.engine_source.resolve(strict=True)
    output = args.output.resolve()
    if output.exists():
        parser.error('--output must name a new directory')
    for notice in ('LICENSE.txt', 'COPYRIGHT.txt'):
        (engine_source / notice).resolve(strict=True)
    args.work_dir.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)

    subprocess.run(['bazel', 'build', '//demo:addon', '--config=release'], cwd=REPO, check=True)
    native_addon, = bazel_files(['//demo:addon'], ['--config=release'])
    subprocess.run(['bazel', 'build', '//demo:web_payload', *WEB_CONFIG], cwd=REPO, check=True)
    modules = bazel_files(['//demo:web_payload'], WEB_CONFIG)

    # A failed build or export leaves the requested output absent. All generated
    # state belongs to these temporary directories and can be discarded together.
    with tempfile.TemporaryDirectory(prefix='project-', dir=args.work_dir) as temporary:
        with tempfile.TemporaryDirectory(prefix='web-', dir=output.parent) as staging:
            package = Path(staging) / 'assets'
            demo = package / 'demo'
            demo.mkdir(parents=True)
            export_project(Path(temporary), demo, template, modules, native_addon)
            publish_sources(demo, engine_source, template)
            shutil.copyfile(REPO / 'demo/build/web_headers', package / '_headers')
            package.rename(output)
    print('Web lab and source package exported to', output)


if __name__ == '__main__':
    main()

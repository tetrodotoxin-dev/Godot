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
BAZEL_ARGS = []
CANDIDATE_ROOT = None


def bazel_files(targets, configuration):
    """Read declared artifacts from Bazel instead of duplicating output paths."""
    root = Path(subprocess.check_output(
        ['bazel', 'info', 'execution_root', *BAZEL_ARGS], cwd=REPO, text=True).strip())
    paths = subprocess.check_output(
        ['bazel', 'cquery', 'set(' + ' '.join(targets) + ')', '--output=files',
         *configuration, *BAZEL_ARGS], cwd=REPO, text=True)
    # Source repositories live outside the action symlink forest. A query
    # alone need not repopulate those symlinks after another configuration ran.
    external = Path(subprocess.check_output(
        ['bazel', 'info', 'output_base', *BAZEL_ARGS], cwd=REPO, text=True).strip())
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
    root_files = {'.bazelrc', '.bazelversion', 'BUILD', 'BUILD.bazel',
                  'MODULE.bazel', 'MODULE.bazel.lock', 'LICENSE', 'README.md'}
    entries = []
    manifest = {'repositories': {}, 'template_sha256': hashlib.sha256(
        template.read_bytes()).hexdigest()}
    repositories = {'godot': REPO}
    if CANDIDATE_ROOT:
        repositories.update({
            name: CANDIDATE_ROOT / name
            for name in ('ttx', 'coherence', 'compute')
        })
    for name, repository in repositories.items():
        if (repository / '.git').exists():
            paths = subprocess.check_output(
                ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
                cwd=repository).decode().split('\0')
            revision = subprocess.run(
                ['git', 'rev-parse', 'HEAD'], cwd=repository, text=True,
                capture_output=True)
            head = revision.stdout.strip() if revision.returncode == 0 else 'unborn'
        else:
            snapshot = json.loads((repository.parent / 'manifest.json').read_text())
            recorded = snapshot['repositories'][name]
            paths = [item['path'] for item in recorded['files']]
            head = recorded['head']
        files = []
        for relative in sorted(set(paths) - {''}):
            path = repository / relative
            if relative not in root_files and Path(relative).parts[0] not in {'source', 'example', 'tools', 'build', 'validation'}:
                continue
            if not path.is_file() or '__pycache__' in path.parts:
                continue
            data = path.read_bytes()
            entries.append((name + '/' + relative, data, bool(path.stat().st_mode & 0o111)))
            files.append({'path': relative, 'sha256': hashlib.sha256(data).hexdigest()})
        manifest['repositories'][name] = {'head': head, 'files': files}
    manifest['candidate_dependencies'] = bool(CANDIDATE_ROOT)

    source = demo / 'source'
    source.mkdir()
    metadata = (json.dumps(manifest, indent=2) + '\n').encode()
    # The archive can rebuild and re-export without carrying private Git state.
    # Its manifest supplies that snapshot's file inventory and original revisions.
    entries.append(('manifest.json', metadata, False))
    write_archive(source / 'lab-source.tar.gz', entries)
    (source / 'manifest.json').write_bytes(metadata)

    sdk_license, = bazel_files(['@godot_cpp_web//:LICENSE.md'], WEB_CONFIG)
    notices = {
        'Lab-MIT.txt': REPO / 'LICENSE',
        'Godot-MIT.txt': engine_source / 'LICENSE.txt',
        'Godot.txt': engine_source / 'COPYRIGHT.txt',
        'Godot-CPP-MIT.txt': sdk_license,
        'Coherence.txt': REPO / 'source/godot/licenses/Coherence.txt',
        'TTX.txt': REPO / 'source/godot/licenses/TTX.txt',
        'Perimortem.txt': REPO / 'source/godot/licenses/Perimortem.txt',
        'Compute.txt': REPO / 'source/godot/licenses/Compute.txt',
    }
    licenses = demo / 'licenses'
    licenses.mkdir()
    for name, path in notices.items():
        shutil.copyfile(path, licenses / name)


def export_project(project, demo, template, modules, native_addon):
    shutil.copytree(REPO / 'example', project, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('.godot', 'addons', 'export_presets.cfg'))
    addon = project / 'addons/godot_ttx'
    addon.mkdir(parents=True)
    subprocess.run(['tar', '-xf', str(native_addon), '-C', str(project)], check=True)
    for module in modules:
        name = module.name
        if module.suffix == '.json':
            document = json.loads(module.read_text())
            if isinstance(document, dict) and isinstance(document.get('library'), str):
                name = document['library'] + '.json'
        destination = addon / 'web' / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(module, destination)

    # The checked-in project declares native and Web provider paths. The
    # bridge exporter stages those configured modules beside the Web engine.
    (project / 'export_presets.cfg').write_text('''[preset.0]
name="Web"
platform="Web"
runnable=true
export_filter="all_resources"
include_filter=""
exclude_filter="addons/godot_ttx/lib*.so,preview.gif"
[preset.0.options]
custom_template/release=''' + json.dumps(str(template)) + '''
variant/extensions_support=true
variant/thread_support=false
html/export_icon=false
html/canvas_resize_policy=2
progressive_web_app/enabled=false
''')
    # Godot's first headless import can destroy editor documentation before its
    # deferred extension callback runs (godotengine/godot#111048). Frame pacing
    # gives that callback time to finish before import shuts the editor down.
    run_godot(project, demo, ['--editor', '--import', '--frame-delay', '1000'])
    run_godot(project, demo, ['--export-release', 'Web', str(demo / 'index.html')])


def run_godot(project, demo, arguments):
    # Export and import need no GPU. A minimal /dev keeps editor startup from
    # initializing accelerator drivers while preserving the real Godot path.
    isolation = ['bwrap', '--ro-bind', '/', '/', '--dev', '/dev', '--proc', '/proc',
                 '--bind', '/tmp', '/tmp', '--bind', str(project), str(project),
                 '--bind', str(demo), str(demo), '--unshare-pid', '--die-with-parent',
                 '--setenv', 'XDG_CONFIG_HOME', str(project / '.config'),
                 '--setenv', 'XDG_DATA_HOME', str(project / '.data'),
                 '--setenv', 'XDG_CACHE_HOME', str(project / '.cache'), '--']
    subprocess.run([*isolation, 'godot', '--headless', '--path', str(project),
                    *arguments], check=True, timeout=120)


def main():
    global BAZEL_ARGS, CANDIDATE_ROOT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel-arg", action="append", default=[], help="Explicit Bazel arguments for an approved local SDK experiment")
    parser.add_argument('--template', required=True, type=Path,
                        help='Web template built with web_engine.py, threads=no and dlink_enabled=yes')
    parser.add_argument('--engine-source', required=True, type=Path,
                        help='Matching Godot source tree, supplying engine and dependency notices')
    parser.add_argument('--output', required=True, type=Path,
                        help='New asset root containing demo/ and _headers')
    parser.add_argument('--work-dir', required=True, type=Path)
    parser.add_argument('--candidate-root', type=Path,
                        help='Explicit sibling source snapshots for an unpublished TTX/Compute demo candidate')
    args = parser.parse_args()
    BAZEL_ARGS = args.bazel_arg
    if args.candidate_root:
        CANDIDATE_ROOT = args.candidate_root.resolve(strict=True)
        for name in ('ttx', 'coherence', 'compute'):
            repository = CANDIDATE_ROOT / name
            subprocess.run(['bazel', 'build', ':sdk', '--config=release', *BAZEL_ARGS],
                           cwd=repository, check=True)
            directory = subprocess.check_output(
                ['bazel', 'info', 'bazel-bin', '--config=release', *BAZEL_ARGS],
                cwd=repository, text=True).strip()
            BAZEL_ARGS.append('--distdir=' + directory)
    template = args.template.resolve(strict=True)
    engine_source = args.engine_source.resolve(strict=True)
    output = args.output.resolve()
    if output.exists():
        parser.error('--output must name a new directory')
    for notice in ('LICENSE.txt', 'COPYRIGHT.txt'):
        (engine_source / notice).resolve(strict=True)
    args.work_dir.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)

    native_configuration = ['--config=release', '--config=cuda']
    subprocess.run(['bazel', 'build', '//:demo_addon', *native_configuration, *BAZEL_ARGS], cwd=REPO, check=True)
    native_addon, = bazel_files(['//:demo_addon'], native_configuration)
    subprocess.run(['bazel', 'build', '//:demo_web_payload', *WEB_CONFIG, *BAZEL_ARGS], cwd=REPO, check=True)
    modules = bazel_files(['//:demo_web_payload'], WEB_CONFIG)

    # A failed build or export leaves the requested output absent. All generated
    # state belongs to these temporary directories and can be discarded together.
    with tempfile.TemporaryDirectory(prefix='project-', dir=args.work_dir) as temporary:
        with tempfile.TemporaryDirectory(prefix='web-', dir=output.parent) as staging:
            package = Path(staging) / 'assets'
            demo = package / 'demo'
            demo.mkdir(parents=True)
            export_project(Path(temporary), demo, template, modules, native_addon)
            publish_sources(demo, engine_source, template)
            shutil.copyfile(REPO / 'tools/web_headers', package / '_headers')
            package.rename(output)
    print('Web lab and source package exported to', output)


if __name__ == '__main__':
    main()

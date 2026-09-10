#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
publish=false
version=""

usage() {
    printf '%s\n' \
        'Usage: tools/release_from_fedora.sh [--publish] [VERSION]' \
        '' \
        'Without --publish, builds/tests Linux and prints the GitHub release plan.' \
        'With --publish, pushes main and an annotated vVERSION tag. GitHub Actions' \
        'then builds both the Windows EXE and Linux AppImage and publishes them.'
}

while (($#)); do
    case "$1" in
        --publish) publish=true ;;
        --help|-h) usage; exit 0 ;;
        --*) printf 'Unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
        *)
            if [[ -n "$version" ]]; then
                printf 'Only one version argument is allowed.\n' >&2
                exit 2
            fi
            version="${1#v}"
            ;;
    esac
    shift
done

for command_name in cmake ctest git ninja; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        printf 'Required command is missing: %s\n' "$command_name" >&2
        exit 1
    fi
done

cmake_version="$(sed -nE 's/^project\(PS2BatchRenamer VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$repo_root/CMakeLists.txt")"
if [[ -z "$cmake_version" ]]; then
    printf 'Could not read the project version from CMakeLists.txt.\n' >&2
    exit 1
fi
version="${version:-$cmake_version}"
tag="v$version"
if [[ "$version" != "$cmake_version" ]]; then
    printf 'Requested version %s does not match CMake project version %s.\n' "$version" "$cmake_version" >&2
    exit 1
fi
if ! grep -Fq "# PS2 Batch Renamer V$version" "$repo_root/RELEASE_NOTES_V4.md"; then
    printf 'RELEASE_NOTES_V4.md is not headed for V%s.\n' "$version" >&2
    exit 1
fi

branch="$(git -C "$repo_root" branch --show-current)"
if [[ "$branch" != "main" ]]; then
    printf 'Release from main, not %s. Merge the audited branch first.\n' "${branch:-detached HEAD}" >&2
    exit 1
fi
if [[ -n "$(git -C "$repo_root" status --porcelain)" ]]; then
    printf 'The worktree is not clean. Commit or stash changes before releasing.\n' >&2
    git -C "$repo_root" status --short >&2
    exit 1
fi

printf 'Running the Fedora/Linux release build for %s...\n' "$tag"
cmake -S "$repo_root" -B "$repo_root/build-release-linux" --fresh -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DPS2BR_BUILD_GUI=ON \
    -DPS2BR_BUILD_TESTS=ON \
    -DPS2BR_ENABLE_CHD=ON \
    -DPS2BR_ENABLE_ONLINE_DATABASE=ON
cmake --build "$repo_root/build-release-linux" --parallel
ctest --test-dir "$repo_root/build-release-linux" --output-on-failure

git -C "$repo_root" fetch --tags origin
if git -C "$repo_root" rev-parse -q --verify "refs/tags/$tag" >/dev/null ||
   git -C "$repo_root" ls-remote --exit-code --tags origin "refs/tags/$tag" >/dev/null 2>&1; then
    printf 'Tag %s already exists locally or on origin; refusing to replace it.\n' "$tag" >&2
    exit 1
fi
if ! git -C "$repo_root" merge-base --is-ancestor origin/main HEAD; then
    printf 'Local main has diverged from origin/main. Reconcile it before releasing.\n' >&2
    exit 1
fi

if [[ "$publish" != true ]]; then
    printf '\nChecks passed. No GitHub changes were made.\n'
    printf 'Run this to publish when ready:\n  tools/release_from_fedora.sh --publish %s\n' "$version"
    printf 'That pushes main and %s; Actions builds Windows and Linux, then creates the release.\n' "$tag"
    exit 0
fi

git -C "$repo_root" push origin HEAD:main
git -C "$repo_root" tag -a "$tag" -m "PS2 Batch Renamer $tag"
git -C "$repo_root" push origin "$tag"

printf '\nPublished %s. GitHub Actions is now responsible for both platform builds and the release.\n' "$tag"

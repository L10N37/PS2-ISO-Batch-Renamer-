#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"
release_version=""
base_ref=""
replace_reports=false

usage() {
    printf '%s\n' \
        'Usage: tools/update_database.sh [--base-ref GIT_REF] [--replace-reports] VERSION' \
        '' \
        'VERSION names new audit artifacts (for example, 4.2.0).' \
        'Existing report paths are protected unless --replace-reports is explicit.' \
        '--base-ref reads gameid.txt from a historical commit/tag for reproducibility.'
}

while (($#)); do
    case "$1" in
        --base-ref)
            shift
            if (($# == 0)); then
                printf '%s\n' '--base-ref requires a Git revision.' >&2
                exit 2
            fi
            base_ref="$1"
            ;;
        --replace-reports) replace_reports=true ;;
        --help|-h) usage; exit 0 ;;
        --*) printf 'Unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
        *)
            if [[ -n "$release_version" ]]; then
                printf 'Only one version argument is allowed.\n' >&2
                exit 2
            fi
            release_version="${1#v}"
            ;;
    esac
    shift
done

if [[ ! "$release_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    printf 'A semantic VERSION such as 4.2.0 is required.\n' >&2
    usage >&2
    exit 2
fi
if [[ -n "$base_ref" && ! "$base_ref" =~ ^[A-Za-z0-9._/-]+$ ]]; then
    printf 'Unsafe or unsupported Git revision: %s\n' "$base_ref" >&2
    exit 2
fi

audit_report="$repo_root/docs/database-audit-v${release_version}.md"
change_report="$repo_root/docs/database-changes-v${release_version}.tsv"
review_report="$repo_root/docs/database-review-v${release_version}.tsv"
if [[ "$replace_reports" != true ]]; then
    for protected_path in "$audit_report" "$change_report" "$review_report"; do
        if [[ -e "$protected_path" ]]; then
            printf 'Refusing to overwrite historical audit artifact: %s\n' "$protected_path" >&2
            printf 'Use a new VERSION, or explicitly pass --replace-reports with the intended --base-ref.\n' >&2
            exit 1
        fi
    done
fi

source_root="$(mktemp -d -t ps2br-database-audit.XXXXXX)"

cleanup() {
    case "$source_root" in
        /tmp/ps2br-database-audit.*) rm -rf -- "$source_root" ;;
        *) printf 'Refusing to remove unexpected temporary path: %s\n' "$source_root" >&2 ;;
    esac
}
trap cleanup EXIT

for command_name in git python3; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        printf 'Required command is missing: %s\n' "$command_name" >&2
        exit 1
    fi
done

base_database="$repo_root/gameid.txt"
if [[ -n "$base_ref" ]]; then
    base_database="$source_root/base-gameid.txt"
    if ! git -C "$repo_root" show "${base_ref}:gameid.txt" >"$base_database"; then
        printf 'Could not read gameid.txt from Git revision %s.\n' "$base_ref" >&2
        exit 1
    fi
fi
printf 'Auditing from base database: %s\n' "${base_ref:-$base_database}"

clone_sparse() {
    local url="$1"
    local destination="$2"
    shift 2
    printf 'Downloading %s\n' "$url"
    git clone --quiet --depth 1 --filter=blob:none --sparse "$url" "$destination"
    git -C "$destination" sparse-checkout set "$@"
}

clone_sparse https://github.com/niemasd/GameDB-PS2.git "$source_root/gamedb" games
clone_sparse https://github.com/GDX-X/PFS-BatchKit-Manager.git "$source_root/gdx" \
    PFS-BatchKit-Manager/BAT/TitlesDB/TitlesDB_PS2_English.txt
clone_sparse https://github.com/PCSX2/pcsx2.git "$source_root/pcsx2" bin/resources/GameIndex.yaml
clone_sparse https://github.com/Zarh/Get_CONFIG.git "$source_root/get-config" get_title/PS2.txt

python3 "$script_dir/audit_database.py" \
    --base "$base_database" \
    --gamedb "$source_root/gamedb" \
    --gdx "$source_root/gdx/PFS-BatchKit-Manager/BAT/TitlesDB/TitlesDB_PS2_English.txt" \
    --pcsx2 "$source_root/pcsx2/bin/resources/GameIndex.yaml" \
    --get-config "$source_root/get-config/get_title/PS2.txt" \
    --reviewed "$script_dir/reviewed_titles.tsv" \
    --translations "$script_dir/verified_translations.tsv" \
    --output "$repo_root/gameid.txt" \
    --report "$audit_report" \
    --changes "$change_report" \
    --review-queue "$review_report" \
    --release-version "$release_version"

git -C "$repo_root" diff --check
printf '\nDatabase audit complete. Review these files before committing:\n'
git -C "$repo_root" status --short -- \
    gameid.txt \
    "docs/database-audit-v${release_version}.md" \
    "docs/database-changes-v${release_version}.tsv" \
    "docs/database-review-v${release_version}.tsv" \
    tools/reviewed_titles.tsv \
    tools/verified_translations.tsv

#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd "$SCRIPT_DIR/../.." && pwd)
VERSION=$(sed -n '1p' "$REPO_ROOT/VERSION")
RELEASE_TAG=$(sed -n '1p' "$SCRIPT_DIR/UPSTREAM_TAG")
RELEASE_VERSION=${RELEASE_TAG#v}
ARCHIVE_ROOT="hum-$RELEASE_VERSION"
RECIPE_DIR="$SCRIPT_DIR/app-misc/hum"
TEMPLATE="$RECIPE_DIR/hum-$VERSION.recipe.in"
RECIPE="$RECIPE_DIR/hum-$VERSION.recipe"
PACKAGE="hum-$VERSION"
SOURCE_URL="https://github.com/sikosis/hum/archive/refs/tags/$RELEASE_TAG.tar.gz"

fail()
{
	printf 'hum-port: %s\n' "$*" >&2
	exit 1
}

usage()
{
	cat <<EOF
Usage: sh packaging/haikuports/hum-port.sh <command> [arguments]

Commands:
  check                 Run Make and CMake builds and their test suites
  finalize              Fetch the published $RELEASE_TAG archive and generate its recipe checksum
  stage HAIKUPORTS_PATH  Copy the finalized recipe into a HaikuPorts checkout
  haiku-test [PATH]     Stage, strictly build, and test the recipe on Haiku

This script never creates or pushes a release tag, changes a HaikuPorts branch,
or submits a pull request.
EOF
}

validateVersion()
{
	case "$VERSION" in
		''|*[!0-9.]*|.*|*.|*..*) fail "VERSION must be a dotted numeric version; found '$VERSION'" ;;
	esac
	case "$RELEASE_TAG" in
		v[0-9]*.[0-9]*|v[0-9]*.[0-9]*.[0-9]*) ;;
		*) fail "UPSTREAM_TAG must be a numeric version tag such as v0.4.0" ;;
	esac
	case "$RELEASE_VERSION" in
		"$VERSION"|"$VERSION.0") ;;
		*) fail "upstream tag $RELEASE_TAG does not match project VERSION $VERSION" ;;
	esac
}

check()
{
	command -v make >/dev/null 2>&1 || fail "make is required"
	make -C "$REPO_ROOT" test
	command -v cmake >/dev/null 2>&1 || fail "cmake is required"
	command -v ctest >/dev/null 2>&1 || fail "ctest is required"

	build_dir=$(mktemp -d "${TMPDIR:-/tmp}/hum-haikuports.XXXXXX") || fail "could not create temporary build directory"
	trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
	cmake -S "$REPO_ROOT" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
	cmake --build "$build_dir" --parallel
	ctest --test-dir "$build_dir" --output-on-failure
	rm -rf "$build_dir"
	trap - EXIT HUP INT TERM
}

finalize()
{
	[ -f "$TEMPLATE" ] || fail "missing recipe template: $TEMPLATE"
	[ ! -e "$RECIPE" ] || fail "$RECIPE already exists; inspect it instead of overwriting it"
	command -v git >/dev/null 2>&1 || fail "git is required to confirm the release tag"
	command -v curl >/dev/null 2>&1 || fail "curl is required to fetch the release archive"
	command -v tar >/dev/null 2>&1 || fail "tar is required to inspect the release archive"
	git ls-remote --exit-code https://github.com/sikosis/hum.git "refs/tags/$RELEASE_TAG" >/dev/null \
		|| fail "remote tag $RELEASE_TAG is not published yet; publish the release before finalizing"

	work_dir=$(mktemp -d "${TMPDIR:-/tmp}/hum-port-finalize.XXXXXX") || fail "could not create temporary directory"
	trap 'rm -rf "$work_dir"' EXIT HUP INT TERM
	archive="$work_dir/hum-$RELEASE_VERSION.tar.gz"
	curl --fail --location --silent --show-error "$SOURCE_URL" --output "$archive"
	archive_version=$(tar -xOf "$archive" "$ARCHIVE_ROOT/VERSION" | sed -n '1p')
	case "$archive_version" in
		"$VERSION"|"$VERSION.0") ;;
		*) fail "tag archive VERSION is '$archive_version', expected '$VERSION' or '$VERSION.0'" ;;
	esac
	if command -v sha256sum >/dev/null 2>&1; then
		checksum=$(sha256sum "$archive" | awk '{print $1}')
	elif command -v shasum >/dev/null 2>&1; then
		checksum=$(shasum -a 256 "$archive" | awk '{print $1}')
	else
		fail "sha256sum or shasum is required to calculate the source checksum"
	fi
	case "$checksum" in
		????????????????????????????????????????????????????????????????) ;;
		*) fail "could not calculate a SHA-256 checksum" ;;
	esac
	sed \
		-e "s|@SOURCE_URI@|$SOURCE_URL|" \
		-e "s|@SOURCE_FILENAME@|hum-$RELEASE_VERSION.tar.gz|" \
		-e "s|@SOURCE_DIR@|$ARCHIVE_ROOT|" \
		-e "s/@CHECKSUM_SHA256@/$checksum/" "$TEMPLATE" > "$RECIPE"
	if grep -Eq '@(SOURCE_URI|SOURCE_FILENAME|SOURCE_DIR|CHECKSUM_SHA256)@' "$RECIPE"; then
		rm -f "$RECIPE"
		fail "recipe still contains an unfilled checksum placeholder"
	fi
	printf 'Created %s\nSHA-256: %s\n' "$RECIPE" "$checksum"
	printf 'The release tag must remain immutable; do not move or replace %s.\n' "$RELEASE_TAG"
	rm -rf "$work_dir"
	trap - EXIT HUP INT TERM
}

stage()
{
	[ "$#" -eq 1 ] || fail "stage requires the path to a HaikuPorts checkout"
	ports_path=$1
	[ -d "$ports_path/.git" ] || [ -f "$ports_path/.git" ] || fail "not a Git checkout: $ports_path"
	[ -f "$RECIPE" ] || fail "run 'finalize' first to create $RECIPE"
	target_dir="$ports_path/app-misc/hum"
	target="$target_dir/hum-$VERSION.recipe"
	mkdir -p "$target_dir"
	if [ -e "$target" ]; then
		cmp -s "$RECIPE" "$target" || fail "refusing to overwrite existing recipe: $target"
		printf 'Recipe is already staged at %s\n' "$target"
	else
		cp "$RECIPE" "$target"
		printf 'Staged %s\n' "$target"
	fi
	stagePatches "$target_dir"
}

stagePatches()
{
	patches_dir="$RECIPE_DIR/patches"
	[ -d "$patches_dir" ] || return 0
	target_patches_dir="$1/patches"
	mkdir -p "$target_patches_dir"
	for patch_file in "$patches_dir"/*; do
		[ -f "$patch_file" ] || continue
		cp "$patch_file" "$target_patches_dir/"
		printf 'Staged %s\n' "$target_patches_dir/$(basename "$patch_file")"
	done
}

haikuTest()
{
	[ "$#" -le 1 ] || fail "haiku-test accepts at most one HaikuPorts checkout path"
	ports_path=${1:-}
	if [ -z "$ports_path" ] && [ -f "$HOME/config/settings/haikuports.conf" ]; then
		ports_path=$(awk -F= '
			/^[[:space:]]*TREE_PATH=/ {
				value = $2
				gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
				gsub(/^"|"$/, "", value)
				print value
				exit
			}' "$HOME/config/settings/haikuports.conf")
	fi
	ports_path=${ports_path:-"$HOME/haikuports"}
	[ -d "$ports_path" ] || fail "HaikuPorts checkout not found: $ports_path"

	haikuporter=${HAIKUPORTER:-}
	if [ -z "$haikuporter" ] && command -v haikuporter >/dev/null 2>&1; then
		haikuporter=$(command -v haikuporter)
	fi
	if [ -z "$haikuporter" ]; then
		for candidate in "$HOME/haikuporter/haikuporter" \
			"$(dirname "$ports_path")/haikuporter/haikuporter" \
			/boot/home/haikuporter/haikuporter; do
			if [ -x "$candidate" ]; then
				haikuporter=$candidate
				break
			fi
		done
	fi
	[ -n "$haikuporter" ] && [ -x "$haikuporter" ] \
		|| fail "HaikuPorter not found; set HAIKUPORTER to its executable"

	[ -f "$RECIPE" ] || fail "missing finalized recipe: $RECIPE"
	target_dir="$ports_path/app-misc/hum"
	target="$target_dir/hum-$VERSION.recipe"
	mkdir -p "$target_dir"
	cp "$RECIPE" "$target"
	printf 'Staged %s\n' "$target"
	stagePatches "$target_dir"
	printf 'Using HaikuPorter: %s\n' "$haikuporter"

	cd "$ports_path"
	"$haikuporter" --clean "$PACKAGE"
	"$haikuporter" -S --test "$PACKAGE"
	printf 'Build and tests completed. Packages are in %s/packages.\n' "$ports_path"
}

validateVersion
[ "$#" -ge 1 ] || { usage; exit 2; }
command=$1
shift
case "$command" in
	check) [ "$#" -eq 0 ] || fail "check takes no arguments"; check ;;
	finalize) [ "$#" -eq 0 ] || fail "finalize takes no arguments"; finalize ;;
	stage) stage "$@" ;;
	haiku-test) haikuTest "$@" ;;
	help|-h|--help) usage ;;
	*) usage >&2; fail "unknown command: $command" ;;
esac

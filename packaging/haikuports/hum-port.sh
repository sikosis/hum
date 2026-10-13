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
PROJECT_REPOSITORY=${HUM_GITHUB_REPOSITORY:-sikosis/hum}
SUBMISSION_BRANCH="hum-$VERSION"
SOURCE_URL="https://github.com/$PROJECT_REPOSITORY/archive/refs/tags/$RELEASE_TAG.tar.gz"

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
  submit                Build in an isolated tree, push a branch, and open the HaikuPorts PR

The submit command prompts for a GitHub token when GITHUB_TOKEN is not set. It
never creates, moves, or replaces a hum release tag.
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
	git ls-remote --exit-code "https://github.com/$PROJECT_REPOSITORY.git" "refs/tags/$RELEASE_TAG" >/dev/null \
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

findHaikuPorter()
{
	haikuporter=${HAIKUPORTER:-}
	if [ -z "$haikuporter" ] && command -v haikuporter >/dev/null 2>&1; then
		haikuporter=$(command -v haikuporter)
	fi
	if [ -z "$haikuporter" ]; then
		for candidate in "$HOME/haikuporter/haikuporter" \
			/boot/home/haikuporter/haikuporter; do
			if [ -x "$candidate" ]; then
				haikuporter=$candidate
				break
			fi
		done
	fi
	[ -n "$haikuporter" ] && [ -x "$haikuporter" ] \
		|| fail "HaikuPorter not found; set HAIKUPORTER to its executable"
	printf '%s\n' "$haikuporter"
}

captureGithubToken()
{
	github_token=${GITHUB_TOKEN:-}
	if [ -n "$github_token" ]; then
		return
	fi
	[ -t 0 ] || fail "set GITHUB_TOKEN to a token with permission to create public pull requests"
	printf 'Paste your GitHub token, then press Enter. Input remains hidden.\nToken: '
	trap 'stty echo; exit 1' HUP INT TERM
	stty -echo
	IFS= read -r github_token || {
		stty echo
		trap - HUP INT TERM
		printf '\n' >&2
		fail "could not read the GitHub token"
	}
	stty echo
	trap - HUP INT TERM
	printf '\n'
	[ -n "$github_token" ] || fail "a GitHub token is required"
}

githubApiGet()
{
	curl --fail --silent --show-error \
		--header "Accept: application/vnd.github+json" \
		--header "Authorization: Bearer $github_token" \
		--header "X-GitHub-Api-Version: 2022-11-28" \
		"$1"
}

githubApiPost()
{
	curl --fail --silent --show-error \
		--request POST \
		--header "Accept: application/vnd.github+json" \
		--header "Authorization: Bearer $github_token" \
		--header "X-GitHub-Api-Version: 2022-11-28" \
		--header "Content-Type: application/json" \
		--data-binary "@$2" \
		"$1"
}

createSubmissionConfiguration()
{
	configuration=$1
	submission_tree=$2
	haikuports_configuration="$HOME/config/settings/haikuports.conf"
	if [ -f "$haikuports_configuration" ]; then
		awk -v tree="$submission_tree" '
			BEGIN { found = 0 }
			/^[[:space:]]*TREE_PATH=/ {
				print "TREE_PATH=\"" tree "\""
				found = 1
				next
			}
			{ print }
			END {
				if (!found)
					print "TREE_PATH=\"" tree "\""
			}' "$haikuports_configuration" > "$configuration"
	else
		printf 'TREE_PATH="%s"\nPACKAGER="Sikosis <phil@sikosis.com>"\n' \
			"$submission_tree" > "$configuration"
	fi
}

configureSubmissionIdentity()
{
	submission_tree=$1
	git_name=${HUM_GIT_NAME:-$(git -C "$REPO_ROOT" config user.name || true)}
	git_email=${HUM_GIT_EMAIL:-$(git -C "$REPO_ROOT" config user.email || true)}
	git_name=${git_name:-Sikosis}
	git_email=${git_email:-phil@sikosis.com}
	git -C "$submission_tree" config user.name "$git_name"
	git -C "$submission_tree" config user.email "$git_email"
}

extractFirstHtmlUrl()
{
	sed -n 's/^[[:space:]]*"html_url": "\([^"]*\)",*/\1/p' "$1" | head -n 1
}

submit()
{
	[ "$#" -eq 0 ] || fail "submit takes no arguments"
	[ -f "$RECIPE" ] || fail "missing finalized recipe: $RECIPE"
	for command_name in git curl awk sed find sort cmp mktemp; do
		command -v "$command_name" >/dev/null 2>&1 \
			|| fail "required command not found: $command_name"
	done

	captureGithubToken
	user_response=$(githubApiGet "https://api.github.com/user") \
		|| fail "GitHub rejected the supplied token"
	github_user=$(printf '%s\n' "$user_response" | sed -n \
		's/^[[:space:]]*"login": "\([^"]*\)",*/\1/p' | head -n 1)
	[ -n "$github_user" ] || fail "could not determine the GitHub user for the supplied token"
	printf 'Authenticated with GitHub as %s.\n' "$github_user"

	submission_directory=$(mktemp -d \
		"${TMPDIR:-/tmp}/hum-haikuports-submit-$VERSION.XXXXXX") \
		|| fail "could not create an isolated submission directory"
	submission_tree="$submission_directory/haikuports"
	printf 'Cloning an isolated HaikuPorts tree into %s\n' "$submission_tree"
	git clone --depth=1 https://github.com/haikuports/haikuports.git "$submission_tree"
	git -C "$submission_tree" switch -c "$SUBMISSION_BRANCH"
	stage "$submission_tree"
	configureSubmissionIdentity "$submission_tree"

	build_configuration="$submission_directory/haikuports.conf"
	createSubmissionConfiguration "$build_configuration" "$submission_tree"
	haikuporter=$(findHaikuPorter)
	printf 'Validating the recipe and tests with %s ...\n' "$haikuporter"
	(
		cd "$submission_tree"
		"$haikuporter" --config="$build_configuration" -S --test "$PACKAGE"
	)
	package=$(find "$submission_tree/packages" -type f \
		-name "hum-$VERSION-*.hpkg" ! -name '*debuginfo*' -print | head -n 1)
	[ -n "$package" ] || fail "HaikuPorter did not produce the expected hum package"

	git -C "$submission_tree" add "app-misc/hum/hum-$VERSION.recipe"
	if [ -d "$submission_tree/app-misc/hum/patches" ]; then
		git -C "$submission_tree" add "app-misc/hum/patches"
	fi
	git -C "$submission_tree" commit -m "hum: add version $VERSION"

	fork_response="$submission_directory/github-fork.json"
	if ! githubApiGet "https://api.github.com/repos/$github_user/haikuports" \
		> "$fork_response" 2>/dev/null; then
		printf 'Creating %s/haikuports fork ...\n' "$github_user"
		fork_request="$submission_directory/github-fork-request.json"
		printf '{"default_branch_only":true}\n' > "$fork_request"
		githubApiPost "https://api.github.com/repos/haikuports/haikuports/forks" \
			"$fork_request" > "$fork_response"
		attempt=0
		while [ "$attempt" -lt 30 ]; do
			if githubApiGet "https://api.github.com/repos/$github_user/haikuports" \
				> "$fork_response" 2>/dev/null; then
				break
			fi
			attempt=$((attempt + 1))
			sleep 2
		done
		[ "$attempt" -lt 30 ] || fail "the HaikuPorts fork was created but is not ready yet"
	fi

	existing_pulls="$submission_directory/github-pulls.json"
	githubApiGet "https://api.github.com/repos/haikuports/haikuports/pulls?state=open&head=$github_user:$SUBMISSION_BRANCH" \
		> "$existing_pulls"
	existing_pull_url=$(extractFirstHtmlUrl "$existing_pulls")
	if [ -n "$existing_pull_url" ]; then
		printf 'HaikuPorts pull request already exists: %s\n' "$existing_pull_url"
		printf 'Validated package: %s\n' "$package"
		return
	fi

	git -C "$submission_tree" remote add fork \
		"https://github.com/$github_user/haikuports.git"
	if git -C "$submission_tree" ls-remote --exit-code fork \
		"refs/heads/$SUBMISSION_BRANCH" >/dev/null 2>&1; then
		git -C "$submission_tree" fetch --depth=2 fork "$SUBMISSION_BRANCH"
		local_files="$submission_directory/local-files"
		remote_files="$submission_directory/remote-files"
		remote_parent=$(git -C "$submission_tree" rev-parse FETCH_HEAD^)
		git -C "$submission_tree" diff --name-only origin/master...HEAD \
			| sort > "$local_files"
		git -C "$submission_tree" diff --name-only "$remote_parent" FETCH_HEAD \
			| sort > "$remote_files"
		cmp -s "$local_files" "$remote_files" \
			|| fail "existing fork branch $SUBMISSION_BRANCH contains different files"
		git -C "$submission_tree" diff --quiet FETCH_HEAD HEAD -- app-misc/hum \
			|| fail "existing fork branch $SUBMISSION_BRANCH contains different hum files"
		printf 'Using matching existing branch %s:%s.\n' \
			"$github_user" "$SUBMISSION_BRANCH"
	else
		askpass="$submission_directory/git-askpass.sh"
		printf '%s\n' \
			'#!/bin/sh' \
			'case "$1" in' \
			'  *Username*) printf "%s\\n" "x-access-token" ;;' \
			'  *) printf "%s\\n" "$GITHUB_TOKEN" ;;' \
			'esac' > "$askpass"
		chmod 700 "$askpass"
		GITHUB_TOKEN=$github_token GIT_ASKPASS=$askpass GIT_TERMINAL_PROMPT=0 \
			git -C "$submission_tree" push --set-upstream fork "$SUBMISSION_BRANCH"
	fi

	pull_request="$submission_directory/github-pull-request.json"
	printf '{"title":"hum: add version %s","head":"%s:%s","base":"master","body":"Adds hum %s, a collection of friendly, composable terminal tools for Haiku shell scripts. Built and tested with HaikuPorter on Haiku x86_64, then installed and smoke-tested with pkgman."}\n' \
		"$VERSION" "$github_user" "$SUBMISSION_BRANCH" "$VERSION" \
		> "$pull_request"
	pull_response="$submission_directory/github-pull-response.json"
	githubApiPost "https://api.github.com/repos/haikuports/haikuports/pulls" \
		"$pull_request" > "$pull_response"
	pull_request_url=$(extractFirstHtmlUrl "$pull_response")
	[ -n "$pull_request_url" ] || fail "GitHub created the response without a pull request URL"
	printf 'HaikuPorts pull request: %s\n' "$pull_request_url"
	printf 'Validated package: %s\n' "$package"
	printf 'Submission checkout retained at: %s\n' "$submission_directory"
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
	submit) submit "$@" ;;
	help|-h|--help) usage ;;
	*) usage >&2; fail "unknown command: $command" ;;
esac

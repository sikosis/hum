# HaikuPorts release workflow

Updated: 13 October 2026

This directory contains the HaikuPorts recipe template and a helper that
validates hum, creates a checksummed recipe from a published release, stages it
in a HaikuPorts checkout, runs HaikuPorter on Haiku, and can submit the finished
port from an isolated checkout. The current project version comes from the root
`VERSION` file.
The GitHub source tag is kept separately in `UPSTREAM_TAG` because the current
project version is `0.4`, while the published tag is `v0.4.0`.

The helper never creates, moves, or replaces a hum release tag. Its `submit`
command performs the HaikuPorts build, branch, commit, push, and pull request
steps only when you explicitly run it. HaikuPorts expects a reviewable recipe
in its own feature branch, and its recipe guidelines require the source
checksum and consistent recipe metadata.

## 1. Validate hum

From the hum repository root, run:

```sh
sh packaging/haikuports/hum-port.sh check
```

This runs both the Make and CMake builds and test suites. Fix any failures
before preparing a release.

## 2. Publish an immutable source tag

Commit the exact source and packaging changes you intend to release, with the
root `VERSION` set to the intended release number. Review the diff and run the
validation command above. After you explicitly decide to release, create and
push the version tag yourself. For the current release, `UPSTREAM_TAG` is
`v0.4.0`; keep this file aligned with the release being packaged.

Do not move or replace a published tag. HaikuPorts verifies the downloaded
archive against a checksum, so corrections require a new hum version.

## 3. Generate the final recipe

After the tag named in `UPSTREAM_TAG` is visible on GitHub, run:

```sh
sh packaging/haikuports/hum-port.sh finalize
```

The helper downloads the GitHub tag archive, checks that its embedded `VERSION`
matches the project version (accepting the release's trailing `.0` spelling),
calculates its SHA-256 checksum, and creates
`app-misc/hum/hum-$VERSION.recipe` from the `.recipe.in` template. It fills in
the upstream archive URL and directory as well as the checksum, and refuses to
overwrite an existing finalized recipe. Review the generated recipe and keep
the `.recipe.in` template and `UPSTREAM_TAG` file out of the HaikuPorts
submission.

## 4. Stage and validate on Haiku

Copy the generated recipe into your HaikuPorts checkout:

```sh
sh packaging/haikuports/hum-port.sh stage /path/to/haikuports
```

On Haiku, pull the latest hum commit and run one command from the hum repository
root. It copies the finalized recipe into the HaikuPorts checkout configured by
`TREE_PATH`, stages any release patches, locates the existing HaikuPorter
checkout, cleans the previous hum work directory, performs a strict build, and
runs the recipe tests:

```sh
sh packaging/haikuports/hum-port.sh haiku-test
```

You may pass a different HaikuPorts checkout path as the final argument, or set
`HAIKUPORTER` if its executable is in a non-standard location.

The recipe supports `x86_64`. After the package is built, install it and check:

```sh
hum --version
hum --help
hum style --foreground 212 'hello from HaikuDepot'
```

## 5. Submit the HaikuPorts contribution

On Haiku, run the submission command from the hum repository:

```sh
sh packaging/haikuports/hum-port.sh submit
```

The command prompts for a GitHub token if `GITHUB_TOKEN` is not already set. It
clones a clean HaikuPorts tree into a temporary directory, stages only the hum
recipe and release patches, runs the strict build and tests, creates the hum
feature branch, pushes it to your fork, and opens the pull request. If the same
branch was already pushed, the command verifies that its file list and hum
contents match before using it. Other projects or untracked files in an
existing HaikuPorts checkout cannot enter the submission.

Set `HUM_GITHUB_REPOSITORY`, `HUM_GIT_NAME`, `HUM_GIT_EMAIL`, or
`HAIKUPORTER` only when their defaults do not match your environment.

See the current
[HaikuPorter guidelines](https://github.com/haikuports/haikuports/wiki/HaikuPorter-Guidelines)
and [HaikuPorts development model](https://github.com/haikuports/haikuports/wiki/DevelopmentModel)
for contribution requirements.

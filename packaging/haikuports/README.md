# HaikuPorts release workflow

Updated: 12 October 2026

This directory contains the HaikuPorts recipe template and a helper that
validates hum, creates a checksummed recipe from a published release, stages it
in a HaikuPorts checkout, and runs HaikuPorter on Haiku. The current project
version comes from the root `VERSION` file.
The GitHub source tag is kept separately in `UPSTREAM_TAG` because the current
project version is `0.4`, while the published tag is `v0.4.0`.

The helper deliberately will not create or push a hum release tag, change
HaikuPorts branches, push commits, or create a pull request. Those actions
remain under your control. HaikuPorts expects a reviewable recipe in its own
feature branch, and its recipe guidelines require the source checksum and
consistent recipe metadata.

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

On Haiku, run the lint, build, and recipe tests:

```sh
sh /path/to/hum/packaging/haikuports/hum-port.sh haiku-test /path/to/haikuports
```

The recipe supports `x86_64`. After the package is built, install it and check:

```sh
hum --version
hum --help
hum style --foreground 212 'hello from HaikuDepot'
```

## 5. Prepare the HaikuPorts contribution

Follow the HaikuPorts feature-branch workflow: update your HaikuPorts `master`
from upstream, create a feature branch in your fork, commit only the recipe,
and open a pull request against `haikuports/haikuports:master`. Suggested title:

```text
app-misc/hum: add version 0.4
```

The helper does not push or open the pull request. See the current
[HaikuPorter guidelines](https://github.com/haikuports/haikuports/wiki/HaikuPorter-Guidelines)
and [HaikuPorts development model](https://github.com/haikuports/haikuports/wiki/DevelopmentModel)
for contribution requirements.

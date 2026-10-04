# HaikuPorts submission staging

Started: 3 October 2026

This directory stages the first HaikuPorts submission for hum under the
project's current versioning policy. The target release is hum 0.34.

There is deliberately no valid `.recipe` file yet. HaikuPorts recipes must
refer to an immutable source archive, and the required `v0.34` Git tag has not
been published. `app-misc/hum/hum-0.34.recipe.in` is a reviewable template; its
checksum placeholder prevents accidental submission as a finished recipe.

## Release prerequisite

Before finalising the recipe:

1. Commit the intended hum 0.34 source, including the root `VERSION` file.
2. Run `make clean && make test` on the release commit.
3. With explicit release approval, create and push the immutable `v0.34` tag.
4. Download the GitHub tag archive and calculate its SHA-256 checksum.
5. Copy the template to `hum-0.34.recipe` and replace `@CHECKSUM_SHA256@`.

Do not retag or replace `v0.34` after publishing it. Any correction must use a
new hum version.

## Finalise after the tag exists

Run these commands from the hum repository root:

```sh
curl -L -o /tmp/hum-0.34.tar.gz \
    https://github.com/sikosis/hum/archive/refs/tags/v0.34.tar.gz
sha256sum /tmp/hum-0.34.tar.gz
cp packaging/haikuports/app-misc/hum/hum-0.34.recipe.in \
    packaging/haikuports/app-misc/hum/hum-0.34.recipe
```

Replace `@CHECKSUM_SHA256@` in the new `.recipe` with the printed digest, then
copy that recipe into `app-misc/hum/` in a current HaikuPorts checkout.

## Haiku validation

From the HaikuPorts checkout, run:

```sh
haikuporter --lint hum-0.34
haikuporter -S hum-0.34
haikuporter -S --test hum-0.34
```

The recipe initially marks `x86_64` as untested. Remove the leading `?` only
after the complete package build and test workflow succeeds on Haiku x86_64.

Install the generated package and confirm:

```sh
hum --version
hum --help
hum style --foreground 212 'hello from HaikuDepot'
```

The version commands must report hum 0.34 before submission.

## Submission

Follow the HaikuPorts feature-branch workflow and open a pull request against
`haikuports/haikuports:master`. Suggested title:

```text
app-misc/hum: add version 0.34
```

Do not submit the `.recipe.in` template.

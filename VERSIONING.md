# Versioning

23.17 uses the [SamVer](https://samver.org/) format that fits the version-string syntax of [Semantic Versioning 2.0.0](https://semver.org/). Each version identifies the Git commit used to build the game.

```text
YYYYMMDD.HHMM.SS+SHORT_SHA
```

Use the commit's committer timestamp in UTC and its abbreviated Git object ID. Interpret `HHMM` and `SS` as decimal integers so those components have no leading zeroes, as required by Semantic Versioning 2.0.0. For example, a commit at 08:59:01 UTC on 24 November 2026 with the short ID `a5a7ec2` produces `20261124.859.1+a5a7ec2`.

Run `python3 Build/Version.py` from the repository root to derive the version of the checked-out commit. The desktop archive stores it beside the executable. The game shows it on launch and prints it with `--version`.

Start the **Desktop prerelease** workflow from `main` after the matching CI and CodeQL runs succeed. The workflow builds and checks the Linux archive, publishes a prerelease for that exact commit, and attaches the archive. Release notes record the full commit ID and any compatibility or save migration changes. Existing release tags are protected against edits and deletion.

Compare the three numeric components to order builds by commit time. Commits made in the same second have equal SemVer precedence because the Git ID is build metadata. The timestamp and commit ID identify the build. Release notes describe compatibility.

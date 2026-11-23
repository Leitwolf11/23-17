# Versioning

23.17 uses the [SamVer](https://samver.org/) format that fits the version-string syntax of [Semantic Versioning 2.0.0](https://semver.org/). Each version identifies the Git commit used to build the game.

```text
YYYYMMDD.HHMM.SS+SHORT_SHA
```

Use the commit's committer timestamp in UTC and its abbreviated Git object ID. Interpret `HHMM` and `SS` as decimal integers so those components have no leading zeroes, as required by Semantic Versioning 2.0.0. For example, a commit at 08:59:01 UTC on 24 November 2026 with the short ID `a5a7ec2` produces `20261124.859.1+a5a7ec2`.

To derive the version of the checked-out commit, run this from the repository root with Git and Python 3 installed.

```python
from datetime import datetime, timezone
from subprocess import check_output

timestamp = int(check_output(["git", "show", "-s", "--format=%ct", "HEAD"]))
commit = check_output(["git", "rev-parse", "--short=7", "HEAD"], text=True).strip()
utc = datetime.fromtimestamp(timestamp, tz=timezone.utc)
print(f"{utc:%Y%m%d}.{utc.hour * 100 + utc.minute}.{utc.second}+{commit}")
```

Use the generated version for the GitHub release tag and the game build's displayed version. Record the full commit ID in the release notes and document player-visible changes and any save compatibility or migration requirements there.

Compare the three numeric components to order builds by commit time. Commits made in the same second have equal SemVer precedence because the Git ID is build metadata. The timestamp and commit ID identify the build. Release notes describe compatibility.

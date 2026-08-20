# H700 Release Packages

Build scripts write versioned PegasusG by ROC frontend packages for H700 into
this directory. The generated ZIP files are ignored by Git:

```text
PegasusG by ROC ver1.08 for H700.zip
PegasusG by ROC ver1.09 for H700.zip
```

New builds must increment the version number. Attach each verified ZIP to the
matching GitHub Release tag, for example `h700-v1.08`, and publish its SHA-256
checksum in the release notes. Do not commit generated archives to the source
branch.

The complete music supplement is intentionally not committed because of its
size and uncertain redistribution status. Every formal frontend package still
contains the selected 11-track built-in music set.

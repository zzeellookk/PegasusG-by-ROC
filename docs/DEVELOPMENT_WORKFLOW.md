# GitHub Development Workflow

## Branches

- `main`: only device-verified stable code.
- `dev/brick`: active TrimUI Brick work.
- `dev/h700`: active H700 work.
- Short-lived fix branches: `fix/brick-zip-cache`, `fix/brick-rumble`, etc.

Do not develop directly on `main`. Merge a change only after its desktop tests
and applicable device checklist pass.

## Commit sequence

Keep commits small and reversible:

1. Source or launcher change.
2. Automated tests.
3. Documentation and changelog.
4. Device verification result.

Example messages:

```text
fix(brick): preserve MainUI SIGKILL autostart handoff
test(brick): cover GBA collection scan paths
docs(brick): record forced-rumble first-frame freeze
```

## Do not commit

- ROMs, BIOS files, saves or copyrighted game media.
- Device logs and private configuration.
- `build/`, sysroots, compiler temporary files or local SDKs.
- Install archives. Attach release ZIP files to GitHub Releases instead.
- Passwords, SSH keys, GitHub tokens or signed download URLs.

The small runtime assets already tracked by the original project remain in
`assets/` with their notices. Review `THIRD_PARTY_NOTICES.md` before publishing
a release.

## Pull request checklist

- [ ] `make test` passes in an environment with SDL2 and ALSA development files.
- [ ] New Brick logic has a test under `tests/` where practical.
- [ ] Shell scripts pass `sh -n` or `bash -n` as appropriate.
- [ ] No new binaries, logs, ROMs, sysroots or secrets are included.
- [ ] `CHANGELOG.md` and the relevant device baseline are updated.
- [ ] A rollback path is documented before device installation.

## Releases

Use annotated tags such as:

```text
h700-v1.08
brick-v1.1.1-hotfix3
```

Build install archives from a clean tag and attach them to the corresponding
GitHub Release. Record the SHA-256 checksum in the release notes.

# Deferred observations

- Plan 03-16: the installed actionlint version does not recognize the repository's existing Ubuntu 26.04
  runner labels. Checking `ci.yml` with only that outdated-label diagnostic filtered exits 0. No runner
  label or unrelated workflow policy was changed; this is a local linter metadata limitation.

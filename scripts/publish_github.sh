#!/usr/bin/env bash
# Explicitly run after authenticating gh. Creates a private repository by default.
set -euo pipefail
cd "$(dirname "$0")/.."
gh auth status
if ! git remote get-url origin >/dev/null 2>&1; then
  gh repo create Dustbound-PSP --private --source=. --remote=origin --push
else
  git push -u origin main
fi
git push origin v0.1.0
gh release create v0.1.0 dist/Dustbound-0.1.0-EBOOT.PBP dist/Dustbound-0.1.0-PSP.zip \
  dist/Dustbound-0.1.0-Evidence.zip dist/SHA256SUMS.txt \
  --title 'Dustbound 0.1.0 — playable PSP alpha' --notes-file docs/RELEASE_NOTES.md

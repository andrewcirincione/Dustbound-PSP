#!/usr/bin/env bash
# Optional maintainer command. Requires an authenticated gh CLI; no tokens are stored here.
set -euo pipefail
cd "$(dirname "$0")/.."
version="${1:-0.2.0}"
gh auth status
git push origin HEAD:main
git push origin "v$version"
gh release create "v$version" "dist/Dustbound-$version-EBOOT.PBP" "dist/Dustbound-$version-PSP.zip" \
  "dist/Dustbound-$version-Evidence.zip" dist/SHA256SUMS-0.2.0.txt \
  --prerelease --title "Dustbound $version — Frontier expansion" --notes-file docs/RELEASE_NOTES.md

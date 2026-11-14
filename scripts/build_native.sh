#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
# Compile each current core module once; run every historical and current suite.
python3 scripts/validate_v1000.py
python3 -c "import json; r=json.load(open('build/v1000/native-results.json')); assert len(r)>=52 and all(x['status']=='passed' for x in r.values()), r"
cp build/v1000/dust-native build/dustbound-native
python3 scripts/audit_catalog.py

#!/usr/bin/env python3
"""Upload a draft, verify every published digest, then make it public."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
REPO=os.environ['GITHUB_REPOSITORY']
SHA=os.environ['GITHUB_SHA']
TAG='v1.0.2'
NAMES=['EBOOT.PBP','Dust-1.0.2-PSP.zip','Dust-1.0.2-Evidence.zip','SHA256SUMS-1.0.2.txt']

def gh(*args):return subprocess.check_output(['gh',*args],cwd=ROOT,text=True)

tag_endpoint=f'repos/{REPO}/git/ref/tags/{TAG}'
tag_result=subprocess.run(['gh','api',tag_endpoint],cwd=ROOT,capture_output=True,text=True)
if tag_result.returncode:
    assert 'HTTP 404' in tag_result.stderr,tag_result.stderr
    gh('api','--method','POST',f'repos/{REPO}/git/refs','-f','ref=refs/tags/'+TAG,'-f','sha='+SHA)
tag=json.loads(gh('api',tag_endpoint))
assert tag['object']['type']=='commit' and tag['object']['sha']==SHA,'Existing release tag belongs to another commit'

view=subprocess.run(['gh','release','view',TAG,'--repo',REPO,'--json','isDraft'],cwd=ROOT,capture_output=True,text=True)
if view.returncode:
    gh('release','create',TAG,*['dist/'+n for n in NAMES],
       '--repo',REPO,'--target',SHA,'--title','Dust 1.0.2 — Armor across the frontier',
       '--notes-file','docs/RELEASE_NOTES.md','--draft','--verify-tag')
elif json.loads(view.stdout)['isDraft']:
    gh('release','upload',TAG,*['dist/'+n for n in NAMES],'--repo',REPO,'--clobber')

release=json.loads(gh('api',f'repos/{REPO}/releases/tags/{TAG}'))
assets={x['name']:x for x in release['assets']}
assert set(assets)==set(NAMES),assets
for name in NAMES:
    data=(ROOT/'dist'/name).read_bytes()
    assert assets[name]['state']=='uploaded' and assets[name]['size']==len(data),name
    assert assets[name]['digest']=='sha256:'+hashlib.sha256(data).hexdigest(),name
if release['draft']:gh('release','edit',TAG,'--repo',REPO,'--draft=false','--latest')
published=json.loads(gh('api',f'repos/{REPO}/releases/tags/{TAG}'))
assert not published['draft'] and not published['prerelease']
print('Published',published['html_url'])

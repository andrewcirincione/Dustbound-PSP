#!/usr/bin/env python3
"""Upload a draft, verify every published digest, then make it public."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
REPO=os.environ['GITHUB_REPOSITORY']
SHA=os.environ.get('DUST_RELEASE_COMMIT') or os.environ['GITHUB_SHA']
TAG='v1.0.5'
NAMES=['EBOOT.PBP','Dust-1.0.5-PSP.zip','Dust-1.0.5-Evidence.zip','SHA256SUMS-1.0.5.txt']

def gh(*args):return subprocess.check_output(['gh',*args],cwd=ROOT,text=True)

tag_endpoint=f'repos/{REPO}/git/ref/tags/{TAG}'
tag_result=subprocess.run(['gh','api',tag_endpoint],cwd=ROOT,capture_output=True,text=True)
if tag_result.returncode:
    assert 'HTTP 404' in tag_result.stderr,tag_result.stderr
    gh('api','--method','POST',f'repos/{REPO}/git/refs','-f','ref=refs/tags/'+TAG,'-f','sha='+SHA)
tag=json.loads(gh('api',tag_endpoint))
assert tag['object']['type']=='commit' and tag['object']['sha']==SHA,'Existing release tag belongs to another commit'

def find_release():
    # Draft releases must be resolved from the release collection, then by ID.
    page=1
    while True:
        releases=json.loads(gh('api',f'repos/{REPO}/releases?per_page=100&page={page}'))
        for release in releases:
            if release['tag_name']==TAG:return release
        if len(releases)<100:return None
        page+=1

release=find_release()
if release is None:
    gh('release','create',TAG,*['dist/'+n for n in NAMES],
       '--repo',REPO,'--target',SHA,'--title','Dust 1.0.5 — Greatsword shoulder alignment',
       '--notes-file','docs/RELEASE_NOTES.md','--draft','--verify-tag')
    release=find_release()
assert release is not None,'Release creation did not return a visible draft'
assets={x['name']:x for x in release['assets']}
matching=set(assets)==set(NAMES) and all(
    assets[n]['state']=='uploaded' and assets[n]['size']==(ROOT/'dist'/n).stat().st_size and
    assets[n]['digest']=='sha256:'+hashlib.sha256((ROOT/'dist'/n).read_bytes()).hexdigest() for n in NAMES)
if not matching:
    assert release['draft'],'Published assets differ; refusing to replace them'
    gh('release','upload',TAG,*['dist/'+n for n in NAMES],'--repo',REPO,'--clobber')
release=json.loads(gh('api',f"repos/{REPO}/releases/{release['id']}"))
assets={x['name']:x for x in release['assets']}
assert set(assets)==set(NAMES),assets
for name in NAMES:
    data=(ROOT/'dist'/name).read_bytes()
    assert assets[name]['state']=='uploaded' and assets[name]['size']==len(data),name
    assert assets[name]['digest']=='sha256:'+hashlib.sha256(data).hexdigest(),name
if release['draft']:
    gh('api','--method','PATCH',f"repos/{REPO}/releases/{release['id']}",'-F','draft=false','-f','make_latest=true')
published=json.loads(gh('api',f"repos/{REPO}/releases/{release['id']}"))
assert not published['draft'] and not published['prerelease']
print('Published',published['html_url'])

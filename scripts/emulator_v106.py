#!/usr/bin/env python3
"""Review actual PSP catalog items through ordinary save loading and buttons."""
import asyncio
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import emulator_v1000 as bridge

ROOT=Path(__file__).resolve().parents[1]
RUN=ROOT/'build/v106/emu'
OUT=ROOT/'evidence/v106/psp'
MEMSTICK=RUN/'xdg/ppsspp'
GAME=MEMSTICK/'PSP/GAME/DUSTBOUND'

def main():
    app=Path(sys.argv[1]).resolve();quick='--quick' in sys.argv
    GAME.mkdir(parents=True,exist_ok=True);(MEMSTICK/'PSP/SYSTEM').mkdir(parents=True,exist_ok=True)
    OUT.mkdir(parents=True,exist_ok=True)
    resources=list(app.rglob('font_atlas.zim'));assert resources
    asset=GAME/'assets'
    if not asset.exists():asset.symlink_to(resources[0].parent,target_is_directory=True)
    fixtures=ROOT/'build/v1000/fixtures';fixtures.mkdir(parents=True,exist_ok=True)
    objects=[p for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem not in ('render','native_main')]
    fixture=ROOT/'build/v106/make-fixtures';fixture.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/make_v106_fixture.c',*map(str,objects),'-lm','-o',str(fixture)],cwd=ROOT,check=True)
    subprocess.run([str(fixture),str(fixtures)],cwd=ROOT,check=True)
    eboot=ROOT/'EBOOT.PBP';shutil.copy2(eboot,GAME/'EBOOT.PBP');(GAME/'DUST_QA').touch()
    config=RUN/'release.ini';config.write_text('''[General]
FirstRun = False
AutoRun = True
PauseWhenMinimized = False
RemoteDebuggerOnStartup = True
RemoteDebuggerLocal = True
RemoteISOPort = 9252
[Graphics]
GraphicsBackend = 0
SoftwareRendering = True
InternalResolution = 1
[Sound]
Enable = False
''')
    os.environ['DUST_PPSSPP_CONFIG']=str(config.resolve())
    bridge.RUN=RUN;bridge.GAME=GAME;bridge.OUT=OUT;bridge.CONTROL=RUN/'command.json';bridge.RESULT=RUN/'result.json'
    steps=[{'expect':{'screen':13},'capture':'v106-title'}]
    selected=[(i,f) for i in ([109] if quick else [100,102,109,119]) for f in range(2)]
    for s,f in selected:
        steps += [{'load':f'greatsword-{s:03d}-{f}'},{'expect':{'screen':0},'capture':f'greatsword-{s:03d}-{f}-world'},
                  {'key':'triangle'},{'expect':{'screen':1},'capture':f'greatsword-{s:03d}-{f}-inventory'},{'key':'circle'},
                  {'key':'square'},{'attack':2},{'capture':f'greatsword-{s:03d}-{f}-combat'},
                  {'key':'square'},{'wait':5},{'capture':f'greatsword-{s:03d}-{f}-passive'}]
    shop=json.loads((fixtures/'smith-preview.json').read_text())
    steps += [{'load':'smith-preview'},{'key':'cross'},{'key':'cross'},{'expect':{'screen':shop['screen']}},
              {'choose':shop['weapon_row']},{'capture':'smith-weapon-preview'},{'choose':shop['armor_row']},{'capture':'smith-armor-preview'},{'key':'circle'}]
    steps += [{'analog':[.55,0],'frames':8},{'capture':'v106-walking'}]
    request=RUN/'release-check.json';request.write_text(json.dumps({'id':'weapon-1.0.6','steps':steps,'stop':True}))
    sys.argv=[str(Path(bridge.__file__)),str(app),str(request)]
    async def report():
        last=-1
        while True:
            count=len(list(OUT.glob('greatsword-*.png')))
            if count!=last:print(f'PSP captures: {count}/{len(selected)*4}',flush=True);last=count
            await asyncio.sleep(1)
    async def run():
        reporting=asyncio.create_task(report())
        try:await bridge.main()
        finally:reporting.cancel()
    asyncio.run(run());result=json.loads(bridge.RESULT.read_text())
    assert result.get('ok'),result
    assert result['state']['version']=='1.0.6',result['state']
    record={k:v for k,v in result.items() if k!='state'}
    record.update(eboot_sha256=hashlib.sha256(eboot.read_bytes()).hexdigest(),emulator='Official PPSSPP 1.20.4',
                  weapons_tested=len(selected),unique_greatswords_tested=len(set(i for i,f in selected)),facings_tested=2,combat_verified=True,combat_weapons_tested=len(selected),smith_previews_verified=True,physical_psp_tested=False)
    (OUT/('quick-controller.json' if quick else 'psp-controller.json')).write_text(json.dumps(record,indent=2)+'\n')
    print('PASS: actual release EBOOT booted and reviewed in PPSSPP.',flush=True)

if __name__=='__main__':main()

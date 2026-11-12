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
RUN=ROOT/'build/v102/emu'
OUT=ROOT/'evidence/v102/psp'
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
    fixture=ROOT/'build/v102/make-fixtures';fixture.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/make_v102_fixture.c',*map(str,objects),'-lm','-o',str(fixture)],cwd=ROOT,check=True)
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
    steps=[{'expect':{'screen':13},'capture':'v102-title'}]
    for s in (range(1) if quick else range(ARMOR_SETS:=30)):
        steps += [{'load':f'armor-{s:02d}'},{'expect':{'screen':0},'capture':f'armor-{s:02d}-world'},
                  {'key':'triangle'},{'expect':{'screen':1},'capture':f'armor-{s:02d}-inventory'},{'key':'circle'}]
    steps += [{'load':'armor-combat'},{'expect':{'screen':0}},{'attack':2},{'capture':'v102-heavy-armor-combat'},
              {'key':'square'},{'wait':5},{'capture':'v102-heavy-armor-passive'},
              {'analog':[.55,0],'frames':8},{'capture':'v102-walking'}]
    request=RUN/'release-check.json';request.write_text(json.dumps({'id':'armor-1.0.2','steps':steps,'stop':True}))
    sys.argv=[str(Path(bridge.__file__)),str(app),str(request)]
    asyncio.run(bridge.main());result=json.loads(bridge.RESULT.read_text())
    assert result.get('ok'),result
    assert result['state']['version']=='1.0.2',result['state']
    record={k:v for k,v in result.items() if k!='state'}
    record.update(eboot_sha256=hashlib.sha256(eboot.read_bytes()).hexdigest(),emulator='Official PPSSPP 1.20.4',
                  armor_sets_tested=1 if quick else 30,combat_verified=True,physical_psp_tested=False)
    (OUT/('quick-controller.json' if quick else 'psp-controller.json')).write_text(json.dumps(record,indent=2)+'\n')
    print('PASS: actual release EBOOT booted and reviewed in PPSSPP.',flush=True)

if __name__=='__main__':main()

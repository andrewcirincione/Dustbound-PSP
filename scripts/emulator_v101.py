#!/usr/bin/env python3
"""Exercise the release PSP EBOOT with PPSSPP and ordinary prepared saves."""
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
RUN=ROOT/'build/v101/emu'
OUT=ROOT/'evidence/v101'
GAME=RUN/'PSP/GAME/DUSTBOUND'

def main():
    app=Path(sys.argv[1]).resolve()
    GAME.mkdir(parents=True,exist_ok=True)
    OUT.mkdir(parents=True,exist_ok=True)
    fixtures=ROOT/'build/v1000/fixtures'
    fixtures.mkdir(parents=True,exist_ok=True)
    objects=[p for p in (ROOT/'build/v1000/obj').glob('*.o') if p.stem not in ('render','native_main')]
    fixture_binary=ROOT/'build/v101/make-fixtures'
    subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/make_v101_fixture.c',*map(str,objects),'-lm','-o',str(fixture_binary)],cwd=ROOT,check=True)
    subprocess.run([str(fixture_binary),str(fixtures)],cwd=ROOT,check=True)
    eboot=ROOT/'EBOOT.PBP'
    shutil.copy2(eboot,GAME/'EBOOT.PBP')
    (GAME/'DUST_QA').touch()
    config=RUN/'release.ini'
    config.write_text('''[General]
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
    bridge.RUN=RUN
    bridge.GAME=GAME
    bridge.OUT=OUT
    bridge.CONTROL=RUN/'command.json'
    bridge.RESULT=RUN/'result.json'
    steps=[{'expect':{'screen':13},'capture':'v101-title'}]
    for scene in ('onehand','greatsword','naginata','unarmed','tetsubo'):
        steps += [
            {'load':scene},
            {'expect':{'screen':0}},
            {'attack':2},
            {'capture':'v101-'+scene+'-fight'},
            {'key':'square'},
            {'wait':5},
            {'capture':'v101-'+scene+'-passive'},
        ]
    steps += [{'analog':[.55,0],'frames':8},{'capture':'v101-passive-walking'},
              {'key':'triangle'},{'expect':{'screen':1},'capture':'v101-inventory'}]
    command=RUN/'release-check.json'
    command.write_text(json.dumps({'id':'release-1.0.1','steps':steps,'stop':True}))
    sys.argv=[str(Path(bridge.__file__)),str(app),str(command)]
    asyncio.run(bridge.main())
    result=json.loads(bridge.RESULT.read_text())
    assert result.get('ok') and result.get('id')=='release-1.0.1',result
    state=result.get('state',{})
    assert state.get('version')=='1.0.1',state
    record={k:v for k,v in result.items() if k!='state'}
    record.update(eboot_sha256=hashlib.sha256(eboot.read_bytes()).hexdigest(),
                  emulator='Official PPSSPP 1.20.4',physical_psp_tested=False)
    (OUT/'psp-controller.json').write_text(json.dumps(record,indent=2)+'\n')
    print('PASS: release EBOOT booted; five loadouts attacked, stowed weapons, walked and opened inventory.')

if __name__=='__main__':main()

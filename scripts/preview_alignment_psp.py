#!/usr/bin/env python3
"""Capture prepared alignment scenes in the PSP build, without release/tests."""
import asyncio
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import emulator_v1000 as bridge

ROOT=Path(__file__).resolve().parents[1]

def main():
    app=Path(sys.argv[1]).resolve()
    executable=Path(sys.argv[2]).resolve()
    label=sys.argv[3]
    run=ROOT/'build/alignment-preview'/('emu-'+label)
    out=ROOT/'evidence/alignment-preview'/label
    memstick=run/'xdg/ppsspp'
    game=memstick/'PSP/GAME/DUSTBOUND'
    game.mkdir(parents=True,exist_ok=True)
    (memstick/'PSP/SYSTEM').mkdir(parents=True,exist_ok=True)
    out.mkdir(parents=True,exist_ok=True)
    resources=next(app.rglob('font_atlas.zim')).parent
    if not (game/'assets').exists():
        (game/'assets').symlink_to(resources,target_is_directory=True)
    shutil.copy2(executable,game/'EBOOT.PBP')
    (game/'DUST_QA').touch()
    config=run/'preview.ini'
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
    os.environ['DUST_PPSSPP_CONFIG']=str(config)
    bridge.RUN=run
    bridge.GAME=game
    bridge.OUT=out
    bridge.CONTROL=run/'command.json'
    bridge.RESULT=run/'result.json'
    steps=[]
    for armor,weapon in [(3,109),(0,100),(9,119),(26,32)]:
        for face in [1,0]:
            name=f'alignment-set-{armor:02d}-weapon-{weapon:03d}-face-{face}'
            steps += [{'load':name},{'wait':4},{'capture':name+'-world'},
                      {'key':'triangle'},{'capture':name+'-pack'},{'key':'circle'}]
    if label=='after':
        for face in [1,0]:
            name=f'alignment-set-03-weapon-109-face-{face}'
            steps += [{'load':name}]
            for n in range(4):
                # Opposite world-axis diagonals move horizontally in the isometric view.
                steps += [{'analog':[.5 if face else -.5,-.5 if face else .5],'frames':5},
                          {'capture':f'walking-face-{face}-{n}'}]
    request=run/'preview-scenes.json'
    request.write_text(json.dumps({'id':'alignment-preview-'+label,'steps':steps,'stop':True}))
    sys.argv=[str(Path(bridge.__file__)),str(app),str(request)]
    asyncio.run(bridge.main())
    result=json.loads(bridge.RESULT.read_text())
    record={k:v for k,v in result.items() if k!='state'}
    record.update(eboot_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                  emulator='Official PPSSPP 1.20.4',prepared_saves=True,
                  regression_tests_run=False,published=False,physical_psp_tested=False)
    (out/'capture-record.json').write_text(json.dumps(record,indent=2)+'\n')
    if not result.get('ok'):raise RuntimeError(record)
    print(f'Captured {len(record["responses"])} actual PSP preview frames ({label}). No regression tests or release publication.',flush=True)

if __name__=='__main__':main()

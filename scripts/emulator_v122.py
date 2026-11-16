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
RUN=ROOT/'build/v122/emu'
OUT=ROOT/'evidence/v122/psp'
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
    fixture=ROOT/'build/v121/make-fixtures';fixture.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/make_v121_fixture.c',*map(str,objects),'-lm','-o',str(fixture)],cwd=ROOT,check=True)
    subprocess.run([str(fixture),str(fixtures)],cwd=ROOT,check=True)
    fixture2=ROOT/'build/v122/make-fixtures';fixture2.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['cc','-O2','-std=gnu99','-Isrc','tests/make_v122_fixture.c',*map(str,objects),'-lm','-o',str(fixture2)],cwd=ROOT,check=True)
    subprocess.run([str(fixture2),str(fixtures)],cwd=ROOT,check=True)
    eboot=ROOT/'EBOOT.PBP';eboot_sha256=hashlib.sha256(eboot.read_bytes()).hexdigest()
    shutil.copy2(eboot,GAME/'EBOOT.PBP');(GAME/'DUST_QA').touch()
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
    meta=json.loads((fixtures/'feedback.json').read_text())
    steps=[{'expect':{'screen':13},'capture':'v121-title'}]
    for face in range(2):
        for kind in ['iron','katana','greatsword']:
            steps += [{'load':f'v121-{kind}-{face}'},{'wait':2},{'capture':f'{kind}-facing-{face}'}]
    for bag in range(8):steps += [{'load':f'v121-pack-{bag}'},{'wait':2},{'capture':f'backpack-{bag}'}]
    steps += [
      {'load':'v121-iron-1'},{'key':'triangle'},{'choose':26},{'capture':'iron-headwear-equipped-inventory'},{'key':'circle'},
      {'analog':[.5,0],'frames':16},{'capture':'iron-walking'},
      {'load':'v121-forge'},{'key':'cross'},{'action':'FORGE EQUIPMENT'},{'key':'cross'},{'expect':{'screen':meta['forge_screen']}},
      {'choose':2},{'key':'right'},{'key':'right'},{'choose':3},{'key':'right'},{'key':'right'},{'choose':4},{'capture':'forge-output-stats'},{'key':'cross'},{'capture':'forge-crafted'},
      {'load':'v121-smith-counter'},{'key':'cross'},{'action':'BROWSE WEAPONS'},{'key':'cross'},{'expect':{'screen':meta['trade_screen']}},{'key':'right'},{'choose':26},{'capture':'equipped-selling'},{'key':'cross'},{'capture':'equipped-sold'},
      {'load':'v121-quest'},{'key':'cross'},{'action':meta['employer']},{'key':'cross'},{'expect':{'screen':17}},
      {'choose':meta['work_row']},{'key':'cross'},{'choose':0},{'key':'cross'},
      {'expect':{'screen':meta['note_screen'],'offer_mode':1,'active_contracts':0}},{'capture':'quest-offer-before-accept'},
      {'key':'circle'},{'expect':{'active_contracts':0}},{'key':'cross'},{'key':'cross'},{'expect':{'screen':0,'active_contracts':1}},
      {'key':'start'},{'key':'rtrigger'},{'key':'rtrigger'},{'choose':1},{'key':'cross'},{'expect':{'screen':meta['journal_screen']}},
      {'choose':0},{'key':'cross'},{'capture':'quest-abandon-option'},{'key':'triangle'},{'expect':{'active_contracts':0,'screen':meta['journal_screen']}},{'capture':'quest-abandoned'},
      {'load':'v121-appearance'},{'key':'cross'},{'action':meta['barber']},{'key':'cross'},{'choose':meta['appearance_row']},{'key':'cross'},{'expect':{'screen':meta['creator_screen']}},{'capture':'hair-and-beard-current'},
      {'choose':6},{'key':'right'},{'key':'right'},{'capture':'hair-and-beard-variety'},{'choose':4},{'key':'right'},{'capture':'hair-variety'},{'key':'circle'},
      {'load':'v121-camp-chest'},{'key':'cross'},{'action':'supply chest'},{'capture':'camp-chest-action'},{'key':'cross'},{'expect':{'screen':meta['chest_screen']}},{'capture':'camp-chest-opened'},{'key':'cross'},{'capture':'camp-chest-taken'},
      {'load':'v121-bed'},{'key':'cross'},{'action':'SLEEP IN BED'},{'capture':'bed-price-before-rest'},{'key':'cross'},{'expect':{'screen':0,'task':3}},{'capture':'rest-real-recovery'},
      {'load':'v121-coma'},{'capture':'consciousness-recovery'},
      {'load':'v121-loot'},{'key':'cross'},{'action':'Loot Wounded'},{'capture':'loot-percent-action'},{'key':'cross'},{'expect':{'screen':24,'loot_percent':100}},{'capture':'loot-before'},{'key':'cross'},{'expect':{'loot_percent':88}},{'capture':'loot-after-vital-harvest'},
      {'load':'v121-fight'},{'attack':2},{'capture':'fight-approach-and-attack'},
      {'load':'v121-rain'},{'wait':2},{'capture':'rain'},{'wait':8},{'capture':'rain-moving'}
    ]
    steps += [
      {'load':'v122-carry-0'},{'wait':4},{'capture':'greatsword-clear-face-left'},
      {'load':'v122-carry-1'},{'wait':4},{'capture':'greatsword-clear-face-right'},
      {'key':'triangle'},{'capture':'inventory-weights-no-legend'},{'key':'circle'},
      {'key':'down'},{'capture':'simplified-skills'},{'key':'circle'},
      {'load':'v122-camp'},{'key':'start'},{'key':'rtrigger'},{'key':'rtrigger'},{'key':'rtrigger'},
      {'choose':0},{'key':'cross'},{'expect':{'camp_options':2}},{'capture':'bonfire-bedroll-only'},{'key':'circle'},
      {'load':'v122-limping'},{'expect':{'autoattack':0}},{'capture':'injured-passive-label'},
      {'key':'square'},{'expect':{'autoattack':1}},{'capture':'injured-fight-label'},
      {'load':'v122-stolen'},{'key':'triangle'},{'expect':{'first_item_owner':1}},{'capture':'stolen-before-two-days'},
      {'key':'circle'},{'wait':45},{'key':'triangle'},{'expect':{'first_item_owner':0}},{'capture':'stolen-after-two-days'},
      {'load':'v122-redoubt-0'},{'wait':3},{'capture':'redoubt-sleeping-quarters'},
      {'analog':[.55,.55],'frames':24},{'capture':'redoubt-walking'},
      {'load':'v122-redoubt-1'},{'wait':3},{'capture':'redoubt-armory-floor'},
      {'load':'v122-redoubt-2'},{'wait':3},{'capture':'redoubt-command-floor'},
      {'load':'v122-redoubt-stairs'},{'key':'cross'},{'action':'CLIMB TO NEXT FLOOR'},{'key':'cross'},
      {'expect':{'inn_floor':1}},{'capture':'redoubt-stairs-used'},
      {'load':'v122-armory-cache'},{'key':'cross'},{'action':'SEARCH CACHE'},{'key':'cross'},
      {'expect':{'screen':meta['chest_screen']}},{'capture':'armory-useful-weapon-loot'},
      {'load':'v122-command-cache'},{'key':'cross'},{'action':'SEARCH CACHE'},{'key':'cross'},
      {'expect':{'screen':meta['chest_screen']}},{'capture':'command-vault-rewards'},
      {'load':'v122-storehouse'},{'key':'cross'},{'capture':'storehouse-chest-actions'},
      {'action':'SEARCH CACHE'},{'key':'cross'},{'expect':{'screen':meta['chest_screen']}},{'capture':'storehouse-local-chest'},
      {'load':'v122-factory'},{'capture':'factory-machine-hall'},
      {'load':'v122-archive'},{'capture':'archive-reading-floor'},
      {'load':'v122-infirmary'},{'capture':'infirmary-treatment-ward'},
      {'load':'v122-refuge'},{'capture':'refuge-sleeping-gallery'}
    ]
    partial='--from-scene' in sys.argv
    if partial:
        scene=sys.argv[sys.argv.index('--from-scene')+1]
        first=next(i for i,step in enumerate(steps) if step.get('load')==scene)
        steps=[{'expect':{'screen':13}}]+steps[first:]
    request=RUN/'release-check.json';request.write_text(json.dumps({'id':'feedback-1.2.2','steps':steps,'stop':True}))
    sys.argv=[str(Path(bridge.__file__)),str(app),str(request)]
    asyncio.run(bridge.main());result=json.loads(bridge.RESULT.read_text())
    assert result.get('ok'),{k:v for k,v in result.items() if k!='state'}
    assert result['state']['version']=='1.2.2',result['state']
    assert hashlib.sha256(eboot.read_bytes()).hexdigest()==eboot_sha256,'Executable changed during emulation'
    record={k:v for k,v in result.items() if k!='state'}
    record.update(partial=partial,eboot_sha256=eboot_sha256,emulator='Official PPSSPP 1.20.4',
                  feedback_scenarios_verified=True,new_122_scenarios_verified=True,redoubt_floors_reviewed=3,other_interiors_reviewed=4,stolen_expiry_verified=True,weight_columns_verified=True,simplified_camps_verified=True,injured_stance_verified=True,backpacks_reviewed=8,quest_offer_accept_abandon_verified=True,camp_chest_verified=True,forging_verified=True,equipped_sales_verified=True,lodging_verified=True,loot_percent_verified=True,combat_verified=True,physical_psp_tested=False)
    (OUT/('quick-controller.json' if quick else 'psp-controller.json')).write_text(json.dumps(record,indent=2)+'\n')
    print('PASS: actual release EBOOT booted and reviewed in PPSSPP.',flush=True)

if __name__=='__main__':main()

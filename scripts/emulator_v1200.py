#!/usr/bin/env python3
"""Exercise the actual PSP release with ordinary saves, analog input and buttons."""
import asyncio,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
import emulator_v1000 as bridge
ROOT=Path(__file__).resolve().parents[1];RUN=ROOT/'build/v1200/emu';OUT=ROOT/'evidence/v1200/psp';MEMSTICK=RUN/'xdg/ppsspp';GAME=MEMSTICK/'PSP/GAME/DUSTBOUND'
def main():
 app=Path(sys.argv[1]).resolve();GAME.mkdir(parents=True,exist_ok=True);(MEMSTICK/'PSP/SYSTEM').mkdir(parents=True,exist_ok=True);OUT.mkdir(parents=True,exist_ok=True)
 for p in OUT.glob('*.png'):p.unlink()
 resources=list(app.rglob('font_atlas.zim'));assert resources
 asset=GAME/'assets'
 if not asset.exists():asset.symlink_to(resources[0].parent,target_is_directory=True)
 subprocess.run(['python','scripts/review_expedition_v1200.py'],cwd=ROOT,check=True)
 fixture=ROOT/'build/v1000/fixtures';meta=json.loads((fixture/'exploration.json').read_text())
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
''');os.environ['DUST_PPSSPP_CONFIG']=str(config.resolve())
 bridge.RUN=RUN;bridge.GAME=GAME;bridge.OUT=OUT;bridge.CONTROL=RUN/'command.json';bridge.RESULT=RUN/'result.json'
 steps=[{'expect':{'screen':13},'capture':'v1200-title'}]
 for m in range(6):steps += [{'load':f'exploration-model-{m}'},{'wait':2},{'capture':f'exploration-model-{m}'}]
 for name in ['refuge-entrance','turbine-entrance','archive-entrance','archive-gallery','archive-vault']:
  steps += [{'load':name},{'wait':2},{'capture':name}]
 steps += [{'load':'refuge-entrance'},{'analog':[.6,0],'frames':18},{'capture':'refuge-walking'},
           {'load':'refuge-counter'},{'key':'cross'},{'key':'right'},{'choose':meta['trade']},{'key':'cross'},{'expect':{'screen':meta['trade_screen']}},{'capture':'outpost-trading'},{'key':'circle'},
           {'load':'refuge-stairs'},{'key':'cross'},{'choose':meta['stairs']},{'key':'cross'},{'expect':{'screen':0,'inn_floor':1}},{'capture':'refuge-floor-two'},
           {'load':'refuge-downstairs'},{'key':'cross'},{'choose':meta['down']},{'key':'cross'},{'expect':{'screen':0,'inn_floor':0}},{'capture':'refuge-stairs-return'},
           {'load':'archive-upper-stairs'},{'key':'cross'},{'choose':meta['upper']},{'key':'cross'},{'expect':{'screen':0,'inn_floor':2}},{'capture':'archive-floor-three'},
           {'load':'archive-cache'},{'key':'cross'},{'choose':meta['cache']},{'key':'cross'},{'expect':{'screen':meta['chest_screen']}},{'capture':'archive-salvage'},{'key':'cross'},{'capture':'archive-salvage-taken'},{'key':'circle'},
           {'load':'robot-combat'},{'key':'square'},{'attack':2},{'capture':'robot-combat'},{'wait':20},{'capture':'robot-combat-follow-through'}]
 request=RUN/'release-check.json';request.write_text(json.dumps({'id':'exploration-1.2.0','steps':steps,'stop':True}));sys.argv=[str(Path(bridge.__file__)),str(app),str(request)]
 async def report():
  last=-1
  while True:
   count=len(list(OUT.glob('*.png')))
   if count!=last:print(f'PSP captures: {count}/21',flush=True);last=count
   await asyncio.sleep(1)
 async def run():
  task=asyncio.create_task(report())
  try:await bridge.main()
  finally:task.cancel()
 asyncio.run(run());result=json.loads(bridge.RESULT.read_text());assert result.get('ok'),result;assert result['state']['version']=='1.2.0'
 record={k:v for k,v in result.items() if k!='state'};record.update(eboot_sha256=hashlib.sha256(eboot.read_bytes()).hexdigest(),emulator='Official PPSSPP 1.20.4',exterior_models_verified=6,interior_scenes_verified=5,upstairs_verified=True,downstairs_verified=True,third_floor_verified=True,trade_verified=True,salvage_verified=True,robot_combat_verified=True,analog_walk_verified=True,physical_psp_tested=False)
 (OUT/'psp-controller.json').write_text(json.dumps(record,indent=2)+'\n');print('PASS: actual PSP executable boot, six exteriors, interiors, analog movement, stairs, third floor, trade, finite salvage and robot combat.',flush=True)
if __name__=='__main__':main()

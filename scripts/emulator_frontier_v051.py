"""Controller-only role/contract/catalog/migration playthrough of the PSP executable."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys
import websockets
from emulator_v051 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v051-roles-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in range(2,6):shutil.copy2(ROOT/'build/frontier-fixtures'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 shutil.copy2(ROOT/'build/v041-outpost.sav',run/'DUST_6.sav');(run/'DUST_QA').touch()
 log=open(run/'ppsspp.log','w');proc=subprocess.Popen([str(pathlib.Path(sys.argv[1]).resolve()),'--graphics=software','--debugger=9251','--timeout=3600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.dialog_count=5;await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   await e.key('cross');await e.key('up');await e.key('cross');await e.expect('money',650)
   await e.move_to(30,34);await e.shot('v051-50-guard-identity',0);await e.key('cross');await e.shot('v051-51-guard-contract',17)
   await e.key('cross');await e.expect('money',410);await e.expect('squad',1)
   mercs=await e.read('mercs','4i');assert any(mercs)
   await e.shot('v051-52-hired-guard',17);await e.key('down');await e.key('cross');await e.expect('money',410)
   await e.key('circle');await e.move_to(33,36);await e.shot('v051-53-escort-follows',0)
   await e.menu(13);await e.shot('v051-54-faction-contract-ledger',20);await e.key('circle')
   await e.menu(4);await e.key('down');await e.key('cross',2);await e.key('circle');await e.load(1)
   assert await e.read('mercs','4i')==mercs;await e.expect('money',410)
   e.dialog_count=4
   for slot,name in [(2,'shogunate'),(3,'rust-caravans'),(4,'black-mesa')]:
    await e.load(slot);await e.key('cross');await e.shot(f'v051-55-{name}-merchant',17)
    await e.key('down');await e.key('cross');await e.shot(f'v051-56-{name}-advice',17)
    await e.key('up');await e.key('cross');await e.key('rtrigger',2);await e.shot(f'v051-57-{name}-stock',6);await e.key('circle')
   await e.load(5)
   for i in range(4):
    await e.key('triangle');await e.key('down',i);await e.shot(f'v051-58-new-weapon-{i}',1)
    pack=await e.read('pack','48I');it=pack[i*2];await e.key('cross');assert (await e.read('gear','6I'))[0]==it
    await e.key('circle');await e.shot(f'v051-59-new-weapon-world-{i}',0)
   await e.load(6);await e.expect('identity',2);await e.shot('v051-60-v041-save-migrated',0)
   await e.menu(7);await e.key('rtrigger',3);await e.shot('v051-61-heavy-combat-guide',12)
   await e.key('rtrigger',10);await e.shot('v051-62-faction-guide',12)
   e.events.append({'fresh_escort_hired_saved_reloaded':True,'duplicate_charge_blocked':True,'squad_count_unchanged':True,'regional_merchants_reviewed':3,'new_weapon_families_equipped':4,'genuine_v041_save_loaded':True})
   (ROOT/'evidence/v051-frontier-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
   print('PSP FRONTIER IDENTITY PLAYTHROUGH PASS',flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':asyncio.run(main())

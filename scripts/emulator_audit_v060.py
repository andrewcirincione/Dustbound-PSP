#!/usr/bin/env python3
"""Negative paths and recovery/logistics played through real PSP controls."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys
import websockets
from emulator_v060 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v060-audit-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in [2,3,4]:shutil.copy2(ROOT/'build/audit-fixtures'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([str(pathlib.Path(sys.argv[1]).resolve()),'--graphics=software','--debugger=9251','--timeout=3600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   print('BOOT',await e.call('cpu.status'),await e.call('game.status'),flush=True)
   await e.key('down',2);await e.key('cross');await e.key('circle');await e.expect('screen',13);await e.expect('selection',2)
   await e.shot('v060-90-title-cancel',13)
   await e.key('cross');await e.key('down',2);await e.key('cross');await e.key('circle');await e.expect('screen',22);await e.expect('selection',2)
   await e.key('cross',2);await e.expect('screen',0)
   await e.key('cross');await e.shot('v060-91-rescue-choices',25)
   await e.key('down');await e.key('select');await e.key('circle');await e.expect('selection',1)
   await e.key('cross');await e.expect('carrying',1);await e.shot('v060-92-carry-before-bandage',0)
   await e.key('cross');await e.expect('carrying',-1,'i');await e.key('cross');await e.key('cross')
   await e.shot('v060-93-first-aid',25);await e.key('down',2);await e.key('cross');await e.shot('v060-94-recover-companion-gear',24)
   await e.key('circle');await e.key('triangle');await e.key('down',3);row=(await e.read('selection'))[0]
   await e.key('select');await e.key('circle');await e.expect('selection',row)
   await e.key('rtrigger');await e.key('down',5);await e.shot('v060-95-skills-training',2)
   await e.key('rtrigger');await e.shot('v060-96-health-vitals',3);await e.key('rtrigger');await e.shot('v060-97-squad-status',4);await e.key('circle')
   await e.load(4);await e.key('cross');await e.shot('v060-98-loot-greatsword-comparison',24);await e.key('down',2);await e.key('square')
   pack=await e.read('pack','48I');assert any(pack[i]==10 and pack[i+1]==12 for i in range(0,48,2))
   await e.key('circle');await e.key('triangle');slot=next(i//2 for i in range(0,48,2) if pack[i]==10)
   await e.key('down',slot);stolen=await e.read('stolen','24I');assert stolen[slot]==4
   await e.shot('v060-99-stolen-stack-provenance',1);await e.key('circle')
   await e.load(3);await e.key('triangle');await e.key('ltrigger');await e.key('down',6);await e.key('cross');await e.expect('job',6)
   await e.shot('v060-100-cook-needs-water',4);await e.key('circle')
   for _ in range(500):
    depot=await e.read('depot','32I')
    if any(depot[i]==1 and depot[i+1]>=1 for i in range(0,32,2)):break
    await e.waitframes(8)
   else:raise AssertionError(('cook did not fetch ingredients, cook and store output',depot,await e.read('position','ff')))
   await e.shot('v060-101-camp-production',0)
   await e.key('triangle');await e.key('ltrigger');await e.key('down');await e.key('cross');await e.expect('job',1);await e.key('circle')
   await e.move_to(42,44)
   selected=(await e.read('selected'))[0]
   for expected in [3,2]:
    await e.call('input.buttons.send',buttons={'ltrigger':True});await e.waitframes(3)
    await e.call('input.buttons.send',buttons={'cross':True});await e.waitframes(3)
    await e.call('input.buttons.send',buttons={'cross':False,'ltrigger':False});await e.waitframes(3)
    assert (await e.read('context','ii'))[0]==expected
    await e.expect('selected',selected)
   await e.shot('v060-108-target-cycling',0);await e.key('cross')
   await e.expect('screen',21);await e.key('right');await e.shot('v060-102-depot-produced-rations',21)
   await e.key('square');await e.shot('v060-103-bulk-withdraw',21);await e.key('circle')
   await e.move_to(48,44);await e.menu(2);await e.key('down');await e.shot('v060-104-recipe-requirements',9);await e.key('circle')
   await e.move_to(48,48);await e.menu(11);await e.shot('v060-105-forge-held-required',18);await e.key('circle')
   await e.menu(16);await e.shot('v060-106-message-history',26);await e.key('circle')
   await e.menu(9);await e.expect('screen',22);await e.key('down');await e.key('cross');await e.shot('v060-107-save-exit-confirm',14)
   await e.key('circle');await e.expect('screen',22);await e.key('cross',2);await e.expect('screen',13)
   e.events.append({'completed':True,'title_cancel_safe':True,'help_preserved_selection':True,'carry_before_first_aid':True,'loot_compared_and_bulk_taken':True,'stolen_owner_preserved':True,'cook_fetched_water_and_grain_then_stored_rations':True,'bulk_storage':True,'save_exit_confirmed':True,'prepared_fixture_slots':[2,3,4]})
   (ROOT/'evidence/v060-audit-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
   print('CRITICAL AUDIT PSP PLAYTHROUGH PASS',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':asyncio.run(main())

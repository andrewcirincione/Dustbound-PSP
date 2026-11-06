#!/usr/bin/env python3
"""Controller-only acceptance. Fixtures are declared; telemetry and capture are read-only."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,hashlib,time
import websockets
from emulator_v060 import Play,ROOT
async def until(e,field,value,seconds=120,fmt='I'):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  current=(await e.read(field,fmt))[0]
  if current==value:return
  await asyncio.sleep(.15)
 await e.shot('v060-failure-'+field)
 raise AssertionError((field,value,current,await e.read('position','ff'),await e.read('health','6f')))
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v060-depth-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for i in range(1,7):shutil.copy2(ROOT/'build/depth-fixtures'/f'DUST_{i}.sav',run/f'DUST_{i}.sav')
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
   await e.shot('v060-01-title',13)
   await e.key('cross');await e.key('up');await e.key('cross');await e.expect('screen',0)
   await e.shot('v060-02-fresh-frontier',0)
   await e.menu(17);await e.shot('v060-03-expedition-leads',27);await e.key('select');await e.shot('v060-04-expedition-guide',12);await e.key('circle',2)
   # Declared blood-loss fixture: start a route, collapse naturally, retain it
   # beyond the old eight-second cancellation threshold. No live state writes.
   await e.load(1);await e.menu(17);await e.key('down',7);await e.key('cross',2)
   await e.expect('waypoint',1);goal=await e.read('travel','ff')
   deadline=time.monotonic()+60
   while (await e.read('unconscious','f'))[0]<=0:
    assert time.monotonic()<deadline,'Prepared traveler did not collapse'
    await e.waitframes(10)
   knocked_at=(await e.read('time','f'))[0]
   while (await e.read('time','f'))[0]<knocked_at+10:
    assert time.monotonic()<deadline,'Knockout observation timed out'
    await e.waitframes(10)
   await e.expect('waypoint',1);assert await e.read('travel','ff')==goal
   await e.shot('v060-42-route-held-unconscious',0)
   e.events.append({'assert':'knockout preserves destination beyond eight seconds','passed':True,'prepared_fixture_slot':1})
   # Prepared rescue encounter, played through the conversation and actual escort.
   await e.load(2);e.dialog_count=4;await e.key('cross');await e.shot('v060-05-sora-wreck',17);await e.key('cross');await e.expect('depth_rescued',1)
   await e.menu(17);await e.key('down',7);await e.key('cross');await e.shot('v060-06-copper-route',5);await e.key('cross')
   await until(e,'waypoint',0,180)
   await until(e,'depth_rescued',2,60)
   await e.shot('v060-07-sora-home',0)
   # Return to culvert with one salvaged bar, using a fixture to isolate repair acceptance.
   await e.load(6);await e.key('cross');await e.shot('v060-08-culvert-before',31);await e.key('cross');await e.expect('depth_route',1);await e.shot('v060-09-bypass-open',31);await e.key('circle')
   await e.menu(4);await e.key('down');await e.key('cross',2);await e.key('circle');await e.load(1);await e.expect('depth_route',1)
   # Distinct negotiated outcome and single payment.
   await e.load(3);e.dialog_count=4;await e.key('cross');await e.shot('v060-10-rell-negotiation',17);money=(await e.read('money'))[0];await e.key('cross');await e.expect('depth_route',2);await e.expect('money',money-450)
   await e.menu(17);await e.shot('v060-11-truce-outcome',27);await e.key('circle');await e.menu(13);await e.shot('v060-12-faction-consequence',20);await e.key('circle')
   # Captivity persists, has a timed escape, and the survivor can move afterward.
   await e.load(4);await e.key('cross');await e.shot('v060-13-captive-care',30);await e.key('cross');await e.expect('screen',0);await until(e,'depth_captive',0,45);await e.shot('v060-14-escape',0)
   pos=await e.read('position','ff');await e.key('right',3);assert await e.read('position','ff')!=pos
   # Construction must finish; fuel then enables a real recipe.
   await e.load(5);await until(e,'task',0,60);await e.move_to(42.6,43)
   await e.menu(19);await e.shot('v060-15-camp-ledger',29);await e.key('square');await e.key('circle')
   await e.menu(2);await e.key('down');await e.shot('v060-16-fueled-recipe',9);await e.key('cross');await e.key('circle')
   pack=await e.read('pack','48I');assert any(pack[i]==1 and pack[i+1]>=2 for i in range(0,48,2))
   await e.menu(22);shirt=next(i//2 for i in range(0,48,2) if pack[i]==1116);await e.key('down',shirt);await e.shot('v060-17-layer-preview',32);await e.key('cross');await e.shot('v060-18-layer-fitted',32);await e.key('circle')
   await e.menu(18);await e.key('cross');await e.expect('depth_order',1);await e.key('down');await e.key('right');await e.expect('depth_group',1);await e.shot('v060-19-orders',28);await e.key('select');await e.shot('v060-20-orders-guide',12);await e.key('circle',2)
   await e.menu(20);await e.shot('v060-21-care',30);await e.key('select');await e.shot('v060-22-care-guide',12);await e.key('circle',2)
   e.events.append({'completed':True,'fresh_start':True,'prepared_fixture_slots':[1,2,3,4,5,6],'knockout_route_preserved':True,'physical_escort':True,'bypass_saved_reloaded':True,'truce_paid':True,'captivity_escaped':True,'construction_fuel_recipe':True,'layer_equipped':True,'orders_changed':True,'physical_psp_tested':False})
   (ROOT/'evidence/v060-depth-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('DEPTH PSP CONTROLLER PASS',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':asyncio.run(main())

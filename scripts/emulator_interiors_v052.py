#!/usr/bin/env python3
"""PSP EBOOT interior playthrough: only buttons; read-only file telemetry verifies state."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,math
import websockets
from emulator_v052 import Play,ROOT
async def room_to(e,x,y,field="roompos",tolerance=.28):
 for attempt in range(220):
  px,py=await e.read(field,'ff')
  if math.hypot(x-px,y-py)<tolerance:return
  sx,sy=x-px-y+py,x-px+y-py
  horizontal=abs(sx)>abs(sy);v=sx if horizontal else sy
  key=('right' if v>0 else 'left') if horizontal else ('down' if v>0 else 'up')
  await e.call('input.buttons.send',buttons={key:True});await e.waitframes(2)
  await e.call('input.buttons.send',buttons={key:False});await e.waitframes(1)
 print('BLOCKED',field,await e.read('health','6f'),await e.read('context','ii'),flush=True);await e.shot('v052-route-debug');raise AssertionError(('route blocked',x,y,px,py))
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v052-rooms-',dir=ROOT/'build'));shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP');(run/'DUST_QA').touch()
 log=open(run/'ppsspp.log','w');exe=pathlib.Path(sys.argv[1]).resolve()
 proc=subprocess.Popen([str(exe),'--graphics=software','--debugger=9251','--timeout=3600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run
   async def worldmove(x,y):await room_to(e,x,y,'position',.35)
   e.move_to=worldmove
   await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   await e.key('cross');await e.key('up');await e.key('cross');await e.expect('screen',0)
   await e.move_to(32,27);await e.move_to(32,24.65);await e.move_to(31.5,24.65)
   await e.key('cross');await e.expect('interior',3);await e.shot('v052-26-shop-entry',0)
   await room_to(e,3,3.4);await e.shot('v052-27-shop-interior',0)
   money=(await e.read('money'))[0];await e.key('cross');await e.expect('screen',6);await e.key('cross');assert (await e.read('money'))[0]<money
   await e.shot('v052-28-indoor-trade',6);await e.key('circle')
   pos=await e.read('roompos','ff');money=(await e.read('money'))[0]
   await e.menu(4);await e.key('down');await e.key('cross',2);await e.key('circle')
   await room_to(e,3.5,5.7);await e.load(1);await e.expect('interior',3);await e.expect('money',money)
   loaded=await e.read('roompos','ff');assert math.dist(pos,loaded)<.1
   await e.shot('v052-29-indoor-save-reloaded',0)
   await room_to(e,3.5,6.1);await e.key('cross');await e.expect('interior',0)
   # South aisle around the homes and central well, then the inn's east door.
   await e.move_to(32,32);await e.move_to(25,32);await e.move_to(24,31.5);await room_to(e,23.45,30.65,'position',.18)
   print('INN APPROACH',await e.read('position','ff'),await e.read('context','ii'),flush=True);await e.shot('v052-36-inn-door',0);await e.key('cross');await e.shot('v052-37-inn-attempt');await e.expect('interior',4)
   await room_to(e,2.5,5.5);await room_to(e,1.2,3.3)
   money=(await e.read('money'))[0];await e.key('cross');await e.expect('money',money-25);await e.waitframes(20)
   await e.shot('v052-30-inn-rest',0)
   await room_to(e,3.5,6.1);await e.key('cross');await e.expect('interior',0)
   await e.key('down');await e.shot('v052-31-left-inn',0)
   await e.menu(7);await e.key('ltrigger',2);await e.expect('selection',18);await e.shot('v052-32-interior-guide',12)
   e.events.append({'indoor_trade':True,'indoor_save_reload':True,'bed_paid_once':True,'doors_enter_exit':True,'input_method':'PSP buttons, no memory writes'})
   (ROOT/'evidence/v052-interior-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
   print('PSP INTERIOR PLAYTHROUGH PASS',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':asyncio.run(main())

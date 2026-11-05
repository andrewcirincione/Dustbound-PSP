#!/usr/bin/env python3
"""Fresh-start, button-driven journey; no debug state writes or prepared save."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,math
import websockets
from emulator_v051 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v051-travel-',dir=ROOT/'build'));shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP');(run/'DUST_QA').touch()
 log=open(run/'ppsspp.log','w');proc=subprocess.Popen([str(pathlib.Path(sys.argv[1]).resolve()),'--graphics=software','--debugger=9251','--timeout=3600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   await e.key('cross');await e.key('up');await e.key('cross');await e.expect('screen',0)
   await e.key('square');await e.key('select');await e.key('rtrigger');await e.shot('v051-38-copper-route',5);await e.key('cross')
   goal=await e.read('travel','ff');start=await e.read('position','ff');t0=(await e.read('time','f'))[0];captured=False
   for i in range(130):
    await e.waitframes(60);pos=await e.read('position','ff')
    if not captured and math.dist(start,pos)>60:
     await e.shot('v051-39-wilderness-travel',0);await e.key('select');await e.shot('v051-40-location-on-route',5);await e.key('rtrigger');await e.key('circle');assert await e.read('travel','ff')==goal;await e.expect('waypoint',1);captured=True
    if not (await e.read('waypoint'))[0]:break
   pos=await e.read('position','ff');await e.shot('v051-41-copper-arrival',0)
   assert captured and math.dist(pos,goal)<.8,(pos,goal,await e.read('health','6f'))
   e.events.append({'fresh_start_journey':True,'goal':goal,'arrival':pos,'map_browsing_preserved_route':True,'simulation_seconds_elapsed':(await e.read('time','f'))[0]-t0,'input_method':'PSP buttons only; no memory writes'})
   (ROOT/'evidence/v051-travel-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP FRONTIER JOURNEY PASS',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':asyncio.run(main())

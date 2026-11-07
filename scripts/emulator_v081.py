"""Atlas controller checks against the final EBOOT; prepared native saves 4/5."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys
import websockets
from emulator_v060 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v081-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in [4,5]:shutil.copy2(ROOT/'build'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=600','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.8.1';await asyncio.sleep(.8);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def expect(field,v):assert (await e.read(field))[0]==v,(field,await e.read(field),v)
   async def choose(n):
    for _ in range(30):
     current=(await e.read('mapchoice'))[0]
     if current==n:return
     await e.press('down' if current<n else 'up',1);await e.waitframes(4)
    raise AssertionError(('mapchoice',await e.read('mapchoice'),n))
   await e.key('down',2);await e.key('cross');await e.key('down',4);await e.key('cross',2);await expect('screen',0)
   await e.key('select');await expect('mapzoom',0);await expect('mapchoice',0);await e.shot('v081-psp-unexplored',5)
   await e.key('cross');await expect('screen',5);await expect('waypoint',0)
   await e.key('circle');await e.key('start');await e.key('down',2);await e.key('cross');await e.key('down',5);await e.key('cross',2);await expect('screen',0)
   await e.key('select');await e.key('square');await e.shot('v081-psp-notice',5);await e.waitframes(260);await e.shot('v081-psp-position',5)
   pos=await e.read('position','ff');await choose(9);await expect('mapcategory',0);assert await e.read('position','ff')==pos
   pin=await e.read('mappin','ff');await e.shot('v081-psp-town',5)
   await e.key('triangle');await expect('mapzoom',1);assert await e.read('mappin','ff')==pin;await e.shot('v081-psp-local',5)
   await e.key('triangle');await expect('mapzoom',0);await expect('mapchoice',9)
   await e.key('rtrigger');await expect('mapcategory',1);await choose(1);eventpin=await e.read('mappin','ff');await e.shot('v081-psp-event',5)
   await e.key('triangle');await expect('mapzoom',1);await e.key('triangle');await expect('mapcategory',1);assert await e.read('mappin','ff')==eventpin
   await e.key('ltrigger');await expect('mapcategory',0);await choose(9);assert await e.read('mappin','ff')==pin
   await e.key('cross');await expect('screen',0);await expect('waypoint',1);assert await e.read('travel','ff')==pin
   e.events.append({'completed':True,'prepared_saves':[4,5],'named_destination':'Black Mesa','category_switching':True,'local_zoom_preserves_destination':True,'event_zoom_preserves_destination':True,'travel_matches_selected_pin':True,'dpad_preserves_player_position':True,'your_position_does_not_issue_travel':True})
   (ROOT/'evidence/v081-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('ATLAS PSP CONTROLLER PASS',run,flush=True)
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())

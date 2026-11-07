"""Focused controller qualification of 0.8.0; prepared native save fixtures."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys
import websockets
from emulator_v060 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v080-',dir=ROOT/'build'));shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in [2,3]:shutil.copy2(ROOT/'build'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w');proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=600','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.events[0]['version']='0.8.0';e.run=run;await asyncio.sleep(.8);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def tap(k):await e.key(k)
   async def expect(field,val):assert (await e.read(field))[0]==val,(field,await e.read(field),val)
   await tap('down');await tap('down');await tap('cross');await tap('down');await tap('down');await tap('cross');await tap('cross');await expect('screen',0)
   pos=await e.read('position','ff');await tap('up');await expect('screen',0);assert await e.read('position','ff')==pos;await tap('up')
   await tap('down');await expect('screen',2);await e.shot('v080-psp-skills',2);await tap('circle')
   await e.call('input.buttons.send',buttons={'ltrigger':True});await e.waitframes(4);await tap('square');await e.call('input.buttons.send',buttons={'ltrigger':False});await expect('screen',34)
   await e.shot('v080-psp-opponents',34);await tap('cross');await expect('screen',0);await expect('command',1);assert (await e.read('bounty'))[0]>=100
   await e.shot('v080-psp-target',0);await tap('square');await expect('command',4294967295)
   await tap('start');await tap('rtrigger');await tap('rtrigger');await tap('down');await tap('down');await tap('cross');await expect('screen',19)
   await tap('rtrigger');await tap('rtrigger');await expect('journalTab',2);await e.shot('v080-psp-events',19)
   await tap('square');await expect('screen',5);await e.shot('v080-psp-map-local',5);await tap('triangle');await e.shot('v080-psp-map-world',5)
   # Load the sixteen-member army fixture via Pause > Load.
   await tap('start');await tap('start');await tap('down');await tap('down');await tap('cross');await expect('screen',22)
   await tap('down');await tap('down');await tap('down');await tap('cross');await tap('cross');await expect('screen',0)
   await e.shot('v080-psp-army',0)
   await tap('select');await e.shot('v080-psp-survey-local',5);await tap('triangle');await e.shot('v080-psp-survey-world',5)
   e.events.append({'completed':True,'prepared_fixtures':[2,3],'neutral_assault':True,'target_cancel':True,'dpad_preserved_position':True,'living_journal_and_map':True,'army_fixture':16})
   (ROOT/'evidence/v080-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP CONTROLLER PASS',run,flush=True)
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())

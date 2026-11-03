#!/usr/bin/env python3
"""v0.2 PSP button-driven playthrough; all images are EBOOT VRAM captures."""
import asyncio, base64, json, pathlib, shutil, subprocess, tempfile, sys, time
import websockets
from PIL import Image
import emulator_test as base
from emulator_test import ROOT,Emulator
class Play(Emulator):
 async def waitframes(self,n):
  before=(await self.read('framecount'))[0]
  for retry in range(400):
   if (await self.read('framecount'))[0]>=before+n:return
   await asyncio.sleep(.005)
  raise TimeoutError('PSP frame loop stopped')
 async def key(self,k,n=1):
  counts={13:4,15:10,17:9,22:7,11:16,1:24,18:5}
  for _ in range(n):
   before=(await self.read('selection'))[0];screen=(await self.read('screen'))[0]
   if k in ('up','down') and screen in counts:
    target=(before+(-1 if k=='up' else 1))%counts[screen]
    for retry in range(12):
     current=(await self.read('selection'))[0]
     if current==target:break
     direction='down' if (target-current)%counts[screen]<counts[screen]/2 else 'up'
     await self.press(direction,1);await self.waitframes(2)
    assert (await self.read('selection'))[0]==target,(screen,k,before,target)
   else:
    await self.call('input.buttons.send',buttons={k:True});await self.waitframes(2)
    await self.call('input.buttons.send',buttons={k:False});await self.waitframes(2)
   print('KEY',k,'SCREEN',screen,'ROW',before,'->',(await self.read('selection'))[0],flush=True)
 async def menu(self,index):
  await self.key('start');await self.key('down',index);await self.key('cross');await asyncio.sleep(.1)
 async def load(self,slot):
  await self.menu(5);await self.key('down',slot);await self.key('cross',2);await asyncio.sleep(.15)
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v02-playtest-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in range(2,7):shutil.copy2(ROOT/'build'/'emu'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch()
 if len(sys.argv)>2 and sys.argv[2]=='--advanced':(run/'DUST_RECORD').touch()
 log=open(run/'ppsspp.log','w');exe=pathlib.Path(sys.argv[1]).resolve()
 proc=subprocess.Popen([str(exe),'--graphics=software','--debugger=9251','--timeout=900','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   advanced=len(sys.argv)>2 and sys.argv[2]=='--advanced'
   if advanced:
    await e.key('down',2);await e.key('cross');await e.key('down',2);await e.key('cross',2)
   else:
    await e.shot('v02-01-title',13)
    await e.key('cross');await e.key('cross');await e.shot('v02-08-name-keyboard',16)
    await e.key('cross');await e.key('start');await e.key('down');await e.key('right')
    await e.shot('v02-02-character-creator',15)
    await e.key('up',2);await e.key('cross');await e.expect('money',420)
    await e.shot('v02-03-dusk-haven',0)
    await e.key('triangle');await e.shot('v02-04-layered-equipment',1)
    await e.key('triangle');await e.shot('v02-05-six-equipment-slots',23)
    await e.key('circle');await e.move_to(24,30.5);await e.key('cross');await e.shot('v02-09-companion-dialogue',17)
    await e.key('down',6);await e.key('cross');await e.expect('squad',2);await e.expect('money',120)
    await e.move_to(28,25.5);await e.key('cross');await e.shot('v02-10-market-dialogue',17);await e.key('cross');await e.key('cross');await e.expect('money',85)
    await e.key('rtrigger',2);await e.shot('v02-11-weapon-market',6);await e.key('circle')
    await e.move_to(32,29);await e.key('cross');await e.key('triangle');pack=await e.read('pack','48I');leather=next(i//2 for i in range(0,48,2) if pack[i]==18 and pack[i+1]>0)
    await e.key('down',leather);await e.key('cross');assert (await e.read('gear','6I'))[1]==18
    await e.shot('v02-12-looted-armor-equipped',1);await e.key('circle')
    await e.move_to(24,34);await e.key('cross');await asyncio.sleep(3.5);pack=await e.read('pack','48I');assert any(pack[i]==3 and pack[i+1]>0 for i in range(0,48,2))
    await e.shot('v02-13-mining',0);await e.key('right')
    await e.menu(4);await e.shot('v02-14-save-ledger',22);await e.key('down');await e.key('cross',2);assert (run/'DUST_1.sav').exists();await e.key('circle')
    await e.load(1);await e.expect('money',85);await e.expect('squad',2);assert (await e.read('gear','6I'))[1]==18
    await e.key('select');await e.shot('v02-06-frontier-atlas',5);await e.key('select');await e.shot('v02-07-contextual-guide',12)
    for _ in range(18):await e.key('rtrigger')
    await e.expect('selection',2);await e.key('circle',2)
   if not advanced:
    e.events.append({'normal_run_saved_reloaded':True,'starting_origin':'Ronin','ending_silver':85,'ending_squad':2})
    (ROOT/'evidence'/'v02-emulator-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
    print('NORMAL PSP PLAYTHROUGH PASS',run,flush=True)
    return
   await e.expect('screen',0);await e.expect('squad',3)
   if advanced:
    for retry in range(120):
     files=sorted(run.glob('QA_FRAME_*.rgba'))
     if len(files)>=100:break
     await asyncio.sleep(.1)
    await e.key('rtrigger')
    import csv
    records=list(csv.DictReader((run/'QA_FRAMES.csv').open()))
    frames=[];phases=[];durations=[]
    for i,rec in enumerate(records):
     path=run/f"QA_FRAME_{int(rec['frame']):03d}.rgba"
     if path.stat().st_size!=512*272*4:continue
     frame=Image.frombytes('RGBA',(512,272),path.read_bytes()).crop((0,0,480,272)).convert('RGB')
     frames.append(frame);phases.append(int(rec['attack_phase']))
     durations.append(max(40,min(200,int((float(records[i+1]['time'])-float(rec['time']))*1000))) if i+1<len(records) else 80)
     if phases[-1]==2:frame.save(ROOT/'evidence'/'v02-25-weapon-contact.png')
    assert 1 in phases and 2 in phases,phases
    frames[0].save(ROOT/'evidence'/'v02-15-combat.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0)
    shutil.copy2(run/'QA_FRAMES.csv',ROOT/'evidence'/'v02-combat-frames.csv')
    print('RECORDED EBOOT COMBAT',len(frames),'frames; phases',sorted(set(phases)),flush=True)
    await e.key('rtrigger')
   else:phases=[]
   await e.shot('v02-16-combat-aftermath',0);await e.key('triangle');await e.key('rtrigger',2);await e.shot('v02-17-limb-health',3);await e.key('circle')
   await e.load(3);await e.shot('v02-18-outpost',0)
   await e.move_to(42.5,47.5);await e.menu(11);await e.key('right');await e.key('down');await e.key('right',2);await e.key('down');await e.key('right',3);await e.key('down');await e.key('right',2);await e.key('down');await e.shot('v02-19-gear-forge',18);await e.key('cross');forged=await e.read('pack','48I');assert any(forged[i]==866 and forged[i+1]>0 for i in range(0,48,2));e.events.append({'assert':'fine steel lamellar forged through PSP UI','item':866,'passed':True});await e.key('circle')
   await e.load(4);await e.shot('v02-20-shoguns-rest',0);await e.key('select');await e.shot('v02-21-surveyed-world',5);await e.key('circle');await e.menu(13);await e.shot('v02-22-faction-ledger',20);await e.key('circle')
   await e.load(5);await e.shot('v02-23-snow-highlands',0)
   await e.load(6);await e.shot('v02-24-pine-reach',0)
   f0=(await e.read('framecount'))[0];t0=time.monotonic();await asyncio.sleep(5);f1=(await e.read('framecount'))[0];elapsed=time.monotonic()-t0
   e.events.append({'unthrottled_host_frames_per_second':(f1-f0)/elapsed,'environment':'PPSSPP headless software renderer, host wall time; not hardware performance'})
   e.events.append({'combat_phases_observed':phases,'normal_run_saved_reloaded':not advanced,'prepared_fixture_slots':[2,3,4,5,6]})
   (ROOT/'evidence'/('v02-advanced-checks.json' if advanced else 'v02-emulator-checks.json')).write_text(json.dumps(e.events,indent=2)+'\n')
   print('V02 PLAYTHROUGH PASS',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close();shutil.copy2(run/'ppsspp.log',ROOT/'evidence'/'v02-ppsspp.log')
if __name__=='__main__':asyncio.run(main())

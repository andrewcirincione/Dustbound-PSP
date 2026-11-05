#!/usr/bin/env python3
"""v0.5.1 PSP controller playthrough with read-only completed-frame capture."""
import asyncio, base64, struct, json, pathlib, shutil, subprocess, tempfile, sys, time, hashlib, math
import websockets
from PIL import Image
import emulator_test as base
from emulator_test import ROOT,Emulator
class Play(Emulator):
 def __init__(self,ws):
  super().__init__(ws)
  self.events.append({"eboot_sha256":hashlib.sha256((ROOT/"EBOOT.PBP").read_bytes()).hexdigest(),"version":"0.5.1"})
 async def read(self,field,fmt='I'):
  for _ in range(300):
   try:
    snapshot=json.loads((self.run/'DUST_STATE.json').read_text())
    data=bytes.fromhex(snapshot[field+'_value'])
    return struct.unpack('<'+fmt,data[:struct.calcsize('<'+fmt)])
   except (FileNotFoundError,json.JSONDecodeError): await asyncio.sleep(.01)
  raise TimeoutError('PSP telemetry did not complete')
 async def shot(self,name,expected=None):
  self.capture_sequence=getattr(self,'capture_sequence',0)+1
  seq=self.capture_sequence
  request=self.run/'DUST_CAPTURE_REQUEST.tmp'
  request.write_text(str(seq))
  request.replace(self.run/'DUST_CAPTURE_REQUEST')
  for _ in range(1000):
   try:
    meta=json.loads((self.run/f'DUST_CAPTURE_{seq}.json').read_text())
    raw=(self.run/f'DUST_CAPTURE_{seq}.rgba').read_bytes()
    if len(raw)==512*272*4:break
   except (FileNotFoundError,json.JSONDecodeError):pass
   await asyncio.sleep(.01)
  else:raise TimeoutError('PSP framebuffer capture did not complete')
  if expected is not None:assert meta['screen']==expected,(name,meta,expected)
  Image.frombytes('RGBA',(512,272),raw).crop((0,0,480,272)).convert('RGB').save(ROOT/'evidence'/(name+'.png'))
  self.events.append({'capture':name,'screen':meta['screen'],'expected':expected,'frame':meta['frame'],'method':'EBOOT completed framebuffer; QA request does not modify state'})
  print('SCREENSHOT',name,'480x272','screen',meta['screen'],flush=True)
  return meta['screen']
 async def waitframes(self,n):
  before=(await self.read('framecount'))[0]
  for retry in range(400):
   if (await self.read('framecount'))[0]>=before+n:return
   await asyncio.sleep(.02)
  raise TimeoutError('PSP frame loop stopped')
 async def key(self,k,n=1):
  counts={13:4,15:10,22:7,11:16,1:24,18:5,17:getattr(self,"dialog_count",4),6:getattr(self,"shop_count",10)}
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
    await self.press(k,5);await self.waitframes(4)
   print('KEY',k,'SCREEN',screen,'ROW',before,'->',(await self.read('selection'))[0],flush=True)
 async def move_to(self,x,y):
  for attempt in range(400):
   px,py=await self.read('position','ff')
   if math.hypot(x-px,y-py)<.3:return
   sx,sy=x-px-y+py,x-px+y-py
   horizontal=abs(sx)>abs(sy);v=sx if horizontal else sy
   key=('right' if v>0 else 'left') if horizontal else ('down' if v>0 else 'up')
   await self.call('input.buttons.send',buttons={key:True});await self.waitframes(2)
   await self.call('input.buttons.send',buttons={key:False});await self.waitframes(1)
  await self.shot('v051-route-debug')
  raise AssertionError(('route blocked',x,y,px,py,await self.read('health','6f')))
 async def menu(self,index):
  await self.key('start');await self.key('down',index);await self.key('cross');await asyncio.sleep(.1)
 async def load(self,slot):
  await self.menu(5);await self.key('down',slot);await self.key('cross',2);await asyncio.sleep(.15)
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v051-playtest-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in range(2,7):shutil.copy2(ROOT/'build'/'emu'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch()
 if len(sys.argv)>2 and sys.argv[2]=='--advanced':(run/'DUST_RECORD').touch()
 log=open(run/'ppsspp.log','w');exe=pathlib.Path(sys.argv[1]).resolve()
 proc=subprocess.Popen([str(exe),'--graphics=software','--debugger=9251','--timeout=3600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   advanced=len(sys.argv)>2 and sys.argv[2]=='--advanced'
   if advanced:
    await e.key('down',2);await e.key('cross');await e.key('down',2);await e.key('cross',2)
   else:
    await e.shot('v051-01-title',13)
    await e.key('cross');await e.key('cross');await e.shot('v051-08-name-keyboard',16)
    await e.key('cross');await e.key('start');await e.key('down');await e.key('right')
    await e.shot('v051-02-character-creator',15)
    await e.key('up',2);await e.key('cross');await e.expect('money',420)
    await e.shot('v051-03-dusk-haven',0)
    for fx,fy in [(24,35.7),(22.3,34),(24,32.3),(25.7,34),(22.8,35.2),(25.2,35.2),(22.8,32.8),(25.2,32.8)]:
     await e.move_to(fx,fy)
     print('RESOURCE REACH PROBE',await e.read('position','ff'),await e.read('context','ii'),flush=True)
     if (await e.read('context','ii'))[0]==1:break
    await e.shot('v051-34-mining-too-far',0)
    assert (await e.read('context','ii'))[0]==1
    ore_before=await e.read('pack','48I');await e.key('cross');await e.expect('task',0);await e.shot('v051-34-mining-too-far',0);await e.waitframes(100);assert await e.read('pack','48I')==ore_before
    await e.move_to(24,34);await e.key('cross');await e.expect('task',1);await e.waitframes(120);pack=await e.read('pack','48I');assert any(pack[i]==3 and pack[i+1]>0 for i in range(0,48,2));await e.shot('v051-13-mining',0);await e.key('right')
    await e.key('triangle');await e.shot('v051-04-layered-equipment',1)
    await e.key('triangle');await e.shot('v051-05-six-equipment-slots',23)
    await e.key('circle');await e.move_to(24,30.5);await e.key('cross');await e.shot('v051-09-companion-dialogue',17)
    await e.key('cross');await e.expect('squad',2);await e.expect('money',120)
    await e.move_to(28,25.5);await e.key('cross');await e.shot('v051-10-market-dialogue',17);await e.key('cross');await e.key('cross');await e.expect('money',85)
    await e.key('rtrigger',2);await e.shot('v051-11-weapon-market',6);await e.key('circle')
    await e.move_to(32,29);await e.key('cross');await e.key('triangle');pack=await e.read('pack','48I');leather=next(i//2 for i in range(0,48,2) if pack[i]==18 and pack[i+1]>0)
    await e.key('down',leather);await e.shot('v051-33-gear-comparison',1);await e.key('cross');assert (await e.read('gear','6I'))[1]==18
    await e.shot('v051-12-looted-armor-equipped',1);await e.key('circle')
    await e.key('right')
    await e.menu(4);await e.shot('v051-14-save-ledger',22);await e.key('down');await e.key('cross',2);assert (run/'DUST_1.sav').exists();await e.key('circle')
    await e.load(1);await e.expect('money',85);await e.expect('squad',2);assert (await e.read('gear','6I'))[1]==18
    await e.key('select');await e.shot('v051-06-frontier-atlas',5);await e.expect('mapzoom',1);await e.key('triangle');await e.expect('mapzoom',0);await e.shot('v051-35-world-atlas',5);await e.key('select');await e.shot('v051-07-contextual-guide',12)
    for _ in range(20):await e.key('rtrigger')
    await e.expect('selection',2);await e.key('circle',2)
   if not advanced:
    e.events.append({'normal_run_saved_reloaded':True,'starting_origin':'Ronin','ending_silver':85,'ending_squad':2})
    (ROOT/'evidence'/'v051-emulator-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
    print('NORMAL PSP PLAYTHROUGH PASS',run,flush=True)
    return
   await e.expect('screen',0);await e.expect('squad',3)
   if advanced:
    for retry in range(600):
     files=sorted(run.glob('QA_FRAME_*.rgba'))
     if len(files)>=240:break
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
     durations.append(max(10,min(200,(round(float(records[i+1]['time'])*100)-round(float(rec['time'])*100))*10)) if i+1<len(records) else 30)
     if phases[-1]==2:frame.save(ROOT/'evidence'/'v051-25-weapon-contact.png')
    assert 1 in phases and 2 in phases,phases
    frames[0].save(ROOT/'evidence'/'v051-15-combat.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0)
    shutil.copy2(run/'QA_FRAMES.csv',ROOT/'evidence'/'v051-combat-frames.csv')
    print('RECORDED EBOOT COMBAT',len(frames),'frames; phases',sorted(set(phases)),flush=True)
    await e.key('rtrigger')
   else:phases=[]
   await e.shot('v051-16-combat-aftermath',0);await e.key('triangle');await e.key('rtrigger',2);await e.shot('v051-17-limb-health',3);await e.key('circle')
   await e.load(3);await e.shot('v051-18-outpost',0)
   await e.move_to(42.5,47.5);await e.menu(11);await e.key('right');await e.key('down');await e.key('right',2);await e.key('down');await e.key('right',3);await e.key('down');await e.key('right',2);await e.key('down');await e.shot('v051-19-gear-forge',18);await e.key('cross');forged=await e.read('pack','48I');assert any(forged[i]==866 and forged[i+1]>0 for i in range(0,48,2));e.events.append({'assert':'fine steel lamellar forged through PSP UI','item':866,'passed':True});await e.key('circle')
   await e.load(4);await e.shot('v051-20-shoguns-rest',0);await e.key('select');await e.shot('v051-21-surveyed-world',5);await e.key('circle');await e.menu(13);await e.shot('v051-22-faction-ledger',20);await e.key('circle')
   await e.load(5);await e.shot('v051-23-snow-highlands',0)
   await e.load(6);await e.shot('v051-24-pine-reach',0)
   f0=(await e.read('framecount'))[0];t0=time.monotonic();await asyncio.sleep(5);f1=(await e.read('framecount'))[0];elapsed=time.monotonic()-t0
   e.events.append({'unthrottled_host_frames_per_second':(f1-f0)/elapsed,'environment':'PPSSPP headless software renderer, host wall time; not hardware performance'})
   e.events.append({'combat_phases_observed':phases,'normal_run_saved_reloaded':not advanced,'prepared_fixture_slots':[2,3,4,5,6]})
   (ROOT/'evidence'/('v051-advanced-checks.json' if advanced else 'v051-emulator-checks.json')).write_text(json.dumps(e.events,indent=2)+'\n')
   print('V051 PLAYTHROUGH PASS',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close();shutil.copy2(run/'ppsspp.log',ROOT/'evidence'/'v051-ppsspp.log')
if __name__=='__main__':asyncio.run(main())

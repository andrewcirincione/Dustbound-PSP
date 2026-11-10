#!/usr/bin/env python3
"""PPSSPP input/readback bridge. The emulator and debugger share one sandbox network."""
import asyncio,json,os,shutil,struct,subprocess,sys,time
from pathlib import Path
import websockets
from PIL import Image
from emulator_test import Emulator
ROOT=Path(__file__).resolve().parents[1]
RUN=ROOT/'build/v1000/emu'
GAME=RUN/'PSP/GAME/DUSTBOUND'
OUT=ROOT/'evidence/v1000'
CONTROL=RUN/'command.json'
RESULT=RUN/'result.json'

def state():
 try:return json.loads((GAME/'DUST_STATE.json').read_text())
 except (FileNotFoundError,json.JSONDecodeError):return {}
def number(k,fmt='I'):
 d=state();return struct.unpack('<'+fmt,bytes.fromhex(d[k+'_value']))[0]
async def frames(n=3):
 before=number('framecount');until=time.monotonic()+40
 while time.monotonic()<until:
  await asyncio.sleep(.04)
  if number('framecount')>=before+n:return
 raise TimeoutError(('frames',before,number('framecount')))
async def main():
 app=Path(sys.argv[1]).resolve()
 env=os.environ|{'XDG_CONFIG_HOME':str(RUN/'xdg'),'SDL_VIDEODRIVER':'offscreen','SDL_AUDIODRIVER':'dummy','LIBGL_ALWAYS_SOFTWARE':'1'}
 for p in [GAME/'DUST_STATE.json',RESULT,CONTROL]:p.unlink(missing_ok=True)
 log=(GAME/'ppsspp.log').open('w')
 proc=subprocess.Popen([str(app/'shared/lib/ld-linux-x86-64.so.2'),'--library-path',str(app/'shared/lib'),str(app/'shared/bin/PPSSPPSDL'),str(GAME/'EBOOT.PBP')],cwd=GAME,env=env,stdout=log,stderr=subprocess.STDOUT)
 try:
  ws=None
  for _ in range(250):
   try:ws=await websockets.connect('ws://127.0.0.1:9252/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  if ws is None:raise RuntimeError('PPSSPP debugger did not start')
  async with ws:
   emu=Emulator(ws)
   try:await emu.call('cpu.resume')
   except RuntimeError:pass
   for _ in range(400):
    if state():break
    await asyncio.sleep(.1)
   async def read(k):
    if k+'_value' in state():return number(k)
    emu.state=state();return (await emu.read(k))[0]
   async def key(k,n=3):
    await emu.call('input.buttons.send',buttons={k:True});await frames(n)
    await emu.call('input.buttons.send',buttons={k:False});await frames(3)
   async def choose(n):
    for _ in range(35):
     v=await read('selection')
     if v==n:return
     await key('down' if v<n else 'up')
    raise AssertionError(('selection',n,await read('selection')))
   async def load_scene(name):
    shutil.copy(ROOT/'build/v1000/fixtures'/(name+'.sav'),GAME/'DUST_1.sav')
    screen=await read('screen')
    if screen==13:await choose(1);await key('cross')
    else:
     if screen!=0:await key('start')
     if await read('screen')!=0:await key('start')
     await key('start')
     while await read('menutab')!=0:await key('ltrigger')
     await choose(2);await key('cross')
    assert await read('screen')==22,('saves',await read('screen'))
    await choose(1);await key('cross');assert await read('screen')==14
    await key('cross');assert await read('screen')==0,('loaded',name,await read('screen'))
   seq=100
   async def capture(name):
    nonlocal seq
    seq+=1;request=GAME/'DUST_CAPTURE_REQUEST'
    meta=GAME/('DUST_CAPTURE_%d.json'%seq);pixels=GAME/('DUST_CAPTURE_%d.rgba'%seq)
    meta.unlink(missing_ok=True);pixels.unlink(missing_ok=True);request.write_text(str(seq))
    metadata=None
    until=time.monotonic()+40
    while time.monotonic()<until:
     if meta.exists() and pixels.exists() and pixels.stat().st_size==512*272*4:
      try:metadata=json.loads(meta.read_text());break
      except json.JSONDecodeError:pass
     await asyncio.sleep(.05)
    assert pixels.exists() and meta.exists(),('capture',name)
    OUT.mkdir(parents=True,exist_ok=True)
    Image.frombytes('RGBA',(512,272),pixels.read_bytes()).crop((0,0,480,272)).convert('RGB').save(OUT/(name+'.png'))
    return metadata
   RESULT.write_text(json.dumps({'ready':True,'screen':await read('screen')}));print('BRIDGE_READY',await read('screen'),flush=True)
   while True:
    if not CONTROL.exists():await asyncio.sleep(.1);continue
    cmd=json.loads(CONTROL.read_text());CONTROL.unlink();responses=[]
    try:
     for step in cmd.get('steps',[]):
      if 'key' in step:await key(step['key'],step.get('frames',3))
      if 'choose' in step:await choose(step['choose'])
      if 'load' in step:await load_scene(step['load'])
      if 'wait' in step:await frames(step['wait'])
      if 'attack' in step:
       for _ in range(150):
        if await read('attack')==step['attack']:break
        await frames(1)
       else:raise AssertionError(('attack phase',step['attack']))
      if 'hold' in step:
       await emu.call('input.buttons.send',buttons={step['hold']:True});await frames(step.get('frames',10));await emu.call('input.buttons.send',buttons={step['hold']:False});await frames(2)
      if 'analog' in step:
       x,y=step['analog'];await emu.call('input.analog.send',x=x,y=y);await frames(step.get('frames',10));await emu.call('input.analog.send',x=0,y=0);await frames(2)
      if 'capture' in step:responses.append({'image':step['capture'],**(await capture(step['capture']))})
      if 'expect' in step:
       for k,v in step['expect'].items():assert await read(k)==v,(k,await read(k),v)
     response={'id':cmd.get('id'),'ok':True,'responses':responses,'state':state()}
    except Exception as ex:response={'id':cmd.get('id'),'ok':False,'error':repr(ex),'responses':responses,'state':state()}
    RESULT.write_text(json.dumps(response));print('DONE',cmd.get('id'),response['ok'],response.get('error',''),flush=True)
    if cmd.get('stop'):break
 finally:
  proc.terminate()
  try:proc.wait(5)
  except subprocess.TimeoutExpired:proc.kill();proc.wait()
  log.close()
if __name__=='__main__':asyncio.run(main())

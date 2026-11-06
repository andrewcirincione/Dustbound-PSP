#!/usr/bin/env python3
"""v0.6.0 PSP controller playthrough with read-only completed-frame capture."""
import asyncio, base64, struct, json, pathlib, shutil, subprocess, tempfile, sys, time, hashlib, math
import websockets
from PIL import Image
import emulator_test as base
from emulator_test import ROOT,Emulator
class Play(Emulator):
 def __init__(self,ws):
  super().__init__(ws)
  self.events.append({"eboot_sha256":hashlib.sha256((ROOT/"EBOOT.PBP").read_bytes()).hexdigest(),"version":"0.6.0"})
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
  counts={27:8,28:9,30:6,31:2,32:24,2:20,3:6,4:8,8:8,9:8,12:25,23:6,24:30,25:4,13:4,15:10,22:7,11:23,1:24,18:5,17:getattr(self,"dialog_count",4),6:getattr(self,"shop_count",10)}
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
  await self.shot('v052-route-debug')
  raise AssertionError(('route blocked',x,y,px,py,await self.read('health','6f')))
 async def menu(self,index):
  await self.key('start');await self.key('down',index);await self.key('cross');await asyncio.sleep(.1)
 async def load(self,slot):
  await self.menu(5);await self.key('down',slot);await self.key('cross',2);await asyncio.sleep(.15)

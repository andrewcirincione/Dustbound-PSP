"""Final-EBOOT button playthrough. Ordinary prepared saves, read-only telemetry/screenshots."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,time,hashlib
import websockets
from emulator_v060 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v090-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in range(7):shutil.copy2(ROOT/'build/v090'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=1200','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.9.0';await asyncio.sleep(.8);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def key(k,n=1):
    for _ in range(n):
     await e.press(k,5);await e.waitframes(12)
   async def expect(field,v):assert (await e.read(field))[0]==v,(field,await e.read(field),v)
   async def wait_log(fragment, limit=2000):
    for _ in range(limit):
     raw=bytes((await e.read('feed','1200B')))
     line=raw[:100].split(b'\0')[0].decode()
     if (await e.read('feedcount'))[0] and fragment.lower() in line.lower():return line
     await e.waitframes(1)
    raise AssertionError(('visible log missing',fragment,line))
   async def choose(n):
    for _ in range(40):
     current=(await e.read('selection'))[0]
     if current==n:return
     await key('down' if current<n else 'up')
    raise AssertionError(('selection',await e.read('selection'),n))
   async def load(slot,initial=False):
    if initial:await choose(2);await key('cross')
    else:
     for _ in range(4):
      if (await e.read('screen'))[0]==11:break
      await key('start')
     await expect('screen',11);await choose(2);await key('cross')
    await expect('screen',22);await choose(slot);await key('cross');await expect('screen',14);await key('cross');
    if (await e.read('screen'))[0]==14:await key('cross')
    await expect('screen',0)
   async def action_row(fragment):
    raw=bytes.fromhex(json.loads((run/'DUST_STATE.json').read_text())['actionlabels_value'])
    rows=[raw[i:i+80].split(b'\0')[0].decode() for i in range(0,len(raw),80)]
    return next(i for i,row in enumerate(rows) if fragment.lower() in row.lower())
   async def merchant():
    await key('cross');await expect('screen',35)
    raw=bytes.fromhex(json.loads((run/'DUST_STATE.json').read_text())['actionlabels_value'])
    rows=[raw[i:i+80].split(b'\0')[0].decode() for i in range(0,len(raw),80)]
    index=next(i for i,row in enumerate(rows) if 'MARA' in row and 'ATTACK' not in row.upper() and 'STEAL' not in row.upper())
    await choose(index);await key('cross');await expect('screen',17)
   await e.shot('v090-psp-title',13)
   await load(1,True);await key('triangle');await e.waitframes(150);await e.shot('v090-psp-inventory-worn',1)
   worn=await e.read('gear','6i');await choose(1);await e.shot('v090-psp-inventory-rapier',1);assert await e.read('gear','6i')==worn
   await choose(2);await e.shot('v090-psp-inventory-armor',1);assert await e.read('gear','6i')==worn
   await key('cross');await choose(3);await key('cross');await choose(0);await e.shot('v090-psp-inventory-disguise',1)
   await key('circle');await key('down');await e.shot('v090-psp-skills',2);await key('right');await expect('skillpage',1);await e.shot('v090-psp-weapon-classes',2)
   await key('circle');await key('select');await e.waitframes(180);await expect('mapcategory',0);await e.shot('v090-psp-atlas',5);await key('rtrigger');await expect('mapcategory',0);await e.shot('v090-psp-atlas-page',5);await key('triangle');await e.shot('v090-psp-local-map',5);await key('circle')
   await load(1);await merchant();await e.shot('v090-psp-merchant',17)
   await choose(0);await key('cross');await expect('screen',6);await e.waitframes(180);await e.shot('v090-psp-buy-prices',6)
   await key('right');await e.shot('v090-psp-sell-prices',6);await key('circle')
   await merchant();await choose(1);await key('cross');await e.shot('v090-psp-trading-tip',17)
   await key('circle');await merchant();await choose(2);await key('cross');await choose(5);await e.shot('v090-psp-work',17);await key('cross');await e.shot('v090-psp-job-accepted',17)
   await load(2);await key('rtrigger');await expect('paused',1);await key('cross');await expect('screen',35);await e.shot('v090-psp-actions',35)
   await choose(0);await key('cross');await expect('screen',35);await expect('queuedactions',1);await choose(1);await key('cross');await expect('queuedactions',2);await e.shot('v090-psp-queued-actions',35)
   await key('rtrigger');await expect('screen',0);await expect('paused',0);await e.waitframes(240);await expect('queuedactions',0);await e.shot('v090-psp-lock-opened',0)
   await load(3);await wait_log('too far away');await e.shot('v090-psp-employer-warning',0);assert (await e.read('jobwarned','24i'))[0]==1;assert (await e.read('jobnear','24f'))[0]==0
   await load(4);await key('rtrigger');await e.waitframes(180);assert (await e.read('bounty'))[0]==160;await e.shot('v090-psp-wanted',0);await key('start');await key('rtrigger',2);await choose(3);await key('cross');await e.shot('v090-psp-faction-warrants',20)
   await load(5);await e.waitframes(60);await e.shot('v090-psp-cannibal-capture',0);await expect('depth_captive',3)
   await wait_log('Cannibals are eating')
   assert (await e.read('blood','f'))[0]<95
   await key('rtrigger');await e.shot('v090-psp-cannibal-danger',0)
   await load(6);before=(await e.read('weaponxp','11f'))[0];await e.waitframes(120);after=(await e.read('weaponxp','11f'))[0];assert after>before,(before,after);await e.shot('v090-psp-combat-training',0);await key('down');await key('right');await e.shot('v090-psp-trained-weapon',2)
   await load(0);roster=(await e.read('squad'))[0];await key('rtrigger');await key('cross');await expect('screen',35)
   await choose(await action_row('Free from shackles: ORIN'));await e.shot('v090-psp-free-prisoners',35);await key('cross')
   await choose(await action_row('Free prisoner: Ilan'));await key('cross');await expect('queuedactions',2);await e.shot('v090-psp-queued-rescues',35)
   await key('rtrigger')
   for _ in range(60):
    if sum(v>0 for v in await e.read('rescuefreed','192i'))>=2:break
    await e.waitframes(30)
   freed=await e.read('rescuefreed','192i');assert 1 in freed and 2 in freed;assert (await e.read('squad'))[0]==roster
   await e.shot('v090-psp-freed-escape',0)
   await key('cross');await expect('screen',35);await choose(await action_row('Leave me alone: ORIN'));await e.shot('v090-psp-freed-options',35)
   await choose(await action_row('Recruit: ORIN'));await key('cross');assert (await e.read('squad'))[0]==roster+1
   await key('right');await e.shot('v090-psp-explicit-recruitment',4)
   e.events.append({'completed':True,'prepared_saves':list(range(7)),'hover_does_not_equip':True,'town_only_atlas':True,'queue_executed':True,'employer_warning':True,'cannibal_capture':True,'queued_other_prisoner_rescues':True,'following_does_not_join_squad':True,'independent_flight_or_following':True,'explicit_recruitment':True,'weapon_xp_before':before,'weapon_xp_after':after,'eboot_sha256':hashlib.sha256((run/'EBOOT.PBP').read_bytes()).hexdigest()})
   (ROOT/'evidence/v090-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP 0.9.0 CONTROLLER PASS',run,flush=True)
 except Exception:
  if 'e' in locals():
   try:await e.shot('v090-psp-failure')
   except Exception:pass
  raise
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())

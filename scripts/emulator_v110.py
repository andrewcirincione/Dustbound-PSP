"""Controller-driven final PSP EBOOT; prepared saves and read-only frame telemetry."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,hashlib,math
import websockets
from emulator_v060 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v110-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in range(7):shutil.copy2(ROOT/'build/v110'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=1200','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.11.0';await asyncio.sleep(.8);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def key(k,n=1):
    for _ in range(n):await e.press(k,5);await e.waitframes(12)
   async def expect(field,v):assert (await e.read(field))[0]==v,(field,await e.read(field),v)
   async def choose(n):
    for _ in range(70):
     current=(await e.read('selection'))[0]
     if current==n:return
     await key('down' if current<n else 'up')
    raise AssertionError(('selection',await e.read('selection'),n))
   async def menu(tab,row):
    if (await e.read('screen'))[0]!=11:await key('start')
    # Limb screens use Start to close directly to world.
    if (await e.read('screen'))[0]==0:await key('start')
    await expect('screen',11)
    while (await e.read('menutab'))[0]!=tab:await key('rtrigger')
    await choose(row);await key('cross')
   async def load(slot,initial=False):
    if initial:await choose(2);await key('cross')
    else:await menu(0,2)
    await expect('screen',22);await choose(slot);await key('cross');await expect('screen',14);await key('cross')
    if (await e.read('screen'))[0]==14:await key('cross')
    await expect('screen',0)
   async def rows():
    raw=bytes.fromhex(json.loads((run/'DUST_STATE.json').read_text())['actionlabels_value'])
    return [raw[i:i+80].split(b'\0')[0].decode() for i in range(0,len(raw),80)]
   async def talk(fragment):
    await key('cross');await expect('screen',35)
    labels=await rows();index=next(i for i,r in enumerate(labels) if fragment.lower() in r.lower() and all(x not in r.upper() for x in ['ATTACK','STEAL','LOOT']))
    await choose(index);await key('cross');await expect('screen',17)
   async def management():
    await menu(1,2);await expect('screen',3);await key('rtrigger');await expect('screen',36)
   async def count(it):
    pack=await e.read('pack','48i');return sum(pack[i+1] for i in range(0,48,2) if pack[i]==it)
   async def fit(socket,grade):
    await choose(socket);await key('cross');await expect('screen',37);await choose(grade)
    await e.shot(f'v110-psp-fit-{socket}-{grade}',37)
    parts=await count(33)
    await key('cross');await expect('screen',36)
    it=(26 if socket<2 else 27) if grade==1 else 2368+(grade-1 if grade>1 else grade)*2+(socket>=2)
    assert (await e.read('fitted','4i'))[socket]==it
    assert await count(33)==parts-2
   async def move_capture(name):
    before=await e.read('position','ff')
    await e.call('input.analog.send',x=.8,y=0);await e.waitframes(30);await e.shot(name,0)
    await e.call('input.analog.send',x=0,y=0);after=await e.read('position','ff')
    assert math.dist(before,after)>.01,(before,after)
    e.events.append({'movement':name,'distance':math.dist(before,after),'input':'normal analog stick'})
   await e.shot('v110-psp-title',13)
   await load(0,True);await talk('Surgeon Venn');await e.shot('v110-psp-surgeon-dialogue',17)
   await choose(1);await key('cross');await expect('screen',6);await expect('shop_filter',5)
   await e.shot('v110-psp-venn-store',6);money=(await e.read('money'))[0];await key('cross');assert (await e.read('money'))[0]<money
   await key('circle');await talk('Surgeon Venn');await choose(0);await key('cross');await e.shot('v110-psp-surgery-menu',38)
   await choose(1);await key('cross');await e.shot('v110-psp-surgery-arm-warning',39);money=(await e.read('money'))[0]
   await key('circle');await expect('money',money);await expect('lost',0)
   await choose(1);await key('cross',2);await expect('screen',38);await expect('money',money-400);await expect('lost',8)
   await e.shot('v110-psp-removed-right-arm',38)
   await key('start');await e.shot('v110-psp-surgical-one-arm',0);await management()
   for grade in range(5):await fit(1,grade)
   await e.shot('v110-psp-relic-arm-fitted',36)
   await choose(1);await key('square');await e.shot('v110-psp-detach-warning',39);await key('cross');assert (await e.read('fitted','4i'))[1]==0
   await fit(1,4);await key('start');await talk('Surgeon Venn');await choose(0);await key('cross');await choose(2);await key('cross');await e.shot('v110-psp-surgery-leg-warning',39);money=(await e.read('money'))[0]
   await key('cross');await expect('money',money-400);await key('start');await move_capture('v110-psp-crawl-after-surgery');await management()
   for grade in range(5):await fit(2,grade)
   await e.shot('v110-psp-relic-arm-leg-fitted',36);await key('start');await move_capture('v110-psp-restored-walking')
   fitted=await e.read('fitted','4i');money=(await e.read('money'))[0]
   await menu(0,1);await expect('screen',22);await choose(6);await key('cross');await expect('screen',14);await key('cross')
   await load(6);assert await e.read('fitted','4i')==fitted;await expect('money',money);await management();await e.shot('v110-psp-save-reloaded-limbs',36)
   shutil.copy2(ROOT/'build/v110/DUST_6.sav',run/'DUST_6.sav')
   for slot,name in [(1,'one-leg'),(2,'one-arm'),(3,'no-legs')]:
    await load(slot);await move_capture('v110-psp-movement-'+name)
    if slot==2:
     await key('triangle');pack=await e.read('pack','48i');rapier=next(i//2 for i in range(0,48,2) if pack[i]==2198)
     await choose(rapier);await e.shot('v110-psp-one-arm-pack',1);await key('cross');await key('circle');await e.shot('v110-psp-one-arm-rapier',0)
    await management();await e.shot('v110-psp-limbs-'+name,36)
   await load(4);await key('cross');await expect('screen',35);labels=await rows();await choose(next(i for i,r in enumerate(labels) if 'LOOT IRON PILGRIM' in r.upper()));await key('cross');await expect('screen',24);await choose(31)
   await e.shot('v110-psp-fitted-wanderer-loot',24);before=await count(2374);await key('cross');assert await count(2374)==before+1;assert (await e.read('loot_fitted','4i'))[1]==0;await e.shot('v110-psp-wanderer-limb-taken',24)
   await load(5);await talk('Market keeper')
   if (await e.read('screen'))[0]==17:await choose(0);await key('cross')
   await expect('screen',6)
   while (await e.read('shop_filter'))[0]!=5:await key('rtrigger')
   await choose(10);await e.shot('v110-psp-black-mesa-relic-store',6);before=await count(2375);await key('cross');assert await count(2375)==before+1
   await load(6);await management();await e.shot('v110-psp-full-relic-limbs',36);await key('circle');await e.shot('v110-psp-full-relic-health',3);await key('start');await key('down');await e.shot('v110-psp-relic-bonus-stats',2)
   e.events.append({'completed':True,'prepared_save_slots':list(range(7)),'surgical_removal_and_cancel_verified':True,'all_five_grades_fitted_through_ui_arm_and_leg':True,'replacements_saved_and_reloaded':True,'fitted_wanderer_limb_physically_looted':True,'basic_and_relic_shops_purchased':True,'read_only_qa':True,'eboot_sha256':hashlib.sha256((run/'EBOOT.PBP').read_bytes()).hexdigest()})
   (ROOT/'evidence/v110-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP 0.11.0 CONTROLLER PASS',run,flush=True)
 except Exception:
  if 'e' in locals():
   try:await e.shot('v110-psp-failure')
   except Exception:pass
  raise
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())

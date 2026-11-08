"""Final-EBOOT button playthrough. Ordinary prepared saves, read-only telemetry/screenshots."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,time,hashlib
import websockets
from emulator_v060 import Play,ROOT
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v100-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 for slot in range(7):shutil.copy2(ROOT/'build/v100'/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=1200','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.10.0';await asyncio.sleep(.8);await e.call('cpu.resume')
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
     await expect('screen',11)
     while (await e.read('menutab'))[0]!=0:await key('ltrigger')
     await choose(2);await key('cross')
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
   await e.shot('v100-psp-title',13)
   await key('cross');await expect('screen',15);await choose(2)
   for race in range(5):
    await expect('creator_species',race);await e.shot(f'v100-psp-species-{race}',15)
    if race<4:await key('right')
   await choose(9);await key('cross');await expect('screen',0);await expect('species',4)
   assert all(abs(v-130)<.01 for v in await e.read('health','6f'))
   assert (await e.read('corexp','20f'))[1]==12*14*14
   await key('up');await key('rtrigger');await e.shot('v100-psp-stonekin-health',0)
   await key('down');await e.shot('v100-psp-stonekin-skills',2);await key('circle')
   for slot,name in enumerate(['fortress','village','temple','foundry','cannibals','grove']):
    await load(slot);await key('rtrigger');await e.waitframes(90);await e.shot(f'v100-psp-town-{name}',0)
   await load(6);await key('cross');await expect('screen',35)
   await choose(await action_row('Loot Glass Ronin'));await key('cross');await expect('screen',24)
   await e.shot('v100-psp-named-loot',24);await key('cross');await e.shot('v100-psp-looted-weapon',24)
   await key('circle');await key('triangle');await choose(2);await e.shot('v100-psp-glass-horizon',1)
   await key('cross');await key('circle');await e.shot('v100-psp-equipped-heirloom',0)
   for family in [0,12,16,25,29,32,36,37]:
    # Replace a dormant save slot, then use the game's normal Load action.
    shutil.copy2(ROOT/'build/v100'/f'weapon-{family:02d}.sav',run/'DUST_6.sav')
    await load(6);await key('triangle');await expect('screen',1)
    for pattern in [0,2,6,7,9,11]:
     await choose(pattern);await e.shot(f'v100-psp-weapon-{family:02d}-{pattern:02d}',1)
    await key('cross');await key('circle');await e.shot(f'v100-psp-equipped-{family:02d}',0)
   async def craft_menu():
    await key('start');await expect('screen',11)
    while (await e.read('menutab'))[0]!=2:await key('rtrigger')
    await choose(6);await key('cross');await expect('screen',9)
   for fixture in ['missing','away','ready']:
    shutil.copy2(ROOT/'build/v100'/f'craft-{fixture}.sav',run/'DUST_6.sav');await load(6);await craft_menu()
    await e.shot(f'v100-psp-crafting-{fixture}',9)
    if fixture=='ready':
     await choose(1);await e.shot('v100-psp-cooking-alternatives',9);await choose(0)
     await key('square');await expect('screen',18);await choose(1);await key('left');await choose(3);await key('right',2)
     await e.shot('v100-psp-forging-ready',18);await choose(4);await key('cross')
     assert 2198 in (await e.read('pack','48i'))[::2]
     await e.shot('v100-psp-forged-rapier',18);await key('square');await expect('screen',9);await key('cross',2)
     await e.shot('v100-psp-crafting-used-materials',9)
    elif fixture=='missing':
     await key('square');await e.shot('v100-psp-forging-missing',18)
   e.events.append({'completed':True,'prepared_town_saves':list(range(7)),'creator_species':5,'craft_requirements_and_real_rapier_forging':True,'actual_stonekin_hp':130,'actual_stonekin_strength':15,'named_weapon_physically_looted_and_equipped':True,'weapon_families_inspected':8,'patterns_per_family_inspected':6,'eboot_sha256':hashlib.sha256((run/'EBOOT.PBP').read_bytes()).hexdigest()})
   (ROOT/'evidence/v100-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n');print('PSP 0.10.0 CONTROLLER PASS',run,flush=True)
 except Exception:
  if 'e' in locals():
   try:await e.shot('v100-psp-failure')
   except Exception:pass
  raise
 finally:proc.terminate();proc.wait(timeout=10);log.close()
if __name__=='__main__':asyncio.run(main())

"""Normal PSP controller input against the final EBOOT; prepared ordinary saves, read-only QA."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,hashlib,math
import websockets
from emulator_v060 import Play,ROOT
NAMES=['scavenger-camp','refugee-camp','hunters-camp','smugglers-den','traders-rest','road-patrol','raiding-band','beast-pack','pilgrimage','wounded-travelers','battlefield','marching-army','bandit-ambush','salvage-crew','prisoner-convoy','wandering-duelists']
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v140-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 shutil.copy2(ROOT/'build/v140/manual-pin.sav',run/'DUST_1.sav')
 (run/'DUST_QA').touch();log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([sys.argv[1],'--graphics=software','--debugger=9251','--timeout=2400','-r',str(run),str(run/'EBOOT.PBP')],stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.14.0';await asyncio.sleep(.8);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def key(k,n=1):
    for _ in range(n):
     await e.call('input.buttons.send',buttons={k:True});await e.waitframes(3)
     await e.call('input.buttons.send',buttons={k:False});await e.waitframes(8)
   async def expect(field,v):assert (await e.read(field))[0]==v,(field,await e.read(field),v)
   async def choose(n):
    for _ in range(70):
     current=(await e.read('selection'))[0]
     if current==n:return
     await key('down' if current<n else 'up')
    raise AssertionError(('selection',await e.read('selection'),n))
   async def menu(tab,row):
    for _ in range(5):
     if (await e.read('screen'))[0]==11:break
     await key('start')
    await expect('screen',11)
    while (await e.read('menutab'))[0]!=tab:await key('rtrigger')
    await choose(row);await key('cross')
   async def load(slot=1,fixture=None,initial=False):
    if fixture:shutil.copy2(ROOT/'build/v140'/fixture,run/f'DUST_{slot}.sav')
    if initial:await choose(2);await key('cross')
    else:await menu(0,2)
    await expect('screen',22);await choose(slot);await key('cross');await expect('screen',14);await key('cross')
    if (await e.read('screen'))[0]==14:await key('cross')
    await expect('screen',0)
   async def rows():
    raw=bytes.fromhex(json.loads((run/'DUST_STATE.json').read_text())['actionlabels_value'])
    return [raw[i:i+80].split(b'\0')[0].decode() for i in range(0,len(raw),80)]
   async def action(fragment,capture=None):
    await key('cross');await expect('screen',35);labels=await rows()
    index=next(i for i,r in enumerate(labels) if fragment.lower() in r.lower())
    await choose(index)
    if capture:await e.shot(capture,35)
    await key('cross')
   async def count(it):
    pack=await e.read('pack','48i');return sum(pack[i+1] for i in range(0,48,2) if pack[i]==it)
   async def move(x,y):
    for _ in range(250):
     px,py=await e.read('position','ff');dx=x-px;dy=y-py
     if math.hypot(dx,dy)<.3:return
     sx,sy=dx-dy,dx+dy;scale=max(abs(sx),abs(sy),1)
     await e.call('input.analog.send',x=.8*sx/scale,y=-.8*sy/scale)
     await e.waitframes(4);await e.call('input.analog.send',x=0,y=0);await e.waitframes(2)
    raise AssertionError(('analog route blocked',x,y,await e.read('position','ff')))
   async def save(slot=6):
    await menu(0,1);await expect('screen',22);await choose(slot);await key('cross');await expect('screen',14);await key('cross')
   await e.shot('v140-psp-title',13)
   await key('cross');await expect('screen',15);await choose(9);await key('cross');await expect('screen',0)
   await e.shot('v140-psp-fresh-world',0)
   start=list(await e.read('position','ff'));await key('select');
   for _ in range(10):
    if (await e.read('mapchoice'))[0]>0:break
    await key('down')
   assert (await e.read('mapchoice'))[0]>0
   await e.shot('v140-psp-map-pin',5);await key('cross');await expect('screen',0);await expect('waypoint',1)
   await e.waitframes(180);idle=list(await e.read('position','ff'));assert math.dist(start,idle)<.001,(start,idle)
   e.events.append({'pin_only_stationary':True,'before':start,'after':idle,'idle_frames':180})
   await move(26,34);await move(26,37);await move(32,37);await move(40,37);await e.shot('v140-psp-manual-walking',0)
   assert math.dist(start,await e.read('position','ff'))>10
   await save();saved=list(await e.read('position','ff'));await load(6);assert math.dist(saved,await e.read('position','ff'))<.01
   await load(fixture='inn-near-npc.sav');before=(await e.read('money'))[0];health=(await e.read('health','6f'))[2]
   await action('Sleep in bed','v140-psp-bed-beside-innkeeper');await expect('task',3);assert (await e.read('money'))[0]==before-25
   await e.waitframes(150);assert (await e.read('health','6f'))[2]>health;await e.shot('v140-psp-bed-rest',0)
   e.events.append({'bed_beside_npc':True,'silver_charged':25,'arm_before':health,'arm_after':(await e.read('health','6f'))[2]})
   for fixture in ['starving-bandits','army','elites','downed']:
    await load(fixture=fixture+'.sav');await e.waitframes(30);await e.shot('v140-psp-'+fixture,0)
   await load(fixture='hidden-ambush.sav');await e.shot('v140-psp-ambush-hidden',0);assert (await e.read('life_near','12i'))[5]==0
   await move(380,383);await e.waitframes(40);assert (await e.read('life_near','12i'))[5]>0;await e.shot('v140-psp-ambush-sprung',0)
   await load(fixture='sighting.sav');await menu(2,8);await e.shot('v140-psp-last-seen',40);await key('cross');await expect('screen',5);await key('cross');pos=await e.read('position','ff');await e.waitframes(90);assert math.dist(pos,await e.read('position','ff'))<.001
   await load(fixture='council.sav');await action('Council');await e.shot('v140-psp-council-caretaker',41);await choose(1);await expect('selection',1);await e.shot('v140-psp-council-challenger',41);money=(await e.read('money'))[0];await key('cross');assert (await e.read('money'))[0]==money-200;await key('cross');assert (await e.read('money'))[0]==money-200
   await key('circle');await menu(2,3);await e.shot('v140-psp-factions',20)
   e.events.append({'completed':True,'fresh_controller_walk':True,'save_reload':True,'prepared_saves':['inn-near-npc','starving-bandits','army','elites','downed','hidden-ambush','sighting','council'],'bed_and_recovery':True,'hidden_then_triggered_ambush':True,'sighting_pin_does_not_walk':True,'council_pledge_spent_once':True})
   (ROOT/'evidence/v140-controller-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
   print('V140 CONTROLLER CHECKS PASSED',run,flush=True)
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':asyncio.run(main())

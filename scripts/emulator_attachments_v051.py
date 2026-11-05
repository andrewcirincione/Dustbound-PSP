"""Real controller shopping and prepared catalog checks; completed PSP frame captures."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys
import websockets
from emulator_v051 import Play,ROOT

def gear(f): return 36+f*24
async def choose(e,target,count):
 for _ in range(count*2):
  current=(await e.read('selection'))[0]
  if current==target:return
  await e.key('down' if (target-current)%count <= count//2 else 'up')
 raise AssertionError(('selection',target,await e.read('selection')))
async def facings(e,name):
 for key,face in [('right',1),('left',0)]:
  await e.key(key);await e.waitframes(4);await e.expect('facing',face)
  await e.shot(f'{name}-{key}',0)
async def main():
 run=pathlib.Path(tempfile.mkdtemp(prefix='v051-attachments-',dir=ROOT/'build'))
 mode=sys.argv[2]
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP')
 fixture='clothing-fixtures' if mode.startswith('clothing') else 'attachment-fixtures'
 for slot in range(2,5 if mode.startswith('clothing') else 6):shutil.copy2(ROOT/'build'/fixture/f'DUST_{slot}.sav',run/f'DUST_{slot}.sav')
 (run/'DUST_QA').touch()
 market=json.loads((ROOT/'build/attachment-fixtures/market.json').read_text())
 log=open(run/'ppsspp.log','w')
 proc=subprocess.Popen([str(pathlib.Path(sys.argv[1]).resolve()),'--graphics=software','--debugger=9251','--timeout=3600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  for _ in range(100):
   try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  async with ws:
   e=Play(ws);e.run=run;await asyncio.sleep(.4);await e.call('cpu.resume')
   for _ in range(600):
    try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   if mode=='shopping':
    await e.key('cross');await e.key('up');await e.key('cross');await e.expect('money',650)
    assert (await e.read('gear','6I'))[5]==gear(76)
    await e.move_to(28,25.5);await e.key('cross');await e.key('cross');await e.key('ltrigger');await e.expect('shop_filter',4)
    await e.shot('v051-70-backpack-market',6)
    await e.key('select');await e.expect('selection',19);await e.shot('v051-71-backpack-guide',12);await e.key('circle')
    money=650
    for family in [78,79,82]:
     item=gear(family);row=next(i for i,v in enumerate(market['stock']) if v['item']==item)
     await choose(e,row,len(market['stock']));await e.shot(f'v051-72-purchase-{family}',6)
     await e.key('cross');money-=market['stock'][row]['price'];await e.expect('money',money)
    await e.key('circle')
    for family in [78,79,82,76]:
     await e.key('triangle');pack=await e.read('pack','48I')
     row=next(i//2 for i in range(0,48,2) if pack[i]==gear(family) and pack[i+1])
     await choose(e,row,24);await e.shot(f'v051-73-pack-compare-{family}',1);await e.key('cross')
     assert (await e.read('gear','6I'))[5]==gear(family)
     await e.key('circle');await facings(e,f'v051-74-bought-pack-{family}')
    # Accept real work from the marshal and locate the destination with Square.
    await e.move_to(*market['marshal']);await e.key('cross');e.dialog_count=4
    await e.key('cross');await e.expect('screen',17);await e.key('cross');await e.key('circle')
    await e.menu(12);await e.shot('v051-75-contract-journal',19);await e.key('square');await e.shot('v051-76-contract-destination',5);await e.key('circle')
    await e.menu(4);await e.key('down');await e.key('cross',2);await e.key('circle');await e.load(1)
    await e.expect('money',money);assert (await e.read('gear','6I'))[5]==gear(76)
   else:
    slot=5 if mode=='backs' else 2+int(mode[-1])
    await e.key('down',2);await e.key('cross');await e.key('down',slot);await e.key('cross',2)
    if mode.startswith('clothing'):
     group=int(mode[-1])
     for i in range(12 if group==2 else 16):
      f=32+group*16+i;await e.key('triangle');pack=await e.read('pack','48I');item=pack[i*2]
      await choose(e,i,24);await e.shot(f'v051-79-clothing-compare-{f:02}',1)
      await e.key('cross');gear_slot=1 if f<48 else 2 if f<60 else 3 if f<68 else 4
      assert (await e.read('gear','6I'))[gear_slot]==item
      await e.key('circle');await facings(e,f'v051-80-clothing-{f:02}')
    elif mode.startswith('weapons'):
     group=int(mode[-1])
     for i in range(4 if group==2 else 16):
      f=group*16+i;await e.key('triangle');pack=await e.read('pack','48I');item=pack[i*2]
      await choose(e,i,24);await e.key('cross');assert (await e.read('gear','6I'))[0]==item
      await e.key('circle');await facings(e,f'v051-77-weapon-{f:02}')
    else:
     for i in range(8):
      await e.key('triangle');pack=await e.read('pack','48I');item=pack[i*2]
      await choose(e,i,24);await e.key('cross');assert (await e.read('gear','6I'))[5]==item
      await e.key('circle');await facings(e,f'v051-78-back-{i:02}')
   e.events.append({'scenario':mode,'completed':True,'prepared_fixture':mode!='shopping'})
   (ROOT/f'evidence/v051-attachments-{mode}-checks.json').write_text(json.dumps(e.events,indent=2)+'\n')
   print('PSP ATTACHMENT SCENARIO PASS',mode,run,flush=True)
 except Exception:
  try: print('DEBUGGER STATUS',await e.call('cpu.status'),flush=True)
  except Exception as problem: print('STATUS UNAVAILABLE',repr(problem),flush=True)
  raise
 finally:
  proc.terminate()
  try:proc.wait(timeout=5)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
if __name__=='__main__':
 if len(sys.argv)>2:asyncio.run(main())
 else:
  all_events=[]
  for mode in ['shopping','weapons0','weapons1','weapons2','backs','clothing0','clothing1','clothing2']:
   subprocess.run([sys.executable,__file__,sys.argv[1],mode],check=True)
   events=json.loads((ROOT/f'evidence/v051-attachments-{mode}-checks.json').read_text())
   assert events[-1]['completed'];all_events.extend(events)
  all_events.append({'fresh_start_bought_pack_families':[78,79,82],'starter_travel_pack_re_equipped':True,'fresh_shopping_saved_reloaded':True,'weapons_both_facings':36,'back_families_both_facings':8,'clothing_families_equipped_both_facings':44,'contract_map_controller_test':True,'prepared_catalog_slots':[2,3,4,5]})
  (ROOT/'evidence/v051-attachments-checks.json').write_text(json.dumps(all_events,indent=2)+'\n')
  print('ALL EIGHT ATTACHMENT SCENARIOS PASS')

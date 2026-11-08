"""Controller-load 39 ordinary saves in the final EBOOT; capture completed PSP frames.
No emulator memory writes. DUST_QA/DUST_RECORD are optional read-only capture markers.
"""
import asyncio,csv,hashlib,json,os,pathlib,shutil,subprocess,sys,tempfile
import websockets
from PIL import Image,ImageDraw
from emulator_v060 import Play,ROOT
OUT=ROOT/'evidence/v160';OUT.mkdir(exist_ok=True)
EXE=pathlib.Path(sys.argv[1]).resolve()
async def run_case(case):
 run=pathlib.Path(tempfile.mkdtemp(prefix=f'v160-{case:02d}-',dir=ROOT/'build'))
 shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP');shutil.copy2(ROOT/f'build/v160/family-{case:02d}.sav',run/'DUST_1.sav')
 (run/'DUST_QA').touch();(run/'DUST_RECORD').touch();log=open(run/'ppsspp.log','w')
 port=9251+case
 proc=subprocess.Popen([str(EXE),'--graphics=software',f'--debugger={port}','--timeout=600','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
 try:
  ws=None
  for _ in range(200):
   try:ws=await websockets.connect(f'ws://127.0.0.1:{port}/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,ping_interval=None,max_size=4*1024*1024);break
   except OSError:await asyncio.sleep(.1)
  assert ws is not None,('debugger missing',case,run)
  async with ws:
   e=Play(ws);e.run=run;e.events[0]['version']='0.16.0';await asyncio.sleep(.3);await e.call('cpu.resume')
   for _ in range(600):
    try:json.loads((run/'DUST_STATE.json').read_text());break
    except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
   async def key(k):
    await e.call('input.buttons.send',buttons={k:True});await e.waitframes(4)
    await e.call('input.buttons.send',buttons={k:False});await e.waitframes(5)
   async def expect(field,value):assert (await e.read(field))[0]==value,(case,field,await e.read(field),value,run)
   async def choose(n):
    for _ in range(60):
     v=(await e.read('selection'))[0]
     if v==n:return
     await e.call('input.buttons.press',button='down' if v<n else 'up',duration=2);await e.waitframes(6)
    raise AssertionError(('selection',case,n,await e.read('selection')))
   await expect('screen',13)
   if case==0:await e.shot('v160-psp-title',13)
   await choose(2);await key('cross');await expect('screen',22);await choose(1);await key('cross');await expect('screen',14);await key('cross');await expect('screen',0)
   for _ in range(2400):
    p=run/'QA_FRAME_239.rgba'
    if p.exists() and p.stat().st_size==512*272*4:break
    await asyncio.sleep(.1)
   else:raise TimeoutError(('240 recorded world frames incomplete',case,run))
   records=list(csv.DictReader((run/'QA_FRAMES.csv').open()));assert len(records)==240,(case,len(records))
   # Read header names from the production recorder, preserve phases in the report.
   phases=[int(list(row.values())[2]) for row in records]
   assert set([0,1,2,3]).issubset(phases),(case,sorted(set(phases)),run)
   frames=[Image.frombytes('RGBA',(512,272),(run/f'QA_FRAME_{i:03d}.rgba').read_bytes()).crop((0,0,480,272)).convert('RGB') for i in range(240)]
   representative=[phases.index(p) for p in [0,1,2,3]]
   sheet=Image.new('RGB',(960,544))
   for k,i in enumerate(representative):sheet.paste(frames[i],(k%2*480,k//2*272))
   sheet.save(OUT/f'psp-family-{case:02d}.png')
   # Centered 8-second clip sampled at 10 Hz; the report covers all 240 source frames.
   clip=[im.crop((144,52,336,224)) for im in frames[::3]]
   clip[0].save(OUT/f'psp-family-{case:02d}.gif',save_all=True,append_images=clip[1:],duration=100,loop=0)
   if case in [0,1,25,33,38]:frames[representative[2]].save(ROOT/f'evidence/v160-psp-combat-{case:02d}.png')
   # Verify passive cancellation and actual analog displacement on one ordinary save.
   extra={}
   if case==1:
    if (await e.read('autoattack'))[0]:await key('square')
    await expect('autoattack',0);await e.waitframes(20);await expect('attack',0)
    before=await e.read('position','ff');await e.call('input.analog.send',x=-.8,y=.6);await e.waitframes(45);await e.call('input.analog.send',x=0,y=0);await e.waitframes(8)
    after=await e.read('position','ff');assert sum((a-b)**2 for a,b in zip(after,before))>.1
    await e.shot('v160-psp-passive-walking',0);extra={'passive_cancel':True,'analog_displacement':[before,after]}
   result={'case':case,'frames':240,'attack_phase_counts':{str(p):phases.count(p) for p in set(phases)},'completed':True,'capture':'Final EBOOT completed framebuffer; normal controller-loaded save','run':str(run),'extra':extra}
   (OUT/f'case-{case:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
   # Raw captures are reproducible and large; keep compact evidence and PSP log.
   for f in run.glob('QA_FRAME_*.rgba'):f.unlink()
   print('PASS',case,'phases',result['attack_phase_counts'],flush=True)
   return result
 finally:
  proc.terminate()
  try:proc.wait(timeout=3)
  except subprocess.TimeoutExpired:proc.kill()
  log.close()
async def main():
 sem=asyncio.Semaphore(3)
 async def limited(case):
  async with sem:return await run_case(case)
 cases=list(map(int,sys.argv[2:])) or list(range(39))
 results=await asyncio.gather(*(limited(c) for c in cases))
 report={'version':'0.16.0','eboot_sha256':hashlib.sha256((ROOT/'EBOOT.PBP').read_bytes()).hexdigest(),'emulator':'PPSSPP 1.20.4 software renderer','cases':results,'total_frames':sum(x['frames'] for x in results),'physical_psp_tested':False}
 (ROOT/'evidence/v160-controller-checks.json').write_text(json.dumps(report,indent=2)+'\n')
 print('V160 PSP CASES PASSED:',len(results),flush=True)
if __name__=='__main__':asyncio.run(main())

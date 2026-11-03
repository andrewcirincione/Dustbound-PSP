#!/usr/bin/env python3
"""Capture the current PSP executable through real PSP inputs and VRAM."""
import asyncio,json,pathlib,shutil,subprocess,tempfile,sys,time
import websockets
from emulator_test import Emulator,ROOT
async def main():
    run=pathlib.Path(tempfile.mkdtemp(prefix='milestone-',dir=ROOT/'build'))
    shutil.copy2(ROOT/'EBOOT.PBP',run/'EBOOT.PBP');(run/'DUST_QA').touch()
    log=open(run/'ppsspp.log','w');exe=pathlib.Path(sys.argv[1]).resolve()
    proc=subprocess.Popen([str(exe),'--graphics=software','--debugger=9251','--timeout=180','-r',str(run),str(run/'EBOOT.PBP')],cwd=run,stdout=log,stderr=subprocess.STDOUT)
    try:
        for _ in range(100):
            try:ws=await websockets.connect('ws://127.0.0.1:9251/debugger',subprotocols=['debugger.ppsspp.org'],proxy=None,max_size=4*1024*1024);break
            except OSError:await asyncio.sleep(.1)
        async with ws:
            e=Emulator(ws);await asyncio.sleep(.3);await e.call('cpu.resume')
            for _ in range(600):
                try:e.state=json.loads((run/'DUST_STATE.json').read_text());break
                except (FileNotFoundError,json.JSONDecodeError):await asyncio.sleep(.1)
            print('BOOT',e.state,flush=True)
            await e.shot('v02-01-title',13)
            await e.press('cross');await asyncio.sleep(.2)
            await e.shot('v02-02-character-creator',15)
            for _ in range(9):await e.press('down',2)
            await e.press('cross');await asyncio.sleep(.4)
            await e.shot('v02-03-dusk-haven',0)
            await e.press('triangle');await asyncio.sleep(.2)
            await e.shot('v02-04-layered-equipment',1)
            await e.press('triangle');await asyncio.sleep(.2)
            await e.shot('v02-05-six-equipment-slots',23)
            await e.press('circle');await e.press('select');await asyncio.sleep(.2)
            await e.shot('v02-06-frontier-atlas',5)
            await e.press('select');await asyncio.sleep(.2)
            await e.shot('v02-07-contextual-guide',12)
            (ROOT/'evidence'/'v02-milestone-checks.json').write_text(json.dumps(e.events,indent=2))
            print('MILESTONE PASS',run,flush=True)
    finally:
        proc.terminate()
        try:proc.wait(timeout=5)
        except subprocess.TimeoutExpired:proc.kill()
        log.close()
if __name__=='__main__':asyncio.run(main())

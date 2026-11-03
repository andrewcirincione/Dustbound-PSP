#!/usr/bin/env python3
"""Drive the packaged PSP executable through PPSSPP's official debugger API.

Uses real emulated PSP button presses and screenshots. No mock rendering.
Requires Python websockets and a PPSSPPHeadless executable (argument 1).
"""
import asyncio, base64, json, math, pathlib, shutil, struct, subprocess, sys, tempfile, time
import websockets
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parents[1]
RUN = ROOT / 'build' / 'emu'
EVIDENCE = ROOT / 'evidence'

class Emulator:
    def __init__(self, ws):
        self.ws = ws
        self.ticket = 0
        self.events = []

    async def call(self, event, **args):
        self.ticket += 1
        ticket = str(self.ticket)
        await self.ws.send(json.dumps(dict(event=event, ticket=ticket, **args)))
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            data = json.loads(await asyncio.wait_for(self.ws.recv(), 60))
            if data.get('ticket') == ticket or data.get('event') == event:
                if data.get('event') == 'error':
                    raise RuntimeError(data)
                return data
        raise TimeoutError(event)

    async def press(self, button, frames=5):
        await self.call('input.buttons.press', button=button, duration=frames)
        await asyncio.sleep(.015)

    async def read(self, field, fmt='I'):
        state = self.state
        result = await self.call('memory.read',address=state[field+'_pointer'],size=struct.calcsize('<'+fmt))
        return struct.unpack('<'+fmt,base64.b64decode(result['base64']))

    async def move_to(self, x, y):
        for attempt in range(90):
            px,py = await self.read('position','ff')
            if math.hypot(x-px,y-py)<.5:
                print('MOVE',round(px,2),round(py,2),flush=True)
                return
            sx,sy=x-px-y+py,x-px+y-py
            horizontal=abs(sx)>abs(sy)
            val=sx if horizontal else sy
            button=('right' if val>0 else 'left') if horizontal else ('down' if val>0 else 'up')
            frames=max(1,min(8,int(abs(val)*2)))
            print('STEP',attempt,round(px,2),round(py,2),button,frames,flush=True)
            await self.press(button,frames)
        raise AssertionError(('route blocked',x,y,px,py))

    async def expect(self, field, value, fmt='I'):
        actual=await self.read(field,fmt)
        expected=value if isinstance(value,tuple) else (value,)
        assert actual==expected,(field,actual,expected)
        self.events.append({'assert':field,'actual':actual,'expected':expected})
        print('ASSERT',field,actual,flush=True)

    async def shot(self, name, expected=None):
        await self.call('cpu.stepping')
        state = self.state
        screen = (await self.call('memory.read_u32',address=state['screen_pointer']))['value']
        if expected is not None:
            assert screen == expected, (name, screen, expected)
        self.events.append({'capture':name,'screen':screen,'expected':expected})
        addr = (await self.call('memory.read_u32',address=state['display_pointer']))['value']
        result = await self.call('memory.read',address=addr,size=512*272*4)
        await self.call('cpu.resume')
        path = EVIDENCE / (name + '.png')
        raw = base64.b64decode(result['base64'])
        Image.frombytes('RGBA',(512,272),raw).crop((0,0,480,272)).convert('RGB').save(path)
        print('SCREENSHOT', name, '480x272', 'screen',screen, 'VRAM',hex(addr), flush=True)
        return screen

async def main():
    global RUN
    EVIDENCE.mkdir(exist_ok=True)
    fixture_dir = RUN
    RUN = pathlib.Path(tempfile.mkdtemp(prefix='emulator-run-',dir=ROOT/'build'))
    for name in ['EBOOT.PBP','DUST_2.sav','DUST_3.sav']:
        shutil.copy2(fixture_dir/name,RUN/name)
    (RUN/'DUST_QA').touch()
    print('RUN',RUN,flush=True)
    exe = pathlib.Path(sys.argv[1]).resolve()
    logfile = open(RUN / 'ppsspp-test.log', 'w')
    proc = subprocess.Popen([str(exe), '--graphics=software', '--debugger=9251', '--timeout=600', '-r', str(RUN), str(RUN/'EBOOT.PBP')], cwd=RUN, stdout=logfile, stderr=subprocess.STDOUT)
    try:
        for attempt in range(50):
            if proc.poll() is not None:
                raise RuntimeError('PPSSPP exited: '+str(proc.returncode))
            try:
                ws = await websockets.connect('ws://127.0.0.1:9251/debugger', subprotocols=['debugger.ppsspp.org'], proxy=None, max_size=4*1024*1024)
                break
            except OSError:
                await asyncio.sleep(.2)
        else:
            raise RuntimeError('PPSSPP debugger not listening')
        async with ws:
            emu=Emulator(ws)
            print(await emu.call('version'),flush=True)
            for _ in range(40):
                status = await emu.call('cpu.status')
                if status.get('stepping'): break
                await asyncio.sleep(.25)
            print(status,flush=True)
            await emu.call('cpu.resume')
            for _ in range(100):
                try:
                    emu.state=json.loads((RUN/'DUST_STATE.json').read_text())
                    break
                except (FileNotFoundError,json.JSONDecodeError):pass
                await asyncio.sleep(.05)
            assert (RUN/'DUST_STATE.json').exists(), 'Game did not complete 30 frames'
            print(await emu.call('game.status'),flush=True)
            await emu.shot('01-title',13)
            await emu.press('cross')
            await asyncio.sleep(.08)
            await emu.shot('02-dusk-haven',0)
            await emu.press('triangle')
            await emu.shot('03-starting-equipment',1)
            await emu.press('rtrigger')
            await emu.shot('04-skills',2)
            await emu.press('rtrigger')
            await emu.shot('05-body',3)
            await emu.press('circle')
            await emu.press('select')
            await emu.shot('06-world-map',5)
            await emu.press('circle')
            # Walk, recruit, trade, loot and equip from a fresh normal start.
            await emu.move_to(24,30.5)
            await emu.press('cross')
            await emu.shot('13-recruit-dialogue',7)
            await emu.press('cross')
            await emu.expect('squad',2)
            await emu.expect('money',350)
            await emu.move_to(28,25.5)
            await emu.press('cross')
            await emu.shot('14-trading-supplies',6)
            await emu.press('cross')
            await emu.expect('money',315)
            await emu.press('circle')
            await emu.move_to(32,29)
            await emu.press('cross')
            await emu.press('triangle')
            pack=await emu.read('pack','40I')
            print('PACK AFTER LOOT',pack,flush=True)
            leather=next(i//2 for i in range(0,40,2) if pack[i]==18 and pack[i+1]>0)
            for _ in range(leather):await emu.press('down',2)
            await emu.press('cross')
            await emu.expect('gear',(11,18,21,25),'4I')
            await emu.shot('15-looted-leather-equipped',1)
            await emu.press('circle')
            await emu.shot('16-exploring-with-companion',0)
            await emu.move_to(24,34)
            await emu.press('cross')
            await asyncio.sleep(3.4)
            pack=await emu.read('pack','40I')
            assert any(pack[i]==3 and pack[i+1]>0 for i in range(0,40,2)),pack
            emu.events.append({'assert':'gathered iron ore','passed':True})
            await emu.shot('17-resource-gathering',0)
            await emu.press('right',4)
            # Save and reload the played expedition, then load dedicated fixtures.
            await emu.press('start')
            for _ in range(4):await emu.press('down')
            await emu.press('cross');await emu.press('cross')
            assert (RUN/'DUST_1.sav').exists()
            await emu.press('circle')
            await emu.press('start')
            await emu.shot('07-expedition-menu',11)
            for _ in range(5):await emu.press('down')
            await emu.press('cross');await emu.press('cross')
            await emu.expect('money',315)
            await emu.expect('squad',2)
            await emu.expect('gear',(11,18,21,25),'4I')
            await emu.press('start')
            # Load combat fixture through the same UI a player uses.
            await emu.press('right')
            for _ in range(5): await emu.press('down')
            await emu.press('cross');await emu.press('cross')
            await asyncio.sleep(.04)
            await emu.shot('08-squad-combat',0)
            await emu.expect('squad',3)
            await emu.press('triangle')
            await emu.shot('09-visible-plate-armor',1)
            await emu.expect('gear',(12,19,23,24),'4I')
            await emu.press('circle')
            await emu.press('start')
            await emu.press('right');await emu.press('right')
            for _ in range(5):await emu.press('down')
            await emu.press('cross');await emu.press('cross')
            await asyncio.sleep(.04)
            await emu.shot('10-player-outpost',0)
            await emu.press('start');await emu.press('down');await emu.press('cross')
            await emu.shot('11-build-menu',8)
            await emu.press('circle');await emu.press('start')
            for _ in range(3):await emu.press('down')
            await emu.press('cross');await emu.shot('12-research',10)
            (EVIDENCE/'emulator-checks.json').write_text(json.dumps(emu.events,indent=2)+'\n')
            print('EMULATOR INPUT / SCREENSHOT PASS',flush=True)
    finally:
        proc.terminate()
        try:proc.wait(timeout=5)
        except subprocess.TimeoutExpired:proc.kill()
        logfile.close()
        shutil.copy2(RUN/'ppsspp-test.log',EVIDENCE/'ppsspp.log')

if __name__ == '__main__':
    asyncio.run(main())

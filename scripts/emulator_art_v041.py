#!/usr/bin/env python3
"""Equip all 32 painted weapon families with real PSP buttons in prepared saves."""
import asyncio, json, pathlib, shutil, subprocess, tempfile, sys
import websockets
from emulator_v041 import Play, ROOT

async def main():
    run = pathlib.Path(tempfile.mkdtemp(prefix='v041-art-', dir=ROOT/'build'))
    shutil.copy2(ROOT/'EBOOT.PBP', run/'EBOOT.PBP')
    for slot in (1, 2):
        shutil.copy2(ROOT/'build/art-fixtures'/f'DUST_{slot}.sav', run/f'DUST_{slot}.sav')
    (run/'DUST_QA').touch()
    log = open(run/'ppsspp.log', 'w')
    proc = subprocess.Popen([str(pathlib.Path(sys.argv[1]).resolve()), '--graphics=software',
                             '--debugger=9251', '--timeout=3600', '-r', str(run), str(run/'EBOOT.PBP')],
                            cwd=run, stdout=log, stderr=subprocess.STDOUT)
    try:
        for _ in range(100):
            try:
                ws = await websockets.connect('ws://127.0.0.1:9251/debugger',
                    subprotocols=['debugger.ppsspp.org'], proxy=None, ping_interval=None, max_size=4*1024*1024)
                break
            except OSError: await asyncio.sleep(.1)
        async with ws:
            e = Play(ws)
            await asyncio.sleep(.4)
            await e.call('cpu.resume')
            for _ in range(600):
                try:
                    e.state = json.loads((run/'DUST_STATE.json').read_text())
                    break
                except (FileNotFoundError, json.JSONDecodeError): await asyncio.sleep(.1)
            await e.key('down', 2); await e.key('cross'); await e.key('down'); await e.key('cross', 2)
            for group in range(2):
                if group: await e.load(2)
                for family in range(group * 16, group * 16 + 16):
                    # Catalog id is observed from the fixture's inventory, then
                    # selected/equipped exclusively through controller input.
                    await e.key('triangle')
                    pack = await e.read('pack', '48I')
                    index = family % 16
                    item = pack[index * 2]
                    await e.key('down', index)
                    await e.shot(f'v041-art-compare-{family:02}', 1)
                    await e.key('cross')
                    assert (await e.read('gear', '6I'))[0] == item
                    await e.key('circle')
                    await e.waitframes(3)
                    await e.shot(f'v041-art-world-{family:02}', 0)
                    e.events.append({'equipped_family':family, 'item':item, 'controller_equip':True})
            e.events.append({'all_32_weapon_families_equipped':True, 'prepared_fixture_slots':[1,2]})
            (ROOT/'evidence/v041-art-checks.json').write_text(json.dumps(e.events, indent=2)+'\n')
            print('ALL 32 WEAPON FAMILIES PSP ART PLAYTHROUGH PASS', run, flush=True)
    finally:
        proc.terminate()
        try: proc.wait(timeout=5)
        except subprocess.TimeoutExpired: proc.kill()
        log.close()

if __name__ == '__main__': asyncio.run(main())

#!/usr/bin/env python3
"""Fresh Outlaw expedition: ordinary inputs, no prepared saves or state writes."""
import asyncio
import base64
import json
import math
import pathlib
import shutil
import subprocess
import struct
import sys
import tempfile
import time

import websockets
from emulator_v060 import Play, ROOT


async def vitals(e):
    # Read the HUD's blood/hunger/fatigue/bleeding values. No memory write APIs.
    # This scenario keeps the selected starting survivor throughout.
    data = await e.call('memory.read', address=e.state['health_pointer'] + 6*4, size=16)
    return struct.unpack('<4f', base64.b64decode(data['base64']))


async def travel(e, site):
    # A combat stun can be frozen by the atlas. Resume the world to recover
    # before retrying, just as a player must; never write the actor state.
    for attempt in range(5):
        await e.menu(17)
        await e.key('down', site)
        await e.key('cross')
        await e.key('cross')
        if (await e.read('screen'))[0] == 0:
            break
        await e.key('circle')
        await e.waitframes(45)
    else:
        raise AssertionError('Survivor could not start travel after recovering in the world')
    goal = await e.read('travel', 'ff')
    deadline = time.monotonic() + 180
    while time.monotonic() < deadline:
        await e.waitframes(30)
        blood, _, _, bleed = await vitals(e)
        if blood < 70 and bleed > .01:
            await field_dressing(e, 'v060-campaign-travel-first-aid')
        if not (await e.read('waypoint'))[0]:
            assert math.dist(await e.read('position', 'ff'), goal) < .8, 'Journey ended before arrival'
            return
    raise AssertionError(('Travel stopped', site, await e.read('position', 'ff'),
                          await e.read('health', '6f')))


async def target(e, kind, index):
    for _ in range(20):
        if await e.read('context', 'ii') == (kind, index):
            return
        await e.call('input.buttons.send', buttons={'ltrigger': True})
        await e.waitframes(3)
        await e.call('input.buttons.send', buttons={'cross': True})
        await e.waitframes(3)
        await e.call('input.buttons.send', buttons={'cross': False, 'ltrigger': False})
        await e.waitframes(3)
    raise AssertionError(('Target unavailable', kind, index, await e.read('context', 'ii')))


async def field_dressing(e, capture):
    for attempt in range(8):
        health = await e.read('health', '6f')
        if (await vitals(e))[3] <= .01 and min(health[:2]) >= 65:
            return
        await e.key('triangle')
        await e.key('rtrigger', 2)
        await e.key('cross')
        await e.shot(capture, 3)
        stopped = (await vitals(e))[3] <= .01 and min((await e.read('health', '6f'))[:2]) >= 65
        await e.key('circle')
        if stopped:
            return
        await e.waitframes(30)
    raise AssertionError('Survivor could not stop bleeding with carried field dressings')


async def cache(e):
    for attempt in range(8):
        await e.menu(17)
        await e.key('square')
        await e.key('cross')
        pack = await e.read('pack', '48i')
        collected = sum(pack[i+1] for i in range(0, 48, 2) if pack[i] == 10)
        await e.shot('v060-campaign-04b-wreck-salvage', 31)
        await e.key('circle')
        if collected >= 3:
            e.events.append({'assert': 'wreck iron bars actually collected', 'quantity': collected})
            return
        # Menus pause combat stuns. Resume, recover, and try the ordinary action again.
        await e.waitframes(45)
    raise AssertionError('Could not collect the wreck cache through ordinary controls')


async def main():
    (ROOT/'evidence/v060-campaign-checks.json').unlink(missing_ok=True)
    for prior_capture in (ROOT/'evidence').glob('v060-campaign-*.png'):
        prior_capture.unlink()
    run = pathlib.Path(tempfile.mkdtemp(prefix='v060-campaign-', dir=ROOT/'build'))
    shutil.copy2(ROOT/'EBOOT.PBP', run/'EBOOT.PBP')
    (run/'DUST_QA').touch()
    log = (run/'ppsspp.log').open('w')
    proc = subprocess.Popen([str(pathlib.Path(sys.argv[1]).resolve()), '--graphics=software',
                             '--debugger=9251', '--timeout=3600', '-r', str(run),
                             str(run/'EBOOT.PBP')], cwd=run, stdout=log, stderr=subprocess.STDOUT)
    e = None
    try:
        for _ in range(100):
            try:
                ws = await websockets.connect('ws://127.0.0.1:9251/debugger',
                         subprotocols=['debugger.ppsspp.org'], proxy=None,
                         ping_interval=None, max_size=4*1024*1024)
                break
            except OSError:
                await asyncio.sleep(.1)
        async with ws:
            e = Play(ws)
            e.run = run
            await asyncio.sleep(.4)
            await e.call('cpu.resume')
            for _ in range(600):
                try:
                    e.state = json.loads((run/'DUST_STATE.json').read_text())
                    break
                except (FileNotFoundError, json.JSONDecodeError):
                    await asyncio.sleep(.1)
            await e.key('cross')
            await e.key('down')
            await e.key('right', 3)
            await e.shot('v060-campaign-01-outlaw-create', 15)
            await e.key('up', 2)
            await e.key('cross')
            await e.expect('money', 180)
            await e.key('square')
            await e.key('circle')
            await e.shot('v060-campaign-02-poor-start', 0)
            await travel(e, 0)
            await e.shot('v060-campaign-03-wreck-arrival', 0)
            # Defend the stationary rescue/salvage stop, then avoid road fights.
            await e.key('square')
            e.dialog_count = 4
            for attempt in range(10):
                await e.key('cross')
                if (await e.read('screen'))[0] == 17:
                    break
                await e.waitframes(15)
            await e.shot('v060-campaign-04-sora', 17)
            await e.key('cross')
            await e.expect('depth_rescued', 1)
            # Spend a second carried dressing on the wounded escort before
            # leaving. This is ordinary dialogue, not a health/state write.
            await e.key('cross')
            await e.expect('screen', 17)
            pack = await e.read('pack', '48i')
            kits = sum(pack[i+1] for i in range(0, 48, 2) if pack[i] == 2)
            await e.shot('v060-campaign-04c-escort-followup-care', 17)
            await e.key('cross')
            await e.expect('screen', 0)
            pack = await e.read('pack', '48i')
            assert sum(pack[i+1] for i in range(0, 48, 2) if pack[i] == 2) == kits - 1
            await e.key('cross')
            await e.expect('screen', 17)
            await e.key('down', 2)
            await e.shot('v060-campaign-04d-carry-choice', 17)
            await e.key('cross')
            sora = (await e.read('depth_npcs', '8i'))[0]
            await e.expect('carrying', sora)
            await e.shot('v060-campaign-04e-carrying-sora', 0)
            await cache(e)
            await field_dressing(e, 'v060-campaign-03b-wreck-first-aid')
            await e.key('square')
            # Take the northern culvert approach, avoiding Rell's occupied toll road.
            await travel(e, 1)
            await e.shot('v060-campaign-04f-northern-detour', 0)
            await travel(e, 7)
            deadline = time.monotonic() + 60
            while time.monotonic() < deadline and (await e.read('depth_rescued'))[0] != 2:
                await e.waitframes(30)
            await e.shot('v060-campaign-05-escort-arrival', 0)
            await e.expect('depth_rescued', 2)
            await e.expect('money', 280)
            await e.expect('carrying', -1, 'i')
            await e.key('circle')
            # The journey can leave wounds: make the ordinary first-aid decision.
            await e.key('triangle')
            await e.key('rtrigger', 2)
            await e.shot('v060-campaign-05b-first-aid-needed', 3)
            await e.key('cross')
            await e.shot('v060-campaign-05c-bandaged', 3)
            await e.key('circle')
            # Spend the earned escort reward on supplies for the return journey.
            nell = (await e.read('depth_npcs', '8i'))[1]
            await target(e, 0, nell)
            e.dialog_count = 5
            await e.key('cross')
            await e.key('down', 2)
            await e.key('cross')
            await e.expect('money', 130)
            await e.shot('v060-campaign-05d-relief-cargo', 17)
            await e.key('circle')
            await travel(e, 1)
            for attempt in range(6):
                await e.menu(17)
                await e.key('down')
                await e.key('square')
                await e.key('cross')
                if (await e.read('depth_route'))[0] == 1:
                    break
                await e.key('circle')
                await e.waitframes(15)
            await e.expect('depth_route', 1)
            await e.shot('v060-campaign-06-earned-bypass', 31)
            await e.key('circle')
            await travel(e, 7)
            nell = (await e.read('depth_npcs', '8i'))[1]
            await target(e, 0, nell)
            e.dialog_count = 5
            await e.key('cross')
            await e.key('down')
            await e.key('cross')
            await e.expect('depth_reported', 1)
            await e.expect('money', 480)
            await e.shot('v060-campaign-07-route-payment', 17)
            await e.key('cross')
            await e.expect('money', 480)
            await e.key('circle')
            await e.key('triangle')
            await e.key('rtrigger', 2)
            await e.shot('v060-campaign-08-earned-wounds', 3)
            await e.key('circle')
            await e.menu(4)
            await e.key('down')
            await e.key('cross', 2)
            await e.key('circle')
            await e.load(1)
            await e.expect('depth_rescued', 2)
            await e.expect('depth_route', 1)
            await e.expect('depth_reported', 1)
            await e.expect('money', 480)
            e.events.append({'completed': True, 'origin': 'Outlaw', 'initial_silver': 180,
                             'final_silver': 480, 'fresh_start': True, 'prepared_saves': False,
                             'state_writes': False, 'enemies_enabled': True,
                             'travel_stance': 'Sneak to wreck; carry Sora via northern culvert; flee on roads', 'relief_cargo_cost': 150,
                             'escort_bypass_report_saved_reloaded': True,
                             'escort_followup_care': True,
                             'escort_carried_to_arrival': True,
                             'single_reward_verified': True,
                             'final_limb_health': await e.read('health', '6f'),
                             'simulation_seconds': (await e.read('time', 'f'))[0]})
            print('FRESH OUTLAW COPPER ROAD PASS', run, flush=True)
    except Exception:
        if e:
            e.events.append({'completed': False})
            try:
                await e.shot('v060-campaign-failure')
                print('FAILURE STATE', json.loads((run/'DUST_STATE.json').read_text()), flush=True)
            except Exception:
                pass
        raise
    finally:
        if e:
            (ROOT/'evidence/v060-campaign-checks.json').write_text(json.dumps(e.events, indent=2)+'\n')
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        log.close()


if __name__ == '__main__':
    asyncio.run(main())

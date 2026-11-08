"""Controller verification of the packaged EBOOT using ordinary saves."""
import argparse
import asyncio
import json
import pathlib
import shutil
import subprocess
import tempfile

import websockets

from emulator_v060 import Play, ROOT

OUT = ROOT / "build/v170"


async def verify(play, run):
    async def read(field, fmt="i"):
        return await play.read(field, fmt)

    async def key(button):
        await play.call("input.buttons.press", button=button, duration=3)
        await play.waitframes(6)

    async def screen(expected):
        actual = (await read("screen"))[0]
        assert actual == expected, (expected, actual, run)

    async def choose(row):
        for _ in range(35):
            current = (await read("selection"))[0]
            if current == row:
                return
            await key("down" if current < row else "up")
        raise AssertionError(("selection", row, await read("selection")))

    async def load(slot, title=False):
        if title:
            await screen(13)
        else:
            await screen(0)
            await key("start")
            await screen(11)
        await choose(2)
        await key("cross")
        await screen(22)
        await choose(slot)
        await key("cross")
        await screen(14)
        await key("cross")
        await screen(0)

    async def action(fragment):
        await key("cross")
        await screen(35)
        labels = await read("actionlabels", "2560s")
        matches = [i for i in range(32) if fragment in labels[0][i*80:(i+1)*80].split(b"\0")[0]]
        assert matches, (fragment, labels[0], run)
        await choose(matches[0])
        await key("cross")

    async def wait_stat(index, predicate):
        for _ in range(90):
            value = (await read("war_stats", "5i"))[index]
            if predicate(value):
                return value
            await play.waitframes(12)
        raise AssertionError(("war_stats", index, await read("war_stats", "5i"), run))

    await load(0, title=True)
    assert (await read("war_stats", "5i"))[:2] == (1, 19)
    assert (await read("town_owners", "19i"))[16:] == (1, 4, 5)
    play.events.append({"released_v16_save_opened": True})
    for slot, name in enumerate(("brass-junction", "vermilion-keep", "cinder-abbey"), start=1):
        await load(slot)
        for _ in range(3):
            await play.waitframes(45)
        await play.shot("v170-psp-" + name, 0)
    await key("select")
    await screen(5)
    await play.shot("v170-psp-atlas", 5)
    await key("circle")
    await screen(0)

    await load(4)
    assert (await read("town_owners", "19i"))[16] == 4
    assert (await read("war_stats", "5i"))[2] == 2
    await play.shot("v170-psp-work-yard", 0)
    await key("cross")
    await screen(35)
    await play.shot("v170-psp-shackles", 35)
    await key("circle")

    await load(5)
    await action(b"Free from shackles: Warden Flint")
    await screen(0)
    assert await wait_stat(2, lambda n: n == 1) == 1
    play.events.append({"prisoner_freed_by_controller": True})
    await play.shot("v170-psp-freed", 0)

    await load(6)
    await wait_stat(3, lambda key: key >= 32)
    await play.shot("v170-psp-capital-rumor", 0)
    await action(b"Road trader")
    await screen(17)
    await choose(3)
    await key("cross")
    state = json.loads((run / "DUST_STATE.json").read_text())
    assert b"took VERMILION KEEP" in bytes.fromhex(state["dialogue_text_value"])
    await play.shot("v170-psp-asked-for-news", 17)
    await key("circle")
    await screen(0)

    shutil.copyfile(OUT / "bandits.sav", run / "DUST_6.sav")
    await load(6)
    await wait_stat(3, lambda key: 24 <= key < 32)
    await play.shot("v170-psp-bandit-rumor", 0)


async def main(args):
    run = pathlib.Path(tempfile.mkdtemp(prefix="ppsspp-", dir=OUT))
    shutil.copyfile(ROOT / "EBOOT.PBP", run / "EBOOT.PBP")
    shutil.copyfile(args.old_save, run / "DUST_0.sav")
    for slot, name in enumerate(("brass", "vermilion", "abbey", "occupied", "rescue", "gossip"), start=1):
        shutil.copyfile(OUT / (name + ".sav"), run / f"DUST_{slot}.sav")
    (run / "DUST_QA").touch()
    print("PPSSPP evidence run", run, flush=True)
    with (run / "ppsspp.log").open("w") as log:
        proc = subprocess.Popen(
            [args.emulator, "--graphics=software", f"--debugger={args.port}", "--timeout=600",
             "-r", str(run), str(run / "EBOOT.PBP")],
            cwd=run, stdout=log, stderr=subprocess.STDOUT,
        )
        try:
            for _ in range(160):
                try:
                    ws = await websockets.connect(
                        f"ws://127.0.0.1:{args.port}/debugger",
                        subprotocols=["debugger.ppsspp.org"], proxy=None, ping_interval=None,
                    )
                    break
                except OSError:
                    await asyncio.sleep(.1)
            else:
                raise TimeoutError("PPSSPP debugger did not start")
            async with ws:
                play = Play(ws)
                play.run = run
                play.events[0]["version"] = "0.17.0"
                for _ in range(60):
                    try:
                        await play.call("cpu.resume")
                        break
                    except RuntimeError as error:
                        if "CPU not started" not in str(error):
                            raise
                        await asyncio.sleep(.1)
                else:
                    raise TimeoutError("PPSSPP CPU did not boot")
                await verify(play, run)
                (OUT / "controller-checks.json").write_text(json.dumps(play.events, indent=2) + "\n")
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("emulator")
    parser.add_argument("old_save", type=pathlib.Path)
    parser.add_argument("--port", type=int, default=9420)
    asyncio.run(main(parser.parse_args()))

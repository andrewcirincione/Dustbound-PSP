"""Audit 0.16.1 attack variants in controller-loaded PPSSPP fights."""

import argparse
import asyncio
import csv
import hashlib
import json
import pathlib
import shutil
import subprocess
import tempfile

import websockets
from PIL import Image, ImageDraw

from emulator_v060 import Play, ROOT


OUT = ROOT / "build/v161"
CAPTURE = 360
CLIP_MOVES = {
    0: ("Unarmed", ("Jab", "Rear cross", "Rising hook")),
    1: ("Katana", ("Rising cut", "Diagonal cut", "Draw cut")),
    33: ("Greatsword", ("Overhead cleave", "Low sweep", "Braced thrust")),
}


def attacks_from(rows):
    attacks = []
    current = None
    for row in rows:
        frame = int(row["frame"])
        phase = int(row["attack_phase"])
        variant = int(row["variant"])
        if phase == 1 and (current is None or 2 in current["frames"]):
            current = {"variant": variant, "frames": {}, "seconds": {}}
            attacks.append(current)
        if not phase:
            current = None
        elif current is not None:
            assert variant == current["variant"], row
            current["frames"].setdefault(phase, []).append(frame)
            current["seconds"][phase] = float(row["phase_seconds"])
    assert all(a["variant"] != b["variant"] for a, b in zip(attacks, attacks[1:])), attacks
    return [a for a in attacks if set(a["frames"]) == {1, 2, 3}]


def render_evidence(run, case, rows, first_moves):
    def frame(index):
        data = (run / f"QA_FRAME_{index:03d}.rgba").read_bytes()
        return Image.frombytes("RGBA", (512, 272), data).crop((179, 53, 314, 171)).convert("RGB")

    sheet = Image.new("RGB", (135 * 4, 118 * 3))
    for move in first_moves:
        stages = move["frames"]
        contact = min(stages[2], key=lambda i: abs(
            float(rows[i]["phase_remaining"]) / float(rows[i]["phase_seconds"]) - .55
        ))
        for column, index in enumerate([stages[1][-1], contact, stages[2][-1], stages[3][0]]):
            sheet.paste(frame(index), (column * 135, move["variant"] * 118))
    sheet.save(OUT / f"psp-moves-{case:02d}.png")
    if case not in CLIP_MOVES:
        return

    title, move_names = CLIP_MOVES[case]
    start = max(0, first_moves[0]["frames"][1][0] - 8)
    end = first_moves[-1]["frames"][3][-1] + 1
    frames = []
    for index in range(start, end):
        im = Image.new("RGB", (405, 382), "#201c18")
        im.paste(frame(index).resize((405, 354), Image.Resampling.NEAREST))
        phase = int(rows[index]["attack_phase"])
        label = move_names[int(rows[index]["variant"])] if phase else "Guard"
        ImageDraw.Draw(im).text((12, 360), f"{title}  /  {label}", fill="#f5dfbb", font_size=15)
        frames.append(im)
    delay = [30, 30, 40] * ((len(frames) + 2) // 3)
    frames[0].save(OUT / f"Dustbound-0.16.1-{title}.gif", save_all=True,
                   append_images=frames[1:], duration=delay[:len(frames)], loop=0)


async def run_case(exe, case, master):
    name = f"family-{case:02d}" + ("-master" if master else "")
    run = pathlib.Path(tempfile.mkdtemp(prefix=f"v161-{name}-", dir=OUT))
    shutil.copy2(ROOT / "EBOOT.PBP", run / "EBOOT.PBP")
    shutil.copy2(OUT / f"{name}.sav", run / "DUST_0.sav")
    (run / "DUST_QA").touch()
    (run / "DUST_RECORD").write_text(str(CAPTURE))
    port = 9351 + case + 50 * master
    with (run / "ppsspp.log").open("w") as log:
        proc = subprocess.Popen(
            [str(exe), "--graphics=software", f"--debugger={port}", "--timeout=600",
             "-r", str(run), str(run / "EBOOT.PBP")],
            cwd=run, stdout=log, stderr=subprocess.STDOUT,
        )
        try:
            for _ in range(200):
                try:
                    socket = await websockets.connect(
                        f"ws://127.0.0.1:{port}/debugger",
                        subprotocols=["debugger.ppsspp.org"], proxy=None, ping_interval=None,
                    )
                    break
                except OSError:
                    await asyncio.sleep(.1)
            else:
                raise TimeoutError(f"PPSSPP debugger unavailable in {run}")
            async with socket:
                play = Play(socket)
                play.run = run
                await asyncio.sleep(.3)
                await play.call("cpu.resume")

                async def key(button):
                    await play.call("input.buttons.press", button=button, duration=3)
                    await play.waitframes(3)

                async def choose(row):
                    for _ in range(12):
                        current = (await play.read("selection"))[0]
                        if current == row:
                            return
                        await key("down" if current < row else "up")
                    raise AssertionError(f"Selection did not reach row {row} in {run}")

                assert (await play.read("screen"))[0] == 13
                await choose(2)
                await key("cross")
                assert (await play.read("screen"))[0] == 22
                assert (await play.read("selection"))[0] == 0
                await key("cross")
                assert (await play.read("screen"))[0] == 14
                await key("cross")
                assert (await play.read("screen"))[0] == 0
                csv_path = run / "QA_FRAMES.csv"
                for _ in range(1800):
                    if csv_path.exists():
                        with csv_path.open() as stream:
                            rows = list(csv.DictReader(stream))
                        if len(rows) == CAPTURE:
                            break
                    await asyncio.sleep(.1)
                else:
                    raise TimeoutError(f"Capture incomplete in {run}")
                attacks = attacks_from(rows)
                first_moves = attacks[:3]
                assert {a["variant"] for a in first_moves} == {0, 1, 2}, (name, attacks, run)
                if not master:
                    render_evidence(run, case, rows, first_moves)
                stroke_seconds = {move["variant"]: move["seconds"][2] for move in first_moves}
                assert len(set(stroke_seconds.values())) == 3, (name, stroke_seconds)
                result = {
                    "case": case, "master": master, "frames": len(rows),
                    "sequence": [move["variant"] for move in attacks],
                    "stroke_seconds": stroke_seconds,
                    "total_seconds": {move["variant"]: sum(move["seconds"].values()) for move in first_moves},
                }
                print("PASS", name, result["sequence"], result["stroke_seconds"], flush=True)
                for path in run.glob("QA_FRAME_*.rgba"):
                    path.unlink()
                return result
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()


async def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("emulator", type=pathlib.Path)
    parser.add_argument("cases", type=int, nargs="*")
    args = parser.parse_args()
    cases = args.cases or list(range(39))
    sem = asyncio.Semaphore(4)

    async def limited(case, master):
        async with sem:
            return await run_case(args.emulator.resolve(), case, master)

    results = await asyncio.gather(
        *(limited(case, master) for case in cases for master in range(2 if case in CLIP_MOVES else 1)),
        return_exceptions=True,
    )
    errors = [result for result in results if isinstance(result, Exception)]
    if errors:
        raise ExceptionGroup("PPSSPP animation cases failed", errors)
    novice = {r["case"]: r for r in results if not r["master"]}
    for master in (r for r in results if r["master"]):
        previous = novice[master["case"]]["stroke_seconds"]
        assert all(seconds < previous[variant] * .90 for variant, seconds in master["stroke_seconds"].items())
        previous_total = novice[master["case"]]["total_seconds"]
        assert all(seconds < previous_total[variant] for variant, seconds in master["total_seconds"].items())
    report = {
        "eboot_sha256": hashlib.sha256((ROOT / "EBOOT.PBP").read_bytes()).hexdigest(),
        "cases": results,
        "frames": sum(r["frames"] for r in results),
    }
    (OUT / "controller-checks.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {len(results)} PPSSPP fights and {report['frames']} frames", flush=True)


if __name__ == "__main__":
    asyncio.run(main())

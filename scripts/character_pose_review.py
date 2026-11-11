#!/usr/bin/env python3
"""Arrange production-rendered RGBA frames for visual review; never repaint them."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

parser = argparse.ArgumentParser()
parser.add_argument("source", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--previous",type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
font = ImageFont.truetype(font_path, 20)
small = ImageFont.truetype(font_path, 17)
bg = (64, 58, 49)
ink, muted = "#f2e2c3", "#c9baa0"
labels = {
    1: "Duster + sabre", 2: "Black brigandine + greatsword",
    3: "Lamellar + naginata", 4: "Patchwork vest + pistol",
    5: "Ronin coat + rapier", 6: "Bone harness / unarmed",
    7: "Iron cuirass + tetsubo", 8: "Monk robes + crossbow",
    9: "Clockwork / ash cloak + rifle", 10: "Gambeson + katana",
    11: "Plated duster + kusarigama", 12: "Reed jacket + scout bow",
    13: "Reedkin / scale haori + saw sword", 14: "Bandolier coat + longbow",
    15: "Scavenger wraps + anchor cleaver", 16: "Stonekin / captain armor + nagamaki",
}

def sprite(name, width=400, height=320,source=None):
    raw = ((source or args.source) / (name + "-hi.rgba")).read_bytes()
    image = Image.frombytes("RGBA", (256, 240), raw)
    return image.crop((48, 34, 208, 162)).resize((width, height), Image.Resampling.NEAREST)

for page, ids in enumerate([[1, 2, 3, 10], [4, 5, 6, 7], [8, 9, 11, 12], [13, 14, 15, 16]], 1):
    image = Image.new("RGB", (1200, 1480), bg)
    draw = ImageDraw.Draw(image)
    draw.text((24, 16), f"DUST / POSE REVIEW {page:02}", font=font, fill=ink)
    draw.text((24, 46), "Actual game compositor, enlarged. Draft poses for spot checking.", font=small, fill=muted)
    for col, label in enumerate(["PASSIVE", "FIGHT / RIGHT", "FIGHT / LEFT"]):
        draw.text((col * 400 + 24, 83), label, font=font, fill="#d2b789")
    for row, n in enumerate(ids):
        y = 123 + row * 337
        draw.text((24, y), f"{n:02} / {labels[n]}", font=font, fill=ink)
        for col, (face, mode) in enumerate([("R", "passive"), ("R", "fight"), ("L", "fight")]):
            actor = sprite(f"{n:02}-{face}-{mode}")
            image.paste(actor, (col * 400, y + 12), actor)
    image.save(args.output / f"Dust-pose-preview-{page:02}.png")

for name, ids in [("melee", [1, 2, 7]), ("mixed", [3, 6, 10])]:
    frames = []
    for index in range(144):
        variant,frame=divmod(index,48)
        image = Image.new("RGB", (960, 1090), bg)
        draw = ImageDraw.Draw(image)
        draw.text((22, 14), "DUST / SLOWED ATTACK POSE REVIEW", font=font, fill=ink)
        draw.text((22, 43), f"Variation {variant+1}/3 / right and left views / slowed for review", font=small, fill=muted)
        for cell, n in enumerate(ids):
            y=82+cell*330
            draw.text((22,y),f"{n:02} / {labels[n]}",font=small,fill=ink)
            for col,face in enumerate(["R","L"]):
                actor=sprite(f"motion-{n:02}-{face}-v{variant}-{frame:02}",400,320)
                image.paste(actor,(col*480+40,y+3),actor)
        frames.append(image)
    duration = [80] * 144
    for v in range(3):duration[v*48],duration[v*48+47]=240,500
    frames[0].save(args.output / f"Dust-attack-review-{name}.gif", save_all=True,
                   append_images=frames[1:], duration=duration, loop=0, disposal=2)
    for variant in range(3):
        strip=Image.new("RGB",(960,1090*4),bg)
        for row,frame in enumerate([0,11,23,35]):strip.paste(frames[variant*48+frame],(0,row*1090))
        strip.save(args.output/f"audit-{name}-v{variant+1}-frames.png")

# Keep the larger weapon catalog available without making the main sheets tiny.
catalog = json.loads((args.source / "catalog.json").read_text())
for page in range(7):
    entries = catalog[page * 6:page * 6 + 6]
    image = Image.new("RGB", (1280, len(entries) * 272 + 62), bg)
    draw = ImageDraw.Draw(image)
    draw.text((16, 10), "Catalog: guard / strike 1 right / strike 2 left / follow-through 3", font=small, fill=ink)
    for row, info in enumerate(entries):
        n = info["id"]
        y = row * 272 + 48
        draw.text((12, y), f'{n:02} / {info["weapon"]}', font=small, fill=ink)
        for col, (face, variation, pose) in enumerate([("R", 0, 0), ("R", 0, 2), ("L", 1, 2), ("R", 2, 3)]):
            raw = (args.source / f"catalog-{n:02}-{face}-v{variation}-p{pose}-hi.rgba").read_bytes()
            actor = Image.frombytes("RGBA", (256, 240), raw).crop((48, 24, 208, 160)).resize((320, 272), Image.Resampling.NEAREST)
            image.paste(actor, (col * 320, y + 5), actor)
    image.save(args.output / f"Dust-weapon-catalog-{page + 1:02}.png")
if args.previous:
    ids=[1,2,3,6]
    image=Image.new("RGB",(1056,1528),bg);draw=ImageDraw.Draw(image)
    draw.text((24,16),"DUST / SHOULDER AND ARM FIT",font=font,fill=ink)
    draw.text((24,47),"Actual renderer closeups. Weapon tips cropped for the anatomy review.",font=small,fill=muted)
    for col,label in enumerate(["BEFORE / PASSIVE","REVISED / PASSIVE","REVISED / FIGHT"]):
        draw.text((col*352+20,84),label,font=font,fill=muted)
    for row,n in enumerate(ids):
        y=123+row*350;draw.text((20,y),f"{n:02} / {labels[n]}",font=font,fill=ink)
        for col,(source,mode) in enumerate([(args.previous,"passive"),(args.source,"passive"),(args.source,"fight")]):
            raw=(source/f"{n:02}-R-{mode}-hi.rgba").read_bytes()
            actor=Image.frombytes("RGBA",(256,240),raw).crop((84,84,172,162)).resize((352,312),Image.Resampling.NEAREST)
            image.paste(actor,(col*352,y+28),actor)
    image.save(args.output/"Dust-pose-corrections.png")
    frames=[]
    for frame in range(48):
        image=Image.new("RGB",(1200,1080),bg);draw=ImageDraw.Draw(image)
        draw.text((24,12),"DUST / ATTACK CORRECTIONS / SLOWED",font=font,fill=ink)
        for col,label in enumerate(["PREVIOUS / RIGHT","REVISED / RIGHT","REVISED / LEFT"]):
            draw.text((col*400+20,48),label,font=small,fill=muted)
        for row,n in enumerate([1,2,7]):
            y=82+row*330;draw.text((22,y),f"{n:02} / {labels[n]}",font=small,fill=ink)
            variant=(n-1)%3
            for col,(source,face) in enumerate([(args.previous,"R"),(args.source,"R"),(args.source,"L")]):
                name=f"motion-{n:02}-{face}-v{variant}-{frame:02}"
                if col==0 and not (source/(name+"-hi.rgba")).exists():name=f"motion-{n:02}-{frame:02}"
                actor=sprite(name,source=source);image.paste(actor,(col*400,y+3),actor)
        frames.append(image)
    durations=[80]*48;durations[0]=240;durations[-1]=500
    frames[0].save(args.output/"Dust-attack-corrections.gif",save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2)
print("Wrote four outfit sheets, two slowed pose loops and seven catalog sheets.")

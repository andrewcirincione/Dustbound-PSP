#!/usr/bin/env python3
"""Fit painted building and furniture foundations to the isometric ground grid."""
from pathlib import Path
from math import ceil, floor

from PIL import Image, ImageFilter

from pack_settlement_art import array, pack

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "assets" / "art"
# Left, forward, and right foundation corners in cropped source coordinates,
# door x, door wall (0 south, 1 east), projected foundation width, roof rise.
BASE = [
    ((.035,.80),(.54,.975),(.965,.79), .35,0, 82,50),
    ((.02,.775),(.53,.99),(.955,.774), .36,0, 93,56),
    ((.035,.82),(.56,.986),(.967,.82), .35,0, 108,68),
    ((.03,.84),(.53,.988),(.94,.82), .78,1, 62,78),
    ((.035,.78),(.57,.984),(.967,.81), .365,0, 84,52),
    ((.04,.843),(.48,.99),(.94,.83), .41,0, 81,49),
    ((.035,.775),(.51,.985),(.945,.82), .32,0, 91,62),
    ((.04,.771),(.56,.99),(.968,.80), .73,1, 90,55),
]
SETTLEMENT = [
    ((.04,.84),(.43,.99),(.956,.805), .68,1, 92,53),
    ((.06,.83),(.61,.986),(.95,.865), .79,1, 105,65),
    ((.055,.831),(.40,.987),(.95,.805), .817,1, 99,64),
    ((.058,.876),(.469,.987),(.972,.874), .755,1, 124,78),
    ((.077,.911),(.41,.984),(.956,.867), .79,1, 94,66),
    ((.04,.889),(.29,.994),(.984,.829), .61,1, 120,76),
    ((.044,.839),(.57,.99),(.95,.883), .768,1, 93,65),
    ((.032,.823),(.585,.985),(.968,.866), .82,1, 102,65),
]
REGION = [
    ((.04,.788),(.522,.982),(.976,.802), .80,1, 94,60),
    ((.055,.83),(.55,.97),(.96,.87), .78,1, 94,60),
    ((.054,.864),(.55,.986),(.964,.899), .79,1, 116,76),
    ((.055,.91),(.58,.986),(.968,.889), .687,1, 92,63),
    ((.06,.826),(.53,.983),(.965,.82), .76,1, 76,47),
]
# Three painted floor contact points, then the packed foundation width and rise.
FURNITURE = [
    ((12,210),(215,312),(330,258), 75,53),  # bed
    ((12,194),(269,314),(353,273), 75,47),  # counter
    ((35,178),(166,237),(330,148), 75,47),  # table with cushions
    ((15,355),(167,430),(328,350), 76,66),  # hearth and anvil
    ((8,217),(235,299),(333,239), 64,50),   # chest
    ((12,420),(232,476),(294,441), 54,65),  # shelf
    ((19,374),(99,395),(174,354), 31,57),   # lamp
    ((2,97),(215,252),(381,119), 99,53),    # mat
]
ROOM_FURNITURE = [
    ((3,37),(48,61),(76,48), 73,42),       # cot
    ((4,71),(51,87),(70,78), 63,53),       # altar
    ((3,78),(30,88),(43,76), 40,42),       # stove
    ((3,73),(41,88),(65,73), 61,61),       # armor stand
    ((2,54),(46,68),(78,52), 66,48),       # bales
    ((2,75),(26,87),(61,75), 46,54),       # practice post
    ((1,54),(19,64),(76,39), 72,46),       # side counter
]

# Foundation landmarks are normalized to each trimmed generated inn sprite.
INN_FURNITURE = [
    ((.025,.81),(.33,.99),(.978,.64), 64,50),   # rigid-frame bed
    ((.018,.845),(.303,.983),(.947,.678), 58,62), # staircase
    ((.025,.678),(.848,.983),(.977,.895), 79,65), # bar
    ((.065,.745),(.61,.984),(.922,.815), 45,35),  # research table
]
BEDROLL = ((.067,.719),(.263,.942),(.932,.379), 40,25)


def tile(sheet, index, columns):
    w, h = sheet.width / columns, sheet.height / 2
    i, j = index % columns, index // columns
    im = sheet.crop((round(i*w), round(j*h), round((i+1)*w), round((j+1)*h)))
    im.putalpha(im.getchannel("A").point(lambda a: 255 if a > 172 else 128 if a > 105 else 0))
    return im.crop(im.getbbox())


def fit_foundation(im, left, front, right, width, rise, samples=1):
    lx, ly = left
    fx, fy = front
    rx, ry = right
    sx = min(1, width/(rx-lx))
    sy = min(1, rise/fy)
    outw = ceil(im.width*sx)
    front = fx*sx
    span = (rx-lx)*sx
    ax, ground = (lx+rx)*sx/2, span/4
    # Change the slope of each facade at the foundation, keeping verticals upright.
    left = (fy-ly)/(fx-lx)
    right = (ry-fy)/(rx-fx)
    slopes = (.5-left*sy/sx, -.5-right*sy/sx)
    offsets = [ground-fy*sy + slopes[0]*(0-front),
               ground-fy*sy,
               ground-fy*sy + slopes[1]*(outw-front)]
    ymin = floor(min(offsets))
    ymax = ceil(max(offsets) + im.height*sy)
    outh = ymax-ymin
    sprite = Image.new("RGBA", (outw*samples,outh*samples))
    corner = round(front)
    for i,(start,end) in enumerate(((0,corner),(corner,outw))):
        shift = ground-fy*sy + slopes[i]*(start-front)
        piece = im.transform(((end-start)*samples,outh*samples), Image.Transform.AFFINE,
                             (1/(sx*samples),0,start/sx,-slopes[i]/(sy*samples),1/(sy*samples),(ymin-shift)/sy),
                             resample=Image.Resampling.BICUBIC)
        sprite.paste(piece,(start*samples,0))
    if samples > 1:
        sprite = sprite.resize((outw,outh),Image.Resampling.LANCZOS)
    sprite.putalpha(sprite.getchannel("A").point(lambda a: 255 if a > 184 else 128 if a > 87 else 0))
    crop = sprite.getbbox()
    sprite = sprite.crop(crop)
    front -= crop[0]
    front_y = ground - ymin - crop[1]
    ax, ay = round(ax-crop[0]), -ymin-crop[1]
    return sprite, (ax,ay), (front,front_y), span, (fx-lx)/(rx-lx)


def fit(im, spec):
    l, f, r, door, side, width, rise = spec
    points = [(x*im.width,y*im.height) for x,y in (l,f,r)]
    sprite,anchor,front,span,share = fit_foundation(im,*points,width,rise)
    half_x, half_y = span*share/44, span*(1-share)/44
    fx = points[1][0]
    sx = span/(points[2][0]-points[0][0])
    screen_door = (door*im.width-fx)*sx
    dx = half_x if side else half_x + screen_door/22
    dy = half_y - screen_door/22 if side else half_y
    assert -half_x <= dx <= half_x and -half_y <= dy <= half_y, spec
    return sprite, anchor, front, (half_x,half_y,dx,dy,sprite.height,side)


def shadow(im, anchor, front):
    w, h = im.size
    ox, oy = front
    out = Image.new("L",(w+h,h*2))
    pix, alpha = out.load(), im.getchannel("A").load()
    for y in range(h):
        for x in range(w):
            if alpha[x,y] < 192:
                continue
            floor_y = oy-abs(x-ox)*.5
            rise = max(0,floor_y-y)
            sx, sy = round(x+.62*rise),round(floor_y+.30*rise)
            if 0<=sx<out.width and 0<=sy<out.height:
                pix[sx,sy] = 56
    out = out.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.GaussianBlur(.6))
    box = out.getbbox()
    out = out.crop(box)
    return out, (anchor[0]-box[0],anchor[1]-box[1])


def pack_shadow(im,name,anchor):
    w,h=im.size
    raw=im.tobytes()
    data,rows=[],[]
    for y in range(h):
        row=raw[y*w:(y+1)*w]
        visible=[x for x,a in enumerate(row) if a]
        lo,hi=(visible[0],visible[-1]+1) if visible else (0,0)
        rows.extend((len(data),lo,hi))
        data.extend(row[lo:hi])
    assert len(data)<65536
    code=array(name+"_pixels","uint8_t",data)+array(name+"_rows","uint16_t",rows)
    return code,f"{{{w},{h},{anchor[0]},{anchor[1]},{name}_pixels,{name}_rows}}"


def pack_furniture():
    art, sprites, models = [], [], []
    for filename, indices, specs in (
        ("furniture.png",range(8),FURNITURE),
        ("settlement-rooms.png",(0,1,2,3,4,5,7),ROOM_FURNITURE),
        ("inn-furniture.png",(1,2,3),INN_FURNITURE[1:]),
        ("bedroll.png",(0,),(BEDROLL,)),
    ):
        sheet = Image.open(ART/filename).convert("RGBA")
        for i, spec in zip(indices,specs):
            im = sheet if filename == "bedroll.png" else tile(sheet,i,2 if filename == "inn-furniture.png" else 4)
            if filename in ("inn-furniture.png", "bedroll.png"):
                spec = tuple((x*im.width,y*im.height) for x,y in spec[:3]) + spec[3:]
            if filename == "furniture.png" and i == 0:
                im = tile(Image.open(ART/"inn-furniture.png").convert("RGBA"),0,2)
                im = im.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                left, front, right, *size = INN_FURNITURE[0]
                points = [(1-x,y) for x,y in (right,front,left)]
                spec = tuple((x*im.width,y*im.height) for x,y in points) + tuple(size)
            if filename == "furniture.png" and i == 6:
                # Remove the corner of the neighboring mat from the lamp tile.
                im = im.crop((0,0,205,im.height))
            sprite, anchor, _, span, share = fit_foundation(im,*spec,samples=3)
            code, entry = pack(sprite,f"furniture_{len(sprites)}",anchor[1],anchor[0])
            art.append(code)
            sprites.append(entry)
            models.append(f"{{{span:.5f}f,{share:.5f}f}}")
    generated = "/* Generated by scripts/pack_structure_art.py. */\n"
    (ROOT/"src"/"furniture_art.h").write_text(
        generated+"".join(art)+"static const ArtSprite furnitureSprites[] = {\n"+",\n".join(sprites)+"\n};\n")
    (ROOT/"src"/"furniture_models.h").write_text(
        generated+"static const FurnitureModel furnitureModels[] = {\n"+",\n".join(models)+"\n};\n")
    return len(sprites)


def main():
    assets=[("buildings-v2",range(8),BASE),
            ("settlement-exteriors",range(8),SETTLEMENT),
            ("settlement-regions",(0,1,2,3,7),REGION)]
    header=["/* Generated by scripts/pack_structure_art.py. */\n"]
    sprites, shadows, geometry, barriers = [], [], [], []
    for name,indices,specs in assets:
        sheet=Image.open(ART/(name+".png")).convert("RGBA")
        for i,spec in zip(indices,specs):
            n=len(sprites)
            im,anchor,front,dims=fit(tile(sheet,i,4),spec)
            body,entry=pack(im,f"structure_{n}",anchor[1],anchor[0])
            header.append(body)
            sprites.append(entry)
            sh,sh_anchor=shadow(im,anchor,front)
            body,entry=pack_shadow(sh,f"structure_shadow_{n}",sh_anchor)
            header.append(body)
            shadows.append(entry)
            geometry.append("{"+",".join(f"{v:.5f}f" for v in dims[:4])+f",{dims[4]},{dims[5]}}}")
    sheet=Image.open(ART/"barrier-elevations.png").convert("RGBA")
    for i in range(4):
        im=tile(sheet,i,2)
        if i == 3:
            # The stone wall's source tile includes a stray line above the cap.
            im=im.crop((0,im.height//5,im.width,im.height))
            im=im.crop(im.getbbox())
        im.thumbnail((80,56),Image.Resampling.LANCZOS)
        body,entry=pack(im,f"barrier_{i}",im.height)
        header.append(body)
        barriers.append(entry)
    for name,typ,entries in (("structureSprites","ArtSprite",sprites),
                             ("structureShadows","ArtShadow",shadows),("barrierSprites","ArtSprite",barriers)):
        header.append(f"static const {typ} {name}[] = {{\n"+",\n".join(entries)+"\n};\n")
    (ROOT/"src"/"structure_art.h").write_text("".join(header))
    (ROOT/"src"/"structure_models.h").write_text(
        "/* Generated by scripts/pack_structure_art.py. */\n"
        "static const BuildingModel structureModels[] = {\n"+",\n".join(geometry)+"\n};\n")
    furniture = pack_furniture()
    print(f"Packed {len(sprites)} buildings, {len(barriers)} wall textures, and {furniture} furniture models.")


if __name__=="__main__":
    main()

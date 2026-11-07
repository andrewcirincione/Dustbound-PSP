#!/usr/bin/env python3
"""Export the exact in-game guide with the same controller pictograms."""
import ast,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root/'src/content.c').read_text()
def array(name):
 text=source.split(name+'[GUIDE_PAGES] = {',1)[1].split('};',1)[0]
 result=[];current=''
 for token in re.findall(r'"(?:\\.|[^"\\])*"|,',text):
  if token==',':result.append(current);current=''
  else:current+=ast.literal_eval(token)
 if current:result.append(current)
 return result
icons={1:'cross',2:'circle',3:'square',4:'triangle',5:'l',6:'r',7:'start',8:'select',9:'dpad',12:'horizontal',13:'vertical'}
ui=root/'assets/ui';ui.mkdir(exist_ok=True)
shapes={'cross':'<path d="M3 3L11 11M11 3L3 11"/>','circle':'<circle cx="7" cy="7" r="5"/>','square':'<rect x="2" y="2" width="10" height="10"/>','triangle':'<path d="M7 2L13 12H1Z"/>','dpad':'<path d="M5 1H9V5H13V9H9V13H5V9H1V5H5Z"/>','horizontal':'<path d="M1 7H13M1 7L5 3M1 7L5 11M13 7L9 3M13 7L9 11"/>','vertical':'<path d="M7 1V13M7 1L3 5M7 1L11 5M7 13L3 9M7 13L11 9"/>'}
for name in icons.values():
 w=42 if name=='select' else 38 if name=='start' else 14
 if name in shapes:body=shapes[name]
 else:body=f'<rect x="1" y="1" width="{w-2}" height="12" rx="3"/><text x="{w/2}" y="10" font-family="sans-serif" font-size="9" text-anchor="middle" fill="#b39a72" stroke="none">{name.upper()}</text>'
 (ui/(name+'.svg')).write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="14" viewBox="0 0 {w} 14"><g fill="none" stroke="#b39a72" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round">{body}</g></svg>\n')
def glyphs(s):
 for code,name in icons.items():s=s.replace(chr(code),f'![{name}](../assets/ui/{name}.svg)')
 return s.replace('\n','\n\n')
text='# Dustbound field guide — 0.8.1\n\nThe 25 chapters below match the in-game guide. ![select](../assets/ui/select.svg) inside a menu opens its contextual chapter.\n\n'
for i,(title,body) in enumerate(zip(array('guideTitles'),array('guideBodies')),1):text+=f'## {i}. {title.title()}\n\n{glyphs(body)}\n\n'
(root/'docs/FIELD_GUIDE.md').write_text(text.rstrip()+'\n')

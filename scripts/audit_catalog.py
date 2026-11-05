#!/usr/bin/env python3
"""Render real character equipment at both scales and reject duplicate pixels."""
import csv, json, pathlib, subprocess
root=pathlib.Path(__file__).resolve().parents[1]
result={}
for name,scale in [('portrait',5),('gameplay',1)]:
 path=root/'evidence'/f'catalog-{name}.csv'
 subprocess.run([str(root/'build'/'dustbound-native'),'--catalog',str(path),str(scale)],check=True,cwd=root)
 rows=list(csv.DictReader(path.open()))
 unique=len({r['render_hash'] for r in rows})
 assert len(rows)==2112 and unique==2112,(name,len(rows),unique)
 result[name]={'items':len(rows),'distinct_renders':unique}
(root/'evidence'/'catalog-audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))

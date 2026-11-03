#!/usr/bin/env python3
import hashlib, pathlib, struct, sys
p=pathlib.Path(sys.argv[1]); data=p.read_bytes()
assert data[:4]==b'\x00PBP', 'Invalid PBP magic'
version,*offsets=struct.unpack_from('<9I',data,4)
assert version==0x10000, f'Unexpected PBP version {version}'
assert offsets==sorted(offsets) and offsets[0]>=40 and offsets[-1]<=len(data)
assert data[offsets[0]:offsets[0]+4]==b'\x00PSF', 'Missing SFO'
assert data[offsets[1]:offsets[1]+8]==b'\x89PNG\r\n\x1a\n', 'Missing icon'
assert data[offsets[6]:offsets[6]+4] in (b'\x7fELF',b'~PSP'), 'Missing PSP executable'
print('PBP valid:', len(data), 'bytes')
print('SHA-256:',hashlib.sha256(data).hexdigest())

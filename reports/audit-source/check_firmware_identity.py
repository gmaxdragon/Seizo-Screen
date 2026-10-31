from pathlib import Path
import hashlib,json
w=Path('work/rev23-firmware');v=json.loads((w/'validation.json').read_text())
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
checks=[]
for name,want in v['source_sha256'].items():checks.append({'file':name,'passed':sha(w/name)==want})
for row in v['builds']:
 env={'seizo_s3_st7789':'seizo_s3_st7789','seizo_s3_hx8347d':'seizo_s3_hx8347d','seizo_s3_st7789_uart':'seizo_s3_st7789_uart'}[row['environment']]
 f=w/'.pio/build'/env/'firmware.bin'
 checks.append({'file':str(f),'passed':f.exists() and sha(f)==row['firmware_bin_sha256']})
assert all(x['passed'] for x in checks),checks
out={'checks':checks,'passed':len(checks),'failed':0,'new_compile_performed':False,'basis':'Exact retained source and compiled image identity against recorded successful builds','hardware_tested':False}
Path('work/rev23-native/firmware-identity.json').write_text(json.dumps(out,indent=2))
print(json.dumps(out))

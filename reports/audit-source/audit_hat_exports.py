from pathlib import Path
import sys
s=Path('work/rev20-audit/audit_exports.py').read_text().replace('szscreen-','test1-')
a=s.index('expectededges=');z=s.index('drillreport={}')
s=s[:a]+'''profile_native=native['edge_cuts']
def canonical(a,b):return tuple(sorted((tuple(round(v,5) for v in a),tuple(round(v,5) for v in b))))
expectededges={canonical(q['a'],q['b']) for q in profile_native if q['shape']=='line'}
actualedges={canonical(q['a'],q['b']) for q in linear}
check('All native straight Edge.Cuts segments exactly match Gerber',len(linear)==len(expectededges) and actualedges==expectededges)
expected_arcs=[q for q in profile_native if q['shape']!='line']
expected_arcs={(*canonical(q['a'],q['b']),tuple(round(v,5) for v in q['centre'])) for q in expected_arcs}
actual_arcs={(*canonical(q['a'],q['b']),tuple(round(q['a'][i]+q['ij'][i],5) for i in range(2))) for q in arcs}
check('All native curved Edge.Cuts endpoints and centres match Gerber',expected_arcs==actual_arcs)
''' + s[z:]
s=s.replace("check(name+' no routed slots',not any(x in s for x in ['G85','M15','M16']))",'''slots=[]
 for line in s.splitlines():
  t=re.fullmatch(r'T(\\d+)',line)
  if t:active=int(t[1])
  m=re.fullmatch(r'X(-?[0-9.]+)Y(-?[0-9.]+)G85X(-?[0-9.]+)Y(-?[0-9.]+)',line)
  if m:slots.append((tools[active],canonical([float(m[1]),float(m[2])],[float(m[3]),float(m[4])])) )
 expectedslots=[(q['diameter'],canonical(q['a'],q['b'])) for q in native['native_slots']] if plated else []
 check(name+' native plated slot diameters and endpoints match',sorted(slots)==sorted(expectedslots),{'actual':slots,'expected':expectedslots})''')
s=s.replace("if h['plated']!=plated:continue","if h['plated']!=plated or abs(h['drill'][0]-h['drill'][1])>1e-6:continue")
s=s.replace("'note':'Gerber job reports 160.05 x 120.05 mm because its bounding box includes the 0.05 mm profile stroke. Actual Edge.Cuts centreline rectangle is 160 x 120 mm.'","'note':'Matching HAT native profile checked separately above; displayed job size includes profile stroke.'")
exec(compile(s,'HAT-Gerber-native-audit','exec'))

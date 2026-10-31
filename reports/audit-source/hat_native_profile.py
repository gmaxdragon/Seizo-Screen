from pathlib import Path
import pcbnew as p,json
w=Path('work/rev23-hat-variant');d=json.loads((w/'NATIVE-GEOMETRY.json').read_text());b=p.LoadBoard(str(w/'test1.kicad_pcb'))
def xy(v):return [p.ToMM(v.x),-p.ToMM(v.y)]
d['edge_cuts']=[{'shape':'line' if q.GetShape()==p.SHAPE_T_SEGMENT else str(q.GetShape()),'a':xy(q.GetStart()),'b':xy(q.GetEnd()),'centre':xy(q.GetCenter())} for q in b.GetDrawings() if q.GetLayer()==p.Edge_Cuts]
slots=[]
import math
for f in b.GetFootprints():
 for q in f.Pads():
  ds=q.GetDrillSize();dx=p.ToMM(ds.x);dy=p.ToMM(ds.y)
  if abs(dx-dy)<1e-8:continue
  if not dx or not dy:continue
  angle=math.radians(q.GetOrientation().AsDegrees());x,y=xy(q.GetPosition());vx=(dx-dy)/2 if dx>dy else 0;vy=(dy-dx)/2 if dy>dx else 0
  rx=vx*math.cos(angle)+vy*math.sin(angle);ry=-vx*math.sin(angle)+vy*math.cos(angle)
  slots.append({'ref':f.GetReference(),'pin':q.GetNumber(),'diameter':min(dx,dy),'a':[x-rx,y+ry],'b':[x+rx,y-ry]})
d['native_slots']=slots
(w/'NATIVE-GEOMETRY.json').write_text(json.dumps(d,indent=2));print(d['edge_cuts'])

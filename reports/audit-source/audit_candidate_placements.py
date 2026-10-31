from pathlib import Path
import ast,json,pcbnew as p,math,sys
native='--native' in sys.argv
board_path=Path(sys.argv[sys.argv.index('--board')+1]) if '--board' in sys.argv else Path('work/rev23-native/prepared.kicad_pcb')
tree=ast.parse(Path('work/rev23-design/prepare_panel.py').read_text())
positions=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(x,ast.Name) and x.id=='positions' for x in n.targets))
for i in range(72,82):
 z=i-72;positions['R'+str(i)]=(80+(z%5)*10,55 if z<5 else 104,270)
positions.update({'U5':(150.5,160,0),'C31':(148,156.5,0),'C32':(144.2,163,0),'R67':(150.16,168,180)})
candidate_board=p.LoadBoard(str(board_path))
fps={f.GetReference():f for f in candidate_board.GetFootprints()}
if native:
 for ref,(x,y,ang) in positions.items():
  f=fps[ref];actual=[f.GetPosition().x/1e6,f.GetPosition().y/1e6,f.GetOrientationDegrees()%360]
  assert all(abs(a-z)<1e-6 for a,z in zip(actual,[x,y,ang%360])),(ref,actual,[x,y,ang])
 assert str(fps['R67'].GetFPID().GetLibItemName())=='R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal'
else:
 old=fps['R67'];candidate_board.Remove(old)
 f=p.FootprintLoad('C:/Program Files/KiCad/10.0/share/kicad/footprints/Resistor_THT.pretty','R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal');f.SetReference('R67');f.SetValue('210k / 1%');candidate_board.Add(f);fps['R67']=f
 for ref,(x,y,ang) in positions.items():
  f=fps[ref];f.SetOrientationDegrees(ang);f.SetPosition(p.VECTOR2I(p.FromMM(x),p.FromMM(y)))
polys={r:f.GetCourtyard(p.B_CrtYd if f.IsFlipped() else p.F_CrtYd) for r,f in fps.items()}
collisions=[];outside=[]
for r,poly in polys.items():
 bb=poly.BBox()
 if poly.OutlineCount() and (bb.GetX()<p.FromMM(50) or bb.GetY()<p.FromMM(50) or bb.GetRight()>p.FromMM(210) or bb.GetBottom()>p.FromMM(170)):outside.append(r)
for i,(r,a) in enumerate(fps.items()):
 for s,b in list(fps.items())[i+1:]:
  if a.IsFlipped()!=b.IsFlipped() or not polys[r].OutlineCount() or not polys[s].OutlineCount():continue
  if not polys[r].BBox().Intersects(polys[s].BBox()):continue
  q=p.SHAPE_POLY_SET(polys[r]);q.BooleanIntersection(polys[s]);area=q.Area()/1e12
  if area>1e-10:collisions.append({'a':r,'b':s,'area_mm2':area})
Path('work/rev23-native/'+('native-' if native else 'candidate-')+'courtyards.json').write_text(json.dumps({'in_memory_only':not native,'positions':positions,'collisions':collisions,'outside':outside},indent=2))
print(json.dumps({'collisions':collisions,'outside':outside}))
# Reuse the independent maximum-body audit with the candidate board injected.
s=Path('work/rev21-preorder/check_selected_max_bodies.py').read_text()
s=s.replace("b=pcbnew.LoadBoard(str(src))","b=candidate_board")
s=s.replace("out=Path(sys.argv[2]) if len(sys.argv)>2 else Path('work/rev21-preorder')","out=Path('work/rev23-native')")
s=s.replace("src=Path(sys.argv[1]) if len(sys.argv)>1 else Path('work/rev22-preorder/prepared.kicad_pcb')","src=Path("+repr(str(board_path))+")")
s=s.replace("shape=polybox(6.5,2.5) if ref in ['R52','R53','R54'] else polybox(3.6,1.6)","shape=polybox(6.8,2.6) if ref=='R67' else polybox(6.5,2.5) if ref in ['R52','R53','R54'] else polybox(3.7,2.1) if f.GetValue() in ['10k','1k','100k','100'] else polybox(3.6,1.6)")
s=s.replace("height=2.5 if ref in ['R52','R53','R54'] else 1.6","height=3.1")
s=s.replace(" elif typ=='C':","  if ref=='R67':spec='YageoMFR25 maximum6.8x2.6; seatedheightbound3.1 with<=0.5bodylift'\n  elif f.GetValue() in ['10k','1k','100k','100']:spec='YageoMFR12 maximum3.7x2.1; seatedheightbound3.1 includesassemblyallowance'\n elif typ=='C':")
s=s.replace("['C15','C17','C27','C30']","['C15','C17','C27','C30','C32']")
s=s.replace("elif ref=='U4':","elif ref in ['U8','U9','U10','U11']:\n  shape=polybox(6.6,19.69);height=5.08;spec='TI primary N14 bodyMAX6.6x19.69,seatedHMAX5.08; exact drawing visually inspected'\n elif ref=='U5':shape=polybox(3.0,3.05);height=1.45;spec='TPS2553 full-leadspan3.0,length3.05,H1.45'\n elif ref=='U6':\n  c=[145.21,145.9725];angle=0;shape=polybox(10.16,17.145);height=6.5;spec='Adafruit4683 actual PCB outline; height bound6.5 withchosenmountingclearance'\n elif ref=='U4':")
reportname=('native-selected-' if native else 'candidate-')+'max-body-review.json'
s=s.replace("selected-max-body-review.json",reportname)
exec(compile(s,'candidate-max-body-review','exec'))
r=Path('work/rev23-native')/reportname;j=json.loads(r.read_text());j['in_memory_only']=not native;j['checked_positions']=positions
if not native:j['source_sha256_before_candidate_moves']=j.pop('sha256');j['caution']='Not a report of the saved native geometry; candidate positions were applied only in memory. Rerun after parent regenerates native board.'
r.write_text(json.dumps(j,indent=2))

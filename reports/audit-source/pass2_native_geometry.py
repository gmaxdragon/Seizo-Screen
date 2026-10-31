from pathlib import Path
import json,pcbnew as p,hashlib,sys
root=Path(__file__).resolve().parents[2];src=Path(sys.argv[1]);dest=Path(sys.argv[2]);b=p.LoadBoard(str(src));M=p.ToMM
annuli=[];vias=[];tracks=[]
for f in b.GetFootprints():
 for z in f.Pads():
  drill=z.GetDrillSize();size=z.GetSize()
  if z.GetAttribute()==p.PAD_ATTRIB_PTH:
   a=min(M(size.x-drill.x),M(size.y-drill.y))/2
   annuli.append({'ref':f.GetReference(),'pin':z.GetNumber(),'size_mm':[M(size.x),M(size.y)],'drill_mm':[M(drill.x),M(drill.y)],'minimum_annular_mm':round(a,6)})
for t in b.GetTracks():
 if isinstance(t,p.PCB_VIA):vias.append({'dia_mm':M(t.GetWidth(p.F_Cu)),'drill_mm':M(t.GetDrillValue()),'annular_mm':M(t.GetWidth(p.F_Cu)-t.GetDrillValue())/2})
 else:tracks.append({'net':t.GetNetname(),'width_mm':M(t.GetWidth()),'length_mm':M(t.GetLength())})
project=json.loads(src.with_suffix('.kicad_pro').read_text())
out={'board':str(src),'sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'PTH_count':len(annuli),'PTH_below_JLC_0_18mm':[x for x in annuli if x['minimum_annular_mm']<.179999],'minimum_PTH_annular_mm':min(x['minimum_annular_mm'] for x in annuli),'via_count':len(vias),'minimum_via_annular_mm':min(x['annular_mm'] for x in vias),'minimum_track_mm':min(x['width_mm'] for x in tracks),'drc_exclusions':project['board']['design_settings']['drc_exclusions'],'rules':project['board']['design_settings']['rules'],'PTH_pads':annuli}
dest.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:out[k] for k in ['PTH_count','PTH_below_JLC_0_18mm','minimum_PTH_annular_mm','via_count','minimum_via_annular_mm','minimum_track_mm','drc_exclusions']},indent=2))

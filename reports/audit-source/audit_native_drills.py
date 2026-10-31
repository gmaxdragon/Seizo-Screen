import pcbnew,json,sys,hashlib
from pathlib import Path
p=Path(sys.argv[1]);b=pcbnew.LoadBoard(str(p));M=pcbnew.ToMM
drills=[];flashes={str(l):[] for l in [pcbnew.F_Cu,pcbnew.B_Cu]}
for fp in b.GetFootprints():
 for pad in fp.Pads():
  ds=pad.GetDrillSize()
  if ds.x or ds.y:drills.append({'kind':'pad','ref':fp.GetReference(),'pin':pad.GetNumber(),'plated':pad.GetAttribute()!=pcbnew.PAD_ATTRIB_NPTH,'xy':[M(pad.GetPosition().x),M(pad.GetPosition().y)],'drill':[M(ds.x),M(ds.y)],'drill_shape':pad.GetDrillShape()})
  if pad.GetAttribute()!=pcbnew.PAD_ATTRIB_NPTH:
   for l in [pcbnew.F_Cu,pcbnew.B_Cu]:
    if pad.IsOnLayer(l):flashes[str(l)].append([M(pad.ShapePos(l).x),M(pad.ShapePos(l).y)])
for t in b.GetTracks():
 if type(t)==pcbnew.PCB_VIA:
  drills.append({'kind':'via','plated':True,'xy':[M(t.GetPosition().x),M(t.GetPosition().y)],'drill':[M(t.GetDrillValue()),M(t.GetDrillValue())]})
  for l in [pcbnew.F_Cu,pcbnew.B_Cu]:flashes[str(l)].append([M(t.GetPosition().x),M(t.GetPosition().y)])
d={'board_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'board':str(p),'drills':drills,'copper_flash_centres':flashes,'native_layer_ids':{'front':pcbnew.F_Cu,'back':pcbnew.B_Cu}}
Path(sys.argv[2]).write_text(json.dumps(d,indent=2));print(len(drills),'native drilled objects')

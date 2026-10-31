"""Read-only checks of actual saved filled power/ground geometry and fixed bypasses."""
from pathlib import Path
import pcbnew as p,sys,json,hashlib,math
src=Path(sys.argv[1]) if len(sys.argv)>1 else Path('work/rev23-native/szscreen.kicad_pcb')
b=p.LoadBoard(str(src));b.BuildConnectivity();c=b.GetConnectivity();fps={f.GetReference():f for f in b.GetFootprints()}
def pad(r,n):return next(q for q in fps[r].Pads() if q.GetNumber()==str(n))
def uid(q):return q.m_Uuid.AsString()
def pos(q):return(q.x,q.y)
def connected(a,z):return uid(z) in {uid(q) for q in c.GetConnectedItems(a)}
checks=[]
for ra,pa,rb,pb,layer in [('C25',1,'U3',8,p.B_Cu),('C26',1,'U4',3,p.F_Cu),('C24',1,'U2',8,p.F_Cu),('C27',1,'U4',2,p.F_Cu),('U6',2,'J13',2,p.F_Cu)]:
 a=pad(ra,pa);z=pad(rb,pb);direct=[t for t in b.GetTracks() if not isinstance(t,p.PCB_VIA) and t.GetNetname()==a.GetNetname() and t.GetLayer()==layer and {pos(t.GetStart()),pos(t.GetEnd())}=={pos(a.GetPosition()),pos(z.GetPosition())}]
 checks.append({'from':ra+'.'+str(pa),'to':rb+'.'+str(pb),'net':a.GetNetname(),'layer':p.LayerName(layer),'direct_link_present':bool(direct),'length_mm':math.hypot((a.GetPosition().x-z.GetPosition().x)/1e6,(a.GetPosition().y-z.GetPosition().y)/1e6),'widths_mm':[t.GetWidth()/1e6 for t in direct],'electrically_connected_in_saved_geometry':connected(a,z)})
grounds=[];gref=pad('J7',2)
for r,n in [('U2',4),('U3',4),('U4',1),('U5',2),('C24',2),('C25',2),('C26',2),('C27',2),('C31',2),('C32',2),('C33',2),('C34',2),('C35',2),('C36',2)]:
 a=pad(r,n);grounds.append({'pad':r+'.'+str(n),'net':a.GetNetname(),'connected_to_input_ground':connected(a,gref),'has_direct_B_Cu_connection':bool(c.IsConnectedOnLayer(a,p.B_Cu))})
powers=[]
for ref,net,source in [('C14','/PANEL_3V3',('U6',2)),('C24','/PANEL_3V3',('U6',2)),('C25','/PANEL_3V3',('U6',2)),('C26','/PANEL_3V3',('U6',2)),('C33','/LCD_VIO',('J11',1)),('C34','/LCD_VIO',('J11',1)),('C35','/PANEL_3V3',('U6',2)),('C36','/PANEL_3V3',('U6',2))]:
 a=pad(ref,1);powers.append({'pad':ref+'.1','net':a.GetNetname(),'expected_net':net,'connected_to_correct_source':a.GetNetname()==net and connected(a,pad(*source))})
zones=[];fasteners=[]
for z in b.Zones():
 if z.GetIsRuleArea():continue
 for layer in z.GetLayerSet().Seq():
  polys=z.GetFilledPolysList(layer)
  zones.append({'net':z.GetNetname(),'layer':p.LayerName(layer),'filled_outline_count':polys.OutlineCount(),'filled_area_mm2':polys.Area()/1e12})
  if z.GetNetname()!='/PANEL_3V3' or layer!=p.F_Cu:continue
  for ref,f in fps.items():
   if ref not in ['H'+str(i) for i in range(1,13)]:continue
   radius=3.5 if ref in ['H1','H2','H3','H4'] else 2.5;circle=p.SHAPE_POLY_SET();circle.NewOutline();v=f.GetPosition()
   for i in range(96):circle.Append(v.x+int(p.FromMM(radius*math.cos(math.tau*i/96))),v.y+int(p.FromMM(radius*math.sin(math.tau*i/96))))
   inter=p.SHAPE_POLY_SET(polys);inter.BooleanIntersection(circle)
   fasteners.append({'hole':ref,'hardware_diameter_bound_mm':radius*2,'power_copper_under_hardware_envelope_mm2':inter.Area()/1e12,'requires_explicit_insulating_hardware_if_nonzero':inter.Area()>1})
report={'source':str(src),'sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'unconnected_count':c.GetUnconnectedCount(False),'fixed_links':checks,'ground_returns':grounds,'local_cap_supply_connections':powers,'zones':zones,'front_power_under_fastener_envelopes':fasteners,'failed_fixed_links':[z for z in checks if not z['direct_link_present'] or not z['electrically_connected_in_saved_geometry']],'failed_ground_returns':[z for z in grounds if not z['connected_to_input_ground']],'failed_supply_connections':[z for z in powers if not z['connected_to_correct_source']],'limits':'Read-only saved-fill connectivity and exact local copper-link checks. No analogue noise/transient/thermal/EMC/hardware validation. Nonzero copper under metal contact envelopes requires changed copper or explicit suitable insulating hardware.'}
Path('work/rev23-native/final-independent-plane-review.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:v for k,v in report.items() if k not in ['fixed_links','ground_returns','local_cap_supply_connections','front_power_under_fastener_envelopes']},indent=2))

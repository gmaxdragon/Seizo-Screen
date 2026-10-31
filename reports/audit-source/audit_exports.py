"""Read actual Gerber/Excellon data and cross-check native hole locations."""
import json,re,sys,hashlib,collections,math
from pathlib import Path
folder=Path(sys.argv[1]);native=json.load(open(sys.argv[2]));out=Path(sys.argv[3]);checks=[]
def check(name,okay,details=None):
 checks.append({'name':name,'passed':bool(okay),'details':details})
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
expected={'szscreen-F_Cu.gtl':'Copper,L1,Top','szscreen-B_Cu.gbl':'Copper,L2,Bot','szscreen-F_Mask.gts':'Soldermask,Top','szscreen-B_Mask.gbs':'Soldermask,Bot','szscreen-F_Silkscreen.gto':'Legend,Top','szscreen-B_Silkscreen.gbo':'Legend,Bot','szscreen-Edge_Cuts.gm1':'Profile,NP'}
files={};flashsets={};profiles=[]
for name,function in expected.items():
 p=folder/name;s=p.read_text();m=re.search(r'%TF.FileFunction,([^*]+)\*%',s);actual=m.group(1) if m else None
 # KiCad's attributes use Soldermask (lowercase m); job uses SolderMask.
 check(name+' file function',actual.lower()==function.lower(),actual)
 check(name+' absolute 4.6 millimetres','%FSLAX46Y46*%' in s and '%MOMM*%' in s)
 check(name+' finished file',s.rstrip().endswith('M02*'))
 transforms=re.findall(r'%(?:LM|MI|IR|OF|SF)[^%]+%',s)
 check(name+' no plotting mirror or offset',all(x in ['%LMN*%','%MIA0B0*%','%IR0*%','%OFA0B0*%','%SFA1B1*%'] for x in transforms),transforms)
 check(name+' common original coordinates','%TF.SameCoordinates,Original*%' in s)
 x=y=0;flashes=[];draws=[];mode='G01';start=None;apertures=[]
 for line in s.splitlines():
  am=re.fullmatch(r'%ADD(\d+)([^*]+)\*%',line)
  if am:apertures.append({'code':int(am[1]),'definition':am[2]})
  if line.strip() in ['G01*','G02*','G03*']:mode=line.strip()[:-1]
  c=re.fullmatch(r'(?:X(-?\d+))?(?:Y(-?\d+))?(?:I(-?\d+))?(?:J(-?\d+))?D0([123])\*',line)
  if c:
   old=[x,y]
   if c[1] is not None:x=int(c[1])/1e6
   if c[2] is not None:y=int(c[2])/1e6
   if c[5]=='3':flashes.append([x,y])
   if c[5]=='1':draws.append({'a':old,'b':[x,y],'mode':mode,'ij':[int(c[3] or 0)/1e6,int(c[4] or 0)/1e6]})
 flashsets[name]=flashes
 files[name]={'bytes':p.stat().st_size,'sha256':sha(p),'file_function':actual,'transforms':transforms,'aperture_count':len(apertures),'flash_count':len(flashes),'draw_count':len(draws),'region_count':s.count('G36*')}
 if name.endswith('gm1'):profiles=draws
linear=[p for p in profiles if p['mode']=='G01'];arcs=[p for p in profiles if p['mode'] in ['G02','G03']]
expectededges={tuple(sorted((a,b))) for a,b in [((50,-50),(210,-50)),((210,-50),(210,-170)),((210,-170),(50,-170)),((50,-170),(50,-50))]}
actualedges={tuple(sorted((tuple(p['a']),tuple(p['b'])))) for p in linear}
check('Profile is exactly one 160 x 120 mm rectangle',len(linear)==4 and actualedges==expectededges,{'line_count':len(linear),'centreline_bounds_mm':[50,-170,210,-50],'width_mm':160,'height_mm':120,'profile_aperture_mm':.05})
centres=[];radii=[]
for a in arcs:
 centres.append([round(a['a'][0]+a['ij'][0],6),round(a['a'][1]+a['ij'][1],6)]);radii.append(round(math.hypot(*a['ij']),6))
check('Profile includes precisely one closed 12.5 mm circular cutout',len(arcs)==2 and centres==[[184,-62],[184,-62]] and radii==[6.25,6.25] and arcs[0]['a']==arcs[1]['b'] and arcs[0]['b']==arcs[1]['a'],{'arc_centres_mm':centres,'radii_mm':radii})
drillreport={}
for plated,name in [(True,'szscreen-PTH.drl'),(False,'szscreen-NPTH.drl')]:
 p=folder/name;s=p.read_text();tools={};hits=[];active=None
 for line in s.splitlines():
  t=re.fullmatch(r'T(\d+)C([0-9.]+)',line)
  if t:tools[int(t[1])]=float(t[2]);continue
  t=re.fullmatch(r'T(\d+)',line)
  if t:active=int(t[1]);continue
  m=re.fullmatch(r'X(-?[0-9.]+)Y(-?[0-9.]+)',line)
  if m:hits.append((tools[active],float(m[1]),float(m[2])))
 check(name+' metric absolute decimal and terminator','METRIC' in s and 'G90' in s and s.rstrip().endswith('M30'))
 check(name+' no routed slots',not any(x in s for x in ['G85','M15','M16']))
 expectedhits=[];expectedraw=[]
 for h in native['drills']:
  if h['plated']!=plated:continue
  check('Native drill circular '+str(h.get('ref','via'))+'.'+str(h.get('pin','')),abs(h['drill'][0]-h['drill'][1])<1e-6)
  expectedhits.append((round(h['drill'][0],3),round(h['xy'][0],3),round(-h['xy'][1],3)));expectedraw.append((h['drill'][0],h['xy'][0],-h['xy'][1]))
 got=collections.Counter(tuple(round(v,3) for v in h) for h in hits);want=collections.Counter(expectedhits)
 remaining=list(hits);missing=[];maxerr=0
 for h in expectedraw:
  options=[(max(abs(h[1]-g[1]),abs(h[2]-g[2])),i) for i,g in enumerate(remaining) if abs(h[0]-g[0])<1e-6]
  if options and min(options)[0]<=.001001:
   e,i=min(options);maxerr=max(maxerr,e);remaining.pop(i)
  else:missing.append(h)
 unexpected=remaining
 check(name+' all native drill diameters and positions match',not missing and not unexpected,{'expected_hits':len(expectedhits),'actual_hits':len(hits),'coordinate_tolerance_mm':.001001,'maximum_coordinate_error_mm':maxerr,'missing':missing,'unexpected':unexpected,'note':'Excellon has three decimal digits in millimetres; sub-micron native coordinates are rounded during export.'})
 tally=dict(sorted(collections.Counter(str(h[0]) for h in hits).items()))
 drillreport[name]={'sha256':sha(p),'bytes':p.stat().st_size,'tools':tools,'hit_counts_by_diameter_mm':tally,'total_hits':len(hits)}
for layer,key in [('szscreen-F_Cu.gtl','front'),('szscreen-B_Cu.gbl','back')]:
 expectedcentres=collections.Counter((round(x,3),round(-y,3)) for x,y in native['copper_flash_centres'][str(native['native_layer_ids'][key])])
 actualcentres=collections.Counter((round(x,3),round(y,3)) for x,y in flashsets[layer])
 check(layer+' every native plated pad and via is flashed at correct unmirrored coordinates',not (expectedcentres-actualcentres),{'native_flashes':sum(expectedcentres.values()),'gerber_flashes':sum(actualcentres.values()),'missing':list((expectedcentres-actualcentres).elements())})
job=folder/'szscreen-job.gbrjob';j=json.load(open(job));paths=[x['Path'] for x in j['FilesAttributes']]
check('Gerber job lists exactly seven actual layer files',set(paths)==set(expected) and all((folder/x).is_file() for x in paths),paths)
check('Gerber job specifies two copper layers and 1.6 mm thickness',j['GeneralSpecs']['LayerNumber']==2 and j['GeneralSpecs']['BoardThickness']==1.6)
manufacturingfiles=list(expected)+['szscreen-PTH.drl','szscreen-NPTH.drl','szscreen-job.gbrjob']
check('Required manufacturing bundle has 10 files',len(manufacturingfiles)==10 and all((folder/x).is_file() for x in manufacturingfiles))
summary={'passed':all(c['passed'] for c in checks),'native_board':native['board'],'native_board_sha256':native['board_sha256'],'export_directory':str(folder),'checks_count':len(checks),'failed_checks':[c for c in checks if not c['passed']],'gerber_files':files,'drill_files':drillreport,'gerber_job':{'sha256':sha(job),'native_plot_bbox_with_0.05mm_profile_stroke':j['GeneralSpecs']['Size'],'note':'Gerber job reports 160.05 x 120.05 mm because its bounding box includes the 0.05 mm profile stroke. Actual Edge.Cuts centreline rectangle is 160 x 120 mm.'},'required_manufacturing_files':manufacturingfiles,'checks':checks,'limitations':['Structural and coordinate audit of exported fabrication data; does not validate physical assembly, firmware, function, or manufacturer acceptance.']}
out.write_text(json.dumps(summary,indent=2));print(json.dumps({k:summary[k] for k in ['passed','checks_count','failed_checks','native_board_sha256']}));print(json.dumps(drillreport,indent=2))

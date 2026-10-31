"""Read-only independent circuit checks against the fresh revision23 XML.
No hardware use. Pin expectations come from primary device pin maps and circuit intent.
"""
from pathlib import Path
import xml.etree.ElementTree as ET,json,hashlib,itertools,math
W=Path('work/rev23-native');src=W/'szscreen.xml';t=ET.parse(src)
comp={c.attrib['ref']:c for c in t.findall('./components/comp')}
pins={(a.attrib['ref'],a.attrib['pin']):n.attrib['name'].removeprefix('/') for n in t.findall('./nets/net') for a in n.findall('node')}
checks=[]
def ck(name,actual,expected):checks.append({'name':name,'actual':actual,'expected':expected,'passed':actual==expected})
def pn(ref,m):
 for p,net in m.items():
  actual=pins.get((ref,str(p)))
  ck(ref+'.'+str(p),bool(actual and actual.startswith('unconnected-')),True) if net=='NC' else ck(ref+'.'+str(p),actual,net)
def val(ref,v):ck(ref+' value',comp[ref].findtext('value'),v)
pn('U1',{1:'HOLD_IN',2:'STOP1_IN',3:'STOP2_IN',4:'BTN8',5:'BTN9',6:'BTN10',7:'METER_EN_IN',8:'NC',9:'PANEL_3V3',10:'GND',11:'NC',12:'SCL',13:'SDA',14:'NC',15:'GND',16:'GND',17:'GND',18:'IO_RESET',19:'NC',20:'NC',21:'BTN1',22:'BTN2',23:'BTN3',24:'BTN4',25:'BTN5',26:'BTN6',27:'BTN7',28:'NC'})
for ref,m in {
 'U2':{1:'ADC_CS',2:'ADC_CH0',3:'ADC_CH1',4:'GND',5:'ADC_MOSI',6:'ADC_MISO',7:'ADC_SCLK',8:'PANEL_3V3'},
 'U3':{1:'METER_BUFFER',2:'METER_BUFFER',3:'METER_AMP_IN',4:'GND',5:'REF_2V5',6:'REF_BUFFER',7:'REF_BUFFER',8:'PANEL_3V3'},
 'U4':{1:'GND',2:'REF_2V5',3:'PANEL_3V3'},
 'K1':{1:'METER_RELAY_A',2:'GND',3:'METER_PROBE_PLUS',4:'METER_SWITCHED'},
 'R52':{1:'METER_SWITCHED',2:'METER_RMID'},'R53':{1:'METER_RMID',2:'METER_DIV'},
 'R54':{1:'METER_DIV',2:'GND'},'R55':{1:'METER_DIV',2:'METER_AMP_IN'},
 'D3':{1:'METER_AMP_IN',2:'GND'},'D4':{1:'PANEL_3V3',2:'METER_AMP_IN'},
 'C21':{1:'METER_DIV',2:'GND'},'C22':{1:'ADC_CH0',2:'GND'},'C23':{1:'ADC_CH1',2:'GND'},
 'R56':{1:'METER_BUFFER',2:'ADC_CH0'},'R57':{1:'REF_BUFFER',2:'ADC_CH1'},
 'C24':{1:'PANEL_3V3',2:'GND'},'C25':{1:'PANEL_3V3',2:'GND'},'C26':{1:'PANEL_3V3',2:'GND'},'C27':{1:'REF_2V5',2:'GND'},
 'R58':{1:'PANEL_3V3',2:'ADC_CS'},'R59':{1:'METER_LED_SUPPLY',2:'METER_RELAY_A'},
 'R60':{1:'PANEL_3V3',2:'METER_EN_RAW'},'R61':{1:'METER_EN_RAW',2:'METER_EN_IN'},
 'C28':{1:'METER_EN_RAW',2:'GND'},'SW13':{1:'PANEL_3V3',2:'METER_LED_SUPPLY',3:'NC',4:'GND',5:'METER_EN_RAW',6:'NC'},
 'R63':{1:'ADC_MISO',2:'HAT_MISO'},'R64':{1:'HAT_MOSI',2:'ADC_MOSI'},'R65':{1:'HAT_SCLK',2:'ADC_SCLK'},'R66':{1:'HAT_ADC_CS',2:'ADC_CS'},
 'U5':{1:'PI_5V_IN',2:'GND',3:'BB_EN',4:'BB_FAULT',5:'BB_ILIM',6:'PROTO_5V'},
 'U6':{1:'GND',2:'PANEL_3V3',3:'PI_5V_IN',4:'NC'},
 'F1':{1:'PI_5V_IN',2:'LCD_5V'},'SW14':{1:'BB_ON',2:'BB_EN',3:'GND',4:'NC',5:'NC',6:'NC'},
 'C31':{1:'PI_5V_IN',2:'GND'},'C32':{1:'PI_5V_IN',2:'GND'},'C30':{1:'PROTO_5V',2:'GND'},'C29':{1:'PROTO_5V',2:'GND'},
 'R67':{1:'BB_ILIM',2:'GND'},'R68':{1:'PI_5V_IN',2:'BB_ON'},'R69':{1:'BB_EN',2:'GND'},'R70':{1:'PANEL_3V3',2:'BB_FAULT'},'R71':{1:'PROTO_5V',2:'GND'},
 'R72':{1:'LCD_VIO',2:'LCD_OE_N'},'R73':{1:'LCD_VIO',2:'GND'},
 'R27':{1:'PANEL_3V3',2:'LED1_A'},'D1':{1:'GND',2:'LED1_A'},'R28':{1:'PANEL_3V3',2:'LED2_A'},'D2':{1:'BB_FAULT',2:'LED2_A'},
 'J7':{1:'PI_5V_IN',2:'GND',3:'NC'},'J8':{1:'PI_5V_IN',2:'GND',3:'NC'},'J9':{1:'METER_PROBE_PLUS',2:'GND'},'J10':{1:'PROTO_5V',2:'GND'},'J11':{1:'LCD_VIO',2:'GND'},'J12':{1:'LCD_VIO_PRESENT',2:'BB_FAULT'},'J13':{1:'GND',2:'PANEL_3V3'},
 'U8':{1:'LCD_OE_N',2:'HAT_MOSI',3:'ISO_MOSI',4:'LCD_OE_N',5:'HAT_SCLK',6:'ISO_SCLK',7:'GND',8:'ISO_LCD_CS',9:'HAT_LCD_CS',10:'LCD_OE_N',11:'ISO_LCD_DC',12:'HAT_LCD_DC',13:'LCD_OE_N',14:'LCD_VIO'},
 'U9':{1:'LCD_OE_N',2:'HAT_LCD_RST',3:'ISO_LCD_RST',4:'LCD_OE_N',5:'HAT_LCD_BL',6:'ISO_LCD_BL',7:'GND',8:'ISO_TP_CS',9:'HAT_TP_CS',10:'LCD_OE_N',11:'NC',12:'GND',13:'LCD_VIO',14:'LCD_VIO'},
 'U10':{1:'HAT_TP_CS',2:'LCD_MISO_RAW',3:'ISO_MISO',4:'GND',5:'GND',6:'LCD_OE_N',7:'GND',8:'NC',9:'GND',10:'PANEL_3V3',11:'NC',12:'GND',13:'PANEL_3V3',14:'PANEL_3V3'},
 'U11':{1:'LCD_TP_IRQ_RAW',2:'IRQ_INVERTED',3:'IRQ_INVERTED',4:'HAT_TP_IRQ',5:'LCD_VIO',6:'LCD_VIO_ABSENT',7:'GND',8:'LCD_VIO_PRESENT',9:'LCD_VIO_ABSENT',10:'NC',11:'GND',12:'NC',13:'GND',14:'PANEL_3V3'},
}.items():pn(ref,m)
for i,sw,pull,series,cap in [(i,i,i,i+8,i) for i in range(1,8)]+[(8,10,46,49,18),(9,11,47,50,19),(10,12,48,51,20),(0,8,8,16,8)]:
 raw='KEY'+str(i) if i else 'HOLD_RAW';inp='BTN'+str(i) if i else 'HOLD_IN'
 for ref,m in {f'SW{sw}':{1:raw,2:'GND'},f'R{pull}':{1:'PANEL_3V3',2:raw},f'R{series}':{1:raw,2:inp},f'C{cap}':{1:raw,2:'GND'}}.items():pn(ref,m)
 val(f'R{pull}','10k');val(f'R{series}','1k');val(f'C{cap}','100n')
for i in [1,2]:
 raw=f'STOP{i}_RAW';inp=f'STOP{i}_IN'
 for ref,m in {f'J{i+2}':{1:raw,2:'GND'},f'R{i+16}':{1:'PANEL_3V3',2:raw},f'R{i+18}':{1:raw,2:inp},f'C{i+8}':{1:inp,2:'GND'}}.items():pn(ref,m)
 val(f'R{i+16}','10k');val(f'R{i+18}','1k');val(f'C{i+8}','100n')
for i,rail,u in [(33,'LCD_VIO',8),(34,'LCD_VIO',9),(35,'PANEL_3V3',10),(36,'PANEL_3V3',11)]:pn('C'+str(i),{1:rail,2:'GND'});val('C'+str(i),'100n')
lcd=['LCD_5V','GND','LCD_MISO_RAW','LCD_MOSI','LCD_SCLK','LCD_CS','LCD_DC','LCD_RST','LCD_BL','TP_CS','LCD_TP_IRQ_RAW'];pn('J2',dict(enumerate(lcd,1)))
for ref,v in {'U1':'MCP23017-E/SP / 0x20','U2':'MCP3202-BI/P','U3':'MCP6002-I/P','U4':'MCP1525-I/TO','U5':'TPS2553DBVR-1 / LATCH OFF','U8':'SN74LV125AN','U9':'SN74LV125AN','U10':'SN74LV125AN','U11':'SN74AHC14N','R52':'49.9k / 0.1%','R53':'49.9k / 0.1%','R54':'10.0k / 0.1%','R55':'1k','R56':'100','R57':'100','C22':'1n','C23':'1n','C24':'100n','C25':'100n','C26':'100n','C27':'4.7u / 50V','R67':'210k / 1%','R68':'10k','R69':'100k','R70':'10k','R71':'10k','R72':'1k','R73':'4.7k','R43':'22k'}.items():val(ref,v)
for ref in ['K2','D6','F2']:ck(ref+' removed',ref not in comp,True)
for bit in ['8','28']:
 q=[n.attrib for n in t.findall('./nets/net/node') if n.attrib['ref']=='U1' and n.attrib['pin']==bit]
 ck('U1.'+bit+' symbol remains output-only',q[0].get('pintype','').split('+')[0] if q else None,'output')
ru=99800;rd=10000;gain=(ru+rd)/rd;rt=ru*rd/(ru+rd)
corners=[]
for a,c,d in itertools.product([.999,1.001],repeat=3):
 g=(49900*(a+c)+10000*d)/(10000*d);corners.append({'gain':g,'indicated24_using_nominal_gain_V':24/g*gain})
calc={'meter_gain_nominal':gain,'input_resistance_ohm':ru+rd,'probe_current_at24V_A':24/(ru+rd),'each_upper_resistor_power_at24V_W':(24/(ru+rd))**2*49900,'divider_24V_V':24/gain,'divider_corner_error24V_V':max(abs(z['indicated24_using_nominal_gain_V']-24) for z in corners),'filter_tau_s':rt*100e-9,'settle50ms_residual':math.exp(-.05/(rt*100e-9)),'AQY212EH_2_5ohm_maxRon_error24V_V':24*2.5/(ru+rd),'MCP1525_1pct_error24V_V':.24,'MCP6002_4_5mV_offset_input_equiv_V':.0045*gain,'reference_capacitance20pct_uF':[4.7*.8,4.7*1.2],'TPS_ILIM_exact210k_datasheet_mA':[110,130,150],'TPS_ILIM_1pct_resistor_equation_bounds_mA':[25230/(212.1**1.016),23950/(210**.977),22980/(207.9**.94)],'TPS_max_initial_short_power_at5_1V_153mA_W':5.1*.153,'proto_bleed10k_tau_with10uF_plus100n_s':10000*10.1e-6,'LV125_OE_pullup1k_current_at3_3V_VOL0_44V_mA':(3.3-.44)/1000*1000,'dead_VIO_conservative_40uA_leak_times4k7_V':40e-6*4700}
report={'source':str(src),'xml_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'scope':'Independent preserved analog/button functions plus new power/buffer topology; no hardware or final routing approval','check_count':len(checks),'failed_checks':[x for x in checks if not x['passed']],'checks':checks,'calculations':calc,'corrected_defect':'Original rev23 assigned U1 pins8/28 to BB_FAULT/LCD_VIO_PRESENT despite MCP23017 output-only limitation. Fresh source now NC, symbols output-only, signals moved to J12 for direct HAT GPIO.','remaining_requirements':['Final power route must retain local C24/C25/C26 and C33-C36 bypass connections and continuous filled ground return.','LCD VIO must physically tap real display regulated3V3; the11-pin connector does not expose it.','Only common-ground0-24V DC meter; hardware offset/gain calibration, reverse/fault/brownout/transient behavior untested.','Prototype output100mA is a recommended working load, not a100mA maximum trip threshold; exact210k gives110-150mA before resistor tolerance.','LCD_VIO_PRESENT is a presence indication, not a precise brownout/power-good comparator.','HAT GPIO6/7 input mapping and external failsafe pulldowns are a separate matching-HAT requirement.'],'sources':['https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf','https://www.ti.com/lit/ds/symlink/tps2553-1.pdf','https://www.ti.com/lit/ds/symlink/sn74lv125a.pdf','https://www.ti.com/lit/ds/symlink/sn74ahc14.pdf','https://ww1.microchip.com/downloads/en/DeviceDoc/21034F.pdf','https://ww1.microchip.com/downloads/en/DeviceDoc/21653C.pdf','https://ww1.microchip.com/downloads/en/DeviceDoc/MCP6001-1R-1U-2-4-1-MHz-Low-Power-Op-Amp-DS20001733L.pdf','https://files.waveshare.com/upload/9/9e/2.8inch-Resistive-Touch-LCD-Schematic.pdf']}
(W/'independent-circuit-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps({k:report[k] for k in ['xml_sha256','check_count','failed_checks','calculations']},indent=2))
if report['failed_checks']:raise SystemExit(1)

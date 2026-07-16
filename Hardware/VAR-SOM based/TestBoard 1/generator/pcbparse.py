"""Parse an existing .kicad_pcb: footprints, absolute pad positions, nets, outline."""
import re, math, sys
sys.path.insert(0, r"C:/Users/hbsn/AppData/Local/Temp/claude/C--Development-OpenTools-Hardware-VAR-SOM-based/775d5b93-ac8a-4646-95fe-b9f743ceac68/scratchpad")
from klib import extract_block

def load(path):
    return open(path,encoding='utf-8').read()

def parse_footprints(t):
    fps=[]
    for m in re.finditer(r'\(footprint ',t):
        blk=extract_block(t,m.start())
        ref=re.search(r'\(property "Reference" "([^"]+)"',blk)
        at=re.search(r'\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)',blk)
        fx,fy=float(at.group(1)),float(at.group(2)); frot=float(at.group(3) or 0)
        rad=math.radians(-frot)   # KiCad board rotation is CW-positive relative to this matrix
        ca,sa=math.cos(rad),math.sin(rad)
        pads=[]
        for pm in re.finditer(r'\(pad ',blk):
            pb=extract_block(blk,pm.start())
            pn=re.match(r'\(pad "([^"]*)"',pb)
            pat=re.search(r'\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)',pb)
            net=re.search(r'\(net (?:\d+ )?"([^"]*)"',pb)
            sz=re.search(r'\(size ([-\d.]+) ([-\d.]+)\)',pb)
            typ=pb.split()[1]  # smd/thru_hole/np_thru_hole
            lays=re.findall(r'"([FB]\.Cu|\*\.Cu)"',pb)
            px,py=float(pat.group(1)),float(pat.group(2))
            # rotate pad local by footprint rotation (KiCad: pad at is pre-rotation local; board applies frot)
            ax=fx+(px*ca - py*sa); ay=fy+(px*sa + py*ca)
            pads.append(dict(num=pn.group(1), x=ax, y=ay,
                             net=(0,net.group(1)) if net else (0,''),
                             size=(float(sz.group(1)),float(sz.group(2))) if sz else (0,0),
                             type=typ, layers=lays))
        # courtyard + pad bbox (abs, corrected rotation)
        def tf(px,py): return (fx+(px*ca - py*sa), fy+(px*sa + py*ca))
        pts=[]
        for gm in re.finditer(r'\(fp_(line|rect|poly|circle|arc)\b', blk):
            gb=extract_block(blk, gm.start())
            if '"F.CrtYd"' not in gb and '"B.CrtYd"' not in gb: continue
            for xm in re.finditer(r'\((?:start|end|center|mid|xy) ([-\d.]+) ([-\d.]+)\)', gb):
                pts.append(tf(float(xm.group(1)),float(xm.group(2))))
        if not pts:
            for p in pads:
                pts.append((p['x']-p['size'][0]/2,p['y']-p['size'][1]/2)); pts.append((p['x']+p['size'][0]/2,p['y']+p['size'][1]/2))
        if pts:
            bx0=min(p[0] for p in pts); bx1=max(p[0] for p in pts)
            by0=min(p[1] for p in pts); by1=max(p[1] for p in pts)
        else:
            bx0=bx1=fx; by0=by1=fy
        fps.append(dict(ref=ref.group(1) if ref else '?', x=fx,y=fy,rot=frot, pads=pads, span=m.start(),
                        bbox=(bx0,by0,bx1,by1)))
    return fps

def net_pads(fps):
    nets={}
    for f in fps:
        for p in f['pads']:
            code,name=p['net']
            if name:
                nets.setdefault(name,[]).append((f['ref'],p['num'],p['x'],p['y'],p['type'],p['layers']))
    return nets

if __name__=="__main__":
    P=r"C:/Development/OpenTools/Hardware/VAR-SOM based/TestBoard 1/TestBoard 1.kicad_pcb"
    t=load(P); fps=parse_footprints(t); nets=net_pads(fps)
    print("footprints:",len(fps),"nets:",len(nets))
    sig=[]; pwr=[]
    PWR={'GND','+3V3','+5V','+12V','VDD_1V2','VBUS_F','VBUS_B','V12_IN','V12_F','SW5','SW3'}
    for n,pl in sorted(nets.items(), key=lambda kv:-len(kv[1])):
        (pwr if n in PWR else sig).append((n,len(pl)))
    print("--- power/plane nets (%d) ---"%len(pwr))
    for n,c in pwr: print("  %-10s %d"%(n,c))
    print("--- signal nets: %d, total pad-endpoints %d ---"%(len(sig),sum(c for _,c in sig)))
    for n,c in sig: print("  %-14s %d"%(n,c))

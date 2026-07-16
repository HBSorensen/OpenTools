"""Additive editor for the user-edited/locked TestBoard1 PCB.
Part A: relocate unlocked J9 + place 4 RPi-HAT M2.5 holes (58x49) all clear; nudge unlocked
        passives out of the hole spots.
Always starts from the clean backup; preserves locked footprints (J1,J2,J3,J4,J8)."""
import re, math, sys, os
sys.path.insert(0, os.path.dirname(__file__))
from klib import FpDef, FPDIR, extract_block, uid
import pcbparse

PCB = r"C:/Development/OpenTools/Hardware/VAR-SOM based/TestBoard 1/TestBoard 1.kicad_pcb"
PCB_IN = PCB + ".bak_useredit"
W,H = 180.0, 130.0
PASSIVE = re.compile(r'^(C|R|FB|FP|D|DL|DR)\d+$')   # small 2-pin parts: relocatable

def bbox_overlap(a,b,m=0.0):
    return not (a[2]+m<b[0] or b[2]+m<a[0] or a[3]+m<b[1] or b[3]+m<a[1])

def pt_in_bbox(x,y,b,m):
    return b[0]-m<x<b[2]+m and b[1]-m<y<b[3]+m

def fp_instance(fpname, ref, x, y, rot=0):
    lib,name=fpname.split(":")
    fpd=FpDef.get(FPDIR+"/"+lib+".pretty/"+name+".kicad_mod", name)
    raw=fpd.raw
    raw=raw.replace('(footprint "%s"'%fpd.name, '(footprint "%s"'%fpd.libid(), 1)
    raw=re.sub(r'\(version \d+\)\s*','',raw,count=1)
    raw=re.sub(r'\(generator "[^"]*"\)\s*','',raw,count=1)
    raw=re.sub(r'\(generator_version "[^"]*"\)\s*','',raw,count=1)
    m=re.search(r'\(layer "[^"]+"\)', raw)
    ins=f'\n\t(at {x:.3f} {y:.3f} {rot})\n\t(uuid "{uid("HATFP-"+ref+str(x))}")'
    raw=raw[:m.end()]+ins+raw[m.end():]
    raw=raw.replace('"REF**"', '"%s"'%ref, 1)
    return "\t"+raw.replace("\n","\n\t").rstrip()+"\n"

def hat_holes(ox,oy):
    # r270 header: pin1=(ox,oy), -x axis, body(+y, into interior). holes 58x49 per HAT spec.
    return [(ox+3.6,oy),(ox-54.4,oy),(ox+3.6,oy+49),(ox-54.4,oy+49)]

def header_box(ox,oy):
    return (ox-48.26-1.5, oy-1.5, ox+1.5, oy+2.54+1.5)

def search_j9(fps):
    nonpass=[f for f in fps if not PASSIVE.match(f['ref']) and f['ref']!='J9']
    allf=[f for f in fps if f['ref']!='J9']
    tall=[f for f in fps if f['ref'] in ('J1','J2','J3','J4','J8')]  # tall edge connectors
    best=None
    for oy10 in range(640,1080,4):        # 64 .. 108
        oy=oy10/10.0
        for ox10 in range(600,1740,4):    # 60 .. 174
            ox=ox10/10.0
            hdr=header_box(ox,oy)
            if hdr[0]<4 or hdr[2]>W-4: continue
            if any(bbox_overlap(hdr,f['bbox'],0.4) for f in nonpass): continue
            holes=hat_holes(ox,oy)
            if any(not(5<hx<W-4 and 5<hy<H-4) for hx,hy in holes): continue
            if any(pt_in_bbox(hx,hy,f['bbox'],1.9) for hx,hy in holes for f in nonpass): continue
            # HAT PCB outline (approx 65x56) must not sit over a tall connector
            hxs=[h[0] for h in holes]; hys=[h[1] for h in holes]
            hat_out=(min(hxs)-3.5, min(hys)-3.5, max(hxs)+3.5, max(hys)+3.5)
            if any(bbox_overlap(hat_out,f['bbox']) for f in tall): continue
            reloc=set()
            for f in allf:
                if not PASSIVE.match(f['ref']): continue
                if bbox_overlap(hdr,f['bbox'],0.4): reloc.add(f['ref'])
                for hx,hy in holes:
                    if pt_in_bbox(hx,hy,f['bbox'],2.6): reloc.add(f['ref'])
            # penalise moving the chained RGB LEDs / diodes (bigger, harder to re-place)
            pen=sum(6 if r.startswith('DR') else 4 if r[0]=='D' else 1 for r in reloc)
            score = -pen*2 - 0.05*(abs(ox-118)+abs(oy-80))
            if best is None or score>best[0]: best=(score,ox,oy,sorted(reloc))
    return best

def free_slot(pw, ph, obstacles):
    """Find (x,y) center on-board where a pw x ph part (courtyard) clears all obstacle bboxes."""
    hw,hh=pw/2+0.4, ph/2+0.4
    for y in [v/2 for v in range(96,254,2)]:      # 48 .. 126
        for x in [v/2 for v in range(16,352,2)]:  # 8 .. 175
            if x-hw<3 or x+hw>W-3 or y-hh<3 or y+hh>H-3: continue
            box=(x-hw,y-hh,x+hw,y+hh)
            if any(bbox_overlap(box,b) for b in obstacles): continue
            return (x,y,box)
    return None

def move_footprint(t, fp, nx, ny, nrot=None):
    blk=extract_block(t, fp['span'])
    if nrot is not None:
        rep='(at %g %g %d)'%(nx,ny,nrot)
    else:
        rep='(at %g %g)'%(nx,ny)
    blk2=re.sub(r'\(at %g %g(?: [\d.-]+)?\)'%(fp['x'],fp['y']), rep, blk, count=1)
    assert blk2!=blk, "move %s failed"%fp['ref']
    return t[:fp['span']]+blk2+t[fp['span']+len(blk):], len(blk2)-len(blk)

def zones_text():
    """GND plane on In1.Cu + B.Cu, +3V3 plane on In2.Cu (fill in pcbnew)."""
    poly="(pts "+" ".join("(xy %g %g)"%p for p in [(3,3),(W-3,3),(W-3,H-3),(3,H-3)])+")"
    def z(net,layer,i):
        return (f'\t(zone\n\t\t(net 0)\n\t\t(net_name "{net}")\n\t\t(layer "{layer}")\n'
                f'\t\t(uuid "{uid("ZONE-"+net+layer)}")\n\t\t(name "{net}_{layer}")\n'
                f'\t\t(hatch edge 0.508)\n\t\t(connect_pads (clearance 0.25))\n\t\t(min_thickness 0.25)\n'
                f'\t\t(fill yes (thermal_gap 0.3) (thermal_bridge_width 0.4))\n\t\t(polygon {poly})\n\t)\n')
    return z("GND","In1.Cu",1)+z("+3V3","In2.Cu",2)+z("GND","B.Cu",3)

def dist_pt_seg(px,py,x1,y1,x2,y2):
    dx,dy=x2-x1,y2-y1; L2=dx*dx+dy*dy
    if L2==0: return math.hypot(px-x1,py-y1)
    tt=max(0,min(1,((px-x1)*dx+(py-y1)*dy)/L2))
    return math.hypot(px-(x1+tt*dx), py-(y1+tt*dy))

def seg_clear(x1,y1,x2,y2, obst, net, clr=0.55):
    for (px,py,pr,pnet) in obst:
        if pnet==net: continue
        if dist_pt_seg(px,py,x1,y1,x2,y2) < pr+clr:
            return False
    return True

def segs_cross(ax,ay,bx,by, cx,cy,dx,dy):
    def ccw( px,py,qx,qy,rx,ry): return (qx-px)*(ry-py)-(qy-py)*(rx-px)
    d1=ccw(cx,cy,dx,dy,ax,ay); d2=ccw(cx,cy,dx,dy,bx,by)
    d3=ccw(ax,ay,bx,by,cx,cy); d4=ccw(ax,ay,bx,by,dx,dy)
    return ((d1>0)!=(d2>0)) and ((d3>0)!=(d4>0))

def route_coarse(fps):
    """Route nets that touch neither the SO-DIMM (J) nor the QFN PHY (U4), with L-shaped
    2-segment traces on F.Cu (obstacle-aware). Returns (segment_text, routed, total)."""
    nets=pcbparse.net_pads(fps)
    # obstacle list: every pad (x,y,eff_radius,net)
    obst=[(p['x'],p['y'],max(p['size'])/2 if max(p['size'])>0 else 0.5, p['net'][1])
          for f in fps for p in f['pads']]
    PLANE={'GND','+3V3','+5V'}
    segs=[]; routed=0; total=0
    for name,pl in nets.items():
        if name in PLANE: continue
        refs={n[0] for n in pl}
        if 'J' in refs or 'U4' in refs: continue      # skip fine-pitch escapes
        if len(pl)<2: continue
        pts=[(n[2],n[3]) for n in pl]
        order=[0]; rem=set(range(1,len(pts)))
        while rem:
            last=pts[order[-1]]
            nxt=min(rem,key=lambda k:math.hypot(pts[k][0]-last[0],pts[k][1]-last[1]))
            order.append(nxt); rem.discard(nxt)
        for a,b in zip(order,order[1:]):
            total+=1
            x1,y1=pts[a]; x2,y2=pts[b]
            for cx,cy in [(x2,y1),(x1,y2)]:
                cand=[(x1,y1,cx,cy),(cx,cy,x2,y2)]
                if not all(seg_clear(sx,sy,ex,ey,obst,name) for sx,sy,ex,ey in cand): continue
                # reject if crosses an already-placed track of a different net
                if any(segs_cross(sx,sy,ex,ey, s[0],s[1],s[2],s[3]) for sx,sy,ex,ey in cand
                       for s in segs if s[4]!=name): continue
                for sx,sy,ex,ey in cand:
                    if abs(sx-ex)<0.01 and abs(sy-ey)<0.01: continue
                    segs.append((sx,sy,ex,ey,name))
                routed+=1; break
            # else leave as ratsnest
    txt="".join(f'\t(segment (start {s[0]:.3f} {s[1]:.3f}) (end {s[2]:.3f} {s[3]:.3f}) '
                f'(width 0.35) (layer "F.Cu") (net "{s[4]}") (uuid "{uid("SEG%d"%i)}"))\n'
                for i,s in enumerate(segs))
    return txt, routed, total

def main():
    t=open(PCB_IN,encoding='utf-8').read()
    fps=pcbparse.parse_footprints(t)
    sr=search_j9(fps)
    assert sr, "no J9 placement found"
    _,ox,oy,reloc=sr
    print("J9 -> (%.1f,%.1f) r90 ; passives to relocate: %s"%(ox,oy,reloc))
    holes=hat_holes(ox,oy)
    for i,(hx,hy) in enumerate(holes): print("  HAT%d (%.2f,%.2f)"%(i+1,hx,hy))

    # build obstacle set: all footprints staying put + HAT outline + holes + header
    hxs=[h[0] for h in holes]; hys=[h[1] for h in holes]
    hat_out=(min(hxs)-3.5,min(hys)-3.5,max(hxs)+3.5,max(hys)+3.5)
    headerbox=header_box(ox,oy)
    obstacles=[f['bbox'] for f in fps if f['ref']!='J9' and f['ref'] not in reloc]
    obstacles+=[hat_out, headerbox]
    obstacles+=[(hx-1.8,hy-1.8,hx+1.8,hy+1.8) for hx,hy in holes]
    relocfps=sorted([f for f in fps if f['ref'] in reloc], key=lambda f:-f['span'])
    for f in relocfps:
        pw=f['bbox'][2]-f['bbox'][0]; ph=f['bbox'][3]-f['bbox'][1]
        slot=free_slot(max(pw,2.0), max(ph,2.0), obstacles)
        assert slot, "no free slot for %s"%f['ref']
        obstacles.append(slot[2])
        t,_=move_footprint(t, f, slot[0], slot[1])
        print("  moved %s -> (%.1f,%.1f)"%(f['ref'],slot[0],slot[1]))

    # move J9 to (ox,oy,270) (reparse spans after passive moves)
    fps=pcbparse.parse_footprints(t)
    j9=[f for f in fps if f['ref']=='J9'][0]
    t,_=move_footprint(t, j9, ox, oy, 270)

    # add 4 holes
    add="".join(fp_instance("MountingHole:MountingHole_2.7mm_M2.5","HAT%d"%(i+1),hx,hy) for i,(hx,hy) in enumerate(holes))

    # coarse routing (planes already exist in the board; do not duplicate them)
    fps=pcbparse.parse_footprints(t.rstrip()[:-1]+add+")\n")
    rt,routed,tot=route_coarse(fps)
    print("coarse routing (nets not on SoM/QFN): %d/%d connections routed"%(routed,tot))

    t=t.rstrip()[:-1]+add+rt+")\n"
    open(PCB,"w",encoding='utf-8').write(t)
    print("wrote board: J9 relocated + 4 HAT holes + coarse tracks")

if __name__=="__main__":
    main()

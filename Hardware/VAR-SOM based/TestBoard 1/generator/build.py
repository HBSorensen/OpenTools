"""Full generator for TestBoard1 (VAR-SOM-MX8M-PLUS carrier).
Emits multi-page schematic + 4-layer PCB into the project directory."""
import sys, os, math
sys.path.insert(0, os.path.dirname(__file__))
from klib import *

PROJDIR = r"C:/Development/OpenTools/Hardware/VAR-SOM based/TestBoard 1"
PROJ = "TestBoard 1"
RAILS = {"+12V","+5V","+3V3","GND"}   # get power-symbol treatment
OUT={0:(-1,0),180:(1,0),90:(0,-1),270:(0,1)}
LBLANG={0:180,180:0,90:270,270:90}
STUB=3.81

def fp_dir(lib):
    if lib=="VAR-SOM": return VARLIB+"/VAR-SOM.pretty"
    return FPDIR+"/"+lib+".pretty"

GRID=1.27
def snap(v): return round(v/GRID)*GRID

# ---- symbol handles ----
S = lambda lib,nm: SymDef.get(SYMDIR+"/"+lib+".kicad_sym", nm)
SOM   = SymDef.get(VARLIB+"/VAR-SOM.kicad_sym","VAR-SOM-MX8M-PLUS")

class Builder:
    def __init__(self):
        self.embeds={}          # libid -> block text
        self.sheets=[]          # ordered list of sheet dicts
        self.sheet_by=None
        self.rootuuid=uid("ROOT")
        self.pcb={}             # ref -> dict(fpdef,x,y,rot,side,nets{padnum:net},value)
        self.pwrn=0
        self.nets=set()

    def sheet(self, name, title):
        d=dict(name=name,title=title,uuid=uid("SH-"+name),body=[],page=len(self.sheets)+2,syms=set())
        self.sheets.append(d); self.sheet_by=d; return d

    def _embed(self, sd):
        for lid,txt in sd.embedded_all():
            self.embeds.setdefault(lid,txt)

    def _use(self, sh, sd):
        self._embed(sd)
        for lid,_ in sd.embedded_all():
            sh['syms'].add(lid)

    def _rootpath(self, sh):
        # symbol instance path: /<rootuuid> for root; /<rootuuid>/<sheetuuid> for child
        if sh is None: return "/"+self.rootuuid
        return "/"+self.rootuuid+"/"+sh['uuid']

    def wire(self,x1,y1,x2,y2):
        x1,y1,x2,y2=snap(x1),snap(y1),snap(x2),snap(y2)
        return f'\t(wire (pts (xy {x1:.4f} {y1:.4f}) (xy {x2:.4f} {y2:.4f})) (stroke (width 0) (type default)) (uuid "{uid()}"))\n'

    def glabel(self,name,x,y,ang):
        x,y=snap(x),snap(y)
        return (f'\t(global_label "{name}" (shape bidirectional) (at {x:.4f} {y:.4f} {ang}) '
                f'(effects (font (size 1.27 1.27)) (justify left)) (uuid "{uid()}"))\n')

    def noconn(self,x,y):
        return f'\t(no_connect (at {snap(x):.4f} {snap(y):.4f}) (uuid "{uid()}"))\n'

    def text(self, sh, s, x, y, size=2.0):
        sh['body'].append(f'\t(text "{s}" (at {snap(x):.4f} {snap(y):.4f} 0) (effects (font (size {size} {size})) (justify left)) (uuid "{uid()}"))\n')

    def _pwr_sym(self, sh, rail, x, y):
        """place power symbol for `rail` with its pin at page (x,y)."""
        sd = S("power", rail if rail in ("GND","+5V","+3V3","+12V") else "GND")
        self._embed(sd)
        gp=sd.pins_for_unit(1)[0]
        sx,sy = x-gp['x'], y+gp['y']
        self.pwrn+=1
        self._sym_block(sh, sd, "#PWR%03d"%self.pwrn, rail, "", sx, sy, unit=1, hidename=True)

    def pwr_flag(self, sh, rail, x, y):
        """PWR_FLAG + rail power symbol coincident at (x,y) to satisfy ERC power driving."""
        pf=S("power","PWR_FLAG"); self._embed(pf)
        self.pwrn+=1
        self._sym_block(sh, pf, "#FLG%03d"%self.pwrn, rail, "", x, y, unit=1, hidename=True)
        if rail in RAILS:
            self._pwr_sym(sh, rail, x, y)
        else:
            sh['body'].append(self.glabel(rail, x, y, 0))

    def _sym_block(self, sh, sd, ref, value, fp, X, Y, unit=1, hidename=False, fields=None):
        self._use(sh, sd)
        X,Y=snap(X),snap(Y)
        s=f'\t(symbol\n\t\t(lib_id "{sd.libid()}")\n\t\t(at {X:.4f} {Y:.4f} 0)\n\t\t(unit {unit})\n'
        s+='\t\t(exclude_from_sim no)(in_bom yes)(on_board yes)(dnp no)\n'
        u=uid(ref+"#"+str(unit)+"#"+str(X)+","+str(Y))
        s+=f'\t\t(uuid "{u}")\n'
        hr='(hide yes)' if hidename else ''
        s+=f'\t\t(property "Reference" "{ref}" (at {X:.4f} {Y-2.54:.4f} 0) {hr} (effects (font (size 1.27 1.27)) (justify left)))\n'
        s+=f'\t\t(property "Value" "{value}" (at {X:.4f} {Y+2.54:.4f} 0) {"(hide yes)" if hidename else ""} (effects (font (size 1.27 1.27)) (justify left)))\n'
        s+=f'\t\t(property "Footprint" "{fp}" (at {X:.4f} {Y:.4f} 0) (hide yes) (effects (font (size 1.27 1.27))))\n'
        if fields:
            for k,v in fields.items():
                s+=f'\t\t(property "{k}" "{v}" (at {X:.4f} {Y:.4f} 0) (hide yes) (effects (font (size 1.27 1.27))))\n'
        for p in sd.pins_for_unit(unit):
            s+=f'\t\t(pin "{p["num"]}" (uuid "{uid(u+"p"+p["num"])}"))\n'
        s+=f'\t\t(instances (project "{PROJ}" (path "{self._rootpath(sh)}" (reference "{ref}") (unit {unit}))))\n'
        s+='\t)\n'
        sh['body'].append(s)

    def place(self, sh, sd, ref, value, fp, X, Y, unit=1, nets=None, fields=None, pcb=True, side="F", prot=0, ppos=None):
        """Place one symbol (unit). nets: {pinnum: netname or None}. Records PCB pads."""
        nets=nets or {}
        X,Y=snap(X),snap(Y)
        self._embed(sd)
        self._sym_block(sh, sd, ref, value, fp, X, Y, unit=unit, fields=fields)
        pins={p['num']:p for p in sd.pins_for_unit(unit)}
        for num,p in pins.items():
            net=nets.get(num, "__NC__")
            ppx,ppy = X+p['x'], Y-p['y']
            ox,oy = OUT.get(p['rot'],(-1,0))
            ex,ey = ppx+STUB*ox, ppy-STUB*oy
            if net in (None,"__NC__"):
                sh['body'].append(self.noconn(ppx,ppy))
            elif net in RAILS:
                sh['body'].append(self.wire(ppx,ppy,ex,ey))
                self._pwr_sym(sh, net, ex, ey)
            else:
                self.nets.add(net)
                sh['body'].append(self.wire(ppx,ppy,ex,ey))
                sh['body'].append(self.glabel(net, ex, ey, LBLANG.get(p['rot'],0)))
        # record PCB
        if pcb and fp:
            lib,name=fp.split(":")
            fpd=FpDef.get(fp_dir(lib)+"/"+name+".kicad_mod", name)
            rec=self.pcb.setdefault(ref, dict(fpdef=fpd,value=value,nets={},xy=ppos,rot=prot,side=side))
            for num,net in nets.items():
                if net not in (None,"__NC__"):
                    rec['nets'][num]=net
                    if net not in RAILS: self.nets.add(net)
                    else: self.nets.add(net)

    # convenience two-pin passive (R/C/L/D) placed vertically, pin1 top pin2 bottom
    def rc(self, sh, sd, ref, value, fp, X, Y, net1, net2, side="F", prot=0, ppos=None):
        self.place(sh, sd, ref, value, fp, X, Y, nets={"1":net1,"2":net2}, side=side, prot=prot, ppos=ppos)

    # -------- emit schematic --------
    def _libsyms(self, ids):
        chunks=[]
        for lid in ids:
            txt=self.embeds[lid]
            chunks.append("\t\t"+txt.replace("\n","\n\t\t").rstrip()+"\n")
        return "".join(chunks)

    def emit_sheets(self, paper_map):
        for sh in self.sheets:
            paper=paper_map.get(sh['name'],"A3")
            fileuuid=uid("FILE-"+sh['name'])
            out=(f'(kicad_sch\n\t(version {SCH_VERSION})\n\t(generator "eeschema")\n\t(generator_version "10.0")\n'
                 f'\t(uuid "{fileuuid}")\n\t(paper "{paper}")\n'
                 f'\t(title_block\n\t\t(title "TestBoard1 - {sh["title"]}")\n\t\t(rev "A")\n\t\t(company "OpenTools / Flux")\n\t)\n'
                 f'\t(lib_symbols\n{self._libsyms(sorted(sh["syms"]))}\t)\n'
                 + "".join(sh['body'])
                 + f'\t(sheet_instances\n\t\t(path "/" (page "{sh["page"]}"))\n\t)\n\t(embedded_fonts no)\n)\n')
            open(PROJDIR+"/"+sh['name']+".kicad_sch","w",encoding='utf-8').write(out)
            print("  wrote %s.kicad_sch (%d syms, %d bodyparts)"%(sh['name'],len(sh['syms']),len(sh['body'])))

    def emit_root(self, notes=None):
        # sheet symbols
        blocks=""
        x=25.4; y=38.0
        for i,sh in enumerate(self.sheets):
            col=i%3; row=i//3
            sx=25.4+col*88.9; sy=38.0+row*50.8
            blocks+=(f'\t(sheet (at {sx:.2f} {sy:.2f}) (size 63.5 33.02)\n'
                     f'\t\t(fields_autoplaced yes)\n\t\t(stroke (width 0.1524) (type solid))\n\t\t(fill (color 0 0 0 0.0))\n'
                     f'\t\t(uuid "{sh["uuid"]}")\n'
                     f'\t\t(property "Sheetname" "{sh["title"]}" (at {sx:.2f} {sy-1.0:.2f} 0) (effects (font (size 1.6 1.6)) (justify left bottom)))\n'
                     f'\t\t(property "Sheetfile" "{sh["name"]}.kicad_sch" (at {sx:.2f} {sy+34.5:.2f} 0) (effects (font (size 1.27 1.27)) (justify left top)))\n'
                     f'\t\t(instances (project "{PROJ}" (path "/{self.rootuuid}" (page "{sh["page"]}"))))\n'
                     f'\t)\n')
        sheet_inst='\t\t(path "/" (page "1"))\n'
        for sh in self.sheets:
            sheet_inst+=f'\t\t(path "/{sh["uuid"]}" (page "{sh["page"]}"))\n'
        notestxt=""
        if notes:
            ny=170.0
            for ln in notes:
                notestxt+=f'\t(text "{ln}" (at 25.4 {ny:.1f} 0) (effects (font (size 2.0 2.0)) (justify left)) (uuid "{uid()}"))\n'
                ny+=6.0
        out=(f'(kicad_sch\n\t(version {SCH_VERSION})\n\t(generator "eeschema")\n\t(generator_version "10.0")\n'
             f'\t(uuid "{self.rootuuid}")\n\t(paper "A3")\n'
             f'\t(title_block\n\t\t(title "TestBoard1 - VAR-SOM-MX8M-PLUS carrier (root)")\n\t\t(rev "A")\n\t\t(company "OpenTools / Flux")\n\t)\n'
             f'\t(lib_symbols)\n'
             f'\t(text "TestBoard1 : VAR-SOM-MX8M-PLUS proof-of-concept carrier" (at 25.4 25.4 0) (effects (font (size 3.0 3.0) (bold yes)) (justify left)) (uuid "{uid()}"))\n'
             + notestxt + blocks
             + f'\t(sheet_instances\n{sheet_inst}\t)\n\t(embedded_fonts no)\n)\n')
        open(PROJDIR+"/"+PROJ+".kicad_sch","w",encoding='utf-8').write(out)
        print("  wrote root %s.kicad_sch (%d child sheets)"%(PROJ,len(self.sheets)))

    # -------- emit PCB --------
    def _fp_instance(self, ref, rec, netmap):
        import re as _re
        fpd=rec['fpdef']; raw=fpd.raw
        raw=raw.replace('(footprint "%s"'%fpd.name, '(footprint "%s"'%fpd.libid(), 1)
        raw=_re.sub(r'\(version \d+\)\s*','',raw,count=1)
        raw=_re.sub(r'\(generator "[^"]*"\)\s*','',raw,count=1)
        raw=_re.sub(r'\(generator_version "[^"]*"\)\s*','',raw,count=1)
        # insert placement after first footprint-level (layer "...")
        m=_re.search(r'\(layer "[^"]+"\)', raw)
        x,y,rot,side = rec['xy'][0], rec['xy'][1], rec['rot'], rec['side']
        ins=f'\n\t(at {x:.3f} {y:.3f} {rot})\n\t(uuid "{uid("FP-"+ref)}")'
        raw=raw[:m.end()]+ins+raw[m.end():]
        # set reference designator (property + any fp_text reference)
        raw=raw.replace('"REF**"', '"%s"'%ref, 1)
        # inject nets into pads
        out=[]; i=0
        while True:
            j=raw.find('(pad ', i)
            if j<0:
                out.append(raw[i:]); break
            out.append(raw[i:j])
            blk=extract_block(raw, j)
            orig_len=len(blk)
            numm=_re.match(r'\(pad "([^"]*)"', blk)
            num=numm.group(1) if numm else ''
            net=rec['nets'].get(num)
            if net is not None and net in netmap:
                code=netmap[net]
                blk=blk[:-1]+f' (net {code} "{net}"))'
            out.append(blk)
            i=j+orig_len
        body="".join(out)
        # indent one level
        return "\t"+body.replace("\n","\n\t").rstrip()+"\n"

    def emit_pcb(self, layout, W, H, holes, park_rect):
        import re as _re
        # assign positions; parts not in layout go into an on-board grid (park_rect)
        px0,py0,px1,pyt,pitch = park_rect
        parked_x=px0; parked_y=py0
        for ref,rec in sorted(self.pcb.items()):
            if ref in layout:
                x,y,rot=layout[ref]; rec['xy']=(x,y); rec['rot']=rot; rec['side']='F'
            else:
                rec['xy']=(parked_x,parked_y); rec['rot']=0; rec['side']='F'
                parked_x+=pitch
                if parked_x>px1: parked_x=px0; parked_y+=pitch
        # net table
        allnets=set()
        for rec in self.pcb.values():
            allnets.update(rec['nets'].values())
        allnets.update(RAILS)
        netlist=sorted(n for n in allnets if n)
        netmap={n:i+1 for i,n in enumerate(netlist)}
        nettbl='\t(net 0 "")\n'+"".join('\t(net %d "%s")\n'%(c,n) for n,c in netmap.items())
        # footprints
        fpblocks="".join(self._fp_instance(ref,rec,netmap) for ref,rec in self.pcb.items())
        # mounting holes
        mh=""
        mhfp=FpDef.get(FPDIR+"/MountingHole.pretty/MountingHole_3.2mm_M3.kicad_mod","MountingHole_3.2mm_M3")
        for i,(hx,hy) in enumerate(holes):
            rec=dict(fpdef=mhfp,value="M3",nets={},xy=(hx,hy),rot=0,side='F')
            mh+=self._fp_instance("H%d"%(i+1),rec,netmap)
        # edge cuts rectangle
        edge=""
        pts=[(0,0),(W,0),(W,H),(0,H),(0,0)]
        for (x1,y1),(x2,y2) in zip(pts,pts[1:]):
            edge+=(f'\t(gr_line (start {x1:.3f} {y1:.3f}) (end {x2:.3f} {y2:.3f}) '
                   f'(stroke (width 0.15) (type solid)) (layer "Edge.Cuts") (uuid "{uid()}"))\n')
        # zones
        def zone(net, layer):
            code=netmap.get(net,0)
            poly=" ".join("(xy %.3f %.3f)"%(x,y) for x,y in [(0.5,0.5),(W-0.5,0.5),(W-0.5,H-0.5),(0.5,H-0.5)])
            return (f'\t(zone (net {code}) (net_name "{net}") (layers "{layer}") (uuid "{uid()}")\n'
                    f'\t\t(hatch edge 0.5)\n\t\t(connect_pads (clearance 0.25))\n\t\t(min_thickness 0.25)\n'
                    f'\t\t(fill yes (thermal_gap 0.3) (thermal_bridge_width 0.4))\n'
                    f'\t\t(polygon (pts {poly}))\n\t)\n')
        zones=zone("GND","F.Cu")+zone("GND","In1.Cu")+zone("+3V3","In2.Cu")+zone("GND","B.Cu")
        layers=('\t\t(0 "F.Cu" signal)\n\t\t(1 "In1.Cu" signal)\n\t\t(2 "In2.Cu" signal)\n\t\t(31 "B.Cu" signal)\n'
                '\t\t(32 "B.Adhes" user "B.Adhesive")\n\t\t(33 "F.Adhes" user "F.Adhesive")\n'
                '\t\t(34 "B.Paste" user)\n\t\t(35 "F.Paste" user)\n'
                '\t\t(36 "B.SilkS" user "B.Silkscreen")\n\t\t(37 "F.SilkS" user "F.Silkscreen")\n'
                '\t\t(38 "B.Mask" user)\n\t\t(39 "F.Mask" user)\n'
                '\t\t(40 "Dwgs.User" user "User.Drawings")\n\t\t(41 "Cmts.User" user "User.Comments")\n'
                '\t\t(44 "Edge.Cuts" user)\n\t\t(46 "B.CrtYd" user "B.Courtyard")\n\t\t(47 "F.CrtYd" user "F.Courtyard")\n'
                '\t\t(48 "B.Fab" user)\n\t\t(49 "F.Fab" user)\n')
        stackup=('\t\t(stackup\n'
                 '\t\t\t(layer "F.SilkS" (type "Top Silk Screen"))\n'
                 '\t\t\t(layer "F.Paste" (type "Top Solder Paste"))\n'
                 '\t\t\t(layer "F.Mask" (type "Top Solder Mask") (thickness 0.01))\n'
                 '\t\t\t(layer "F.Cu" (type "copper") (thickness 0.035))\n'
                 '\t\t\t(layer "dielectric 1" (type "prepreg") (thickness 0.2104) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))\n'
                 '\t\t\t(layer "In1.Cu" (type "copper") (thickness 0.0175))\n'
                 '\t\t\t(layer "dielectric 2" (type "core") (thickness 1.065) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))\n'
                 '\t\t\t(layer "In2.Cu" (type "copper") (thickness 0.0175))\n'
                 '\t\t\t(layer "dielectric 3" (type "prepreg") (thickness 0.2104) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))\n'
                 '\t\t\t(layer "B.Cu" (type "copper") (thickness 0.035))\n'
                 '\t\t\t(layer "B.Mask" (type "Bottom Solder Mask") (thickness 0.01))\n'
                 '\t\t\t(layer "B.Paste" (type "Bottom Solder Paste"))\n'
                 '\t\t\t(layer "B.SilkS" (type "Bottom Silk Screen"))\n'
                 '\t\t\t(copper_finish "None")\n\t\t\t(dielectric_constraints no)\n\t\t)\n')
        setup=('\t(setup\n'+stackup+
               '\t\t(pad_to_mask_clearance 0)\n'
               '\t\t(grid_origin 0 0)\n\t)\n')
        out=(f'(kicad_pcb\n\t(version {PCB_VERSION})\n\t(generator "pcbnew")\n\t(generator_version "10.0")\n'
             f'\t(general\n\t\t(thickness 1.6)\n\t\t(legacy_teardrops no)\n\t)\n'
             f'\t(paper "A3")\n'
             f'\t(layers\n{layers}\t)\n'
             + setup
             + nettbl
             + fpblocks + mh + edge + zones
             + ')\n')
        open(PROJDIR+"/"+PROJ+".kicad_pcb","w",encoding='utf-8').write(out)
        print("  wrote %s.kicad_pcb (%d footprints, %d nets, board %gx%g)"%(PROJ,len(self.pcb),len(netmap),W,H))

B = Builder()
print("Builder ready. SOM units:", sorted(SOM.units))

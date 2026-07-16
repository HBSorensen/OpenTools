"""KiCad 10 s-expression parsing + emit helpers for the VAR-SOM TestBoard1 generator."""
import re, os, hashlib

KICAD = r"C:/Program Files/KiCad/10.0/share/kicad"
SYMDIR = KICAD + "/symbols"
FPDIR  = KICAD + "/footprints"
VARLIB = r"C:/Development/OpenTools/Hardware/VAR-SOM based/Library"
SCH_VERSION = 20260306
PCB_VERSION = 20260206

# Deterministic uuid generator (no randomness needed; stable across regens)
_counter = [0]
def uid(seed=None):
    if seed is None:
        _counter[0]+=1
        seed = "auto-%d"%_counter[0]
    h = hashlib.md5(seed.encode()).hexdigest()
    return "%s-%s-%s-%s-%s"%(h[0:8],h[8:12],h[12:16],h[16:20],h[20:32])

def extract_block(text, start):
    """Return balanced-paren substring beginning at index `start` (which must be a '(')."""
    depth=0; i=start; n=len(text)
    while i<n:
        c=text[i]
        if c=='"':
            i+=1
            while i<n and text[i]!='"':
                if text[i]=='\\': i+=1
                i+=1
        elif c=='(':
            depth+=1
        elif c==')':
            depth-=1
            if depth==0:
                return text[start:i+1]
        i+=1
    raise ValueError("unbalanced")

def find_symbol_start(text, name):
    m=re.search(r'\(symbol "%s"'%re.escape(name), text)
    if not m: raise ValueError("symbol %s not found"%name)
    return m.start()

class SymDef:
    _cache={}
    def __init__(self, libfile, name):
        self.libfile=libfile; self.name=name
        self.libname=os.path.splitext(os.path.basename(libfile))[0]
        txt=open(libfile,encoding='utf-8').read()
        self.block=extract_block(txt, find_symbol_start(txt,name))
        self.is_power = self.block[:200].find('(power')!=-1
        m=re.search(r'\(extends "([^"]+)"', self.block[:400])
        self.extends = m.group(1) if m else None
        if self.extends:
            self.base = SymDef.get(libfile, self.extends)
            self.units = self.base.units
        else:
            self.base=None
            self.units=self._parse_pins()
    @classmethod
    def get(cls, libfile, name):
        key=(libfile,name)
        if key not in cls._cache: cls._cache[key]=SymDef(libfile,name)
        return cls._cache[key]
    def libid(self): return "%s:%s"%(self.libname,self.name)
    def embedded(self):
        return self.block.replace('(symbol "%s"'%self.name, '(symbol "%s:%s"'%(self.libname,self.name), 1)
    def embedded_all(self):
        """Return [(libid, block_text)]. Derived symbols are FLATTENED: the base geometry is
        renamed to this symbol's name so the cached block is self-contained (KiCad cannot resolve
        an (extends ..) to a differently-named embedded parent)."""
        if self.base is not None:
            # flatten: derived's own header/properties + parent's graphic sub-symbols (renamed)
            der=re.sub(r'\s*\(extends "[^"]+"\)','',self.block,count=1)
            subs=""
            for m in re.finditer(r'\(symbol "%s_\d+_\d+"'%re.escape(self.base.name), self.base.block):
                blk=extract_block(self.base.block, m.start())
                blk=blk.replace('"%s_'%self.base.name, '"%s_'%self.name)
                subs+="\t\t"+blk.replace("\n","\n\t\t")+"\n"
            cut=der.rstrip()
            der=cut[:cut.rfind(')')]+subs+"\t)"
            der=der.replace('(symbol "%s"'%self.name, '(symbol "%s"'%self.libid(), 1)
            return [(self.libid(), der)]
        return [(self.libid(), self.embedded())]
    def _parse_pins(self):
        units={}
        for m in re.finditer(r'\(symbol "%s_(\d+)_(\d+)"'%re.escape(self.name), self.block):
            u=int(m.group(1))
            sub=extract_block(self.block, m.start())
            plist=units.setdefault(u,[])
            for pm in re.finditer(r'\(pin ', sub):
                blk=extract_block(sub, pm.start())
                at=re.search(r'\(at ([-\d.]+) ([-\d.]+) (\d+)\)',blk)
                ln=re.search(r'\(length ([-\d.]+)\)',blk)
                nm=re.search(r'\(name "([^"]*)"',blk)
                num=re.search(r'\(number "([^"]*)"',blk)
                if not (at and num): continue
                plist.append(dict(type=blk.split()[1],
                                  x=float(at.group(1)),y=float(at.group(2)),rot=int(at.group(3)),
                                  length=float(ln.group(1)) if ln else 0.0,
                                  name=nm.group(1) if nm else '',
                                  num=num.group(1)))
        return units
    def all_pins(self):
        out=[]
        for u,pl in self.units.items():
            for p in pl:
                q=dict(p); q['unit']=u; out.append(q)
        return out
    def pins_for_unit(self,u):
        # unit u pins plus common unit-0 pins
        return self.units.get(u,[])+self.units.get(0,[])

class FpDef:
    _cache={}
    def __init__(self, libfile, name):
        self.libfile=libfile; self.name=name
        self.libname=os.path.splitext(os.path.basename(libfile))[0]
        self.raw=open(libfile,encoding='utf-8').read()
        self.pads=self._parse_pads()
    @classmethod
    def get(cls, libfile, name):
        key=(libfile,name)
        if key not in cls._cache: cls._cache[key]=FpDef(libfile,name)
        return cls._cache[key]
    def libid(self): return "%s:%s"%(self.libname,self.name)
    def _parse_pads(self):
        pads=[]
        for pm in re.finditer(r'\(pad ', self.raw):
            blk=extract_block(self.raw, pm.start())
            num=re.match(r'\(pad "([^"]*)"',blk)
            at=re.search(r'\(at ([-\d.]+) ([-\d.]+)',blk)
            pads.append(dict(num=num.group(1) if num else '', x=float(at.group(1)) if at else 0.0,
                             y=float(at.group(2)) if at else 0.0))
        return pads

"""TestBoard1 design content: all schematic sheets + PCB layout + emit."""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from build import *   # B, S, SOM, SymDef, FPDIR, PROJDIR ...

# ---------------- footprint shortcuts ----------------
FP = dict(
 R0805="Resistor_SMD:R_0805_2012Metric", R0603="Resistor_SMD:R_0603_1608Metric",
 C0805="Capacitor_SMD:C_0805_2012Metric", C0603="Capacitor_SMD:C_0603_1608Metric",
 C1210="Capacitor_SMD:C_1210_3225Metric",
 LED0805="LED_SMD:LED_0805_2012Metric", L0805="Inductor_SMD:L_0805_2012Metric",
 SMB="Diode_SMD:D_SMB", SMA="Diode_SMD:D_SMA", poly="Fuse:Fuse_1812_4532Metric",
 fuse="Fuse:Fuse_1206_3216Metric",
 ind="Inductor_SMD:L_Bourns_SRR1260", to263="Package_TO_SOT_SMD:TO-263-5_TabPin3",
 qfn48="Package_DFN_QFN:QFN-48-1EP_7x7mm_P0.5mm_EP5.1x5.1mm",
 rj45="Connector_RJ:RJ45_Amphenol_RJMG1BD3B8K1ANR",
 usba="Connector_USB:USB_A_Molex_67643_Horizontal",
 sot236="Package_TO_SOT_SMD:SOT-23-6", sot353="Package_TO_SOT_SMD:SOT-353_SC-70-5",
 microsd="Connector_Card:microSD_HC_Hirose_DM3AT-SF-PEJM5",
 sk6812="LED_SMD:LED_SK6812_PLCC4_5.0x5.0mm_P3.2mm",
 barrel="Connector_BarrelJack:BarrelJack_CUI_PJ-036AH-SMT_Horizontal",
 xtal="Crystal:Crystal_SMD_HC49-SD",
 hdr2x20="Connector_PinHeader_2.54mm:PinHeader_2x20_P2.54mm_Vertical",
 hdr2x05="Connector_PinHeader_2.54mm:PinHeader_2x05_P2.54mm_Vertical",
 hdr2x03="Connector_PinHeader_2.54mm:PinHeader_2x03_P2.54mm_Vertical",
 hdr1x03="Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
 btn="Button_Switch_SMD:SW_Push_1P1T_NO_CK_PTS125Sx43SMTR",
 mh="MountingHole:MountingHole_3.2mm_M3",
)
# symbol handles
R=S("Device","R"); C=S("Device","C"); L=S("Device","L"); LEDd=S("Device","LED")
D_SCH=S("Device","D_Schottky"); D_TVS=S("Device","D_TVS"); FB=S("Device","FerriteBead")
POLY=S("Device","Polyfuse"); XTAL=S("Device","Crystal"); SWP=S("Switch","SW_Push"); FUSE=S("Device","Fuse")
LM5=S("Regulator_Switching","LM2596S-5"); LM33=S("Regulator_Switching","LM2596S-3.3")
PHY=S("Interface_Ethernet","KSZ9031RNXCA"); RJ=S("Connector","RJ45_Amphenol_RJMG1BD3B8K1ANR")
USBA=S("Connector","USB_A"); ESD=S("Power_Protection","USBLC6-2SC6")
SDC=S("Connector","Micro_SD_Card_Det_Hirose_DM3AT"); SK=S("LED","SK6812")
BUF=S("74xGxx","74LVC1G125"); BJ=S("Connector","Barrel_Jack_Switch")
H2x20=S("Connector_Generic","Conn_02x20_Odd_Even"); H2x05=S("Connector_Generic","Conn_02x05_Odd_Even")
H2x03=S("Connector_Generic","Conn_02x03_Odd_Even"); H1x03=S("Connector_Generic","Conn_01x03")

# ---- ref counter ----
_rc={}
def R_(pfx):
    _rc[pfx]=_rc.get(pfx,0)+1; return "%s%d"%(pfx,_rc[pfx])

def cap(sh,val,x,y,n1,n2,fp=None):  B.rc(sh,C,R_("C"),val,fp or FP['C0603'],x,y,n1,n2)
def res(sh,val,x,y,n1,n2,fp=None):  B.rc(sh,R,R_("R"),val,fp or FP['R0805'],x,y,n1,n2)
def led(sh,ref,val,x,y,k,a):        B.rc(sh,LEDd,ref,val,FP['LED0805'],x,y,k,a)

# ==================================================================
# SHEET : SOM connector (all 8 banks)
# ==================================================================
def sheet_som():
    sh=B.sheet("SOM","SoM Connector (VAR-SOM-MX8M-PLUS)")
    sig = {
      "1":"RGMII_TXCTL","3":"RGMII_TD3","4":"RGMII_RD0","5":"RGMII_TD2","6":"RGMII_RD1",
      "9":"RGMII_TD1","10":"RGMII_RD2","11":"RGMII_TD0","12":"RGMII_RD3","15":"RGMII_RXCTL",
      "16":"RGMII_RXC","30":"ENET_MDIO","58":"RGMII_TXC","74":"ENET_MDC",
      "108":"USB2_DN","110":"USB2_DP","114":"USB1_DN","116":"USB1_DP",
      "60":"SD_CLK","64":"SD_CMD","62":"SD_D0","63":"SD_D1","61":"SD_D2","65":"SD_D3",
      "39":"SPI2_CS0","41":"SPI2_MISO","43":"SPI2_SCLK","45":"SPI2_MOSI",
      "44":"UART3_RXD","46":"UART3_TXD",
      "50":"SPI1_MISO","51":"SPI1_CS0","52":"SPI1_MOSI","53":"SPI1_SCLK",
      "87":"I2C3_SDA","88":"I2C3_SCL","90":"I2C4_SDA","92":"I2C4_SCL",
      "124":"DBG_TXD","175":"DBG_RXD","187":"I2C1_SDA","189":"I2C1_SCL",
      "29":"SD_CD","40":"RGB_DIN_3V3","69":"PER_INT","70":"PER_RST","72":"SPI1_CS1",
      "75":"SPI2_CS1","77":"SPI2_INT","80":"SPI1_INT","82":"RPI_GPIO4","86":"RPI_GPIO17",
      "117":"RPI_CE1","173":"RPI_GPIO18","42":"BOOT_SEL","98":"SYS_nRST",
    }
    pos={1:(105,150),2:(275,150),3:(445,150),4:(575,150),
         5:(105,325),6:(275,325),7:(445,325),8:(575,325)}
    for u in range(1,9):
        nets={}
        for p in SOM.units[u]:
            n=p['num']; nm=p['name']
            if nm in ("GND","AGND"): nets[n]="GND"
            elif nm=="VCC_SOM": nets[n]="+5V"
            elif nm.startswith("VDD_ENET"): nets[n]="+3V3"
            elif n in sig: nets[n]=sig[n]
            else: nets[n]=None
        cx,cy=pos[u]
        B.place(sh, SOM, "U1", "VAR-SOM-MX8M-PLUS", "VAR-SOM:VAR-SOM_SODIMM-200",
                cx, cy, unit=u, nets=nets)
    B.text(sh,"VAR-SOM-MX8M-PLUS 200-pin SO-DIMM. VCC_SOM<-+5V, VDD_ENET<-+3V3, unused display/audio = NC.",30,35,2.4)

# ==================================================================
# SHEET : Power
# ==================================================================
def sheet_power():
    sh=B.sheet("Power","Power input / regulators / status LEDs")
    # 12V input: barrel jack -> fuse -> reverse-polarity Schottky -> +12V ; TVS clamp
    B.place(sh,BJ,"J1","12V DC 2.1mm",FP['barrel'],40,70,nets={"1":"V12_IN","2":"GND","3":"GND"})
    B.rc(sh,FUSE,"F1","2A",FP['fuse'],70,55,"V12_IN","V12_F")
    B.place(sh,D_SCH,"D1","SS34",FP['SMB'],95,55,nets={"1":"+12V","2":"V12_F"})  # K=+12V, A=V12_F
    B.rc(sh,D_TVS,"D2","SMBJ15A",FP['SMB'],120,70,"+12V","GND")
    cap(sh,"470uF",135,70,"+12V","GND",FP['C1210'])
    B.pwr_flag(sh,"+12V",150,55); B.pwr_flag(sh,"GND",150,90)
    # ---- 5V buck (LM2596-5) ----
    B.place(sh,LM5,"U2","LM2596S-5.0",FP['to263'],80,150,
            nets={"1":"+12V","2":"SW5","3":"GND","4":"+5V","5":"GND"})
    cap(sh,"470uF",50,165,"+12V","GND",FP['C1210'])
    B.place(sh,D_SCH,"D3","SS54",FP['SMB'],100,175,nets={"1":"SW5","2":"GND"})  # catch: K=SW5,A=GND
    B.rc(sh,L,"L1","33uH",FP['ind'],120,150,"SW5","+5V")
    cap(sh,"470uF",140,165,"+5V","GND",FP['C1210'])
    cap(sh,"100nF",155,165,"+5V","GND")
    B.pwr_flag(sh,"+5V",165,140)
    # ---- 3V3 buck (LM2596-3.3) from +5V ----
    B.place(sh,LM33,"U3","LM2596S-3.3",FP['to263'],80,240,
            nets={"1":"+5V","2":"SW3","3":"GND","4":"+3V3","5":"GND"})
    cap(sh,"220uF",50,255,"+5V","GND",FP['C1210'])
    B.place(sh,D_SCH,"D4","SS54",FP['SMB'],100,265,nets={"1":"SW3","2":"GND"})
    B.rc(sh,L,"L2","33uH",FP['ind'],120,240,"SW3","+3V3")
    cap(sh,"220uF",140,255,"+3V3","GND",FP['C1210'])
    cap(sh,"100nF",155,255,"+3V3","GND")
    B.pwr_flag(sh,"+3V3",165,230)
    # ---- status LEDs ----
    res(sh,"1k",230,60,"+12V","LED12"); led(sh,"DL1","RED",230,80,"GND","LED12")
    res(sh,"1k",255,60,"+5V","LED5");   led(sh,"DL2","YELLOW",255,80,"GND","LED5")
    res(sh,"1k",280,60,"+3V3","LED33"); led(sh,"DL3","GREEN",280,80,"GND","LED33")
    B.text(sh,"Status LEDs: 12V(red) / 5V(yellow) / 3V3(green)",225,45,2.0)
    # ---- reset + boot strap ----
    res(sh,"10k",230,150,"+3V3","SYS_nRST")
    B.place(sh,SWP,"SW1","RESET",FP['btn'],230,175,nets={"1":"SYS_nRST","2":"GND"})
    cap(sh,"100nF",255,165,"SYS_nRST","GND")
    res(sh,"10k",300,150,"BOOT_SEL","GND")   # normal boot strap
    B.text(sh,"SYS_nRST: pull-up + reset button. BOOT_SEL strapped to GND (normal boot).",225,135,2.0)

# ==================================================================
# SHEET : Ethernet
# ==================================================================
def sheet_eth():
    sh=B.sheet("Ethernet","Gigabit Ethernet (KSZ9031 RGMII PHY + MagJack)")
    net={
     "1":"+3V3","4":"VDD_1V2","9":"VDD_1V2","12":"+3V3","14":"VDD_1V2","16":"+3V3","18":"VDD_1V2",
     "23":"VDD_1V2","26":"VDD_1V2","29":"GND","30":"VDD_1V2","34":"+3V3","39":"VDD_1V2","40":"+3V3",
     "43":"VDD_1V2","44":"VDD_1V2","49":"GND",
     "2":"MDI0_P","3":"MDI0_N","5":"MDI1_P","6":"MDI1_N",
     "7":None,"8":None,"10":None,"11":None,"13":None,"47":None,"41":None,
     "15":"ETH_LED2","17":"ETH_LED1",
     "19":"RGMII_TD0","20":"RGMII_TD1","21":"RGMII_TD2","22":"RGMII_TD3","24":"RGMII_TXC","25":"RGMII_TXCTL",
     "27":"RGMII_RD3","28":"RGMII_RD2","31":"RGMII_RD1","32":"RGMII_RD0","33":"RGMII_RXCTL","35":"RGMII_RXC",
     "36":"ENET_MDC","37":"ENET_MDIO","38":"ETH_INT","42":"SYS_nRST","45":"XTAL_XO","46":"XTAL_XI",
     "48":"ETH_ISET",
    }
    B.place(sh,PHY,"U4","KSZ9031RNXCA",FP['qfn48'],120,180,nets=net)
    # crystal 25MHz
    B.place(sh,XTAL,"Y1","25MHz",FP['xtal'],60,150,nets={"1":"XTAL_XI","2":"XTAL_XO"})
    cap(sh,"18pF",45,165,"XTAL_XI","GND"); cap(sh,"18pF",75,165,"XTAL_XO","GND")
    # bias + straps
    res(sh,"6.49k",60,210,"ETH_ISET","GND")
    res(sh,"1k",60,235,"+3V3","ENET_MDIO")     # MDIO pull-up
    res(sh,"10k",85,235,"+3V3","ETH_INT")      # INT pull-up
    # 1V2 core LDO output decoupling
    cap(sh,"10uF",40,270,"VDD_1V2","GND",FP['C0805']); cap(sh,"100nF",55,270,"VDD_1V2","GND")
    B.pwr_flag(sh,"VDD_1V2",70,285)
    # 3V3 PHY decoupling
    for i,x in enumerate((160,175,190,205,220)):
        cap(sh,"100nF",x,110,"+3V3","GND")
    cap(sh,"10uF",235,110,"+3V3","GND",FP['C0805'])
    # RJ45 MagJack
    rjn={"R1":"MDI0_P","R2":"MDI0_N","R4":"ETH_TCT","R3":"MDI1_P","R6":"MDI1_N","R5":"ETH_RCT",
         "R7":None,"R8":"+3V3","L1":"+3V3","L2":"ETH_LED1","L3":"+3V3","L4":"ETH_LED2","SH":"GND"}
    B.place(sh,RJ,"J2","RJ45 MagJack",FP['rj45'],320,180,nets=rjn)
    cap(sh,"100nF",270,140,"ETH_TCT","GND"); cap(sh,"100nF",285,140,"ETH_RCT","GND")
    B.text(sh,"KSZ9031 RGMII PHY. MDI pairs A/B -> MagJack TD/RD (10/100/1000 auto). PHY straps at defaults; verify PHYAD before fab.",40,90,2.2)

# ==================================================================
# SHEET : USB (two Type-A hosts)
# ==================================================================
def usb_port(sh, jref, dn, dp, vbus, esdref, x, name):
    B.place(sh,USBA,jref,name,FP['usba'],x,90,nets={"1":vbus,"2":dn,"3":dp,"4":"GND","SH":"GND"})
    B.place(sh,ESD,esdref,"USBLC6-2SC6",FP['sot236'],x,150,
            nets={"5":vbus,"2":"GND","1":dn,"3":dp,"4":None,"6":None})
    # VBUS: +5V -> polyfuse -> ferrite -> vbus, decoupled
    B.rc(sh,POLY,R_("FP"),"500mA",FP['poly'],x-30,60,"+5V",vbus+"_PF")
    B.rc(sh,FB,R_("FB"),"600R",FP['L0805'],x-10,60,vbus+"_PF",vbus)
    cap(sh,"10uF",x+20,60,vbus,"GND",FP['C0805']); cap(sh,"100nF",x+35,60,vbus,"GND")
    B.pwr_flag(sh,vbus,x+10,45)

def sheet_usb():
    sh=B.sheet("USB","USB 2.0 host ports (front + back)")
    usb_port(sh,"J3","USB1_DN","USB1_DP","VBUS_F","U5",90,"USB-A front (USB1)")
    usb_port(sh,"J4","USB2_DN","USB2_DP","VBUS_B","U6",250,"USB-A back (USB2 - printer)")
    B.text(sh,"Front host = SoM USB1, Back host (Vevor printer) = SoM USB2. VBUS from +5V via poly-fuse + ferrite + USBLC6 ESD.",40,40,2.2)

# ==================================================================
# SHEET : Peripherals (I2C, 2x SPI, microSD, RGB, debug UART)
# ==================================================================
def sheet_periph():
    sh=B.sheet("Peripherals","I2C / SPI / microSD / RGB LEDs / debug UART")
    # I2C IDC (card reader + display)
    B.place(sh,H2x03,"J5","I2C (IDC 2x3)",FP['hdr2x03'],60,70,
            nets={"1":"+3V3","2":"GND","3":"I2C3_SDA","4":"I2C3_SCL","5":"PER_INT","6":"PER_RST"})
    res(sh,"4.7k",30,60,"+3V3","I2C3_SDA"); res(sh,"4.7k",30,85,"+3V3","I2C3_SCL")
    # SPI1 IDC
    B.place(sh,H2x05,"J6","SPI1 (IDC 2x5)",FP['hdr2x05'],130,80,
            nets={"1":"+3V3","2":"GND","3":"SPI1_SCLK","4":"SPI1_MOSI","5":"SPI1_MISO",
                  "6":"SPI1_CS0","7":"SPI1_CS1","8":"SPI1_INT","9":"GND","10":"GND"})
    # SPI2 IDC
    B.place(sh,H2x05,"J7","SPI2 (IDC 2x5)",FP['hdr2x05'],200,80,
            nets={"1":"+3V3","2":"GND","3":"SPI2_SCLK","4":"SPI2_MOSI","5":"SPI2_MISO",
                  "6":"SPI2_CS0","7":"SPI2_CS1","8":"SPI2_INT","9":"GND","10":"GND"})
    # microSD (front)
    B.place(sh,SDC,"J8","microSD",FP['microsd'],300,90,
            nets={"1":"SD_D2","2":"SD_D3","3":"SD_CMD","4":"+3V3","5":"SD_CLK","6":"GND",
                  "7":"SD_D0","8":"SD_D1","9":"SD_CD","10":"GND","SH":"GND"})
    res(sh,"47k",260,60,"+3V3","SD_CD"); cap(sh,"100nF",275,60,"+3V3","GND")
    for i,(nn) in enumerate(("SD_CMD","SD_D0","SD_D1","SD_D2","SD_D3")):
        res(sh,"47k",340+i*15,55,"+3V3",nn)
    # RGB: level shifter + 3x SK6812 chain
    B.place(sh,BUF,"U7","74LVC1G125",FP['sot353'],80,180,
            nets={"1":"GND","2":"RGB_DIN_3V3","4":"RGB_DIN_5V","5":"+5V","3":"GND"})
    cap(sh,"100nF",110,175,"+5V","GND")
    chain=["RGB_DIN_5V","RGB_D1","RGB_D2","RGB_DOUT"]
    for i in range(3):
        din,dout=chain[i],chain[i+1]
        B.place(sh,SK,"DR%d"%(i+1),"SK6812",FP['sk6812'],150+i*40,180,
                nets={"2":din,"4":(dout if i<2 else None),"3":"+5V","1":"GND"})
        cap(sh,"100nF",165+i*40,200,"+5V","GND")
    B.text(sh,"3x SK6812 RGB (5V) driven by SoM GPIO1_IO00 via 74LVC1G125 3V3->5V level shifter.",140,150,2.0)
    # debug UART (UART1)
    B.place(sh,H1x03,"J10","Debug UART",FP['hdr1x03'],80,240,
            nets={"1":"DBG_TXD","2":"DBG_RXD","3":"GND"})
    B.text(sh,"Debug console = SoM UART1 (3.3V TTL).",70,225,2.0)

# ==================================================================
# SHEET : Raspberry Pi 40-pin HAT header
# ==================================================================
def sheet_rpi():
    sh=B.sheet("RPi_Header","Raspberry Pi 40-pin HAT header (J9)")
    rpi={
     "1":"+3V3","2":"+5V","3":"I2C1_SDA","4":"+5V","5":"I2C1_SCL","6":"GND",
     "7":"RPI_GPIO4","8":"UART3_TXD","9":"GND","10":"UART3_RXD",
     "11":"RPI_GPIO17","12":"RPI_GPIO18","13":None,"14":"GND","15":None,"16":None,
     "17":"+3V3","18":None,"19":"SPI2_MOSI","20":"GND","21":"SPI2_MISO","22":None,
     "23":"SPI2_SCLK","24":"SPI2_CS0","25":"GND","26":"RPI_CE1","27":"I2C4_SDA","28":"I2C4_SCL",
     "29":None,"30":"GND","31":None,"32":None,"33":None,"34":"GND","35":None,"36":None,
     "37":None,"38":None,"39":"GND","40":None,
    }
    B.place(sh,H2x20,"J9","RPi 40-pin HAT",FP['hdr2x20'],150,160,nets=rpi)
    res(sh,"3.3k",60,120,"+3V3","I2C1_SDA"); res(sh,"3.3k",60,145,"+3V3","I2C1_SCL")
    B.text(sh,"RPi-style 40-pin header. Power(3V3/5V/GND), I2C1->SDA1/SCL1, SPI2 (shared bus w/ J7),",30,60,2.2)
    B.text(sh,"UART3->TXD/RXD, I2C4->HAT-ID EEPROM (pin27/28), + free GPIO. Lines with no free SoM GPIO = NC.",30,72,2.2)

# ==================================================================
# BUILD
# ==================================================================
sheet_som(); sheet_power(); sheet_eth(); sheet_usb(); sheet_periph(); sheet_rpi()

PAPER={"SOM":"A2"}   # others default A3
NOTES=[
 "Proof-of-concept carrier for the Flux project. Features per Readme.md:",
 "Ethernet (back), 2x USB2.0-A host (front+back), I2C & 2x SPI IDC, microSD (front),",
 "RPi 40-pin HAT header, 12V input with on-board 5V/3V3 regulators, power status LEDs,",
 "3x addressable RGB LEDs. 4-layer PCB, whole-mm outline, 4x M3 mounting holes 4mm from edges.",
]
B.emit_sheets(PAPER)
B.emit_root(NOTES)

# ---------------- PCB layout (mm; whole-mm outline)
# SoM socket at top; the module reclines UP and overhangs the top edge, leaving the board clear.
# Left edge = "back" panel (Ethernet + printer USB); right edge = "front" panel (user USB + microSD).
W,H = 180.0, 130.0
LAYOUT = {
 "U1":(90,22,0),        # SoM SO-DIMM socket (module reclines/overhangs top edge)
 # ---- left edge = BACK panel: Ethernet + printer USB ----
 "J2":(24,58,0), "U4":(48,56,0), "Y1":(48,44,0),
 "J4":(24,90,0), "U6":(48,92,0),
 "DL1":(7,48,0), "DL2":(7,56,0), "DL3":(7,64,0),
 # ---- right edge = FRONT panel: user USB + microSD ----
 "J3":(156,58,0), "U5":(136,58,0), "J8":(156,94,0),
 # ---- peripheral IDC headers (row below SoM keep-out) ----
 "J5":(66,49,0), "J6":(88,49,0), "J7":(110,49,0),
 # ---- RGB + level shifter (row) ----
 "U7":(60,66,0), "DR1":(72,66,0), "DR2":(84,66,0), "DR3":(96,66,0),
 # ---- power band (bottom, main row y=113) ----
 "J1":(16,112,0), "U2":(46,113,0), "L1":(62,113,0), "U3":(86,113,0), "L2":(102,113,0),
 "F1":(30,124,0), "D1":(38,124,0), "D2":(46,124,0), "D3":(62,124,0), "D4":(102,124,0),
 # ---- bottom-right: reset, debug, RPi HAT header (horizontal) ----
 "SW1":(120,103,0), "J10":(136,103,0), "J9":(120,116,90),
}
HOLES=[(4,4),(W-4,4),(4,H-4),(W-4,H-4)]
# on-board grid for decoupling/pull passives (clear central band, no named part intrudes)
PARK=(66.0, 70.0, 146.0, 100.0, 6.0)
B.emit_pcb(LAYOUT,W,H,HOLES,PARK)
print("DONE")

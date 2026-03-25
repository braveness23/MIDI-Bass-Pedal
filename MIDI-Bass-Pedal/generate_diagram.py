"""
Generates MIDI-Bass-Pedal-Wiring.png using Pillow (PIL).
Run: python3 generate_diagram.py
"""

from PIL import Image, ImageDraw, ImageFont
import colorsys, os

# ── Canvas ────────────────────────────────────────────────────────────────────
SCALE   = 2          # render at 2× then downsample for anti-aliasing
W, H    = 1700, 980
IW, IH  = W * SCALE, H * SCALE

img  = Image.new("RGB", (IW, IH), "#1e1e2e")
draw = ImageDraw.Draw(img)

def s(v):
    """Scale a value or tuple for hi-dpi rendering."""
    if isinstance(v, (list, tuple)):
        return tuple(x * SCALE for x in v)
    return v * SCALE

def sp(x, y):     return (x * SCALE, y * SCALE)
def sr(x,y,w,h):  return [x*SCALE, y*SCALE, (x+w)*SCALE, (y+h)*SCALE]

# ── Colours ───────────────────────────────────────────────────────────────────
BG         = "#1e1e2e"
BOARD_BODY = "#1a6b3c"
BOARD_EDGE = "#0d4a28"
PIN_COL    = "#c0c0c0"
WIRE_KEY   = "#4fc3f7"
WIRE_OCT   = "#ffb74d"
WIRE_PANIC = "#ef5350"
WIRE_MODE  = "#ab47bc"
WIRE_VEL   = "#66bb6a"
WIRE_NEO   = "#ffd54f"
WIRE_PWR   = "#ff5252"
WIRE_GND   = "#78909c"
TEXT_MAIN  = "#f8f8f2"
TEXT_DIM   = "#888899"
COMP_BODY  = "#2d2d44"
COMP_BDR   = "#555577"
PANEL_BG   = "#26263a"

# ── Fonts ─────────────────────────────────────────────────────────────────────
def load_font(size):
    for name in [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
    ]:
        if os.path.exists(name):
            return ImageFont.truetype(name, size * SCALE)
    return ImageFont.load_default()

def load_bold(size):
    for name in [
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
    ]:
        if os.path.exists(name):
            return ImageFont.truetype(name, size * SCALE)
    return load_font(size)

F8  = load_font(8);  F9  = load_font(9)
F10 = load_font(10); F12 = load_font(12)
F13 = load_font(13); F16 = load_font(16)
F22 = load_font(22)
B12 = load_bold(12); B16 = load_bold(16); B22 = load_bold(22)

# ── Drawing helpers ───────────────────────────────────────────────────────────
def rrect(x,y,w,h, fill, outline=None, radius=8, lw=2):
    draw.rounded_rectangle(sr(x,y,w,h), radius=radius*SCALE,
                           fill=fill,
                           outline=outline, width=lw*SCALE)

def txt(x, y, s, font=F10, fill=TEXT_MAIN, anchor="lt"):
    draw.text(sp(x,y), s, font=font, fill=fill, anchor=anchor)

def wire(pts, col, lw=2):
    scaled = [sp(x,y) for x,y in pts]
    draw.line(scaled, fill=col, width=lw*SCALE, joint="curve")

def dot(x, y, r=4, col=PIN_COL):
    draw.ellipse([sp(x-r, y-r), sp(x+r, y+r)], fill=col)

# ── Layout ────────────────────────────────────────────────────────────────────
BX, BY = 80, 110        # board top-left
BW, BH = 400, 700       # board size

PIN_X  = BX + BW        # right-edge pin header x
PIN_Y0 = BY + 65        # first pin row y
PSTEP  = 19             # pixels per pin row

used_pins = [
    (22, "D22 Key 0",   WIRE_KEY),
    (23, "D23 Key 1",   WIRE_KEY),
    (24, "D24 Key 2",   WIRE_KEY),
    (25, "D25 Key 3",   WIRE_KEY),
    (26, "D26 Key 4",   WIRE_KEY),
    (27, "D27 Key 5",   WIRE_KEY),
    (28, "D28 Key 6",   WIRE_KEY),
    (29, "D29 Key 7",   WIRE_KEY),
    (30, "D30 Key 8",   WIRE_KEY),
    (31, "D31 Key 9",   WIRE_KEY),
    (32, "D32 Key 10",  WIRE_KEY),
    (33, "D33 Key 11",  WIRE_KEY),
    (34, "D34 Key 12",  WIRE_KEY),
    (17, "D17 Mode",    WIRE_MODE),
    (18, "D18 Panic",   WIRE_PANIC),
    (19, "D19 Oct -",   WIRE_OCT),
    (20, "D20 Oct +",   WIRE_OCT),
    (6,  "D6  NeoPixel",WIRE_NEO),
]

pin_row = {pnum: i for i, (pnum, _, _) in enumerate(used_pins)}

def pin_y(pnum):
    return PIN_Y0 + pin_row[pnum] * PSTEP

# Analog / GND / 5V on bottom edge
A0_X  = BX + 80;  A0_Y  = BY + BH + 8
GND_X = BX + 180; GND_Y = A0_Y
V5_X  = BX + 280; V5_Y  = A0_Y

CX = 660    # component area left edge
CW = 560    # component area width
MX = CX - 28  # routing channel x between board and components

# ── Board ─────────────────────────────────────────────────────────────────────
rrect(BX, BY, BW, BH, BOARD_BODY, BOARD_EDGE, radius=14, lw=3)
txt(BX + BW//2, BY + 22, "Arduino Mega ADK", font=B16, fill=TEXT_MAIN, anchor="mt")

# USB stub
rrect(BX - 30, BY + 60, 30, 44, "#333355", COMP_BDR, radius=4)
txt(BX - 15, BY + 80, "USB", font=F8, fill=TEXT_DIM, anchor="mt")

# Power jack
rrect(BX - 28, BY + 130, 28, 28, "#222233", COMP_BDR, radius=4)
txt(BX - 14, BY + 145, "PWR", font=F8, fill=TEXT_DIM, anchor="mt")

# Pin header bar
header_top = pin_y(22) - PSTEP // 2
header_bot = pin_y(6)  + PSTEP // 2
rrect(PIN_X, header_top, 14, header_bot - header_top, "#2a2a2a", "#555", radius=2)

for (pnum, label, col) in used_pins:
    py = pin_y(pnum)
    dot(PIN_X + 7, py, col=PIN_COL)
    txt(PIN_X - 6, py, label, font=F8, fill=col, anchor="rm")

# Analog header
rrect(BX + 50, BY + BH, 80, 14, "#2a2a2a", "#555", radius=2)
dot(A0_X, A0_Y, col=WIRE_VEL)
txt(A0_X, BY + BH - 4, "A0  Velocity", font=F8, fill=WIRE_VEL, anchor="mb")

rrect(BX + 155, BY + BH, 60, 14, "#2a2a2a", "#555", radius=2)
dot(GND_X, GND_Y, col=WIRE_GND)
txt(GND_X, BY + BH - 4, "GND", font=F8, fill=WIRE_GND, anchor="mb")

rrect(BX + 250, BY + BH, 60, 14, "#2a2a2a", "#555", radius=2)
dot(V5_X, V5_Y, col=WIRE_PWR)
txt(V5_X, BY + BH - 4, "5V", font=F8, fill=WIRE_PWR, anchor="mb")

# ── Keys panel ────────────────────────────────────────────────────────────────
KP_Y = 75;  KP_H = 310
rrect(CX, KP_Y, CW, KP_H, PANEL_BG, WIRE_KEY, radius=10)
txt(CX + CW//2, KP_Y + 10, "13 Keys  (INPUT_PULLUP · active LOW)",
    font=B12, fill=WIRE_KEY, anchor="mt")

KY0    = KP_Y + 32
K_STEP = (KP_H - 48) / 13
KH     = int(K_STEP) - 3
NOTES  = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B","C"]

# GND bus line for keys
BUS_X = CX + 18
draw.line([sp(BUS_X, KY0), sp(BUS_X, KY0 + 12*K_STEP + KH//2)],
          fill=WIRE_GND, width=3*SCALE)
txt(BUS_X, KY0 - 6, "GND", font=F8, fill=WIRE_GND, anchor="mb")

for i in range(13):
    ky = KY0 + i * K_STEP
    pnum = 22 + i
    bpy  = pin_y(pnum)

    # switch body
    sx = CX + 36
    rrect(sx, ky, 80, KH, COMP_BODY, WIRE_KEY, radius=3)
    txt(sx + 40, ky + KH//2, f"Key {i}  {NOTES[i]}",
        font=F8, fill=WIRE_KEY, anchor="mm")

    # GND stub (left terminal)
    wire([(sx, ky + KH//2), (BUS_X, ky + KH//2)], WIRE_GND, lw=1)
    dot(BUS_X, ky + KH//2, r=3, col=WIRE_GND)

    # Signal wire (right terminal → routing channel → board pin)
    rx = sx + 80
    wire([(rx, ky + KH//2), (MX, ky + KH//2), (MX, bpy), (PIN_X + 14, bpy)],
         WIRE_KEY, lw=2)
    dot(PIN_X + 14, bpy, r=3, col=WIRE_KEY)

# ── Control buttons panel ─────────────────────────────────────────────────────
BP_Y = KP_Y + KP_H + 18;  BP_H = 165
rrect(CX, BP_Y, CW, BP_H, PANEL_BG, "#aaaaff", radius=10)
txt(CX + CW//2, BP_Y + 10,
    "Control Buttons  (INPUT_PULLUP · active LOW)",
    font=B12, fill="#aaaaff", anchor="mt")

BTN_DEFS = [
    ("Octave  −",  19, WIRE_OCT),
    ("Octave  +",  20, WIRE_OCT),
    ("Panic",      18, WIRE_PANIC),
    ("Mode Toggle",17, WIRE_MODE),
]
BY0    = BP_Y + 34
B_STEP = (BP_H - 48) / 4
BH2    = int(B_STEP) - 4

B_BUS_X = CX + 18
draw.line([sp(B_BUS_X, BY0), sp(B_BUS_X, BY0 + 3*B_STEP + BH2//2)],
          fill=WIRE_GND, width=3*SCALE)
txt(B_BUS_X, BY0 - 6, "GND", font=F8, fill=WIRE_GND, anchor="mb")

for i, (label, pnum, col) in enumerate(BTN_DEFS):
    by2 = BY0 + i * B_STEP
    bpy = pin_y(pnum)
    bx  = CX + 36
    rrect(bx, by2, 110, BH2, COMP_BODY, col, radius=4)
    txt(bx + 55, by2 + BH2//2, label, font=F9, fill=col, anchor="mm")

    wire([(bx, by2 + BH2//2), (B_BUS_X, by2 + BH2//2)], WIRE_GND, lw=1)
    dot(B_BUS_X, by2 + BH2//2, r=3, col=WIRE_GND)

    wire([(bx + 110, by2 + BH2//2),
          (MX, by2 + BH2//2), (MX, bpy), (PIN_X + 14, bpy)],
         col, lw=2)
    dot(PIN_X + 14, bpy, r=3, col=col)

# ── Velocity pot panel ────────────────────────────────────────────────────────
VP_Y = BP_Y + BP_H + 18;  VP_H = 120
rrect(CX, VP_Y, CW, VP_H, PANEL_BG, WIRE_VEL, radius=10)
txt(CX + CW//2, VP_Y + 10, "Velocity Potentiometer  (10kΩ linear)",
    font=B12, fill=WIRE_VEL, anchor="mt")

PCX = CX + 110;  PCY = VP_Y + 70
R   = 30
draw.ellipse([sp(PCX-R, PCY-R), sp(PCX+R, PCY+R)],
             outline=WIRE_VEL, width=3*SCALE, fill=COMP_BODY)
draw.ellipse([sp(PCX-10, PCY-10), sp(PCX+10, PCY+10)],
             fill="#111122")

# terminals
T_GND = (PCX - 44, PCY)
T_WIP = (PCX,      PCY + R + 6)
T_5V  = (PCX + 44, PCY)

for pt, col, lbl in [(T_GND, WIRE_GND, "GND"),
                      (T_WIP, WIRE_VEL, "Wiper→A0"),
                      (T_5V,  WIRE_PWR,  "5V")]:
    dot(*pt, r=4, col=col)
    txt(pt[0], pt[1] + 12, lbl, font=F8, fill=col, anchor="mt")

# Wiper → A0
wpy = VP_Y + VP_H + 14
wire([(T_WIP[0], T_WIP[1]), (T_WIP[0], wpy),
      (A0_X, wpy), (A0_X, A0_Y)], WIRE_VEL, lw=2)

# GND → GND pin
gpy = VP_Y + VP_H + 26
wire([(T_GND[0], T_GND[1]), (T_GND[0], gpy),
      (GND_X, gpy), (GND_X, GND_Y)], WIRE_GND, lw=2)

# 5V → 5V pin
vpy = VP_Y + VP_H + 38
wire([(T_5V[0], T_5V[1]), (T_5V[0], vpy),
      (V5_X, vpy), (V5_X, V5_Y)], WIRE_PWR, lw=2)

txt(CX + 160, PCY - R + 4, "10kΩ", font=F9, fill=WIRE_VEL, anchor="lt")
txt(CX + 160, PCY + 6,      "linear", font=F8, fill=TEXT_DIM, anchor="lt")

# ── NeoPixel strip panel ──────────────────────────────────────────────────────
NP_Y = VP_Y + VP_H + 60;  NP_H = 110
rrect(CX, NP_Y, CW, NP_H, PANEL_BG, WIRE_NEO, radius=10)
txt(CX + CW//2, NP_Y + 10, "NeoPixel Strip  (40 pixels, 5V)",
    font=B12, fill=WIRE_NEO, anchor="mt")

STRIP_X = CX + 20;  STRIP_Y = NP_Y + 34;  STRIP_W = CW - 40;  STRIP_H = 40
rrect(STRIP_X, STRIP_Y, STRIP_W, STRIP_H, "#111122", WIRE_NEO, radius=4)

NUM_PIX = 18
PW = (STRIP_W - 10) / NUM_PIX
for p in range(NUM_PIX):
    h = p / NUM_PIX
    r,g,b = colorsys.hsv_to_rgb(h, 0.9, 0.9)
    col_hex = "#{:02x}{:02x}{:02x}".format(int(r*255), int(g*255), int(b*255))
    px = STRIP_X + 5 + p * PW
    rrect(px+1, STRIP_Y+4, PW-3, STRIP_H-8, col_hex, radius=2)
txt(STRIP_X + STRIP_W + 4, STRIP_Y + STRIP_H//2, "…40px",
    font=F9, fill=WIRE_NEO, anchor="lm")

DIN_X = STRIP_X + 12;  DIN_Y = STRIP_Y + STRIP_H + 2
dot(DIN_X, DIN_Y, r=4, col=WIRE_NEO)
txt(DIN_X, DIN_Y + 10, "DIN", font=F8, fill=WIRE_NEO, anchor="mt")

GND2_X = STRIP_X + 36;  GND2_Y = DIN_Y
dot(GND2_X, GND2_Y, r=4, col=WIRE_GND)
txt(GND2_X, GND2_Y + 10, "GND", font=F8, fill=WIRE_GND, anchor="mt")

V5B_X = STRIP_X + 60;  V5B_Y = DIN_Y
dot(V5B_X, V5B_Y, r=4, col=WIRE_PWR)
txt(V5B_X, V5B_Y + 10, "5V*", font=F8, fill=WIRE_PWR, anchor="mt")

# DIN wire → D6 board pin
d6_py = pin_y(6)
wire([(DIN_X, DIN_Y), (DIN_X, NP_Y + NP_H + 12),
      (MX, NP_Y + NP_H + 12), (MX, d6_py), (PIN_X + 14, d6_py)],
     WIRE_NEO, lw=2)
dot(PIN_X + 14, d6_py, r=3, col=WIRE_NEO)

# ── Legend ────────────────────────────────────────────────────────────────────
LX = CX + CW + 32;  LY = 80
LW = 280
legend = [
    (WIRE_KEY,   "Key signals (D22–D34)"),
    (WIRE_OCT,   "Octave buttons (D19, D20)"),
    (WIRE_PANIC, "Panic button (D18)"),
    (WIRE_MODE,  "Mode toggle (D17)"),
    (WIRE_VEL,   "Velocity wiper → A0"),
    (WIRE_NEO,   "NeoPixel DIN → D6"),
    (WIRE_PWR,   "5V power"),
    (WIRE_GND,   "GND"),
]
LH = len(legend) * 24 + 36
rrect(LX, LY, LW, LH, PANEL_BG, COMP_BDR, radius=8)
txt(LX + LW//2, LY + 10, "Legend", font=B12, fill=TEXT_MAIN, anchor="mt")
for i, (col, label) in enumerate(legend):
    ly = LY + 36 + i * 24
    draw.line([sp(LX+10, ly), sp(LX+40, ly)], fill=col, width=3*SCALE)
    dot(LX + 40, ly, r=4, col=col)
    txt(LX + 50, ly, label, font=F9, fill=TEXT_MAIN, anchor="lm")

# ── Notes box ─────────────────────────────────────────────────────────────────
NY = LY + LH + 16
notes = [
    "All key & button inputs: INPUT_PULLUP,",
    "  one leg to GND, other leg to Arduino pin.",
    "",
    "Velocity pot: wiper → A0, ends to 5V & GND.",
    "",
    "NeoPixels (*): use an external 5V supply for",
    "  full brightness. Add 300–500Ω on DIN line.",
    "  100–470µF cap across strip 5V/GND.",
    "",
    "Debounce: 20ms per key & button in firmware.",
    "Mono mode: last-note priority with note stack.",
    "Panic: CC123 All-Notes-Off on all 16 channels.",
]
NW = LW + 20
NH = len(notes) * 18 + 28
rrect(LX, NY, NW, NH, PANEL_BG, COMP_BDR, radius=8)
txt(LX + NW//2, NY + 10, "Notes", font=B12, fill=TEXT_MAIN, anchor="mt")
for i, note in enumerate(notes):
    txt(LX + 10, NY + 30 + i * 18, note, font=F9, fill=TEXT_DIM if note else TEXT_DIM, anchor="lt")

# ── Title ─────────────────────────────────────────────────────────────────────
txt(W//2, 30, "MIDI Bass Pedal — v3 Wiring Diagram",
    font=B22, fill=TEXT_MAIN, anchor="mt")
txt(W//2, 62,
    "Arduino Mega ADK  |  13 Keys  Octave ±  Panic  Mode  Velocity Pot  NeoPixel",
    font=F13, fill=TEXT_DIM, anchor="mt")

# ── Downsample ────────────────────────────────────────────────────────────────
out = img.resize((W, H), Image.LANCZOS)
out_path = "MIDI-Bass-Pedal/MIDI-Bass-Pedal-Wiring.png"
out.save(out_path, "PNG")
print(f"Saved {W}×{H} PNG → {out_path}")

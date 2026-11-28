"""build_reports.py -- build the two SIM16 report PDFs (report tooling, not part of the C code)

Reads output/*.txt and `./sim16 list ...` (never retyped), writes one HTML file per
report with fixed-size pages and ##/## page numbers, then prints each to PDF with
headless Chrome.

    python3 report/build_reports.py          (run `make sim16 outputs` first)

Outputs: report/Assignment1_CPU_Design.pdf, report/Assignment2_Program_Layout.pdf
"""

import html
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "report", "build")
CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"

NAMES = ["Jahnavi Kedia", "Shrutthi Khatta", "Snehil Raut"]
COURSE = "CMPE 220 System Software"
RULE = "=" * 78


# ---------------------------------------------------------------- inputs

def read_output(name):
    with open(os.path.join(ROOT, "output", name)) as f:
        return f.read().rstrip("\n").split("\n")


def sim16_list(program):
    out = subprocess.run([os.path.join(ROOT, "sim16"), "list", program],
                         capture_output=True, text=True, check=True, cwd=ROOT)
    return out.stdout.rstrip("\n").split("\n")


def chunks(lines):
    """Split a run into blocks: each starts at a '=' rule followed by a title or STEP line."""
    starts = [i for i in range(len(lines) - 1)
              if lines[i] == RULE and (lines[i + 1].startswith(" ") or lines[i + 1].startswith("STEP"))]
    blocks = []
    for k, s in enumerate(starts):
        e = starts[k + 1] if k + 1 < len(starts) else len(lines)
        block = lines[s:e]
        while block and block[-1] == "":
            block.pop()
        blocks.append(block)
    summary = [l for l in lines if l.startswith("Program halted")]
    if blocks and summary and blocks[-1][-1] == summary[0]:
        blocks[-1] = blocks[-1][:-1]
        while blocks[-1] and blocks[-1][-1] == "":
            blocks[-1].pop()
    return blocks, summary[0]


def pre(lines, cls="dump"):
    return '<pre class="%s">%s</pre>' % (cls, html.escape("\n".join(lines)))


# ---------------------------------------------------------------- page helpers

class Report:
    def __init__(self, title):
        self.title = title
        self.pages = []
        self.fig = 0
        self.tab = 0

    def figure(self, caption):
        self.fig += 1
        return "Figure %d: %s" % (self.fig, caption)

    def table(self, caption):
        self.tab += 1
        return "Table %d: %s" % (self.tab, caption)

    def add(self, body, header=True, cls=""):
        self.pages.append((body, header, cls))

    def html(self):
        n = len(self.pages)
        out = [HEAD % html.escape(self.title)]
        for i, (body, header, cls) in enumerate(self.pages, 1):
            hdr = ('<div class="hdr"><span>SIM16 &middot; %s</span><span>%s</span></div>'
                   % (html.escape(self.title), COURSE)) if header else ""
            out.append('<section class="page %s">%s%s<div class="pno">%02d/%02d</div></section>'
                       % (cls, hdr, body, i, n))
        out.append(TAIL)
        return "\n".join(out)


def fig(svg_or_html, caption):
    return '<figure>%s<figcaption>%s</figcaption></figure>' % (svg_or_html, caption)


def tab(rows, caption, header, widths=None, cls=""):
    cols = ""
    if widths:
        cols = "<colgroup>%s</colgroup>" % "".join('<col style="width:%s">' % w for w in widths)
    th = "".join("<th>%s</th>" % h for h in header)
    trs = "".join("<tr>%s</tr>" % "".join("<td>%s</td>" % c for c in r) for r in rows)
    return ('<table class="%s"><caption>%s</caption>%s<thead><tr>%s</tr></thead><tbody>%s</tbody></table>'
            % (cls, caption, cols, th, trs))


def code(s):
    return "<code>%s</code>" % html.escape(s)


def title_page(rep, assignment, subtitle):
    names = "".join('<div class="name">%s</div>' % html.escape(n) for n in NAMES)
    rep.add('<div class="title">'
            '<div class="course">%s</div>'
            '<div class="assign">Assignment: %s</div>'
            '<div class="proj">SIM16 &mdash; %s</div>'
            '<div class="team-label">Team</div>%s</div>' % (COURSE, assignment, subtitle, names),
            header=False)


CONTRIB = [
    ("Jahnavi",
     "CPU core (<code>cpu.c</code>, <code>cpu.h</code>): registers, fetch/decode/execute, all 24 "
     "instructions, flags, stack operations, CALL/RET; unit tests and golden runs "
     "(<code>tests/test_cpu.c</code>)",
     "CPU block diagram, instruction encoding table, ISA summary (registers, memory map), "
     "Assignment 1 execution-trace pages"),
    ("Shrutthi",
     "Assembler (<code>assembler.c</code>, <code>text.c</code>): two-pass assembly and disassembly; "
     "command-line runner (<code>main.c</code>), <code>Makefile</code>, <code>README.md</code>; the 8! "
     "assembly program and the recursive factorial C program with its assembly translation",
     "C &harr; assembly translation tables, stack-frame / calling-convention diagram, program-layout "
     "(segment) diagram"),
    ("Snehil",
     "Loader (<code>loader.c</code>): placing the program into the four segments; printer "
     "(<code>printer.c</code>): memory grid, step traces, four-segment views, per-call stack frames; "
     "the 8&sup2; C program with its assembly translation",
     "Assignment 2 per-call memory pages; assembled and formatted both reports"),
]


def contributions_page(rep):
    body = '<h1>Team contributions</h1>'
    body += tab([("<b>%s</b>" % m, c, r) for m, c, r in CONTRIB], rep.table("Team member contributions"),
                ["Member", "Code", "Report"], ["14%", "50%", "36%"], "contrib")
    body += '<p class="note"><code>isa.h</code> (the shared instruction-set definition) is the team\'s jointly agreed design.</p>'
    rep.add(body)


def run_page(rep, assignment):
    if assignment == 1:
        rows = [(code("./sim16 factorial"), "8! in assembly, memory after every instruction", "29 steps, Output: 40320"),
                (code("./sim16 power"), "8&sup2; (C &rarr; assembly), memory after every instruction", "20 steps, Output: 64"),
                (code("./sim16 list factorial"), "Assembly listing: address, machine code, source", "8 instructions, 2 data words"),
                (code("./sim16 list power"), "Assembly listing of the 8&sup2; program", "13 instructions, 3 data words"),
                (code("./test_cpu"), "Unit tests + golden runs", "0 failed")]
        sees = "8! then 8&sup2;, memory after every instruction; ends with Output: 40320 and Output: 64"
    else:
        rows = [(code("./sim16 recursive"), "Load view, 4 segments after every recursive call, final view", "5 calls, 52 steps, Output: 120"),
                (code("./sim16 list recursive"), "Assembly listing: address, machine code, source", "19 instructions, 2 data words"),
                (code("./test_cpu"), "Unit tests + golden runs", "0 failed")]
        sees = "load view, the 4 segments after each of the 5 recursive calls, final state; ends with Output: 120"
    steps = [("1", "Unzip, open a terminal in the folder", code("cd sim16_assignment%d" % assignment)),
             ("2", "Compile", code("make")),
             ("3", "Run", code("./sim16")),
             ("&nbsp;", "You will see", sees)]
    body = '<h1>How to compile and run</h1><p class="sub">Needs only a C compiler (gcc or clang). Windows: WSL or MinGW.</p>'
    body += tab(steps, rep.table("How to compile and run"), ["Step", "Action", "Command"], ["8%", "32%", "60%"])
    body += tab(rows, rep.table("Other commands"), ["Command", "Shows", "Expected result"], ["26%", "46%", "28%"], "plain")
    body += tab([(code("cc -std=c99 -Wall -o sim16 src/main.c src/text.c src/cpu.c src/loader.c src/assembler.c src/programs.c src/printer.c"),),
                 (code("cc -std=c99 -Wall -o test_cpu tests/test_cpu.c src/text.c src/cpu.c src/loader.c src/assembler.c src/programs.c src/printer.c"),)],
                rep.table("Compile without make"), ["Command"], None, "plain")
    rep.add(body)


# ---------------------------------------------------------------- SVG diagrams

def svg(w, h, body, maxw="100%"):
    return ('<svg viewBox="0 0 %d %d" style="width:%s" xmlns="http://www.w3.org/2000/svg" '
            'font-family="Helvetica Neue, Helvetica, Arial, sans-serif">%s%s</svg>' % (w, h, maxw, MARKERS, body))


MARKERS = """<defs>
<marker id="ab" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="#222"/></marker>
<marker id="abl" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="4" markerHeight="4" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="#0050ef"/></marker>
<marker id="agr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="4" markerHeight="4" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="#008a00"/></marker>
<marker id="ard" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="4" markerHeight="4" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="#a20025"/></marker>
</defs>"""


def box(x, y, w, h, fill, stroke, dash=False, rx=0):
    return ('<rect x="%g" y="%g" width="%g" height="%g" rx="%g" fill="%s" stroke="%s" stroke-width="1.3"%s/>'
            % (x, y, w, h, rx, fill, stroke, ' stroke-dasharray="5,4"' if dash else ""))


def text(x, y, s, size=12, weight="normal", anchor="middle", fill="#1a1a1a", mono=False):
    fam = ' font-family="Menlo, Courier New, monospace"' if mono else ""
    return ('<text x="%g" y="%g" font-size="%g" font-weight="%s" text-anchor="%s" fill="%s"%s>%s</text>'
            % (x, y, size, weight, anchor, fill, fam, html.escape(s)))


def line(points, color="#222", width=1.8, marker="ab", start=False):
    pts = " ".join("%g,%g" % p for p in points)
    m = ' marker-end="url(#%s)"' % marker if marker else ""
    if start:
        m += ' marker-start="url(#%s)"' % marker
    return '<polyline points="%s" fill="none" stroke="%s" stroke-width="%g"%s/>' % (pts, color, width, m)


def label(x, y, s, size=11, fill="#1a1a1a", bg="#f5f5f5"):
    w = len(s) * size * 0.56 + 8
    return ('<rect x="%g" y="%g" width="%g" height="%g" fill="%s"/>' % (x - w / 2, y - size + 1, w, size + 4, bg)
            + text(x, y + 2, s, size, fill=fill))


def cpu_block_diagram():
    """Figure: SIM16 CPU block diagram (same layout as report/jahnavi/cpu_block_diagram.drawio)."""
    b = []
    b.append(box(20, 20, 630, 440, "#f5f5f5", "#666", rx=10) + text(335, 46, "SIM16 CPU", 16, "bold"))
    b.append(box(40, 60, 180, 380, "#dae8fc", "#6c8ebf") + text(130, 80, "Register File", 12, "bold")
             + text(130, 95, "8 x 16-bit", 11))
    regs = ["R0  (return value)", "R1", "R2", "R3", "R4", "R5", "FP  (R6)", "SP  (R7)"]
    for i, r in enumerate(regs):
        y = 105 + i * 41
        b.append(box(55, y, 150, 34, "#fff", "#6c8ebf") + text(130, y + 21, r, 11, "bold" if i >= 6 else "normal", mono=True))
    for (y, a, c) in [(230, "Flags", "Z | N"), (300, "PC", "next instr addr"), (370, "IR", "fetched word")]:
        b.append(box(250, y, 130, 44, "#fff2cc", "#d6b656") + text(315, y + 18, a, 12, "bold") + text(315, y + 34, c, 11))
    b.append('<polygon points="470,70 630,70 611,180 489,180" fill="#d5e8d4" stroke="#82b366" stroke-width="1.3"/>')
    b.append(text(550, 110, "ALU", 13, "bold") + text(550, 128, "ADD SUB MUL", 11) + text(550, 144, "CMP", 11))
    b.append(box(470, 230, 160, 180, "#e1d5e7", "#9673a6") + text(550, 280, "Control Unit /", 12, "bold")
             + text(550, 296, "Decoder", 12, "bold") + text(550, 330, "fetch > decode > execute", 10)
             + text(550, 362, "opcode | rd | rs |", 10, mono=True) + text(550, 377, "off5 / imm8", 10, mono=True))
    # datapath
    b.append(line([(220, 100), (466, 100)]) + label(343, 96, "operands rd, rs"))
    b.append(line([(483, 145), (224, 145)]) + label(343, 141, "result"))
    b.append(line([(510, 180), (510, 205), (315, 205), (315, 226)]) + label(410, 201, "Z, N"))
    b.append(line([(550, 230), (550, 184)]) + label(550, 214, "ALU op"))
    b.append(line([(380, 252), (466, 252)]) + label(423, 248, "Z/N"))
    b.append(line([(470, 322), (384, 322)]) + label(425, 318, "PC+1 / jump"))
    b.append(line([(380, 392), (466, 392)]) + label(423, 388, "fields"))
    # memory
    b.append(box(880, 20, 240, 440, "#f5f5f5", "#666", rx=10) + text(1000, 42, "Main Memory", 14, "bold")
             + text(1000, 58, "256 x 16-bit words", 12, "bold"))
    segs = [(65, 139, "#dae8fc", "#6c8ebf", "TEXT  0x00-0x5F", "machine code", False),
            (204, 46, "#d5e8d4", "#82b366", "DATA  0x60-0x7F", "globals", False),
            (250, 46, "#fff", "#999", "HEAP  0x80-0x9F", "(unused)", True),
            (296, 139, "#f8cecc", "#b85450", "STACK  0xA0-0xFF", "grows down from 0xFF", False)]
    for (y, h, f, s, a, c, d) in segs:
        b.append(box(895, y, 210, h, f, s, d) + text(1000, y + h / 2 - 2, a, 12, "bold") + text(1000, y + h / 2 + 14, c, 11))
    b.append(text(1000, 412, "empty: SP = FP = 0x100", 11))
    # buses
    b.append(line([(650, 120), (874, 120)], "#0050ef", 5, "abl") + text(765, 104, "ADDRESS BUS (8-bit)", 12, "bold", fill="#0050ef")
             + text(765, 140, "PC, addr8, rs+off5, SP", 11, fill="#0050ef"))
    b.append(line([(656, 240), (874, 240)], "#008a00", 5, "agr", start=True) + text(765, 224, "DATA BUS (16-bit)", 12, "bold", fill="#008a00")
             + text(765, 260, "fetch, LD/ST, PUSH/POP", 11, fill="#008a00"))
    b.append(line([(650, 360), (874, 360)], "#a20025", 5, "ard") + text(765, 344, "CONTROL BUS", 12, "bold", fill="#a20025")
             + text(765, 380, "READ / WRITE", 11, fill="#a20025"))
    return svg(1140, 480, "".join(b))


FIELD_COLORS = {"opcode": "#e1d5e7", "rd": "#dae8fc", "rs": "#d5e8d4", "off5": "#fff2cc", "imm8 / addr8": "#ffe6cc"}


def bit_layout():
    """Figure: instruction word layout, two field variants + a worked example."""
    x0, cw = 150, 40
    b = []
    for i in range(16):
        b.append(text(x0 + i * cw + cw / 2, 22, str(15 - i), 11, fill="#555"))

    def row(y, fields, name):
        out = [text(x0 - 12, y + 26, name, 12, "bold", "end")]
        col = 0
        for (fname, nbits) in fields:
            out.append(box(x0 + col * cw, y, nbits * cw, 40, FIELD_COLORS[fname], "#555"))
            out.append(text(x0 + (col + nbits / 2) * cw, y + 18, fname, 12, "bold"))
            out.append(text(x0 + (col + nbits / 2) * cw, y + 33, "%d bit%s" % (nbits, "s" if nbits > 1 else ""), 10, fill="#444"))
            col += nbits
        return "".join(out)

    b.append(row(32, [("opcode", 5), ("rd", 3), ("rs", 3), ("off5", 5)], "R, M, S, N"))
    b.append(row(84, [("opcode", 5), ("rd", 3), ("imm8 / addr8", 8)], "I, J"))
    bits = "0010100111000010"
    groups = [(0, 5, "00101 = 0x05 LDR"), (5, 3, "001 = R1"), (8, 3, "110 = FP"), (11, 5, "00010 = +2")]
    b.append(text(x0 - 12, 166, "0x29C2", 12, "bold", "end"))
    for i, ch in enumerate(bits):
        b.append(box(x0 + i * cw, 146, cw, 30, "#fff", "#999") + text(x0 + i * cw + cw / 2, 166, ch, 13, mono=True))
    for (s, n, t) in groups:
        b.append(line([(x0 + s * cw + 4, 184), (x0 + (s + n) * cw - 4, 184)], "#555", 1, None))
        b.append(text(x0 + (s + n / 2) * cw, 199, t, 11))
    b.append(text(x0 + 8 * cw, 222, "LDR R1, [FP+2]   (unused fields are 0; off5 is signed -16..+15)", 12, "bold"))
    return svg(810, 232, "".join(b))


def memory_map():
    """Figure: SIM16 memory map (4 segments)."""
    b = []
    segs = [(20, 120, "#dae8fc", "#6c8ebf", "TEXT", "0x00", "0x5F", "96 words", "machine code, loaded at 0x00"),
            (140, 50, "#d5e8d4", "#82b366", "DATA", "0x60", "0x7F", "32 words", "global variables (.data)"),
            (190, 50, "#fff", "#999", "HEAP", "0x80", "0x9F", "32 words", "dynamic memory (unused)"),
            (240, 120, "#f8cecc", "#b85450", "STACK", "0xA0", "0xFF", "96 words", "call frames, grows DOWN")]
    for (y, h, f, s, name, lo, hi, size, what) in segs:
        b.append(box(150, y, 180, h, f, s, name == "HEAP"))
        b.append(text(240, y + h / 2 + 5, name, 15, "bold"))
        b.append(text(140, y + 14, lo, 12, anchor="end", mono=True) + text(140, y + h - 4, hi, 12, anchor="end", mono=True))
        b.append(text(345, y + h / 2 - 3, size, 12, "bold", "start") + text(345, y + h / 2 + 13, what, 12, anchor="start"))
    b.append(line([(85, 352), (85, 262)], "#b85450", 2.5) + text(78, 312, "SP", 12, "bold", "end", "#b85450"))
    b.append(text(240, 382, "SP = FP = 0x100 when the stack is empty", 12, anchor="middle"))
    b.append(text(20, 30, "low", 11, anchor="start", fill="#666") + text(20, 355, "high", 11, anchor="start", fill="#666"))
    return svg(600, 392, "".join(b), "78%")


def program_layout():
    """A2 Figure: layout of the recursive program in memory."""
    b = []
    x, w = 170, 220
    rows = [  # y, h, fill, stroke, dashed, left-addr, text lines
        (20, 26, "#dae8fc", "#6c8ebf", False, "0x00", ["main: 0x00-0x06"]),
        (46, 40, "#dae8fc", "#6c8ebf", False, "0x07", ["fact: 0x07-0x10"]),
        (86, 22, "#dae8fc", "#6c8ebf", False, "0x11", ["base: 0x11-0x12"]),
        (108, 52, "#fff", "#6c8ebf", True, "0x13", ["unused 0x13-0x5F"]),
        (160, 22, "#d5e8d4", "#82b366", False, "0x60", ["n = 5"]),
        (182, 22, "#d5e8d4", "#82b366", False, "0x61", ["result (= 120 at end)"]),
        (204, 30, "#fff", "#82b366", True, "0x62", ["unused 0x62-0x7F"]),
        (234, 46, "#fff", "#999", True, "0x80", ["empty (no malloc)"]),
        (280, 70, "#fff", "#b85450", True, "0xA0", ["free 0xA0-0xF0", "", ""]),
        (350, 60, "#f8cecc", "#b85450", False, "0xF1", ["5 frames x 3 words", "at the deepest call"]),
    ]
    for (y, h, f, s, d, a, lines) in rows:
        b.append(box(x, y, w, h, f, s, d))
        b.append(text(x - 10, y + 14, a, 12, anchor="end", mono=True))
        for k, t in enumerate(lines):
            b.append(text(x + w / 2, y + h / 2 + 5 - 8 * (len(lines) - 1) + 16 * k, t, 12))
    b.append(text(x - 10, 406, "0xFF", 12, anchor="end", mono=True))
    braces = [(20, 160, "TEXT", "19 of 96 words used", "#6c8ebf"), (160, 234, "DATA", "2 of 32 words used", "#82b366"),
              (234, 280, "HEAP", "0x80-0x9F", "#777"), (280, 410, "STACK", "grows from 0xFF toward 0xA0", "#b85450")]
    for (y1, y2, name, sub, col) in braces:
        b.append(line([(x + w + 12, y1 + 2), (x + w + 22, y1 + 2), (x + w + 22, y2 - 2), (x + w + 12, y2 - 2)], col, 2, None))
        b.append(text(x + w + 32, (y1 + y2) / 2, name, 14, "bold", "start", col) + text(x + w + 32, (y1 + y2) / 2 + 16, sub, 11, anchor="start"))
    b.append(line([(x + w - 30, 344), (x + w - 30, 292)], "#b85450", 2.5))
    b.append(text(x + w - 40, 330, "CALL / PUSH", 10, anchor="end", fill="#b85450"))
    return svg(620, 420, "".join(b), "80%")


def stack_frame():
    """A2 Figure: one stack frame and the CALL / RET steps."""
    b = []
    x, w = 150, 200
    b.append(box(x, 30, w, 34, "#eee", "#999", True) + text(x + w / 2, 52, "caller's frame (higher)", 12, fill="#555"))
    cells = [(70, "FP + 2", "argument", "pushed by caller (PUSH)"),
             (110, "FP + 1", "return address", "pushed by CALL"),
             (150, "FP + 0", "saved FP (caller's FP)", "pushed by CALL")]
    for (y, off, what, who) in cells:
        b.append(box(x, y, w, 40, "#f8cecc", "#b85450") + text(x + w / 2, y + 25, what, 13, "bold"))
        b.append(text(x + w + 10, y + 25, off, 12, "bold", "start", mono=True) + text(x + w + 75, y + 25, who, 11, anchor="start", fill="#444"))
    b.append(box(x, 190, w, 34, "#fff", "#b85450", True) + text(x + w / 2, 212, "locals (optional, SUBI SP, k)", 11, fill="#555"))
    b.append(line([(60, 170), (x - 4, 170)], "#222", 2) + text(30, 174, "FP", 13, "bold"))
    b.append(line([(60, 236), (x - 4, 230)], "#222", 2) + text(30, 240, "SP", 13, "bold"))
    b.append(text(x + w / 2, 250, "SP = FP when there are no locals; stack grows down", 11, fill="#444"))
    b.append(text(x + w / 2, 18, "higher addresses", 10, fill="#777") + text(x + w / 2, 268, "lower addresses", 10, fill="#777"))
    steps_x = 600
    call = ["CALL f", "1. push PC (return address)", "2. push FP (saved FP)", "3. FP <- SP", "4. PC <- f"]
    ret = ["RET", "1. SP <- FP (drop locals)", "2. FP <- pop (caller's FP)", "3. PC <- pop (return address)"]
    caller = ["Caller", "PUSH arg", "CALL f", "ADDI SP, 1 (drop arg)"]
    for (yy, items, col) in [(34, call, "#9673a6"), (136, ret, "#9673a6"), (226, caller, "#6c8ebf")]:
        b.append(box(steps_x, yy - 18, 250, 22 + 17 * (len(items) - 1), "#fff", col, rx=6))
        b.append(text(steps_x + 12, yy, items[0], 13, "bold", "start", col))
        for k, t in enumerate(items[1:]):
            b.append(text(steps_x + 12, yy + 17 * (k + 1), t, 11.5, anchor="start", mono=True))
    return svg(880, 290, "".join(b))


def stack_growth(views):
    """A2 Figure: the stack after each of the 5 CALLs, side by side (values from the output)."""
    colors = ["#fde0dc", "#fbe9c6", "#dcedc8", "#d6e6f5", "#e8daf0"]
    b = []
    cw, rh, x0, y0 = 150, 22, 70, 60
    for a in range(0xF1, 0x100):
        b.append(text(x0 - 10, y0 + (a - 0xF1) * rh + 15, "%02X" % a, 11, anchor="end", mono=True))
    for k, v in enumerate(views):
        cx = x0 + k * (cw + 14)
        b.append(text(cx + cw / 2, 24, "after CALL #%d" % (k + 1), 12, "bold"))
        b.append(text(cx + cw / 2, 42, "%s   SP=FP=%s" % (v["name"], v["sp"]), 11))
        for a in range(0xF1, 0x100):
            y = y0 + (a - 0xF1) * rh
            if a in v["words"]:
                val, content, frame_no = v["words"][a]
                b.append(box(cx, y, cw, rh, colors[((0xFF - a) // 3) % 5], "#999"))
                b.append(text(cx + 30, y + 15, val, 11, mono=True) + text(cx + 62, y + 15, content, 10, anchor="start"))
            else:
                b.append(box(cx, y, cw, rh, "#fff", "#ddd"))
        sp_y = y0 + (int(v["sp"], 16) - 0xF1) * rh
        b.append(line([(cx + cw + 12, sp_y + 11), (cx + cw + 2, sp_y + 11)], "#b85450", 2))
    b.append(text(x0 + 2.5 * (cw + 14), y0 + 15 * rh + 22, "Each colour = one frame (saved FP, return addr, arg). Red arrow = SP = FP.", 11, fill="#444"))
    return svg(x0 + 5 * (cw + 14) + 10, y0 + 15 * rh + 34, "".join(b))


def parse_stack(view_lines):
    """From a four-segment view: {addr: (hex, contents, frame index)}, frame title, SP."""
    words, frame_no, name, sp = {}, -1, "", ""
    for l in view_lines:
        if l.startswith("[STACK]"):
            sp = l.split("SP=")[1].split()[0]
        if len(l) > 30 and l.startswith("  ") and l[2:4].strip() and l[4:8] == "    " and sp:
            addr = int(l[2:4], 16)
            val = l[8:12]
            content = l[21:43].strip()
            if "+-- " in l:
                frame_no += 1
                if not name:
                    name = l.split("+-- ")[1].split()[0]
            words[addr] = (val, content, frame_no)
    return {"words": words, "name": name, "sp": "0x" + sp}


# ---------------------------------------------------------------- Assignment 1

ENCODING = [
    ("0x00", "HALT", "N", "HALT", "stop execution", "HALT", "0000"),
    ("0x01", "NOP", "N", "NOP", "do nothing", "NOP", "0800"),
    ("0x02", "LDI", "I", "LDI rd, imm8", "rd &larr; imm8", "LDI R0, 1", "1001"),
    ("0x03", "LD", "I", "LD rd, addr8", "rd &larr; M[addr8]", "LD R1, 0x60", "1960"),
    ("0x04", "ST", "I", "ST rd, addr8", "M[addr8] &larr; rd", "ST R0, 0x61", "2061"),
    ("0x05", "LDR", "M", "LDR rd, [rs+off5]", "rd &larr; M[rs + off5]", "LDR R1, [FP+2]", "29C2"),
    ("0x06", "STR", "M", "STR rd, [rs+off5]", "M[rs + off5] &larr; rd", "STR R2, [SP-1]", "32FF"),
    ("0x07", "MOV", "R", "MOV rd, rs", "rd &larr; rs", "MOV R1, R2", "3940"),
    ("0x08", "ADD", "R", "ADD rd, rs", "rd &larr; rd + rs (Z,N)", "ADD R1, R2", "4140"),
    ("0x09", "SUB", "R", "SUB rd, rs", "rd &larr; rd &minus; rs (Z,N)", "SUB R1, R2", "4940"),
    ("0x0A", "MUL", "R", "MUL rd, rs", "rd &larr; rd &times; rs (Z,N)", "MUL R0, R1", "5020"),
    ("0x0B", "ADDI", "I", "ADDI rd, imm8", "rd &larr; rd + imm8 (Z,N)", "ADDI SP, 1", "5F01"),
    ("0x0C", "SUBI", "I", "SUBI rd, imm8", "rd &larr; rd &minus; imm8 (Z,N)", "SUBI R1, 1", "6101"),
    ("0x0D", "CMP", "R", "CMP rd, rs", "flags &larr; rd &minus; rs (Z,N)", "CMP R1, R2", "6940"),
    ("0x0E", "CMPI", "I", "CMPI rd, imm8", "flags &larr; rd &minus; imm8 (Z,N)", "CMPI R1, 2", "7102"),
    ("0x0F", "JMP", "J", "JMP addr8", "PC &larr; addr8", "JMP 0x04", "7804"),
    ("0x10", "JZ", "J", "JZ addr8", "if Z = 1: PC &larr; addr8", "JZ 0x10", "8010"),
    ("0x11", "JNZ", "J", "JNZ addr8", "if Z = 0: PC &larr; addr8", "JNZ 0x02", "8802"),
    ("0x12", "JN", "J", "JN addr8", "if N = 1: PC &larr; addr8", "JN 0x11", "9011"),
    ("0x13", "PUSH", "S", "PUSH rd", "SP &larr; SP&minus;1; M[SP] &larr; rd", "PUSH R1", "9900"),
    ("0x14", "POP", "S", "POP rd", "rd &larr; M[SP]; SP &larr; SP+1", "POP R2", "A200"),
    ("0x15", "CALL", "J", "CALL addr8", "push PC; push FP; FP &larr; SP; PC &larr; addr8", "CALL 0x07", "A807"),
    ("0x16", "RET", "N", "RET", "SP &larr; FP; FP &larr; pop; PC &larr; pop", "RET", "B000"),
    ("0x17", "OUT", "S", "OUT rd", "print rd", "OUT R0", "B800"),
    ("0x18&ndash;1F", "&mdash;", "&mdash;", "&mdash;", "reserved: invalid opcode error", "&mdash;", "&mdash;"),
]

REGISTERS = [
    ("R0", "R0", "return value of a function; general"),
    ("R1&ndash;R5", "R1&hellip;R5", "general purpose (caller-saved)"),
    ("R6", "FP", "frame pointer: base of the current stack frame"),
    ("R7", "SP", "stack pointer: last pushed word (empty = 0x100)"),
    ("&mdash;", "PC", "address of the next instruction"),
    ("&mdash;", "IR", "instruction register: last fetched word"),
    ("&mdash;", "Z, N", "flags: result is zero / bit 15 of result"),
]

POWER_MAP = [
    ("int base = 8;", "base:     .word 8", "60"),
    ("int exponent = 2;", "exponent: .word 2", "61"),
    ("int result;", "result:   .word 0", "62"),
    ("int r = 1;", "LDI R0, 1", "00"),
    ("for (i = 0; ...", "LDI R1, 0", "01"),
    ("(copy globals to registers)", "LD R2, exponent<br>LD R3, base", "02<br>03"),
    ("i &lt; exponent", "loop: CMP R1, R2<br>JN body<br>JMP done", "04<br>05<br>06"),
    ("r = r * base;", "body: MUL R0, R3", "07"),
    ("i++", "ADDI R1, 1", "08"),
    ("} (loop back)", "JMP loop", "09"),
    ("result = r;", "done: ST R0, result", "0A"),
    ("printf(\"%d\\n\", result);", "OUT R0", "0B"),
    ("return 0;", "HALT", "0C"),
]

RECURSIVE_MAP = [
    ("int n = 5;", "n:      .word 5", "60"),
    ("int result;", "result: .word 0", "61"),
    ("result = fact(n);  <i>(main)</i>", "main: LD R1, n<br>PUSH R1<br>CALL fact<br>ADDI SP, 1<br>ST R0, result", "00<br>01<br>02<br>03<br>04"),
    ("printf(\"%d\\n\", result);", "OUT R0", "05"),
    ("return 0;", "HALT", "06"),
    ("int fact(int k) {", "fact: LDR R1, [FP+2]   <i>(k)</i>", "07"),
    ("if (k &lt;= 1)", "CMPI R1, 2<br>JN base", "08<br>09"),
    ("return k * fact(k - 1);", "SUBI R1, 1<br>PUSH R1<br>CALL fact<br>ADDI SP, 1<br>LDR R1, [FP+2]<br>MUL R0, R1<br>RET", "0A<br>0B<br>0C<br>0D<br>0E<br>0F<br>10"),
    ("return 1;", "base: LDI R0, 1<br>RET", "11<br>12"),
]


def map_table(rep, rows, caption, extra=None):
    trs = [(code_html(c), asm_html(a), '<span class="mono">%s</span>' % ad) for c, a, ad in rows]
    return tab(trs, caption, ["C source", "SIM16 assembly", "Addr"], ["44%", "44%", "12%"], "map") + (extra or "")


def code_html(s):
    return '<span class="mono">%s</span>' % s


def asm_html(s):
    return '<span class="mono">%s</span>' % s


def trace_pages(rep, run_lines, prog_title, short, listing, map_page=None):
    blocks, summary = chunks(run_lines)
    load, steps, final = blocks[0], blocks[1:-1], blocks[-1]
    # listing + load view
    body = '<h1>%s</h1>' % prog_title
    if map_page:
        rep.add(body + map_page)
        body = '<h1>%s</h1>' % prog_title
    body += '<div class="cap-t">%s</div>' % rep.table("Assembly listing (<code>./sim16 list %s</code>)" % short[1])
    body += pre(listing, "dump small")
    if len(listing) + len(load) > 58:                   # too tall together: load view on its own page
        rep.add(body)
        body = '<h1>%s</h1>' % prog_title
    body += fig(pre(run_lines[:1] + load), rep.figure("%s loaded into memory (step 0)" % short[0]))
    rep.add(body)
    groups, cur, used = [], [], 0
    for b in steps:                                     # fill pages up to ~71 lines (block + 2 for spacing)
        if cur and used + len(b) + 2 > 71:
            groups.append(cur)
            cur, used = [], 0
        cur.append(b)
        used += len(b) + 2
    groups.append(cur)
    for group in groups:
        first = int(group[0][1].split()[1])
        last = int(group[-1][1].split()[1])
        inner = "".join(pre(b) for b in group)
        rep.add('<h1>%s</h1>' % prog_title + fig('<div class="steps">%s</div>' % inner,
                rep.figure("Execution trace, %s (steps %d&ndash;%d): memory after every instruction" % (short[0], first, last))))
    rep.add('<h1>%s</h1>' % prog_title + fig(pre(final + ["", summary]), rep.figure("%s final state (after HALT)" % short[0])))


def build_a1():
    rep = Report("Assignment 1: CPU Design")
    title_page(rep, "CPU Design", "a 16-bit CPU of our own design")
    contributions_page(rep)
    rep.add('<h1>CPU design</h1><p class="sub">16-bit words &middot; 256-word memory &middot; 8 registers &middot; 24 instructions</p>'
            + fig(cpu_block_diagram(), rep.figure("SIM16 CPU block diagram")))
    enc = [(r[0], "<b>%s</b>" % r[1], r[2], code_html(r[3]), r[4], code_html(r[5]), code_html(r[6])) for r in ENCODING]
    rep.add('<h1>Instruction encoding</h1>'
            + fig(bit_layout(), rep.figure("Instruction word layout (one 16-bit word per instruction)"))
            + tab(enc, rep.table("Instruction encoding (all 24 instructions)"),
                  ["Op", "Mnemonic", "Fmt", "Syntax", "Meaning", "Example", "Hex"],
                  ["8%", "9%", "5%", "19%", "30%", "18%", "7%"], "enc"))
    regs = [("<b>%s</b>" % a, code_html(b), c) for a, b, c in REGISTERS]
    fmts = [("N", code_html("HALT"), "HALT, NOP, RET"), ("R", code_html("ADD rd, rs"), "MOV, ADD, SUB, MUL, CMP"),
            ("I", code_html("LDI rd, imm8"), "LDI, LD, ST, ADDI, SUBI, CMPI"), ("M", code_html("LDR rd, [rs+off5]"), "LDR, STR"),
            ("J", code_html("JMP addr8"), "JMP, JZ, JNZ, JN, CALL"), ("S", code_html("PUSH rd"), "PUSH, POP, OUT")]
    rep.add('<h1>ISA summary</h1>'
            + '<div class="two"><div>' + tab(regs, rep.table("Registers"), ["Register", "Name", "Role"], ["22%", "18%", "60%"]) + '</div><div>'
            + tab(fmts, rep.table("Instruction formats"), ["Fmt", "Shape", "Instructions"], ["12%", "40%", "48%"], "plain") + '</div></div>'
            + fig(memory_map(), rep.figure("Memory map: the 4 segments")))
    run_page(rep, 1)
    trace_pages(rep, read_output("factorial_trace.txt"), "Program 1: 8! written in assembly",
                ("8!", "factorial"), sim16_list("factorial"))
    pmap = map_table(rep, POWER_MAP, rep.table("power.c next to its SIM16 assembly"),
                     tab([(code_html("r"), "R0"), (code_html("i"), "R1"), (code_html("exponent"), "R2 (copy of global)"),
                          (code_html("base"), "R3 (copy of global)")], rep.table("Register mapping"), ["C variable", "Register"],
                         ["30%", "70%"], "plain small-tab"))
    trace_pages(rep, read_output("power_trace.txt"), "Program 2: 8&sup2; written in C, translated to assembly",
                ("8&sup2;", "power"), sim16_list("power"), map_page=pmap)
    return rep


# ---------------------------------------------------------------- Assignment 2

def build_a2():
    rep = Report("Assignment 2: Program Layout & Execution")
    title_page(rep, "Program Layout &amp; Execution", "recursive factorial in four memory segments")
    contributions_page(rep)
    rep.add('<h1>Program layout</h1><p class="sub">Recursive factorial (fact(5) = 120) in SIM16 memory</p>'
            + fig(program_layout(), rep.figure("Program layout in memory: TEXT, DATA, HEAP, STACK")))
    rep.add('<h1>Stack frame and calling convention</h1><p class="sub">One argument on the stack, result in R0; CALL and RET build and remove the frame</p>'
            + fig(stack_frame(), rep.figure("Stack frame (3 words) with FP and SP; CALL and RET steps")))
    rep.add('<h1>C to assembly</h1>' + map_table(rep, RECURSIVE_MAP, rep.table("fact_recursive.c next to its SIM16 assembly"))
            + '<p class="note">Argument <code>k</code> is read from <code>[FP+2]</code>; R1 is reloaded after the recursive CALL (caller-saved).</p>')
    lines = read_output("recursive_calls.txt")
    blocks, summary = chunks(lines)
    load, calls, final = blocks[0], blocks[1:-1], blocks[-1]
    rep.add('<h1>Program loaded into memory</h1>' + fig(pre(load), rep.figure("Program loaded into memory (step 0): the 4 segments, code decoded")))
    calls = [[l for l in v if not l.startswith("OUTPUT>")] for v in calls]   # I/O left out of memory views
    for v in calls:
        while v and v[-1] == "":
            v.pop()
    for k, v in enumerate(calls, 1):
        t = v[1].strip()
        name = t.split(": ", 1)[1].split()[0]
        step = t.split()[-1]
        rep.add('<h1>Recursive call %d: %s</h1>' % (k, html.escape(name))
                + fig(pre(v), rep.figure("Memory after CALL #%d: %s (step %s)" % (k, name, step))))
    rep.add('<h1>End of the program</h1>' + fig(pre(final + ["", summary]), rep.figure("Final state (after HALT): stack empty, result = 120")))
    rep.add('<h1>Stack growth</h1><p class="sub">The stack right after each of the 5 recursive calls (taken from the views above)</p>'
            + fig(stack_growth([parse_stack(v) for v in calls]), rep.figure("Stack growth summary: fact(5) to fact(1)")))
    run_page(rep, 2)
    return rep


# ---------------------------------------------------------------- HTML + PDF

HEAD = """<!doctype html><html><head><meta charset="utf-8"><title>%s</title><style>
@page { size: 8.5in 11in; margin: 0; }
* { box-sizing: border-box; }
body { margin: 0; font-family: "Helvetica Neue", Helvetica, Arial, sans-serif; color: #1a1a1a; }
.page { width: 8.5in; height: 11in; padding: 0.55in 0.65in 0.7in; position: relative; overflow: hidden;
        break-after: page; page-break-after: always; }
.page:last-of-type { break-after: auto; page-break-after: auto; }
.pno { position: absolute; bottom: 0.32in; left: 0; right: 0; text-align: center; font-size: 10pt; color: #333; }
.hdr { display: flex; justify-content: space-between; font-size: 8.5pt; color: #666;
       border-bottom: 1px solid #ccc; padding-bottom: 4px; margin-bottom: 14px; }
h1 { font-size: 15pt; margin: 0 0 4px; color: #1f3b5c; }
.sub { font-size: 10pt; color: #555; margin: 0 0 12px; }
.note { font-size: 9.5pt; color: #444; margin: 8px 0 0; }
figure { margin: 10px 0 0; }
figcaption, .cap-t, caption { font-size: 9.5pt; font-weight: 700; color: #1f3b5c; }
figcaption { text-align: center; margin-top: 6px; }
caption, .cap-t { text-align: left; margin: 10px 0 5px; caption-side: top; }
pre.dump { font-family: Menlo, "Courier New", monospace; font-size: 8pt; line-height: 1.17; margin: 0 auto;
           white-space: pre; background: #fafafa; border: 1px solid #ddd; padding: 6px 8px; width: fit-content; }
pre.small { font-size: 8.5pt; line-height: 1.3; }
.steps pre.dump { margin-bottom: 6px; }
table { border-collapse: collapse; width: 100%%; font-size: 9pt; margin-bottom: 8px; }
th, td { border: 1px solid #b8c2cc; padding: 3px 6px; text-align: left; vertical-align: top; }
th { background: #e8eef5; }
table.enc td, table.enc th { font-size: 8.2pt; padding: 2px 5px; }
table.contrib td { font-size: 10pt; padding: 8px 8px; line-height: 1.35; }
table.map td { font-size: 9pt; }
table.small-tab { width: 60%%; }
code, .mono { font-family: Menlo, "Courier New", monospace; font-size: 8.3pt; }
.two { display: flex; gap: 18px; } .two > div { flex: 1; }
.title { text-align: center; padding-top: 2.1in; }
.course { font-size: 16pt; color: #555; letter-spacing: 0.5px; }
.assign { font-size: 30pt; font-weight: 700; color: #1f3b5c; margin: 18px 0 10px; }
.proj { font-size: 14pt; color: #444; margin-bottom: 1.3in; }
.team-label { font-size: 11pt; text-transform: uppercase; letter-spacing: 2px; color: #777; margin-bottom: 10px; }
.name { font-size: 20pt; font-weight: 600; margin: 8px 0; }
svg { display: block; margin: 0 auto; }
</style></head><body>"""

TAIL = """<script>
document.querySelectorAll('.page').forEach(function (p, i) {
  if (p.scrollHeight > p.clientHeight + 1 || p.scrollWidth > p.clientWidth + 1) {
    document.body.setAttribute('data-overflow', (document.body.getAttribute('data-overflow') || '') + ' ' + (i + 1));
  }
});
</script></body></html>"""


def write_pdf(rep, name):
    os.makedirs(BUILD, exist_ok=True)
    src = os.path.join(BUILD, name + ".html")
    pdf = os.path.join(ROOT, "report", name + ".pdf")
    with open(src, "w") as f:
        f.write(rep.html())
    dom = subprocess.run([CHROME, "--headless=new", "--disable-gpu", "--dump-dom", "file://" + src],
                         capture_output=True, text=True).stdout
    over = dom.split('data-overflow="')[1].split('"')[0].strip() if 'data-overflow="' in dom else ""
    subprocess.run([CHROME, "--headless=new", "--disable-gpu", "--no-pdf-header-footer",
                    "--print-to-pdf=" + pdf, "file://" + src], capture_output=True, check=True)
    print("%s: %d pages%s" % (os.path.relpath(pdf, ROOT), len(rep.pages),
                              ("   OVERFLOW on pages: " + over) if over else "   (no page overflows)"))
    return over


if __name__ == "__main__":
    bad = write_pdf(build_a1(), "Assignment1_CPU_Design")
    bad += write_pdf(build_a2(), "Assignment2_Program_Layout")
    sys.exit(1 if bad else 0)

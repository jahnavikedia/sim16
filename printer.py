"""printer.py -- SIM16 printer: all formatted output (grid, step blocks, segments, stack)

Owner   : Snehil
Contract: see SPEC.md sections 7, 6.4
Status  : TODO -- replace this stub with the real implementation.
"""
def format_memory_grid(snap, prev=None, obj=None, full=False):
    """Return the compact or full memory grid for a snapshot."""
    obj = obj or {"text": [], "data": []}
    # The snapshot supplies the live SP; use a private display hint for row selection.
    grid_obj = dict(obj)
    grid_obj["_sp_for_grid"] = snap["regs"][7]
    lines = ["MEMORY  (* = changed by this instruction)",
             " " * 12 + "".join("+%-5d" % i for i in range(8))]
    for name, row in _rows(grid_obj, full):
        cells = []
        for addr in range(row, row + 8):
            cells.append("%04X%s " % (
                snap["mem"][addr], _mark(_changed(snap, prev, "mem", addr))
            ))
        lines.append("%-6s %02X   " % (name, row) + "".join(cells))
    return "\n".join(line.rstrip() for line in lines)


def _disassemble(word, symbols):
    """Call the canonical assembler disassembler, with a pre-integration fallback."""
    try:
        from assembler import disassemble
        return disassemble(word, symbols)
    except (ImportError, AttributeError):
        return "%04X" % word


def format_step(prev, snap, obj, full=False):
    """Return one Assignment 1 instruction trace block."""
    event = snap.get("event")
    header = "STEP %-3d @%02X  %04X  %s" % (
        snap["step"], snap["exec_addr"], snap["ir"],
        _disassemble(snap["ir"], obj.get("symbols", {})),
    )
    lines = ["=" * 78, header, "-" * 78]
    if event and event.get("type") == "OUT":
        lines.append("OUTPUT> %d" % event["value"])
    lines.append(format_registers(snap, prev))
    lines.append(format_memory_grid(snap, prev, obj, full))
    return "\n".join(lines)


def _symbol_at(symbols, address):
    """Return the first symbol defined at address, preserving insertion order."""
    for name, value in symbols.items():
        if value == address:
            return name
    return ""


def _frame_info(snap, obj):
    """Build frame labels for stack words using the saved-FP chain."""
    mem = snap["mem"]
    sp = snap["regs"][7]
    fp = snap["regs"][6]
    symbols = obj.get("symbols", {})
    info = {}
    seen = set()
    current = fp
    while 0xA0 <= current <= 0xFD and current not in seen:
        seen.add(current)
        if current + 2 >= 0x100:
            break
        saved_fp, ret_addr, arg = mem[current], mem[current + 1], mem[current + 2]
        target = mem[ret_addr - 1] & 0xFF if 0 < ret_addr < 0x100 else -1
        # The CALL word is immediately before the return address; its target is low 8 bits.
        target_name = _symbol_at(symbols, target) or "function"
        info[current] = ("saved FP", "+-- %s(%d)" % (target_name, arg))
        info[current + 1] = ("return addr", "|")
        info[current + 2] = ("arg", "+--")
        if saved_fp == 0x100 or not 0xA0 <= saved_fp < current:
            break
        current = saved_fp
    return info


def format_stack(snap, obj, prev=None):
    """Return only the stack portion of the four-segment view."""
    sp = snap["regs"][7]
    fp = snap["regs"][6]
    lines = ["[STACK] 0xA0-0xFF   %d of 96 words used   SP=%02X  FP=%02X   (grows down)" %
             (0x100 - sp, sp, fp)]
    if sp == 0x100:
        lines.append("  (empty)")
        return "\n".join(lines)
    lines.append("  ADDR  HEX    DEC   CONTENTS              FRAME")
    frames = _frame_info(snap, obj)
    for addr in range(sp, 0x100):
        contents, frame = frames.get(addr, ("local", ""))
        if addr in frames and addr == fp:
            pointer = "SP, FP" if sp == fp else "FP"
            frame = "%-13s <- %s" % (frame, pointer)
        elif addr == sp:
            frame = "%-13s <- SP" % frame
        elif addr == fp:
            frame = "%-13s <- FP" % frame
        lines.append("  %02X    %04X%s  %3d   %-22s%s" %
                     (addr, snap["mem"][addr], _mark(_changed(snap, prev, "mem", addr)),
                      snap["mem"][addr], contents, frame))
    if sp > 0xA0:
        lines.append("  A0-%02X (free)" % (sp - 1))
    return "\n".join(lines)


def format_segments(snap, obj, title, prev=None):
    """Return the Assignment 2 four-segment memory view."""
    right = "step %d" % snap["step"]
    heading = " " + title + " " * (78 - 1 - len(title) - len(right)) + right
    symbols = obj.get("symbols", {})
    lines = ["=" * 78, heading, "=" * 78, format_registers(snap, prev)]
    lines.extend(["", "[TEXT]  0x00-0x5F   %d of 96 words used" % len(obj.get("text", [])),
                  "  ADDR  HEX    LABEL    INSTRUCTION"])
    for addr, word in enumerate(obj.get("text", [])):
        label = _symbol_at(symbols, addr)
        line = "  %02X    %04X%s  %-9s%-20s" % (
            addr, word, _mark(_changed(snap, prev, "mem", addr)),
            (label + ":") if label else "", _disassemble(word, symbols))
        if addr == snap["pc"]:
            line += "<- PC"
        lines.append(line.rstrip())
    lines.append("  %02X-5F (unused)" % len(obj.get("text", [])))
    lines.extend(["", "[DATA]  0x60-0x7F   %d of 32 words used" % len(obj.get("data", [])),
                  "  ADDR  HEX    NAME     VALUE"])
    for offset, word in enumerate(obj.get("data", [])):
        addr = 0x60 + offset
        name = _symbol_at(symbols, addr)
        lines.append("  %02X    %04X%s  %-9s%d" %
                     (addr, word, _mark(_changed(snap, prev, "mem", addr)), name, word))
    lines.append("  %02X-7F (unused)" % (0x60 + len(obj.get("data", []))))
    lines.extend(["", "[HEAP]  0x80-0x9F", "  (empty -- program uses no dynamic memory)", ""])
    lines.append(format_stack(snap, obj, prev))
    return "\n".join(lines)


def format_summary(snap):
    """Return the final one-line program summary."""
    output = ", ".join(str(value) for value in snap.get("output", []))
    return "Program halted after %d steps.  Output: %s" % (snap["step"], output)

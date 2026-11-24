"""cpu.py -- SIM16 CPU: registers, memory, fetch/decode/execute, stack, CALL/RET

Owner   : Jahnavi
Contract: SPEC.md sections 3.4-3.6 (ISA semantics, step order, stack/CALL/RET),
          5 (CPU state, snapshot, events) and 6.1 (CPU class interface).

The CPU works on raw 16-bit words only; it never imports the assembler.
Every error is raised as isa.CPUError, and every check is made before the
instruction changes any register, flag or memory word.
"""

import copy

from isa import (
    WORD_MASK, MEM_SIZE, NUM_REGS, REG_FP, REG_SP, STACK_BASE, STACK_TOP,
    OP_SHIFT, RD_SHIFT, RS_SHIFT, OP_MASK, REG_MASK, IMM8_MASK, OFF5_MASK,
    OPCODE_INFO, SETS_FLAGS, DEFAULT_MAX_STEPS, CPUError,
)


class CPU:
    """The SIM16 processor: 8 registers, PC, IR, Z/N flags and 256 words of memory."""

    def __init__(self):
        """Create a CPU in the reset state (SPEC 4.3 step 2)."""
        self.reset()

    def reset(self):
        """Return to the reset state: memory and R0-R5 zero, FP = SP = 0x100, PC = 0.

        Returns:
            None
        """
        self.mem = [0] * MEM_SIZE           # 256 words, all zero
        self.regs = [0] * NUM_REGS          # R0..R7
        self.regs[REG_FP] = STACK_TOP       # empty stack: FP = SP = 0x100
        self.regs[REG_SP] = STACK_TOP
        self.pc = 0
        self.ir = 0
        self.exec_addr = None
        self.flags = {"Z": 0, "N": 0}
        self.halted = False
        self.steps = 0
        self.output = []
        self.last_event = None

    # ---- convenience properties --------------------------------------------

    @property
    def sp(self):
        """The stack pointer (R7)."""
        return self.regs[REG_SP]

    @property
    def fp(self):
        """The frame pointer (R6)."""
        return self.regs[REG_FP]

    # ---- internal helpers --------------------------------------------------

    @staticmethod
    def _decode(word):
        """Split an instruction word into its fields (SPEC 3.3).

        Args:
            word: 16-bit instruction word.
        Returns:
            (opcode, rd, rs, off5, imm8), with off5 sign-extended to -16..+15.
        """
        opcode = (word >> OP_SHIFT) & OP_MASK
        rd = (word >> RD_SHIFT) & REG_MASK
        rs = (word >> RS_SHIFT) & REG_MASK
        off5 = word & OFF5_MASK
        if off5 & 0x10:                     # bit 4 set -> negative offset
            off5 -= 32
        imm8 = word & IMM8_MASK
        return opcode, rd, rs, off5, imm8

    @staticmethod
    def _check_addr(addr):
        """Raise CPUError unless addr is a valid memory address (0x00-0xFF).

        Args:
            addr: computed address.
        Raises:
            CPUError: "memory address out of range: 0x.."
        """
        if not 0 <= addr < MEM_SIZE:
            raise CPUError("memory address out of range: 0x%02X" % addr)

    def _set_reg(self, r, value):
        """Write value & 0xFFFF into register r."""
        self.regs[r] = value & WORD_MASK

    def _set_flags(self, result):
        """Update Z and N from the 16-bit value of result (SPEC 3.4 flag rules)."""
        v = result & WORD_MASK
        self.flags["Z"] = 1 if v == 0 else 0
        self.flags["N"] = (v >> 15) & 1

    def _push(self, value):
        """Push one word: SP <- SP - 1; M[SP] <- value (SPEC 3.6).

        Raises:
            CPUError: "stack overflow" if SP - 1 < 0xA0 (checked before any write).
        """
        new_sp = self.sp - 1
        if new_sp < STACK_BASE:
            raise CPUError("stack overflow")
        self._check_addr(new_sp)
        self.regs[REG_SP] = new_sp
        self.mem[new_sp] = value & WORD_MASK

    def _pop(self):
        """Pop one word: v <- M[SP]; SP <- SP + 1 (SPEC 3.6). The word is not erased.

        Returns:
            The popped value.
        Raises:
            CPUError: "stack underflow" if SP >= 0x100.
        """
        if self.sp >= STACK_TOP:
            raise CPUError("stack underflow")
        value = self.mem[self.sp]
        self.regs[REG_SP] = self.sp + 1
        return value

    # ---- execution ---------------------------------------------------------

    def step(self):
        """Fetch, decode and execute ONE instruction (SPEC 3.5).

        Returns:
            The event dict for this instruction (SPEC 5.3), or None.
        Raises:
            CPUError: halted CPU, PC out of range, invalid opcode, memory address
                out of range, stack overflow / underflow. On error nothing is changed.
        """
        if self.halted:
            raise CPUError("CPU is halted")
        if not 0 <= self.pc <= 0xFF:
            raise CPUError("PC out of range: 0x%02X" % self.pc)

        # Remember fetch state so it can be restored if execute fails.
        saved = (self.pc, self.ir, self.exec_addr)

        # FETCH
        self.exec_addr = self.pc
        self.ir = self.mem[self.pc]
        self.pc = self.pc + 1               # PC = address of next instruction

        # DECODE
        opcode, rd, rs, off5, imm8 = self._decode(self.ir)
        try:
            if opcode not in OPCODE_INFO:
                raise CPUError("invalid opcode 0x%02X at 0x%02X" % (opcode, self.exec_addr))
            name = OPCODE_INFO[opcode][0]
            event = self._execute(name, rd, rs, off5, imm8)
        except CPUError:
            self.pc, self.ir, self.exec_addr = saved
            raise

        self.steps += 1
        self.last_event = event
        return event

    def _execute(self, name, rd, rs, off5, imm8):
        """Execute one decoded instruction (SPEC 3.4). PC already points past it.

        Args:
            name: mnemonic; rd, rs: register numbers; off5: signed offset; imm8: 0..255.
        Returns:
            Event dict or None (SPEC 5.3).
        Raises:
            CPUError: raised before any state is changed.
        """
        regs = self.regs
        event = None

        # ---- HALT: stop execution ----
        if name == "HALT":
            self.halted = True
            event = {"type": "HALT"}

        # ---- NOP: do nothing ----
        elif name == "NOP":
            pass

        # ---- LDI rd, imm8: rd <- imm8 ----
        elif name == "LDI":
            self._set_reg(rd, imm8)

        # ---- LD rd, addr8: rd <- M[addr8] ----
        elif name == "LD":
            self._check_addr(imm8)
            self._set_reg(rd, self.mem[imm8])

        # ---- ST rd, addr8: M[addr8] <- rd (rd is the source) ----
        elif name == "ST":
            self._check_addr(imm8)
            self.mem[imm8] = regs[rd]

        # ---- LDR rd, [rs+off5]: rd <- M[rs + off5] ----
        elif name == "LDR":
            addr = regs[rs] + off5
            self._check_addr(addr)
            self._set_reg(rd, self.mem[addr])

        # ---- STR rd, [rs+off5]: M[rs + off5] <- rd (rd is the source) ----
        elif name == "STR":
            addr = regs[rs] + off5
            self._check_addr(addr)
            self.mem[addr] = regs[rd]

        # ---- MOV rd, rs: rd <- rs ----
        elif name == "MOV":
            self._set_reg(rd, regs[rs])

        # ---- ADD / SUB / MUL rd, rs: rd <- rd op rs, sets Z,N ----
        elif name == "ADD":
            self._set_reg(rd, regs[rd] + regs[rs])
        elif name == "SUB":
            self._set_reg(rd, regs[rd] - regs[rs])
        elif name == "MUL":
            self._set_reg(rd, regs[rd] * regs[rs])

        # ---- ADDI / SUBI rd, imm8: rd <- rd op imm8, sets Z,N ----
        elif name == "ADDI":
            self._set_reg(rd, regs[rd] + imm8)
        elif name == "SUBI":
            self._set_reg(rd, regs[rd] - imm8)

        # ---- CMP rd, rs / CMPI rd, imm8: flags from rd - x, result discarded ----
        elif name == "CMP":
            self._set_flags(regs[rd] - regs[rs])
        elif name == "CMPI":
            self._set_flags(regs[rd] - imm8)

        # ---- JMP / JZ / JNZ / JN addr8: (conditional) PC <- addr8 ----
        elif name == "JMP":
            self.pc = imm8
        elif name == "JZ":
            if self.flags["Z"] == 1:
                self.pc = imm8
        elif name == "JNZ":
            if self.flags["Z"] == 0:
                self.pc = imm8
        elif name == "JN":
            if self.flags["N"] == 1:
                self.pc = imm8

        # ---- PUSH rd: SP <- SP - 1; M[SP] <- rd ----
        elif name == "PUSH":
            self._push(regs[rd])

        # ---- POP rd: rd <- M[SP]; SP <- SP + 1 ----
        elif name == "POP":
            self._set_reg(rd, self._pop())

        # ---- CALL addr8: push PC; push FP; FP <- SP; PC <- addr8 (SPEC 3.6) ----
        elif name == "CALL":
            if self.sp - 2 < STACK_BASE:    # check both pushes before writing anything
                raise CPUError("stack overflow")
            self._check_addr(self.sp - 1)
            return_addr = self.pc
            self._push(return_addr)         # 1. return address
            self._push(regs[REG_FP])        # 2. caller's FP
            regs[REG_FP] = self.sp          # 3. FP points at the saved FP
            self.pc = imm8                  # 4. jump to the function
            arg_addr = self.fp + 2          # argument pushed by the caller
            arg = self.mem[arg_addr] if arg_addr < MEM_SIZE else None
            event = {"type": "CALL", "target": imm8, "return": return_addr, "arg": arg}

        # ---- RET: SP <- FP; FP <- pop; PC <- pop (SPEC 3.6) ----
        elif name == "RET":
            if self.fp + 1 >= STACK_TOP:    # no frame to return from
                raise CPUError("stack underflow")
            regs[REG_SP] = self.fp          # 1. discard locals
            regs[REG_FP] = self._pop()      # 2. restore caller's FP
            self.pc = self._pop()           # 3. return address
            event = {"type": "RET", "to": self.pc, "value": regs[0]}

        # ---- OUT rd: append rd to output (the "printf") ----
        elif name == "OUT":
            self.output.append(regs[rd])
            event = {"type": "OUT", "value": regs[rd]}

        # Arithmetic and compare results update Z and N
        if name in SETS_FLAGS and name not in ("CMP", "CMPI"):
            self._set_flags(regs[rd])
        return event

    def run(self, max_steps=DEFAULT_MAX_STEPS, on_step=None):
        """Step until the CPU halts.

        Args:
            max_steps: limit on instructions executed by this call.
            on_step: optional callback, called with self.snapshot() after each step.
        Returns:
            Number of instructions executed by this call.
        Raises:
            CPUError: any error from step(), or
                "exceeded max steps (N), infinite loop?" if the limit is reached.
        """
        count = 0
        while not self.halted:
            if count >= max_steps:
                raise CPUError("exceeded max steps (%d), infinite loop?" % max_steps)
            self.step()
            count += 1
            if on_step is not None:
                on_step(self.snapshot())
        return count

    def snapshot(self):
        """Return a deep copy of the CPU state in the shape of SPEC 5.2.

        Returns:
            dict with keys step, pc, ir, exec_addr, regs, flags, mem, halted,
            output, event. Later steps never change a returned snapshot.
        """
        return copy.deepcopy({
            "step": self.steps,
            "pc": self.pc,
            "ir": self.ir,
            "exec_addr": self.exec_addr,
            "regs": self.regs,
            "flags": self.flags,
            "mem": self.mem,
            "halted": self.halted,
            "output": self.output,
            "event": self.last_event,
        })

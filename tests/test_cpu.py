"""tests/test_cpu.py -- unittest tests for the CPU (every instruction + golden runs)

Owner   : Jahnavi
Contract: SPEC.md sections 3.4-3.8 (semantics, errors), 5 (snapshot/events),
          9 (golden programs) and 10.1.

Programs are written as raw hex words, so these tests do not need the assembler.
Run from the repo root:  python3 -m unittest discover -s tests -v
"""

import os
import sys
import unittest

# Make the repo root importable when run via "discover -s tests".
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from cpu import CPU                                    # noqa: E402
from isa import CPUError, REG_FP, REG_SP, STACK_TOP    # noqa: E402

HALT = 0x0000


def make_cpu(text, data=None):
    """Build a CPU with text at 0x00 and data at 0x60 (SP = FP = 0x100, PC = 0)."""
    cpu = CPU()
    for i, w in enumerate(text):
        cpu.mem[0x00 + i] = w
    for i, w in enumerate(data or []):
        cpu.mem[0x60 + i] = w
    return cpu


class TestReset(unittest.TestCase):
    """Reset state, properties and snapshots."""

    def test_reset_state(self):
        cpu = CPU()
        self.assertEqual(cpu.mem, [0] * 256)
        self.assertEqual(cpu.regs, [0, 0, 0, 0, 0, 0, 0x100, 0x100])
        self.assertEqual((cpu.pc, cpu.ir, cpu.exec_addr), (0, 0, None))
        self.assertEqual(cpu.flags, {"Z": 0, "N": 0})
        self.assertFalse(cpu.halted)
        self.assertEqual((cpu.steps, cpu.output, cpu.last_event), (0, [], None))
        self.assertEqual((cpu.sp, cpu.fp), (0x100, 0x100))

    def test_reset_clears_after_run(self):
        cpu = make_cpu([0x1005, 0xB800, HALT])          # LDI R0,5; OUT R0; HALT
        cpu.run()
        cpu.reset()
        self.assertEqual(cpu.snapshot(), CPU().snapshot())

    def test_snapshot_is_deep_copy(self):
        cpu = make_cpu([0x1005, 0xB800, HALT])          # LDI R0,5; OUT R0; HALT
        cpu.step()
        snap = cpu.snapshot()
        self.assertEqual(snap["step"], 1)
        self.assertEqual(snap["exec_addr"], 0)
        self.assertEqual(snap["ir"], 0x1005)
        self.assertEqual(set(snap), {"step", "pc", "ir", "exec_addr", "regs", "flags",
                                     "mem", "halted", "output", "event"})
        cpu.run()
        self.assertEqual(snap["regs"][0], 5)
        self.assertEqual(snap["output"], [])
        self.assertFalse(snap["halted"])
        snap["mem"][0] = 0xFFFF
        self.assertEqual(cpu.mem[0], 0x1005)


class TestInstructions(unittest.TestCase):
    """One test per instruction, including flag behaviour."""

    def test_halt(self):
        cpu = make_cpu([HALT])
        self.assertEqual(cpu.step(), {"type": "HALT"})
        self.assertTrue(cpu.halted)
        self.assertEqual((cpu.pc, cpu.steps), (1, 1))

    def test_nop(self):
        cpu = make_cpu([0x0800])
        before = cpu.snapshot()
        self.assertIsNone(cpu.step())
        self.assertEqual(cpu.pc, 1)
        self.assertEqual(cpu.regs, before["regs"])
        self.assertEqual(cpu.flags, before["flags"])

    def test_ldi(self):
        cpu = make_cpu([0x1001, 0x13FF])                # LDI R0,1; LDI R3,255
        cpu.step()
        cpu.step()
        self.assertEqual(cpu.regs[0], 1)
        self.assertEqual(cpu.regs[3], 255)
        self.assertEqual(cpu.flags, {"Z": 0, "N": 0})

    def test_ld(self):
        cpu = make_cpu([0x1960], [0x1234])              # LD R1, 0x60
        cpu.step()
        self.assertEqual(cpu.regs[1], 0x1234)

    def test_st(self):
        cpu = make_cpu([0x1007, 0x2061])                # LDI R0,7; ST R0, 0x61
        cpu.step()
        cpu.step()
        self.assertEqual(cpu.mem[0x61], 7)

    def test_ldr(self):
        cpu = make_cpu([0x29C2])                        # LDR R1, [FP+2]
        cpu.regs[REG_FP] = 0xFA
        cpu.mem[0xFC] = 0x0004
        cpu.step()
        self.assertEqual(cpu.regs[1], 4)

    def test_ldr_negative_offset(self):
        cpu = make_cpu([0x29DF])                        # LDR R1, [FP-1]
        cpu.regs[REG_FP] = 0xFA
        cpu.mem[0xF9] = 0x0042
        cpu.step()
        self.assertEqual(cpu.regs[1], 0x42)

    def test_str(self):
        cpu = make_cpu([0x1209, 0x32FF])                # LDI R2,9; STR R2, [SP-1]
        cpu.step()
        cpu.step()
        self.assertEqual(cpu.mem[0xFF], 9)
        self.assertEqual(cpu.sp, 0x100)                 # STR does not move SP

    def test_mov(self):
        cpu = make_cpu([0x1207, 0x3940])                # LDI R2,7; MOV R1, R2
        cpu.step()
        cpu.step()
        self.assertEqual(cpu.regs[1], 7)

    def test_add_and_flags(self):
        cpu = make_cpu([0x4140])                        # ADD R1, R2
        cpu.regs[1], cpu.regs[2] = 3, 4
        cpu.step()
        self.assertEqual(cpu.regs[1], 7)
        self.assertEqual(cpu.flags, {"Z": 0, "N": 0})
        # wrap-around to zero sets Z
        cpu = make_cpu([0x4140])
        cpu.regs[1], cpu.regs[2] = 0xFFFF, 1
        cpu.step()
        self.assertEqual(cpu.regs[1], 0)
        self.assertEqual(cpu.flags, {"Z": 1, "N": 0})

    def test_sub_and_flags(self):
        cpu = make_cpu([0x4940])                        # SUB R1, R2
        cpu.regs[1], cpu.regs[2] = 3, 5
        cpu.step()
        self.assertEqual(cpu.regs[1], 0xFFFE)           # -2 in two's complement
        self.assertEqual(cpu.flags, {"Z": 0, "N": 1})

    def test_mul_and_flags(self):
        cpu = make_cpu([0x5020])                        # MUL R0, R1
        cpu.regs[0], cpu.regs[1] = 5040, 8
        cpu.step()
        self.assertEqual(cpu.regs[0], 0x9D80)           # 40320 has bit 15 set
        self.assertEqual(cpu.flags, {"Z": 0, "N": 1})
        cpu = make_cpu([0x5020])
        cpu.regs[0], cpu.regs[1] = 0x100, 0x100         # 65536 wraps to 0
        cpu.step()
        self.assertEqual(cpu.regs[0], 0)
        self.assertEqual(cpu.flags, {"Z": 1, "N": 0})

    def test_addi(self):
        cpu = make_cpu([0x5F01])                        # ADDI SP, 1
        cpu.regs[REG_SP] = 0xFE
        cpu.step()
        self.assertEqual(cpu.sp, 0xFF)
        self.assertEqual(cpu.flags, {"Z": 0, "N": 0})

    def test_subi_and_flags(self):
        cpu = make_cpu([0x6101, 0x6101])                # SUBI R1, 1 (twice)
        cpu.regs[1] = 1
        cpu.step()
        self.assertEqual(cpu.regs[1], 0)
        self.assertEqual(cpu.flags, {"Z": 1, "N": 0})
        cpu.step()
        self.assertEqual(cpu.regs[1], 0xFFFF)
        self.assertEqual(cpu.flags, {"Z": 0, "N": 1})

    def test_cmp(self):
        cpu = make_cpu([0x6940, 0x6940])                # CMP R1, R2
        cpu.regs[1], cpu.regs[2] = 1, 2
        cpu.step()
        self.assertEqual((cpu.regs[1], cpu.regs[2]), (1, 2))   # result discarded
        self.assertEqual(cpu.flags, {"Z": 0, "N": 1})          # 1 < 2
        cpu.regs[1] = 2
        cpu.step()
        self.assertEqual(cpu.flags, {"Z": 1, "N": 0})          # equal

    def test_cmpi(self):
        cpu = make_cpu([0x7102])                        # CMPI R1, 2
        cpu.regs[1] = 5
        cpu.step()
        self.assertEqual(cpu.regs[1], 5)
        self.assertEqual(cpu.flags, {"Z": 0, "N": 0})

    def test_non_alu_leaves_flags(self):
        cpu = make_cpu([0x1000, 0x3940, 0x9900, 0xA200])  # LDI R0,0; MOV; PUSH; POP
        cpu.flags = {"Z": 1, "N": 1}
        for _ in range(4):
            cpu.step()
        self.assertEqual(cpu.flags, {"Z": 1, "N": 1})

    def test_jmp(self):
        cpu = make_cpu([0x7804])                        # JMP 0x04
        cpu.step()
        self.assertEqual(cpu.pc, 0x04)

    def test_jz(self):
        cpu = make_cpu([0x8010])                        # JZ 0x10
        cpu.step()
        self.assertEqual(cpu.pc, 1)                     # Z=0: not taken
        cpu = make_cpu([0x8010])
        cpu.flags["Z"] = 1
        cpu.step()
        self.assertEqual(cpu.pc, 0x10)                  # taken

    def test_jnz(self):
        cpu = make_cpu([0x8802])                        # JNZ 0x02
        cpu.step()
        self.assertEqual(cpu.pc, 0x02)                  # Z=0: taken
        cpu = make_cpu([0x8802])
        cpu.flags["Z"] = 1
        cpu.step()
        self.assertEqual(cpu.pc, 1)                     # not taken

    def test_jn(self):
        cpu = make_cpu([0x9011])                        # JN 0x11
        cpu.step()
        self.assertEqual(cpu.pc, 1)                     # N=0: not taken
        cpu = make_cpu([0x9011])
        cpu.flags["N"] = 1
        cpu.step()
        self.assertEqual(cpu.pc, 0x11)                  # taken

    def test_push(self):
        cpu = make_cpu([0x9900])                        # PUSH R1
        cpu.regs[1] = 0xBEEF
        cpu.step()
        self.assertEqual(cpu.sp, 0xFF)
        self.assertEqual(cpu.mem[0xFF], 0xBEEF)

    def test_pop(self):
        cpu = make_cpu([0x9900, 0xA200])                # PUSH R1; POP R2
        cpu.regs[1] = 42
        cpu.step()
        cpu.step()
        self.assertEqual(cpu.regs[2], 42)
        self.assertEqual(cpu.sp, 0x100)
        self.assertEqual(cpu.mem[0xFF], 42)             # popped value is not erased

    def test_call(self):
        # 00: PUSH R1   01: CALL 0x05
        cpu = make_cpu([0x9900, 0xA805])
        cpu.regs[1] = 3
        cpu.step()
        event = cpu.step()
        self.assertEqual(event, {"type": "CALL", "target": 0x05, "return": 0x02, "arg": 3})
        self.assertEqual(cpu.pc, 0x05)
        self.assertEqual((cpu.sp, cpu.fp), (0xFD, 0xFD))
        self.assertEqual(cpu.mem[0xFD:0x100], [0x100, 0x02, 3])
        self.assertEqual(cpu.last_event, event)

    def test_ret(self):
        # 00: PUSH R1  01: CALL 0x04  02: HALT  03: -  04: LDI R0,9  05: SUBI SP,2  06: RET
        cpu = make_cpu([0x9900, 0xA804, HALT, HALT, 0x1009, 0x6702, 0xB000])
        for _ in range(4):
            cpu.step()
        self.assertEqual(cpu.sp, 0xFB)                  # 2 locals below the frame
        event = cpu.step()
        self.assertEqual(event, {"type": "RET", "to": 0x02, "value": 9})
        self.assertEqual(cpu.pc, 0x02)
        self.assertEqual((cpu.fp, cpu.sp), (0x100, 0xFF))   # only the argument remains

    def test_out(self):
        cpu = make_cpu([0x1005, 0xB800])                # LDI R0,5; OUT R0
        cpu.step()
        self.assertEqual(cpu.step(), {"type": "OUT", "value": 5})
        self.assertEqual(cpu.output, [5])

    def test_other_instructions_have_no_event(self):
        cpu = make_cpu([0x1005])
        self.assertIsNone(cpu.step())
        self.assertIsNone(cpu.last_event)


class TestErrors(unittest.TestCase):
    """Every CPUError, and that a failing instruction changes nothing."""

    def assert_error(self, cpu, message):
        """Step cpu, expect CPUError(message), and check nothing changed."""
        before = cpu.snapshot()
        with self.assertRaises(CPUError) as ctx:
            cpu.step()
        self.assertEqual(str(ctx.exception), message)
        self.assertEqual(cpu.snapshot(), before)

    def test_stack_overflow_push(self):
        cpu = make_cpu([0x9900])
        cpu.regs[REG_SP] = 0xA0
        self.assert_error(cpu, "stack overflow")
        self.assertEqual(cpu.mem[0x9F], 0)              # heap never touched

    def test_stack_overflow_call(self):
        cpu = make_cpu([0xA805])
        cpu.regs[REG_SP] = 0xA1                         # room for only one word
        self.assert_error(cpu, "stack overflow")

    def test_stack_overflow_infinite_recursion(self):
        cpu = make_cpu([0xA800])                        # 00: CALL 0x00
        with self.assertRaises(CPUError) as ctx:
            cpu.run()
        self.assertEqual(str(ctx.exception), "stack overflow")
        self.assertEqual(cpu.steps, 48)                 # 96 words / 2 per CALL

    def test_stack_underflow_pop(self):
        self.assert_error(make_cpu([0xA200]), "stack underflow")

    def test_stack_underflow_ret(self):
        self.assert_error(make_cpu([0xB000]), "stack underflow")

    def test_invalid_opcode(self):
        cpu = make_cpu([0x0800] * 5 + [0xD000])         # opcode 0x1A at 0x05
        for _ in range(5):
            cpu.step()
        self.assert_error(cpu, "invalid opcode 0x1A at 0x05")

    def test_all_reserved_opcodes(self):
        for op in range(0x18, 0x20):
            self.assert_error(make_cpu([op << 11]), "invalid opcode 0x%02X at 0x00" % op)

    def test_memory_address_out_of_range_ldr(self):
        cpu = make_cpu([0x29C5])                        # LDR R1, [FP+5]
        self.assert_error(cpu, "memory address out of range: 0x105")

    def test_memory_address_out_of_range_str(self):
        cpu = make_cpu([0x31A1])                        # STR R1, [R5+1]
        cpu.regs[5] = 0xFF
        self.assert_error(cpu, "memory address out of range: 0x100")

    def test_pc_out_of_range(self):
        cpu = CPU()
        cpu.mem[0xFF] = 0x0800                          # NOP at the last address
        cpu.pc = 0xFF
        cpu.step()
        self.assert_error(cpu, "PC out of range: 0x100")

    def test_exceeded_max_steps(self):
        cpu = make_cpu([0x7800])                        # 00: JMP 0x00 (forever)
        with self.assertRaises(CPUError) as ctx:
            cpu.run(max_steps=100)
        self.assertEqual(str(ctx.exception), "exceeded max steps (100), infinite loop?")
        self.assertEqual(cpu.steps, 100)

    def test_default_max_steps(self):
        cpu = make_cpu([0x7800])
        with self.assertRaises(CPUError) as ctx:
            cpu.run()
        self.assertEqual(str(ctx.exception), "exceeded max steps (10000), infinite loop?")

    def test_step_halted_cpu(self):
        cpu = make_cpu([HALT])
        cpu.step()
        self.assert_error(cpu, "CPU is halted")


# ---- Golden programs (SPEC section 9) -------------------------------------------

FACTORIAL_TEXT = [0x1960, 0x1001, 0x5020, 0x6101, 0x8802, 0x2061, 0xB800, 0x0000]
FACTORIAL_DATA = [0x0008, 0x0000]

POWER_TEXT = [0x1001, 0x1100, 0x1A61, 0x1B60, 0x6940, 0x9007, 0x780A,
              0x5060, 0x5901, 0x7804, 0x2062, 0xB800, 0x0000]
POWER_DATA = [0x0008, 0x0002, 0x0000]

FACT_REC_TEXT = [0x1960, 0x9900, 0xA807, 0x5F01, 0x2061, 0xB800, 0x0000,
                 0x29C2, 0x7102, 0x9011, 0x6101, 0x9900, 0xA807, 0x5F01,
                 0x29C2, 0x5020, 0xB000, 0x1001, 0xB000]
FACT_REC_DATA = [0x0005, 0x0000]


class TestGolden(unittest.TestCase):
    """The three reference programs must reproduce SPEC section 9 exactly."""

    def test_factorial(self):
        cpu = make_cpu(FACTORIAL_TEXT, FACTORIAL_DATA)
        self.assertEqual(cpu.run(), 29)
        self.assertEqual(cpu.steps, 29)
        self.assertTrue(cpu.halted)
        self.assertEqual(cpu.output, [40320])
        self.assertEqual(cpu.mem[0x61], 0x9D80)
        self.assertEqual(cpu.regs[0], 0x9D80)
        self.assertEqual(cpu.regs[1], 0x0000)
        self.assertEqual((cpu.fp, cpu.sp), (STACK_TOP, STACK_TOP))
        self.assertEqual(cpu.flags, {"Z": 1, "N": 0})

    def test_power(self):
        cpu = make_cpu(POWER_TEXT, POWER_DATA)
        self.assertEqual(cpu.run(), 20)
        self.assertEqual(cpu.output, [64])
        self.assertEqual(cpu.mem[0x62], 0x0040)
        self.assertEqual(cpu.regs[:4], [0x0040, 0x0002, 0x0002, 0x0008])
        self.assertEqual((cpu.fp, cpu.sp), (STACK_TOP, STACK_TOP))
        self.assertEqual(cpu.flags, {"Z": 1, "N": 0})

    def test_fact_recursive(self):
        cpu = make_cpu(FACT_REC_TEXT, FACT_REC_DATA)
        calls = []

        def on_step(snap):
            if snap["event"] is not None and snap["event"]["type"] == "CALL":
                calls.append(snap)

        self.assertEqual(cpu.run(on_step=on_step), 52)
        self.assertEqual(cpu.output, [120])
        self.assertEqual(cpu.mem[0x61], 0x0078)
        self.assertEqual(cpu.regs[0], 0x0078)
        self.assertEqual(cpu.regs[1], 0x0005)
        self.assertEqual((cpu.fp, cpu.sp), (STACK_TOP, STACK_TOP))
        self.assertEqual(cpu.flags, {"Z": 0, "N": 0})

        # Stack right after each CALL (SPEC 9.3): (step, SP=FP, new frame words)
        expected = [
            (3,  0xFD, [0x0100, 0x0003, 0x0005]),
            (9,  0xFA, [0x00FD, 0x000D, 0x0004]),
            (15, 0xF7, [0x00FA, 0x000D, 0x0003]),
            (21, 0xF4, [0x00F7, 0x000D, 0x0002]),
            (27, 0xF1, [0x00F4, 0x000D, 0x0001]),
        ]
        self.assertEqual(len(calls), 5)
        stack = []                                      # words from SP to 0xFF
        for snap, (step, sp, frame) in zip(calls, expected):
            stack = frame + stack
            self.assertEqual(snap["step"], step)
            self.assertEqual(snap["regs"][REG_SP], sp)
            self.assertEqual(snap["regs"][REG_FP], sp)
            self.assertEqual(snap["mem"][sp:0x100], stack)
            self.assertEqual(snap["pc"], 0x07)
            self.assertEqual(snap["event"]["target"], 0x07)
            self.assertEqual(snap["event"]["arg"], frame[2])

        # Golden snapshot after CALL #2 (SPEC 5.4)
        call2 = calls[1]
        self.assertEqual(call2["ir"], 0xA807)
        self.assertEqual(call2["exec_addr"], 0x0C)
        self.assertEqual(call2["regs"], [0, 4, 0, 0, 0, 0, 0xFA, 0xFA])
        self.assertEqual(call2["flags"], {"Z": 0, "N": 0})
        self.assertEqual(call2["output"], [])
        self.assertFalse(call2["halted"])
        self.assertEqual(call2["event"], {"type": "CALL", "target": 0x07,
                                          "return": 0x0D, "arg": 4})

        # Stale values stay below SP after the stack empties
        self.assertEqual(cpu.mem[0xF1:0xF4], [0x00F4, 0x000D, 0x0001])


if __name__ == "__main__":
    unittest.main()

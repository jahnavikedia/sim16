"""loader.py -- SIM16 loader: places an object into CPU memory (4 segments)

Owner   : Snehil
Contract: see SPEC.md sections 4.3, 6.3
Status  : TODO -- replace this stub with the real implementation.
"""

from isa import (
    DATA_BASE,
    DATA_END,
    LoaderError,
    MEM_SIZE,
    TEXT_BASE,
    TEXT_END,
    WORD_MASK,
    STACK_TOP,
    REG_FP,
    REG_SP,
)


def _valid_words(values, limit, field):
    """Validate a word list and return a copy suitable for loading."""
    if not isinstance(values, list) or len(values) > limit:
        raise LoaderError("invalid %s size" % field)
    for word in values:
        if not isinstance(word, int) or not 0 <= word <= WORD_MASK:
            raise LoaderError("invalid word in %s" % field)
    return list(values)


def load(obj, cpu):
    """Validate and load an object dictionary into a CPU-like object.

    Args:
        obj: Object dictionary returned by assembler.assemble().
        cpu: Object exposing reset(), mem, regs and pc attributes.
    Returns:
        None.
    Raises:
        LoaderError: if the object violates the SIM16 object contract.
    """
    if not isinstance(obj, dict):
        raise LoaderError("object must be a dictionary")
    if obj.get("format") != "SIM16-OBJ":
        raise LoaderError("invalid object format")
    if obj.get("version") != 1:
        raise LoaderError("invalid object version")

    text = _valid_words(obj.get("text"), TEXT_END - TEXT_BASE, "text")
    data = _valid_words(obj.get("data"), DATA_END - DATA_BASE, "data")
    entry = obj.get("entry")
    if not isinstance(entry, int) or not TEXT_BASE <= entry < TEXT_BASE + len(text):
        raise LoaderError("entry point out of range")

    cpu.reset()
    for offset, word in enumerate(text):
        cpu.mem[TEXT_BASE + offset] = word
    for offset, word in enumerate(data):
        cpu.mem[DATA_BASE + offset] = word
    cpu.pc = entry
    cpu.regs[REG_FP] = STACK_TOP
    cpu.regs[REG_SP] = STACK_TOP
    return None


def load_file(path, cpu):
    """Assemble the file at path and load it into cpu."""
    import assembler

    return_obj = assembler.assemble_file(path)
    load(return_obj, cpu)
    return return_obj

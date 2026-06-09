# pylint: disable=missing-module-docstring, missing-function-docstring, invalid-name

"""
Flatten a guarded C/C++ header by inlining local includes and collecting
system includes together with the #if context active at each inclusion.

Conditionals of the #if family are evaluated against a table of known tokens
(macros and __has_* operators, with optional numeric values). When a condition
is fully decidable it is resolved (live branch unwrapped, dead branches
dropped); when it isn't, the conditional is left in place. See expand_header().
"""

import re
import sys
from pathlib import Path


# Sentinel marking a token as *known to be undefined*. This is distinct from a
# token being absent from the table: absent means "indeterminate" (leave the
# conditional verbatim), whereas UNDEFINED means "this macro is definitely not
# defined" -- so defined(X) is 0, a bare X evaluates to 0 (as in a real #if),
# and #ifndef X becomes a taken branch that gets unwrapped.
UNDEFINED = object()


# --------------------------------------------------------------------------- #
# Small parsing helpers
# --------------------------------------------------------------------------- #

_DIRECTIVE_RE = re.compile(r"\s*#\s*([A-Za-z]+)(.*)$")
_IDENT_RE = re.compile(r"\s*([A-Za-z_]\w*)")
_ANGLE_RE = re.compile(r"<([^>]*)>")
_QUOTE_RE = re.compile(r'"([^"]*)"')


def _parse_directive(line):
    """Return (name, remainder) for a preprocessor line, else None."""
    m = _DIRECTIVE_RE.match(line)
    if not m:
        return None
    return m.group(1), m.group(2)


def _first_ident(s):
    m = _IDENT_RE.match(s)
    return m.group(1) if m else None


def _clean_expr(rem):
    """Strip trailing // and simple /* */ comments from an #if/#elif expression."""
    rem = re.sub(r"//.*$", "", rem)
    rem = re.sub(r"/\*.*?\*/", "", rem)
    return rem.strip()


def _extract_include(rem):
    """Return ('angle', path) or ('quote', path) for an #include, else None."""
    m = _ANGLE_RE.search(rem)
    if m:
        return "angle", m.group(1)
    m = _QUOTE_RE.search(rem)
    if m:
        return "quote", m.group(1)
    return None


# --------------------------------------------------------------------------- #
# #if expression evaluator (three-valued: int value, or None = indeterminate)
# --------------------------------------------------------------------------- #

_HAS_RE = re.compile(r"__has_\w+\s*\([^)]*\)")
_NUM_RE = re.compile(r"\d[\w']*")
_ID_RE = re.compile(r"[A-Za-z_]\w*")


def _parse_int(tok):
    """Parse a C integer literal to int, or None if it isn't a plain integer."""
    s = tok.replace("'", "").rstrip("uUlL")
    try:
        if s[:2] in ("0x", "0X"):
            return int(s, 16)
        if s[:2] in ("0b", "0B"):
            return int(s, 2)
        if len(s) > 1 and s[0] == "0":
            return int(s, 8)
        return int(s, 10)
    except ValueError:
        return None


def _tokenize_expr(expr):
    """Tokenize a preprocessor condition into (kind, text) tuples.

    Raises ValueError on an unexpected character (caller treats as
    indeterminate / pass-through).
    """
    toks = []
    i, n = 0, len(expr)
    while i < n:
        c = expr[i]
        if c.isspace():
            i += 1
            continue
        m = _HAS_RE.match(expr, i)
        if m:
            toks.append(("has", re.sub(r"\s+", "", m.group(0))))
            i = m.end()
            continue
        if c.isalpha() or c == "_":
            m = _ID_RE.match(expr, i)
            toks.append(("id", m.group(0)))
            i = m.end()
            continue
        if c.isdigit():
            m = _NUM_RE.match(expr, i)
            toks.append(("num", m.group(0)))
            i = m.end()
            continue
        if c in "'\"":
            q, j = c, i + 1
            while j < n and expr[j] != q:
                j += 2 if expr[j] == "\\" else 1
            toks.append(("lit", expr[i:j + 1]))   # char/string -> indeterminate
            i = j + 1
            continue
        two = expr[i:i + 2]
        if two in ("<<", ">>", "<=", ">=", "==", "!=", "&&", "||"):
            toks.append(("op", two))
            i += 2
            continue
        if c in "+-*/%&|^~!<>()?:":
            toks.append(("op", c))
            i += 1
            continue
        raise ValueError(f"unexpected char {c!r} in #if expression")
    return toks


def _trunc_div(a, b):
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


def _eval_pp_expr(expr, tokens):
    """Evaluate a preprocessor condition against `tokens`.

    Returns an int (the value) when fully decidable, or None when the
    expression involves any indeterminate token / unsupported construct
    (in which case the caller leaves the conditional in place). Logical
    && / || / ?: short-circuit, so a determinate operand can still decide
    the result (e.g. `1 || UNKNOWN` -> 1).
    """
    try:
        toks = _tokenize_expr(expr)
    except ValueError:
        return None
    pos = [0]

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else (None, None)

    def advance():
        t = toks[pos[0]]
        pos[0] += 1
        return t

    def expect(text):
        if peek()[1] != text:
            raise _Bail()
        advance()

    def lookup(name):
        if name not in tokens:
            return None
        val = tokens[name]
        if val is UNDEFINED:
            return 0
        return 0 if val is None else int(val)

    def primary():
        ty, tx = peek()
        if tx == "(":
            advance()
            v = ternary()
            expect(")")
            return v
        if ty == "num":
            advance()
            return _parse_int(tx)
        if ty == "lit":
            advance()
            return None
        if ty == "has":
            advance()
            return lookup(tx)
        if ty == "id":
            if tx == "defined":
                advance()
                if peek()[1] == "(":
                    advance()
                    nm = advance()
                    if nm[0] != "id":
                        raise _Bail()
                    expect(")")
                else:
                    nm = advance()
                    if nm[0] != "id":
                        raise _Bail()
                if nm[1] not in tokens:
                    return None
                return 0 if tokens[nm[1]] is UNDEFINED else 1
            advance()
            if tx == "true":
                return 1
            if tx == "false":
                return 0
            return lookup(tx)
        raise _Bail()

    def unary():
        ty, tx = peek()
        if ty == "op" and tx in ("!", "~", "-", "+"):
            advance()
            v = unary()
            if v is None:
                return None
            if tx == "!":
                return 0 if v else 1
            if tx == "~":
                return ~v
            if tx == "-":
                return -v
            return +v
        return primary()

    def _bin_level(sub, ops):
        def parse():
            v = sub()
            while peek()[0] == "op" and peek()[1] in ops:
                op = advance()[1]
                rhs = sub()
                v = _apply(op, v, rhs)
            return v
        return parse

    def _apply(op, a, b):
        if op == "&&":
            if (a is not None and a == 0) or (b is not None and b == 0):
                return 0
            return None if (a is None or b is None) else (1 if a and b else 0)
        if op == "||":
            if (a is not None and a != 0) or (b is not None and b != 0):
                return 1
            return None if (a is None or b is None) else 0
        if a is None or b is None:
            return None
        if op == "|":
            return a | b
        if op == "^":
            return a ^ b
        if op == "&":
            return a & b
        if op == "==":
            return 1 if a == b else 0
        if op == "!=":
            return 1 if a != b else 0
        if op == "<":
            return 1 if a < b else 0
        if op == ">":
            return 1 if a > b else 0
        if op == "<=":
            return 1 if a <= b else 0
        if op == ">=":
            return 1 if a >= b else 0
        if op == "<<":
            return a << b
        if op == ">>":
            return a >> b
        if op == "+":
            return a + b
        if op == "-":
            return a - b
        if op == "*":
            return a * b
        if op == "/":
            return None if b == 0 else _trunc_div(a, b)
        if op == "%":
            return None if b == 0 else a - b * _trunc_div(a, b)
        raise _Bail()

    mul = _bin_level(unary, ("*", "/", "%"))
    add = _bin_level(mul, ("+", "-"))
    shift = _bin_level(add, ("<<", ">>"))
    rel = _bin_level(shift, ("<", ">", "<=", ">="))
    eq = _bin_level(rel, ("==", "!="))
    band = _bin_level(eq, ("&",))
    bxor = _bin_level(band, ("^",))
    bor = _bin_level(bxor, ("|",))
    land = _bin_level(bor, ("&&",))
    lor = _bin_level(land, ("||",))

    def ternary():
        cond = lor()
        if peek()[1] == "?":
            advance()
            then_v = ternary()
            expect(":")
            else_v = ternary()
            if cond is None:
                return None
            return then_v if cond != 0 else else_v
        return cond

    try:
        result = ternary()
    except _Bail:
        return None
    if pos[0] != len(toks):   # leftover tokens -> something we didn't model
        return None
    return result


class _Bail(Exception):
    pass


def _analyze_group(lines, start_idx, if_cond, tokens):
    """Decide what to do with a conditional group opened at `start_idx`.

    Returns "keep" (leave the whole group verbatim) or an int index of the
    single live branch (#if = 0, each #elif/#else the next index; -1 if none
    is live). A group is only resolved when its outcome is fully decidable;
    if an indeterminate condition is *reached*, the whole group is kept.
    """
    branch_conditions = [if_cond]
    depth = 0
    for i in range(start_idx, len(lines)):
        d = _parse_directive(lines[i])
        if not d:
            continue
        nm, rem = d
        if nm in ("if", "ifdef", "ifndef"):
            depth += 1
        elif nm == "endif":
            if depth == 0:
                break
            depth -= 1
        elif depth == 0 and nm == "elif":
            branch_conditions.append(_clean_expr(rem))
        elif depth == 0 and nm == "elifdef":
            branch_conditions.append(f"defined({_first_ident(rem)})")
        elif depth == 0 and nm == "elifndef":
            branch_conditions.append(f"!defined({_first_ident(rem)})")
        elif depth == 0 and nm == "else":
            branch_conditions.append(None)

    for idx, cond in enumerate(branch_conditions):
        if cond is None:           # #else, reached only if all prior were false
            return idx
        v = _eval_pp_expr(cond, tokens)
        if v is None:
            return "keep"          # indeterminate reached -> leave whole group
        if v != 0:
            return idx             # first decidably-true branch wins
    return -1                      # all decidably false, no #else -> nothing live


# --------------------------------------------------------------------------- #
# Condition stack
# --------------------------------------------------------------------------- #

class _Frame:
    """An open conditional block. kinds:

    "keep"     -- left in the output (condition undecidable). `branches` /
                  `current` track its condition for system-header context.
                  Always live (both branches emitted).
    "resolved" -- statically decided. Directives dropped; only the branch at
                  `live_index` survives; contributes nothing to context.
    "dead"     -- placeholder pushed inside an already-dead branch (tracking).
    """

    __slots__ = ("kind", "branches", "current", "live_index", "branch_index")

    def __init__(self, kind):
        self.kind = kind
        self.branches = []
        self.current = None
        self.live_index = -1
        self.branch_index = 0

    @classmethod
    def keep(cls, cond):
        f = cls("keep")
        f.current = cond
        return f

    @classmethod
    def resolved(cls, live_index):
        f = cls("resolved")
        f.live_index = live_index
        return f

    @classmethod
    def dead(cls):
        return cls("dead")

    def is_live(self):
        if self.kind == "keep":
            return True
        if self.kind == "dead":
            return False
        return self.branch_index == self.live_index

    def active(self):
        if not self.branches and self.current is not None:
            return self.current
        parts = [f"!({c})" for c in self.branches]
        if self.current is not None:
            parts.append(f"({self.current})")
        return " && ".join(parts)


def _default_contexts_match(recorded_ctx, new_ctx):
    """Whether two #if-context stacks count as the same (swap to customise)."""
    return recorded_ctx == new_ctx


# --------------------------------------------------------------------------- #
# Guard stripping
# --------------------------------------------------------------------------- #

def _strip_guard(lines, path):
    """Return the interior lines of a header, with its include guard removed."""
    n = len(lines)

    guard_start = guard_name = None
    for i, line in enumerate(lines):
        d = _parse_directive(line)
        if d and d[0] == "ifndef":
            guard_start = i
            guard_name = _first_ident(d[1])
            break
    if guard_start is None or not guard_name:
        raise ValueError(f"No include guard (#ifndef NAME) found in {path}")

    define_idx = None
    for j in range(guard_start + 1, n):
        d = _parse_directive(lines[j])
        if d is None:
            continue
        if d[0] == "define" and _first_ident(d[1]) == guard_name:
            define_idx = j
        break
    if define_idx is None:
        raise ValueError(f"Expected '#define {guard_name}' after guard in {path}")

    depth, end_idx = 1, None
    for k in range(guard_start + 1, n):
        d = _parse_directive(lines[k])
        if d is None:
            continue
        name = d[0]
        if name in ("if", "ifdef", "ifndef"):
            depth += 1
        elif name == "endif":
            depth -= 1
            if depth == 0:
                end_idx = k
                break
    if end_idx is None:
        raise ValueError(f"Unterminated include guard in {path}")

    return lines[define_idx + 1:end_idx]


# --------------------------------------------------------------------------- #
# Output tidying
# --------------------------------------------------------------------------- #

def _tidy_blanks(lines):
    """Collapse runs of blank lines to one and trim leading/trailing blanks."""
    out = []
    for line in lines:
        if line.strip() == "":
            if out and out[-1] == "":
                continue
            out.append("")
        else:
            out.append(line)
    while out and out[0] == "":
        out.pop(0)
    while out and out[-1] == "":
        out.pop()
    return out


# --------------------------------------------------------------------------- #
# File state for the iterative traversal
# --------------------------------------------------------------------------- #

class _FileState:
    def __init__(self, path, lines):
        self.path = Path(path)
        self.dir = self.path.resolve().parent
        self.lines = lines
        self.idx = 0
        self.cond_depth_at_entry = 0


# --------------------------------------------------------------------------- #
# Entry point
# --------------------------------------------------------------------------- #

def expand_header(filepath, local_include_prefix, prefix_base_dir,
                  contexts_match=None, tokens=None):
    """Flatten a guarded header.

    Args:
        filepath:             path to the root .h file.
        local_include_prefix: angle includes whose path starts with this are
                              treated as local '#include "..."' (recursed into).
        prefix_base_dir:      base dir to resolve those prefixed angle includes
                              (prefix is NOT stripped).
        contexts_match:       optional fn(recorded_ctx, new_ctx) -> bool for
                              comparing #if contexts of a repeated system header.
        tokens:               dict mapping known tokens to optional numeric
                              values, e.g. {'__cplusplus': 202002,
                              '__has_include(<format>)': 1, '_WIN32': None,
                              'SQLITEPP_OMIT_MUTEX': UNDEFINED}.
                              A numeric value means "defined, with that value".
                              None means "defined, value 0". The UNDEFINED
                              sentinel means "known to be undefined" (defined(X)
                              is 0, bare X is 0, #ifndef X is taken). A token
                              absent from the dict is indeterminate -- its
                              conditional is left verbatim. __has_* keys must be
                              whitespace-free, e.g.
                              '__has_cpp_attribute(nodiscard)'.

    Returns:
        (output_lines, system_headers)

    Conditional handling: #if / #ifdef / #ifndef / #elif / #elifdef /
    #elifndef expressions are evaluated against `tokens` using normal
    preprocessor arithmetic and logic (with `defined`, `__has_*`, and
    short-circuit && / || / ?:). A fully-decidable group is resolved (live
    branch unwrapped, dead branches and directives dropped); any group whose
    outcome depends on an indeterminate token is left verbatim. Nothing errors
    out. Resolved conditions do not contribute to a system header's context.
    """
    if contexts_match is None:
        contexts_match = _default_contexts_match
    if tokens is None:
        tokens = {}

    output = []
    system_headers = {}
    cond_frames = []
    file_stack = []
    seen = set()

    def push_file(path):
        path = Path(path)
        real = path.resolve()
        if real in seen:
            return
        if not path.is_file():
            raise FileNotFoundError(f"Header not found: {path}")
        seen.add(real)
        raw = path.read_text().splitlines()
        st = _FileState(path, _strip_guard(raw, path))
        st.cond_depth_at_entry = len(cond_frames)
        file_stack.append(st)

    def scream_and_exit(header, old_ctx, new_ctx, where):
        sys.stderr.write(
            f"ERROR: system header <{header}> included under conflicting #if "
            f"contexts.\n  in:        {where}\n  previously: {old_ctx}\n"
            f"  now:        {new_ctx}\n"
        )
        sys.exit(1)

    OPEN = ("if", "ifdef", "ifndef")
    ELIF = ("elif", "elifdef", "elifndef")

    def cond_for(name, rem):
        if name in ("if", "elif"):
            return _clean_expr(rem)
        if name in ("ifdef", "elifdef"):
            return f"defined({_first_ident(rem)})"
        return f"!defined({_first_ident(rem)})"   # ifndef / elifndef

    push_file(filepath)

    while file_stack:
        st = file_stack[-1]

        if st.idx >= len(st.lines):
            if len(cond_frames) != st.cond_depth_at_entry:
                raise RuntimeError(f"Unbalanced #if/#endif in {st.path}")
            file_stack.pop()
            continue

        line = st.lines[st.idx]
        st.idx += 1

        d = _parse_directive(line)
        dead_region = any(not f.is_live() for f in cond_frames)

        if d is None:
            if not dead_region:
                output.append(line)
            continue
        name, rem = d

        if name in OPEN:
            if dead_region:
                cond_frames.append(_Frame.dead())
                continue
            cond = cond_for(name, rem)
            outcome = _analyze_group(st.lines, st.idx, cond, tokens)
            if outcome == "keep":
                cond_frames.append(_Frame.keep(cond))
                output.append(line)
            else:
                cond_frames.append(_Frame.resolved(outcome))
            continue

        if name in ELIF:
            if not cond_frames:
                raise RuntimeError(f"#{name} without #if in {st.path}")
            top = cond_frames[-1]
            if top.kind == "keep":
                if top.current is None:
                    raise RuntimeError(f"#{name} after #else in {st.path}")
                top.branches.append(top.current)
                top.current = cond_for(name, rem)
                if not dead_region:
                    output.append(line)
            elif top.kind == "resolved":
                top.branch_index += 1
            continue

        if name == "else":
            if not cond_frames:
                raise RuntimeError(f"#else without #if in {st.path}")
            top = cond_frames[-1]
            if top.kind == "keep":
                if top.current is None:
                    raise RuntimeError(f"#else after #else in {st.path}")
                top.branches.append(top.current)
                top.current = None
                if not dead_region:
                    output.append(line)
            elif top.kind == "resolved":
                top.branch_index += 1
            continue

        if name == "endif":
            if len(cond_frames) <= st.cond_depth_at_entry:
                raise RuntimeError(f"#endif without matching #if in {st.path}")
            top = cond_frames[-1]
            ancestor_dead = any(not f.is_live() for f in cond_frames[:-1])
            emit = (top.kind == "keep") and not ancestor_dead
            cond_frames.pop()
            if emit:
                output.append(line)
            continue

        if name == "include":
            if dead_region:
                continue
            inc = _extract_include(rem)
            if inc is None:
                output.append(line)
                continue
            kind, path = inc
            if kind == "quote":
                push_file(st.dir / path)
            elif path.startswith(local_include_prefix):
                push_file(Path(prefix_base_dir) / path)
            else:
                ctx = [f.active() for f in cond_frames if f.kind == "keep"]
                if path in system_headers:
                    if not contexts_match(system_headers[path], ctx):
                        scream_and_exit(path, system_headers[path], ctx, st.path)
                else:
                    system_headers[path] = ctx
            continue

        # #define, #undef, #pragma, #error, plain code, etc. -> keep if live.
        if not dead_region:
            output.append(line)

    return _tidy_blanks(output), system_headers

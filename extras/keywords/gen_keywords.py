"""Regenerate the KEYWORD1/KEYWORD2 sections of RadioLib's keywords.txt by
parsing public classes, structs, enums, typedefs, extern globals and public
member functions from the library headers. The LITERAL1 section is kept."""
import os
import re
import sys

ROOT = sys.argv[1]  # path to RadioLib library root
SRC = os.path.join(ROOT, "src")
KW = os.path.join(ROOT, "keywords.txt")

SKIP_DIRS = {"firmware", "patches"}
SKIP_FILES_RE = re.compile(r"_(commands|registers|firmware)\.h$|^BuildOpt(User)?\.h$")
NOT_FUNC = {"if", "while", "for", "switch", "return", "sizeof", "operator",
            "void", "int", "char", "bool", "float", "double", "unsigned", "signed",
            "const", "static", "virtual", "inline", "decltype", "alignas", "static_assert"}


def strip_source(text):
    # remove comments and string/char literals
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            seg = text[i:n if j < 0 else j + 2]
            out.append("\n" * seg.count("\n") or " ")
            i = n if j < 0 else j + 2
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(c + c)
            i = j + 1
        else:
            out.append(c)
            i += 1
    text = "".join(out)
    # remove preprocessor directives (including continued lines)
    lines, res, cont = text.split("\n"), [], False
    for line in lines:
        if cont or line.lstrip().startswith("#"):
            cont = line.rstrip().endswith("\\")
            res.append("")
        else:
            res.append(line)
    return "\n".join(res)


class Scope:
    def __init__(self, kind, name=None, access="public", public=True):
        self.kind = kind          # file, namespace, class, enum, block
        self.name = name
        self.access = access
        self.public = public      # is this scope itself publicly reachable
        self.pending_typedef = False


def parse_header(path, types, funcs, globals_):
    text = strip_source(open(path, encoding="utf-8", errors="replace").read())
    # RADIOLIB_TYPE_ALIAS(type, alias) expands to a typedef/using declaration
    def alias(m):
        types.append(m.group(1))
        return ";"
    text = re.sub(r"\bRADIOLIB_TYPE_ALIAS\(\s*\w+\s*,\s*(\w+)\s*\)", alias, text)
    text = re.sub(r"\b__attribute__\s*\(\(.*?\)\)", " ", text)
    stack = [Scope("file")]
    buf = ""
    owner_stack = []  # for anonymous typedef struct: remembers the buffer

    def in_public_class():
        s = stack[-1]
        return s.kind == "class" and s.public and s.access == "public"

    def handle_statement(stmt, scope):
        stmt = " ".join(stmt.split())
        if not stmt:
            return
        if scope.kind in ("file", "namespace"):
            m = re.match(r"typedef\b(.*)$", stmt)
            if m:
                fp = re.search(r"\(\s*\*\s*(\w+)\s*\)", stmt)
                name = fp.group(1) if fp else re.findall(r"(\w+)\s*(?:\[[^\]]*\])*\s*$", stmt)
                if fp or name:
                    types.append(fp.group(1) if fp else name[0])
                return
            m = re.match(r"using\s+(\w+)\s*=", stmt)
            if m:
                types.append(m.group(1))
                return
            m = re.match(r"(?:class|struct|union|enum(?:\s+class)?)\s+(\w+)\s*$", stmt)
            if m:  # forward declaration
                return
            m = re.match(r"extern\b(?!\s*\"\")(.*?)(\w+)\s*(\[[^\]]*\])?$", stmt)
            if m and "(" not in stmt:
                globals_.append(m.group(2))
                return
            return
        if scope.kind == "class" and scope.public and scope.access == "public":
            if re.match(r"(typedef|using|friend)\b", stmt):
                m = re.match(r"typedef\b.*?(\w+)\s*$", stmt)
                if m:
                    types.append(m.group(1))
                return
            p = stmt.find("(")
            if p < 0:
                return
            eq = stmt.find("=")
            if 0 <= eq < p:
                return  # variable with initializer
            if re.search(r"\(\s*\*", stmt[:stmt.find(")") + 1] if ")" in stmt else stmt):
                if not re.match(r"[^(]*\w\s*\(", stmt):
                    return  # function pointer member
            m = re.match(r"(.*?)(~?\b\w+)\s*\($", stmt[:p + 1])
            if not m:
                return
            name = m.group(2)
            if name.startswith("~") or name == scope.name or name in NOT_FUNC:
                return
            if "operator" in m.group(1):
                return
            if re.search(r"\(\s*\*", stmt[p:p + 4]):
                return  # e.g. void (*cb)(...)
            funcs.setdefault(scope.name, [])
            if name not in funcs[scope.name]:
                funcs[scope.name].append(name)

    i, n = 0, len(text)
    while i < n:
        c = text[i]
        scope = stack[-1]
        if scope.kind == "block":
            # skip function bodies / initializers, just track nesting
            if c == "{":
                stack.append(Scope("block", public=False))
            elif c == "}":
                stack.pop()
            i += 1
            continue
        if c == ":" and not text.startswith("::", i) and (i == 0 or text[i - 1] != ":"):
            word = buf.strip()
            if word in ("public", "private", "protected") and scope.kind == "class":
                scope.access = word
                buf = ""
                i += 1
                continue
        if c == ";":
            if scope.kind != "enum":
                handle_statement(buf, scope)
            buf = ""
        elif c == "{":
            stmt = " ".join(buf.split())
            visible = scope.kind in ("file", "namespace") or (scope.kind == "class" and scope.public and scope.access == "public")
            m_ns = re.match(r"(?:inline\s+)?namespace\b\s*(\w*)", stmt)
            m_cls = re.match(r"(typedef\s+)?(?:\w+\s+)*?(class|struct|union)\b\s*(?:RADIOLIB_\w+\s+)?(\w+)?\s*(?:final\b)?\s*(?::[^{]*)?$", stmt)
            m_enum = re.match(r"(typedef\s+)?enum\b(?:\s+class|\s+struct)?\s*(\w+)?\s*(?::\s*[\w:\s]+)?$", stmt)
            m_ext = re.match(r"extern\s*\"\"$", stmt)
            if m_ns or m_ext:
                stack.append(Scope("namespace"))
            elif m_cls and "(" not in stmt and "=" not in stmt:
                kind = m_cls.group(2)
                name = m_cls.group(3)
                if name and visible:
                    types.append(name)
                new = Scope("class", name or "", access="private" if kind == "class" else "public", public=visible)
                new.pending_typedef = bool(m_cls.group(1)) and visible
                stack.append(new)
            elif m_enum and "(" not in stmt:
                name = m_enum.group(2)
                if name and visible:
                    types.append(name)
                new = Scope("enum", name, public=visible)
                new.pending_typedef = bool(m_enum.group(1)) and visible
                stack.append(new)
            else:
                # function body (inline definition) or brace initializer
                if scope.kind == "class" and "(" in stmt:
                    handle_statement(stmt, scope)
                stack.append(Scope("block", public=False))
            buf = ""
        elif c == "}":
            closed = stack.pop()
            buf = ""
            if closed.kind in ("class", "enum"):
                # capture declarator(s) after closing brace up to ';'
                j = text.find(";", i)
                tail = text[i + 1:j] if j >= 0 else ""
                if closed.pending_typedef:
                    for nm in re.findall(r"\b(\w+)\b", tail):
                        if nm not in ("RADIOLIB_PACKED",) and not nm.startswith("__"):
                            types.append(nm)
                    if closed.kind == "class" and not closed.name:
                        # anonymous typedef'd struct: rename collected funcs
                        pass
                i = j if j >= 0 else n
        else:
            buf += c
        i += 1


def classify(rel):
    parts = rel.replace("\\", "/").split("/")
    if parts[0] == "modules":
        return "modules", parts[1]
    if parts[0] == "protocols":
        return "protocols", parts[1]
    if parts[0] == "hal":
        return "hal", parts[1]
    if parts[0] == "utils":
        return "utils", "utils"
    return "core", "RadioLib"


def main():
    per_file = []
    for dirpath, dirnames, filenames in os.walk(SRC):
        dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS)
        for fn in sorted(filenames):
            if not fn.endswith(".h") or SKIP_FILES_RE.search(fn):
                continue
            path = os.path.join(dirpath, fn)
            rel = os.path.relpath(path, SRC)
            types, funcs, globals_ = [], {}, []
            parse_header(path, types, funcs, globals_)
            per_file.append((rel, types, funcs, globals_))

    with open(KW, encoding="utf-8") as f:
        old = f.read().replace("\r\n", "\n")
    literal_idx = old.index("#######################################\n# Constants (LITERAL1)")
    literals = old[literal_idx:]
    # normalise separators in the kept section to true tabs and drop duplicates
    seen_lit, lit_lines = set(), []
    for line in literals.split("\n"):
        m = re.match(r"^(\w+)\s+(LITERAL1)\s*$", line)
        if m:
            if m.group(1) in seen_lit:
                continue
            seen_lit.add(m.group(1))
            line = f"{m.group(1)}\t{m.group(2)}"
        lit_lines.append(line.rstrip())
    while lit_lines and lit_lines[-1] == "":
        lit_lines.pop()

    order = {"core": 0, "hal": 1, "modules": 2, "protocols": 3, "utils": 4}
    # group sections by folder (e.g. SX126x, LoRaWAN) keeping header order
    groups = {}
    for rel, types, funcs, globals_ in per_file:
        cat, grp = classify(rel)
        g = groups.setdefault((order[cat], cat, grp), {"types": [], "funcs": {}, "globals": []})
        g["types"] += types
        g["globals"] += globals_
        for cls, fl in funcs.items():
            g["funcs"].setdefault(cls, [])
            g["funcs"][cls] += [f for f in fl if f not in g["funcs"][cls]]

    used = set(seen_lit)
    out = ["#######################################",
           "# Syntax Coloring Map For RadioLib",
           "#######################################",
           "",
           "#######################################",
           "# Datatypes (KEYWORD1)",
           "#######################################",
           "",
           "RadioLib\tKEYWORD1"]
    used.add("RadioLib")

    def emit(title, names, kw):
        names = [x for x in dict.fromkeys(names) if x and x not in used]
        if not names:
            return
        out.append("")
        out.append(f"# {title}")
        for x in names:
            out.append(f"{x}\t{kw}")
            used.add(x)

    titles = {"core": "", "hal": "HAL ", "modules": "", "protocols": "", "utils": ""}
    for key in sorted(groups, key=lambda k: (k[0], k[2].lower())):
        _, cat, grp = key
        emit(f"{titles[cat]}{grp}".strip(), groups[key]["types"], "KEYWORD1")
    for key in sorted(groups, key=lambda k: (k[0], k[2].lower())):
        _, cat, grp = key
        emit(f"{grp} constants", groups[key]["globals"], "KEYWORD1")

    out += ["",
            "#######################################",
            "# Methods and Functions (KEYWORD2)",
            "#######################################"]
    # base classes first so shared method names are listed under them
    base_first = ["RadioLibHal", "Module", "PhysicalLayer"]
    all_classes = []
    for key in sorted(groups, key=lambda k: (k[0], k[2].lower())):
        for cls, fl in groups[key]["funcs"].items():
            all_classes.append((cls, fl))
    all_classes.sort(key=lambda cf: base_first.index(cf[0]) if cf[0] in base_first else len(base_first))
    for cls, fl in all_classes:
        emit(cls, fl, "KEYWORD2")

    out.append("")
    out += lit_lines
    with open(KW, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()

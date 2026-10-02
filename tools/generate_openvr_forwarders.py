#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate type-checked forwarding adapters from the pinned Valve interfaces.

The headers define the complete vtable/flat-table order. No guessed slot offsets
or writes to runtime-owned vtables are used. Run by CMake in the build directory.
"""
import pathlib
import re
import sys

source = pathlib.Path(sys.argv[1]).read_text()
output = ["// Generated from Valve's BSD-3-Clause API declarations; do not edit.",
          "// See third_party/openvr/LICENSE and THIRD_PARTY.md."]
for name in ("System", "Input"):
    body = source.split("class IVR" + name, 1)[1].split("\n};" if name == "System" else "\n\t};", 1)[0]
    body = re.sub(r"/\*.*?\*/|//[^\n]*", "", body, flags=re.S)
    body = re.sub(r"VR_\w+\s*\([^)]*\)", "", body)
    methods = []
    for ret, method, params in re.findall(r"virtual\s+([\w:* ]+?[*&\s])(\w+)\s*\((.*?)\)\s*=\s*0\s*;", body, re.S):
        params = re.sub(r"\s+", " ", params).strip()
        args = []
        clean = []
        for p in params.split(",") if params else []:
            p = p.split("=")[0].strip()
            clean.append(p)
            args.append(re.search(r"(\w+)\s*(?:\[[^]]*\])?$", p).group(1))
        methods.append((ret.strip(), method, ", ".join(clean), ", ".join(args)))
    assert len(methods) == len(re.findall(r"virtual\s", body)), (name, len(methods))
    output.append(f"struct {name}Table {{")
    for ret, method, params, _ in methods:
        output.append(f"    {ret} (*{method})({params});")
    output.append("};")
    for flat in (False, True):
        cls = name + ("FlatForward" if flat else "Forward")
        ptr = name + "Table" if flat else "vr::IVR" + name
        output.append(f"struct {cls} : vr::IVR{name} {{\n    {ptr}* next = nullptr;")
        for ret, method, params, args in methods:
            output.append(f"    {ret} {method}({params}) override {{ return next->{method}({args}); }}")
        output.append("};")
    output.append(f"struct {name}Trampoline {{\n    inline static vr::IVR{name}* target = nullptr;")
    output.append(f"    inline static {name}Table table = {{")
    for ret, method, params, args in methods:
        output.append(f"        +[]({params}) -> {ret} {{ return target->{method}({args}); }},")
    output.append("    };\n};")
    output.append(f"struct {name}Defaults : vr::IVR{name} {{")
    for ret, method, params, args in methods:
        unused = " ".join(f"(void){arg};" for arg in args.split(", ") if arg)
        result = "" if ret == "void" else "return {};"
        output.append(f"    {ret} {method}({params}) override {{ {unused} {result} }}")
    output.append("};")
pathlib.Path(sys.argv[2]).write_text("\n".join(output) + "\n")

#!/usr/bin/env python3
import os
import re
import sys


def generate_arrayname(input_file):
    return "xml_feature_" + re.sub(r"[-.]", "_", os.path.basename(input_file))


def convert_to_c_array(input_file, arrayname):
    """
    Generates a C array string representation from an input file.

    Args:
        arrayname: Name of the array.
        input_file: Path to the input file.

    Returns:
        A string containing the formatted C array declaration and data.
    """
    ord_map = {chr(i): i for i in range(255)}

    with open(
        input_file,
        "r",
        encoding="utf-8",
    ) as f:
        lines = f.read().splitlines()

    output = '#include "qemu/osdep.h"\n'
    output += f"static const char {arrayname}[] = {{\n"

    for line in lines:
        output += "  "

        for i, c in enumerate(line):
            if c == "'":
                output += "'\\'', "
            elif c == "\\":
                output += "'\\\\', "
            elif 32 <= ord_map[c] < 127:
                output += f"'{c}', "
            else:
                output += f"'\\{ord_map[c]:03o}', "
            if (i + 1) % 10 == 0:
                output += "\n  "

        output += "'\\n', \n"

    output += "  0 };"
    return output


def main():
    if len(sys.argv) < 2 or not sys.argv[1]:
        print("Usage: {} INPUTFILE...".format(sys.argv[0]))
        sys.exit(1)

    for input_file in sys.argv[1:]:
        arrayname = generate_arrayname(input_file)
        print(convert_to_c_array(input_file, arrayname))

    print()
    print('#include "exec/gdbstub.h"')
    print("const char *const xml_builtin[][2] = {")

    for input_file in sys.argv[1:]:
        basename = os.path.basename(input_file)
        arrayname = generate_arrayname(input_file)
        print(f'  {{ "{basename}", {arrayname} }},')

    print("  { (char *)0, (char *)0 }")
    print("};")


if __name__ == "__main__":
    main()

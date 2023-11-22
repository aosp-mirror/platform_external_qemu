#!/usr/bin/env python3
import argparse
import sys
import os

def cat(files, show_non_printing=False, number_lines=False, show_ends=False, squeeze_empty_lines=False, show_tabs=False, disable_output_buffering=False):
    try:
        if not files:
            # Read from standard input if no files are provided
            content = sys.stdin.read()
            print_content(content, show_non_printing, number_lines, show_ends, show_tabs)
        else:
            for file in files:
                if file == '-':
                    # Read from standard input
                    content = sys.stdin.read()
                    print_content(content, show_non_printing, number_lines, show_ends, show_tabs)
                else:
                    with open(file, 'r') as f:
                        content = f.read()
                        print_content(content, show_non_printing, number_lines, show_ends, show_tabs)

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

def print_content(content, show_non_printing, number_lines, show_ends, show_tabs):
    lines = content.splitlines(keepends=True)
    line_number = 1

    for line in lines:
        if show_non_printing:
            line = repr(line)[1:-1]  # Display non-printing characters
        if show_tabs:
            line = line.replace('\t', '^I')  # Display tab characters as '^I'

        if number_lines:
            print(f"{line_number}\t{line}", end='')
            line_number += 1
        elif show_ends:
            print(f"{line.rstrip('$')}$")
        else:
            print(line, end='')

def main():
    parser = argparse.ArgumentParser(description="Concatenate and print files")
    parser.add_argument('files', nargs='*', help="Files to concatenate")
    parser.add_argument('-b', action='store_true', help="Number the non-blank output lines, starting at 1")
    parser.add_argument('-e', action='store_true', help="Display non-printing characters and display a dollar sign ('$') at the end of each line")
    parser.add_argument('-n', action='store_true', help="Number the output lines, starting at 1")
    parser.add_argument('-s', action='store_true', help="Squeeze multiple adjacent empty lines")
    parser.add_argument('-t', action='store_true', help="Display non-printing characters and display tab characters as '^I'")
    parser.add_argument('-u', action='store_true', help="Disable output buffering")
    parser.add_argument('-v', action='store_true', help="Display non-printing characters so they are visible")

    args = parser.parse_args()

    if args.u:
        # Disable output buffering
        sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    cat(
        args.files,
        show_non_printing=args.v,
        number_lines=args.n or args.b,
        show_ends=args.e,
        squeeze_empty_lines=args.s,
        show_tabs=args.t,
        disable_output_buffering=args.u
    )

if __name__ == '__main__':
    main()

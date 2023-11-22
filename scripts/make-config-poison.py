#!/usr/bin/env python3
import argparse
import re


def poison_config_switches(files):
    config_switches_to_poison = ["CONFIG_TCG", "CONFIG_USER_ONLY", "CONFIG_SOFTMMU"]

    poisoned_switches = set()

    for filename in files:
        with open(filename, "r", encoding="utf-8") as file:
            for line in file:
                if any([x in line for x in config_switches_to_poison]):
                    continue
                match = re.match(r"#define (\S+) .*", line)
                if match:
                    switch = match.group(1)
                    poisoned_switches.add(switch)

    for switch in sorted(poisoned_switches):
        print(f"#pragma GCC poison {switch}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate #pragma GCC poison statements for specified config switches."
    )
    parser.add_argument("files", nargs="+", help="List of files to process")

    args = parser.parse_args()
    poison_config_switches(args.files)


if __name__ == "__main__":
    main()

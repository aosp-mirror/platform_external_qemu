#!/usr/bin/env python


def hxtoh(file_path):
    flag = 1
    with open(file_path, "r") as file:
        for line in file:
            str_line = line.strip()
            if str_line.startswith("HXCOMM"):
                pass
            elif str_line.startswith(("SRST", "ERST")):
                flag ^= 1
            else:
                if flag == 1:
                    print(str_line)


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 3 or sys.argv[1] != "-h":
        print("Usage: python script.py -h input_file")
        sys.exit(1)

    hxtoh(sys.argv[2])
    sys.exit(0)

#!/bin/env python3
"""Python mako application to generate the www_bin source files

Copyright Melexis N.V.

This product includes software developed at Melexis N.V. (https://www.melexis.com).

Melexis N.V. has provided this code according to LICENSE file attached to repository
"""
import argparse
from pathlib import Path
from mako.template import Template


def get_all_files(root):
    files = []
    for item in root.iterdir():
        if item.is_file():
            files.append(item)
        else:
            files.extend(get_all_files(item))
    return files


def main():
    parser = argparse.ArgumentParser(description="Melexis www_bin source generator")
    parser.add_argument("dist",
                        action="store",
                        type=Path,
                        help="path to the VueJs dist folder")
    args = parser.parse_args()

    files = list(filter(lambda x: x.is_file(), args.dist.rglob('*')))
    cur_dir = Path(__file__).parent.resolve()

    header_file = cur_dir / ".." / "include" / "www_bin.h"
    header_file.parent.mkdir(parents=True, exist_ok=True)
    with open(header_file, "w") as fd:
        fd.write(Template(filename=f"{cur_dir / 'www_bin.h.mako'}").render(dist=args.dist, files=files))

    source_file = cur_dir / ".." / "www_bin.c"
    with open(source_file, "w") as fd:
        fd.write(Template(filename=f"{cur_dir / 'www_bin.c.mako'}").render(dist=args.dist, files=files))


if __name__ == "__main__":
    main()

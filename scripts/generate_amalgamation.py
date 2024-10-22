#!/usr/bin/env python

import argparse
import os
import pathlib
import re
import subprocess
import sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            prog='generate_amalgamation.py',
            description='Generates a PLYwoot amalgamation header, outputs on the standard output stream')

    args = parser.parse_args()


    encoding = os.device_encoding(sys.stdout.fileno()) or 'utf-8'

    result = subprocess.run(['/usr/bin/g++', '-E', '-CC', '-nostdinc', '-nostdinc++', 'include/plywoot/plywoot.hpp'], capture_output=True, stderr=None, encoding=encoding)

    # List all system headers in the source code.
    system_headers = set()
    for header in pathlib.Path('include').rglob('*.hpp'):
        source_code = open(header).read()
        for match in re.findall(r'#include\s*<(.*)>', source_code):
            system_headers.add(match)

    # Filter out `fast_float` and `fast_int` headers.
    system_headers.remove('fast_float/fast_float.h')
    system_headers.remove('fast_int/fast_int.hpp')

    # Note; we ignore any errors here, specifying `-nostdinc` will result in a
    # failures by default.
    preprocessor_output = result.stdout

    # Determine the license header.
    license_header = re.search(r'^/\*[^*]*\*/', preprocessor_output, flags=re.MULTILINE).group(0)

    # Strip out decorations from the preprocessor.
    output = re.sub(r'^#.*$', '', preprocessor_output, flags=re.MULTILINE)

    # Strip out duplicate license headers.
    output = re.sub(r'^/\*[^*]*\*/', '', output, flags=re.MULTILINE)

    # Strip out `\file` tags.
    output = re.sub(r'^/// \\file.*$', '', output, flags=re.MULTILINE)

    # Join empty lines.
    output = re.sub(r'\n[\n]+\n', '\n\n', output, flags=re.MULTILINE).strip()

    # Prepend the license header, system headers, and `fast_float` and
    # `fast_int` headers.
    output = f"""
{license_header}

{'\n'.join(['#include <' + system_header + '>' for system_header in system_headers])}

#ifdef PLYWOOT_USE_FAST_FLOAT
#include <fast_float/fast_float.h>
#endif

#ifdef PLYWOOT_USE_FAST_INT
#include <fast_int/fast_int.hpp>
#endif

{output}"""

    # Prepend with the system includes.
    print(output)

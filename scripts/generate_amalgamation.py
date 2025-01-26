#!/usr/bin/env python

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            prog='generate_amalgamation.py',
            description='Generates a PLYwoot amalgamation header, outputs on the standard output stream')

    args = parser.parse_args()

    encoding = os.device_encoding(sys.stdout.fileno()) or 'utf-8'

    # Copy all of the PLYwoot code to /tmp, to be able to modify some of the
    # `#ifdef` clauses.
    td = tempfile.TemporaryDirectory()
    tmp_path = os.path.join(td.name, 'plywoot')

    shutil.copytree('include/plywoot', tmp_path)

    # In `std.hpp`, comment out all `#ifdef PLYWOOT_HAS_*` preprocessor
    # directives using this super-ugly hard-coded mechanism :P
    lines = open(os.path.join(tmp_path, 'plywoot/std.hpp')).readlines()

    # Strip out the last `#ifdef` to prevent it from being commented out below.
    lines = lines[:-1]

    with open(os.path.join(tmp_path, 'plywoot/std.hpp'), 'w+') as std_hpp:
        in_namespace = False
        for line in lines:
            in_namespace = in_namespace or ('namespace' in line)
            if in_namespace and line.startswith('#'):
                std_hpp.write('//' + line)
            else:
                std_hpp.write(line)

        # Write out last `#endif` that was stripped out above.
        std_hpp.write('#endif\n')

    result = subprocess.run(['/usr/bin/g++', '-E', '-CC', '-nostdinc', '-nostdinc++', os.path.join(tmp_path, 'plywoot.hpp')], capture_output=True, stderr=None, encoding=encoding)

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

    # Fixup commented out preprocessor directives.
    output = re.sub(r'//#(.*)$', '#\\1', output, flags=re.MULTILINE)

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

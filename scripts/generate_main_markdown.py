#!/usr/bin/env python

import argparse
import re
import sys

header = """
# PLYwoot

PLYwoot is a C++17 header-only library providing read/write support for [PLY](https://paulbourke.net/dataformats/ply/) files. A major design goal of PLYwoot was to provide an [iostreams](https://en.cppreference.com/w/cpp/io) based interface without sacrificing performance.
"""


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
            prog='generate_main_markdown.py',
            description='Generates Markdown for the PLYwoot Doxygen documentation from the Github README.md, outputs on the standard output stream')

    parser.add_argument('-i', '--input', help='input README.md file')

    args = parser.parse_args()

    # Extract the parsing PLY and writing PLY files sections.
    contents = []
    with open('README.md', 'r') as readme_file:
        # Extract all paragraphs from 'Getting started' up to 'Dependencies'.
        from_paragraph_re = re.compile('^## Getting')
        upto_paragraph_re = re.compile('^## API')
        section_re = re.compile('^#+ (.*)$')

        # Do not put a link to the API documentation itself in the
        # documentation.
        remove_res = [re.compile('For more details on the functions.*$')]

        # Some sections need to be linkable.
        link_section_res = [re.compile('## (Dependencies).*$'), re.compile('## (Generating an amalgamation header).*$')]

        extract = False

        for line in readme_file:
            if not extract and from_paragraph_re.match(line):
                extract = True

            if extract and upto_paragraph_re.match(line):
                break

            if extract:
                # In case this is a section, append '{section name}' to make
                # sure section links work in the same way between Github and
                # Doxygen.
                for link_section_re in link_section_res:
                    m = re.match(link_section_re, line)
                    if m:
                        line = line.strip() + ' {#%s}\n' % m.group(1).replace(' ', '-')

                for remove_re in remove_res:
                    line = re.sub(remove_re, '', line)

                contents.append(line)

    sys.stdout.write(header)
    sys.stdout.write(''.join(contents))

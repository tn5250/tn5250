#!/usr/bin/env python3

# mkunicodemaps.py -- Generate unicodemaps.h for tn5250.
#
#   This file is part of TN5250.
#
#   TN5250 is free software; you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation; either version 2.1 of the License, or
#   (at your option) any later version.
#
#   TN5250 is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

"""Generate the EBCDIC to Unicode tables used by lib5250.

The tables are built from the conversion tables (.ucm files) of the ICU
project, which were themselves derived from IBM's CDRA conversion tables:

    mkunicodemaps.py --download --ucm-dir /tmp/ucm -o unicodemaps.h

--download fetches the tables which are missing from the --ucm-dir
directory, along with the license they come with.  Use --check instead of
-o to verify that an existing header matches what this script would
generate.

To support another single-byte EBCDIC CCSID, add it to CCSIDS below, run
this script again and add the new table to tn5250_transmaps in utility.c.
"""

import argparse
import os
import re
import sys
import textwrap
import unicodedata
import urllib.request

# Where the conversion tables come from.  The revisions are pinned so that
# this script generates the same header until somebody decides otherwise.
SOURCES = {
    'icu': 'https://raw.githubusercontent.com/unicode-org/icu/release-79.1/'
           'icu4c/source/data/mappings/',
    'icu-data': 'https://raw.githubusercontent.com/unicode-org/icu-data/'
                '8d9eb3e27e79f59dd76e278e58d68b4668835027/charset/data/ucm/',
}

# The license of the conversion tables.  Its notice has to go with them, so
# it is copied to the generated header.
LICENSE = 'ICU-LICENSE'
LICENSE_URL = ('https://raw.githubusercontent.com/unicode-org/icu/'
               'release-79.1/LICENSE')

# (CCSID, source, conversion table, description)
CCSIDS = [
    (37, 'icu', 'ibm-37_P100-1995', 'USA, Canada, Netherlands, Portugal'),
    (256, 'icu-data', 'ibm-256_P100-1995', 'Netherlands (obsolete)'),
    (273, 'icu', 'ibm-273_P100-1995', 'Austria, Germany'),
    (277, 'icu', 'ibm-277_P100-1995', 'Denmark, Norway'),
    (278, 'icu', 'ibm-278_P100-1995', 'Finland, Sweden'),
    (280, 'icu', 'ibm-280_P100-1995', 'Italy'),
    (284, 'icu', 'ibm-284_P100-1995', 'Spain, Latin America'),
    (285, 'icu', 'ibm-285_P100-1995', 'United Kingdom'),
    (290, 'icu', 'ibm-290_P100-1995', 'Japan (Katakana)'),
    (297, 'icu', 'ibm-297_P100-1995', 'France'),
    (420, 'icu', 'ibm-420_X120-1999', 'Arabic'),
    (424, 'icu', 'ibm-424_P100-1995', 'Hebrew'),
    (500, 'icu', 'ibm-500_P100-1995', 'Belgium, Canada, Switzerland'),
    (870, 'icu', 'ibm-870_P100-1995', 'Latin-2 (Central Europe)'),
    (871, 'icu', 'ibm-871_P100-1995', 'Iceland'),
    (875, 'icu', 'ibm-875_P100-1995', 'Greece'),
    (880, 'icu-data', 'ibm-880_P100-1995', 'Cyrillic'),
    (905, 'icu-data', 'ibm-905_P100-1995', 'Turkey (Latin-3)'),
    (1026, 'icu', 'ibm-1026_P100-1995', 'Turkey (Latin-5)'),
    (1140, 'icu', 'ibm-1140_P100-1997', '37 with euro sign'),
    (1141, 'icu', 'ibm-1141_P100-1997', '273 with euro sign'),
    (1142, 'icu', 'ibm-1142_P100-1997', '277 with euro sign'),
    (1143, 'icu', 'ibm-1143_P100-1997', '278 with euro sign'),
    (1144, 'icu', 'ibm-1144_P100-1997', '280 with euro sign'),
    (1145, 'icu', 'ibm-1145_P100-1997', '284 with euro sign'),
    (1146, 'icu', 'ibm-1146_P100-1997', '285 with euro sign'),
    (1147, 'icu', 'ibm-1147_P100-1997', '297 with euro sign'),
    (1148, 'icu', 'ibm-1148_P100-1997', '500 with euro sign'),
    (1149, 'icu', 'ibm-1149_P100-1997', '871 with euro sign'),
    (1153, 'icu', 'ibm-1153_P100-1999', '870 with euro sign'),
    (1155, 'icu', 'ibm-1155_P100-1999', '1026 with euro sign'),
    (4971, 'icu', 'ibm-4971_P100-1999', '875 with euro sign'),
    (12712, 'icu', 'ibm-12712_P100-1998', '424 with euro sign'),
    (16804, 'icu', 'ibm-16804_X110-1999', '420 with euro sign'),
]

# Code point used for the EBCDIC codes a CCSID does not define.
UNDEFINED = 0xFFFD

MAPPING = re.compile(r'^<U([0-9A-Fa-f]{4,6})>\s+((?:\\x[0-9A-Fa-f]{2})+)'
                     r'\s+\|(\d)\s*$')


def fetch(url, path):
    """Download a file."""
    sys.stderr.write('Fetching %s\n' % url)
    with urllib.request.urlopen(url) as response:
        data = response.read()
    with open(path, 'wb') as out:
        out.write(data)


def read_license(path):
    """Return the lines of the copyright and permission notice which comes
    first in the license file of ICU, before the ones of the software ICU
    includes."""
    lines = []
    with open(path, encoding='utf-8') as license_file:
        for line in license_file:
            if line.startswith('-----'):
                break
            # The header is plain ASCII, to suit every compiler.
            lines.append(line.rstrip().replace('\u00a9', '(C)'))
    while lines and not lines[-1]:
        lines.pop()
    if not lines or any('*/' in line for line in lines):
        raise ValueError('%s: unexpected contents' % path)
    return lines


def read_copyrights(path):
    """Return the copyright statements at the top of a .ucm file."""
    comment = []
    with open(path, encoding='ascii') as ucm:
        for line in ucm:
            if not line.startswith('#'):
                break
            comment.append(line.strip('#* \n'))
    return re.findall(r'Copyright \(C\) .*? and others\.'
                      r'(?: +All Rights Reserved\.)?', ' '.join(comment))


def read_ucm(path):
    """Return the EBCDIC to Unicode table of a single-byte .ucm file.

    Only the round-trip mappings (|0) are used: the other ones are
    fallbacks from Unicode, which are of no use to display a character.
    """
    table = [UNDEFINED] * 256
    seen = set()
    in_charmap = False
    with open(path, encoding='ascii') as ucm:
        for line in ucm:
            line = line.strip()
            if line == 'CHARMAP':
                in_charmap = True
            elif line == 'END CHARMAP':
                in_charmap = False
            elif in_charmap and line and not line.startswith('#'):
                mapping = MAPPING.match(line)
                if mapping is None:
                    raise ValueError('%s: cannot parse "%s"' % (path, line))
                code_point = int(mapping.group(1), 16)
                ebcdic = bytes.fromhex(mapping.group(2).replace('\\x', ''))
                if len(ebcdic) != 1:
                    raise ValueError('%s: not a single-byte table' % path)
                if mapping.group(3) != '0':
                    continue
                if ebcdic[0] in seen or code_point > 0xFFFF:
                    raise ValueError('%s: unexpected mapping "%s"'
                                     % (path, line))
                seen.add(ebcdic[0])
                table[ebcdic[0]] = code_point
    if not seen:
        raise ValueError('%s: no mappings found' % path)
    return table


def is_alpha(code_point):
    """Letters, and the marks that only make sense along with letters."""
    return unicodedata.category(chr(code_point))[0] in 'LM'


def alpha_ranges(repertoire):
    """Ranges of alphabetic code points covering the alphabetic characters
    of the repertoire."""
    ranges = []
    for code_point in sorted(repertoire):
        if not is_alpha(code_point):
            continue
        if ranges and code_point <= ranges[-1][1]:
            continue
        first = last = code_point
        while first > 0 and is_alpha(first - 1):
            first -= 1
        while last < 0xFFFF and is_alpha(last + 1):
            last += 1
        ranges.append((first, last))
    return ranges


def upper_pairs(repertoire):
    """(lower case, upper case) pairs of the repertoire.  Characters whose
    upper case is not a single character of the repertoire are left out, as
    no host could be sent their upper case anyway."""
    pairs = []
    for code_point in sorted(repertoire):
        upper = chr(code_point).upper()
        if len(upper) == 1 and ord(upper) != code_point \
           and ord(upper) in repertoire:
            pairs.append((code_point, ord(upper)))
    return pairs


def format_pairs(pairs):
    """Format (first, second) pairs as C initializers, three on a line."""
    lines = []
    for i in range(0, len(pairs), 3):
        lines.append('    ' + ' '.join('{ 0x%04X, 0x%04X },' % pair
                                       for pair in pairs[i:i + 3]))
    return lines


def generate(ucm_dir):
    """Return the contents of unicodemaps.h."""
    copyrights = set()
    for _, _, name, _ in CCSIDS:
        copyrights.update(read_copyrights(os.path.join(ucm_dir,
                                                       name + '.ucm')))

    out = [
        '/* unicodemaps.h was automagically generated by mkunicodemaps.py '
        'from the',
        '   conversion tables of the ICU project (https://icu.unicode.org/), '
        'and from',
        '   the Unicode Character Database.',
        '   Any changes should be made in mkunicodemaps.py and NOT in this '
        'file!',
        '',
        '   The conversion tables are:',
    ]
    for statement in sorted(copyrights):
        out.extend(textwrap.wrap(statement, 76, initial_indent='   ',
                                 subsequent_indent='   '))
    out.extend(['', '   They come with this notice:', ''])
    out.extend(('   ' + line).rstrip()
               for line in read_license(os.path.join(ucm_dir, LICENSE)))
    out.extend([' */', '', '/* clang-format off */', ''])

    repertoire = set()
    for ccsid, _, name, description in CCSIDS:
        table = read_ucm(os.path.join(ucm_dir, name + '.ucm'))
        repertoire.update(code_point for code_point in table[0x40:0xFF]
                          if code_point != UNDEFINED)
        out.append('/* CCSID %d (%s), from %s.ucm */' % (ccsid, description,
                                                       name))
        out.append('')
        out.append('static const Tn5250Uint16 ibm%03d_to_unicode[256] = {'
                   % ccsid)
        for row in range(0, 256, 8):
            out.append('    %s /* %02X */' % (
                ' '.join('0x%04X,' % code_point
                         for code_point in table[row:row + 8]), row))
        out.extend(['};', ''])

    out.extend([
        '/* Ranges of alphabetic characters, limited to the ones which '
        'cover the',
        '   characters of the tables above.  From the Unicode Character '
        'Database. */',
        '',
        'static const Tn5250Uint16 unicode_alpha_ranges[][2] = {',
    ])
    out.extend(format_pairs(alpha_ranges(repertoire)))
    out.extend(['};', ''])

    out.extend([
        '/* Lower case characters of the tables above, with their upper '
        'case.  From',
        '   the Unicode Character Database. */',
        '',
        'static const Tn5250Uint16 unicode_upper_map[][2] = {',
    ])
    out.extend(format_pairs(upper_pairs(repertoire)))
    out.extend(['};', '', '/* clang-format on */'])
    return '\n'.join(out) + '\n'


def main():
    parser = argparse.ArgumentParser(
        description='Generate unicodemaps.h from ICU conversion tables.')
    parser.add_argument('--ucm-dir', required=True, metavar='DIR',
                        help='directory holding the .ucm files')
    parser.add_argument('--download', action='store_true',
                        help='download the .ucm files missing from DIR')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-o', '--output', metavar='FILE',
                       help='write the header to FILE instead of stdout')
    group.add_argument('--check', metavar='FILE',
                       help='check that FILE is up to date')
    args = parser.parse_args()

    if args.download:
        os.makedirs(args.ucm_dir, exist_ok=True)
        wanted = [(LICENSE_URL, LICENSE)]
        wanted.extend((SOURCES[source] + name + '.ucm', name + '.ucm')
                      for _, source, name, _ in CCSIDS)
        for url, name in wanted:
            path = os.path.join(args.ucm_dir, name)
            if not os.path.exists(path):
                fetch(url, path)

    header = generate(args.ucm_dir)

    if args.check:
        with open(args.check, encoding='ascii') as existing:
            if existing.read() != header:
                sys.exit('%s is not up to date' % args.check)
    elif args.output:
        with open(args.output, 'w', encoding='ascii') as output:
            output.write(header)
    else:
        sys.stdout.write(header)


if __name__ == '__main__':
    main()

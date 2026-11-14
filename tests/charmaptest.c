/* TN5250 - An implementation of the 5250 telnet protocol.
 *
 * This file is part of TN5250.
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1, or (at your option)
 * any later version.
 *
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 */

/* Tests of the character maps of lib5250 and of what goes with them: the
 * UTF-8 functions and the keys which stand for Unicode characters.
 *
 *    charmaptest          runs the tests
 *    charmaptest --dump   prints the character maps, for test_terminal.py
 *
 * The tables of unicodemaps.h are not compared with where they come from
 * here, as it takes the conversion tables of ICU: mkunicodemaps.py --check
 * does that.
 */

#include "tn5250-private.h"

static int checks = 0;
static int failures = 0;

static void check(int ok, const char* fmt, ...) {
    va_list args;

    checks++;
    if (ok) {
        return;
    }
    failures++;
    if (failures <= 40) {
        printf("FAIL: ");
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }
}

/* What tells the character maps apart: the characters of the EBCDIC codes
 * below, which are the ones the national character sets disagree on, plus
 * a few which only matter to Greek, Hebrew and Arabic. */
#define SIGNATURE_SIZE 15

static const unsigned char signature_codes[SIGNATURE_SIZE] = {
    0x4A, 0x5A, 0x5B, 0x6A, 0x79, 0x7B, 0x7C, 0x9C,
    0x9F, 0xA1, 0xC0, 0xD0, 0xE0, 0xE1, 0xFC
};

static const struct {
    const char* name;
    Tn5250Uint16 unicode[SIGNATURE_SIZE];
} signatures[] = {
    // clang-format off
    { "37",
      { 0x00A2, 0x0021, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x00A4, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "256",
      { 0x005B, 0x005D, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x00A4, 0x007E, 0x007B, 0x007D, 0x005C, 0x2007, 0x00DC } },
    { "273",
      { 0x00C4, 0x00DC, 0x0024, 0x00F6, 0x0060, 0x0023, 0x00A7, 0x00E6,
        0x00A4, 0x00DF, 0x00E4, 0x00FC, 0x00D6, 0x00F7, 0x005D } },
    { "277",
      { 0x0023, 0x00A4, 0x00C5, 0x00F8, 0x0060, 0x00C6, 0x00D8, 0x007B,
        0x005D, 0x00FC, 0x00E6, 0x00E5, 0x005C, 0x00F7, 0x00DC } },
    { "278",
      { 0x00A7, 0x00A4, 0x00C5, 0x00F6, 0x00E9, 0x00C4, 0x00D6, 0x00E6,
        0x005D, 0x00FC, 0x00E4, 0x00E5, 0x00C9, 0x00F7, 0x00DC } },
    { "280",
      { 0x00B0, 0x00E9, 0x0024, 0x00F2, 0x00F9, 0x00A3, 0x00A7, 0x00E6,
        0x00A4, 0x00EC, 0x00E0, 0x00E8, 0x00E7, 0x00F7, 0x00DC } },
    { "284",
      { 0x005B, 0x005D, 0x0024, 0x00F1, 0x0060, 0x00D1, 0x0040, 0x00E6,
        0x00A4, 0x00A8, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "285",
      { 0x0024, 0x0021, 0x00A3, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x00A4, 0x00AF, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "297",
      { 0x00B0, 0x00A7, 0x0024, 0x00F9, 0x00B5, 0x00A3, 0x00E0, 0x00E6,
        0x00A4, 0x00A8, 0x00E9, 0x00E8, 0x00E7, 0x00F7, 0x00DC } },
    { "500",
      { 0x005B, 0x005D, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x00A4, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "871",
      { 0x00DE, 0x00C6, 0x0024, 0x00A6, 0x00F0, 0x0023, 0x00D0, 0x007D,
        0x00A4, 0x00F6, 0x00FE, 0x00E6, 0x00B4, 0x00F7, 0x00DC } },
    { "870",
      { 0x005B, 0x005D, 0x0024, 0x007C, 0x0060, 0x0023, 0x0040, 0x0161,
        0x00A4, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "905",
      { 0x00C7, 0x011E, 0x0130, 0x015F, 0x0131, 0x00D6, 0x015E, 0x0135,
        0x00A4, 0x00F6, 0x00E7, 0x011F, 0x00FC, 0x00F7, 0x0022 } },
    { "880",
      { 0x005B, 0x005D, 0x0024, 0x007C, 0x0060, 0x0023, 0x0040, 0x043C,
        0x043F, 0x007E, 0x007B, 0x007D, 0x005C, 0x00A4, 0x042D } },
    { "420",
      { 0x00A2, 0x0021, 0x0024, 0x00A6, 0x060C, 0x0023, 0x0040, 0xFECB,
        0xFECE, 0x00F7, 0x061B, 0x061F, 0x00D7, 0xFFFD, 0x0667 } },
    { "875",
      { 0x005B, 0x005D, 0x0024, 0x007C, 0x0060, 0x0023, 0x0040, 0x03B9,
        0x03BC, 0x007E, 0x007B, 0x007D, 0x005C, 0xFFFD, 0xFFFD } },
    { "424",
      { 0x00A2, 0x0021, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0xFFFD,
        0x00A4, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0xFFFD } },
    { "1026",
      { 0x00C7, 0x011E, 0x0130, 0x015F, 0x0131, 0x00D6, 0x015E, 0x00E6,
        0x00A4, 0x00F6, 0x00E7, 0x011F, 0x00FC, 0x00F7, 0x0022 } },
    { "290",
      { 0x00A3, 0x0021, 0x00A5, 0xFFFD, 0x0060, 0x0023, 0x0040, 0xFFFD,
        0xFF8C, 0x203E, 0x007B, 0x007D, 0x0024, 0xFFFD, 0xFFFD } },
    { "1140",
      { 0x00A2, 0x0021, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x20AC, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "1141",
      { 0x00C4, 0x00DC, 0x0024, 0x00F6, 0x0060, 0x0023, 0x00A7, 0x00E6,
        0x20AC, 0x00DF, 0x00E4, 0x00FC, 0x00D6, 0x00F7, 0x005D } },
    { "1142",
      { 0x0023, 0x20AC, 0x00C5, 0x00F8, 0x0060, 0x00C6, 0x00D8, 0x007B,
        0x005D, 0x00FC, 0x00E6, 0x00E5, 0x005C, 0x00F7, 0x00DC } },
    { "1143",
      { 0x00A7, 0x20AC, 0x00C5, 0x00F6, 0x00E9, 0x00C4, 0x00D6, 0x00E6,
        0x005D, 0x00FC, 0x00E4, 0x00E5, 0x00C9, 0x00F7, 0x00DC } },
    { "1144",
      { 0x00B0, 0x00E9, 0x0024, 0x00F2, 0x00F9, 0x00A3, 0x00A7, 0x00E6,
        0x20AC, 0x00EC, 0x00E0, 0x00E8, 0x00E7, 0x00F7, 0x00DC } },
    { "1145",
      { 0x005B, 0x005D, 0x0024, 0x00F1, 0x0060, 0x00D1, 0x0040, 0x00E6,
        0x20AC, 0x00A8, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "1146",
      { 0x0024, 0x0021, 0x00A3, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x20AC, 0x00AF, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "1147",
      { 0x00B0, 0x00A7, 0x0024, 0x00F9, 0x00B5, 0x00A3, 0x00E0, 0x00E6,
        0x20AC, 0x00A8, 0x00E9, 0x00E8, 0x00E7, 0x00F7, 0x00DC } },
    { "1148",
      { 0x005B, 0x005D, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0x00E6,
        0x20AC, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "1149",
      { 0x00DE, 0x00C6, 0x0024, 0x00A6, 0x00F0, 0x0023, 0x00D0, 0x007D,
        0x20AC, 0x00F6, 0x00FE, 0x00E6, 0x00B4, 0x00F7, 0x00DC } },
    { "1153",
      { 0x005B, 0x005D, 0x0024, 0x007C, 0x0060, 0x0023, 0x0040, 0x0161,
        0x20AC, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x00DC } },
    { "1155",
      { 0x00C7, 0x011E, 0x0130, 0x015F, 0x0131, 0x00D6, 0x015E, 0x00E6,
        0x20AC, 0x00F6, 0x00E7, 0x011F, 0x00FC, 0x00F7, 0x0022 } },
    { "4971",
      { 0x005B, 0x005D, 0x0024, 0x007C, 0x0060, 0x0023, 0x0040, 0x03B9,
        0x03BC, 0x007E, 0x007B, 0x007D, 0x005C, 0xFFFD, 0x20AC } },
    { "12712",
      { 0x00A2, 0x0021, 0x0024, 0x00A6, 0x0060, 0x0023, 0x0040, 0x20AC,
        0x00A4, 0x007E, 0x007B, 0x007D, 0x005C, 0x00F7, 0x202B } },
    { "16804",
      { 0x00A2, 0x0021, 0x0024, 0x00A6, 0x060C, 0x0023, 0x0040, 0xFECB,
        0xFECE, 0x00F7, 0x061B, 0x061F, 0x00D7, 0x2007, 0x0667 } },
    // clang-format on
};

/* The CCSIDs which add the euro sign to another CCSID, that CCSID, and the
 * EBCDIC code of the euro sign. */
static const struct {
    const char* name;
    const char* base;
    unsigned char euro;
} euro_maps[] = {
    // clang-format off
    { "1140",  "37",   0x9F },
    { "1141",  "273",  0x9F },
    { "1142",  "277",  0x5A },
    { "1143",  "278",  0x5A },
    { "1144",  "280",  0x9F },
    { "1145",  "284",  0x9F },
    { "1146",  "285",  0x9F },
    { "1147",  "297",  0x9F },
    { "1148",  "500",  0x9F },
    { "1149",  "871",  0x9F },
    { "1153",  "870",  0x9F },
    { "1155",  "1026", 0x9F },
    { "4971",  "875",  0xFC },
    { "12712", "424",  0x9C },
    { "16804", "420",  0xFA }
    // clang-format on
};

#define N_ELEMENTS(array) ((int)(sizeof(array) / sizeof((array)[0])))

/* The characters which have the same EBCDIC code in every CCSID. */
static const char invariant_40[] = " ";
static const char invariant_4B[] = ".<(+";
static const char invariant_5C[] = "*);";
static const char invariant_60[] = "-/";
static const char invariant_6B[] = ",%_>?";
static const char invariant_C1[] = "ABCDEFGHI";
static const char invariant_D1[] = "JKLMNOPQR";
static const char invariant_E2[] = "STUVWXYZ";
static const char invariant_F0[] = "0123456789";

/* The lower case letters, which only CCSID 290 has elsewhere. */
static const char lower_81[] = "abcdefghi";
static const char lower_91[] = "jklmnopqr";
static const char lower_A2[] = "stuvwxyz";

/****i* tests/reference_decode
 * NAME
 *    reference_decode
 * SYNOPSIS
 *    ret = reference_decode (bytes, len, &uc);
 * DESCRIPTION
 *    What tn5250_utf8_decode is compared with: a decoder made straight
 *    from the table of well-formed UTF-8 byte sequences of the Unicode
 *    standard.  It tells a sequence is wrong as soon as it can.
 *****/
static int reference_decode(const unsigned char* s, int len, unsigned* uc) {
    unsigned char low = 0x80, high = 0xBF;
    unsigned c;
    int need, i;

    if (len <= 0) {
        return 0;
    }
    if (s[0] < 0x80) {
        *uc = s[0];
        return 1;
    }
    else if (s[0] >= 0xC2 && s[0] <= 0xDF) {
        need = 2;
        c = s[0] & 0x1F;
    }
    else if (s[0] >= 0xE0 && s[0] <= 0xEF) {
        need = 3;
        c = s[0] & 0x0F;
        if (s[0] == 0xE0) {
            low = 0xA0;
        }
        if (s[0] == 0xED) {
            high = 0x9F;
        }
    }
    else if (s[0] >= 0xF0 && s[0] <= 0xF4) {
        need = 4;
        c = s[0] & 0x07;
        if (s[0] == 0xF0) {
            low = 0x90;
        }
        if (s[0] == 0xF4) {
            high = 0x8F;
        }
    }
    else {
        return -1;
    }

    for (i = 1; i < need; i++) {
        if (i == len) {
            return 0;
        }
        if (s[i] < low || s[i] > high) {
            return -1;
        }
        c = (c << 6) | (s[i] & 0x3F);
        low = 0x80;
        high = 0xBF;
    }
    *uc = c;
    return need;
}

/* How many bytes the character which starts with this byte is made of, or
 * 0 if no character starts with it. */
static int sequence_length(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if (lead >= 0xC2 && lead <= 0xDF) {
        return 2;
    }
    if (lead >= 0xE0 && lead <= 0xEF) {
        return 3;
    }
    if (lead >= 0xF0 && lead <= 0xF4) {
        return 4;
    }
    return 0;
}

static void check_decode(const unsigned char* s, int len) {
    unsigned expected = 0xDEADBEEF;
    Tn5250Unicode decoded = 0xDEADBEEF;
    int ref = reference_decode(s, len, &expected);
    int ret = tn5250_utf8_decode((const char*)s, len, &decoded);
    int need = sequence_length(s[0]);

    /* A sequence which is not all there yet may be reported as such even
     * though it can be told already that it won't make a character. */
    if (ref == -1 && ret == 0 && len < need) {
        checks++;
        return;
    }
    check(ret == ref && decoded == expected,
          "decoding %02X %02X %02X %02X (%d bytes): %d U+%04X, not %d U+%04X",
          s[0], s[1], s[2], s[3], len, ret, decoded, ref, expected);
}

static void test_utf8(void) {
    /* The values of the last byte which are worth trying when all the
     * values of the other ones are. */
    static const unsigned char last[] = { 0x00, 0x7F, 0x80, 0x8F,
                                          0x90, 0xBF, 0xC0, 0xFF };
    unsigned char s[8];
    Tn5250Unicode uc, decoded;
    unsigned expected;
    unsigned b0, b1, b2;
    int len, want, i, n;

    /* Every character is encoded as it should, and decoded back. */
    for (uc = 0; uc < 0x110000; uc++) {
        if (uc >= 0xD800 && uc <= 0xDFFF) {
            continue;
        }
        want = uc < 0x80 ? 1 : uc < 0x800 ? 2 : uc < 0x10000 ? 3 : 4;
        memset(s, 0x55, sizeof(s));
        len = tn5250_utf8_encode(uc, (char*)s);
        expected = 0xDEADBEEF;
        if (len != want || s[len] != 0x55 ||
            reference_decode(s, len, &expected) != len || expected != uc) {
            check(0, "encoding U+%04X: %d bytes %02X %02X %02X %02X", uc, len,
                  s[0], s[1], s[2], s[3]);
            continue;
        }
        decoded = 0xDEADBEEF;
        n = tn5250_utf8_decode((const char*)s, len, &decoded);
        check(n == len && decoded == uc, "decoding U+%04X: %d U+%04X", uc, n,
              decoded);
        for (i = 1; i < len; i++) {
            decoded = 0xDEADBEEF;
            n = tn5250_utf8_decode((const char*)s, i, &decoded);
            if (n != 0 || decoded != 0xDEADBEEF) {
                check(0, "decoding %d of the %d bytes of U+%04X: %d", i, len,
                      uc, n);
            }
        }
    }

    /* What is not a character is not encoded. */
    memset(s, 0x55, sizeof(s));
    check(tn5250_utf8_encode(0xD800, (char*)s) == 0, "encoding U+D800");
    check(tn5250_utf8_encode(0xDFFF, (char*)s) == 0, "encoding U+DFFF");
    check(tn5250_utf8_encode(0x110000, (char*)s) == 0, "encoding U+110000");
    check(tn5250_utf8_encode(0xFFFFFFFFu, (char*)s) == 0, "encoding -1");
    check(s[0] == 0x55, "encoding nothing wrote something");

    /* Decoding, right or wrong: every sequence of up to three bytes, and
     * the ones of four bytes which start like a character. */
    for (b0 = 0; b0 < 0x100; b0++) {
        for (b1 = 0; b1 < 0x100; b1++) {
            for (b2 = 0; b2 < 0x100; b2++) {
                s[0] = (unsigned char)b0;
                s[1] = (unsigned char)b1;
                s[2] = (unsigned char)b2;
                s[3] = 0x80;
                for (len = (b2 == 0 ? 1 : 3); len <= 3; len++) {
                    check_decode(s, len);
                }
                if (b0 < 0xF0 || b0 > 0xF4) {
                    continue;
                }
                for (i = 0; i < N_ELEMENTS(last); i++) {
                    s[3] = last[i];
                    check_decode(s, 4);
                }
            }
        }
    }

    decoded = 0xDEADBEEF;
    check(tn5250_utf8_decode("A", 0, &decoded) == 0, "decoding no byte");
    check(tn5250_utf8_decode("A", -1, &decoded) == 0, "decoding -1 byte");
    check(decoded == 0xDEADBEEF, "decoding nothing stored something");
}

static void test_keys(void) {
    static const int keys[] = {
        // clang-format off
        K_ENTER, K_NEWLINE, K_TAB, K_BACKTAB, K_F1, K_F2, K_F3, K_F4, K_F5,
        K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_F13, K_F14, K_F15,
        K_F16, K_F17, K_F18, K_F19, K_F20, K_F21, K_F22, K_F23, K_F24,
        K_LEFT, K_RIGHT, K_UP, K_DOWN, K_ROLLDN, K_ROLLUP, K_BACKSPACE,
        K_HOME, K_END, K_INSERT, K_DELETE, K_RESET, K_PRINT, K_HELP,
        K_SYSREQ, K_CLEAR, K_REFRESH, K_FIELDEXIT, K_TESTREQ, K_TOGGLE,
        K_ERASE, K_ATTENTION, K_DUPLICATE, K_FIELDMINUS, K_FIELDPLUS,
        K_PREVWORD, K_NEXTWORD, K_PREVFLD, K_NEXTFLD, K_FIELDHOME, K_EXEC,
        K_MEMO, K_COPY_TEXT, K_PASTE_TEXT, K_UNKNOW, -1, 0
        // clang-format on
    };
    Tn5250Unicode uc;
    int i, key;

    for (uc = 0; uc < 0x110000; uc++) {
        key = K_UNICODE(uc);
        if (key <= 0 || !K_IS_UNICODE(key) || K_UNICODE_CHAR(key) != uc) {
            check(0, "K_UNICODE(U+%04X) is %d", uc, key);
        }
    }
    /* Characters of 8-bit terminals, and every other key there is. */
    for (key = -0x1000; key < 0x20000; key++) {
        if (K_IS_UNICODE(key)) {
            check(0, "key %d taken for a Unicode character", key);
        }
    }
    for (i = 0; i < N_ELEMENTS(keys); i++) {
        check(!K_IS_UNICODE(keys[i]), "key %d taken for a Unicode character",
              keys[i]);
    }
    check(K_UNICODE_CHAR('a') == 'a', "character of the plain key a");
    check(K_UNICODE_CHAR(0xE9) == 0xE9, "character of the plain key 0xE9");
    checks++;
}

static void check_invariant(Tn5250CharMap* map, int first, const char* text) {
    int i;

    for (i = 0; text[i] != '\0'; i++) {
        check(tn5250_char_map_to_unicode(map, (Tn5250Char)(first + i)) ==
                  (Tn5250Unicode)text[i],
              "%s: %02X is not %c", map->name, first + i, text[i]);
    }
}

/* Whether this is one of the maps which have the workaround of
 * tn5250_char_map_new for CCSID 870: it makes some characters of the local
 * character set translate to the same EBCDIC code. */
static int has_870_workaround(const char* name) {
    return !strcmp(name, "870") || !strcmp(name, "win870") ||
           !strcmp(name, "1153") || !strcmp(name, "win1153");
}

static void test_map(Tn5250CharMap* entry) {
    Tn5250CharMap* map = tn5250_char_map_new(entry->name);
    Tn5250CharMap* win;
    Tn5250Unicode uc;
    Tn5250Char ec;
    char name[32];
    int is_win = !strncmp(entry->name, "win", 3);
    int e, i, found;

    check(map == entry, "%s: not found by its name", entry->name);
    if (map != entry) {
        return;
    }
    check(map->to_remote_map != NULL && map->to_local_map != NULL &&
              map->to_unicode_map != NULL,
          "%s: missing table", map->name);

    /* NUL is shown as a blank and DUP as an asterisk. */
    check(tn5250_char_map_to_unicode(map, 0x00) == ' ', "%s: NUL", map->name);
    check(tn5250_char_map_to_unicode(map, 0x1C) == '*', "%s: DUP", map->name);
    check(tn5250_char_map_to_local(map, 0x00) == ' ', "%s: NUL", map->name);
    check(tn5250_char_map_to_local(map, 0x1C) == '*', "%s: DUP", map->name);

    check_invariant(map, 0x40, invariant_40);
    check_invariant(map, 0x4B, invariant_4B);
    check_invariant(map, 0x5C, invariant_5C);
    check_invariant(map, 0x60, invariant_60);
    check_invariant(map, 0x6B, invariant_6B);
    check_invariant(map, 0xC1, invariant_C1);
    check_invariant(map, 0xD1, invariant_D1);
    check_invariant(map, 0xE2, invariant_E2);
    check_invariant(map, 0xF0, invariant_F0);
    if (strcmp(map->name, "290") != 0) {
        check_invariant(map, 0x81, lower_81);
        check_invariant(map, 0x91, lower_91);
        check_invariant(map, 0xA2, lower_A2);
    }

    /* Every displayable character translates back to its EBCDIC code,
     * which also means that none comes twice. */
    for (e = 0x40; e < 0xFF; e++) {
        uc = tn5250_char_map_to_unicode(map, (Tn5250Char)e);
        check(uc == map->to_unicode_map[e], "%s: %02X is U+%04X", map->name, e,
              uc);
        if (uc == TN5250_UNICODE_REPLACEMENT) {
            continue;
        }
        check(uc >= 0x20 && (uc < 0x7F || uc >= 0xA0), "%s: %02X is U+%04X",
              map->name, e, uc);
        ec = (Tn5250Char)(e ^ 0xFF);
        found = tn5250_char_map_from_unicode(map, uc, &ec);
        check(found == 1 && ec == e, "%s: U+%04X is %02X, not %02X", map->name,
              uc, ec, e);
    }

    /* What is not a character of the map does not translate. */
    ec = 0xEE;
    check(!tn5250_char_map_from_unicode(map, TN5250_UNICODE_REPLACEMENT, &ec),
          "%s: U+FFFD translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0x00, &ec),
          "%s: U+0000 translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0x1F, &ec),
          "%s: U+001F translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0x7F, &ec),
          "%s: U+007F translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0x9F, &ec),
          "%s: U+009F translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0x4E2D, &ec),
          "%s: U+4E2D translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0x10041, &ec),
          "%s: U+10041 translates", map->name);
    check(!tn5250_char_map_from_unicode(map, 0xFFFFFFFFu, &ec),
          "%s: -1 translates", map->name);
    check(ec == 0xEE, "%s: a failed translation stored %02X", map->name, ec);

    /* The 8-bit tables go together.  In the Windows maps of the CCSIDs
     * which have a euro sign, local 0x80 is the euro sign whatever else
     * used to translate to it: see test_euro_map. */
    found = tn5250_char_map_from_unicode(map, 0x20AC, &ec);
    if (!has_870_workaround(map->name)) {
        for (e = 0x40; e < 0xFF; e++) {
            ec = tn5250_char_map_to_local(map, (Tn5250Char)e);
            if (is_win && found && ec == 0x80) {
                continue;
            }
            check(tn5250_char_map_to_remote(map, ec) == e,
                  "%s: %02X is local %02X, which is not %02X", map->name, e, ec,
                  e);
        }
    }

    if (is_win) {
        return;
    }

    /* This is the CCSID the name says. */
    found = 0;
    for (i = 0; i < N_ELEMENTS(signatures); i++) {
        if (strcmp(signatures[i].name, map->name) != 0) {
            continue;
        }
        found = 1;
        for (e = 0; e < SIGNATURE_SIZE; e++) {
            uc = map->to_unicode_map[signature_codes[e]];
            check(uc == signatures[i].unicode[e],
                  "%s: %02X is U+%04X, not U+%04X", map->name,
                  signature_codes[e], uc, signatures[i].unicode[e]);
        }
    }
    check(found, "%s: this test does not know this map", map->name);

    /* Its Windows map is about the same characters. */
    sprintf(name, "win%s", map->name);
    win = tn5250_char_map_new(name);
    if (strcmp(map->name, "290") == 0) {
        check(win == NULL, "%s: found", name);
    }
    else {
        check(win != NULL && !strcmp(win->name, name), "%s: not found", name);
        if (win != NULL) {
            check(win->to_unicode_map == map->to_unicode_map,
                  "%s: not the Unicode table of %s", name, map->name);
        }
    }
}

static void test_euro_map(int n) {
    Tn5250CharMap *map, *base, *win, *win_base;
    Tn5250Unicode uc;
    Tn5250Char ec;
    char name[32], base_name[32];
    int euro = euro_maps[n].euro;
    int e;

    sprintf(name, "win%s", euro_maps[n].name);
    sprintf(base_name, "win%s", euro_maps[n].base);
    map = tn5250_char_map_new(euro_maps[n].name);
    base = tn5250_char_map_new(euro_maps[n].base);
    win = tn5250_char_map_new(name);
    win_base = tn5250_char_map_new(base_name);
    check(map && base && win && win_base, "%s: missing map", name);
    if (!(map && base && win && win_base)) {
        return;
    }

    /* The euro sign is where it should be, and nowhere in the other
     * CCSID. */
    ec = 0;
    check(tn5250_char_map_to_unicode(map, (Tn5250Char)euro) == 0x20AC,
          "%s: %02X is not the euro sign", map->name, euro);
    check(tn5250_char_map_from_unicode(map, 0x20AC, &ec) && ec == euro,
          "%s: the euro sign is not %02X", map->name, euro);
    check(!tn5250_char_map_from_unicode(base, 0x20AC, &ec),
          "%s: has a euro sign", base->name);

    /* Every other character of the other CCSID is the same. */
    for (e = 0; e < 0x100; e++) {
        uc = base->to_unicode_map[e];
        if (e != euro && uc != TN5250_UNICODE_REPLACEMENT) {
            check(map->to_unicode_map[e] == uc,
                  "%s: %02X is U+%04X, not U+%04X as in %s", map->name, e,
                  map->to_unicode_map[e], uc, base->name);
        }
    }

    /* 8-bit terminals get what they got with the other CCSID, as the ISO
     * character sets have no euro sign... */
    for (e = 0; e < 0x100; e++) {
        check(tn5250_char_map_to_local(map, (Tn5250Char)e) ==
                  tn5250_char_map_to_local(base, (Tn5250Char)e),
              "%s: %02X is not the local character of %s", map->name, e,
              base->name);
        check(tn5250_char_map_to_remote(map, (Tn5250Char)e) ==
                  tn5250_char_map_to_remote(base, (Tn5250Char)e),
              "%s: local %02X is not the character of %s", map->name, e,
              base->name);
    }

    /* ... whereas it is 0x80 in the Windows code pages. */
    check(tn5250_char_map_to_local(win, (Tn5250Char)euro) == 0x80,
          "%s: the euro sign is local %02X", win->name,
          tn5250_char_map_to_local(win, (Tn5250Char)euro));
    check(tn5250_char_map_to_remote(win, 0x80) == euro, "%s: local 80 is %02X",
          win->name, tn5250_char_map_to_remote(win, 0x80));
    for (e = 0; e < 0x100; e++) {
        if (e != euro) {
            check(tn5250_char_map_to_local(win, (Tn5250Char)e) ==
                      tn5250_char_map_to_local(win_base, (Tn5250Char)e),
                  "%s: %02X is not the local character of %s", win->name, e,
                  win_base->name);
        }
        if (e != 0x80) {
            check(tn5250_char_map_to_remote(win, (Tn5250Char)e) ==
                      tn5250_char_map_to_remote(win_base, (Tn5250Char)e),
                  "%s: local %02X is not the character of %s", win->name, e,
                  win_base->name);
        }
    }
    check(tn5250_char_map_to_local(win_base, (Tn5250Char)euro) != 0x80 ||
              base->to_unicode_map[euro] == TN5250_UNICODE_REPLACEMENT,
          "%s: has a euro sign", win_base->name);
}

static void test_maps(void) {
    Tn5250CharMap* map;
    Tn5250CharMap legacy;
    Tn5250Char ec = 0xEE;
    int count = 0;
    int i, pass;

    /* Maps are set up as they are asked for, so do it all twice, the
     * second time the other way round. */
    for (pass = 0; pass < 2; pass++) {
        if (pass == 0) {
            for (map = tn5250_transmaps; map->name != NULL; map++) {
                test_map(map);
                count++;
            }
            for (i = 0; i < N_ELEMENTS(euro_maps); i++) {
                test_euro_map(i);
            }
        }
        else {
            for (i = N_ELEMENTS(euro_maps) - 1; i >= 0; i--) {
                test_euro_map(i);
            }
            for (map = tn5250_transmaps + count - 1; map >= tn5250_transmaps;
                 map--) {
                test_map(map);
            }
        }
    }
    check(count == 2 * N_ELEMENTS(signatures) - 1, "%d maps, not %d", count,
          2 * N_ELEMENTS(signatures) - 1);

    check(tn5250_char_map_new("nosuchmap") == NULL, "found a map");
    check(tn5250_char_map_new("") == NULL, "found a map without a name");
    check(tn5250_char_map_new("win") == NULL, "found a map called win");
    check(tn5250_char_map_new("1147 ") == NULL, "found a map with a blank");

    /* A map made by someone else has no Unicode table: its local
     * characters are taken for ISO-8859-1. */
    map = tn5250_char_map_new("37");
    memset(&legacy, 0, sizeof(legacy));
    legacy.name = "legacy";
    legacy.to_remote_map = map->to_remote_map;
    legacy.to_local_map = map->to_local_map;
    check(tn5250_char_map_to_unicode(&legacy, 0x51) == 0xE9, "legacy: 51");
    check(tn5250_char_map_to_unicode(&legacy, 0x00) == ' ', "legacy: NUL");
    check(tn5250_char_map_to_unicode(&legacy, 0x1C) == '*', "legacy: DUP");
    check(tn5250_char_map_from_unicode(&legacy, 0xE9, &ec) && ec == 0x51,
          "legacy: U+00E9 is %02X", ec);
    ec = 0xEE;
    check(!tn5250_char_map_from_unicode(&legacy, 0x20AC, &ec) && ec == 0xEE,
          "legacy: has a euro sign");
    check(!tn5250_char_map_from_unicode(&legacy, 0x1F, &ec) && ec == 0xEE,
          "legacy: U+001F translates");
}

static void test_case(void) {
    Tn5250CharMap* map;
    Tn5250Unicode uc, upper;
    int e;

    for (uc = 0; uc < 0x80; uc++) {
        check(tn5250_unicode_toupper(uc) == (Tn5250Unicode)toupper((int)uc),
              "upper case of U+%04X", uc);
        check(!tn5250_unicode_isalpha(uc) ==
                  !((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z')),
              "U+%04X is a letter or not", uc);
    }

    check(tn5250_unicode_toupper(0xE9) == 0xC9, "upper case of U+00E9");
    check(tn5250_unicode_toupper(0xC9) == 0xC9, "upper case of U+00C9");
    check(tn5250_unicode_toupper(0xDF) == 0xDF, "upper case of U+00DF");
    check(tn5250_unicode_toupper(0xFF) == 0xFF, "upper case of U+00FF");
    check(tn5250_unicode_toupper(0x3B1) == 0x391, "upper case of U+03B1");
    check(tn5250_unicode_toupper(0x3C2) == 0x3A3, "upper case of U+03C2");
    check(tn5250_unicode_toupper(0x430) == 0x410, "upper case of U+0430");
    check(tn5250_unicode_toupper(0x131) == 'I', "upper case of U+0131");
    check(tn5250_unicode_toupper(0x5D0) == 0x5D0, "upper case of U+05D0");
    check(tn5250_unicode_toupper(0x20AC) == 0x20AC, "upper case of U+20AC");
    check(tn5250_unicode_toupper(0x10FFFF) == 0x10FFFF, "upper case of max");
    check(tn5250_unicode_toupper(0xFFFFFFFFu) == 0xFFFFFFFFu,
          "upper case of -1");

    check(tn5250_unicode_isalpha(0xE9), "U+00E9 is a letter");
    check(tn5250_unicode_isalpha(0xDF), "U+00DF is a letter");
    check(tn5250_unicode_isalpha(0x3A9), "U+03A9 is a letter");
    check(tn5250_unicode_isalpha(0x44F), "U+044F is a letter");
    check(tn5250_unicode_isalpha(0x5D0), "U+05D0 is a letter");
    check(tn5250_unicode_isalpha(0x627), "U+0627 is a letter");
    check(tn5250_unicode_isalpha(0xFF71), "U+FF71 is a letter");
    check(!tn5250_unicode_isalpha(0xA3), "U+00A3 is not a letter");
    check(!tn5250_unicode_isalpha(0xD7), "U+00D7 is not a letter");
    check(!tn5250_unicode_isalpha(0xF7), "U+00F7 is not a letter");
    check(!tn5250_unicode_isalpha(0xB2), "U+00B2 is not a letter");
    check(!tn5250_unicode_isalpha(0x20AC), "U+20AC is not a letter");
    check(!tn5250_unicode_isalpha(0xFFFD), "U+FFFD is not a letter");
    check(!tn5250_unicode_isalpha(0xFFFFFFFFu), "-1 is not a letter");

    /* Whatever has an upper case is a letter, and so is its upper case,
     * which has no other upper case than itself. */
    for (map = tn5250_transmaps; map->name != NULL; map++) {
        for (e = 0x40; e < 0xFF; e++) {
            uc = map->to_unicode_map[e];
            upper = tn5250_unicode_toupper(uc);
            if (upper == uc) {
                continue;
            }
            check(tn5250_unicode_isalpha(uc) && tn5250_unicode_isalpha(upper),
                  "U+%04X or its upper case U+%04X is not a letter", uc, upper);
            check(tn5250_unicode_toupper(upper) == upper,
                  "U+%04X is the upper case of U+%04X and has another one",
                  upper, uc);
        }
    }
}

/* Print the character maps but the Windows ones: the Unicode characters,
 * the local characters, and the EBCDIC codes of the local characters. */
static void dump_maps(void) {
    Tn5250CharMap* map;
    int e;

    for (map = tn5250_transmaps; map->name != NULL; map++) {
        if (!strncmp(map->name, "win", 3)) {
            continue;
        }
        if (tn5250_char_map_new(map->name) != map) {
            continue;
        }
        printf("U %s", map->name);
        for (e = 0; e < 0x100; e++) {
            printf(" %04X", tn5250_char_map_to_unicode(map, (Tn5250Char)e));
        }
        printf("\nL %s", map->name);
        for (e = 0; e < 0x100; e++) {
            printf(" %02X", tn5250_char_map_to_local(map, (Tn5250Char)e));
        }
        printf("\nR %s", map->name);
        for (e = 0; e < 0x100; e++) {
            printf(" %02X", tn5250_char_map_to_remote(map, (Tn5250Char)e));
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    if (argc > 1 && !strcmp(argv[1], "--dump")) {
        dump_maps();
        return 0;
    }

    test_utf8();
    test_keys();
    test_maps();
    test_case();

    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}

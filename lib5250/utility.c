/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997-2008 Michael Madore
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
#include "tn5250-private.h"
#include "transmaps.h"
#include "unicodemaps.h"

#if defined(__SVR4) && defined(__sun)
#include <sys/filio.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIBSSL
#include <openssl/err.h>
#endif

static unsigned char mapfix[256];
static unsigned char mapfix2[256];
static unsigned char mapfix3[256];
static unsigned char mapfix4[256];

/* The 8-bit tables of the Windows maps whose CCSID has a euro sign, see
 * char_map_win_euro. */
#define WIN_EURO_MAPS 15
static unsigned char win_euro_tables[WIN_EURO_MAPS][2][256];

#define N_ELEMENTS(array) ((int)(sizeof(array) / sizeof((array)[0])))

/* The tables of transmaps.h which translate between an 8-bit character set
 * and EBCDIC. */
#define MAP_ISO(part, ebcdic)                                                  \
    iso_8859_##part##_to_##ebcdic, ebcdic##_to_iso_8859_##part
#define MAP_WIN(codepage, ebcdic)                                              \
    windows_##codepage##_to_##ebcdic, ebcdic##_to_windows_##codepage
#define MAP_JIS(ebcdic) jis_x0201_to_##ebcdic, ebcdic##_to_jis_x0201

/* This is the translation-map index which is scanned by
 * tn5250_char_map_new.  Each map has the tables which translate between the
 * local 8-bit character set and EBCDIC (from transmaps.h), and the table
 * which translates EBCDIC to Unicode (from unicodemaps.h).
 *
 * The CCSIDs which only add the euro sign to another CCSID have the 8-bit
 * tables of that CCSID.  As the ISO character sets have no euro sign, 8-bit
 * terminals get it where the currency sign used to be, which is how
 * ISO-8859-15 was made out of ISO-8859-1.  The Windows code pages do have a
 * euro sign, see char_map_win_euro.
 */
Tn5250CharMap tn5250_transmaps[] = {
    // clang-format off
    { "37",       MAP_ISO(1, ibm037),     ibm037_to_unicode   },
    { "256",      MAP_ISO(1, ibm256),     ibm256_to_unicode   },
    { "273",      MAP_ISO(1, ibm273),     ibm273_to_unicode   },
    { "277",      MAP_ISO(1, ibm277),     ibm277_to_unicode   },
    { "278",      MAP_ISO(1, ibm278),     ibm278_to_unicode   },
    { "280",      MAP_ISO(1, ibm280),     ibm280_to_unicode   },
    { "284",      MAP_ISO(1, ibm284),     ibm284_to_unicode   },
    { "285",      MAP_ISO(1, ibm285),     ibm285_to_unicode   },
    { "297",      MAP_ISO(1, ibm297),     ibm297_to_unicode   },
    { "500",      MAP_ISO(1, ibm500),     ibm500_to_unicode   },
    { "871",      MAP_ISO(1, ibm871),     ibm871_to_unicode   },
    { "870",      MAP_ISO(2, ibm870),     ibm870_to_unicode   },
    { "905",      MAP_ISO(3, ibm905),     ibm905_to_unicode   },
    { "880",      MAP_ISO(5, ibm880),     ibm880_to_unicode   },
    { "420",      MAP_ISO(6, ibm420),     ibm420_to_unicode   },
    { "875",      MAP_ISO(7, ibm875),     ibm875_to_unicode   },
    { "424",      MAP_ISO(8, ibm424),     ibm424_to_unicode   },
    { "1026",     MAP_ISO(9, ibm1026),    ibm1026_to_unicode  },
    { "290",      MAP_JIS(ibm290),        ibm290_to_unicode   },
    { "1140",     MAP_ISO(1, ibm037),     ibm1140_to_unicode  },
    { "1141",     MAP_ISO(1, ibm273),     ibm1141_to_unicode  },
    { "1142",     MAP_ISO(1, ibm277),     ibm1142_to_unicode  },
    { "1143",     MAP_ISO(1, ibm278),     ibm1143_to_unicode  },
    { "1144",     MAP_ISO(1, ibm280),     ibm1144_to_unicode  },
    { "1145",     MAP_ISO(1, ibm284),     ibm1145_to_unicode  },
    { "1146",     MAP_ISO(1, ibm285),     ibm1146_to_unicode  },
    { "1147",     MAP_ISO(1, ibm297),     ibm1147_to_unicode  },
    { "1148",     MAP_ISO(1, ibm500),     ibm1148_to_unicode  },
    { "1149",     MAP_ISO(1, ibm871),     ibm1149_to_unicode  },
    { "1153",     MAP_ISO(2, ibm870),     ibm1153_to_unicode  },
    { "1155",     MAP_ISO(9, ibm1026),    ibm1155_to_unicode  },
    { "4971",     MAP_ISO(7, ibm875),     ibm4971_to_unicode  },
    { "12712",    MAP_ISO(8, ibm424),     ibm12712_to_unicode },
    { "16804",    MAP_ISO(6, ibm420),     ibm16804_to_unicode },
    { "win37",    MAP_WIN(1252, ibm037),  ibm037_to_unicode   },
    { "win256",   MAP_WIN(1252, ibm256),  ibm256_to_unicode   },
    { "win273",   MAP_WIN(1252, ibm273),  ibm273_to_unicode   },
    { "win277",   MAP_WIN(1252, ibm277),  ibm277_to_unicode   },
    { "win278",   MAP_WIN(1252, ibm278),  ibm278_to_unicode   },
    { "win280",   MAP_WIN(1252, ibm280),  ibm280_to_unicode   },
    { "win284",   MAP_WIN(1252, ibm284),  ibm284_to_unicode   },
    { "win285",   MAP_WIN(1252, ibm285),  ibm285_to_unicode   },
    { "win297",   MAP_WIN(1252, ibm297),  ibm297_to_unicode   },
    { "win500",   MAP_WIN(1252, ibm500),  ibm500_to_unicode   },
    { "win871",   MAP_WIN(1252, ibm871),  ibm871_to_unicode   },
    { "win870",   MAP_WIN(1250, ibm870),  ibm870_to_unicode   },
    { "win905",   MAP_WIN(1254, ibm905),  ibm905_to_unicode   },
    { "win880",   MAP_WIN(1251, ibm880),  ibm880_to_unicode   },
    { "win420",   MAP_WIN(1256, ibm420),  ibm420_to_unicode   },
    { "win875",   MAP_WIN(1253, ibm875),  ibm875_to_unicode   },
    { "win424",   MAP_WIN(1255, ibm424),  ibm424_to_unicode   },
    { "win1026",  MAP_WIN(1254, ibm1026), ibm1026_to_unicode  },
    { "win1140",  MAP_WIN(1252, ibm037),  ibm1140_to_unicode  },
    { "win1141",  MAP_WIN(1252, ibm273),  ibm1141_to_unicode  },
    { "win1142",  MAP_WIN(1252, ibm277),  ibm1142_to_unicode  },
    { "win1143",  MAP_WIN(1252, ibm278),  ibm1143_to_unicode  },
    { "win1144",  MAP_WIN(1252, ibm280),  ibm1144_to_unicode  },
    { "win1145",  MAP_WIN(1252, ibm284),  ibm1145_to_unicode  },
    { "win1146",  MAP_WIN(1252, ibm285),  ibm1146_to_unicode  },
    { "win1147",  MAP_WIN(1252, ibm297),  ibm1147_to_unicode  },
    { "win1148",  MAP_WIN(1252, ibm500),  ibm1148_to_unicode  },
    { "win1149",  MAP_WIN(1252, ibm871),  ibm1149_to_unicode  },
    { "win1153",  MAP_WIN(1250, ibm870),  ibm1153_to_unicode  },
    { "win1155",  MAP_WIN(1254, ibm1026), ibm1155_to_unicode  },
    { "win4971",  MAP_WIN(1253, ibm875),  ibm4971_to_unicode  },
    { "win12712", MAP_WIN(1255, ibm424),  ibm12712_to_unicode },
    { "win16804", MAP_WIN(1256, ibm420),  ibm16804_to_unicode },
    { NULL,       NULL, NULL,             NULL                }
    // clang-format on
};

#ifndef _WIN32

/****f* lp5250d/tn5250_closeall
 * NAME
 *    tn5250_closeall
 * SYNOPSIS
 *    tn5250_closeall (fd);
 * INPUTS
 *    int fd	- The starting file descriptor.
 * DESCRIPTION
 *    Closes all file descriptors >= a specified value.
 *****/
void tn5250_closeall(int fd) {
    int fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit) {
        close(fd++);
    }
}

/*
  Signal handler for SIGCHLD.  We use waitpid instead of wait, since there
  is no way to tell wait not to block if there are still non-terminated child
  processes.
*/
void sig_child(int signum) {
    int pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        ;

    return;
}

/****f* lp5250d/tn5250_daemon
 * NAME
 *    tn5250_daemon
 * SYNOPSIS
 *    ret = tn5250_daemon (nochdir, noclose);
 * INPUTS
 *    int nochdir	- 0 to perform chdir.
 *    int noclose	- 0 to close all file handles.
 * DESCRIPTION
 *    Detach process from user and disappear into the background
 *    returns -1 on failure, but you can't do much except exit in that
 *    case since we may already have forked. Believed to work on all
 *    Posix systems.
 *****/
int tn5250_daemon(int nochdir, int noclose, int ignsigcld) {
    struct sigaction sa;

    switch (fork()) {
    case 0:
        break;
    case -1:
        return -1;
    default:
        _exit(0); /* exit the original process */
    }

    if (setsid() < 0) { /* shoudn't fail */
        return -1;
    }

    /* dyke out this switch if you want to acquire a control tty in */
    /* the future -- not normally advisable for daemons */

    switch (fork()) {
    case 0:
        break;
    case -1:
        return -1;
    default:
        _exit(0);
    }

    if (!nochdir) {
        chdir("/");
    }

    if (!noclose) {
        tn5250_closeall(0);
        open("/dev/null", O_RDWR);
        dup(0);
        dup(0);
    }

    umask(0);

    if (ignsigcld) {
        sa.sa_handler = sig_child;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

#ifdef SIGCHLD
        sigaction(SIGCHLD, &sa, NULL);
#else
#ifdef SIGCLD
        sigaction(SIGCLD, &sa, NULL);
#endif
#endif
    }

    return 0;
}

#endif /* ifndef _WIN32 */

static struct {
    Tn5250ErrorType type;
    unsigned long code; // for compat with OpenSSL
} tn5250_error;

void _tn5250_set_error(Tn5250ErrorType type, int code) {
    tn5250_error.type = type;
    tn5250_error.code = code;
}

int tn5250_has_error(void) { return tn5250_error.type != TN5250_ERROR_UNKNOWN; }

void tn5250_clear_error(void) { _tn5250_set_error(0, 0); }

const char* tn5250_strerror(void) {
    if (tn5250_error.type == TN5250_ERROR_ERRNO) {
        return strerror(tn5250_error.code);
    }
    else if (tn5250_error.type == TN5250_ERROR_GAI) {
        return gai_strerror(tn5250_error.code);
    }
#ifdef HAVE_LIBSSL
    else if (tn5250_error.type == TN5250_ERROR_SSL) {
        return ERR_error_string(tn5250_error.code, NULL);
    }
#endif
    else if (tn5250_error.type == TN5250_ERROR_INTERNAL) {
        switch (tn5250_error.code) {
        case TN5250_INTERNALERROR_INVALIDADDRESS:
            return "Invalid address";
        case TN5250_INTERNALERROR_INVALIDCERT:
            return "Certificate verification failure";
        }
    }
    return NULL;
}

/****f* lib5250/tn5250_char_map_to_remote
 * NAME
 *    tn5250_char_map_to_remote
 * SYNOPSIS
 *    ret = tn5250_char_map_to_remote (map,ascii);
 * INPUTS
 *    Tn5250Char           ascii      - The local character to translate.
 * DESCRIPTION
 *    Translate the specified character from local to remote.  The local
 *    character is in the 8-bit character set which goes with the map (such
 *    as ISO-8859-1), see tn5250_char_map_from_unicode for Unicode.
 *****/
Tn5250Char tn5250_char_map_to_remote(Tn5250CharMap* map, Tn5250Char ascii) {
    return map->to_remote_map[ascii];
}

/****f* lib5250/tn5250_char_map_to_local
 * NAME
 *    tn5250_char_map_to_local
 * SYNOPSIS
 *    local = tn5250_char_map_to_local (map, ebcdic);
 * INPUTS
 *    Tn5250Char           ebcdic     - The remote character to translate.
 * DESCRIPTION
 *    Translate the specified character from remote character to local.  The
 *    local character is in the 8-bit character set which goes with the map
 *    (such as ISO-8859-1), see tn5250_char_map_to_unicode for Unicode.
 *****/
Tn5250Char tn5250_char_map_to_local(Tn5250CharMap* map, Tn5250Char ebcdic) {
    switch (ebcdic) {
    case 0x1C:
        return '*'; /* This should be an overstriken asterisk (DUP) */
    case 0:
        return ' ';
    default:
        return map->to_local_map[ebcdic];
    }
}

/****f* lib5250/tn5250_char_map_to_unicode
 * NAME
 *    tn5250_char_map_to_unicode
 * SYNOPSIS
 *    uc = tn5250_char_map_to_unicode (map, ebcdic);
 * INPUTS
 *    Tn5250CharMap *      map        - The character map to use.
 *    Tn5250Char           ebcdic     - The remote character to translate.
 * DESCRIPTION
 *    Translate the specified character from remote character to Unicode.
 *    TN5250_UNICODE_REPLACEMENT is returned for the characters which are
 *    not defined by the character map.
 *****/
Tn5250Unicode tn5250_char_map_to_unicode(Tn5250CharMap* map,
                                         Tn5250Char ebcdic) {
    switch (ebcdic) {
    case 0x1C:
        return '*'; /* This should be an overstriken asterisk (DUP) */
    case 0:
        return ' ';
    default:
        /* Maps which weren't taken from tn5250_transmaps may lack a Unicode
         * table: the best guess for their local characters is ISO-8859-1. */
        if (map->to_unicode_map == NULL) {
            return map->to_local_map[ebcdic];
        }
        return map->to_unicode_map[ebcdic];
    }
}

/****f* lib5250/tn5250_char_map_from_unicode
 * NAME
 *    tn5250_char_map_from_unicode
 * SYNOPSIS
 *    if (tn5250_char_map_from_unicode (map, uc, &ebcdic))
 *       ;
 * INPUTS
 *    Tn5250CharMap *      map        - The character map to use.
 *    Tn5250Unicode        uc         - The Unicode character to translate.
 *    Tn5250Char *         ebcdic     - Where to store the remote character.
 * DESCRIPTION
 *    Translate the specified character from Unicode to remote character.
 *    Returns 1 if the character was translated, or 0 if the remote character
 *    set has no such character (in which case ebcdic is left alone).  Only
 *    the displayable characters are translated, not the control codes.
 *****/
int tn5250_char_map_from_unicode(Tn5250CharMap* map, Tn5250Unicode uc,
                                 Tn5250Char* ebcdic) {
    int i;

    if (uc < 0x20 || uc == TN5250_UNICODE_REPLACEMENT) {
        return 0;
    }

    if (map->to_unicode_map == NULL) {
        if (uc > 0xFF) {
            return 0;
        }
        *ebcdic = map->to_remote_map[uc];
        return 1;
    }

    /* Typing is slow enough for a linear search not to matter, and the
     * displayable characters are all between 0x40 and 0xFE. */
    for (i = 0x40; i < 0xFF; i++) {
        if (map->to_unicode_map[i] == uc) {
            *ebcdic = (Tn5250Char)i;
            return 1;
        }
    }
    return 0;
}

/****f* lib5250/tn5250_utf8_encode
 * NAME
 *    tn5250_utf8_encode
 * SYNOPSIS
 *    len = tn5250_utf8_encode (uc, buf);
 * INPUTS
 *    Tn5250Unicode        uc         - The Unicode character to encode.
 *    char *               utf8       - Buffer of at least TN5250_UTF8_MAX
 *                                      bytes.
 * DESCRIPTION
 *    Encode a Unicode character in UTF-8.  Returns the number of bytes
 *    stored in the buffer (which doesn't get a terminating NUL), or 0 if
 *    this is not a character which can be encoded.
 *****/
int tn5250_utf8_encode(Tn5250Unicode uc, char* utf8) {
    if (uc < 0x80) {
        utf8[0] = (char)uc;
        return 1;
    }
    if (uc < 0x800) {
        utf8[0] = (char)(0xC0 | (uc >> 6));
        utf8[1] = (char)(0x80 | (uc & 0x3F));
        return 2;
    }
    if (uc >= 0xD800 && uc <= 0xDFFF) {
        return 0; /* Surrogates aren't characters. */
    }
    if (uc < 0x10000) {
        utf8[0] = (char)(0xE0 | (uc >> 12));
        utf8[1] = (char)(0x80 | ((uc >> 6) & 0x3F));
        utf8[2] = (char)(0x80 | (uc & 0x3F));
        return 3;
    }
    if (uc < 0x110000) {
        utf8[0] = (char)(0xF0 | (uc >> 18));
        utf8[1] = (char)(0x80 | ((uc >> 12) & 0x3F));
        utf8[2] = (char)(0x80 | ((uc >> 6) & 0x3F));
        utf8[3] = (char)(0x80 | (uc & 0x3F));
        return 4;
    }
    return 0;
}

/****f* lib5250/tn5250_utf8_decode
 * NAME
 *    tn5250_utf8_decode
 * SYNOPSIS
 *    used = tn5250_utf8_decode (buf, len, &uc);
 * INPUTS
 *    const char *         utf8       - The bytes to decode.
 *    int                  len        - How many bytes there are.
 *    Tn5250Unicode *      uc         - Where to store the character.
 * DESCRIPTION
 *    Decode the UTF-8 character at the start of a buffer.  Returns the
 *    number of bytes this character is made of, 0 if the buffer ends before
 *    the character does (there might be more to come), or -1 if the buffer
 *    doesn't start with a valid character (in which case the first byte
 *    should be skipped).
 *****/
int tn5250_utf8_decode(const char* utf8, int len, Tn5250Unicode* uc) {
    const unsigned char* s = (const unsigned char*)utf8;
    Tn5250Unicode c, min;
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
        min = 0x80;
    }
    else if (s[0] >= 0xE0 && s[0] <= 0xEF) {
        need = 3;
        c = s[0] & 0x0F;
        min = 0x800;
    }
    else if (s[0] >= 0xF0 && s[0] <= 0xF4) {
        need = 4;
        c = s[0] & 0x07;
        min = 0x10000;
    }
    else {
        return -1;
    }

    for (i = 1; i < need; i++) {
        if (i == len) {
            return 0;
        }
        if ((s[i] & 0xC0) != 0x80) {
            return -1;
        }
        c = (c << 6) | (s[i] & 0x3F);
    }

    /* Reject what a strict decoder must: overlong forms, surrogates and
     * what is beyond Unicode. */
    if (c < min || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) {
        return -1;
    }

    *uc = c;
    return need;
}

/****f* lib5250/tn5250_unicode_toupper
 * NAME
 *    tn5250_unicode_toupper
 * SYNOPSIS
 *    uc = tn5250_unicode_toupper (uc);
 * INPUTS
 *    Tn5250Unicode        uc         - The Unicode character to convert.
 * DESCRIPTION
 *    Convert a character to upper case.  Unlike towupper(), this neither
 *    depends on the locale nor on wchar_t being Unicode, but it only knows
 *    the characters of the remote character sets.
 *****/
Tn5250Unicode tn5250_unicode_toupper(Tn5250Unicode uc) {
    int low = 0;
    int high = N_ELEMENTS(unicode_upper_map) - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (unicode_upper_map[mid][0] == uc) {
            return unicode_upper_map[mid][1];
        }
        if (unicode_upper_map[mid][0] < uc) {
            low = mid + 1;
        }
        else {
            high = mid - 1;
        }
    }
    return uc;
}

/****f* lib5250/tn5250_unicode_isalpha
 * NAME
 *    tn5250_unicode_isalpha
 * SYNOPSIS
 *    if (tn5250_unicode_isalpha (uc))
 *       ;
 * INPUTS
 *    Tn5250Unicode        uc         - The Unicode character to test.
 * DESCRIPTION
 *    Determines whether a character is alphabetic.  Unlike iswalpha(), this
 *    neither depends on the locale nor on wchar_t being Unicode, but it only
 *    knows the characters of the remote character sets.
 *****/
int tn5250_unicode_isalpha(Tn5250Unicode uc) {
    int low = 0;
    int high = N_ELEMENTS(unicode_alpha_ranges) - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (uc < unicode_alpha_ranges[mid][0]) {
            high = mid - 1;
        }
        else if (uc > unicode_alpha_ranges[mid][1]) {
            low = mid + 1;
        }
        else {
            return 1;
        }
    }
    return 0;
}

/****i* lib5250/char_map_win_euro
 * NAME
 *    char_map_win_euro
 * SYNOPSIS
 *    char_map_win_euro (map);
 * INPUTS
 *    Tn5250CharMap *      map        - One of tn5250_transmaps.
 * DESCRIPTION
 *    Unlike the ISO character sets, the Windows code pages have a euro
 *    sign, which is unknown to the 8-bit tables as they were made for
 *    CCSIDs without one.  If this is the Windows map of a CCSID which has
 *    a euro sign, give it 8-bit tables of its own which know about it.
 *
 *    The euro sign is 0x80 in the code pages of all the CCSIDs we have with
 *    a euro sign (1250 and 1252 to 1256).  Mind that it is 0x88 in 1251.
 *****/
static void char_map_win_euro(Tn5250CharMap* map) {
    Tn5250CharMap* t;
    unsigned char* to_remote;
    unsigned char* to_local;
    int euro = 0;
    int n = 0;
    int e;

    /* Each of these maps has its own tables: count the ones before it. */
    for (t = tn5250_transmaps; t->name != NULL; t++) {
        if (strncmp(t->name, "win", 3) != 0) {
            continue;
        }
        e = 0x40;
        while (e < 0xFF && t->to_unicode_map[e] != 0x20AC) {
            e++;
        }
        if (t == map) {
            euro = (e < 0xFF) ? e : 0;
            break;
        }
        if (e < 0xFF) {
            n++;
        }
    }
    if (euro == 0 || n >= WIN_EURO_MAPS) {
        return;
    }

    to_remote = win_euro_tables[n][0];
    to_local = win_euro_tables[n][1];
    if (map->to_remote_map != to_remote) {
        memcpy(to_remote, map->to_remote_map, 256);
        memcpy(to_local, map->to_local_map, 256);
        map->to_remote_map = to_remote;
        map->to_local_map = to_local;
    }
    to_remote[0x80] = (unsigned char)euro;
    to_local[euro] = 0x80;
}

/****f* lib5250/tn5250_char_map_new
 * NAME
 *    tn5250_char_map_new
 * SYNOPSIS
 *    cmap = tn5250_char_map_new ("37");
 * INPUTS
 *    const char *         map        - Name of the character translation map.
 * DESCRIPTION
 *    Create a new translation map.
 * NOTES
 *    Translation maps are currently statically allocated, although you should
 *    call tn5250_char_map_destroy (a no-op) for future compatibility.
 *****/
Tn5250CharMap* tn5250_char_map_new(const char* map) {
    Tn5250CharMap* t;

    /* XXX: HACK: These characters were reported wrong in transmaps.h.
            Since that's a generated file, I'm overriding them here -SCK */

    TN5250_LOG(("tn5250_char_map_new: map = \"%s\"\n", map));

    /* 1153 is 870 with a euro sign, and has the same 8-bit tables. */
    if (!strcmp(map, "870") || !strcmp(map, "win870") || !strcmp(map, "1153") ||
        !strcmp(map, "win1153")) {

        TN5250_LOG(("tn5250_char_map_new: Installing 870 workaround\n"));

        memcpy(mapfix, windows_1250_to_ibm870, sizeof(mapfix));
        memcpy(mapfix2, ibm870_to_windows_1250, sizeof(mapfix2));
        memcpy(mapfix3, iso_8859_2_to_ibm870, sizeof(mapfix3));
        memcpy(mapfix4, ibm870_to_iso_8859_2, sizeof(mapfix4));

        mapfix[142] = 184;
        mapfix[143] = 185;
        mapfix[158] = 182;
        mapfix[159] = 183;
        mapfix[163] = 186;
        mapfix[202] = 114;
        mapfix[234] = 82;

        mapfix2[82] = 234;
        mapfix2[114] = 202;
        mapfix2[182] = 158;
        mapfix2[183] = 159;
        mapfix2[184] = 142;
        mapfix2[185] = 143;
        mapfix2[186] = 163;

        mapfix3[163] = 186;
        mapfix3[172] = 185;
        mapfix3[188] = 183;
        mapfix3[202] = 114;
        mapfix3[234] = 82;

        mapfix4[82] = 234;
        mapfix4[114] = 202;
        mapfix4[183] = 188;
        mapfix4[185] = 172;
        mapfix4[186] = 163;

        for (t = tn5250_transmaps; t->name; t++) {
            if (!strcmp(t->name, "win870") || !strcmp(t->name, "win1153")) {
                t->to_remote_map = mapfix;
                t->to_local_map = mapfix2;
                /* This just took its euro sign away from win1153. */
                char_map_win_euro(t);
                TN5250_LOG(("Workaround installed for map \"%s\"\n", t->name));
            }
            else if (!strcmp(t->name, "870") || !strcmp(t->name, "1153")) {
                t->to_remote_map = mapfix3;
                t->to_local_map = mapfix4;
                TN5250_LOG(("Workaround installed for map \"%s\"\n", t->name));
            }
        }
    }

    /* Under Windows, we'll try the "winXXX" maps first, then fall back
       to the standard (unix) versions */
#ifdef _WIN32
    {
        char winmap[10];
        _snprintf(winmap, sizeof(winmap) - 1, "win%s", map);
        for (t = tn5250_transmaps; t->name; t++) {
            if (strcmp(t->name, winmap) == 0) {
                TN5250_LOG(("Using map %s\n", t->name));
                char_map_win_euro(t);
                return t;
            }
        }
    }
#endif

    for (t = tn5250_transmaps; t->name; t++) {
        if (strcmp(t->name, map) == 0) {
            char_map_win_euro(t);
            return t;
        }
    }
    return NULL;
}

/****f* lib5250/tn5250_char_map_destroy
 * NAME
 *    tn5250_char_map_destroy
 * SYNOPSIS
 *    tn5250_char_map_destroy (map);
 * INPUTS
 *    Tn5250CharMap *         map     - The character map to destroy.
 * DESCRIPTION
 *    Frees the character map's resources.
 *****/
void tn5250_char_map_destroy(Tn5250CharMap* map) { /* NOOP */
}

/****f* lib5250/tn5250_char_map_printable_p
 * NAME
 *    tn5250_char_map_printable_p
 * SYNOPSIS
 *    if (tn5250_char_map_printable_p (map,ec))
 *	 ;
 * INPUTS
 *    Tn5250CharMap *      map        - the character map to use.
 *    Tn5250Char           ec         - the character to test.
 * DESCRIPTION
 *    Determines whether the specified character is printable, which means
 *    either it is a displayable EBCDIC character, an ideographic control
 *    character, a NUL, or a few other odds and ends.
 * SOURCE
 */
int tn5250_char_map_printable_p(Tn5250CharMap* map, Tn5250Char data) {
    switch (data) {
        /*
           Ideographic Shift-In and Shift-Out.
           case 0x0e:
           case 0x0f:
        */
    default:
        break;
    }
    return 1;
}
/*******/

/****f* lib5250/tn5250_char_map_attribute_p
 * NAME
 *    tn5250_char_map_attribute_p
 * SYNOPSIS
 *    ret = tn5250_char_map_attribute_p (map,ec);
 * INPUTS
 *    Tn5250CharMap *      map        - the translation map to use.
 *    Tn5250Char           ec         - the character to test.
 * DESCRIPTION
 *    Determines whether the character is a 5250 attribute.
 *****/
int tn5250_char_map_attribute_p(Tn5250CharMap* map, Tn5250Char data) {
    return ((data & 0xE0) == 0x20);
}

#ifndef NDEBUG
FILE* tn5250_logfile = NULL;

/****f* lib5250/tn5250_log_open
 * NAME
 *    tn5250_log_open
 * SYNOPSIS
 *    tn5250_log_open (fname);
 * INPUTS
 *    const char *         fname      - Filename of tracefile.
 * DESCRIPTION
 *    Opens the debug tracefile for this session.
 *****/
void tn5250_log_open(const char* fname) {
    if (tn5250_logfile != NULL) {
        fclose(tn5250_logfile);
    }
    tn5250_logfile = fopen(fname, "w");
    if (tn5250_logfile == NULL) {
        perror(fname);
        exit(1);
    }
    /* FIXME: Write $TERM, version, and uname -a to the file. */
#ifndef _WIN32
    /* Set file mode to 0600 since it may contain passwords. */
    fchmod(fileno(tn5250_logfile), 0600);
#endif
    setbuf(tn5250_logfile, NULL);
}

/****f* lib5250/tn5250_log_close
 * NAME
 *    tn5250_log_close
 * SYNOPSIS
 *    tn5250_log_close ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    Close the current tracefile if one is open.
 *****/
void tn5250_log_close() {
    if (tn5250_logfile != NULL) {
        fclose(tn5250_logfile);
        tn5250_logfile = NULL;
    }
}

/****f* lib5250/tn5250_log_printf
 * NAME
 *    tn5250_log_printf
 * SYNOPSIS
 *    tn5250_log_printf (fmt, );
 * INPUTS
 *    const char *         fmt        -
 * DESCRIPTION
 *    This is an internal function called by the TN5250_LOG() macro.  Use
 *    the macro instead, since it can be conditionally compiled.
 *****/
void tn5250_log_printf(const char* fmt, ...) {
    va_list vl;
    if (tn5250_logfile != NULL) {
        va_start(vl, fmt);
        vfprintf(tn5250_logfile, fmt, vl);
        va_end(vl);
    }
}

/****f* lib5250/tn5250_log_assert
 * NAME
 *    tn5250_log_assert
 * SYNOPSIS
 *    tn5250_log_assert (val, expr, file, line);
 * INPUTS
 *    int                  val        -
 *    char const *         expr       -
 *    char const *         file       -
 *    int                  line       -
 * DESCRIPTION
 *    This is an internal function called by the TN5250_ASSERT() macro.  Use
 *    the macro instead, since it can be conditionally compiled.
 *****/
void tn5250_log_assert(int val, char const* expr, char const* file, int line) {
    if (!val) {
        tn5250_log_printf("\nAssertion %s failed at %s, line %d.\n", expr, file,
                          line);
        fprintf(stderr, "\nAssertion %s failed at %s, line %d.\n", expr, file,
                line);
        abort();
    }
}
#endif /* NDEBUG */

/****f* lib5250/tn5250_parse_color
 * NAME
 *    tn5250_parse_color
 * SYNOPSIS
 *    tn5250_parse_color (config, "green", &red, &green, &blue);
 * INPUTS
 *    Tn5250Config *       config     -
 *    const char   *       colorname  -
 *    int          *       red        -
 *    int          *       green      -
 *    int          *       blue       -
 * DESCRIPTION
 *    This loads a color from the TN5250 config object, and then
 *    parses it into it's red, green, blue components.
 *****/
int tn5250_parse_color(Tn5250Config* config, const char* colorname, int* red,
                       int* green, int* blue) {

    const char* p;
    char colorspec[16];
    int r, g, b;

    if ((p = tn5250_config_get(config, colorname)) == NULL) {
        return -1;
    }

    strncpy(colorspec, p, sizeof(colorspec));
    colorspec[sizeof(colorspec) - 1] = '\0';

    if (*colorspec != '#') {
        if (!strcasecmp(colorspec, "white")) {
            r = 255;
            g = 255;
            b = 255;
        }
        else if (!strcasecmp(colorspec, "yellow")) {
            r = 255;
            g = 255;
            b = 0;
        }
        else if (!strcasecmp(colorspec, "lightmagenta")) {
            r = 255;
            g = 0;
            b = 255;
        }
        else if (!strcasecmp(colorspec, "lightred")) {
            r = 255;
            g = 0;
            b = 0;
        }
        else if (!strcasecmp(colorspec, "lightcyan")) {
            r = 0;
            g = 255;
            b = 255;
        }
        else if (!strcasecmp(colorspec, "lightgreen")) {
            r = 0;
            g = 255;
            b = 0;
        }
        else if (!strcasecmp(colorspec, "lightblue")) {
            r = 0;
            g = 0;
            b = 255;
        }
        else if (!strcasecmp(colorspec, "lightgray")) {
            r = 192;
            g = 192;
            b = 192;
        }
        else if (!strcasecmp(colorspec, "gray")) {
            r = 128;
            g = 128;
            b = 128;
        }
        else if (!strcasecmp(colorspec, "brown")) {
            r = 128;
            g = 128;
            b = 0;
        }
        else if (!strcasecmp(colorspec, "red")) {
            r = 128;
            g = 0;
            b = 0;
        }
        else if (!strcasecmp(colorspec, "cyan")) {
            r = 0;
            g = 128;
            b = 128;
        }
        else if (!strcasecmp(colorspec, "green")) {
            r = 0;
            g = 128;
            b = 0;
        }
        else if (!strcasecmp(colorspec, "blue")) {
            r = 0;
            g = 0;
            b = 128;
        }
        else if (!strcasecmp(colorspec, "black")) {
            r = 0;
            g = 0;
            b = 0;
        }
    }
    else {
        if (strlen(colorspec) != 7) {
            return -1;
        }
        if (sscanf(&colorspec[1], "%02x%02x%02x", &r, &g, &b) != 3) {
            return -1;
        }
    }

    *red = r;
    *green = g;
    *blue = b;
    return 0;
}

/****f* lib5250/tn5250_run_cmd
 * NAME
 *    tn5250_run_cmd
 * SYNOPSIS
 *    tn5250_run_cmd ("notepad", 0);
 * INPUTS
 *    const char   *       cmd        -
 *    int                  wait       -
 * DESCRIPTION
 *    Run a command (submitted to us by the AS/400)
 *****/
#ifndef _WIN32
int tn5250_run_cmd(const char* cmd, int wait) {

    struct sigaction sa;
    int sig;
    int cpid;

    sa.sa_handler = sig_child;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

#ifdef SIGCHLD
    sigaction(SIGCHLD, &sa, NULL);
#else
#ifdef SIGCLD
    sigaction(SIGCLD, &sa, NULL);
#endif
#endif

    switch (cpid = fork()) {
    case -1:
        return -1;

    case 0:
        system(cmd);
        _exit(0);
    }

    if (wait) {
        waitpid(cpid, NULL, 0);
    }

    return 0;
}
#endif

#ifdef _WIN32
int win32_run_process(const char* cmd, int wait);

int tn5250_run_cmd(const char* cmd, int wait) {

    char* tempcmd;
    char* prefix = "cmd.exe /c ";
    int rc;

    rc = win32_run_process(cmd, wait);

    if (rc == -1) {

        tempcmd = malloc(strlen(prefix) + strlen(cmd) + 1);
        strcpy(tempcmd, prefix);
        strcat(tempcmd, cmd);

        rc = win32_run_process(tempcmd, wait);
        free(tempcmd);
    }

    return rc;
}

int win32_run_process(const char* cmd, int wait) {

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL,        /* no module name */
                       (LPTSTR)cmd, /* command line */
                       NULL,        /* don't inherit process bandle */
                       NULL,        /* don't inherit thread handle */
                       FALSE,       /* no inheritance, by golly! */
                       0,           /* flags */
                       NULL,        /* use parent's environment */
                       NULL,        /* use parent's directory */
                       &si,         /* STARTUPINFO */
                       &pi)) {      /* PROCESS_INFORMATION */
        return -1;
    }

    if (wait) {
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

#endif /* ifdef _WIN32 */

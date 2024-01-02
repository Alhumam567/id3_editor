/* C code produced by gperf version 3.1 */
/* Command-line: gperf --delimiters=, -L C -m 10000 --output-file=id3_hash.c id3_keys.txt  */
/* Computed positions: -k'1-4' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif


#define TOTAL_KEYWORDS 84
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 5
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 129
/* maximum key range = 129, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register size_t len;
{
  static unsigned char asso_values[] =
    {
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130,   0, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
       40,  14,  26,  43, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130,  28,  44,   9,   3,   8,
       41,  33,  35,  46,  69,  18,  18,   6,  23,   1,
        2,  62,   4,   0,  14,  29,  35,  13,  23,  13,
       49, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130, 130, 130, 130,
      130, 130, 130, 130, 130, 130, 130
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]+1];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

const char *
in_word_set (str, len)
     register const char *str;
     register size_t len;
{
  static const char * wordlist[] =
    {
      "",
      "\015",
      "", "", "", "", "", "", "", "", "",
      "", "", "", "", "", "", "", "", "",
      "",
      "COMR\015",
      "POSS\015",
      "TDOR\015",
      "TRSN\015",
      "TRSO\015",
      "TSRC\015",
      "TPRO\015",
      "TMOO\015",
      "TDRC\015",
      "TCON\015",
      "TDEN\015",
      "TDRL\015",
      "POPM\015",
      "TOWN\015",
      "SYTC\015",
      "TPOS\015",
      "WORS\015",
      "ETCO\015",
      "SEEK\015",
      "TMCL\015",
      "TMED\015",
      "USER\015",
      "TPE2\015",
      "COMM\015",
      "ENCR\015",
      "TLEN\015",
      "WOAR\015",
      "TSST\015",
      "TSOT\015",
      "TRCK\015",
      "WCOM\015",
      "TCOM\015",
      "TENC\015",
      "TOAL\015",
      "TPE3\015",
      "GEOB\015",
      "RVRB\015",
      "WPUB\015",
      "TPUB\015",
      "TSSE\015",
      "WOAS\015",
      "TOFN\015",
      "TOPE\015",
      "TSOA\015",
      "SYLT\015",
      "TLAN\015",
      "AENC\015",
      "PCNT\015",
      "TPE1\015",
      "PRIV\015",
      "TDTG\015",
      "TPE4\015",
      "TIPL\015",
      "TALB\015",
      "",
      "MLLT\015",
      "WXXX\015",
      "TXXX\015",
      "TEXT\015",
      "WOAF\015",
      "USLT\015",
      "TSOP\015",
      "OWNE\015",
      "APIC\015",
      "SIGN\015",
      "RVA2\015",
      "TOLY\015",
      "TBPM\015",
      "TDLY\015",
      "WCOP\015",
      "TCOP\015",
      "MCDI\015",
      "TIT2\015",
      "TKEY\015",
      "",
      "GRID\015",
      "WPAY\015",
      "", "", "", "", "", "",
      "ASPI\015",
      "TIT3\015",
      "",
      "TFLT\015",
      "", "",
      "LINK\015",
      "", "", "", "",
      "RBUF\015",
      "", "",
      "EQU2\015",
      "TIT1\015",
      "", "", "", "", "", "", "", "", "",
      "UFID\015"
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key];

          if (*str == *s && !strcmp (str + 1, s + 1))
            return s;
        }
    }
  return 0;
}

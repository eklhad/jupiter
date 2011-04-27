/*********************************************************************

jup.h: header file for the Jupiter speech system.

Copyright (C) Karl Dahlke, 2011.
This software may be freely distributed under the GPL, general public license,
as articulated by the Free Software Foundation.

*********************************************************************/

#ifndef JUP_H
#define JUP_H 1

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "acsbridge.h"

#define stringEqual !strcmp

/* Sizes and limits. */
/* WORDLEN comes from acsbridge.h */
#define NEWWORDLEN 200 /* size of word or number after expansion */
#define SENTENCEWORDS 20 /* max number of words in a sentence */

/* Coded constructs such as date, time, state, etc. */
/* This is generally assigned to, and compared against, a char */
#define SP_MARK ((char)0x80)

enum sp_codes {
SP_NONE,
SP_REPEAT,
SP_DATE,
SP_TIME,
SP_PHONE, // USA phone numbers only
SP_LI, // list item
SP_EMOT,
SP_FRAC,
SP_WDAY,
SP_STATE,
SP_BIBLE,
SP_URL,
SP_DF, // date or fraction
};


/* Global variables */

struct textbuf {
	char *buf;
	ofs_type *offset;
	unsigned short room;
	unsigned short len;
};

extern struct textbuf *j_in, *j_out;

#define appendBackup() (--j_out->len)
/* case independent character compare */
#define ci_cmp(x, y) (tolower(x) != tolower(y))

extern char relativeDate;
extern char showZones;
extern int myZone; /* offset from gmt */
extern char digitWords; /* read digits as words */
extern char acronUpper; /* acronym letters in upper case? */
extern char acronDelim;
extern char oneSymbol; /* read one symbol - not a sentence */
extern char readLiteral; // read each punctuation mark
/* a convenient place to put little phrases to speak */
extern char shortPhrase[NEWWORDLEN];

/* prototypes */

/* sourcefile=jencode.c */
void ascify(void) ;
void doWhitespace(void) ;
void ungarbage(void) ;
void titles(void) ;
void sortReservedWords(void) ;
void listItem(void) ;
void doEncode(void) ;

/* sourcefile=jxlate.c */
int setupTTS(void) ;
void speakChar(int c, int sayit, int bellsound, int asword) ;
void textBufSwitch(void) ;
void carryOffsetForward(const char *s) ;
void textbufClose(const char *s, int overflow) ;
int isvowel(char c) ;
int case_different(char x, char y) ;
int isSubword(const char *s, const char *t) ;
int wordInList(const char * const *list, const char *s, int s_len) ;
int appendChar(char c) ;
int appendString(const char *s) ;
void lastUncomma(void) ;
int alphaLength(const char *s) ;
int atoiLength(const char *s, int len) ;
void prepTTS(void) ;
char *prepTTSmsg(const char *msg) ;

#endif

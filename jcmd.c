/*********************************************************************

jcmd.c: jupiter speech commands.

Copyright (C) Karl Dahlke, 2011.
This software may be freely distributed under the GPL, general public license,
as articulated by the Free Software Foundation.
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "jup.h"


// Speech command structure, one instance for each jupiter command.
struct cmd {
	const char *desc; // description
	const char brief[12]; // brief name for the command
	char nonempty; // buffer must be nonempty
	char endstring; // must be last in a command sequence
	char nextchar; // needs next key to complete command
	char nextline; // needs line of text to complete command
};

// the available speech commands
static const struct cmd speechcommands[] = {
	{0,""}, // 0 is not a function
	{"clear buffer","clbuf"},
	{"visual cursor","cursor",1},
	{"start of buffer","sbuf",1},
	{"end of buffer","ebuf",1},
	{"start of line","sline",1},
	{"end of line","eline",1},
	{"start of word","sword",1},
	{"end of word","eword",1},
	{"left spaces","lspc",1},
	{"right spaces","rspc",1},
	{"back one character","back",1},
	{"forward one character","for",1},
	{"preivious row","prow",1},
	{"next row","nrow",1},
	{"reed the current karecter as a word","asword",1,1},
	{"reed the current karecter","char",1,1},
	{"upper or lower case","case",1,1},
	{"current cohllumm number","colnum",1,1},
	{"reed the current word","word",1,1},
	{"start reeding","read",1,1},
	{"stop speaking","shutup"},
	{"pass next karecter threw","bypass",0,1,1},
	{"clear bighnary mode","clmode",0,0,1},
	{"set bighnary mode","stmode",0,0,1},
	{"toggle bighnary mode","toggle",0,0,1},
	{"serch up","searchu",1,1,0,1},
	{"serch down","searchd",1,1,0,1},
	{"set volume","volume",0,0,1},
{"increase volume", "incvol"},
{"decrease volume", "decvol"},
	{"set speed","speed",0,0,1},
{"increase speed", "incspd"},
{"decrease speed", "decspd"},
	{"set pitch","pitch",0,0,1},
{"increase pitch", "incpch"},
{"decrease pitch", "decpch"},
{"set voice", "voice", 0, 0, 1},
	{"key binding","bind",0,1,0,1},
	{"last complete line","lcline",1},
{"mark left", "markl", 1},
{"mark right", "markr", 1, 0, 1},
{"set echo", "echo", 0, 0, 1},
{"label", "label", 1, 0, 1},
{"jump", "jump", 1, 0, 1},
	{0,""}
};

static short const max_cmd = sizeof(speechcommands)/sizeof(struct cmd) - 1;

// derive a command code from its brief name
static int
cmdByName(const char *name)
{
	const char *t;
	const struct cmd *cmdp = &speechcommands[1];

	while(*(t = cmdp->brief)) {
		if(stringEqual(name, t)) return cmdp - speechcommands;
		++cmdp;
}

	return 0;
} // cmdByName

// Return the composite status of an atomic command.
//  1  must end the composite.
//  2  requires follow-on char.
//  4  requires follow-on string -- stands alone.
static unsigned
compStatus(int cmd)
{
	unsigned compstat = 0;
	const struct cmd *cmdp = speechcommands + cmd;
	if(cmdp->endstring) compstat |= 1;
	if(cmdp->nextchar) compstat |= 2;
	/* Follow-on string always ends the composite. */
	if(cmdp->nextline) compstat |= 1;
	if(cmd == 22) compstat = 5; /* bypass */
	return compstat;
} // compStatus

static int last_cmd_index(const char *list)
{
	int i;
	char cmd;
	const struct cmd *cmdp;
	for(i=0; list[i]; ++i) {
		cmd = list[i];
		cmdp = &speechcommands[cmd];
		if(cmdp->nextchar && list[i+1]) {
			if(!list[i+2]) return i;
			++i;
		}
	}
	return i-1;
} // last_cmd_index

static char *last_atom;
static int cfg_syntax(char *s)
{
char *v = s;
char *s0 = s;
char *t;
char mustend = 0, nextchar = 0;
char c;
int cmd;
struct cmd *cmdp;
unsigned compstat;

	// look up each command designator
	while(c = *s) {
if(c == ' ' || c == '\t') { ++s; continue; }

		if(mustend) return -2;
		t = strpbrk(s, " \t");
		if(t) *t = 0;
		if(nextchar) {
			if(c < 0 || !isalnum(c) || s[1]) return -3;
*v++ = c;
			nextchar = 0;
		} else {
			cmd = cmdByName(s);
last_atom = s;
			if(!cmd) return -4;
			compstat = compStatus(cmd);
			if(compstat&4 && v > s0) return -5;
			if(compstat & 1) mustend = 1;
			if(compstat & 2) nextchar = 1;
*v++ = cmd;
		}

		if(!t) break;
		s = ++t;
	}

*v = 0;
	return 0;
} // cfg_syntax

// configure the jupiter system.
static void
j_configure(const char *filename)
{
FILE *f;
char line[200];
char *s;
int lineno, rc;

f = fopen(filename, "r");
if(!f) {
fprintf(stderr, "cannot open config file %s\n", filename);
return;
}

lineno = 0;
while(fgets(line, sizeof(line), f)) {
++lineno;

// strip off crlf
s = line + strlen(line);
if(s > line && s[-1] == '\n') --s;
if(s > line && s[-1] == '\r') --s;
*s = 0;

if(rc = acs_line_configure(line, cfg_syntax)) {
fprintf(stderr, "%s line %d: ", filename, lineno);
switch(rc) {
case -2:
fprintf(stderr, "%s cannot be in the middle of a composite speech command", last_atom);
break;
case -3:
fprintf(stderr, "%s must be followed by a letter or digit", last_atom);
break;
case -4:
fprintf(stderr, "%s is not a recognized speech command", last_atom);
break;
case -5:
fprintf(stderr, "bypass cannot be mixed with any other commands");
break;
default:
fprintf(stderr, "syntax error");
} // switch
fprintf(stderr, "\n");
}
}
fclose(f);
} // j_configure


/*********************************************************************
Set, clear, or toggle a binary mode based on the follow-on character.
Also generate the appropriate sound.
For instance, ^G beep if the character doesn't correspond
to a boolean mode in the system.
*********************************************************************/

static char clicksOn = 1; // clicks accompany tty output
static char autoRead; // read new text automatically
static char oneLine; /* read one line at a time */
static char transparent; // pass through
static char overrideSignals = 0; // don't rely on cts rts etc
static char reading; /* continuous reading in progress */
/* for cut&paste */
#define markleft rb->marks[26]
static unsigned int *markright;
static char screenmode = 0;
static char cc_buffer = 0; // control chars in the buffer
static char echoMode; // echo keys as they are typed
static char indexmarkers = 1; // use index markers for the synthesizer
/* buffer for cut&paste */
static char cutbuf[10000];

static void binmode(int action, int c, int quiet)
{
	char *p;

	switch(c) {
	case 'e': p = &echoMode; break;
	case 'n': p = &clicksOn; break;
	case 'a': p = &autoRead; break;
	case '1': p = &oneLine; break;
	case 'o': p = &overrideSignals; break;
case 's': markleft = 0; p = &screenmode; break;
	case 'c': p = &cc_buffer; break;
	case 'l': p = &readLiteral; break;
/* don't see the point of this one
	case 'x': p = &indexmarkers; break;
*/
	default: acs_bell(); return;
} // switch

	if(action == 0) *p = 0;
	if(action == 1) *p = 1;
	if(action == 2) *p ^= 1;
	if(!quiet) acs_tone_onoff(*p);

switch(c) {
case 'n':
acs_sounds(*p);
/* If turning sounds on, then the previous tone didn't take. */
if(!quiet && *p) acs_tone_onoff(1);
break;
case 'o': ess_flowcontrol(1-*p); break;
case 's': acs_screenmode(*p); break;
case 'c':
if(*p) acs_postprocess &= ~ACS_PP_STRIP_CTRL;
else acs_postprocess |= ACS_PP_STRIP_CTRL;
break;
} // switch
} /* binmode */

/* speak a single character */
void speakChar(int c, int sayit, int asword)
{
	short i;
	const char *t;
	static char ctrlstr[] = "controal x";
	static const char * const wfl[] = {
"al fa",
"brohvo",
"charlie",
"delta",
"eko",
"foxtrot",
"gawlf",
"hotell",
"india",
"juleyet",
"killo",
"liema",
"mike",
"noavember",
"oscar",
"popa",
"kebeck",
"romeo",
"seeara",
"tango",
"uniform",
"victor",
"wiskey",
"x ray",
"yangkey",
"zoolu",
};

	if(c == '\7') {
		if(clicksOn) { acs_bell(); return; }
		t = "bell";
		goto copy_t;
	}

	if(c == '\r') {
		t = "return";
		goto copy_t;
	}

	if(c == '\n') {
		if(clicksOn) { acs_cr(); return; }
		if(reading) return;
		t = "line";
		goto copy_t;
	}

	if(c == '\t') {
		t = "tab";
		goto copy_t;
	}

	if(c < 27) {
		ctrlstr[9] = c|0x40;
		t = ctrlstr;

copy_t:
		strcpy((char*)shortPhrase, t);
		if(sayit) ss_say_string(shortPhrase);
		return;
	} /* control character */

top:
t = (char*)acs_getpunc(c);
if(t) goto copy_t;

if(c >= 256) {
/* We are past getpunc(), guess we don't know how to say this unicode. */
c = '?';
goto top;
}

		if(!isalnum(c)) {
			static char nonascii[] = "bite x x";
			static const char almostHex[] = "0123456789xqcdlf";
			nonascii[5] = almostHex[c>>4];
			nonascii[7] = almostHex[c&15];
			t =  nonascii;
		goto copy_t;
	}

/* now a letter or digit */
c = tolower(c);
if(isalpha(c) && asword) {
t = wfl[c-'a'];
goto copy_t;
}

	if(sayit) ss_say_char(c);
} /* speakChar */


/*********************************************************************
An event is interrupting speech.
Key command, echoed character, console switch.
Stop any reading in progress.
*********************************************************************/

static void interrupt(void)
{
reading = oneSymbol = 0;
if(ss_stillTalking()) ss_shutup();
}

#define readNextMark rb->marks[27]

static void
readNextPart(void)
{
int gsprop;
int i;
char *end; /* the end of the sentence */
char first;

acs_refresh(); /* whether we need to or not */

if(readNextMark)
rb->cursor = readNextMark;
readNextMark = 0;

if(!rb->cursor) {
/* lots of text has pushed the reading cursor off the edge. */
acs_buzz();
reading = 0;
return;
}

gsprop = ACS_GS_REPEAT;
if(oneLine | clicksOn)
gsprop |= ACS_GS_STOPLINE;
else
gsprop |= ACS_GS_NLSPACE;

top:
/* grab something to read */
j_in->buf[0] = 0;
j_in->offset[0] = 0;
acs_getsentence(j_in->buf+1, 120, j_in->offset+1, gsprop);

if(!j_in->buf[1]) {
/* Empty sentence, nothing else to read. */
reading = 0;
readNextMark = 0;
return;
}

first = j_in->buf[1];
if(first == '\n' || first == '\7') {
/* starts out with newline or bell, which is usually associated with a sound */
/* This will swoop/beep with clicks on, or say the word newline or bell with clicks off */
speakChar(j_in->buf[1], 1, 0);

/* Find the next token/offset, which should be the next character */
for(i=2; !j_in->offset[i]; ++i)  ;
rb->cursor += j_in->offset[i];
/* but don't leave it there if you have run off the end of the buffer */
if(rb->cursor >= rb->end) {
rb->cursor = rb->end-1;
reading = 0;
readNextMark = 0;
return;
}

/* The following line is bad if there are ten thousand bells, no way to interrupt */
/* I'll deal with that case later. */
goto top;
}

j_in->len = 1 + strlen(j_in->buf+1);
prepTTS();

/* Cut the text at a logical sentence, as indicated by newline.
 * If newline wasn't already present in the input, this has been
 * set for you by prepTTS. */
end = strpbrk(j_out->buf+1, "\7\n");
if(!end)
end = j_out->buf+1 + strlen(j_out->buf+1);
*end = 0;
j_out->len = end - j_out->buf;
readNextMark = rb->cursor + j_out->offset[j_out->len];
ss_say_string_imarks(j_out->buf+1, j_out->offset+1, 1);
reading = 0; /* temporary */
readNextMark = 0;
} /* readNextPart */

/*********************************************************************
Execute the speech command.
The argument is the command list, null-terminated.
*********************************************************************/

static void runSpeechCommand(int input, const char *cmdlist)
{
	const struct cmd *cmdp;
	char suptext[256]; /* supporting text */
	char lasttext[256]; /* supporting text */
	char support; /* supporting character */
	int i, n;
	int asword, quiet, rc, gsprop, c;
	char cmd;
char *t;

interrupt();

acs_cursorset();

top:
	cmd = *cmdlist;
if(cmd) ++cmdlist;
	cmdp = &speechcommands[cmd];
asword = 0;

	/* some comands are meaningless when the buffer is empty */
	if(cmdp->nonempty && rb->end == rb->start) goto error_bound;

	support = 0;
	if(cmdp->nextchar) {
		if(*cmdlist) support = *cmdlist++;
else{
acs_click();
if(acs_get1char(&support)) goto error_bell;
}
	}

suptext[0] = 0;
if(cmdp->nextline) {
acs_tone_onoff(0);
if(acs_keystring(suptext, sizeof(suptext), ACS_KS_DEFAULT)) return;
}

quiet = ((!input)|*cmdlist);

	/* perform the requested action */
	switch(cmd) {
	case 0: /* command string is finished */
		acs_cursorsync();
if(!quiet) acs_click();
		return;

	case 1:
acs_clearbuf();
markleft = 0;
if(!quiet) acs_tone_onoff(0);
break;

	case 2: /* locate visual cursor */
		if(!screenmode) goto error_bell;
rb->cursor = rb->v_cursor;
acs_cursorset();
		break;

	case 3: acs_startbuf(); break;

	case 4: acs_endbuf(); break;

	case 5: acs_startline(); break;

	case 6: acs_endline(); break;

	case 7: acs_startword(); break;

	case 8: acs_endword(); break;

	case 9: acs_lspc(); break;

	case 10: acs_rspc(); break;

	case 11: if(!acs_back()) goto error_bound; break;

	case 12: if(!acs_forward()) goto error_bound; break;

	case 13: /* up a row */
		n = acs_startline();
		if(!acs_back()) goto error_bound;
		acs_startline();
		for(i=1; i<n; ++i) {
			if(acs_getc() == '\n') goto error_bell;
acs_forward();
		}
		break;

	case 14: /* down a row */
		n = acs_startline();
acs_endline();
		if(!acs_forward()) goto error_bound;
		for(i=1; i<n; ++i) {
			if(acs_getc() == '\n') goto error_bell;
			if(!acs_forward()) goto error_bound;
		}
		break;

	case 15: asword = 1;
	case 16:
letter:
acs_cursorsync();
		speakChar(acs_getc_uc(), 1, asword);
		break;

	case 17: /* indicate case */
acs_cursorsync();
		c = acs_getc();
		if(!isalpha(c)) goto error_bell;
		rc = isupper(c);
		if(clicksOn)
			acs_tone_onoff(rc);
		else {
if(!quiet) acs_click();
			ss_say_string((rc ? "upper" : "lower"));
}
return;

	case 18: /* read column number */
if(!quiet) acs_click();
		acs_cursorsync();
		n = acs_startline();
j_in->buf[0] = 0;
		sprintf(j_in->buf+1, "%d", n);
		j_in->len = strlen(j_in->buf+1) + 1;
prepTTS();
		ss_say_string(j_out->buf+1);
		return;

	case 19: /* just read one word */
acs_cursorsync();
c = acs_getc_uc();
if(c <= ' ') goto letter;
acs_startword();
acs_cursorsync();
gsprop = ACS_GS_STOPLINE | ACS_GS_REPEAT | ACS_GS_ONEWORD;
j_in->buf[0] = 0;
j_in->offset[0] = 0;
acs_getsentence(j_in->buf+1, WORDLEN, j_in->offset+1, gsprop);
		j_in->len = strlen(j_in->buf+1) + 1;
rb->cursor += j_in->offset[j_in->len] - 1;
acs_cursorset();
prepTTS();
		ss_say_string(j_out->buf+1);
break;

	case 20: /* start continuous reading */
if(!quiet) acs_click();
startread:
		/* We always start reading at the beginning of a word */
acs_startword();
		acs_cursorsync();
reading = 1;
readNextPart();
		return; /* has to be the end of the composite */

	case 21: ss_shutup(); break;

	case 22: acs_bypass(); break;

	/* clear, set, and toggle binary modes */
	case 23: binmode(0, support, quiet); break;
	case 24: binmode(1, support, quiet); break;
	case 25: binmode(2, support, quiet); break;

	case 26: asword = 1; /* search backwards */
	case 27: /* search forward */
		if(*suptext)
			strcpy(lasttext, suptext);
else strcpy(suptext, lasttext);
		if(!*suptext) goto error_bell;
		if(!acs_bufsearch(suptext, asword, oneLine)) goto error_bound;
		acs_cursorsync();
if(!quiet) acs_cr();
		if(!oneLine) {
			ss_say_string("o k");
rb->cursor -= (strlen(suptext)-1);
			return;
		}
		/* start reading at the beginning of this line */
acs_startline();
		goto startread;

	case 28: /* volume */
		if(!isdigit(support)) goto error_bell;
rc = ss_setvolume(support-'0');
t = "set volume";
speechparam:
if(rc == -1) goto error_bound;
if(rc == -2) goto error_bell;
		if(quiet) break;
		ss_say_string(t);
		break;

	case 29: /* inc volume */
rc = ss_incvolume();
t = "louder";
goto speechparam;

	case 30: /* dec volume */
rc = ss_decvolume();
t = "softer";
goto speechparam;

	case 31: /* speed */
		if(!isdigit(support)) goto error_bell;
rc = ss_setspeed(support-'0');
t = "set rate";
goto speechparam;

	case 32: /* inc speed */
rc = ss_incspeed();
t = "faster";
goto speechparam;

	case 33: /* dec speed */
rc = ss_decspeed();
t = "slower";
goto speechparam;

	case 34: /* pitch */
		if(!isdigit(support)) goto error_bell;
rc = ss_setpitch(support-'0');
t = "set pitch";
goto speechparam;

	case 35: /* inc pitch */
rc = ss_incpitch();
t = "higher";
goto speechparam;

	case 36: /* dec pitch */
rc = ss_decpitch();
t = "lower";
goto speechparam;

	case 37: /* set voice */
		if(support < '0' || support > '9') goto error_bell;
		rc = ss_setvoice(support-'0');
t = "hello there";
goto speechparam;

	case 38: /* key binding */
if(acs_line_configure(suptext, cfg_syntax))
goto error_bell;
if(!quiet) acs_cr();
		return;

	case 39: /* last complete line */
acs_endbuf();
		if(screenmode) acs_back();
	while(1) {
		c = acs_getc();
		if(c == '\n') asword = 1;
		if(!isspace(c) && asword) break;
		if(!acs_back()) goto error_bound;
	}
	break;

case 40: /* mark left */
if(!input) goto error_bell;
acs_cursorsync();
markleft = rb->cursor;
if(!quiet) acs_tone_onoff(0);
break;

case 41: /* mark right */
if(!input) goto error_bell;
if(support < 'a' || support > 'z') goto error_bell;
if(!markleft) goto error_bell;
n = 0;
cutbuf[n++] = '@';
cutbuf[n++] = support;
cutbuf[n] = 0;
acs_line_configure(cutbuf, 0);
cutbuf[n++] = '<';
acs_cursorsync();
markright = rb->cursor;
if(markright < markleft) goto error_bound;
i = markright - markleft;
++i;
if(i + n >= sizeof(cutbuf)) goto error_bound;
while(i) {
cutbuf[n] = acs_downshift(*markleft);
++n, ++markleft, --i;
}
cutbuf[n] = 0;
/* Stash it in the macro */
if(acs_line_configure(cutbuf, cfg_syntax) < 0)
goto error_bell;
markleft = 0;
if(!quiet) acs_tone_onoff(0);
return;

	case 42: /* set echo */
		if(support < '0' || support > '4') goto error_bell;
echoMode = support - '0';
		if(input && !*cmdlist) {
static const char * const echoWords[] = { "off", "letters", "words", "letters pause", "words pause"};
ss_say_string(echoWords[echoMode]);
}
		break;

case 43: /* set a marker in the tty buffer */
if(support < 'a' || support > 'z') goto error_bell;
acs_cursorsync();
if(!rb->cursor) goto error_bell;
rb->marks[support-'a'] = rb->cursor;
if(!quiet) acs_tone_onoff(0);
break;

case 44: /* jump to a preset marker */
if(support < 'a' || support > 'z') goto error_bell;
if(!rb->marks[support-'a']) goto error_bell;
rb->cursor = rb->marks[support-'a'];
acs_cursorset();
if(!quiet) acs_tone_onoff(0);
break;

	default:
	error_bell:
		acs_bell();
		return;

	error_bound:
acs_highbeeps();
		return;
	} /* end switch on function */

	goto top; /* next command */
} /* runSpeechCommand */

/* keystroke handler */
static int last_key, last_ss;
static void key_h(int key, int ss, int leds)
{
last_key = key;
last_ss = ss;
} /* key_h */

/* Remember the last console, and speak a message if it changes */
static int last_fgc;
static void fgc_h(void)
{
if(!last_fgc) { /* first time */
last_fgc = acs_fgc;
return;
}

if(last_fgc == acs_fgc)
return; /* should never happen */

last_fgc = acs_fgc;
/* kill any pending keystroke command; we just switched consoles */
last_key = last_ss = 0;

/* stop reading, and speak the new console */
interrupt();
j_in->buf[0] = 0;
sprintf(j_in->buf+1, "console %d", acs_fgc);
		j_in->len = strlen(j_in->buf+1) + 1;
prepTTS();
		ss_say_string(j_out->buf+1);
} /* fgc_h */

static void more_h(int echo, unsigned int c)
{
if(echoMode && echo == 1 && c < 256 && isprint(c)) {
interrupt();
speakChar(c, 1, 0);
}
} /* more_h */

static void
openSound(void)
{
static const short startsnd[] = {
		476,5,
530,5,
596,5,
662,5,
762,5,
858,5,
942,5,
0,0};
acs_notes(startsnd);
} /* openSound */

static void
testTTS(int n)
{
char line[400];

readLiteral = n;

/* This doesn't go through the normal acs_open() process */
acs_reset_configure();
/* key bindings don't matter here, but let's load our pronunciations */
j_configure("jup.cfg");

while(fgets(line, sizeof(line), stdin))
puts(prepTTSmsg(line));

exit(0);
} /* testTTS */
int
main(int argc, char **argv)
{
++argv, --argc;

/* this has to be done first */
acs_lang = LANG_ENGLISH;

if(setupTTS()) {
fprintf(stderr, "Could not malloc space for text preprocessing buffers.\n");
exit(1);
}

if(argc && stringEqual(argv[0], "-d")) {
acs_debug = 1;
++argv, --argc;
}

if(argc && stringEqual(argv[0], "tts"))
testTTS(0);

if(argc && stringEqual(argv[0], "ltts"))
testTTS(1);

if(acs_open("/dev/acsint") < 0) {
fprintf(stderr, "cannot open the driver /dev/acsint;\n\
did you make this character special,\n\
and do you have permission to read and write it,\n\
and did you install the acsint module,\n\
and do you also have permission to read /dev/vcsa?\n");
exit(1);
}

acs_key_h = key_h;
acs_fgc_h = fgc_h;
acs_more_h = more_h;

// this has to run after the device is open,
// because it sends "key capture" commands to the acsint driver
j_configure("jup.cfg");

/* only doubletalk for now; this should come from argv */
ss_style = SS_STYLE_DOUBLE;
ss_startvalues();

if(ess_open("/dev/ttyS2", 9600)) {
fprintf(stderr, "Cannot open serial port %d\n", 2);
exit(1);
}

openSound();

/* Initialize the doubletalk. */
/* I don't even remember what these do any more. */
ss_say_string("\1@ \0012b \00126g \0012o \00194i ");

/* Adjust rate and voice, then greet. */
ss_setvoice(7);
ss_setspeed(9);
ss_say_string("jupiter ready");

// This runs forever, you have to hit interrupt to kill it,
// or kill it from another tty.
while(1) {
acs_ss_events();

if(last_key) {
char *cmdlist; // the speech command
int mkcode = acs_build_mkcode(last_key, last_ss);
cmdlist = acs_getspeechcommand(mkcode);
//There ought to be a speech command, else why were we called?
// Oh well, let's make sure.
if(!cmdlist) {
// Well you might not get it because the shift state is left or right alt,
// and the command is generic alt.
// Let's check for that.
if(last_ss&ACS_SS_ALT) {
mkcode = acs_build_mkcode(last_key, (last_ss|ACS_SS_ALT));
cmdlist = acs_getspeechcommand(mkcode);
}
}
last_key = last_ss = 0;
// Now we ought to have it!
if(cmdlist) runSpeechCommand(1, cmdlist);
}

}

acs_close();
} // main


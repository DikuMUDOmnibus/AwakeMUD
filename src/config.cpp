/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_CC__

#include "structs.h"
#include "awake.h"

#define YES     1
#define NO      0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/

/****************************************************************************/
/* exp change limits */
int max_exp_gain = 250; /* max gainable per kill */
int max_exp_loss = 1000;        /* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 3;
int max_pc_corpse_time = 10;

/* should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/* "okay" etc. */
char *OK = "Okay.\r\n";
char *NOPERSON = "No-one by that name here.\r\n";
char *NOEFFECT = "Nothing seems to happen.\r\n";

/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that newbies should enter at */
long newbie_start_room = 60500;

/* virtual number of room that mortals should enter at */
long mortal_start_room = 60599;

/* virtual number of room that immorts should enter at by default */
long immort_start_room = 60599;

/* virtual number of room that frozen players should enter at */
long frozen_start_room = 60599;

/*
 * virtual numbers of donation rooms.  note: you must change code in
 * do_drop of act.obj1.c if you change the number of non-NOWHERE
 * donation rooms.
 */
vnum_t donation_room_1 = 60570;
vnum_t donation_room_2 = 60571;
vnum_t donation_room_3 = 60572;


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/* default port the game should run on if no port given on command-line */
int DFLT_PORT = 4000;

/* default directory to use as data directory */
char *DFLT_DIR = "lib";

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 300;

/* maximum size of bug, typo and idea files (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 3;

/*
 * Some nameservers are very slow and cause the game to lag terribly every
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = YES;

char *MENU =
  "\r\n"
  "^YWelcome ^Cto ^BAwake 2062^n!\r\n"
  "^R0^n) Exit from ^BAwake 2062^n.\r\n"
  "^G1^n) Enter the game.\r\n"
  "^G2^n) Read the background story.\r\n"
  "^G3^n) Check rent status.\r\n"
  "^G4^n) Change password.\r\n"
  "^L5^n) Delete this character.\r\n"
  "\r\n"
  "   Make your choice: ";

char *QMENU =
  "\r\n"
  "Current options:\r\n"
  "^R0^n) Exit from ^BAwake 2062^n.\r\n"
  "^G1^n) Check rent status.\r\n"
  "^G2^n) Change password.\r\n"
  "^G3^n) Delete this character.\r\n"
  "\r\n"
  "   Make your choice: ";

char *GREETINGS =
  "\r\n"
  "Administration Email: che@awakenedworlds.net\r\n"
  "The following mud is based on CircleMUD 3.0 by Jeremy Elson.  It is a\r\n"
  "derivative of DikuMUD (GAMMA 0.0) by Hans Henrik Staerfeldt, Katja Nyboe,\r\n"
  "Tom Madsen, Michael Seifert, and Sebastian Hammer.\r\n"
  "AwakeMUD Code Level 0.84 BETA RELEASE, by Flynn, Fastjack, Rift, Washu, and Che.\r\n"
  "\r\n"
  "_____   .                    A            .              .   .       .\r\n"
  "o o o\\            .        _/_\\_                                  |\\\r\n"
  "------\\\\      .         __//...\\\\__                .              ||\\   .\r\n"
  "__ A . |\\           .  <----------->     .                  .     ||||\r\n"
  "HH|\\. .|||                \\\\\\|///                 ___|_           ||||\r\n"
  "||| | . \\\\\\     A    .      |.|                  /|  .|    .      /||\\\r\n"
  "  | | .  |||   / \\          |.|     .           | | ..|          /.||.\\\r\n"
  "..| | . . \\\\\\ ||**|         |.|   _A_     ___   | | ..|         || |\\ .|\r\n"
  "..| | , ,  |||||**|         |.|  /| |   /|   |  |.| ..|         || |*|*|\r\n"
  "..|.| . . . \\\\\\|**|.  ____  |.| | | |  | |***|  |.| ..|  _____  || |*|*|\r\n"
  "..|.| . . .  |||**| /|.. .| |.| |*|*|  | |*  | ___| ..|/|  .  | ||.|*|\\|\\\r\n"
  "_________ . . \\\\\\*|| |.. .|//|\\\\|*|*_____| **||| ||  .| | ..  |/|| |*| |\\\\\r\n"
  "Awake 2062\\ .  ||||| |..  // A \\\\*/| . ..| * ||| || ..| |  .  ||||,|*| | \\\r\n"
  "By:        |\\ . \\\\\\| |.. // /|\\ \\\\ | . ..|** ||| || ..| | . . ||||.|*| |\\\\\r\n"
  "Rift, Pook  \\\\.  ||| |..|| | | | ||| . ..| * ||| ||  .| | ..  ||||.|*| ||||\r\n"
  "& Harlequin  ||  ||| |, ||.| | | ||| . ..| * ||| || ..| | . ..||||.|*| ||||\r\n"
  "---------------------------------------------------------------------------\r\n"
  "\r\n"
  "Slot me some identification, chummer: ";

char *WELC_MESSG =
  "\r\n"
  "Welcome to the future, 2062, the Sixth World to some, an Awakening to all.\r\n"
  "\r\n\r\n";

char *AUTH_MESSG=
  "\r\n"
  "To begin play at AwakeMUD, you will need to get this character authorized.\r\n"
  "To do so, your character name must be non-offensive, and appropriate for the world of\r\n"
  "Shadowrun.  For more information on Shadowrun, see the official Shadowrun\r\n"
  "website at http://www.shadowrunrpg.com/\r\n"
  "-- Admin\n\r";

char *START_MESSG =
  "Welcome to the future, 2062, where mankind has entered what the Mayans would\r\n"
  "call the Sixth World.  New races, magic, and technology all clash in what we\r\n"
  "call Awake 2062.  If you have never experienced this world before, typing\r\n"
  "help NEWBIE will help you understand what exactly is going on here.\r\n\r\n"
  "Awake 2062 is a BETA mud.  There are bugs.  There will be\r\n"
  "pwipes, and things will go poof for seemingly no reason at all.\r\n"
  "Reimbursements are no longer available.\r\n";


/****************************************************************************/
/****************************************************************************/


int ident = YES;

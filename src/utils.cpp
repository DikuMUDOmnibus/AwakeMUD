/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <iostream>

using namespace std;

#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock.h>
#define random() rand()
#define srandom(x) srand(x)
#else
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "telnet.h"
#include "structs.h"
#include "utils.h"
#include "awake.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "memory.h"
#include "house.h"
#include "db.h"
#include "constants.h"

extern class memoryClass *Mem;
extern struct time_info_data time_info;

extern void die(struct char_data * ch);
extern int damage_array[];
extern const struct target_type target_array[];
extern const char *log_types[];
extern long beginning_of_time;


// this just checks to see if two people are in the same group
// they both MUST be following the same leader or one following the other
bool in_group(struct char_data *one, struct char_data *two)
{
  // if one is following the other
  if ((one->master == two) || (two->master == one))
    return TRUE;

  // if they are following the same person
  if (one->master && (one->master == two->master))
    return TRUE;

  // if they are the same person
  if (one == two)
    return TRUE;

  // oh well, not in the same group
  return FALSE;
}

/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  if (from == to)
    return from;
  else if (from > to) {
    // it should not happen, but if it does...
    int temp = to;
    to = from;
    from = temp;
  }
  return ((random() % (to - from + 1)) + from);
}

/* simulates dice roll */
int dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
    return 0;

  while (number-- > 0)
    sum += ((random() % size) + 1);

  return sum;
}

// if we're using GNU C++, we don't need these functions
#ifndef __GNUG__
int MIN(int a, int b)
{
  return a < b ? a : b;
}

int MAX(int a, int b)
{
  return a > b ? a : b;
}
#endif

/* rolls a 6-sided dice by rule of 6 and rule of 1 */
int srdice(void)
{
  static int roll;
  int sum = 0, num = 1;
  register int i;

  for (i = 1; i <= num; i++) {
    roll = ((random() % 6) + 1);
    if (roll == 6)
      num++;
    sum += roll;
  }
  return sum;
}

int success_test(int number, int target)
{
  int total = 0, roll, one = 0;
  register int i;

  target = MAX(target, 2);

  for (i = 1; i <= number; i++) {
    if ((roll = srdice()) == 1)
      one++;
    else if (roll >= target)
      total++;
  }

  if (one == number)
    return -1;
  return total;
}

int resisted_test(int num4ch, int tar4ch, int num4vict, int tar4vict)
{
  return (success_test(num4ch, tar4ch) - success_test(num4vict, tar4vict));
}

int dec_staging(int successes, int wound)
{
  while (successes >= 2) {
    wound--;
    successes -= 2;
  }
  return wound;
}

int stage(int successes, int wound)
{
  if (successes >= 0)
    while (successes >= 2) {
      wound++;
      successes -= 2;
    }
  else
    while (successes <= -2) {
      wound--;
      successes += 2;
    }
  return wound;
}

int convert_damage(int damage)
{
  int extra = 0;

  if (damage < 0)
    damage = 0;
  else if (damage > 4) {
    extra = (damage - 4);
    damage = 10; // deadly
  } else
    damage = damage_array[damage];

  return (damage + extra);
}

int modify_target_rbuf(struct char_data *ch, char *rbuf)
{
  extern time_info_data time_info;
  int base_target = 0, temp;
  struct affected_type *af;

  // first apply physical damage modifiers
  if (GET_PHYSICAL(ch) <= (GET_MAX_PHYSICAL(ch) * 2/5))
  {
    base_target += 3;
    buf_mod( rbuf, "PhyS", 3 );
  } else if (GET_PHYSICAL(ch) <= (GET_MAX_PHYSICAL(ch) * 7/10))
  {
    base_target += 2;
    buf_mod( rbuf, "PhyM", 2 );
  } else if (GET_PHYSICAL(ch) <= (GET_MAX_PHYSICAL(ch) * 9/10))
  {
    base_target += 1;
    buf_mod( rbuf, "PhyL", 1 );
  }
  if (GET_TRADITION(ch) == TRAD_ADEPT
      && GET_POWER(ch, ADEPT_PAIN_RESISTANCE) > 0)
  {
    temp = (int)(GET_MAX_PHYSICAL(ch) - GET_PHYSICAL(ch) / 100);
    if (temp <= GET_POWER(ch, ADEPT_PAIN_RESISTANCE)) {
      if (temp >= (int)(GET_MAX_PHYSICAL(ch) * 3/500)) {
        buf_mod( rbuf, "PainResP", -3 );
        base_target -= 3;
      } else if (temp >= (int)(GET_MAX_PHYSICAL(ch) * 3/1000)) {
        base_target -= 2;
        buf_mod( rbuf, "PainResP", -2 );
      } else if (temp >= (int)(GET_MAX_PHYSICAL(ch) / 1000)) {
        base_target -= 1;
        buf_mod( rbuf, "PainResP", -1 );
      }
    }
    temp = (int)(GET_MAX_MENTAL(ch) - GET_MENTAL(ch) / 100);
    if ((temp + (int)(GET_MAX_PHYSICAL(ch) - GET_PHYSICAL(ch) / 100)) <=
        GET_POWER(ch, ADEPT_PAIN_RESISTANCE)) {
      if (temp >= (int)(GET_MAX_MENTAL(ch) * 3/500)) {
        base_target -= 3;
        buf_mod( rbuf, "PainResM", -3 );
      } else if (temp >= (int)(GET_MAX_MENTAL(ch) * 3/1000)) {
        base_target -= 2;
        buf_mod( rbuf, "PainResM", -2 );
      } else if (temp >= (int)(GET_MAX_MENTAL(ch) / 1000)) {
        base_target -= 1;
        buf_mod( rbuf, "PainResM", -1 );
      }
    }
  } else
    for (af = ch->affected; af; af = af->next)
      if (af->type == SPELL_RESIST_PAIN && af->modifier > 0)
      {
        if (GET_PHYSICAL(ch) <= (GET_MAX_PHYSICAL(ch) * 2/5) && af->modifier == 3) {
          base_target -= 3;
          buf_mod( rbuf, "ResPainP", -3 );
        } else if (GET_PHYSICAL(ch) <= (GET_MAX_PHYSICAL(ch) * 7/10) && af->modifier == 2) {
          base_target -= 2;
          buf_mod( rbuf, "ResPainP", -2 );
        } else if (GET_PHYSICAL(ch) <= (GET_MAX_PHYSICAL(ch) * 9/10)) {
          base_target -= 1;
          buf_mod( rbuf, "ResPainP", -1 );
        }
      }

  // then apply mental damage modifiers
  if (GET_MENTAL(ch) <= (GET_MAX_MENTAL(ch) * 2/5))
  {
    base_target += 3;
    buf_mod( rbuf, "MenS", 3 );
  } else if (GET_MENTAL(ch) <= (GET_MAX_MENTAL(ch) * 7/10))
  {
    base_target += 2;
    buf_mod( rbuf, "MenM", 2 );
  } else if (GET_MENTAL(ch) <= (GET_MAX_MENTAL(ch) * 9/10))
  {
    base_target += 1;
    buf_mod( rbuf, "MenL", 1 );
  }

  // then apply modifiers for sustained spells
  if (GET_SUSTAINED(ch) > 0)
  {
    base_target += (GET_SUSTAINED(ch) << 1);
    buf_mod( rbuf, "Sustain", GET_SUSTAINED(ch) << 1 );
  }

  // then account for visibility
  if (ch->in_room != NOWHERE)
    if (!IS_AFFECTED(ch, AFF_INFRAVISION) && IS_DARK(ch->in_room))
    {
      base_target += 8;
      buf_mod( rbuf, "Dark", 8 );
    } else if (IS_AFFECTED(ch, AFF_INFRAVISION) && IS_DARK(ch->in_room))
    {
      base_target += 3;
      buf_mod( rbuf, "DarkInfra", 3 );
    } else if (!IS_AFFECTED(ch, AFF_LOW_LIGHT) && !IS_AFFECTED(ch, AFF_INFRAVISION) && IS_LOW(ch->in_room))
    {
      base_target += 4;
      buf_mod( rbuf, "Low", 4 );
    } else if (IS_AFFECTED(ch, AFF_INFRAVISION) && !IS_AFFECTED(ch, AFF_LOW_LIGHT) && IS_LOW(ch->in_room))
    {
      base_target += 1;
      buf_mod( rbuf, "LowInfra", 1 );
    }

  base_target += GET_TARGET_MOD(ch);
  buf_mod( rbuf, "GET_TARGET_MOD", GET_TARGET_MOD(ch) );
  if (PLR_FLAGGED(ch, PLR_PERCEIVE))
  {
    base_target += 2;
    buf_mod(rbuf, "AstralPercep", 2);
  }
  if (GET_RACE(ch) == RACE_NIGHTONE && ((time_info.hours > 6) || (time_info.hours < 19)) && OUTSIDE(ch))
  {
    base_target += 1;
    buf_mod( rbuf, "Sunlight", 1);
  }
  if (!IS_NPC(ch))
  {
    // if you're an owl shaman and it's daytime, uh oh... (=
    if (GET_TRADITION(ch) == TRAD_SHAMANIC) {
      if ((GET_TOTEM(ch) == TOTEM_OWL) && ((time_info.hours > 6) || (time_info.hours < 19))) {
        base_target += 2;
        buf_mod( rbuf, "OwlDay", 2 );
      } else if ((GET_TOTEM(ch) == TOTEM_RAVEN) && !OUTSIDE(ch)) {
        base_target += 1;
        buf_mod( rbuf, "RavenInside", 1 );
      }
    }
  }

  return base_target;
}

int modify_target(struct char_data *ch)
{
  return modify_target_rbuf(ch, NULL);
}

// this returns the general skill
int return_general(int skill_num)
          {
            switch (skill_num) {
            case SKILL_PISTOLS:
            case SKILL_RIFLES:
            case SKILL_SHOTGUNS:
            case SKILL_ASSAULT_RIFLES:
            case SKILL_SMG:
            case SKILL_GRENADE_LAUNCHERS:
            case SKILL_TASERS:
            case SKILL_MACHINE_GUNS:
            case SKILL_MISSILE_LAUNCHERS:
            case SKILL_ASSAULT_CANNON:
            case SKILL_ARTILLERY:
              return (SKILL_FIREARMS);
              break;
            case SKILL_EDGED_WEAPONS:
            case SKILL_POLE_ARMS:
            case SKILL_WHIPS_FLAILS:
            case SKILL_CLUBS:
              return (SKILL_ARMED_COMBAT);
              break;
            default:
              return (skill_num);
            }
          }

          int reverse_web(struct char_data *ch, int &skill, int &target)
          {
            target += 4;
            return GET_ATT(ch, skills[skill].attribute);
          }

          static char *power_name[] =
            {
              "strength based",
              "dainty",
              "feeble",
              "weak",
              "poor",
              "decent",
              "modest",
              "average",
              "above average",
              "satisfactory",
              "commendable",
              "good",
              "very good",
              "admirable",
              "exemplary",
              "great",
              "excellent",
              "superb",
              "amazing",
            };

static char *attrib_name[] =
  {
    "no",
    "terrible",
    "below average",
    "average",
    "average",
    "above average",
    "high",
    "super-human",
    "super-human",
    "super-human",
    "super-ork",
    "super-ork",
    "super-troll",
  };

// this returns a pointer to name, and fills it with the power description
char *get_power(int number)
{

  number = MIN(18, (MAX(0, number)));

  return power_name[number];
}

char *get_attrib(int number)
{
  number = MIN(12, (MAX(0, number)));

  return attrib_name[number];
}

// capitalize a string
char *capitalize(const char *source)
{
  static char dest[MAX_STRING_LENGTH];
  strcpy(dest, source);
  *dest = UPPER(*dest);
  return dest;
}

// duplicate a string -- uses new!
char *str_dup(const char *source)
{
  if (!source)
    return NULL;

  char *New = new char[strlen(source) + 1];
  sprintf(New, "%s", source);
  return New;
}

// this function runs through 'str' and copies the first token to 'token'.
// it assumes that token is already allocated--it returns a pointer to the
// next char after the token in str
char *get_token(char *str, char *token)
{
  if (!str)
    return NULL;

  register char *temp = str;
  register char *temp1 = token;

  // first eat up any white space
  while (isspace(*temp))
    temp++;

  // now loop through the string and copy each char till we find a space
  while (*temp && !isspace(*temp))
    *temp1++ = *temp++;

  // terminate the string properly
  *temp1 = '\0';

  return temp;
}

/* strips \r's from line -- Chris*/
char *cleanup(char *dest, const char *src)
{
  if (!src) // this is because sometimes a null gets sent to src
    return NULL;

  register char *temp = &dest[0];

  for (; *src; src++)
    if (*src != '\r')
      *temp++ = *src;

  *temp = '\0';
  return dest;
}

/* str_cmp: a case-insensitive version of strcmp */
/* returns: 0 if equal, pos if arg1 > arg2, neg if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(const char *one, const char *two)
{
  for (; *one; one++, two++) {
    int diff = LOWER(*one) - LOWER(*two);

    if (diff!= 0)
      return diff;
  }

  return (LOWER(*one) - LOWER(*two));
}


/* str_str: A case-insensitive version of strstr */
/* returns: A pointer to the first occurance of str2 in str1 */
/* or a null pointer if it isn't found.                      */
char *str_str( const char *str1, const char *str2 )
{
  int i;
  char temp1[MAX_INPUT_LENGTH], temp2[MAX_INPUT_LENGTH];

  for ( i = 0; *(str1 + i); i++ ) {
    temp1[i] = LOWER(*(str1 + i));
  }

  temp1[i] = '\0';

  for ( i = 0; *(str2 + i); i++ ) {
    temp2[i] = LOWER(*(str2 + i));
  }

  temp2[i] = '\0';

  return (strstr(temp1, temp2));
}


/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk); /* not equal */

  return (0);
}

/* str_prefix: a case-insensitive version of strcmp that     */
/* does prefix matching.                                     */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of arg1.                */
int str_prefix(char *arg1, char *arg2)
{
  int chk, i;

  for (i = 0; *(arg1 + i) && *(arg2 + i); i++)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i))))
      if (chk < 0)
        return (-1);
      else
        return (1);

  if ( *(arg1 + i) )
    return (1);
  else
    return (0);
}
/* returns 1 if the character has a cyberweapon; 0 otherwise */
int has_cyberweapon(struct char_data * ch)
{
  struct obj_data *obj = NULL;
  for (obj = ch->cyberware;
       obj ;
       obj = obj->next_content)
  {
    /* is the cyberware object a weapon? */
    switch(GET_OBJ_VAL(obj,2)) {
    case 19:
    case 21:
      return 1;
    }
  }
  return 0;
}


/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[150];
  extern struct room_data *world;

  sprintf(buf, "%s hit DeathTrap #%ld (%s)", GET_CHAR_NAME(ch),
          world[ch->in_room].number, world[ch->in_room].name);
  mudlog(buf, ch, LOG_DEATHLOG, TRUE);
}

void log(const char *format, ...)
{
  va_list args;
  time_t ct = time(0);
  char *tmstr;

  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  fprintf(stderr, "%-15.15s :: ", tmstr + 4);

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fprintf(stderr, "\n");
}

void time_stamp(char *str, time_t when)
{
  struct tm *newtime;

  newtime = localtime(&when);
  newtime->tm_year = newtime->tm_year % 100;

  sprintf(str, "(%s%d:%s%d:%s%d/%s%d-%s%d-%s%d)",
          newtime->tm_hour < 10 ? "0" : "", newtime->tm_hour,
          newtime->tm_min < 10 ? "0" : "", newtime->tm_min,
          newtime->tm_sec < 10 ? "0" : "", newtime->tm_sec,
          newtime->tm_mon < 9 ? "0" : "", newtime->tm_mon + 1,
          newtime->tm_mday < 10 ? "0" : "", newtime->tm_mday,
          newtime->tm_year < 10 ? "0" : "", newtime->tm_year);
}

/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(char *str, struct char_data *ch, int log, byte file)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  extern struct descriptor_data *descriptor_list;
  struct descriptor_data *i;
  struct char_data *tch;
  char *tmp;
  time_t ct;
  int check_log = 0;

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if ( ch && ch->desc && ch->desc->original )
    sprintf(buf2, "[%5ld] (%s) ",
            world[ch->in_room].number,
            GET_CHAR_NAME(ch));
  else if (ch && ch->in_room != NOWHERE)
    sprintf(buf2, "[%5ld] ", world[ch->in_room].number);
  else
    strcpy(buf2, "");

  if (file)
    fprintf(stderr, "%-19.19s :: %s: %s%s\n", tmp, log_types[log], buf2, str);

  ct = ct;
  sprintf(buf, "^g[%s: %s%s]^n\r\n", log_types[log], buf2, str);

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
    {
      if (i->original)
        tch = i->original;
      else
        tch = i->character;
      if (!tch ||
          PLR_FLAGS(tch).AreAnySet(PLR_WRITING, PLR_MAILING,
                                   PLR_EDITING, ENDBIT))
        continue;

      if (ch
          && !access_level(tch, GET_INVIS_LEV(ch))
          && !access_level(tch, LVL_VICEPRES))
        continue;
      switch (log) {
      case LOG_CONNLOG:
        check_log = PRF_CONNLOG;
        break;
      case LOG_DEATHLOG:
        check_log = PRF_DEATHLOG;
        break;
      case LOG_MISCLOG:
        check_log = PRF_MISCLOG;
        break;
      case LOG_WIZLOG:
        check_log = PRF_WIZLOG;
        break;
      case LOG_SYSLOG:
        check_log = PRF_SYSLOG;
        break;
      case LOG_ZONELOG:
        check_log = PRF_ZONELOG;
        break;
      case LOG_CHEATLOG:
        check_log = PRF_CHEATLOG;
        break;
      case LOG_WIZITEMLOG:
        check_log = PRF_CHEATLOG;
        break;
      case LOG_BANLOG:
        check_log = PRF_BANLOG;
        break;
      case LOG_GRIDLOG:
        check_log = PRF_GRIDLOG;
        break;
      case LOG_WRECKLOG:
        check_log = PRF_WRECKLOG;
        break;
      }
      if (PRF_FLAGGED(tch, check_log))
        SEND_TO_Q(buf, i);
    }
}

void sprintbit(long vektor, const char *names[], char *result)
{
  long nr;

  *result = '\0';

  if (vektor < 0) {
    strcpy(result, "SPRINTBIT ERROR!");
    return;
  }
  for (nr = 0; vektor; vektor >>= 1) {
    if (IS_SET(1, vektor)) {
      if (*names[nr] != '\n') {
        strcat(result, names[nr]);
        strcat(result, " ");
      } else
        strcat(result, "UNDEFINED ");
    }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    strcat(result, "None ");
}

void sprinttype(int type, const char *names[], char *result)
{
  sprintf(result, "%s", names[type]);

  if (result == "(null")
    result = "UNDEFINED";

}

void sprint_obj_mods(struct obj_data *obj, char *result)
{
  *result = 0;
  if (obj->obj_flags.bitvector.GetNumSet() > 0)
  {
    char xbuf[MAX_STRING_LENGTH];
    obj->obj_flags.bitvector.PrintBits(xbuf, MAX_STRING_LENGTH,
                                       affected_bits, AFF_MAX);
    sprintf(result,"%s %s", result, xbuf);
  }

  for (register int i = 0; i < MAX_OBJ_AFFECT; i++)
    if (obj->affected[i].modifier != 0)
    {
      char xbuf[MAX_STRING_LENGTH];
      sprinttype(obj->affected[i].location, apply_types, xbuf);
      sprintf(result,"%s (%+d %s)",
              result, obj->affected[i].modifier, xbuf);
    }
  return;
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24; /* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY); /* 0..34 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = 0;
  now.year = 0;

  return now;
}

/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;  /* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 30;     /* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 12; /* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);        /* 0..XX? years */

  return now;
}

struct time_info_data aaaage(struct char_data * ch)
{
  struct time_info_data player_age;

  player_age = mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;

  return player_age;
}

bool access_level(struct char_data *ch, int level)
{
  ch = ch->desc && ch->desc->original ? ch->desc->original : ch;

  return (!IS_NPC(ch)
          && (GET_LEVEL(ch) >= level ));
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master)
  {
    if (k == ch)
      return TRUE;
  }

  return FALSE;
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (IS_AFFECTED(ch, AFF_CHARM))
  {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, SPELL_INFLUENCE) == 1) {
      affect_from_char(ch, SPELL_INFLUENCE);
      return;
    }
  } else
  {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
  }

  if (ch->master->followers->follower == ch)
  {  /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    delete k;
  } else
  {                      /* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next)
      ;

    j = k->next;
    k->next = j->next;
    delete j;
  }

  ch->master = NULL;
  AFF_FLAGS(ch).RemoveBits(AFF_CHARM, AFF_GROUP, ENDBIT);
}

/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j)
  {
    j = k->next;
    stop_follower(k->follower);
  }
  if (ch->player_specials->gname)
  {
    delete [] ch->player_specials->gname;
    ch->player_specials->gname = NULL;
  }
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

  ch->master = leader;

  k = new follow_type;

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}

bool PRF_TOG_CHK(char_data *ch, dword offset)
{
  PRF_FLAGS(ch).ToggleBit(offset);

  return PRF_FLAGS(ch).IsSet(offset);
}

bool PLR_TOG_CHK(char_data *ch, dword offset)
{
  PLR_FLAGS(ch).ToggleBit(offset);

  return PLR_FLAGS(ch).IsSet(offset);
}

char * buf_mod(char *rbuf, char *name, int bonus)
{
  if ( !rbuf )
    return rbuf;
  if ( bonus == 0 )
    return rbuf;
  rbuf += strlen(rbuf);
  if ( bonus > 0 )
    sprintf(rbuf, "%s +%d, ", name, bonus);
  else
    sprintf(rbuf, "%s %d, ", name, bonus);
  rbuf += strlen(rbuf);
  return rbuf;
}

char * buf_roll(char *rbuf, char *name, int bonus)
{
  if ( !rbuf )
    return rbuf;
  rbuf += strlen(rbuf);
  sprintf(rbuf, " [%s %d]", name, bonus);
  return rbuf;
}

int get_speed(struct veh_data *veh)
{
  int speed = 0;

  switch (veh->cspeed)
  {
  case SPEED_OFF:
  case SPEED_IDLE:
    speed = 0;
    break;
  case SPEED_CRUISING:
    if (ROOM_FLAGGED(veh->in_room, ROOM_INDOORS))
      speed = MIN(veh->speed, 3);
    else
      speed = MIN(veh->speed, 55);
    break;
  case SPEED_SPEEDING:
    if (ROOM_FLAGGED(veh->in_room, ROOM_INDOORS))
      speed = MIN(veh->speed, MAX(5, (int)(veh->speed * .7)));
    else
      speed = MIN(veh->speed, MAX(55, (int)(veh->speed * .7)));
    break;
  case SPEED_MAX:
    if (ROOM_FLAGGED(veh->in_room, ROOM_INDOORS))
      speed = MIN(veh->speed, 8);
    else
      speed = veh->speed;
    break;
  }
  return (speed);
}

int negotiate(struct char_data *ch, struct char_data *tch, int comp, int basevalue, int mod, bool buy)
{
  struct obj_data *bio;
  int cskill = GET_SKILL(ch, SKILL_NEGOTIATION);
  for (bio = ch->bioware; bio; bio = bio->next_content)
    if (GET_OBJ_VAL(bio, 2) == 3)
    {
      cskill += GET_OBJ_VAL(bio, 0);
      break;
    }
  int tskill = GET_SKILL(tch, SKILL_NEGOTIATION);
  for (bio = tch->bioware; bio; bio = bio->next_content)
    if (GET_OBJ_VAL(bio, 2) == 3)
    {
      tskill += GET_OBJ_VAL(bio, 0);
      break;
    }
  int chnego = success_test(cskill, GET_INT(tch)+mod);
  int tchnego = success_test(tskill, GET_INT(ch)+mod);
  if (comp)
  {
    chnego += success_test(GET_SKILL(ch, comp), GET_INT(tch)+mod) / 2;
    tchnego += success_test(GET_SKILL(tch, comp), GET_INT(ch)+mod) / 2;
  }
  int num = chnego - tchnego;
  if (num > 0)
  {
    if (buy)
      basevalue = MAX((int)(basevalue * 3/4), basevalue - (num * (basevalue / 20)));
    else
      basevalue = MIN((int)(basevalue * 5/4), basevalue + (num * (basevalue / 15)));
  } else
  {
    if (buy)
      basevalue = MIN((int)(basevalue * 5/4), basevalue + (num * (basevalue / 15)));
    else
      basevalue = MAX((int)(basevalue * 3/4), basevalue - (num * (basevalue / 20)));
  }
  return basevalue;

}

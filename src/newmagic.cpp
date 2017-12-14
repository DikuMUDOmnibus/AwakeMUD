/* *******************************************************************
* file: newmagic.cc                                                  *
* authors: Christopher J. Dickey and Andrew Hynek                    *
* purpose: this defines all the new magic routines for the mud       *
* (c)1997-2000 Christopher J. Dickey, Andrew Hynek, and Nick         *
* Robertson, (c)2001 The AwakeMUD Consortium                         *
******************************************************************* */


#define _newmagic_cc_

#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "structs.h"
#include "awake.h"
#include "db.h"
#include "newdb.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "handler.h"
#include "newmagic.h"
#include "spells.h"
#include "mag_create.h"
#include "shop.h"
#include "constants.h"

// extern vars
extern struct time_info_data time_info;

extern const char *lookdirs[];
extern int convert_look[];
extern train_t trainers[];
extern teach_t teachers[];
extern adept_t adepts[];
extern master_t masters[];

// extern funcs
extern void damage_obj(struct char_data *, struct obj_data *, int, int);
extern void look_at_room(struct char_data * ch, int ignore_brief);
extern void add_follower(struct char_data * ch, struct char_data * leader);
extern void ranged_response(struct char_data *ch, struct char_data *vict);
extern void die(struct char_data *);
extern bool in_group(char_t *, char_t *);
extern int modify_target(char_t *);
extern int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
extern int find_sight(struct char_data * ch);
extern char *get_power(int);
extern char *get_attrib(int);
extern void damage_door(struct char_data *ch, int room, int dir, int power, int type);

#define FORCE_PENALTY(spell) (0-((spell)->damage <= LIGHT ? 100 : 8 - 2*((spell)->damage)))

// drain is compacted into 1 int...i % 10 = drain_level, i - (i % 10) = drain_add
spell_a grimoire[] =
  {  // physical, category, target, drain, damage_level
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0},
    { TRUE, DETECT, TOUCH,  12, 0 },         // analyze device
    { TRUE, MANIPU, RANGED, 12, 0 },         // anti_ballistic barrier
    {FALSE, MANIPU, RANGED, 11, 0 },         // 5  - anti_mana barrier
    { TRUE, HEALTH, TOUCH,   0, 0 },         // antidote
    { TRUE, MANIPU, RANGED, 22, 0 },         // armor
    { TRUE, ILLUSI, RANGED, 22, 0 },         // chaos
    { TRUE, ILLUSI, AREA,   23, 0 },         // chaotic_world
    {FALSE, DETECT, RANGED,  2, 0 },         // 10 - clairvoyance
    { TRUE, MANIPU, RANGED, 12, MODERATE},   // clout
    { TRUE, DETECT, RANGED, 13, 0 },         // combat_sense
    {FALSE, ILLUSI, RANGED,  3, 0 },         // confusion
    { TRUE, HEALTH, TOUCH,   0, 0 },         // cure disease
    { TRUE, COMBAT, TOUCH,  13, DEADLY},     // 15 - death_touch
    {FALSE, DETECT, RANGED,  1, 0 },         // detect_align
    {FALSE, DETECT, RANGED,  1, 0 },         // detect_invis
    {FALSE, DETECT, RANGED,  1, 0 },         // detect_magic
    {FALSE, HEALTH, TOUCH,   0, 0 },         // heal
    { TRUE, COMBAT, AREA,   64, DEADLY},     // hellblast
    { TRUE, ILLUSI, TOUCH,  12, 0 },         // 20 - improved_invisibility
    {FALSE, MANIPU, RANGED, 23, 0 },         // influence
    {FALSE, ILLUSI, TOUCH,   2, 0 },         // invisibility
    {FALSE, COMBAT, AREA,    3, MODERATE},   // mana_ball
    {FALSE, COMBAT, AREA,    4, SERIOUS},    // mana_blast
    {FALSE, COMBAT, RANGED,  3, SERIOUS},    // 25 - mana_bolt
    {FALSE, COMBAT, AREA,    2, LIGHT},      // mana_cloud
    {FALSE, COMBAT, RANGED,  1, LIGHT},      // mana_dart
    {FALSE, COMBAT, RANGED,  2, MODERATE},   // mana_missile
    {FALSE, ILLUSI, RANGED, 12, 0 },         // overstimulation
    { TRUE, MANIPU, RANGED, 64, SERIOUS },   // 30 - petrify
    { TRUE, COMBAT, AREA,   13, MODERATE},   // power_ball
    { TRUE, COMBAT, AREA,   14, SERIOUS},    // power_blast
    { TRUE, COMBAT, RANGED, 13, SERIOUS},    // power_bolt
    { TRUE, COMBAT, AREA,   12, LIGHT},      // power_cloud
    { TRUE, COMBAT, RANGED, 11, LIGHT},      // 35 - power_dart
    { TRUE, COMBAT, RANGED, 12, MODERATE},   // power_missile
    { TRUE, COMBAT, RANGED, 13, SERIOUS},    // ram
    { TRUE, COMBAT, TOUCH,  12, SERIOUS},    // ram_touch
    {FALSE, HEALTH, TOUCH,   0, 0 },         // resist_pain
    { TRUE, MANIPU, CASTER, 34, 0 },         // 40 - shape_change
    {FALSE, HEALTH, RANGED,  1, 0 },         // stabilize
    {FALSE, COMBAT, AREA,   -7, MODERATE},   // stun_ball
    {FALSE, COMBAT, AREA,   -6, SERIOUS},    // stun_blast
    {FALSE, COMBAT, RANGED, -7, SERIOUS},    // stun_bolt
    {FALSE, COMBAT, AREA,   -8, LIGHT},      // 45 - stun_cloud
    {FALSE, COMBAT, RANGED, -8, MODERATE},   // stun_missile
    {FALSE, COMBAT, RANGED, -7, DEADLY},     // stun_touch
    { TRUE, MANIPU, AREA,   84, DEADLY},     // toxic_wave
    { TRUE, COMBAT, RANGED, 14, DEADLY},     // power_shaft
    { TRUE, COMBAT, AREA,   34, DEADLY},     // 50 - power_burst
    {FALSE, COMBAT, RANGED,  4, DEADLY},     // mana_shaft
    {FALSE, COMBAT, AREA,   24, DEADLY},     // mana_burst
    {FALSE, COMBAT, RANGED, -6, DEADLY},     // stun_shaft
    {FALSE, COMBAT, AREA,   14, DEADLY},     // stun_burst
    {FALSE, COMBAT, AREA,   -9, LIGHT},      // 55 - stun_dart
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, // 60
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, // 65
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, // 70
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, // 75
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, // 80
    {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0}, // 85
    {0,0,0,0,0},
    { TRUE, DETECT, TOUCH,   2, 0 },         // analyze magic
    { TRUE, DETECT, TOUCH,   2, 0 },         // analyze person
    { TRUE, HEALTH, TOUCH,  10, 0 },         // decrease attribute
    { TRUE, COMBAT, AREA,   34, SERIOUS},    // 90 - <element>ball
    { TRUE, COMBAT, RANGED, 14, SERIOUS},    // <element>bolt
    { TRUE, COMBAT, AREA,   13, LIGHT},      // <element>_cloud
    { TRUE, COMBAT, RANGED, 12, LIGHT},      // <element>_dart
    { TRUE, COMBAT, RANGED, 13, MODERATE},   // <element>_missile
    { TRUE, HEALTH, TOUCH,  10, 0 },         // 95 - increase attribute
    { TRUE, HEALTH, TOUCH,   0, 0 },         // increase reflexes
    { TRUE, MANIPU, CASTER, 22, 0 },         // light
    { TRUE, HEALTH, TOUCH,   2, 0 },         // poison
    {FALSE, MANIPU, CASTER, 33, 0 },         // teleport
  };

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  static struct affected_type *af;
  static struct char_data *i;
  extern const char *spell_wear_off_msg[];

  // loop through all the characters in the mud--eek
  for (i = character_list; i; i = i->next) {
    // reset their # of sustained spells
    GET_SUSTAINED(i) = 0;
    // then loop through the affected structs
    for (af = i->affected; af; af = af->next) {
      // and here we add up the # of sustained spells
      if (af)
        if (af->caster)
          GET_SUSTAINED(i)++;
        else if (!mag_can_see(i, af->sustained_by) && af->type != SPELL_HEAL && af->type != SPELL_ANTIDOTE &&
                 af->type != SPELL_STABILIZE && af->type != SPELL_CURE_DISEASE) {
          send_to_char(i, "%s\r\n", spell_wear_off_msg[af->type]);
          affect_remove(i, af, 1);
        }
    }
  }
}

void check_spell_drain( struct char_data *ch, spell_t *spell )
{
  if (DRAIN_LEVEL(spell->drain) <= 4)
    return;

  sprintf(buf2, "Correcting drain code for %s from %d to %d",
          spell->name, spell->drain, grimoire[spell->type].drain);
  mudlog(buf2, ch, LOG_SYSLOG, TRUE);
  spell->drain = grimoire[spell->type].drain;
}

int mag_can_see(struct char_data *ch, int id)
{
  int sight, room, nextroom, dir, distance;
  struct char_data *i, *vict, *tch = NULL;
  bool found = FALSE;

  for (i = character_list; i && !tch; i = i->next)
    if ((IS_NPC(i) ? -i->nr : GET_IDNUM(i)) == id)
      tch = i;

  if (!tch)
    return(FALSE);

  sight = find_sight(tch);

  for (vict = world[tch->in_room].people; vict && !found; vict = vict->next_in_room)
    if (vict == ch)
      found = TRUE;

  for (dir = 0; dir < NUM_OF_DIRS && !found; dir++)
  {
    room = tch->in_room;
    if (CAN_GO2(room, dir))
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;
    for (distance = 1; ((nextroom != NOWHERE) && !found && (distance <= sight)); distance++) {
      for (vict = world[nextroom].people; vict; vict = vict->next_in_room)
        if (vict == ch)
          found = TRUE;
      room = nextroom;
      if (CAN_GO2(room, dir))
        nextroom = EXIT2(room, dir)->to_room;
      else
        nextroom = NOWHERE;
    }
  }
  return found;
}

int spell_resist(struct char_data *ch)
{
  struct affected_type *af;
  int resist = (int)(GET_MAGIC(ch) / 4);

  for (af = ch->affected; af; af = af->next)
    if (af->type == SPELL_ANTI_SPELL && af->modifier > 0)
      resist += af->modifier;

  return resist;
}

int foci_bonus(struct char_data *ch, struct spell_data *spell,
               int force, bool fCast)
{
  int i;
  int bonus=0;
  struct obj_data *obj;

  for (i = 0; i < (NUM_WEARS - 1); i++)
  {
    if ((obj = GET_EQ(ch, i))
        && GET_OBJ_TYPE(obj) == ITEM_FOCUS
        && GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) == 1
        && GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) == GET_IDNUM(ch)) {
      switch (GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE)) {
      case FOCI_SPELL:
        if (spell->type == GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY))
          bonus += GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
        break;
      case FOCI_SPELL_CAT:
        if (spell->category == GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY))
          bonus += GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
        break;
      }
    }
  }
  /*  for (obj = ch->carrying; obj; obj = obj->next_content)
      {
        if (GET_OBJ_TYPE(obj) == ITEM_FOCUS
     && GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) == 1
     && GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) == GET_IDNUM(ch))
   {
     switch (GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE))
       {
       case FOCI_SPELL:
         if (spell->type == GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY))
    bonus += GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
         break;
       case FOCI_SPELL_CAT:
         if (spell->category == GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY))
    bonus += GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
         break;
       }
   }
      }
  */
  if ( !fCast )
    bonus += 1;
  return bonus/2;
}

int magic_pool_bonus(struct char_data *ch, struct spell_data *spell,
                     int force, bool fCast)
{
  int bonus;
  bonus = GET_MAGIC(ch);
#if 0

  if ( fCast )
  {
    if ( bonus > GET_MAG(ch) )
      bonus = GET_MAG(ch);
  } else
  {
    bonus -= GET_MAG(ch);
    if (bonus < 0)
      bonus = 0;
  }
#else
  bonus = (bonus + (fCast ? 1 : 0)) / 2;
#endif

  return bonus;
}

int totem_bonus(struct char_data *ch, struct spell_data *spell)
{
  int bonus = 0;
  if (GET_TRADITION(ch) == TRAD_SHAMANIC)
  {
    switch (spell->category) {
    case SPELL_CATEGORY_COMBAT:
      switch (GET_TOTEM(ch)) {
      case TOTEM_OWL:
        if (time_info.hours > 6 && time_info.hours < 19)
          break;
      case TOTEM_GATOR:
      case TOTEM_LION:
      case TOTEM_SHARK:
      case TOTEM_WOLF:
        bonus += 2;
        break;
      case TOTEM_SNAKE:
        if (!FIGHTING(ch))
          break;
      case TOTEM_RAVEN:
      case TOTEM_RAT:
      case TOTEM_RACCOON:
        bonus -= 1;
        break;
      default:
        break;
      }
      break;
    case SPELL_CATEGORY_DETECTION:
      switch (GET_TOTEM(ch)) {
      case TOTEM_SNAKE:
        if (FIGHTING(ch))
          bonus -= 1;
        else
          bonus += 2;
        break;
      case TOTEM_OWL:
        if (time_info.hours > 6 && time_info.hours < 19)
          break;
      case TOTEM_DOG:
      case TOTEM_EAGLE:
      case TOTEM_GATOR:
      case TOTEM_RAT:
      case TOTEM_SHARK:
      case TOTEM_WOLF:
        bonus += 2;
        break;
      default:
        break;
      }
      break;
    case SPELL_CATEGORY_HEALTH:
      switch (GET_TOTEM(ch)) {
      case TOTEM_SNAKE:
        if (FIGHTING(ch))
          bonus -= 1;
        else
          bonus += 2;
        break;
      case TOTEM_OWL:
        if (time_info.hours > 6 && time_info.hours < 19)
          break;
      case TOTEM_BEAR:
        bonus += 2;
        break;
      case TOTEM_LION:
        bonus -= 1;
        break;
      default:
        break;
      }
      break;
    case SPELL_CATEGORY_ILLUSION:
      switch (GET_TOTEM(ch)) {
      case TOTEM_SNAKE:
        if (FIGHTING(ch))
          bonus -= 1;
        else
          bonus += 2;
        break;
      case TOTEM_OWL:
        if (time_info.hours > 6 && time_info.hours < 19)
          break;
      case TOTEM_CAT:
      case TOTEM_RAT:
        bonus += 2;
        break;
      case TOTEM_GATOR:
        bonus -= 1;
        break;
      default:
        break;
      }
      break;
    case SPELL_CATEGORY_MANIPULATION:
      switch (GET_TOTEM(ch)) {
      case TOTEM_OWL:
        if (time_info.hours > 6 && time_info.hours < 19)
          break;
      case TOTEM_RACCOON:
      case TOTEM_RAVEN:
        bonus += 2;
        break;
      case TOTEM_SNAKE:
        if (!FIGHTING(ch))
          break;
        bonus -= 1;
        break;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }

  return bonus;
}



void resist_drain(struct char_data *ch, int force, spell_t *spell, int wound)
{
  int successes, drain;
  int old_physical, old_mental;
  int staged_wound;
  int wil_dice = 0, foci_dice = 0, pool_dice = 0, tnum;
  bool is_physical = FALSE;

  //  if (!IS_NPC(ch) && access_level(ch, LVL_ADMIN))
  //  return;

  if (force > (GET_MAG(ch) / 100))
    is_physical = TRUE;

  wil_dice = GET_WIL(ch);
  if (spell)
  {
    foci_dice = foci_bonus(ch, spell, force, FALSE);
    pool_dice = magic_pool_bonus(ch, spell, force, FALSE);
    tnum = MAX(2, (force >> 1) + DRAIN_POWER(spell->drain));
  } else
    tnum = force;
  successes = success_test(wil_dice + foci_dice + pool_dice, tnum);

  staged_wound = dec_staging(successes, wound);
  drain = convert_damage(staged_wound);

  old_physical = GET_PHYSICAL(ch);
  old_mental = GET_MENTAL(ch);
  if (is_physical)
  {
    GET_PHYSICAL(ch) -= drain * 100;
  } else
  {
    GET_MENTAL(ch) -= drain * 100;
    if (GET_MENTAL(ch) < 0) {
      GET_PHYSICAL(ch) += GET_MENTAL(ch);
      GET_MENTAL(ch) = 0;
    }
  }

  if (1 && spell)
  {
    char rbuf[MAX_STRING_LENGTH];
    sprintf( rbuf,
             "Drain: F:%d/%d, Mag:%d, WFP/T %d+%d+%d/%d. %d(%d)->%d, D: %d%c. %d->%d/%d->%d",
             force, spell->force,
             GET_MAG(ch), wil_dice, foci_dice, pool_dice, tnum, wound,
             successes, staged_wound, drain,
             is_physical ? 'P' : 'M',
             old_physical, GET_PHYSICAL(ch), old_mental, GET_MENTAL(ch));
    act( rbuf, 1, ch, NULL, NULL, TO_ROLLS );
  }

  update_pos(ch);
  if ((GET_POS(ch) <= POS_STUNNED) && (GET_POS(ch) > POS_DEAD))
  {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("You are unable to resist the drain from using magic and fall unconscious!\r\n", ch);
    act("$n collapses unconscious from the use of magic!", FALSE, ch, 0, 0, TO_ROOM);
  } else
  {
    if (GET_POS(ch) == POS_DEAD) {
      if (FIGHTING(ch))
        stop_fighting(ch);
      send_to_char("The energy from the spell overloads your systems, killing you...\r\n", ch);
      act("$n collapses DEAD from an overload of magic!", FALSE, ch, 0, 0, TO_ROOM);
      die(ch);
    }
  }
}

void sustain_spell(int force, struct char_data *ch, struct char_data *victim, spell_t *spell, int level)
{
  char xbuf[MAX_STRING_LENGTH];
  struct affected_type af, af2;
  int success;
  int dice, tnum;
  int fbonus, tbonus, pbonus;
  int mod;
  sh_int health = 0, attrib = 0;
  char *to_vict = NULL;
  char *to_room = NULL;

  if (victim == NULL || ch == NULL)
    return;

  if (spell->type == SPELL_HEAL || spell->type == SPELL_ANTIDOTE ||
      spell->type == SPELL_STABILIZE || spell->type == SPELL_CURE_DISEASE)
    health = 1;
  else if (spell->type == SPELL_DECREASE_ATTRIB || spell->type == SPELL_INCREASE_ATTRIB ||
           spell->type == SPELL_INCREASE_REFLEXES)
    attrib = 1;

  if (affected_by_spell(victim, spell->type))
  {
    if (!health)
      sprintf(buf, "%s already affected by that spell.", (ch == victim ?
              "You are" : "$N is"));
    else
      strcpy(buf, "The spell has no effect.");
    act(buf, FALSE, ch, 0, victim, TO_CHAR);
    return;
  }

  if (GET_SUSTAINED(ch) >= (ubyte)(GET_SKILL(ch, SKILL_SORCERY)) && !health)
  {
    send_to_char("You can't sustain any more spells.  Release one first.\r\n", ch);
    return;
  }

  af.type = spell->type;
  af.bitvector = 0;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.duration = -1;
  af.sustained_by = (IS_NPC(ch) ? -ch->nr : GET_IDNUM(ch));
  af.caster = FALSE;

  af2.type = spell->type;
  af2.bitvector = 0;
  af2.modifier = 0;
  af2.location = APPLY_NONE;
  af2.duration = -1;
  af2.sustained_by = (IS_NPC(victim) ? -victim->nr : GET_IDNUM(victim));
  af2.caster = TRUE;

  tbonus = totem_bonus(ch, spell);
  fbonus = foci_bonus(ch, spell, force, TRUE);
  pbonus = magic_pool_bonus(ch, spell, force, TRUE);

  dice = force + tbonus + pbonus + fbonus;

  tnum = (spell->type == SPELL_ANTI_BULLET ||
          spell->type == SPELL_ANTI_SPELL) ? 6 : 4;

  xbuf[0] = ' ';
  xbuf[1] = 0;
  mod = modify_target_rbuf(ch, xbuf);

  success = success_test(dice, tnum + mod);

  if ( 1 )
  {
    char rbuf[MAX_STRING_LENGTH];
    sprintf(rbuf,
            "SusSpell: F:%d/%d, Mag:%d, FFPT/TM %d+%d+%d+%d/%d+%d.  %d.%s",
            force, spell->force,
            GET_MAG(ch),
            force, fbonus, pbonus, tbonus,
            tnum, mod, success,
            xbuf);
    act(rbuf, 1, ch, NULL, NULL, TO_ROLLS);
  }


  if (success < 1 && !health && !attrib)
  {
    send_to_char("Your spell force doesn't seem powerful enough.\r\n", ch);
    return;
  }

  switch (spell->type)
  {
  case SPELL_ANTIDOTE:
  case SPELL_CURE_DISEASE:
  case SPELL_HEAL:
  case SPELL_STABILIZE:
    af.duration = level;
    af2.duration = level;
    break;
  case SPELL_ANTI_BULLET:
    to_vict = "A shimmering sphere surrounds you.";
    to_room = "A shimmering sphere surrounds $n.";
    af.modifier = force;
    af.location = APPLY_BALLISTIC;
    break;
  case SPELL_ANTI_SPELL:
    to_vict = "A glowing sphere surrounds you briefly, then disappears.";
    to_room = "A glowing sphere surrounds $n briefly.";
    af.modifier = (int)(force / 2);
    break;
  case SPELL_ARMOR:
    af.modifier = (int)(success / 2);
    af.location = APPLY_BOD;
    to_vict = "You feel your skin toughen!";
    break;
  case SPELL_CHAOTIC_WORLD:
    af.type = SPELL_CHAOS;
    af2.type = SPELL_CHAOS;
  case SPELL_CHAOS:
    success -= success_test(GET_WIL(victim) + spell_resist(victim),
                            force + modify_target(victim));

    if (success < 1) {
      act("$N seems to resist the effects!", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    af.modifier = success;
    af.location = APPLY_TARGET;
    to_vict = "You feel disorientated!";
    break;
  case SPELL_COMBAT_SENSE:
    af.modifier = (int)(success / 2);
    af.location = APPLY_COMBAT_POOL;
    to_vict = "You feel your combat sense improve!";
    if ( af.modifier == 0 ) {
      send_to_char("Your casting wasn't powerful enough.\r\n", ch);
      return;
    }
    break;
  case SPELL_CONFUSION:
    success -= success_test(GET_WIL(victim) + spell_resist(victim),
                            force + modify_target(victim));
    if (success < 1) {
      act("$N seems to resist the effects!", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    success = stage(success, 1);
    af.modifier = success;
    af.location = APPLY_TARGET;
    to_vict = "You feel confused!";
    break;
  case SPELL_DETECT_ALIGNMENT:
    af.bitvector = AFF_DETECT_ALIGN;
    to_vict = "Your eyes tingle.";
    break;
  case SPELL_DETECT_INVIS:
    to_vict = "Your eyes tingle.";
    af.bitvector = AFF_DETECT_INVIS;
    break;
  case SPELL_DETECT_MAGIC:
    to_vict = "Your eyes tingle.";
    af.bitvector = AFF_DETECT_MAGIC;
    break;
  case SPELL_INCREASE_REFLEXES:
    af.modifier = spell->damage;
    af.location = APPLY_INITIATIVE_DICE;
    if (GET_INIT_DICE(ch) > 0) {
      if (ch == victim)
        send_to_char("Your reflexes are already modified.", ch);
      else
        act("$S reflexes are already modified.", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    success = success_test(force + spell_bonus(ch, spell),
                           2 * GET_REA(victim) + modify_target(ch));
    if (success < 1) {
      if (ch == victim)
        send_to_char(NOEFFECT, ch);
      else
        act("Your spell has no effect on $M.", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    break;
  case SPELL_INFLUENCE:
    act("$N seems to resist the effects!", FALSE, ch, 0, victim, TO_CHAR);
    break;
  case SPELL_IMPROVED_INVIS:
    to_room = "$n slowly fades out of existence.";
    to_vict = "You vanish.";
    af.bitvector = AFF_IMP_INVIS;
    break;
  case SPELL_INVISIBILITY:
    to_room = "$n slowly fades out of existence.";
    to_vict = "You vanish.";
    af.bitvector = AFF_INVISIBLE;
    break;
  case SPELL_LIGHT:
    to_room = "A softly glowing sphere appears from $n's hands.";
    to_vict = "A small sphere of light appears and begins to hover about your head.";
    world[victim->in_room].light++;
    break;
  case SPELL_OVERSTIMULATION:
    success -= success_test(GET_WIL(victim) + spell_resist(victim), force + modify_target(victim));
    if (success < 1) {
      act("$N seems to resist the effects!", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (success >= 10) {
      af.bitvector = AFF_PETRIFY;
      to_vict = "Your brain screams in pain, and your muscles become distant!";
    }
    if (success >= 6)
      af.modifier = 3;
    else if (success >= 3)
      af.modifier = 2;
    else if (success >= 1)
      af.modifier = 1;
    else
      af.modifier = 0;
    af.location = APPLY_TARGET;
    break;
  case SPELL_PETRIFY:
    success -= success_test(GET_BOD(victim) + spell_resist(victim), force + modify_target(victim));
    if (success < 1) {
      act("$N seems to resist the effects!", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    af.bitvector = AFF_PETRIFY;
    af.modifier = success;
    to_vict = "Your muscles seem to become solid stone.";
    to_room = "$n freezes in place, $s skin turning a textured gray.";
    break;
  case SPELL_POISON:
    success -= resisted_test(GET_BOD(victim) + spell_resist(victim), force + modify_target(victim),
                             force, GET_BOD(victim) + modify_target(ch));
    if (success < 1) {
      act("$N seems to resist the effects!", FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    af.modifier = level;
    af.duration = success;
    af.bitvector = AFF_POISON;
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;
  case SPELL_RESIST_PAIN:
    to_vict = "You lose feeling of your physical wounds.";
    af.modifier = level;
    break;
  }

  affect_to_char(victim, &af);
  affect_to_char(ch, &af2);
  GET_SUSTAINED(ch) += 1;
  if (!health)
    send_to_char("The spell is sustained.\r\n", ch);

  /* send messages to room and victim if necessary */
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

/* the way a mage/shaman stops sustaining a spell */
ACMD(do_release)
{
  char who[255], *spell;
  struct char_data *vict;
  int spellnum;
  static struct affected_type *af;

  if (!*argument) {
    send_to_char("What spell and who do you wish to release it from?\r\n", ch);
    return;
  }
  spell = any_one_arg(argument, who);

  if (!(vict = get_char_room_vis(ch, who))) {
    send_to_char("You can't seem to find that person here.\r\n", ch);
    return;
  }

  if (!spell) {
    send_to_char("What spell do you wish to release?\r\n", ch);
    return;
  }
  spellnum = find_skill_num(spell);
  if (spellnum == -1) {
    send_to_char("What spell is that?\r\n", ch);
    return;
  } else if (spellnum == SPELL_HEAL || spellnum == SPELL_CURE_DISEASE || spellnum == SPELL_STABILIZE ||
             spellnum == SPELL_ANTIDOTE) {
    send_to_char("How do you expect to release that?!\r\n", ch);
    return;
  }

  if (affected_by_spell(vict, spellnum) != 1) {
    act("$N doesn't have that spell sustained on $M.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  for (af = vict->affected; af; af = af->next)
    if ((af->type == spellnum) && (af->caster == FALSE)) {
      if (af->sustained_by != (IS_NPC(ch) ? -ch->nr : GET_IDNUM(ch))) {
        send_to_char("But you aren't sustaining that spell!\r\n", ch);
        return;
      } else {
        sprintf(buf, "%s\r\n", spell_wear_off_msg[af->type]);
        send_to_char(buf, vict);
        affect_remove(vict, af, 1);
        return;
      }
    }
}

#define NUM_SPIRITS       17

ACMD(do_bond)
{
  int i;
  struct obj_data *obj = NULL, *weapon = NULL, *temp;
  spell_t *spell;
  char *pbuf;

  pbuf = any_one_arg(argument, arg);
  while(*pbuf == ' ')
    pbuf++;

  if (!*arg) {
    send_to_char("Usage: bond <focus> <spell/category/weapon>\r\n"
                 "       bond <docwagon>\r\n", ch);
    return;
  }

  if (IS_NPC(ch)) {
    send_to_char("Mobs can't bond; go away.\r\n", ch);
    return;
  }

  for (i = 0; !obj && i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && isname(arg, GET_EQ(ch, i)->text.keywords))
      obj = GET_EQ(ch, i);

  if (!obj)
    for (temp = ch->carrying; !obj && temp; temp = temp->next_content)
      if (isname(arg, temp->text.keywords))
        obj = temp;

  if (!obj) {
    send_to_char(ch, "You do not seem to have a '%s'.\r\n", arg);
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_DOCWAGON) {
    if (GET_OBJ_VAL(obj, 1)) {
      act("$p has already been activated.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    GET_OBJ_VAL(obj, 1) = GET_IDNUM(ch);
    act("$p's lights begin to subtly flash in a rhythmic sequence.", FALSE,
        ch, obj, 0, TO_CHAR);
    return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_FOCUS) {
    send_to_char("You can only bond foci and DocWagon contracts.\r\n", ch);
    return;
  }
  if (GET_TRADITION(ch) == TRAD_MUNDANE) {
    send_to_char("Mundanes can't bond foci.\r\n", ch);
    return;
  } else if (GET_FOCI(ch) >= GET_INT(ch)) {
    send_to_char("You cannot bond any more foci.\r\n", ch);
    return;
    /*  } else if (GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) > 0
          || GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM)) {
    */
  } else if (GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM)) {
    send_to_char("That focus has already been bonded.\r\n", ch);
    return;
  } else if (GET_TRADITION(ch) == TRAD_ADEPT
             && GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE) != FOCI_WEAPON) {
    send_to_char("Adepts can only bond weapon foci.\r\n", ch);
    return;
  }

  if (obj->worn_by == ch)
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      affect_modify(ch,
                    obj->affected[i].location,
                    obj->affected[i].modifier,
                    obj->obj_flags.bitvector, FALSE);

  switch (GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE)) {
  case FOCI_SPELL:
    if (GET_KARMA(ch) < (GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 100)) {
      send_to_char(ch, "You do not have %d karma to bond this focus.\r\n",
                   GET_OBJ_VAL(obj, VALUE_FOCUS_RATING));
      return;
    }
    spell = find_spell( ch, pbuf );
    if ( !spell ) {
      send_to_char(ch, "You do not know spell '%s'.\r\n", pbuf);
      return;
    }
    GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) = GET_IDNUM(ch);
    GET_OBJ_VAL(obj, VALUE_FOCUS_CAT) = spell->type;
    GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY) = spell->type;
    GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) = 1;
    sprintf(buf, "You have successfully bonded $p (%s).",
            spells[GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY)]);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    GET_KARMA(ch) -= GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 100;
    GET_FOCI(ch)++;
    break;
  case FOCI_SPELL_CAT:
    if (GET_KARMA(ch) < (GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 300)) {
      send_to_char(ch, "You do not have %d karma to bond this focus.\r\n",
                   GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 3);
      return;
    }
    for( i = 0; *spell_categories[i] != '\n'; i++ )
      if (is_abbrev(pbuf, spell_categories[i]))
        break;

    if ( *spell_categories[i] == '\n' ) {
      send_to_char("You must specify a category for this focus to assist.\r\n", ch);
      return;
    }
    GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) = GET_IDNUM(ch);
    GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY) = i;
    GET_OBJ_VAL(obj, VALUE_FOCUS_CAT) = i;
    GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) = 1;
    GET_KARMA(ch) -= GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 300;
    GET_FOCI(ch)++;
    sprintf(buf, "You have successfully bonded $p (%s).",
            spell_categories[GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY)]);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    break;
  case FOCI_SPIRIT:
    if (GET_KARMA(ch) < (GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 200)) {
      send_to_char(ch, "You do not have %d karma to bond this focus.\r\n",
                   GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 2);
      return;
    }
    for( i = 0; *spirits[i] != '\n'; i++ )
      if (is_abbrev(pbuf, spirits[i]))
        break;
    if ( *spell_categories[i] == '\n' ) {
      send_to_char("You must specify a spirit type for this focus to assist.\r\n", ch);
      return;
    }
    GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) = GET_IDNUM(ch);
    GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY) = i;
    GET_OBJ_VAL(obj, VALUE_FOCUS_CAT) = i;
    GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) = 1;
    GET_KARMA(ch) -= GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 200;
    GET_FOCI(ch)++;
    sprintf(buf, "You bond $p (%s).",
            spirits[GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY)]);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    break;
  case FOCI_POWER:
    if (GET_KARMA(ch) < (GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 500)) {
      send_to_char(ch, "You do not have %d karma to bond this focus.\r\n",
                   GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 5);
      return;
    }
    GET_KARMA(ch) -= GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 500;
    GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) = GET_IDNUM(ch);
    GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) = 1;
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (!obj->affected[i].modifier) {
        obj->affected[i].modifier = GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
        obj->affected[i].location = APPLY_MAG;
        affect_modify(ch,
                      obj->affected[i].location,
                      obj->affected[i].modifier,
                      obj->obj_flags.bitvector, TRUE);
        break;
      }
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (!obj->affected[i].modifier) {
        obj->affected[i].modifier = GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
        obj->affected[i].location = APPLY_MAGIC_POOL;
        affect_modify(ch,
                      obj->affected[i].location,
                      obj->affected[i].modifier,
                      obj->obj_flags.bitvector, TRUE);
        break;
      }
    act("You have successfully bonded $p.", FALSE, ch, obj, 0, TO_CHAR);
    GET_FOCI(ch)++;
    break;
  case FOCI_WEAPON:
    if (GET_KARMA(ch) < (GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 500)) {
      send_to_char(ch, "You do not have %d karma to bond this focus.\r\n",
                   GET_OBJ_VAL(obj, VALUE_FOCUS_RATING) * 5);
      return;
    }
    for (i = 0; !weapon && i < NUM_WEARS; i++)
      if (GET_EQ(ch, i) && isname(pbuf, GET_EQ(ch, i)->text.keywords))
        weapon = GET_EQ(ch, i);
    if (!weapon)
      for (temp = ch->carrying; !weapon && temp; temp = temp->next_content)
        if (isname(pbuf, temp->text.keywords))
          weapon = temp;
    if (!weapon) {
      send_to_char(ch, "You do not seem to have a '%s'.\r\n", pbuf);
      break;
    }
    if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON) {
      send_to_char("You can only bond weapon foci to weapons.\r\n", ch);
      break;
    }
    if (GET_OBJ_VAL(weapon, 3) == TYPE_HIT || GET_OBJ_VAL(weapon, 3) >= TYPE_TASER) {
      send_to_char("That is not a suitable weapon to bond a foci to.\r\n", ch);
      return;
    }
    GET_KARMA(ch) -= GET_OBJ_VAL(obj, 1) * 500;
    GET_FOCI(ch)++;
    GET_OBJ_VAL(weapon, 9) = GET_IDNUM(ch);
    GET_OBJ_VAL(weapon, 8) = GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);
    GET_OBJ_VAL(weapon, 7) = GET_OBJ_VNUM(obj);
    GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) = 1;
    sprintf(buf, "%s melts as its power flows into %s.\r\n",
            CAP(obj->text.name),
            weapon->text.name);
    send_to_char(buf, ch);
    extract_obj(obj);
    break;
  }
}

ACMD(do_status)
{
  struct affected_type *af;
  bool last_cast = FALSE, found = FALSE;
  struct char_data *tch, *target;
  struct obj_data *obj;
  int i;

  if (IS_AFFECTED(ch, AFF_DETECT_MAGIC) || IS_ASTRAL(ch) || IS_DUAL(ch)) {
    skip_spaces(&argument);
    if (!*argument)
      target = ch;
    else
      target = get_char_room_vis(ch, argument);
  } else
    target = ch;

  if (!target) {
    send_to_char("You can't seem to find that person.\r\n", ch);
    return;
  }

  sprintf(buf, "%s affected by:", (ch == target ? "You are" : "$N is"));
  act(buf, FALSE, ch, 0, target, TO_CHAR);

  if (ch == target) {
    for (i = 0; i < NUM_WEARS; i++)
      if ((obj = GET_EQ(ch, i))
          && GET_OBJ_TYPE(obj) == ITEM_FOCUS
          && GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE) == FOCI_LOCK
          && GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) == GET_IDNUM(ch)) {
        sprintf(buf, "  %-20s          Locked on: $p",
                spells[GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY)]);
        act(buf, FALSE, ch, obj, 0, TO_CHAR);
        found = TRUE;
      }
    /*
        for (obj = ch->carrying; obj; obj = obj->next_content)
          if (GET_OBJ_TYPE(obj) == ITEM_FOCUS
       && GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE) == FOCI_LOCK
       && GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) == GET_IDNUM(ch)) {
            sprintf(buf, "  %-20s          Locked on: $p",
      spells[GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY)]);
            act(buf, FALSE, ch, obj, 0, TO_CHAR);
            found = TRUE;
          }*/
  }

  for (af = target->affected; af; af = af->next) {
    if (af->caster == TRUE && af->sustained_by ==
        (IS_NPC(target) ? -target->nr : GET_IDNUM(target)))
      last_cast = TRUE;
    else {
      if (af->caster == FALSE && af->type != SPELL_HEAL && af->type != SPELL_CURE_DISEASE &&
          af->type != SPELL_STABILIZE && af->type != SPELL_ANTIDOTE) {
        if (last_cast == TRUE) {
          last_cast = FALSE;
          if (ch == target)
            sprintf(buf, "  %-20s       Sustained by: yourself", spells[af->type]);
          else
            sprintf(buf, "  %-20s", spells[af->type]);
          act(buf, FALSE, ch, 0, target, TO_CHAR);
          found = TRUE;
        } else {
          tch = character_list;
          if (ch == target)
            sprintf(buf, "  %-20s       Sustained by: $N", spells[af->type]);
          else
            sprintf(buf, "  %-20s", spells[af->type]);
          found = TRUE;
          if (af->sustained_by < 0)
            while (tch != NULL && (IS_NPC(tch) ? tch->nr : 0) != -af->sustained_by)
              tch = tch->next;
          else
            while (tch != NULL && (!IS_NPC(tch) ? GET_IDNUM(tch) : 0) != af->sustained_by)
              tch = tch->next;
          act(buf, FALSE, ch, 0, tch, TO_CHAR);
        }
      }
    }
  }
  if (!found)
    send_to_char("  Nothing.\r\n", ch);

  found = FALSE;
  last_cast = FALSE;

  if (ch != target)
    return;

  if (GET_TRADITION(ch) != TRAD_SHAMANIC && GET_TRADITION(ch) != TRAD_HERMETIC &&
      !(!IS_NPC(ch) && access_level(ch, LVL_ADMIN)))
    return;

  send_to_char("\r\nYou are sustaining:\r\n", ch);

  if (!ch->affected)
    send_to_char("  Nothing.\r\n", ch);
  else {
    for (af = ch->affected; af; af = af->next) {
      if (af->caster == TRUE
          && af->type != SPELL_HEAL
          && af->type != SPELL_CURE_DISEASE
          && af->type != SPELL_STABILIZE
          && af->type != SPELL_ANTIDOTE) {
        if (af->sustained_by == (IS_NPC(ch) ? -ch->nr : GET_IDNUM(ch)))
          sprintf(buf, "  %-20s       Sustained on: yourself", spells[af->type]);
        else
          sprintf(buf, "  %-20s       Sustained on: $N", spells[af->type]);
        found = TRUE;
        tch = character_list;
        if (af->sustained_by < 0)
          while (tch != NULL && (IS_NPC(tch) ? tch->nr : 0) != -af->sustained_by)
            tch = tch->next;
        else
          while (tch != NULL && (!IS_NPC(tch) ? GET_IDNUM(tch) : 0) != af->sustained_by)
            tch = tch->next;
        act(buf, FALSE, ch, 0, tch, TO_CHAR);
      }
    }
    if (!found)
      send_to_char("  Nothing.\r\n", ch);
  }
}

int check_spirit_sector(int room, int spirit)
{
  if (spirit < 5)
    return 1;

  switch (spirit) {
  case 5:
    if (world[room].sector_type != SECT_CITY)
      return 0;
    break;
  case 6:
    if (world[room].sector_type != SECT_INSIDE)
      return 0;
    break;
  case 7:
  case 8:
  case 11:
    if (world[room].sector_type != SECT_FIELD)
      return 0;
    break;
  case 9:
    if (world[room].sector_type != SECT_FOREST)
      return 0;
    break;
  case 10:
    if (world[room].sector_type != SECT_MOUNTAIN && world[room].sector_type != SECT_HILLS)
      return 0;
    break;
  case 12:
  case 13:
    if (world[room].sector_type == SECT_INSIDE || world[room].sector_type > SECT_FLYING)
      return 0;
    break;
  case 14:
  case 15:
  case 17:
    if (world[room].sector_type != SECT_WATER_SWIM && world[room].sector_type != SECT_WATER_NOSWIM)
      return 0;
    break;
  case 16:
    if (world[room].sector_type != SECT_WATER_NOSWIM && world[room].sector_type != SECT_UNDERWATER)
      return 0;
    break;
  }

  return 1;
}

int spirit_bonus(struct char_data *ch, int spirit)
{
  int i, bonus = 0;
  struct obj_data *obj;

  if (IS_NPC(ch))
    return 0;

  for (i = 0; !bonus && i < (NUM_WEARS - 1); i++)
    if ((obj = GET_EQ(ch, i))
        && GET_OBJ_TYPE(obj) == ITEM_FOCUS
        && GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) == GET_IDNUM(ch)
        && GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY) == spirit
        && GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) == 1)
      bonus = GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);

  for (obj = ch->carrying; !bonus && obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_FOCUS
        && GET_OBJ_VAL(obj, VALUE_FOCUS_CHAR_IDNUM) == GET_IDNUM(ch)
        && GET_OBJ_VAL(obj, VALUE_FOCUS_SPECIFY) == spirit
        && GET_OBJ_VAL(obj, VALUE_FOCUS_BONDED) == 1)
      bonus = GET_OBJ_VAL(obj, VALUE_FOCUS_RATING);

  if (GET_TRADITION(ch) == TRAD_SHAMANIC)
    switch (GET_TOTEM(ch))
    {
    case TOTEM_BEAR:
      if (spirit == 9)
        bonus += 2;
      break;
    case TOTEM_RACCOON:
    case TOTEM_CAT:
      if (spirit == 5)
        bonus += 2;
      break;
    case TOTEM_DOG:
      if (spirit == 6 || spirit == 7)
        bonus += 2;
      break;
    case TOTEM_EAGLE:
    case TOTEM_RAVEN:
      if (spirit == 13)
        bonus += 2;
      break;
    case TOTEM_GATOR:
      if ((!number(0, 1) ? spirit == 5 : spirit == 14 || spirit == 15 || spirit == 17))
        bonus += 2;
    case TOTEM_LION:
      if (spirit == 11)
        bonus += 2;
      break;
    case TOTEM_OWL:
      if (time_info.hours <= 6 || time_info.hours >= 19)
        bonus += 2;
      break;
    case TOTEM_RAT:
      if (spirit >= 5 && spirit <= 7)
        bonus += 2;
      break;
    case TOTEM_SHARK:
      if (spirit == 16)
        bonus += 2;
      break;
    case TOTEM_SNAKE:
      if (!number(0, 1)) {
        if (spirit == (5 + number(0, 2)))
          bonus += 2;
      } else if (spirit == (7 + number(0, 2)))
        bonus +=  2;
      break;
    case TOTEM_WOLF:
      if ((!number(0, 1) ? spirit == 9 : spirit == 11))
        bonus += 2;
      break;
    }
  return bonus;
}

ACMD(do_conjure)
{
  int num, i, force, successes, drain, wound, is_physical;
  struct char_data *spirit;
  char *s;

  if (IS_NPC(ch) || (GET_TRADITION(ch) != TRAD_HERMETIC &&
                     GET_TRADITION(ch) != TRAD_SHAMANIC && !access_level(ch, LVL_VICEPRES))) {
    send_to_char("You aren't able to conjure beings!\r\n", ch);
    return;
  }

  if (GET_MAG(ch) < 100) {
    send_to_char("You have to have a magic rating of at least one to conjure.\r\n", ch);
    return;
  } else if (GET_SKILL(ch, SKILL_CONJURING) < 1) {
    send_to_char("You have to have some conjuring skill to attempt that!\r\n", ch);
    return;
  }
  any_one_arg(argument, arg);

  if (!*arg || !*argument) {
    send_to_char("Usage: conjure <force> <spirit/elemental>\r\n", ch);
    return;
  }

  if ((force = atoi(arg)) < 1) {
    send_to_char("You must supply a valid force for that to work.\r\n", ch);
    return;
  }

  s = strtok(argument, "'");

  if (s == NULL) {
    send_to_char("You must specify the name of the spirit you wish to conjure.\r\n", ch);
    return;
  }

  s = strtok(NULL, "'");

  if (s == NULL)  {
    send_to_char("Spirit names must be enclosed in single quotes (').\r\n", ch);
    return;
  }

  for (i = 1; *spirits[i] != '\n'; i++)
    if (is_abbrev(s, spirits[i]))
      break;

  if (*spirits[i] == '\n') {
    send_to_char(ch, "There is no '%s' spirit.\r\n", s);
    return;
  }

  if (GET_TRADITION(ch) == TRAD_HERMETIC && i > 4) {
    send_to_char("Hermetic mages can only conjure elementals.\r\n", ch);
    return;
  } else if (GET_TRADITION(ch) == TRAD_HERMETIC &&
             !ROOM_FLAGGED(ch->in_room, ROOM_HERMETIC_LIBRARY)) {
    send_to_char("Conjuring elementals requires a hermetic library.\r\n", ch);
    return;
  } else if (GET_TRADITION(ch) == TRAD_SHAMANIC) {
    if (i < 5) {
      send_to_char("Shamans can only conjure nature spirits.\r\n", ch);
      return;
    }
    if (!check_spirit_sector(ch->in_room, i)) {
      send_to_char("You can't conjure that spirit here.\r\n", ch);
      return;
    }
  }

  if ((num = real_mobile(24 + i)) < 0) {
    log("No %s mob", spirits[i]);

    send_to_char("The conjuring system seems to be screwed up right now...\r\n", ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
    send_to_char(ch, "Your attempt to conjure %s fizzles into oblivion.\r\n", spirits[i]);
    return;
  }

  successes = success_test(GET_CHA(ch) + (spirit_bonus(ch,i)/2),
                           force + modify_target(ch));
  if (force < (GET_CHA(ch) / 2)) {
    wound = LIGHT;
    is_physical = FALSE;
  } else if (force <= GET_CHA(ch)) {
    wound = MODERATE;
    is_physical = FALSE;
  } else if (force > (GET_CHA(ch) * 2)) {
    wound = DEADLY;
    is_physical = TRUE;
  } else {
    wound = SERIOUS;
    is_physical = TRUE;
  }

  drain = convert_damage(dec_staging(successes, wound));

  if (is_physical)
    GET_PHYSICAL(ch) -= drain * 100;
  else if ((int)(GET_MENTAL(ch) / 100) < drain) {
    GET_PHYSICAL(ch) -= (drain - (int)(GET_MENTAL(ch) / 100)) * 100;
    GET_MENTAL(ch) = 0;
  } else
    GET_MENTAL(ch) -= drain * 100;
  update_pos(ch);

  if (GET_POS(ch) <= POS_STUNNED && GET_POS(ch) > POS_DEAD) {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("You are unable to resist the drain from the evocation and fall unconscious!\r\n", ch);
    act("$n collapses unconscious from the use of magic!", FALSE, ch, 0, 0, TO_ROOM);
    return;
  } else if (GET_POS(ch) == POS_DEAD) {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("The energy from the evocation overloads your systems, killing you...\r\n", ch);
    act("$n collapses DEAD from an overload of magic!", FALSE, ch, 0, 0, TO_ROOM);
    die(ch);
    return;
  }

  if ((successes = success_test(GET_SKILL(ch, SKILL_CONJURING)
                                + (spirit_bonus(ch, i)+1)/2,
                                force + modify_target(ch))) < 1) {
    send_to_char(ch, "You fail to conjure %s!\r\n", spirits[i]);
    return;
  }

  spirit = read_mobile(num, REAL);
  char_to_room(spirit, ch->in_room);
  GET_ACTIVE(spirit) = successes;
  affect_total(spirit);
  act("$n appears from thin air.", TRUE, spirit, 0, 0, TO_ROOM);
  act("$n fades in from the air.", FALSE, spirit, 0, 0, TO_CHAR);
  AFF_FLAGS(spirit).SetBit(AFF_CHARM);
  add_follower(spirit, ch);
}

void add_to_spell_q(struct char_data *ch, spell_t *spell, int force, char *arg)
{
  spell_t *temp, *i;

  temp = new spell_t;
  *temp = *spell;
  temp->force = force;

  if (arg)
  {
    temp->name = new char[strlen(arg)+1];
    strcpy(temp->name, arg);
  } else
    temp->name = NULL;

  if (!GET_SPELL_Q(ch))
  {
    GET_SPELL_Q(ch) = temp;
    GET_SPELL_Q(ch)->next = NULL;
  } else
  {
    // this loop goes until i->next is NULL, or until temp is the last
    for (i = GET_SPELL_Q(ch); i->next; i = i->next)
      ;
    i->next = temp;
    temp->next = NULL;
  }
}

void delete_top_of_spell_q(struct char_data *ch)
{
  if (!GET_SPELL_Q(ch))
    return;

  spell_t *temp = GET_SPELL_Q(ch);
  GET_SPELL_Q(ch) = GET_SPELL_Q(ch)->next;

  if (temp->name)
    delete [] temp->name;
  delete temp;
}

void purge_spell_q(struct char_data *ch)
{
  while (GET_SPELL_Q(ch))
    delete_top_of_spell_q(ch);
}

bool cycle_spell_q(struct char_data *ch)
{
  spell_t *temp = GET_SPELL_Q(ch);
  spell_t spellcopy;
  bool success;

  if (!temp)
    return FALSE;

  // Make a copy of the spell data so any frees will not affect us.
  spellcopy = *temp;
  temp = &spellcopy;

  switch (temp->category)
  {
  case SPELL_CATEGORY_COMBAT:
    success = process_combat_target(ch, temp, temp->name, temp->force);
    break;
  case SPELL_CATEGORY_HEALTH:
    success = process_health_target(ch, temp, temp->name, temp->force);
    break;
  case SPELL_CATEGORY_ILLUSION:
    success = process_illusion_target(ch, temp, temp->name, temp->force);
    break;
  case SPELL_CATEGORY_MANIPULATION:
    success = process_manipulation_target(ch, temp, temp->name, temp->force);
    break;
  default:
    send_to_char("Your spell seems to make no sense to you whatsoever.\r\n", ch);
    mudlog("Unknown spell category in newmagic.cc", ch, LOG_SYSLOG, TRUE);
    delete_top_of_spell_q(ch);
    return FALSE;
  }

  // success determines if the mud found a target and the spell was cast
  if (success)
  {
    if (temp->type == SPELL_RESIST_PAIN)
      resist_drain(ch, temp->force, temp, success + 1);
    else if (temp->type != SPELL_HEAL && temp->type != SPELL_ANTIDOTE &&
             temp->type != SPELL_CURE_DISEASE && temp->type != SPELL_POISON)
      resist_drain(ch, temp->force, temp, DRAIN_LEVEL(temp->drain));
    else
      resist_drain(ch, temp->force, temp, success);
    delete_top_of_spell_q(ch);
    return TRUE;
  }

  delete_top_of_spell_q(ch);
  return FALSE;
}

// the actual cast command
ACMD(do_cast)
{
  spell_t *spell;
  char *s, *t;
  bool newforce = FALSE;
  int i, spellnum = 0;
  int delete_spell = 0;
  char argcopy[MAX_STRING_LENGTH];

  if (!IS_NPC(ch) && !access_level(ch, LVL_ADMIN) && GET_TRADITION(ch) != TRAD_SHAMANIC &&
      GET_TRADITION(ch) != TRAD_HERMETIC) {
    send_to_char("You are but a mundane, sorry.\r\n", ch);
    return;
  }

  if (GET_MAG(ch) < 100) {
    send_to_char("You must have a magic rating of at least one to cast spells.\r\n", ch);
    return;
  }

  any_one_arg(argument, arg);

  int force = atoi(arg);
  if (force > 0)
    newforce = TRUE;

  /* get: blank, spell name, target name */
  strcpy(argcopy, argument);
  s = strtok(argcopy, "'");

  if (s == NULL) {
    send_to_char("You must specify the name of the spell you wish to cast.\r\n", ch);
    return;
  }

  s = strtok(NULL, "'");

  if (s == NULL)  {
    send_to_char("Spell names must be enclosed in single quotes (').\r\n", ch);
    return;
  }

  if (!access_level(ch, LVL_ADMIN))
    spell = find_spell(ch, s);
  else {
    spell = find_spell(ch, s);
    if (!spell) {
      for (i = 3; !spellnum && i <= MAX_SPELLS; i++)
        if (is_abbrev(s, spells[i]))
          spellnum = i;

      if (!spellnum) {
        send_to_char(ch, "'%s' doesn't seem to be a standard spell.\r\n", s);
        return;
      }
      delete_spell = 1;
      spell = new spell_t;
      spell->name = str_dup(spells[spellnum]);
      spell->physical = grimoire[spellnum].physical;
      spell->category = grimoire[spellnum].category;
      spell->force = 10;
      spell->target = grimoire[spellnum].target;
      spell->drain = grimoire[spellnum].drain;
      spell->damage = grimoire[spellnum].damage;
      spell->effect = SPELL_EFFECT_NONE;
      spell->type = spellnum;
      spell->next = NULL;
    }
  }

  if (!spell) {
    send_to_char(ch, "You don't seem to know a spell named '%s'.\r\n", s);
    return;
  }

  if (newforce) {
    if (spell->force < force) {
      send_to_char("You can't cast a spell at a force higher than you learned it.\r\n", ch);
      return;
    }
  } else
    force = spell->force;

  if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
    send_to_char("Your magic fizzles into oblivion.\r\n", ch);
    return;
  }

  // pull off the last of the arguments and put it in t
  t = strtok(NULL, "\0");
  while(t && *t == ' ')
    t++;

  if (FIGHTING(ch) && spell->category != SPELL_CATEGORY_DETECTION) {
    add_to_spell_q(ch, spell, force, t);
    send_to_char(ch, "You begin to concentrate on casting %s.\r\n",
                 spell->name);
    if (delete_spell) {
      if (spell->name)
        delete [] spell->name;
      delete spell;
    }
    return;
  }

  bool success;
  switch (spell->category) {
  case SPELL_CATEGORY_COMBAT:
    success = process_combat_target(ch, spell, t, force);
    break;
  case SPELL_CATEGORY_DETECTION:
    success = process_detection_target(ch, spell, t, force);
    break;
  case SPELL_CATEGORY_HEALTH:
    success = process_health_target(ch, spell, t, force);
    break;
  case SPELL_CATEGORY_ILLUSION:
    success = process_illusion_target(ch, spell, t, force);
    break;
  case SPELL_CATEGORY_MANIPULATION:
    success = process_manipulation_target(ch, spell, t, force);
    break;
  default:
    send_to_char("Your spell seems to make no sense to you whatsoever.\r\n", ch);
    mudlog("Unknown spell category in newmagic.cc", ch, LOG_SYSLOG, TRUE);
    return;
  }

  // success determines if the mud found a target and the spell was cast
  if (success) {
    if (spell->type == SPELL_RESIST_PAIN)
      resist_drain(ch, force, spell, success + 1);
    else if (spell->type != SPELL_HEAL && spell->type != SPELL_ANTIDOTE &&
             spell->type != SPELL_CURE_DISEASE && spell->type != SPELL_POISON)
      resist_drain(ch, force, spell, DRAIN_LEVEL(spell->drain));
    else
      resist_drain(ch, force, spell, success);
    // if they are in combat, they lose a turn
    if (FIGHTING(ch) && !AFF_FLAGGED(ch, AFF_ACTION))
      AFF_FLAGS(ch).SetBit(AFF_ACTION);
    // also, they get a wait state for casting
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }
  if (delete_spell) {
    if (spell->name)
      delete [] spell->name;
    delete spell;
  }
}

struct char_data *range_spell(struct char_data *ch, char *target, char *direction)
{
  int room = ch->in_room, nextroom, dir, sight, distance;
  struct char_data *vict;
  bool found = FALSE;

  if ((dir = search_block(direction, lookdirs, FALSE)) == -1)
    return NULL;
  dir = convert_look[dir];

  if (CAN_GO2(room, dir))
    nextroom = EXIT2(room, dir)->to_room;
  else
    nextroom = NOWHERE;

  sight = find_sight(ch);

  for (distance = 1; ((nextroom != NOWHERE) && (distance <= sight)); distance++)
  {
    for (vict = world[nextroom].people; vict; vict = vict->next_in_room) {
      if (isname(target, GET_KEYWORDS(vict)) && vict != ch && CAN_SEE(ch, vict)) {
        found = TRUE;
        break;
      }
    }

    if (found == TRUE) {
      if (world[vict->in_room].peaceful) {
        send_to_char("Nah - leave them in peace.\r\n", ch);
        return ch;
      }

      if (ROOM_FLAGGED(vict->in_room, ROOM_NOMAGIC)) {
        act("Your spell vanishes before reaching $M.", FALSE, ch, 0, vict, TO_CHAR);
        return ch;
      }

      if ( !IS_NPC(vict) && str_cmp(vict->player.char_name,target) ) {
        send_to_char("You need to type the player's ENTIRE name.\n\r",ch);
        return ch;
      }

      if (!ok_damage_shopkeeper(ch, vict)) {
        send_to_char("Maybe that's not such a good idea.\r\n", ch);
        return ch;
      }

      return vict;
    }

    room = nextroom;
    if (CAN_GO2(room, dir))
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;
  }

  return NULL;
}

int spell_damage_door(struct char_data *ch, spell_t *spell, char *target)
{
  int dir;
  char type[MAX_INPUT_LENGTH], door[MAX_INPUT_LENGTH];

  two_arguments(target, type, door);

  if (!spell->physical)
  {
    send_to_char("You can only cast that spell on living targets.\r\n", ch);
    return -1;
  }

  if (*door)
  {
    if ((dir = search_block(door, lookdirs, FALSE)) == -1)
      return 0;
    dir = convert_look[dir];

    int dist, nextroom, sight = find_sight(ch), room = ch->in_room;

    if (EXIT(ch, dir) && EXIT(ch, dir)->keyword &&
        isname(type, EXIT(ch, dir)->keyword) &&
        !IS_SET(EXIT(ch, dir)->exit_info, EX_DESTROYED)) {
      if (FIGHTING(ch)) {
        send_to_char("Maybe you'd better wait...\r\n", ch);
        return -1;
      } else if (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
        send_to_char("You can only damage closed doors!\r\n", ch);
        return -1;
      } else if (world[ch->in_room].peaceful) {
        send_to_char("Nah - leave it in peace.\r\n", ch);
        return -1;
      } else if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
        send_to_char(ch, "Your spell vanishes before reaching the %s.\r\n",
                     EXIT(ch, dir)->keyword ? fname(EXIT(ch, dir)->keyword)
                     : "door");
        return -1;
      }

      if (spell->category == SPELL_CATEGORY_MANIPULATION)
        damage_door(ch, ch->in_room, dir, (int)(spell->force),
                    spell->effect | DAMOBJ_MANIPULATION);
      else
        damage_door(ch, ch->in_room, dir, (int)(spell->force),
                    spell->effect);
      return 1;
    }

    if (CAN_GO2(room, dir))
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;

    for (dist = 1; nextroom != NOWHERE && dist <= sight; dist++) {
      if (EXIT2(nextroom, dir) && EXIT2(nextroom, dir)->keyword &&
          isname(type, EXIT2(nextroom, dir)->keyword) &&
          !IS_SET(EXIT2(nextroom, dir)->exit_info, EX_DESTROYED)) {
        if (FIGHTING(ch)) {
          send_to_char("Maybe you'd better wait...\r\n", ch);
          return -1;
        } else if (!IS_SET(EXIT2(nextroom, dir)->exit_info, EX_CLOSED)) {
          send_to_char("You can only damage closed doors!\r\n", ch);
          return -1;
        } else if (world[nextroom].peaceful) {
          send_to_char("Nah - leave it in peace.\r\n", ch);
          return -1;
        } else if (ROOM_FLAGGED(nextroom, ROOM_NOMAGIC)) {
          send_to_char(ch, "Your spell vanishes before reaching the %s.\r\n",
                       EXIT2(nextroom, dir)->keyword ?
                       fname(EXIT2(nextroom, dir)->keyword) : "door");
          return -1;
        }

        if (spell->category == SPELL_CATEGORY_MANIPULATION)
          damage_door(ch, nextroom, dir, (int)(spell->force),
                      spell->effect | DAMOBJ_MANIPULATION);
        else
          damage_door(ch, nextroom, dir, (int)(spell->force),
                      spell->effect);
        return 1;
      }
      room = nextroom;
      if (CAN_GO2(room, dir))
        nextroom = EXIT2(room, dir)->to_room;
      else
        nextroom = NOWHERE;
    }
  } else if (*type)
  {
    for (dir = 0; dir < NUM_OF_DIRS; dir++)
      if (EXIT(ch, dir) && EXIT(ch, dir)->keyword &&
          isname(type, EXIT(ch, dir)->keyword) &&
          !IS_SET(EXIT(ch, dir)->exit_info, EX_DESTROYED)) {
        if (FIGHTING(ch)) {
          send_to_char("Maybe you'd better wait...\r\n", ch);
          return -1;
        } else if (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
          send_to_char("You can only damage closed doors!\r\n", ch);
          return -1;
        } else if (world[ch->in_room].peaceful) {
          send_to_char("Nah - leave it in peace.\r\n", ch);
          return -1;
        }

        if (spell->category == SPELL_CATEGORY_MANIPULATION)
          damage_door(ch, ch->in_room, dir, (int)(spell->force),
                      spell->effect | DAMOBJ_MANIPULATION);
        else
          damage_door(ch, ch->in_room, dir, (int)(spell->force),
                      spell->effect);
        return 1;
      }
  }

  return 0;
}

// the processing of the spells follow by category
bool process_combat_target(char_t *ch, spell_t *spell, char *t, int force)
{
  char_t *vict = NULL;
  obj_t *obj = NULL;
  char targ1[80], targ2[80];
  int num_targets=0;
  sh_int room_num;
  int val;

  if (t)
    skip_spaces(&t);

  // first we find the appropriate target
  switch (spell->target) {
  case SPELL_TARGET_CASTER:
    if (t && !isname(t, GET_KEYWORDS(ch))) {
      send_to_char("You can only target yourself with this spell.\r\n", ch);
      return FALSE;
    }
    process_combat_spell(ch, ch, (obj_t *)NULL, spell, force);
    break;
  case SPELL_TARGET_TOUCH:
    if (!t) {
      if (!FIGHTING(ch)) {
        send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
        return FALSE;
      } else
        vict = FIGHTING(ch);
    } else {
      // here we look for a mob in the room first, then search for an item
      // in the room, then for an item in inventory
      if (!(vict = get_char_room_vis(ch, t)))
        if (!(val = spell_damage_door(ch, spell, t))) {
          if (!(obj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents)))
            if (!(obj = get_obj_in_list_vis(ch, t, ch->carrying))) {
              send_to_char("You can't seem to find the target you are "
                           "looking for.\r\n", ch);
              return FALSE;
            }
        } else
          return (val == 1 ? TRUE : FALSE);
    }
    if (vict && GET_POS(vict) == POS_STANDING) {
      if (resisted_test(GET_QUI(ch),
                        GET_QUI(vict) + modify_target(ch),
                        GET_QUI(vict),
                        GET_QUI(ch) + modify_target(vict)) < 1) {
        act("You stumble over your own legs as you try to touch $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!", FALSE, ch, 0, vict, TO_VICT);
        if (!FIGHTING(vict))
          set_fighting(vict, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, vict);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return FALSE;
      }
    } else if (vict && !STUNNED(vict)) {
      if (resisted_test(GET_QUI(ch),
                        (GET_QUI(vict) / 2) + modify_target(ch),
                        (GET_QUI(vict) / 2),
                        GET_QUI(ch) + modify_target(vict)) < 1) {
        act("You stumble over your own legs as you try to touch $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!", FALSE, ch, 0, vict, TO_VICT);
        if (!FIGHTING(vict))
          set_fighting(vict, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, vict);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return FALSE;
      }
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_combat_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) && (IS_ASTRAL(ch) || IS_DUAL(ch) ||
                                 (PLR_FLAGGED(ch, PLR_PERCEIVE) && vict == FIGHTING(ch))))
      process_combat_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_combat_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict) && PLR_FLAGGED(ch, PLR_PERCEIVE))
        send_to_char("You can't initiate attacks on astral beings!\r\n", ch);
      else if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_RANGE:
    if (!t) {
      if (!FIGHTING(ch)) {
        send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
        return FALSE;
      } else
        vict = FIGHTING(ch);
    } else {
      any_one_arg(any_one_arg(t, targ1), targ2);
      // if there's not another argument in targ2, we assume it's not
      // an object on a mob
      if (!*targ2) {
        if (!(vict = get_char_room_vis(ch, targ1)))
          if (!(val = spell_damage_door(ch, spell, t))) {
            if (!(obj = get_obj_in_list_vis(ch, targ1, world[ch->in_room].contents)) &&
                !(obj = get_obj_in_list_vis(ch, targ1, ch->carrying))) {
              send_to_char("You can't seem to find the target you are "
                           "looking for.\r\n", ch);
              return FALSE;
            }
          } else
            return (val == 1 ? TRUE : FALSE);
      } else {
        /* if there's a targ2, we know it's either <obj> <victim>, <victim> <direction>
           or <door> <direction> */
        if (!(vict = range_spell(ch, targ1, targ2))) {
          if (!(val = spell_damage_door(ch, spell, t))) {
            if (!(vict = get_char_room_vis(ch, targ2))) {
              send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
              return FALSE;
            } else {
              // then we search for the item equiped by the victim
              for (register int i = 0; i < NUM_WEARS; ++i)
                if (GET_EQ(vict, i) &&
                    isname(targ1, GET_EQ(vict, i)->text.keywords)) {
                  obj = GET_EQ(vict, i);
                  break; // and get out when we do
                }
            }
          } else
            return (val == 1 ? TRUE : FALSE);
        } else if (vict == ch)
          return FALSE;
      }
      // finally, we make sure they live up to the restriction that
      // mana spells may only be cast on living targets.
      if (!spell->physical && obj) {
        send_to_char("You can only cast that spell on living targets.\r\n", ch);
        return FALSE;
      }
      // at the end of all this, we have an object pointed to by *obj if
      // available, and at minimum a victim pointed to by *vict
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_combat_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
               vict == FIGHTING(ch))))
      process_combat_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_combat_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict) && PLR_FLAGGED(ch, PLR_PERCEIVE))
        send_to_char("You can't initiate attacks on astral beings!\r\n", ch);
      else if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_AREA:
    // here we run through a loop and damage lots of folks (=
    // also, you cannot single out objects with area effects spells
    any_one_arg(any_one_arg(t, targ1), targ2);
    if (!t) {
      vict = ch;
    } else {
      any_one_arg(any_one_arg(t, targ1), targ2);
      // if there's not another argument in targ2, we assume it's not
      // an object on a mob
      if (!*targ2) {
        if (!(vict = get_char_room_vis(ch, targ1)))
          if (!(val = spell_damage_door(ch, spell, t))) {
            if (!(obj = get_obj_in_list_vis(ch, targ1, world[ch->in_room].contents)) &&
                !(obj = get_obj_in_list_vis(ch, targ1, ch->carrying))) {
              send_to_char("You can't seem to find the target you are "
                           "looking for.\r\n", ch);
              return FALSE;
            }
          } else
            return (val == 1 ? TRUE : FALSE);
      } else {
        /* if there's a targ2, we know it's either <obj> <victim>, <victim> <direction>
           or <door> <direction> */
        if (!(vict = range_spell(ch, targ1, targ2))) {
          if (!(val = spell_damage_door(ch, spell, t))) {
            if (!(vict = get_char_room_vis(ch, targ2))
                ||!(vict = get_char_room_vis(ch, targ1))) {
              send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
              return FALSE;
            } else {
              // then we search for the item equiped by the victim
              for (register int i = 0; i < NUM_WEARS; ++i)
                if (GET_EQ(vict, i) &&
                    isname(targ1, GET_EQ(vict, i)->text.keywords)) {
                  obj = GET_EQ(vict, i);
                  break; // and get out when we do
                }
            }
          } else
            return (val == 1 ? TRUE : FALSE);
        } else if (vict == ch)
          return FALSE;
      }
      // finally, we make sure they live up to the restriction that
      // mana spells may only be cast on living targets.
      if (!spell->physical && obj) {
        send_to_char("You can only cast that spell on living targets.\r\n", ch);
        return FALSE;
      }
      // at the end of all this, we have an object pointed to by *obj if
      // available, and at minimum a victim pointed to by *vict
    }

    if ( !vict ) {
      send_to_char("Your spell cannot find that target.\r\n", ch);
      return FALSE;
    }

    room_num = vict->in_room;

    struct char_data *tch, *next_tch;
    num_targets = 0;
    for (tch = world[room_num].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      if (tch == ch)
        continue;
      if (!IS_NPC(tch) && IS_SENATOR(tch))
        continue;
      if (in_group(ch, tch))
        continue;
      num_targets++;
      if (!IS_ASTRAL(ch) && !IS_ASTRAL(tch))
        process_combat_spell(ch, tch, (obj_t *) NULL, spell, force);
      else if (IS_ASTRAL(tch) &&
               (IS_ASTRAL(ch) ||
                IS_DUAL(ch) ||
                (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
                 tch == FIGHTING(ch))))
        process_combat_spell(ch, tch, (obj_t *) NULL, spell, force);
      else if (IS_ASTRAL(ch) &&
               (IS_ASTRAL(tch) ||
                IS_DUAL(tch) ||
                PLR_FLAGGED(tch, PLR_PERCEIVE)))
        process_combat_spell(ch, tch, (obj_t *) NULL, spell, force);
      else
        num_targets--;
    }
    struct obj_data *obj, *next;
    if (spell->physical) {
      for (obj = world[room_num].contents; obj; obj = next) {
        next = obj->next_content;
        damage_obj(NULL, obj, force + FORCE_PENALTY(spell), spell->effect);
        num_targets++;
      }
      for (val = 0; val < NUM_OF_DIRS; val++)
        if (EXIT(ch, val) && EXIT(ch, val)->keyword &&
            IS_SET(EXIT(ch, val)->exit_info, EX_CLOSED)) {
          damage_door(NULL, ch->in_room, val, (int)(force), spell->effect);
          num_targets++;
        }
    }
    if ( num_targets == 0 ) {
      send_to_char("Your area affect spell would hit nothing.\r\n", ch );
      return FALSE;
    }
    break;
  default:
    send_to_char("You can't seem to target this spell for some reason.\r\n", ch);
    mudlog("Illegal target in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return FALSE;
  }

  return TRUE;
}

// now for the fun part
void process_combat_spell(char_t *ch, char_t *vict, obj_t *obj, spell_t *spell, int force)
{
  struct obj_data *next;
  int resist = 0, success = 0, dam, i, chance;
  int pbonus, fbonus, tbonus;
  int tnum;
  int rsuccess;

  if (!obj) {
    char xbuf[MAX_STRING_LENGTH];
    // figure out if the spell is physical or mana
    if (spell->physical)
      resist = GET_BOD(vict);
    else
      if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_INANIMATE))
        resist = 100;
      else
        resist = GET_WIL(vict);

    tbonus = totem_bonus(ch, spell);
    pbonus = magic_pool_bonus(ch, spell, force, TRUE);
    fbonus = foci_bonus(ch, spell, force, TRUE);

    xbuf[0] = ' ';
    xbuf[1] = 0;
    tnum = modify_target_rbuf(ch, xbuf);

    // we include totem bonuses and target modifiers to the test
    success = success_test(force + pbonus + fbonus + tbonus, resist + tnum );

    if (access_level(ch, LVL_VICEPRES) && spell->target != SPELL_TARGET_AREA)
      send_to_char(ch, "(%s) F: %d, R: %d, S: %d, D: %d\r\n",
                   spell->physical ? "Physical" : "Mana", force, resist, success, 0);

    if ( 1 ) {
      char rbuf[MAX_STRING_LENGTH];
      sprintf(rbuf,
              "ComSpell: F:%d/%d,%s%s Mag:%d, FFPT/RM %d+%d+%d+%d/%d+%d.  %d.%s",
              force, spell->force,
              spell->target == SPELL_TARGET_AREA ? " (area)" : "",
              spell->effect != SPELL_ELEMENT_NONE ? " (elem)" : "",
              GET_MAG(ch),
              force, fbonus, pbonus, tbonus,
              resist, tnum, success,
              xbuf);
      act(rbuf, 1, ch, NULL, NULL, TO_ROLLS);
    }
    if (success < 1 && !STUNNED(vict)) {
      damage(ch, vict, 0, spell->type, FALSE);
      ranged_response(ch, vict);
      return;
    }

    if (spell->effect != SPELL_ELEMENT_NONE) {
      elemental_damage(ch, vict, spell, force, success);
      return;
    }

    int srvict = spell_resist(vict);
    int mtvict = modify_target(vict);

    rsuccess = success_test(resist + srvict,
                            force + mtvict);

    int basedamage;

    if (IS_ASTRAL(ch))
      basedamage = DRAIN_LEVEL(grimoire[spell->type].drain);
    else
      basedamage = grimoire[spell->type].damage;

    // here we stage the damage up or down
    int stagedamage = stage(success - rsuccess, basedamage);

    dam = convert_damage(stagedamage);

    int is_physicaldam = (((spell->type >= SPELL_STUNBALL)
                           && (spell->type <= SPELL_STUN_TOUCH))
                          ||((spell->type >= SPELL_STUN_SHAFT)
                             && (spell->type <= SPELL_STUN_DART)))
                         ? FALSE : TRUE;

    if (1) {
      char rbuf[MAX_STRING_LENGTH];
      sprintf( rbuf,
               "ComSpell: %c RSr/FM %d+%d/%d+%d. %d-%d=%d, %d->%d.  %d/%d.  D: %d%c.",
               spell->physical ? 'B' : 'W',
               resist, srvict,
               force, mtvict,
               success, rsuccess, success-rsuccess,
               basedamage, stagedamage,
               GET_PHYSICAL(vict), GET_MENTAL(vict),
               dam,
               is_physicaldam ? 'P' : 'M' );

      act( rbuf, 1, ch, NULL, NULL, TO_ROLLS );
    }

    if (spell->physical) {
      if (spell->target == SPELL_TARGET_AREA)
        chance = 10;
      else
        chance = 2;
      for (i = 0; i < (NUM_WEARS - 1); i++)
        if (GET_EQ(vict, i) && number(1, 100) < chance) {
          damage_obj(NULL, GET_EQ(vict, i), force + FORCE_PENALTY(spell),
                     DAMOBJ_PROJECTILE);
          if (spell->target != SPELL_TARGET_AREA)
            break;
        }
      chance = (int)(chance / 2);
      for (obj = vict->carrying; obj; obj = next) {
        next = obj->next_content;
        if (number(1, 100) < chance)
          damage_obj(NULL, obj, force + FORCE_PENALTY(spell),
                     DAMOBJ_PROJECTILE);
      }
    }
    if (((spell->type >= SPELL_STUNBALL) && (spell->type <= SPELL_STUN_TOUCH))
        ||((spell->type >= SPELL_STUN_SHAFT)
           && (spell->type <= SPELL_STUN_DART)))
      damage(ch, vict, dam, spell->type, FALSE);
    else
      damage(ch, vict, dam, spell->type, TRUE);

    if (ch->in_room != vict->in_room)
      ranged_response(ch, vict);
  } else {   // end combat section to living targets
    // this is how magic affects objects
    // first we find the target number for the spell
    damage_obj(ch, obj, force + FORCE_PENALTY(spell), spell->effect);

    // gotta make em attack if they are trying to blow up an object on a mob
    if (vict && IS_NPC(vict) && !FIGHTING(vict))
      set_fighting(vict, ch);

    if (access_level(ch, LVL_VICEPRES) && spell->target != SPELL_TARGET_AREA)
      send_to_char(ch, "(%s) F: %d, R: %d, S: %d, D: %d\r\n",
                   spell->physical ? "Physical" : "Mana", force, resist, success, 0);
  }
}

void elemental_damage(char_t *ch, char_t *vict, spell_t *spell, int force, int success)
{
  // if the spell doesn't do greater than light damage, it won't have
  // secondary effects -- ie, the mage has to take a little risk at least

  if (spell->effect == SPELL_ELEMENT_NONE
      || DRAIN_LEVEL(spell->drain) < MODERATE)
    return;

  int modifier = 0, resist, dam, i, chance, num;

  if (spell->category == SPELL_CATEGORY_COMBAT)
    modifier = 4;
  else
    switch (DRAIN_LEVEL(spell->drain)) {
    case 2:
      modifier = 4;
      break;
    case 3:
      modifier = 2;
      break;
    case 4:
      modifier = 0;
      break;
    default:
      modifier = 4;
    }

  if (spell->effect == SPELL_ELEMENT_ICE)
    resist = GET_IMPACT(vict);
  else
    resist = GET_IMPACT(vict) >> 1;

  success -= success_test(resist, force + modify_target(vict));
  if (IS_ASTRAL(ch))
    dam = convert_damage(stage(success, grimoire[spell->type].drain % 10));
  else
    dam = convert_damage(stage(success, grimoire[spell->type].damage));

  if (spell->category == SPELL_CATEGORY_COMBAT)
    num = 0;
  else
    num = DAMOBJ_MANIPULATION;

  num += spell->effect;

  if (success > 0) {
    struct obj_data *obj, *next;
    if (spell->physical) {
      if (spell->target == SPELL_TARGET_AREA)
        chance = 10;
      else
        chance = 2;
      for (i = 0; i < (NUM_WEARS - 1); i++)
        if (GET_EQ(vict, i) && number(1, 100) < chance) {
          damage_obj(NULL, GET_EQ(vict, i),
                     force + FORCE_PENALTY(spell), num);
          if (spell->target != SPELL_TARGET_AREA)
            break;
        }
      chance = (int)(chance / 2);
      for (obj = vict->carrying; obj; obj = next) {
        next = obj->next_content;
        if (number(1, 100) < chance)
          damage_obj(NULL, obj, force + FORCE_PENALTY(spell), num);
      }
    }
    damage(ch, vict, dam, spell->type, TRUE);

    ranged_response(ch, vict);
  }
}

// finds the target for the detection spells
bool process_detection_target(char_t *ch, spell_t *spell, char *t, int force)
{
  char_t *vict = NULL;
  obj_t *obj = NULL;
  char targ1[80], targ2[80];

  if (t)
    skip_spaces(&t);
  // first we find the appropriate target
  switch (spell->target) {
  case SPELL_TARGET_CASTER:
    if (t && !isname(t, GET_KEYWORDS(ch))) {
      send_to_char("You can only target yourself with this spell.\r\n", ch);
      return FALSE;
    }
    process_combat_spell(ch, ch, (obj_t *)NULL, spell, force);
    break;
  case SPELL_TARGET_TOUCH:
    if (FIGHTING(ch)) {
      send_to_char("You can't cast this spell during combat.\r\n", ch);
      return FALSE;
    }
    if (!t) {
      send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
      return FALSE;
    } else {
      // for detection spells, we search the room for a character first,
      // then for an object in inventory, then an object in the room
      if (!(vict = get_char_room_vis(ch, t)))
        if (!(obj = get_obj_in_list_vis(ch, t, ch->carrying)))
          if (!(obj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents))) {
            send_to_char("You can't seem to find the target you wish to cast this spell on.\r\n", ch);
            return FALSE;
          }
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_detection_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              PLR_FLAGGED(ch, PLR_PERCEIVE)))
      process_detection_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_detection_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_RANGE:
    if (!t) {
      send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
      return FALSE;
    }
    any_one_arg(any_one_arg(t, targ1), targ2);
    if (!*targ2) {
      if (!(vict = get_char_room_vis(ch, targ1))) {
        send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
        return FALSE;
      }
    } else {
      if (!(vict = range_spell(ch, targ1, targ2))) {
        if (!(vict = get_char_room_vis(ch, targ2))
            ||!(vict = get_char_room_vis(ch, targ1))) {
          send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
          return FALSE;
        } else {
          for (register int i = 0; i < NUM_WEARS; ++i)
            if (GET_EQ(vict, i) &&
                isname(targ1, GET_EQ(vict, i)->text.keywords)) {
              obj = GET_EQ(vict, i);
              break;
            }
        }
      } else if (vict == ch)
        return FALSE;
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_detection_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              PLR_FLAGGED(ch, PLR_PERCEIVE)))
      process_detection_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_detection_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_AREA:
    if (!t) {
      send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
      return FALSE;
    }

    any_one_arg(any_one_arg(t, targ1), targ2);
    // if there's not another argument in targ2, we assume it's not
    // an object on a mob
    if (!*targ2) {
      if (!(vict = get_char_room_vis(ch, targ1)) &&
          !(obj = get_obj_in_list_vis(ch, targ1, ch->carrying)) &&
          !(obj = get_obj_in_list_vis(ch, targ1, world[ch->in_room].contents))) {
        send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
        return FALSE;
      }
    } else {  // if there is a targ2, we know it is in the form 'spell' <obj> <mob>
      // here we grab the pointer to the victim if it is there
      if (!(vict = get_char_room_vis(ch, targ2))) {
        send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
        return FALSE;
      } else { // then we search for the item equiped by the victim
        for (register int i = 0; i < NUM_WEARS; ++i)
          if (GET_EQ(vict, i) &&
              isname(targ1, GET_EQ(vict, i)->text.keywords)) {
            obj = GET_EQ(vict, i);
            break; // and get out when we do
          }
      }
    }

    if (!spell->physical && obj) {
      send_to_char("You can only cast that spell on living targets.\r\n", ch);
      return FALSE;
    }

    // there are not really any area detection spells, but if I ever come
    // up with some, I will need to set up a loop here to proccess it
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_detection_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              PLR_FLAGGED(ch, PLR_PERCEIVE)))
      process_detection_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_detection_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  default:
    send_to_char("You can't seem to target this spell for some reason.\r\n", ch);
    mudlog("Illegal target in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return FALSE;
  }

  return TRUE;
}

// and finally for the fun part of the detection effects
void process_detection_spell(char_t *ch, char_t * vict, obj_t *obj, spell_t *spell, int force)
{
  static int success;
  static int resist;
  static bool found;
  static struct affected_type af, af2;
  const char *owner_name;

  switch (spell->type) {
  case SPELL_ANALYZE_DEVICE:
    if (!obj) {
      send_to_char("You realize as you cast the spell that it only works on inanimate objects.\r\n", ch);
      return;
    }
    // resistance is based off the material of the object
    resist = material_ratings[(int)GET_OBJ_MATERIAL(obj)];
    success = success_test(force + spell_bonus(ch, spell), resist + modify_target(ch));
    // if there were no successes, we just return
    if (success == 0) {
      send_to_char("You seem unable to learn anything new about it.\r\n", ch);
      return;
    }
    if (success >= 1) {
      send_to_char(ch, "%s is %s %s weighing %d.\r\n",
                   CAP(obj->text.name),
                   AN(item_types[(int)obj->obj_flags.type_flag]),
                   item_types[(int)obj->obj_flags.type_flag],
                   obj->obj_flags.weight);
      if (GET_OBJ_TYPE(obj) != ITEM_FOOD)
        send_to_char(ch,
                     "It appears to be made of %s.\r\n",
                     material_names[(int)GET_OBJ_MATERIAL(obj)]);
    }
    if (success >= 2) {
      obj->obj_flags.wear_flags.PrintBits(buf1, MAX_STRING_LENGTH,
                                          wear_bits, ITEM_WEAR_MAX);
      send_to_char(ch,
                   "It is useable in the following positions: %s\r\n",
                   buf1);
      send_to_char(ch,
                   "It has a durability rating of %d.\r\n",
                   GET_OBJ_BARRIER(obj));
    }
    if (success >= 3) {
      if (obj->obj_flags.bitvector.GetNumSet() > 0) {
        obj->obj_flags.bitvector.PrintBits(buf1, MAX_STRING_LENGTH,
                                           affected_bits, AFF_MAX);
        send_to_char(ch, "It grants following abilities: %s\r\n", buf1);
      }
      send_to_char("It has the following affections:", ch);
      for (register int i = 0; i < MAX_OBJ_AFFECT; i++)
        if (obj->affected[i].modifier) {
          strcpy(buf2, apply_types[(int)obj->affected[i].location]);
          send_to_char(ch, "%s %+d to %s", found++ ? "," : "",
                       obj->affected[i].modifier, buf2);
        }
      if (!found)
        send_to_char(" None\r\n", ch);
      else
        send_to_char("\r\n", ch);
    }
    if (success >= 4) {
      switch (GET_OBJ_TYPE(obj)) {
      case ITEM_LIGHT:
        if (GET_OBJ_VAL(obj, 2) < 0)
          send_to_char("Light is permanent.\r\n", ch);
        else
          send_to_char(ch, "Hours left for light: %d\r\n",
                       GET_OBJ_VAL(obj, 2));
        break;
      case ITEM_WEAPON:
        send_to_char(ch, "Power: %s, Damage: %s, Skill needed: %s\r\n",
                     get_power(GET_OBJ_VAL(obj, 0)),
                     wound_name[GET_OBJ_VAL(obj, 1)],
                     spells[GET_OBJ_VAL(obj, 4)]);
        break;
      case ITEM_FIREWEAPON:
        send_to_char(ch,
                     "Str. Minimum: %d, Str+: %d, Power: %d, Skill needed: %s\r\n",
                     GET_OBJ_VAL(obj, 6),
                     GET_OBJ_VAL(obj, 2),
                     GET_OBJ_VAL(obj, 0),
                     spells[GET_OBJ_VAL(obj, 4)]);
        break;
      case ITEM_WORN:
        send_to_char(ch, "Ballistic rating: %d, Impact Rating: %d\r\n",
                     GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
        break;
      case ITEM_CONTAINER:
        send_to_char(ch, "Max weight containable: %d kilograms\r\n", GET_OBJ_VAL(obj, 0));
        break;
      case ITEM_DRINKCON:
        send_to_char(ch, "Max contains: %d, contains: %d, poisoned: %s, "
                     "liquid: %s\r\n",
                     GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
                     GET_OBJ_VAL(obj, 3) ? "Yes" : "No",
                     drinks[GET_OBJ_VAL(obj, 2)]);
        break;
      case ITEM_FOOD:
        send_to_char(ch, "Filling for %d hours, poisoned: %s\r\n",
                     GET_OBJ_VAL(obj, 0),
                     GET_OBJ_VAL(obj, 3) ? "Yes" : "No");
        break;
      case ITEM_WORKING_GEAR:
        send_to_char(ch, "Type: %s\r\n", gear_name[GET_OBJ_VAL(obj, 0)]);
        break;
      case ITEM_QUIVER:
        send_to_char(ch, "Max contains: %d, Type contains: %s\r\n",
                     GET_OBJ_VAL(obj, 0),
                     (GET_OBJ_VAL(obj, 1) == 0 ? "Arrow"
                      : (GET_OBJ_VAL(obj, 1) == 1 ? "Bolt"
                         : (GET_OBJ_VAL(obj, 1) == 2 ? "Shuriken"
                            : (GET_OBJ_VAL(obj, 1) == 3 ? "Throwing knife"
                               : "Undefined")))));
        break;
      case ITEM_PATCH:
        send_to_char(ch, "Type: %s, Rating: %d\r\n",
                     patch_names[GET_OBJ_VAL(obj, 0)],
                     GET_OBJ_VAL(obj, 1));
        break;
      case ITEM_CYBERDECK:
        send_to_char(ch, "MPCP: %d, Hardening: %d, Active: %d, "
                     "Storage: %d, Load: %d\r\n",
                     GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
                     GET_OBJ_VAL(obj, 2),
                     GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4));
        break;
      case ITEM_PROGRAM:
        if (GET_PROG_TYPE(obj) == SOFT_ATTACK)
          sprintf(buf2, " DamType: %s, ",
                  attack_types[GET_OBJ_VAL(obj, 3) - TYPE_HIT]);
        else
          sprintf(buf2, " ");
        send_to_char(ch, "Type: %s, Rating: %d, Size: %d,"
                     "%sInstalled?: %s\r\n",
                     program_types[GET_OBJ_VAL(obj, 0)],
                     GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), buf2,
                     (GET_OBJ_VAL(obj, 6) ? "yes" : "no"));
        break;
      case ITEM_FOCUS:
      case ITEM_SPELL_FORMULA:
        send_to_char(ch, "%s cannot ascertain detailed information "
                     "about magical objects.\r\n",
                     spell->name);
        break;
      }
    }
    break;
  case SPELL_CLAIRVOYANCE:
    if (obj) {
      send_to_char("View what an item sees?\r\n", ch);
      return;
    }
    GET_WAS_IN(ch) = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, vict->in_room);
    look_at_room(ch, 1);
    char_from_room(ch);
    char_to_room(ch, GET_WAS_IN(ch));
    GET_WAS_IN(ch) = NOWHERE;
    break;
  case SPELL_COMBAT_SENSE:
    if (obj) {
      send_to_char("That spell can only be cast on living targets.\r\n", ch);
      return;
    } else
      sustain_spell(force, ch, vict, spell, 0);
    break;
  case SPELL_DETECT_ALIGNMENT:
    if (obj) {
      send_to_char("Items are most definately neutral.\r\n", ch);
      return;
    } else
      sustain_spell(force, ch, vict, spell, 0);
    break;
  case SPELL_DETECT_INVIS:
    if (obj) {
      send_to_char("That just doesn't make sense.\r\n", ch);
      return;
    } else
      sustain_spell(force, ch, vict, spell, 0);
    break;
  case SPELL_DETECT_MAGIC:
    if (obj) {
      send_to_char("That just doesn't make sense.\r\n", ch);
      return;
    } else
      sustain_spell(force, ch, vict, spell, 0);
    break;
  case SPELL_ANALYZE_MAGIC:
    if (obj) {
      switch (GET_OBJ_TYPE(obj)) {
      case ITEM_SPELL_FORMULA:
        send_to_char(ch,
                     "(%s) Category: %s, Force: %d, Target: %s, "
                     "Drain %d%s\r\n"
                     "Damage: %s, Type: %s, Effect: %s, For: %s",
                     (GET_OBJ_VAL(obj, 0) ? "Physical" : "Mana"),
                     spell_categories[GET_OBJ_VAL(obj, 1)],
                     GET_OBJ_VAL(obj, 2),
                     spell_target[GET_OBJ_VAL(obj, 3)],
                     (GET_OBJ_VAL(obj, 2)/2) + (GET_OBJ_VAL(obj, 4)/10),
                     wound_arr[GET_OBJ_VAL(obj, 4) % 10],
                     wound_arr[GET_OBJ_VAL(obj, 5)],
                     spells[GET_OBJ_VAL(obj, 6)],
                     elements[GET_OBJ_VAL(obj, 8)],
                     (GET_OBJ_VAL(obj, 7) ? "Shaman" : "Mage"));
        break;

      case ITEM_FOCUS:
        owner_name = playerDB.GetNameV(GET_OBJ_VAL(obj, 9));

        if (GET_OBJ_VAL(obj, VALUE_FOCUS_TYPE) == FOCI_LOCK)
          send_to_char(ch, "Spell lock, Spell: %s, Bonded by: %s",
                       (!GET_OBJ_VAL(obj, 8)
                        ? "None"
                        : spells[GET_OBJ_VAL(obj, 8)]),
                       owner_name? owner_name : "undefined");
        else
          sprintf(buf, "%s, Rating: %d, Spec.: %d, Bonded by: %s",
                  foci_types[GET_OBJ_VAL(obj, 0)],
                  GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 8),
                  owner_name? owner_name : "undefined");
        break;

      case ITEM_POTION:
        send_to_char(ch, "Force: %d, Spell: %s",
                     GET_OBJ_VAL(obj, 0), spells[GET_OBJ_VAL(obj, 3)]);
        break;
      }
    } else
      send_to_char("You need to target an object with this spell.\r\n", ch);
    break;

  case SPELL_ANALYZE_PERSON:
    if ( !vict ) {
      send_to_char("Must be cast on a person.\n\r",ch);
      return;
    }

    if (spell->physical)
      resist = GET_BOD(vict);
    else
      resist = GET_WIL(vict);

    success = resisted_test(force + spell_bonus(ch, spell),
                            resist + modify_target(ch),
                            resist + spell_resist(vict),
                            force + modify_target(vict));

    if (success <= 20) {
      send_to_char("You don't learn anything new.\r\n", ch);
      return;
    }

    if (success >= 2) {
      switch (GET_SEX(vict)) {
      case SEX_MALE:
        strcpy(buf1, "He");
        break;
      case SEX_FEMALE:
        strcpy(buf1, "She");
        break;
      default:
        strcpy(buf1, "It");
        break;
      }

      send_to_char(ch,
                   "%s has %s strength, %s quickness, and %s body.\r\n",
                   buf1,
                   get_attrib(GET_STR(vict)),
                   get_attrib(GET_QUI(vict)),
                   get_attrib(GET_BOD(vict)));
    }

    if (success >= 3) {
      send_to_char(ch,
                   "%s has %s charisma, %s intelligence, %s willpower.\r\n",
                   buf1,
                   get_attrib(GET_CHA(vict)),
                   get_attrib(GET_INT(vict)),
                   get_attrib(GET_WIL(vict)));
    }

    if (success >= 5) {
      send_to_char(ch, "%s has %s reaction, %s magic.\r\n",
                   buf1,
                   get_attrib(GET_REA(vict)),
                   get_attrib(GET_MAG(vict) / 100));
    }

    if (success >= 7) {
      send_to_char(ch, "%s knows:\r\n", buf1);
      for (int i = 100; i < MAX_SKILLS; i++)
        if (GET_SKILL(vict, i) > 0)
          send_to_char(ch, "%s\r\n", spells[i]);
    }

    break;

  default:
    send_to_char("Your spell fizzles into oblivion.\r\n", ch);
    mudlog("Illegal type in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return;
  }
}

int process_health_target(char_t *ch, spell_t *spell, char *t, int force)
{
  char_t *vict;
  char targ1[80], targ2[80], targ3[80];
  int mode = 1;

  if (t)
    skip_spaces(&t);

  // first we find the appropriate target
  switch (spell->target) {
  case SPELL_TARGET_CASTER:
    if (spell->type == SPELL_HEAL || spell->type == SPELL_ANTIDOTE || spell->type == SPELL_RESIST_PAIN ||
        spell->type == SPELL_CURE_DISEASE || spell->type == SPELL_POISON) {
      any_one_arg(t, targ1);
      if (!targ1) {
        send_to_char("At what level would you like to cast this spell?\r\n", ch);
        return FALSE;
      }
      if (is_abbrev(targ1, "light"))
        mode = LIGHT;
      else if (is_abbrev(targ1, "moderate"))
        mode = MODERATE;
      else if (is_abbrev(targ1, "serious"))
        mode = SERIOUS;
      else if (is_abbrev(targ1, "deadly"))
        mode = DEADLY;
      else {
        send_to_char("At what level would you like to cast this spell?\r\n", ch);
        return FALSE;
      }
    }
    if (t && !isname(t, GET_KEYWORDS(ch))) {
      send_to_char("You can only target yourself with this spell.\r\n", ch);
      return FALSE;
    }
    if (mode == DEADLY && spell->type == SPELL_RESIST_PAIN) {
      send_to_char("You can't resist the effects of deadly wounds.\r\n", ch);
      return FALSE;
    }
    process_health_spell(ch, ch, mode, spell, force);
    break;
  case SPELL_TARGET_TOUCH:
    if (spell->type == SPELL_HEAL || spell->type == SPELL_ANTIDOTE ||
        spell->type == SPELL_RESIST_PAIN ||
        spell->type == SPELL_CURE_DISEASE || spell->type == SPELL_POISON) {
      any_one_arg(any_one_arg(t, targ1), targ2);
      if (!targ1 || !targ2) {
        send_to_char("Who do you wish to cast this spell on?\r\n", ch);
        return FALSE;
      }
      if (is_abbrev(targ1, "light"))
        mode = LIGHT;
      else if (is_abbrev(targ1, "moderate"))
        mode = MODERATE;
      else if (is_abbrev(targ1, "serious"))
        mode = SERIOUS;
      else if (is_abbrev(targ1, "deadly"))
        mode = DEADLY;
      else {
        send_to_char("At what level would you like to cast this spell?\r\n", ch);
        return FALSE;
      }
    } else
      any_one_arg(t, targ2);
    if (!(vict = get_char_room_vis(ch, targ2))) {
      send_to_char("You can't seem to find the target you wish to cast "
                   "this spell on.\r\n", ch);
      return FALSE;
    }
    if (mode == DEADLY && spell->type == SPELL_RESIST_PAIN) {
      send_to_char("You can't resist the effects of deadly wounds.\r\n", ch);
      return FALSE;
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_health_spell(ch, vict, mode, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              PLR_FLAGGED(ch, PLR_PERCEIVE)))
      process_health_spell(ch, vict, mode, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_health_spell(ch, vict, mode, spell, force);
    else {
      if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_RANGE:
    if (spell->type == SPELL_HEAL || spell->type == SPELL_ANTIDOTE ||
        spell->type == SPELL_RESIST_PAIN ||
        spell->type == SPELL_CURE_DISEASE || spell->type == SPELL_POISON) {
      any_one_arg(any_one_arg(any_one_arg(t, targ1), targ2), targ3);
      if (!targ1 || !targ2) {
        send_to_char("Who do you wish to cast this spell on?\r\n", ch);
        return FALSE;
      }
      if (is_abbrev(targ1, "light"))
        mode = LIGHT;
      else if (is_abbrev(targ1, "moderate"))
        mode = MODERATE;
      else if (is_abbrev(targ1, "serious"))
        mode = SERIOUS;
      else if (is_abbrev(targ1, "deadly"))
        mode = DEADLY;
      else {
        send_to_char("At what level would you like to cast this spell?\r\n", ch);
        return FALSE;
      }
    } else
      any_one_arg(any_one_arg(t, targ2), targ3);
    if (!targ1 || !targ2) {
      send_to_char("Who do you wish to cast this spell on?\r\n", ch);
      return FALSE;
    }
    if (!*targ3) {
      if (!(vict = get_char_room_vis(ch, targ2))) {
        send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
        return FALSE;
      }
    } else {
      if (!(vict = range_spell(ch, targ2, targ3))) {
        if (!(vict = get_char_room_vis(ch, targ2))) {
          send_to_char("You can't seem to find the target you are looking "
                       "for.\r\n", ch);
          return FALSE;
        }
      } else if (vict == ch)
        return FALSE;
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_health_spell(ch, vict, mode, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              PLR_FLAGGED(ch, PLR_PERCEIVE)))
      process_health_spell(ch, vict, mode, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_health_spell(ch, vict, mode, spell, force);
    else {
      if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_AREA:
    send_to_char("That is not currently a valid spell type.", ch);
    mudlog("Undefined (area or ranged) health spell", ch, LOG_SYSLOG, TRUE);
    return(FALSE);
  default:
    send_to_char("You can't seem to target this spell for some reason.\r\n", ch);
    mudlog("Illegal target in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return FALSE;
  }
  return mode;
}

void process_health_spell(char_t *ch, char_t *vict, int level, spell_t *spell, int force)
{
  int success, time = (int)(level * 5/4);
  int mod = 0, min = 0;
  struct affected_type *af;

  switch (spell->type) {
  case SPELL_ANTIDOTE:
    if (affected_by_spell(vict, SPELL_POISON) != 1) {
      sprintf(buf, "%s not affected by any toxin.", (ch == vict ? "You are" : "$N is"));
      act(buf, FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    for (af = vict->affected; af; af = af->next)
      if (af->type == SPELL_POISON) {
        if (af->duration > level) {
          send_to_char(NOEFFECT, ch);
          sustain_spell(force, ch, vict, spell, time);
          return;
        }
        success = success_test(force + spell_bonus(ch, spell), af->modifier + modify_target(ch));
        if (!success) {
          send_to_char("You fail to dispel the toxin.\r\n", ch);
          sustain_spell(force, ch, vict, spell, time);
          return;
        }
        time = (int)(time / success);
        send_to_char("Your flows of magic erradicate the toxin.\r\n", ch);
        send_to_char(vict, "%s\r\n", spell_wear_off_msg[af->type]);
        affect_remove(vict, af, 1);
        sustain_spell(force, ch, vict, spell, time);
        return;
      }
    break;
  case SPELL_CURE_DISEASE:
    send_to_char("This spell is useless at the moment.\r\n", ch);
    break;
  case SPELL_HEAL:
    if (level == LIGHT) {
      mod = 1;
      min = (int)(GET_MAX_PHYSICAL(vict) * 4/500);
    } else if (level == MODERATE) {
      mod = 3;
      min = (int)(GET_MAX_PHYSICAL(vict) * 3/500);
    } else if (level == SERIOUS) {
      mod = 6;
      min = (int)(GET_MAX_PHYSICAL(vict) / 1000);
    } else if (level == DEADLY) {
      mod = 10;
      min = -(int)(GET_MAX_PHYSICAL(vict) / 100);
    }
    success = success_test(force + spell_bonus(ch, spell), (9 - (GET_ESS(vict) / 100)) + modify_target(ch));
    if (success < 1) {
      if (ch == vict)
        sprintf(buf, "You can't get your healing magic to bind with your life force!\r\n");
      else
        sprintf(buf, "You can't get your healing magic to bind with %s's life force!\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
      sustain_spell(force, ch, vict, spell, time);
      return;
    }
    time = (int)(time / success);
    sustain_spell(force, ch, vict, spell, time);
    if ((int)(GET_PHYSICAL(vict) / 100) < min) {
      sprintf(buf, "Your healing magic isn't powerful enough to help %s.\r\n",
              (ch != vict ? GET_NAME(vict) : "yourself"));
      send_to_char(buf, ch);
      return;
    }
    LAST_HEAL(vict) = -1;
    send_to_char("A warm feeling floods your body.\r\n", vict);
    act("$n appears better.", TRUE, vict, 0, 0, TO_ROOM);
    GET_PHYSICAL(vict) = MIN(GET_MAX_PHYSICAL(vict), GET_PHYSICAL(vict) + (mod * 100));
    update_pos(vict);
    break;
  case SPELL_DECREASE_ATTRIB:
  case SPELL_INCREASE_ATTRIB:
  case SPELL_INCREASE_REFLEXES:
  case SPELL_POISON:
  case SPELL_RESIST_PAIN:
    sustain_spell(force, ch, vict, spell, level);
    break;
  case SPELL_STABILIZE:
    time = 5;
    if (GET_POS(vict) > POS_MORTALLYW) {
      if (ch == vict)
        sprintf(buf, "You aren't in any danger of dying!");
      else
        sprintf(buf, "$N isn't in any danger of dying!");
      act(buf, FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
    success = success_test(force + spell_bonus(ch, spell), 4 + modify_target(ch) -
                           (int)(GET_PHYSICAL(vict) / 100));
    if (success) {
      if (ch == vict)
        log("ERROR: In newmagic.cc, stabilizing self!");
      act("You succeed in stabilizing $N.", FALSE, ch, 0, vict, TO_CHAR);
      act("Your wounds stop bleeding.", FALSE, vict, 0, 0, TO_CHAR);
      AFF_FLAGS(vict).SetBit(AFF_STABILIZE);
      time = (int)(time / success);
    } else
      send_to_char(NOEFFECT, ch);
    break;
  default:
    send_to_char("Your spell fizzles into oblivion.\r\n", ch);
    mudlog("Illegal type in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return;
  }
  return;
}

bool process_illusion_target(char_t *ch, spell_t *spell, char *t, int force)
{
  char_t *vict = NULL;
  obj_t *obj = NULL;
  char targ1[80], targ2[80];

  if (t)
    skip_spaces(&t);

  switch (spell->target) {
  case SPELL_TARGET_CASTER:
    if (t && !isname(t, GET_KEYWORDS(ch))) {
      send_to_char("You can only target yourself with this spell.\r\n",
                   ch);
      return FALSE;
    }
    process_illusion_spell(ch, ch, (obj_t *)NULL, spell, force);
    break;
  case SPELL_TARGET_TOUCH:
    if (!t) {
      if (!FIGHTING(ch)) {
        send_to_char("You must specify the target you wish to cast this "
                     "spell on.\r\n", ch);
        return FALSE;
      } else
        vict = FIGHTING(ch);
    } else {
      if (!(vict = get_char_room_vis(ch, t)))
        if (!(obj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents)))
          if (!(obj = get_obj_in_list_vis(ch, t, ch->carrying))) {
            send_to_char("You can't seem to find the target you wish to cast this spell on.\r\n", ch);
            return FALSE;
          }
    }
    if (vict && GET_POS(vict) == POS_STANDING && spell->type != SPELL_INVISIBILITY && spell->type != SPELL_IMPROVED_INVIS) {
      if (resisted_test(GET_QUI(ch), GET_QUI(vict) + modify_target(ch), GET_QUI(vict), GET_QUI(ch) + modify_target(vict)) < 1) {
        act("You stumble over your own legs as you try to touch $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!", FALSE, ch, 0, vict, TO_VICT);
        if (!FIGHTING(vict))
          set_fighting(vict, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, vict);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return FALSE;
      }
    } else if (vict && spell->type != SPELL_INVISIBILITY && spell->type != SPELL_IMPROVED_INVIS) {
      if (resisted_test(GET_QUI(ch), (GET_QUI(vict) / 2) + modify_target(ch),
                        (GET_QUI(vict) / 2), GET_QUI(ch) + modify_target(vict)) < 1) {
        act("You stumble over your own legs as you try to touch $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!", FALSE, ch, 0, vict, TO_VICT);
        if (!FIGHTING(vict))
          set_fighting(vict, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, vict);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return FALSE;
      }
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_illusion_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
               vict == FIGHTING(ch))))
      process_illusion_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_illusion_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict) &&
          PLR_FLAGGED(ch, PLR_PERCEIVE))
        send_to_char("You can't initiate attacks on astral beings!\r\n", ch);
      else if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_RANGE:
    if (!t) {
      if (!FIGHTING(ch)) {
        send_to_char("You must specify the target you wish to cast this "
                     "spell on.\r\n", ch);
        return FALSE;
      } else
        vict = FIGHTING(ch);
    } else {
      any_one_arg(any_one_arg(t, targ1), targ2);
      if (!*targ2) {
        if (!(vict = get_char_room_vis(ch, targ1)) &&
            !(obj = get_obj_in_list_vis(ch, targ1, world[ch->in_room].contents)) &&
            !(obj = get_obj_in_list_vis(ch, targ1, ch->carrying))) {
          send_to_char("You can't seem to find the target you are looking "
                       "for.\r\n", ch);
          return FALSE;
        }
      } else {
        if (!(vict = range_spell(ch, targ1, targ2))) {
          if (!(vict = get_char_room_vis(ch, targ2))) {
            send_to_char("You can't seem to find the target you are looking "
                         "for.\r\n", ch);
            return FALSE;
          } else {
            for (register int i = 0; i < NUM_WEARS; ++i)
              if (GET_EQ(vict, i) &&
                  isname(targ1, GET_EQ(vict, i)->text.keywords)) {
                obj = GET_EQ(vict, i);
                break; // and get out when we do
              }
          }
        } else if (vict == ch)
          return FALSE;
      }
      if (!spell->physical && obj) {
        send_to_char("You can only cast that spell on living targets.\r\n", ch);
        return FALSE;
      }
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_illusion_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
               vict == FIGHTING(ch))))
      process_illusion_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_illusion_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict) && PLR_FLAGGED(ch, PLR_PERCEIVE))
        send_to_char("You can't initiate attacks on astral beings!\r\n", ch);
      else if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_AREA:
    struct char_data *tch, *next_tch;
    for (tch = world[ch->in_room].people; tch; tch = next_tch)  {
      next_tch = tch->next_in_room;
      if (tch == ch)
        continue;
      if (!IS_NPC(tch) && IS_SENATOR(tch))
        continue;
      if (in_group(ch, tch))
        continue;
      if (!IS_ASTRAL(ch) && !IS_ASTRAL(tch))
        process_illusion_spell(ch, tch, (obj_t *) NULL, spell, force);
      else if (IS_ASTRAL(tch) &&
               (IS_ASTRAL(ch) ||
                IS_DUAL(ch) ||
                (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
                 tch == FIGHTING(ch))))
        process_illusion_spell(ch, tch, (obj_t *) NULL, spell, force);
      else if (IS_ASTRAL(ch) &&
               (IS_ASTRAL(tch) ||
                IS_DUAL(tch) ||
                PLR_FLAGGED(tch, PLR_PERCEIVE)))
        process_illusion_spell(ch, tch, (obj_t *) NULL, spell, force);
    }
    break;
  default:
    send_to_char("You can't seem to target this spell for some reason.\r\n", ch);
    mudlog("Illegal target in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return FALSE;
  }

  return TRUE;
}

void process_illusion_spell(char_t *ch, char_t *vict, obj_t *obj, spell_t *spell, int force)
{
  switch (spell->type) {
  case SPELL_CHAOS:
  case SPELL_CHAOTIC_WORLD:
  case SPELL_CONFUSION:
  case SPELL_INVISIBILITY:
  case SPELL_OVERSTIMULATION:
  case SPELL_IMPROVED_INVIS:
    sustain_spell(force, ch, vict, spell, 0);
    break;
  default:
    send_to_char("Your spell fizzles into oblivion.\r\n", ch);
    mudlog("Illegal type in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return;
  }
  return;
}

bool process_manipulation_target(char_t *ch, spell_t *spell, char *t, int force)
{
  char_t *vict = NULL;
  obj_t *obj = NULL;
  char targ1[80], targ2[80];
  int i, vnum = -1, val;

  if (t)
    skip_spaces(&t);

  if (spell->type == SPELL_SHAPE_CHANGE) {
    if (ch->desc->original) {
      send_to_char("You can't shape change in your current form!\r\n", ch);
      return FALSE;
    }
    if (!t) {
      send_to_char("What do you want to transform into?\r\n", ch);
      return FALSE;
    }
    for (i = 0; *shape_forms[i] != '\n'; i++)
      if (!str_cmp(t, (char *)shape_forms[i])) {
        vnum = 50 + i;
        break;
      }
    if (vnum == -1) {
      send_to_char("What do you want to transform into?\r\n", ch);
      return FALSE;
    }
    vict = read_mobile(vnum, VIRTUAL);
    act("You feel your body morph into its new shape.", FALSE, ch, 0, 0, TO_CHAR);
    sprintf(arg, "$n disappears, only to be replaced by %s!", GET_NAME(vict));
    act(arg, TRUE, ch, 0, 0, TO_ROOM);
    PLR_FLAGS(ch).SetBit(PLR_SWITCHED);
    char_to_room(vict, ch->in_room);
    GET_WAS_IN(ch) = ch->in_room;
    char_from_room(ch);


    char_to_room(ch, 0);
    ch->desc->character = vict;
    ch->desc->original = ch;
    vict->desc = ch->desc;
    ch->desc = NULL;

    look_at_room(vict, 1);
    return TRUE;
  }

  switch (spell->target) {
  case SPELL_TARGET_CASTER:
    if (t && !isname(t, GET_KEYWORDS(ch))) {
      send_to_char("You can only target yourself with this spell.\r\n", ch);
      return FALSE;
    }
    process_manipulation_spell(ch, ch, (obj_t *)NULL, spell, force);
    break;
  case SPELL_TARGET_TOUCH:
    if (!t) {
      if (!FIGHTING(ch)) {
        send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
        return FALSE;
      } else
        vict = FIGHTING(ch);
    } else {
      if (!(vict = get_char_room_vis(ch, t)))
        if (!(val = spell_damage_door(ch, spell, t))) {
          if (!(obj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents)))
            if (!(obj = get_obj_in_list_vis(ch, t, ch->carrying))) {
              send_to_char("You can't seem to find the target you "
                           "wish to cast this spell on.\r\n", ch);
              return FALSE;
            }
        } else
          return (val == 1 ? TRUE : FALSE);
    }
    if (vict && GET_POS(vict) == POS_STANDING) {
      if (resisted_test(GET_QUI(ch), GET_QUI(vict) + modify_target(ch), GET_QUI(vict), GET_QUI(ch) + modify_target(vict)) < 1) {
        act("You stumble over your own legs as you try to touch $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!", FALSE, ch, 0, vict, TO_VICT);
        if (!FIGHTING(vict))
          set_fighting(vict, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, vict);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return FALSE;
      }
    } else if (vict) {
      if (resisted_test(GET_QUI(ch), (GET_QUI(vict) / 2) + modify_target(ch),
                        (GET_QUI(vict) / 2), GET_QUI(ch) + modify_target(vict)) < 1) {
        act("You stumble over your own legs as you try to touch $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!", FALSE, ch, 0, vict, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!", FALSE, ch, 0, vict, TO_VICT);
        if (!FIGHTING(vict))
          set_fighting(vict, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, vict);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return FALSE;
      }
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_manipulation_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
               vict == FIGHTING(ch))))
      process_manipulation_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_manipulation_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict) && PLR_FLAGGED(ch, PLR_PERCEIVE))
        send_to_char("You can't initiate attacks on astral beings!\r\n", ch);
      else if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_RANGE:
    if (!t) {
      if (!FIGHTING(ch)) {
        send_to_char("You must specify the target you wish to cast this spell on.\r\n", ch);
        return FALSE;
      } else
        vict = FIGHTING(ch);
    } else {
      any_one_arg(any_one_arg(t, targ1), targ2);
      if (!*targ2) {
        if (!(vict = get_char_room_vis(ch, targ1)))
          if (!(val = spell_damage_door(ch, spell, t))) {
            if (!(obj = get_obj_in_list_vis(ch, targ1, world[ch->in_room].contents)) &&
                !(obj = get_obj_in_list_vis(ch, targ1, ch->carrying))) {
              send_to_char("You can't seem to find the target you "
                           "are looking for.\r\n", ch);
              return FALSE;
            }
          } else
            return (val == 1 ? TRUE : FALSE);
      } else {
        if (!(vict = range_spell(ch, targ1, targ2))) {
          if (!(val = spell_damage_door(ch, spell, t))) {
            if (!(vict = get_char_room_vis(ch, targ2))) {
              send_to_char("You can't seem to find the target you are looking for.\r\n", ch);
              return FALSE;
            } else {
              for (i = 0; i < NUM_WEARS; ++i)
                if (GET_EQ(vict, i) &&
                    isname(targ1, GET_EQ(vict, i)->text.keywords)) {
                  obj = GET_EQ(vict, i);
                  break; // and get out when we do
                }
            }
          } else
            return (val == 1 ? TRUE : FALSE);
        } else if (vict == ch)
          return FALSE;
      }
      if (!spell->physical && obj) {
        send_to_char("You can only cast that spell on living targets.\r\n", ch);
        return FALSE;
      }
    }
    if (!vict || (!IS_ASTRAL(ch) && !IS_ASTRAL(vict)))
      process_manipulation_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(vict) &&
             (IS_ASTRAL(ch) ||
              IS_DUAL(ch) ||
              (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
               vict == FIGHTING(ch))))
      process_manipulation_spell(ch, vict, obj, spell, force);
    else if (IS_ASTRAL(ch) &&
             (IS_ASTRAL(vict) ||
              IS_DUAL(vict) ||
              PLR_FLAGGED(vict, PLR_PERCEIVE)))
      process_manipulation_spell(ch, vict, obj, spell, force);
    else {
      if (IS_ASTRAL(vict) && PLR_FLAGGED(ch, PLR_PERCEIVE))
        send_to_char("You can't initiate attacks on astral beings!\r\n", ch);
      else if (IS_ASTRAL(vict))
        send_to_char("You can't target astral beings!\r\n", ch);
      else
        send_to_char("You can't target physical beings!\r\n", ch);
      return FALSE;
    }
    break;
  case SPELL_TARGET_AREA:
    struct char_data *tch, *next_tch;
    for (tch = world[ch->in_room].people; tch; tch = next_tch)  {
      next_tch = tch->next_in_room;
      if (tch == ch)
        continue;
      if (!IS_NPC(tch) && IS_SENATOR(tch))
        continue;
      if (in_group(ch, tch))
        continue;
      if (!IS_ASTRAL(ch) && !IS_ASTRAL(tch))
        process_manipulation_spell(ch, tch, (obj_t *) NULL, spell, force);
      else if (IS_ASTRAL(tch) &&
               (IS_ASTRAL(ch) ||
                IS_DUAL(ch) ||
                (PLR_FLAGGED(ch, PLR_PERCEIVE) &&
                 tch == FIGHTING(ch))))
        process_manipulation_spell(ch, tch, (obj_t *) NULL, spell, force);
      else if (IS_ASTRAL(ch) &&
               (IS_ASTRAL(tch) ||
                IS_DUAL(tch) ||
                PLR_FLAGGED(tch, PLR_PERCEIVE)))
        process_manipulation_spell(ch, tch, (obj_t *) NULL, spell, force);
    }
    struct obj_data *obj, *next;
    if (spell->physical) {
      for (obj = world[ch->in_room].contents; obj; obj = next) {
        next = obj->next_content;
        damage_obj(NULL, obj, force + FORCE_PENALTY(spell),
                   spell->effect | DAMOBJ_MANIPULATION);
      }
      for (val = 0; val < NUM_OF_DIRS; val++)
        if (EXIT(ch, val) && EXIT(ch, val)->keyword &&
            IS_SET(EXIT(ch, val)->exit_info, EX_CLOSED))
          damage_door(ch, ch->in_room, val, (int)(force), spell->effect);
    }
    break;
  default:
    send_to_char("You can't seem to target this spell for some reason.\r\n", ch);
    mudlog("Illegal target in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return FALSE;
  }

  return TRUE;
}

void process_manipulation_spell(char_t *ch, char_t *vict, obj_t *obj, spell_t *spell, int force)
{
  struct obj_data *next;
  int resist = 0, success = 0, dam, i, low = NOWHERE, high = NOWHERE;
  int chance, room;

  switch (spell->type) {
  case SPELL_ANTI_BULLET:
  case SPELL_ANTI_SPELL:
  case SPELL_ARMOR:
  case SPELL_INFLUENCE:
  case SPELL_LIGHT:
  case SPELL_PETRIFY:
    sustain_spell(force, ch, vict, spell, 0);
    break;
  case SPELL_CLOUT:
    if (obj) {
      send_to_char("This spell can't be used on objects\r\n.", ch);
      return;
    }
    resist = GET_WIL(vict);

    success = success_test(force + spell_bonus(ch, spell), GET_IMPACT(vict));
    success -= success_test(resist + spell_resist(vict), GET_WIL(ch) + modify_target(vict));

    if (success < 1) {
      damage(ch, vict, 0, spell->type, FALSE);
      ranged_response(ch, vict);
      return;
    }
    if (IS_ASTRAL(ch))
      dam = convert_damage(stage(success, grimoire[spell->type].drain % 10));
    else
      dam = convert_damage(stage(success, grimoire[spell->type].damage));

    if (access_level(ch, LVL_VICEPRES) && spell->target != SPELL_TARGET_AREA)
      send_to_char(ch, "(%s) F: %d, R: %d, S: %d, D: %d\r\n",
                   spell->physical ? "Physical" : "Mana", force, resist, success, 0);

    if (spell->target == SPELL_TARGET_AREA)
      chance = 10;
    else
      chance = 2;
    for (i = 0; i < (NUM_WEARS - 1); i++)
      if (GET_EQ(vict, i) && number(1, 100) < chance) {
        damage_obj(NULL, GET_EQ(vict, i), force + FORCE_PENALTY(spell),
                   DAMOBJ_PROJECTILE);
        if (spell->target != SPELL_TARGET_AREA)
          break;
      }
    chance = (int)(chance / 2);
    for (obj = vict->carrying; obj; obj = next) {
      next = obj->next_content;
      if (number(1, 100) < chance)
        damage_obj(NULL, obj, force + FORCE_PENALTY(spell),
                   DAMOBJ_PROJECTILE);
    }

    damage(ch, vict, dam, spell->type, FALSE);
    ranged_response(ch, vict);
    break;
  case SPELL_TOXIC_WAVE:
  case SPELL_ELEMENTBALL:
  case SPELL_ELEMENT_BOLT:
  case SPELL_ELEMENT_CLOUD:
  case SPELL_ELEMENT_DART:
  case SPELL_ELEMENT_MISSILE:
    if (!obj) {
      if (spell->physical)
        resist = GET_BOD(vict);
      else
        resist = GET_WIL(vict);

      success = success_test(force + spell_bonus(ch, spell), resist + modify_target(ch));

      if (access_level(ch, LVL_VICEPRES) && spell->target != SPELL_TARGET_AREA)
        send_to_char(ch, "(%s) F: %d, R: %d, S: %d, D: %d\r\n",
                     spell->physical ? "Physical" : "Mana", force, resist, success, 0);

      if (success < 1) {
        damage(ch, vict, 0, spell->type, FALSE);
        ranged_response(ch, vict);
        return;
      }
      if (spell->effect != SPELL_EFFECT_NONE) {
        elemental_damage(ch, vict, spell, force, success);
        return;
      }
      success -= success_test(resist + spell_resist(vict), force + modify_target(vict));
      if (IS_ASTRAL(ch))
        dam = convert_damage(stage(success, grimoire[spell->type].drain % 10));
      else
        dam = convert_damage(stage(success, grimoire[spell->type].damage));

      if (spell->target == SPELL_TARGET_AREA)
        chance = 10;
      else
        chance = 2;
      for (i = 0; i < (NUM_WEARS - 1); i++)
        if (GET_EQ(vict, i) && number(1, 100) < chance) {
          damage_obj(NULL, GET_EQ(vict, i), force + FORCE_PENALTY(spell),
                     DAMOBJ_PROJECTILE);
          if (spell->target != SPELL_TARGET_AREA)
            break;
        }
      chance = (int)(chance / 2);
      for (obj = vict->carrying; obj; obj = next) {
        next = obj->next_content;
        if (number(1, 100) < chance)
          damage_obj(NULL, obj, force + FORCE_PENALTY(spell),
                     DAMOBJ_PROJECTILE);
      }

      damage(ch, vict, dam, spell->type, TRUE);
      ranged_response(ch, vict);
    } else {
      damage_obj(ch, obj, force + FORCE_PENALTY(spell),
                 spell->effect | DAMOBJ_MANIPULATION);

      if (vict && IS_NPC(vict) && !FIGHTING(vict))
        set_fighting(vict, ch);
    }
    break;
  case SPELL_TELEPORT:
    for (i = zone_table[world[ch->in_room].zone].number * 100; low == NOWHERE &&

         i <= zone_table[world[ch->in_room].zone].top; i++)
      low = real_room(i);
    for (i = zone_table[world[ch->in_room].zone].top; high == NOWHERE &&
         i >= zone_table[world[ch->in_room].zone].number * 100; i--)
      high = real_room(i);
    if ((low == NOWHERE || high == NOWHERE) || low >= high) {
      send_to_char("You are unable to control the necessary forces.\r\n", ch);
      return;
    }
    act("You carefully rip a whole in space and step through it.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n tears a whole in space and disappears into it.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    for (room = number(low, high), i = 0;
         i < 50 && ROOM_FLAGGED(room, ROOM_SENATE);
         i++)
      room = number(low, high);
    char_to_room(ch, room);
    act("$n appears from thin air.", TRUE, ch, 0, 0, TO_ROOM);
    if (ch->desc)
      look_at_room(ch, 0);
    break;
  default:
    send_to_char("Your spell fizzles into oblivion.\r\n", ch);
    mudlog("Illegal type in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return;
  }
  return;
}

void parse_category(spell_t *spell, struct char_data *ch, struct char_data *tch,
                    struct obj_data *tobj, int level)
{
  if (!tch || (!IS_ASTRAL(ch) && !IS_ASTRAL(tch)))
    ;
  else if (IS_ASTRAL(tch) &&
           (IS_ASTRAL(ch) || IS_DUAL(ch)))
    ;
  else if (IS_ASTRAL(ch) &&
           (IS_ASTRAL(tch) ||
            IS_DUAL(tch) ||
            PLR_FLAGGED(tch, PLR_PERCEIVE)))
    ;
  else
    return;

  switch (spell->category)
  {
  case SPELL_CATEGORY_COMBAT:
    process_combat_spell(ch, tch, tobj, spell, spell->force);
    break;
  case SPELL_CATEGORY_DETECTION:
    process_detection_spell(ch, tch, tobj, spell, spell->force);
    break;
  case SPELL_CATEGORY_HEALTH:
    process_health_spell(ch, tch, level, spell, spell->force);
    break;
  case SPELL_CATEGORY_ILLUSION:
    process_illusion_spell(ch, tch, tobj, spell, spell->force);
    break;
  case SPELL_CATEGORY_MANIPULATION:
    process_manipulation_spell(ch, tch, tobj, spell, spell->force);
    break;
  default:
    send_to_char("Your spell seems to make no sense to you whatsoever.\r\n", ch);
    mudlog("Unknown spell category in newmagic.cc.", ch, LOG_SYSLOG, TRUE);
    return;
  }
}

void mob_cast(struct char_data * ch, struct char_data * tch, struct obj_data * tobj,
              int spellnum, int level)
{
  spell_t *spell;

  if (GET_POS(ch) < POS_SITTING)
  {
    send_to_char("Not now!\r\n", ch);
    return;
  } else if (GET_MENTAL(ch) < (GET_MAX_MENTAL(ch) / 3))
  {
    send_to_char("You fear casting right now would do more harm than good.\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == tch))
  {
    send_to_char("You are afraid you might hurt your master!\r\n", ch);
    return;
  } else if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC))
  {
    send_to_char("You just can't do that here!\r\n", ch);
    return;
  }

  spell = new spell_t;
  int i = strlen(spells[spellnum]);
  spell->name = new char[i+1];
  strcpy(spell->name, spells[spellnum]);
  spell->physical = grimoire[spellnum].physical;
  spell->category = grimoire[spellnum].category;
  spell->force = MIN(GET_MAG(ch) / 100, MIN(10,
                     (GET_SKILL(ch, SKILL_SORCERY) ? GET_SKILL(ch, SKILL_SORCERY) : 0)));
  spell->target = grimoire[spellnum].target;
  spell->drain = grimoire[spellnum].drain;
  spell->damage = grimoire[spellnum].damage;
  spell->type = spellnum;
  spell->effect = level;
  spell->next = NULL;

  if (spell->force < 1)
  {
    send_to_char("You need some knowledge of sorcery to cast spells!\r\n", ch);
    if (spell->name)
      delete [] spell->name;
    delete spell;
    return;
  }

  if ((tch != ch) && spell->target == CASTER)
  {
    send_to_char("You can only cast this spell upon yourself!\r\n", ch);
    if (spell->name)
      delete [] spell->name;
    delete spell;
    return;
  }

  if (!tch || (!IS_ASTRAL(ch) && !IS_ASTRAL(tch)))
    ;
  else if (IS_ASTRAL(tch) &&
           (IS_ASTRAL(ch) ||
            IS_DUAL(ch)))
    ;
  else if (IS_ASTRAL(ch) &&
           (IS_ASTRAL(tch) ||
            IS_DUAL(tch) ||
            PLR_FLAGGED(tch, PLR_PERCEIVE)))
    ;
  else
  {
    if (IS_ASTRAL(tch))
      send_to_char("You can't target astral beings.\r\n", ch);
    else
      send_to_char("You can't target physical beings.\r\n", ch);
    if (spell->name)
      delete [] spell->name;
    delete spell;
    return;
  }

  if (spell->target == SPELL_TARGET_TOUCH)
  {
    if (GET_POS(tch) == POS_STANDING) {
      if (resisted_test(GET_QUI(ch), GET_QUI(tch) + modify_target(ch),
                        GET_QUI(tch), GET_QUI(ch) + modify_target(tch)) < 1) {
        act("You stumble over your own legs as you try to touch $N!",
            FALSE, ch, 0, tch, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!",
            FALSE, ch, 0, tch, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!",
            FALSE, ch, 0, tch, TO_VICT);
        if (!FIGHTING(tch))
          set_fighting(tch, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, tch);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (spell->name)
          delete [] spell->name;
        delete spell;
        return;
      }
    } else {
      if (resisted_test(GET_QUI(ch), (GET_QUI(tch) / 2) + modify_target(ch),
                        (GET_QUI(tch) / 2), GET_QUI(ch) + modify_target(tch)) < 1) {
        act("You stumble over your own legs as you try to touch $N!",
            FALSE, ch, 0, tch, TO_CHAR);
        act("$n stumbles over $s legs as $e reaches to touch $N!",
            FALSE, ch, 0, tch, TO_NOTVICT);
        act("$n stumbles over $s legs as $e reaches to touch you!",
            FALSE, ch, 0, tch, TO_VICT);
        if (!FIGHTING(tch))
          set_fighting(tch, ch);
        if (!FIGHTING(ch))
          set_fighting(ch, tch);
        WAIT_STATE(ch, PULSE_VIOLENCE);
        if (spell->name)
          delete [] spell->name;
        delete spell;
        return;
      }
    }
  }

  if (spell->target == AREA)
  {
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
      if (tch != ch)
        parse_category(spell, ch, tch, tobj, level);
  } else
    parse_category(spell, ch, tch, tobj, level);

  if (spellnum != SPELL_HEAL && spellnum != SPELL_ANTIDOTE &&
      spellnum != SPELL_CURE_DISEASE && spellnum != SPELL_RESIST_PAIN &&
      spellnum != SPELL_POISON)
    resist_drain(ch, spell->force, spell, DRAIN_LEVEL(spell->drain));
  else
    resist_drain(ch, spell->force, spell, level);

  if (FIGHTING(ch) && !AFF_FLAGGED(ch, AFF_ACTION))
    AFF_FLAGS(ch).SetBit(AFF_ACTION);
  WAIT_STATE(ch, PULSE_VIOLENCE);

  if (spell->name)
    delete [] spell->name;
  delete spell;
}

int spell_bonus(char_t *ch, spell_t *spell)
{
  return foci_bonus(ch, spell, spell->force, TRUE)
         + magic_pool_bonus(ch, spell, spell->force, TRUE)
         + totem_bonus(ch, spell);
}

// finds the spell by name and returns a pointer to it
spell_t *find_spell(char_t *ch, char *name)
{
  register spell_t *temp;

  for (temp = ch->spells; temp; temp = temp->next)
    if (is_abbrev(name, temp->name))
      break;

  if (temp)
    return temp;

  return NULL;
}

#undef _newmagic_cc_

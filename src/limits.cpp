/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#include <process.h>
#else
#include <unistd.h>
#endif

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "newdb.h"
#include "dblist.h"
#include "handler.h"
#include "interpreter.h"

extern class objList ObjList;
extern int check_spirit_sector(int room, int spirit);
extern int max_exp_gain;
extern int max_exp_loss;
extern int fixers_need_save;
extern int modify_target(struct char_data *ch);
extern int spell_resist(struct char_data *ch);
extern int reverse_web(struct char_data *ch, int &skill, int &target);
extern const char *composition_names[];
extern void write_spells(struct char_data *ch);
extern void check_trace(struct char_data *ic);
extern void end_quest(struct char_data *ch);
extern char *cleanup(char *dest, const char *src);

void mental_gain(struct char_data * ch)
{
  int gain = 0;

  if (IS_PROJECT(ch))
    return;

  if (IS_AFFECTED(ch, AFF_SLEEP))
  {
    AFF_FLAGS(ch).RemoveBit(AFF_SLEEP);
    return;
  }

  switch (GET_POS(ch))
  {
  case POS_STUNNED:
    gain = 20;
    break;
  case POS_SLEEPING:
    gain = 25;
    break;
  case POS_RESTING:
    gain = 20;
    break;
  case POS_SITTING:
    gain = 15;
    break;
  case POS_FIGHTING:
    gain = 5;
    break;
  case POS_STANDING:
    gain = 10;
    break;
  }

  if (IS_NPC(ch))
    gain *= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 1;

  if (GET_TRADITION(ch) == TRAD_ADEPT)
    gain *= GET_POWER(ch, ADEPT_HEALING) + 1;

  gain = MAX(1, gain);

  GET_MENTAL(ch) = MIN(GET_MAX_MENTAL(ch), GET_MENTAL(ch) + gain);
}

void physical_gain(struct char_data * ch)
{
  int gain = 0;
  struct obj_data *bio;

  if (IS_PROJECT(ch))
    return;

  switch (GET_POS(ch))
  {
  case POS_STUNNED:
    gain = 13;
    break;
  case POS_SLEEPING:
    gain = 15;
    break;
  case POS_RESTING:
    gain = 13;
    break;
  case POS_SITTING:
    gain = 10;
    break;
  case POS_FIGHTING:
    gain = 5;
    break;
  case POS_STANDING:
    gain = 7;
    break;
  }

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 1;

  if (IS_NPC(ch))
    gain *= 2;
  else
  {
    gain = MAX(1, gain);
    for (bio = ch->bioware; bio; bio = bio->next_content)
      if (GET_OBJ_VAL(bio, 2) == 1) {
        switch (GET_OBJ_VAL(bio, 0)) {
        case 1:
          gain = (int)(gain * 10/9);
          break;
        case 2:
          gain = (int)(gain * 7/5);
          break;
        case 3:
          gain *= 2;
          break;
        }
        break;
      }
  }
  if (GET_TRADITION(ch) == TRAD_ADEPT)
    gain *= GET_POWER(ch, ADEPT_HEALING) + 1;
  GET_PHYSICAL(ch) = MIN(GET_MAX_PHYSICAL(ch), GET_PHYSICAL(ch) + gain);
}

void set_title(struct char_data * ch, char *title)
{
  if (title == NULL)
    title = "";

  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    delete [] GET_TITLE(ch);

  GET_TITLE(ch) = str_dup(title);
}


void set_whotitle(struct char_data * ch, char *title)
{
  if (title == NULL)
    title = "title";

  if (strlen(title) > MAX_WHOTITLE_LENGTH)
    title[MAX_WHOTITLE_LENGTH] = '\0';


  if (GET_WHOTITLE(ch) != NULL)
    delete [] GET_WHOTITLE(ch);

  GET_WHOTITLE(ch) = str_dup(title);
}

void set_pretitle(struct char_data * ch, char *title)
{
  if (title == NULL)
    title = "";

  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_PRETITLE(ch) != NULL)
    delete [] GET_PRETITLE(ch);

  GET_PRETITLE(ch) = str_dup(title);
}

int gain_exp(struct char_data * ch, int gain, bool rep)
{
  int max_gain, old = (int)(GET_KARMA(ch) / 100);

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || IS_SENATOR(ch))))
    return 0;

  if (IS_NPC(ch))
  {
    GET_KARMA(ch) += (int)(gain / 10);
    return (int)(gain / 10);
  }

  if ( GET_TKE(ch) >= 0 && GET_TKE(ch) < 50 )
  {
    max_gain = 20;
  } else if ( GET_TKE(ch) >= 50 && GET_TKE(ch) < 100 )
  {
    max_gain = 50;
  } else
  {
    max_gain = GET_TKE(ch)/2;
  }

  if (gain > 0)
  {

    gain = MIN(max_gain, gain); /* put a cap on the max gain per kill */

    GET_KARMA(ch) += gain;
    GET_TKE(ch) += (int)(GET_KARMA(ch) / 100) - old;
    if (rep)
      GET_REP(ch) += (int)(GET_KARMA(ch) / 100) - old;
    else
      GET_NOT(ch) += (int)(GET_KARMA(ch) / 100) - old;
  } else if (gain < 0)
  {
    gain = MAX(-max_exp_loss, gain);    /* Cap max exp lost per death */
    GET_KARMA(ch) += gain;
    if (GET_KARMA(ch) < 0)
      GET_KARMA(ch) = 0;
    if (rep)
      GET_REP(ch) += (int)(GET_KARMA(ch) / 100) - old;
    else
      GET_NOT(ch) += (int)(GET_KARMA(ch) / 100) - old;
    GET_TKE(ch) += (int)(GET_KARMA(ch) / 100) - old;
  }

  return gain;
}

void gain_exp_regardless(struct char_data * ch, int gain)
{
  int old = (int)(GET_KARMA(ch) / 100);

  if (!IS_NPC(ch))
  {
    GET_KARMA(ch) += gain;
    if (GET_KARMA(ch) < 0)
      GET_KARMA(ch) = 0;
    GET_TKE(ch) += (int)(GET_KARMA(ch) / 100) - old;
    GET_REP(ch) += (int)(GET_KARMA(ch) / 100) - old;
  } else
  {
    GET_KARMA(ch) += gain;
    if (GET_KARMA(ch) < 0)
      GET_KARMA(ch) = 0;
  }
}

// only the pcs should need to access this
void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;
  struct obj_data *bio;

  if (GET_COND(ch, condition) == -1)    /* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  if (value == -1)
    for (bio = ch->bioware; bio; bio = bio->next_content)
      if (GET_OBJ_VAL(bio, 2) == 1)
      {
        switch (GET_OBJ_VAL(bio, 0)) {
        case 1:
          if (GET_OBJ_VAL(bio, 6))
            value--;
          GET_OBJ_VAL(bio, 6) = !GET_OBJ_VAL(bio, 6);
          break;
        case 2:
          if (!(GET_OBJ_VAL(bio, 6) % 3))
            value--;
          if ((++GET_OBJ_VAL(bio, 6)) > 9)
            GET_OBJ_VAL(bio, 6) = 0;
          break;
        case 3:
          value--;
          break;
        }
      } else if (GET_OBJ_VAL(bio, 2) == 5)
        value--;

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  if ( condition == DRUNK )
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));
  else
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_CUSTOMIZE) ||
      PLR_FLAGGED(ch, PLR_WRITING) || PLR_FLAGGED(ch, PLR_MAILING))
    return;

  switch (condition)
  {
  case FULL:
    send_to_char("Your stomach growls.\r\n", ch);
    return;
  case THIRST:
    send_to_char("Your mouth is dry.\r\n", ch);
    return;
  case DRUNK:
    if (intoxicated)
      send_to_char("Your head seems to clear slightly...\r\n", ch);
    return;
  default:
    break;
  }
}

void resist_petrify(struct char_data *ch)
{
  extern const char *spell_wear_off_msg[];
  struct affected_type *af;
  int resist = 0;

  for (af = ch->affected; af; af = af->next)
    if (af->type == SPELL_PETRIFY)
    {
      resist = GET_BOD(ch);
      break;
    } else if (af->type == SPELL_OVERSTIMULATION)
    {
      resist = GET_WIL(ch);
      break;
    }

  if (!resist || !af)
    return;

  if (success_test(resist + spell_resist(ch), af->modifier + modify_target(ch)) > 0)
  {
    send_to_char(ch, "%s\r\n", spell_wear_off_msg[af->type]);
    if (af->type == SPELL_PETRIFY)
      act("$n's skin returns to normal.", TRUE, ch, 0, 0, TO_ROOM);
    affect_remove(ch, af, 1);
  }
}

void remove_patch(struct char_data *ch)
{
  struct obj_data *patch = GET_EQ(ch, WEAR_PATCH);
  int stun;

  if (!patch)
    return;

  switch (GET_OBJ_VAL(patch, 0))
  {
  case 0:
    if (AFF_FLAGGED(ch, AFF_POISON) &&
        affected_by_spell(ch, SPELL_POISON) != 1) {
      AFF_FLAGS(ch).RemoveBit(AFF_POISON);
      send_to_char("You feel the effects of the toxin dissipate.\r\n", ch);
    }
    break;
  case 1:
    act("The effects of $p wear off, leaving you exhausted!", FALSE, ch, patch, 0, TO_CHAR);
    GET_MENTAL(ch) = MAX(0, GET_MENTAL(ch) - (GET_OBJ_VAL(patch, 1) - 1) * 100);
    if (GET_TRADITION(ch) == TRAD_HERMETIC || GET_TRADITION(ch) == TRAD_SHAMANIC &&
        success_test(GET_MAGIC(ch), GET_OBJ_VAL(patch, 1)) < 0) {
      send_to_char("You feel your magical ability decline.\r\n", ch);
      ch->real_abils.mag = MAX(0, ch->real_abils.mag - 100);
      affect_total(ch);
    }
    update_pos(ch);
    break;
  case 2:
    stun = resisted_test(GET_OBJ_VAL(patch, 1), GET_BOD(ch),
                         GET_BOD(ch), GET_OBJ_VAL(patch, 1));
    if (stun > 0) {
      act("You feel the drugs from $p take effect.", FALSE, ch, patch, 0, TO_CHAR);
      GET_MENTAL(ch) = MAX(0, GET_MENTAL(ch) - (stun * 100));
      update_pos(ch);
    } else
      act("You resist the feeble effects of $p.", FALSE, ch, patch, 0, TO_CHAR);
    break;
  case 3:
    if (success_test(GET_REAL_BOD(ch), GET_OBJ_VAL(patch, 1)) > 0)
      AFF_FLAGS(ch).RemoveBit(AFF_STABILIZE);
    break;
  }
  GET_EQ(ch, WEAR_PATCH) = NULL;
  patch->worn_by = NULL;
  patch->worn_on = -1;
  extract_obj(patch);
}

void check_idling(void)
{
  void perform_immort_invis(struct char_data *ch, int level);
  ACMD(do_return);
  ACMD(do_disconnect);
  struct char_data *ch, *next;

  for (ch = character_list; ch; ch = next) {
    next = ch->next;

    if (IS_NPC(ch) && ch->desc && ch->desc->original) {
      if (ch->desc->original->char_specials.timer > 10)
        do_return(ch, "", 0, 0);
    } else if (!IS_NPC(ch)) {
      ch->char_specials.timer++;
      if (!IS_SENATOR(ch)) {
        if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE &&
            ch->char_specials.timer > 15) {
          if ((AFF_FLAGGED(ch, AFF_PROGRAM) || AFF_FLAGGED(ch, AFF_PROGRAM)) && ch->desc)
            ch->char_specials.timer = 0;
          else {
            GET_WAS_IN(ch) = ch->in_room;
            if (FIGHTING(ch)) {
              stop_fighting(FIGHTING(ch));
              stop_fighting(ch);
            }
            act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
            send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
            char_from_room(ch);
            char_to_room(ch, 1);
          }
        } else if (ch->char_specials.timer > 30) {
          if (ch->in_room != NOWHERE)
            char_from_room(ch);
          char_to_room(ch, 1);
          if (ch->desc)
            close_socket(ch->desc);
          ch->desc = NULL;
          if (GET_QUEST(ch))
            end_quest(ch);
          sprintf(buf, "%s force-rented and extracted (idle).",
                  GET_CHAR_NAME(ch));
          mudlog(buf, ch, LOG_CONNLOG, TRUE);
          extract_char(ch);
        }
      } else if (!ch->desc && ch->char_specials.timer > 15) {
        sprintf(buf, "%s removed from game (no link).", GET_CHAR_NAME(ch));
        mudlog(buf, ch, LOG_CONNLOG, TRUE);
        extract_char(ch);
      } else if (IS_SENATOR(ch) && ch->char_specials.timer > 15 &&
                 GET_INVIS_LEV(ch) < 2 &&
                 PRF_FLAGGED(ch, PRF_AUTOINVIS))
        perform_immort_invis(ch, 2);
    }
  }
}

void check_bioware(struct char_data *ch)
{
  if (!ch->desc || (ch->desc && ch->desc->connected))
    return;

  struct obj_data *bio;
  int dam = 0;

  for (bio = ch->bioware; bio; bio = bio->next_content)
    if (GET_OBJ_VAL(bio, 2) == 0)
      break;

  if (bio && GET_OBJ_VAL(bio, 2) == 0)
  {
    if (GET_OBJ_VAL(bio, 5) < 1) {
      if (!success_test(GET_BOD(ch),
                        3 + modify_target(ch) + (int)(GET_OBJ_VAL(bio, 6) / 2))) {
        dam = convert_damage(stage(-success_test(GET_BOD(ch), 4 +
                                   modify_target(ch)), DEADLY));
        send_to_char("Your blood seems to erupt.\r\n", ch);
        damage(ch, ch, dam, TYPE_BIOWARE, PHYSICAL);
      }
      GET_OBJ_VAL(bio, 5) = 12;
      GET_OBJ_VAL(bio, 6)++;
    } else
      GET_OBJ_VAL(bio, 5)--;
  }
}

void check_swimming(struct char_data *ch)
{
  int target, skill, i, dam, test;

  if (IS_NPC(ch) || IS_SENATOR(ch))
    return;

  target = MAX(2, world[ch->in_room].rating);
  if (GET_POS(ch) < POS_RESTING)
  {
    target -= success_test(MAX(1, (int)(GET_REAL_BOD(ch) / 3)), target);
    dam = convert_damage(stage(target, 0));
    if (dam > 0) {
      act("$n's unconscious body is mercilessly thrown about by the current.",
          FALSE, ch, 0, 0, TO_ROOM);
      damage(ch, ch, dam, TYPE_DROWN, FALSE);
    }
    return;
  }
  skill = SKILL_ATHLETICS;
  if (!GET_SKILL(ch, skill))
    i = reverse_web(ch, skill, target);
  else
    i = GET_SKILL(ch, skill);
  i = resisted_test(i, target + modify_target(ch), target, i);
  if (i < 0)
  {
    test = success_test(GET_WIL(ch), modify_target(ch) - i);
    dam = convert_damage(stage(-test, SERIOUS));
    if (dam > 0) {
      send_to_char(ch, "You struggle to prevent your lungs getting "
                   "flooded with water.\r\n");
      damage(ch, ch, dam, TYPE_DROWN, FALSE);
    }
  } else if (!i)
  {
    test = success_test(GET_WIL(ch), 3 + modify_target(ch));
    dam = convert_damage(stage(-test, MODERATE));
    if (dam > 0) {
      send_to_char(ch, "You struggle to prevent your lungs getting "
                   "flooded with water.\r\n");
      damage(ch, ch, dam, TYPE_DROWN, FALSE);
    }
  } else if (i < 3)
  {
    test = success_test(GET_WIL(ch), 5 - i + modify_target(ch));
    dam = convert_damage(stage(-test, LIGHT));
    if (dam > 0) {
      send_to_char(ch, "You struggle to prevent your lungs getting "
                   "flooded with water.\r\n");
      damage(ch, ch, dam, TYPE_DROWN, FALSE);
    }
  }
}

void process_regeneration(int half_hour)
{
  struct char_data *ch, *next_char;

  for (ch = character_list; ch; ch = next_char) {
    next_char = ch->next;

    if ((affected_by_spell(ch, SPELL_PETRIFY) == 1 ||
         affected_by_spell(ch, SPELL_OVERSTIMULATION) == 1) && half_hour)
      resist_petrify(ch);
    if (GET_POS(ch) >= POS_STUNNED) {
      physical_gain(ch);
      mental_gain(ch);
      if (!IS_NPC(ch) && SECT(ch->in_room) == SECT_WATER_SWIM && half_hour)
        check_swimming(ch);
      if (GET_POS(ch) == POS_STUNNED)
        update_pos(ch);
    } else if (AFF_FLAGGED(ch, AFF_STABILIZE) && half_hour)
      AFF_FLAGS(ch).RemoveBit(AFF_STABILIZE);
    else if (GET_POS(ch) == POS_MORTALLYW && half_hour)
      damage(ch, ch, 1, TYPE_SUFFERING, PHYSICAL);
  }
  /* blood stuff */
  if (half_hour)
    for (int i = 0; i < top_of_world; i++)
      if (world[i].blood > 0)
        world[i].blood--;
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  SPECIAL(fixer);
  struct char_data *i, *next_char;
  FILE *fl;
  struct veh_data *veh;
  long room, v;
  struct obj_data *obj;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    if (!IS_NPC(i)) {
      i->char_specials.subscribe = NULL;
      playerDB.SaveChar(i, GET_LOADROOM(i));

      PLR_FLAGS(i).RemoveBit(PLR_CRASH);

      gain_condition(i, FULL, -1);
      gain_condition(i, DRUNK, -1);
      gain_condition(i, THIRST, -1);

      if (IS_SENATOR(i)
          && !access_level(i, LVL_ADMIN)) {
        GET_NUYEN(i) = 0;
        GET_BANK(i) = 0;
      }

      if (i->bioware)
        check_bioware(i);
      // check to see if char just died, as check_bioware might kill
    } else {
      /*     if (GET_MOB_SPEC(i) && GET_MOB_SPEC(i) == fixer && fixers_need_save)
             save_fixer_data(i);
       */
    }

    if (IS_NPC(i) || !PLR_FLAGGED(i, PLR_JUST_DIED)) {
      if (LAST_HEAL(i) > 0)
        LAST_HEAL(i)--;
      else if (LAST_HEAL(i) < 0)
        LAST_HEAL(i)++;
      if (GET_EQ(i, WEAR_PATCH))
        remove_patch(i);
    }
  }
  int num_veh = 0;
  for (veh = veh_list; veh; veh = veh->next)
    if ((veh->owner > 0 && veh->damage < 10) && playerDB.DoesExist(veh->owner))
      num_veh++;

  if (!(fl = fopen("veh/vfile", "w"))) {
    mudlog("SYSERR: Can't Open Vehicle File For Write.", NULL, LOG_SYSLOG, FALSE);
    return;
  }
  fprintf(fl, "%d\n", num_veh);
  fclose(fl);
  for (veh = veh_list, v = 0; veh && v < num_veh; veh = veh->next) {
    if (!(veh->owner > 0 && veh->damage < 10) || !playerDB.DoesExist(veh->owner)) {
      veh->owner = 0;
      continue;
    }
    sprintf(buf, "veh/%07ld", v);
    v++;
    if (!(fl = fopen(buf, "w"))) {
      mudlog("SYSERR: Can't Open Vehicle File For Write.", NULL, LOG_SYSLOG, FALSE);
      return;
    }

    if (veh->sub)
      for (i = character_list; i; i = i->next)
        if (GET_IDNUM(i) == veh->owner) {
          veh->next_sub = i->char_specials.subscribe;
          i->char_specials.subscribe = veh;
          break;
        }
    room = veh->in_room;
    if (!ROOM_FLAGGED(room, ROOM_GARAGE))
      if (zone_table[world[veh->in_room].zone].juridiction)
        switch (number(0, 2)) {
        case 0:
          room = real_room(2751 + number(0, 2));
          break;
        case 1:
          room = real_room(2756 + number(0, 2));
          break;
        case 2:
          room = real_room(2762 + number(0, 2));
          break;
        }
      else
        room = real_room(22670 + number(0, 16));
    fprintf(fl, "[VEHICLE]\n");
    fprintf(fl, "\tVnum:\t%ld\n", veh_index[veh->veh_number].vnum);
    fprintf(fl, "\tOwner:\t%ld\n", veh->owner);
    fprintf(fl, "\tInRoom:\t%ld\n", GET_ROOM_VNUM(room));
    fprintf(fl, "\tSubscribed:\t%d\n", veh->sub);
    fprintf(fl, "\tDamage:\t%d\n", veh->damage);

    fprintf(fl, "[CONTENTS]\n");
    int o = 0, level = 0;
    for (obj = veh->contents;obj;) {
      if (!IS_OBJ_STAT(obj, ITEM_NORENT)) {
        fprintf(fl, "\t[Object %d]\n", o);
        o++;
        fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj));
        fprintf(fl, "\t\tInside:\t%d\n", level);
        if (GET_OBJ_TYPE(obj) == ITEM_PHONE)
          for (int x = 0; x < 4; x++)
            fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
        else if (GET_OBJ_TYPE(obj) != ITEM_WORN)
          for (int x = 0; x < 10; x++)
            fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
        fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj).ToString());
        fprintf(fl, "\t\tAffFlags:\t%s\n", obj->obj_flags.bitvector.ToString());
        fprintf(fl, "\t\tCondition:\t%d\n", GET_OBJ_CONDITION(obj));
        if (obj->restring)
          fprintf(fl, "\t\tName:\t%s\n", obj->restring);
        if (obj->photo)
          fprintf(fl, "\t\tPhoto:$\n%s~\n", cleanup(buf2, obj->photo));
        for (int c = 0; c < MAX_OBJ_AFFECT; c++)
          if (obj->affected[c].location != APPLY_NONE && obj->affected[c].modifier != 0)
            fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                    c, obj->affected[c].location, c, obj->affected[c].modifier);
      }

      if (obj->contains && !IS_OBJ_STAT(obj, ITEM_NORENT)) {
        obj = obj->contains;
        level++;
        continue;
      } else if (!obj->next_content && obj->in_obj)
        while (obj && !obj->next_content && level >= 0) {
          obj = obj->in_obj;
          level--;
        }

      if (obj)
        obj = obj->next_content;
    }


    fprintf(fl, "[MODS]\n");
    for (int x = 0, v = 0; x < NUM_MODS - 1; x++)
      if (GET_MOD(veh, x)) {
        fprintf(fl, "\tMod%d:\t%ld\n", v, GET_OBJ_VNUM(GET_MOD(veh, x)));
        v++;
      }
    fprintf(fl, "[MOUNTS]\n");
    int m = 0;
    for (obj = veh->mount; obj; obj = obj->next_content, m++) {
      fprintf(fl, "\t[Mount %d]\n", m);
      fprintf(fl, "\t\tMountNum:\t%ld\n", GET_OBJ_VNUM(obj));
      fprintf(fl, "\t\tAmmo:\t%d\n", GET_OBJ_VAL(obj, 9));
      if (obj->contains) {
        fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj->contains));
        fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj->contains).ToString());
        fprintf(fl, "\t\tAffFlags:\t%s\n", obj->contains->obj_flags.bitvector.ToString());
        fprintf(fl, "\t\tCondition:\t%d\n", GET_OBJ_CONDITION(obj->contains));
        if (obj->restring)
          fprintf(fl, "\t\tName:\t%s\n", obj->contains->restring);
        for (int x = 0; x < 10; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj->contains, x));
        for (int c = 0; c < MAX_OBJ_AFFECT; c++)
          if (obj->contains->affected[c].location != APPLY_NONE && obj->contains->affected[c].modifier != 0)
            fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                    c, obj->contains->affected[c].location, c, obj->contains->affected[c].modifier);

      }
    }
    fprintf(fl, "[GRIDGUIDE]\n");
    o = 0;
    for (struct grid_data *grid = veh->grid; grid; grid = grid->next) {
      fprintf(fl, "\t[GRID %d]\n", o++);
      fprintf(fl, "\t\tName:\t%s\n", grid->name);
      fprintf(fl, "\t\tRoom:\t%ld\n", grid->room);
    }
    fclose(fl);
  }
  if (!(fl = fopen("etc/consist", "w"))) {
    mudlog("SYSERR: Can't Open Consistentcy File For Write.", NULL, LOG_SYSLOG, FALSE);
    return;
  }
  for (int m = 0; m < 5; m++) {
    market[m] = MIN(5000, market[m] + 10 + number(0, 10));
  }
  fprintf(fl, "[MARKET]\r\n");
  fprintf(fl, "\tBlue:\t%d\n", market[0]);
  fprintf(fl, "\tGreen:\t%d\n", market[1]);
  fprintf(fl, "\tOrange:\t%d\n", market[2]);
  fprintf(fl, "\tRed:\t%d\n", market[3]);
  fprintf(fl, "\tBlack:\t%d\n", market[4]);
  fclose(fl);
  // process the objects in the object list
  ObjList.UpdateCounters();
}

void misc_update(void)
{
  struct char_data *ch, *next_ch;
  struct obj_data *obj, *o = NULL;
  int i;

  // loop through all the characters
  for (ch = character_list; ch; ch = ch->next) {
    next_ch = ch->next;

    /*    for (af = ch->affected; af; af = af->next)
          if ((af->type == SPELL_HEAL || af->type == SPELL_ANTIDOTE ||
              af->type == SPELL_STABILIZE || af->type == SPELL_CURE_DISEASE) &&
              !af->caster) {
            if (af->duration > 0)
              af->duration--;
            else affect_remove(ch, af, 0);
          }
    */
    if (IS_NPC(ch) && !ch->desc && GET_MOB_VNUM(ch) >= 20 &&
        GET_MOB_VNUM(ch) <= 22) {
      act("$n dissolves into the background and is no more.",
          TRUE, ch, 0, 0, TO_ROOM);
      for (i = 0; i < NUM_WEARS; i++)
        if (ch->equipment[i])
          extract_obj(ch->equipment[i]);
      for (obj = ch->carrying; obj; obj = o) {
        o = obj->next_content;
        extract_obj(obj);
      }
      extract_char(ch);
    } else if (IS_NPC(ch) && !ch->desc && GET_MOB_VNUM(ch) >= 50 &&
               GET_MOB_VNUM(ch) < 70)
      extract_char(ch);
    else if (IS_SPIRIT(ch) && GET_MOB_VNUM(ch) >= 25 &&
             GET_MOB_VNUM(ch) <= 41) {
      if (GET_ACTIVE(ch) < 1) {
        act("$s service fulfilled, $n dissolves from existence.",
            TRUE, ch, 0, 0, TO_ROOM);
        act("Your pitiful life as a slave to a mortal is over."
            "Have a nice nonexistence!", FALSE, ch, 0, 0, TO_CHAR);
        extract_char(ch);
      } else if (!check_spirit_sector(ch->in_room, GET_MOB_VNUM(ch) - 24)) {
        act("Being away from its environment, $n suddenly ceases to "
            "physically exist.", TRUE, ch, 0, 0, TO_ROOM);
        act("You feel extremely homesick, and depart the depressing "
            "physical realm.", FALSE, ch, 0, 0, TO_CHAR);
        extract_char(ch);
      }
    }
  }
}

float gen_size(int race, bool height, int size, int sex)
{
  float mod;
  switch (size) {
  case 1:
    mod = 0.75;
    break;
  case 2:
    mod = 0.88;
    break;
  case 3:
  default:
    mod = 1;
    break;
  case 4:
    mod = 1.13;
    break;
  case 5:
    mod = 1.25;
    break;

  }
  switch (race) {
  case RACE_HUMAN:
    if (sex == SEX_MALE) {
      if (height)
        return number(160, 187) * mod;
      else
        return number(65, 77) * mod;
    } else {
      if (height)
        return number(145, 175) * mod;
      else
        return number(56, 69) * mod;
    }
    break;
  case RACE_DWARF:
    if (sex == SEX_MALE) {
      if (height)
        return number(115, 133) * mod;
      else
        return number(50, 62) * mod;
    } else {
      if (height)
        return number(80, 115) * mod;
      else
        return number(45, 56) * mod;
    }
    break;
  case RACE_ELF:
    if (sex == SEX_MALE) {
      if (height)
        return number(180, 205) * mod;
      else
        return number(70, 82) * mod;
    } else {
      if (height)
        return number(175, 195) * mod;
      else
        return number(60, 75) * mod;
    }
    break;
  case RACE_ORK:
    if (sex == SEX_MALE) {
      if (height)
        return number(185, 210) * mod;
      else
        return number(90, 105) * mod;
    } else {
      if (height)
        return number(178, 195) * mod;
      else
        return number(90, 105) * mod;
    }
    break;
  case RACE_TROLL:
    if (sex == SEX_MALE) {
      if (height)
        return number(270, 295) * mod;
      else
        return number(215, 245) * mod;

    } else {
      if (height)
        return number(255, 280) * mod;
      else
        return number(200, 230) * mod;
    }
    break;
  case RACE_CYCLOPS:
    if (sex == SEX_MALE) {
      if (height)
        return number(290, 340) * mod;
      else
        return number(240, 350) * mod;
    } else {
      if (height)
        return number(275, 320) * mod;
      else
        return number(220, 340) * mod;
    }
    break;
  case RACE_KOBOROKURU:
    if (sex == SEX_MALE) {
      if (height)
        return number(115, 133) * mod;
      else
        return number(50, 62) * mod;
    } else {
      if (height)
        return number(80, 112) * mod;
      else
        return number(45, 56) * mod;
    }
    break;
  case RACE_FOMORI:
    if (sex == SEX_MALE) {
      if (height)
        return number(270, 295) * mod;
      else
        return number(215, 245) * mod;
    } else {
      if (height)
        return number(255, 280) * mod;
      else
        return number(200, 230) * mod;
    }
    break;
  case RACE_MENEHUNE:
    if (sex == SEX_MALE) {
      if (height)
        return number(115, 133) * mod;
      else
        return number(50, 62) * mod;
    } else {
      if (height)
        return number(80, 112) * mod;
      else
        return number(45, 56) * mod;
    }
    break;
  case RACE_HOBGOBLIN:
    if (sex == SEX_MALE) {
      if (height)
        return number(185, 210) * mod;
      else
        return number(90, 105) * mod;
    } else {
      if (height)
        return number(178, 195) * mod;
      else
        return number(85, 95) * mod;
    }
    break;
  case RACE_GIANT:
    if (sex == SEX_MALE) {
      if (height)
        return number(300, 450) * mod;
      else
        return number(380, 477) * mod;
    } else {
      if (height)
        return number(296, 369) * mod;
      else
        return number(380, 430) * mod;
    }
    break;
  case RACE_GNOME:
    if (sex == SEX_MALE) {
      if (height)
        return number(85, 137) * mod;
      else
        return number(45, 59) * mod;
    } else {
      if (height)
        return number(75, 95) * mod;
      else
        return number(39, 52) * mod;
    }
    break;
  case RACE_ONI:
    if (sex == SEX_MALE) {
      if (height)
        return number(185, 215) * mod;
      else
        return number(90, 105) * mod;
    } else {
      if (height)
        return number(178, 195) * mod;
      else
        return number(80, 95) * mod;
    }
    break;
  case RACE_WAKYAMBI:
    if (sex == SEX_MALE) {
      if (height)
        return number(180, 205) * mod;
      else
        return number(70, 82) * mod;
    } else {
      if (height)
        return number(175, 195) * mod;
      else
        return number(60, 75) * mod;
    }
    break;
  case RACE_OGRE:
    if (sex == SEX_MALE) {
      if (height)
        return number(185, 235) * mod;
      else
        return number(90, 105) * mod;
    } else {
      if (height)
        return number(175, 195) * mod;
      else
        return number(85, 96) * mod;
    }
    break;
  case RACE_MINOTAUR:
    if (sex == SEX_MALE) {
      if (height)
        return number(200, 255) * mod;
      else
        return number(100, 145) * mod;
    } else {
      if (height)
        return number(145, 180) * mod;
      else
        return number(95, 120) * mod;
    }
    break;
  case RACE_SATYR:
    if (sex == SEX_MALE) {
      if (height)
        return number(180, 217) * mod;
      else
        return number(90, 105) * mod;
    } else {
      if (height)
        return number(175, 195) * mod;
      else
        return number(80, 95) * mod;
    }
    break;
  case RACE_NIGHTONE:
    if (sex == SEX_MALE) {
      if (height)
        return number(185, 227) * mod;
      else
        return number(90, 108) * mod;
    } else {
      if (height)
        return number(180, 188) * mod;
      else
        return number(80, 91) * mod;
    }
    break;
  case RACE_DRAGON:
    if (sex == SEX_MALE) {
      if (height)
        return number(300, 400) * mod;
      else
        return number(1900, 2100) * mod;
    } else {
      if (height)
        return number(400, 500) * mod;
      else
        return number(1950, 2300) * mod;
    }
    break;
  default:
    if (sex == SEX_MALE) {
      if (height)
        return number(160, 187) * mod;
      else
        return number(65, 77) * mod;
    } else {
      if (height)
        return number(145, 175) * mod;
      else
        return number(56, 69) * mod;
    }
  }
}

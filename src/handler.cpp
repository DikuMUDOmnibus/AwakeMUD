/**************************************************************************
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "newdb.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "memory.h"
#include "dblist.h"
#include "constants.h"
#include "newmatrix.h"

// memory object

/* external vars */
extern char *MENU;
extern char *QMENU;

/* external functions */
extern void stop_fighting(struct char_data * ch);
extern void remove_follower(struct char_data * ch);
extern void clearMemory(struct char_data * ch);
extern void print_object_location(int, struct obj_data *, struct char_data *, int);
extern void calc_weight(struct char_data *);
extern int skill_web(struct char_data *, int);
extern int return_general(int skill_num);
extern int can_wield_both(struct char_data *, struct obj_data *, struct obj_data *);
extern int ability_cost(int abil, int level);
extern int max_ability(int i);
extern bool write_spells(struct char_data *ch);

struct obj_data *find_obj(struct char_data *ch, char *name, int num);

char *fname(char *namelist)
{
  static char holder[30];
  register char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}


int isname(char *str, char *namelist)
{
  if(namelist == NULL)
    return 0;
  if(*namelist == '\0')
    return 0;
  if (namelist[0] == '\0')
    return 0;

  register char *curname, *curstr;

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if ((!*curstr && !isalpha(*curname)) || is_abbrev(curstr, curname))
        //      if (!*curstr && !isalpha(*curname))
        return (1);

      if (!*curname)
        return (0);

      if (!*curstr || *curname == ' ')
        break;

      if (LOWER(*curstr) != LOWER(*curname))
        break;
    }

    // skip to next name

    for (; isalpha(*curname); curname++)
      ;
    if (!*curname)
      return (0);
    curname++;                  // first char of new name
  }
}


void affect_modify(struct char_data * ch,
                   byte loc,
                   sbyte mod,
                   const Bitfield &bitv,
                   bool add
                    )
{
  int maxabil;

  if (add
     )
  {
    AFF_FLAGS(ch).SetAll(bitv);
  }
  else
  {
    AFF_FLAGS(ch).RemoveAll(bitv);
    mod = -mod;
  }

  maxabil = ((IS_NPC(ch) || (GET_LEVEL(ch) >= LVL_ADMIN)) ? 50 : 20);

  switch (loc)
  {
  case APPLY_NONE:
    break;

  case APPLY_STR:
    GET_STR(ch) += mod;
    break;
  case APPLY_QUI:
    GET_QUI(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT(ch) += mod;
    break;
  case APPLY_WIL:
    GET_WIL(ch) += mod;
    break;
  case APPLY_BOD:
    GET_BOD(ch) += mod;
    break;
  case APPLY_CHA:
    GET_CHA(ch) += mod;
    break;
  case APPLY_MAG:
    GET_MAG(ch) += (mod * 100);
    break;
  case APPLY_ESS:
    GET_ESS(ch) += (mod * 100);
    break;
  case APPLY_REA:
    GET_REA(ch) += mod;
    break;

  case APPLY_AGE:
    ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT(ch) += mod;
    break;

  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT(ch) += mod;
    break;

  case APPLY_MENTAL:
    GET_MAX_MENTAL(ch) += mod * 100;
    break;

  case APPLY_PHYSICAL:
    GET_MAX_PHYSICAL(ch) += mod * 100;
    break;

  case APPLY_BALLISTIC:
    GET_BALLISTIC(ch) += mod;
    break;

  case APPLY_IMPACT:
    GET_IMPACT(ch) += mod;
    break;

  case APPLY_ASTRAL_POOL:
    GET_ASTRAL(ch) += mod;
    break;

  case APPLY_COMBAT_POOL:
    GET_COMBAT(ch) += mod;
    break;

  case APPLY_HACKING_POOL:
    GET_HACKING(ch) += mod;
    break;

  case APPLY_CONTROL_POOL:
    GET_CONTROL(ch) += mod;
    break;

  case APPLY_MAGIC_POOL:
    GET_MAGIC(ch) += mod;   /* GET_MAGIC gets their magic pool, GET_MAG is for attribute*/
    break;

  case APPLY_INITIATIVE_DICE:
    GET_INIT_DICE(ch) += mod;
    break;

  case APPLY_TARGET:
    GET_TARGET_MOD(ch) += mod;
    break;

  default:
    log("SYSLOG: Unknown apply adjust: %s/%d.", GET_NAME(ch), loc);
    break;
  }                             /* switch */
}

void apply_focus_effect( struct char_data *ch, struct obj_data *object )
{
  int i;

  if (object->worn_by == NULL )
    return;

  if (GET_OBJ_TYPE(object) != ITEM_FOCUS)
    return;

  if (GET_OBJ_VAL(object, 9) != GET_IDNUM(ch))
    return;

  for (i = 0; i < (NUM_WEARS - 1); i++)
    if (GET_EQ(ch, i))
      if ((GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS) && (GET_EQ(ch,i) != object) &&
          (GET_OBJ_VAL(object, 8) == GET_OBJ_VAL(GET_EQ(ch, i), 8)) &&
          (object->affected[0].modifier <= GET_EQ(ch, i)->affected[0].modifier))
        return;

  if (GET_OBJ_VAL(object, 5) == 0)
  {
    if ((GET_FOCI(ch) + 1) > GET_INT(ch))
      return;

    GET_OBJ_VAL(object, 5) = 1;
    GET_FOCI(ch)++;
  }


  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    affect_modify(ch,
                  object->affected[i].location,
                  object->affected[i].modifier,
                  object->obj_flags.bitvector,TRUE);
  }
}

void remove_focus_effect( struct char_data *ch, struct obj_data *object )
{
  int i;
  if (GET_OBJ_TYPE(object) != ITEM_FOCUS)
    return;

  if (GET_OBJ_VAL(object, 9) != GET_IDNUM(ch))
    return;

  if (GET_OBJ_VAL(object, 5) == 0)
    return;

  for (i = 0; i < (NUM_WEARS - 1); i++)
    if (GET_EQ(ch, i))
      if ((GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS) && (GET_EQ(ch,i) != object) &&
          (GET_OBJ_VAL(object, 8) == GET_OBJ_VAL(GET_EQ(ch, i), 8)) &&
          (object->affected[i].modifier <= GET_EQ(ch, i)->affected[i].modifier))
        return;

  GET_OBJ_VAL(object, 5) = 0;
  GET_FOCI(ch)--;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    affect_modify(ch,
                  object->affected[i].location,
                  object->affected[i].modifier,
                  object->obj_flags.bitvector,
                  FALSE);
}

void affect_veh(struct veh_data *veh, byte loc, sbyte mod)
{
  switch (loc)
  {
  case VAFF_NONE:
    break;
  case VAFF_HAND:
    veh->handling += mod;
    break;
  case VAFF_SPD:
    veh->speed += mod;
    break;
  case VAFF_ACCL:
    veh->accel += mod;
    break;
  case VAFF_BOD:
    veh->body += mod;
    break;
  case VAFF_ARM:
    veh->armor += mod;
    break;
  case VAFF_SEN:
    veh->sensor = mod;
    break;
  case VAFF_SIG:
    veh->sig += mod;
    break;
  case VAFF_AUTO:
    veh->autonav = mod;
    break;
  case VAFF_SEA:
    veh->seating += mod;
    break;
  case VAFF_LOAD:
    veh->load += mod;
    break;
  case VAFF_PILOT:
    veh->pilot = mod;
    break;
  default:
    log("SYSLOG: Unknown apply adjust: %s/%d.", veh->short_description, loc);
    break;


  }
}
/* As affect_total() but for vehicles */
void affect_total_veh(struct veh_data * veh)
{
  int i, j;
  for (i = 0; i < (NUM_MODS - 1); i++)
  {
    if (GET_MOD(veh, i)) {
      for (j = 0; j < MAX_OBJ_AFFECT; j++) {
        affect_veh(veh, GET_MOD(veh, i)->affected[j].location,
                   GET_MOD(veh, i)->affected[j].modifier);
      }
    }
  }
}
/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data * ch)
{
  struct obj_data *cyber, *one, *two;
  struct affected_type *af;
  sh_int i, j, skill;
  int has_rig = 0;

  if (IS_PROJECT(ch))
    return;

  /* effects of used equipment */
  for (i = 0; i < (NUM_WEARS - 1); i++)
  {
    if (GET_EQ(ch, i)) {
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPON && i != WEAR_WIELD)
        continue;
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS)
        remove_focus_effect(ch, GET_EQ(ch, i));
      else
        for (j = 0; j < MAX_OBJ_AFFECT; j++)
          affect_modify(ch,
                        GET_EQ(ch, i)->affected[j].location,
                        GET_EQ(ch, i)->affected[j].modifier,
                        GET_EQ(ch, i)->obj_flags.bitvector, FALSE);
    }
  }

  /* effects of foci in inventory *
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_FOCUS)
      remove_focus_effect(ch, obj);*/

  /* effects of cyberware */
  for (cyber = ch->cyberware; cyber; cyber = cyber->next_content)
  {
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch,
                    cyber->affected[j].location,
                    cyber->affected[j].modifier,
                    cyber->obj_flags.bitvector, FALSE);
  }

  /* effects of bioware */
  for (cyber = ch->bioware; cyber; cyber = cyber->next_content)
  {
    if (GET_OBJ_VAL(cyber, 2) != 4 || (GET_OBJ_VAL(cyber, 2) == 4 &&
                                       GET_OBJ_VAL(cyber, 5) > 0))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify(ch,
                      cyber->affected[j].location,
                      cyber->affected[j].modifier,
                      cyber->obj_flags.bitvector, FALSE);
  }

  /* effects of spells */
  for (af = ch->affected; af; af = af->next)
    affect_modify(ch,
                  af->location,
                  af->modifier,
                  af->bitvector, FALSE);

  ch->aff_abils = ch->real_abils;

  /* calculate reaction before you add eq, cyberware, etc so that things *
   * such as wired reflexes work properly (as they only modify reaction  *
   * and not intelligence and quickness).            -cjd                */
  GET_REAL_REA(ch) = (GET_REAL_INT(ch) + GET_REAL_QUI(ch)) >> 1;
  GET_REA(ch) = (GET_INT(ch) + GET_QUI(ch)) >> 1;

  if (PLR_FLAGGED(ch, PLR_NEWBIE) && GET_TKE(ch) > 25)
  {
    PLR_FLAGS(ch).RemoveBit(PLR_NEWBIE);
    for (cyber = ch->cyberware; cyber; cyber = cyber->next_content) {
      if (IS_OBJ_STAT(cyber, ITEM_NODONATE))
        GET_OBJ_EXTRA(cyber).RemoveBit(ITEM_NODONATE);
      if (IS_OBJ_STAT(cyber, ITEM_NOSELL))
        GET_OBJ_EXTRA(cyber).RemoveBit(ITEM_NOSELL);
    }
  }

  /* set the dice pools before equip so that they can be affected */
  /* combat pool is equal to quickness, wil, and int divided by 2 */
  GET_COMBAT(ch) = 0;
  GET_HACKING(ch) = 0;
  GET_ASTRAL(ch) = 0;
  GET_MAGIC(ch) = 0;
  GET_CONTROL(ch) = 0;
  // reset initiative dice
  GET_INIT_DICE(ch) = 0;
  /* reset # of foci char has */
  if (!IS_NPC(ch))
    GET_FOCI(ch) = 0;

  if (REAL_SKILL(ch, SKILL_COMPUTER) > 0)
  {
    if (PLR_FLAGGED(ch, PLR_MATRIX) && ch->persona) {
      GET_HACKING(ch) += (int)((GET_INT(ch) + ch->persona->decker->mpcp) / 3);
    } else {
      for (struct obj_data *deck = ch->carrying; deck; deck = deck->next_content)
        if (GET_OBJ_TYPE(deck) == ITEM_CYBERDECK) {
          GET_HACKING(ch) += GET_OBJ_VAL(deck, 0);
          break;
        }
      GET_HACKING(ch) += GET_INT(ch);
      GET_HACKING(ch) = (int)(GET_HACKING(ch) / 3);
    }
  }

  if (IS_NPC(ch))
  {
    GET_BALLISTIC(ch) = GET_TOTALBAL(ch) = mob_proto[GET_MOB_RNUM(ch)].points.ballistic[0];
    GET_IMPACT(ch) = GET_TOTALIMP(ch) = mob_proto[GET_MOB_RNUM(ch)].points.impact[0];
  } else
    GET_BALLISTIC(ch) = GET_IMPACT(ch) = GET_TOTALBAL(ch) = GET_TOTALIMP(ch) = 0;
  /* effects of foci in inventory *
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_FOCUS)
      apply_focus_effect(ch, obj);*/

  /* effects of equipment */
  for (i = 0; i < (NUM_WEARS - 1); i++)
    if (GET_EQ(ch, i))
    {
      bool highestbal = FALSE, highestimp = FALSE;
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS)
        apply_focus_effect(ch, GET_EQ(ch,i));
      else
        for (j = 0; j < MAX_OBJ_AFFECT; j++)
          affect_modify(ch,
                        GET_EQ(ch, i)->affected[j].location,
                        GET_EQ(ch, i)->affected[j].modifier,
                        GET_EQ(ch, i)->obj_flags.bitvector, TRUE);
      if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WORN)
        if (GET_OBJ_VAL(GET_EQ(ch, i), 5) || GET_OBJ_VAL(GET_EQ(ch, i), 6)) {
          int bal = GET_OBJ_VAL(GET_EQ(ch, i), 5), imp = GET_OBJ_VAL(GET_EQ(ch, i), 6);
          for (j = 0; j < (NUM_WEARS - 1); j++)
            if (GET_EQ(ch, j) && j != i && GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_WORN) {
              if ((highestbal || GET_OBJ_VAL(GET_EQ(ch, i), 5) < GET_OBJ_VAL(GET_EQ(ch, j), 5)) &&
                  bal == GET_OBJ_VAL(GET_EQ(ch, i), 5))
                bal /= 2;
              if ((highestimp || GET_OBJ_VAL(GET_EQ(ch, i), 6) < GET_OBJ_VAL(GET_EQ(ch, j), 6)) &&
                  imp == GET_OBJ_VAL(GET_EQ(ch, i), 6))
                imp /= 2;
            }
          if (bal == GET_OBJ_VAL(GET_EQ(ch, i), 5))
            highestbal = TRUE;
          if (imp == GET_OBJ_VAL(GET_EQ(ch, i), 6))
            highestimp = TRUE;
          GET_IMPACT(ch) += imp;
          GET_BALLISTIC(ch) += bal;
          GET_TOTALIMP(ch) += GET_OBJ_VAL(GET_EQ(ch, i), 6);
          GET_TOTALBAL(ch) += GET_OBJ_VAL(GET_EQ(ch, i), 5);
        }
    }

  /* effects of cyberware */
  for (cyber = ch->cyberware; cyber; cyber = cyber->next_content)
  {
    if (GET_OBJ_VAL(cyber, 2) == 32)
      has_rig = GET_OBJ_VAL(cyber, 0);
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch,
                    cyber->affected[j].location,
                    cyber->affected[j].modifier,
                    cyber->obj_flags.bitvector, TRUE);
  }

  /* effects of bioware */
  for (cyber = ch->bioware; cyber; cyber = cyber->next_content)
  {
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_modify(ch,
                    cyber->affected[j].location,
                    cyber->affected[j].modifier,
                    cyber->obj_flags.bitvector, TRUE);
  }

  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

  /* Make certain values are between 1..50, not < 0 and not > 50! */

  i = ((IS_NPC(ch) || (GET_LEVEL(ch) >= LVL_ADMIN)) ? 50 : 20);

  GET_QUI(ch) = MAX(1, MIN(GET_QUI(ch), i));
  GET_CHA(ch) = MAX(1, MIN(GET_CHA(ch), i));
  GET_INT(ch) = MAX(1, MIN(GET_INT(ch), i));
  GET_WIL(ch) = MAX(1, MIN(GET_WIL(ch), i));
  GET_BOD(ch) = MAX(1, MIN(GET_BOD(ch), i));
  GET_STR(ch) = MAX(1, MIN(GET_STR(ch), i));
  GET_MAG(ch) = MAX(0, MIN(GET_MAG(ch), i * 100));
  GET_ESS(ch) = MAX(0, MIN(GET_ESS(ch), 600));
  GET_REA(ch) = MAX(1, MIN(GET_REA(ch), i));
  if (GET_TRADITION(ch) == TRAD_ADEPT)
  {
    if ( GET_INIT_DICE(ch) == 0 )
      GET_INIT_DICE(ch) += MIN(3, GET_POWER(ch, ADEPT_REFLEXES));
    if (GET_REAL_REA(ch) == GET_REA(ch))
      GET_REA(ch) += 2*MIN(3, GET_POWER(ch, ADEPT_REFLEXES));
    if (GET_POWER(ch, ADEPT_IMPROVED_QUI)) {
      GET_REA(ch) -= (GET_QUI(ch) + GET_INT(ch)) / 2;
      GET_QUI(ch) += GET_POWER(ch, ADEPT_IMPROVED_QUI);
      GET_REA(ch) += (GET_QUI(ch) + GET_INT(ch)) / 2;
    }
    GET_BOD(ch) += GET_POWER(ch, ADEPT_IMPROVED_BOD);
    GET_STR(ch) += GET_POWER(ch, ADEPT_IMPROVED_STR);
    if (BOOST(ch)[0][0] > 0)
      GET_STR(ch) += BOOST(ch)[0][1];
    if (BOOST(ch)[1][0] > 0)
      GET_QUI(ch) += BOOST(ch)[1][1];
    if (BOOST(ch)[2][0] > 0)
      GET_BOD(ch) += BOOST(ch)[2][1];
    GET_COMBAT(ch) += MIN(3, GET_POWER(ch, ADEPT_COMBAT_SENSE));
    if (GET_POWER(ch, ADEPT_LOW_LIGHT))
      AFF_FLAGS(ch).SetBit(AFF_LOW_LIGHT);
    if (GET_POWER(ch, ADEPT_THERMO))
      AFF_FLAGS(ch).SetBit(AFF_INFRAVISION);
    if (GET_POWER(ch, ADEPT_IMAGE_MAG))
      AFF_FLAGS(ch).SetBit(AFF_VISION_MAG_2);
  }


  /* fix pools to take into account new attributes */
  one = (GET_WIELDED(ch, 0) ? GET_EQ(ch, WEAR_WIELD) :
         (struct obj_data *) NULL);
  two = (GET_WIELDED(ch, 1) ? GET_EQ(ch, WEAR_HOLD) :
         (struct obj_data *) NULL);

  /* fixes a bug where unarmed combat was used instead of cyber-weapons - python */
  if (!one && !two) /* unarmed, so use cyber-weapons or unarmed combat */
  {
    if(has_cyberweapon(ch))
    {
      skill = REAL_SKILL(ch, SKILL_CYBER_IMPLANTS);
    } else
    {
      skill = REAL_SKILL(ch, SKILL_UNARMED_COMBAT);
    }
  }
  else if (one)
  {
    if (!REAL_SKILL(ch, GET_OBJ_VAL(one, 4)))
      skill = REAL_SKILL(ch, return_general(GET_OBJ_VAL(one, 4)));
    else
      skill = REAL_SKILL(ch, GET_OBJ_VAL(one, 4));
  } else if (two)
  {
    if (!REAL_SKILL(ch, GET_OBJ_VAL(two, 4)))
      skill = REAL_SKILL(ch, return_general(GET_OBJ_VAL(two, 4)));
    else
      skill = REAL_SKILL(ch, GET_OBJ_VAL(two, 4));
  } else
  {
    if (REAL_SKILL(ch, GET_OBJ_VAL(one, 4)) <= REAL_SKILL(ch, GET_OBJ_VAL(two, 4))) {
      if (!REAL_SKILL(ch, GET_OBJ_VAL(one, 4)))
        skill = REAL_SKILL(ch, return_general(GET_OBJ_VAL(one, 4)));
      else
        skill = REAL_SKILL(ch, GET_OBJ_VAL(one, 4));
    } else {
      if (!GET_SKILL(ch, GET_OBJ_VAL(two, 4)))
        skill = REAL_SKILL(ch, return_general(GET_OBJ_VAL(two, 4)));
      else
        skill = REAL_SKILL(ch, GET_OBJ_VAL(two, 4));
    }
  }
  GET_COMBAT(ch) += (GET_QUI(ch) + GET_WIL(ch) + GET_INT(ch)) / 2;
  if (GET_COMBAT(ch) < 0)
    GET_COMBAT(ch) = 0;
  if (GET_TOTALBAL(ch) > GET_QUI(ch))
    GET_COMBAT(ch) -= (GET_TOTALBAL(ch) - GET_QUI(ch)) / 2;
  if (GET_TOTALIMP(ch) > GET_QUI(ch))
    GET_COMBAT(ch) -= (GET_TOTALIMP(ch) - GET_QUI(ch)) / 2;
  if (GET_TRADITION(ch) == TRAD_ADEPT)
    GET_IMPACT(ch) += GET_POWER(ch, ADEPT_MYSTIC_ARMOUR);
  for (i = 0; i < (NUM_WEARS -1); i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_GYRO)
    {
      GET_COMBAT(ch) /= 2;
      break;
    }
  GET_DEFENSE(ch) = MIN(GET_DEFENSE(ch), GET_COMBAT(ch));
  GET_OFFENSE(ch) = GET_COMBAT(ch) - GET_DEFENSE(ch);
  if (GET_OFFENSE(ch) > skill)
  {
    GET_DEFENSE(ch) += GET_OFFENSE(ch) - skill;
    GET_OFFENSE(ch) = skill;
  }
  if (!(!IS_NPC(ch) && GET_TRADITION(ch) != TRAD_SHAMANIC &&
        GET_TRADITION(ch) != TRAD_HERMETIC))
    GET_ASTRAL(ch) += (GET_MAG(ch) / 100) + GET_INT(ch) + GET_SKILL(ch, SKILL_SORCERY);
  if (!(!IS_NPC(ch) && GET_TRADITION(ch) != TRAD_SHAMANIC &&
        GET_TRADITION(ch) != TRAD_HERMETIC))
    GET_MAGIC(ch) += GET_SKILL(ch, SKILL_SORCERY);

  if (has_rig)
  {
    GET_CONTROL(ch) += GET_REA(ch) + (int)(2 * has_rig);
    GET_HACKING(ch) -= has_rig;
    if (GET_HACKING(ch) < 0)
      GET_HACKING(ch) = 0;
  }


}

/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af)
{
  int i = 0;
  for (struct affected_type *aff = ch->affected; aff; aff = aff->next)
    i++;
  if (i >= MAX_AFFECT)
    return;
  struct affected_type *affected_alloc;

  affected_alloc = new struct affected_type;

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
  affect_total(ch);
}

/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(struct char_data * ch, struct affected_type * af, int message)
{
  struct char_data *tch = character_list;
  struct affected_type *af2, *temp;
  bool found = FALSE;

  affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);

  if (af->sustained_by < 0)
    while (tch != NULL && (IS_NPC(tch) ? tch->nr : 0) != -af->sustained_by)
      tch = tch->next;
  else
    while (tch != NULL && (IS_NPC(tch) ? 0 : GET_IDNUM(tch)) != af->sustained_by)
      tch = tch->next;

  if (tch)
    for (af2 = tch->affected; af2 && !found; af2 = af2->next)
      if ((af2->type == af->type) && (af2->caster == TRUE) &&
          (af2->sustained_by == (IS_NPC(ch) ? -ch->nr : GET_IDNUM(ch))))
      {
        if (af2->type < MAX_SPELLS) {
          sprintf(buf, "You no longer sustain %s.\r\n", spells[af->type]);
          if (message && af->type != SPELL_HEAL && af->type != SPELL_ANTIDOTE &&
              af->type != SPELL_STABILIZE && af->type != SPELL_CURE_DISEASE)
            send_to_char(buf, tch);
          if (GET_SUSTAINED(tch) > 0)
            GET_SUSTAINED(tch) -= 1;
        }
        REMOVE_FROM_LIST(af2, tch->affected, next);
        if ( af != af2 )
          delete af2;
        found = TRUE;
      }

  if (af->type == SPELL_INFLUENCE && ch->master)
    stop_follower(ch);
  else if (af->type == SPELL_LIGHT && ch->in_room != NOWHERE)
    world[ch->in_room].light--;
  REMOVE_FROM_LIST(af, ch->affected, next);
  delete af;
  affect_total(ch);
}

/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data * ch, sh_int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next)
  {
    next = hjp->next;
    if (hjp->type == type)
      affect_remove(ch, hjp, 1);
  }
}

/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates
 * not affected
 */
int affected_by_spell(struct char_data * ch, sh_int type)
{
  struct affected_type *hjp;
  struct obj_data *obj;
  int i = 0;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if ((hjp->type == type) && (hjp->caster == FALSE))
      return 1;

  for (obj = ch->carrying; obj && i < 100; obj = obj->next_content)
  {
    if (GET_OBJ_TYPE(obj) == ITEM_FOCUS
        && GET_OBJ_VAL(obj, 0) == FOCI_LOCK
        && GET_OBJ_VAL(obj, 9) == (IS_NPC(ch) ? -1 : GET_IDNUM(ch))
        && GET_OBJ_VAL(obj, 8) == type)
      return 2;
    i++;
  }
  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(ch, i)) && GET_OBJ_TYPE(obj) == ITEM_FOCUS &&
        GET_OBJ_VAL(obj, 0) == FOCI_LOCK && GET_OBJ_VAL(obj, 8) == type &&
        GET_OBJ_VAL(obj, 9) == (IS_NPC(ch) ? -1 : GET_IDNUM(ch)))
      return 2;

  return 0;
}

void affect_join(struct char_data * ch, struct affected_type * af,
                 bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = hjp->next)
  {
    if (hjp->type == af->type) {

      if (add_dur)
        af->duration += hjp->duration;
      if (avg_dur)
        af->duration >>= 1;

      if (add_mod)
        af->modifier += hjp->modifier;
      if (avg_mod)
        af->modifier >>= 1;

      affect_remove(ch, hjp, 1);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}

void veh_from_room(struct veh_data * veh)
{
  struct veh_data *temp;
  if (veh == NULL || veh->in_room == NOWHERE)
  {
    log("YOU SCREWED IT UP");
    shutdown();
  }
  REMOVE_FROM_LIST(veh, world[veh->in_room].vehicles, next_veh);
  world[veh->in_room].light--;
  veh->in_room = NOWHERE;
  veh->next_veh = NULL;
}

/* move a player out of a room */
void char_from_room(struct char_data * ch)
{
  struct char_data *temp;

  if ((ch == NULL || ch->in_room == NOWHERE) && ch->in_veh == NULL)
  {
    log("SYSLOG: NULL or NOWHERE in handler.c, char_from_room");
    shutdown();
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))       /* Light is ON */
        world[ch->in_room].light--;
  if (affected_by_spell(ch, SPELL_LIGHT))
    world[ch->in_room].light--;

  if (ch->in_veh)
  {
    REMOVE_FROM_LIST(ch, ch->in_veh->people, next_in_veh);
    ch->in_veh = NULL;
    ch->next_in_veh = NULL;
  } else
  {
    if (IS_SENATOR(ch) && PRF_FLAGGED(ch, PRF_PACIFY) && world[ch->in_room].peaceful > 0)
      world[ch->in_room].peaceful--;
    REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
    ch->in_room = NOWHERE;
    ch->next_in_room = NULL;

  }
}

void char_to_veh(struct veh_data * veh, struct char_data * ch)
{
  if (!veh || !ch)
    log("SYSLOG: Illegal value(s) passed to char_to_veh");
  else
  {
    if (ch->in_room != NOWHERE)
      char_from_room(ch);
    ch->next_in_veh = veh->people;
    veh->people = ch;
    ch->in_veh = veh;
  }
}

/* place a vehicle in a room */
void veh_to_room(struct veh_data * veh, int room)
{
  if (!veh || room < 0 || room > top_of_world)
    log("SYSLOG: Illegal value(s) passed to veh_to_room");
  else
  {
    veh->next_veh = world[room].vehicles;
    world[room].vehicles = veh;
    veh->in_room = room;
    world[room].light++;
  }
}

void icon_to_host(struct matrix_icon *icon, vnum_t to_host)
{
  extern void make_seen(struct matrix_icon *icon, int idnum);
  if (!icon || to_host < 0 || to_host > top_of_matrix)
    log("SYSLOG: Illegal value(s) passed to icon_to_host");
  else
  {
    if (icon->decker)
      for (struct matrix_icon *icon2 = matrix[to_host].icons; icon2; icon2 = icon2->next_in_host)
        if (icon2->decker) {
          int target = icon->decker->masking;
          for (struct obj_data *soft = icon->decker->software; soft; soft = soft->next_content)
            if (GET_OBJ_VAL(soft, 0) == SOFT_SLEAZE)
              target += GET_OBJ_VAL(soft, 1);
          if (success_test(icon2->decker->sensor, target) > 0) {
            make_seen(icon2, icon->idnum);
            send_to_icon(icon2, "%s enters the host.\r\n", icon->name);
          }
        }
    icon->next_in_host = matrix[to_host].icons;
    matrix[to_host].icons = icon;
    icon->in_host = to_host;
  }
}

void icon_from_host(struct matrix_icon *icon)
{
  struct matrix_icon *temp;
  if (icon == NULL || icon->in_host == NOWHERE)
  {
    log("If thats happens to much im going to make it intentionally segfault.\r\nPS. Don't you just love my pointless debugging messages :)");
    shutdown();
  }
  REMOVE_FROM_LIST(icon, matrix[icon->in_host].icons, next_in_host);
  REMOVE_FROM_LIST(icon, matrix[icon->in_host].fighting, next_fighting);
  temp = NULL;
  for (struct matrix_icon *icon2 = matrix[icon->in_host].icons; icon2; icon2 = icon2->next_in_host)
  {
    if (icon2->fighting == icon) {
      icon2->fighting = NULL;
      REMOVE_FROM_LIST(icon2, matrix[icon->in_host].fighting, next_fighting);
    }
  }
  icon->in_host = NOWHERE;
  icon->next_in_host = NULL;
  icon->fighting = NULL;
}
/* place a character in a room */
void char_to_room(struct char_data * ch, long room)
{
  if (!ch || room < 0 || room > top_of_world)
  {
    log("SYSLOG: Illegal value(s) passed to char_to_room");
    room = 0;
  }
  ch->next_in_room = world[room].people;
  world[room].people = ch;
  ch->in_room = room;
  if (IS_SENATOR(ch) && PRF_FLAGGED(ch, PRF_PACIFY))
    world[ch->in_room].peaceful++;

  if (GET_EQ(ch, WEAR_LIGHT))
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))     /* Light ON */
        world[room].light++;
  if (affected_by_spell(ch, SPELL_LIGHT))
    world[room].light++;
}

#define IS_INVIS(o) IS_OBJ_STAT(o, ITEM_INVISIBLE)

/* give an object to a char   */
void
obj_to_char(struct obj_data * object, struct char_data * ch)
{
  struct obj_data *i = NULL, *op = NULL;

  if (object && ch)
  {
    if (object->carried_by) {
      sprintf(buf, "Obj_to_char error on %s giving to %s. Already belongs to %s.", object->text.name,
              GET_CHAR_NAME(ch) ? GET_CHAR_NAME(ch) : GET_NAME(ch),
              GET_CHAR_NAME(object->carried_by) ? GET_CHAR_NAME(object->carried_by) : GET_NAME(object->carried_by));
      mudlog(buf, ch, LOG_WIZLOG, TRUE);

      return;
    } else if (object->in_room != NOWHERE) {
      sprintf(buf, "Obj_to_char error on %s giving to %s. Already in %ld.", object->text.name,
              GET_CHAR_NAME(ch) ? GET_CHAR_NAME(ch) : GET_NAME(ch), world[object->in_room].number);
      mudlog(buf, ch, LOG_WIZLOG, TRUE);
      return;
    }
    for (i = ch->carrying; i; i = i->next_content) {
      if (i->item_number == object->item_number &&
          !strcmp(i->text.room_desc, object->text.room_desc))
        break;
      op = i;
    }

    if (i) {
      object->next_content = i;
      if (op)
        op->next_content = object;
      else
        ch->carrying = object;
    } else {
      object->next_content = ch->carrying;
      ch->carrying = object;
    }

    object->carried_by = ch;
    object->in_room = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    /* set flag for crash-save system */
    if (!IS_NPC(ch))
      PLR_FLAGS(ch).SetBit(PLR_CRASH);

    if (GET_OBJ_TYPE(object) == ITEM_FOCUS)
      apply_focus_effect(ch, object);

  } else
    log("SYSLOG: NULL obj or char passed to obj_to_char");
}

void reduce_abilities(struct char_data *vict)
{
  int i;

  if (GET_PP(vict) >= 0)
    return;

  for (i = number(1, ADEPT_NUMPOWER); GET_PP(vict) < 0;
       i = number(1, ADEPT_NUMPOWER))
  {
    if (GET_POWER(vict, i) > 0) {
      GET_PP(vict) += ability_cost(i, GET_POWER(vict, i));
      GET_POWER(vict, i)--;
      send_to_char(vict, "Your loss in magic makes you feel less "
                   "skilled in %s.\r\n", adept_powers[i]);
    }
    int y = 0;
    for (int x = 0; x < ADEPT_NUMPOWER; x++)
      if (GET_POWER(vict, x))
        y += ability_cost(x, GET_POWER(vict, x));
    if (y < 1)
      return;
  }
}

void obj_to_cyberware(struct obj_data * object, struct char_data * ch)
{
  int temp;

  if (object && ch)
  {
    if (GET_OBJ_TYPE(object) != ITEM_CYBERWARE) {
      log("Non-cyberware object type passed to obj_to_cyberware.");
      return;
    }

    object->next_content = ch->cyberware;
    ch->cyberware = object;
    object->carried_by = ch;
    object->in_room = NOWHERE;

    if (!IS_NPC(ch))
      PLR_FLAGS(ch).SetBit(PLR_CRASH);

    ch->real_abils.ess -= GET_TOTEM(ch) == TOTEM_EAGLE ?
                          GET_OBJ_VAL(object, 1) << 1 :
                          GET_OBJ_VAL(object, 1);

    if (GET_TRADITION(ch) == TRAD_ADEPT) {
      ch->real_abils.mag -= GET_TOTEM(ch) == TOTEM_EAGLE ?
                            GET_OBJ_VAL(object, 1) << 1 :
                            GET_OBJ_VAL(object, 1);
      if (GET_PP(ch) < 0)
        reduce_abilities(ch);
    }
    ch->real_abils.mag = ch->real_abils.rmag - (600 - ((int)(ch->real_abils.ess / 100) * 100));

    if (ch->real_abils.mag < 100 && GET_TRADITION(ch) != TRAD_MUNDANE) {
      send_to_char(ch, "You feel the last of your magic leave your body.\r\n");
      GET_TRADITION(ch) = TRAD_MUNDANE;
    }
    for (temp = 0; temp < MAX_OBJ_AFFECT; temp++)
      affect_modify(ch,
                    object->affected[temp].location,
                    object->affected[temp].modifier,
                    object->obj_flags.bitvector, TRUE);

    affect_total(ch);
  } else
    log("SYSLOG: NULL obj or char passed to obj_to_cyberware");
}

void obj_to_bioware(struct obj_data * object, struct char_data * ch)
{
  int temp;

  if (object && ch)
  {
    if (GET_OBJ_TYPE(object) != ITEM_BIOWARE) {
      log("Non-bioware object type passed to obj_to_bioware.");
      return;
    }

    object->next_content = ch->bioware;
    ch->bioware = object;
    object->carried_by = ch;
    object->in_room = NOWHERE;

    if (!IS_NPC(ch))
      PLR_FLAGS(ch).SetBit(PLR_CRASH);

    GET_INDEX(ch) -= GET_OBJ_VAL(object, 1);

    GET_MAG(ch) -= GET_TOTEM(ch) == TOTEM_EAGLE ?
                   GET_OBJ_VAL(object, 1) << 1 :
                   GET_OBJ_VAL(object, 1);
    if (GET_TRADITION(ch) == TRAD_ADEPT && GET_PP(ch) < 0)
      reduce_abilities(ch);
    if (GET_OBJ_VAL(object, 4) != 1)
      object->obj_flags.value[4] = 1;

    if (GET_OBJ_VAL(object, 2) != 4 || GET_OBJ_VAL(object, 5) > 0)
      for (temp = 0; temp < MAX_OBJ_AFFECT; temp++)
        affect_modify(ch,
                      object->affected[temp].location,
                      object->affected[temp].modifier,
                      object->obj_flags.bitvector, TRUE);

    affect_total(ch);
  } else
    log("SYSLOG: NULL obj or char passed to obj_to_bioware");
}

void obj_from_bioware(struct obj_data *bio)
{
  struct obj_data *temp;
  int i;

  if (bio == NULL)
  {
    log("SYSLOG: NULL object passed to obj_from_bioware");
    return;
  }
  if (!IS_NPC(bio->carried_by))
    PLR_FLAGS(bio->carried_by).SetBit(PLR_CRASH);

  GET_INDEX(bio->carried_by) += GET_OBJ_VAL(bio, 1);

  if (GET_OBJ_VAL(bio, 2) == 4 && GET_OBJ_VAL(bio, 5) < 1)
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      affect_modify(bio->carried_by,
                    bio->affected[i].location,
                    bio->affected[i].modifier,
                    bio->obj_flags.bitvector, FALSE);

  affect_total(bio->carried_by);

  REMOVE_FROM_LIST(bio, bio->carried_by->bioware, next_content);
  bio->carried_by = NULL;
  bio->next_content = NULL;
}

/* take an object from a char */
void obj_from_char(struct obj_data * object)
{
  struct obj_data *temp;

  if (object == NULL)
  {
    log("SYSLOG: NULL object passed to obj_from_char");
    return;
  }
  if (object->in_obj)
  {
    sprintf(buf, "%s removed from char (%s), also in obj (%s). Removing.", object->text.name,
            GET_CHAR_NAME(object->carried_by) ? GET_CHAR_NAME(object->carried_by) :
            GET_NAME(object->carried_by), object->in_obj->text.name);
    mudlog(buf, object->carried_by, LOG_WIZLOG, TRUE);
    obj_from_obj(object);
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system */
  if (!IS_NPC(object->carried_by))
    PLR_FLAGS(object->carried_by).SetBit(PLR_CRASH);

  if (GET_OBJ_TYPE(object) == ITEM_FOCUS)
    remove_focus_effect(object->carried_by, object);
  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}

/* Removes a piece of cyberware from the cyberware list */
void obj_from_cyberware(struct obj_data * cyber)
{
  struct obj_data *temp;
  int i;
  int ess;

  if (cyber == NULL)
  {
    log("SYSLOG: NULL object passed to obj_from_cyberware");
    return;
  }

  /* set flag for crash-save system */
  if (!IS_NPC(cyber->carried_by))
    PLR_FLAGS(cyber->carried_by).SetBit(PLR_CRASH);


  ess =(600 - ((int)(cyber->carried_by->real_abils.ess / 100) * 100 ));
  cyber->carried_by->real_abils.ess += GET_OBJ_VAL(cyber, 1);

  if (GET_TRADITION(cyber->carried_by) == TRAD_ADEPT)
    cyber->carried_by->real_abils.mag += GET_OBJ_VAL(cyber, 1);
  else if (GET_TRADITION(cyber->carried_by) == TRAD_HERMETIC ||
           GET_TRADITION(cyber->carried_by) == TRAD_SHAMANIC)
    cyber->carried_by->real_abils.mag = cyber->carried_by->real_abils.rmag - ess;


  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    affect_modify(cyber->carried_by,
                  cyber->affected[i].location,
                  cyber->affected[i].modifier,
                  cyber->obj_flags.bitvector, FALSE);

  affect_total(cyber->carried_by);

  REMOVE_FROM_LIST(cyber, cyber->carried_by->cyberware, next_content);
  cyber->carried_by = NULL;
  cyber->next_content = NULL;
}

void equip_char(struct char_data * ch, struct obj_data * obj, int pos)
{
  int j;
  int invalid_class(struct char_data *ch, struct obj_data *obj);

  if (IS_NPC(ch) && pos == WEAR_WIELD)
  {
    if (!GET_EQ(ch, WEAR_WIELD))
      GET_WIELDED(ch, 0) = 1;
    else if (!GET_EQ(ch, WEAR_HOLD) && can_wield_both(ch, GET_EQ(ch, WEAR_WIELD), obj)) {
      pos = WEAR_HOLD;
      GET_WIELDED(ch, 1) = 1;
    } else {
      log("SYSLOG: trying to equip invalid or third weapon: %s, %s", GET_NAME(ch),
          obj->text.name);

      return;
    }
  } else if (GET_EQ(ch, pos))
  {
    log("SYSLOG: Char is already equipped: %s, %s",
        GET_NAME(ch), obj->text.name);
    return;
  }
  if (obj->carried_by)
  {
    log("SYSLOG: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (obj->in_room != NOWHERE)
  {
    log("SYSLOG: EQUIP: Obj is in_room when equip.");
    return;
  }
  if (invalid_class(ch, obj))
  {
    act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
    obj_to_char(obj, ch);     /* changed to drop in inventory instead of
                                                                                 * ground */
    return;
  }

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  if (ch->in_room != NOWHERE)
  {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))  /* if light is ON */
        world[ch->in_room].light++;
  }

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify(ch,
                  obj->affected[j].location,
                  obj->affected[j].modifier,
                  obj->obj_flags.bitvector, TRUE);

  affect_total(ch);
  calc_weight(ch);
}

struct obj_data *unequip_char(struct char_data * ch, int pos)
{
  int j;
  struct obj_data *obj, *tempobj;
  bool natural = TRUE;

  if (pos < 0 || pos >= NUM_WEARS)
    log("SYSERR: pos < 0 || pos >= NUM_WEARS, %s - %d", GET_NAME(ch), pos);

  if (!GET_EQ(ch, pos))
    log("SYSERR: Trying to remove non-existent item from %s at %d", GET_NAME(ch), pos);

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (ch->in_room != NOWHERE)
  {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))  /* if light is ON */
        world[ch->in_room].light--;
  }

  if (pos == WEAR_HOLD || pos == WEAR_WIELD)
  {
    if (FIGHTING(ch))
      AFF_FLAGS(ch).SetBit(AFF_APPROACH);
    GET_WIELDED(ch, pos - WEAR_WIELD) = 0;
  }

  GET_EQ(ch, pos) = NULL;

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify(ch,
                  obj->affected[j].location,
                  obj->affected[j].modifier,
                  obj->obj_flags.bitvector, FALSE);

  /* Give natural vision back */
  for ( tempobj = ch->cyberware; tempobj != NULL; tempobj = tempobj->next_content )
  {
    if ( !str_cmp(tempobj->text.name,"thermographic vision")
         || !str_cmp(tempobj->text.name,"low-light vision")
         || !str_cmp(tempobj->text.name,"flare compensation")
         || !str_cmp(tempobj->text.name, "optical magnification")
         || !str_cmp(tempobj->text.name,"electrical magnification") )
      natural = FALSE;
  }

  if ( natural )
  {
    switch (GET_RACE(ch)) {
    case RACE_ELF:
    case RACE_HOBGOBLIN:
    case RACE_ONI:
    case RACE_WAKYAMBI:
    case RACE_NIGHTONE:
    case RACE_DRYAD:
    case RACE_SATYR:
    case RACE_ORK:
      ch->char_specials.saved.affected_by.SetBit(AFF_LOW_LIGHT);
      break;

    case RACE_DRAGON:
    case RACE_TROLL:
    case RACE_KOBOROKURU:
    case RACE_FOMORI:
    case RACE_MENEHUNE:
    case RACE_GIANT:
    case RACE_GNOME:
    case RACE_MINOTAUR:
    case RACE_DWARF:
      ch->char_specials.saved.affected_by.SetBit(AFF_INFRAVISION);
      break;

    default:
      break;
    }
  }
  affect_total(ch);
  calc_weight(ch);
  return (obj);
}

int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr((const char *)*name, '.'))) {
    *ppos++ = '\0';
    strcpy(number, *name);
    strcpy(*name, ppos);

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
        return 0;

    return (atoi(number));
  }
  return 1;
}

struct veh_data *get_veh_list(char *name, struct veh_data *list)
{
  struct veh_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  if (!list)
    return NULL;
  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = list; i && (j <= number); i = i->next_veh)
    if (isname(tmp, i->name))
      if (++j == number)
        return i;

  return NULL;
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data * list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return i;

  return NULL;
}

int from_ip_zone(int vnum)
{
  int counter;
  if (vnum == -1)  // obj made using create_obj, like mail and corpses
    return 0;
  else if (vnum < 0 || vnum > (zone_table[top_of_zone_table].top))
    return 1;

  for (counter = 0; counter <= top_of_zone_table; counter++)
    if (!(zone_table[counter].connected) && vnum >= (zone_table[counter].number * 100) &&
        vnum <= zone_table[counter].top)
      return 1;

  return 0;
}

/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int room)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = world[room].people; i && (j <= number); i = i->next_in_room)
    if (isname(tmp, GET_KEYWORDS(i)) ||
        isname(tmp, GET_NAME(i)) || isname(tmp, GET_CHAR_NAME(i)))
      if (++j == number)
        return i;

  return NULL;
}

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(int nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return i;

  return NULL;
}

void obj_to_veh(struct obj_data * object, struct veh_data * veh)
{
  struct obj_data *i = NULL, *op = NULL;
  if (!object || !veh)
    log("SYSLOG: Illegal value(s) passed to veh_to_room");
  else
  {
    for (i = veh->contents; i; i = i->next_content) {
      if (i->item_number == object->item_number &&
          !strcmp(i->text.room_desc, object->text.room_desc) &&
          IS_INVIS(i) == IS_INVIS(object))
        break;

      op = i;
    }

    if (i) {
      object->next_content = i;
      if (op)
        op->next_content = object;
      else
        veh->contents = object;
    } else {
      object->next_content = veh->contents;
      veh->contents = object;
    }
    veh->load -= GET_OBJ_WEIGHT(object);
    object->in_veh = veh;
    object->in_room = NOWHERE;
    object->carried_by = NULL;
  }
}
/* put an object in a room */
void obj_to_room(struct obj_data * object, int room)
{
  struct obj_data *i = NULL, *op = NULL;

  if (!object || room < 0 || room > top_of_world)
    log("SYSLOG: Illegal value(s) passed to obj_to_room");
  else
  {
    for (i = world[room].contents; i; i = i->next_content) {
      if (i->item_number == object->item_number &&
          !strcmp(i->text.room_desc, object->text.room_desc) &&
          IS_INVIS(i) == IS_INVIS(object))
        break;

      op = i;
    }

    if (op == object) {
      log("SYSLOG: WTF? ^.^");
      return;
    }
    if (i) {
      object->next_content = i;
      if (op)
        op->next_content = object;
      else
        world[room].contents = object;
    } else {
      object->next_content = world[room].contents;
      world[room].contents = object;
    }

    object->in_room = room;
    object->carried_by = NULL;

    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      ROOM_FLAGS(room).SetBit(ROOM_HOUSE_CRASH);
  }
}


/* Take an object from a room */
void obj_from_room(struct obj_data * object)
{
  struct obj_data *temp;
  if (!object || (object->in_room == NOWHERE && !object->in_veh))
  {
    log("SYSLOG: NULL object or obj not in a room passed to obj_from_room");
    return;
  }
  if (object->in_veh)
  {
    object->in_veh->load += GET_OBJ_WEIGHT(object);
    REMOVE_FROM_LIST(object, object->in_veh->contents, next_content);
  } else
    REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

  if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
    ROOM_FLAGS(object->in_room).SetBit(ROOM_HOUSE_CRASH);
  object->in_veh = NULL;
  object->in_room = NOWHERE;
  object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data * obj, struct obj_data * obj_to)
{
  struct obj_data *tmp_obj;
  struct obj_data *i = NULL, *op = NULL;

  for (i = obj_to->contains; i; i = i->next_content)
  {
    if (i->item_number == obj->item_number &&
        !strcmp(i->text.room_desc, obj->text.room_desc) &&
        IS_INVIS(i) == IS_INVIS(obj))
      break;
    op=i;
  }

  if (i)
  {
    obj->next_content = i;
    if (op)
      op->next_content = obj;
    else
      obj_to->contains = obj;
  } else
  {
    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
  }

  obj->in_obj = obj_to;

  for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (GET_OBJ_TYPE(tmp_obj) != ITEM_CYBERDECK || GET_OBJ_TYPE(tmp_obj) != ITEM_DECK_ACCESSORY)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->worn_by)
    IS_CARRYING_W(tmp_obj->worn_by) += GET_OBJ_WEIGHT(obj);
  if (obj_to->in_room != NOWHERE && ROOM_FLAGGED(obj_to->in_room, ROOM_HOUSE))
    ROOM_FLAGS(obj_to->in_room).SetBit(ROOM_HOUSE_CRASH);

}


/* remove an object from an object */
void obj_from_obj(struct obj_data * obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL)
  {
    log("error (handler.c): trying to illegally extract obj from obj");
    return;
  }
  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);
  if (GET_OBJ_TYPE(obj_from) == ITEM_HOLSTER && GET_OBJ_VAL(obj_from, 3))
    GET_OBJ_VAL(obj_from, 3) = 0;
  /* Subtract weight from containers container */

  for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (GET_OBJ_TYPE(temp) != ITEM_CYBERDECK || GET_OBJ_TYPE(temp) != ITEM_DECK_ACCESSORY)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
  if (temp->worn_by)
    IS_CARRYING_W(temp->worn_by) -= GET_OBJ_WEIGHT(obj);

  if (obj->in_obj && obj->in_obj->in_room != NOWHERE && ROOM_FLAGGED(obj->in_obj->in_room, ROOM_HOUSE))
    ROOM_FLAGS(obj->in_obj->in_room).SetBit(ROOM_HOUSE_CRASH);
  obj->in_obj = NULL;
  obj->next_content = NULL;
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data * list, struct char_data * ch)
{
  if (list)
  {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}

void extract_icon(struct matrix_icon * icon)
{
  struct matrix_icon *temp;
  for (struct phone_data *k = phone_list; k; k = k->next)
    if (phone_list->persona == icon)
    {
      struct phone_data *temp;
      if (k->dest) {
        if (k->dest->persona)
          send_to_icon(k->dest->persona, "The call is terminated from the other end.\r\n");
        else if (k->dest->phone)
          if (k->dest->connected) {
            if (k->dest->phone->carried_by)
              send_to_char("The phone is hung up from the other side.\r\n", k->dest->phone->carried_by);
          } else {
            if (k->dest->phone->carried_by)
              send_to_char("Your phone stops ringing.\r\n", k->dest->phone->carried_by);
            else if (k->dest->phone->in_obj && k->dest->phone->in_obj->carried_by)
              send_to_char("Your phone stops ringing.\r\n", k->dest->phone->in_obj->carried_by);
            else {
              sprintf(buf, "%s stops ringing.\r\n", k->dest->phone->text.name);
              act(buf, FALSE, 0, k->dest->phone, 0, TO_ROOM);
            }
          }
        k->dest->connected = FALSE;
        k->dest->dest = NULL;
      }
      REMOVE_FROM_LIST(k, phone_list, next);
      delete k;
      break;
    }

  if (icon->in_host)
  {
    if (icon->fighting) {
      for (struct matrix_icon *vict = matrix[icon->in_host].icons; vict; vict = vict->next_in_host)
        if (vict->fighting == icon) {
          REMOVE_FROM_LIST(vict, matrix[icon->in_host].fighting, next_fighting);
          vict->fighting = NULL;
        }
      REMOVE_FROM_LIST(icon, matrix[icon->in_host].fighting, next_fighting);
    }
    icon_from_host(icon);
  }
  if (icon->decker)
  {
    if (icon->decker->hitcher) {
      PLR_FLAGS(icon->decker->hitcher).RemoveBit(PLR_MATRIX);
      send_to_char(icon->decker->hitcher, "You return to your senses.\r\n");
    }
    struct obj_data *temp;
    for (struct obj_data *obj = icon->decker->software; obj; obj = temp) {
      temp = obj->next_content;
      extract_obj(obj);
    }
    struct seen_data *temp2;
    for (struct seen_data *seen = icon->decker->seen; seen; seen = temp2) {
      temp2 = seen->next;
      delete seen;
    }
    delete icon->decker;
  } else
  {
    ic_index[icon->number].number--;
  }
  REMOVE_FROM_LIST(icon, icon_list, next);
  Mem->DeleteIcon(icon);
}

void extract_veh(struct veh_data * veh)
{
  struct veh_data *temp;
  REMOVE_FROM_LIST(veh, veh_list, next);
  if (veh->in_room)
    veh_from_room(veh);
  clear_queue(veh);
  veh_index[veh->veh_number].number--;
  Mem->DeleteVehicle(veh);
}

/* Extract an object from the world */
void extract_obj(struct obj_data * obj)
{
  struct phone_data *phone, *temp;
  bool set
    = FALSE;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      log("SYSLOG: Inconsistent worn_by and worn_on pointers!!");
  if (GET_OBJ_TYPE(obj) == ITEM_PHONE ||
      (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE && GET_OBJ_VAL(obj, 2) == 4))
    for (phone = phone_list; phone; phone = phone->next)
      if (phone->phone == obj)
      {
        if (phone->dest) {
          phone->dest->dest = NULL;
          phone->dest->connected = FALSE;
          if (phone->dest->persona)
            send_to_icon(phone->dest->persona, "The connection is closed from the other side.\r\n");
          else {
            if (phone->dest->phone->carried_by) {
              if (phone->dest->connected) {
                if (phone->dest->phone->carried_by)
                  send_to_char("The phone is hung up from the other side.\r\n", phone->dest->phone->carried_by);
              } else {
                if (phone->dest->phone->carried_by)
                  send_to_char("Your phone stops ringing.\r\n", phone->dest->phone->carried_by);
                else if (phone->dest->phone->in_obj && phone->dest->phone->in_obj->carried_by)
                  send_to_char("Your phone stops ringing.\r\n", phone->dest->phone->in_obj->carried_by);
                else {
                  sprintf(buf, "%s stops ringing.\r\n", phone->dest->phone->text.name);
                  act(buf, FALSE, 0, phone->dest->phone, 0, TO_ROOM);
                }
              }
            }

          }
        }
        REMOVE_FROM_LIST(phone, phone_list, next);
        delete phone;
        break;
      }
  if (obj->in_room != NOWHERE || obj->in_veh != NULL)
  {
    obj_from_room(obj);
    set
      = TRUE;
  }
  if (obj->carried_by)
  {
    obj_from_char(obj);
    if (set
       )
      log("SYSLOG: More than one list pointer set!");
    set
      = TRUE;
  }
  if (obj->in_obj)
  {
    obj_from_obj(obj);
    if (set
       )
      log("SYSLOG: More than one list pointer set!");
    set
      = TRUE;
  }

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

  if (!ObjList.Remove(obj))
    log("ObjList.Remove returned FALSE!  (%d)", GET_OBJ_VNUM(obj));

  if (GET_OBJ_RNUM(obj) >= 0)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;

  Mem->DeleteObject(obj);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(struct char_data * ch)
{
  struct char_data *k, *temp;
  struct descriptor_data *t_desc;
  struct obj_data *obj, *next;
  int i, wield[2];
  struct veh_data *veh;
  int in_room;

  extern struct char_data *combat_list;

  ACMD(do_return);

  void die_follower(struct char_data * ch);

  if (!IS_NPC(ch))
    playerDB.SaveChar(ch, GET_LOADROOM(ch));

  if (!IS_NPC(ch) && !ch->desc)
  {
    for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
      if (t_desc->original == ch)
        do_return(t_desc->character, "", 0, 0);
  }
  if (ch->followers || ch->master)
    die_follower(ch);

  /* Forget snooping, if applicable */
  if (ch->desc)
  {
    if (ch->desc->snooping) {
      ch->desc->snooping->snoop_by = NULL;
      ch->desc->snooping = NULL;
    }
    if (ch->desc->snoop_by) {
      SEND_TO_Q("Your victim is no longer among us.\r\n", ch->desc->snoop_by);
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }
  }

  /* transfer objects to room, if any */
  while (ch->carrying)
  {
    obj = ch->carrying;
    obj_from_char(obj);
    extract_obj(obj);
  }

  /* extract all cyberware from NPC's since it can't be reused */
  for (obj = ch->cyberware; obj; obj = next)
  {
    next = obj->next_content;
    obj_from_cyberware(obj);
    extract_obj(obj);
  }
  for (obj = ch->bioware; obj; obj = next)
  {
    next = obj->next_content;
    obj_from_bioware(obj);
    extract_obj(obj);
  }

  wield[0] = GET_WIELDED(ch, 0);
  wield[1] = GET_WIELDED(ch, 1);

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      extract_obj(unequip_char(ch, i));

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp)
  {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  while (ch->affected)
    affect_remove(ch, ch->affected, 0);

  in_room = ch->in_room;
  if (ch->in_room > NOWHERE || ch->in_veh)
    char_from_room(ch);

  if (ch->in_room > NOWHERE)
    for (veh = world[ch->in_room].vehicles; veh; veh = veh->next_veh)
      for (obj = veh->mount; obj; obj = obj->next_content)
        if (obj->targ == ch)
          obj->targ = NULL;

  if (ch->persona)
  {
    sprintf(buf, "%s depixelizes and vanishes from the host.\r\n", ch->persona->name);
    send_to_host(ch->persona->in_host, buf, ch->persona, TRUE);
    extract_icon(ch->persona);
    ch->persona = NULL;
    PLR_FLAGS(ch).RemoveBit(PLR_MATRIX);
  } else if (PLR_FLAGGED(ch, PLR_MATRIX))
    for (struct char_data *temp = world[ch->in_room].people; temp; temp = temp->next_in_room)
      if (PLR_FLAGGED(temp, PLR_MATRIX))
        temp->persona->decker->hitcher = NULL;
  /* pull the char from the list */
  REMOVE_FROM_LIST(ch, character_list, next);

  if (ch->desc && ch->desc->original)
    do_return(ch, "", 0, 0);

  if (!IS_NPC(ch))
  {
    GET_WIELDED(ch, 0) = wield[0];
    GET_WIELDED(ch, 1) = wield[1];
    PLR_FLAGS(ch).RemoveBits(PLR_MATRIX, PLR_PROJECT, PLR_SWITCHED,
                             PLR_WRITING, PLR_MAILING, PLR_EDITING,
                             PLR_SPELL_CREATE, PLR_PROJECT, PLR_CUSTOMIZE,
                             PLR_REMOTE, ENDBIT);
    ch->in_room = in_room;
    if (ch->desc) {
      if (STATE(ch->desc) == CON_QMENU)
        SEND_TO_Q(QMENU, ch->desc);
      else {
        STATE(ch->desc) = CON_MENU;
        SEND_TO_Q(MENU, ch->desc);
      }
    }
  } else
  {
    if (GET_MOB_RNUM(ch) > -1)          /* if mobile */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);            /* Only NPC's can have memory */
    Mem->DeleteCh(ch);
  }
}

/* ***********************************************************************
   Here follows high-level versions of some earlier routines, ie functions
   which incorporate the actual player-data.
   *********************************************************************** */

struct char_data *get_player_vis(struct char_data * ch, char *name, int inroom)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (!IS_NPC(i) && (!inroom || i->in_room == ch->in_room) &&
        (isname(name, GET_KEYWORDS(i)) || isname(name, GET_CHAR_NAME(i)) || recog(ch, i, name))
        && GET_LEVEL(ch) >= GET_INCOG_LEV(i))
      return i;

  return NULL;
}

struct char_data *get_char_veh(struct char_data * ch, char *name, struct veh_data * veh)
{
  struct char_data *i;

  for (i = veh->people; i; i = i->next_in_veh)
    if ((isname(name, GET_KEYWORDS(i)) || isname(name, GET_CHAR_NAME(i)) || recog(ch, i, name))
        && CAN_SEE(ch, i))
      return i;

  return NULL;
}

struct char_data *get_char_room_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* JE 7/18/94 :-) :-) */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return ch;

  /* 0.<name> means PC with name */
  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return get_player_vis(ch, tmp, 1);

  if (ch->in_veh)
    if ((i = get_char_veh(ch, name, ch->in_veh)))
      return i;

  for (i = world[ch->in_room].people; i && j <= number; i = i->next_in_room)
    if ((isname(tmp, GET_KEYWORDS(i)) ||
         isname(tmp, GET_NAME(i)) || recog(ch, i, name)) &&
        CAN_SEE(ch, i))
      if (++j == number)
        return i;

  return NULL;
}

struct char_data *get_char_vis(struct char_data * ch, char *name)
{
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* check the room first */
  if (ch->in_veh)
    if ((i = get_char_veh(ch, name, ch->in_veh)))
      return i;
  if ((i = get_char_room_vis(ch, name)) != NULL)
    return i;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return get_player_vis(ch, tmp, 0);

  for (i = character_list; i && (j <= number); i = i->next)
    if ((isname(tmp, GET_KEYWORDS(i)) || recog(ch, i, name)) &&
        CAN_SEE(ch, i))
      if (++j == number)
        return i;

  return NULL;
}

struct obj_data *get_obj_in_list_vis(struct char_data * ch, char *name, struct obj_data * list)
{
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = list; i && (j <= number); i = i->next_content)
  {
    if (isname(tmp, i->text.keywords) || isname(tmp, i->text.name) || (i->restring && isname(tmp, i->restring)))
      if (CAN_SEE_OBJ(ch, i))
        if (++j == number)
          return i;
  }

  return NULL;
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data * ch, char *name)
{
  struct obj_data *i;
  int number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, ch->carrying)))
    return i;

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
    return i;

  strcpy(tmp, name);
  if (!(number = get_number(&tmp)))
    return NULL;

  //  return find_obj(ch, tmp, number);
  return ObjList.FindObj(ch, tmp, number);
}

struct obj_data *get_object_in_equip_vis(struct char_data * ch,
          char *arg, struct obj_data * equipment[], int *j)
{
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;
  int i = 0, number;

  strcpy(tmp, arg);
  if (!(number = get_number(&tmp)))
    return NULL;

  for ((*j) = 0; (*j) < NUM_WEARS && i <= number; (*j)++)
    if (equipment[(*j)])
    {
      if (GET_OBJ_TYPE(equipment[(*j)]) == ITEM_WORN)
        if (equipment[(*j)]->contains)
          for (struct obj_data *obj = equipment[(*j)]->contains; obj; obj = obj->next_content)
            if (isname(tmp, obj->text.keywords) || isname(tmp, obj->text.name) ||
                (obj->restring && isname(tmp, obj->restring)))
              if (++i == number)
                return (obj);
      if (isname(tmp, equipment[(*j)]->text.keywords) || isname(tmp, equipment[(*j)]->text.name)  ||
          (equipment[(*j)]->restring && isname(tmp, equipment[(*j)]->restring)))
        if (++i == number)
          return (equipment[(*j)]);
    }

  return NULL;
}

int belongs_to(struct char_data *ch, struct obj_data *obj)
{
  if (GET_OBJ_TYPE(obj) == ITEM_MONEY && GET_OBJ_VAL(obj, 1) == 1 &&
      (IS_NPC(ch) ? GET_OBJ_VAL(obj, 3) == 0 : GET_OBJ_VAL(obj, 3) == 1) &&
      GET_OBJ_VAL(obj, 4) == (IS_NPC(ch) ? ch->nr : GET_IDNUM(ch)))
    return 1;
  else
    return 0;
}

struct obj_data *get_first_credstick(struct char_data *ch, char *arg)
{
  struct obj_data *obj;
  int i;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (belongs_to(ch, obj) && isname(arg, obj->text.keywords))
      return obj;

  for (i = 0; i < NUM_WEARS - 1; i++)
    if (GET_EQ(ch, i) && belongs_to(ch, GET_EQ(ch, i)) &&
        isname(arg, GET_EQ(ch, i)->text.keywords))
      return GET_EQ(ch, i);

  return NULL;
}

struct obj_data *create_credstick(struct char_data *ch, int amount)
{
  struct obj_data *obj;
  int num;

  if (amount <= 0)
  {
    log("SYSLOG: Try to create negative or 0 nuyen.");
    return NULL;
  } else if (!IS_NPC(ch))
  {
    log("SYSLOG: Creating a credstick for a PC corpse.");
    return NULL;
  }

  if (amount < 2500)
    num = 100;     // plastic credstick
  else if (amount < 10000)
    num = 101;     // steel credstick
  else if (amount < 50000)
    num = 102;     // silver credstick
  else if (amount < 150000)
    num = 103;     // titanium credstick
  else if (amount < 500000)
    num = 104;     // platinum credstick
  else
    num = 105;  // emerald credstick

  obj = read_object(num, VIRTUAL);

  GET_OBJ_VAL(obj, 0) = amount;
  GET_OBJ_VAL(obj, 3) = 0;
  GET_OBJ_VAL(obj, 4) = ch->nr;
  if (num < 102)
    GET_OBJ_VAL(obj, 5) = (number(1, 9) * 100000) + (number(0, 9) * 10000) +
                          (number(0, 9) * 1000) + (number(0, 9) * 100) +
                          (number(0, 9) * 10) + number(0, 9);

  return obj;
}

struct obj_data *create_nuyen(int amount)
{
  struct obj_data *obj;

  if (amount <= 0)
  {
    log("SYSLOG: Try to create negative or 0 nuyen.");
    return NULL;
  }
  obj = read_object(110, VIRTUAL);

  GET_OBJ_VAL(obj, 0) = amount;

  return obj;
}

int find_skill_num(char *name)
{
  int index = 0, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  while (++index < MAX_SKILLS) {
    if (is_abbrev(name, skills[index].name))
      return index;

    ok = 1;
    temp = any_one_arg((char *)skills[index].name, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = 0;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }

    if (ok && !*first2)
      return index;
  }

  return -1;
}

/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(char *arg, int bitvector, struct char_data * ch,
                 struct char_data ** tar_ch, struct obj_data ** tar_obj)
{
  int i;
  char name[256];

  one_argument(arg, name);

  if (!*name)
    return (0);

  *tar_ch = NULL;
  *tar_obj = NULL;

  if (IS_SET(bitvector, FIND_CHAR_ROOM))
  {      /* Find person in room */
    if (ch->in_veh) {
      if ((*tar_ch = get_char_veh(ch, name, ch->in_veh)))
        return (FIND_CHAR_ROOM);
      else {
        int x = ch->in_room;
        ch->in_room = ch->in_veh->in_room;
        if ((*tar_ch = get_char_room_vis(ch, name))) {
          ch->in_room = x;
          return (FIND_CHAR_ROOM);
        }
        ch->in_room = x;
      }
    } else {
      if ((*tar_ch = get_char_room_vis(ch, name)))
        return (FIND_CHAR_ROOM);
    }
  }
  if (IS_SET(bitvector, FIND_CHAR_WORLD))
  {
    if ((*tar_ch = get_char_vis(ch, name))) {
      return (FIND_CHAR_WORLD);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP))
  {
    if ((*tar_obj = get_object_in_equip_vis(ch, name, ch->equipment, &i)))
      return (FIND_OBJ_EQUIP);
  }
  if (IS_SET(bitvector, FIND_OBJ_INV))
  {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
      return (FIND_OBJ_INV);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_ROOM))
  {
    if (ch->in_veh) {
      if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_veh->in_room].contents)))
        return (FIND_OBJ_ROOM);
    } else if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
      return (FIND_OBJ_ROOM);
  }
  if (IS_SET(bitvector, FIND_OBJ_WORLD))
  {
    if ((*tar_obj = get_obj_vis(ch, name))) {
      return (FIND_OBJ_WORLD);
    }
  }
  return (0);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return FIND_ALL;
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(buf, arg + 4);
    strcpy(arg, buf);
    return FIND_ALLDOT;
  } else
    return FIND_INDIV;
}

int veh_skill(struct char_data *ch, struct veh_data *veh)
{
  int skill = 0;

  switch (veh->type)
  {
  case VEH_CAR:
    skill = GET_SKILL(ch, SKILL_PILOT_CAR);
    if (!skill)
      skill = (int)(GET_SKILL(ch, SKILL_PILOT_TRUCK) / 2);
    if (!skill)
      skill = (int)(GET_SKILL(ch, SKILL_PILOT_CAR) / 2);
    break;
  case VEH_BIKE:
    skill = GET_SKILL(ch, SKILL_PILOT_BIKE);
    if (!skill)
      skill = (int)(GET_SKILL(ch, SKILL_PILOT_CAR) / 2);
    if (!skill)
      skill = (int)(GET_SKILL(ch, SKILL_PILOT_TRUCK) / 2);
    break;
  case VEH_TRUCK:
    skill = GET_SKILL(ch, SKILL_PILOT_TRUCK);
    if (!skill)
      skill = (int)(GET_SKILL(ch, SKILL_PILOT_CAR) / 2);
    if (!skill)
      skill = (int)(GET_SKILL(ch, SKILL_PILOT_BIKE) / 2);
    break;
  }
  if (AFF_FLAGGED(ch, AFF_RIG) || PLR_FLAGGED(ch, PLR_REMOTE))
    skill += GET_CONTROL(ch);

  return skill;
}


void clear_queue(struct veh_data *veh)
{
  struct char_data *ch, *next;
  struct obj_data *obj;
  for (ch = veh->fighting; ch; ch = next)
  {
    next = ch->next_fighting;
    stop_fighting(ch);
  }
  for (obj = veh->mount; obj; obj = obj->next_content)
  {
    obj->targ = NULL;
    obj->tveh = NULL;
  }
}

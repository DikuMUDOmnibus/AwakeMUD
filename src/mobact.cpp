/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"

/* external structs */
extern struct spell_info_type spell_info[];
extern void resist_drain(struct char_data *ch, int power, int drain_add, int wound);
extern int find_sight(struct char_data *ch);
extern int find_weapon_range(struct char_data *ch, struct obj_data *weapon);

extern void perform_wear(struct char_data *, struct obj_data *, int);
extern void perform_remove(struct char_data *, int);

extern bool is_escortee(struct char_data *mob);
extern bool hunting_escortee(struct char_data *ch, struct char_data *vict);

bool memory(struct char_data *ch, struct char_data *vict);
int violates_zsp(int security, struct char_data *ch, int pos, struct char_data *mob);
bool attempt_reload(struct char_data *mob, int pos);

#define MOB_AGGR_TO_RACE (MOB_AGGR_ELF | MOB_AGGR_DWARF | MOB_AGGR_HUMAN | MOB_AGGR_ORK | MOB_AGGR_TROLL | MOB_AGGR_DRAGON)

void mobile_activity(void)
{
  struct char_data *ch, *next_ch, *vict;
  struct veh_data *veh;
  struct obj_data *obj, *best_obj;
  int door, max, i, dir, distance, room, nextroom, has_acted;

  extern int no_specials;

  ACMD(do_get);

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!IS_MOB(ch) || FIGHTING(ch) || !AWAKE(ch) || FIGHTING_VEH(ch) || ch->desc)
      continue;
    has_acted = 0;

    /* Examine call for special procedure */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
        log("%s (#%d): Attempting to call non-existing mob func",
            GET_NAME(ch), GET_MOB_VNUM(ch));

        MOB_FLAGS(ch).RemoveBit(MOB_SPEC);
      } else if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
        continue;
      else if (mob_index[GET_MOB_RNUM(ch)].sfunc != NULL)
        if ((mob_index[GET_MOB_RNUM(ch)].sfunc) (ch, ch, 0, ""))
          continue;
    }

    /* Scavenger (picking up objects) */
    if (!has_acted && MOB_FLAGGED(ch, MOB_SCAVENGER))
      if (world[ch->in_room].contents && !number(0, 10)) {
        max = 1;
        best_obj = NULL;
        for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
          if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
            best_obj = obj;
            max = GET_OBJ_COST(obj);
          }
        if (best_obj != NULL) {
          obj_from_room(best_obj);
          obj_to_char(best_obj, ch);
          act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
          has_acted = TRUE;
        }
      }

    /* Mob Movement */
    if (!has_acted && !MOB_FLAGGED(ch, MOB_SENTINEL) &&
        GET_POS(ch) == POS_STANDING && ch->in_room != NOWHERE &&
        (door = number(0, 18)) < NUM_OF_DIRS && CAN_GO(ch, door) &&
        !ROOM_FLAGS(EXIT(ch, door)->to_room).AreAnySet(ROOM_NOMOB,
            ROOM_DEATH, ENDBIT) &&
        (!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
         (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone))) {
      perform_move(ch, door, CHECK_SPECIAL | LEADER, NULL);
      has_acted = TRUE;
    }
    /* Mob Driving */
    if (!has_acted && ch->in_veh && AFF_FLAGGED(ch, AFF_PILOT)
        && (door = number(0, 18)) < NUM_OF_DIRS && EXIT(ch->in_veh, door) &&
        (ROOM_FLAGGED(EXIT(ch->in_veh, door)->to_room, ROOM_ROAD) ||
         ROOM_FLAGGED(EXIT(ch->in_veh, door)->to_room, ROOM_GARAGE))) {
      perform_move(ch, door, LEADER, NULL);
      has_acted = TRUE;
    }
    /* Aggressive Mobs */

    if (!ch->in_veh)
      if (!has_acted &&
          MOB_FLAGS(ch).AreAnySet(MOB_AGGRESSIVE, MOB_AGGR_TO_RACE, ENDBIT)) {
        bool vehicle = number(0, 1);
        if (vehicle) {
          for (veh = world[ch->in_room].vehicles; veh; veh = veh->next_veh) {
            if (veh->damage < 10) {
              set_fighting(ch, veh);
              has_acted = TRUE;
              break;
            }
          }
        } else if (!vehicle || !has_acted) {
          for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
            if ((IS_NPC(vict) && !IS_PROJECT(vict) && !is_escortee(vict)) ||
                !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) || GET_PHYSICAL(vict) < 0)
              continue;
            if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
              continue;
            if (hunting_escortee(ch, vict)) {
              /* Roots Aggressiveness based on Charisma */
              if (number(0, 20) <= GET_CHA(vict)) {
                act("$n nods to $N.",
                    FALSE, ch, 0, vict, TO_NOTVICT);
                act("$N nods to you.",
                    FALSE, vict, 0, ch, TO_CHAR);
              } else
                /* End part 1, look below for more */
              {
                set_fighting(ch, vict);
                has_acted = TRUE;
                break;
              }
            } else if (!MOB_FLAGGED(ch, MOB_AGGR_TO_RACE) ||
                       (MOB_FLAGGED(ch, MOB_AGGR_ELF) &&
                        GET_RACE(vict) == RACE_ELF || RACE_WAKYAMBI || RACE_NIGHTONE || RACE_DRYAD) ||
                       (MOB_FLAGGED(ch, MOB_AGGR_DWARF) &&
                        GET_RACE(vict) == RACE_DWARF || RACE_GNOME || RACE_MENEHUNE || RACE_KOBOROKURU) ||
                       (MOB_FLAGGED(ch, MOB_AGGR_HUMAN) &&
                        GET_RACE(vict) == RACE_HUMAN) ||
                       (MOB_FLAGGED(ch, MOB_AGGR_ORK) &&
                        GET_RACE(vict) == RACE_ORK || RACE_HOBGOBLIN || RACE_OGRE || RACE_SATYR || RACE_ONI) ||
                       (MOB_FLAGGED(ch, MOB_AGGR_TROLL) &&
                        GET_RACE(vict) == RACE_TROLL || RACE_CYCLOPS || RACE_FOMORI || RACE_GIANT || RACE_MINOTAUR) ||
                       (MOB_FLAGGED(ch, MOB_AGGR_DRAGON) &&
                        GET_RACE(vict) == RACE_DRAGON)) {
              /* More more more lalalala */
              if (number(0, 20) <= GET_CHA(vict)) {
                act("$n nods to $N.",
                    FALSE, ch, 0, vict, TO_NOTVICT);
                act("$N nods to you.",
                    FALSE, vict, 0, ch, TO_CHAR);
              } else
                /* End both */
              {
                set_fighting(ch, vict);
                has_acted = TRUE;
                break;
              }
            }
          }
        }
      }

    /* Mob Memory */
    if (!has_acted && MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
        if (IS_NPC(vict) ||
            !CAN_SEE(ch, vict) ||
            PRF_FLAGGED(vict, PRF_NOHASSLE))
          continue;
        if (memory(ch, vict)) {
          act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.",
              FALSE, ch, 0, 0, TO_ROOM);
          set_fighting(ch, vict);
          has_acted = TRUE;
          break;
        }
      }
    }

    /* Helper Mobs */
    if (!has_acted && MOB_FLAGGED(ch, MOB_HELPER)) {
      for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
        if (ch != vict &&
            IS_NPC(vict) &&
            CAN_SEE(ch, vict) &&
            FIGHTING(vict) &&
            !IS_NPC(FIGHTING(vict)) &&
            ch != FIGHTING(vict) &&
            (ch->in_room == FIGHTING(vict)->in_room)) {
          act("$n jumps to the aid of $N!",
              FALSE, ch, 0, vict, TO_ROOM);
          set_fighting(ch, FIGHTING(vict));
          has_acted = TRUE;
          break;
        }
    }

    if (!ch->in_veh)
      if (!has_acted && MOB_FLAGGED(ch, MOB_GUARD)) {
        for (veh = world[ch->in_room].vehicles; veh; veh = veh->next_veh)
          if (veh->type == VEH_DRONE) {
            set_fighting(ch, veh);
            has_acted = TRUE;
            break;
          }
        for (vict = world[ch->in_room].people;
             vict && !has_acted;
             vict = vict->next_in_room)
          if (!IS_NPC(vict) &&
              !PRF_FLAGGED(vict, PRF_NOHASSLE) &&
              CAN_SEE(ch, vict) && GET_PHYSICAL(vict) > 0)
            for (i = 0; i < NUM_WEARS; i++)
              if (GET_EQ(vict, i) &&
                  !FIGHTING(ch) &&
                  violates_zsp(zone_table[world[ch->in_room].zone].security,
                               vict, i, ch)) {
                set_fighting(ch, vict);
                has_acted = TRUE;
                break;
              }
      }
    if (!has_acted &&
        MOB_FLAGGED(ch, MOB_SNIPER) &&
        GET_EQ(ch, WEAR_WIELD) &&
        MOB_FLAGS(ch).AreAnySet(MOB_GUARD, MOB_AGGRESSIVE,
                                MOB_AGGR_TO_RACE, MOB_MEMORY, ENDBIT)) {
      for (dir = 0; !FIGHTING(ch) && dir < NUM_OF_DIRS; dir++) {
        room = ch->in_room;

        if (CAN_GO2(room, dir) &&
            world[EXIT2(room, dir)->to_room].zone == world[ch->in_room].zone)
          nextroom = EXIT2(room, dir)->to_room;
        else
          nextroom = NOWHERE;

        for (distance = 1;
             nextroom != NOWHERE &&
             distance <= find_sight(ch) &&
             distance <= find_weapon_range(ch, GET_EQ(ch, WEAR_WIELD));
             distance++) {
          for (vict = world[nextroom].people;
               vict;
               vict = vict->next_in_room) {
            if (!IS_NPC(vict) &&
                !PRF_FLAGGED(vict, PRF_NOHASSLE) &&
                CAN_SEE(ch, vict)) {
              if (MOB_FLAGGED(ch, MOB_GUARD))
                for (i = 0; i < NUM_WEARS; i++)
                  if (GET_EQ(vict, i) &&
                      violates_zsp(zone_table[world[ch->in_room].zone].security, vict, i, ch)) {
                    set_fighting(ch, vict);
                    has_acted = TRUE;
                    break;
                  }
              if (!has_acted &&
                  MOB_FLAGS(ch).AreAnySet(MOB_AGGRESSIVE,
                                          MOB_AGGR_TO_RACE, ENDBIT))
                if (!(MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict)) &&
                    (!MOB_FLAGGED(ch, MOB_AGGR_TO_RACE) ||
                     (MOB_FLAGGED(ch, MOB_AGGR_ELF) &&
                      GET_RACE(vict) == RACE_ELF || RACE_DRYAD || RACE_WAKYAMBI || RACE_NIGHTONE) ||
                     (MOB_FLAGGED(ch, MOB_AGGR_DWARF) &&
                      GET_RACE(vict) == RACE_DWARF || RACE_MENEHUNE || RACE_KOBOROKURU || RACE_GNOME) ||
                     (MOB_FLAGGED(ch, MOB_AGGR_HUMAN) &&
                      GET_RACE(vict) == RACE_HUMAN) ||
                     (MOB_FLAGGED(ch, MOB_AGGR_ORK) &&
                      GET_RACE(vict) == RACE_ORK || RACE_HOBGOBLIN || RACE_SATYR || RACE_ONI || RACE_OGRE) ||
                     (MOB_FLAGGED(ch, MOB_AGGR_DRAGON) &&
                      GET_RACE(vict) == RACE_DRAGON) ||
                     (MOB_FLAGGED(ch, MOB_AGGR_TROLL) &&
                      GET_RACE(vict) == RACE_TROLL || RACE_FOMORI || RACE_GIANT || RACE_MINOTAUR || RACE_CYCLOPS))) {
                  set_fighting(ch, vict);
                  has_acted = TRUE;
                }
              if (!has_acted &&
                  MOB_FLAGGED(ch, MOB_MEMORY) &&
                  MEMORY(ch) &&
                  memory(ch, vict)) {
                act("'Hey!  You're the fiend that attacked me!!!', "
                    "exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
                act("'Hey!  You're the fiend that attacked me!!!', "
                    "exclaims $N.", FALSE, vict, 0, ch, TO_ROOM);
                set_fighting(ch, vict);
                has_acted = TRUE;
              }
            }
            if (has_acted)
              break;
          }
          room = nextroom;
          if (CAN_GO2(room, dir) &&
              world[EXIT2(room, dir)->to_room].zone == world[ch->in_room].zone)
            nextroom = EXIT2(room, dir)->to_room;
          else
            nextroom = NOWHERE;
          if (has_acted)
            break;
        }
        if (has_acted)
          break;
      }
    }
    if (GET_EQ(ch, WEAR_WIELD) && IS_GUN(GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3)) && !GET_EQ(ch, WEAR_WIELD)->contains)
      attempt_reload(ch, WEAR_WIELD);

    /* Add new mobile actions here */

  }                             /* end for() */
}

int violates_zsp(int security, struct char_data *ch, int pos, struct char_data *mob)
{
  struct obj_data *obj = GET_EQ(ch, pos);
  int i = 15;

  if (!obj ||
      !(GET_OBJ_TYPE(obj) == ITEM_WORN ||
        ((pos == WEAR_WIELD || pos == WEAR_HOLD) &&
         (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON ||
          GET_OBJ_TYPE(obj) == ITEM_WEAPON))))
    return 0;

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_WORN:
    switch (pos) {
    case WEAR_UNDER:
      if (GET_EQ(ch, WEAR_ABOUT) || GET_EQ(ch, WEAR_BODY))
        if (success_test(GET_INT(mob), 6 +
                         (GET_EQ(ch, WEAR_ABOUT) ? GET_OBJ_VAL(GET_EQ(ch, WEAR_ABOUT), 7) : 0) +
                         (GET_EQ(ch, WEAR_BODY) ? GET_OBJ_VAL(GET_EQ(ch, WEAR_BODY), 7) : 0)) < 2)
          return 0;
      break;
    case WEAR_LARM:
    case WEAR_RARM:
    case WEAR_WAIST:
    case WEAR_BODY:
      if (GET_EQ(ch, WEAR_ABOUT))
        if (success_test(GET_INT(mob), 4 + GET_OBJ_VAL(GET_EQ(ch, WEAR_ABOUT), 7)) < 2)
          return 0;
      break;
    case WEAR_LEGS:
      if (GET_EQ(ch, WEAR_ABOUT))
        if (success_test(GET_INT(mob), 2 + GET_OBJ_VAL(GET_EQ(ch, WEAR_ABOUT), 7)) < 2)
          return 0;
      break;
    case WEAR_LANKLE:
    case WEAR_RANKLE:
      if (GET_EQ(ch, WEAR_ABOUT) || GET_EQ(ch, WEAR_LEGS))
        if (success_test(GET_INT(mob), 5 +
                         (GET_EQ(ch, WEAR_ABOUT) ? GET_OBJ_VAL(GET_EQ(ch, WEAR_ABOUT), 7) : 0) +
                         (GET_EQ(ch, WEAR_LEGS) ? GET_OBJ_VAL(GET_EQ(ch, WEAR_LEGS), 7) : 0)) < 2)
          return 0;
      break;
    }
    i = (int)((GET_OBJ_VAL(obj, 5) + GET_OBJ_VAL(obj, 6)) * .75);
    if (i < 5)
      return 0;
    break;
  case ITEM_HOLSTER:
    if (GET_OBJ_VAL(obj, 0) != 2 || !obj->contains)
      return 0;
    obj = obj->contains;
  case ITEM_WEAPON:
    if (GET_OBJ_VAL(obj, 3) < TYPE_HAND_GRENADE)
      i = (GET_OBJ_VAL(obj, 1) * 2) + GET_OBJ_VAL(obj, 2);
    else if (GET_OBJ_VAL(obj, 3) == TYPE_HAND_GRENADE)
      i = (int)(GET_OBJ_VAL(obj, 0) / 4) + GET_OBJ_VAL(obj, 1);
    else if (GET_OBJ_VAL(obj, 3) < TYPE_PISTOL)
      i = (GET_OBJ_VAL(obj, 3) - TYPE_HAND_GRENADE) + (int)(GET_OBJ_VAL(obj, 0) / 2) +
          GET_OBJ_VAL(obj, 1);
    else
      i = (int)(GET_OBJ_VAL(obj, 0) / 2) + GET_OBJ_VAL(obj, 1);
    break;
  case ITEM_FIREWEAPON:
    i = (int)((GET_OBJ_VAL(obj, 6) + GET_OBJ_VAL(obj, 2)) / 2) + GET_OBJ_VAL(obj, 1);
    break;
  }

  i = 15 - MIN(15, MAX(0, i));

  if (security >= i)
    return 1;
  else
    return 0;
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data * ch, struct char_data * victim)
{
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present)
  {
    tmp = new memory_rec;
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data * ch, struct char_data * victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim))
  {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;                     /* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  delete curr;
}

/* check ch's memory for victim */
bool memory(struct char_data *ch, struct char_data *vict)
{
  memory_rec *names;

  for (names = MEMORY(ch); names; names = names->next)
    if (names->id == GET_IDNUM(vict))
      return(TRUE);

  return(FALSE);
}

/* erase ch's memory */
void clearMemory(struct char_data * ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr)
  {
    next = curr->next;
    delete curr;
    curr = next;
  }

  MEMORY(ch) = NULL;
}

bool mob_magic(struct char_data * mob)
{
  struct char_data *vict;
  int num;

  if (GET_POS(mob) != POS_FIGHTING || GET_MOB_SPEC(mob) || IS_PROJECT(mob))
    return FALSE;

  /* return so the mob won't keep casting if mental drops below 3 */
  if (GET_MENTAL(mob) < 400)
    return FALSE;

  /* make sure the mob HAS sorcery skill and a Magic attribute */
  if ((GET_SKILL(mob, SKILL_SORCERY) < 1) || (GET_MAG(mob) < 100))
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[mob->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == mob && !number(0, 3))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(mob);

  /* cast spells about 33% of the time */
  if (number(0, 2))
    return FALSE;

  num = (int)(GET_MAG(mob) / 100) + GET_SKILL(mob, SKILL_SORCERY) +
        number(0, MIN(25, GET_TKE(vict)));

  if (mob->in_room == vict->in_room)
    switch (num)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
      mob_cast(mob, vict, NULL, SPELL_STUN_TOUCH, 0);
      break;
    case 13:
      mob_cast(mob, vict, NULL, SPELL_STUN_MISSILE, 0);
      break;
    case 14:
      mob_cast(mob, vict, NULL, SPELL_STUN_BOLT, 0);
      break;
    case 15:
    case 16:
    case 17:
      mob_cast(mob, vict, NULL, SPELL_STUN_CLOUD, 0);
      break;
    case 18:
    case 19:
      mob_cast(mob, vict, NULL, SPELL_POWER_DART, 0);
      break;
    case 20:
    case 21:
      mob_cast(mob, vict, NULL, SPELL_MANA_DART, 0);
      break;
    case 22:
    case 23:
      mob_cast(mob, vict, NULL, SPELL_POWER_MISSILE, 0);
      break;
    case 24:
    case 25:
      mob_cast(mob, vict, NULL, SPELL_MANA_MISSILE, 0);
      break;
    case 26:
    case 27:
    case 28:
      mob_cast(mob, vict, NULL, SPELL_POWER_CLOUD, 0);
      break;
    case 29:
    case 30:
    case 31:
      mob_cast(mob, vict, NULL, SPELL_MANA_CLOUD, 0);
      break;
    case 32:
    case 33:
    case 34:
      mob_cast(mob, vict, NULL, SPELL_POWER_DART, 0);
      break;
    case 35:
    case 36:
    case 37:
      mob_cast(mob, vict, NULL, SPELL_MANA_DART, 0);
      break;
    case 38:
    case 39:
    case 40:
      mob_cast(mob, vict, NULL, SPELL_POWERBALL, 0);
      break;
    case 41:
    case 42:
    case 43:
      mob_cast(mob, vict, NULL, SPELL_MANABALL, 0);
      break;
    case 44:
    case 45:
    case 46:
      mob_cast(mob, vict, NULL, SPELL_POWERBLAST, 0);
      break;
    case 47:
    case 48:
    case 49:
      mob_cast(mob, vict, NULL, SPELL_MANABLAST, 0);
      break;
    default:
      mob_cast(mob, vict, NULL, SPELL_HELLBLAST, 0);
      break;
    }
  else
    switch(number(1,7))
    {
    case 1:
      mob_cast(mob, vict, NULL, SPELL_MANA_DART, 0);
      break;
    case 2:
      mob_cast(mob, vict, NULL, SPELL_MANA_MISSILE, 0);
      break;
    case 3:
      mob_cast(mob, vict, NULL, SPELL_POWER_DART, 0);
      break;
    case 4:
      mob_cast(mob, vict, NULL, SPELL_POWER_MISSILE, 0);
      break;
    case 5:
      mob_cast(mob, vict, NULL, SPELL_STUN_BOLT, 0);
      break;
    case 6:
      mob_cast(mob, vict, NULL, SPELL_STUN_MISSILE, 0);
      break;
    default:
      break;
    }
  return TRUE;
}

bool attempt_reload(struct char_data *mob, int pos)
{
  // I would call the reload routine for players, but this is slightly faster
  struct obj_data *clip, *gun = NULL;
  bool found = FALSE;

  if (!(gun = GET_EQ(mob, pos)) || !GET_WIELDED(mob, pos - WEAR_WIELD))
    return FALSE;

  for (clip = mob->carrying; clip; clip = clip->next_content)
  {
    if (GET_OBJ_TYPE(clip) == ITEM_GUN_CLIP &&
        GET_OBJ_VAL(clip, 0) == GET_OBJ_VAL(gun, 5) &&
        GET_OBJ_VAL(clip, 1) == (GET_OBJ_VAL(gun, 3) - TYPE_HIT)) {
      found = TRUE;
      break;
    }
  }

  if (!found)
    return FALSE;

  if (gun->contains)
  {
    struct obj_data *tempobj = gun->contains;
    obj_from_obj(tempobj);
    obj_to_room(tempobj, mob->in_room);
  }
  obj_from_char(clip);
  obj_to_obj(clip, gun);
  GET_OBJ_VAL(gun, 6) = GET_OBJ_VAL(gun, 5) + 1;

  act("$n reloads $p.", TRUE, mob, gun, 0, TO_ROOM);
  act("You reload $p.", FALSE, mob, gun, 0, TO_CHAR);
  return TRUE;
}

void switch_weapons(struct char_data *mob, int pos)
{
  struct obj_data *i, *temp = NULL, *temp2 = NULL;

  if (!GET_EQ(mob, pos) || !GET_WIELDED(mob, pos - WEAR_WIELD))
    return;

  for (i = mob->carrying; i && !temp; i = i->next_content)
  {
    // search for a new gun
    if (GET_OBJ_TYPE(i) == ITEM_WEAPON)
      if (GET_OBJ_VAL(i, 6) > 0)
        temp = i;
    if (GET_OBJ_TYPE(i) == ITEM_WEAPON)
      if (GET_OBJ_VAL(i, 5) == -1)
        temp2 = i;
  }

  perform_remove(mob, pos);

  if (temp)
    perform_wear(mob, temp, pos);
  else if (temp2)
    perform_wear(mob, temp2, pos);
}


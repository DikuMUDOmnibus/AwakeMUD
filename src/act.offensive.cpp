/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "newmagic.h"
#include "awake.h"
#include "constants.h"

/* extern variables */
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct descriptor_data *descriptor_list;
extern const char *spirit_powers[];
extern int convert_look[];

/* extern functions */
void raw_kill(struct char_data * ch);
extern void set_attacking(struct char_data *, struct char_data *, const char *, int);
extern void range_combat(struct char_data *ch, char target[MAX_INPUT_LENGTH],
                           struct obj_data *weapon, int range, int dir);
extern int find_weapon_range(struct char_data *ch, struct obj_data *weapon);
extern void roll_individual_initiative(struct char_data *ch);
extern bool has_ammo(struct char_data *ch, int pos);
extern void damage_door(struct char_data *ch, int room, int dir, int power, int type);
extern void perform_wear(struct char_data *, struct obj_data *, int);
extern void perform_get_from_container(struct char_data *, struct obj_data *, struct obj_data *, int);
extern int can_wield_both(struct char_data *, struct obj_data *, struct obj_data *);
extern void draw_weapon(struct char_data *);

ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = get_char_room_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else {
    for (opponent = world[ch->in_room].people; opponent && (FIGHTING(opponent) != helpee);
         opponent = opponent->next_in_room)
      ;

    if (!opponent)
      opponent = FIGHTING(helpee);
    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", FALSE, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      // here we add the chars to the respective lists
      set_fighting(ch, opponent);
      if (!FIGHTING(opponent))
        set_fighting(opponent, ch);
    }
  }
}

#define IS_EXPLOSIVE(ch, pos) \
        (GET_EQ(ch, pos) && GET_WIELDED(ch, pos - WEAR_WIELD) && \
        GET_OBJ_TYPE(GET_EQ(ch, pos)) == ITEM_WEAPON && \
        (GET_OBJ_VAL(GET_EQ(ch, pos), 3) == TYPE_ROCKET || \
        GET_OBJ_VAL(GET_EQ(ch, pos), 3) == TYPE_GRENADE_LAUNCHER || \
        GET_OBJ_VAL(GET_EQ(ch, pos), 3) == TYPE_HAND_GRENADE))

int messageless_find_door(struct char_data *ch, char *type, char *dir, char *cmdname)
{
  int door;

  if (*dir)
  {
    if ((door = search_block(dir, lookdirs, FALSE)) == -1)
      return -1;

    door = convert_look[door];
    if (EXIT(ch, door)) {
      if (EXIT(ch, door)->keyword) {
        if (isname(type, EXIT(ch, door)->keyword) &&
            !IS_SET(EXIT(ch, door)->exit_info, EX_DESTROYED))
          return door;
        else
          return -1;
      } else
        return door;
    } else
      return -1;
  } else
  {                      /* try to locate the keyword */
    if (!*type)
      return -1;

    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
        if (EXIT(ch, door)->keyword)
          if (isname(type, EXIT(ch, door)->keyword) &&
              !IS_SET(EXIT(ch, door)->exit_info, EX_DESTROYED))
            return door;

    return -1;
  }
}

bool perform_hit(struct char_data *ch, char *argument, char *cmdname)
{
  //  extern struct index_data *mob_index;
  struct char_data *vict = NULL;
  struct obj_data *wielded = NULL;
  struct veh_data *veh = NULL;
  int dir, type = DAMOBJ_CRUSH;

  if (world[ch->in_room].peaceful)
  {
    send_to_char("This room just has a peaceful, easy feeling...\r\n", ch);
    return TRUE;
  }
  two_arguments(argument, arg, buf2);

  if (ch->in_veh)
  {
    send_to_char(ch, "You can only drive by or use mounts inside a vehicle.\r\n");
    return TRUE;
  }
  if (!*arg)
  {
    sprintf(buf, "%s what?\r\n", cmdname);
    send_to_char(ch, CAP(buf));
    return TRUE;
  } else if (!(vict = get_char_room_vis(ch, arg)) && !(veh = get_veh_list(arg, world[ch->in_room].vehicles)))
  {
    if ((dir = messageless_find_door(ch, arg, buf2, cmdname)) < 0)
      return FALSE;
    if (!str_cmp(cmdname, "kill") || !str_cmp(cmdname, "murder")) {
      send_to_char(ch, "How would you go about %sing an object?\r\n",
                   cmdname);
      return TRUE;
    } else if (FIGHTING(ch)) {
      send_to_char("Maybe you'd better wait...\r\n", ch);
      return TRUE;
    } else if (!LIGHT_OK(ch)) {
      send_to_char("How do you expect to do that when you can't see anything?\r\n", ch);
      return TRUE;
    } else if (!EXIT(ch,dir)->keyword) {
      return FALSE;
    } else if (!isname(arg, EXIT(ch, dir)->keyword)) {
      return FALSE;
    } else if (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
      send_to_char("You can only damage closed doors!\r\n", ch);
      return TRUE;
    } else if (IS_EXPLOSIVE(ch, WEAR_WIELD) || IS_EXPLOSIVE(ch, WEAR_HOLD)) {
      send_to_char("You might want to think that over.\r\n", ch);
      return TRUE;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && IS_SPIRIT(ch) && ch->master &&
        !IS_NPC(ch->master))
      GET_ACTIVE(ch)--;

    if (GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0))
      wielded = GET_EQ(ch, WEAR_WIELD);
    else if (GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1))
      wielded = GET_EQ(ch, WEAR_HOLD);
    if (!wielded)
      draw_weapon(ch);
    if (wielded) {
      if (GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON) {
        if (!has_ammo(ch, wielded->worn_on))
          return TRUE;
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        if (EXIT(ch, dir)->keyword) {
          sprintf(buf, "$n attacks the %s.", fname(EXIT(ch, dir)->keyword));
          act(buf, TRUE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "You aim $p at the %s!", fname(EXIT(ch, dir)->keyword));
          act(buf, FALSE, ch, wielded, 0, TO_CHAR);
        } else {
          act("$n attacks the door.", TRUE, ch, 0, 0, TO_ROOM);
          act("You aim $p at the door!", FALSE, ch, wielded, 0, TO_CHAR);
        }
        damage_door(ch, ch->in_room, dir, GET_OBJ_VAL(wielded, 0), DAMOBJ_PIERCE);
        return TRUE;
      } else if (GET_OBJ_VAL(wielded, 3) >= TYPE_PISTOL) {
        if (!has_ammo(ch, wielded->worn_on))
          return TRUE;
        WAIT_STATE(ch, PULSE_VIOLENCE * 2);
        if (EXIT(ch, dir)->keyword) {
          sprintf(buf, "$n attacks the %s.", fname(EXIT(ch, dir)->keyword));
          act(buf, TRUE, ch, 0, 0, TO_ROOM);
          sprintf(buf, "You aim $p at the %s!", fname(EXIT(ch, dir)->keyword));
          act(buf, FALSE, ch, wielded, 0, TO_CHAR);
        } else {
          act("$n attacks the door.", TRUE, ch, 0, 0, TO_ROOM);
          act("You aim $p at the door!", FALSE, ch, wielded, 0, TO_CHAR);
        }
        damage_door(ch, ch->in_room, dir, GET_OBJ_VAL(wielded, 0), DAMOBJ_PROJECTILE);
        return TRUE;
      } else
        switch (GET_OBJ_VAL(wielded, 3)) {
        case TYPE_PIERCE:
        case TYPE_STAB:
        case TYPE_SHURIKEN:
          type = DAMOBJ_PIERCE;
          break;
        case TYPE_STING:
        case TYPE_SLASH:
        case TYPE_CLAW:
        case TYPE_THRASH:
        case TYPE_ARROW:
        case TYPE_THROWING_KNIFE:
          type = DAMOBJ_SLASH;
          break;
        case TYPE_HIT:
        case TYPE_BLUDGEON:
        case TYPE_POUND:
        case TYPE_MAUL:
        case TYPE_PUNCH:
          type = DAMOBJ_CRUSH;
          break;
        default:
          if (EXIT(ch, dir)->keyword) {
            sprintf(buf, "You can't damage the %s with $p!",
                    fname(EXIT(ch, dir)->keyword));
            act(buf, FALSE, ch, wielded, 0, TO_CHAR);
          } else
            act("You can't damage the door with $p!",
                FALSE, ch, wielded, 0, TO_CHAR);
          return TRUE;
        }
      WAIT_STATE(ch, PULSE_VIOLENCE * 2);
      if (EXIT(ch, dir)->keyword) {
        sprintf(buf, "$n attacks the %s.", fname(EXIT(ch, dir)->keyword));
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
        sprintf(buf, "You aim $p at the %s!", fname(EXIT(ch, dir)->keyword));
        act(buf, FALSE, ch, wielded, 0, TO_CHAR);
      } else {
        act("$n attacks the door.", TRUE, ch, 0, 0, TO_ROOM);
        act("You aim $p at the door!", FALSE, ch, wielded, 0, TO_CHAR);
      }
      damage_door(ch, ch->in_room, dir, GET_OBJ_VAL(wielded, 0), type);
    } else {

      WAIT_STATE(ch, PULSE_VIOLENCE * 2);
      if (EXIT(ch, dir)->keyword) {
        send_to_char(ch, "You take a swing at the %s!\r\n",
                     fname(EXIT(ch, dir)->keyword));
        sprintf(buf, "$n attacks the %s.", fname(EXIT(ch, dir)->keyword));
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
      } else {
        act("You take a swing at the door!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n attacks the door.", FALSE, ch, 0, 0, TO_ROOM);
      }
      damage_door(ch, ch->in_room, dir, GET_STR(ch), DAMOBJ_CRUSH);
    }
    return TRUE;
  }

  if (veh)
  {
    if (!FIGHTING(ch)) {
      if (!(GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0)) &&
          !(GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1)))
        draw_weapon(ch);

      if (GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0)) {
        sprintf(buf, "You aim %s at %s!\r\n", GET_EQ(ch, WEAR_WIELD)->text.name, veh->short_description);
        send_to_char(buf, ch);
        sprintf(buf, "%s aims %s right at your ride!\r\n", GET_NAME(ch), GET_EQ(ch, WEAR_WIELD)->text.name);
        send_to_veh(buf, veh, NULL, TRUE);
      } else if (GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1)) {
        sprintf(buf, "You aim %s at %s!\r\n", GET_EQ(ch, WEAR_HOLD)->text.name, veh->short_description);
        send_to_char(buf, ch);
        sprintf(buf, "%s aims %s right at your ride!\r\n", GET_NAME(ch), GET_EQ(ch, WEAR_HOLD)->text.name);
        send_to_veh(buf, veh, NULL, TRUE);
      } else {
        sprintf(buf, "You take a swing at %s!\r\n", veh->short_description);
        send_to_char(buf, ch);
        if (get_speed(veh) > 10)
          sprintf(buf, "%s throws %sself out infront of you!\r\n", GET_NAME(ch), thrdgenders[(int)GET_SEX(ch)]);
        else
          sprintf(buf, "%s takes a swing at your ride!\r\n", GET_NAME(ch));
        send_to_veh(buf, veh, NULL, TRUE);
      }
      if (FIGHTING_VEH(ch) == veh) {
        send_to_char(ch, "But you're already attacking it.\r\n");
        return TRUE;
      }
      set_fighting(ch, veh);
      return TRUE;
    }
  }
  if (vict == ch)
  {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.",
        FALSE, ch, 0, vict, TO_CHAR);
  else
  {
    if (IS_ASTRAL(ch) && !IS_DUAL(vict) && !IS_ASTRAL(vict) &&
        !PLR_FLAGGED(vict, PLR_PERCEIVE)) {
      send_to_char("Nah...that wouldn't work.\r\n", ch);
      return TRUE;
    }
    if (IS_ASTRAL(vict) && PLR_FLAGGED(ch, PLR_PERCEIVE)) {
      send_to_char("You can't initiate attacks on astral beings in your "
                   "current form.\r\n", ch);
      return TRUE;
    }
    if (!(GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0)) &&
        !(GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1)))
      draw_weapon(ch);

    if (IS_EXPLOSIVE(ch, WEAR_WIELD) || IS_EXPLOSIVE(ch, WEAR_HOLD)) {
      send_to_char("You might want to think that over.\r\n", ch);
      return TRUE;
    }
    if (IS_AFFECTED(ch, AFF_CHARM) && IS_SPIRIT(ch) && ch->master && !IS_NPC(ch->master))
      GET_ACTIVE(ch)--;
    if (!FIGHTING(ch)) {
      set_fighting(ch, vict);
      if (!FIGHTING(vict) && AWAKE(vict))
        set_fighting(vict, ch);
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
      act("$n attacks $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      if (GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0)) {
        act("You aim $p at $N!", FALSE, ch, GET_EQ(ch, WEAR_WIELD), vict, TO_CHAR);
        act("$n aims $p straight at you!", FALSE, ch, GET_EQ(ch, WEAR_WIELD), vict, TO_VICT);
      } else if (GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1)) {
        act("You aim $p at $N!", FALSE, ch, GET_EQ(ch, WEAR_HOLD), vict, TO_CHAR);
        act("$n aims $p straight at you!", FALSE, ch, GET_EQ(ch, WEAR_HOLD), vict, TO_VICT);
      } else {
        act("You take a swing at $N!", FALSE, ch, 0, vict, TO_CHAR);
        act("$n prepares to take a swing at you!", FALSE, ch, 0, vict, TO_VICT);
      }
    } else if (vict != FIGHTING(ch)) {
      char name[80];
      strcpy(name, GET_NAME(FIGHTING(ch)));
      stop_fighting(ch);
      set_fighting(ch, vict);
      if (!FIGHTING(vict) && AWAKE(vict))
        set_fighting(vict, ch);
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
      sprintf(buf, "$n stops fighting %s and attacks $N!", name);
      act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
      act("You focus on attacking $N.", FALSE, ch, 0, vict, TO_CHAR);
      sprintf(buf, "$n stops fighting %s and aims at you!", name);
      act(buf, TRUE, ch, 0, vict, TO_VICT);
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
  return TRUE;
}

ACMD(do_hit)
{
  if (!perform_hit(ch, argument, (char *)(subcmd == SCMD_HIT ? "hit" :
                                          (subcmd == SCMD_KILL ? "kill" : "murder"))))
    send_to_char("You can't seem to find the target you're looking for.\r\n", ch);
}

/* so they have to type out 'kill' */
ACMD(do_kil)
{
  send_to_char("You need to type it out if you wanna kill something.\n\r",ch);
}

ACMD(do_kill)
{
  struct char_data *vict;

  if ((!access_level(ch, LVL_VICEPRES)) || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg)))
      send_to_char("They aren't here.\r\n", ch);
    else if (ch == vict)
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      if (!IS_NPC(vict)) {
        sprintf(buf2, "%s raw killed by %s. {%s (%ld)}", GET_CHAR_NAME(vict),
                GET_NAME(ch), world[vict->in_room].name,
                world[vict->in_room].number);

        mudlog(buf2, vict, LOG_DEATHLOG, TRUE);
      }

      raw_kill(vict);
    }
  }
}

ACMD(do_shoot)
{
  struct obj_data *weapon = NULL;
  char direction[MAX_INPUT_LENGTH];
  char target[MAX_INPUT_LENGTH];
  int dir, i, pos = 0, range = -1;

  if (!GET_WIELDED(ch, 0) && !GET_WIELDED(ch, 1)) {
    send_to_char("Wielding a weapon generally helps.\r\n", ch);
    return;
  }

  two_arguments(argument, target, direction);

  if (*direction && AFF_FLAGGED(ch, AFF_DETECT_INVIS)) {
    send_to_char(ch, "The ultrasound distorts your vision.\r\n");
    return;
  }

  for (i = WEAR_WIELD; i <= WEAR_HOLD; i++)
    if (GET_WIELDED(ch, i - WEAR_WIELD) && (weapon = GET_EQ(ch, i)) &&
        (GET_OBJ_TYPE(weapon) == ITEM_FIREWEAPON || (GET_OBJ_TYPE(weapon) == ITEM_WEAPON &&
            GET_OBJ_VAL(weapon, 3) >= TYPE_GRENADE_LAUNCHER)))
      if (find_weapon_range(ch, weapon) > range)
        pos = i;

  if (!pos) {
    send_to_char("Normally guns or bows are used to do that.\r\n", ch);
    return;
  }

  if (perform_hit(ch, argument, "shoot"))
    return;

  weapon = GET_EQ(ch, pos);
  range = find_weapon_range(ch, weapon);

  if (!*target) {
    send_to_char("Syntax: shoot <target>\r\n"
                 "        shoot <target> <direction>\r\n", ch);
    return;
  }

  if ((dir = search_block(direction, lookdirs, FALSE)) == -1) {
    send_to_char("What direction?\r\n", ch);
    return;
  }
  dir = convert_look[dir];

  if (dir == NUM_OF_DIRS) {
    send_to_char("What direction?\r\n", ch);
    return;
  }

  if (!CAN_GO(ch, dir)) {
    send_to_char("There seems to be something in the way...\r\n", ch);
    return;
  }

  range_combat(ch, target, weapon, range, dir);
}

ACMD(do_throw)
{
  struct obj_data *weapon = NULL;
  int i, dir;
  char arg2[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  two_arguments(argument, arg1, arg2);

  if (!GET_WIELDED(ch, 0) && !GET_WIELDED(ch, 1)) {
    send_to_char("You're not wielding anything!\r\n", ch);
    return;
  }

  for (i = WEAR_WIELD; i <= WEAR_HOLD; i++)
    if (GET_WIELDED(ch, i - WEAR_WIELD) && (weapon = GET_EQ(ch, i)) &&
        (GET_OBJ_VAL(weapon, 3) == TYPE_SHURIKEN ||
         GET_OBJ_VAL(weapon, 3) == TYPE_THROWING_KNIFE ||
         GET_OBJ_VAL(weapon, 3) == TYPE_HAND_GRENADE))
      break;

  if (i > WEAR_HOLD || !weapon) {
    send_to_char("You should wield a throwing weapon first!\r\n", ch);
    return;
  }

  if (GET_OBJ_VAL(weapon, 3) == TYPE_HAND_GRENADE) {
    if (!*arg1) {
      send_to_char("Syntax: throw <direction>\r\n", ch);
      return;
    }
    if ((dir = search_block(arg1, lookdirs, FALSE)) == -1) {
      send_to_char("What direction?\r\n", ch);
      return;
    }
    dir = convert_look[dir];
  } else {
    if (!*arg1 || !*arg2) {
      send_to_char("Syntax: throw <target> <direction>\r\n", ch);
      return;
    }

    if ((dir = search_block(arg2, lookdirs, FALSE)) == -1) {
      send_to_char("What direction?\r\n", ch);
      return;
    }
    dir = convert_look[dir];
  }

  if (dir == NUM_OF_DIRS) {
    send_to_char("What direction?\r\n", ch);
    return;
  }

  if (!CAN_GO(ch, dir)) {
    send_to_char("There seems to be something in the way...\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(weapon, 3) == TYPE_HAND_GRENADE)
    range_combat(ch, NULL, weapon, 1, dir);
  else
    range_combat(ch, arg1, weapon, 1, dir);
}

int has_power(struct char_data *ch, int power)
{
  if (!IS_SPIRIT(ch) || GET_MOB_VNUM(ch) < 25 || GET_MOB_VNUM(ch) > 41)
    return 0;

  if (power == PWR_MANIFEST)
    return 1;
  else
    switch (GET_MOB_VNUM(ch))
    {
    case 25:     // air elemental
      if (power == PWR_ENGULF || power == PWR_BREATHE)
        return 1;
      break;
    case 26:     // earth elemental
    case 28:     // water elemental
      if (power == PWR_ENGULF)
        return 1;
      break;
    case 27:     // fire elemental
      if (power == PWR_AURA || power == PWR_ENGULF || power == PWR_PROJECT ||
          power == PWR_GUARD)
        return 1;
      break;
    case 29:     // city spirit
      if (power == PWR_ACCIDENT || power == PWR_ALIENATE ||
          power == PWR_CONCEAL || power == PWR_CONFUSE || power == PWR_FEAR ||
          power == PWR_FIND || power == PWR_GUARD)
        return 1;
      break;
    case 30:     // hearth spirit
      if (power == PWR_ACCIDENT || power == PWR_ALIENATE ||
          power == PWR_CONCEAL || power == PWR_CONFUSE || power == PWR_FIND ||
          power == PWR_GUARD)
        return 1;
      break;
    case 31:     // field spirit
      if (power == PWR_ACCIDENT || power == PWR_CONCEAL ||
          power == PWR_FIND || power == PWR_GUARD)
        return 1;
      break;
    case 32:     // desert spirit
      if (power == PWR_CONCEAL || power == PWR_FIND || power == PWR_GUARD)
        return 1;
      break;
    case 33:     // forest spirit
      if (power == PWR_ACCIDENT || power == PWR_CONCEAL ||
          power == PWR_CONFUSE || power == PWR_FEAR ||
          power == PWR_GUARD)
        return 1;
      break;
    case 34:     // mountain spirit
      if (power == PWR_ACCIDENT || power == PWR_CONCEAL ||
          power == PWR_FIND || power == PWR_GUARD)
        return 1;
      break;
    case 35:     // prairie spirit
      if (power == PWR_ACCIDENT || power == PWR_ALIENATE ||
          power == PWR_CONCEAL || power == PWR_FIND || power == PWR_GUARD)
        return 1;
      break;
    case 36:     // mist spirit
      if (power == PWR_ACCIDENT || power == PWR_CONCEAL ||
          power == PWR_CONFUSE || power == PWR_GUARD)
        return 1;
      break;
    case 37:     // storm spirit
      if (power == PWR_CONCEAL || power == PWR_CONFUSE || power == PWR_FEAR ||
          power == PWR_PROJECT)
        return 1;
      break;
    case 38:     // lake spirit
      if (power == PWR_ACCIDENT || power == PWR_ENGULF ||
          power == PWR_FEAR || power == PWR_FIND || power == PWR_GUARD)
        return 1;
      break;
    case 39:     // river spirit
    case 40:     // sea spirit
    case 41:     // swamp spirit
      if (GET_MOB_VNUM(ch) != 39 && power == PWR_CONFUSE)
        return 1;
      if (GET_MOB_VNUM(ch) == 40 && power == PWR_ALIENATE)
        return 1;
      if (GET_MOB_VNUM(ch) == 41 && power == PWR_BIND)
        return 1;
      if (power == PWR_ACCIDENT || power == PWR_CONCEAL ||
          power == PWR_ENGULF || power == PWR_FEAR || power == PWR_FIND ||
          power == PWR_GUARD)
        return 1;
      break;
    }

  return 0;
}

void process_command(struct char_data *ch, char *message)
{
  command_interpreter(ch, message);
}

ACMD(do_order)
{
  char name[100], message[256];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Order who to do what?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers"))
    send_to_char("That person isn't here.\r\n", ch);
  else if (ch == vict)
    send_to_char("You obviously suffer from schizophrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_NOTVICT);

      if ((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM))
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
        send_to_char(OK, ch);
        process_command(vict, message);
      }
    } else {                    /* This is order "followers" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room)
          if (IS_AFFECTED(k->follower, AFF_CHARM)) {
            found = TRUE;
            process_command(k->follower, message);
          }
      }
      if (found)
        send_to_char(OK, ch);
      else
        send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}

ACMD(do_flee)
{
  int i, attempt, wasin = ch->in_room;
  struct char_data *temp;

  WAIT_STATE(ch, 24);

  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 2);       /* Select a random direction */
    if (CAN_GO(ch, attempt) && (!IS_NPC(ch) ||
                                !ROOM_FLAGGED(world[ch->in_room].dir_option[attempt]->to_room, ROOM_NOMOB))) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (do_simple_move(ch, attempt, CHECK_SPECIAL | LEADER, NULL)) {
        send_to_char("You flee head over heels.\r\n", ch);
        for (temp = world[wasin].people; temp; temp = temp->next_in_room)
          if (FIGHTING(temp) == ch && CAN_SEE(temp, ch) &&
              success_test(GET_QUI(temp), GET_QUI(ch)) >=
              success_test(GET_QUI(ch), GET_QUI(temp)))
            hit(temp, ch, TYPE_MELEE);
      } else
        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}

ACMD(do_retreat)
{
  int attempt = 0, wasin = ch->in_room;
  struct char_data *temp;
  char arg[25];

  if ( FIGHTING(ch) == NULL ) {
    send_to_char("But you aren't fighting anyone!\n\r",ch);
    return;
  }

  one_argument(argument, arg);

  switch ( arg[0] ) {
  case 'n':
  case 'N':
    switch ( arg[1] ) {
    case 'e':
    case 'E':
      attempt = 1;
      break;
    case 'w':
    case 'W':
      attempt = 7;
      break;
    default:
      attempt = 0;
    }
    break;
  case 'e':
  case 'E':
    attempt = 2;
    break;
  case 's':
  case 'S':
    switch( arg[1] ) {
    case 'e':
    case 'E':
      attempt = 3;
      break;
    case 'w':
    case 'W':
      attempt = 5;
      break;
    default:
      attempt = 4;
    }
    break;
  case 'w':
  case 'W':
    attempt = 6;
    break;
  case 'u':
  case 'U':
    attempt = 8;
    break;
  case 'd':
  case 'D':
    attempt = 9;
    break;
  default:
    send_to_char("You must retreat in a direction.\n\r",ch);
    return;
  }

  /* Wait on it */
  WAIT_STATE(ch, PULSE_VIOLENCE * 6);

  if (success_test(GET_QUI(ch), GET_QUI(FIGHTING(ch))) && (CAN_GO(ch, attempt) && (!IS_NPC(ch) ||
      !ROOM_FLAGGED(world[ch->in_room].dir_option[attempt]->to_room, ROOM_NOMOB)))) {
    act("$n searches for a quick escape!", TRUE, ch, 0, 0, TO_ROOM);
    if (do_simple_move(ch, attempt, CHECK_SPECIAL | LEADER, NULL)) {
      send_to_char("You start moving away for a clever escape.\r\n", ch);
      for (temp = world[wasin].people; temp; temp = temp->next_in_room)
        if (FIGHTING(temp) == ch && CAN_SEE(temp, ch) &&
            success_test(GET_QUI(temp), GET_QUI(ch)) >=
            success_test(GET_QUI(ch), GET_QUI(temp)))
          hit(temp, ch, TYPE_MELEE);
    } else
      act("$n attempts a retreat, but fails.", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  send_to_char("Couldn't make your way out in that direction.\r\n", ch);
}

ACMD(do_kick)
{
  struct char_data *vict;
  int dir;

  any_one_arg(argument, arg);

  if (!*arg) {
    send_to_char("Kick who or what?\r\n", ch);
    return;
  }

  for (dir = 0; dir < NUM_OF_DIRS; dir++)
    if (EXIT(ch, dir) && EXIT(ch, dir)->keyword &&
        isname(arg, EXIT(ch, dir)->keyword) &&
        !IS_SET(EXIT(ch, dir)->exit_info, EX_DESTROYED))
      break;
  if (dir >= NUM_OF_DIRS) {
    if (!(vict = get_char_room_vis(ch, arg)))
      send_to_char("They aren't here.\r\n", ch);
    else if (vict == ch) {
      send_to_char("Are you flexible enough to do that?\r\n", ch);
      act("$n tries to kick $s own ass... not too bright, is $e?",
          TRUE, ch, 0, 0, TO_ROOM);
    } else {
      act("You sneak up behind $N and kick $M straight in the ass!",
          FALSE, ch, 0, vict, TO_CHAR);
      act("$n sneaks up behind you and kicks you in the ass!\r\n"
          "That's gonna leave a mark...", FALSE, ch, 0, vict, TO_VICT);
      act("$n sneaks up behind $N and gives $M a swift kick in the ass!",
          TRUE, ch, 0, vict, TO_NOTVICT);
    }
    return;
  }

  if (FIGHTING(ch))
    send_to_char("Maybe you'd better wait...\r\n", ch);
  else if (!IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED))
    send_to_char("You can only damage closed doors!\r\n", ch);
  else if (!LIGHT_OK(ch))
    send_to_char("How do you expect to do that when you can't see anything?\r\n", ch);
  else if (world[ch->in_room].peaceful)
    send_to_char("Nah - leave it in peace.\r\n", ch);
  else {
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    if (EXIT(ch, dir)->keyword) {
      send_to_char(ch, "You show the %s the bottom of %s!\r\n",
                   fname(EXIT(ch, dir)->keyword), GET_EQ(ch, WEAR_FEET) ?
                   GET_EQ(ch, WEAR_FEET)->text.name : "your foot");
      sprintf(buf, "$n kicks the %s.", fname(EXIT(ch, dir)->keyword));
      act(buf, TRUE, ch, 0, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You show the door the bottom of %s!\r\n",
                   GET_EQ(ch, WEAR_FEET) ?
                   GET_EQ(ch, WEAR_FEET)->text.name : "your foot");
      act("$n kicks the door.", TRUE, ch, 0, 0, TO_ROOM);
    }
    damage_door(ch, ch->in_room, dir, (int)(GET_STR(ch) / 2), DAMOBJ_CRUSH);
  }
}

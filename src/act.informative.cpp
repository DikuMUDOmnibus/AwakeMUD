/*************************************************************************
*  File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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
#include <time.h>
#include <errno.h>
#include <iostream>
#include <fstream>

using namespace std;

#if !defined(WIN32) || defined(__CYGWIN__)
#include <sys/time.h>
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "dblist.h"
#include "spells.h"
#include "screen.h"
#include "list.h"
#include "awake.h"
#include "constants.h"
#include "class.h"

char *CCHAR;

/* extern variables */
extern class objList ObjList;
extern class helpList Help;
extern class helpList WizHelp;

extern char *short_object(int virt, int where);
extern const char *dist_name[];

extern int same_obj(struct obj_data * obj1, struct obj_data * obj2);
extern int find_sight(struct char_data *ch);
extern int belongs_to(struct char_data *ch, struct obj_data *obj);
extern int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
extern char *colorize(struct descriptor_data *, char *);
/* blood stuff */

char* blood_messages[] = {
                           "If you see this, alert an immort.\r\n", /* 0 */
                           "^rThere is a little blood here.\r\n",
                           "^rYou are standing in a pool of blood.\r\n",
                           "^rBlood is flowing here. Gross.\r\n",
                           "^rThere's a lot of blood here...you feel sick.\r\n",
                           "^rYou've seen less blood at a GWAR concert.\r\n", /* 5 */
                           "^rCripes, there's blood EVERYWHERE. You can taste it in the air.\r\n",
                           "^rThe walls are practically painted red with blood.\r\n",
                           "^rBlood drips from the walls and ceiling, and covers the floor an inch deep.\r\n",
                           "^rThere is gore, blood, guts and parts everywhere. The stench is horrible.\r\n",
                           "^rThe gore is indescribible, and you feel numb and light headed.\r\n", /* 10 */
                           "If you see this, alert an immort that the blood level is too high.\r\n"
                         };


/* end blood stuff */

char *make_desc(struct char_data *ch, struct char_data *i, char *buf, int act)
{
  char buf2[1024];
  if (((GET_EQ(i, WEAR_HEAD) && GET_OBJ_VAL(GET_EQ(i, WEAR_HEAD), 7) > 1) ||
       (GET_EQ(i, WEAR_FACE) && GET_OBJ_VAL(GET_EQ(i, WEAR_FACE), 7) > 1)) && (act == 2 ||
           success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT),
                        GET_EQ(i, WEAR_HEAD) ? GET_OBJ_VAL(GET_EQ(i, WEAR_HEAD), 7) : 0 +
                        GET_EQ(i, WEAR_FACE) ? GET_OBJ_VAL(GET_EQ(i, WEAR_FACE), 7) : 0) < 1))
  {
    int conceal = (GET_EQ(i, WEAR_ABOUT) ? GET_OBJ_VAL(GET_EQ(i, WEAR_ABOUT), 7) : 0) +
                  (GET_EQ(i, WEAR_BODY) ? GET_OBJ_VAL(GET_EQ(i, WEAR_BODY), 7) : 0) +
                  (GET_EQ(i, WEAR_UNDER) ? GET_OBJ_VAL(GET_EQ(i, WEAR_UNDER), 7) : 0);
    conceal = act == 2 ? 4 : success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), conceal);
    sprintf(buf, "A");
    if (conceal > 0) {
      if (GET_HEIGHT(i) < 130)
        strcat(buf, " tiny");
      else if (GET_HEIGHT(i) < 160)
        strcat(buf, " small");
      else if (GET_HEIGHT(i) < 190)
        strcat(buf, " average");
      else if (GET_HEIGHT(i) < 220)
        strcat(buf, " large");
      else
        strcat(buf, " huge");
    }
    if (conceal > 2)
      sprintf(buf + strlen(buf), " %s", genders[(int)GET_SEX(i)]);
    if (conceal > 3)
      sprintf(buf + strlen(buf), " %s", pc_race_types[(int)GET_RACE(i)]);
    else
      strcat(buf, " person");
    if (GET_EQ(i, WEAR_ABOUT))
      sprintf(buf + strlen(buf), " wearing %s", GET_OBJ_NAME(GET_EQ(i, WEAR_ABOUT)));
    else if (GET_EQ(i, WEAR_BODY))
      sprintf(buf + strlen(buf), " wearing %s", GET_OBJ_NAME(GET_EQ(i, WEAR_BODY)));
    else if (GET_EQ(i, WEAR_UNDER))
      sprintf(buf + strlen(buf), " wearing %s", GET_OBJ_NAME(GET_EQ(i, WEAR_UNDER)));
  } else
  {
    struct remem *mem;
    if (!act) {
      strcpy(buf, CAP(GET_NAME(i)));
      if (IS_SENATOR(ch) && !IS_NPC(i)) {
        sprintf(buf2, " (%s)", CAP(GET_CHAR_NAME(i)));
        strcat(buf, buf2);
      } else if ((mem = found_mem(GET_MEMORY(ch), i))) {
        sprintf(buf2, " (%s)", CAP(mem->mem));
        strcat(buf, buf2);
      }
    } else if ((mem = found_mem(GET_MEMORY(ch), i)) && act != 2)
      strcpy(buf, CAP(mem->mem));
    else
      strcpy(buf, CAP(GET_NAME(i)));
  }
  return buf;
}

void get_obj_condition(struct char_data *ch, struct obj_data *obj)
{
  if (GET_OBJ_EXTRA(obj).IsSet(ITEM_CORPSE))
  {
    send_to_char("Examining it reveals that it really IS dead.\r\n", ch);
    return;
  }

  int condition = GET_OBJ_CONDITION(obj) * 100 / GET_OBJ_BARRIER(obj);
  sprintf(buf2, "%s is ", GET_OBJ_NAME(obj));
  if (condition >= 100)
    strcat(buf2, "in excellent condition.");
  else if (condition >= 90)
    strcat(buf2, "barely damaged.");
  else if (condition >= 80)
    strcat(buf2, "lightly damaged.");
  else if (condition >= 60)
    strcat(buf2, "moderately damaged.");
  else if (condition >= 30)
    strcat(buf2, "seriously damaged.");
  else if (condition >= 10)
    strcat(buf2, "extremely damaged.");
  else
    strcat(buf2, "almost completely destroyed.");
  strcat(buf2, "\r\n");
  send_to_char(ch, buf2);
}

void show_obj_to_char(struct obj_data * object, struct char_data * ch, int mode)
{
  bool found;

  *buf = '\0';
  if ((mode == 0) && object->text.room_desc)
  {
    strcpy(buf, CCHAR ? CCHAR : "");
    strcat(buf, object->text.room_desc);
  } else if (object->text.name && ((mode == 1) ||
                                   (mode == 2) || (mode == 3) || (mode == 4) || (mode == 7)))
    strcpy(buf, GET_OBJ_NAME(object));
  else if (mode == 5)
  {
    if (object->photo)
      strcpy(buf, object->photo);
    else if (object->text. look_desc)
      strcpy(buf, object->text.look_desc);
    else
      strcpy(buf, "You see nothing special..");
  } else if (mode == 8)
    sprintf(buf, "\t\t\t\t%s", GET_OBJ_NAME(object));
  if (mode == 7 || mode == 8)
    if (GET_OBJ_TYPE(object) == ITEM_HOLSTER)
    {
      if (object->contains)
        sprintf(buf, "%s (Holding %s)", buf, GET_OBJ_NAME(object->contains));
      if (GET_OBJ_VAL(object, 3) == 1 && object->worn_by && object->worn_by == ch)
        sprintf(buf, "%s ^Y(Ready)", buf);
    } else if (GET_OBJ_TYPE(object) == ITEM_WORN && object->contains && !PRF_FLAGGED(ch, PRF_COMPACT))
      sprintf(buf, "%s carrying:", buf);
  if (mode != 3)
  {
    found = FALSE;

    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      sprintf(buf, "%s ^B(invisible)", buf);
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_BLESS) &&
        (IS_ASTRAL(ch) || IS_DUAL(ch) ||
         IS_AFFECTED(ch, AFF_DETECT_ALIGN))) {
      sprintf(buf, "%s ^b(blue aura)", buf);
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_MAGIC)
        && (IS_ASTRAL(ch)
            || IS_DUAL(ch)
            || IS_AFFECTED(ch, AFF_DETECT_MAGIC))) {
      sprintf(buf, "%s ^Y(yellow aura)", buf);
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      sprintf(buf, "%s ^W(glowing)", buf);
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      sprintf(buf, "%s ^c(humming)", buf);
      found = TRUE;
    }

    if ((IS_PROJECT(ch) || PLR_FLAGGED(ch, PLR_PERCEIVE)
         || IS_ASTRAL(ch) || IS_DUAL(ch))
        && GET_OBJ_TYPE(object) == ITEM_WEAPON
        && GET_OBJ_VAL(object, 3) < TYPE_TASER
        && GET_OBJ_VAL(object, 7) > 0
        && GET_OBJ_VAL(object, 8) > 0
        && GET_OBJ_VAL(object, 9) > 0) {
      sprintf(buf, "%s ^m(weapon focus)", buf );
      found = TRUE;
    } else if (GET_OBJ_TYPE(object) == ITEM_FOCUS
               && (IS_ASTRAL(ch) || IS_DUAL(ch))) {
      found = TRUE;
      if ( GET_OBJ_VAL(object, 9) > 0 ) {
        switch( GET_OBJ_VAL(object, VALUE_FOCUS_TYPE) ) {
        case FOCI_SPELL:
          sprintf(buf, "%s ^m(%s focus", buf,
                  spells[GET_OBJ_VAL(object,VALUE_FOCUS_SPECIFY)]);
          break;
        case FOCI_SPELL_CAT:
          sprintf(buf, "%s ^m(%c%s focus", buf,
                  LOWER(*spell_categories[GET_OBJ_VAL(object,VALUE_FOCUS_SPECIFY)]),
                  spell_categories[GET_OBJ_VAL(object,VALUE_FOCUS_SPECIFY)]+1);
          break;
        case FOCI_SPIRIT:
          sprintf(buf, "%s ^m(%c%s focus", buf,
                  LOWER(*spirits[GET_OBJ_VAL(object,VALUE_FOCUS_SPECIFY)]),
                  spirits[GET_OBJ_VAL(object,VALUE_FOCUS_SPECIFY)]+1);
          break;
        case FOCI_POWER:
          sprintf(buf, "%s ^m(power focus", buf );
          break;
        case FOCI_LOCK:
          sprintf(buf, "%s ^m(spell lock", buf );
          break;
        case FOCI_WEAPON:
          sprintf(buf, "%s ^m(weapon focus", buf );
          break;
        default:
          sprintf(buf, "%s ^m(focus", buf);
          break;
        }
        if ( GET_OBJ_VAL(object, 5) == 0 )
          sprintf(buf, "%s--inactive", buf);
        strcat(buf, ")");
      } else {
        switch( GET_OBJ_VAL(object, VALUE_FOCUS_TYPE) ) {
        case FOCI_SPELL:
          sprintf(buf, "%s ^m(unbonded spell focus)", buf );
          break;
        case FOCI_SPELL_CAT:
          sprintf(buf, "%s ^m(unbonded category focus)", buf );
          break;
        case FOCI_SPIRIT:
          sprintf(buf, "%s ^m(unbonded spirit focus)", buf );
          break;
        case FOCI_POWER:
          sprintf(buf, "%s ^m(unbonded power focus)", buf );
          break;
        case FOCI_LOCK:
          sprintf(buf, "%s ^m(unbonded spell lock)", buf );
          break;
        case FOCI_WEAPON:
          sprintf(buf, "%s ^m(unbonded weapon focus)", buf );
          break;
        default:
          sprintf(buf, "%s ^m(unbonded focus)", buf);
          break;
        }
      }
    }
  }
  strcat(buf, "^N\r\n");
  send_to_char(buf, ch);
  if ((mode == 7 || mode == 8) && !PRF_FLAGGED(ch, PRF_COMPACT))
    if (GET_OBJ_TYPE(object) == ITEM_WORN && object->contains)
    {
      for (struct obj_data *cont = object->contains; cont; cont = cont->next_content)
        show_obj_to_char(cont, ch, 8);
    }

}

void show_veh_to_char(struct veh_data * vehicle, struct char_data * ch)
{
  *buf = '\0';

  strcpy(buf, CCHAR ? CCHAR : "");

  if (vehicle->damage >= 10)
  {
    strcat(buf, vehicle->short_description);
    strcat(buf, " lies here wrecked.");
  } else
  {
    if (vehicle->type == VEH_BIKE && vehicle->people)
      sprintf(buf, "%s%s sitting on ", buf, found_mem(GET_MEMORY(ch), vehicle->people) ?
              found_mem(GET_MEMORY(ch), vehicle->people)->mem :
              GET_NAME(vehicle->people));
    switch (vehicle->cspeed) {
    case SPEED_OFF:
      if (vehicle->type == VEH_BIKE && vehicle->people) {
        strcat(buf, vehicle->short_description);
        strcat(buf, " waits here.");
      } else
        strcat(buf, vehicle->description);
      break;
    case SPEED_IDLE:
      strcat(buf, vehicle->short_description);
      strcat(buf, " idles here.");
      break;
    case SPEED_CRUISING:
      strcat(buf, vehicle->short_description);
      strcat(buf, " cruises through here.");
      break;
    case SPEED_SPEEDING:
      strcat(buf, vehicle->short_description);
      strcat(buf, " speeds past you.");
      break;
    case SPEED_MAX:
      strcat(buf, vehicle->short_description);
      strcat(buf, " zooms by you.");
      break;
    }
  }
  if (GET_IDNUM(ch) == vehicle->owner)
    strcat(buf, " ^Y(Yours)");
  strcat(buf, "^N\r\n");
  send_to_char(ch, buf);
}

void list_veh_to_char(struct veh_data * list, struct char_data * ch)
{
  struct veh_data *i;
  for (i = list; i; i = i->next_veh)
    if (ch->in_veh != i && ch->char_specials.rigging != i)
      show_veh_to_char(i, ch);
}

#define IS_INVIS(o) IS_OBJ_STAT(o, ITEM_INVISIBLE)

void
list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                 bool show, bool corpse)
{
  struct obj_data *i, *vis_obj;
  int num = 1;
  bool found;

  found = FALSE;

  for (i = list; i; i = i->next_content)
  {
    if (ch->in_veh && i->in_room != ch->in_veh->in_room)
      if (ch->in_veh->cspeed > SPEED_IDLE)
        if (get_speed(ch->in_veh) >= 200)
          if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 7))
            continue;
          else if (get_speed(ch->in_veh) < 200 && get_speed(ch->in_veh) >= 120)
            if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6))
              continue;
            else if (get_speed(ch->in_veh) < 120 && get_speed(ch->in_veh) >= 60)
              if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5))
                continue;
              else
                if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4))
                  continue;


    if (ch->char_specials.rigging)
      if (ch->char_specials.rigging->cspeed > SPEED_IDLE)
        if (get_speed(ch->char_specials.rigging) >= 240)
          if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6))
            continue;
          else if (get_speed(ch->char_specials.rigging) < 240 && get_speed(ch->char_specials.rigging) >= 180)
            if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5))
              continue;
            else if (get_speed(ch->char_specials.rigging) < 180 && get_speed(ch->char_specials.rigging) >= 90)
              if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4))
                continue;
              else
                if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 3))
                  continue;

    while (i->next_content) {
      if (i->item_number != i->next_content->item_number ||
          strcmp(i->text.name, i->next_content->text.name) ||
          IS_INVIS(i) != IS_INVIS(i->next_content) || i->restring)
        break;
      if (CAN_SEE_OBJ(ch, i)) {
        num++;
        vis_obj = i;
      }
      i = i->next_content;
    }


    if (CAN_SEE_OBJ(ch, i)) {
      if (corpse && IS_OBJ_STAT(i, ITEM_CORPSE)) {
        if (num > 1) {
          send_to_char(ch, "(%d) ", num);
        }
        show_obj_to_char(i, ch, mode);
      } else
        if (!corpse && !mode && !IS_OBJ_STAT(i, ITEM_CORPSE)) {
          if (num > 1) {
            send_to_char(ch, "(%d) ", num);
          }
          show_obj_to_char(i, ch, mode);
        } else if (mode) {
          if (num > 1) {
            send_to_char(ch, "(%d) ", num);
          }
          show_obj_to_char(i, ch, mode);
        }
      found = TRUE;
      num = 1;
    }
  }

  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}

void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int ment, phys;

  if (GET_MAX_PHYSICAL(i) >= 100)
    phys = (100 * GET_PHYSICAL(i)) / GET_MAX_PHYSICAL(i);
  else
    phys = -1;               /* How could MAX_PHYSICAL be < 1?? */

  if (GET_MAX_MENTAL(i) >= 100)
    ment = (100 * GET_MENTAL(i)) / GET_MAX_MENTAL(i);
  else
    ment = -1;

  make_desc(ch, i, buf, TRUE);
  CAP(buf);

  if (phys >= 100 || (GET_TRADITION(i) == TRAD_ADEPT && phys >= 0 &&
                      ((100 - phys) / 10) <= GET_POWER(i, ADEPT_PAIN_RESISTANCE)))
    strcat(buf, " is in excellent physical condition");
  else if (phys >= 90)
    strcat(buf, " has a few scratches");
  else if (phys >= 75)
    strcat(buf, " has some small wounds and bruises");
  else if (phys >= 50)
    strcat(buf, " has quite a few wounds");
  else if (phys >= 30)
    strcat(buf, " has some big nasty wounds and scratches");
  else if (phys >= 15)
    strcat(buf, " looks pretty hurt");
  else if (phys >= 0)
    strcat(buf, " is in awful condition");
  else
    strcat(buf, " is bleeding awfully from big wounds");

  if (phys <= 0)
    strcat(buf, " and is unconscious.\r\n");
  else if (ment >= 100 || (GET_TRADITION(i) == TRAD_ADEPT && ment >= 0 &&
                           ((100 - ment) / 10) <= (GET_POWER(i, ADEPT_PAIN_RESISTANCE) -
                                                   (int)((GET_MAX_PHYSICAL(i) - GET_PHYSICAL(i)) / 100))))
    strcat(buf, " and is alert.\r\n");
  else if (ment >= 90)
    strcat(buf, " and is barely tired.\r\n");
  else if (ment >= 75)
    strcat(buf, " and is slightly worn out.\r\n");
  else if (ment >= 50)
    strcat(buf, " and is fatigued.\r\n");
  else if (ment >= 30)
    strcat(buf, " and is weary.\r\n");
  else if (ment >= 10)
    strcat(buf, " and is groggy.\r\n");
  else
    strcat(buf, " is completely unconscious.\r\n");

  send_to_char(buf, ch);
}

void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found, weight;
  float height;
  struct obj_data *tmp_obj;


  if (((GET_EQ(i, WEAR_HEAD) && GET_OBJ_VAL(GET_EQ(i, WEAR_HEAD), 7) > 1) ||
       (GET_EQ(i, WEAR_FACE) && GET_OBJ_VAL(GET_EQ(i, WEAR_FACE), 7) > 1)) &&
      success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT),
                   GET_EQ(i, WEAR_HEAD) ? GET_OBJ_VAL(GET_EQ(i, WEAR_HEAD), 7) : 0 +
                   GET_EQ(i, WEAR_FACE) ? GET_OBJ_VAL(GET_EQ(i, WEAR_FACE), 7) : 0) < 1)
  {
    if (GET_EQ(i, WEAR_HEAD))
      send_to_char(ch, GET_EQ(i, WEAR_HEAD)->text.look_desc);
    else if (GET_EQ(i, WEAR_FACE))
      send_to_char(ch, GET_EQ(i, WEAR_FACE)->text.look_desc);
    if (GET_EQ(i, WEAR_ABOUT))
      send_to_char(ch, GET_EQ(i, WEAR_ABOUT)->text.look_desc);
    else if (GET_EQ(i, WEAR_BODY))
      send_to_char(ch, GET_EQ(i, WEAR_BODY)->text.look_desc);
    else if (GET_EQ(i, WEAR_UNDER))
      send_to_char(ch, GET_EQ(i, WEAR_UNDER)->text.look_desc);
  } else
  {
    if (i->player.physical_text.look_desc)
      send_to_char(i->player.physical_text.look_desc, ch);
    else
      act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

    if (i != ch && GET_HEIGHT(i) > 0 && GET_WEIGHT(i) > 0) {
      if ((GET_HEIGHT(i) % 10) < 5)
        height = (float)(GET_HEIGHT(i) - (GET_HEIGHT(i) % 10)) / 100;
      else
        height = (float)(GET_HEIGHT(i) - (GET_HEIGHT(i) % 10) + 10) / 100;
      if ((GET_WEIGHT(i) % 10) < 5)
        weight = GET_WEIGHT(i) - (GET_WEIGHT(i) % 10);
      else
        weight = GET_WEIGHT(i) - (GET_WEIGHT(i) % 10) + 10;
      sprintf(buf, "$e looks to be about %0.1f meters tall and "
              "seems to weigh about %d kg.", height, weight);
      act(buf, FALSE, i, 0, ch, TO_VICT);
    }
    diag_char_to_char(i, ch);
  }

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found)
  {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
        if (GET_OBJ_TYPE(GET_EQ(i, j)) == ITEM_HOLSTER && GET_OBJ_VAL(GET_EQ(i, j), 0) == 2) {
          send_to_char(where[j], ch);
          show_obj_to_char(GET_EQ(i, j), ch, 7);
        } else if (j == WEAR_WIELD || j == WEAR_HOLD) {
          if (IS_OBJ_STAT(GET_EQ(i, j), ITEM_TWOHANDS))
            send_to_char(hands[2], ch);
          else if (j == WEAR_WIELD)
            send_to_char(hands[(int)i->char_specials.saved.left_handed], ch);
          else
            send_to_char(hands[!i->char_specials.saved.left_handed], ch);
          show_obj_to_char(GET_EQ(i, j), ch, 1);
        } else if ((j == WEAR_BODY || j == WEAR_LARM || j == WEAR_RARM || j == WEAR_WAIST)
                   && GET_EQ(i, WEAR_ABOUT)) {
          if (success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4 + GET_OBJ_VAL(GET_EQ(i, WEAR_ABOUT), 7)) >= 2) {
            send_to_char(where[j], ch);
            show_obj_to_char(GET_EQ(i, j), ch, 1);
          }
        } else if (j == WEAR_UNDER && (GET_EQ(i, WEAR_ABOUT) || GET_EQ(i, WEAR_BODY))) {
          if (success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6 +
                           (GET_EQ(i, WEAR_ABOUT) ? GET_OBJ_VAL(GET_EQ(i, WEAR_ABOUT), 7) : 0) +
                           (GET_EQ(i, WEAR_BODY) ? GET_OBJ_VAL(GET_EQ(i, WEAR_BODY), 7) : 0)) >= 2) {
            send_to_char(where[j], ch);
            show_obj_to_char(GET_EQ(i, j), ch, 1);
          }
        } else if (j == WEAR_LEGS && GET_EQ(i, WEAR_ABOUT)) {
          if (success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 2 + GET_OBJ_VAL(GET_EQ(i, WEAR_ABOUT), 7)) >= 2) {
            send_to_char(where[j], ch);
            show_obj_to_char(GET_EQ(i, j), ch, 1);
          }
        } else if ((j == WEAR_RANKLE || j == WEAR_LANKLE) && (GET_EQ(i, WEAR_ABOUT) || GET_EQ(i, WEAR_LEGS))) {
          if (success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5 +
                           (GET_EQ(i, WEAR_ABOUT) ? GET_OBJ_VAL(GET_EQ(i, WEAR_ABOUT), 7) : 0) +
                           (GET_EQ(i, WEAR_LEGS) ? GET_OBJ_VAL(GET_EQ(i, WEAR_LEGS), 7) : 0)) >= 2) {
            send_to_char(where[j], ch);
            show_obj_to_char(GET_EQ(i, j), ch, 1);
          }
        } else {
          send_to_char(where[j], ch);
          show_obj_to_char(GET_EQ(i, j), ch, 1);
        }
      }
  }

  found = FALSE;
  for (tmp_obj = i->cyberware; tmp_obj && !found; tmp_obj = tmp_obj->next_content)
    if ((GET_OBJ_VAL(tmp_obj, 2) == 0 || GET_OBJ_VAL(tmp_obj, 2) == 1 ||
         GET_OBJ_VAL(tmp_obj, 2) == 23 || (GET_OBJ_VAL(tmp_obj, 2) == 27 && GET_OBJ_VAL(tmp_obj, 1) > 0) ||
         ((GET_OBJ_VAL(tmp_obj, 2) == 19 || GET_OBJ_VAL(tmp_obj, 2) == 21) &&
          GET_POS(i) == POS_FIGHTING && !GET_EQ(i, WEAR_WIELD))))
      found = TRUE;

  if (found)
  {
    act("\r\nVisible cyberware:", FALSE, i, 0, ch, TO_VICT);

    for (tmp_obj = i->cyberware; tmp_obj; tmp_obj = tmp_obj->next_content)
      if ((GET_OBJ_VAL(tmp_obj, 2) == 0 ||
           GET_OBJ_VAL(tmp_obj, 2) == 1 ||
           GET_OBJ_VAL(tmp_obj, 2) == 23 ||
           (GET_OBJ_VAL(tmp_obj, 2) == 27 &&
            GET_OBJ_VAL(tmp_obj, 1) > 0) ||
           ((GET_OBJ_VAL(tmp_obj, 2) == 19 ||
             GET_OBJ_VAL(tmp_obj, 2) == 21) &&
            GET_POS(i) == POS_FIGHTING &&
            !GET_EQ(i, WEAR_WIELD))))
        if (GET_OBJ_VAL(tmp_obj, 2) == 23 &&
            isname("sheathing", tmp_obj->text.keywords))
          send_to_char(ch, "dermal sheathing\r\n");
        else
          send_to_char(ch, "%s\r\n", cyberware_names[GET_OBJ_VAL(tmp_obj, 2)]);
  }

  if ( GET_REAL_LEVEL(ch) >= LVL_BUILDER )
  {
    found = FALSE;
    for (tmp_obj = i->cyberware;
         tmp_obj && !found;
         tmp_obj = tmp_obj->next_content) {
      if ((GET_OBJ_VAL(tmp_obj, 2) == 0 ||
           GET_OBJ_VAL(tmp_obj, 2) == 1 ||
           GET_OBJ_VAL(tmp_obj, 2) == 23 ||
           (GET_OBJ_VAL(tmp_obj, 2) == 27 &&
            GET_OBJ_VAL(tmp_obj, 1) > 0) ||
           ((GET_OBJ_VAL(tmp_obj, 2) == 19 ||
             GET_OBJ_VAL(tmp_obj, 2) == 21) &&
            GET_POS(i) == POS_FIGHTING &&
            !GET_EQ(i, WEAR_WIELD))))
        ;
      else
        found = TRUE;
    }

    if (found) {
      act("\r\nInternal cyberware:", FALSE, i, 0, ch, TO_VICT);
      for (tmp_obj = i->cyberware; tmp_obj; tmp_obj = tmp_obj->next_content) {
        if ((GET_OBJ_VAL(tmp_obj, 2) == 0 ||
             GET_OBJ_VAL(tmp_obj, 2) == 1 ||
             GET_OBJ_VAL(tmp_obj, 2) == 23 ||
             (GET_OBJ_VAL(tmp_obj, 2) == 27 &&
              GET_OBJ_VAL(tmp_obj, 1) > 0) ||
             ((GET_OBJ_VAL(tmp_obj, 2) == 19 ||
               GET_OBJ_VAL(tmp_obj, 2) == 21) &&
              GET_POS(i) == POS_FIGHTING &&
              !GET_EQ(i, WEAR_WIELD))))
          ;
        else
          if (GET_OBJ_VAL(tmp_obj, 2) == 23 &&
              isname("sheathing", tmp_obj->text.keywords))
            send_to_char(ch, "dermal sheathing\r\n");
          else
            send_to_char(ch, "%s\r\n",
                         cyberware_names[GET_OBJ_VAL(tmp_obj, 2)]);
      }
    }
  }

  found = FALSE;
  for (tmp_obj = i->bioware; tmp_obj && !found; tmp_obj = tmp_obj->next_content)
    if (GET_OBJ_VAL(tmp_obj, 2) == 2)
      found = TRUE;

  if (found)
  {
    act("\r\nVisible bioware:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->bioware; tmp_obj; tmp_obj = tmp_obj->next_content)
      if (GET_OBJ_VAL(tmp_obj, 2) == 2)
        send_to_char(ch, "%s\r\n", bioware_names[GET_OBJ_VAL(tmp_obj, 2)]);
  }

  if ( GET_REAL_LEVEL( ch ) >= LVL_BUILDER )
  {
    found = FALSE;
    for (tmp_obj = i->bioware; tmp_obj && !found; tmp_obj = tmp_obj->next_content) {
      if (GET_OBJ_VAL(tmp_obj, 2) == 2)
        ;
      else
        found = TRUE;
    }

    if (found) {
      act("\r\nVisible bioware:", FALSE, i, 0, ch, TO_VICT);
      for (tmp_obj = i->bioware; tmp_obj; tmp_obj = tmp_obj->next_content) {
        if (GET_OBJ_VAL(tmp_obj, 2) == 2)
          ;
        else
          send_to_char(ch, "%s\r\n", bioware_names[GET_OBJ_VAL(tmp_obj, 2)]);
      }
    }
  }

  if (ch != i && (GET_REAL_LEVEL(ch) >= LVL_BUILDER))
  {
    found = FALSE;
    act("\r\nYou peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj)) {
        show_obj_to_char(tmp_obj, ch, 1);
        found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
}

void list_one_char(struct char_data * i, struct char_data * ch)
{
  struct obj_data *obj = NULL;
  if (IS_NPC(i) && i->player.physical_text.room_desc &&
      GET_POS(i) == GET_DEFAULT_POS(i))
  {
    if (IS_AFFECTED(i, AFF_INVISIBLE))
      strcpy(buf, "*");
    else
      *buf = '\0';

    if (IS_ASTRAL(ch) || IS_DUAL(ch)) {
      if (IS_ASTRAL(i))
        strcat(buf, "(astral) ");
      else if (IS_DUAL(i) && IS_NPC(i))
        strcat(buf, "(dual) ");
    }

    strcat(buf, i->player.physical_text.room_desc);
    send_to_char(buf, ch);

    if (IS_AFFECTED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

    return;
  }
  make_desc(ch, i, buf, FALSE);
  if (PRF_FLAGGED(i, PRF_AFK))
    strcat(buf, " (AFK)");
  if (PLR_FLAGGED(i, PLR_SWITCHED))
    strcat(buf, " (switched)");
  if (IS_AFFECTED(i, AFF_INVISIBLE) || IS_AFFECTED(i, AFF_IMP_INVIS))
    strcat(buf, " (invisible)");
  if (IS_AFFECTED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc &&
      !PLR_FLAGS(i).AreAnySet(PLR_MATRIX, PLR_PROJECT, PLR_SWITCHED, ENDBIT))
    strcat(buf, " (linkless)");
  if (IS_ASTRAL(i) && (IS_ASTRAL(ch) || IS_DUAL(ch)))
    strcat(buf, " (astral)");
  if (IS_NPC(i) && IS_DUAL(i) && (IS_ASTRAL(ch) || IS_DUAL(ch)))
    strcat(buf, " (dual)");
  if (PLR_FLAGGED(i, PLR_WRITING))
    strcat(buf, " (writing)");
  if (PLR_FLAGGED(i, PLR_MAILING))
    strcat(buf, " (mailing)");
  if (PLR_FLAGGED(i, PLR_EDITING))
    strcat(buf, " (editing)");
  if (PLR_FLAGGED(i, PLR_PROJECT))
    strcat(buf, " (projecting)");

  if (PLR_FLAGGED(i, PLR_MATRIX))
    strcat(buf, " is jacked into a cyberdeck.");
  else if (PLR_FLAGGED(i, PLR_REMOTE))
    strcat(buf, " is jacked into a remote control deck.");
  else if (AFF_FLAGGED(i, AFF_DESIGN) || AFF_FLAGGED(i, AFF_PROGRAM))
    strcat(buf, " is typing away at a computer.");
  else if (AFF_FLAGGED(i, AFF_PILOT))
  {
    if (AFF_FLAGGED(i, AFF_RIG))
      strcat(buf, " is plugged into the dashboard.");
    else
      strcat(buf, " is sitting in the drivers seat.");
  } else if (AFF_FLAGGED(i, AFF_MANNING))
  {
    for (obj = i->in_veh->mount; obj; obj = obj->next_content)
      if (obj->worn_by == i)
        break;
    sprintf(buf, "%s is manning a %s.", buf, GET_OBJ_NAME(obj));
  } else if (GET_POS(i) != POS_FIGHTING)
  {
    strcat(buf, positions[(int) GET_POS(i)]);
    if (GET_DEFPOS(i))
      sprintf(buf2, ", %s.", GET_DEFPOS(i));
    else
      sprintf(buf2, ".");
    strcat(buf, buf2);
  } else
  {
    if (FIGHTING(i)) {
      strcat(buf, " is here, fighting ");
      if (FIGHTING(i) == ch)
        strcat(buf, "YOU!");
      else {
        if (i->in_room == FIGHTING(i)->in_room)
          strcat(buf, PERS(FIGHTING(i), ch));
        else
          strcat(buf, "someone in the distance");
        strcat(buf, "!");
      }
    } else if (FIGHTING_VEH(i)) {
      strcat(buf, " is here, fighting ");
      if ((ch->in_veh && ch->in_veh == FIGHTING_VEH(i)) || (ch->char_specials.rigging && ch->char_specials.rigging == FIGHTING_VEH(i)))
        strcat(buf, "YOU!");
      else {
        if (i->in_room == FIGHTING_VEH(i)->in_room)
          strcat(buf, FIGHTING_VEH(i)->short_description);
        else
          strcat(buf, "someone in the distance");
        strcat(buf, "!");
      }
    } else                      /* NIL fighting pointer */
      strcat(buf, " is here struggling with thin air.");
  }

  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;
  struct veh_data *veh;

  if (ch->in_veh && ch->in_room == NOWHERE)
  {
    for (i = list; i; i = i->next_in_veh)
      if (ch != i)
        if (CAN_SEE(ch, i))
          list_one_char(i, ch);
  } else
  {
    for (i = list; i; i = i->next_in_room) {
      if ((ch->in_veh || (ch->char_specials.rigging))) {
        RIG_VEH(ch, veh);
        if (veh->cspeed > SPEED_IDLE)
          if (get_speed(veh) >= 200)
            if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 7))
              continue;
            else if (get_speed(veh) < 200 && get_speed(veh) >= 120)
              if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6))
                continue;
              else if (get_speed(veh) < 120 && get_speed(veh) >= 60)
                if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5))
                  continue;
                else
                  if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4))
                    continue;
      }
      if (ch != i || ch->char_specials.rigging)
        if (CAN_SEE(ch, i))
          list_one_char(i, ch);
    }
  }
}

void do_auto_exits(struct char_data * ch)
{
  int door;
  struct veh_data *veh;
  *buf = '\0';

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE)
    {
      if (ch->in_veh || ch->char_specials.rigging) {
        RIG_VEH(ch, veh);
        if (!ROOM_FLAGGED(EXIT(veh, door)->to_room, ROOM_ROAD) &&
            !ROOM_FLAGGED(EXIT(veh, door)->to_room, ROOM_GARAGE) && !IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN))
          sprintf(buf, "%s(%s) ", buf, exitdirs[door]);
        else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED | EX_HIDDEN))
          sprintf(buf, "%s%s ", buf, exitdirs[door]);
      } else {
        if (!IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN) || GET_LEVEL(ch) > LVL_MORTAL) {
          if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
            sprintf(buf, "%s%s(L) ", buf, exitdirs[door]);
          else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
            sprintf(buf, "%s%s(C) ", buf, exitdirs[door]);
          else
            sprintf(buf, "%s%s ", buf, exitdirs[door]);
        }
      }
    }

  sprintf(buf2, "^c[ Exits: %s]^n\r\n", *buf ? buf : "None! ");

  send_to_char(buf2, ch);
}


ACMD(do_exits)
{
  int door;
  vnum_t wasin = NOWHERE;
  struct veh_data *veh;
  *buf = '\0';
  buf2[0] = '\0'; // so strcats will start at the beginning

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  RIG_VEH(ch, veh);
  if (veh) {
    wasin = ch->in_room;
    ch->in_room = veh->in_room;
  }
  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
        (IS_ASTRAL(ch) || IS_SPIRIT(ch) || GET_REAL_LEVEL(ch) >= LVL_BUILDER ||
         !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))) {
      if (GET_REAL_LEVEL(ch) >= LVL_BUILDER) {
        sprintf(buf2, "%-5s - [%5ld] %s%s\r\n", dirs[door],
                world[EXIT(ch, door)->to_room].number,
                world[EXIT(ch, door)->to_room].name,
                (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) ? " (closed)" : ""));
      } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN)) {
        sprintf(buf2, "%-5s - ", dirs[door]);
        if (!IS_ASTRAL(ch) &&
            ((IS_DARK(EXIT(ch, door)->to_room) && !IS_AFFECTED(ch, AFF_INFRAVISION)) ||
             (IS_LOW(EXIT(ch, door)->to_room) &&
              !IS_AFFECTED(ch, AFF_INFRAVISION | AFF_LOW_LIGHT))))
          strcat(buf2, "Too dark to tell.\r\n");
        else {
          strcat(buf2, world[EXIT(ch, door)->to_room].name);
          if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) && IS_ASTRAL(ch))
            strcat(buf2, " (closed)\r\n");
          else
            strcat(buf2, "\r\n");
        }
      }
      strcat(buf, CAP(buf2));
    }
  }

  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
  if (veh)
    ch->in_room = wasin;
}

void update_blood(void)
{
  int i;
  extern rnum_t top_of_world;

  for (i = 0; i < top_of_world; i++)
    if (RM_BLOOD(i) > 0) {
      RM_BLOOD(i)--;
      if (!ROOM_FLAGGED(i, ROOM_INDOORS)) {
        if (weather_info.sky == SKY_RAINING)
          RM_BLOOD(i)--;
        else if (weather_info.sky == SKY_LIGHTNING)
          RM_BLOOD(i) -= 2;
      }
    }
}

void look_in_veh(struct char_data * ch)
{
  int was_in = NOWHERE;
  if (!(AFF_FLAGGED(ch, AFF_RIG) || PLR_FLAGGED(ch, PLR_REMOTE)))
  {
    send_to_char(ch, "^CInside %s^n\r\n", ch->in_veh->short_description, ch);
    send_to_char(ch, ch->in_veh->inside_description);
    CCHAR = "^g";
    CGLOB = KGRN;
    list_obj_to_char(ch->in_veh->contents, ch, 0, FALSE, FALSE);
    CGLOB = KNRM;
    CCHAR = NULL;
    list_char_to_char(ch->in_veh->people, ch);
  }
  if (ch->in_room == NOWHERE || PLR_FLAGGED(ch, PLR_REMOTE))
  {
    struct veh_data *veh;
    RIG_VEH(ch, veh);
    send_to_char(ch, "\r\n^CAround you is %s\r\n", world[veh->in_room].name);
    if (get_speed(veh) <= 200)
      send_to_char(ch, world[veh->in_room].description);
    if (PLR_FLAGGED(ch, PLR_REMOTE))
      was_in = ch->in_room;
    ch->in_room = veh->in_room;
    do_auto_exits(ch);
    CCHAR = "^g";
    CGLOB = KGRN;
    list_obj_to_char(world[veh->in_room].contents, ch, 0, FALSE, FALSE);
    CGLOB = KNRM;
    CCHAR = NULL;
    list_char_to_char(world[veh->in_room].people, ch);
    CCHAR = "^y";
    list_veh_to_char(world[veh->in_room].vehicles, ch);
    if (PLR_FLAGGED(ch, PLR_REMOTE))
      ch->in_room = was_in;
    else
      ch->in_room = NOWHERE;
  }
}

void look_at_room(struct char_data * ch, int ignore_brief)
{
  if (!ch->in_veh && ch->in_room == NOWHERE)
  {
    send_to_char(ch, "Please alert an imm immediately.\r\n");
    return;
  }
  if (IS_AFFECTED(ch, AFF_BLIND))
  {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
    // Modified for streetlight
  } else if (!LIGHT_OK(ch))
  {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  }

  // Streetlight code
  if ((ch->in_veh && ch->in_room == NOWHERE) || PLR_FLAGGED(ch, PLR_REMOTE))
  {
    look_in_veh(ch);
    return;
  } else
  {
    if ((PRF_FLAGGED(ch, PRF_ROOMFLAGS) && GET_REAL_LEVEL(ch) >= LVL_BUILDER)) {
      ROOM_FLAGS(ch->in_room).PrintBits(buf, MAX_STRING_LENGTH,
                                        room_bits, ROOM_MAX);
      sprintf(buf2, "^C[%5ld] %s [ %s]^n", world[ch->in_room].number,
              world[ch->in_room].name, buf);
      send_to_char(buf2, ch);
    } else
      send_to_char(ch, "^C%s^n", world[ch->in_room].name, ch);
  }
  send_to_char("\r\n", ch);

  if (!(ch->in_veh && get_speed(ch->in_veh) > 200))
    send_to_char(world[ch->in_room].description, ch);


  /* autoexits */
  if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);

  /* blood */
  if (RM_BLOOD(ch->in_room) > 0)
    send_to_char(blood_messages[(int)RM_BLOOD(ch->in_room)], ch);
  /* blood */
  /* rain */
  if (!ROOM_FLAGGED(ch->in_room, ROOM_INDOORS) && SECT(ch->in_room) != SECT_WATER_SWIM)
    if (weather_info.sky >= SKY_RAINING)
    {
      send_to_char(ch, "^cRain splashes into the puddles around your feet.^n\r\n");
    } else if (weather_info.lastrain < 5)
    {
      send_to_char(ch, "^cThe ground is wet, it must have rained recently.^n\r\n");
    }
  /* now list characters & objects */
  // what fun just to get a colorized listing
  CCHAR = "^g";
  CGLOB = KGRN;
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE, FALSE);
  CGLOB = KNRM;
  CCHAR = NULL;
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE, TRUE);
  list_char_to_char(world[ch->in_room].people, ch);
  CCHAR = "^y";
  list_veh_to_char(world[ch->in_room].vehicles, ch);
}

void look_in_direction(struct char_data * ch, int dir)
{
  bool found = FALSE;

  if (EXIT(ch, dir))
  {
    if (IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN))
      if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), EXIT(ch, dir)->hidden)) {
        send_to_char("You see nothing special.\r\n", ch);
        return;
      } else {
        REMOVE_BIT(EXIT(ch, dir)->exit_info, EX_HIDDEN);
        found = TRUE;
      }

    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (found)
      send_to_char("You discover an exit...\r\n", ch);

    if (IS_SET(EXIT(ch, dir)->exit_info, EX_DESTROYED) && EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s is destroyed.\r\n", fname(EXIT(ch, dir)->keyword));
    else if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) && EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
    else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) && EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
  } else
    send_to_char("You see nothing special.\r\n", ch);
}

void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  struct veh_data *veh = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
                                 FIND_OBJ_EQUIP, ch, &dummy, &obj))
           && !(!ch->in_veh &&(veh = get_veh_list(arg,
                                                  world[ch->in_room].vehicles))))
  {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if (veh)
  {
    if (veh->cspeed > SPEED_IDLE) {
      if (success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 8)) {
        ch->in_veh = veh;
        look_in_veh(ch);
        ch->in_veh = NULL;
      } else
        send_to_char(ch, "It's moving too fast for you to get a good look inside.\r\n");
    } else {
      sprintf(buf, "%s peers inside.\r\n", GET_NAME(ch));
      send_to_veh(buf, veh, NULL, TRUE);
      ch->in_veh = veh;
      look_in_veh(ch);
      ch->in_veh = NULL;
    }
    return;
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
             (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
             (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) &&
             (GET_OBJ_TYPE(obj) != ITEM_QUIVER) &&
             (GET_OBJ_TYPE(obj) != ITEM_HOLSTER) &&
             (GET_OBJ_TYPE(obj) != ITEM_WORN))
    send_to_char("There's nothing inside that!\r\n", ch);
  else
  {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER || GET_OBJ_TYPE(obj) == ITEM_HOLSTER ||
        GET_OBJ_TYPE(obj) == ITEM_WORN || GET_OBJ_TYPE(obj) == ITEM_QUIVER) {
      if(IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
        send_to_char("It is closed.\r\n", ch);
        return;
      } else {
        send_to_char(GET_OBJ_NAME(obj), ch);

        switch (bits) {
        case FIND_OBJ_INV:
          send_to_char(" (carried): \r\n", ch);
          break;
        case FIND_OBJ_ROOM:
          send_to_char(" (here): \r\n", ch);
          break;
        case FIND_OBJ_EQUIP:
          send_to_char(" (used): \r\n", ch);
          break;
        }

        list_obj_to_char(obj->contains, ch, 2, TRUE, FALSE);
      }
    } else {            /* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
        send_to_char("It is empty.\r\n", ch);
      else {
        amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
        sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt],
                color_liquid[GET_OBJ_VAL(obj, 2)]);
        send_to_char(buf, ch);
      }
    }
  }
}

char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return NULL;
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = 0, j;
  int was_in = ch->in_room;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  struct veh_data *found_veh = NULL;
  char *desc;

  if (!*arg)
  {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  if (ch->char_specials.rigging)
    if (ch->char_specials.rigging->type == VEH_DRONE)
      ch->in_room = ch->char_specials.rigging->in_room;

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
                      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  if ((!str_cmp(arg, "self") || !str_cmp(arg, "me")))
    if (AFF_FLAGGED(ch, AFF_RIG))
    {
      send_to_char(ch->in_veh->long_description, ch);
      return;
    } else if (PLR_FLAGGED(ch, PLR_REMOTE))
    {
      send_to_char(ch->char_specials.rigging->long_description, ch);
      ch->in_room = was_in;
      return;
    }

  if (!ch->in_veh)
  {
    found_veh = get_veh_list(arg, world[ch->in_room].vehicles);

    if (found_veh) {
      send_to_char(found_veh->long_description, ch);
      if (PLR_FLAGGED(ch, PLR_REMOTE))
        ch->in_room = was_in;
      return;
    }
  }
  /* Is the target a character? */
  if (found_char != NULL)
  {
    look_at_char(found_char, ch);
    if (ch != found_char && !ch->char_specials.rigging) {
      if (CAN_SEE(found_char, ch))
        act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    ch->in_room = was_in;
    return;
  } else if (ch->in_veh)
  {
    found_char = get_char_veh(ch, arg, ch->in_veh);
    if (found_char) {
      look_at_char(found_char, ch);
      if (ch != found_char) {
        if (CAN_SEE(found_char, ch)) {
          sprintf(buf, "%s looks at you.\r\n", GET_NAME(ch));
          send_to_char(buf, found_char);
        }
        sprintf(buf, "%s looks at %s.\r\n", GET_NAME(ch), GET_NAME(found_char));
        send_to_veh(buf, ch->in_veh, ch, found_char, FALSE);
      }
      ch->in_room = was_in;
      return;
    }
  }
  /* Does the argument match an extra desc in the room? */
  if (ch->in_room != NOWHERE && (desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL)
  {
    page_string(ch->desc, desc, 0);
    ch->in_room = was_in;
    return;
  }
  /* Does the argument match a piece of equipment */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j]) &&
        isname(arg, ch->equipment[j]->text.keywords))
      if (ch->equipment[j]->text.look_desc)
      {
        send_to_char(ch->equipment[j]->text.look_desc, ch);
        found = TRUE;
      }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j]))
      if ((desc = find_exdesc(arg, ch->equipment[j]->ex_description)) != NULL)
      {
        page_string(ch->desc, desc, 1);
        found = 1;
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content)
  {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
        page_string(ch->desc, desc, 1);
        found = 1;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  if (ch->in_room != NOWHERE)
    for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj))
        if ((desc = find_exdesc(arg, obj->ex_description)) != NULL)
        {
          send_to_char(desc, ch);
          found = 1;
        }
  if (bits)
  {                   /* If an object was found back in
                                                                         * generic_find */
    if (!found)
      show_obj_to_char(found_obj, ch, 5);       /* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);       /* Find hum, glow etc */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
  ch->in_room = was_in;

}

ACMD(do_look)
{
  static char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (!LIGHT_OK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
        send_to_char("Read what?\r\n", ch);
      else
        look_at_target(ch, arg);
      return;
    }
    if (!*arg)                  /* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, lookdirs, FALSE)) >= 0)
      look_in_direction(ch, convert_look[look_type]);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}

void look_at_veh(struct char_data *ch, struct veh_data *veh, int success)
{
  strcpy(buf, veh->short_description);
  int cond = 10 - veh->damage;
  if (cond >= 10)
    strcat(buf, " is in perfect condition.\r\n");
  else if (cond >= 9)
    strcat(buf, " has some light scratches.\r\n");
  else if (cond >= 5)
    strcat(buf, " is dented and scratched.\r\n");
  else if (cond >= 3)
    strcat(buf, " has seen better days.\r\n");
  else if (cond >= 1)
    strcat(buf, " is barely holding together.\r\n");
  else
    strcat(buf, " is wrecked.\r\n");
  send_to_char(buf, ch);
  disp_mod(veh, ch, success);
}

ACMD(do_examine)
{
  int bits, i, skill = 0;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  struct veh_data *found_veh;
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  if (!ch->in_veh) {
    found_veh = get_veh_list(arg, world[ch->in_room].vehicles);
    if (found_veh) {
      switch(found_veh->type) {
      case VEH_DRONE:
        skill = GET_SKILL(ch, SKILL_BR_DRONE);
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_BIKE) / 3;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_CAR) / 4;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_TRUCK) / 4;
        break;
      case VEH_BIKE:
        skill = GET_SKILL(ch, SKILL_BR_BIKE);
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_CAR) / 2;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_DRONE) / 3;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_TRUCK) / 4;
        break;
      case VEH_CAR:
        skill = GET_SKILL(ch, SKILL_BR_CAR);
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_TRUCK) / 2;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_BIKE) / 2;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_DRONE) / 4;
        break;
      case VEH_TRUCK:
        skill = GET_SKILL(ch, SKILL_BR_TRUCK);
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_CAR) / 2;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_DRONE) / 4;
        if (!skill)
          skill = GET_SKILL(ch, SKILL_BR_BIKE) / 4;
        break;
      }
      if (GET_IDNUM(ch) == found_veh->owner)
        i = 8;
      else if (found_veh->cspeed > SPEED_IDLE)
        i = success_test(skill + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 8);
      else
        i = success_test(skill + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4);
      look_at_veh(ch, found_veh, i);
    }
  }

  if ((!str_cmp(arg, "self") || !str_cmp(arg, "me")))
    if (AFF_FLAGGED(ch, AFF_RIG)) {
      look_at_veh(ch, ch->in_veh, 10);
      return;
    } else if (PLR_FLAGGED(ch, PLR_REMOTE)) {
      look_at_veh(ch, ch->char_specials.rigging, 10);
      return;
    }

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
                      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);


  if (tmp_object) {
    // this cyberdeck bit donated by Chipjack, modified by AH:
    if (GET_OBJ_TYPE(tmp_object) == ITEM_CYBERDECK) {
      switch (success_test(GET_SKILL(ch, SKILL_COMPUTER), 6)) {
      case 0:
      case 1:
        send_to_char("You cannot determine the quality of this  cyberdeck.\r\n", ch);

        break;
      case 2:
      case 3:
        if (GET_OBJ_VAL(tmp_object, 0) < 4)
          send_to_char("This cyberdeck will get you by.\r\n", ch);
        else if (GET_OBJ_VAL(tmp_object, 0) < 8)
          send_to_char("This cyberdeck is moderate in ability.\r\n", ch);
        else
          send_to_char("This cyberdeck is of exceptional quality.\r\n",  ch);

        break;
      default:
        sprintf(buf, "MPCP: %d, Hardening: %d, Active: %d, Storage: %d, Load: %d\r\n",
                GET_OBJ_VAL(tmp_object, 0), GET_OBJ_VAL(tmp_object, 1),
                GET_OBJ_VAL(tmp_object, 2), GET_OBJ_VAL(tmp_object, 3),
                GET_OBJ_VAL(tmp_object, 4));
        send_to_char(buf, ch);

        break;
      }
    } else
      if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
          (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
          (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER) ||
          (GET_OBJ_TYPE(tmp_object) == ITEM_WORN)) {
        send_to_char("When you look inside, you see:\r\n", ch);
        look_in_obj(ch, arg);
      } else
        if ((GET_OBJ_TYPE(tmp_object) == ITEM_WEAPON) &&
            (GET_OBJ_VAL(tmp_object, 3) >= TYPE_PISTOL)) {
          for (i = 7; i < 10; ++i)
            if (GET_OBJ_VAL(tmp_object, i) > 0) {
              sprintf(buf1, "There is %s attached to the %s of it.\r\n",
                      short_object(GET_OBJ_VAL(tmp_object, i), 2), (i == 7 ? "top" :
                          (i == 8 ? "barrel" : "bottom")));
              send_to_char(buf1, ch);
            }
          if (tmp_object->contains)
            sprintf(buf, "It contains %d round%s of %s ammunition, and can hold %d round%s.\r\n",
                    MIN(GET_OBJ_VAL(tmp_object->contains, 9), GET_OBJ_VAL(tmp_object, 5)),
                    (GET_OBJ_VAL(tmp_object->contains, 9) != 1 ? "s" : ""), ammo_type[GET_OBJ_VAL(tmp_object->contains, 2)],
                    GET_OBJ_VAL(tmp_object, 5), (GET_OBJ_VAL(tmp_object, 5) != 1 ? "s" : ""));
          else
            sprintf(buf, "It does not contain any ammunition, but looks to hold %d round%s.\r\n",
                    GET_OBJ_VAL(tmp_object, 5), (GET_OBJ_VAL(tmp_object, 5) != 1 ? "s" : ""));
          if (GET_OBJ_VAL(tmp_object, 5) > 0)
            send_to_char(buf, ch);
        } else if (GET_OBJ_TYPE(tmp_object) == ITEM_MONEY) {
          if (!GET_OBJ_VAL(tmp_object, 1))    // paper money
            send_to_char(ch, "There looks to be about %d nuyen.\r\n", GET_OBJ_VAL(tmp_object, 0));
          else {
            if (GET_OBJ_VAL(tmp_object, 4)) {
              if (belongs_to(ch, tmp_object))
                send_to_char("It has been activated by you.\r\n", ch);
              else
                send_to_char("It has been activated.\r\n", ch);
            } else
              send_to_char("It has not been activated.\r\n", ch);
            send_to_char(ch, "The account display shows %d nuyen.\r\n", GET_OBJ_VAL(tmp_object, 0));
          }
        } else if (GET_OBJ_TYPE(tmp_object) == ITEM_SPELL_FORMULA)
          send_to_char(ch, "It appears to be a %s spell.\r\n",
                       (GET_OBJ_VAL(tmp_object, 7) ? "shaman" : "mage"));
        else if (GET_OBJ_TYPE(tmp_object) == ITEM_GUN_CLIP)
          send_to_char(ch, "It has %d round%s left.", GET_OBJ_VAL(tmp_object, 9),
                       GET_OBJ_VAL(tmp_object, 9) != 1 ? "s" : "");
    get_obj_condition(ch, tmp_object);
  }
}

ACMD(do_gold)
{
  if (GET_NUYEN(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_NUYEN(ch) == 1)
    send_to_char("You have one miserable nuyen.\r\n", ch);
  else {
    sprintf(buf, "You have %d nuyen.\r\n", GET_NUYEN(ch));
    send_to_char(buf, ch);
  }
}

ACMD(do_pool)
{
  char pools[MAX_INPUT_LENGTH];

  sprintf(pools, "  Combat: %d     (Defense: %d     Offense: %d)\r\n",
          GET_COMBAT(ch), GET_DEFENSE(ch), GET_OFFENSE(ch));
  if (GET_ASTRAL(ch) > 0)
    sprintf(pools, "%s  Astral: %d\r\n", pools, GET_ASTRAL(ch));
  if (GET_HACKING(ch) > 0)
    sprintf(pools, "%s  Hacking: %d\r\n", pools, GET_HACKING(ch));
  if (GET_MAGIC(ch) > 0)
    sprintf(pools, "%s  Magic: %d\r\n", pools, GET_MAGIC(ch));
  if (GET_CONTROL(ch) > 0)
    sprintf(pools, "%s  Control: %d\r\n", pools, GET_CONTROL(ch));
  send_to_char(pools, ch);
}

ACMD(do_score)
{
  extern int has_mail(long); // to check to see if they have mail

  struct time_info_data playing_time;
  struct time_info_data real_time_passed(time_t t2, time_t t1);

  if ( IS_NPC(ch) && ch->desc == NULL )
    return;

  else if (ch->desc != NULL && ch->desc->original != NULL ) {
    if (PLR_FLAGGED(ch->desc->original, PLR_MATRIX))
      sprintf(buf, "You are connected to the Matrix.");
    else if (IS_PROJECT(ch))
      sprintf(buf, "You are astrally projecting.");
    else
      sprintf(buf, "You are occupying the body of %s.", GET_NAME(ch));
  }

  if (AFF_FLAGGED(ch, AFF_RIG) || PLR_FLAGGED(ch, PLR_REMOTE)) {
    struct veh_data *veh;
    RIG_VEH(ch, veh);
    sprintf(buf, "You are rigging %s.\r\n", veh->short_description);
    sprintf(buf, "%s  Damage:^R%3d/10^n      Mental:^B%3d(%2d)^n\r\n"
            "  Reaction:%3d      Int:%3d\r\n"
            "       Wil:%3d      Bod:%3d\r\n"
            "     Armor:%3d  Autonav:%3d\r\n"
            "  Handling:%3d    Speed:%3d\r\n"
            "     Accel:%3d      Sig:%3d\r\n"
            "   Sensors:%3d\r\n", buf,
            veh->damage, (int)(GET_MENTAL(ch) / 100),
            (int)(GET_MAX_MENTAL(ch) / 100), GET_REA(ch), GET_INT(ch),
            GET_WIL(ch), veh->body, veh->armor, veh->autonav,
            veh->handling, veh->speed, veh->accel, veh->sig,
            veh->sensor);

  } else {
    sprintf(buf, "^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//"
            "^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//"
            "^b//^L^L//^b//^L//^b//^L//^b//^L//^b//^L/\r\n^b/^L/"
            "  ^L \\_\\                                 ^rconditi"
            "on monitor           ^L/^b/\r\n");

    sprintf(buf, "%s^L/^b/^L `//-\\\\                      ^gMent: ", buf);
    if (GET_MENTAL(ch) >= 900 && GET_MENTAL(ch) < 1000)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[^gL^b]", buf);
    if (GET_MENTAL(ch) >= 800 && GET_MENTAL(ch) < 900)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[ ]", buf);
    if (GET_MENTAL(ch) >= 700 && GET_MENTAL(ch) < 800)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[^yM^b]", buf);
    if (GET_MENTAL(ch) >= 600 && GET_MENTAL(ch) < 700)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[ ]", buf);
    if (GET_MENTAL(ch) >= 500 && GET_MENTAL(ch) < 600)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[ ]", buf);
    if (GET_MENTAL(ch) >= 400 && GET_MENTAL(ch) < 500)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[^rS^b]", buf);
    if (GET_MENTAL(ch) >= 300 && GET_MENTAL(ch) < 400)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[ ]", buf);
    if (GET_MENTAL(ch) >= 200 && GET_MENTAL(ch) < 300)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[ ]", buf);
    if (GET_MENTAL(ch) >= 100 && GET_MENTAL(ch) < 200)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[ ]", buf);
    if (GET_MENTAL(ch) < 100)
      sprintf(buf, "%s^b[^R*^b]", buf);
    else
      sprintf(buf, "%s^b[^RD^b]", buf);
    sprintf( buf, "%s  ^b/^L/\r\n", buf);

    sprintf(buf, "%s^b/^L/ ^L`\\\\-\\^wHADOWRUN 3rd Edition   ^rPhys: ", buf);
    if (GET_PHYSICAL(ch) >= 900 && GET_PHYSICAL(ch) < 1000)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[^gL^L]", buf);
    if (GET_PHYSICAL(ch) >= 800 && GET_PHYSICAL(ch) < 900)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[ ]", buf);
    if (GET_PHYSICAL(ch) >= 700 && GET_PHYSICAL(ch) < 800)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[^yM^L]", buf);
    if (GET_PHYSICAL(ch) >= 600 && GET_PHYSICAL(ch) < 700)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[ ]", buf);
    if (GET_PHYSICAL(ch) >= 500 && GET_PHYSICAL(ch) < 600)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[ ]", buf);
    if (GET_PHYSICAL(ch) >= 400 && GET_PHYSICAL(ch) < 500)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[^rS^L]", buf);
    if (GET_PHYSICAL(ch) >= 300 && GET_PHYSICAL(ch) < 400)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[ ]", buf);
    if (GET_PHYSICAL(ch) >= 200 && GET_PHYSICAL(ch) < 300)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[ ]", buf);
    if (GET_PHYSICAL(ch) >= 100 && GET_PHYSICAL(ch) < 200)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[ ]", buf);
    if (GET_PHYSICAL(ch) < 100)
      sprintf(buf, "%s^L[^R*^L]", buf);
    else
      sprintf(buf, "%s^L[^RD^L]", buf);
    sprintf( buf, "%s  ^L/^b/\r\n", buf);

    sprintf(buf, "%s^L/^b/  ^L///-\\  ^wcharacter sheet           ^LPhysical Damage Overflow: ^R[", buf);
    if (GET_PHYSICAL(ch) < 0)
      sprintf(buf, "%s%2d]  ^b/^L/\r\n", buf, (int)(GET_PHYSICAL(ch) / 100) * -1);
    else
      sprintf( buf, "%s 0]  ^b/^L/\r\n", buf);


    sprintf(buf, "%s^b/^L/  ^L\\\\@//                        "
            "                                    ^L/^b/\r\n", buf);

    sprintf(buf, "%s^L/^b/   ^L`^                            "
            "                                  ^b/^L/\r\n", buf);
    sprintf(buf, "%s^b/^L/                                  "
            "                                 ^L/^b/\r\n"
            "^L/^b/ ^nBody          ^w%2d (^W%2d^w)"
            "    Height: ^W%0.2f^w meters   Weight: ^W%3d^w kilos  ^b/^L/\r\n",
            buf, GET_REAL_BOD(ch), GET_BOD(ch), ((float)GET_HEIGHT(ch) / 100), GET_WEIGHT(ch));
    sprintf(buf, "%s^b/^L/ ^nQuickness     ^w%2d (^W%2d^w)"
            "    Encumbrance: ^W%3.2f^w kilos carried, ^W%3d^w max ^L/^b/\r\n",
            buf, GET_REAL_QUI(ch), GET_QUI(ch), IS_CARRYING_W(ch) ,CAN_CARRY_W(ch));
    playing_time = real_time_passed((time(0) - ch->player.time.logon) +
                                    ch->player.time.played, 0);
    sprintf(buf, "%s^L/^b/ ^nStrength      ^w%2d (^W%2d^w)"
            "    You have played for ^W%2d^w days, ^W%2d^w hours.   ^b/^L/\r\n",
            buf, GET_REAL_STR(ch), GET_STR(ch), playing_time.day, playing_time.hours);
    sprintf(buf, "%s^b/^L/ ^nCharisma      ^w%2d (^W%2d^w)"
            "    ^wKarma ^B[^W%7.2f^B] ^wRep ^B[^W%4d^B] ^rNotor ^r[^R%4d^r]  ^L/^b/\r\n",
            buf, GET_REAL_CHA(ch), GET_CHA(ch), ((float)GET_KARMA(ch) / 100), GET_REP(ch), GET_NOT(ch));
    if (PLR_FLAGGED(ch, PLR_PERCEIVE))
      strcpy(buf2, "You are astrally perceiving.");
    else if (IS_AFFECTED(ch, AFF_INFRAVISION))
      strcpy(buf2, "You have thermographic vision.");
    else if (IS_AFFECTED(ch, AFF_LOW_LIGHT))
      strcpy(buf2, "You have low-light vision.");
    else
      strcpy(buf2, "");

    sprintf(buf, "%s^L/^b/ ^nIntelligence  ^w%2d (^W%2d^w)"
            "    ^r%-33s        ^b/^L/\r\n", buf,
            GET_REAL_INT(ch), GET_INT(ch), buf2);


    switch (GET_POS(ch)) {
    case POS_DEAD:
      strcpy(buf2, "DEAD!");
      break;
    case POS_MORTALLYW:
      strcpy(buf2, "mortally wounded!  You should seek help!");
      break;
    case POS_STUNNED:
      strcpy(buf2, "stunned!  You can't move!");
      break;
    case POS_SLEEPING:
      strcpy(buf2, "sleeping.");
      break;
    case POS_RESTING:
      strcpy(buf2, "resting.");
      break;
    case POS_SITTING:
      strcpy(buf2, "sitting.");
      break;
    case POS_FIGHTING:
      if (FIGHTING(ch))
        sprintf(buf2, "fighting %s.",PERS(FIGHTING(ch), ch));
      else
        strcpy(buf2, "fighting thin air.");
      break;
    case POS_STANDING:
      if (SECT(ch->in_room) == SECT_WATER_SWIM)
        strcpy(buf2, "swimming.");
      else
        strcpy(buf2, "standing.");
      break;
    case POS_LYING:
      strcpy(buf2, "lying down.");
      break;
    default:
      strcpy(buf2, "floating.");
      break;
    }
    sprintf(buf, "%s^b/^L/ ^nWillpower     ^w%2d (^W%2d^w)"
            "    ^nYou are %-33s^L/^b/\r\n", buf,
            GET_REAL_WIL(ch), GET_WIL(ch), buf2);
    if (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED(ch, AFF_IMP_INVIS))
      strcpy(buf2, "You are invisible.");
    else
      strcpy(buf2, "");
    sprintf(buf, "%s^L/^b/ ^nEssence Index ^W[^w%5.2f^W]    "
            "^W%-18s                       ^b/^L/\r\n",
            buf, ((float)(GET_ESS(ch) / 100) + 3), buf2);

    if (GET_COND(ch, FULL) == 0)
      strcpy(buf2, "You are hungry.");
    else
      strcpy(buf2, "");
    sprintf(buf, "%s^b/^L/ ^nBioware Index ^B[^w%5.2f^B]    "
            "^n%-15s                          ^L/^b/\r\n",
            buf, ((float)GET_INDEX(ch) / 100), buf2);

    if (GET_COND(ch, THIRST) == 0)
      strcpy(buf2, "You are thirsty.");
    else
      strcpy(buf2, "");
    sprintf(buf, "%s^L/^b/ ^nEssence       ^g[^w%5.2f^g]    "
            "^n%-16s                         ^b/^L/\r\n",
            buf, ((float)GET_ESS(ch) / 100), buf2);

    if (GET_COND(ch, DRUNK) > 10)
      strcpy(buf2, "You are intoxicated.");
    else
      strcpy(buf2, "");
    sprintf(buf, "%s^b/^L/ ^nMagic         ^w%2d (^W%2d^w)    "
            "^g%-20s                     ^L/^b/\r\n",
            buf, ((int)ch->real_abils.mag / 100), ((int)GET_MAG(ch) / 100), buf2);

    if (IS_AFFECTED(ch, AFF_POISON))
      strcpy(buf2, "You are poisoned!");
    else
      strcpy(buf2, "");

    sprintf(buf, "%s^L/^b/ ^nReaction      ^w%2d (^W%2d^w)    "
            "^b%-17s                        ^b/^L/\r\n", buf,
            GET_REAL_REA(ch), GET_REA(ch), buf2);

    if (GET_TRADITION(ch) == TRAD_SHAMANIC)
      sprintf(buf2, "You follow the %s.", totem_types[GET_TOTEM(ch)]);
    else
      strcpy(buf2, "");
    sprintf(buf, "%s^b/^L/ ^nArmor     ^w[ ^W%2d^rB^w/ ^W%2d^rI^w]    "
            "^n%-32s         ^L/^b/\r\n", buf,
            GET_BALLISTIC(ch), GET_IMPACT(ch), buf2);

    if (GET_TRADITION(ch) != TRAD_MUNDANE)
      sprintf(buf2, "^nGrade: ^w[^W%2d^w]", GET_GRADE(ch));
    else
      strcpy(buf2, "");

    sprintf(buf, "%s^L/^b/ ^nNuyen:    ^w[^W%'9d^w]    %-11s"
            "                              ^b/^L/\r\n", buf,
            GET_NUYEN(ch), buf2);

    sprintf(buf, "%s^b/^L/                                  "
            "                                 ^L/^b/\r\n"
            "^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//"
            "^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//^b//^L//"
            "^b//^L^L//^b//^L//^b//^L//^b//^L//^b//^L/\r\n", buf);
  }
  send_to_char(buf, ch);
}

ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE, FALSE);
}

ACMD(do_cyberware)
{
  struct obj_data *obj;

  if (!ch->cyberware) {
    send_to_char("You have no cyberware.\r\n", ch);
    return;
  }

  send_to_char("You have the following cyberware:\r\n", ch);
  for (obj = ch->cyberware; obj != NULL; obj = obj->next_content) {
    sprintf(buf, "%-30s Rating: %-2d     Essence: %0.2f\r\n",
            GET_OBJ_NAME(obj),
            GET_OBJ_VAL(obj, 0), ((float)GET_OBJ_VAL(obj, 1) / 100));
    send_to_char(buf, ch);
  }
}

ACMD(do_bioware)
{
  struct obj_data *obj;

  if (!ch->bioware) {
    send_to_char("You have no bioware.\r\n", ch);
    return;
  }

  send_to_char("You have the following bioware:\r\n", ch);
  for (obj = ch->bioware; obj != NULL; obj = obj->next_content) {
    sprintf(buf, "%-30s Rating: %-2d     Body Index: %0.2f\r\n",
            GET_OBJ_NAME(obj),
            GET_OBJ_VAL(obj, 0), ((float)GET_OBJ_VAL(obj, 1) / 100));
    send_to_char(buf, ch);
  }
}

ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (i == WEAR_WIELD || i == WEAR_HOLD) {
        if (IS_OBJ_STAT(GET_EQ(ch, i), ITEM_TWOHANDS))
          send_to_char(hands[2], ch);
        else if (GET_WIELDED(ch, i - WEAR_WIELD)) { // wielding something?
          if (i == WEAR_WIELD)
            send_to_char(wielding_hands[(int)ch->char_specials.saved.left_handed], ch);
          else
            send_to_char(wielding_hands[!ch->char_specials.saved.left_handed], ch);
        } else {                                              // just held
          if (i == WEAR_WIELD)
            send_to_char(hands[(int)ch->char_specials.saved.left_handed], ch);
          else
            send_to_char(hands[!ch->char_specials.saved.left_handed], ch);
        }
      } else
        send_to_char(where[i], ch);
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
        show_obj_to_char(GET_EQ(ch, i), ch, 7);
      else
        send_to_char("Something.\r\n", ch);
      found = TRUE;
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}

ACMD(do_time)
{
  sh_int year, month, day, hour, minute, pm;
  extern struct time_info_data time_info;
  extern const char *weekdays[];
  extern const char *month_name[];
  struct obj_data *check;

  if (subcmd == SCMD_NORMAL && GET_REAL_LEVEL(ch) >= LVL_BUILDER)
    subcmd = SCMD_PRECISE;
  for (check = ch->cyberware; check; check = check->next_content)
    if (GET_OBJ_VNUM(check) == 38108 || GET_OBJ_VNUM(check) == 38109 || GET_OBJ_VAL(check, 2) == 34)
      subcmd = SCMD_PRECISE;

  year = time_info.year % 100;
  month = time_info.month + 1;
  day = time_info.day + 1;
  hour = (time_info.hours % 12 == 0 ? 12 : time_info.hours % 12);
  minute = time_info.minute;
  pm = (time_info.hours >= 12);

  if (subcmd == SCMD_NORMAL)
    sprintf(buf, "%d o'clock %s, %s, %s %d, %d.\r\n", hour, pm ? "PM" : "AM",
            weekdays[(int)time_info.weekday], month_name[month - 1], day, time_info.year);
  else
    sprintf(buf, "%d:%s%d %s, %s, %s%d/%s%d/%d.\r\n", hour,
            minute < 10 ? "0" : "", minute, pm ? "PM" : "AM",
            weekdays[(int)time_info.weekday], month < 10 ? "0" : "", month,
            day < 10 ? "0" : "", day, year);

  //For Retinal Mod

  send_to_char(buf, ch);
}

ACMD(do_weather)
{
  static char *sky_look[] =
    {
      "cloudless",
      "cloudy",
      "rainy",
      "lit by flashes of lightning"
    };

  if (OUTSIDE(ch)) {
    sprintf(buf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
            (weather_info.change >= 0 ? "you feel a warm wind from south" :
             "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}


// this command sends the index to the player--it can take a letter as an
// argument and will send only words that fall under that letter
ACMD(do_index)
{
  char *temp = argument;

  skip_spaces(&temp);

  if (!*temp) {
    Help.ListIndex(ch, NULL);
    return;
  }

  char letter = *temp;
  if (!isalpha(letter)) {
    send_to_char("Only letters can be sent to the index command.\r\n", ch);
    return;
  }

  letter = tolower(letter);
  Help.ListIndex(ch, &letter);
}

ACMD(do_help)
{
  extern char *help;
  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (Help.FindTopic(buf, argument))
    send_to_char(ch, buf);
  else
    send_to_char("That help topic doesn't exist.\r\n", ch);
}

ACMD(do_wizhelp)
{
  struct char_data *vict = NULL;
  int no, cmd_num;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument || ((vict = get_char_vis(ch, argument)) && !IS_NPC(vict) &&
                     access_level(vict, LVL_BUILDER))) {
    if (!vict)
      vict = ch;
    sprintf(buf, "The following privileged commands are available to %s:\r\n",
            vict == ch ? "you" : GET_CHAR_NAME(vict));

    /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
    for (no = 1, cmd_num = 1; *cmd_info[cmd_num].command != '\n'; cmd_num++)
      if (cmd_info[cmd_num].minimum_level >= LVL_BUILDER
          && GET_LEVEL(vict) >= cmd_info[cmd_num].minimum_level) {
        sprintf(buf + strlen(buf), "%-13s", cmd_info[cmd_num].command);
        if (!(no % 6))
          strcat(buf, "\r\n");
        no++;
      }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }
  if (WizHelp.FindTopic(buf, argument))
    send_to_char(ch, buf);
  else
    send_to_char("That help topic doesn't exist.\r\n", ch);
}

#define WHO_FORMAT "format: who [sort] [quest] [pker] [immortal] [mortal]\r\n"

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  int sort = LVL_MAX, quest = 0, pker = 0, immort = 0, mortal = 0, num_can_see = 0;
  int output_header;

  skip_spaces(&argument);
  strcpy(buf, argument);

  while (*buf) {
    half_chop(buf, arg, buf1);

    if (is_abbrev(arg, "sort"))
      sort = LVL_MAX;
    else if (is_abbrev(arg, "quest"))
      quest = 1;
    else if (is_abbrev(arg, "pker"))
      pker = 1;
    else if (is_abbrev(arg, "immortal"))
      immort = 1;
    else if (is_abbrev(arg, "mortal"))
      mortal = 1;
    else {
      send_to_char(WHO_FORMAT, ch);
      return;
    }
    strcpy(buf, buf1);
  }                             /* end while (parser) */

  *buf = '\0';
  output_header = 1;

  for (; sort != 0; sort > 0 ? sort-- : sort++) {
    if ( sort == 1 )
      output_header = 1;
    for (d = descriptor_list; d; d = d->next) {
      if (d->connected)
        continue;

      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if ((mortal && IS_SENATOR(tch)) ||
          (immort && !IS_SENATOR(tch)))
        continue;
      if (quest && !PRF_FLAGGED(tch, PRF_QUEST))
        continue;
      if (pker && !PRF_FLAGGED(tch, PRF_PKER))
        continue;
      if (sort > 0 && GET_LEVEL(tch) != sort)
        continue;
      if (GET_INCOG_LEV(tch) > ch->player.level)
        continue;
      num_can_see++;

      if ( output_header ) {
        output_header = 0;
        if ( sort >= LVL_BUILDER )
          strcat(buf, "\r\n^W Immortals ^L: ^WStaff Online\r\n ^w---------------------------\r\n");
        else
          strcat(buf, "\r\n     ^W Race ^L: ^WVisible Players\r\n ^W---------------------------\r\n");
      }

      switch (GET_LEVEL(tch)) {
      case LVL_BUILDER:
      case LVL_ARCHITECT:
        sprintf(buf1, "^G");
        break;
      case LVL_FIXER:
      case LVL_CONSPIRATOR:
        sprintf(buf1, "^m");
        break;
      case LVL_EXECUTIVE:
        sprintf(buf1, "^b");
        break;
      case LVL_DEVELOPER:
        sprintf(buf1, "^r");
        break;
      case LVL_VICEPRES:
      case LVL_ADMIN:
        sprintf(buf1, "^b");
        break;
      case LVL_PRESIDENT:
        sprintf(buf1, "^B");
        break;
      default:
        sprintf(buf1, "^L");
        break;
      }
      sprintf(buf2, "%10s :^N %s%s%s %s^n", (GET_WHOTITLE(tch) ? GET_WHOTITLE(tch) : ""),
              (GET_PRETITLE(tch) ? GET_PRETITLE(tch) : ""), (GET_PRETITLE(tch) &&
                  strlen(GET_PRETITLE(tch)) ? " " : ""), GET_CHAR_NAME(tch), GET_TITLE(tch));
      strcat(buf1, buf2);

      if (PRF_FLAGGED(tch, PRF_AFK))
        strcat(buf1, " (AFK)");
      if (PLR_FLAGGED(tch, PLR_RPE) && (GET_LEVEL(ch) > LVL_MORTAL || PLR_FLAGGED(ch, PLR_RPE)))
        strcat(buf1, " ^R(RPE)^n");
      if (PLR_FLAGGED(tch, PLR_PG))
        strcat(buf1, " ^R(Power Gamer)^n");

      if (GET_LEVEL(ch) > LVL_MORTAL) {
        if (GET_INVIS_LEV(tch) && GET_LEVEL(ch) >= GET_INVIS_LEV(tch))
          sprintf(buf1, "%s (i%d)", buf1, GET_INVIS_LEV(tch));
        if (GET_INCOG_LEV(tch))
          sprintf(buf1, "%s (c%d)", buf1, GET_INCOG_LEV(tch));
        if (PLR_FLAGGED(tch, PLR_MAILING))
          strcat(buf1, " (mailing)");
        else if (PLR_FLAGGED(tch, PLR_WRITING))
          strcat(buf1, " (writing)");
        if (PLR_FLAGGED(tch, PLR_EDITING))
          strcat(buf1, " (editing)");
        if (PRF_FLAGGED(tch, PRF_QUESTOR))
          strcat(buf1, " ^Y(questor)^n");
        if (PLR_FLAGGED(tch, PLR_AUTH))
          strcat(buf1, " ^G(unauthed)^n");
        if (PLR_FLAGGED(tch, PLR_MATRIX))
          strcat(buf1, " (decking)");
        else if (PLR_FLAGGED(tch, PLR_PROJECT))
          strcat(buf1, " (projecting)");
        else if (d->original) {
          if (IS_NPC(d->character) && GET_MOB_VNUM(d->character) >= 50 &&
              GET_MOB_VNUM(d->character) < 70)
            strcat(buf1, " (morphed)");
          else
            strcat(buf1, " (switched)");
        }
      }
      if (!quest && PRF_FLAGGED(tch, PRF_QUEST))
        strcat(buf1, " ^Y(hired)^n");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
        strcat(buf1, " (!tell)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
        strcat(buf1, " ^R(KILLER)^N");
      if (PLR_FLAGGED(tch, PLR_WANTED))
        strcat(buf1, " ^R(WANTED)^N");
      strcat(buf1, "\r\n");
      strcat(buf, buf1);

      CGLOB = KNRM;
    }
  }

  if (num_can_see == 0)
    strcat(buf, "No-one at all!\r\n");
  else if (num_can_see == 1)
    strcat(buf, "One lonely chummer displayed.\r\n");
  else
    sprintf(buf, "%s\r\n%d chummers displayed.\r\n", buf, num_can_see);
  page_string(ch->desc, buf, 1);
}

#define USERS_FORMAT "format: users [-n name] [-h host] [-o] [-p]\r\n"

ACMD(do_users)
{
  extern const char *connected_types[];
  char line[200], line2[220], idletime[10];
  char state[30], *timeptr, *format, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int num_can_see = 0;
  int outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
        outlaws = 1;
        playing = 1;
        strcpy(buf, buf1);
        break;
      case 'p':
        playing = 1;
        strcpy(buf, buf1);
        break;
      case 'd':
        deadweight = 1;
        strcpy(buf, buf1);
        break;
      case 'n':
        playing = 1;
        half_chop(buf1, name_search, buf);
        break;
      case 'h':
        playing = 1;
        half_chop(buf1, host_search, buf);
        break;
      default:
        send_to_char(USERS_FORMAT, ch);
        return;
        break;
      }                         /* end of switch */

    } else {                    /* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }                             /* end while (parser) */
  strcpy(line, "Num  Name           State           Idle Login@   Site\r\n");
  strcat(line, "---- -------------- --------------- ---- -------- ---------------------------\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && playing)
      continue;
    if (!d->connected && deadweight)
      continue;
    if (!d->connected) {
      if (d->original)
        tch = d->original;
      else if (!(tch = d->character))
        continue;

      if (*host_search && !strstr((const char *)d->host, host_search))
        continue;
      if (*name_search && isname(name_search, GET_KEYWORDS(tch)))
        continue;
      if (!CAN_SEE(ch, tch))
        continue;
      if (outlaws &&
          !PLR_FLAGGED(tch, PLR_KILLER))
        continue;
      if (GET_INVIS_LEV(tch) > GET_REAL_LEVEL(ch))
        continue;
      if (GET_INCOG_LEV(tch) > GET_REAL_LEVEL(ch))
        continue;
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (!d->connected && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[d->connected]);

    if (d->character && !d->connected)
      sprintf(idletime, "%4d", d->character->char_specials.timer);
    else
      strcpy(idletime, "");

    format = "%-4d %-14s %-15s %-4s %-8s ";

    if (d->character && GET_CHAR_NAME(d->character)) {
      if (d->original)
        sprintf(line, format, d->desc_num, GET_CHAR_NAME(d->original),
                state, idletime, timeptr);
      else
        sprintf(line, format, d->desc_num, GET_CHAR_NAME(d->character),
                state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "UNDEFINED",
              state, idletime, timeptr);

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (d->connected) {
      sprintf(line2, "^g%s^n", line);
      strcpy(line, line2);
    }
    if ((d->connected && !d->character) || CAN_SEE(ch, d->character)) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}

ACMD(do_glist)
{
  struct descriptor_data *d;
  struct follow_type *k;
  char line[80];
  int group = 0, found = 0, previous = 0;

  for (d = descriptor_list; d; d = d->next)
    if (d->character && !d->connected && !d->character->master) {
      if (IS_AFFECTED(d->character, AFF_GROUP) && CAN_SEE(ch, d->character))
        group = 1;
      else
        group = 0;
      for (k = d->character->followers; k && !group; k = k->next)
        if (IS_AFFECTED(k->follower, AFF_GROUP) && CAN_SEE(ch, k->follower))
          group = 1;
      if (group) {
        if (!found) {
          sprintf(buf, "The following groups are currently running around Seattle:\r\n\r\n");
          found = 1;
        }
        previous = 0;
        if (d->character->player_specials->gname)
          sprintf(buf, "%s%s:\r\n", buf, d->character->player_specials->gname);
        else
          strcat(buf, "(unnamed):\r\n");
        if (IS_AFFECTED(d->character, AFF_GROUP)) {
          if (CAN_SEE(ch, d->character)) {
            sprintf(line, "  %s (leader)", GET_NAME(d->character));
            previous = 1;
          } else
            sprintf(line, "  ");
        } else
          sprintf(line, "  ");
        for (k = d->character->followers; k; k = k->next)
          if (IS_AFFECTED(k->follower, AFF_GROUP) && CAN_SEE(ch, k->follower)) {
            if (strlen(line) + strlen(GET_NAME(k->follower)) > 77) {
              strcat(line, ",\r\n");
              strcat(buf, line);
              sprintf(line, "  %s", GET_NAME(k->follower));
            } else
              sprintf(line, "%s%s%s", line, previous ? ", " : "",
                      GET_NAME(k->follower));
            if (!previous)
              previous = 1;
          }
        strcat(line, "\r\n");
        strcat(buf, line);
      }
    }
  if (!found)
    sprintf(buf, "There are presently no groups running around Seattle.\r\n");
  send_to_char(buf, ch);
}

void save_text(struct char_data *ch, int subcmd)
{
  switch (subcmd)
  {
  case SCMD_MOTD:
    strcpy(buf, MOTD_FILE);
    break;
  case SCMD_IMOTD:
    strcpy(buf, IMOTD_FILE);
    break;
  default:
    cerr << "UNKNOWN SCMD" << endl;
    return;
  }

  ofstream file(buf);
  if (!file)
  {
    cerr << "Unable to open " << buf << " for writing (act.informative.cc)"
    << endl;
    return;
  }

  switch (subcmd)
  {
  case SCMD_MOTD:
    file << motd;
    file.close();
    break;
  case SCMD_IMOTD:
    file << imotd;
    file.close();
    break;
  default:
    cerr << "Unknown SCMD in save_text in act.informative.cc" << endl;
    break;
  }
}

void edit_text(struct char_data *ch, int subcmd)
{
  switch (subcmd)
  {
  case SCMD_MOTD:
    if (!IS_NPC(ch))
      PLR_FLAGS(ch).SetBit(PLR_WRITING);
    if (motd)
      delete [] motd;
    motd = new char[MAX_STRING_LENGTH];
    motd = 0;
    ch->desc->str = &motd;
    ch->desc->max_str = MAX_STRING_LENGTH;
    break;
  case SCMD_IMOTD:
    if (!IS_NPC(ch))
      PLR_FLAGS(ch).SetBit(PLR_WRITING);
    if (imotd)
      delete [] imotd;
    imotd = new char[MAX_STRING_LENGTH];
    imotd = 0;
    ch->desc->str = &imotd;
    ch->desc->max_str = MAX_STRING_LENGTH;
    break;
  default:
    cerr << "Unknown SCMD in edit_text, act.informative.cc" << endl;
  }

  return;
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{

  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_IMMLIST:
    send_to_char(ch, immlist);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    skip_spaces(&argument);
    if (access_level(ch, LVL_VICEPRES)) {
      if (!strcmp(argument, "edit"))
        edit_text(ch, subcmd);
      else if (!strcmp(argument, "save"))
        save_text(ch, subcmd);
      else
        page_string(ch->desc, motd, 0);
    } else
      page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    skip_spaces(&argument);
    if (access_level(ch, LVL_VICEPRES)) {
      if (!strcmp(argument, "edit"))
        edit_text(ch, subcmd);
      else if (!strcmp(argument, "save"))
        save_text(ch, subcmd);
      else
        page_string(ch->desc, imotd, 0);
    } else
      page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(strcat(strcpy(buf, *awakemud_version), "\r\n"), ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_CHAR_NAME(ch)), "\r\n"), ch);
    break;
  default:
    return;
    break;
  }
}

extern void nonsensical_reply( struct char_data *ch );

void perform_mortal_where(struct char_data * ch, char *arg)
{
  /* DISABLED FOR MORTALS */
  nonsensical_reply(ch);
  return;
}

void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
                           int recur)
{
  if (num > 0)
    sprintf(buf + strlen(buf), "O%3d. %-25s - ", num,
            GET_OBJ_NAME(obj));
  else
    sprintf(buf + strlen(buf), "%33s", " - ");

  if (obj->in_room > NOWHERE)
    sprintf(buf + strlen(buf), "[%5ld] %s\r\n", world[obj->in_room].number,
            world[obj->in_room].name);
  else if (obj->carried_by)
    sprintf(buf + strlen(buf), "carried by %s\r\n", PERS(obj->carried_by, ch));
  else if (obj->worn_by)
    sprintf(buf + strlen(buf), "worn by %s\r\n", PERS(obj->worn_by, ch));
  else if (obj->in_obj)
  {
    sprintf(buf + strlen(buf), "inside %s%s\r\n",
            GET_OBJ_NAME(obj->in_obj), (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    sprintf(buf + strlen(buf), "in an unknown location\r\n");
}

void perform_immort_where(struct char_data * ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;
  int num = 0, found = 0;
  int found2 = FALSE;

  if (!*arg)
  {
    strcpy(buf, "Players\r\n-------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
        i = (d->original ? d->original : d->character);
        if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE || i->in_veh)) {
          if (d->original)
            if (d->character->in_veh)
              sprintf(buf + strlen(buf), "%-20s - [%5ld] %s (in %s) (in %s)\r\n",
                      GET_CHAR_NAME(i), world[d->character->in_room].number,
                      world[d->character->in_veh->in_room].name, GET_NAME(d->character),
                      d->character->in_veh->short_description);
            else
              sprintf(buf + strlen(buf), "%-20s - [%5ld] %s (in %s)\r\n",
                      GET_CHAR_NAME(i), world[d->character->in_room].number,
                      world[d->character->in_room].name, GET_NAME(d->character));

          else
            if (i->in_veh)
              sprintf(buf + strlen(buf), "%-20s - [%5ld] %s (in %s)\r\n",
                      GET_CHAR_NAME(i),
                      world[i->in_veh->in_room].number, world[i->in_veh->in_room].name, i->in_veh->short_description);
            else
              sprintf(buf + strlen(buf), "%-20s - [%5ld] %s\r\n",
                      GET_CHAR_NAME(i),
                      world[i->in_room].number, world[i->in_room].name);
        }
      }
    page_string(ch->desc, buf, 1);
  } else
  {
    *buf = '\0';
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && (i->in_room != NOWHERE || i->in_veh) &&
          isname(arg, GET_KEYWORDS(i))) {
        found = 1;
        sprintf(buf + strlen(buf), "M%3d. %-25s - [%5ld] %s\r\n", ++num,
                GET_NAME(i),
                (i->in_veh ? world[i->in_veh->in_room].number : world[i->in_room].number),
                (i->in_veh ? i->in_veh->short_description : world[i->in_room].name));
      }
    found2 = ObjList.PrintList(ch, arg);

    if (!found && !found2)
      send_to_char("Couldn't find any such thing.\r\n", ch);
    else
      page_string(ch->desc, buf, 1);
  }
}

ACMD(do_where)
{
  one_argument(argument, arg);

  /* DISABLED FOR MORTALS */

  if (GET_REAL_LEVEL(ch) >= LVL_BUILDER)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}

ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Easy!  Very easy indeed!  \r\n", ch);
    return;
  }
  if (!IS_NPC(victim) || !ok_damage_shopkeeper(ch, victim)) {
    send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
    return;
  }

  diff = (GET_BALLISTIC(victim) - GET_BALLISTIC(ch));
  diff += (GET_IMPACT(victim) - GET_IMPACT(ch));
  diff += (GET_BOD(victim) - GET_BOD(ch));
  diff += (GET_QUI(victim) - GET_QUI(ch));
  diff += (GET_STR(victim) - GET_STR(ch));
  diff += (GET_COMBAT(victim) - GET_COMBAT(ch));
  diff += (GET_REA(victim) - GET_REA(ch));
  diff += (GET_INIT_DICE(victim) - GET_INIT_DICE(ch));
  if (GET_MAG(victim) >= 100) {
    diff += (int)((GET_MAG(victim) - GET_MAG(ch)) / 100);
    diff += GET_SKILL(victim, SKILL_SORCERY);
  }
  if (GET_MAG(ch) >= 100 && (IS_NPC(ch) || (GET_TRADITION(ch) == TRAD_HERMETIC ||
                             GET_TRADITION(ch) == TRAD_SHAMANIC)))
    diff -= GET_SKILL(ch, SKILL_SORCERY);
  if (GET_EQ(victim, WEAR_WIELD))
    diff += GET_SKILL(victim, GET_OBJ_VAL(GET_EQ(victim, WEAR_WIELD), 4));
  else
    diff += GET_SKILL(victim, SKILL_UNARMED_COMBAT);
  if (GET_EQ(ch, WEAR_WIELD))
    diff -= GET_SKILL(ch, GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 4));
  else
    diff -= GET_SKILL(ch, SKILL_UNARMED_COMBAT);

  if (diff <= -18)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -12)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -6)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -3)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 3)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 6)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 12)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 18)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 30)
    send_to_char("Are you mad!?\r\n", ch);
  else
    send_to_char("You ARE mad!\r\n", ch);
}

ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}

const char *ctypes[] =
  {
    "off",
    "sparse",
    "normal",
    "complete",
    "\n"
  };

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch)) {
    send_to_char("You shouldn't tamper with color while in this form.\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }

  PRF_FLAGS(ch).RemoveBits(PRF_COLOR_1, PRF_COLOR_2, ENDBIT);
  if (tp & 1)
    PRF_FLAGS(ch).SetBit(PRF_COLOR_1);
  if (tp & 2)
    PRF_FLAGS(ch).SetBit(PRF_COLOR_2);

  sprintf(buf, "Your ^Rc^Bo^Gl^Co^Yr^n is now %s.\r\n", ctypes[tp]);
  send_to_char(buf, ch);
}

struct sort_struct
{
  int sort_pos;
  byte is_social;
}
*cmd_sort_info = NULL;

int num_of_cmds;

void sort_commands(void)
{
  int a, b, tmp;

  ACMD(do_action);

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  cmd_sort_info = new sort_struct[num_of_cmds];

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
                 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
        tmp = cmd_sort_info[a].sort_pos;
        cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
        cmd_sort_info[b].sort_pos = tmp;
      }
}

ACMD(do_commands)
{
  int no, i, cmd_num;
  int socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_REAL_LEVEL(ch) < GET_REAL_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;

  sprintf(buf, "The following %s are available to %s:\r\n",
          socials ? "socials" : "commands",
          vict == ch ? "you" : GET_NAME(vict));

  if (PLR_FLAGGED(ch, PLR_MATRIX)) {
    for (no = 1, cmd_num = 1;;cmd_num++) {
      if (mtx_info[cmd_num].minimum_level >= 0 &&
          ((!IS_NPC(vict) && GET_REAL_LEVEL(vict) >= mtx_info[cmd_num].minimum_level) ||
           (IS_NPC(vict) && vict->desc->original && GET_REAL_LEVEL(vict->desc->original) >= mtx_info[cmd_num].minimum_level))) {
        sprintf(buf + strlen(buf), "%-11s", mtx_info[cmd_num].command);
        if (!(no % 7))
          strcat(buf, "\r\n");
        no++;
        if (*mtx_info[cmd_num].command == '\n')
          break;
      }
    }
  } else if (PLR_FLAGGED(ch, PLR_REMOTE) || AFF_FLAGGED(ch, AFF_RIG)) {
    for (no = 1, cmd_num = 1;;cmd_num++) {
      if (rig_info[cmd_num].minimum_level >= 0 &&
          ((!IS_NPC(vict) && GET_REAL_LEVEL(vict) >= rig_info[cmd_num].minimum_level) ||
           (IS_NPC(vict) && vict->desc->original && GET_REAL_LEVEL(vict->desc->original) >= rig_info[cmd_num].minimum_level))) {
        sprintf(buf + strlen(buf), "%-11s", rig_info[cmd_num].command);
        if (!(no % 7))
          strcat(buf, "\r\n");
        no++;
        if (*rig_info[cmd_num].command == '\n')
          break;
      }
    }
  } else {
    for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
      i = cmd_sort_info[cmd_num].sort_pos;
      if (cmd_info[i].minimum_level >= 0 &&
          ((!IS_NPC(vict) && GET_REAL_LEVEL(vict) >= cmd_info[i].minimum_level) ||
           (IS_NPC(vict) && vict->desc->original && GET_REAL_LEVEL(vict->desc->original) >= cmd_info[i].minimum_level)) &&
          (socials == cmd_sort_info[i].is_social)) {
        sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
        if (!(no % 7))
          strcat(buf, "\r\n");
        no++;
      }
    }
  }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

ACMD(do_scan)
{
  struct char_data *list;
  struct veh_data *veh, *in_veh = NULL;
  bool specific = FALSE, infra, lowlight, onethere, anythere = FALSE, done = FALSE;
  int i = 0, j, was_in_room, dist = 3, x = NOWHERE;

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS)) {
    send_to_char(ch, "The ultrasound distorts your vision.\r\n");
    return;
  }
  argument = any_one_arg(argument, buf);

  if (*buf) {
    if (is_abbrev(buf, "south")) {
      i = SCMD_SOUTH;
      specific = TRUE;
    } else
      for (;!specific && (i < NUM_OF_DIRS); ++i) {
        if (is_abbrev(buf, dirs[i]))
          specific = TRUE;
      }
  }
  if (ch->in_veh || ch->char_specials.rigging) {
    RIG_VEH(ch, in_veh);
    if (ch->in_room != NOWHERE)
      x = ch->in_room;
    ch->in_room = in_veh->in_room;
  }

  infra = ((PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
            PLR_FLAGGED(ch, AFF_INFRAVISION)) ? TRUE : FALSE);
  lowlight = ((PRF_FLAGGED(ch, PRF_HOLYLIGHT) ||
               PLR_FLAGGED(ch, AFF_LOW_LIGHT)) ? TRUE : FALSE);

  if (!infra && IS_ASTRAL(ch))
    infra = TRUE;
  if (!specific) {
    for (i = 0; i < NUM_OF_DIRS; ++i) {
      if (CAN_GO(ch, i)) {
        onethere = FALSE;
        if (!((!infra && IS_DARK(EXIT(ch, i)->to_room)) ||
              ((!infra || !lowlight) && IS_LOW(EXIT(ch, i)->to_room)))) {
          strcpy(buf1, "");
          for (list = world[EXIT(ch, i)->to_room].people; list; list = list->next_in_room)
            if (CAN_SEE(ch, list)) {
              if (in_veh) {
                if (in_veh->cspeed > SPEED_IDLE)
                  if (get_speed(in_veh) >= 200)
                    if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 7))
                      continue;
                    else if (get_speed(in_veh) < 200 && get_speed(in_veh) >= 120)
                      if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6))
                        continue;
                      else if (get_speed(in_veh) < 120 && get_speed(in_veh) >= 60)
                        if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5))
                          continue;
                        else
                          if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4))
                            continue;
              }
              sprintf(buf1, "%s  %s\r\n",
                      buf1, GET_NAME(list));
              onethere = TRUE;
              anythere = TRUE;
            }
          for (veh = world[EXIT(ch, i)->to_room].vehicles; veh; veh = veh->next_veh) {
            if (in_veh) {
              if (in_veh->cspeed > SPEED_IDLE)
                if (get_speed(in_veh) >= 200)
                  if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 7))
                    continue;
                  else if (get_speed(in_veh) < 200 && get_speed(in_veh) >= 120)
                    if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6))
                      continue;
                    else if (get_speed(in_veh) < 120 && get_speed(in_veh) >= 60)
                      if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5))
                        continue;
                      else
                        if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4))
                          continue;
            }
            sprintf(buf1, "%s  %s (%s)\r\n",
                    buf1, veh->short_description, (get_speed(veh) ? "Moving" : "Stationary"));
            onethere = TRUE;
            anythere = TRUE;
          }
        }
        if (onethere) {
          sprintf(buf2, "%s %s:\r\n%s\r\n", dirs[i], dist_name[0], buf1);
          CAP(buf2);
          send_to_char(buf2, ch);
        }
      }
    }
    if (!anythere) {
      send_to_char("You don't seem to see anyone in the surrounding areas.\r\n", ch);
      if (in_veh) {
        if (x > NOWHERE)
          ch->in_room = x;
        else
          ch->in_room = NOWHERE;
      }
      return;
    }
  } else {
    --i;

    dist = find_sight(ch);

    if (CAN_GO(ch, i)) {
      was_in_room = ch->in_room;
      anythere = FALSE;
      for (j = 0;!done && (j < dist); ++j) {
        onethere = FALSE;
        if (CAN_GO(ch, i)) {
          strcpy(buf1, "");
          for (list = world[EXIT(ch, i)->to_room].people; list; list = list->next_in_room)
            if (CAN_SEE(ch, list)) {
              if (in_veh) {
                if (in_veh->cspeed > SPEED_IDLE)
                  if (get_speed(in_veh) >= 200)
                    if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 7))
                      continue;
                    else if (get_speed(in_veh) < 200 && get_speed(in_veh) >= 120)
                      if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 6))
                        continue;
                      else if (get_speed(in_veh) < 120 && get_speed(in_veh) >= 60)
                        if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 5))
                          continue;
                        else
                          if (!success_test(GET_INT(ch) + GET_POWER(ch, ADEPT_IMPROVED_PERCEPT), 4))
                            continue;
              }
              sprintf(buf1, "%s  %s\r\n", buf1, GET_NAME(list));

              onethere = TRUE;
              anythere = TRUE;
            }

          ch->in_room = EXIT(ch, i)->to_room;

          if (onethere) {
            sprintf(buf2, "%s %s:\r\n%s\r\n", dirs[i], dist_name[j], buf1);
            CAP(buf2);
            send_to_char(buf2, ch);
          }
        } else
          done = TRUE;
      }

      ch->in_room = was_in_room;

      if (!anythere) {
        if (in_veh) {
          if (x > NOWHERE)
            ch->in_room = x;
          else
            ch->in_room = NOWHERE;
        }
        send_to_char("You don't seem to see anyone in that direction.\r\n", ch);
        return;
      }
    } else {
      if (in_veh) {
        if (x > NOWHERE)
          ch->in_room = x;
        else
          ch->in_room = NOWHERE;
      }
      send_to_char("There is no exit in that direction.\r\n", ch);
      return;
    }
  }
  if (in_veh) {
    if (x > NOWHERE)
      ch->in_room = x;
    else
      ch->in_room = NOWHERE;
  }
}


ACMD(do_position)
{
  skip_spaces(&argument);
  if (!*argument) {
    if (GET_DEFPOS(ch)) {
      delete [] GET_DEFPOS(ch);
      GET_DEFPOS(ch) = NULL;
      send_to_char(ch, "Position cleared.\r\n");
    } else
      send_to_char(ch, "What position do you wish to be seen in?\r\n");
    return;
  }
  if (GET_POS(ch) == POS_FIGHTING) {
    send_to_char(ch, "You can't set your position while fighting.\r\n");
    return;
  }
  if (GET_DEFPOS(ch))
    delete [] GET_DEFPOS(ch);
  GET_DEFPOS(ch) = str_dup(argument);
  send_to_char(ch, "Position set.\r\n");
}

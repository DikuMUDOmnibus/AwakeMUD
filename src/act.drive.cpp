#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "transport.h"
#include "constants.h"

void die_follower(struct char_data *ch);
void roll_individual_initiative(struct char_data *ch);
void order_list(struct char_data *start);
int find_first_step(vnum_t src, vnum_t target);
int move_vehicle(struct char_data *ch, int dir);
ACMD(do_return);

#define VEH ch->in_veh

int get_maneuver(struct veh_data *veh)
{
  int score = -2;
  struct char_data *ch = NULL;
  int i = 0, x = 0, skill;

  switch (veh->type)
  {
  case VEH_BIKE:
    score += 5;
    break;
  case VEH_TRUCK:
    score -= 5;
    break;
  case VEH_CAR:
    if (veh->speed >= 200)
      score +=5;
    break;
  }
  if (veh->cspeed > SPEED_IDLE)
    score += (int)(get_speed(veh) / 10);

  if (veh->rigger)
    ch = veh->rigger;
  else
    for (ch = veh->people; ch; ch = ch->next_in_veh)
      if (AFF_FLAGGED(ch, AFF_PILOT))
        break;
  if (ch)
  {
    for (skill = veh_skill(ch, veh);skill > 0; skill--) {
      i = srdice();
      if (i > x)
        x = i;
    }
    score += x;
  }
  return score;
}

int modify_veh(struct veh_data *veh)
{
  struct char_data *ch;
  struct obj_data *cyber;
  int mod = 0;

  if (!(ch = veh->rigger))
    for (ch = veh->people; ch; ch = ch->next_in_veh)
      if (AFF_FLAGGED(ch, AFF_PILOT))
        break;

  if (veh->cspeed > SPEED_IDLE && ch)
  {
    for (cyber = ch->cyberware; cyber; cyber = cyber->next_content)
      if (GET_OBJ_VAL(cyber, 2) == 32) {
        mod -= GET_OBJ_VAL(cyber, 0);
        break;
      }
    if (mod == 0 && AFF_FLAGGED(ch, AFF_RIG))
      mod--;
    if (get_speed(veh) > (int)(GET_REA(ch) * 20))
      if (get_speed(veh) > (int)(GET_REA(ch) * 40))
        mod += 3;
      else if (get_speed(veh) < (int)(GET_REA(ch) * 40) && get_speed(veh) > (int)(GET_REA(ch) * 30))
        mod += 2;
      else
        mod++;
  }
  switch (veh->damage)
  {
  case 1:
  case 2:
    mod += 1;
    break;
  case 3:
  case 4:
  case 5:
    mod += 2;
    break;
  case 6:
  case 7:
  case 8:
  case 9:
    mod += 3;
    break;
  }
  if (weather_info.sky >= SKY_RAINING)
    mod++;
  return mod;
}

void stop_chase(struct veh_data *veh)
{
  struct veh_follow *k, *j;

  if (!veh->following)
    return;

  if (veh == veh->following->followers->follower)
  {
    k = veh->following->followers;
    veh->following->followers = k->next;
    delete k;
  } else
  {
    for (k = veh->following->followers; k->next->follower != veh; k = k->next)
      ;
    j = k->next;
    k->next = j->next;
    delete j;
  }
  veh->following = NULL;
}

void crash_test(struct char_data *ch)
{
  int target = 0, skill = 0;
  int power, attack_resist = 0, damage_total = SERIOUS;
  struct veh_data *veh;
  struct char_data *tch, *next;

  RIG_VEH(ch, veh);
  target = veh->handling + modify_veh(veh) + modify_target(ch);
  power = (int)(ceilf(get_speed(veh) / 10));

  sprintf(buf, "%s begins to lose control!\r\n", veh->short_description);
  send_to_room(buf, veh->in_room);

  send_to_char("You begin to lose control!\r\n", ch);
  skill = veh_skill(ch, veh) + veh->autonav;

  if (success_test(skill, target))
  {
    send_to_char("You get your ride under control.\r\n", ch);
    return;
  }
  send_to_veh("You careen off the road!\r\n", veh, NULL, TRUE);
  sprintf(buf, "A %s careens off the road!\r\n", veh->short_description);
  send_to_room(buf, veh->in_room);

  attack_resist = success_test(veh->body, power) * -1;

  int staged_damage = stage(attack_resist, damage_total);
  damage_total = convert_damage(staged_damage);

  veh->damage += damage_total;
  stop_chase(veh);
  if (veh->type == VEH_BIKE && veh->people)
  {
    power = (int)(get_speed(veh) / 10);
    for (tch = veh->people; tch; tch = next) {
      next = tch->next_in_veh;
      char_from_room(tch);
      char_to_room(tch, veh->in_room);
      veh->seating++;
      damage_total = convert_damage(stage(0 - success_test(GET_BOD(tch), power), MODERATE));
      damage(tch, tch, damage_total, TYPE_CRASH, PHYSICAL);
      AFF_FLAGS(tch).RemoveBits(AFF_PILOT, AFF_RIG, ENDBIT);
      send_to_char("You are thrown from the bike!\r\n", tch);
    }
    veh->cspeed = SPEED_OFF;
  }
}

ACMD(do_drive)
{
  struct char_data *temp;

  if (!VEH) {
    send_to_char("You have to be in a vehicle to do that.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_RIG)) {
    send_to_char("You are already rigging the vehicle.\r\n", ch);
    return;
  }
  if(GET_SKILL(ch, SKILL_PILOT_CAR) == 0 && GET_SKILL(ch, SKILL_PILOT_BIKE) == 0 &&
      GET_SKILL(ch, SKILL_PILOT_TRUCK) == 0) {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }
  if (VEH->damage >= 10) {
    send_to_char("This vehicle is too much of a wreck to move!\r\n", ch);
    return;
  }
  if (VEH->rigger || VEH->dest) {
    send_to_char("You can't seem to take control!\r\n", ch);
    return;
  }
  for(temp = VEH->people; temp; temp= temp->next_in_veh)
    if(ch != temp && AFF_FLAGGED(temp, AFF_PILOT)) {
      send_to_char("Someone is already in charge!\r\n", ch);
      return;
    }
  if (VEH->type == VEH_BIKE && VEH->locked && GET_IDNUM(ch) != VEH->owner) {
    send_to_char("You can't seem to start it.\r\n", ch);
    return;
  }
  if (VEH->cspeed == SPEED_OFF || VEH->dest) {
    AFF_FLAGS(ch).ToggleBit(AFF_PILOT);
    send_to_char("The wheel is in your hands.\r\n", ch);
    sprintf(buf1, "%s takes the wheel.\r\n", GET_NAME(ch));
    VEH->cspeed = SPEED_IDLE;
    VEH->lastin[0] = VEH->in_room;
    send_to_veh(buf1, VEH, ch, FALSE);
    if (AFF_FLAGGED(ch, AFF_MANNING)) {
      struct obj_data *mount;
      for (mount = ch->in_veh->mount; mount; mount = mount->next_content)
        if (mount->worn_by == ch)
          break;
      mount->worn_by = NULL;
      AFF_FLAGS(ch).RemoveBit(AFF_MANNING);
    }
  } else {
    AFF_FLAGS(ch).ToggleBit(AFF_PILOT);
    send_to_char("You relinquish the driver's seat.\r\n", ch);
    sprintf(buf1, "%s relinquishes the driver's seat.\r\n", GET_NAME(ch));
    stop_chase(VEH);
    if (!VEH->dest)
      VEH->cspeed = SPEED_OFF;
    send_to_veh(buf1, VEH, ch, FALSE);
  }
  return;
}

ACMD(do_rig)
{
  struct char_data *temp;
  struct obj_data *cyber;
  int has_datajack = 0;

  if (!VEH) {
    send_to_char("You have to be in a vehicle to do that.\r\n", ch);
    return;
  }
  for (cyber = ch->cyberware; cyber; cyber = cyber->next_content)
    if (GET_OBJ_VAL(cyber, 2) == CYB_DATAJACK)
      has_datajack = TRUE;

  if (!has_datajack) {
    send_to_char("You need atleast a datajack to do that.\r\n", ch);
    return;
  }
  if (GET_EQ(ch, WEAR_HEAD) && GET_OBJ_VAL(GET_EQ(ch, WEAR_HEAD), 7) > 0) {
    send_to_char(ch, "Try removing your helmet first.\r\n");
    return;
  }
  if (GET_SKILL(ch, SKILL_PILOT_CAR) == 0 && GET_SKILL(ch, SKILL_PILOT_BIKE) == 0 &&
      GET_SKILL(ch, SKILL_PILOT_TRUCK) == 0) {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }
  if (VEH->damage >= 10) {
    send_to_char("This vehicle is too much of a wreck to move!\r\n", ch);
    return;
  }
  if (VEH->rigger || VEH->dest) {
    send_to_char("The system is unresponsive!\r\n", ch);
    return;
  }
  for(temp = VEH->people; temp; temp= temp->next_in_veh)
    if(ch != temp && AFF_FLAGGED(temp, AFF_PILOT)) {
      send_to_char("Someone is already in charge!\r\n", ch);
      return;
    }
  if (VEH->type == VEH_BIKE && VEH->locked && GET_IDNUM(ch) != VEH->owner) {
    send_to_char("You jack in and nothing happens.\r\n", ch);
    return;
  }
  if(VEH->cspeed == SPEED_OFF || VEH->dest) {
    AFF_FLAGS(ch).ToggleBits(AFF_PILOT, AFF_RIG, ENDBIT);
    send_to_char("As you jack in, your perception shifts.\r\n", ch);
    sprintf(buf1, "%s jacks into the vehicle control system.\r\n", GET_NAME(ch));
    VEH->cspeed = SPEED_IDLE;
    VEH->lastin[0] = VEH->in_room;
    send_to_veh(buf1, VEH, ch, TRUE);
    if (AFF_FLAGGED(ch, AFF_MANNING)) {
      struct obj_data *mount;
      for (mount = ch->in_veh->mount; mount; mount = mount->next_content)
        if (mount->worn_by == ch)
          break;
      mount->worn_by = NULL;
      AFF_FLAGS(ch).RemoveBit(AFF_MANNING);
    }
  } else {
    if (!AFF_FLAGGED(ch, AFF_RIG)) {
      send_to_char("But you're not rigging.\r\n", ch);
      return;
    }
    AFF_FLAGS(ch).ToggleBits(AFF_PILOT, AFF_RIG, ENDBIT);
    if (!VEH->dest)
      VEH->cspeed = SPEED_OFF;
    stop_chase(VEH);
    send_to_char("You return to your senses.\r\n", ch);
    sprintf(buf1, "%s returns to their senses.\r\n", GET_NAME(ch));
    send_to_veh(buf1, VEH, ch, FALSE);
    stop_fighting(ch);
  }
  return;
}

ACMD(do_vemote)
{
  struct veh_data *veh;
  if (!(AFF_FLAGGED(ch, AFF_PILOT) || PLR_FLAGGED(ch, PLR_REMOTE))) {
    send_to_char("You must be driving a vehicle to use that command.\r\n", ch);
    return;
  }
  if (!*argument) {
    send_to_char("Yes.. but what?\r\n", ch);
    return;
  }
  RIG_VEH(ch, veh)
  sprintf(buf, "%s%s.\r\n", veh->short_description, argument);
  send_to_room(buf, veh->in_room);
  sprintf(buf, "Your vehicle%s.\r\n", argument);
  send_to_veh(buf, veh, NULL, TRUE);
  return;
}

ACMD(do_ram)
{
  struct veh_data *veh, *tveh = NULL;
  struct char_data *vict = NULL;
  int skill = 0, target = 0, tvehm = 0, vehm = 0;

  if (!(AFF_FLAGGED(ch, AFF_PILOT) || PLR_FLAGGED(ch, PLR_REMOTE))) {
    send_to_char("You need to be driving a vehicle to do that...\r\n", ch);
    return;
  }
  if (FIGHTING(ch) || FIGHTING_VEH(ch)) {
    send_to_char(ch, "You're in the middle of combat!\r\n");
    return;
  }
  RIG_VEH(ch, veh);
  if (get_speed(veh) <= SPEED_IDLE) {
    send_to_char("You're moving far too slowly.\r\n", ch);
    return;
  }
  if (world[veh->in_room].peaceful) {
    send_to_char("This room just has a peaceful, easy feeling...\r\n", ch);
    return;
  }
  two_arguments(argument, arg, buf2);

  if (!*arg) {
    send_to_char("Ram what?", ch);
    return;
  }
  if (!(vict = get_char_room(arg, veh->in_room)) &&
      !(tveh = get_veh_list(arg, world[veh->in_room].vehicles))) {
    send_to_char("You can't seem to find the target you're looking for.\r\n", ch);
    return;
  }
  if (vict && !INVIS_OK(ch, vict)) {
    send_to_char("You can't seem to find the target you're looking for.\r\n", ch);
    return;
  }
  if (vict && !IS_NPC(vict) && !(PRF_FLAGGED(ch, PRF_PKER) && PRF_FLAGGED(vict, PRF_PKER))) {
    send_to_char(ch, "You have to be flagged PK to attack another player.\r\n");
    return;
  }

  skill = veh_skill(ch, veh);
  if (tveh) {
    target = modify_veh(veh) + veh->handling + modify_target(ch);
    vehm = get_maneuver(veh);
    tvehm = get_maneuver(tveh);
    if (vehm > (tvehm + 10))
      target -= 4;
    else if (vehm > tvehm)
      target -= 2;
    else if (vehm < (tvehm - 10))
      target += 4;
    else if (vehm < tvehm)
      target += 2;
    sprintf(buf, "%s heads straight towards your ride.\r\n", veh->short_description);
    sprintf(buf1, "%s heads straight towards %s.\r\n", veh->short_description, tveh->short_description);
    sprintf(buf2, "You attempt to ram %s.\r\n", tveh->short_description);
    send_to_veh(buf, tveh, 0, TRUE);
    send_to_room(buf1, veh->in_room);
    send_to_char(buf2, ch);
  } else {
    target = modify_veh(veh) + veh->handling + modify_target(ch);
    sprintf(buf, "%s heads straight towards you.", veh->short_description);
    sprintf(buf1, "%s heads straight towards $n.", veh->short_description);
    act(buf, FALSE, vict, 0, 0, TO_CHAR);
    act(buf1, FALSE, vict, 0, 0, TO_ROOM);
    act("You head straight towards $N.", FALSE, ch, 0, vict, TO_CHAR);
    if (success_test(GET_REA(vict), 4)) {
      send_to_char(ch, "You quickly let out an attack at it!\r\n");
      act("$n quickly lets an attack fly.", FALSE, vict, 0, 0, TO_ROOM);
      act("$N suddenly attacks!", FALSE, ch, 0, vict, TO_CHAR);
      AFF_FLAGS(vict).SetBit(AFF_COUNTER_ATT);
      vcombat(vict, veh);
      AFF_FLAGS(vict).RemoveBit(AFF_COUNTER_ATT);
      if (veh->damage > 9)
        return;
    }
  }
  if (success_test(skill, target))
    if (vict)
      vram(veh, vict, NULL);
    else
      vram(veh, NULL, tveh);
  else {
    crash_test(ch);
    chkdmg(veh);
  }
}

ACMD(do_upgrade)
{
  struct veh_data *veh;
  struct obj_data *mod, *obj, *shop = NULL;
  int j = 0, skill = 0, target = 6, kit = 0;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char("What do you want to upgrade?\r\n", ch);
    return;
  }
  if (ch->in_veh) {
    send_to_char("You have to be outside what you want to upgrade.\r\n", ch);
    return;
  }
  if (!(veh = get_veh_list(buf1, world[ch->in_room].vehicles))) {
    send_to_char("You can't seem to find that vehicle here.\r\n", ch);
    return;
  }

  if (!*buf2) {
    send_to_char("You have to upgrade it with something!\r\n", ch);
    return;
  } else if (!(mod = get_obj_in_list_vis(ch, buf2, ch->carrying))) {
    send_to_char("You don't seem to have that item.\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(mod) != ITEM_MOD) {
    send_to_char(ch, "That is not a vehicle modification.\r\n");
    return;
  }
  if (!IS_NPC(ch)) {
    switch(veh->type) {
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
        skill = GET_SKILL(ch, SKILL_BR_BIKE) / 4;
      if (!skill)
        skill = GET_SKILL(ch, SKILL_BR_DRONE) / 4;
      break;
    }
    if (!skill) {
      send_to_char(ch, "You have no idea what you are doing...\r\n");
      return;
    }
    target += modify_target(ch);
    for (obj = ch->carrying; obj && !kit; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_WORKING_GEAR && GET_OBJ_VAL(obj, 0) == 4)
        kit = GET_OBJ_VAL(obj, 1);
    for (int i = 0; !kit && i < (NUM_WEARS - 1); i++)
      if ((obj = GET_EQ(ch, i)) && GET_OBJ_TYPE(obj) == ITEM_WORKING_GEAR && GET_OBJ_VAL(obj, 0) == 4)
        kit = GET_OBJ_VAL(obj, 1);
    for (obj = world[ch->in_room].contents; obj && !shop; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_WORKSHOP && GET_OBJ_VAL(obj, 0) == 4 && GET_OBJ_VAL(obj, 1))
        shop = obj;
    if (!shop)
      target += 2;

    switch (kit) {
    case 0:
      target += 4;
      break;
    case 1:
      target += 2;
      break;
    case 3:
      target -= 2;
      break;
    }

    if ((skill = success_test(skill, target)) == -1) {
      sprintf(buf, "You botch up the upgrade completely, destroying %s.\r\n", GET_OBJ_NAME(mod));
      send_to_char(buf, ch);
      extract_obj(mod);
      return;
    } else if (skill < 1) {
      send_to_char(ch, "You can't figure out how to install it. \r\n");
      return;
    }
  }

  if ((veh->type == VEH_DRONE && GET_OBJ_VAL(mod, 4) < 1) ||
      (veh->type != VEH_DRONE && GET_OBJ_VAL(mod, 4) == 1)) {
    send_to_char(ch, "That part won't fit on.\r\n");
    return;
  }
  if (veh->type == VEH_DRONE) {
    switch(GET_OBJ_VAL(mod, 0)) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 7:
    case 10:
    case 11:
    case 12:
      send_to_char(ch, "You can't upgrade that on a drone.\r\n");
      return;
    }
  }

  if (GET_OBJ_VAL(mod, 0) == MOD_MOUNT) {
    skill = 0;
    for (obj = veh->mount; obj; obj = obj->next_content)
      switch (GET_OBJ_VAL(obj, 1)) {
      case 0:
      case 1:
        j++;
        break;
      case 2:
      case 3:
      case 5:
        j += 2;
        break;
      case 4:
        j += 4;
        break;
      }
    switch (GET_OBJ_VAL(mod, 1)) {
    case 1:
      skill = 1;
    case 0:
      j++;
      target = 10;
      break;
    case 3:
      skill = 1;
    case 2:
      j += 2;
      target = 10;
      break;
    case 4:
      skill = 1;
      j += 4;
      target = 100;
      break;
    case 5:
      skill = 1;
      j += 2;
      target = 25;
      break;
    }
    if (j > veh->body || (veh->load - target) < 0) {
      send_to_char("Try as you might, you just can't fit it on.\r\n", ch);
      return;
    }
    veh->load -= target;
    veh->sig -= skill;
    obj_from_char(mod);
    if (veh->mount)
      mod->next_content = veh->mount;
    veh->mount = mod;
  } else {

    if (GET_OBJ_VAL(mod, 0) != MOD_SEAT && GET_MOD(veh, GET_OBJ_VAL(mod, 0))) {
      send_to_char(ch, "There is already a mod of that type installed.\r\n");
      return;
    }
    if (veh->load < GET_OBJ_VAL(mod, 1)) {
      send_to_char(ch, "Try as you might, you just can't fit it in.\r\n");
      return;
    }
    obj_from_char(mod);
    veh->load -= GET_OBJ_VAL(mod, 1);
    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      affect_veh(veh, mod->affected[j].location, mod->affected[j].modifier);
    if (GET_OBJ_VAL(mod, 0) == MOD_SEAT && GET_MOD(veh, GET_OBJ_VAL(mod, 0))) {
      GET_MOD(veh, GET_OBJ_VAL(mod, 0))->affected[0].modifier++;
      extract_obj(mod);
    } else
      GET_MOD(veh, GET_OBJ_VAL(mod, 0)) = mod;
  }

  sprintf(buf, "$n goes to work on %s.\r\n", veh->short_description);
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  sprintf(buf, "You go to work on %s.\r\n", veh->short_description);
  send_to_char(buf, ch);
}

void disp_mod(struct veh_data *veh, struct char_data *ch, int i)
{
  send_to_char(ch, "\r\nMounts:\r\n");
  for (struct obj_data *mount = veh->mount; mount; mount = mount->next_content)
    if (GET_OBJ_VAL(mount, 1) != 0 && GET_OBJ_VAL(mount, 1) != 2 && mount->contains)
      send_to_char(ch, "%s mounted on %s.\r\n", CAP(GET_OBJ_NAME(mount->contains)), GET_OBJ_NAME(mount));
  send_to_char(ch, "Modifications:\r\n");
  if (GET_MOD(veh, 13))
    send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 13)));
  if (GET_MOD(veh, 5))
    send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 5)));
  if (i <= 1)
    return;
  if (i >= 2)
  {
    if (i >= 3) {
      if (GET_MOD(veh, 3))
        send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 3)));
      if (GET_MOD(veh, 12))
        send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 12)));
      if (GET_MOD(veh, 1))
        send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 1)));
      if (i >= 4) {
        if (GET_MOD(veh, 0))
          send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 0)));
        if (GET_MOD(veh, 4))
          send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 4)));
        if (i >= 5) {
          if (GET_MOD(veh, 2))
            send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 2)));
          if (GET_MOD(veh, 8))
            send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 8)));
          if (GET_MOD(veh, 9))
            send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 9)));
          if (i >= 6) {
            if (GET_MOD(veh, 11))
              send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 11)));
            if (GET_MOD(veh, 7))
              send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 7)));
            if (i >= 7) {
              if (GET_MOD(veh, 6))
                send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 6)));
              if (GET_MOD(veh, 10))
                send_to_char(ch, "%s\r\n", GET_OBJ_NAME(GET_MOD(veh, 10)));
            }
          }
        }
      }
    }
  }
}

ACMD(do_control)
{
  struct obj_data *cyber;
  struct veh_data *veh;
  struct char_data *temp;
  bool has_rig = FALSE, has_jack = FALSE;
  int i;

  for (cyber = ch->cyberware; cyber; cyber = cyber->next_content)
    if (GET_OBJ_VAL(cyber, 2) == 32)
      has_rig = TRUE;
    else if (GET_OBJ_VAL(cyber, 2) == CYB_DATAJACK)
      has_jack = TRUE;

  if (AFF_FLAGS(ch).AreAnySet(AFF_PILOT, AFF_PROGRAM, AFF_DESIGN, ENDBIT)) {
    send_to_char("Now that would be a neat trick.\r\n", ch);
    return;
  }
  if (!has_jack) {
    send_to_char("You need a datajack to do that.\r\n", ch);
    return;
  }
  if (!has_rig) {
    send_to_char("You need a vehicle control rig to do that.\r\n", ch);
    return;
  }
  if (GET_EQ(ch, WEAR_HEAD) && GET_OBJ_VAL(GET_EQ(ch, WEAR_HEAD), 7) > 0) {
    send_to_char(ch, "Try removing your helmet first.\r\n");
    return;
  }
  if (ch->in_veh) {
    send_to_char(ch, "You can't control a vehicle from inside one.\r\n");
    return;
  }
  has_rig = FALSE;
  for (cyber = ch->carrying; cyber; cyber = cyber->next_content)
    if (GET_OBJ_TYPE(cyber) == ITEM_RCDECK)
      has_rig = TRUE;

  if (!has_rig)
    for (int x = 0; x < NUM_WEARS; x++)
      if (GET_EQ(ch, x))
        if (GET_OBJ_TYPE(GET_EQ(ch, x)) == ITEM_RCDECK)
          has_rig = TRUE;

  if (!has_rig) {
    send_to_char("You need a Remote Control Deck to do that.\r\n", ch);
    return;
  }

  if ((i = atoi(argument)) < 0) {
    send_to_char("Which position on your subscriber list?\r\n", ch);
    return;
  }

  for (veh = ch->char_specials.subscribe; veh; veh = veh->next_sub)
    if (--i < 0)
      break;

  if (!veh) {
    send_to_char("Your subscriber list isn't that big.\r\n", ch);
    return;
  }
  if (PLR_FLAGGED(ch, PLR_REMOTE))
    do_return(ch, NULL, 0, 0);

  ch->char_specials.rigging = veh;
  veh->rigger = ch;
  for(temp = veh->people; temp; temp = temp->next_in_veh)
    if (AFF_FLAGGED(temp, AFF_RIG)) {
      send_to_char("You are forced out of the system!\r\n", temp);
      AFF_FLAGS(temp).ToggleBits(AFF_RIG, AFF_PILOT, ENDBIT);
    } else if (AFF_FLAGGED(temp, AFF_PILOT)) {
      send_to_char("The controls stop responding!\r\n", temp);
      AFF_FLAGS(temp).ToggleBit(AFF_PILOT);
    } else
      send_to_char("The vehicle begins to move.\r\n", temp);

  GET_POS(ch) = POS_SITTING;
  act("$n jacks into $s deck.", TRUE, ch, NULL, NULL, TO_ROOM);
  PLR_FLAGS(ch).ToggleBit(PLR_REMOTE);
  sprintf(buf, "You take control of %s.\r\n", veh->short_description);
  send_to_char(buf, ch);
}


ACMD(do_subscribe)
{
  struct veh_data *veh, *temp;
  struct obj_data *obj;
  bool has_deck = FALSE;
  int i = 0, num;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_RCDECK)
      has_deck = TRUE;

  if (!has_deck)
    for (int x = 0; x < NUM_WEARS; x++)
      if (GET_EQ(ch, x))
        if (GET_OBJ_TYPE(GET_EQ(ch, x)) == ITEM_RCDECK)
          has_deck = TRUE;

  if (!has_deck) {
    send_to_char("You need a Remote Control Deck to do that.\r\n", ch);
    return;
  }
  if (subcmd == SCMD_UNSUB) {
    if (!*argument) {
      send_to_char("You need to supply a number.\r\n", ch);
      return;
    }
    num = atoi(argument);
    for (veh = ch->char_specials.subscribe; veh; veh = veh->next_sub)
      if (--num < 0)
        break;
    if (!veh) {
      send_to_char("Your subscriber list isn't that big.\r\n", ch);
      return;
    }
    REMOVE_FROM_LIST(veh, ch->char_specials.subscribe, next_sub);
    veh->sub = FALSE;
    veh->next_sub = NULL;
    sprintf(buf, "You remove %s from your subscriber list.\r\n", veh->short_description);
    send_to_char(buf, ch);
    return;
  }

  if (!*argument) {
    if (ch->char_specials.subscribe) {
      send_to_char("Your subscriber list contains:\r\n", ch);
      for (veh = ch->char_specials.subscribe; veh; veh = veh->next_sub) {
        sprintf(buf, "%2d) %-30s (At %s) [%2d/10] Damage\r\n", i, veh->short_description,
                world[veh->in_room].name, veh->damage);
        send_to_char(buf, ch);
        i++;
      }
    } else
      send_to_char("Your subscriber list is empty.\r\n", ch);
    return;
  }


  one_argument(argument, buf);
  if (ch->in_veh)
    veh = get_veh_list(buf, world[ch->in_veh->in_room].vehicles);
  else
    veh = get_veh_list(buf, world[ch->in_room].vehicles);
  if (!veh) {
    send_to_char("You don't see that here.\r\n", ch);
    return;
  }
  if (veh->owner != GET_IDNUM(ch)) {
    send_to_char("That's not yours.\r\n", ch);
    return;
  }
  if (veh->sub) {
    send_to_char("That is already part of your subscriber list.\r\n", ch);
    return;
  }
  veh->sub = TRUE;
  veh->next_sub = ch->char_specials.subscribe;
  ch->char_specials.subscribe = veh;
  sprintf(buf, "You add %s to your subscriber list.\r\n", veh->short_description);
  send_to_char(buf, ch);
}

ACMD(do_repair)
{
  struct obj_data *obj, *shop = NULL;
  ;
  struct veh_data *veh;
  int target = 4, skill = 0, success, mod = 0;

  one_argument(argument, buf1);

  if (!*buf1) {
    send_to_char("What do you want to repair?\r\n", ch);
    return;
  }
  if (ch->in_veh) {
    send_to_char("Well, you could probably polish the dash from in here.\r\n", ch);
    return;
  }
  if (!(veh = get_veh_list(buf1, world[ch->in_room].vehicles))) {
    send_to_char("You can't seem to find that vehicle here.\r\n", ch);
    return;
  }

  if (veh->damage == 0) {
    send_to_char(ch, "But it's not damaged!\r\n");
    return;
  }

  switch(veh->type) {
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
      skill = GET_SKILL(ch, SKILL_BR_BIKE) / 4;
    if (!skill)
      skill = GET_SKILL(ch, SKILL_BR_DRONE) / 4;
    break;
  }
  if (!skill) {
    send_to_char(ch, "You have no idea what you are doing...\r\n");
    return;
  }
  target += (veh->damage - 2) / 2;
  target += modify_target(ch);

  for (obj = ch->carrying; obj && !mod; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_WORKING_GEAR && GET_OBJ_VAL(obj, 0) == 4)
      mod = GET_OBJ_VAL(obj, 1);
  for (int i = 0; !mod && i < (NUM_WEARS - 1); i++)
    if ((obj = GET_EQ(ch, i)) && GET_OBJ_TYPE(obj) == ITEM_WORKING_GEAR && GET_OBJ_VAL(obj, 0) == 4)
      mod = GET_OBJ_VAL(obj, 1);
  for (obj = world[ch->in_room].contents; obj && !shop; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_WORKSHOP && GET_OBJ_VAL(obj, 0) == 4 && GET_OBJ_VAL(obj, 1))
      shop = obj;
  if (!shop)
    target += 2;

  switch (mod) {
  case 0:
    target += 4;
    break;
  case 1:
    target += 2;
    break;
  case 3:
    target -= 2;
    break;
  }

  if ((success = success_test(skill, target)) < 1) {
    send_to_char(ch, "It seems to be beyond your skill.\r\n");
    return;
  }
  veh->damage -= (success + 1) / 2;
  if (veh->damage <= 0) {
    send_to_char(ch, "You go to work on the vehicle and it looks as good as new.\r\n");
    veh->damage = 0;
  } else
    send_to_char(ch, "You go to work and repair part of the damage.\r\n");
  WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}

ACMD(do_driveby)
{
  struct obj_data *wielded;
  struct char_data *vict, *pass, *list = NULL;
  int dir;

  if (!ch->in_veh) {
    send_to_char(ch, "You must be in a vehicle to do that.\r\n");
    return;
  }
  if (!AFF_FLAGGED(ch, AFF_PILOT)) {
    if (PLR_FLAGGED(ch, PLR_DRIVEBY))
      send_to_char(ch, "You will no longer perform a drive-by.\r\n");
    else
      send_to_char(ch, "You will now perform a drive-by.\r\n");
    PLR_FLAGS(ch).ToggleBit(PLR_DRIVEBY);
    return;
  }

  two_arguments(argument, arg, buf2);
  if (!*arg || !*buf2) {
    send_to_char(ch, "Syntax is driveby <character> <direction to leave>.\r\n");
    return;
  }
  if (!(vict = get_char_room(arg, ch->in_veh->in_room))) {
    send_to_char(ch, "You don't see them there.\r\n");
    return;
  }
  if ((dir = search_block(buf2, lookdirs, FALSE)) == -1) {
    send_to_char("What direction?\r\n", ch);
    return;
  }
  dir = convert_look[dir];
  if (!EXIT(ch->in_veh, dir)) {
    send_to_char("That would just be crazy...\r\n", ch);
    return;
  }

  if (ch->in_veh->cspeed <= SPEED_IDLE) {
    send_to_char("How do you expect to driveby from a stationary vehicle?\r\n", ch);
    return;
  }

  if ((wielded = GET_EQ(ch, WEAR_WIELD))) {
    if (!(GET_OBJ_VAL(wielded, 4) == SKILL_PISTOLS || GET_OBJ_VAL(wielded, 4) == SKILL_SMG)) {
      send_to_char(ch, "You can only do a driveby with a pistol or SMG.\r\n");
      return;
    }
    if (!IS_NPC(vict) && !PRF_FLAGGED(ch, PRF_PKER) && !PRF_FLAGGED(vict, PRF_PKER)) {
      send_to_char(ch, "You have to be flagged PK to attack another player.\r\n");
      return;
    }
    sprintf(buf, "You point %s towards %s and open fire!\r\n", GET_OBJ_NAME(wielded), GET_NAME(vict));
    send_to_char(ch, buf);
    sprintf(buf, "%s aims %s towards %s and opens fire!\r\n", GET_NAME(ch), GET_OBJ_NAME(wielded), GET_NAME(vict));
    send_to_veh(buf, ch->in_veh, ch, FALSE);
    list = ch;
    roll_individual_initiative(ch);
    GET_INIT_ROLL(ch) += 20;
  } else {
    sprintf(buf, "You point towards %s and shout, \"Light 'em Up!\".\r\n", GET_NAME(vict));
    send_to_char(ch, buf);
    sprintf(buf, "%s points towards %s and shouts, \"Light 'em Up!\".!\r\n", GET_NAME(ch), GET_NAME(vict));
    send_to_veh(buf, ch->in_veh, ch, FALSE);
  }

  for (pass = ch->in_veh->people; pass; pass = pass->next_in_veh) {
    if (pass != ch && PLR_FLAGGED(pass, PLR_DRIVEBY) && AWAKE(pass) && GET_EQ(pass, WEAR_WIELD) &&
        GET_OBJ_VAL(GET_EQ(pass, WEAR_WIELD),4) >= SKILL_FIREARMS) {
      if (!IS_NPC(vict) && !PRF_FLAGGED(pass, PRF_PKER) && !PRF_FLAGGED(vict, PRF_PKER))
        continue;
      sprintf(buf, "You follow %s lead and take aim.\r\n", HSHR(ch));
      send_to_char(pass, buf);
      PLR_FLAGS(pass).RemoveBit(PLR_DRIVEBY);
      pass->next_fighting = list;
      list = pass;
      roll_individual_initiative(pass);
      GET_INIT_ROLL(pass) += 20;
    }
  }
  if (!list) {
    send_to_char(ch, "But no one is in the car to hear you.\r\n");
    return;
  }
  order_list(list);
  roll_individual_initiative(vict);
  if (GET_INIT_ROLL(vict) < 12)
    sprintf(buf, "You suddenly feel a cap busted in your ass as %s zooms past!\r\n", ch->in_veh->short_description);
  else
    sprintf(buf, "A hail of bullets flies from %s as it zooms past!\r\n", ch->in_veh->short_description);
  send_to_char(vict, buf);

  for (int i = 2; i > 0; i--) {
    for (pass = list; pass && vict->in_room == ch->in_veh->in_room; pass = pass->next_fighting) {
      if (GET_INIT_ROLL(pass) >= 0) {
        hit(pass, vict, TYPE_UNDEFINED);
        GET_INIT_ROLL(pass) -= 10;
      }
    }
  }

  for (;list ; list = pass) {
    pass = list->next_fighting;
    list->next_fighting = NULL;
  }

  if (GET_INIT_ROLL(vict) >= 12) {
    if (GET_EQ(vict, WEAR_WIELD) && GET_OBJ_VAL(GET_EQ(vict, WEAR_WIELD),4) >= SKILL_FIREARMS) {
      sprintf(buf, "You swing around and take aim at %s.\r\n", ch->in_veh->short_description);
      send_to_char(vict, buf);
      sprintf(buf, "%s swings around and takes aim at your ride.\r\n", GET_NAME(vict));
      send_to_veh(buf, ch->in_veh, NULL, TRUE);
      vcombat(vict, ch->in_veh);
    }
  }
  if (ch->in_veh)
    move_vehicle(ch, dir);
}

ACMD(do_speed)
{
  struct veh_data *veh;
  int i = 0;

  if (!(AFF_FLAGGED(ch, AFF_PILOT) || PLR_FLAGGED(ch, PLR_REMOTE))) {
    send_to_char("You must be driving a vehicle to do that.\r\n", ch);
    return;
  }
  one_argument(argument, buf);
  if (!*buf) {
    send_to_char("What speed?\r\n", ch);
    return;
  }

  switch (LOWER(*buf)) {
  case 'i':
    i = 1;
    break;
  case 'c':
    i = 2;
    break;
  case 's':
    i = 3;
    break;
  case 'm':
    i = 4;
    break;
  default:
    send_to_char("That is not a valid speed.\r\n", ch);
    return;
    break;
  }

  RIG_VEH(ch, veh);
  if (i < veh->cspeed) {
    if (i == 1) {
      send_to_char("You bring the vehicle to a halt.\r\n", ch);
      send_to_veh("The vehicle slows to a stop.\r\n", veh, ch, FALSE);
    } else {
      send_to_char("You put your foot on the brake.\r\n", ch);
      send_to_veh("You slow down.", veh, ch, FALSE);
    }
  } else if (i > veh->cspeed) {
    send_to_char("You put your foot on the accelerator.\r\n", ch);
    send_to_veh("You speed up.", veh, ch, FALSE);
  } else {
    send_to_char("But you're already travelling that fast!\r\n", ch);
    return;
  }
  veh->cspeed = i;
}

ACMD(do_chase)
{
  struct veh_data *veh, *tveh;
  struct veh_follow *k;
  struct char_data *vict;

  if (!(AFF_FLAGGED(ch, AFF_PILOT) || PLR_FLAGGED(ch, PLR_REMOTE))) {
    send_to_char("You have to be controlling a vehicle to do that.\r\n", ch);
    return;
  }
  RIG_VEH(ch, veh);

  if (veh->following) {
    sprintf(buf, "You stop following a %s.\r\n", veh->following->short_description);
    stop_chase(veh);
    send_to_char(buf, ch);
    return;
  } else if (veh->followch) {
    sprintf(buf, "You stop following %s.\r\n", GET_NAME(veh->followch));
    veh->followch = NULL;
    send_to_char(buf, ch);
    return;
  }
  two_arguments(argument, arg, buf2);

  if (!*arg) {
    send_to_char("Chase what?\r\n", ch);
    return;
  }
  if (!(tveh = get_veh_list(arg, world[veh->in_room].vehicles)) &&
      !(vict = get_char_room(arg, veh->in_room))) {
    send_to_char("You don't see that here.\r\n", ch);
    return;
  }

  if (tveh == veh) {
    send_to_char(ch, "You cannot chase yourself.\r\n");
    return;
  }
  if (tveh) {
    sprintf(buf, "You start chasing %s.\r\n", tveh->short_description);
    send_to_char(ch, buf);
    veh->following = tveh;
    veh->dest = NOWHERE;
    k = new veh_follow;
    k->follower = veh;
    if (tveh->followers)
      k->next = tveh->followers;
    tveh->followers = k;
    if (veh->cspeed == SPEED_IDLE) {
      send_to_char(ch, "You put your foot on the accelarator.\r\n");
      send_to_veh("You speed up.\r\n", veh, ch, FALSE);
      veh->cspeed = SPEED_CRUISING;
    }
  } else if (vict) {}
}

ACMD(do_target)
{
  struct char_data *vict = NULL;
  struct veh_data *veh, *tveh = NULL;
  struct obj_data *obj;
  bool modeall = FALSE;
  int j;
  RIG_VEH(ch, veh);
  if (!veh) {
    send_to_char("You must be in a vehicle to do that.\r\n", ch);
    return;
  }
  if (!veh->mount) {
    send_to_char("There is nothing to target.\r\n", ch);
    return;
  }
  two_arguments(argument, arg, buf2);
  if (!*arg) {
    send_to_char("Target what at who?\r\n", ch);
    return;
  }
  if (*arg && *buf2) {
    if (!(AFF_FLAGGED(ch, AFF_RIG) || PLR_FLAGGED(ch, PLR_REMOTE))) {
      send_to_char("But you aren't piloting this vehicle.\r\n", ch);
      return;
    }
    if ((j = atoi(arg)) < 0) {
      send_to_char("Target mount number what?\r\n", ch);
      return;
    }
    for (obj = veh->mount; obj; obj = obj->next_content)
      if (--j < 0)
        break;
    if (!obj) {
      send_to_char("There aren't that many mounts.\r\n", ch);
      return;
    }
    if (!obj->contains) {
      send_to_char("It has no weapon mounted.\r\n", ch);
      return;
    }
    strcpy(arg, buf2);
  } else {
    if (!(AFF_FLAGGED(ch, AFF_RIG) || PLR_FLAGGED(ch, PLR_REMOTE) || AFF_FLAGGED(ch, AFF_MANNING))) {
      send_to_char("You don't have control over any mounts.\r\n", ch);
      return;
    }
    if (AFF_FLAGGED(ch, AFF_MANNING)) {
      for (obj = veh->mount; obj; obj = obj->next_content)
        if (obj->worn_by == ch)
          break;
    } else {
      for (j = 0, obj = veh->mount; obj; obj = obj->next_content)
        if (obj->contains && !obj->worn_by)
          j++;
      if (!j) {
        send_to_char("You have nothing to target.\r\n", ch);
        return;
      }
      modeall = TRUE;
    }
  }
  if (!(vict = get_char_room(arg, veh->in_room)) &&
      !(tveh = get_veh_list(arg, world[veh->in_room].vehicles))) {
    send_to_char(ch, "You don't see %s there.\r\n", arg);
    return;
  }
  if (tveh == veh) {
    send_to_char("Why would you want to target yourself?", ch);
    return;
  }
  if (FIGHTING(ch) || FIGHTING_VEH(ch))
    stop_fighting(ch);
  if (modeall) {
    for (obj = veh->mount; obj; obj = obj->next_content)
      if (obj->contains && !obj->worn_by)
        if (vict) {
          set_fighting(ch, vict);
          if (obj->tveh)
            obj->tveh = NULL;
          obj->targ = vict;
          set_fighting(vict, veh);
          sprintf(buf, "You target %s towards %s.\r\n", GET_OBJ_NAME(obj->contains), PERS(vict, ch));
          send_to_char(buf, ch);
        } else {
          set_fighting(ch, tveh);
          if (obj->targ)
            obj->targ = NULL;
          obj->tveh = tveh;
          sprintf(buf, "You target %s towards %s.\r\n", GET_OBJ_NAME(obj->contains), tveh->short_description);
          send_to_char(buf, ch);
        }
    return;
  }
  if (vict) {
    set_fighting(ch, vict);
    if (obj->tveh)
      obj->tveh = NULL;
    obj->targ = vict;
    set_fighting(vict, veh);
    act("You target $o towards $N.", FALSE, ch, obj->contains, vict, TO_CHAR);
    if (AFF_FLAGGED(ch, AFF_MANNING)) {
      sprintf(buf, "%s's %s swivels towards you.\r\n", ch->in_veh->short_description, GET_OBJ_NAME(obj));
      send_to_char(buf, vict);
    }
  } else {
    set_fighting(ch, tveh);
    if (obj->targ)
      obj->targ = NULL;
    obj->tveh = tveh;
    sprintf(buf, "You target %s towards %s.\r\n", GET_OBJ_NAME(obj->contains), tveh->short_description);
    send_to_char(buf, ch);
    if (AFF_FLAGGED(ch, AFF_MANNING)) {
      sprintf(buf, "%s's %s swivels towards your ride.\r\n", ch->in_veh->short_description, GET_OBJ_NAME(obj));
      send_to_veh(buf, tveh, 0, TRUE);
    }
  }
}

ACMD(do_mount)
{
  int i = 0;
  struct veh_data *veh;
  struct obj_data *obj;
  RIG_VEH(ch, veh);
  if (!veh) {
    send_to_char("You must be in a vehicle to use that.\r\n", ch);
    return;
  }

  sprintf(buf, "%s is mounting the following:\r\n", CAP(veh->short_description));
  send_to_char(buf, ch);
  for (obj = veh->mount; obj; obj = obj->next_content) {
    sprintf(buf, "%2d) %-20s (Mounting %s)", i, GET_OBJ_NAME(obj), (obj->contains ?
            GET_OBJ_NAME(obj->contains) : "Nothing"));
    if (obj->worn_by)
      sprintf(buf + strlen(buf), " (Manned by %s)", (found_mem(GET_MEMORY(ch), obj->worn_by) ?
              (found_mem(GET_MEMORY(ch), obj->worn_by))->mem
              : GET_NAME(obj->worn_by)));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    i++;
  }
}

ACMD(do_man)
{
  struct obj_data *mount;
  int j;
  if (!ch->in_veh) {
    send_to_char("But you're not in a vehicle.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_PILOT)) {
    send_to_char(ch, "You can't do that now!\r\n");
    return;
  }

  if (!*argument && AFF_FLAGGED(ch, AFF_MANNING)) {
    for (mount = ch->in_veh->mount; mount; mount = mount->next_content)
      if (mount->worn_by == ch)
        break;
    mount->worn_by = NULL;
    AFF_FLAGS(ch).ToggleBit(AFF_MANNING);
    act("$n stops manning $o.", FALSE, ch, mount, 0, TO_ROOM);
    act("You stop manning $o.", FALSE, ch, mount, 0, TO_CHAR);
    return;
  }

  if ((j = atoi(argument)) < 0) {
    send_to_char("Which mount?", ch);
    return;
  }
  for (mount = ch->in_veh->mount; mount; mount = mount->next_content)
    if (--j < 0)
      break;

  if (!mount) {
    send_to_char("There are not that many mounts?\r\n", ch);
    return;
  }
  if (!mount->contains) {
    send_to_char("But there is no weapon mounted there.\r\n", ch);
  }
  if (mount->worn_by) {
    send_to_char("Someone is already manning it.\r\n", ch);
    return;
  }
  mount->worn_by = ch;
  act("$n mans $o.", FALSE, ch, mount, 0, TO_ROOM);
  act("You man $o.", FALSE, ch, mount, 0, TO_CHAR);
  AFF_FLAGS(ch).ToggleBit(AFF_MANNING);
}

ACMD(do_gridguide)
{
  struct veh_data *veh;
  struct grid_data *grid = NULL;
  vnum_t x = 0, y = 0;
  RIG_VEH(ch, veh);

  if (!veh) {
    send_to_char("You have to be in a vehicle to do that.\r\n", ch);
    return;
  }
  if (!veh->autonav) {
    send_to_char("You need to have an autonav system installed.\r\n", ch);
    return;
  }
  argument = two_arguments(argument, arg, buf2);
  if (!*arg) {
    int i = 0;
    send_to_char("The following destinations are available:\r\n", ch);
    for (grid = veh->grid; grid; grid = grid->next) {
      i++;
      if (find_first_step(veh->in_room, real_room(grid->room)) < 0)
        sprintf(buf, "^r%-20s [%-6ld, %-6ld](Unavailable)\r\n", CAP(grid->name),
                grid->room - (grid->room * 3), grid->room + 100);
      else
        sprintf(buf, "^B%-20s [%-6ld, %-6ld](Available)\r\n", CAP(grid->name),
                grid->room - (grid->room * 3), grid->room + 100);
      send_to_char(buf, ch);
    }
    send_to_char(ch, "%d Entries remaining.\r\n", (veh->autonav * 5) - i);
  } else if (!*buf2) {
    if ((veh->cspeed > SPEED_IDLE && !AFF_FLAGGED(ch, AFF_PILOT)) ||
        (veh->locked && GET_IDNUM(ch) != veh->owner) && !PLR_FLAGGED(ch, PLR_REMOTE)) {
      send_to_char("You don't have control over the vehicle.\r\n", ch);
      return;
    }
    for (grid = veh->grid; grid; grid = grid->next)
      if (is_abbrev(arg, grid->name))
        break;
    if (!grid)
      send_to_char("That destination doesn't seem to be in the system.\r\n", ch);
    else if (find_first_step(veh->in_room, real_room(grid->room)) < 0)
      send_to_char("That destination is currently unavailable.\r\n", ch);
    else {
      veh->dest = real_room(grid->room);
      veh->cspeed = SPEED_CRUISING;
      if (AFF_FLAGGED(ch, AFF_PILOT))
        AFF_FLAGS(ch).RemoveBits(AFF_PILOT, AFF_RIG, ENDBIT);
      send_to_veh("The autonav beeps and the vehicle starts to move towards its destination.\r\n", veh, 0, TRUE);
    }
  } else if (!str_cmp(arg, "del")) {
    struct grid_data *temp;
    for (grid = veh->grid; grid; grid = grid->next)
      if (!str_cmp(buf2, grid->name))
        break;
    if (!grid)
      send_to_char("That destination doesn't seem to be in the system.\r\n", ch);
    else {
      REMOVE_FROM_LIST(grid, veh->grid, next);
      send_to_char("You remove the destination from the system.\r\n", ch);
      act("$n punches something into the autonav.", FALSE, ch, 0 , 0, TO_ROOM);
    }
  } else if (!str_cmp(arg, "stop")) {
    if ((veh->locked && GET_IDNUM(ch) != veh->owner) && !PLR_FLAGGED(ch, PLR_REMOTE)) {
      send_to_char("You don't have control over the vehicle.\r\n", ch);
      return;
    }
    veh->dest = 0;
    send_to_veh("The autonav disengages.\r\n", veh, 0, TRUE);
    if (!AFF_FLAGGED(ch, AFF_PILOT))
      veh->cspeed = SPEED_OFF;
  } else if (!str_cmp(arg, "add")) {
    int i = 0;
    for (grid = veh->grid; grid; grid = grid->next) {
      if (!str_cmp(buf2, grid->name)) {
        send_to_char("That entry already exists.\r\n", ch);
        return;
      }
      i++;
    }
    if (i >= (veh->autonav * 5)) {
      send_to_char("The system seems to be full.\r\n", ch);
      return;
    }
    if (!*argument) {
      if (!(ROOM_FLAGGED(veh->in_room, ROOM_ROAD) || ROOM_FLAGGED(veh->in_room, ROOM_GARAGE)) ||
          ROOM_FLAGGED(veh->in_room, ROOM_NOGRID)) {
        send_to_char("You currently aren't on the grid.\r\n", ch);
        return;
      }
      x = veh->in_room;
    } else {
      two_arguments(argument, arg, buf);
      if (!*buf) {
        send_to_char("You need a second co-ordinate.\r\n", ch);
        return;
      }
      if (!((x = atoi(arg)) && (y = atoi(buf)))) {
        send_to_char("Those co-ordinates seem invalid.\r\n", ch);
        return;
      }
      x = (x + ((y - 100) * 3));
      if (!(x = real_room(x)) || !(ROOM_FLAGGED(x, ROOM_ROAD) || ROOM_FLAGGED(x, ROOM_GARAGE)) ||
          ROOM_FLAGGED(x, ROOM_NOGRID)) {
        send_to_char("Those co-ordinates seem invalid.\r\n", ch);
        return;
      }
    }
    grid = new grid_data;
    grid->name = str_dup(buf2);
    grid->room = world[x].number;
    grid->next = veh->grid;
    veh->grid = grid;
    send_to_char("You add the destination into the system.\r\n", ch);
    act("$n punches something into the autonav.", FALSE, ch, 0 , 0, TO_ROOM);
  }
}

void process_autonav(void)
{
  for (struct veh_data *veh = veh_list; veh; veh = veh->next)
    if (veh->dest && veh->cspeed > SPEED_IDLE && veh->damage < 10) {
      struct char_data *ch = NULL;
      if (!(ch = veh->rigger))
        ch = veh->people;
      // Stop empty vehicles
      if (!ch) {
        veh->dest = 0;
        veh->cspeed = SPEED_OFF;
      }
      int dir = 0;
      for (int x = (int)get_speed(veh) / 10; x && dir >= 0 && veh->dest; x--) {
        dir = find_first_step(veh->in_room, veh->dest);
        if (dir >= 0)
          move_vehicle(ch, dir);
      }
      if (veh->in_room == veh->dest) {
        send_to_veh("Having reached its destination, the autonav shuts off.\r\n", veh, 0, TRUE);
        if (!(ch = veh->rigger))
          for (ch = veh->people; ch; ch = ch->next_in_veh)
            if (AFF_FLAGGED(ch, AFF_PILOT))
              break;
        if (!ch)
          veh->cspeed = SPEED_OFF;
        veh->dest = 0;
      }
    }
}

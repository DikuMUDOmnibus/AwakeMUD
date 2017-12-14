/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "newmagic.h"
#include "constants.h"
#include "config.h"
#include "memory.h"
#include "newmatrix.h"

int pass = 0;

/* Structures */
struct char_data *combat_list = NULL;   /* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct message_list fight_messages[MAX_MESSAGES];


int find_sight(struct char_data *ch);
void damage_door(struct char_data *ch, int room, int dir, int power, int type);
void damage_obj(struct char_data *ch, struct obj_data *obj, int power, int type);
void mount_fire(struct char_data *ch);
/* External procedures */
char *fread_action(FILE * fl, int nr);
char *fread_string(FILE * fl, char *error);
void stop_follower(struct char_data * ch);
ACMD(do_assist);
ACMD(do_flee);
ACMD(do_action);
void docwagon(struct char_data *ch);
void range_combat(struct char_data *ch, char target[MAX_INPUT_LENGTH],
                  struct obj_data *weapon, int range, int dir);
void roll_individual_initiative(struct char_data *ch);
void ranged_response(struct char_data *ch, struct char_data *vict);
int find_weapon_range(struct char_data *ch, struct obj_data *weapon);
void weapon_scatter(struct char_data *ch, struct char_data *victim,
                    struct obj_data *weapon);
void explode(struct char_data *ch, struct obj_data *weapon, int room);
void target_explode(struct char_data *ch, struct obj_data *weapon,
                    int room, int mode);
void forget(struct char_data * ch, struct char_data * victim);
void remember(struct char_data * ch, struct char_data * victim);
void order_list(...);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
extern int success_test(int number, int target);
extern int resisted_test(int num_for_ch, int tar_for_ch, int num_for_vict,
                           int tar_for_vict);
extern int modify_target(struct char_data *ch);
extern int skill_web (struct char_data *ch, int skillnumber);
extern bool mob_magic(struct char_data * mob);
extern int reverse_web(struct char_data *, int &, int &);
extern bool attempt_reload(struct char_data *mob, int pos);
extern void switch_weapons(struct char_data *mob, int pos);
extern void hunt_victim(struct char_data * ch);
extern void matrix_fight(struct char_data *ch, struct char_data *victim);
extern void check_quest_kill(struct char_data *ch, struct char_data *victim);
extern void check_quest_destroy(struct char_data *ch, struct obj_data *obj);
extern void purge_spell_q(struct char_data *ch);
extern bool cycle_spell_q(struct char_data *ch);
extern int return_general(int);
extern struct zone_data *zone_table;
extern void perform_tell(struct char_data *, struct char_data *, char *);
extern int can_wield_both(struct char_data *, struct obj_data *, struct obj_data *);
extern void draw_weapon(struct char_data *);
extern void crash_test(struct char_data *ch);
extern int modify_veh(struct veh_data *veh);
/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
  {
    {"hit", "hits", "hit"
    }
    ,               /* 0 */
    {"sting", "stings", "sting"},
    {"whip", "whips", "whip"},
    {"slash", "slashes", "slash"},
    {"bite", "bites", "bite"},
    {"bludgeon", "bludgeons", "bludgeon"},        /* 5 */
    {"crush", "crushes", "crush"},
    {"pound", "pounds", "pound"},
    {"claw", "claws", "claw"},
    {"maul", "mauls", "maul"},
    {"thrash", "thrashes", "thrash"},     /* 10 */
    {"pierce", "pierces", "pierce"},
    {"punch", "punches", "punch"},
    {"stab", "stabs", "stab"},
    {"shock", "shocks", "shock"},
    {"shuriken", "shurikens", "shuriken"}, /* 15 */
    {"pierce", "pierces", "pierce"},
    {"pierce", "pierces", "pierce"},
    {"grenade", "grenades", "grenade"},
    {"grenade", "grenades", "grenade"},
    {"rocket", "rockets", "rocket"},  /* 20 */
    {"shoot", "shoots", "shot"},
    {"blast", "blasts", "blast"},
    {"shoot", "shoots", "shot"},
    {"blast", "blasts", "blast"},
    {"blast", "blasts", "burst fire"},    /* 25 */
    {"blast", "blasts", "blast"},
    {"bifurcate", "bifurcates", "BIFURCATION"}
  };
int cmd_slp;

void set_attacking(struct char_data *ch, struct char_data *vict, const char *file, int line)
{
  //ch->char_specials.fightList.ADD(vict);
  //vict->char_specials.defendList.ADD(ch);
}

/* The Fight related routines */

void appear(struct char_data * ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBILITY) == 1)
    affect_from_char(ch, SPELL_INVISIBILITY);

  if (affected_by_spell(ch, SPELL_IMPROVED_INVIS) == 1)
    affect_from_char(ch, SPELL_IMPROVED_INVIS);

  AFF_FLAGS(ch).RemoveBits(AFF_IMP_INVIS,
                           AFF_INVISIBLE,
                           AFF_HIDE, ENDBIT);

  if (!IS_SENATOR(ch))
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
        FALSE, ch, 0, 0, TO_ROOM);
}

void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    shutdown();
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }

  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
         (fight_messages[i].a_type); i++)
      ;
    if (i >= MAX_MESSAGES) {
      fprintf(stderr, "Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      shutdown();
    }
    messages = new message_type;
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
  cmd_slp = find_command("slap");
}

void update_pos(struct char_data * victim)
{
  if ((GET_MENTAL(victim) < 100) && (GET_PHYSICAL(victim) >= 100))
  {
    GET_POS(victim) = POS_STUNNED;
    GET_INIT_ROLL(victim) = 0;
    return;
  }

  if ((GET_PHYSICAL(victim) >= 100) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_PHYSICAL(victim) >= 100)
  {
    GET_POS(victim) = POS_STANDING;
    return;
  } else if ((int)(GET_PHYSICAL(victim) / 100) <= -GET_BOD(victim))
    GET_POS(victim) = POS_DEAD;
  else
    GET_POS(victim) = POS_MORTALLYW;

  GET_INIT_ROLL(victim) = 0;
}

/* blood blood blood, root */
void increase_blood(int rm)
{
  RM_BLOOD(rm) = MIN(RM_BLOOD(rm) + 1, 10);
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  char_data *attacker;
  char buf[256];

  if (IS_NPC(ch) && (ch->desc == NULL || ch->desc->original == NULL))
    return;

  if (!IS_NPC(ch))
    attacker = ch;
  else
    attacker = ch->desc->original;

  if (!IS_NPC(vict) &&
      !PLR_FLAGS(vict).AreAnySet(PLR_KILLER, ENDBIT) &&
      (!PRF_FLAGGED(attacker, PRF_PKER) || !PRF_FLAGGED(vict, PRF_PKER)) &&
      !PLR_FLAGGED(attacker, PLR_KILLER) && attacker != vict && !IS_SENATOR(attacker))
  {
    PLR_FLAGS(attacker).SetBit(PLR_KILLER);

    sprintf(buf, "PC Killer but set on %s for initiating attack on %s at %s.",
            GET_CHAR_NAME(attacker),
            GET_CHAR_NAME(vict), world[vict->in_room].name);
    mudlog(buf, ch, LOG_MISCLOG, TRUE);

    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict, ...)
{
  struct follow_type *k;
  if (ch == vict)
    return;

  if (FIGHTING(ch) || FIGHTING_VEH(ch))
    return;

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    AFF_FLAGS(ch).RemoveBit(AFF_SLEEP);

  roll_individual_initiative(ch);
  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!(AFF_FLAGGED(ch, AFF_MANNING) || PLR_FLAGGED(ch, PLR_REMOTE) || AFF_FLAGGED(ch, AFF_RIG)))
  {
    if (!(GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0)) &&
        !(GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1)))
      draw_weapon(ch);

    if (GET_EQ(ch, WEAR_WIELD))
      if (!IS_GUN(GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3)) &&
          GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_ARROW)
        AFF_FLAGS(ch).SetBit(AFF_APPROACH);
    if (GET_EQ(ch, WEAR_HOLD))
      if (!IS_GUN(GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 3)) &&
          GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 3) != TYPE_ARROW)
        AFF_FLAGS(ch).SetBit(AFF_APPROACH);
    if (!GET_EQ(ch, WEAR_WIELD) && !GET_EQ(ch, WEAR_HOLD))
      AFF_FLAGS(ch).SetBit(AFF_APPROACH);
  }
  check_killer(ch, vict);

  if (IS_NPC(ch))
    strcpy(buf, GET_NAME(ch));
  else
    strcpy(buf, GET_CHAR_NAME(ch));

  if (ch->followers)
    for (k = ch->followers; k; k = k->next)
      if (PRF_FLAGGED(k->follower, PRF_ASSIST) && k->follower->in_room == ch->in_room &&
          !FIGHTING(k->follower) && AWAKE(k->follower))
        do_assist(k->follower, buf, 0, 0);
  if (ch->master)
  {
    if (PRF_FLAGGED(ch->master, PRF_ASSIST) && ch->master->in_room == ch->in_room &&
        !FIGHTING(ch->master) && AWAKE(ch->master))
      do_assist(ch->master, buf, 0, 0);
    for (k = ch->master->followers; k; k = k->next)
      if (PRF_FLAGGED(k->follower, PRF_ASSIST) && k->follower->in_room == ch->in_room
          && k->follower != ch && !FIGHTING(k->follower) && AWAKE(k->follower))
        do_assist(k->follower, buf, 0, 0);
  }
}

void set_fighting(struct char_data * ch, struct veh_data * vict)
{
  struct follow_type *k;

  if (FIGHTING(ch) || FIGHTING_VEH(ch))
    return;

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    AFF_FLAGS(ch).RemoveBit(AFF_SLEEP);

  roll_individual_initiative(ch);
  FIGHTING_VEH(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!(GET_EQ(ch, WEAR_WIELD) && GET_WIELDED(ch, 0)) &&
      !(GET_EQ(ch, WEAR_HOLD) && GET_WIELDED(ch, 1)))
    draw_weapon(ch);

  if (IS_NPC(ch))
    strcpy(buf, GET_NAME(ch));
  else
    strcpy(buf, GET_CHAR_NAME(ch));
  if (ch->followers)
    for (k = ch->followers; k; k = k->next)
      if (PRF_FLAGGED(k->follower, PRF_ASSIST) && k->follower->in_room == ch->in_room &&
          !FIGHTING(k->follower) && AWAKE(k->follower))
        do_assist(k->follower, buf, 0, 0);
  if (ch->master)
  {
    if (PRF_FLAGGED(ch->master, PRF_ASSIST) && ch->master->in_room == ch->in_room &&
        !FIGHTING(ch->master) && AWAKE(ch->master))
      do_assist(ch->master, buf, 0, 0);
    for (k = ch->master->followers; k; k = k->next)
      if (PRF_FLAGGED(k->follower, PRF_ASSIST) && k->follower->in_room == ch->in_room
          && k->follower != ch && !FIGHTING(k->follower) && AWAKE(k->follower))
        do_assist(k->follower, buf, 0, 0);
  }
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;
  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  if (IS_AFFECTED(ch, AFF_APPROACH))
    AFF_FLAGS(ch).RemoveBit(AFF_APPROACH);

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  FIGHTING(ch) = NULL;
  FIGHTING_VEH(ch) = NULL;
  ch->next_fighting = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
  purge_spell_q(ch);
  GET_INIT_ROLL(ch) = 0;
}

void make_corpse(struct char_data * ch)
{
  struct obj_data *create_nuyen(int amount);
  struct obj_data *create_credstick(struct char_data *ch, int amount);

  struct obj_data *corpse, *o, *obj;
  struct obj_data *money;
  int i, nuyen, credits, corpse_value = 0;
  if (PLR_FLAGGED(ch, PLR_NEWBIE))
    return;
  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;

  if (IS_NPC(ch))
  {
    sprintf(buf, "corpse %s", ch->player.physical_text.keywords);
    sprintf(buf1, "^rThe corpse of %s is lying here.^n", GET_NAME(ch));
    sprintf(buf2, "^rthe corpse of %s^n", GET_NAME(ch));
  } else
  {
    sprintf(buf, "belongings %s", ch->player.physical_text.keywords);
    sprintf(buf1, "^rThe belongings of %s are lying here.^n", GET_NAME(ch));
    sprintf(buf2, "^rthe belongings of %s^n", GET_NAME(ch));
  }
  corpse->text.keywords = str_dup(buf);
  corpse->text.room_desc = str_dup(buf1);
  corpse->text.name = str_dup(buf2);
  corpse->text.look_desc = str_dup("What once was living is no longer. Poor sap.\r\n");
  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  (corpse)->obj_flags.wear_flags.SetBits(ITEM_WEAR_TAKE, ENDBIT);
  (corpse)->obj_flags.extra_flags.SetBits(ITEM_NODONATE, ITEM_NORENT,
                                          ITEM_CORPSE, ENDBIT);

  GET_OBJ_VAL(corpse, 0) = 0;   /* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 4) = 1;   /* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = MAX(100, GET_WEIGHT(ch)) + IS_CARRYING_W(ch);
  if (IS_NPC(ch))
  {
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
    GET_OBJ_VAL(corpse, 4) = 0;
    GET_OBJ_VAL(corpse, 5) = ch->nr;
  } else
  {
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;
    if (PRF_FLAGGED(ch, PRF_PKER))
      GET_OBJ_VAL(corpse, 4) = 2;
    else
      GET_OBJ_VAL(corpse, 4) = 1;
    GET_OBJ_VAL(corpse, 5) = GET_IDNUM(ch);
    /* make 'em bullet proof...(anti-twink measure) */
    GET_OBJ_BARRIER(corpse) = 75;
  }

  /* transfer character's inventory to the corpse */
  for (o = ch->carrying; o; o = obj)
  {
    obj = o->next_content;
    obj_from_char(o);
    obj_to_obj(o, corpse);
    corpse_value += GET_OBJ_COST( o );
  }

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (ch->equipment[i]) {
      corpse_value += GET_OBJ_COST( ch->equipment[i] );
      obj_to_obj(unequip_char(ch, i), corpse);
    }
  }

  /* transfer nuyen & credstick */
  if (IS_NPC(ch))
  {
    nuyen = (int)(GET_NUYEN(ch) / 10);
    nuyen = number(GET_NUYEN(ch) - nuyen, GET_NUYEN(ch) + nuyen);
    credits = (int)(GET_BANK(ch) / 10);
    credits = number(GET_BANK(ch) - credits, GET_BANK(ch) + credits);
  } else
  {
    nuyen = GET_NUYEN(ch);
    credits = 0;
  }

  if (IS_NPC(ch) && (nuyen > 0 || credits > 0))
    if (from_ip_zone(GET_MOB_VNUM(ch)))
    {
      nuyen = 0;
      credits = 0;
    }

  if (nuyen > 0)
  {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_nuyen(nuyen);
      obj_to_obj(money, corpse);
    }
    GET_NUYEN(ch) = 0;
  }

  if (credits > 0)
  {
    money = create_credstick(ch, credits);
    obj_to_obj(money, corpse);
  }

  if ( IS_NPC(ch) )
  {
    extern struct char_data *mob_proto;
    mob_proto[GET_MOB_RNUM(ch)].mob_specials.value_death_nuyen += credits + nuyen;
    mob_proto[GET_MOB_RNUM(ch)].mob_specials.value_death_items += corpse_value;
  }

  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  if (ch->in_veh)
    obj_to_veh(corpse, ch->in_veh);
  else
    obj_to_room(corpse, ch->in_room);
}

void death_cry(struct char_data * ch)
{
  int door, was_in;

  act("$n cries out $s last breath as $e dies!", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++)
  {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Somewhere close, you hear someone's death cry!", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}

void raw_kill(struct char_data * ch)
{
  struct obj_data *bio, *obj, *o;
  long i;

  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected, 0);

  if (IS_ASTRAL(ch))
  {
    act("$n vanishes.", FALSE, ch, 0, 0, TO_ROOM);
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
        extract_obj(GET_EQ(ch, i));
    for (obj = ch->carrying; obj; obj = o) {
      o = obj->next_content;
      extract_obj(obj);
    }
  } else
  {
    death_cry(ch);

    if (!(IS_SPIRIT(ch) && GET_MOB_VNUM(ch) >= 25 && GET_MOB_VNUM(ch) <= 42))
      make_corpse(ch);

    if (!IS_NPC(ch)) {
      for (bio = ch->bioware; bio; bio = bio->next_content)
        if (!GET_OBJ_VAL(bio, 2)) {
          GET_OBJ_VAL(bio, 5) = 12;
          GET_OBJ_VAL(bio, 6) = 0;
        } else if (GET_OBJ_VAL(bio, 2) == 4) {
          if (GET_OBJ_VAL(bio, 5) > 0)
            for (i = 0; i < MAX_OBJ_AFFECT; i++)
              affect_modify(ch, bio->affected[i].location,
                            bio->affected[i].modifier,
                            bio->obj_flags.bitvector, FALSE);
          GET_OBJ_VAL(bio, 5) = 0;
        }
      if (PLR_FLAGGED(ch, PLR_AUTH))
        i = real_room(60500);
      else if (zone_table[world[ch->in_room].zone].juridiction > 0)
        i = real_room(14709);
      else
        i = real_room(16295);
      char_from_room(ch);
      char_to_room(ch, i);
      PLR_FLAGS(ch).SetBit(PLR_JUST_DIED);
    }
  }

  extract_char(ch);
}

void death_penalty(struct char_data *ch)
{
  int attribute = 0;
  int old_rep = GET_TKE( ch );

  if(!IS_NPC(ch)
      && !PLR_FLAGGED(ch, PLR_NEWBIE)
      && GET_REAL_BOD(ch) > 1
      && !(success_test(GET_BOD(ch),4) >= 1 ))
  {
    do {
      attribute = number(0,5);
    } while (attribute == CHA);
    GET_TKE(ch) -= 2*GET_REAL_ATT(ch, attribute);
    GET_REAL_ATT(ch, attribute)--;
    if (attribute == BOD)
      GET_INDEX(ch) -= 100;
    sprintf(buf,"%s lost a point of attribute %d.  Total Karma Earned from %d to %d.",
            GET_CHAR_NAME(ch), attribute, old_rep, GET_TKE( ch ) );
    mudlog(buf, ch, LOG_DEATHLOG, TRUE);

  }
}

void die(struct char_data * ch)
{
  increase_blood(ch->in_room);
  act("^rBlood splatters everywhere!^n", FALSE, ch, 0, 0, TO_ROOM);
  if (!IS_NPC(ch))
  {
    death_penalty(ch);
    PLR_FLAGS(ch).RemoveBit(PLR_WANTED);
  }

  raw_kill(ch);
}

/*
 * Lets the player give up and die if they're at 0 or less
 * physical points.
 */
ACMD(do_die)
{
  char buf[100];

  /* If they're still okay... */
  if ( GET_PHYSICAL(ch) > 0 ) {
    send_to_char("Your mother would be so sad.. :(\n\r",ch);
    return;
  }

  send_to_char("You give up the will to live..\n\r",ch);

  /* log it */
  sprintf(buf,"%s gave up the will to live. {%s (%ld)}",
          GET_CHAR_NAME(ch),
          world[ch->in_room].name, world[ch->in_room].number );
  mudlog(buf, ch, LOG_DEATHLOG, TRUE);

  /* Now we just kill them, MuHahAhAhahhaAHhaAHaA!!...or something */
  die(ch);

  return;
}

int calc_karma(struct char_data *ch, struct char_data *vict)
{
  int base = 0, i, bonus_karma;

  if (!vict)
    return 0;

  base = (int)((1.5 * (GET_REAL_BOD(vict) + GET_BALLISTIC(vict) + GET_IMPACT(vict))) +
               (GET_REAL_QUI(vict) + (int)(vict->real_abils.mag / 100) +
                GET_REAL_STR(vict) + GET_REAL_WIL(vict) + GET_REAL_REA(vict)));

  for (i = 0; i <= MAX_SKILLS; i++)
    if (GET_SKILL(vict, i) > 0)
    {
      if (i == SKILL_SORCERY && GET_MAG(vict) > 0)
        base += (int)(1.15 * GET_SKILL(vict, i));
      else if (return_general(i) == SKILL_FIREARMS ||
                     return_general(i) == SKILL_UNARMED_COMBAT ||
                           return_general(i) == SKILL_ARMED_COMBAT)
        base += (int)(1.4 * GET_SKILL(vict, i));
    }

  if (MOB_FLAGGED(vict, MOB_GUARD))
    base = (int)(base * 1.1);
  if (MOB_FLAGGED(vict, MOB_NOBLIND))
    base = (int)(base * 1.05);
  if (MOB_FLAGGED(vict, MOB_IMMEXPLODE))
    base = (int)(base * 1.05);
  if (IS_NPC(vict) && GET_RACE(vict) == CLASS_SPIRIT)
    base = (int)(base * 1.15);

  if (IS_NPC(vict))
    bonus_karma = GET_KARMA(vict);

  else
    bonus_karma = 0;

  bonus_karma = MIN( bonus_karma, base );
  base += bonus_karma;

  if (ch && !IS_NPC(ch))
  {
    if (PLR_FLAGGED(ch, PLR_NEWBIE))
      base -= (int)(GET_TKE(ch) / 5);
    else
      base -= (int)(GET_TKE(ch) / 2);
  }

  //now to randomize it a bit
  base += (!ch ? 0 : (!number(0,2) ? number(0,5) :
                      0 - number(0,5)));

  base = (ch ? MIN(max_exp_gain, base) : base);
  base = MAX(base, 1);

  if (ch && !IS_NPC(ch) && !IS_SENATOR(ch) && IS_NPC(vict))
    if (from_ip_zone(GET_MOB_VNUM(vict)))
      base = 0;
  //if(ch && base  > 10 && !PLR_FLAGGED(ch,PLR_NEWBIE))
  //   base = 10;


  return base;
}

void perform_group_gain(struct char_data * ch, int base, struct char_data * victim)
{
  int share;

  share = MIN(max_exp_gain, MAX(1, base));
  if (!IS_NPC(ch))
    share = MIN(base, (PLR_FLAGGED(ch, PLR_NEWBIE) ? 20 : GET_TKE(ch) * 2));

  /* psuedo-fix of the group with a newbie to get more exp exploit */
  if ( !PLR_FLAGGED(ch, PLR_NEWBIE) )
    share /= 2;

  share = gain_exp(ch, share, 0);

  if ( share >= 100 || access_level(ch, LVL_BUILDER) )
  {
    sprintf(buf,"%s gains %.2f karma from killing %s.", GET_CHAR_NAME(ch),
            (double)share/100.0, GET_CHAR_NAME(victim));
    mudlog(buf, ch, LOG_DEATHLOG, TRUE);
  }
  if ( IS_NPC( victim ) )
  {
    extern struct char_data *mob_proto;
    mob_proto[GET_MOB_RNUM(victim)].mob_specials.value_death_karma += share;
  }

  send_to_char(ch, "You receive your share of %0.2f karma.\r\n", ((float)share / 100));
}

void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP)
        && f->follower->in_room == ch->in_room)
      tot_members++;

  base = calc_karma(k, victim);

  if (tot_members >= 1)
    base = MAX(1, (base / tot_members));
  else
    base = 0;

  if (IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room)
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      perform_group_gain(f->follower, base, victim);
}

char *replace_string(char *str, char *weapon_singular, char *weapon_plural,
                     char *weapon_different)
{
  static char buf[256];
  char *cp;

  cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
        for (; *weapon_plural; *(cp++) = *(weapon_plural++))
          ;
        break;
      case 'w':
        for (; *weapon_singular; *(cp++) = *(weapon_singular++))
          ;
        break;
      case 'd':
        for (; *weapon_different; *(cp++) = *(weapon_different++))
          ;
        break;
      default:
        *(cp++) = '#';
        break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }                             /* For */

  return (buf);
}

#define SENDOK(ch) ((ch)->desc && AWAKE(ch) && !(PLR_FLAGGED((ch), PLR_WRITING) || PLR_FLAGGED((ch), PLR_EDITING) || PLR_FLAGGED((ch), PLR_MAILING) || PLR_FLAGGED((ch), PLR_CUSTOMIZE)) && (STATE(ch->desc) != CON_SPELL_CREATE))

/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type)
{
  char *buf;
  int msgnum;
  struct char_data *witness;
  int was_in = 0, door1, door2, door3, room1 = 0, room2 = 0, room3 = 0;
  char been_heard[MAX_STRING_LENGTH];
  char temp[20];
  long rnum = 0;

  static struct dam_weapon_type
  {
    char *to_room;
    char *to_char;
    char *to_victim;
  }
  dam_weapons[] = {
                    {
                      "$n tries to #w $N, but misses.", /* 0: 0     */
                      "You try to #w $N, but miss.",
                      "$n tries to #w you, but misses."
                    },

                    {
                      "$n grazes $N as $e #W $M.",      /* 1: 1..2  */
                      "You graze $N as you #w $M.",
                      "$n grazes you as $e #W you."
                    },

                    {
                      "$n barely #W $N.",               /* 2: 3..4  */
                      "You barely #w $N.",
                      "$n barely #W you."
                    },

                    {
                      "$n #W $N.",                      /* 3: 5..6  */
                      "You #w $N.",
                      "$n #W you."
                    },

                    {
                      "$n #W $N hard.",                 /* 4: 7..10  */
                      "You #w $N hard.",
                      "$n #W you hard."
                    },

                    {
                      "$n #W $N very hard.",            /* 5: 11..14  */
                      "You #w $N very hard.",
                      "$n #W you very hard."
                    },

                    {
                      "$n #W $N extremely hard.",       /* 6: 15..19  */
                      "You #w $N extremely hard.",
                      "$n #W you extremely hard."
                    },

                    {
                      "$n massacres $N to small fragments with $s #d.", /* 7: 19..23 */
                      "You massacre $N to small fragments with your #d.",
                      "$n massacres you to small fragments with $s #d."
                    },

                    {
                      "$n demolishes $N with $s deadly #d!",    /* 8: > 23   */
                      "You demolish $N with your deadly #d!",
                      "$n demolishes you with $s deadly #d!"
                    },

                    {
                      "$n pulverizes $N with $s incredibly powerful #d!!",
                      "You pulverize $N with your incredibly powerful #d!!",
                      "$n pulverizes you with $s incredibly powerful #d!!",
                    },

                    {
                      "$n sublimates $N with an ultimate #d!!!",
                      "You sublimate $N with an ultimate #d!!!",
                      "$n sublimates you with an ultimate #d!!!",
                    }
                  };


  w_type -= TYPE_HIT;           /* Change to base of table with text */

  if (dam < 0)
    msgnum = 0;
  else if (dam == 0)
  {
    switch(number(0,1)) {
    case 0:
      msgnum = 1;
      break;
    case 1:
      msgnum = 2;
      break;
    default:
      msgnum = 0;
      break;
    }
  } else if (dam <= 1)
    msgnum = 3;
  else if (dam <= 3)
    msgnum = 5;
  else if (dam <= 6)
    msgnum = 7;
  else if (dam <= 10)
    msgnum = 8;
  else if (dam <= 13)
    msgnum = 9;
  else
    msgnum = 10;
  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
                       attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
                       attack_hit_text[w_type].different);
  for (witness = world[victim->in_room].people; witness; witness = witness->next_in_room)
    if (witness != ch && witness != victim && !PRF_FLAGGED(witness, PRF_FIGHTGAG) && SENDOK(witness))
      perform_act(buf, ch, NULL, victim, witness);
  if (ch->in_room != victim->in_room && !PLR_FLAGGED(ch, PLR_REMOTE))
    for (witness = world[ch->in_room].people; witness; witness = witness->next_in_room)
      if (witness != ch && witness != victim && !PRF_FLAGGED(witness, PRF_FIGHTGAG) && SENDOK(witness))
        perform_act(buf, ch, NULL, victim, witness);


  /* damage message to damager */
  strcpy(buf1, "^y");
  buf = replace_string(dam_weapons[msgnum].to_char,
                       attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
                       attack_hit_text[w_type].different);
  strcat(buf1, buf);
  if (SENDOK(ch))
    perform_act(buf1, ch, NULL, victim, ch);

  /* damage message to damagee */
  strcpy(buf1, "^r");
  buf = replace_string(dam_weapons[msgnum].to_victim,
                       attack_hit_text[w_type].singular, attack_hit_text[w_type].plural,
                       attack_hit_text[w_type].different);
  strcat(buf1, buf);
  act(buf1, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);

  /* Hear gunshots even if not in the room */
  if (w_type >= 21 && w_type <= 26)
  {
    for (int i = 7; i < 10; i++) {
      struct obj_data *obj = NULL;
      if (GET_EQ(ch, WEAR_WIELD) && GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), i) > 0 &&
          (rnum = real_object(GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), i))) > -1 &&
          (obj = &obj_proto[rnum]) && GET_OBJ_TYPE(obj) == ITEM_GUN_ACCESSORY)
        if (GET_OBJ_VAL(obj, 1) == 5)
          return;
    }

    was_in = ch->in_room;

    /* Make sure the sound doesn't 'echo' back */
    sprintf( been_heard, "%ld", ch->in_room );

    for (door1 = 0; door1 < NUM_OF_DIRS; door1++ ) {
      if ( EXIT( ch, door1 ) ) {
        if ( (room1 = EXIT( ch, door1 )->to_room) == was_in )
          continue;

        for ( door2 = 0; door2 < NUM_OF_DIRS; door2++ ) {
          if ( EXIT( ch, door2 ) ) {
            if ( (room2 = EXIT( ch, door2 )->to_room) == room1 )
              continue;

            for ( door3 = 0; door3 < NUM_OF_DIRS; door3++ ) {
              if ( EXIT( ch, door3 ) ) {
                if ( (room3 = EXIT( ch, door3 )->to_room) == room2 )
                  continue;

                ch->in_room = room3;

                sprintf( temp, "%ld", ch->in_room );

                if ( !strstr( been_heard, temp ) ) {
                  act("You hear gunshots nearby!", FALSE, ch, 0, 0, TO_ROOM);
                  for (struct char_data *tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
                    if (IS_NPC(tch))
                      if ((MOB_FLAGGED(tch, MOB_GUARD) || MOB_FLAGGED(tch, MOB_HELPER))
                          && number(0, 6) >= 2 && (IS_NPC(ch) && !IS_NPC(tch))) {
                        ch->in_room = was_in;
                        ranged_response(ch, tch);
                        ch->in_room = room3;
                      }
                  strcat( been_heard, temp );
                }
                ch->in_room = room2;
              }
            }

            ch->in_room = room2;

            sprintf( temp, "%ld", ch->in_room );

            if ( !strstr( been_heard, temp ) ) {
              act("You hear gunshots not far off.", FALSE, ch, 0, 0, TO_ROOM);
              for (struct char_data *tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
                if (IS_NPC(tch))
                  if ((MOB_FLAGGED(tch, MOB_GUARD) || MOB_FLAGGED(tch, MOB_HELPER))
                      && number(0, 6) >= 2 && (IS_NPC(ch) && !IS_NPC(tch))) {
                    ch->in_room = was_in;
                    ranged_response(ch, tch);
                    ch->in_room = room2;
                  }
              strcat( been_heard, temp );
            }
            ch->in_room = room1;
          }
        }

        ch->in_room = room1;

        sprintf( temp, "%ld", ch->in_room );

        if ( !strstr( been_heard, temp ) ) {
          act("You hear gunshots in the distance.", FALSE, ch, 0, 0, TO_ROOM);
          for (struct char_data *tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
            if (IS_NPC(tch))
              if ((MOB_FLAGGED(tch, MOB_GUARD) || MOB_FLAGGED(tch, MOB_HELPER))
                  && number(0, 6) >= 2 && (IS_NPC(ch) && !IS_NPC(tch))) {
                ch->in_room = was_in;
                ranged_response(ch, tch);
                ch->in_room = room1;
              }
          strcat( been_heard, temp );
        }
        ch->in_room = was_in;
      }
    }

    ch->in_room = was_in;
  }
}
#undef SENDOK

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
bool skill_message(int dam, struct char_data * ch, struct char_data * vict, int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = ch->equipment[WEAR_WIELD];

  for (i = 0; i < MAX_MESSAGES; i++)
  {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
        msg = msg->next;

      if (!IS_NPC(vict) && (IS_SENATOR(vict)) && dam < 1) {
        act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
        act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
        act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam > 0) {
        if (GET_POS(vict) == POS_DEAD) {
          send_to_char(CCYEL(ch, C_CMP), ch);
          act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);

          send_to_char(CCRED(vict, C_CMP), vict);
          act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(CCNRM(vict, C_CMP), vict);

          act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        } else {
          send_to_char(CCYEL(ch, C_CMP), ch);
          act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);

          send_to_char(CCRED(vict, C_CMP), vict);
          act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(CCNRM(vict, C_CMP), vict);

          act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        }
      } else if ((ch != vict) && (dam <= 0)) {  /* Dam == 0 */
        send_to_char(CCYEL(ch, C_CMP), ch);
        act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
        send_to_char(CCNRM(ch, C_CMP), ch);

        send_to_char(CCRED(vict, C_CMP), vict);
        act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
        send_to_char(CCNRM(vict, C_CMP), vict);

        act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}

int find_orig_dir(struct char_data *ch, struct char_data *victim)
{
  struct char_data *vict;
  int nextroom, dir, dist, room;

  for (dir = 0; dir < NUM_OF_DIRS; dir++)
  {
    room = victim->in_room;

    if (CAN_GO2(room, dir))
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;

    for (dist = 1; nextroom != NOWHERE && dist <= 4; dist++) {
      for (vict = world[nextroom].people; vict; vict = vict->next_in_room)
        if (vict == ch)
          return (rev_dir[dir]);

      room = nextroom;
      if (CAN_GO2(room, dir))
        nextroom = EXIT2(room, dir)->to_room;
      else
        nextroom = NOWHERE;
    }
  }
  return -1;
}

void weapon_scatter(struct char_data *ch, struct char_data *victim, struct obj_data *weapon)
{
  struct char_data *vict;
  char ammo_type[20];
  int damage_total, power, total = 0, door = 0, dir[3], i;

  if (!ch || !victim || !weapon)
    return;

  dir[1] = find_orig_dir(ch, victim);
  if (dir[1] >= NORTH && dir[1] <= NORTHWEST)
  {
    if (dir[1] == NORTH)
      dir[0] = NORTHWEST;
    else
      dir[0] = dir[1] - 1;
    if (dir[1] == NORTHWEST)
      dir[2] = NORTH;
    else
      dir[2] = dir[1] + 1;
  } else
    dir[0] = dir[2] = -1;

  for (vict = world[victim->in_room].people; vict; vict = vict->next_in_room)
    if (vict != victim && !IS_ASTRAL(vict) && GET_POS(vict) > POS_SLEEPING)
      total++;

  for (i = 0; i < 3; i++)
    if (dir[i] != -1 && EXIT(victim, dir[i]) &&
        IS_SET(EXIT(victim, dir[i])->exit_info, EX_CLOSED))
      door += 2;

  switch(GET_OBJ_VAL(weapon, 3))
  {
  case TYPE_SHOTGUN:
    sprintf(ammo_type, "horde of pellets");
    break;
  case TYPE_MACHINE_GUN:
    sprintf(ammo_type, "stream of bullets");
    break;
  case TYPE_CANNON:
    sprintf(ammo_type, "shell");
    break;
  case TYPE_ROCKET:
    sprintf(ammo_type, "rocket");
    break;
  default:
    sprintf(ammo_type, "bullet");
    break;
  }

  i = number(1, MAX(20, total + door + 2));
  if (i <= total)
  { // hits a victim
    for (vict = world[victim->in_room].people; vict; vict = vict->next_in_room)
      if (vict != victim && !IS_ASTRAL(vict) && GET_POS(vict) > POS_SLEEPING &&
          !number(0, total - 1))
        break;

    if (vict && (IS_NPC(vict) || (!IS_NPC(vict) && vict->desc))) {
      sprintf(buf, "A %s flies in from nowhere, hitting you!", ammo_type);
      act(buf, FALSE, vict, 0, 0, TO_CHAR);
      sprintf(buf, "A %s hums into the room and hits $n!", ammo_type);
      act(buf, FALSE, vict, 0, 0, TO_ROOM);
      power = MAX(GET_OBJ_VAL(weapon, 0) - GET_BALLISTIC(vict) - 3, 2);
      damage_total = MAX(1, GET_OBJ_VAL(weapon, 1));
      damage_total = convert_damage(stage((2 - success_test(GET_BOD(vict), power +
                                           modify_target(vict))), damage_total));
      damage(ch, vict, damage_total, TYPE_SCATTERING, PHYSICAL);
      return;
    }
  } else if (i > (MAX(20, total + door + 2) - door))
  { // hits a door
    for (i = 0; i < 3; i++)
      if (dir[i] != -1 && EXIT(victim, dir[i]) && !number(0, door - 1) &&
          IS_SET(EXIT(victim, dir[i])->exit_info, EX_CLOSED))
        break;

    if (i < 3) {
      sprintf(buf, "A %s hums into the room and hits the %s!", ammo_type,
              fname(EXIT(victim, dir[i])->keyword));
      act(buf, FALSE, victim, 0, 0, TO_ROOM);
      act(buf, FALSE, victim, 0, 0, TO_CHAR);
      damage_door(NULL, victim->in_room, dir[i], GET_OBJ_VAL(weapon, 0), DAMOBJ_PROJECTILE);
      return;
    }
  }

  // if it's reached this point, it's harmless
  sprintf(buf, "A %s hums harmlessly through the room.", ammo_type);
  act(buf, FALSE, victim, 0, 0, TO_ROOM);
  act(buf, FALSE, victim, 0, 0, TO_CHAR);
}

void damage_equip(struct char_data *ch, struct char_data *victim, int power,
                  int attacktype)
{
  int i = number(0, 39);

  if (i >= WEAR_PATCH || !GET_EQ(victim, i))
    return;

  if (attacktype >= TYPE_PISTOL && attacktype <= TYPE_BIFURCATE)
  {
    if (i >= WEAR_EYES)
      return;
    damage_obj(ch, GET_EQ(victim, i), power, DAMOBJ_PROJECTILE);
  }
  switch (attacktype)
  {
  case TYPE_PIERCE:
  case TYPE_STAB:
  case TYPE_SHURIKEN:
    if (i >= WEAR_EYES)
      return;
    damage_obj(ch, GET_EQ(victim, i), power, DAMOBJ_PIERCE);
    break;
  case TYPE_STING:
  case TYPE_SLASH:
  case TYPE_CLAW:
  case TYPE_THRASH:
  case TYPE_ARROW:
  case TYPE_THROWING_KNIFE:
    if (i >= WEAR_EYES)
      return;
    damage_obj(ch, GET_EQ(victim, i), power, DAMOBJ_SLASH);
    break;
  case TYPE_HIT:
  case TYPE_BLUDGEON:
  case TYPE_POUND:
  case TYPE_MAUL:
  case TYPE_PUNCH:
    damage_obj(ch, GET_EQ(victim, i), power, DAMOBJ_CRUSH);
    break;
  }
}

float power_multiplier(int type, int material)
{
  switch (type) {
  case DAMOBJ_ACID:
    switch (material) {
    case 6:
    case 7:
    case 10:
    case 11:
      return 1.2;
    case 13:
      return 2.0;
    }
    break;
  case DAMOBJ_AIR:
    switch (material) {
    case 2:
      return 2.0;
    case 5:
    case 8:
    case 14:
    case 15:
    case 16:
      return 0.4;
    default:
      return 0.8;
    }
    break;
  case DAMOBJ_EARTH:
    switch (material) {
    case 2:
      return 2.0;
    default:
      return 0.9;
    }
    break;
  case DAMOBJ_FIRE:
    switch (material) {
    case 0:
    case 1:
    case 3:
      return 1.5;
    case 4:
    case 6:
    case 7:
      return 1.1;
    default:
      return 0.8;
    }
    break;
  case DAMOBJ_ICE:
    switch (material) {
    case 3:
    case 4:
      return 1.2;
    default:
      return 0.9;
    }
    break;
  case DAMOBJ_LIGHTNING:
    switch (material) {
    case 8:
      return 2.0;
    case 10:
    case 11:
      return 5.0;
    default:
      return 0.7;
    }
    break;
  case DAMOBJ_WATER:
    switch (material) {
    case 10:
    case 11:
      return 3.0;
    default:
      return 0.5;
    }
    break;
  case DAMOBJ_EXPLODE:
    switch (material) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 13:
      return 1.3;
    case 2:
      return 3.0;
    default:
      return 1.1;
    }
    break;
  case DAMOBJ_PROJECTILE:
    switch (material) {
    case 2:
      return 3.0;
    case 10:
    case 11:
      return 1.1;
    default:
      return 0.8;
    }
    break;
  case DAMOBJ_CRUSH:
    switch (material) {
    case 1:
    case 6:
    case 7:
      return 1.4;
    case 2:
      return 4.0;
    case 5:
    case 14:
    case 15:
    case 16:
      return 1.0;
    default:
      return 0.7;
    }
    break;
  case DAMOBJ_SLASH:
    switch (material) {
    case 0:
    case 3:
    case 4:
    case 13:
      return 1.4;
    case 5:
    case 14:
    case 15:
    case 16:
      return 0.3;
    default:
      return 0.6;
    }
    break;
  case DAMOBJ_PIERCE:
    switch (material) {
    case 0:
    case 3:
    case 4:
    case 13:
      return 1.0;
    case 5:
    case 8:
    case 14:
    case 15:
    case 16:
      return 0.1;
    default:
      return 0.3;
    }
    break;
  }
  return 1.0;
}

void damage_door(struct char_data *ch, int room, int dir, int power, int type)
{
  if (room < 0 || room > top_of_world || dir < NORTH || dir > DOWN ||
      !world[room].dir_option[dir] || !world[room].dir_option[dir]->keyword ||
      !IS_SET(world[room].dir_option[dir]->exit_info, EX_CLOSED))
    return;

  int rating, half, opposite, rev, ok = 0;

  opposite = world[room].dir_option[dir]->to_room;
  rev = rev_dir[dir];
  if (opposite > -1 && world[opposite].dir_option[rev] &&
      world[opposite].dir_option[rev]->to_room == room)
    ok = TRUE;

  if (IS_SET(type, DAMOBJ_MANIPULATION))
  {
    rating = world[room].dir_option[dir]->barrier;
    REMOVE_BIT(type, DAMOBJ_MANIPULATION);
  } else
    rating = world[room].dir_option[dir]->barrier * 2;

  half = MAX(1, rating >> 1);

  if (ch && IS_SET(type, DAMOBJ_CRUSH) && GET_TRADITION(ch) == TRAD_ADEPT && GET_POWER(ch, ADEPT_SMASHING_BLOW))
    power += MAX(0, success_test(GET_POWER(ch, ADEPT_SMASHING_BLOW), 4));

  if (power < half)
  {
    sprintf(buf, "The %s remains undamaged.\r\n", fname(world[room].dir_option[dir]->keyword));
    send_to_room(buf, room);
    if (ch && ch->in_room != room)
      send_to_char(buf, ch);
    return;
  } else if (power < rating)
  {
    sprintf(buf, "The %s has been slightly damaged.\r\n",
            fname(world[room].dir_option[dir]->keyword));
    send_to_room(buf, room);
    if (ch && ch->in_room != room)
      send_to_char(buf, ch);
    world[room].dir_option[dir]->condition--;
  } else
  {
    sprintf(buf, "The %s has been damaged!\r\n", fname(world[room].dir_option[dir]->keyword));
    send_to_room(buf, room);
    if (ch && ch->in_room != room)
      send_to_char(buf, ch);
    world[room].dir_option[dir]->condition -= 1 + (power - rating) / half;
  }

  if (ok)
    world[opposite].dir_option[rev]->condition = world[room].dir_option[dir]->condition;

  if (world[room].dir_option[dir]->condition <= 0)
  {
    sprintf(buf, "The %s has been destroyed!\r\n", fname(world[room].dir_option[dir]->keyword));
    send_to_room(buf, room);
    if (ch && ch->in_room != room)
      send_to_char(buf, ch);
    REMOVE_BIT(world[room].dir_option[dir]->exit_info, EX_CLOSED);
    REMOVE_BIT(world[room].dir_option[dir]->exit_info, EX_LOCKED);
    SET_BIT(world[room].dir_option[dir]->exit_info, EX_DESTROYED);
    if (ok) {
      sprintf(buf, "The %s is destroyed from the other side!\r\n",
              fname(world[room].dir_option[dir]->keyword));
      send_to_room(buf, opposite);
      REMOVE_BIT(world[opposite].dir_option[rev]->exit_info, EX_CLOSED);
      REMOVE_BIT(world[opposite].dir_option[rev]->exit_info, EX_LOCKED);
      SET_BIT(world[opposite].dir_option[rev]->exit_info, EX_DESTROYED);
    }
  }
}

// damage_obj does what its name says, it figures out effects of successes
// damage, applies that damage to that object, and sends messages to the player
void damage_obj(struct char_data *ch, struct obj_data *obj, int power, int type)
{
  if (!obj)
    return;
  if (power <= 0)
    return;

  int success, modifier, dam, target, rating, half;
  struct char_data *vict = (obj->worn_by ? obj->worn_by : obj->carried_by);
  struct obj_data *temp, *next;

  // PC corpses are indestructable by normal means
  if ( IS_OBJ_STAT(obj, ITEM_CORPSE) && GET_OBJ_VAL(obj, 4) == 1 )
  {
    if ( ch != NULL )
      send_to_char("Nuh uh fuck nut.\n\r",ch);
    return;
  }
  if (IS_SET(type, DAMOBJ_MANIPULATION))
  {
    rating = GET_OBJ_BARRIER(obj);
    modifier = 2;
    REMOVE_BIT(type, DAMOBJ_MANIPULATION);
  } else
  {
    rating = GET_OBJ_BARRIER(obj) << 1;
    modifier = 4;
  }

  half = MAX(1, rating >> 1);

  modifier -= power >> 2;

  success = success_test(2, MAX(2, material_ratings[(int)GET_OBJ_MATERIAL(obj)] + modifier));

  if (success > 0)
  {
    switch (type) {
    case DAMOBJ_ACID:
        if ((GET_OBJ_TYPE(obj) == ITEM_GYRO || GET_OBJ_TYPE(obj) == ITEM_WORN) && success == 2)
          switch (number(0, 4)) {
          case 0:
            GET_OBJ_VAL(obj, 0) = MAX(GET_OBJ_VAL(obj, 0) - 1, 0);
            GET_OBJ_VAL(obj, 1) = MAX(GET_OBJ_VAL(obj, 1) - 1, 0);
            break;
          case 1:
          case 2:
            GET_OBJ_VAL(obj, 0) = MAX(GET_OBJ_VAL(obj, 0) - 1, 0);
            break;
          case 3:
          case 4:
            GET_OBJ_VAL(obj, 1) = MAX(GET_OBJ_VAL(obj, 1) - 1, 0);
            break;
          }
      break;
    case DAMOBJ_FIRE:
    case DAMOBJ_EXPLODE:
      if (GET_OBJ_TYPE(obj) == ITEM_GUN_CLIP && success == 2 && vict) {
        act("$p ignites, spraying bullets about!", FALSE, vict, obj, 0, TO_CHAR);
        act("One of $n's clips ignites, spraying bullets about!",
            FALSE, vict, obj, 0, TO_ROOM);
        target = (int)(GET_OBJ_VAL(obj, 0) / 4);
        switch (GET_OBJ_VAL(obj, 1) + 300) {
        case TYPE_ROCKET:
          dam = DEADLY;
          break;
        case TYPE_GRENADE_LAUNCHER:
        case TYPE_CANNON:
          dam = SERIOUS;
          break;
        case TYPE_SHOTGUN:
        case TYPE_MACHINE_GUN:
        case TYPE_RIFLE:
          dam = MODERATE;
          break;
        default:
          dam = LIGHT;
        }
        dam = convert_damage(stage(-success_test(GET_BOD(vict) + GET_DEFENSE(vict),
                                   target + modify_target(vict)), dam));
        damage(vict, vict, dam, TYPE_SCATTERING, TRUE);
        extract_obj(obj);
        return;
      }
      if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && success == 2 &&
          GET_OBJ_VAL(obj, 3) == TYPE_HAND_GRENADE) {
        if (vict) {
          act("$p is set off by the fire!", FALSE, vict, obj, 0, TO_CHAR);
          act("A $p carried by $n is set off by the fire!", FALSE, vict, obj, 0, TO_ROOM);
          explode(NULL, obj, obj->in_room);
        } else if (obj->in_room != NOWHERE) {
          sprintf(buf, "%s is set off by the flames!",
                  CAP(obj->text.name));
          send_to_room(buf, obj->in_room);
          explode(NULL, obj, obj->in_room);
        }
        return;
      }
      break;
    }
    power = (int)(power * power_multiplier(type, GET_OBJ_MATERIAL(obj)));
  }

  if (power < half)
  {
    if (ch)
      send_to_char(ch, "%s remains undamaged.\r\n",
                   CAP(obj->text.name));
    return;
  } else if (power < rating)
  {
    if (ch)
      send_to_char(ch, "%s has been slightly damaged.\r\n",
                   CAP(obj->text.name));
    if (vict && vict != ch)
      send_to_char(vict, "%s has been slightly damaged.\r\n",
                   CAP(obj->text.name));
    GET_OBJ_CONDITION(obj)--;
  } else
  {
    if (ch)
      send_to_char(ch, "%s has been damaged!\r\n",
                   CAP(obj->text.name));
    if (vict && vict != ch)
      send_to_char(vict, "%s has been damaged!\r\n",
                   CAP(obj->text.name));
    GET_OBJ_CONDITION(obj) -= 1 + (power - rating) / half;
  }

  // if the end result is that the object condition rating is 0 or less
  // it is destroyed -- a good reason to keep objects in good repair
  if (GET_OBJ_CONDITION(obj) <= 0)
  {
    if (ch)
      send_to_char(ch, "%s has been destroyed!\r\n",
                   CAP(obj->text.name));
    if (vict && vict != ch)
      send_to_char(vict, "%s has been destroyed!\r\n",
                   CAP(obj->text.name));
    if (ch && !IS_NPC(ch) && GET_QUEST(ch))
      check_quest_destroy(ch, obj);
    else if (ch && AFF_FLAGGED(ch, AFF_GROUP) && ch->master &&
             !IS_NPC(ch->master) && GET_QUEST(ch->master))
      check_quest_destroy(ch->master, obj);
    for (temp = obj->contains; temp; temp = next) {
      next = temp->next_content;
      obj_from_obj(temp);
      if (IS_OBJ_STAT(obj, ITEM_CORPSE) && !GET_OBJ_VAL(obj, 4) &&
          GET_OBJ_TYPE(temp) != ITEM_MONEY)
        extract_obj(temp);
      else if (vict)
        obj_to_char(temp, vict);
      else if (obj->in_room != NOWHERE)
        obj_to_room(temp, obj->in_room);
      else
        extract_obj(temp);
    }
    extract_obj(obj);
  }
}

void docwagon_message(struct char_data *ch)
{
  char buf[MAX_STRING_LENGTH];

  switch (SECT(ch->in_room))
  {
  case SECT_INSIDE:
    sprintf(buf,"A DocWagon employee suddenly appears, transports %s's body to\r\nsafety, and rushes away.", GET_NAME(ch));
    act(buf,FALSE, ch, 0, 0, TO_ROOM);
    sprintf(buf,"A DocWagon employee suddenly appears, transports %s's body to\r\nsafety, and rushes away.", GET_CHAR_NAME(ch));
    break;
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
    sprintf(buf,"A DocWagon armored speedboat arrives, loading %s's body on\r\nboard before leaving.", GET_NAME(ch));
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    sprintf(buf,"A DocWagon armored speedboat arrives, loading %s's body on\r\nboard before leaving.", GET_CHAR_NAME(ch));
    break;
  case SECT_UNDERWATER:
    sprintf(buf,"A DocWagon submarine silently arrives and loads %s's body into the\r\ndecompression chamber.", GET_NAME(ch));
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    sprintf(buf,"A DocWagon submarine silently arrives and loads %s's body into the\r\ndecompression chamber.", GET_CHAR_NAME(ch));
    break;
  default:
    sprintf(buf,"A DocWagon helicopter flies in, taking %s's body to safety.", GET_NAME(ch));
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    sprintf(buf,"A DocWagon helicopter flies in, taking %s's body to safety.", GET_CHAR_NAME(ch));
    break;
  }

  mudlog(buf, ch, LOG_DEATHLOG, TRUE);
}

void docwagon(struct char_data *ch)
{
  int i, creds;
  struct obj_data *docwagon = NULL, *obj, *temp;

  if (IS_NPC(ch))
    return;

  for (obj = ch->carrying; obj && !docwagon; obj = obj->next_content)
  {
    if (GET_OBJ_TYPE(obj) == ITEM_DOCWAGON && GET_OBJ_VAL(obj, 1) == GET_IDNUM(ch))
      docwagon = obj;
    else
      for (temp = obj->contains; temp && !docwagon; temp = temp->next_content)
        if (GET_OBJ_TYPE(temp) == ITEM_DOCWAGON && GET_OBJ_VAL(temp, 1) == GET_IDNUM(ch))
          docwagon = temp;
  }

  for (i = 0; (i < NUM_WEARS && !docwagon); i++)
    if ((obj = GET_EQ(ch, i)))
    {
      if (GET_OBJ_TYPE(obj) == ITEM_DOCWAGON && GET_OBJ_VAL(obj, 1) == GET_IDNUM(ch))
        docwagon = obj;
      else
        for (temp = obj->contains; temp && !docwagon; temp = temp->next_content)
          if (GET_OBJ_TYPE(temp) == ITEM_DOCWAGON && GET_OBJ_VAL(temp, 1) == GET_IDNUM(ch))
            docwagon = temp;
    }

  if (!docwagon || PLR_FLAGGED(ch, PLR_AUTH))
    return;

  if (success_test(GET_OBJ_VAL(docwagon, 0),
                   MAX(zone_table[world[ch->in_room].zone].security, 4)))
  {
    if (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) == ch)
      stop_fighting(FIGHTING(ch));
    if (FIGHTING(ch))
      stop_fighting(ch);
    docwagon_message(ch);
    death_penalty(ch);  /* Penalty for deadly wounds */
    //    GET_PHYSICAL(ch) = GET_MAX_PHYSICAL(ch);
    GET_PHYSICAL(ch) = 400; /* They can't make you heal THAT fast... */
    GET_MENTAL(ch) = 0;
    AFF_FLAGS(ch).SetBit(AFF_SLEEP);
    update_pos(ch);
    send_to_char("\r\n\r\nYour last conscious memory is the arrival of a DocWagon.\r\n", ch);
    if (zone_table[world[ch->in_room].zone].juridiction > 0)
      i = real_room(14709);
    else
      i = real_room(16295);
    char_from_room(ch);
    char_to_room(ch, i);
    creds = MAX((number(8, 12) * 500 / GET_OBJ_VAL(docwagon, 0)), (int)(GET_NUYEN(ch) / 10));
    if ((GET_NUYEN(ch) + GET_BANK(ch)) < creds) {
      send_to_char("Not finding sufficient payment, your DocWagon contract was retracted.\r\n", ch);
      extract_obj(docwagon);
    } else if (GET_BANK(ch) < creds) {
      GET_NUYEN(ch) -= (creds - GET_BANK(ch));
      GET_BANK(ch) = 0;
    } else
      GET_BANK(ch) -= creds;
  }
  return;
}


void check_adrenaline(struct char_data *ch, int mode)
{
  int i, dam;
  struct obj_data *bio, *pump = NULL;

  for (bio = ch->bioware; bio && !pump; bio = bio->next_content)
    if (GET_OBJ_VAL(bio, 2) == 4)
      pump = bio;

  if (!pump)
    return;

  if (GET_OBJ_VAL(pump, 5) == 0 && mode == 1)
  {
    GET_OBJ_VAL(pump, 5) = dice(GET_OBJ_VAL(pump, 0), 6);
    GET_OBJ_VAL(pump, 6) = GET_OBJ_VAL(pump, 5);
    send_to_char("Your body is wracked with renewed vitality as adrenaline "
                 "pumps into your\r\nbloodstream.\r\n", ch);
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      affect_modify(ch,
                    pump->affected[i].location,
                    pump->affected[i].modifier,
                    pump->obj_flags.bitvector, TRUE);
  } else if (GET_OBJ_VAL(pump, 5) > 0 && !mode)
  {
    GET_OBJ_VAL(pump, 5)--;
    if (GET_OBJ_VAL(pump, 5) == 0) {
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
        affect_modify(ch,
                      pump->affected[i].location,
                      pump->affected[i].modifier,
                      pump->obj_flags.bitvector, FALSE);
      GET_OBJ_VAL(pump, 5) = -number(168, 180);
      send_to_char("Your body softens and relaxes as the adrenaline wears off.\r\n", ch);
      dam = convert_damage(stage(-success_test(GET_BOD(ch),
                                 (int)(GET_OBJ_VAL(pump, 6) / 2) + modify_target(ch)), DEADLY));
      GET_OBJ_VAL(pump, 6) = 0;
      damage(ch, ch, dam, TYPE_BIOWARE, FALSE);
    }
  } else if (GET_OBJ_VAL(pump, 5) < 0 && !mode)
    GET_OBJ_VAL(pump, 5)++;
}

void gen_death_msg(struct char_data *ch, struct char_data *vict, int attacktype)
{
  switch (attacktype)
  {
  case TYPE_SUFFERING:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "%s died (no blood, it seems). {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s's brain ran out of oxygen {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "%s simply ran out of blood. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "%s unwittingly committed suicide. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "Bloodlack stole %s's life. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_EXPLOSION:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "%s blew %s to pieces. {%s (%ld)}", ch == vict ? "???"
              : GET_NAME(ch), GET_CHAR_NAME(vict),
              world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s was blown to bits. [%s] {%s (%ld)}",
              GET_CHAR_NAME(vict), ch == vict ? "???" : GET_NAME(ch),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "%s got vaporized by %s. {%s (%ld)}",
              GET_CHAR_NAME(vict), ch == vict ? "???" : GET_NAME(ch),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "%s was incinerated by an explosion. [%s] {%s (%ld)}",
              GET_CHAR_NAME(vict), ch == vict ? "???" : GET_NAME(ch),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "%s got blown to hell by %s. {%s (%ld)}",
              GET_CHAR_NAME(vict), ch == vict ? "???" : GET_NAME(ch),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_SCATTERING:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "%s accidentally (?) killed by %s. {%s (%ld)}",
              GET_CHAR_NAME(vict), GET_NAME(ch),
              world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "Shouldn't have been standing there, should you %s? "
              "[%s] {%s (%ld)}", GET_CHAR_NAME(vict), GET_NAME(ch),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "Oops....%s just blew %s's head off. {%s (%ld)}",
              GET_NAME(ch), GET_CHAR_NAME(vict),
              world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "%s's stray bullet caught %s in the heart.  "
              "What a shame. {%s (%ld)}",
              GET_NAME(ch), GET_CHAR_NAME(vict),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "A random bullet killed a random person -- poor %s. "
              "[%s] {%s (%ld)}", GET_CHAR_NAME(vict), GET_NAME(ch),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_FALL:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "%s died on impact. {%s (%ld)}", GET_CHAR_NAME(vict),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s failed to miss the ground. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "Life's a bitch, %s.  So's concrete. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "What %s wouldn't have given for a safety net... "
              "{%s (%ld)}", GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "The ground can be such an unforgiving thing.  "
              "Right, %s? {%s (%ld)}", GET_CHAR_NAME(vict),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_DROWN:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "%s drank way, way, WAY too much. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s sleeps with the fishes.  Involuntarily. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "The water had a fight with %s's lungs.  "
              "The water won. {%s (%ld)}", GET_CHAR_NAME(vict),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "Water doesn't seem so harmless now, does it %s? "
              "{%s (%ld)}", GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "%s didn't float. {%s (%ld)}", GET_CHAR_NAME(vict),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_BIOWARE:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "%s just hasn't been taking %s medication.  Oops. "
              "{%s (%ld)}", GET_CHAR_NAME(vict), GET_SEX(vict) == SEX_MALE ?
              "his" : (GET_SEX(vict) == SEX_FEMALE ? "her" : "its"),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s was killed in a bioware rebellion. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "%s has a fatal heart attack.  Wuss. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "Still think the bioware was worth it, %s?. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "Maybe %s got a defective piece of bioware... {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_RECOIL:
    switch (number(0, 4)) {
    case 0:
      sprintf(buf2, "You're meant to hit *other* people with that whip, "
              "%s. {%s (%ld)}", GET_CHAR_NAME(vict),
              world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s lopped off %s own head.  Oops. {%s (%ld)}",
              GET_CHAR_NAME(vict), GET_SEX(vict) == SEX_MALE ? "his" :
              (GET_SEX(vict) == SEX_FEMALE ? "her" : "its"),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    case 2:
      sprintf(buf2, "%s, watch out for your whi....nevermind. {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 3:
      sprintf(buf2, "THWAP!  Wait.....was that *your* whip, %s?!? "
              "{%s (%ld)}", GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 4:
      sprintf(buf2, "%s's whip didn't agree with %s. {%s (%ld)}",
              GET_CHAR_NAME(vict), GET_SEX(vict) == SEX_MALE ? "his" :
              (GET_SEX(vict) == SEX_FEMALE ? "her" : "its"),
              world[vict->in_room].name, world[vict->in_room].number);
      break;
    }
    break;
  case TYPE_RAM:
    sprintf(buf2, "%s was made a hood ornament. {%s (%ld)}",
            GET_CHAR_NAME(vict), world[vict->in_room].name,
            world[vict->in_room].number);
    break;
  case TYPE_DUMPSHOCK:
    sprintf(buf2, "%s couldn't quite manage a graceful logoff. { %s (%ld)}",
            GET_CHAR_NAME(vict), world[vict->in_room].name,
            world[vict->in_room].number);

    break;
  case TYPE_BLACKIC:
    sprintf(buf2, "%s couldn't haxor the gibson. { %s (%ld)}",
            GET_CHAR_NAME(vict), world[vict->in_room].name,
            world[vict->in_room].number);
    break;
  case TYPE_CRASH:
    switch(number(0, 1)) {
    case 0:
      sprintf(buf2, "%s forgot to wear his seatbelt. { %s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    case 1:
      sprintf(buf2, "%s become one with the dashboard. { %s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
      break;
    }
    break;
  default:
    if (ch == vict)
      sprintf(buf2, "%s died (cause uncertain). {%s (%ld)}",
              GET_CHAR_NAME(vict), world[vict->in_room].name,
              world[vict->in_room].number);
    else
      switch (number(0, 5)) {
      case 0:
        sprintf(buf2, "%s killed by %s. {%s (%ld)}", GET_CHAR_NAME(vict),
                GET_NAME(ch), world[vict->in_room].name,
                world[vict->in_room].number);
        break;
      case 1:
        sprintf(buf2, "%s wiped %s out of existence. {%s (%ld)}",
                GET_NAME(ch), GET_CHAR_NAME(vict), world[vict->in_room].name,
                world[vict->in_room].number);
        break;
      case 2:
        sprintf(buf2, "%s's life terminated by %s. {%s (%ld)}",
                GET_CHAR_NAME(vict), GET_NAME(ch), world[vict->in_room].name,
                world[vict->in_room].number);
        break;
      case 3:
        sprintf(buf2, "%s flatlined %s. {%s (%ld)}",
                GET_NAME(ch), GET_CHAR_NAME(vict), world[vict->in_room].name,
                world[vict->in_room].number);
        break;
      case 4:
        sprintf(buf2, "%s transformed %s into a corpse. {%s (%ld)}",
                GET_NAME(ch), GET_CHAR_NAME(vict), world[vict->in_room].name,
                world[vict->in_room].number);
        break;
      case 5:
        sprintf(buf2, "%s got geeked by %s. {%s (%ld)}",
                GET_CHAR_NAME(vict), GET_NAME(ch), world[vict->in_room].name,
                world[vict->in_room].number);
        break;
      }
    break;
  }

  mudlog(buf2, vict, LOG_DEATHLOG, TRUE);
}

#define IS_RANGED(eq)   (GET_OBJ_TYPE(eq) == ITEM_FIREWEAPON || \
                        (GET_OBJ_TYPE(eq) == ITEM_WEAPON && \
                        (GET_OBJ_VAL(eq, 3) == TYPE_SHURIKEN || \
                         GET_OBJ_VAL(eq, 3) == TYPE_THROWING_KNIFE || \
                         GET_OBJ_VAL(eq, 3) >= TYPE_PISTOL)))

#define RANGE_OK(ch) ((GET_WIELDED(ch, 0) && GET_EQ(ch, WEAR_WIELD) && \
                      IS_RANGED(GET_EQ(ch, WEAR_WIELD))) || (GET_WIELDED(ch, 1) && \
                      GET_EQ(ch, WEAR_HOLD) && IS_RANGED(GET_EQ(ch, WEAR_HOLD))))

#define IS_BURST(eq)    (GET_OBJ_TYPE(eq) == ITEM_WEAPON && \
                        (GET_OBJ_VAL(eq, 3) == TYPE_MACHINE_GUN || \
                        (GET_OBJ_VAL(eq, 3) == TYPE_RIFLE && \
                         GET_OBJ_VAL(eq, 4) == SKILL_ASSAULT_RIFLES)))

// return 1 if victim died, 0 otherwise
bool damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype,
            bool is_physical)
{
  char rbuf[MAX_STRING_LENGTH];
  int exp;
  bool total_miss = FALSE;
  struct obj_data *bio;
  ACMD(do_disconnect);
  ACMD(do_return);

  if (GET_POS(victim) <= POS_DEAD)
  {
    log("SYSERR: Attempt to damage a corpse.");
    return 0;                   /* -je, 7/7/92 */
  }

  sprintf(rbuf,"Damage: ");
  if (( (!IS_NPC(ch)
         && IS_SENATOR(ch)
         && !access_level(ch, LVL_ADMIN))
        || (IS_NPC(ch)
            && ch->master
            && AFF_FLAGGED(ch, AFF_CHARM)
            && !IS_NPC(ch->master)
            && IS_SENATOR(ch->master)
            && !access_level(ch->master, LVL_ADMIN)))
      && IS_NPC(victim)
      && dam > 0
      && !from_ip_zone(GET_MOB_VNUM(victim)))
  {
    dam = -1;
    buf_mod(rbuf,"Invalid",dam);
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
  {
    dam = -1;
    buf_mod(rbuf,"Keeper",dam);
  }
  if(IS_NPC(victim) && MOB_FLAGGED(victim,MOB_NOKILL))
  {
    dam =-1;
    buf_mod(rbuf,"Nokill",dam);
    if (ch->in_room == victim->in_room) {
      do_action(victim, GET_NAME(ch), cmd_slp, 0);
      GET_MENTAL(ch) = 1;
    }

  }

  if (victim != ch)
  {
    if (GET_POS(ch) > POS_STUNNED && attacktype < TYPE_SUFFERING) {
      if (!FIGHTING(ch) && !ch->in_veh)
        set_fighting(ch, victim);

      if (!IS_NPC(ch) && IS_NPC(victim) && victim->master && !number(0, 10) &&
          IS_AFFECTED(victim, AFF_CHARM) && (victim->master->in_room == ch->in_room) &&
          !(ch->master && ch->master == victim->master)) {
        if (FIGHTING(ch))
          stop_fighting(ch);
        set_fighting(ch, victim->master);
        if (!FIGHTING(victim->master))
          set_fighting(victim->master, ch);
        return 0;
      }
    }
    if (GET_POS(victim) > POS_STUNNED && !FIGHTING(victim)) {
      if (victim->in_room == ch->in_room)
        set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) &&
          (!IS_SENATOR(ch)))
        remember(victim, ch);
    }
  }
  if (victim->master && victim->master == ch)
    stop_follower(victim);

  if (IS_AFFECTED(ch, AFF_HIDE))
    appear(ch);

  /* stop sneaking if it's the case */
  if (IS_AFFECTED(ch, AFF_SNEAK))
    AFF_FLAGS(ch).RemoveBit(AFF_SNEAK);

  if (PLR_FLAGGED(victim, PLR_PROJECT) && ch != victim)
  {
    do_return(victim, "", 0, 0);
    WAIT_STATE(victim, PULSE_VIOLENCE);
  }
  /* Firgure out how to do WANTED flag*/

  if (ch != victim)
  {
    check_killer(ch, victim);
  }
  if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(victim))
  {
    dam = -1;
    buf_mod(rbuf,"No-PK",dam);
  }
  if (attacktype == TYPE_EXPLOSION && (IS_ASTRAL(victim) || MOB_FLAGGED(victim, MOB_IMMEXPLODE)))
  {
    dam = -1;
    buf_mod(rbuf,"ImmExplode",dam);
  }

  if (dam == -1)
  {
    total_miss = TRUE;
  }

  if (ch != victim && !IS_NPC(victim) && !(victim->desc))
  {
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      GET_PHYSICAL(victim) = MAX(100, GET_PHYSICAL(victim) -
                                 (int)(GET_MAX_PHYSICAL(victim) / 2));
      GET_MENTAL(victim) = MAX(100, GET_MENTAL(victim) -
                               (int)(GET_MAX_MENTAL(victim) / 2));
      char_from_room(victim);
      char_to_room(victim, 0);
    }
    return 0;
  }

  if (is_physical && dam >= 3)
    for (bio = ch->bioware; bio; bio = bio->next_content)
      if (GET_OBJ_VAL(bio, 2) == 0)
      {
        dam--;
        break;
      }

  if (is_physical)
    GET_PHYSICAL(victim) -= MAX(dam * 100, 0);
  else if (((int)(GET_MENTAL(victim) / 100) - dam) < 0)
  {
    int physdam = dam - (int)(GET_MENTAL(victim) / 100);
    GET_MENTAL(victim) = 0;
    GET_PHYSICAL(victim) -= MAX(physdam * 100, 0);
  } else
    GET_MENTAL(victim) -= MAX(dam * 100, 0);

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   *
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (attacktype > 0 && ch != victim && attacktype < TYPE_SUFFERING)
  {
    if (!IS_WEAPON(attacktype)) {
      skill_message(dam, ch, victim, attacktype);
    } else {
      if (GET_POS(victim) == POS_DEAD) {
        if (!skill_message(dam, ch, victim, attacktype))
          dam_message(dam, ch, victim, attacktype);
      } else
        dam_message(dam, ch, victim, attacktype);
    }
  }

  if ((ch->in_room != victim->in_room) && total_miss && attacktype < TYPE_SUFFERING &&
      attacktype >= TYPE_PISTOL)
    weapon_scatter(ch, victim, GET_EQ(ch, WEAR_WIELD));

  if (victim->bioware && GET_POS(victim) > POS_STUNNED && dam > 0 && ch != victim)
    check_adrenaline(victim, 1);

  if (ch != victim && dam > 0 && attacktype >= TYPE_HIT)
    damage_equip(ch, victim, dam, attacktype);

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim))
  {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.",
        TRUE, victim, 0, 0, TO_ROOM);
    if (!PLR_FLAGGED(victim, PLR_MATRIX)) {
      send_to_char("You are mortally wounded, and will die soon, if not "
                   "aided.\r\n", victim);
    }
    if (!IS_NPC(victim))
      docwagon(victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.",
        TRUE, victim, 0, 0, TO_ROOM);
    if (!PLR_FLAGGED(victim, PLR_MATRIX)) {
      send_to_char("You're stunned, but will probably regain consciousness "
                   "again.\r\n", victim);
    }
    break;
  case POS_DEAD:
    if (IS_NPC(victim))
      act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    else
      act("$n slumps in a pile. You hear sirens as docwagon rush in and grab $m.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("You feel the world slip in to darkness, you better hope a wandering Docwagon finds you.\r\n", victim);
    break;
  default:                      /* >= POSITION SLEEPING */
    if (dam > ((int)(GET_MAX_PHYSICAL(victim) / 100) >> 2))
      act("^RThat really did HURT!^n", FALSE, victim, 0, 0, TO_CHAR);

    if (GET_PHYSICAL(victim) < (GET_MAX_PHYSICAL(victim) >> 2))
      send_to_char(victim, "%sYou wish that your wounds would stop "
                   "^RBLEEDING^n so much!%s\r\n", CCRED(victim, C_SPR),
                   CCNRM(victim, C_SPR));

    if (MOB_FLAGGED(victim, MOB_WIMPY) && ch != victim &&
        GET_PHYSICAL(victim) < (GET_MAX_PHYSICAL(victim) >> 2))
      do_flee(victim, "", 0, 0);

    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && victim != ch &&
        (int)(GET_PHYSICAL(victim) / 100) < GET_WIMP_LEV(victim)) {
      send_to_char("You ^Ywimp^n out, and attempt to flee!\r\n", victim);
      do_flee(victim, "", 0, 0);
    }
    break;
  }

  if (GET_MENTAL(victim) < 100 || GET_PHYSICAL(victim) < 0)
    if (FIGHTING(ch) == victim)
      stop_fighting(ch);

  if (!AWAKE(victim))
    if (FIGHTING(victim))
      stop_fighting(victim);

  if (GET_POS(victim) == POS_DEAD)
  {
    if (ch != victim && !IS_NPC(ch) && GET_QUEST(ch) && IS_NPC(victim))
      check_quest_kill(ch, victim);
    else if (ch != victim && AFF_FLAGGED(ch, AFF_GROUP) && ch->master &&
             !IS_NPC(ch->master) && GET_QUEST(ch->master) && IS_NPC(victim))
      check_quest_kill(ch->master, victim);

    if ((IS_NPC(victim) || victim->desc) && ch != victim &&
        attacktype != TYPE_EXPLOSION) {
      if ( IS_NPC( victim ) ) {
        extern struct char_data *mob_proto;
        mob_proto[GET_MOB_RNUM(victim)].mob_specials.count_death++;
      }
      if (IS_AFFECTED(ch, AFF_GROUP)) {
        group_gain(ch, victim);
      } else {
        exp = calc_karma(ch, victim);
        exp = gain_exp(ch, exp, 0);
        if ( exp >= 100 || access_level(ch, LVL_BUILDER) ) {
          sprintf(buf,"%s gains %.2f karma from killing %s.",
                  IS_NPC(ch) ? GET_NAME(ch) : GET_CHAR_NAME(ch),
                  (double)exp/100.0, GET_NAME(victim));
          mudlog(buf, ch, LOG_DEATHLOG, TRUE);
        }
        if ( IS_NPC( victim ) ) {
          extern struct char_data *mob_proto;
          mob_proto[GET_MOB_RNUM(victim)].mob_specials.value_death_karma += exp;
        }

        send_to_char(ch, "You receive %0.2f karma.\r\n", ((float)exp / 100));
      }
    }
    if (!IS_NPC(victim)) {
      gen_death_msg(ch, victim, attacktype);
      if (MOB_FLAGGED(ch, MOB_MEMORY) && !IS_NPC(victim))
        forget(ch, victim);
    }
    die(victim);
    return 1;
  }
  return 0;
}

bool has_ammo(struct char_data *ch, int pos)
{
  int i;
  bool found = FALSE;
  struct obj_data *obj, *cont;

  if (pos != WEAR_WIELD && pos != WEAR_HOLD)
    return FALSE;

  struct obj_data *wielded = GET_EQ(ch, pos);

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON)
  {
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_QUIVER)
        for (obj = GET_EQ(ch, i)->contains; obj; obj = GET_EQ(ch, i)->next_content)
          if (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(wielded, 5)) {
            GET_OBJ_VAL(GET_EQ(ch, i), 2)--;
            extract_obj(obj);
            found = TRUE;
          }
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WORN)
        for (cont = GET_EQ(ch,i)->contains; cont; cont = cont->next_content)
          if (GET_OBJ_TYPE(cont) == ITEM_QUIVER)
            for (obj = cont->contains; obj; obj = cont->next_content)
              if (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, 0) == GET_OBJ_VAL(wielded, 5)) {
                GET_OBJ_VAL(cont, 2)--;
                extract_obj(obj);
                found = TRUE;
              }

    }
    if (found)
      return TRUE;
    else {
      if (IS_NPC(ch)) {
        switch_weapons(ch, pos);
        return TRUE;
      } else {
        send_to_char("You're out of arrows!\r\n", ch);
        stop_fighting(ch);
        return FALSE;
      }
    }
  }
  if (wielded && (GET_OBJ_VAL(wielded, 5) > 0))
  {
    if (IS_NPC(ch)) {
      if (GET_OBJ_VAL(wielded, 6) > 0) {
        GET_OBJ_VAL(wielded, 6)--;
        if (wielded->contains)
          GET_OBJ_VAL(wielded->contains, 9)--;
        return TRUE;
      } else
        // make the mob do something (intelligent hopefully)
        if (!attempt_reload(ch, pos))
          switch_weapons(ch, pos);
      return TRUE;
    } else {
      if (wielded->contains && GET_OBJ_VAL(wielded->contains, 9) > 0) {
        if (wielded->contains)
          GET_OBJ_VAL(wielded->contains, 9)--;
        return TRUE;
      } else {
        send_to_char("*Click*\r\n", ch);
        return FALSE;
      }
    }
  } else
    return TRUE;
}

int check_smartlink(struct char_data *ch, struct obj_data *weapon)
{
  struct obj_data *obj, *access = NULL;
  int i, mod = 0;

  // are they wielding two weapons?
  if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD) &&
      CAN_WEAR(GET_EQ(ch, WEAR_HOLD), ITEM_WEAR_WIELD))
    return 0;

  for (i = 7; !mod && i < 10; i++)
  {
    if (GET_OBJ_VAL(weapon, i) > 0
        && real_object(GET_OBJ_VAL(weapon, i)) > 0
        && (access = &obj_proto[real_object(GET_OBJ_VAL(weapon, i))])
        && GET_OBJ_VAL(access, 1) == 1) {
      for (obj = ch->cyberware; !mod && obj; obj = obj->next_content)
        if (GET_OBJ_VAL(obj, 2) == 22)
          if (GET_OBJ_VAL(obj, 3) == 1 || GET_OBJ_VAL(access, 2) == 1)
            mod = 2;
          else
            mod = 4;
      access = NULL;
    }
  }

  if (mod < 1 && AFF_FLAGGED(ch, AFF_LASER_SIGHT))
    mod = 1;

  return mod;
}

int check_recoil(struct char_data *ch, struct obj_data *gun)
{
  struct obj_data *obj;
  int i, rnum, comp = 0;

  if (!gun || GET_OBJ_TYPE(gun) != ITEM_WEAPON)
    return 0;

  for (i = 7; i < 10; i++)
  {
    obj = NULL;

    if (GET_OBJ_VAL(gun, i) > 0 &&
        (rnum = real_object(GET_OBJ_VAL(gun, i))) > -1 &&
        (obj = &obj_proto[rnum]) && GET_OBJ_TYPE(obj) == ITEM_GUN_ACCESSORY) {
      if (GET_OBJ_VAL(obj, 1) == 3)
        comp += 0 - GET_OBJ_VAL(obj, 2);
      else if (GET_OBJ_VAL(obj, 1) == 4)
        comp++;
    }
  }

  return comp;
}

void astral_fight(struct char_data *ch, struct char_data *vict)
{
  int w_type, dam, power, attack_success, newskill, skill_total, base_target;
  bool focus = FALSE, is_physical;

  struct obj_data *wielded = ch->equipment[WEAR_WIELD];

  if (ch->in_room != vict->in_room)
  {
    stop_fighting(ch);
    if (FIGHTING(vict) == ch)
      stop_fighting(vict);
    return;
  }

  if ((IS_PROJECT(ch) || PLR_FLAGGED(ch, PLR_PERCEIVE))
      && wielded
      && GET_OBJ_TYPE(wielded) == ITEM_WEAPON
      && GET_OBJ_VAL(wielded, 3) < TYPE_TASER
      && GET_OBJ_VAL(wielded, 7)
      && GET_OBJ_VAL(wielded, 8)
      && GET_OBJ_VAL(wielded, 9) == GET_IDNUM(ch))
    focus = TRUE;
  else if (wielded)
  {
    stop_fighting(ch);
    return;
  }

  if (IS_PROJECT(ch) && (ch != vict) && PLR_FLAGGED(vict, PLR_PERCEIVE) &&
      !PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict,PLR_WANTED) &&
      (!PRF_FLAGGED(ch, PRF_PKER) || !PRF_FLAGGED(vict, PRF_PKER)) &&
      !PLR_FLAGGED(ch->desc->original, PLR_KILLER))
  {
    PLR_FLAGS(ch->desc->original).SetBit(PLR_KILLER);
    sprintf(buf, "PC Killer bit set on %s (astral) for initiating attack on %s at %s.",
            GET_CHAR_NAME(ch), GET_CHAR_NAME(vict), world[vict->in_room].name);
    mudlog(buf, ch, LOG_MISCLOG, TRUE);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }

  if (GET_POS(vict) <= POS_DEAD)
  {
    log("SYSERR: Attempt to damage a corpse.");
    return;                     /* -je, 7/7/92 */
  }

  if (wielded)
    w_type = GET_OBJ_VAL(wielded, 3);
  else
  {
    if (ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type;
    else
      w_type = TYPE_HIT;
  }

  if (((w_type == TYPE_HIT) || (w_type == TYPE_BLUDGEON) || (w_type == TYPE_PUNCH) ||
       (w_type == TYPE_TASER) || (w_type == TYPE_CRUSH) || (w_type == TYPE_POUND)) &&
      (GET_MENTAL(vict) >= 100))
    is_physical = FALSE;
  else
    is_physical = TRUE;

  base_target = 4 + modify_target(ch);

  if (!AWAKE(vict))
    base_target -= 2;

  if ((w_type != TYPE_HIT) && wielded)
  {
    power = (GET_OBJ_VAL(wielded, 0) ? GET_OBJ_VAL(wielded, 0) : GET_STR(ch)) +
            GET_OBJ_VAL(wielded, 2);
    if (focus)
      power += GET_OBJ_VAL(wielded, 8);
    power -= GET_IMPACT(vict);
    dam = MODERATE;
    if (IS_SPIRIT(vict))
      skill_total = GET_WIL(ch);
    else if (GET_SKILL(ch, GET_OBJ_VAL(wielded, 4)) < 1) {
      newskill = return_general(GET_OBJ_VAL(wielded, 4));
      if (GET_SKILL(ch, newskill) < 1)
        skill_total = reverse_web(ch, newskill, base_target);
      else
        skill_total = GET_SKILL(ch, newskill);
    } else
      skill_total = GET_SKILL(ch, GET_OBJ_VAL(wielded, 4));
  } else
  {
    power = GET_STR(ch) - GET_IMPACT(vict);
    if (IS_PROJECT(ch) || PLR_FLAGGED(ch, PLR_PERCEIVE))
      dam = LIGHT;
    else
      dam = MODERATE;
    if (IS_SPIRIT(vict))
      skill_total = GET_WIL(ch);
    else if (GET_SKILL(ch, SKILL_UNARMED_COMBAT) < 1) {
      if (GET_SKILL(ch, SKILL_SORCERY) < 1) {
        newskill = SKILL_UNARMED_COMBAT;
        skill_total = reverse_web(ch, newskill, base_target);
      } else
        skill_total = GET_SKILL(ch, SKILL_SORCERY);
    } else
      skill_total = MAX(GET_SKILL(ch, SKILL_UNARMED_COMBAT),
                        GET_SKILL(ch, SKILL_SORCERY));
  }

  skill_total += (int)(GET_ASTRAL(ch) / 2);

  if (power < 2)
  {
    skill_total = MAX(0, skill_total - (2 - power));
    power = 2;
  }

  if (AWAKE(vict))
    attack_success = resisted_test(skill_total, base_target,
                                   (int)(GET_ASTRAL(vict) / 2), power + modify_target(vict));
  else
    attack_success = success_test(skill_total, base_target);

  if (attack_success < 1)
  {
    if (!AFF_FLAGGED(ch, AFF_COUNTER_ATT)) {
      if ((GET_ASTRAL(vict) > 0) && (attack_success < 0)
          && FIGHTING(vict) == ch && GET_POS(vict) <= POS_SLEEPING) {
        send_to_char(ch, "%s counters your attack!\r\n", GET_NAME(vict));
        send_to_char(vict, "You counter %s's attack!\r\n", GET_NAME(ch));
        AFF_FLAGS(vict).SetBit(AFF_COUNTER_ATT);
        astral_fight(vict, ch);
      }
      return;
    } else {
      AFF_FLAGS(ch).RemoveBit(AFF_COUNTER_ATT);
      return;
    }
  } else if (AFF_FLAGGED(ch, AFF_COUNTER_ATT))
    AFF_FLAGS(ch).RemoveBit(AFF_COUNTER_ATT);

  attack_success -= success_test(GET_BOD(vict), power);

  dam = convert_damage(stage(attack_success, dam));

  if (IS_PROJECT(ch) && PLR_FLAGGED(ch->desc->original, PLR_KILLER))
    dam = 0;

  damage(ch, vict, dam, w_type, is_physical);
  if (IS_PROJECT(vict) && dam > 0)
    damage(vict->desc->original, vict->desc->original, dam, 0, is_physical);
}

void remove_throwing(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  int i, pos, type;

  for (pos = WEAR_WIELD; pos <= WEAR_HOLD; pos++)
    if (GET_EQ(ch, pos))
    {
      type = GET_OBJ_VAL(GET_EQ(ch, pos), 3);
      if (type == TYPE_SHURIKEN || type == TYPE_THROWING_KNIFE) {
        extract_obj(unequip_char(ch, pos));
        for (i = 0; i < NUM_WEARS; i++)
          if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_QUIVER)
            for (obj = GET_EQ(ch, i)->contains; obj; obj = obj->next_content)
              if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && GET_OBJ_VAL(obj, 3) == type) {
                obj_from_obj(obj);
                equip_char(ch, obj, WEAR_WIELD);
                return;
              }
        return;
      }
    }
}

void hit(struct char_data * ch, struct char_data * victim, int type)
{
  char rbuf[MAX_STRING_LENGTH];
  static struct obj_data *wielded;
  static bool is_physical;
  static bool melee;
  static int base_target, power, w_type, damage_total;
  struct obj_data *obj, *clip = NULL;
  struct veh_data *veh;
  struct char_data *vict;

  int attack_success = 0, attack_resist=0, skill_total = 1, newskill;
  int room, nextroom, dir, distance, pos, range, dead = 0, hits = 0;
  int vict_found, recoil=0, burst=0, recoil_comp=0;
  int tsleep=0, tcansee=0, toffhand=0, ttwoweap=0, tsmartlink=0;
  int tdistance=0, modtarget=0, tsight=0;

  if (IS_AFFECTED(ch, AFF_PETRIFY))
    return;

  if ((IS_ASTRAL(victim) && !IS_ASTRAL(ch) && !IS_DUAL(ch) &&
       !PLR_FLAGGED(ch, PLR_PERCEIVE)) ||
      (IS_ASTRAL(ch) &&
       !IS_ASTRAL(victim) && !IS_DUAL(victim) &&
       !PLR_FLAGGED(victim, PLR_PERCEIVE)))
    return;

  if ((IS_ASTRAL(victim) && CAN_SEE(ch, victim)) ||
      (IS_ASTRAL(ch) &&
       (IS_DUAL(victim) || PLR_FLAGGED(victim, PLR_PERCEIVE))))
  {
    astral_fight(ch, victim);
    return;
  }

  for (pos = WEAR_WIELD; !dead && pos <= WEAR_HOLD; pos++)
  {
    if (GET_WIELDED(ch, pos - WEAR_WIELD)) {
      if (!(wielded = GET_EQ(ch, pos)))
        GET_WIELDED(ch, pos - WEAR_WIELD) = 0;
    } else
      wielded = NULL;
    if (pos == WEAR_HOLD && (type == TYPE_MELEE || !wielded))
      continue;
    else if (pos == WEAR_WIELD && !wielded && GET_WIELDED(ch, 1) &&
             GET_EQ(ch, WEAR_HOLD))
      continue;

    // before you do anything, check to see if they have ammo
    if (wielded && !has_ammo(ch, pos)) {
      if (!FIGHTING(victim)
          && ch->in_room == victim->in_room
          && GET_POS(victim) > POS_STUNNED)
        set_fighting(victim, ch);
      hits++;
      continue;
    }

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON &&
        GET_OBJ_VAL(wielded, 3) >= TYPE_HAND_GRENADE &&
        GET_OBJ_VAL(wielded, 3) <= TYPE_ROCKET && ch->in_room == victim->in_room)
      continue;

    RIG_VEH(ch, veh);
    if ((ch->in_room != victim->in_room && !RANGE_OK(ch)) &&
        !(veh && veh->in_room == victim->in_room)) {
      stop_fighting(ch);
      return;
    }

    // determine attack type
    if (wielded && (GET_OBJ_TYPE(wielded) == ITEM_WEAPON ||
                    GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON))
      w_type = GET_OBJ_VAL(wielded, 3);
    else if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type;
    else
      w_type = TYPE_HIT;

    /* determine if this attack does mental or physical damage */
    if ((w_type == TYPE_HIT || w_type == TYPE_BLUDGEON || w_type == TYPE_PUNCH ||
         w_type == TYPE_TASER || w_type == TYPE_CRUSH || w_type == TYPE_POUND) &&
        GET_MENTAL(victim) >= 100)
      is_physical = FALSE;
    else
      is_physical = TRUE;

    /* determine if attacker is using melee or fire combat */
    if (!IS_GUN(w_type))
      melee = TRUE;
    else {
      melee = FALSE;
      if (wielded)
        clip = wielded->contains;
    }
    if (clip && GET_OBJ_VAL(clip, 2) == AMMO_GEL)
      is_physical = FALSE;
    if (type == TYPE_MELEE) {
      if (!melee) {
        wielded = GET_EQ(victim, WEAR_WIELD);
        if (wielded && (GET_OBJ_TYPE(wielded) == ITEM_WEAPON ||
                        GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON))
          w_type = GET_OBJ_VAL(wielded, 3);
        else {
          if (IS_NPC(victim) && (victim->mob_specials.attack_type != 0))
            w_type = victim->mob_specials.attack_type;
          else
            w_type = TYPE_HIT;
        }
        damage(victim, ch, -1, w_type, TRUE);
        return;
      }
      if ( GET_POS(victim) <= POS_SLEEPING ) {
        send_to_char(victim, "%s counters your attack!\r\n", GET_NAME(ch));
        send_to_char(ch, "You counter %s's attack!\r\n", GET_NAME(victim));
        AFF_FLAGS(ch).SetBit(AFF_COUNTER_ATT);
      }
    }

    if (wielded && IS_BURST(wielded)) {
      if (IS_NPC(ch) && !clip) {
        if (GET_OBJ_VAL(wielded, 6) >= 2) {
          burst = 3;
          GET_OBJ_VAL(wielded, 6) -= 2;
        } else if (GET_OBJ_VAL(wielded, 6) == 1) {
          burst = 2;
          GET_OBJ_VAL(wielded, 6)--;
        } else
          burst = 0;
      } else if (clip) {
        if (GET_OBJ_VAL(clip, 9) >= 2) {
          burst = 3;
          GET_OBJ_VAL(clip, 9) -= 2;
        } else if (GET_OBJ_VAL(clip, 9) == 1) {
          burst = 2;
          GET_OBJ_VAL(clip, 9)--;
        } else
          burst = 0;
      }
    }

    recoil_comp = check_recoil(ch, wielded);
    recoil = MAX(0, burst - recoil_comp);

    if (veh && get_speed(veh) > 0 && !(AFF_FLAGGED(ch, AFF_MANNING) || AFF_FLAGGED(ch, AFF_RIG)))
      if (get_speed(veh) < 60)
        recoil++;
      else if (get_speed(veh) < 120)
        recoil += 2;
      else if (get_speed(veh) < 200)
        recoil += 3;
      else
        recoil += 4;

    if (AFF_FLAGGED(ch, AFF_MANNING) || AFF_FLAGGED(ch, AFF_RIG))
      recoil = MAX(0, recoil - 2);

    /*  lower target number for victim being asleep */
    if (!AWAKE(victim))
      tsleep = -2;

    if (!CAN_SEE(ch, victim) && !(ch->in_veh || PLR_FLAGGED(ch, PLR_REMOTE) || AFF_FLAGGED(ch, AFF_MANNING))) {
      if (GET_POWER(ch, ADEPT_BLIND_FIGHTING))
        tcansee = 2;
      else
        tcansee = 4;
    }
    if (pos == WEAR_HOLD)
      toffhand = 1;

    // wielding two weapons
    if (GET_WIELDED(ch, 0) && GET_WIELDED(ch, 1))
      ttwoweap = 1;
    else if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON &&
             GET_OBJ_VAL(wielded, 3) >= TYPE_PISTOL)
      tsmartlink = 0 - check_smartlink(ch, wielded);

    if (wielded && IS_OBJ_STAT(wielded, ITEM_SNIPER) && ch->in_room == victim->in_room)
      tdistance += 6;

    if (ch->in_room != victim->in_room && !veh) {
      vict_found = FALSE;
      if (wielded && IS_RANGED(wielded)) {
        range = MIN(find_sight(ch), find_weapon_range(ch, GET_EQ(ch, pos)));
        for (dir = 0; dir < NUM_OF_DIRS && !vict_found; dir++) {
          room = ch->in_room;
          if (CAN_GO2(room, dir))
            nextroom = EXIT2(room, dir)->to_room;
          else
            nextroom = NOWHERE;
          for (distance = 1; (nextroom != NOWHERE) && (distance <= range) &&
               !vict_found; distance++) {
            for (vict = world[nextroom].people; vict;
                 vict = vict->next_in_room)
              if (vict == victim) {
                tdistance += 2 * distance;
                vict_found = TRUE;
                break;
              }
            room = nextroom;
            if (CAN_GO2(room, dir))
              nextroom = EXIT2(room, dir)->to_room;
            else
              nextroom = NOWHERE;
          }
        }
      }
      if (!vict_found)
        continue;
    }

    if (1) {
      sprintf( rbuf, "%s", GET_NAME( ch ) );
      rbuf[3] = 0;
      sprintf( rbuf+strlen(rbuf), "|%s", GET_NAME( victim ) );
      rbuf[7] = 0;
      sprintf( rbuf+strlen(rbuf),
               ">Targ: (b/r %d-%d) ",
               burst, recoil_comp );
      modtarget = modify_target_rbuf(ch, rbuf);

      buf_mod( rbuf, "Recoil", recoil);
      buf_mod( rbuf, "Sleep", tsleep);
      buf_mod( rbuf, "Cansee", tcansee);
      buf_mod( rbuf, "Offhand", toffhand);
      buf_mod( rbuf, "2Weap", ttwoweap);
      buf_mod( rbuf, "Smart", tsmartlink);
      buf_mod( rbuf, "Distance", tdistance);
      buf_mod( rbuf, "Sight", tsight);

      // set the base target number and modify
      base_target = 4 + modtarget + recoil + tsleep + tcansee
                    + toffhand + ttwoweap + tsmartlink + tdistance + tsight;

      buf_roll( rbuf, "Total", base_target);
      act( rbuf, 1, ch, NULL, NULL, TO_ROLLS );
    }

    damage_total = 0;
    hits++;
    /* find power, staging, and initial damage */
    if (w_type != TYPE_HIT && w_type != TYPE_PUNCH && wielded) {
      power = (GET_OBJ_VAL(wielded, 0) ? GET_OBJ_VAL(wielded, 0) :
               GET_STR(ch)) + GET_OBJ_VAL(wielded, 2) + burst;
      // this is how armor now works
      if (w_type >= TYPE_PISTOL) {
        if (clip)
          switch (GET_OBJ_VAL(clip, 2)) {
          case AMMO_APDS:
            power -= (int)(GET_BALLISTIC(victim) / 2);
            break;
          case AMMO_EX:
            power++;
          case AMMO_EXPLOSIVE:
            power -= GET_BALLISTIC(victim) - 1;
            break;
          case AMMO_FLECHETTE:
            if (!GET_IMPACT(victim) && !GET_BALLISTIC(victim))
              damage_total++;
            else
              power -= MAX(GET_BALLISTIC(victim), GET_IMPACT(victim) * 2);
            break;
          case AMMO_GEL:
            power -= GET_BALLISTIC(victim) + 2;
            break;
          default:
            power -= GET_BALLISTIC(victim);
          }
        else
          power -= GET_BALLISTIC(victim);
      } else
        power -= GET_IMPACT(victim);
      damage_total = GET_OBJ_VAL(wielded, 1) + (burst == 3 ? 1 : 0);
      if (AFF_FLAGGED(ch, AFF_RIG) || AFF_FLAGGED(ch, AFF_MANNING)) {
        if ((skill_total = GET_SKILL(ch, SKILL_GUNNERY)) < 1)
          skill_total = reverse_web(ch, newskill, base_target);
      } else if (IS_SPIRIT(victim)) {
        skill_total = GET_WIL(ch);
      } else if (GET_SKILL(ch, GET_OBJ_VAL(wielded, 4)) < 1) {
        newskill = return_general(GET_OBJ_VAL(wielded, 4));
        if (GET_SKILL(ch, newskill) < 1)
          skill_total = reverse_web(ch, newskill, base_target);
        else
          skill_total = GET_SKILL(ch, newskill);
      } else {
        skill_total = GET_SKILL(ch, GET_OBJ_VAL(wielded, 4));
      }
      if (w_type < TYPE_TASER
          && GET_OBJ_VAL(wielded, 7)
          && GET_OBJ_VAL(wielded, 8)
          && return_general(GET_OBJ_VAL(wielded, 4)) == SKILL_ARMED_COMBAT
                   && GET_OBJ_VAL(wielded, 9) == (IS_NPC(ch) ? -1 : GET_IDNUM(ch)))
        skill_total += GET_OBJ_VAL(wielded, 8);
    } else { // fists
      power = GET_STR(ch) - GET_IMPACT(victim);
      if ((IS_NPC(ch) || GET_TRADITION(ch) == TRAD_ADEPT) &&
          GET_POWER(ch, ADEPT_KILLING_HANDS) > 0) {
        damage_total = GET_POWER(ch, ADEPT_KILLING_HANDS);
        is_physical = TRUE;
      } else {
        for (obj = ch->cyberware;
             obj && !damage_total;
             obj = obj->next_content)
          if (w_type == TYPE_HIT || w_type == TYPE_MELEE)
            switch (GET_OBJ_VAL(obj, 2)) {
            case 19:
              damage_total = LIGHT;
              is_physical = TRUE;
              w_type = TYPE_SLASH;
              break;
            case 21:
              is_physical = TRUE;
              w_type = TYPE_SLASH;
              damage_total = MODERATE;
              break;
            case 29:
              power += GET_OBJ_VAL(obj, 0);
              damage_total = MODERATE;
              is_physical = TRUE;
              break;
            }

      }
      if (!damage_total)
        damage_total = MODERATE;

      if (IS_SPIRIT(victim)) {
        skill_total = GET_WIL(ch);
      } else if (GET_SKILL(ch, SKILL_UNARMED_COMBAT) < 1) {
        newskill = SKILL_UNARMED_COMBAT;
        skill_total = reverse_web(ch, newskill, base_target);
      } else {
        skill_total = GET_SKILL(ch, SKILL_UNARMED_COMBAT);
      }
    }

    if (IS_SPIRIT(victim)
        && wielded
        && ((GET_OBJ_TYPE(wielded) == ITEM_WEAPON
             && w_type >= TYPE_PISTOL)
            || (GET_OBJ_TYPE(wielded) == ITEM_FIREWEAPON
                && GET_OBJ_VAL(wielded, 5) == 1)))
      power -= (2 * GET_LEVEL(victim)) - MAX(GET_BALLISTIC(victim),
                                             GET_IMPACT(victim));

    // add offense (from combat pool) to dice pool for attacking--divided
    // by the number of folks you are attacking
    if (!IS_SPIRIT(victim) && !PLR_FLAGGED(ch, PLR_REMOTE))
      skill_total += MAX(0,GET_OFFENSE(ch)); // GET_NUM_FIGHTING(ch);

    // find # of attack successes, allow defense if victim is awake
    attack_success = success_test(skill_total, base_target);

    modtarget = modify_target(victim);
    if (AWAKE(victim)) {
      attack_resist = success_test(GET_DEFENSE(victim), 4);
    } else {
      // Always hit !awake victims
      attack_success = MAX( attack_success, 1 );
    }

    sprintf(rbuf,"Fight: I %2d, Ski %d, Targ %d, Succ %d. %sDef %d, Targ %d+%d, Succ %d. L:%d",
            GET_INIT_ROLL(ch),
            skill_total, base_target, attack_success,
            AWAKE(victim) ? "" : "!A ",
            GET_DEFENSE(victim), power, modtarget, attack_resist,
            damage_total);
    act(rbuf, FALSE, ch, NULL, NULL, TO_ROLLS);

    attack_success -= attack_resist;

    /* if there were no successes, fighter missed from lack of skill or dodge */
    /* send a -1 damage to indicate complete miss                    */
    if (ch->in_room != victim->in_room && attack_success < 1) {
      if (wielded && GET_WSPEC(wielded) != NULL) {
        if (!(GET_WSPEC(wielded) (ch, victim, wielded, -1)))
          dead = damage(ch, victim, -1, w_type, is_physical);
        else if (GET_POS(victim) == POS_DEAD || GET_POS(ch) == POS_DEAD)
          dead = TRUE;
      } else
        dead = damage(ch, victim, -1, w_type, is_physical);
      remove_throwing(ch);
      continue;
    } else if (attack_success < 1) {
      if (!AFF_FLAGGED(ch, AFF_COUNTER_ATT)) {
        if ((GET_DEFENSE(victim) > 0)
            && melee
            && FIGHTING(victim) == ch
            && GET_POS(victim) == POS_FIGHTING ) {
          if ( type == TYPE_MELEE )
            hit(victim, ch, TYPE_HIT);
        } else if (wielded && GET_WSPEC(wielded) != NULL) {
          if (!(GET_WSPEC(wielded) (ch, victim, wielded, -1)))
            dead = damage(ch, victim, -1, w_type, is_physical);
          else if (GET_POS(victim) == POS_DEAD || GET_POS(ch) == POS_DEAD)
            dead = TRUE;
        } else {
          dead = damage(ch, victim, -1, w_type, is_physical);
        }
        remove_throwing(ch);
        continue;
      } else {
        if (wielded && GET_WSPEC(wielded) != NULL) {
          if (!(GET_WSPEC(wielded) (ch, victim, wielded, -1)))
            dead = damage(ch, victim, -1, w_type, is_physical);
          else if (GET_POS(victim) == POS_DEAD || GET_POS(ch) == POS_DEAD)
            dead = TRUE;
        } else
          dead = damage(ch, victim, -1, w_type, is_physical);
        AFF_FLAGS(ch).RemoveBit(AFF_COUNTER_ATT);
        remove_throwing(ch);
        continue;
      }
    } else if (AFF_FLAGGED(ch, AFF_COUNTER_ATT))
      AFF_FLAGS(ch).RemoveBit(AFF_COUNTER_ATT);

    int bod_success=0;

    // then subtract successes from body test of victim if they did not dodge
    if (IS_SPIRIT(ch) && !IS_NPC(victim) && (GET_TRADITION(victim) == TRAD_HERMETIC ||
        GET_TRADITION(victim) == TRAD_SHAMANIC))
      bod_success = success_test(MAX(GET_BOD(victim), GET_SKILL(victim, SKILL_CONJURING)),
                                 power);
    else
      bod_success = success_test(GET_BOD(victim), power);

    attack_success -= bod_success;
    int old_damage_total = damage_total;
    int staged_damage = stage(attack_success, damage_total);
    damage_total = convert_damage(staged_damage);

    sprintf(rbuf, "Fight: Bod %d, Pow %d, Suc %d.  %d(%d)->%d.  %d%c.",
            GET_BOD(victim), power, bod_success,
            old_damage_total, attack_success, staged_damage,
            damage_total,
            is_physical ? 'P' : 'M');
    act(rbuf, FALSE, ch, NULL, NULL, TO_ROLLS);

    if (wielded && GET_WSPEC(wielded) != NULL) {
      if (!(GET_WSPEC(wielded) (ch, victim, wielded, -1)))
        dead = damage(ch, victim, damage_total, w_type, is_physical);
      else if (GET_POS(victim) == POS_DEAD || GET_POS(ch) == POS_DEAD)
        dead = TRUE;
    } else
      dead = damage(ch, victim, damage_total, w_type, is_physical);
    remove_throwing(ch);
  }

  if (!hits)
    stop_fighting(ch);

  if (wielded && GET_OBJ_VAL(wielded, 4) >= SKILL_MACHINE_GUNS && GET_OBJ_VAL(wielded, 4) <= SKILL_ASSAULT_CANNON)
  {
    for (int i = 0; i < (NUM_WEARS - 1); i++)
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_GYRO)
        return;
    damage(ch, ch, convert_damage(stage(-success_test(GET_BOD(ch), power / 2 + modify_target(ch)), LIGHT))
           , TYPE_HIT, FALSE);
  }
}

int find_sight(struct char_data *ch)
{
  int sight;

  if ((!IS_NPC(ch) && access_level(ch, LVL_VICEPRES)) || AFF_FLAGGED(ch, AFF_VISION_MAG_3))
    sight = 4;
  else if (AFF_FLAGGED(ch, AFF_VISION_MAG_2))
    sight = 3;
  else if (AFF_FLAGGED(ch, AFF_VISION_MAG_1))
    sight = 2;
  else
    sight = 1;

  /* add more weather conditions here to affect scan */
  if (SECT(ch->in_room) != SECT_INSIDE && (IS_NPC(ch) || !access_level(ch, LVL_VICEPRES)))
    switch (weather_info.sky)
    {
    case SKY_RAINING:
      sight -= 1;
      break;
    case SKY_LIGHTNING:
      sight -= 2;
      break;
    }

  sight = MIN(4, MAX(1, sight));

  return sight;
}

int find_weapon_range(struct char_data *ch, struct obj_data *weapon)
{
  int temp;

  if ( weapon == NULL )
    return 0;

  if (GET_OBJ_TYPE(weapon) == ITEM_FIREWEAPON)
  {
    temp = MIN(MAX(1, ((int)(GET_STR(ch)/3))), 4);
    return temp;
  }

  switch(GET_OBJ_VAL(weapon, 3))
  {
  case TYPE_SHURIKEN:
  case TYPE_THROWING_KNIFE:
  case TYPE_HAND_GRENADE:
  case TYPE_PISTOL:
  case TYPE_SHOTGUN:
  case TYPE_BLAST:
    return 1;
  case TYPE_GRENADE_LAUNCHER:
  case TYPE_BIFURCATE:
    return 2;
  case TYPE_RIFLE:
  case TYPE_MACHINE_GUN:
    return 3;
  case TYPE_CANNON:
  case TYPE_ROCKET:
    return 4;
  default:
    return 0;
  }
}

void ranged_response(struct char_data *ch, struct char_data *vict)
{
  int range, sight, distance, dir, found = 0;
  long room, nextroom = NOWHERE;
  struct char_data *temp;

  if (!vict || ch->in_room == vict->in_room ||
      GET_POS(vict) <= POS_STUNNED || vict->in_room == NOWHERE)
    return;

  if (IS_NPC(vict) && (MOB_FLAGGED(vict, MOB_INANIMATE) || MOB_FLAGGED(vict, MOB_NOKILL)))
    return;
  if (GET_POS(vict) < POS_FIGHTING)
    GET_POS(vict) = POS_STANDING;
  if (MOB_FLAGGED(vict, MOB_WIMPY))
    do_flee(vict, "", 0, 0);
  else if (RANGE_OK(vict) && !FIGHTING(vict))
  {
    sight = find_sight(vict);
    range = find_weapon_range(vict, GET_EQ(vict, WEAR_WIELD));
    for (dir = 0; dir < NUM_OF_DIRS  && !found; dir++) {
      room = vict->in_room;
      if (CAN_GO2(room, dir))
        nextroom = EXIT2(room, dir)->to_room;
      else
        nextroom = NOWHERE;
      for (distance = 1; !found && ((nextroom != NOWHERE) && (distance <= 4)); distance++) {
        for (temp = world[nextroom].people; !found && temp; temp = temp->next_in_room)
          if (temp == ch && (distance > range || distance > sight)) {
            act("$n runs after $s distant attacker.", TRUE, vict, 0, 0, TO_ROOM);
            act("You charge after $N.", FALSE, vict, 0, ch, TO_CHAR);
            char_from_room(vict);
            char_to_room(vict, EXIT2(room, rev_dir[dir])->to_room);
            set_fighting(vict, ch);
            found = 1;
            if (vict->in_room == ch->in_room) {
              act("$n arrives in a rush of fury, immediately attacking $N!",
                  TRUE, vict, 0, ch, TO_NOTVICT);
              act("$n arrives in a rush of fury, rushing straight towards you!",
                  FALSE, vict, 0, ch, TO_VICT);
            }
          }
        room = nextroom;
        if (CAN_GO2(room, dir))
          nextroom = EXIT2(room, dir)->to_room;
        else
          nextroom = NOWHERE;
      }
    }
    if (!found)
      set_fighting(vict, ch);
  } else if (!FIGHTING(vict))
  {
    for (dir = 0; dir < NUM_OF_DIRS && !found; dir++) {
      if (CAN_GO2(vict->in_room, dir) && EXIT2(vict->in_room, dir)->to_room == ch->in_room) {
        act("$n runs after $s distant attacker.", TRUE, vict, 0, 0, TO_ROOM);
        act("You charge after $N.", FALSE, vict, 0, ch, TO_CHAR);
        char_from_room(vict);
        char_to_room(vict, ch->in_room);
        set_fighting(vict, ch);
        act("$n arrives in a rush of fury, immediately attacking $N!", TRUE, vict, 0, ch, TO_NOTVICT);
        act("$n arrives in a rush of fury, rushing straight towards you!", FALSE, vict, 0, ch, TO_VICT);
      }
    }
  }
  return;
}

void explode(struct char_data *ch, struct obj_data *weapon, int room)
{
  int damage_total, i, power, level;
  struct char_data *victim, *next_vict;
  struct obj_data *obj, *next;

  power = GET_OBJ_VAL(weapon, 0);
  level = GET_OBJ_VAL(weapon, 1);

  extract_obj(weapon);

  if (world[room].people)
  {
    act("The room is lit by an explosion!", FALSE, world[room].people, 0, 0, TO_ROOM);
    act("The room is lit by an explosion!", FALSE, world[room].people, 0, 0, TO_CHAR);
  }

  for (obj = world[room].contents; obj; obj = next)
  {
    next = obj->next_content;
    damage_obj(NULL, obj, level * 2 + (int)(power / 6), DAMOBJ_EXPLODE);
  }

  for (victim = world[room].people; victim; victim = next_vict)
  {
    next_vict = victim->next_in_room;
    if (IS_ASTRAL(victim))
      continue;

    act("You are hit by the flames!", FALSE, victim, 0, 0, TO_CHAR);

    for (obj = victim->carrying; obj; obj = next) {
      next = obj->next_content;
      if (number(1, 100) < 50)
        damage_obj(NULL, obj, level * 2 + (int)(power / 6), DAMOBJ_EXPLODE);
    }

    for (i = 0; i < (NUM_WEARS - 1); i++)
      if (GET_EQ(victim, i) && number(1, 100) < 100)
        damage_obj(NULL, GET_EQ(victim, i), level * 2 + (int)(power / 6),
                   DAMOBJ_EXPLODE);

    if (IS_NPC(victim) && !FIGHTING(victim)) {
      GET_DEFENSE(victim) = GET_COMBAT(victim);
      GET_OFFENSE(victim) = 0;
    }
    damage_total =
      convert_damage(stage((number(1, 3) - success_test(GET_BOD(victim) +
                            GET_DEFENSE(victim), MAX(2, (power -
                                                         (int)(GET_IMPACT(victim) / 2))) + modify_target(victim))),
                           level));
    if (!ch)
      damage(victim, victim, damage_total, TYPE_EXPLOSION, PHYSICAL);
    else
      damage(ch, victim, damage_total, TYPE_EXPLOSION, PHYSICAL);
  }

  for (i = 0; i < NUM_OF_DIRS; i++)
    if (world[room].dir_option[i] && IS_SET(world[room].dir_option[i]->exit_info, EX_CLOSED))
      damage_door(NULL, room, i, level * 2 + (int)(power / 6), DAMOBJ_EXPLODE);
}

void target_explode(struct char_data *ch, struct obj_data *weapon, int room, int mode)
{
  int damage_total, i;
  struct char_data *victim, *next_vict;
  struct obj_data *obj, *next;

  sprintf(buf, "The room is lit by a%s explosion!",
          (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET ? " massive" : "n"));

  if (world[room].people)
  {
    act(buf, FALSE, world[room].people, 0, 0, TO_ROOM);
    act(buf, FALSE, world[room].people, 0, 0, TO_CHAR);
  }

  for (obj = world[room].contents; obj; obj = next)
  {
    next = obj->next_content;
    damage_obj(NULL, obj, GET_OBJ_VAL(weapon, 1) * 2 +
               (int)(GET_OBJ_VAL(weapon, 0) / 6), DAMOBJ_EXPLODE);
  }

  for (victim = world[room].people; victim; victim = next_vict)
  {
    next_vict = victim->next_in_room;
    if (IS_ASTRAL(victim))
      continue;

    act("You are hit by the flames!", FALSE, victim, 0, 0, TO_CHAR);

    for (obj = victim->carrying; obj; obj = next) {
      next = obj->next_content;
      if (number(1, 100) < 50)
        damage_obj(NULL, obj, GET_OBJ_VAL(weapon, 1) * 2 +
                   (int)(GET_OBJ_VAL(weapon, 0) / 6), DAMOBJ_EXPLODE);
    }

    for (i = 0; i < (NUM_WEARS - 1); i++)
      if (GET_EQ(victim, i) && number(1, 100) < 100)
        damage_obj(NULL, GET_EQ(victim, i), GET_OBJ_VAL(weapon, 1) * 2 +
                   (int)(GET_OBJ_VAL(weapon, 0) / 6), DAMOBJ_EXPLODE);

    if (IS_NPC(victim) && !FIGHTING(victim)) {
      GET_DEFENSE(victim) = GET_COMBAT(victim);
      GET_OFFENSE(victim) = 0;
    }
    if (!mode) {
      if (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET)
        damage_total = convert_damage(stage((number(3,6) - success_test(GET_BOD(victim) + GET_DEFENSE(victim),
                                             MAX(2, (GET_OBJ_VAL(weapon, 0) - (int)(GET_IMPACT(victim) / 2))) +
                                             modify_target(victim))), GET_OBJ_VAL(weapon, 1)));
      else
        damage_total = convert_damage(stage((number(2,5) - success_test(GET_BOD(victim) + GET_DEFENSE(victim),
                                             MAX(2, (GET_OBJ_VAL(weapon, 0) - (int)(GET_IMPACT(victim) / 2))) +
                                             modify_target(victim))), GET_OBJ_VAL(weapon, 1)));
    } else
      damage_total = convert_damage(stage((number(2,5) - success_test(GET_BOD(victim) + GET_DEFENSE(victim),
                                           MAX(2, (GET_OBJ_VAL(weapon, 0) - 4 - (int)(GET_IMPACT(victim) / 2))) +
                                           modify_target(victim))), GET_OBJ_VAL(weapon, 1) - 1));
    damage(ch, victim, damage_total, TYPE_EXPLOSION, PHYSICAL);
  }

  if (!mode)
    for (i = 0; i < NUM_OF_DIRS; i++)
      if (world[room].dir_option[i] &&
          IS_SET(world[room].dir_option[i]->exit_info, EX_CLOSED))
        damage_door(NULL, room, i, GET_OBJ_VAL(weapon, 1) * 2 +
                    (int)(GET_OBJ_VAL(weapon, 0) / 6), DAMOBJ_EXPLODE);
}

void range_combat(struct char_data *ch, char *target, struct obj_data *weapon,
                  int range, int dir)
{
  int room, nextroom, distance, sight, temp, temp2, left, right, scatter[4];
  int power, level;
  struct char_data *vict = NULL;

  if (world[ch->in_room].peaceful)
  {
    send_to_char("This room just has a peaceful, easy feeling...\r\n", ch);
    return;
  }

  sight = find_sight(ch);

  if (CAN_GO2(ch->in_room, dir))
    nextroom = EXIT2(ch->in_room, dir)->to_room;
  else
    nextroom = NOWHERE;

  if (GET_OBJ_TYPE(weapon) == ITEM_WEAPON && GET_OBJ_VAL(weapon, 3) == TYPE_HAND_GRENADE)
  {
    if (nextroom == NOWHERE) {
      send_to_char("There seems to be something in the way...\r\n", ch);
      return;
    }
    if (world[nextroom].peaceful) {
      send_to_char("Nah - leave them in peace.\r\n", ch);
      return;
    }
    power = GET_OBJ_VAL(weapon, 0);
    level = GET_OBJ_VAL(weapon, 1);
    if (FIGHTING(ch))
      stop_fighting(ch);
    act("You pull the pin and throw $p!", FALSE, ch, weapon, 0, TO_CHAR);
    act("$n pulls the pin and throws $p!", FALSE, ch, weapon, 0, TO_ROOM);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    temp = MAX(1, GET_SKILL(ch, SKILL_THROWING_WEAPONS));

    if (!number(0, 2)) {
      sprintf(buf, "A defective grenade lands on the floor.\r\n");
      if (world[nextroom].people) {
        act(buf, FALSE, world[nextroom].people, 0, 0, TO_ROOM);
        act(buf, FALSE, world[nextroom].people, 0, 0, TO_CHAR);
      }
      return;
    } else if (!success_test(temp+GET_OFFENSE(ch), 5) < 2) {
      left = -1;
      right = -1;
      if (dir < UP) {
        if ((dir - 1) < NORTH)
          left = NORTHWEST;
        else
          left = dir - 1;
        if ((dir + 1) > NORTHWEST)
          right = NORTH;
        else
          right = dir + 1;
      }
      scatter[0] = ch->in_room;
      if (left > 0 && CAN_GO(ch, left))
        scatter[1] = EXIT(ch, left)->to_room;
      else
        scatter[1] = NOWHERE;
      if (right > 0 && CAN_GO(ch, right))
        scatter[2] = EXIT(ch, right)->to_room;
      else
        scatter[2] = NOWHERE;
      if (CAN_GO2(nextroom, dir))
        scatter[3] = EXIT2(nextroom, dir)->to_room;
      else
        scatter[3] = NOWHERE;
      for (temp = 0, temp2 = 0; temp2 < 4; temp2++)
        if (scatter[temp2] != NOWHERE)
          temp++;
      for (temp2 = 0; temp2 < 4; temp2++)
        if (scatter[temp2] != NOWHERE && !number(0, temp-1)) {
          if (temp2 == 0) {
            act("$p deflects due to $n's poor accuracy, landing at $s feet.",
                FALSE, ch, weapon, 0, TO_ROOM);
            sprintf(buf, "Your realize your aim must've been off-target as "
                    "$p lands at your feet.");
          } else if (temp2 == 3)
            sprintf(buf, "Your aim is slightly off, going past its target.");
          else
            sprintf(buf, "Your aim is slightly off, and $p veers to the %s.",
                    dirs[temp2 == 1 ? left : right]);
          act(buf, FALSE, ch, weapon, 0, TO_CHAR);
          explode(ch, weapon, scatter[temp2]);
          return;
        }
    }
    explode(ch, weapon, nextroom);
    return;
  }

  for (distance = 1; ((nextroom != NOWHERE) && (distance <= sight)); distance++)
  {
    if ((vict = get_char_room(target, nextroom)) && vict != ch &&
        CAN_SEE(ch, vict))
      break;
    vict = NULL;
    room = nextroom;
    if (CAN_GO2(room, dir))
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;
  }

  if (vict)
  {
    if (vict == FIGHTING(ch)) {
      send_to_char("You're doing the best you can!\r\n", ch);
      return;
    } else if (world[vict->in_room].peaceful) {
      send_to_char("Nah - leave them in peace.\r\n", ch);
      return;
    } else if (distance > range) {
      act("$N seems to be out of $p's range.", FALSE, ch, weapon, vict, TO_CHAR);
      return;
    } else if (!ok_damage_shopkeeper(ch, vict)) {
      send_to_char("Maybe that's not such a good idea.\r\n", ch);
      return;
    }

    if (GET_OBJ_TYPE(weapon) == ITEM_FIREWEAPON) {
      act("$n draws $p and fires into the distance!", TRUE, ch, weapon, 0, TO_ROOM);
      act("You draw $p, aim it at $N and fire!", FALSE, ch, weapon, vict, TO_CHAR);
      check_killer(ch, vict);
      if (IS_NPC(vict) && !IS_PROJECT(vict) && !FIGHTING(vict)) {
        GET_DEFENSE(vict) = GET_COMBAT(vict);
        GET_OFFENSE(vict) = 0;
      }
      if (FIGHTING(ch))
        stop_fighting(ch);
      hit(ch, vict, TYPE_UNDEFINED);
      WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
      return;
    }

    switch(GET_OBJ_VAL(weapon, 3)) {
    case TYPE_SHURIKEN:
    case TYPE_THROWING_KNIFE:
      act("$n throws $p at something in the distance!", TRUE, ch, weapon, 0, TO_ROOM);
      act("You throw $p at $N!", FALSE, ch, weapon, vict, TO_CHAR);
      break;
    default:
      act("$n aims $p and fires into the distance!", TRUE, ch, weapon, 0, TO_ROOM);
      act("You aim $p at $N and fire!", FALSE, ch, weapon, vict, TO_CHAR);
      break;
    }
    if (GET_OBJ_VAL(weapon, 3) >= TYPE_PISTOL
        || GET_OBJ_VAL(weapon, 3) == TYPE_SHURIKEN
        || GET_OBJ_VAL(weapon, 3) == TYPE_THROWING_KNIFE) {
      check_killer(ch, vict);
      if (GET_OBJ_VAL(weapon, 3) < TYPE_PISTOL
          || GET_OBJ_VAL(weapon, 6) > 0) {
        if (IS_NPC(vict) && !IS_PROJECT(vict) && !FIGHTING(vict)) {
          GET_DEFENSE(vict) = GET_COMBAT(vict);
          GET_OFFENSE(vict) = 0;
        }
        if (FIGHTING(ch))
          stop_fighting(ch);
        hit(ch, vict, TYPE_UNDEFINED);
        ranged_response(ch, vict);
      } else
        send_to_char("*Click*\r\n", ch);
      WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
    } else {
      if (!has_ammo(ch, weapon->worn_on))
        return;
      stop_fighting(ch);
      if (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET) {
        if (!GET_SKILL(ch, SKILL_MISSILE_LAUNCHERS))
          temp = MAX(1, GET_SKILL(ch, SKILL_GUNNERY));
        else
          temp = GET_SKILL(ch, SKILL_MISSILE_LAUNCHERS);
      } else {
        if (!GET_SKILL(ch, SKILL_GRENADE_LAUNCHERS))
          temp = MAX(1, GET_SKILL(ch, SKILL_FIREARMS));
        else
          temp = GET_SKILL(ch, SKILL_GRENADE_LAUNCHERS);
      }

      WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

      if (!number(0,49)) {
        sprintf(buf, "A defective %s lands on the floor.",
                (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET ? "rocket" : "grenade"));
        act(buf, TRUE, vict, 0, 0, TO_ROOM);
        act(buf, TRUE, vict, 0, 0, TO_CHAR);
        return;
      } else if (!success_test(temp + GET_OFFENSE(ch), 6)) {
        left = -1;
        right = -1;
        if (dir < UP) {
          if ((dir - 1) < NORTH)
            left = NORTHWEST;
          else
            left = dir - 1;
          if ((dir + 1) > NORTHWEST)
            right = NORTH;
          else
            right = dir + 1;
        }
        scatter[0] = ch->in_room;
        if (left > 0 && CAN_GO(ch, left))
          scatter[1] = EXIT(ch, left)->to_room;
        else
          scatter[1] = NOWHERE;
        if (right > 0 && CAN_GO(ch, right))
          scatter[2] = EXIT(ch, right)->to_room;
        else
          scatter[2] = NOWHERE;
        if (CAN_GO2(nextroom, dir))
          scatter[3] = EXIT2(nextroom, dir)->to_room;
        else
          scatter[3] = NOWHERE;
        for (temp = 0, temp2 = 0; temp2 < 4; temp2++)
          if (scatter[temp2] != NOWHERE)
            temp++;
        for (temp2 = 0; temp2 < 4; temp2++)
          if (scatter[temp2] != NOWHERE && !number(0, temp-1)) {
            if (temp2 == 0) {
              act("$p's trajectory is slightly off...", FALSE, ch, weapon, 0, TO_ROOM);
              sprintf(buf, "Your arm jerks just before you fire...");
            } else if (temp2 == 3)
              sprintf(buf, "Your aim is slightly off, going past $N.");
            else
              sprintf(buf, "Your aim is slightly off, the %s veering to the %s.",
                      (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET ? "rocket" : "grenade"),
                      dirs[temp2 == 1 ? left : right]);
            act(buf, FALSE, ch, weapon, vict, TO_CHAR);
            target_explode(ch, weapon, scatter[temp2], 0);
            if (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET)
              for (temp = 0; temp < NUM_OF_DIRS; temp++)
                if (CAN_GO2(scatter[temp2], temp))
                  target_explode(ch, weapon, EXIT2(scatter[temp2], temp)->to_room, 1);
            return;
          }
      }
      temp2 = vict->in_room;
      target_explode(ch, weapon, vict->in_room, 0);
      if (GET_OBJ_VAL(weapon, 3) == TYPE_ROCKET)
        for (temp = 0; temp < NUM_OF_DIRS; temp++)
          if (CAN_GO2(temp2, temp))
            target_explode(ch, weapon, EXIT2(temp2, temp)->to_room, 1);
    }
    return;
  }
  bool found = FALSE;

  if (CAN_GO2(ch->in_room, dir))
    nextroom = EXIT2(ch->in_room, dir)->to_room;
  else
    nextroom = NOWHERE;

  // now we search for a door by the given name
  for (distance = 1; nextroom != NOWHERE && distance <= sight; distance++)
  {
    if (EXIT2(nextroom, dir) && EXIT2(nextroom, dir)->keyword &&
        isname(target, EXIT2(nextroom, dir)->keyword) &&
        !IS_SET(EXIT2(nextroom, dir)->exit_info, EX_DESTROYED) &&
        (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || IS_AFFECTED(ch, AFF_INFRAVISION) ||
         (IS_LIGHT(nextroom) || (IS_LOW(nextroom) &&
                                 IS_AFFECTED(ch, AFF_LOW_LIGHT))))) {
      found = TRUE;
      break;
    }
    room = nextroom;
    if (CAN_GO2(room, dir))
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;
  }

  if (found)
  {
    if (FIGHTING(ch)) {
      send_to_char("Maybe you'd better wait...\r\n", ch);
      return;
    } else if (!IS_SET(EXIT2(nextroom, dir)->exit_info, EX_CLOSED) && isname(target, EXIT2(nextroom, dir)->keyword) ) {
      send_to_char("You can only damage closed doors!\r\n", ch);
      return;
    } else if (world[nextroom].peaceful) {
      send_to_char("Nah - leave it in peace.\r\n", ch);
      return;
    } else if (distance > range) {
      send_to_char(ch, "The %s seems to be out of %s's range.\r\n",
                   CAP(fname(EXIT2(nextroom, dir)->keyword)),
                   weapon->text.name);
      return;
    }

    if (GET_OBJ_TYPE(weapon) == ITEM_FIREWEAPON) {
      act("$n draws $p and fires into the distance!", TRUE, ch, weapon, 0, TO_ROOM);
      sprintf(buf, "You draw $p, aim it at the %s and fire!",
              fname(EXIT2(nextroom, dir)->keyword));
      act(buf, FALSE, ch, weapon, vict, TO_CHAR);
    } else {
      switch(GET_OBJ_VAL(weapon, 3)) {
      case TYPE_SHURIKEN:
      case TYPE_THROWING_KNIFE:
        act("$n throws $p at something in the distance!", TRUE, ch, weapon, 0, TO_ROOM);
        sprintf(buf, "You throw $p at the %s!", fname(EXIT2(nextroom, dir)->keyword));
        act(buf, FALSE, ch, weapon, vict, TO_CHAR);
        break;
      default:
        act("$n aims $p and fires into the distance!", TRUE, ch, weapon, 0, TO_ROOM);
        sprintf(buf, "You aim $p at the %s and fire!",
                fname(EXIT2(nextroom, dir)->keyword));
        act(buf, FALSE, ch, weapon, vict, TO_CHAR);
        break;
      }
      if (GET_OBJ_VAL(weapon, 3) >= TYPE_PISTOL)
        if (!has_ammo(ch, weapon->worn_on))
          return;
    }
    switch (GET_OBJ_VAL(weapon, 3)) {
    case TYPE_SHURIKEN:
    case TYPE_THROWING_KNIFE:
    case TYPE_ARROW:
      damage_door(ch, nextroom, dir, GET_OBJ_VAL(weapon, 0), DAMOBJ_PIERCE);
      break;
    case TYPE_GRENADE_LAUNCHER:
      target_explode(ch, weapon, nextroom, 0);
      break;
    case TYPE_ROCKET:
      target_explode(ch, weapon, nextroom, 0);
      for (temp = 0; temp < NUM_OF_DIRS; temp++)
        if (CAN_GO2(nextroom, temp))
          target_explode(ch, weapon, EXIT2(nextroom, temp)->to_room, 1);
      break;
    default:
      damage_door(ch, nextroom, dir, GET_OBJ_VAL(weapon, 0), DAMOBJ_PROJECTILE);
      break;
    }
    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
    return;
  }

  sprintf(buf, "You can't see any %s there.\r\n", target);
  send_to_char(buf, ch);
  return;
}

void roll_individual_initiative(struct char_data *ch)
{
  if (AWAKE(ch))
  {
    if (AFF_FLAGGED(ch, AFF_PILOT))
      GET_INIT_ROLL(ch) = dice(1, 6) + GET_REA(ch);
    else
      GET_INIT_ROLL(ch) = dice(1 + GET_INIT_DICE(ch), 6) + GET_REA(ch);
    if (AFF_FLAGGED(ch, AFF_ACTION)) {
      GET_INIT_ROLL(ch) -= 10;
      AFF_FLAGS(ch).RemoveBit(AFF_ACTION);
    }
    if (combat_list && GET_INIT_ROLL(ch) >= GET_INIT_ROLL(combat_list))
      GET_INIT_ROLL(ch) = GET_INIT_ROLL(combat_list) - dice(1, 6);
  }
  if (1)
  {
    char rbuf[MAX_STRING_LENGTH];
    sprintf(rbuf,"Init: %2d %s",
            GET_INIT_ROLL(ch), GET_NAME(ch));
    act(rbuf,TRUE,ch,NULL,NULL,TO_ROLLS);
  }
}

void decide_combat_pool(void)
{
  struct char_data *ch;

  for (ch = combat_list; ch; ch = ch->next_fighting) {
    if (ch->bioware)
      check_adrenaline(ch, 0);

    if (IS_NPC(ch) && !IS_PROJECT(ch) && FIGHTING(ch)) {
      if (GET_INIT_ROLL(ch) == GET_INIT_ROLL(FIGHTING(ch)))
        GET_OFFENSE(ch) = GET_COMBAT(ch) >> 1;
      else if (GET_INIT_ROLL(ch) > GET_INIT_ROLL(FIGHTING(ch)))
        GET_OFFENSE(ch) = (int)(GET_COMBAT(ch) * .75);
      else
        GET_OFFENSE(ch) = (int)(GET_COMBAT(ch) / 4);
      GET_DEFENSE(ch) = GET_COMBAT(ch) - GET_OFFENSE(ch);
    }
  }
}

void roll_initiative(void)
{
  struct char_data *ch;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;
    roll_individual_initiative(ch);
  }
  pass = 0;
  return;
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  extern struct index_data *mob_index;
  int high = 0;
  if (combat_list) {
    if (GET_INIT_ROLL(combat_list) <= 0) {
      roll_initiative();
      order_list();
    }
    high = GET_INIT_ROLL(combat_list);
  }
  for (ch = combat_list; ch; ch = next_combat_list) {
    pass++;
    next_combat_list = ch->next_fighting;
    if (!(FIGHTING(ch) || FIGHTING_VEH(ch)) || GET_POS(ch) <= POS_STUNNED)
      stop_fighting(ch);
    else if (GET_INIT_ROLL(ch) >= MAX(0, high - 10)) {
      if (IS_AFFECTED(ch, AFF_APPROACH)) {
        if (IS_AFFECTED(FIGHTING(ch), AFF_APPROACH) || GET_POS(FIGHTING(ch)) < POS_FIGHTING ||
            (GET_EQ(ch, WEAR_WIELD) && (IS_GUN(GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3)) ||
                                        GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_ARROW)) ||
            (GET_EQ(ch, WEAR_HOLD) && (IS_GUN(GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 3)) ||
                                       GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 3) != TYPE_ARROW)) ||
            (FIGHTING(ch) && !(GET_EQ(FIGHTING(ch), WEAR_WIELD) && (IS_GUN(GET_OBJ_VAL(GET_EQ(FIGHTING(ch), WEAR_WIELD), 3)) ||
                               GET_OBJ_VAL(GET_EQ(FIGHTING(ch), WEAR_WIELD), 3) != TYPE_ARROW)) &&
             (GET_EQ(FIGHTING(ch), WEAR_HOLD) && (IS_GUN(GET_OBJ_VAL(GET_EQ(FIGHTING(ch), WEAR_HOLD), 3)) ||
                                                  GET_OBJ_VAL(GET_EQ(FIGHTING(ch), WEAR_HOLD), 3) != TYPE_ARROW)))) {
          AFF_FLAGS(ch).RemoveBit(AFF_APPROACH);
          AFF_FLAGS(FIGHTING(ch)).RemoveBit(AFF_APPROACH);
        } else {
          int target = 8;
          if (GET_TRADITION(ch) == TRAD_ADEPT && GET_POWER(ch, ADEPT_DISTANCE_STRIKE))
            target -= (int)GET_REAL_MAG(ch) / 150;
          if (success_test(GET_QUI(ch), target) > 1) {
            send_to_char(ch, "You close the distance and strike!\r\n");
            act("$n closes the distance and strikes.", FALSE, ch, 0, 0, TO_ROOM);
            AFF_FLAGS(ch).RemoveBit(AFF_APPROACH);
          } else {
            send_to_char(ch, "You attempt to close the distance!\r\n");
            act("$n charges towards $N.", FALSE, ch, 0, FIGHTING(ch), TO_ROOM);
            GET_INIT_ROLL(ch) -= 10;
            continue;
          }
        }
      }
      if (AFF_FLAGGED(ch, AFF_MANNING) || PLR_FLAGGED(ch, PLR_REMOTE) || AFF_FLAGGED(ch, AFF_RIG))
        mount_fire(ch);
      else if (FIGHTING_VEH(ch)) {
        if (ch->in_room != FIGHTING_VEH(ch)->in_room)
          stop_fighting(ch);
        else
          vcombat(ch, FIGHTING_VEH(ch));
      } else if ((!IS_NPC(ch) ||
                  (IS_NPC(ch) && !mob_magic(ch))) && !cycle_spell_q(ch))
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

      GET_INIT_ROLL(ch) -= 10;

      if (MOB_FLAGGED(ch, MOB_SPEC) &&
          mob_index[GET_MOB_RNUM(ch)].func != NULL)
        (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
    }
  }
}

void order_list(...)
{
  register struct char_data *one, *two = combat_list, *next, *previous = NULL, *temp;

  if (combat_list == NULL)
    return;
  for (one = combat_list; one; one = next) {
    next = one->next_fighting;
    two = one;
    if (GET_TRADITION(one) == TRAD_ADEPT && GET_POWER(one, ADEPT_QUICK_STRIKE)) {
      REMOVE_FROM_LIST(one, combat_list, next_fighting);
      one->next_fighting = combat_list;
      combat_list = one;
    }
  }

  for (one = two; one; previous = NULL, one = next) {
    next = one->next_fighting;
    for (two = combat_list; two && two->next_fighting; previous = two,
         two = two->next_fighting) {
      if (GET_INIT_ROLL(two->next_fighting) > GET_INIT_ROLL(two)) {
        if (previous)
          previous->next_fighting = two->next_fighting;
        else
          combat_list = two->next_fighting;
        temp = two->next_fighting->next_fighting;
        two->next_fighting->next_fighting = two;
        two->next_fighting = temp;
      }
    }
  }
}

void order_list(struct char_data *start)
{
  register struct char_data *one, *two, *next, *previous = NULL, *temp;

  for (one = start; one; previous = NULL, one = next)
  {
    next = one->next_fighting;
    for (two = start; two && two->next_fighting; previous = two,
         two = two->next_fighting) {
      if (GET_INIT_ROLL(two->next_fighting) > GET_INIT_ROLL(two)) {
        if (previous)
          previous->next_fighting = two->next_fighting;
        else
          start = two->next_fighting;
        temp = two->next_fighting->next_fighting;
        two->next_fighting->next_fighting = two;
        two->next_fighting = temp;
      }
    }
  }
}

void order_list(struct matrix_icon *start)
{
  register struct matrix_icon *one, *two, *next, *previous = NULL, *temp;

  for (one = start; one; previous = NULL, one = next)
  {
    next = one->next_fighting;
    for (two = start; two && two->next_fighting; previous = two,
         two = two->next_fighting) {
      if (two->next_fighting->initiative > two->initiative) {
        if (previous)
          previous->next_fighting = two->next_fighting;
        else
          start = two->next_fighting;
        temp = two->next_fighting->next_fighting;
        two->next_fighting->next_fighting = two;
        two->next_fighting = temp;
      }
    }
  }
  matrix[start->in_host].fighting = start;
}

void chkdmg(struct veh_data * veh)
{
  if (veh->damage <= 1)
  {
    send_to_veh("A scratch appears on the paintwork.\r\n", veh, NULL, TRUE);
  } else if (veh->damage <= 3)
  {
    send_to_veh("You see some dings and scratches appear on the paintwork.\r\n", veh, NULL, TRUE);
  } else if (veh->damage <= 6)
  {
    send_to_veh("The wind screen shatters and the bumper falls off.\r\n", veh, NULL, TRUE);
  } else if (veh->damage < 10)
  {
    send_to_veh("The engine starts spewing smoke and flames.\r\n", veh, NULL, TRUE);
  } else
  {
    struct char_data *i, *next;
    struct obj_data *obj;
    int dam;

    if (veh->cspeed >= SPEED_IDLE) {
      stop_chase(veh);
      send_to_veh("You are hurled into the street as your ride is wrecked!\r\n", veh, NULL, TRUE);
      sprintf(buf, "%s careens off the road, it's occupants hurled to the street.!\r\n", veh->short_description);
      send_to_room(buf, veh->in_room);
      veh->cspeed = SPEED_OFF;
      veh->dest = 0;
      if (veh->rigger) {
        send_to_char("Your mind is blasted with pain as your vehicle is wrecked.\r\n", veh->rigger);
        damage(veh->rigger, veh->rigger, convert_damage(stage(-success_test(GET_WIL(veh->rigger),
               6), SERIOUS)), TYPE_CRASH, MENTAL);
        veh->rigger->char_specials.rigging = NULL;
        PLR_FLAGS(veh->rigger).RemoveBit(PLR_REMOTE);
        veh->rigger = NULL;
      }
      for (i = veh->people; i; i = next) {
        next = i->next_in_veh;
        char_from_room(i);
        char_to_room(i, veh->in_room);
        if (AFF_FLAGGED(i, AFF_PILOT))
          AFF_FLAGS(i).RemoveBit(AFF_PILOT);
        if (AFF_FLAGGED(i, AFF_RIG))
          AFF_FLAGS(i).RemoveBit(AFF_RIG);
        if (AFF_FLAGGED(i, AFF_MANNING))
          AFF_FLAGS(i).RemoveBit(AFF_MANNING);
        dam = convert_damage(stage(-success_test(GET_BOD(i), 8), SERIOUS));
        damage(i, i, dam, TYPE_CRASH, PHYSICAL);
      }
      for (obj = veh->contents; obj; obj = obj->next_content)
        switch(number(0, 2)) {
        case 1:
          obj_from_room(obj);
          obj_to_room(obj, veh->in_room);
          break;
        case 2:
          extract_obj(obj);
          break;
        }
    } else {
      send_to_veh("You scramble into the street as your ride is wrecked!\r\n", veh, NULL, TRUE);
      struct char_data *next;
      for (i = veh->people; i; i = i->next) {
        next = i->next_in_veh;
        char_from_room(i);
        char_to_room(i, veh->in_room);
        dam = convert_damage(stage(-success_test(GET_BOD(i), 4), MODERATE));
        damage(i, i, dam, TYPE_CRASH, PHYSICAL);
      }
      for (obj = veh->contents; obj; obj = obj->next_content)
        switch(number(0, 2)) {
        case 1:
          obj_from_room(obj);
          obj_to_room(obj, veh->in_room);
          break;
        case 2:
          extract_obj(obj);
          break;
        }
    }
  }
  return;
}

void vram(struct veh_data * veh, struct char_data * ch, struct veh_data * tveh)
{
  int power, damage_total = 0, veh_dam = 0;
  int veh_resist = 0, ch_resist = 0, modbod = 0;

  if (ch)
  {
    power = (int)(get_speed(veh) / 10);

    if (GET_BOD(ch) < 8)
      modbod = 1;
    else
      modbod = 2;

    if (modbod > (veh->body * 2)) {
      damage_total = LIGHT;
      veh_dam = DEADLY;
    } else if (modbod > veh->body) {
      damage_total = MODERATE;
      veh_dam = SERIOUS;
    } else if (modbod == veh->body) {
      damage_total = SERIOUS;
      veh_dam = MODERATE;
    } else if (modbod < veh->body) {
      damage_total = DEADLY;
      veh_dam = LIGHT;
    }

    ch_resist = 0 - success_test(GET_BOD(ch), power);
    int staged_damage = stage(ch_resist, damage_total);
    damage_total = convert_damage(staged_damage);

    veh_resist = success_test(veh->body, power);
    veh_dam -= veh_resist;

    if (veh_dam > 0) {
      veh->damage += veh_dam;
      send_to_veh("THUMP!\r\n", veh, NULL, TRUE);
      sprintf(buf, "You run %s down!\r\n", thrdgenders[(int)GET_SEX(ch)]);
      sprintf(buf1, "%s runs $n down!", veh->short_description);
      sprintf(buf2, "%s runs you down!", veh->short_description);
      send_to_driver(buf, veh);
      chkdmg(veh);
    } else {
      send_to_veh("THUTHUMP!\r\n", veh, NULL, TRUE);
      sprintf(buf, "You roll right over %s!\r\n", thrdgenders[(int)GET_SEX(ch)]);
      sprintf(buf1, "%s rolls right over $n!", veh->short_description);
      sprintf(buf2, "%s runs right over you!", veh->short_description);
      send_to_driver(buf, veh);
    }
    act(buf1, FALSE, ch, 0, 0, TO_ROOM);
    act(buf2, FALSE, ch, 0, 0, TO_CHAR);
    damage(ch, ch, damage_total, TYPE_RAM, PHYSICAL);
  } else
  {
    power = veh->cspeed - tveh->cspeed;
    if (power < 0)
      power = -power;
    power = (int)(ceil((float)power / 10));

    if (tveh->body > (veh->body * 2)) {
      damage_total = LIGHT;
      veh_dam = DEADLY;
    } else if (tveh->body > veh->body) {
      damage_total = MODERATE;
      veh_dam = SERIOUS;
    } else if (tveh->body == veh->body) {
      damage_total = SERIOUS;
      veh_dam = MODERATE;
    } else if ((tveh->body * 2) < veh->body) {
      damage_total = DEADLY;
      veh_dam = LIGHT;
    } else if (tveh->body < veh->body) {
      damage_total = SERIOUS;
      veh_dam = MODERATE;
    }

    ch_resist = 0 - success_test(tveh->body, power);
    int staged_damage = stage(ch_resist, damage_total);
    damage_total = convert_damage(staged_damage);
    tveh->damage += damage_total;

    sprintf(buf, "A %s rams into you!\r\n", veh->short_description);
    send_to_veh(buf, tveh, NULL, TRUE);

    if (tveh->cspeed > SPEED_IDLE)
      switch (damage_total) {
      case MODERATE:
      case LIGHT:
        tveh->cspeed = SPEED_CRUISING;
        send_to_veh("You lose some speed.\r\n", tveh, NULL, TRUE);
        break;
      case SERIOUS:
        tveh->cspeed = SPEED_CRUISING;
        if (tveh->rigger)
          crash_test(tveh->rigger);
        else
          for (struct char_data *pilot = tveh->people; pilot; pilot = pilot->next_in_veh)
            if (AFF_FLAGGED(pilot, AFF_PILOT))
              crash_test(pilot);
        break;
      }
    chkdmg(tveh);

    veh_resist = 0 - success_test(veh->body, power);
    staged_damage = stage(veh_resist, veh_dam);
    damage_total = convert_damage(staged_damage);
    veh->damage += damage_total;

    sprintf(buf, "You ram a %s!\r\n", tveh->short_description);
    sprintf(buf1, "%s rams straight into your ride!\r\n", veh->short_description);
    sprintf(buf2, "%s rams straight into %s!\r\n", veh->short_description, tveh->short_description);
    send_to_veh(buf, veh, NULL, TRUE);
    send_to_veh(buf1, tveh, NULL, TRUE);
    send_to_room(buf2, veh->in_room);

    if (veh->cspeed > SPEED_IDLE)
      switch (damage_total) {
      case MODERATE:
      case LIGHT:
        veh->cspeed = SPEED_CRUISING;
        send_to_veh("You lose some speed.\r\n", tveh, NULL, TRUE);
        break;
      case SERIOUS:
        veh->cspeed = SPEED_CRUISING;
        if (veh->rigger)
          crash_test(veh->rigger);
        else
          for (struct char_data *pilot = veh->people; pilot; pilot = pilot->next_in_veh)
            if (AFF_FLAGGED(pilot, AFF_PILOT))
              crash_test(pilot);
        break;
      }
    chkdmg(veh);
  }
}

void vcombat(struct char_data * ch, struct veh_data * veh)
{
  char ammo_type[20];
  static struct obj_data *wielded, *obj;
  static int base_target, power, damage_total;

  int attack_success = 0, attack_resist=0, skill_total = 1;
  int recoil=0, burst=0, recoil_comp=0, newskill, modtarget = 0;

  if (IS_AFFECTED(ch, AFF_PETRIFY))
    return;
  if (veh->damage > 9)
  {
    stop_fighting(ch);
    return;
  }
  wielded = GET_EQ(ch, WEAR_WIELD);
  if (get_speed(veh) > 10 && !AFF_FLAGGED(ch, AFF_COUNTER_ATT) && ((!wielded || !IS_GUN(GET_OBJ_VAL(wielded, 3)))))
  {
    vram(veh, ch, NULL);
    return;
  }
  if (wielded)
  {
    switch(GET_OBJ_VAL(wielded, 3)) {
    case TYPE_SHOTGUN:
      sprintf(ammo_type, "horde of pellets");
      break;
    case TYPE_MACHINE_GUN:
      sprintf(ammo_type, "stream of bullets");
      break;
    case TYPE_CANNON:
      sprintf(ammo_type, "shell");
      break;
    case TYPE_ROCKET:
      sprintf(ammo_type, "rocket");
      break;
    default:
      sprintf(ammo_type, "bullet");
      break;
    }
    if (!has_ammo(ch, wielded->worn_on))
      return;
    if (IS_BURST(wielded)) {
      if (GET_OBJ_VAL(wielded, 6) >= 2) {
        burst = 3;
        GET_OBJ_VAL(wielded, 6) -= 2;
      } else if (GET_OBJ_VAL(wielded, 6) == 1) {
        burst = 2;
        GET_OBJ_VAL(wielded, 6)--;
      } else
        burst = 0;
    }

    if (IS_GUN(GET_OBJ_VAL(wielded, 3)))
      power = GET_OBJ_VAL(wielded, 0);
    else
      power = GET_STR(ch) + GET_OBJ_VAL(wielded, 2);
    damage_total = GET_OBJ_VAL(wielded, 1);
  } else
  {
    power = GET_STR(ch);
    strcpy(ammo_type, "blow");
    for (obj = ch->cyberware;
         obj && !damage_total;
         obj = obj->next_content)
      switch (GET_OBJ_VAL(obj, 2)) {
      case 19:
        damage_total = LIGHT;
        break;
      case 29:
        power += GET_OBJ_VAL(obj, 0);
      case 21:
        damage_total = MODERATE;
        break;
      }
    if (!damage_total)
      damage_total = MODERATE;
  }

  power = (int)(power / 2);
  damage_total--;
  if (power <= veh->armor || !damage_total)
  {
    sprintf(buf, "A %s ricochets off of %s.", ammo_type, veh->short_description);
    sprintf(buf2, "Your attack ricochets off of %s.", veh->short_description);
    act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
    act(buf2, FALSE, ch, NULL, NULL, TO_CHAR);
    sprintf(buf, "A %s ricochets off of your ride.\r\n", ammo_type);
    send_to_veh(buf, veh, 0, TRUE);
    return;
  } else
    power -= veh->armor;


  if (wielded)
    recoil_comp = check_recoil(ch, wielded);
  recoil = MAX(0, burst - recoil_comp);
  if (AFF_FLAGGED(ch, AFF_RIG) || AFF_FLAGGED(ch, AFF_MANNING))
    recoil = MAX(0, recoil - 2);

  if (get_speed(veh) > 10)
    if (get_speed(veh) < 60)
      modtarget++;
    else if (get_speed(veh) < 120)
      modtarget += 2;
    else if (get_speed(veh) < 200)
      modtarget += 2;
    else if (get_speed(veh) >= 200)
      modtarget += 3;

  if (wielded)
    modtarget -= check_smartlink(ch, wielded);
  if (wielded && IS_OBJ_STAT(wielded, ITEM_SNIPER) && ch->in_room == veh->in_room)
    modtarget += 6;

  if (GET_WIELDED(ch, 0) && GET_WIELDED(ch, 1))
    modtarget++;

  if (AFF_FLAGGED(ch, AFF_RIG) || AFF_FLAGGED(ch, AFF_MANNING))
  {
    if ((newskill = GET_SKILL(ch, SKILL_GUNNERY)) < 1)
      skill_total = reverse_web(ch, newskill, base_target);
  } else if (wielded && GET_SKILL(ch, GET_OBJ_VAL(wielded, 4)) < 1)
  {
    newskill = return_general(GET_OBJ_VAL(wielded, 4));
    if (GET_SKILL(ch, newskill) < 1)
      skill_total = reverse_web(ch, newskill, base_target);
    else
      skill_total = GET_SKILL(ch, newskill);
  } else if (!wielded)
  {
    if (GET_SKILL(ch, SKILL_UNARMED_COMBAT) < 1) {
      newskill = SKILL_UNARMED_COMBAT;
      skill_total = reverse_web(ch, newskill, base_target);
    } else
      skill_total = GET_SKILL(ch, SKILL_UNARMED_COMBAT);
  } else
    skill_total = GET_SKILL(ch, GET_OBJ_VAL(wielded, 4));

  base_target = 4 + modtarget + recoil;

  attack_success = success_test(skill_total, base_target);
  if (attack_success < 1)
  {
    sprintf(buf, "$n fires his $o at %s, but misses.", veh->short_description);
    sprintf(buf1, "You fire your $o at %s, but miss.", veh->short_description);
    sprintf(buf2, "%s's %s misses you completly.\r\n", GET_NAME(ch), ammo_type);
    act(buf, FALSE, ch, wielded, 0, TO_NOTVICT);
    act(buf1, FALSE, ch, wielded, 0, TO_CHAR);
    send_to_veh(buf2, veh, 0, TRUE);
    weapon_scatter(ch, ch, wielded);
    return;
  }
  attack_resist = success_test(veh->body, power + modify_veh(veh));
  attack_success -= attack_resist;

  int staged_damage = stage(attack_success, damage_total);
  damage_total = convert_damage(staged_damage);

  if (damage_total < LIGHT)
  {
    sprintf(buf, "$n's %s ricochets off of %s.", ammo_type, veh->short_description);
    sprintf(buf1, "Your attack ricochets off of %s.", veh->short_description);
    sprintf(buf2, "A %s ricochets off of your ride.\r\n", ammo_type);
  } else if (damage_total == LIGHT)
  {
    sprintf(buf, "$n's %s causes extensive damage to %s paintwork.", ammo_type, veh->short_description);
    sprintf(buf1, "Your attack causes extensive damage to %s paintwork.", veh->short_description);
    sprintf(buf2, "A %s scratches your paintjob.\r\n", ammo_type);
  } else if (damage_total == MODERATE)
  {
    sprintf(buf, "$n's %s leaves %s riddled with holes.", ammo_type, veh->short_description);
    sprintf(buf1, "Your attack leave %s riddled with holes.", veh->short_description);
    sprintf(buf2, "A %s leaves your ride full of holes.\r\n", ammo_type);
  } else if (damage_total == SERIOUS)
  {
    sprintf(buf, "$n's %s obliterates %s.", ammo_type, veh->short_description);
    sprintf(buf1, "You obliterate %s with your attack.", veh->short_description);
    sprintf(buf2, "A %s obliterates your ride.\r\n", ammo_type);
  } else if (damage_total >= DEADLY)
  {
    sprintf(buf, "$n's %s completly destroys %s.", ammo_type, veh->short_description);
    sprintf(buf1, "Your attack completly destroys %s.", veh->short_description);
    sprintf(buf2, "A %s completly destroys your ride.\r\n", ammo_type);
  }

  act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
  act(buf1, FALSE, ch, NULL, NULL, TO_CHAR);
  send_to_veh(buf2, veh, 0, TRUE);

  veh->damage += damage_total;
  if (veh->owner && !IS_NPC(ch))
  {
    sprintf(buf, "%s attacked vehicle (%s) owned by player.\r\n", GET_CHAR_NAME(ch), veh->name);
    mudlog(buf, ch, LOG_WRECKLOG, TRUE);
  }
  chkdmg(veh);
}

void mount_fire(struct char_data *ch)
{
  struct veh_data *veh;
  struct obj_data *mount, *temp;
  RIG_VEH(ch, veh);
  if (AFF_FLAGGED(ch, AFF_MANNING))
  {
    for (mount = veh->mount; mount; mount = mount->next_content)
      if (mount->worn_by == ch)
        break;
    if (!(mount->targ || mount->tveh))
      return;
    temp = GET_EQ(ch, WEAR_WIELD);
    GET_EQ(ch, WEAR_WIELD) = mount->contains;
    GET_WIELDED(ch, 0) = 1;
    mount->contains->worn_on = WEAR_WIELD;
    if (mount->targ)
      hit(ch, mount->targ, TYPE_UNDEFINED);
    else
      vcombat(ch, mount->tveh);
    if (temp) {
      GET_EQ(ch, WEAR_WIELD) = temp;
    } else {
      GET_EQ(ch, WEAR_WIELD) = NULL;
      GET_WIELDED(ch, 0) = 0;
    }
    mount->contains->worn_on = NOWHERE;
  } else if (ch->char_specials.rigging || AFF_FLAGGED(ch, AFF_RIG))
  {
    for (mount = veh->mount; mount; mount = mount->next_content)
      if ((!mount->worn_by && (mount->targ || mount->tveh) && (FIGHTING(ch) || FIGHTING_VEH(ch))) && mount->contains) {
        temp = GET_EQ(ch, WEAR_WIELD);
        GET_EQ(ch, WEAR_WIELD) = mount->contains;
        GET_WIELDED(ch, 0) = 1;
        mount->contains->worn_on = WEAR_WIELD;
        if (mount->targ)
          hit(ch, mount->targ, TYPE_UNDEFINED);
        else
          vcombat(ch, mount->tveh);
        if (temp) {
          GET_EQ(ch, WEAR_WIELD) = temp;
        } else {
          GET_EQ(ch, WEAR_WIELD) = NULL;
          GET_WIELDED(ch, 0) = 0;
        }
        mount->contains->worn_on = NOWHERE;
      }
  }
}


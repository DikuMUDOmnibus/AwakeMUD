/* ************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
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

#include "structs.h"
#include "awake.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "screen.h"
#include "olc.h"
#include "spells.h"
#include "constants.h"

/* External variables */
extern struct time_info_data time_info;

extern char *cleanup(char *dest, const char *src);
extern struct obj_data *get_first_credstick(struct char_data *ch, char *arg);
extern void reduce_abilities(struct char_data *vict);
extern void free_shop(struct shop_data *shop);
extern bool resize_shp_array(void);
extern int olc_state;

/* Forward/External function declarations */
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
ACMD(do_treat);
char *fname(char *namelist);
void sort_keeper_objs(struct char_data * keeper, int shop_nr);
void regen_amount_shopkeeper(int shop_nr, struct char_data * keeper);

extern long mud_boot_time;

/* Local variables */
int cmd_say, cmd_tell, cmd_emote, cmd_slap;

/* Constant list for printing out who we sell to */
const char *trade_letters[] =
  {
    "Human",
    "Elf",
    "Dwarf",
    "Ork",
    "Troll",
    "Dragon",
    "\n"
  };

const char *shop_bits[] =
  {
    "WILL_FIGHT",
    "USES_BANK",
    "DOCTOR",
    "!NEGOTIATE",
    "CASH_ONLY",
    "CREDS_ONLY",
    "\n"
  };

// this determines whether the character may be sold to or not
int is_ok_char(struct char_data * keeper, struct char_data * ch, int shop_nr)
{
  char buf[200];

  if (!access_level(ch, LVL_ADMIN)
      && !(CAN_SEE(keeper, ch)))
  {
    do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
    return (FALSE);
  }

  if (IS_NPC(ch) || access_level(ch, LVL_BUILDER))
    return (TRUE);

  if ((GET_RACE(ch) == RACE_HUMAN && NOTRADE_HUMAN(shop_nr)) ||
      (GET_RACE(ch) == RACE_ELF && NOTRADE_ELF(shop_nr)) ||
      (GET_RACE(ch) == RACE_WAKYAMBI && NOTRADE_ELF(shop_nr)) ||
      (GET_RACE(ch) == RACE_NIGHTONE && NOTRADE_ELF(shop_nr)) ||
      (GET_RACE(ch) == RACE_DRYAD && NOTRADE_ELF(shop_nr)) ||
      (GET_RACE(ch) == RACE_DWARF && NOTRADE_DWARF(shop_nr)) ||
      (GET_RACE(ch) == RACE_KOBOROKURU && NOTRADE_DWARF(shop_nr)) ||
      (GET_RACE(ch) == RACE_MENEHUNE && NOTRADE_DWARF(shop_nr)) ||
      (GET_RACE(ch) == RACE_GNOME && NOTRADE_DWARF(shop_nr)) ||
      (GET_RACE(ch) == RACE_ORK && NOTRADE_ORK(shop_nr)) ||
      (GET_RACE(ch) == RACE_ONI && NOTRADE_ORK(shop_nr)) ||
      (GET_RACE(ch) == RACE_SATYR && NOTRADE_ORK(shop_nr)) ||
      (GET_RACE(ch) == RACE_OGRE && NOTRADE_ORK(shop_nr)) ||
      (GET_RACE(ch) == RACE_HOBGOBLIN && NOTRADE_ORK(shop_nr)) ||
      (GET_RACE(ch) == RACE_TROLL && NOTRADE_TROLL(shop_nr)) ||
      (GET_RACE(ch) == RACE_CYCLOPS && NOTRADE_TROLL(shop_nr)) ||
      (GET_RACE(ch) == RACE_GIANT && NOTRADE_TROLL(shop_nr)) ||
      (GET_RACE(ch) == RACE_MINOTAUR && NOTRADE_TROLL(shop_nr)) ||
      (GET_RACE(ch) == RACE_FOMORI && NOTRADE_TROLL(shop_nr)) ||
      (GET_RACE(ch) == RACE_DRAGON && NOTRADE_DRAGON(shop_nr)))
  {
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), MSG_NO_SELL);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return (FALSE);
  }
  return (TRUE);
}

// this determines opening and closing times
int is_open(struct char_data * keeper, int shop_nr, int msg)
{
  char buf[200];

  *buf = 0;
  if (SHOP_OPEN1(shop_nr) > time_info.hours)
    strcpy(buf, MSG_NOT_OPEN_YET);
  else if (SHOP_CLOSE1(shop_nr) < time_info.hours)
    if (SHOP_OPEN2(shop_nr) > time_info.hours)
      strcpy(buf, MSG_NOT_REOPEN_YET);
    else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
      strcpy(buf, MSG_CLOSED_FOR_DAY);

  if (!(*buf))
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);
  return (FALSE);
}

// combines a check if shop is open and can sell to char
int is_ok(struct char_data * keeper, struct char_data * ch, int shop_nr)
{
  if (access_level(ch, LVL_ADMIN)
      || is_open(keeper, shop_nr, TRUE))
    return (is_ok_char(keeper, ch, shop_nr));
  else
    return (FALSE);
}

// makes sure item is okay to trade with
int trade_with(struct obj_data * item, int shop_nr)
{
  if (GET_OBJ_COST(item) < 1)
    return (OBJECT_NOTOK);

  if (IS_OBJ_STAT(item, ITEM_NOSELL) || !shop_table[shop_nr].type)
    return (OBJECT_NOTOK);

  if (GET_OBJ_TYPE(item) == ITEM_DECK_ACCESSORY && GET_OBJ_VAL(item, 0) == TYPE_FILE)
    return (OBJECT_NOTOK);
  for (int counter = 0; counter < shop_table[shop_nr].num_buy_types; counter++)
    if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item))
      return (OBJECT_OK);
  return (OBJECT_NOTOK);
}

int same_obj(struct obj_data * obj1, struct obj_data * obj2)
{
  int index;

  if (!obj1 || !obj2)
    return (obj1 == obj2);

  if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
    return (FALSE);

  if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
    return (FALSE);

  if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
    return (FALSE);

  for (index = 0; index < MAX_OBJ_AFFECT; index++)
    if ((obj1->affected[index].location != obj2->affected[index].location) ||
        (obj1->affected[index].modifier != obj2->affected[index].modifier))
      return (FALSE);

  return (TRUE);
}

int shop_producing(struct obj_data * item, int shop_nr)
{
  int counter;

  if (GET_OBJ_RNUM(item) < 0 || !shop_table[shop_nr].producing)
    return (FALSE);

  for (counter = 0; counter < shop_table[shop_nr].num_producing; counter++)
    if (same_obj(item, &obj_proto[real_object(SHOP_PRODUCT(shop_nr, counter))]))
      return (TRUE);
  return (FALSE);
}

int transaction_amt(char *arg)
{
  int num;

  one_argument(arg, buf);
  if (*buf)
    if ((is_number(buf))) {
      num = atoi(buf);
      strcpy(arg, arg + strlen(buf) + 1);
      return (num);
    }
  return (1);
}

// how many of a particular object were purchased
char *times_message(struct obj_data * obj, char *name, int num)
{
  static char buf[256];
  const char *ptr;

  if (obj)
    strcpy(buf, obj->text.name);
  else
  {
    if ((ptr = strchr((const char *)name, '.')) == NULL)
      ptr = name;
    else
      ptr++;
    sprintf(buf, "%s %s", AN(ptr), ptr);
  }

  if (num > 1)
    sprintf(END_OF(buf), " (x %d)", num);
  return (buf);
}

struct obj_data *get_slide_obj_vis(struct char_data * ch, char *name,
                                         struct obj_data * list)
{
  struct obj_data *i, *last_match = 0;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(number = get_number(&tmp)))
    return (0);

  for (i = list, j = 1; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->text.keywords))
      if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i))
      {
        if (j == number)
          return (i);
        last_match = i;
        j++;
      }
  return (0);
}

struct obj_data *get_hash_obj_vis(struct char_data * ch, char *name,
                                        struct obj_data * list)
{
  struct obj_data *loop, *last_obj = 0;
  int index;

  if ((is_number(name + 1)))
    index = atoi(name + 1);
  else
    return (0);

  for (loop = list; loop; loop = loop->next_content)
    if (CAN_SEE_OBJ(ch, loop) && (loop->obj_flags.cost > 0))
      if (!same_obj(last_obj, loop))
      {
        if (--index == 0)
          return (loop);
        last_obj = loop;
      }
  return (0);
}

struct obj_data *get_purchase_obj(struct char_data * ch, char *arg,
                                        struct char_data * keeper, int shop_nr, int msg)
{
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(arg, name);
  do
  {
    if (*name == '#')
      obj = get_hash_obj_vis(ch, name, keeper->carrying);
    else
      obj = get_slide_obj_vis(ch, name, keeper->carrying);
    if (!obj) {
      if (msg  && !(IS_DOCTOR(shop_nr) && !str_cmp(arg, "treatment"))) {
        sprintf(buf, "%s %s", GET_CHAR_NAME(ch), shop_table[shop_nr].no_such_item1);
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
      }
      return (0);
    }
    if (GET_OBJ_COST(obj) <= 0) {
      extract_obj(obj);
      obj = 0;
    }
  } while (!obj);
  return (obj);
}

int buy_price(struct obj_data * obj, int shop_nr)
{
  int i;

  i = (int)(GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr));
  i = i + (int)((i * SHOP_CURCENT(shop_nr)) / 100);

  return i;
}

void shopping_info(char *arg, struct char_data *ch, struct char_data *keeper, vnum_t shop_nr)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  struct obj_data *obj;
  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  skip_spaces(&arg);
  if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
    return;
  sprintf(buf, "%s %s is", GET_CHAR_NAME(ch), CAP(obj->text.name));
  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_WEAPON:
    if (IS_GUN(GET_OBJ_VAL(obj, 3))) {
      if (GET_OBJ_VAL(obj, 0) < 3)
        strcat(buf, " a weak");
      else if (GET_OBJ_VAL(obj, 0) < 6)
        strcat(buf, " a low powered");
      else if (GET_OBJ_VAL(obj, 0) < 10)
        strcat(buf, " a moderately powered");
      else if (GET_OBJ_VAL(obj, 0) < 12)
        strcat(buf, " a strong");
      else
        strcat(buf, " a high powered");
      if (IS_OBJ_STAT(obj, ITEM_SNIPER))
        strcat(buf, " snipers rifle");
      else {
        strncpy(buf2, skills[GET_OBJ_VAL(obj, 4)].name, strlen(skills[GET_OBJ_VAL(obj, 4)].name) - 1);
        buf2[strlen(skills[GET_OBJ_VAL(obj, 4)].name) - 1] = '\0';
        sprintf(buf + strlen(buf), " %s", buf2);
      }
      if (GET_OBJ_VAL(obj, 3) == TYPE_MACHINE_GUN) {
        strcat(buf, " that operates in burst fire mode");
        if (IS_OBJ_STAT(obj, ITEM_TWOHANDS))
          strcat(buf, " and requires two hands to wield correctly");
      } else if (IS_OBJ_STAT(obj, ITEM_TWOHANDS))
        strcat(buf, " that requires two hands to wield correctly");
      if (GET_OBJ_VAL(obj, 7) > 0 || GET_OBJ_VAL(obj, 8) > 0 || GET_OBJ_VAL(obj, 9) > 0)
        strcat(buf, ". It comes standard with ");
      if (real_object(GET_OBJ_VAL(obj, 7)) > 0) {
        strcat(buf, obj_proto[real_object(GET_OBJ_VAL(obj, 7))].text.name);
        if ((GET_OBJ_VAL(obj, 8) > 0 && GET_OBJ_VAL(obj, 9) < 1) || (GET_OBJ_VAL(obj, 8) < 1 && GET_OBJ_VAL(obj, 9) > 0))
          strcat(buf, " and ");
        if (GET_OBJ_VAL(obj, 8) > 0 && GET_OBJ_VAL(obj, 9) > 0)
          strcat(buf, ", ");
      }
      if (real_object(GET_OBJ_VAL(obj, 8)) > 0) {
        strcat(buf, obj_proto[real_object(GET_OBJ_VAL(obj, 8))].text.name);
        if (GET_OBJ_VAL(obj, 9) > 0)
          strcat(buf, " and ");
      }
      if (real_object(GET_OBJ_VAL(obj, 9)) > 0)
        strcat(buf, obj_proto[real_object(GET_OBJ_VAL(obj, 9))].text.name);
      sprintf(buf + strlen(buf), ". It can hold a maximum of %d rounds.", GET_OBJ_VAL(obj, 5));
    } else {}
    break;
  case ITEM_WORN:
    if (GET_OBJ_VAL(obj, 5) + GET_OBJ_VAL(obj, 6) < 1)
      strcat(buf, " a piece of clothing");
    else if (GET_OBJ_VAL(obj, 5) + GET_OBJ_VAL(obj, 6) < 4)
      strcat(buf, " a lightly armoured piece of clothing");
    else if (GET_OBJ_VAL(obj, 5) + GET_OBJ_VAL(obj, 6) < 7)
      strcat(buf, " a piece of light armour");
    else if (GET_OBJ_VAL(obj, 5) + GET_OBJ_VAL(obj, 6) < 10)
      strcat(buf, " a moderately rated piece of armour");
    else
      strcat(buf, " a piece of heavy armour");
    if (GET_OBJ_VAL(obj, 1) > 5)
      strcat(buf, " designed for carrying ammunition");
    else if (GET_OBJ_VAL(obj, 4) > 3 && GET_OBJ_VAL(obj, 4) < 6)
      strcat(buf, " that can carry a bit of gear");
    else if (GET_OBJ_VAL(obj, 4) >= 6)
      strcat(buf, " that can carry alot of gear");
    if (GET_OBJ_VAL(obj, 7) < -2)
      strcat(buf, ". It is also very bulky.");
    else if (GET_OBJ_VAL(obj, 7) < 1)
      strcat(buf, ". It is easier to see under clothing.");
    else if (GET_OBJ_VAL(obj, 7) < 4)
      strcat(buf, ". It is quite concealable.");
    else
      strcat(buf, ". It's almost invisible under clothing.");
    break;
  case ITEM_CYBERDECK:
    if (GET_OBJ_VAL(obj, 0) < 4)
      strcat(buf, " a beginners cyberdeck");
    else if (GET_OBJ_VAL(obj, 0) < 9)
      strcat(buf, " a cyberdeck of moderate ability");
    else if (GET_OBJ_VAL(obj, 0) < 12)
      strcat(buf, " a top of the range cyberdeck");
    else
      strcat(buf, " one of the best cyberdecks you'll ever see");
    if (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 3) < 600)
      strcat(buf, " with a modest amount of memory.");
    else if (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 3) < 1400)
      strcat(buf, " containing a satisfactory amount of memory.");
    else if (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 3) < 3000)
      strcat(buf, " featuring a fair amount of memory.");
    else if (GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(obj, 3) < 5000)
      strcat(buf, " with oodles of memory.");
    else
      strcat(buf, " with more memory than you could shake a datajack at.");
    if (GET_OBJ_VAL(obj, 1) < 2)
      strcat(buf, " You better hope you don't run into anything nasty while using it");
    else if (GET_OBJ_VAL(obj, 1) < 5)
      strcat(buf, " It offers adequate protection from feedback");
    else if (GET_OBJ_VAL(obj, 1) < 9)
      strcat(buf, " Nothing will phase you");
    else
      strcat(buf, " It could protect you from anything");
    if (GET_OBJ_VAL(obj, 4) < 100)
      strcat(buf, " but you're out of luck if you want to transfer anything.");
    else if (GET_OBJ_VAL(obj, 4) < 200)
      strcat(buf, " and it transfers slowly, but will get the job done.");
    else if (GET_OBJ_VAL(obj, 4) < 300)
      strcat(buf, " and on the plus side it's IO is excellent.");
    else if (GET_OBJ_VAL(obj, 4) < 500)
      strcat(buf, " also the IO is second to none.");
    else
      strcat(buf, " and it can upload faster than light.");
    break;
  case ITEM_FOOD:
    if (GET_OBJ_VAL(obj, 0) < 2)
      strcat(buf, " a small");
    else if (GET_OBJ_VAL(obj, 0) < 5)
      strcat(buf, " a average");
    else if (GET_OBJ_VAL(obj, 0) < 10)
      strcat(buf, " a large");
    else
      strcat(buf, " a huge");
    strcat(buf, " portion of food.");
    break;
  case ITEM_DOCWAGON:
    strcat(buf, " a docwagon contract, it will call them out when your vital signs drop.");
    break;
  case ITEM_CONTAINER:
    if (GET_OBJ_VAL(obj, 0) < 5)
      strcat(buf, " tiny");
    else if (GET_OBJ_VAL(obj, 0) < 15)
      strcat(buf, " small");
    else if (GET_OBJ_VAL(obj, 0) < 30)
      strcat(buf, " large");
    else if (GET_OBJ_VAL(obj, 0) < 60)
      strcat(buf, " huge");
    else
      strcat(buf, " gigantic");
    if (obj->obj_flags.wear_flags.AreAnySet(ITEM_WEAR_BACK, ITEM_WEAR_ABOUT, ITEM_WEAR_ABOUT, ENDBIT))
      strcat(buf, " backpack.");
    else
      strcat(buf, " piece of furniture");
    break;
  default:
    sprintf(buf, "%s I don't know anything about that.", GET_CHAR_NAME(ch));
  }
  if (!WONT_NEGOTIATE(shop_nr))
    sprintf(buf + strlen(buf), " How does %d sound?", buy_price(obj, shop_nr));
  else
    sprintf(buf + strlen(buf), " I couldn't let it go for less than %d nuyen.", buy_price(obj, shop_nr));
  do_say(keeper, buf, cmd_say, SCMD_SAYTO);
}

void shopping_buy(char *arg, struct char_data * ch, struct char_data * keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  struct obj_data *obj, *last_obj = NULL, *credstick = NULL, *check;
  int goldamt = 0, buynum, bought = 0, price, cash = 0;

  regen_amount_shopkeeper(shop_nr, keeper);

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  skip_spaces(&arg);
  any_one_arg(arg, buf);

  if (!str_cmp(buf, "cash"))
  {
    arg = any_one_arg(arg, buf);
    skip_spaces(&arg);
    cash = 1;
  } else if (!(credstick = get_first_credstick(ch, "credstick")))
  {
    send_to_char("Lacking an activated credstick, you choose to deal in cash.\r\n", ch );
    cash = 1;
  }

  if ((buynum = transaction_amt(arg)) < 0)
  {
    sprintf(buf, "%s A negative amount?  Try selling me something.", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }
  if (!(*arg) || !(buynum))
  {
    sprintf(buf, "%s What do you want to buy?", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }

  if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
  {
    if (!str_cmp(arg, "treatment") && IS_DOCTOR(shop_nr)) {
      if (ch == keeper)
        return;
      if (IS_CASH_ONLY(shop_nr) && !cash) {
        sprintf(buf, "%s I only deal in cash!", GET_CHAR_NAME(ch));
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
        return;
      } else if (IS_CRED_ONLY(shop_nr) && cash) {
        sprintf(buf, "%s I don't accept cash!", GET_CHAR_NAME(ch));
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
        return;
      }
      if (FIGHTING(keeper) || !GET_KEYWORDS(ch) ||
          FIGHTING(ch) || LAST_HEAL(ch)) {
        sprintf(buf, "%s I can't treat you at this time.", GET_CHAR_NAME(ch));
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
      } else if ((cash ? GET_NUYEN(ch) : GET_OBJ_VAL(credstick, 0)) < 200 &&
                 !IS_GOD(ch)) {
        sprintf(buf, "%s %s", GET_CHAR_NAME(ch), shop_table[shop_nr].missing_cash2);
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
        switch (SHOP_TEMPER(shop_nr)) {
        case 0:
          do_echo(keeper, "waits for some money.", cmd_emote, SCMD_EMOTE);
          return;
        case 1:
          do_echo(keeper, "doesn't take sexual favours as payment.", cmd_emote, SCMD_EMOTE);
          return;
        }
      } else {
        if (!IS_GOD(ch)) {
          if (cash) {
            GET_NUYEN(ch) -= 200;
            GET_NUYEN(keeper) += 200;
          } else {
            SHOP_BANK(shop_nr) += 200;
            GET_OBJ_VAL(credstick, 0) -= 200;
          }
        }
        do_treat(keeper, fname(GET_KEYWORDS(ch)), 0, 1);
      }
    }
    return;
  }

  if (IS_CASH_ONLY(shop_nr) && !cash)
  {
    sprintf(buf, "%s We only accept cash here!", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  } else if (IS_CRED_ONLY(shop_nr) && cash)
  {
    sprintf(buf, "%s We don't accept cash!", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }

  price = buy_price(obj, shop_nr);
  if (!IS_DOCTOR(shop_nr) && !WONT_NEGOTIATE(shop_nr))
    price = negotiate(ch, keeper, 0, price, 0, TRUE);

  if ((price > (cash ? GET_NUYEN(ch) : GET_OBJ_VAL(credstick, 0))) && !IS_GOD(ch))
  {
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), shop_table[shop_nr].missing_cash2);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);

    switch (SHOP_TEMPER(shop_nr)) {
    case 0:
      do_echo(keeper, "waits for some money.", cmd_emote, SCMD_EMOTE);
      return;
    case 1:
      do_echo(keeper, "doesn't take sexual favours as payment.", cmd_emote, SCMD_EMOTE);
      return;
    default:
      return;
    }
  }

  if (!from_ip_zone(GET_OBJ_VNUM(obj)) && from_ip_zone(SHOP_NUM(shop_nr)))
  {
    sprintf(buf, "%s We're not yet licensed to sell that product.", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }

  if (IS_DOCTOR(shop_nr))
  {
    if (buynum != 1) {
      sprintf(buf, "%s You can only have one of those installed!", GET_CHAR_NAME(ch));
      do_say(keeper, buf, cmd_say, SCMD_SAYTO);
      return;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE) {
      if (ch->real_abils.ess < (GET_TOTEM(ch) == TOTEM_EAGLE ?
                                GET_OBJ_VAL(obj, 1) << 1 : GET_OBJ_VAL(obj, 1))) {
        printf(buf, "%s That operation would kill you!", GET_CHAR_NAME(ch));
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
        return;
      }
      for (check = ch->cyberware; check != NULL; check = check->next_content) {
        if ((GET_OBJ_VNUM(check) == GET_OBJ_VNUM(obj))) {
          sprintf(buf, "%s You already have that installed.", GET_CHAR_NAME(ch));
          do_say(keeper, buf, cmd_say, SCMD_SAYTO);
          return;
        }
        if (GET_OBJ_VAL(check, 2) == GET_OBJ_VAL(obj, 2)) {
          printf(buf, "%s You already have a similar piece of cyberware.", GET_CHAR_NAME(ch));
          do_say(keeper, buf, cmd_say, SCMD_SAYTO);
          return;
        }
      }
      if (GET_OBJ_VAL(obj, 2) == 23 || GET_OBJ_VAL(obj, 2) == 30 || GET_OBJ_VAL(obj, 2) == 20)
        for (check = ch->bioware; check; check = check->next_content) {
          if (GET_OBJ_VAL(check, 2) == 2 && GET_OBJ_VAL(obj, 2) == 23) {
            sprintf(buf, "%s %s is not compatible with orthoskin.",
                    GET_CHAR_NAME(ch), obj->text.name);
            do_say(keeper, buf, cmd_say, SCMD_SAYTO);
            return;
          }
          if (GET_OBJ_VAL(check, 2) == 8 && GET_OBJ_VAL(obj, 2) == 30) {
            sprintf(buf, "%s %s is not compatible with synaptic accelerators.",
                    GET_CHAR_NAME(ch), obj->text.name);
            do_say(keeper, buf, cmd_say, SCMD_SAYTO);
            return;
          }
          if (GET_OBJ_VAL(check, 2) == 10 && GET_OBJ_VAL(obj, 2) == 20) {
            sprintf(buf, "%s %s is not compatible with muscle augmentation.",
                    GET_CHAR_NAME(ch), obj->text.name);
            do_say(keeper, buf, cmd_say, SCMD_SAYTO);
            return;
          }
        }
      if (shop_producing(obj, shop_nr))
        obj = read_object(GET_OBJ_RNUM(obj), REAL);
      else {
        obj_from_char(obj);
        SHOP_SORT(shop_nr)--;
      }
      if (GET_TRADITION(ch) == TRAD_ADEPT)
        GET_PP(ch) -= 100;
      obj_to_cyberware(obj, ch);
    } else if (GET_OBJ_TYPE(obj) == ITEM_BIOWARE) {
      if (GET_INDEX(ch) < GET_OBJ_VAL(obj, 1)) {
        sprintf(buf, "%s That operation would kill you!", GET_CHAR_NAME(ch));
        do_say(keeper, buf, cmd_say, SCMD_SAYTO);
        return;
      }
      for (check = ch->bioware; check; check = check->next_content) {
        if ((GET_OBJ_VNUM(check) == GET_OBJ_VNUM(obj))) {
          sprintf(buf, "%s You already have that installed.", GET_CHAR_NAME(ch));
          do_say(keeper, buf, cmd_say, SCMD_SAYTO);
          return;
        }
        if (GET_OBJ_VAL(check, 2) == GET_OBJ_VAL(obj, 2)) {
          sprintf(buf, "%s You already have a similar piece of bioware.", GET_CHAR_NAME(ch));
          do_say(keeper, buf, cmd_say, SCMD_SAYTO);
          return;
        }
      }
      if (GET_OBJ_VAL(obj, 2) == 2 || GET_OBJ_VAL(obj, 2) == 8 || GET_OBJ_VAL(obj, 2) == 10)
        for (check = ch->cyberware; check; check = check->next_content) {
          if (GET_OBJ_VAL(check, 2) == 23 && GET_OBJ_VAL(obj, 2) == 2) {
            sprintf(buf, "%s Orthoskin is not compatible with any form of dermal plating.", GET_CHAR_NAME(ch));
            do_say(keeper, buf, cmd_say, SCMD_SAYTO);
            return;
          }
          if (GET_OBJ_VAL(check, 2) == 20 && GET_OBJ_VAL(obj, 2) == 10) {
            sprintf(buf, "%s Muscle augmentation is not compatible with muscle replacement.", GET_CHAR_NAME(ch));
            do_say(keeper, buf, cmd_say, SCMD_SAYTO);
            return;
          }
        }
      if (shop_producing(obj, shop_nr))
        obj = read_object(GET_OBJ_RNUM(obj), REAL);
      else {
        obj_from_char(obj);
        SHOP_SORT(shop_nr)--;
      }
      if (GET_OBJ_VAL(obj, 2) == 0) {
        GET_OBJ_VAL(obj, 5) = 12;
        GET_OBJ_VAL(obj, 6) = 0;
      }
      if (GET_TRADITION(ch) == TRAD_ADEPT)
        GET_PP(ch) -= 100;
      obj_to_bioware(obj, ch);
    } else {
      sprintf(buf, "%s That's not something that should be implanted into the body!", GET_CHAR_NAME(ch));
      do_say(keeper, buf, cmd_say, SCMD_SAYTO);
      return;
    }
    if (!IS_GOD(ch)) {
      if (cash) {
        GET_NUYEN(ch) -= price;
        GET_NUYEN(keeper) += price;
      } else {
        SHOP_BANK(shop_nr) += price;
        GET_OBJ_VAL(credstick, 0) -= price;
      }
    }
    act("$n takes out a a sharpened scalpel and lies $N down on the operating table.",
        FALSE, keeper, 0, ch, TO_NOTVICT);
    sprintf(arg, shop_table[shop_nr].message_buy, price);
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), arg);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    sprintf(buf, "%s Relax...this won't hurt a bit.", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    GET_PHYSICAL(ch) = 100;
    GET_MENTAL(ch) = 0;
    act("You delicately install $p into $N's body.",
        FALSE, keeper, obj, ch, TO_CHAR);
    act("$n performs a delicate procedure on $N.",
        FALSE, keeper, 0, ch, TO_NOTVICT);
    act("$n delicately installs $p into your body.",
        FALSE, keeper, obj, ch, TO_VICT);
    return;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE || GET_OBJ_TYPE(obj) == ITEM_BIOWARE)
  {
    sprintf(buf, "%s You'd better ask a doctor about that!", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }
  if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)))
  {
    sprintf(buf, "%s: You can't carry any more items.\r\n", obj->text.name);
    send_to_char(buf, ch);
    return;
  }
  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
  {
    sprintf(buf, "%s: You can't carry that much weight.\r\n", obj->text.name);
    send_to_char(buf, ch);
    return;
  }

  while ((obj) && (((cash ? GET_NUYEN(ch) : GET_OBJ_VAL(credstick, 0)) >= buy_price(obj, shop_nr)) ||
                   IS_GOD(ch)) && (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)) && (bought < buynum) &&
         (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)))
  {
    bought++;
    /* Test if producing shop ! */
    if (shop_producing(obj, shop_nr))
      obj = read_object(GET_OBJ_RNUM(obj), REAL);
    else {
      obj_from_char(obj);
      SHOP_SORT(shop_nr)--;
    }
    obj_to_char(obj, ch);

    goldamt += price;
    if (!IS_GOD(ch)) {
      if (cash)
        GET_NUYEN(ch) -= price;
      else
        GET_OBJ_VAL(credstick, 0) -= price;
    }

    last_obj = obj;
    obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
    if (!same_obj(obj, last_obj))
      break;
  }
  if (bought < 1)
  {
    sprintf(buf, "$n stares off into the distance.");
    act(buf, FALSE, keeper, 0, 0, TO_ROOM);
    return;
  }

  if (bought < buynum)
  {
    if (!obj || !same_obj(last_obj, obj))
      sprintf(arg, "%s I only have %d to sell you.", GET_CHAR_NAME(ch), bought);
    else if ((cash ? GET_NUYEN(ch) : GET_OBJ_VAL(credstick, 0)) < buy_price(obj, shop_nr))
      sprintf(arg, "%s You can only afford %d.", GET_CHAR_NAME(ch), bought);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      sprintf(arg, "%s You can only hold %d.", GET_CHAR_NAME(ch), bought);
    else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
      sprintf(arg, "%s You can only carry %d.", GET_CHAR_NAME(ch), bought);
    else
      sprintf(arg, "%s Something screwy only gave you %d.", GET_CHAR_NAME(ch), bought);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
  }

  // this only gives nuyen to the shopkeeper if a god is not selling
  if (!IS_GOD(ch))
  {
    if (cash)
      GET_NUYEN(keeper) += goldamt;
    else
      SHOP_BANK(shop_nr) += goldamt;
  }

  strcpy(buf2, last_obj->text.name);
  if (bought > 1)
    sprintf(buf2, "%s (x %d)", buf2, bought);
  sprintf(buf, "$n buys %s.", buf2);
  act(buf, FALSE, ch, obj, 0, TO_ROOM);

  sprintf(arg, shop_table[shop_nr].message_buy, goldamt);
  sprintf(buf, "%s %s", GET_CHAR_NAME(ch), arg);
  do_say(keeper, buf, cmd_say, SCMD_SAYTO);
  sprintf(buf, "You now have %s.\r\n", buf2);
  send_to_char(buf, ch);

  if (SHOP_USES_BANK(shop_nr))
    if (GET_NUYEN(keeper) > MAX_OUTSIDE_BANK)
    {
      SHOP_BANK(shop_nr) += (GET_NUYEN(keeper) - MAX_OUTSIDE_BANK);
      GET_NUYEN(keeper) = MAX_OUTSIDE_BANK;
    }
}

struct obj_data *get_selling_obj(struct char_data * ch, char *name,
                                       struct char_data * keeper, int shop_nr, int msg)
{
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj = NULL;
  int result;

  if ((!IS_DOCTOR(shop_nr) && !(obj = get_obj_in_list_vis(ch, name, ch->carrying))) ||
      (IS_DOCTOR(shop_nr) && !(obj = get_obj_in_list_vis(ch, name, ch->cyberware)) &&
       !(obj = get_obj_in_list_vis(ch, name, ch->bioware))))
  {
    if (msg) {
      sprintf(buf, "%s %s", GET_CHAR_NAME(ch), shop_table[shop_nr].no_such_item2);
      do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    }
    return (0);
  }

  if (IS_OBJ_STAT(obj, ITEM_IMMLOAD))
  {
    sprintf(buf, "%s I won't buy that!", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return 0;
  }

  if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
    return (obj);

  switch (result)
  {
  case OBJECT_NOTOK:
    strcpy(arg, shop_table[shop_nr].do_not_buy);
    break;
  case OBJECT_DEAD:
    strcpy(arg, MSG_NO_USED_WANDSTAFF);
    break;
  default:
    log("Illegal return value of %d from trade_with() (shop.c)", result);
    strcpy(arg, "An error has occurred.");
    break;
  }
  if (msg)
  {
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), arg);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
  }
  return (0);
}

int sell_price(struct char_data * ch, struct obj_data * obj, int shop_nr)
{
  int i;

  i = (int)(GET_OBJ_COST(obj) * SHOP_SELLPROFIT(shop_nr));
  i = i + (int)((i * SHOP_CURCENT(shop_nr)) / 100);

  return i;
}

struct obj_data *slide_obj(struct obj_data * obj, struct char_data * keeper, int shop_nr)
      /*
         This function is a slight hack!  To make sure that duplicate items are
         only listed once on the "list", this function groups "identical"
         objects together on the shopkeeper's inventory list.  The hack involves
         knowing how the list is put together, and manipulating the order of
         the objects on the list.  (But since most of DIKU is not encapsulated,
         and information hiding is almost never used, it isn't that big a deal) -JF
      */
{
  struct obj_data *loop;
  int temp;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  /* Extract the object if it is identical to one produced */
  if (shop_producing(obj, shop_nr))
  {
    temp = GET_OBJ_RNUM(obj);
    extract_obj(obj);
    return (&obj_proto[temp]);
  }
  SHOP_SORT(shop_nr)++;
  loop = keeper->carrying;
  obj_to_char(obj, keeper);
  /*  keeper->carrying = loop;
    while (loop)
    {
      if (same_obj(obj, loop)) {
        obj->next_content = loop->next_content;
        loop->next_content = obj;
        return (obj);
      }
      loop = loop->next_content;
    }
    keeper->carrying = obj;
   */
  return (obj);
}

void sort_keeper_objs(struct char_data * keeper, int shop_nr)
{
  struct obj_data *list = 0, *temp;

  while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
  {
    temp = keeper->carrying;
    obj_from_char(temp);
    temp->next_content = list;
    list = temp;
  }

  while (list)
  {
    temp = list;
    list = list->next_content;
    if ((shop_producing(temp, shop_nr)) && !(get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying))) {
      obj_to_char(temp, keeper);
      SHOP_SORT(shop_nr)++;
    } else
      (void) slide_obj(temp, keeper, shop_nr);
  }
}

void shopping_sell(char *arg, struct char_data * ch, struct char_data * keeper, int shop_nr)
{
  char tempstr[200], buf[MAX_STRING_LENGTH], name[200];
  struct obj_data *obj = NULL, *credstick = NULL;
  int sellnum, sold = 0, goldamt = 0, cash = 0;
  int sellprice = 0;

  regen_amount_shopkeeper(shop_nr, keeper);

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  skip_spaces(&arg);
  any_one_arg(arg, name);

  if (!str_cmp(name, "cash"))
  {
    arg = any_one_arg(arg, name);
    any_one_arg(arg, name);
    skip_spaces(&arg);
    cash = 1;
  } else if (!(credstick = get_first_credstick(ch, "credstick")))
  {
    send_to_char("Lacking an activated credstick, you choose to deal in cash.\r\n", ch );
    cash = 1;
  }

  if ((sellnum = transaction_amt(arg)) < 0)
  {
    sprintf(buf, "%s A negative amount?  Try buying something.", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }
  if (!(*arg) || !(sellnum))
  {
    sprintf(buf, "%s What do you want to sell??", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }

  if (!(obj = get_selling_obj(ch, arg, keeper, shop_nr, TRUE)))
    return;

  if (IS_CASH_ONLY(shop_nr) && !cash)
  {
    sprintf(buf, "%s We only accept cash here!", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  } else if (IS_CRED_ONLY(shop_nr) && cash)
  {
    sprintf(buf, "%s We don't accept cash!", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }

  sellprice = sell_price(ch, obj, shop_nr);
  if (!IS_DOCTOR(shop_nr) && !WONT_NEGOTIATE(shop_nr))
    sellprice = negotiate(ch, keeper, 0, sellprice, 0, FALSE);

  if (GET_NUYEN(keeper) + SHOP_BANK(shop_nr) < sellprice)
  {
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), shop_table[shop_nr].missing_cash1);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  } else if (GET_OBJ_RNUM(obj) == 1)
  {
    sprintf(buf, "%s I won't buy that.", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }
  if (IS_DOCTOR(shop_nr))
  {
    if (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE) {
      // int ess = GET_ESS(ch);
      obj_from_cyberware(obj);
      //     GET_ESS(ch) = ess;
      //   ch->real_abils.ess = ess;
    } else {
      // int index = GET_INDEX(ch);
      obj_from_bioware(obj);
      // GET_INDEX(ch) = index;
      // ch->real_abils.bod_index = index;
    }

    GET_OBJ_VAL(obj, 4) = 0;

    act("$n takes out a a sharpened scalpel and lies $N down on the operating table.",
        FALSE, keeper, 0, ch, TO_NOTVICT);
    sprintf(arg, shop_table[shop_nr].message_sell, sellprice);
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), arg);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    sprintf(buf, "%s Relax...this won't hurt a bit.", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    act("You delicately remove $p from $N's body.",
        FALSE, keeper, obj, ch, TO_CHAR);
    act("$n performs a delicate procedure on $N.",
        FALSE, keeper, 0, ch, TO_NOTVICT);
    act("$n delicately removes $p from your body.",
        FALSE, keeper, obj, ch, TO_VICT);
    if (cash)
      GET_NUYEN(ch) += sellprice;
    else
      GET_OBJ_VAL(credstick, 0) += sellprice;
    if (GET_NUYEN(keeper) < MIN_OUTSIDE_BANK) {
      sellprice = MIN(MAX_OUTSIDE_BANK - GET_NUYEN(keeper), SHOP_BANK(shop_nr));
      SHOP_BANK(shop_nr) -= sellprice;
      GET_NUYEN(keeper) += sellprice;
    }
    if (shop_producing(obj, shop_nr))
      extract_obj(obj);
    else
      obj_to_char(obj, keeper);
    return;
  }
  if (goldamt >= 150000)
  {
    sprintf(buf, "%s sold %s (%ld) for more than 150k.", GET_CHAR_NAME(ch), obj->text.name, GET_OBJ_VNUM(obj));
    mudlog(buf, ch, LOG_CHEATLOG, TRUE);
  }
  while ((obj) && (GET_NUYEN(keeper) + SHOP_BANK(shop_nr) >= sellprice)
         && (sold < sellnum))
  {
    sold++;
    obj_from_char(obj);
    slide_obj(obj, keeper, shop_nr);
    goldamt += sellprice;
    GET_NUYEN(keeper) -= sellprice;
    obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
  }

  if (sold < sellnum)
  {
    if (!obj)
      sprintf(arg, "You only have %d of those.", sold);
    else if (GET_NUYEN(keeper) + SHOP_BANK(shop_nr) < sellprice)
      sprintf(arg, "I can only afford to buy %d of those.", sold);
    else
      sprintf(arg, "Something really screwy made me buy %d.", sold);
    sprintf(buf, "%s %s", GET_CHAR_NAME(ch), arg);
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
  }
  if (cash)
    GET_NUYEN(ch) += goldamt;
  else
    GET_OBJ_VAL(credstick, 0) += goldamt;
  strcpy(tempstr, times_message(0, name, sold));
  sprintf(buf, "$n sells %s.", tempstr);
  act(buf, FALSE, ch, obj, 0, TO_ROOM);

  sprintf(arg, shop_table[shop_nr].message_sell, goldamt);
  sprintf(buf, "%s %s", GET_CHAR_NAME(ch), arg);
  do_say(keeper, buf, cmd_say, SCMD_SAYTO);
  sprintf(buf, "The shopkeeper now has %s.\r\n", tempstr);
  send_to_char(buf, ch);

  if (GET_NUYEN(keeper) < MIN_OUTSIDE_BANK)
  {
    goldamt = MIN(MAX_OUTSIDE_BANK - GET_NUYEN(keeper), SHOP_BANK(shop_nr));
    SHOP_BANK(shop_nr) -= goldamt;
    GET_NUYEN(keeper) += goldamt;
  }
}

void shopping_value(char *arg, struct char_data * ch,
                    struct char_data * keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  char name[MAX_INPUT_LENGTH];

  regen_amount_shopkeeper(shop_nr, keeper);

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (!(*arg))
  {
    sprintf(buf, "%s What do you want me to evaluate?", GET_CHAR_NAME(ch));
    do_say(keeper, buf, cmd_say, SCMD_SAYTO);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  sprintf(buf, "%s I'll give you about %d credits for that.", GET_CHAR_NAME(ch), sell_price(ch, obj, shop_nr));
  do_say(keeper, buf, cmd_say, SCMD_SAYTO);

  return;
}

char *list_object(struct obj_data * obj, int cnt, int index, int shop_nr)
{
  static char buf[256];
  char buf2[300], buf3[200];

  if (shop_producing(obj, shop_nr))
    strcpy(buf2, "Unlimited   ");
  else
    sprintf(buf2, "%5d       ", cnt);
  sprintf(buf, " %2d)  %s", index, buf2);

  /* Compile object name and information */
  strcpy(buf3, obj->text.name);

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_PROGRAM:
    sprintf(buf3, "%s (R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    break;
  case ITEM_SPELL_FORMULA:
    sprintf(buf3, "%s (%s, F:%2d)", buf3,
            GET_OBJ_VAL(obj, 7) ? "shamanic" : "hermetic",
            GET_OBJ_VAL(obj, 2));
    break;
  case ITEM_FOCUS:
    if (GET_OBJ_VAL(obj, 0) == FOCI_LOCK)
      strcat(buf3, " (lock)");
    else if (GET_OBJ_VAL(obj, 0) == FOCI_SPELL)
      sprintf(buf3, "%s (spell focus, R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    else if (GET_OBJ_VAL(obj, 0) == FOCI_SPELL_CAT)
      sprintf(buf3, "%s (category focus, R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    else if (GET_OBJ_VAL(obj, 0) == FOCI_POWER)
      sprintf(buf3, "%s (power focus, R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    else if (GET_OBJ_VAL(obj, 0) == FOCI_WEAPON)
      sprintf(buf3, "%s (weapon focus, R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    else if (GET_OBJ_VAL(obj, 0) == FOCI_SPIRIT)
      sprintf(buf3, "%s (spirit focus, R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    else
      sprintf(buf3, "%s (unknown focus, R:%2d)", buf3, GET_OBJ_VAL(obj, 1));
    break;
  case ITEM_FIREWEAPON:
    sprintf(buf3, "%s (Str min:%d)", buf3, (!GET_OBJ_VAL(obj, 5) ? GET_OBJ_VAL(obj, 6) :
                                            GET_OBJ_VAL(obj, 1) + 2));
    break;
  case ITEM_RADIO:
    sprintf(buf3, "%s (É:%d, Crypt:%d)", buf3, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
    break;
  }

  sprintf(buf2, "%-48s %6d\r\n", buf3, buy_price(obj, shop_nr));
  strcat(buf, CAP(buf2));
  return (buf);
}

char *list_cyber_obj(struct obj_data * obj, int cnt, int index, int shop_nr)
{
  static char buf[256];
  char buf2[300], buf3[200];

  if (shop_producing(obj, shop_nr))
    strcpy(buf2, " Unlim.   ");
  else
    sprintf(buf2, "%5d     ", cnt);
  sprintf(buf, " %2d %s", index, buf2);

  strcpy(buf3, obj->text.name);
  if (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE && (GET_OBJ_VAL(obj, 2) == 2 ||
      GET_OBJ_VAL(obj, 2) == 3))
    sprintf(buf3, "%s (É:%d, Crypt:%d)", buf3, GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5));

  sprintf(buf2, "%-33s %-6s%2d   %0.2f%c  %9d\r\n", buf3,


          GET_OBJ_TYPE(obj) == ITEM_CYBERWARE ? "Cyber" : "Bio",
          GET_OBJ_VAL(obj, 0), ((float)GET_OBJ_VAL(obj, 1) / 100),
          GET_OBJ_TYPE(obj) == ITEM_CYBERWARE ? 'E' : 'I', buy_price(obj, shop_nr));
  strcat(buf, CAP(buf2));
  return (buf);
}

void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], name[200];
  struct obj_data *obj = NULL, *last_obj = NULL;
  int cnt = 0, index = 0;

  regen_amount_shopkeeper(shop_nr, keeper);

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  one_argument(arg, name);
  if (IS_DOCTOR(shop_nr))
  {
    strcpy(buf, " ## Available Item / Service                     Rating Ess/Index    Price\r\n");
    strcat(buf, "--- --------- --------------------------------- -------- --------- --------\r\n");
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && (obj->obj_flags.cost > 0) &&
          (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE || GET_OBJ_TYPE(obj) == ITEM_BIOWARE)) {
        if (!last_obj) {
          last_obj = obj;
          cnt = 1;
        } else if (same_obj(last_obj, obj))
          cnt++;
        else {
          index++;
          if (!(*name) || isname(name, last_obj->text.keywords))
            strcat(buf, list_cyber_obj(last_obj, cnt, index, shop_nr));
          cnt = 1;
          last_obj = obj;
        }
      }
    index++;
    if (last_obj && (!*name || isname(name, last_obj->text.keywords)))
      strcat(buf, list_cyber_obj(last_obj, cnt, index, shop_nr));
    strcat(buf, "  -  Unlim.   Treatment                                                 200\r\n");
  } else
  {
    if (WONT_NEGOTIATE(shop_nr))
      strcpy(buf, " ##   Available   Item                                              Price\r\n");
    else
      strcpy(buf, " ##   Available   Item                                                RRP\r\n");
    strcat(buf, "-------------------------------------------------------------------------\r\n");
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && (obj->obj_flags.cost > 0)) {
        if (!last_obj) {
          last_obj = obj;
          cnt = 1;
        } else if (same_obj(last_obj, obj))
          cnt++;
        else {
          index++;
          if (!(*name) || isname(name, last_obj->text.keywords))
            strcat(buf, list_object(last_obj, cnt, index, shop_nr));
          cnt = 1;
          last_obj = obj;
        }
      }
    index++;
    if (!last_obj)
      if (*name)
        strcpy(buf, "Presently, none of those are for sale.\r\n");
      else
        strcpy(buf, "Currently, there is nothing for sale.\r\n");
    else if (!(*name) || isname(name, last_obj->text.keywords))
      strcat(buf, list_object(last_obj, cnt, index, shop_nr));
  }
  page_string(ch->desc, buf, 1);
}

int ok_shop_room(int shop_nr, int room)
{
  int index;

  if (!shop_table[shop_nr].in_room)
    return TRUE;

  for (index = 0; index < shop_table[shop_nr].num_rooms; index++)
    if (SHOP_ROOM(shop_nr, index) == room)
      return (TRUE);
  return (FALSE);
}

SPECIAL(shop_keeper)
{
  struct char_data *keeper = (struct char_data *) me;
  int shop_nr;

  if (!cmd)
    return FALSE;

  for (shop_nr = 0; shop_nr < top_of_shopt; shop_nr++)
    if (SHOP_KEEPER(shop_nr) == GET_MOB_VNUM(keeper))
      break;

  if (shop_nr >= top_of_shopt)
    return (FALSE);

  if (keeper == ch) {
    if (cmd)
      SHOP_SORT(shop_nr) = 0;   /* Safety in case "drop all" */
    return (FALSE);
  }
  if (!ok_shop_room(shop_nr, world[ch->in_room].number))
    return (0);

  if (!AWAKE(keeper))
    return (FALSE);

  if (CMD_IS("buy")) {
    shopping_buy(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("sell")) {
    shopping_sell(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("value")) {
    shopping_value(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("list")) {
    shopping_list(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("info")) {
    shopping_info(argument, ch, keeper, shop_nr);
    return TRUE;
  }
  return (FALSE);
}

int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim)
{
  char buf[200];
  int index;
  SPECIAL(receptionist);
  SPECIAL(taxi);
  SPECIAL(fixer);

  if (IS_NPC(victim) && (mob_index[GET_MOB_RNUM(victim)].func == shop_keeper))
    for (index = 0; index < top_of_shopt; index++)
      //if ((GET_MOB_VNUM(victim) == SHOP_KEEPER(index)) && !SHOP_KILL_CHARS(index)) {
      if (GET_MOB_VNUM(victim) == SHOP_KEEPER(index))
      {
        if (ch->in_room == victim->in_room) {
          do_action(victim, GET_NAME(ch), cmd_slap, 0);
          sprintf(buf, "%s %s", GET_CHAR_NAME(ch), MSG_CANT_KILL_KEEPER);
          do_say(victim, buf, cmd_say, SCMD_SAYTO);
        }
        return (FALSE);
      }
  if (IS_NPC(victim) && GET_MOB_SPEC(victim) &&
      (GET_MOB_SPEC(victim) == receptionist || GET_MOB_SPEC(victim) == taxi ||
       GET_MOB_SPEC(victim) == fixer))
    return (FALSE);

  return (TRUE);
}

void regen_amount_shopkeeper(int shop_nr, struct char_data * keeper)
{
  int hasNow;
  int addAmount;
  long lastTime;
  float fraction;

  lastTime = SHOP_LASTTIME(shop_nr);
  if (lastTime == 0)
  {
    int index, max_val, total_val;
    struct obj_data *obj;

    lastTime = mud_boot_time;
    SHOP_BANK(shop_nr) = GET_BANK(keeper);
    SHOP_REGEN_AMOUNT(shop_nr) = GET_BANK(keeper) + GET_NUYEN(keeper);

    max_val = 0;
    total_val = 0;
    for (index = 0; index < shop_table[shop_nr].num_producing; index++) {
      obj = &obj_proto[real_object(SHOP_PRODUCT(shop_nr, index))];
      if ( obj != NULL ) {
        total_val += GET_OBJ_COST(obj) / 8;
        if (GET_OBJ_COST(obj) > max_val)
          max_val = GET_OBJ_COST(obj);
      }
    }
    if (SHOP_REGEN_AMOUNT(shop_nr) < total_val)
      SHOP_REGEN_AMOUNT(shop_nr) = total_val;
    if (SHOP_REGEN_AMOUNT(shop_nr) < max_val/2)
      SHOP_REGEN_AMOUNT(shop_nr) = max_val/2;
  }

  SHOP_LASTTIME(shop_nr) = time(0);

  hasNow = GET_NUYEN(keeper) + SHOP_BANK(shop_nr);
  if (hasNow >= SHOP_REGEN_AMOUNT(shop_nr) )
    return;

  fraction = (float)(SHOP_LASTTIME(shop_nr) - lastTime) /
             (float) SHOP_REGEN_TIME;
  addAmount = (int) (fraction * SHOP_REGEN_AMOUNT(shop_nr));
  if ((addAmount+hasNow) > SHOP_REGEN_AMOUNT(shop_nr))
    addAmount = SHOP_REGEN_AMOUNT(shop_nr) - hasNow;

  SHOP_BANK(shop_nr) += addAmount;
}

void randomize_shop_prices(void)
{
  int i;

  for (i = 0; i < top_of_shopt; i++)
    if (SHOP_PERCENTAGE(i) > 0)
      SHOP_CURCENT(i) = number(-SHOP_PERCENTAGE(i), SHOP_PERCENTAGE(i));
}

#define SHOP            d->edit_shop

void boot_one_shop(struct shop_data *shop)
{
  int count, i;
  vnum_t shop_nr = NOWHERE;

  if ((top_of_shopt + 1) >= top_of_shop_array)
    // if it cannot resize, free the edit_shop and return
    if (!resize_shp_array())
    {
      olc_state = 0;
      return;
    }

  for (count = 0; count < top_of_shopt; count++)
    if (SHOP_NUM(count) > shop->vnum)
    {
      shop_nr = count;
      break;
    }

  if (shop_nr == -1)
    shop_nr = top_of_shopt;
  else
    for (count = top_of_shopt; count > shop_nr; count--)
      shop_table[count] = shop_table[count-1];

  top_of_shopt++;

  SHOP_NUM(shop_nr) = shop->vnum;

  shop_table[shop_nr].num_producing = shop->num_producing;
  if (shop_table[shop_nr].num_producing > 0)
  {
    shop_table[shop_nr].producing = new long[shop_table[shop_nr].num_producing];
    for (count = 0; count < shop_table[shop_nr].num_producing; count++)
      SHOP_PRODUCT(shop_nr, count) = shop->producing[count];
  } else
    shop_table[shop_nr].producing = NULL;

  SHOP_BUYPROFIT(shop_nr) = shop->profit_buy;
  SHOP_SELLPROFIT(shop_nr) = shop->profit_sell;

  shop_table[shop_nr].num_buy_types = shop->num_buy_types;
  if (shop_table[shop_nr].num_buy_types > 0)
  {
    i = 0;
    shop_table[shop_nr].type = new int[shop_table[shop_nr].num_buy_types];
    for (count = 0; count < NUM_ITEM_TYPES &&
         i < shop_table[shop_nr].num_buy_types; count++)
      if (shop->type[count] == 1) {
        SHOP_BUYTYPE(shop_nr, i) = count + 1;
        i++;
      }
  } else
    shop_table[shop_nr].type = NULL;

  shop_table[shop_nr].no_such_item1 = str_dup(shop->no_such_item1);
  shop_table[shop_nr].no_such_item2 = str_dup(shop->no_such_item2);
  shop_table[shop_nr].do_not_buy = str_dup(shop->do_not_buy);
  shop_table[shop_nr].missing_cash1 = str_dup(shop->missing_cash1);
  shop_table[shop_nr].missing_cash2 = str_dup(shop->missing_cash2);
  shop_table[shop_nr].message_buy = str_dup(shop->message_buy);
  shop_table[shop_nr].message_sell = str_dup(shop->message_sell);
  SHOP_TEMPER(shop_nr) = shop->temper;
  SHOP_BITVECTOR(shop_nr) = shop->bitvector;
  SHOP_KEEPER(shop_nr) = shop->keeper;
  SHOP_TRADE_WITH(shop_nr) = shop->with_who;

  shop_table[shop_nr].num_rooms = shop->num_rooms;
  if (shop_table[shop_nr].num_rooms > 0)
  {
    shop_table[shop_nr].in_room = new long[shop_table[shop_nr].num_rooms];
    for (count = 0; count < shop_table[shop_nr].num_rooms; count++)
      SHOP_ROOM(shop_nr, count) = shop->in_room[count];
  } else
    shop_table[shop_nr].in_room = NULL;

  SHOP_OPEN1(shop_nr) = shop->open1;
  SHOP_CLOSE1(shop_nr) = shop->close1;
  SHOP_OPEN2(shop_nr) = shop->open2;
  SHOP_CLOSE2(shop_nr) = shop->close2;

  SHOP_BANK(shop_nr) = 0;
  SHOP_SORT(shop_nr) = 0;
  SHOP_LASTTIME(shop_nr) = 0;

  if ((i = real_mobile(SHOP_KEEPER(shop_nr))) > 0 && SHOP_KEEPER(shop_nr) != 1151)
  {
    mob_index[i].sfunc = mob_index[i].func;
    mob_index[i].func = shop_keeper;
  }
}

void reboot_shop(int shop_nr, struct shop_data *shop)
{
  int count, i, okn, nkn;

  SHOP_BUYPROFIT(shop_nr) = shop->profit_buy;
  SHOP_SELLPROFIT(shop_nr) = shop->profit_sell;

  if (shop_table[shop_nr].type)
    delete [] shop_table[shop_nr].type;

  shop_table[shop_nr].num_buy_types = shop->num_buy_types;
  if (shop_table[shop_nr].num_buy_types > 0)
  {
    shop_table[shop_nr].type = new int[shop_table[shop_nr].num_buy_types];
    i = 0;
    for (count = 0; count < NUM_ITEM_TYPES &&
         i < shop_table[shop_nr].num_buy_types; count++)
      if (shop->type[count] == 1) {
        SHOP_BUYTYPE(shop_nr, i) = count + 1;
        i++;
      }
  } else
    shop_table[shop_nr].type = NULL;

  if (shop_table[shop_nr].no_such_item1)
    delete [] shop_table[shop_nr].no_such_item1;
  shop_table[shop_nr].no_such_item1 = str_dup(shop->no_such_item1);
  if (shop_table[shop_nr].no_such_item2)
    delete [] shop_table[shop_nr].no_such_item2;
  shop_table[shop_nr].no_such_item2 = str_dup(shop->no_such_item2);
  if (shop_table[shop_nr].do_not_buy)
    delete [] shop_table[shop_nr].do_not_buy;
  shop_table[shop_nr].do_not_buy = str_dup(shop->do_not_buy);
  if (shop_table[shop_nr].missing_cash1)
    delete [] shop_table[shop_nr].missing_cash1;
  shop_table[shop_nr].missing_cash1 = str_dup(shop->missing_cash1);
  if (shop_table[shop_nr].missing_cash2)
    delete [] shop_table[shop_nr].missing_cash2;
  shop_table[shop_nr].missing_cash2 = str_dup(shop->missing_cash2);
  if (shop_table[shop_nr].message_buy)
    delete [] shop_table[shop_nr].message_buy;
  shop_table[shop_nr].message_buy = str_dup(shop->message_buy);
  if (shop_table[shop_nr].message_sell)
    delete [] shop_table[shop_nr].message_sell;
  shop_table[shop_nr].message_sell = str_dup(shop->message_sell);
  SHOP_TEMPER(shop_nr) = shop->temper;
  SHOP_BITVECTOR(shop_nr) = shop->bitvector;
  SHOP_TRADE_WITH(shop_nr) = shop->with_who;

  if (shop_table[shop_nr].producing)
    delete [] shop_table[shop_nr].producing;

  shop_table[shop_nr].num_producing = shop->num_producing;
  if (shop_table[shop_nr].num_producing > 0)
  {
    shop_table[shop_nr].producing = new long[shop_table[shop_nr].num_producing];
    for (count = 0; count < shop_table[shop_nr].num_producing; count++)
      SHOP_PRODUCT(shop_nr, count) = shop->producing[count];
  } else
    shop_table[shop_nr].producing = NULL;

  if (shop_table[shop_nr].in_room)
    delete [] shop_table[shop_nr].in_room;
  shop_table[shop_nr].num_rooms = shop->num_rooms;
  if (shop_table[shop_nr].num_rooms > 0)
  {
    shop_table[shop_nr].in_room = new long[shop_table[shop_nr].num_rooms];
    for (count = 0; count < shop_table[shop_nr].num_rooms; count++)
      SHOP_ROOM(shop_nr, count) = shop->in_room[count];
  } else
    shop_table[shop_nr].in_room = NULL;

  SHOP_OPEN1(shop_nr) = shop->open1;
  SHOP_CLOSE1(shop_nr) = shop->close1;
  SHOP_OPEN2(shop_nr) = shop->open2;
  SHOP_CLOSE2(shop_nr) = shop->close2;
  SHOP_BUYPROFIT(shop_nr) = shop->profit_buy;
  SHOP_SELLPROFIT(shop_nr) = shop->profit_sell;
  SHOP_SORT(shop_nr) = 0;

  if (SHOP_KEEPER(shop_nr) != shop->keeper && SHOP_KEEPER(shop_nr) != 1151)
  {
    okn = real_mobile(shop_table[shop_nr].keeper);
    nkn = real_mobile(shop->keeper);
    if (mob_index[okn].func == shop_keeper) {
      mob_index[okn].func = mob_index[okn].sfunc;
      mob_index[okn].sfunc = NULL;
    } else if (mob_index[okn].sfunc == shop_keeper)
      mob_index[okn].sfunc = NULL;
    mob_index[nkn].sfunc = mob_index[nkn].func;
    mob_index[nkn].func = shop_keeper;
    SHOP_BANK(shop_nr) = MAX(GET_BANK(&mob_proto[nkn]),SHOP_BANK(shop_nr));
    shop_table[shop_nr].keeper = shop->keeper;
  }
}

void assign_the_shopkeepers(void)
{
  int index, rnum;

  cmd_say = find_command("say");
  cmd_tell = find_command("tell");
  cmd_emote = find_command("emote");
  cmd_slap = find_command("slap");
  for (index = 0; index < top_of_shopt; index++) {
    if ((rnum = real_mobile(SHOP_KEEPER(index))) < 0)
      log("Shopkeeper #%d does not exist (shop #%d)",
          SHOP_KEEPER(index), SHOP_NUM(index));
    else if (mob_index[rnum].func != shop_keeper && SHOP_KEEPER(index) != 1151) {
      mob_index[rnum].sfunc = mob_index[rnum].func;
      mob_index[rnum].func = shop_keeper;
    }
  }
}

char *customer_string(int shop_nr, int detailed)
{
  int index, cnt = 1;
  static char buf[256];

  *buf = 0;
  for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
    if (!(SHOP_TRADE_WITH(shop_nr) & cnt))
      if (detailed) {
        if (*buf)
          strcat(buf, ", ");
        strcat(buf, trade_letters[index]);
      } else
        sprintf(END_OF(buf), "%c", *trade_letters[index]);
    else if (!detailed)
      strcat(buf, "_");

  return (buf);
}

char *customer_string_shop(struct shop_data *shop)
{
  int index, cnt = 1;
  static char buf[256];

  *buf = 0;
  for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
    if (!(shop->with_who & cnt))
      sprintf(END_OF(buf), "%c", *trade_letters[index]);
    else
      strcat(buf, "_");

  return (buf);
}

void list_all_shops(struct char_data * ch)
{
  int shop_nr;

  strcpy(buf, "\r\n");
  for (shop_nr = 0; shop_nr < top_of_shopt; shop_nr++)
  {
    if (shop_nr == 19 || shop_nr == 0 || (!((shop_nr - 1) % 19) && shop_nr > 20)) {
      strcat(buf, " ##   Virtual   Where    Keeper    Buy   Sell Customers\r\n");
      strcat(buf, "---------------------------------------------------------\r\n");
    }
    sprintf(buf2, "%3d   %6ld   %6ld    ", shop_nr + 1, SHOP_NUM(shop_nr),
            shop_table[shop_nr].in_room ? SHOP_ROOM(shop_nr, 0) : -1);
    if (real_mobile(SHOP_KEEPER(shop_nr)) < 0)
      strcpy(buf1, "<NONE>");
    else
      sprintf(buf1, "%6ld", SHOP_KEEPER(shop_nr));
    sprintf(END_OF(buf2), "%s   %3.2f   %3.2f    ", buf1,
            SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr));
    strcat(buf2, customer_string(shop_nr, FALSE));
    sprintf(END_OF(buf), "%s\r\n", buf2);
  }

  page_string(ch->desc, buf, 1);
}

void handle_detailed_list(char *buf, char *buf1, struct char_data * ch)
{
  if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
    strcat(buf, buf1);
  else
  {
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    sprintf(buf, "            %s", buf1);
  }
}

void list_detailed_shop(struct char_data * ch, long shop_nr)
{
  struct obj_data *obj;
  struct char_data *k;
  int index, temp;

  sprintf(buf, "Vnum:       [%5ld], Rnum: [%5ld]\r\n", SHOP_NUM(shop_nr), shop_nr + 1);
  send_to_char(buf, ch);

  strcpy(buf, "Rooms:      ");
  if (shop_table[shop_nr].in_room)
  {
    for (index = 0; index < shop_table[shop_nr].num_rooms; index++) {
      if (index)
        strcat(buf, ", ");
      if ((temp = real_room(SHOP_ROOM(shop_nr, index))) != NOWHERE)
        sprintf(buf1, "%s (#%ld)", world[temp].name, world[temp].number);
      else
        sprintf(buf1, "<UNKNOWN> (#%ld)", SHOP_ROOM(shop_nr, index));
      handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
      send_to_char("Rooms:      None!\r\n", ch);
    else {
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Rooms:      None!\r\n", ch);

  strcpy(buf, "Shopkeeper: ");
  if (real_mobile(SHOP_KEEPER(shop_nr)) >= 0)
  {
    sprintf(END_OF(buf), "%s (#%ld)\r\n",
            GET_NAME((&mob_proto[real_mobile(SHOP_KEEPER(shop_nr))])),
            SHOP_KEEPER(shop_nr));
    if ((k = get_char_num(real_mobile(SHOP_KEEPER(shop_nr))))) {
      send_to_char(buf, ch);
      regen_amount_shopkeeper(shop_nr, k);
      sprintf(buf, "Coins:      [%9d], Bank: [%9d] (Total: %d) Regen: %d\r\n",
              GET_NUYEN(k), SHOP_BANK(shop_nr), GET_NUYEN(k) + SHOP_BANK(shop_nr),
              SHOP_REGEN_AMOUNT(shop_nr));
    }
  } else
    strcat(buf, "<NONE>\r\n");
  send_to_char(buf, ch);

  strcpy(buf1, customer_string(shop_nr, TRUE));
  sprintf(buf, "Customers:  %s\r\n", (*buf1) ? buf1 : "None");
  send_to_char(buf, ch);

  strcpy(buf, "Produces:   ");
  if (shop_table[shop_nr].producing)
  {
    for (index = 0; index < shop_table[shop_nr].num_producing; index++) {
      obj = &obj_proto[real_object(SHOP_PRODUCT(shop_nr, index))];
      if (index)
        strcat(buf, ", ");
      sprintf(buf1, "%s (#%ld)", obj->text.name,
              obj_index[real_object(SHOP_PRODUCT(shop_nr, index))].vnum);
      handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
      send_to_char("Produces:   Nothing!\r\n", ch);
    else {
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Produces:   Nothing!\r\n", ch);

  strcpy(buf, "Buys:       ");
  if (shop_table[shop_nr].type)
  {
    for (index = 0; index < shop_table[shop_nr].num_buy_types; index++) {
      if (index)
        strcat(buf, ", ");
      sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, index)],
              SHOP_BUYTYPE(shop_nr, index));
      handle_detailed_list(buf, buf1, ch);
    }
    if (!index)
      send_to_char("Buys:       Nothing!\r\n", ch);
    else {
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Buys:       Nothing!\r\n", ch);

  sprintf(buf, "Buy at:     [%0.2f], Sell at: [%0.2f], ± %%: [%d], "
          "Current %%: [%d], Open: [%d-%d, %d-%d]\r\n",
          SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr),
          SHOP_PERCENTAGE(shop_nr), SHOP_CURCENT(shop_nr),
          SHOP_OPEN1(shop_nr), SHOP_CLOSE1(shop_nr),
          SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr));

  send_to_char(buf, ch);

  sprintbit((long) SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
  sprintf(buf, "Bits:       %s\r\n", buf1);
  send_to_char(buf, ch);
}

int write_shops_to_disk(int zone)
{
  long i, j, found = 0, counter, max;
  FILE *fp;
  zone = real_zone(zone);

  sprintf(buf, "world/shp/%d.shp", zone_table[zone].number);

  if (!(fp = fopen(buf, "w+"))) {
    log("SYSERR: could not open file %d.shp", zone_table[zone].number);

    fclose(fp);

    return 0;
  }
  for (counter = zone_table[zone].number * 100;
       counter <= zone_table[zone].top; counter++) {
    if ((i = real_shop(counter)) > -1) {
      fprintf(fp, "#%ld\n", SHOP_NUM(i));

      fprintf(fp, "%ld %ld %ld\n", shop_table[i].num_producing,
              shop_table[i].num_buy_types, shop_table[i].num_rooms);

      max = MAX(shop_table[i].num_producing, MAX(shop_table[i].num_buy_types,
                shop_table[i].num_rooms));

      for (j = 0; j < max; j++) {
        if (j < shop_table[i].num_producing)
          fprintf(fp, "%ld ", SHOP_PRODUCT(i, j));
        else
          fprintf(fp, "-1 ");
        if (j < shop_table[i].num_buy_types)
          fprintf(fp, "%d ", SHOP_BUYTYPE(i, j));
        else
          fprintf(fp, "-1 ");
        if (j < shop_table[i].num_rooms)
          fprintf(fp, "%ld\n", SHOP_ROOM(i, j));
        else
          fprintf(fp, "-1\n");
      }

      fprintf(fp, "%0.2f %0.2f %d %d %d %ld %d\n", SHOP_BUYPROFIT(i),
              SHOP_SELLPROFIT(i), SHOP_PERCENTAGE(i), SHOP_TEMPER(i),
              SHOP_BITVECTOR(i), SHOP_KEEPER(i), SHOP_TRADE_WITH(i));

      fprintf(fp, "%d %d %d %d\n", SHOP_OPEN1(i), SHOP_CLOSE1(i),
              SHOP_OPEN2(i), SHOP_CLOSE2(i));

      fprintf(fp, "%s~\n", shop_table[i].no_such_item1);
      fprintf(fp, "%s~\n", shop_table[i].no_such_item2);
      fprintf(fp, "%s~\n", shop_table[i].do_not_buy);
      fprintf(fp, "%s~\n", shop_table[i].missing_cash1);
      fprintf(fp, "%s~\n", shop_table[i].missing_cash2);
      fprintf(fp, "%s~\n", shop_table[i].message_buy);
      fprintf(fp, "%s~\n", shop_table[i].message_sell);
    }
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  fp = fopen("world/shp/index", "w+");

  for (i = 0; i <= top_of_zone_table; ++i) {
    found = 0;
    for (j = 0; !found && j < top_of_shopt; j++)
      if (SHOP_NUM(j) >= (zone_table[i].number * 100) && SHOP_NUM(j) <= zone_table[i].top) {
        found = 1;
        fprintf(fp, "%d.shp\n", zone_table[i].number);
      }
  }

  fprintf(fp, "$~\n");
  fclose(fp);
  return 1;
}

void shedit_buy_type_menu(struct descriptor_data *d)
{
  int i, first = 1;

  CLS(CH);
  for (i = 1; i <= NUM_ITEM_TYPES; i += 2)
    send_to_char(CH, "%2d) %-20s %2d) %-20s\r\n",
                 i, item_types[i], i + 1, i + 1 <= NUM_ITEM_TYPES ? item_types[i + 1] : "");
  sprintf(buf, "Current item types to buy: %s", CCCYN(CH, C_CMP));
  for (i = 0; i < NUM_ITEM_TYPES; i++)
    if (SHOP->type[i] == 1)
    {
      if (!first)
        strcat(buf, ", ");
      else
        first = 0;
      sprintf(buf, "%s%s", buf, item_types[i+1]);
    }
  sprintf(buf, "%s%s\r\n", buf, CCNRM(CH, C_CMP));
  send_to_char(buf, CH);
  send_to_char(CH, "Item type to buy (0 to quit): ");
  d->edit_mode = SHEDIT_BUY_TYPE_MENU;
}

void shedit_disp_extras(struct descriptor_data *d)
{
  int counter;

  CLS(CH);
  for (counter = 0; counter < NUM_SHOP_EXTRAS; counter++)
    send_to_char(CH, "%2d) %s\r\n", counter + 1, shop_bits[counter]);

  sprintbit(SHOP->bitvector, shop_bits, buf1);
  send_to_char(CH, "Shop extras: %s%s%s\r\nEnter shop extra flag, 0 to quit:",
               CCCYN(CH, C_CMP), buf1, CCNRM(CH, C_CMP));
  d->edit_mode = SHEDIT_BITVECTOR;
}

void shedit_trade_with(struct descriptor_data *d)
{
  int i;

  CLS(CH);
  for (i = 1; *trade_letters[i - 1] != '\n'; i++)
    send_to_char(CH, " %2d) %s\r\n", i, trade_letters[i - 1]);
  send_to_char(CH, "Trade with: %s%s%s\r\n", CCCYN(CH, C_CMP),
               customer_string_shop(SHOP), CCNRM(CH, C_CMP));
  send_to_char("Enter trade restriction (0 to quit): ", CH);
}

void shedit_disp_menu(struct descriptor_data *d)
{
  int i;

  CLS(CH);
  send_to_char(CH, "Shop number: %s%ld%s\r\n", CCCYN(CH, C_CMP), d->edit_number, CCNRM(CH, C_CMP));
  sprintf(buf, "1) Items: %s", CCCYN(CH, C_CMP));
  for (i = 0; i < SHOP->num_producing; i++)
  {
    if (i)
      strcat(buf, ", ");
    sprintf(buf + strlen(buf), "%ld", SHOP->producing[i]);
  }
  sprintf(buf, "%s%s\r\n", buf, CCNRM(CH, C_CMP));
  send_to_char(buf, CH);
  send_to_char(CH, "2) Selling profit: %s%0.2f%s\r\n", CCCYN(CH, C_CMP),
               SHOP->profit_buy, CCNRM(CH, C_CMP));
  send_to_char(CH, "3) Buying profit: %s%0.2f%s\r\n", CCCYN(CH, C_CMP),
               SHOP->profit_sell, CCNRM(CH, C_CMP));
  send_to_char(CH, "4) ± cost %%: %s%d%s\r\n", CCCYN(CH, C_CMP),
               SHOP->percentage, CCNRM(CH, C_CMP));
  send_to_char(CH, "5) Buy type menu\r\n");
  send_to_char(CH, "6) No such item (keeper):    %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->no_such_item1,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "7) No such item (player):    %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->no_such_item2,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "8) Not enough cash (keeper): %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->missing_cash1,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "9) Not enough cash (player): %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->missing_cash2,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "a) Keeper doesn't buy:       %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->do_not_buy,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "b) Player buys item:         %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->message_buy,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "c) Player sells item:        %s%s%s\r\n", CCCYN(CH, C_CMP), SHOP->message_sell,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "d) Keeper's temper: %s%d%s\r\n", CCCYN(CH, C_CMP), SHOP->temper, CCNRM(CH, C_CMP));
  send_to_char(CH, "e) Extras menu\r\n");
  send_to_char(CH, "f) Keeper: %s%ld%s (%s%s%s)\r\n", CCCYN(CH, C_CMP), SHOP->keeper, CCNRM(CH, C_CMP),
               CCCYN(CH, C_CMP), real_mobile(SHOP->keeper) < 0 ? "null" :
               GET_NAME(mob_proto+real_mobile(SHOP->keeper)),
               CCNRM(CH, C_CMP));
  send_to_char(CH, "g) Trade with: %s%s%s\r\n", CCCYN(CH, C_CMP),
               customer_string_shop(SHOP), CCNRM(CH, C_CMP));
  sprintf(buf, "h) Rooms to sell in: %s", CCCYN(CH, C_CMP));
  for (i = 0; i < SHOP->num_rooms; i++)
  {
    if (i)
      strcat(buf, ", ");
    sprintf(buf + strlen(buf), "%ld", SHOP->in_room[i]);
  }
  sprintf(buf, "%s%s\r\n", buf, CCNRM(CH, C_CMP));
  send_to_char(buf, CH);
  send_to_char(CH, "i) First opening time: %s%d%s\r\n", CCCYN(CH, C_CMP), SHOP->open1, CCNRM(CH, C_CMP));
  send_to_char(CH, "j) First closing time: %s%d%s\r\n", CCCYN(CH, C_CMP), SHOP->close1, CCNRM(CH, C_CMP));
  send_to_char(CH, "k) Second opening time: %s%d%s\r\n", CCCYN(CH, C_CMP), SHOP->open2, CCNRM(CH, C_CMP));
  send_to_char(CH, "l) Second closing time: %s%d%s\r\n", CCCYN(CH, C_CMP), SHOP->close2, CCNRM(CH, C_CMP));
  send_to_char("q) Quit and save\r\n", CH);
  send_to_char("x) Exit and abort\r\n", CH);
  send_to_char("Enter your choice:\r\n", CH);
  d->edit_mode = SHEDIT_MAIN_MENU;
}

void shedit_parse(struct descriptor_data *d, char *arg)
{
  int number, i, found;
  float profit;

  switch(d->edit_mode)
  {
  case SHEDIT_CONFIRM_EDIT:
    switch (*arg) {
    case 'y':
    case 'Y':
      shedit_disp_menu(d);
      break;
    case 'n':
    case 'N':
      STATE(d) = CON_PLAYING;
      free_shop(SHOP);
      delete d->edit_shop;
      d->edit_shop = NULL;
      d->edit_number = 0;
      PLR_FLAGS(d->character).RemoveBit(PLR_EDITING);
      break;
    default:
      send_to_char("That's not a valid choice!\r\n", CH);
      send_to_char("Do you wish to edit it?\r\n", CH);
      break;
    }
    break;
  case SHEDIT_CONFIRM_SAVESTRING:
    switch(*arg) {
    case 'y':
    case 'Y':
      if (!from_ip_zone(d->edit_number)) {
        sprintf(buf,"%s wrote new shop #%ld",
                GET_CHAR_NAME(d->character), d->edit_number);
        mudlog(buf, d->character, LOG_WIZLOG, TRUE);
      }
      if (real_shop(d->edit_number) == -1)
        boot_one_shop(SHOP);
      else
        reboot_shop(real_shop(d->edit_number), SHOP);
      if (!write_shops_to_disk(d->character->player_specials->saved.zonenum))
        send_to_char("There was an error in writing the zone's shops.\r\n", d->character);
      free_shop(d->edit_shop);
      delete d->edit_shop;
      d->edit_shop = NULL;
      d->edit_number = 0;
      d->edit_number2 = 0;
      STATE(d) = CON_PLAYING;
      PLR_FLAGS(d->character).RemoveBit(PLR_EDITING);
      send_to_char("Done.\r\n", d->character);
      break;
    case 'n':
    case 'N':
      send_to_char("Shop not saved, aborting.\r\n", d->character);
      STATE(d) = CON_PLAYING;
      free_shop(SHOP);
      delete d->edit_shop;
      d->edit_shop = NULL;
      d->edit_number = 0;
      d->edit_number2 = 0;
      PLR_FLAGS(d->character).RemoveBit(PLR_EDITING);
      break;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save this shop internally?\r\n", d->character);
      break;
    }
    break;
  case SHEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      d->edit_mode = SHEDIT_CONFIRM_SAVESTRING;
      shedit_parse(d, "y");
      break;
    case 'x':
    case 'X':
      d->edit_mode = SHEDIT_CONFIRM_SAVESTRING;
      shedit_parse(d,"n");
      break;
    case '1':
      send_to_char("Enter vnum of item to sell (-1 to quit): ", CH);
      SHOP->num_producing = 0;
      for (i = 0; i < MAX_SHOP_OBJ; i++)
        SHOP->producing[i] = -1;
      d->edit_mode = SHEDIT_PRODUCING;
      break;
    case '2':
      send_to_char("Enter selling profit: ", CH);
      d->edit_mode = SHEDIT_PROFIT_SELL;
      break;
    case '3':
      send_to_char("Enter buying profit: ", CH);
      d->edit_mode = SHEDIT_PROFIT_BUY;
      break;
    case '4':
      send_to_char("Enter ± cost %: ", CH);
      d->edit_mode = SHEDIT_PERCENTAGE;
      break;
    case '5':
      shedit_buy_type_menu(d);
      break;
    case '6':
      send_to_char("Enter no such item (keeper) message:\r\n", CH);
      d->edit_mode = SHEDIT_NO_SUCH_ITEM1;
      break;
    case '7':
      send_to_char("Enter no such item (player) message:\r\n", CH);
      d->edit_mode = SHEDIT_NO_SUCH_ITEM2;
      break;
    case '8':
      send_to_char("Enter not enough cash (keeper) message:\r\n", CH);
      d->edit_mode = SHEDIT_MISSING_CASH1;
      break;
    case '9':
      send_to_char("Enter not enough cash (player) message:\r\n", CH);
      d->edit_mode = SHEDIT_MISSING_CASH2;
      break;
    case 'a':
    case 'A':
      send_to_char("Enter keeper doesn't buy message:\r\n", CH);
      d->edit_mode = SHEDIT_DO_NOT_BUY;
      break;
    case 'b':
    case 'B':
      send_to_char("Enter player buys item message (use %d for price):\r\n", CH);
      d->edit_mode = SHEDIT_MESSAGE_BUY;
      break;
    case 'c':
    case 'C':
      send_to_char("Enter player sells item message (use %d for price):\r\n", CH);
      d->edit_mode = SHEDIT_MESSAGE_SELL;
      break;
    case 'd':
    case 'D':
      send_to_char("  1) Waits for payment\r\n  2) Sexual Favour\r\n 3) None\r\n"
                   "Enter keeper's temper: ", CH);
      d->edit_mode = SHEDIT_TEMPER;
      break;
    case 'e':
    case 'E':
      shedit_disp_extras(d);
      break;
    case 'f':
    case 'F':
      send_to_char("Enter keeper's vnum: ", CH);
      d->edit_mode = SHEDIT_KEEPER;
      break;
    case 'g':
    case 'G':
      shedit_trade_with(d);
      d->edit_mode = SHEDIT_WITH_WHO;
      break;
    case 'h':
    case 'H':
      send_to_char("Enter vnum of room to sell in (-1 to quit): ", CH);
      SHOP->num_rooms = 0;
      for (i = 0; i < MAX_SHOP_ROOMS; i++)
        SHOP->in_room[i] = -1;
      d->edit_mode = SHEDIT_IN_ROOM;
      break;
    case 'i':
    case 'I':
      send_to_char("Enter first opening time: ", CH);
      d->edit_mode = SHEDIT_OPEN1;
      break;
    case 'j':
    case 'J':
      send_to_char("Enter first closing time: ", CH);
      d->edit_mode = SHEDIT_CLOSE1;
      break;
    case 'k':
    case 'K':
      send_to_char("Enter second opening time: ", CH);
      d->edit_mode = SHEDIT_OPEN2;
      break;
    case 'l':
    case 'L':
      send_to_char("Enter second closing time: ", CH);
      d->edit_mode = SHEDIT_CLOSE2;
      break;
    default:
      shedit_disp_menu(d);
      break;
    }
    break;
  case SHEDIT_PRODUCING:
    number = atoi(arg);

    if (number == -1)
      shedit_disp_menu(d);
    else if (real_object(number) < 0) {
      send_to_char("No such item!  Enter vnum of item to sell (-1 to quit): ", CH);
      return;
    } else {
      SHOP->producing[SHOP->num_producing] = number;
      SHOP->num_producing++;
      if (SHOP->num_producing >= MAX_SHOP_OBJ)
        shedit_disp_menu(d);
      else
        send_to_char("Enter vnum of item to sell (-1 to quit): ", CH);
    }
    break;
  case SHEDIT_PROFIT_SELL:
    profit = atof(arg);
    if (profit < 1.0) {
      send_to_char("Selling profit must be greater than or equal to 1!\r\nEnter selling profit: ", CH);
      return;
    }
    SHOP->profit_buy = profit;
    shedit_disp_menu(d);
    break;
  case SHEDIT_PROFIT_BUY:
    profit = atof(arg);
    if (profit < 0.0 || profit > 1.0) {
      send_to_char("Buying profit must be between 0 and 1 (inclusive)!\r\nEnter buying profit: ", CH);
      return;
    }
    SHOP->profit_sell = profit;
    shedit_disp_menu(d);
    break;
  case SHEDIT_PERCENTAGE:
    number = atoi(arg);
    if (number < 0 || number > MAX_PERCENTAGE)
      send_to_char("Invalid value.  Enter ± cost % (0-10): ", CH);
    else {
      SHOP->percentage = number;
      shedit_disp_menu(d);
    }
    break;
  case SHEDIT_BUY_TYPE_MENU:
    number = atoi(arg);
    found = 0;
    if (number < 0 || number > NUM_ITEM_TYPES) {
      send_to_char("Illegal value!  Item type to buy (0 to quit): ", CH);
      return;
    } else if (number == 0)
      shedit_disp_menu(d);
    else {
      if (SHOP->type[number-1] == 1) {
        SHOP->type[number-1] = 0;
        SHOP->num_buy_types--;
      } else {
        SHOP->type[number-1] = 1;
        SHOP->num_buy_types++;
      }
      shedit_buy_type_menu(d);
    }
    break;
  case SHEDIT_NO_SUCH_ITEM1:
    if (SHOP->no_such_item1)
      delete [] SHOP->no_such_item1;
    SHOP->no_such_item1 = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_NO_SUCH_ITEM2:
    if (SHOP->no_such_item2)
      delete [] SHOP->no_such_item2;
    SHOP->no_such_item2 = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_MISSING_CASH1:
    if (SHOP->missing_cash1)
      delete [] SHOP->missing_cash1;
    SHOP->missing_cash1 = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_MISSING_CASH2:
    if (SHOP->missing_cash2)
      delete [] SHOP->missing_cash2;
    SHOP->missing_cash2 = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_DO_NOT_BUY:
    if (SHOP->do_not_buy)
      delete [] SHOP->do_not_buy;
    SHOP->do_not_buy = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_MESSAGE_BUY:
    if (SHOP->message_buy)
      delete [] SHOP->message_buy;
    SHOP->message_buy = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_MESSAGE_SELL:
    if (SHOP->message_sell)
      delete [] SHOP->message_sell;
    SHOP->message_sell = str_dup(arg);
    shedit_disp_menu(d);
    break;
  case SHEDIT_TEMPER:
    number = atoi(arg);
    if (number < 1 || number > 3) {
      send_to_char("Value must be 1, 2, or 3.  Enter keeper's temper: ", CH);
      return;
    }
    number--;
    SHOP->temper = number;
    shedit_disp_menu(d);
    break;
  case SHEDIT_BITVECTOR:
    number = atoi(arg);
    if (!number) {
      shedit_disp_menu(d);
      return;
    } else if (number < 1 || number > NUM_SHOP_EXTRAS) {
      shedit_disp_extras(d);
      return;
    }
    if (IS_SET(SHOP->bitvector, 1 << (number - 1)))
      REMOVE_BIT(SHOP->bitvector, 1 << (number - 1));
    else
      SET_BIT(SHOP->bitvector, 1 << (number - 1));
    shedit_disp_extras(d);
    break;
  case SHEDIT_KEEPER:
    number = atoi(arg);
    if (real_mobile(number) < 0) {
      send_to_char("No such mob!  Enter vnum of keeper: ", CH);
      return;
    } else {
      SHOP->keeper = number;
      shedit_disp_menu(d);
    }
    break;
  case SHEDIT_WITH_WHO:
    number = atoi(arg);
    if (number < 0 || number > 5)
      shedit_trade_with(d);
    else {
      if (number == 0)
        shedit_disp_menu(d);
      else {
        if (IS_SET(SHOP->with_who, 1 << (number - 1)))
          REMOVE_BIT(SHOP->with_who, 1 << (number - 1));
        else
          SET_BIT(SHOP->with_who, 1 << (number - 1));
        shedit_trade_with(d);
      }
    }
    break;
  case SHEDIT_IN_ROOM:
    number = atoi(arg);

    if (number == -1)
      shedit_disp_menu(d);
    else if (real_room(number) < 0) {
      send_to_char("No such room!  Enter vnum of room to sell in (-1 to quit): ", CH);
      return;
    } else {
      SHOP->in_room[SHOP->num_rooms] = number;
      SHOP->num_rooms++;
      if (SHOP->num_rooms >= MAX_SHOP_ROOMS)
        shedit_disp_menu(d);
      else
        send_to_char("Enter vnum of room to sell in (-1 to quit): ", CH);
    }
    break;
  case SHEDIT_OPEN1:
    number = atoi(arg);
    if (number < 0 || number > 24) {
      send_to_char("Time must be between 0 and 24.  Enter first opening time: ", CH);
      return;
    }
    SHOP->open1 = number;
    shedit_disp_menu(d);
    break;
  case SHEDIT_CLOSE1:
    number = atoi(arg);
    if (number < 0 || number > 24) {
      send_to_char("Time must be between 0 and 24.  Enter first closing time: ", CH);
      return;
    }
    SHOP->close1 = number;
    shedit_disp_menu(d);
    break;
  case SHEDIT_OPEN2:
    number = atoi(arg);
    if (number < 0 || number > 24) {
      send_to_char("Time must be between 0 and 24.  Enter second openingtime: ", CH);
      return;
    }
    SHOP->open2 = number;
    shedit_disp_menu(d);
    break;
  case SHEDIT_CLOSE2:
    number = atoi(arg);
    if (number < 0 || number > 24) {
      send_to_char("Time must be between 0 and 24.  Enter second closing time: ", CH);
      return;
    }
    SHOP->close2 = number;
    shedit_disp_menu(d);
    break;
  }
}

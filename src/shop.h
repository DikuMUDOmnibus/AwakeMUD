
#ifndef _shop_h_
#define _shop_h_

struct shop_data
{
  vnum_t vnum;                    /* Virtual number of this shop          */
  long num_producing,        /* Number of items shops produces       */
  num_buy_types,        /* Number of buy types                  */
  num_rooms;            /* Number of rooms                      */
  long *producing;              /* Which item to produce (virtual)      */
  long *in_room;                /* Where is the shop?                   */
  int *type;                   /* Which items to trade                 */
  float profit_buy;            /* Factor to multiply cost with         */
  float profit_sell;           /* Factor to multiply cost with         */
  sh_int percentage;           /* Daily random +/- cost % multiplier   */
  sh_int curcent;              /* shop's current %                     */
  char *no_such_item1;         /* Message if keeper hasn't got an item */
  char *no_such_item2;         /* Message if player hasn't got an item */
  char *missing_cash1;         /* Message if keeper hasn't got cash    */
  char *missing_cash2;         /* Message if player hasn't got cash    */
  char *do_not_buy;            /* If keeper dosn't buy such things     */
  char *message_buy;           /* Message when player buys item        */
  char *message_sell;          /* Message when player sells item       */
  int temper;                  /* How does keeper react if no money    */
  int bitvector;               /* Can attack? Use bank? Cast here?     */
  long keeper;                  /* The mobil who owns the shop (virtual)*/
  int with_who;                /* Who does the shop trade with?        */
  sh_int open1, open2;         /* When does the shop open?             */
  sh_int close1, close2;       /* When does the shop close?            */
  int bankAccount;             /* Store all gold over 15000 (disabled) */
  int lastsort;                /* How many items are sorted in inven?  */
  int regen_amount;            /* How much should the shop regen to?   */
  long lasttime;               /* When was the shop last regenned      */

  shop_data() :
      producing(NULL), in_room(NULL), type(NULL), no_such_item1(NULL),
      no_such_item2(NULL), missing_cash1(NULL), missing_cash2(NULL),
      do_not_buy(NULL), message_buy(NULL), message_sell(NULL)
  {}
}
;

#define MAX_TRADE       5       /* List maximums for compatibility      */
#define MAX_SHOP_OBJ    25      /* "Soft" maximum for list maximums     */
#define MAX_SHOP_ROOMS  25      /* max for shop rooms                   */
#define MAX_PERCENTAGE  10

/* Pretty general macros that could be used elsewhere */
#define IS_GOD(ch)              (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_BUILDER))
#define GET_OBJ_NUM(obj)        (obj->item_number)
#define END_OF(buffer)          ((buffer) + strlen((buffer)))

/* Possible states for objects trying to be sold */
#define OBJECT_DEAD             0
#define OBJECT_NOTOK            1
#define OBJECT_OK               2

/* Whom will we not trade with (bitvector for SHOP_TRADE_WITH()) */
#define TRADE_HUMAN               1
#define TRADE_ELF                 2
#define TRADE_DWARF               4
#define TRADE_ORK                 8
#define TRADE_TROLL              16
#define TRADE_DRAGON              32

#define SHOP_NUM(i)             (shop_table[(i)].vnum)
#define SHOP_KEEPER(i)          (shop_table[(i)].keeper)
#define SHOP_OPEN1(i)           (shop_table[(i)].open1)
#define SHOP_CLOSE1(i)          (shop_table[(i)].close1)
#define SHOP_OPEN2(i)           (shop_table[(i)].open2)
#define SHOP_CLOSE2(i)          (shop_table[(i)].close2)
#define SHOP_ROOM(i, num)       (shop_table[(i)].in_room[(num)])
#define SHOP_BUYTYPE(i, num)    (shop_table[(i)].type[(num)])
#define SHOP_PRODUCT(i, num)    (shop_table[(i)].producing[(num)])
#define SHOP_BANK(i)            (shop_table[(i)].bankAccount)
#define SHOP_TEMPER(i)          (shop_table[(i)].temper)
#define SHOP_BITVECTOR(i)       (shop_table[(i)].bitvector)
#define SHOP_TRADE_WITH(i)      (shop_table[(i)].with_who)
#define SHOP_SORT(i)            (shop_table[(i)].lastsort)
#define SHOP_BUYPROFIT(i)       (shop_table[(i)].profit_buy)
#define SHOP_SELLPROFIT(i)      (shop_table[(i)].profit_sell)
#define SHOP_PERCENTAGE(i)      (shop_table[(i)].percentage)
#define SHOP_CURCENT(i)         (shop_table[(i)].curcent)
#define SHOP_REGEN_AMOUNT(i)    (shop_table[(i)].regen_amount)
#define SHOP_LASTTIME(i)        (shop_table[(i)].lasttime)


#define NOTRADE_HUMAN(i)        (IS_SET(SHOP_TRADE_WITH((i)), TRADE_HUMAN))
#define NOTRADE_ELF(i)          (IS_SET(SHOP_TRADE_WITH((i)), TRADE_ELF))
#define NOTRADE_DWARF(i)        (IS_SET(SHOP_TRADE_WITH((i)), TRADE_DWARF))
#define NOTRADE_ORK(i)          (IS_SET(SHOP_TRADE_WITH((i)), TRADE_ORK))
#define NOTRADE_TROLL(i)        (IS_SET(SHOP_TRADE_WITH((i)), TRADE_TROLL))
#define NOTRADE_DRAGON(i)        (IS_SET(SHOP_TRADE_WITH((i)), TRADE_DRAGON))

#define EXTRA_WILL_FIGHT        1
#define EXTRA_USES_BANK         2
#define EXTRA_DOCTOR            4
#define EXTRA_NO_NEGOTIATE      8
#define EXTRA_CASH_ONLY         16
#define EXTRA_CRED_ONLY         32

#define NUM_SHOP_EXTRAS         6

#define SHOP_KILL_CHARS(i)      (IS_SET(SHOP_BITVECTOR(i), EXTRA_WILL_FIGHT))
#define SHOP_USES_BANK(i)       (IS_SET(SHOP_BITVECTOR(i), EXTRA_USES_BANK))
#define IS_DOCTOR(i)            (IS_SET(SHOP_BITVECTOR(i), EXTRA_DOCTOR))
#define WONT_NEGOTIATE(i)       (IS_SET(SHOP_BITVECTOR(i), EXTRA_NO_NEGOTIATE))
#define IS_CASH_ONLY(i)         (IS_SET(SHOP_BITVECTOR(i), EXTRA_CASH_ONLY))
#define IS_CRED_ONLY(i)         (IS_SET(SHOP_BITVECTOR(i), EXTRA_CRED_ONLY))

#define MIN_OUTSIDE_BANK        5000
#define MAX_OUTSIDE_BANK        15000

#define MSG_NOT_OPEN_YET        "Come back later!"
#define MSG_NOT_REOPEN_YET      "Sorry, we have closed, but come back later."
#define MSG_CLOSED_FOR_DAY      "Sorry, come back tomorrow."
#define MSG_NO_STEAL_HERE       "$n is a bloody thief!"
#define MSG_NO_SEE_CHAR         "I don't trade with someone I can't see!"
#define MSG_NO_SELL             "We don't serve your kind here!"
#define MSG_NO_USED_WANDSTAFF   "I don't buy used up wands or staves!"
#define MSG_CANT_KILL_KEEPER    "Get out of here before I call the guards!"

#endif

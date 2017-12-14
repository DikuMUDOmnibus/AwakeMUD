/* *************************************************************************/
/*   File: structs.h                                     Part of CircleMUD */
/*  Usage: header file for central structures and contstants               */
/*                                                                         */
/*  All rights reserved.  See license.doc for complete information.        */
/*                                                                         */
/*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University */
/*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               */
/************************************************************************ */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <sys/types.h>
#include "types.h"
#include "awake.h"
#include "list.h"
#include "bitfield.h"

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)
#define WSPEC(name) \
   int (name)(struct char_data *ch, struct char_data *vict, struct obj_data \
              *weapon, int dam)

/***********************************************************************
 * Structures                                                          *
 **********************************************************************/

#define NOWHERE (-1)
#define NOTHING (-1)
#define NOBODY  (-1)

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data
{
  char *keyword;                 /* Keyword in look/examine          */
  char *description;             /* What to see                      */
  struct extra_descr_data *next; /* Next in list                     */

  extra_descr_data() :
      keyword(NULL), description(NULL), next(NULL)
  {}
}
;

struct text_data
{
  char *keywords;
  char *name;
  char *room_desc;
  char *look_desc;

  text_data() :
      keywords(NULL), name(NULL), room_desc(NULL), look_desc(NULL)
  {}
}
;

/* object-related structures ******************************************/

/* object flags; used in obj_data */
struct obj_flag_data
{
  int value[10];       /* Values of the item (see list)      */
  byte type_flag;      /* Type of item                       */
  Bitfield wear_flags;  /* Where you can wear it              */
  Bitfield extra_flags;  /* If it hums, glows, etc.            */
  float weight;          /* Weigt what else                    */
  int cost;            /* Value when sold (gp.)              */
  int timer;           /* Timer for object                   */
  Bitfield bitvector;       /* To set chars bits                  */
  byte material;       /* Type of material item is made from */
  byte barrier;        // barrier rating of object if any
  byte condition;      // current barrier rating of the object
  int availtn;
  float availdays;
  long quest_id;
};

/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type
{
  byte location;      /* Which ability to change (APPLY_XXX) */
  sbyte modifier;     /* How much it changes by              */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data
{
  rnum_t item_number;         /* Where in data-base                   */
  rnum_t in_room;            /* In what room -1 when conta/carr      */
  struct veh_data *in_veh;

  struct obj_flag_data obj_flags; /* Object information                 */
  struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

  text_data text;
  struct extra_descr_data *ex_description; /* extra descriptions     */
  char *restring;
  char *photo;

  struct char_data *carried_by;   /* Carried by :NULL in room/conta   */
  struct char_data *worn_by;      /* Worn by?                         */
  sh_int worn_on;                 /* Worn where?                      */

  struct obj_data *in_obj;        /* In what object NULL when none    */
  struct obj_data *contains;      /* Contains objects                 */
  struct obj_data *next_content;  /* For 'contains' lists             */
  
  struct char_data *targ;	  /* Data for mounts */
  struct veh_data *tveh;
  obj_data() :
      ex_description(NULL), restring(NULL), photo(NULL), carried_by(NULL), worn_by(NULL),
      in_obj(NULL), contains(NULL), next_content(NULL), targ(NULL), tveh(NULL)
  {}
};
/* ======================================================================= */

/* room-related structures ************************************************/

struct room_direction_data
{
  char *general_description;       /* When look DIR.                   */

  char *keyword;               /* for open/close                       */

  sh_int exit_info;            /* Exit info                            */
  vnum_t key;                 /* Key's number (-1 for no key)         */
  rnum_t to_room;            /* Where direction leads (NOWHERE)      */
  sh_int key_level;            /* Level of electronic lock             */

  byte hidden;                 /* target number for exit if hidden     */
  byte material;               /* material                             */
  byte barrier;                /* barrier rating                       */
  byte condition;      // current barrier rating
  vnum_t to_room_vnum;       /* the vnum of the room. Used for OLC   */

  room_direction_data() :
      general_description(NULL), keyword(NULL)
  {}
}
;

/* ================== Memory Structure for room ======================= */
struct room_data
{
  vnum_t number;             /* Rooms number (vnum)                */
  int zone;                 /* Room zone (for resetting)          */
  int  sector_type;            /* sector type (move/hide)            */
  char *name;                  /* Rooms name 'You are ...'           */
  char *description;           /* Shown when entered                 */
  struct extra_descr_data *ex_description; /* for examine/look       */
  struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
  vnum_t matrix;		/* Matrix exit */
  int access;
  int io;
  int trace;
  int bandwidth;
  vnum_t rtg;
  int jacknumber;
  char *address;
  Bitfield room_flags;     /* DEATH,DARK ... etc                 */
  byte blood;         /* mmmm blood, addded by root        */
  byte spec;            // auto-assigns specs
  sbyte rating;                 // rating of room for various things

  byte light;                  /* Number of lightsources in room     */
  byte peaceful;
  SPECIAL(*func);

  struct obj_data *contents;   /* List of items in room              */
  struct char_data *people;    /* List of NPC / PC in room           */
  struct veh_data *vehicles;


  room_data() :
      name(NULL), description(NULL), ex_description(NULL), matrix(0), access(0), io(0), trace(0),
      bandwidth(0), jacknumber(0), address(NULL), peaceful(0), func(NULL), contents(NULL),
      people(NULL), vehicles(NULL)
  {}
}
;
/* ====================================================================== */

/* char-related structures ************************************************/

typedef struct spell_data
{
  char *name;                           // spell name
  sh_int physical;                      // is spell mana or physical spell
  u_char category;                      // category of spell
  u_char force;                         // force of spell
  sh_int target;                        // spell targets
  sh_int drain;                         // drain addition and level of spell
  sh_int damage;                        // damage level of spell
  sh_int type;                          // type for determing spell mesg
  sh_int effect;                        // element, etc
  struct spell_data *next;              // pointer to next spell in list

  spell_data() :
      name(NULL), next(NULL)
  {}
}
spell_t;

typedef struct train_data
{
  int vnum;
  int attribs;
  byte is_newbie;
}
train_t;

typedef struct teach_data
{
  int vnum;
  sh_int s1, s2, s3, s4, s5, s6, s7, s8;
  char *msg;
  sh_int type;
}
teach_t;

typedef struct adept_data
{
  int vnum;
  sh_int skills[ADEPT_NUMPOWER];
  byte is_newbie;
}
adept_t;

typedef struct master_data
{
  int vnum;
  sh_int spell1, force1;
  sh_int spell2, force2;
  byte is_hermetic;
  byte is_newbie;
}
master_t;

/* memory structure for characters */
struct memory_rec_struct
{
  long id;
  struct memory_rec_struct *next;

  memory_rec_struct() :
      next(NULL)
  {}
}
;

typedef struct memory_rec_struct memory_rec;

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data
{
  byte minute, hours, day, month, weekday;
  sh_int year;
};

/* These data contain information about a players time data */
struct time_data
{
  time_t birth;    /* This represents the characters age                */
  time_t logon;    /* Time of the last logon (used to calculate played) */
  int  played;     /* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data
{
  char *char_name;
  char passwd[MAX_PWD_LENGTH+1];     /* character's password         */

  text_data physical_text;
  text_data astral_text;
  text_data matrix_text;

  char *title;               /* PC / NPC's title                     */
  char *pretitle, *whotitle; /* PC's pre/whotitles                   */
  char *prompt;              /* PC's customized prompt               */
  char *poofin, *poofout;    /* PC's poofin/poofout                  */

  byte sex;                  /* PC / NPC's sex                       */
  byte level;
  long last_room;              /* PC s Hometown (zone)                 */
  struct time_data time;     /* PC's AGE in days                     */
  u_short weight;              /* PC / NPC's weight                    */
  u_short height;              /* PC / NPC's height                    */
  byte race;                 /* PC / NPC's race                      */
  byte tradition;            /* PC / NPC's tradition                       */
  char *host;   /* player host    */

  char_player_data() :
      char_name(NULL), title(NULL), pretitle(NULL), whotitle(NULL),
      prompt(NULL), poofin(NULL), poofout(NULL), tradition(2), host(NULL)
  {}
}
;

/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data
{
  sbyte attributes[7];
  sh_int mag;
  sh_int rmag;
  sh_int bod_index;
  sh_int ess;
  sh_int astral_pool;
  sh_int defense_pool;
  sh_int combat_pool;
  sh_int hacking_pool;
  sh_int hacking_pool_max;
  sh_int hacking_pool_remaining;
  sh_int magic_pool;
  sh_int offense_pool;
  sh_int control_pool;
};

/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data
{
  sh_int mental;
  sh_int max_mental;   /* Max move for PC/NPC                     */
  sh_int physical;
  sh_int max_physical; /* Max hit for PC/NPC                      */

  sh_int ballistic[2];    /* Ballistic armor class for bullets and such */
  sh_int impact[2];       /* Impact armor class for clubs and such   */
  int nuyen;           /* Money carried */
  int bank;           /* Nuyen the char has in a bank account */
  int karma;            /* The experience of the player */
  int rep;              /* the reputation of the player  (karma earned via runs) */
  int noto;              /* the reputation of the player  (karma earned via kills) */
  int tke;              /* the reputation of the player  (karma earned total) */
  sbyte init_dice;     /* Bonuses for initiative dice             */
  sbyte init_roll;     /* Total of init roll                      */
  int grade;      /* Initiate Grade        */
  int extrapp;
  ubyte sustained;     /* total of sustained spells               */
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved
{
  int powerpoints;
  byte left_handed;
  byte wielded[2];
  int cur_lang;         /* # of language char is currently speaking */
  Bitfield act;    /* act flag for NPC's; player flag for PC's */

  Bitfield affected_by;        /* Bitvector for spells/skills affected by */
  byte skills[MAX_SKILLS+1][2];   /* array of skills plus skill 0         */
  byte powers[ADEPT_NUMPOWER+1];
  int boosted[3][2];           /* str/qui/bod timeleft/amount		*/
};

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data
{
  struct veh_data *fight_veh;
  struct char_data *fighting;  /* Opponent                             */
  struct char_data *hunting;   /* Char hunted by this char             */
  struct obj_data *programming; /* Program char is currently designing/programming */
  struct spell_data *spell_q;  /* queue of spells                      */

  int idnum;

  byte position;               /* Standing, fighting, sleeping, etc.   */
  char *defined_position;
  int target_mod;

  float carry_weight;           /* Carried weight                       */
  byte carry_items;            /* Number of items carried              */
  sh_int foci;
  sh_int last_healed;
  int timer;                  /* Timer for update                     */
  int mag_create;             /* Variable for spell creation menus    */
  int spell_create;           /* Variable for current spell being created */
  struct veh_data *subscribe;   /* subscriber list */
  struct veh_data *rigging;     /* Vehicle char is controlling remotely */

  struct char_special_data_saved saved; /* constants saved in plrfile  */

  char_special_data() :
      fight_veh(NULL), fighting(NULL), hunting(NULL), spell_q(NULL), defined_position(NULL),
      subscribe(NULL), rigging(NULL)
  {}
}
;

/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved
{
  int wimp_level;              /* Below this # of hit points, flee!    */
  byte freeze_level;           /* Level of god who froze char, if any  */
  sh_int invis_level;          /* level of invisibility                */
  sh_int incog_level;          /* level of incognito         */
  vnum_t load_room;          /* Which room to place char in          */
  vnum_t last_in;            /* Room to start them in, if 2 hours isn't up */
  Bitfield pref;        /* preference flags for PC's.           */
  ubyte bad_pws;               /* number of bad password attemps       */
  sbyte conditions[3];         /* Drunk, full, thirsty                 */

  ubyte totem;                 /* totem player chooses                 */
  int att_points;              /* attrib points for when you first create */
  int skill_points;            /* starting skill points                   */
  int zonenum;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data
{
  struct player_special_data_saved saved;

  struct alias *aliases;       /* Character's aliases                  */
  struct remem *remem;         /* Character's Remembers          */
  char *gname;
  long last_tell;              /* idnum of last tell from              */
  sh_int  questnum;
  sh_int *obj_complete;
  sh_int *mob_complete;
  int last_quest[QUEST_TIMER];

  player_special_data() :
      aliases(NULL), remem(NULL), gname(NULL), last_tell(0),
      questnum(0), obj_complete(NULL), mob_complete(NULL)
  {}
}
;

/* Specials used by NPCs, not PCs */
struct mob_special_data
{
  byte last_direction;     /* The last direction the monster went     */
  int attack_type;         /* The Attack Type Bitvector for NPC's     */
  byte default_pos;        /* Default position for NPC                */
  int active;
  memory_rec *memory;      /* List of attackers to remember           */
  sh_int mob_skills[10];   /* loaded up skills, I gues this is acceptable */
  int wait_state;          /* Wait state for bashed mobs              */
  char *leave;             // leave keywords 'mob flies south'
  char *arrive;            // arrive keywords
  int quest_id;
  int value_death_nuyen;
  int value_death_items;
  int value_death_karma;
  int count_death;
  vnum_t spare1, spare2;

  mob_special_data() :
      memory(NULL), leave(NULL), arrive(NULL)
  {}
}
;

/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type
{
  bool caster;          /* caster or receiver of spell             */
  sh_int type;          /* The type of spell that caused this      */
  sh_int duration;      /* For how long its effects will last      */
  sbyte modifier;       /* This is added to apropriate ability     */
  byte location;        /* Tells which ability to change(APPLY_XXX)*/
  Bitfield bitvector;   /* Tells which bits to set (AFF_XXX)       */
  long sustained_by;    /* id number of who is sustaining spell    */

  struct affected_type *next;

  affected_type() :
      caster(0), type(0), duration(0), modifier(0), location(0),
      sustained_by(0), next(NULL)
  {}
}
;

struct veh_follow
{
  struct veh_data *follower;
  struct veh_follow *next;

  veh_follow() :
      follower(NULL), next(NULL)
  {}
};

/* Structure used for chars following other chars */
struct follow_type
{
  struct char_data *follower;
  struct follow_type *next;

  follow_type() :
      follower(NULL), next(NULL)
  {}
};

struct grid_data 
{
  char *name;
  vnum_t room;
  struct grid_data *next;

  grid_data() :
      name(NULL), next(NULL)
  {}
};
struct veh_data
{
  vnum_t veh_number;         /* Where in data-base                   */
  rnum_t in_room;            /* In what room -1 when conta/carr      */

  char *name;
  char *description;              /* When in room (without driver)    */
  char *short_description;        /* when worn/carry/in cont.         */
  char *long_description;         /* long description when examined   */
  char *inside_description;       /*  Description inside the car      */
  sbyte handling;
  int speed;
  sbyte accel;
  sbyte body;
  sbyte armor;
  sbyte sig;
  sbyte autonav;
  sbyte seating;
  float load;
  sbyte sensor;
  sbyte pilot;
  int cost;

  sbyte type;
  int damage;
  sh_int cspeed;

  struct veh_follow *followers;
  struct veh_data *following;
  struct char_data *followch;
  long lastin[3];

  struct obj_data *mount;
  struct obj_data *mod[NUM_MODS];
  bool sub;

  long owner;
  bool locked;
  vnum_t dest;
  Bitfield flags;

  struct obj_data *contents;   /* List of items in room */
  struct char_data *people;    /* List of NPC / PC in room */
  struct char_data *rigger;
  struct char_data *fighting;
  struct veh_data *fight_veh;
  struct veh_data *next_veh;
  struct veh_data *next_sub;
  struct grid_data *grid;
  char *leave;             /* leave keywords 'mob flies south' */
  char *arrive;            /* arrive keywords */

  struct veh_data *next;
  veh_data() :
      name(NULL), description(NULL), short_description(NULL),
      long_description(NULL), inside_description(NULL), followers(NULL),
      following(NULL), followch(NULL), contents(NULL), people(NULL), 
      fighting(NULL), fight_veh(NULL), next_veh(NULL), next_sub(NULL), 
      leave(NULL), arrive(NULL)
  {}
}
;

/* ================== Structure for player/non-player ===================== */
struct char_data
{
  long nr;                            /* Mob's rnum                    */
  // the previous will be DEFUNCT once MobIndex is written
  rnum_t in_room;                     /* Location (real room number)   */
  rnum_t was_in_room;                 /* location for linkdead people  */

  struct char_player_data player;       /* Normal data                   */
  struct char_ability_data real_abils;  /* Abilities without modifiers   */
  struct char_ability_data aff_abils;   /* Abils with spells/stones/etc  */
  struct char_point_data points;        /* Points                        */
  struct char_special_data char_specials;      /* PC/NPC specials        */
  struct player_special_data *player_specials; /* PC specials            */
  struct mob_special_data mob_specials;        /* NPC specials           */
  struct veh_data *in_veh;
  struct matrix_icon *persona;

  struct affected_type *affected;       /* affected by what spells       */
  struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

  struct obj_data *carrying;            /* Head of list                  */
  struct descriptor_data *desc;         /* NULL for mobiles              */
  struct obj_data *cyberware;           /* Head of list of cyberware     */
  struct obj_data *bioware;             /* Head of list of bioware       */

  struct char_data *next_in_room;     /* For room->people - list         */
  struct char_data *next;             /* For either monster or ppl-list  */
  struct char_data *next_fighting;    /* For fighting list               */
  struct char_data *next_in_zone;     /* for zone->people - list         */
  struct char_data *next_in_veh;      /* For veh->people - list          */

  struct follow_type *followers;        /* List of chars followers       */
  struct char_data *master;             /* Who is char following?        */
  spell_t *spells;                     /* linked list of spells          */


  char_data() :
      player_specials(NULL), in_veh(NULL), persona(NULL), affected(NULL), carrying(NULL),
      desc(NULL),
      cyberware(NULL), bioware(NULL), next_in_room(NULL), next(NULL),
      next_fighting(NULL), next_in_zone(NULL), next_in_veh(NULL), followers(NULL),
      master(NULL), spells(NULL)
  {}
};
/* ====================================================================== */

/* descriptor-related structures ******************************************/

struct txt_block
{
  char *text;
  int aliased;
  struct txt_block *next;

  txt_block() :
      text(NULL), next(NULL)
  {}
}
;

struct txt_q
{
  struct txt_block *head;
  struct txt_block *tail;

  txt_q() :
      head(NULL), tail(NULL)
  {}
}
;

struct ccreate_t
{
  sh_int mode;
  sh_int archetype;
  sh_int pr[5];
  sh_int force_points;
  sh_int temp;
};

struct descriptor_data
{
  int descriptor;                         /* file descriptor for socket         */
  int ident_sock;               /* socket used for ident process        */
  u_short peer_port;            /* port of peer                         */
  char host[HOST_LENGTH+1];       /* hostname                           */
  byte bad_pws;                   /* number of bad pw attemps this login        */
  byte idle_tics;               /* tics idle at password prompt         */
  byte invalid_name;              /* number of invalid name attempts    */
  int connected;                          /* mode of 'connectedness'            */
  int wait;                               /* wait for how many loops            */
  int desc_num;                   /* unique num assigned to desc                */
  time_t login_time;              /* when the person connected          */
  char **str;                     /* for the modify-str system          */
  int   max_str;                        /*              -                       */
  long mail_to;                 /* name for mail system                 */
  int   prompt_mode;              /* control of prompt-printing         */
  char inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input            */
  char last_input[MAX_INPUT_LENGTH]; /* the last input                  */
  char last_tell[MAX_INPUT_LENGTH];
  char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer          */
  char *output;                 /* ptr to the current output buffer     */
  int bufptr;                     /* ptr to end of current output               */
  int bufspace;                 /* space left in the output buffer      */
  struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
  struct txt_q input;             /* q of unprocessed input             */
  struct char_data *character;  /* linked to char                       */
  struct char_data *original;   /* original char if switched            */
  struct descriptor_data *snooping; /* Who is this char snooping        */
  struct descriptor_data *snoop_by; /* And who is snooping this char    */
  struct descriptor_data *next; /* link to next descriptor              */
  struct ccreate_t ccr;

  // all this from here down is stuff for on-line creation
  int edit_mode;                /* editing sub mode */
  long edit_number;              /* virtual num of thing being edited */
  long edit_number2;             /* misc number for editing */
  int edit_zone;                /* which zone object is part of      */
  void **misc_data;             /* misc data, usually for extra data crap */
  struct obj_data *edit_obj;    /* iedit */
  struct room_data *edit_room;  /* redit */
  struct char_data *edit_mob;   /* medit */
  struct quest_data *edit_quest;/* qedit */
  struct shop_data *edit_shop;  /* sedit */
  struct zone_data *edit_zon;   /* zedit */
  struct reset_com *edit_cmd;   /* zedit command */
  struct veh_data *edit_veh;    /* vedit */
  struct host_data *edit_host;  /* hedit */
  struct matrix_icon *edit_icon; /* icedit */
  // this is for spell creation
  spell_t *edit_spell;            /* spell creation */

  descriptor_data() :
      output(NULL),
      large_outbuf(NULL), character(NULL), original(NULL), snooping(NULL),
      snoop_by(NULL), next(NULL), edit_obj(NULL), edit_room(NULL),
      edit_mob(NULL), edit_quest(NULL), edit_shop(NULL), edit_zon(NULL),
      edit_cmd(NULL), edit_spell(NULL)
  {}
}
;


/* other miscellaneous structures ***************************************/

struct msg_type
{
  char *attacker_msg;  /* message to attacker */
  char *victim_msg;    /* message to victim   */
  char *room_msg;      /* message to room     */

  msg_type() :
      attacker_msg(NULL), victim_msg(NULL), room_msg(NULL)
  {}
}
;

struct message_type
{
  struct msg_type die_msg;       /* messages when death                  */
  struct msg_type miss_msg;      /* messages when miss                   */
  struct msg_type hit_msg;       /* messages when hit                    */
  struct msg_type god_msg;       /* messages when hit on god             */
  struct message_type *next;     /* to next messages of this kind.       */
};

struct message_list
{
  int a_type;                    /* Attack type                          */
  int number_of_attacks;         /* How many attack messages to chose from. */
  struct message_type *msg;      /* List of messages.                    */
};

struct str_app_type
{
  sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
  sh_int todam;    /* Damage Bonus/Penalty                */
  sh_int carry_w;  /* Maximum weight that can be carrried */
  sh_int wield_w;  /* Maximum weight that can be wielded  */
};

struct weather_data
{
  int pressure;       /* How is the pressure ( Mb ) */
  int change;         /* How fast and what way does it change. */
  int sky;            /* How is the sky. */
  int sunlight;       /* And how much sun. */
  int moonphase;      /* What is the Moon Phae */
  int lastrain;       /* cycles since last rain */
};

/* element in monster and object index-tables   */
struct index_data
{
  vnum_t   vnum;    /* virtual number of this mob/obj           */
  long   number;     /* number of existing units of this mob/obj */
  SPECIAL(*func);
  SPECIAL(*sfunc);
  WSPEC(*wfunc);

  index_data() :
      func(NULL), sfunc(NULL), wfunc(NULL)
  {}
}
;

struct remem
{
  long idnum;
  char *mem;
  struct remem *next;

};

struct phone_data
{
  int number;
  bool connected;
  vnum_t rtg;
  struct phone_data *dest;  
  struct obj_data *phone;
  struct matrix_icon *persona;
  struct phone_data *next;
  phone_data() :
      number(0), connected(FALSE), rtg(1102), dest(NULL), phone(NULL), persona(NULL), next(NULL)
  {}
};

struct skill_data {
  char name[50];
  sh_int attribute;
  bool type;
};
#endif

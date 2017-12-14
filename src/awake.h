/* ****************************************
* file: awake.h -- AwakeMUD Header File   *
* A centralized definition file for all   *
* the major defines, etc...               *
* (c)2001 the AwakeMUD Consortium         *
* Code migrated from other header files   *
* (such as spells.h) (c) 1993, 94 by the  *
* trustees of the John Hopkins University *
* and (c) 1990-1991 The DikuMUD group     *
**************************************** */

#ifndef _awake_h_
#define _awake_h_

#if !defined(WIN32) || defined(__CYGWIN__)
using namespace std;
#endif

#define NUM_RESERVED_DESCS      8

# ifdef DEBUG
# define _STLP_DEBUG 1
# else
# undef _STLP_DEBUG
# endif

/* pertaining to vnums/rnums */

#define REAL 0
#define VIRTUAL 1

/* gender */

#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

/* positions */

#define POS_DEAD       0        /* dead                 */
#define POS_MORTALLYW  1        /* mortally wounded     */
#define POS_STUNNED    2        /* stunned              */
#define POS_SLEEPING   3        /* sleeping             */
#define POS_LYING      4
#define POS_RESTING    5        /* resting              */
#define POS_SITTING    6        /* sitting              */
#define POS_FIGHTING   7        /* fighting             */
#define POS_STANDING   8        /* standing             */

/* attribute defines */

#define ATT_BOD         400
#define ATT_QUI         401
#define ATT_STR         402
#define ATT_CHA         403
#define ATT_INT         404
#define ATT_WIL         405
#define ATT_MAG         406
#define ATT_REA         407
#define ATT_ESS         408

/* attributes (mostly for trainers) */
#define BOD		0
#define QUI		1
#define STR		2
#define CHA		3
#define INT		4
#define WIL		5
#define REA		6
#define TBOD             (1 << 0)
#define TQUI             (1 << 1)
#define TSTR             (1 << 2)
#define TCHA             (1 << 3)
#define TINT             (1 << 4)
#define TWIL             (1 << 5)
#define TESS             (1 << 6)
#define TREA             (1 << 7)
#define TMAG             (1 << 8)

/* char and mob related defines */

/* PC archetypes */

#define AT_UNDEFINED    -1
#define AT_MAGE         0
#define AT_SHAMAN       1
#define AT_DECKER       2
#define AT_SAMURAI      3
#define AT_ADEPT        4
#define AT_RIGGER       5
#define NUM_ATS         6  /* This must be the number of archetypes!! */

/* magical traditions */

#define TRAD_HERMETIC   0
#define TRAD_SHAMANIC   1
#define TRAD_MUNDANE    2
#define TRAD_ADEPT      4

/* totems */

#define TOTEM_UNDEFINED         0
#define TOTEM_BEAR              1
#define TOTEM_EAGLE             2
#define TOTEM_RAVEN             3
#define TOTEM_WOLF              4
#define TOTEM_SHARK             5
#define TOTEM_LION              6
#define TOTEM_COYOTE            7
#define TOTEM_GATOR             8
#define TOTEM_OWL               9
#define TOTEM_SNAKE             10
#define TOTEM_RACCOON           11
#define TOTEM_CAT               12
#define TOTEM_DOG               13
#define TOTEM_RAT               14
/* new totems*/
#define TOTEM_BADGER   15
#define TOTEM_BAT    16
#define TOTEM_BOAR    17
#define TOTEM_BULL    18
#define TOTEM_CHEETAH   19
#define TOTEM_COBRA    20
#define TOTEM_CRAB    21
#define TOTEM_CROCODILE   22
#define TOTEM_DOVE    23
#define TOTEM_ELK    24
#define TOTEM_FISH    25
#define TOTEM_FOX    26
#define TOTEM_GECKO    27
#define TOTEM_GOOSE    28
#define TOTEM_HORSE    29
#define TOTEM_HYENA    30
#define TOTEM_JACKAL   31
#define TOTEM_JAGUAR   32
#define TOTEM_LEOPARD   33
#define TOTEM_LIZARD   34
#define TOTEM_MONKEY   35
#define TOTEM_OTTER    37
#define TOTEM_PARROT   38
#define TOTEM_POLECAT   39
#define TOTEM_PRARIEDOG   40
#define TOTEM_PUMA    41
#define TOTEM_SCORPION   42
#define TOTEM_SPIDER   43
#define TOTEM_STAG    44
#define TOTEM_TURTLE   45
#define TOTEM_WHALE    46
#define TOTEM_MOON    47
#define TOTEM_MOUNTAIN   48
#define TOTEM_OAK    49
#define TOTEM_SEA    50
#define TOTEM_STREAM   51
#define TOTEM_SUN    52
#define TOTEM_WIND    53

/* PC races */

#define RACE_UNDEFINED          1
#define RACE_HUMAN              2
#define RACE_DWARF              3
#define RACE_ELF                4
#define RACE_ORK                5
#define RACE_TROLL              6
#define RACE_CYCLOPS  7
#define RACE_KOBOROKURU  8
#define RACE_FOMORI  9
#define RACE_MENEHUNE  10
#define RACE_HOBGOBLIN  11
#define RACE_GIANT  12
#define RACE_GNOME  13
#define RACE_ONI  14
#define RACE_WAKYAMBI  15
#define RACE_OGRE  16
#define RACE_MINOTAUR  17
#define RACE_SATYR  18
#define RACE_NIGHTONE  19
#define RACE_DRYAD  20
#define RACE_DRAGON             21


#define NUM_RACES               21  /* This must be the number of races */

/* level definitions */

enum {
  LVL_MORTAL = 1,
  LVL_BUILDER,
  LVL_ARCHITECT,
  LVL_FIXER,
  LVL_CONSPIRATOR,
  LVL_EXECUTIVE,
  LVL_DEVELOPER,
  LVL_VICEPRES,
  LVL_ADMIN,
  LVL_PRESIDENT,

  LVL_MAX = LVL_PRESIDENT,
  LVL_FREEZE = LVL_EXECUTIVE
};

/* character equipment positions: used as index for char_data.equipment[] */
/* note: don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */

#define WEAR_LIGHT      0
#define WEAR_HEAD       1
#define WEAR_EYES       2
#define WEAR_EAR        3
#define WEAR_EAR2       4
#define WEAR_FACE	5
#define WEAR_NECK_1     6
#define WEAR_NECK_2     7
#define WEAR_BACK       8
#define WEAR_ABOUT      9
#define WEAR_BODY      10
#define WEAR_UNDER     11
#define WEAR_ARMS      12
#define WEAR_LARM      13
#define WEAR_RARM      14
#define WEAR_WRIST_R   15
#define WEAR_WRIST_L   16
#define WEAR_HANDS     17
#define WEAR_WIELD     18
#define WEAR_HOLD      19
#define WEAR_SHIELD    20
#define WEAR_FINGER_R  21
#define WEAR_FINGER_L  22
#define WEAR_FINGER3   23
#define WEAR_FINGER4   24
#define WEAR_FINGER5   25
#define WEAR_FINGER6   26
#define WEAR_FINGER7   27
#define WEAR_FINGER8   28
#define WEAR_BELLY     29
#define WEAR_WAIST     30
#define WEAR_LEGS      31
#define WEAR_LANKLE    32
#define WEAR_RANKLE    33
#define WEAR_SOCK      34
#define WEAR_FEET      35
#define WEAR_PATCH     36      
#define NUM_WEARS      37


// Old wear positions for item file conversios
#define OLD_WEAR_LIGHT      0
#define OLD_WEAR_FINGER_R   1
#define OLD_WEAR_FINGER_L   2
#define OLD_WEAR_NECK_1     3
#define OLD_WEAR_NECK_2     4
#define OLD_WEAR_BODY       5
#define OLD_WEAR_HEAD       6
#define OLD_WEAR_LEGS       7
#define OLD_WEAR_FEET       8
#define OLD_WEAR_HANDS      9
#define OLD_WEAR_ARMS      10
#define OLD_WEAR_SHIELD    11
#define OLD_WEAR_ABOUT     12
#define OLD_WEAR_WAIST     13
#define OLD_WEAR_WRIST_R   14
#define OLD_WEAR_WRIST_L   15
#define OLD_WEAR_WIELD     16
#define OLD_WEAR_HOLD      17
#define OLD_WEAR_EYES      18
#define OLD_WEAR_EAR       19
#define OLD_WEAR_EAR2      20
#define OLD_WEAR_UNDER     21
#define OLD_WEAR_BACK      22
#define OLD_WEAR_LANKLE    23
#define OLD_WEAR_RANKLE    24
#define OLD_WEAR_SOCK      25
#define OLD_WEAR_FINGER3   26
#define OLD_WEAR_FINGER4   27
#define OLD_WEAR_FINGER5   28
#define OLD_WEAR_FINGER6   29
#define OLD_WEAR_FINGER7   30
#define OLD_WEAR_FINGER8   31
#define OLD_WEAR_BELLY     32
#define OLD_WEAR_LARM      33
#define OLD_WEAR_RARM      34
#define OLD_WEAR_PATCH     35    
#define OLD_NUM_WEARS      36


/* player flags: used by char_data.char_specials.act */
#define PLR_KILLER              0  /* Player is a player-killer              */
#define PLR_FROZEN              2  /* Player is frozen                       */
#define PLR_DONTSET             3  /* Don't EVER set (ISNPC bit)             */
#define PLR_NEWBIE              4  /* Player is a newbie still               */
#define PLR_JUST_DIED           5  /* Player just died                       */
#define PLR_CRASH               6  /* Player needs to be crash-saved         */
#define PLR_SITEOK              8  /* Player has been site-cleared           */
#define PLR_NOSHOUT             9  /* Player not allowed to shout/goss       */
#define PLR_NOTITLE             10 /* Player not allowed to set title        */
#define PLR_DELETED             11 /* Player deleted - space reusable        */
#define PLR_NODELETE            12 /* Player shouldn't be deleted            */
#define PLR_PACKING             13 /* Player shouldn't be on wizlist         */
#define PLR_NOSTAT              14 /* Player cannot be statted, etc          */
#define PLR_LOADROOM            15 /* Player uses nonstandard loadroom       */
#define PLR_INVSTART            16 /* Player should enter game wizinvis      */
#define PLR_OLC                 19 /* Player has access to olc commands      */
#define PLR_MATRIX              20 /* Player is in the Matrix                */
#define PLR_PERCEIVE            21 /* player is astrally perceiving          */
#define PLR_PROJECT             22 /* player is astrally projecting          */
#define PLR_SWITCHED            23 /* player is switched to a mob            */
#define PLR_WRITING             24 /* Player writing (board/mail/olc)        */
#define PLR_MAILING             25 /* Player is writing mail                 */
#define PLR_EDITING             26 /* Player is zone editing                 */
#define PLR_SPELL_CREATE        27 /* Player is creating a spell             */
#define PLR_CUSTOMIZE           28 /* Player is customizing persona          */
#define PLR_NOSNOOP             29 /* Player is not snoopable                */
#define PLR_WANTED              30 /* Player wanted by the law      */
#define PLR_NOOOC               31 /* Player is muted from the OOC channel   */
#define PLR_AUTH                32 /* Player needs Auth */
#define PLR_EDCON               33
#define PLR_REMOTE              34
#define PLR_PG          35 /* Player is a power gamer      */
#define PLR_DRIVEBY  36
#define PLR_RPE   37
#define PLR_MAX                 38


/* mobile flags: used by char_data.char_specials.act */

#define MOB_SPEC                0  /* Mob has a callable spec-proc           */
#define MOB_SENTINEL            1  /* Mob should not move                    */
#define MOB_SCAVENGER           2  /* Mob picks up stuff on the ground       */
#define MOB_ISNPC               3  /* (R) Automatically set on all Mobs      */
#define MOB_AWARE               4  /* Mob can't be backstabbed               */
#define MOB_AGGRESSIVE          5  /* Mob hits players in the room           */
#define MOB_STAY_ZONE           6  /* Mob shouldn't wander out of zone       */
#define MOB_WIMPY               7  /* Mob flees if severely injured          */
#define MOB_AGGR_ORK            8  /* auto attack ork PC's                   */
#define MOB_AGGR_ELF            9  /* auto attack elf PC's                   */
#define MOB_AGGR_DWARF          10 /* auto attack dwarf PC's                 */
#define MOB_MEMORY              11 /* remember attackers if attacked         */
#define MOB_HELPER              12 /* attack PCs fighting other NPCs         */
#define MOB_NOCHARM             13 /* Mob can't be charmed                   */
#define MOB_DUAL_NATURE         14 /* mob is dual-natured                    */
#define MOB_IMMEXPLODE          15 /* mob is immune to explosions            */
#define MOB_AGGR_TROLL          16 /* auto attack troll PC's                 */
#define MOB_NOBLIND             17 /* Mob can't be blinded                   */
#define MOB_ASTRAL              18 /* Mob is solely in the astral plane      */
#define MOB_GUARD               19 /* mob carries out security               */
#define MOB_AGGR_HUMAN          20 /* auto attack human PC's                 */
#define MOB_SNIPER              21 /* mob searches area for PCs              */
#define MOB_PRIVATE             22 /* mob cannot be statted                  */
#define MOB_TRACK               23 /* (R) for security routines              */
/* New Flags added by WASHU*/
#define MOB_LONESTAR            24 /* Lone Star Cops  */
#define MOB_KNIGHT_ERRENT       25 /* Knight Errent Security */
#define MOB_SAEDER_KRUPP        26 /* Saeder Krupp*/
#define MOB_TIR_SECURITY        27 /* Tir Security Forces  */
#define MOB_AZTECHNOLOGY        28 /* Azzies                                 */
#define MOB_RENRAKU             29 /* Renraku   */
#define MOB_NOKILL         30 /* Unkillable mob */
/* New Race(s)! -- Khepri is a dumb slut */
#define MOB_AGGR_DRAGON         31 /* auto attack dragon PCs                 */
#define MOB_INANIMATE  32
#define MOB_MAX          33

/* preference flags: used by char_data.player_specials.pref */

#define PRF_PACIFY               0  /* Room descs won't normally be shown  */
#define PRF_COMPACT             1  /* No extra CRLF pair before prompts   */
#define PRF_AUTOEXIT         2  /* Display exits in a room       */
#define PRF_FIGHTGAG         3  /* Gag extra fight messages  */
#define PRF_MOVEGAG             4  /* Gag extra movement messages    */
#define PRF_DEAF                5  /* Can't hear shouts     */
#define PRF_NOTELL              6  /* Can't receive tells     */
#define PRF_NORADIO             7  /* Can't hear radio frequencies     */
#define PRF_NONEWBIE         8  /* Can't hear newbie channel    */
#define PRF_NOREPEAT         9  /* No repetition of comm commands  */
#define PRF_NOWIZ               10 /* Can't hear wizline      */
#define PRF_PKER                11 /* is able to pk/be pked        */
#define PRF_QUEST               12 /* On quest        */
#define PRF_AFK              13 /* Afk   */
#define PRF_COLOR_1             14 /* Color (low bit)   */
#define PRF_COLOR_2             15 /* Color (high bit)    */
#define PRF_NOHASSLE         16 /* Aggr mobs won't attack  */
#define PRF_ROOMFLAGS        17 /* Can see room flags (ROOM_x) */
#define PRF_HOLYLIGHT        18 /* Can see in dark   */
#define PRF_CONNLOG             19 /* Views ConnLog      */
#define PRF_DEATHLOG         20 /* Views DeathLog        */
#define PRF_MISCLOG             21 /* Views MiscLog          */
#define PRF_WIZLOG              22 /* Views WizLog          */
#define PRF_SYSLOG              23 /* Views SysLog         */
#define PRF_ZONELOG             24 /* Views ZoneLog          */
#define PRF_MSP                 25 /* has enabled MSP          */
#define PRF_ROLLS               26 /* sees details on rolls        */
#define PRF_NOOOC               27 /* can't hear ooc channel      */
#define PRF_AUTOINVIS         28 /* to toggle auto-invis for immortals    */
#define PRF_CHEATLOG            29 /* Views CheatLog         */
#define PRF_ASSIST  30 /* auto assist */
#define PRF_BANLOG              31
#define PRF_NORPE  32
#define PRF_NOHIRED 33
#define PRF_GRIDLOG 34
#define PRF_WRECKLOG 35
#define PRF_QUESTOR 36
#define PRF_MAX   37

/* log watch */

#define LOG_CONNLOG        0
#define LOG_DEATHLOG       1
#define LOG_MISCLOG        2
#define LOG_WIZLOG         3
#define LOG_SYSLOG         4
#define LOG_ZONELOG        5
#define LOG_CHEATLOG       6
#define LOG_WIZITEMLOG     7
#define LOG_BANLOG    	   8
#define LOG_GRIDLOG	   9
#define LOG_WRECKLOG	   10

/* player conditions */

#define DRUNK        0
#define FULL         1
#define THIRST       2

/* affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */

#define AFF_INVISIBLE        1  /* Char is invisible        */
#define AFF_DETECT_ALIGN        2  /* Char is sensitive to align      */
#define AFF_DETECT_INVIS        3  /* Char can see invis chars    */
#define AFF_DETECT_MAGIC        4  /* Char is sensitive to magic  */
#define AFF_SENSE_LIFE        5  /* Char can sense hidden life    */
#define AFF_WATERWALK        6  /* Char can walk on water       */
#define AFF_GROUP               7  /* (R) Char is grouped       */
#define AFF_CURSE               8  /* Char is cursed       */
#define AFF_INFRAVISION      9  /* Char can see in dark        */
#define AFF_POISON              10 /* (R) Char is poisoned        */
#define AFF_SLEEP               11 /* (R) Char magically asleep       */
#define AFF_NOTRACK             12 /* Char can't be tracked            */
#define AFF_LOW_LIGHT        13 /* Char has low light eyes       */
#define AFF_LASER_SIGHT      14 /* Char using laser sight       */
#define AFF_SNEAK               15 /* Char can move quietly           */
#define AFF_HIDE                16 /* Char is hidden                */
#define AFF_VISION_MAG_1        17 /* Magnification level 1        */
#define AFF_CHARM               18 /* Char is charmed              */
#define AFF_ACTION              19 /* Player gets -10 on next init roll     */
#define AFF_VISION_MAG_2        20 /* Magnification level 2         */
#define AFF_VISION_MAG_3        21 /* Magnification level 3         */
#define AFF_COUNTER_ATT      22 /* plr just attacked          */
#define AFF_STABILIZE        23 /* player won't die due to phys loss */
#define AFF_PETRIFY             24 /* player's commands don't work       */
#define AFF_IMP_INVIS        25 /* char is improved invis         */
#define AFF_BLIND               26  /* (R) Char is blind            */
#define AFF_APPROACH      27 /* Character is using melee */
#define AFF_PILOT	  28 /* Char is piloting a vehicle */
#define AFF_RIG           29
#define AFF_MANNING       30 /* Char is in a (mini)turret */
#define AFF_DESIGN	  	31
#define AFF_PROGRAM	 	32
#define AFF_MAX   33

/* room-related defines */

/* The cardinal directions: used as index to room_data.dir_option[] */

#define NORTH          0
#define NORTHEAST      1
#define EAST           2
#define SOUTHEAST      3
#define SOUTH          4
#define SOUTHWEST      5
#define WEST           6
#define NORTHWEST      7
#define UP             8
#define DOWN           9

#define NUM_OF_DIRS     10      /* number of directions in a room (nsewud) */
/* also, ne, se, sw, nw, and matrix        */

/* "extra" bitvector settings for movement */
#define CHECK_SPECIAL   1
#define LEADER          2

/* Room flags: used in room_data.room_flags */

/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK               0   /* Dark                      */
#define ROOM_DEATH              1   /* Death trap                */
#define ROOM_NOMOB              2   /* MOBs not allowed          */
#define ROOM_INDOORS            3   /* Indoors                   */
#define ROOM_PEACEFUL           4   /* Violence not allowed      */
#define ROOM_SOUNDPROOF         5   /* Shouts, gossip blocked    */
#define ROOM_NOTRACK            6   /* Track won't go through    */
#define ROOM_NOMAGIC            7   /* Magic not allowed         */
#define ROOM_TUNNEL             8   /* room for only 1 pers      */
#define ROOM_PRIVATE            9   /* Can't teleport in         */
#define ROOM_LIT                10  /* Room has a streetlight    */
#define ROOM_HOUSE              11  /* (R) Room is a house       */
#define ROOM_HOUSE_CRASH        12  /* (R) House needs saving    */
#define ROOM_ATRIUM             13  /* (R) The door to a house   */
#define ROOM_OLC                14  /* (R) Modifyable/!compress  */
#define ROOM_BFS_MARK           15  /* (R) breath-first srch mrk */
#define ROOM_LOW_LIGHT          16  /* Room viewable with ll-eyes */
#define ROOM_NO_RADIO           18  /* Radio is sketchy and phones dont work */
#define ROOM_HERMETIC_LIBRARY   19  /* Room is spell library     */
#define ROOM_MEDICINE_LODGE     20  /* Room is a medicin lodge   */
#define ROOM_FALL               21  // room is a 'fall' room
#define ROOM_ROAD               22
#define ROOM_GARAGE             23
#define ROOM_SENATE             24
#define ROOM_NOQUIT             25
#define ROOM_SENT               26
#define ROOM_ASTRAL 		27 // Astral room
#define ROOM_NOGRID    		28
#define ROOM_STORAGE		29
#define ROOM_MAX        	30

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR               (1 << 0)   /* Exit is a door            */
#define EX_CLOSED               (1 << 1)   /* The door is closed        */
#define EX_LOCKED               (1 << 2)   /* The door is locked        */
#define EX_PICKPROOF            (1 << 3)   /* Lock can't be picked      */
#define EX_DESTROYED            (1 << 4)   /* door has been destroyed   */
#define EX_HIDDEN               (1 << 5)   /* exit is hidden            */

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0             /* Indoors                   */
#define SECT_CITY            1             /* In a city                 */
#define SECT_FIELD           2             /* In a field                */
#define SECT_FOREST          3             /* In a forest               */
#define SECT_HILLS           4             /* In the hills              */
#define SECT_MOUNTAIN        5             /* On a mountain             */
#define SECT_WATER_SWIM      6             /* Swimmable water           */
#define SECT_WATER_NOSWIM    7             /* Water - need a boat       */
#define SECT_UNDERWATER      8             /* Underwater                */
#define SECT_FLYING          9             /* Wheee!                    */
/* npc types */

#define CLASS_OTHER       0
#define CLASS_SPIRIT      1
#define CLASS_HUMANOID    2
#define CLASS_ANIMAL      3
#define CLASS_DRAGON      4

/* spirit powers */

#define PWR_ACCIDENT            0
#define PWR_ALIENATE            1
#define PWR_AURA                2
#define PWR_BIND                3
#define PWR_BREATHE             4
#define PWR_CONCEAL             5
#define PWR_CONFUSE             6
#define PWR_ENGULF              7
#define PWR_FEAR                8
#define PWR_FIND                9
#define PWR_GUARD               10
#define PWR_MANIFEST            11
#define PWR_PROJECT             12

#define NUM_SPIRIT_POWERS       13

/* magic global defines */

/* Note, we add the 100/10 and subtract 10 later to adjust for
 * spells with negative drain.*/
#define DRAIN_POWER(x) ((int)(((x)+100) / 10)-10)
/* Add 100 so -6 gives us 4. */
#define DRAIN_LEVEL(x) (((x)+100)%10)

#define DEFAULT_STAFF_LVL       12
#define DEFAULT_WAND_LVL        12

#define CAST_UNDEFINED  -1
#define CAST_SPELL      0
#define CAST_POTION     1
#define CAST_WAND       2
#define CAST_STAFF      3
#define CAST_SCROLL     4

#define MAG_DAMAGE      (1 << 0)
#define MAG_AFFECTS     (1 << 1)
#define MAG_UNAFFECTS   (1 << 2)
#define MAG_POINTS      (1 << 3)
#define MAG_ALTER_OBJS  (1 << 4)
#define MAG_GROUPS      (1 << 5)
#define MAG_MASSES      (1 << 6)
#define MAG_AREAS       (1 << 7)
#define MAG_SUMMONS     (1 << 8)
#define MAG_CREATIONS   (1 << 9)
#define MAG_MANUAL      (1 << 10)
#define MAG_COMBAT      (1 << 11)

#define MANA         0
#define MENTAL       0
#define PHYSICAL     1

/* reserved skill defines */
#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_X                         1
#define SPELL_XX                        2
#define SPELL_ANALYZE_DEVICE            3
#define SPELL_ANTI_BULLET               4
#define SPELL_ANTI_SPELL                5
#define SPELL_ANTIDOTE                  6
#define SPELL_ARMOR                     7
#define SPELL_CHAOS                     8
#define SPELL_CHAOTIC_WORLD             9
#define SPELL_CLAIRVOYANCE              10
#define SPELL_CLOUT                     11
#define SPELL_COMBAT_SENSE              12
#define SPELL_CONFUSION                 13
#define SPELL_CURE_DISEASE              14
#define SPELL_DEATH_TOUCH               15
#define SPELL_DETECT_ALIGNMENT          16
#define SPELL_DETECT_INVIS              17
#define SPELL_DETECT_MAGIC              18
#define SPELL_HEAL                      19
#define SPELL_HELLBLAST                 20
#define SPELL_IMPROVED_INVIS            21
#define SPELL_INFLUENCE                 22
#define SPELL_INVISIBILITY              23
#define SPELL_MANABALL                  24
#define SPELL_MANABLAST                 25
#define SPELL_MANA_BOLT                 26
#define SPELL_MANA_CLOUD                27
#define SPELL_MANA_DART                 28
#define SPELL_MANA_MISSILE              29
#define SPELL_OVERSTIMULATION           30
#define SPELL_PETRIFY                   31
#define SPELL_POWERBALL                 32
#define SPELL_POWERBLAST                33
#define SPELL_POWER_BOLT                34
#define SPELL_POWER_CLOUD               35
#define SPELL_POWER_DART                36
#define SPELL_POWER_MISSILE             37
#define SPELL_RAM                       38
#define SPELL_RAM_TOUCH                 39
#define SPELL_RESIST_PAIN               40
#define SPELL_SHAPE_CHANGE              41
#define SPELL_STABILIZE                 42
#define SPELL_STUNBALL                  43
#define SPELL_STUNBLAST                 44
#define SPELL_STUN_BOLT                 45
#define SPELL_STUN_CLOUD                46
#define SPELL_STUN_MISSILE              47
#define SPELL_STUN_TOUCH                48
#define SPELL_TOXIC_WAVE                49
#define SPELL_POWER_SHAFT               50
#define SPELL_POWER_BURST               51
#define SPELL_MANA_SHAFT                52
#define SPELL_MANA_BURST                53
#define SPELL_STUN_SHAFT                54
#define SPELL_STUN_BURST                55
#define SPELL_STUN_DART                 56
#define SPELL_ALLEVIATE_ALLERGY   57
#define SPELL_DECREASE_REFLEXES   58
#define SPELL_CAMOFLAUGE    59
#define SPELL_STEALTH     60

/* new spells can be added here */

/* spirit powers */
#define SPELL_FEAR     81
#define SPELL_ENGULF    82
#define SPELL_ALIENATE              83
#define SPELL_AURA                  84
#define SPELL_BIND                  85
#define SPELL_GUARD                 86

/* player created spells */
#define SPELL_ANALYZE_MAGIC         87
#define SPELL_ANALYZE_PERSON        88
#define SPELL_DECREASE_ATTRIB       89
#define SPELL_ELEMENTBALL           90
#define SPELL_ELEMENT_BOLT          91
#define SPELL_ELEMENT_CLOUD         92
#define SPELL_ELEMENT_DART          93
#define SPELL_ELEMENT_MISSILE       94
#define SPELL_INCREASE_ATTRIB       95
#define SPELL_INCREASE_REFLEXES     96
#define SPELL_LIGHT                 97

/* mobile-only spells */
#define SPELL_POISON                98
#define SPELL_TELEPORT              99

/* Insert new spells here, up to MAX_SPELLS */
#define MAX_SPELLS                   99

#define SKILL_ATHLETICS              1
#define SKILL_ARMED_COMBAT           2
#define SKILL_EDGED_WEAPONS          3
#define SKILL_POLE_ARMS              4
#define SKILL_WHIPS_FLAILS           5
#define SKILL_CLUBS                  6
#define SKILL_UNARMED_COMBAT         7
#define SKILL_GRAPPLE                8
#define SKILL_CYBER_IMPLANTS         9
#define SKILL_FIREARMS               10
#define SKILL_PISTOLS                11
#define SKILL_RIFLES                 12
#define SKILL_SHOTGUNS               13
#define SKILL_ASSAULT_RIFLES         14
#define SKILL_SMG                    15
#define SKILL_GRENADE_LAUNCHERS      16
#define SKILL_TASERS                 17
#define SKILL_GUNNERY                18
#define SKILL_MACHINE_GUNS           19
#define SKILL_MISSILE_LAUNCHERS      20
#define SKILL_ASSAULT_CANNON         21
#define SKILL_ARTILLERY              22
#define SKILL_PROJECTILES            23
#define SKILL_BOWS                   24
#define SKILL_CROSSBOWS              25
#define SKILL_THROWING_WEAPONS       26
#define SKILL_NONAERODYNAMIC         27
#define SKILL_CYBETERM_DESIGN        28
#define SKILL_DEMOLITIONS            29
#define SKILL_COMPUTER               30
#define SKILL_ELECTRONICS            31
#define SKILL_BR_COMPUTER            32
#define SKILL_BIOTECH                33
#define SKILL_MEDICAL                34
#define SKILL_CYBERSURGERY           35
#define SKILL_BIOLOGY                36
#define SKILL_LEADERSHIP             37
#define SKILL_INTERROGATION          38
#define SKILL_NEGOTIATION            39
#define SKILL_MAGICAL_THEORY         40
#define SKILL_CONJURING              41
#define SKILL_SORCERY                42
#define SKILL_SHAMANIC_STUDIES       43
#define SKILL_CORPORATE_ETIQUETTE    44
#define SKILL_MEDIA_ETIQUETTE        45
#define SKILL_STREET_ETIQUETTE       46
#define SKILL_TRIBAL_ETIQUETTE       47
#define SKILL_ELF_ETIQUETTE          48
#define SKILL_PROGRAM_COMBAT	    49
#define SKILL_PROGRAM_DEFENSIVE	    50
#define SKILL_PROGRAM_OPERATIONAL   51
#define SKILL_PROGRAM_SPECIAL	    52
#define SKILL_PROGRAM_CYBERTERM	    53
#define SKILL_DATA_BROKERAGE        54
#define SKILL_AURA_READING          55
#define SKILL_STEALTH               56
#define SKILL_STEAL                 57
#define SKILL_TRACK                 58
#define SKILL_CLIMBING            59 /* concentration of athletics */
#define SKILL_PILOT_BIKE    60
#define SKILL_PILOT_FIXED_WING   61
#define SKILL_PILOT_CAR     62
#define SKILL_PILOT_TRUCK   63
#define SKILL_BR_BIKE     64
#define SKILL_BR_CAR     65
#define SKILL_BR_DRONE     66
#define SKILL_BR_TRUCK        67
#define SKILL_PILOT_SUBMARINE   68
#define SKILL_PILOT_VECTORTHURST  69
#define SKILL_ENGLISH   	  70
#define SKILL_SPERETHIEL 	  71
#define SKILL_SPANISH		  72
#define SKILL_JAPANESE		  73
#define SKILL_CHINESE		  74
#define SKILL_KOREAN		  75
#define SKILL_ITALIAN		  76
#define SKILL_RUSSIAN		  77
#define SKILL_SIOUX		  78
#define SKILL_MAKAW		  79
#define SKILL_CROW		  80
#define SKILL_SALISH		  81
#define SKILL_UTE		  82
#define SKILL_NAVAJO		  83
#define SKILL_GERMAN		  84
#define MAX_SKILLS		  85

#define ADEPT_PERCEPTION	1
#define ADEPT_COMBAT_SENSE	2
#define ADEPT_BLIND_FIGHTING	3
#define ADEPT_QUICK_STRIKE	4
#define ADEPT_KILLING_HANDS	5
#define ADEPT_NERVE_STRIKE	6
#define ADEPT_SMASHING_BLOW	7
#define ADEPT_DISTANCE_STRIKE	8
#define ADEPT_REFLEXES		9
#define ADEPT_BOOST_STR		10
#define ADEPT_BOOST_QUI		11
#define ADEPT_BOOST_BOD		12
#define ADEPT_IMPROVED_STR	13
#define ADEPT_IMPROVED_QUI	14
#define ADEPT_IMPROVED_BOD	15
#define ADEPT_IMPROVED_PERCEPT	16
#define ADEPT_LOW_LIGHT		17
#define ADEPT_THERMO		18
#define ADEPT_IMAGE_MAG		19
#define ADEPT_MAGIC_RESISTANCE	20
#define ADEPT_PAIN_RESISTANCE	21
#define ADEPT_TEMPERATURE_TOLERANCE	22
#define ADEPT_SPELL_SHROUD	23
#define ADEPT_TRUE_SIGHT	24
#define ADEPT_MISSILE_PARRY	25
#define ADEPT_MISSILE_MASTERY	26
#define ADEPT_MYSTIC_ARMOUR	27
#define ADEPT_HEALING		28
#define ADEPT_FREEFALL		29
#define ADEPT_IRONWILL		30
#define ADEPT_NUMPOWER		31

/* object-spell related defines */

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4

/* ban defines -- do not change */

#define BAN_NOT         0
#define BAN_NEW         1
#define BAN_SELECT      2
#define BAN_ALL         3

#define BANNED_SITE_LENGTH    50

/* weapon attack types */

#define TYPE_HIT              300     // melee
#define TYPE_STING            301
#define TYPE_WHIP             302
#define TYPE_SLASH            303
#define TYPE_BITE             304
#define TYPE_BLUDGEON         305
#define TYPE_CRUSH            306
#define TYPE_POUND            307
#define TYPE_CLAW             308
#define TYPE_MAUL             309
#define TYPE_THRASH           310
#define TYPE_PIERCE           311
#define TYPE_PUNCH            312
#define TYPE_STAB             313
#define TYPE_TASER            314
#define TYPE_SHURIKEN         315    // throwing weapons - 1 room
#define TYPE_THROWING_KNIFE   316
#define TYPE_ARROW            317
#define TYPE_HAND_GRENADE     318    // explosive weapons
#define TYPE_GRENADE_LAUNCHER 319
#define TYPE_ROCKET           320
#define TYPE_PISTOL           321    // gun
#define TYPE_BLAST            322
#define TYPE_RIFLE            323
#define TYPE_SHOTGUN          324
#define TYPE_MACHINE_GUN      325
#define TYPE_CANNON           326
#define TYPE_BIFURCATE        327
#define TYPE_CRASH            328
#define TYPE_DUMPSHOCK        329
#define TYPE_BLACKIC	      330
/* new attack types can be added here - up to TYPE_SUFFERING */

/* death messages */
#define TYPE_SUFFERING          399
#define TYPE_EXPLOSION          400
#define TYPE_SCATTERING         401
#define TYPE_FALL               402
#define TYPE_DROWN              403
#define TYPE_ALLERGY            404
#define TYPE_BIOWARE            405
#define TYPE_RECOIL             406
#define TYPE_RAM  407

/* all those attack types can be used plus these for damage types to
* objects */

/* magic attack types */
#define TYPE_COMBAT_SPELL               500
#define TYPE_MANIPULATION_SPELL         501


/* used in void hit, for counter attacks */
#define TYPE_MELEE 1234

/* combat spell effects */
#define SPELL_EFFECT_NONE               0
#define SPELL_EFFECT_ACID               1
#define SPELL_EFFECT_AIR                2
#define SPELL_EFFECT_EARTH              3
#define SPELL_EFFECT_FIRE               4
#define SPELL_EFFECT_ICE                5
#define SPELL_EFFECT_LIGHTNING          6
#define SPELL_EFFECT_WATER              7

/* detection spell effects */
#define SPELL_EFFECT_ANALYZE_MAGIC      0
#define SPELL_EFFECT_ANALYZE_DEVICE     1
#define SPELL_EFFECT_ANALYZE_PERSON     2
#define SPELL_EFFECT_CLAIRVOYANCE       3
#define SPELL_EFFECT_COMBAT_SENSE       4
#define SPELL_EFFECT_DETECT_ALIGN       5
#define SPELL_EFFECT_DETECT_INVIS       6
#define SPELL_EFFECT_DETECT_MAGIC       7

/* health spell effects */
#define SPELL_EFFECT_ANTIDOTE            0
#define SPELL_EFFECT_CURE                   1
#define SPELL_EFFECT_DECREASE_BOD           2
#define SPELL_EFFECT_DECREASE_CHA           3
#define SPELL_EFFECT_DECREASE_INT           4
#define SPELL_EFFECT_DECREASE_QUI           5
#define SPELL_EFFECT_DECREASE_REA           6
#define SPELL_EFFECT_DECREASE_STR           7
#define SPELL_EFFECT_DECREASE_WIL           8
#define SPELL_EFFECT_HEAL                   9
#define SPELL_EFFECT_INCREASE_BOD           10
#define SPELL_EFFECT_INCREASE_CHA           11
#define SPELL_EFFECT_INCREASE_INT           12
#define SPELL_EFFECT_INCREASE_QUI           13
#define SPELL_EFFECT_INCREASE_REA           14
#define SPELL_EFFECT_INCREASE_REFLEX     15
#define SPELL_EFFECT_INCREASE_STR           16
#define SPELL_EFFECT_INCREASE_WIL           17
#define SPELL_EFFECT_RESIST_PAIN            18
#define SPELL_EFFECT_STABILIZE           19
#define SPELL_EFFECT_ALLEVIATE_ALLERGY  20
#define SPELL_EFFECT_DECREASE_REFLEX  21
/* illusion spell effects */
#define SPELL_EFFECT_CHAOS              0
#define SPELL_EFFECT_CONFUSION      1
#define SPELL_EFFECT_IMP_INVIS      2
#define SPELL_EFFECT_INVIS              3
#define SPELL_EFFECT_OVERSTIM       4
#define SPELL_EFFECT_CAMO    5
#define SPELL_EFFECT_STEALTH   6

/* manipulation spell effects */
#define SPELL_EFFECT_ARMOR              8
#define SPELL_EFFECT_LIGHT              9
#define SPELL_EFFECT_INFLUENCE          10

/* object-related defines */

/* item types: used by obj_data.obj_flags.type_flag */

#define ITEM_LIGHT      1               /* Item is a light source       */
#define ITEM_WORKSHOP   2               /* Item is a scroll             */
#define ITEM_CAMERA     3               /* Item is a wand               */
#define ITEM_STAFF      4               /* Item is a staff              */
#define ITEM_WEAPON     5               /* Item is a weapon             */
#define ITEM_FIREWEAPON 6               /* Item is bow/xbow             */
#define ITEM_MISSILE    7               /* Item is arrow/bolt   */
#define ITEM_TREASURE   8               /* Item is a treasure, not nuyen        */
#define ITEM_GYRO       9               /* Item is Gyroscopic Harness   */
#define ITEM_POTION    10               /* Item is a potion             */
#define ITEM_WORN      11               /* Item is worn, not armor      */
#define ITEM_OTHER     12               /* Misc object                  */
#define ITEM_TRASH     13               /* Trash - shopkeeps won't buy  */
#define ITEM_DOCWAGON  14               /* Item is a docwagon contract  */
#define ITEM_CONTAINER 15               /* Item is a container          */
#define ITEM_RADIO     16               /* Item is radio                */
#define ITEM_DRINKCON  17               /* Item is a drink container    */
#define ITEM_KEY       18               /* Item is a key                */
#define ITEM_FOOD      19               /* Item is food                 */
#define ITEM_MONEY     20               /* Item is money (nuyen/credstick)      */
#define ITEM_PHONE     21               /* Item is a phone              */
#define ITEM_BIOWARE   22               /* Item is bioware            */
#define ITEM_FOUNTAIN  23               /* Item is a fountain           */
#define ITEM_CYBERWARE 24               /* Item is cyberware            */
#define ITEM_CYBERDECK 25               /* Item is a cyberdeck          */
#define ITEM_PROGRAM   26               /* Item is a program            */
#define ITEM_GUN_CLIP  27               /* Item is a gun clip           */
#define ITEM_GUN_ACCESSORY 28           /* Item is a gun accessory      */
#define ITEM_SPELL_FORMULA 29           /* Item is a spell formula      */
#define ITEM_WORKING_GEAR 30            /* kit, shop, facility  */
#define ITEM_FOCUS      31              /* magical foci of various types */
#define ITEM_PATCH      32              /* type of slap patch  */
#define ITEM_CLIMBING   33              /* climbing gear  */
#define ITEM_QUIVER     34              /* holds projectiles   */
#define ITEM_DECK_ACCESSORY 35       /* decking accessory   */
#define ITEM_RCDECK     36
#define ITEM_CHIP 	37
#define ITEM_MOD     	38
#define ITEM_HOLSTER 	39
#define ITEM_DESIGN	40

/* take/wear flags: used by obj_data.obj_flags.wear_flags */

#define ITEM_WEAR_TAKE          0  /* Item can be takes          */
#define ITEM_WEAR_FINGER        1  /* Can be worn on finger      */
#define ITEM_WEAR_NECK          2  /* Can be worn around neck    */
#define ITEM_WEAR_BODY          3  /* Can be worn on body        */
#define ITEM_WEAR_HEAD          4  /* Can be worn on head        */
#define ITEM_WEAR_LEGS          5  /* Can be worn on legs        */
#define ITEM_WEAR_FEET          6  /* Can be worn on feet        */
#define ITEM_WEAR_HANDS         7  /* Can be worn on hands       */
#define ITEM_WEAR_ARMS          8  /* Can be worn on arms        */
#define ITEM_WEAR_SHIELD        9  /* Can be used as a shield    */
#define ITEM_WEAR_ABOUT         10 /* Can be worn about body     */
#define ITEM_WEAR_WAIST         11 /* Can be worn around waist   */
#define ITEM_WEAR_WRIST         12 /* Can be worn on wrist       */
#define ITEM_WEAR_WIELD         13 /* Can be wielded             */
#define ITEM_WEAR_HOLD          14 /* Can be held                */
#define ITEM_WEAR_EYES          15 /* worn on eyes          */
#define ITEM_WEAR_EAR          16 /* can be worn on/in ear  */
#define ITEM_WEAR_UNDER  17
#define ITEM_WEAR_BACK  18
#define ITEM_WEAR_ANKLE  19
#define ITEM_WEAR_SOCK  20
#define ITEM_WEAR_BELLY  21
#define ITEM_WEAR_ARM    22
#define ITEM_WEAR_FACE   23
#define ITEM_WEAR_MAX           24

/* extra object flags: used by obj_data.obj_flags.extra_flags */

#define ITEM_GLOW          0     /* Item is glowing              */
#define ITEM_HUM           1     /* Item is humming              */
#define ITEM_NORENT        2     /* Item cannot be rented        */
#define ITEM_NODONATE      3     /* Item cannot be donated       */
#define ITEM_NOINVIS       4     /* Item cannot be made invis    */
#define ITEM_INVISIBLE     5     /* Item is invisible            */
#define ITEM_MAGIC         6     /* Item is magical              */
#define ITEM_NODROP        7     /* Item is cursed: can't drop   */
#define ITEM_BLESS         8     /* Item is blessed              */
#define ITEM_NOSELL        9     /* Shopkeepers won't touch it   */
#define ITEM_CORPSE        10    /* Item is a corpse             */
#define ITEM_GODONLY       11    /* Only a god may use this item */
#define ITEM_TWOHANDS      12    /* weapon takes 2 hands to use */
#define ITEM_STARTER       13    /* REMOVE!!!! item is from character gen */
#define ITEM_VOLATILE      14    /* connected item loaded in ip zone */
#define ITEM_WIZLOAD       15    /* item was loaded by an immortal */
#define ITEM_NOTROLL    16
#define ITEM_NOELF    17
#define ITEM_NODWARF    18
#define ITEM_NOORK    19
#define ITEM_NOHUMAN    20
#define ITEM_SNIPER      21
#define ITEM_IMMLOAD       22    /* decays after timer runs out  */
#define ITEM_EXTRA_MAX    23
/* always keep immload the last */

/* Ammo types */ 
#define AMMO_NORMAL     0
#define AMMO_APDS       1
#define AMMO_EXPLOSIVE  2
#define AMMO_EX         3
#define AMMO_FLECHETTE  4
#define AMMO_GEL        5

/* material type for item */
#define ITEM_NONE                  0
#define ITEM_PLASTIC             1
#define ITEM_IRON                   2
#define ITEM_SILVER                 3

/* decking accessory types */

#define TYPE_FILE            0
#define TYPE_UPGRADE         1
#define TYPE_COMPUTER	     2
#define TYPE_PARTS	     3
#define TYPE_COOKER	     4

/* vehicle types table */
#define VEH_DRONE 0
#define VEH_BIKE 1
#define VEH_CAR 2
#define VEH_TRUCK 3
#define VEH_FIXEDWING 4
#define VEH_ROTORCRAFT 5
#define VEH_VECTORTHRUST 6
#define VEH_HOVERCRAFT 7
#define VEH_MOTORBOAT 8
#define VEH_SHIP 9
#define VEH_LTA 10

/* vehicle affection table */
#define VAFF_NONE 0
#define VAFF_HAND 1
#define VAFF_SPD  2
#define VAFF_ACCL 3
#define VAFF_BOD  4
#define VAFF_ARM  5
#define VAFF_SIG  6
#define VAFF_AUTO 7
#define VAFF_SEA  8
#define VAFF_LOAD 9
#define VAFF_SEN  10
#define VAFF_PILOT 11
#define VAFF_MAX  12

/* vehicle flag table */
#define VFLAG_NONE      0
#define VFLAG_CAN_FLY   1
#define VFLAG_AMPHIB   2

/* vehicle speed table */
#define SPEED_OFF 0
#define SPEED_IDLE 1
#define SPEED_CRUISING 2
#define SPEED_SPEEDING 3
#define SPEED_MAX 4

/* types of cyberware for value[2] on a cyberware object */

#define CYB_DATAJACK            1

/* program types */
#define SOFT_BOD                1
#define SOFT_EVASION            2
#define SOFT_MASKING            3
#define SOFT_SENSOR             4
#define SOFT_ATTACK             5
#define SOFT_SLOW               6
#define SOFT_MEDIC              7
#define SOFT_SNOOPER		8
#define SOFT_BATTLETEC		9
#define SOFT_COMPRESSOR		10
#define SOFT_ANALYZE            11
#define SOFT_DECRYPT            12
#define SOFT_DECEPTION          13
#define SOFT_RELOCATE           14
#define SOFT_SLEAZE             15
#define SOFT_SCANNER		16
#define SOFT_BROWSE		17
#define SOFT_READ		18
#define SOFT_TRACK		19
#define SOFT_ARMOUR		20
#define SOFT_CAMO		21
#define SOFT_CRASH		22
#define SOFT_DEFUSE		23
#define SOFT_EVALUATE		24
#define SOFT_VALIDATE		25
#define SOFT_SWERVE		26
#define SOFT_SUITE		27
#define SOFT_COMMLINK		28
#define SOFT_CLOAK		29
#define SOFT_LOCKON		30
#define NUM_PROGRAMS		31

/* modifier constants used with obj affects ('A' fields) */

#define APPLY_NONE               0      /* No effect                    */
#define APPLY_BOD                1      /* Apply to Body                */
#define APPLY_QUI                2      /* Apply to Quickness           */
#define APPLY_STR                3      /* Apply to Strength            */
#define APPLY_CHA                4      /* Apply to Charisma            */
#define APPLY_INT                5      /* Apply to Intelligence        */
#define APPLY_WIL                6      /* Apply to Willpower           */
#define APPLY_ESS                7      /* Apply to Essence             */
#define APPLY_MAG                8      /* Apply to Magic               */
#define APPLY_REA                9      /* Apply to Reaction            */
#define APPLY_AGE               10      /* Apply to age                 */
#define APPLY_CHAR_WEIGHT       11      /* Apply to weight              */
#define APPLY_CHAR_HEIGHT       12      /* Apply to height              */
#define APPLY_MENTAL            13      /* Apply to max mental          */
#define APPLY_PHYSICAL          14      /* Apply to max physical points */
#define APPLY_MOVE              15      /* Apply to max move points     */
#define APPLY_BALLISTIC         16      /* Apply to Ballistic armor     */
#define APPLY_IMPACT            17      /* Apply to Impact armor rating */
#define APPLY_ASTRAL_POOL       18      /* Apply to Astral Pool         */
#define APPLY_DEFENSE_POOL      19      /* Apply to Defense Pool        */
#define APPLY_COMBAT_POOL       20      /* Apply to Dodge Pool          */
#define APPLY_HACKING_POOL      21      /* Apply to Hacking Pool        */
#define APPLY_MAGIC_POOL        22      /* Apply to Magic Pool          */
#define APPLY_INITIATIVE_DICE   23      /* Apply to Initiative Dice     */
#define APPLY_TARGET            24      /* Apply to Target Numbers      */
#define APPLY_CONTROL_POOL      25
#define APPLY_MAX               26

/* container flags - value[1] */

#define CONT_CLOSEABLE      (1 << 0)    /* Container can be closed      */
#define CONT_PICKPROOF      (1 << 1)    /* Container is pickproof       */
#define CONT_CLOSED         (1 << 2)    /* Container is closed          */
#define CONT_LOCKED         (1 << 3)    /* Container is locked          */

/* some different kind of liquids for use in values of drink containers */
/* New and improved liquids by Jordan */

#define LIQ_WATER      0
#define LIQ_SODA       1
#define LIQ_COFFEE     2
#define LIQ_MILK       3
#define LIQ_JUICE      4
#define LIQ_TEA        5
#define LIQ_SOYKAF     6
#define LIQ_SMOOTHIE   7
#define LIQ_SPORTS     8
#define LIQ_ESPRESSO   9
#define LIQ_BEER       10
#define LIQ_ALE        11
#define LIQ_LAGER      12
#define LIQ_IMPORTBEER 13
#define LIQ_MICROBREW  14
#define LIQ_MALTLIQ    15
#define LIQ_COCKTAIL   16
#define LIQ_MARGARITA  17
#define LIQ_LIQUOR     18
#define LIQ_EVERCLEAR  19
#define LIQ_VODKA      20
#define LIQ_TEQUILA    21
#define LIQ_BRANDY     22
#define LIQ_RUM        23
#define LIQ_WHISKEY    24
#define LIQ_GIN        25
#define LIQ_CHAMPAGNE  26
#define LIQ_REDWINE    27
#define LIQ_WHITEWINE  28
#define LIQ_BLOOD      29
#define LIQ_HOTSAUCE   30
#define LIQ_PISS       31
#define LIQ_LOCAL      32
#define LIQ_FUCKUP     33

/* focus values */

#define VALUE_FOCUS_TYPE        0
#define VALUE_FOCUS_RATING      1
#define VALUE_FOCUS_CAT         2
#define VALUE_FOCUS_BONDED      5
/* #define VALUE_FOCUS_WEAPON_VNUM      7 */
#define VALUE_FOCUS_SPECIFY     8
#define VALUE_FOCUS_CHAR_IDNUM  9

/* types of foci */

#define FOCI_SPELL         0
#define FOCI_SPELL_CAT     1
#define FOCI_SPIRIT        2
#define FOCI_POWER         3
#define FOCI_LOCK          4
#define FOCI_WEAPON        5

/* Vehicle mods */
#define MOD_SUSPENSION 0
#define MOD_ENGINE 1
#define MOD_STEERING 2
#define MOD_TIRES 3
#define MOD_BRAKES 4
#define MOD_PLATING 5
#define MOD_COMP 6
#define MOD_PHONE 7
#define MOD_SENSOR 8
#define MOD_ECM  9
#define MOD_IGNITION 10
#define MOD_ALARM 11
#define MOD_SEAT 12
#define MOD_BODY 13
#define MOD_RADIO       14
#define MOD_MOUNT	15
#define NUM_MODS        16

/* house value defines */
#define MAX_HOUSES      100
#define MAX_GUESTS      10
#define NORMAL_MAX_GUESTS    2
#define OBJS_PER_LIFESTYLE   35

#define HOUSE_LOW     0
#define HOUSE_MIDDLE  1
#define HOUSE_HIGH    2
#define HOUSE_LUXURY  3

/* alias defines */
#define ALIAS_SIMPLE    0
#define ALIAS_COMPLEX   1

#define ALIAS_SEP_CHAR  ';'
#define ALIAS_VAR_CHAR  '$'
#define ALIAS_GLOB_CHAR '*'

/* Subcommands section: Originally from interpreter.h */

/* directions */
#define SCMD_NORTH      1
#define SCMD_NORTHEAST  2
#define SCMD_EAST       3
#define SCMD_SOUTHEAST  4
#define SCMD_SOUTH      5
#define SCMD_SOUTHWEST  6
#define SCMD_WEST       7
#define SCMD_NORTHWEST  8
#define SCMD_UP         9
#define SCMD_DOWN      10
/* #define SCMD_OUT 11   for vehicles, not rooms */

/* do_gen_ps */
#define SCMD_INFO       0
#define SCMD_HANDBOOK   1
#define SCMD_CREDITS    2
#define SCMD_NEWS       3
#define SCMD_POLICIES   4
#define SCMD_VERSION    5
#define SCMD_IMMLIST    6
#define SCMD_MOTD       7
#define SCMD_IMOTD      8
#define SCMD_CLEAR      9
#define SCMD_WHOAMI     10

/* do_wizutil */
#define SCMD_PARDON     0
#define SCMD_NOTITLE    1
#define SCMD_SQUELCH    2
#define SCMD_FREEZE     3
#define SCMD_THAW       4
#define SCMD_UNAFFECT   5
#define SCMD_SQUELCHOOC 6
#define SCMD_INITIATE   7
#define SCMD_PG         8
#define SCMD_RPE 9
#define SCMD_POWERPOINT 10
/* do_say */
#define SCMD_SAY        0
#define SCMD_OSAY       1
#define SCMD_SAYTO      2

/* do_spec_com */
#define SCMD_WHISPER    0
#define SCMD_ASK        1

/* do_gen_com */
#define SCMD_SHOUT      0
#define SCMD_NEWBIE     1
#define SCMD_OOC        2
#define SCMD_RPETALK 3
#define SCMD_HIREDTALK 4

/* do_last */
#define SCMD_LAST     0
#define SCMD_FINGER   1

/* do_shutdown */
#define SCMD_SHUTDOW    0
#define SCMD_SHUTDOWN   1

/* do_quit */
#define SCMD_QUI        0
#define SCMD_QUIT       1

/* do_date */
#define SCMD_DATE       0
#define SCMD_UPTIME     1

/* do disconnect */
#define SCMD_DISCONNECT    0
#define SCMD_MORTED        1

/* do wiztitle */
#define SCMD_WHOTITLE      0
#define SCMD_PRETITLE      1

/* do_commands */
#define SCMD_COMMANDS   0
#define SCMD_SOCIALS    1

/* do_drop */
#define SCMD_DROP       0
#define SCMD_JUNK       1
#define SCMD_DONATE     2

/* do_gen_write */
#define SCMD_BUG        0
#define SCMD_TYPO       1
#define SCMD_IDEA       2

/* do_look */
#define SCMD_LOOK       0
#define SCMD_READ       1

/* do_qcomm */
#define SCMD_QSAY       0
#define SCMD_QECHO      1

/* do_pour */
#define SCMD_POUR       0
#define SCMD_FILL       1

/* do_poof */
#define SCMD_POOFIN     0
#define SCMD_POOFOUT    1

/* do_astral */
#define SCMD_PROJECT       0
#define SCMD_PERCEIVE      1

/* do_hit */
#define SCMD_HIT        0
#define SCMD_MURDER     1
#define SCMD_KILL       2

/* do_eat */
#define SCMD_EAT        0
#define SCMD_TASTE      1
#define SCMD_DRINK      2
#define SCMD_SIP        3

/* do_use */
#define SCMD_USE        0

/* do_echo */
#define SCMD_ECHO       0
#define SCMD_EMOTE      1
#define SCMD_AECHO	3

/* do_gen_door */
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4
#define SCMD_KNOCK      5

/* matrix subcommands */
#define SCMD_INSTALL    1
#define SCMD_UNINSTALL  2

/* do_skills */
#define SCMD_SKILLS     0
#define SCMD_ABILITIES  1

/* do_time */
#define SCMD_NORMAL     0
#define SCMD_PRECISE    1

/* do_subscribe */
#define SCMD_SUB 0
#define SCMD_UNSUB 1

/* do_phone */
#define SCMD_RING 1
#define SCMD_HANGUP 2
#define SCMD_TALK 3
#define SCMD_ANSWER 4

/* do_man */
#define SCMD_UNMAN 1

/* do_load */
#define SCMD_SWAP 	0
#define SCMD_UPLOAD 	1
#define SCMD_UNLOAD	2

/* END SUBCOMMANDS SECTION */

/* modes of connectedness: used by descriptor_data.state */

#define CON_PLAYING      0              /* Playing - Nominal state      */
#define CON_CLOSE        1              /* Disconnecting                */
#define CON_GET_NAME     2              /* By what name ..?             */
#define CON_NAME_CNFRM   3              /* Did I get that right, x?     */
#define CON_PASSWORD     4              /* Password:                    */
#define CON_NEWPASSWD    5              /* Give me a password for x     */
#define CON_CNFPASSWD    6              /* Please retype password:      */
#define CON_CCREATE      7
#define CON_RMOTD        8              /* PRESS RETURN after MOTD      */
#define CON_MENU         9              /* Your choice: (main menu)     */
#define CON_CHPWD_GETOLD 11             /* Changing passwd: get old     */
#define CON_CHPWD_GETNEW 12             /* Changing passwd: get new     */
#define CON_CHPWD_VRFY   13             /* Verify new password          */
#define CON_DELCNF1      14             /* Delete confirmation 1        */
#define CON_DELCNF2      15             /* Delete confirmation 2        */
#define CON_QMENU        16             /* quit menu                    */
#define CON_QGETOLDPW    17
#define CON_QGETNEWPW    18
#define CON_QVERIFYPW    19
#define CON_QDELCONF1    20
#define CON_QDELCONF2    21
#define CON_SPELL_CREATE 22             /* Spell creation menus         */
#define CON_PCUSTOMIZE   23             /* customize persona description menu */
#define CON_ACUSTOMIZE   24             /* customize reflection description menu */
#define CON_FCUSTOMIZE   25
#define CON_VEDIT        26
#define CON_IEDIT        27  /* olc edit mode */
#define CON_REDIT        28  /* olc edit mode */
#define CON_MEDIT        29
#define CON_QEDIT        30
#define CON_SHEDIT       31
#define CON_ZEDIT        32
#define CON_HEDIT        33
#define CON_ICEDIT       34
#define CON_PRO_CREATE   35
#define CON_IDCONING     36            /* waiting for ident connection */
#define CON_IDCONED      37            /* ident connection complete    */
#define CON_IDREADING    38            /* waiting to read ident sock   */
#define CON_IDREAD       39            /* ident results read           */
#define CON_ASKNAME      40            /* Ask user for name            */

/* chargen connected modes */
#define CCR_AWAIT_CR    -1
#define CCR_SEX         0
#define CCR_RACE        1
#define CCR_TYPE        2
#define CCR_ARCHETYPE   3
#define CCR_TOTEM       4
#define CCR_PRIORITY    5
#define CCR_ASSIGN      6
#define CCR_TRADITION   15
/* priority choosing chargen modes */
#define PR_NONE         0
#define PR_ATTRIB       1
#define PR_MAGIC        2
#define PR_RESOURCE     3
#define PR_SKILL        4
#define PR_RACE         5

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD     0
#define DB_BOOT_MOB     1
#define DB_BOOT_OBJ     2
#define DB_BOOT_ZON     3
#define DB_BOOT_SHP     4
#define DB_BOOT_QST     5
#define DB_BOOT_VEH     6
#define DB_BOOT_MTX     7
#define DB_BOOT_IC      8
/* Defines for sending text */

#define TO_ROOM         1
#define TO_VICT         2
#define TO_NOTVICT      3
#define TO_CHAR         4
#define TO_ROLLS        5
#define TO_VEH		6
#define TO_DECK		7
#define TO_SLEEP        128     /* to char, even if sleeping */

/* Boards */

#define NUM_OF_BOARDS           24 /* change if needed! */
#define MAX_BOARD_MESSAGES      300     /* arbitrary -- change if needed */
#define MAX_MESSAGE_LENGTH      5120    /* arbitrary -- change if needed */
#define INDEX_SIZE         ((NUM_OF_BOARDS*MAX_BOARD_MESSAGES) + 5)

/* teacher modes */

#define NEWBIE          0
#define AMATEUR         1
#define ADVANCED        2

/* sun state for weather_data */

#define SUN_DARK        0
#define SUN_RISE        1
#define SUN_LIGHT       2
#define SUN_SET         3
#define MOON_NEW  0
#define MOON_WAX  1
#define MOON_FULL  2
#define MOON_WANE  3

/* sky conditions for weather_data */

#define SKY_CLOUDLESS   0
#define SKY_CLOUDY      1
#define SKY_RAINING     2
#define SKY_LIGHTNING   3

/* rent codes */

#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5

/* MATRIX HOSTS TYPES */
#define HOST_DATASTORE	0
#define HOST_LTG	1
#define HOST_RTG	2
#define HOST_PLTG	3
#define HOST_SECURITY	4
#define HOST_CONTOLLER	5

/* IC Options */
#define IC_ARMOUR	1
#define IC_CASCADE	2
#define IC_EX_DEFENSE	3
#define IC_EX_OFFENSE	4
#define IC_SHIELD	5
#define IC_SHIFT	6
#define IC_TRAP		7

#define QUEST_TIMER	10

#define OPT_USEC        100000  /* 10 passes per second */
#define PASSES_PER_SEC  (1000000 / OPT_USEC)
#define RL_SEC          * PASSES_PER_SEC

#define PULSE_ZONE      (3 RL_SEC)
#define PULSE_SPECIAL   (10 RL_SEC)
#define PULSE_MOBILE    (10 RL_SEC)
#define PULSE_VIOLENCE  (1 RL_SEC)
#define PULSE_MONORAIL  (5 RL_SEC)

#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       96          /* Max length of prompt        */
#define GARBAGE_SPACE           32
#define SMALL_BUFSIZE           1024
#define LARGE_BUFSIZE    (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define MAX_STRING_LENGTH       8192
#define MAX_INPUT_LENGTH        2048     /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH    4096     /* Max size of *raw* input */
#define MAX_MESSAGES            100
#define MAX_NAME_LENGTH         20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH          30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH        50  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_WHOTITLE_LENGTH     10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH             30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define LINE_LENGTH             80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH           2040/* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT              32  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT          6 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define IDENT_LENGTH            8

/* ban struct */
struct ban_list_element
{
  char site[BANNED_SITE_LENGTH+1];
  int  type;
  time_t date;
  char name[MAX_NAME_LENGTH+1];
  struct ban_list_element *next;
};

#endif

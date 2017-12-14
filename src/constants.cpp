/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "awake.h"

extern const char *circlemud_version[] =
    {
      "CircleMUD, version 3.00 beta patchlevel 9\r\n"
    };
extern const char *awakemud_version[] =
    {
      "AwakeMUD, version 0.84 BETA RELEASE\r\n"
    };


/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */
const char *status_ratings[] =
  {
    "Nonexistent",
    "Mortal",
    "Builder",
    "Architect",
    "Fixer",
    "Conspirator",
    "Executive",
    "Developer",
    "Vice-President",
    "Admin",
    "President",
    "\n"
  };

const char *cyberware_names[] =
  {
    "chipjack",                  // 0
    "datajack",
    "radio",
    "radio receiver",
    "commlink",
    "damper",                    // 5
    "high frequency",
    "low frequency",
    "flare compensation",
    "low-light",
    "thermographic",             // 10
    "cortex bomb",
    "memory",
    "datasoft link",
    "display link",
    "ActiveSoft",                // 15
    "DataSoft",
    "KnowSoft",
    "LinguaSoft",
    "hand razors",
    "muscle replacement",        // 20
    "spur",
    "smartlink",
    "dermal plating",
    "air filtration",
    "blood filtration",          // 25
    "ingestion filtration",
    "limb",
    "skillwires",
    "bone lacing",
    "wired/boosted reflexes",            // 30
    "magnification",
    "vehicle control rig",
    "encephalon",
    "maths SPU",
    "\n"
  };

const char *bioware_names[] =
  {
    "platelet factory",          //0
    "symbiotes",
    "orthoskin",
    "tailored pheromones",
    "adrenal pump",
    "suprathyroid gland",        //5
    "cerebral booster",
    "pain editor",
    "synaptic accelerator",
    "enhanced articulation",
    "muscle augmentation",       //10
    "\n"
  };

const char *patch_names[] =
  {
    "antidote",
    "stimulant",
    "tranq",
    "trauma"
  };

const char *gear_name[] =
  {
    "medical",
    "electronic",
    "cyberdeck",
    "\n"
  };

const char *foci_types[] =
  {
    "specific spell",
    "spell category",
    "spirit",
    "power",
    "spell lock",
    "weapon",
    "\n"
  };

const char *dist_name[] =
  {
    "close by",
    "not far off",
    "in the distance",
    "almost out of sight"
  };

const char *wound_arr[] =
  {
    "None",
    "L",
    "M",
    "S",
    "D",
    "BUGD1",
    "BUGD2",
    "BUGD3",
    "BUGD4",
    "BUGD5"
  };

const char *wound_name[] =
  {
    "None",
    "Light",
    "Moderate",
    "Serious",
    "Deadly",
    "\n",
    "BUGDeadly2",
    "BUGDeadly3",
    "BUGDeadly4",
    "BUGDeadly5"
  };

int damage_array[] =
  {
    0,
    1,
    3,
    6,
    10,
  };

// names of materials objects may be made out of
const char *material_names[] =
  {
    "paper",
    "wood",
    "glass",
    "fabric",
    "leather",
    "brick",            // 5
    "plastic",
    "adv. plastics",
    "metal",
    "orichalcum",
    "electronics",      // 10
    "computers",
    "toxic wastes",
    "organic",
    "stone",
    "ceramic",          // 15
    "concrete",
    "\n"
  };

// resistance target numbers by material type
int material_ratings[] =
  {
    3,
    3,
    3,
    3,
    4,
    5,
    5,
    8,
    8,
    8,
    8,
    10,
    10,
    2,
    3,
    5,
    4
  };

const char *barrier_names[] =
  {
    "Standard glass",
    "Cheap material",
    "Average material/ballistic glass",
    "Heavy material",
    "Reinforced/armored glass",
    "Structural material",
    "Heavy structural material",
    "Armored/reinforced material",
    "Hardened material",
    "\n"
  };

int barrier_ratings[] =
  {
    2,
    3,
    4,
    6,
    8,
    12,
    16,
    24,
    32
  };

/* cardinal directions */
const char *dirs[] =
  {
    "north",
    "ne",
    "east",
    "se",
    "south",
    "sw",
    "west",
    "nw",
    "up",
    "down",
    "\n"
  };

const char *exitdirs[] =
  {
    "n",
    "ne",
    "e",
    "se",
    "s",
    "sw",
    "w",
    "nw",
    "u",
    "d",
    "\n"
  };

const char *lookdirs[] =
  {
    "north",
    "east",
    "south",
    "west",
    "ne",
    "se",
    "sw",
    "nw",
    "up",
    "down",
    "\n"
  };

int convert_look[] =
  {
    0,
    2,
    4,
    6,
    1,
    3,
    5,
    7,
    8,
    9,
    10,
  };

int convert_dir[] =
  {
    1,
    7,
    2,
    8,
    3,
    9,
    4,
    10,
    5,
    6,
    0
  };

const char *fulldirs[] =
  {
    "north",
    "northeast",
    "east",
    "southeast",
    "south",
    "southwest",
    "west",
    "northwest",
    "up",
    "down",
    "\n"
  };

const char *thedirs[] =
  {
    "the north",
    "the northeast",
    "the east",
    "the southeast",
    "the south",
    "the southwest",
    "the west",
    "the northwest",
    "above",
    "below",
    "\n"
  };


/* ROOM_x */
const char *room_bits[] =
  {
    "DARK",
    "DEATH",
    "!MOB",
    "INDOORS",
    "PEACEFUL",
    "SOUNDPROOF",
    "!TRACK",
    "!MAGIC",
    "TUNNEL",
    "PRIVATE",
    "LIT",
    "HOUSE",
    "HCRSH",
    "ATRIUM",
    "OLC",
    "*",                          /* BFS MARK */
    "LOW_LIGHT",
    "!USED",
    "!RADIO",
    "LIBRARY",
    "LODGE",
    "FALL",
    "ROAD",
    "GARAGE",
    "SENATE",
    "!QUIT",
    "SENT",
    "ASTRAL",
    "!GRID",
    "STORAGE",
    "\n"
  };


/* EX_x */
const char *exit_bits[] =
  {
    "DOOR",
    "CLOSED",
    "LOCKED",
    "PICKPROOF",
    "DESTROYED",
    "HIDDEN",
    "\n"
  };


/* SECT_ */
const char *sector_types[] =
  {
    "Inside",
    "City",
    "Field",
    "Forest",
    "Hills",
    "Mountains",
    "Water (Swim)",
    "Water (No Swim)",
    "Underwater",
    "Flying",
    "\n"
  };


/* SEX_x */
const char *genders[] =
  {
    "Neutral",
    "Male",
    "Female",
    "\n"
  };

const char *thrdgenders[] =
  {
    "it",
    "him",
    "her",
    "\n"
  };

const char *posgenders[] =
  {
    "its",
    "his",
    "her",
    "\n"
  };

/* NPC classes */
const char *npc_classes[] =
  {
    "Other",
    "Spirit",
    "Humanoid",
    "Animal",
    "Dragon",
    "\n"
  };

const char *shape_forms[] =
  {
    "bat",
    "large bear",
    "bear",
    "housecat",
    "wildcat",
    "deer",
    "large dog",
    "dog",
    "elephant",
    "fox",
    "goat",
    "horse",
    "leopard",
    "rat",
    "rhinoceros",
    "seal",
    "large shark",
    "shark",
    "tiger",
    "wolf",
    "\n"
  };

const char *spell_categories[] =
  {
    "Combat",
    "Detection",
    "Health",
    "Illusion",
    "Manipulation",
    "\n"
  };

const char *spirits[] =
  {
    "NULL",
    "Air elemental",
    "Earth elemental",
    "Fire elemental",
    "Water elemental",
    "City spirit",       // 5
    "Hearth spirit",
    "Field spirit",
    "Desert spirit",
    "Forest spirit",
    "Mountain spirit",   // 10
    "Prairie spirit",
    "Mist spirit",
    "Storm spirit",
    "Lake spirit",
    "River spirit",      // 15
    "Sea spirit",
    "Swamp spirit",
    "\n"
  };

const char *spirit_powers[] =
  {
    "accident",
    "alienate",
    "aura",
    "bind",
    "breathe",
    "conceal",
    "confuse",
    "engulf",
    "fear",
    "find",
    "guard",
    "manifest",
    "project",
    "\n"
  };

/* POS_x */
const char *position_types[] =
  {
    "Dead",
    "Mortally wounded",
    "Stunned",
    "Sleeping",
    "Lying",
    "Resting",
    "Sitting",
    "Fighting",
    "Standing",
    "\n"
  };

const char *attack_types[] =
  {
    "Hit",
    "Sting",
    "Whip",
    "Slash",
    "Bite",
    "Bludgeon",
    "Crush",
    "Pound",
    "Claw",
    "Maul",
    "Thrash",
    "Pierce",
    "Punch",
    "Stab",
    "Taser",
    "Shuriken",
    "Throwing knife",
    "Arrow/bolt",
    "Hand Grenade",
    "Grenade Launcher",
    "Rocket",
    "Pistol",
    "Blast",
    "Rifle",
    "Shotgun",
    "Machine Gun",
    "Cannon",
    "Bifurcate",
    "\n"
  };

/* PLR_x */
const char *player_bits[] =
  {
    "KILLER",
    "!USED",
    "FROZEN",
    "DONTSET",
    "NEWBIE",
    "JUST_DIED",
    "CSH",
    "!USED",
    "SITEOK",
    "!SHOUT",
    "!TITLE",
    "DELETED",
    "!DELETE",
    "PACKING",
    "!STAT",
    "LOADRM",
    "INVST",
    "!USED",
    "!USED",
    "OLC",
    "MATRIX",
    "PERCEIVE",
    "PROJECT",
    "SWITCHED",
    "WRITING",
    "MAILING",
    "EDITING",
    "SPELL-CREATE",
    "CUSTOMIZE",
    "!SNOOP",
    "WANTED",
    "NOOOC",
    "!AUTH",
    "EDITCON",
    "REMOTE",
    "PGER",
    "DRIVEBY",
    "RPE",
    "\n"
  };

/* MOB_x */
const char *action_bits[] =
  {
    "SPEC",
    "SENTINEL",
    "SCAVENGER",
    "ISNPC",
    "AWARE",
    "AGGR",
    "STAY-ZONE",
    "WIMPY",
    "AGGR_ORK",
    "AGGR_ELF",
    "AGGR_DWARF",
    "MEMORY",
    "HELPER",
    "!CHARM",
    "DUAL",
    "!EXPLODE",
    "AGGR_TROLL",
    "!BLIND",
    "ASTRAL",
    "GUARD",
    "AGGR_HUMAN",
    "SNIPER",
    "PRIVATE",
    "TRACK",
    "LONESTAR",
    "KNIGHT_ERRENT",
    "SAEDER_KRUPP",
    "TIR_SECURITY",
    "AZTECHNOLOGY",
    "RENRAKU",
    "!KILL",
    "AGGR_DRAGON",
    "INANIMATE",
    "\n"
  };

/* PRF_x */
const char *preference_bits[] =
  {
    "PACIFY",
    "COMPACT",
    "AUTOEX",
    "FIGHTGAG",
    "MOVEGAG",
    "DEAF",
    "!TELL",
    "!RADIO",
    "!NEWBIE",
    "!REPEAT",
    "!WIZ",
    "PKER",
    "QUEST",
    "AFK",
    "C1",
    "C2",
    "NOHASSLE",
    "ROOMFLAGS",
    "HOLYLIGHT",
    "CONNLOG",
    "DEATHLOG",
    "MISCLOG",
    "WIZLOG",
    "SYSLOG",
    "ZONELOG",
    "MSP",
    "ROLLS",
    "!OOC",
    "AUTOINVIS",
    "CHEATLOG",
    "ASSIST",
    "BANLOG",
    "!RPE",
    "!HIRED",
    "GRIDLOG",
    "WRECKLOG",
    "QUESTOR",
    "\n"
  };

/* AFF_x */
const char *affected_bits[] =
  {
    "NOTHING",
    "Invis",
    "Det-align",
    "Det-invis",
    "Det-magic",
    "Sense-life",
    "Waterwalk",
    "Group",
    "Curse",
    "Infravision",
    "Poison",
    "Sleep",
    "No-Track",
    "LL-eyes",
    "Laser-sight",
    "Sneak",
    "Hide",
    "Vision x1",
    "Charm",
    "ACTION",
    "Vision x2",
    "Vision x3",
    "COUNTERATTACK",
    "Stabilize",
    "Petrify",
    "Imp_invis",
    "Blind",
    "APPROACH",
    "Pilot",
    "Rigger",
    "Manning",
    "Designing",
    "Programming",
    "\n"
  };

/* CON_x */
const char *connected_types[] =
  {
    "Playing",                                    // 0
    "Disconnecting",
    "Get name",
    "Confirm name",
    "Get password",
    "Get new PW",                                 // 5
    "Confirm new PW",
    "CharGen",
    "Reading MOTD",
    "Main Menu",
    "Get descript.",                              // 10
    "Changing PW 1",
    "Changing PW 2",
    "Changing PW 3",
    "Self-Delete 1",
    "Self-Delete 2",                              // 15
    "Quit menu",
    "Changing PW Q1",
    "Changing PW Q2",
    "Changing PW Q3",
    "Self-Delete Q1",                             // 20
    "Self-Delete Q2",
    "Creating Spell",
    "Persona Editing",
    "Astral Editing",
    "Physical Editing",      // 25
    "Vehicle Editing",
    "Item Editing",
    "Room Editing",
    "Mob Editing",
    "Quest Editing",      // 30
    "Shop Editing",
    "Zone Editing",
    "Host Editing",
    "IC Editing",
    "Ident Conning",      // 35
    "Ident conned",
    "Ident reading",
    "Ident read",
    "Asking name",
    "\n"
  };

/* WEAR_x - for eq list */
const char *where[] =
  {
    "<used as light>      ",
    "<worn on head>       ",
    "<eyes>               ",
    "<ear>                ",
    "<ear>                ",
    "<worn on face>       ",
    "<worn around neck>   ",
    "<worn around neck>   ",
    "<worn over shoulder> ",
    "<worn about body>    ",
    "<worn on body>       ",
    "<worn underneath>    ",
    "<worn on arms>       ",
    "<slung under arm>    ",
    "<slung under arm>    ",
    "<worn around wrist>  ",
    "<worn around wrist>  ",
    "<worn on hands>      ",
    "<wielded>            ",
    "<held>               ",
    "<worn as shield>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<belly button>       ",
    "<worn about waist>   ",
    "<worn on legs>       ",
    "<worn around ankle>  ",
    "<worn around ankle>  ",
    "<worn as socks>      ",
    "<worn on feet>       ",
    "<patch>              "
  };


const char *hands[] =
  {
    "<right hand>         ",
    "<left hand>          ",
    "<both hands>         ",
  };

const char *wielding_hands[] =
  {
    "<right hand (w)>     ",
    "<left hand (w)>      ",
    "<both hands>         ",
  };

const char *short_where[] =
  {
    "LIGHT",
    "HEAD",
    "EYES",
    "EAR1",
    "EAR2",
    "FACE",
    "NECK1",
    "NECK2",
    "BACK",
    "ABOUT",
    "BODY",
    "UNDER",
    "ARMS",
    "ARM1",
    "ARM2",
    "R_WRIST",
    "L_WRIST",
    "HANDS",
    "WIELD",
    "HELD",
    "SHIELD",
    "FINGER1",
    "FINGER2",
    "FINGER3",
    "FINGER4",
    "FINGER5",
    "FINGER6",
    "FINGER7",
    "FINGER8",
    "BELLY",
    "WAIST",
    "LEGS",
    "ANKLE1",
    "ANKLE2",
    "SOCK",
    "FEET",
    "PATCH"
  };

/* WEAR_x - for stat */
const char *equipment_types[] =
  {
    "Used as light",
    "Worn on head",
    "Eyes",
    "First set on Ears",
    "Second set on Ears",
    "Worn on Face",
    "First worn around Neck",
    "Second worn around Neck",
    "Over the shoulder",
    "Worn about body",
    "Worn on body",
    "Underneath clothes",
    "Worn on arms",
    "Under Left Arm",
    "Under Right Arm",
    "Worn around right wrist",
    "Worn around left wrist",
    "Worn on hands",
    "Wielded",
    "Held",
    "Worn as shield",
    "Worn on right ring finger",
    "Worn on left ring finger",
    "Worn on right index finger",
    "Worn on left index finger",
    "Worn on right middle finger",
    "Worn on left middle finger",
    "Worn on right pinkie finger",
    "Worn on left pinkie finger",
    "In Belly Button",
    "Worn around waist",
    "Worn on legs",
    "Around left ankle",
    "Around right ankle",
    "between feet and shoes",
    "Worn on feet",
    "Patch",
    "\n"
  };



/* ITEM_x (ordinal object types) */
const char *item_types[] =
  {
    "UNDEFINED",
    "Light",
    "Workshop",
    "Camera",
    "Staff",
    "Weapon",
    "Bow/X-bow",
    "Arrow/bolt",
    "Treasure",
    "Gyro Harness",
    "Potion",
    "Worn",
    "Other",
    "Trash",
    "DocWagon",
    "Container",
    "Radio",
    "Liq Container",
    "Key",
    "Food",
    "Money",
    "Phone",
    "Bioware",
    "Fountain",
    "Cyberware",
    "Cyberdeck",
    "Program",
    "Gun Clip",
    "Gun Accessory",
    "Spell Formula",
    "Kit",
    "Focus",
    "Patch",
    "Climbing gear",
    "Quiver",
    "Decking Accessory",
    "Rigger Control Deck",
    "Skillsoft",
    "Vehicle Mod",
    "Holster",
    "Program Design",
    "\n"
  };

/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] =
  {
    "TAKE",
    "FINGER",
    "NECK",
    "BODY",
    "HEAD",
    "LEGS",
    "FEET",
    "HANDS",
    "ARMS",
    "SHIELD",
    "ABOUT",
    "WAIST",
    "WRIST",
    "WIELD",
    "HOLD",
    "EYES",
    "EAR",
    "UNDER",
    "BACK",
    "ANKLE",
    "SOCK",
    "BELLY",
    "ARM",
    "FACE",
    "PATCH",
    "\n"
  };

/* ITEM_x (extra bits) */
const char *extra_bits[] =
  {
    "GLOW",
    "HUM",
    "!RENT",
    "!DONATE",
    "!INVIS",
    "INVISIBLE",
    "MAGIC",
    "!DROP",
    "BLESS",
    "!SELL",
    "CORPSE",
    "GOD_ONLY",
    "TWO_HANDS",
    "STARTER",
    "VOLATILE",
    "WIZLOAD",
    "NOTROLL",
    "NOELF",
    "NODWARF",
    "NOORK",
    "NOHUMAN",
    "SNIPER",
    "IMMLOAD",
    "\n"
  };

/* APPLY_x */
const char *apply_types[] =
  {
    "Nothing",
    "Body",
    "Quickness",
    "Strength",
    "Charisma",
    "Intelligence",
    "Willpower",
    "Essence",
    "Magic",
    "Reaction",
    "Age",
    "Weight",
    "Height",
    "Max Mental",
    "Max Physical",
    "Nothing",
    "Ballistic",
    "Impact",
    "Astral Pool",
    "Nothing",
    "Combat Pool",
    "Hacking Pool",
    "Magic Pool",
    "Init Dice",
    "Target Numbers",
    "Control Pool",
    "Nothing",
    "Nothing",
    "\n"
  };

const char *program_types[] =
  {
    "None",
    "Bod",
    "Evasion",
    "Masking",
    "Sensor",
    "Attack",//5
    "Slow",
    "Medic",
    "Snooper",
    "BattleTac",
    "Compressor",//10
    "Analyze",
    "Decrypt",
    "Deception",
    "Relocate",
    "Sleaze",//15
    "Scanner",
    "Browse",
    "Read/Write",
    "Track",
    "Armour",//20
    "Camo",
    "Crash",
    "Defuse",
    "Evaluate",
    "Validate",//25
    "Swerve",
    "Suite",
    "Commlink",
    "Cloak",
    "Lock-On",//30
    "\n"
  };

int program_multiplier[] =
  {
    0,
    3,
    3,
    2,
    2,
    0,
    4,
    4,
    2,
    5,
    2,//10
    3,
    1,
    2,
    2,
    3,//15
    3,
    1,
    2,
    8,
    3,//20
    3,
    3,
    2,
    2,
    4,//25
    3,
    15,
    1,
    3,
    3//30
  };

int attack_multiplier[] = { 0, 2, 3, 4, 5 };

const char *log_types[] =
  {
    "CONNLOG",
    "DEATHLOG",
    "MISCLOG",
    "WIZLOG",
    "SYSLOG",
    "ZONELOG",
    "CHEATLOG",
    "WIZITEMLOG",
    "BANLOG",
    "GRIDLOG",
    "WRECKLOG",
    "\n"
  };

/* CONT_x */
const char *container_bits[] =
  {
    "CLOSEABLE",
    "PICKPROOF",
    "CLOSED",
    "LOCKED",
    "\n",
  };

/* one-word alias for each drink */
/* LIQ_x */
/* New and improved liquids by Jordan */
const char *drinks[] =
  {
    "water",
    "soda",
    "coffee",
    "milk",
    "juice",
    "tea",
    "soykaf",
    "smoothie",
    "sports juice",
    "espresso",
    "beer",
    "ale",
    "lager",
    "import beer",
    "microbrew beer",
    "malt liquor",
    "cocktail",
    "margarita",
    "liquor",
    "vodka",
    "tequila",
    "brandy",
    "rum",
    "whiskey",
    "gin",
    "champagne",
    "red wine",
    "white wine",
    "blood",
    "hot sauce",
    "piss",
    "local speciality",
    "fuckup juice",
    "\n"
  };

/* one-word alias for each drink */
const char *drinknames[] =
  {
    "water",
    "soda",
    "coffee",
    "milk",
    "juice",
    "tea",
    "soykaf",
    "smoothie",
    "sportsjuice",
    "espresso",
    "beer",
    "ale",
    "lager",
    "import",
    "microbrew",
    "maltliquor",
    "cocktail",
    "margarita",
    "liquor",
    "vodka",
    "tequila",
    "brandy",
    "rum",
    "whiskey",
    "gin",
    "champagne",
    "redwine",
    "whitewine",
    "blood",
    "hotsauce",
    "piss",
    "local",
    "fuckup",
    "\n"
  };


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
                       {0, 1, 10},     // Water
                       {0, 1, 5},      // Soda
                       {0, 1, 4},      // Coffee
                       {0, 3, 6},      // Milk
                       {0, 1, 5},      // Juice
                       {0, 1, 6},      // Tea
                       {0, 2, 6},      // Soykaf
                       {0, 2, 3},      // Smoothie
                       {0, 1, 11},     // Sports Juice
                       {0, 1, 2},      // Espresso
                       {3, 2, -1},     // Beer
                       {3, 2, -1},     // Ale
                       {3, 2, -1},     // Lager
                       {3, 2, 0},      // Import
                       {3, 2, -1},     // Microbrew
                       {5, 3, -2},     // Malt Liquor
                       {6, 0, 1},      // Cocktail
                       {6, 0, 1},      // Margarita
                       {8, 0, 1},      // Liquor
                       {8, 0, 1},      // Vodka
                       {9, 0, 0},      // Tequila
                       {7, 0, -1},      // Brandy
                       {8, 0, 0},      // Rum
                       {8, 0, -1},      // Whiskey
                       {7, 0, 1},      // Gin
                       {5, 0, 1},      // Champagne
                       {4, 1, 2},      // Red Wine
                       {4, 1, 2},      // White Wine
                       {0, 2, -1},     // Blood
                       {0, 1, -1},     // Hot Sauce
                       {0, 1, -4},     // Urine
                       {6, 0, -1},     // Local
                       {12, 0, -1}     // Fuckup Juice
                     };


/* color of the various drinks */
const char *color_liquid[] =
  {

    "clear",    // Water
    "dark",    // Soda
    "dark brown",   // Coffee
    "white",    // Milk
    "colourful",   // Juice
    "golden",    // Tea
    "white",    // Soykaf
    "colourful",   // Smoothie
    "clear",    // Sports Juice
    "black",    // Espresso
    "brown",    // Beer
    "dark brown",   // Ale
    "brown",    // Lager
    "light brown",  // Import
    "brown",    // Microbrew
    "brownish",     // Malt Liquor
    "colourful",   // Cocktail
    "colourful",   // Margarita
    "clear",    // Liquor
    "clear",    // Vodka
    "golden brown", // Tequila
    "amber",    // Brandy
    "dark brown",   // Rum
    "dark amber",   // Whiskey
    "clear",    // Gin
    "yellowish",   // Champagne
    "dark red",   // Red Wine
    "light yellow", // White Wine
    "dark red",   // Blood
    "blazing red",  // Hot Sauce
    "bright yellow", // Urine
    "dark amber",    //Local
    "thick"          //fuckup juice
  };

const char *fullness[] =
  {
    "less than half ",
    "about half ",
    "more than half ",
    ""
  };
/*
Let it do the weight by Str x 10Kg for load
*/
struct str_app_type str_app[] =
  {
    {
      0, 0,   0,  0
    },
    { 1, 1,  10,  3 },//30
    { 2, 2,  20,  6 },//60
    { 3, 3,  30,  9 },//90
    { 4, 4,  40, 12 },//120
    { 5, 5,  50, 15 },//150
    { 6, 6,  60, 18 },//180
    { 7, 7,  70, 21 },//210
    { 8, 8,  80, 24 },//240
    { 9, 9,  90, 27 },//270
    { 10, 10, 100, 30 },//300
    { 11, 11, 110, 33 },//330
    { 12, 12, 120, 36 },//360
    { 13, 13, 130, 39 },//390
    { 14, 14, 140, 42 },//420
    { 15, 15, 150, 45 },//450
    { 16, 16, 160, 48 },//480
    { 17, 17, 170, 51 },//510
    /* Mortals can't get past 15 anyway */
    { 18, 18, 540, 54 },
    { 19, 19, 570, 57 },
    { 20, 20, 600, 60 },
    { 21, 21, 630, 63 },
    { 22, 22, 660, 66 },
    { 23, 23, 690, 69 },
    { 24, 24, 720, 72 },
    { 25, 25, 750, 75 },
    { 26, 26, 780, 78 },
    { 27, 27, 810, 81 },
    { 28, 28, 840, 84 },
    { 29, 29, 870, 87 },
    { 30, 30, 900, 90 },
    { 31, 31, 930, 93 },
    { 32, 32, 960, 96 },
    { 33, 33, 990, 99 },
    { 34, 34, 1020, 102 },
    { 35, 35, 1050, 105 },
    { 36, 36, 1080, 108 },
    { 37, 37, 1110, 111 },
    { 38, 38, 1140, 114 },
    { 39, 39, 1170, 117 },
    { 40, 40, 1200, 120 },
    { 41, 41, 1230, 123 },
    { 42, 42, 1250, 125 },
    { 43, 43, 1280, 128 },
    { 44, 44, 1310, 131 },
    { 45, 45, 1340, 134 },
    { 46, 46, 1370, 137 },
    { 47, 47, 1400, 140 },
    { 48, 48, 1430, 143 },
    { 49, 49, 1470, 147 },
    { 50, 50, 1500, 150 }
  };

const char *adept_powers[] =
  {
    "None",
    "Astral Perception",
    "Combat Sense",
    "Blind Fighting",
    "Quick Strike",
    "Killing Hands",
    "Nerve Strike",
    "Smashing Blow",
    "Distance Strike",
    "Increased Reflexes",
    "Boosted Strength",
    "Boosted Quickness",
    "Boosted Body",
    "Improved Strength",
    "Improved Quickness",
    "Improved Body",
    "Improved Perception",
    "Improved Sense (Low Light)",
    "Improved Sense (Thermographic)",
    "Improved Sense (Magnification)",
    "Magic Resistance",
    "Pain Resistance",
    "Temperature Tolerance",
    "Spell Shroud",
    "True Sight",
    "Missile Parry",
    "Missile Mastery",
    "Mystic Armour",
    "Rapid Healing",
    "Freefall",
    "Ironwill"
  };


const char *spells[] =
  {
    "!RESERVED!",                 /* 0 - reserved */
    "X",
    "XX",
    "analyze device",
    "anti bullet barrier",
    "anti spell barrier",      // 5
    "antidote",
    "armor",
    "chaos",
    "chaotic world",
    "clairvoyance",            // 10
    "clout",
    "combat sense",
    "confusion",
    "cure disease",
    "death touch",             // 15
    "detect alignment",
    "detect invisibility",
    "detect magic",
    "heal",
    "hellblast",               // 20
    "improved invisibility",
    "influence",
    "invisibility",
    "manaball",
    "manablast",               // 25
    "mana bolt",
    "mana cloud",
    "mana dart",
    "mana missile",
    "overstimulation",         // 30
    "petrify",
    "powerball",
    "powerblast",
    "power bolt",
    "power cloud",             // 35
    "power dart",
    "power missile",
    "ram",
    "ram touch",
    "resist pain",             // 40
    "shape change",
    "stabilize",
    "stunball",
    "stunblast",
    "stun bolt",               // 45
    "stun cloud",
    "stun missile",
    "stun touch",
    "toxic wave",
    "power shaft",
    "power burst",
    "mana shaft",
    "mana burst",
    "stun shaft",
    "stun burst",
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",    // 60
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",    // 65
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",    // 70
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",    // 75
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",    // 80
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!",    //85
    "!UNUSED!",
    "analyze magic",
    "analyze person",
    "decrease attribute",
    "element ball",            // 90
    "element bolt",
    "element cloud",
    "element dart",
    "element missile",
    "increase attribute",      // 95
    "increase reflexes",
    "light",
    "poison",
    "teleport"
  };

struct skill_data skills[] =
  {
    {"OMGWTFBBQ", BOD, 0
    },
    {"athletics", BOD, 0},
    {"armed combat", STR, 0},
    {"edged weapons", STR, 0},
    {"pole arms", STR, 0},
    {"whips & flails", QUI, 0},
    {"clubs", STR, 0},
    {"unarmed combat", STR, 0},
    {"grapple", STR, 0},
    {"cyber implants", STR, 0},
    {"firearms", STR, 0},
    {"pistols", QUI, 0},
    {"rifles", QUI, 0},
    {"shotguns", QUI, 0},
    {"assault rifles", QUI, 0},
    {"sub-machine guns", QUI, 0},
    {"grenade launchers", INT, 0},
    {"tasers", QUI, 0},
    {"gunnery", INT, 0},
    {"machine guns", STR, 0},
    {"missile launchers", INT, 0},
    {"assault cannons", STR, 0},
    {"artillery", STR, 0},
    {"projectiles", STR, 0},
    {"bows - Obsolete", 0, 0},
    {"crossbows - Obsolete", 0, 0},
    {"throwing weapons", STR, 0},
    {"non-aerodynamic - OBSOLETE", 0, 0},
    {"cyberterminal design", INT, 1},
    {"demolitions", INT, 0},
    {"computers", INT, 0},
    {"electronics", INT, 0},
    {"computer B/R", INT, 0},
    {"biotech", INT, 0},
    {"medical - OBSOLETE", 0, 0},
    {"cybersurgery - OBSOLETE", 0, 0},
    {"biology - OBSOLETE", 0, 0},
    {"leadership", CHA, 0},
    {"interrogation", CHA, 0},
    {"negotiation", CHA, 0},
    {"magical theory", INT, 1},
    {"conjuring", WIL, 0},
    {"sorcery", WIL, 0},
    {"shamanic studies", INT, 1},
    {"corporate etiquette", CHA, 0},
    {"media etiquette", CHA, 0},
    {"street etiquette", CHA, 0},
    {"tribal etiquette", CHA, 0},
    {"elf etiquette", CHA, 0},
    {"program design (Combat)", INT, 1},
    {"program design (Defensive)", INT, 1},
    {"program design (Operational)", INT, 1},
    {"program design (Special)", INT, 1},
    {"program design (Cyberterminal)", INT, 1},
    {"data brokerage", INT, 1},
    {"aura reading", INT, 0},
    {"stealth", QUI, 0},
    {"steal", 0, 0},
    {"track", 0, 0},
    {"climbing", 0, 0},
    {"driving/Bike", REA, 0},
    {"driving/Fixed Wing", REA, 0},
    {"driving/Car", REA, 0},
    {"driving/Truck", REA, 0},
    {"BR/Bike", INT, 0},
    {"BR/Car", INT, 0},
    {"BR/Drone", INT, 0},
    {"BR/Truck", INT, 0},
    {"driving/Submarine", REA, 0},
    {"driving/Vector Thrust", REA, 0},
    {"English", INT, 1},
    {"Sperethiel", INT, 1},
    {"Spanish", INT, 1},
    {"Japanese", INT, 1},
    {"Chinese", INT, 1},
    {"Korean", INT, 1},
    {"Italian", INT, 1},
    {"Russian", INT, 1},
    {"German", INT, 1},
    {"Sioux", INT, 1},
    {"Makaw", INT, 1},
    {"Crow", INT, 1},
    {"Salish", INT, 1},
    {"Ute", INT, 1},
    {"Navajo", INT, 1},
    {"German", INT, 1}
  };

const char *spell_wear_off_msg[] =
  {
    "RESERVED DB.CC",             /* 0 */
    "!X!",                        /* 1 */
    "!XX!",
    "!Analyze Device!",
    "You feel your shimmer of protection dissolve.",
    "You feel more vulnerable to magical attacks.",               /* 5 */
    "!Antidote!",
    "You feel your skin soften.",
    "Chaos wears off.",
    "Chaotic world wears off.",
    "!Clairvoyance!",                                   // 10
    "!Clout!",
    "The awareness and tension recede.",
    "You feel less confused.",
    "!Cure disease!",
    "!Death touch!",                                    // 15
    "You no longer see creatures' auras.",
    "You are no longer aware of essence.",
    "You feel less in touch with the astral plane",
    "!Heal!",
    "!Hellblast!",                                      // 20
    "The invisibility wears off.",
    "You feel in control of yourself.",
    "The invisibility wears off.",
    "!Manaball!",
    "!Manablast!",                                      // 25
    "!Mana bolt!",
    "!Mana cloud!",
    "!Mana dart!",
    "!Mana missile!",
    "!Overstimulation!",                                // 30
    "Your muscles regain their flexibility.",
    "!Powerball!",
    "!Powerblast!",
    "!Power bolt!",
    "!Power cloud!",                                    // 35
    "!Power dart!",
    "!Power missile!",
    "!Ram!",
    "!Ram touch!",
    "You no longer resist pain.",                       // 40
    "Your body returns to its natural form.",
    "!Stabilize!",
    "!Stunball!",
    "!Stunblast!",
    "!Stun bolt!",                                      // 45
    "!Stun cloud!",
    "!Stun missile!",
    "!Stun touch!",
    "!Toxic wave!",
    "!UNUSED!",                                         // 50
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 55
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 60
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 65
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 70
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 75
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 80
    "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", "!UNUSED!", // 85
    "!UNUSED!",
    "!Analyze magic!",
    "!Analyze person!",
    "You feel more in control of yourself.",
    "!Element ball!",                                   // 90
    "!Element bolt!",
    "!Element cloud!",
    "!Element dart!",
    "!Element missile!",
    "You feel a magical presense depart your being.",   // 95
    "Your enhanced reflexes return to normal.",
    "The room seems dimmer.",
    "The sickness wears off.",
    "!Teleport!"
  };

int rev_dir[] =
  {
    4,
    5,
    6,
    7,
    0,
    1,
    2,
    3,
    9,
    8
  };

const char *weekdays[7] =
  {
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
    "Sun"
  };

const char *month_name[12] =
  {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
  };

const char *veh_type[11] =
  {
    "Drone",
    "Bike",
    "Car",
    "Truck",
    "Fixed Wing",
    "Rotorcraft",
    "Vector Thrust",
    "Hovercraft",
    "Motorboat",
    "Ship",
    "Lighter Than Air"
  };

const char *mod_name[15] =
  {
    "Suspension",
    "Engine",
    "Steering",
    "Tires",
    "Brakes",
    "Armor Plating",
    "Computer",
    "Phones",
    "Sensors",
    "ECM",
    "Ignition",
    "Alarm",
    "Seats",
    "Body Style",
    "Radio"
  };

const char *veh_aff[12] =
  {
    "Nothing",
    "Handling",
    "Speed",
    "Acceleration",
    "Body",
    "Armor",
    "Sig",
    "Autonav",
    "Seating",
    "Load",
    "Sensors",
    "Pilot"
  };

const char *veh_flag[] =
  {
    "Nothing",
    "Can Fly",
    "Amphibious"
  };

const char *jurid[] =
  {
    "Seattle",
    "Portland",
    "\n"
  };

const char *host_sec[] =
  {
    "^BBlue",
    "^GGreen",
    "^YOrange",
    "^RRed",
    "^LBlack"
  };

const char *host_type[] =
  {
    "Datastore",
    "LTG",
    "RTG",
    "PLTG",
    "Security",
    "Controller"
  };

const char *intrusion[] =
  {
    "Easy",
    "Average",
    "Hard"
  };

const char *alerts[] =
  {
    "No Alert",
    "Passive",
    "Active",
    "Shutdown"
  };

const char *ic_type[] =
  {
    "^WCrippler^n",
    "^WKiller^n",
    "^WProbe^n",
    "^WScramble^n",
    "^WTar Baby^n",
    "^WScout^n",
    "Trace",
    "Blaster",
    "Ripper",
    "Sparky",
    "Tar Pit",
    "^LLethal Black^n",
    "^LNon-Lethal Black^n"
  };

const char *ic_option[] =
  {
    "UNDEF",
    "ARMOUR",
    "CASCADING",
    "EXPERT DEFENSE",
    "EXPERT OFFENSE",
    "SHIELD",
    "SHIFT",
    "TRAP"
  };

const char *ic_option_long[] =
  {
    "Undefined",
    "Armour, ",
    "Cascading Attack, ",
    "Expert Defense, ",
    "Expert Offense, ",
    "Shield, ",
    "Shift, ",
    "Trap  "
  };

const char *ammo_type[] =
  {
    "Normal",
    "APDS",
    "Explosive",
    "EX",
    "Flechette",
    "Gel"
  };

const char *positions[] =
  {
    " is lying here, dead",
    " is lying here, mortally wounded",
    " is lying here, stunned",
    " is sleeping here",
    " is lying here",
    " is resting here",
    " is sitting here",
    "!FIGHTING!",
    " is standing here"
  };

int racial_limits[][2][6] = {
                              {{ 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0 }}, // undef
                              {{ 0, 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0, 0 }}, // undef
                              {{ 6, 6, 6, 6, 6, 6 },{ 9, 9, 9, 9, 9, 9 }}, //human
                              {{ 7, 6, 8, 6, 6, 7 },{ 11, 9, 12, 9, 9, 11 }}, //dwarf
                              {{ 6, 7, 6, 8, 6, 6 },{ 9, 11, 9, 12, 9, 9 }}, // elf
                              {{ 9, 6, 8, 5, 5, 6 },{ 14, 9 , 12, 8, 8, 9 }}, // ork
                              {{ 11, 5, 10, 4, 4, 6 },{ 17, 8, 15, 6, 6, 9 }}, // troll
                              {{ 11, 5, 12, 4, 4, 6 },{ 17, 8, 18, 6, 6, 9 }}, // cyclops
                              {{ 7, 6, 8, 6, 6, 7 },{ 11, 9, 12, 9, 9, 11 }}, // koborokuru
                              {{ 10, 5, 9, 4, 6, 6 },{ 15, 8, 13, 6, 9, 9 }}, // fomori
                              {{ 8, 6, 7, 6, 6, 7 },{ 12, 9, 11, 9, 9, 11 }}, // menehune
                              {{ 8, 6, 8, 5, 6, 6 },{ 12, 9, 12, 8, 9, 9 }}, // hobgoblin
                              {{ 11, 5, 11, 4, 4, 6 },{ 17, 8, 17, 6, 6, 9 }}, // giant
                              {{ 7, 6, 7, 6, 6, 8 },{ 11, 9, 11, 9, 9, 12 }}, // gnome
                              {{ 8, 6, 8, 5, 5, 7 },{ 12, 9, 12, 8, 8, 11 }}, // oni
                              {{ 6, 6, 6, 8, 6, 7 },{ 9, 9, 9, 12, 9, 11 }}, // wakyambi
                              {{ 9, 6, 8, 6, 5, 6 },{ 14, 9, 12, 9, 8, 9 }}, // ogre
                              {{ 10, 5, 9, 5, 5, 6 },{ 15, 8, 14, 8, 8, 9 }},  // minotaur
                              {{ 9, 5, 8, 5, 5, 7 },{ 14, 8, 12, 8, 8, 11 }}, // satyr
                              {{ 6, 8, 6, 8, 6, 6 },{ 9, 12, 9, 12, 9, 9 }} // night one
                            };


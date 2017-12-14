/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_CC__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iomanip>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <time.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#include <process.h>
#define getpid() _getpid()
#else
#include <unistd.h>
#endif

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "file.h"
#include "db.h"
#include "newdb.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "newmatrix.h"
#include "memory.h"
#include "dblist.h"
#include "quest.h"
#include "shop.h"
#include "constants.h"
#include "vtable.h"
#include <new>

extern void calc_weight(struct char_data *ch);
extern void read_spells(struct char_data *ch);
extern struct obj_data *find_obj(int num);
extern void obj_to_cyberware(struct obj_data *, struct char_data *);
extern void obj_from_cyberware(struct obj_data *);
extern void obj_to_bioware(struct obj_data *, struct char_data *);
extern void obj_from_bioware(struct obj_data *);
extern char *cleanup(char *dest, const char *src);


/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

// beginning of mud time
unsigned long beginning_of_time = 2157880000U;
long mud_boot_time = 0;
struct room_data *world = NULL; /* array of rooms   */
rnum_t top_of_world = 0; /* ref to top element of world  */

struct host_data *matrix = NULL;
rnum_t top_of_matrix = 0;
struct matrix_icon *ic_proto;
struct index_data *ic_index;
rnum_t top_of_ic = 0;
struct matrix_icon *icon_list = NULL;

struct char_data *character_list = NULL; /* global linked list of
       * chars  */
struct index_data *mob_index; /* index table for mobile file  */
struct char_data *mob_proto; /* prototypes for mobs   */
rnum_t top_of_mobt = 0; /* top of mobile index table  */
int mob_chunk_size = 100;       // default to 100
int top_of_mob_array = 0;

struct obj_data *object_list = NULL; /* global linked list of objs  */
struct index_data *obj_index; /* index table for object file  */
struct obj_data *obj_proto; /* prototypes for objs   */
rnum_t top_of_objt = 0; /* top of object index table  */
int top_of_obj_array = 0;
int obj_chunk_size = 100;       // default to 100

struct zone_data *zone_table; /* zone table    */
rnum_t top_of_zone_table = 0;/* top element of zone tab  */
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages  */

struct quest_data *quest_table = NULL; // array of quests
rnum_t top_of_questt = 0;
int top_of_quest_array = 0;
int quest_chunk_size = 25;

struct shop_data *shop_table = NULL;   // array of shops
rnum_t top_of_shopt = 0;            // ref to top element of shop_table
int top_of_shop_array = 0;
int shop_chunk_size = 25;

int top_of_matrix_array = 0;
int top_of_ic_array = 0;
int top_of_world_array = 0;
int world_chunk_size = 100;     /* size of world to add on each reallocation */
int olc_state = 1;              /* current olc state */
int _NO_OOC_  = 0;  /* Disable the OOC Channel */

struct veh_data *veh_list;
struct index_data *veh_index;
struct veh_data *veh_proto;
int top_of_veht = 0;
int top_of_veh_array = 0;
int veh_chunk_size = 100;

class objList ObjList;          // contains the global list of perm objects

time_t boot_time = 0;           /* time of mud boot              */
int restrict = 0;               /* level of game restriction     */
rnum_t r_mortal_start_room;     /* rnum of mortal start room     */
rnum_t r_immort_start_room;     /* rnum of immort start room     */
rnum_t r_frozen_start_room;     /* rnum of frozen start room     */
rnum_t r_newbie_start_room;     /* rnum of newbie start room     */

char *credits = NULL;           /* game credits                  */
char *news = NULL;              /* mud news                      */
char *motd = NULL;              /* message of the day - mortals */
char *imotd = NULL;             /* message of the day - immorts */
char *help = NULL;              /* help screen                   */
char *info = NULL;              /* info page                     */
char *immlist = NULL;           /* list of lower gods            */
char *background = NULL;        /* background story              */
char *handbook = NULL;          /* handbook for new immortals    */
char *policies = NULL;          /* policies page                 */

class helpList Help;
class helpList WizHelp;

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;       /* the infomation about the weather */
struct player_special_data dummy_mob;   /* dummy spec area for mobs      */
struct phone_data *phone_list = NULL;
int market[5] = { 5000, 5000, 5000, 5000, 5000 };

/* local functions */
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(File &fl, int mode);
void parse_room(File &in, long nr);
void parse_mobile(File &in, long nr);
void parse_object(File &in, long nr);
void parse_shop(File &fl, long virtual_nr);
void parse_quest(File &fl, long virtual_nr);
void parse_host(File &fl, long nr);
void parse_ic(File &fl, long nr);
void load_zones(File &fl);
void load_saved_veh();
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void assign_johnsons(void);
void randomize_shop_prices(void);
int is_empty(int zone_nr);
void reset_zone(int zone, int reboot);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, char *message);
void reset_time(void);
void clear_char(struct char_data * ch);
long asciiflag_conv(char *);
void kill_ems(char *);
void parse_veh(File &fl, long virtual_nr);
/* external functions */
extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time(int mode);
void boot_social_messages(void);
void sort_commands(void);
void load_banned(void);
void init_quests(void);
extern void TransportInit();
extern void TransportEnd();
extern void BoardInit();
extern void BoardEnd();
extern void affect_total(struct char_data * ch);
void load_consist(void);

/* external vars */
extern int no_specials;
/* external ascii pfile vars */
extern struct pclean_criteria_data pclean_criteria[];
extern int selfdelete_fastwipe;
extern const char *pc_race_types[];
/* memory objects */
extern class memoryClass *Mem;
/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */

ACMD(do_reboot)
{
  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
    file_to_string_alloc(HELP_KWRD_FILE, &help);
    WizHelp.RebootIndex(TRUE);
    Help.RebootIndex(FALSE);
  } else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  else if (!str_cmp(arg, "news"))
    file_to_string_alloc(NEWS_FILE, &news);
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "xhelp"))
    Help.RebootIndex(FALSE);
  else if (!str_cmp(arg, "wizhelp"))
    WizHelp.RebootIndex(TRUE);
  else {
    send_to_char("Unknown reboot option.\r\n", ch);
    return;
  }

  send_to_char(OK, ch);
}

void boot_world(void)
{
  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Loading vehicles and generating index.");
  index_boot(DB_BOOT_VEH);

  log("Loading quests.");
  index_boot(DB_BOOT_QST);

  log("Loading shops.");
  index_boot(DB_BOOT_SHP);

  log("Loading Matrix Hosts.");
  index_boot(DB_BOOT_MTX);

  log("Loading IC.");
  index_boot(DB_BOOT_IC);

  log("Renumbering rooms.");
  renum_world();

  log("Renumbering zone table.");
  renum_zone_table();

  log("Reloading Consistency Files.");
  load_consist();
}

/* body of the booting system */
void DBInit()
{
  int i;

  log("DBInit -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(HELP_KWRD_FILE, &help);

  log("Creating Help Indexes.");
  Help.CreateIndex(FALSE);
  WizHelp.CreateIndex(TRUE);

  log("Booting World.");
  boot_world();

  // DEFUNCT
  //  log("Generating player index.");
  //  build_player_index();
  log("Loading player index.");
  playerDB.Load();

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();
  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Johnsons.");
    assign_johnsons();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    randomize_shop_prices();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
  }

  log("Sorting command list.");
  sort_commands();

  log("Initializing board system:");
  BoardInit();

  log("Booting mail system.");
  scan_file();
  log("Reading banned site list.");
  load_banned();

  log("Initializing transportation system:");
  TransportInit();

  for (i = 0; i <= top_of_zone_table; i++) {
    log("Resetting %s (rooms %d-%d).", zone_table[i].name,
        (i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);

    reset_zone(i, 1);
  }

  log("Booting houses.");
  House_boot();
  boot_time = time(0);

  log("Loading Saved Vehicles.");
  load_saved_veh();
  log("Boot db -- DONE.");
}

void DBEnd()
{
  // more to come as the code gets cleaned up
  BoardEnd();

  TransportEnd();
}

/* reset the time in the game from file
   weekday is lost on reboot.... implement
   something different if this is mission
   breaking behavior */
void reset_time(void)
{
  extern struct time_info_data mud_time_passed(time_t t2, time_t t1);

  mud_boot_time = time(0);
  time_info = mud_time_passed(beginning_of_time, time(0));

  if (time_info.year < 2062)
    time_info.year = 2062;
  time_info.minute = 0;
  time_info.weekday = 0;

  if (time_info.hours <= 5)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 6)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 18)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 19)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %d/%d/%d %d:00.", time_info.month,
      time_info.day, time_info.year,
      (time_info.hours % 12) == 0 ? 12 : time_info.hours % 12);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  if(time_info.day < 7)
    weather_info.moonphase = MOON_NEW;
  else if(time_info.day > 7 && time_info.day < 14)
    weather_info.moonphase = MOON_WAX;
  else if(time_info.day > 14 && time_info.day < 21)
    weather_info.moonphase = MOON_FULL;
  else
    weather_info.moonphase = MOON_WANE;

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
  weather_info.lastrain = 10;
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}

void index_boot(int mode)
{
  char *index_filename, *prefix;
  FILE *index, *db_file;
  int rec_count = 0;

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_QST:
    prefix = QST_PREFIX;
    break;
  case DB_BOOT_VEH:
    prefix = VEH_PREFIX;
    break;
  case DB_BOOT_IC:
  case DB_BOOT_MTX:
    prefix = MTX_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand to index_boot!");
    exit(1);
    break;
  }

  if (mode == DB_BOOT_IC)
    index_filename = ICINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s/%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    sprintf(buf1, "Error opening index file '%s'", buf2);
    perror(buf1);
    exit(1);
  }
  /* first, count the number of records in the file so we can calloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      /* geez, no need to not boot just cause it's not found... */
      //perror(buf2);
      //exit(1);
      sprintf(buf, "Unable to find file %s.", buf2);
      mudlog(buf, NULL, LOG_SYSLOG, TRUE);
    } else {
      if (mode == DB_BOOT_ZON)
        rec_count++;
      else
        rec_count += count_hash_records(db_file);
      fclose(db_file);
    }

    fscanf(index, "%s\n", buf1);
  }
  if (!rec_count) {
    log("SYSERR: boot error - 0 records counted");
    exit(1);
  }
  rec_count++;

  switch (mode) {
  case DB_BOOT_WLD:
    // here I am booting with 100 extra slots for creation
    world = new struct room_data[rec_count + world_chunk_size];
    memset((char *) world, 0, (sizeof(struct room_data) *
                               (rec_count + world_chunk_size)));
    top_of_world_array = rec_count + world_chunk_size; // assign the real size of the array
    break;
  case DB_BOOT_MTX:
    // here I am booting with 100 extra slots for creation
    matrix = new struct host_data[rec_count + world_chunk_size];
    memset((char *) matrix, 0, (sizeof(struct host_data) *
                                (rec_count + world_chunk_size)));
    top_of_matrix_array = rec_count + world_chunk_size; // assign the real size of the array
    break;
  case DB_BOOT_IC:
    // here I am booting with 100 extra slots for creation
    ic_proto = new struct matrix_icon[rec_count + mob_chunk_size];
    memset((char *) ic_proto, 0, (sizeof(struct matrix_icon) *
                                  (rec_count + mob_chunk_size)));
    ic_index = new struct index_data[rec_count + mob_chunk_size];
    memset((char *) ic_index, 0, (sizeof(struct index_data) *
                                  (rec_count + mob_chunk_size)));
    top_of_ic_array = rec_count + mob_chunk_size; // assign the real size of the array
    break;
  case DB_BOOT_MOB:
    // here I am booting with 100 extra slots for creation
    mob_proto = new struct char_data[rec_count + mob_chunk_size];
    memset((char *) mob_proto, 0, (sizeof(struct char_data) *
                                   (rec_count + mob_chunk_size)));

    mob_index = new struct index_data[rec_count + mob_chunk_size];
    memset((char *) mob_index, 0, (sizeof(struct index_data) *
                                   (rec_count + mob_chunk_size)));

    top_of_mob_array = rec_count + mob_chunk_size; // assign the real size of the array
    break;
  case DB_BOOT_OBJ:
    // here I am booting with 100 extra slots for creation
    obj_proto = new struct obj_data[rec_count + obj_chunk_size];
    memset((char *) obj_proto, 0, (sizeof(struct obj_data) *
                                   (rec_count + obj_chunk_size)));

    obj_index = new struct index_data[rec_count + obj_chunk_size];
    memset((char *) obj_index, 0, (sizeof(struct index_data) *
                                   (rec_count + obj_chunk_size)));

    top_of_obj_array = rec_count + obj_chunk_size;
    break;
  case DB_BOOT_VEH:
    veh_proto = new struct veh_data[rec_count + veh_chunk_size];
    memset((char *) veh_proto, 0, (sizeof(struct veh_data) *
                                   (rec_count + veh_chunk_size)));
    veh_index = new struct index_data[rec_count + veh_chunk_size];
    memset((char *) veh_index, 0, (sizeof(struct index_data) * (rec_count + veh_chunk_size)));
    top_of_veh_array = rec_count + veh_chunk_size;
    break;

  case DB_BOOT_ZON:
    // the zone table is pretty small, so it is no biggie
    zone_table = new struct zone_data[rec_count];
    memset((char *) zone_table, 0, (sizeof(struct zone_data) * rec_count));
    break;
  case DB_BOOT_SHP:
    shop_table = new struct shop_data[rec_count + shop_chunk_size];
    memset((char *) shop_table, 0, (sizeof(struct shop_data) *
                                    (rec_count + shop_chunk_size)));
    top_of_shop_array = rec_count + shop_chunk_size;
    break;
  case DB_BOOT_QST:
    quest_table = new struct quest_data[rec_count + quest_chunk_size];
    memset((char *) quest_table, 0, (sizeof(struct quest_data) *
                                     (rec_count + quest_chunk_size)));
    top_of_quest_array = rec_count + quest_chunk_size;
    break;
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s/%s", prefix, buf1);
    File in_file(buf2, "r");

    if (!in_file.IsOpen()) {
      sprintf(buf, "Unable to find file %s.", buf2);
      mudlog(buf, NULL, LOG_SYSLOG, TRUE);
    } else {
      switch (mode) {
      case DB_BOOT_VEH:
      case DB_BOOT_WLD:
      case DB_BOOT_OBJ:
      case DB_BOOT_MTX:
      case DB_BOOT_MOB:
      case DB_BOOT_SHP:
      case DB_BOOT_QST:
      case DB_BOOT_IC:
        discrete_load(in_file, mode);
        break;
      case DB_BOOT_ZON:
        load_zones(in_file);
        break;
      }
      in_file.Close();
    }
    fscanf(index, "%s\n", buf1);
  }
}

void discrete_load(File &fl, int mode)
{
  long nr = -1, last = 0;
  char line[256];
  bool is_new = (mode == DB_BOOT_WLD || mode == DB_BOOT_MTX ||
                 mode == DB_BOOT_OBJ || mode == DB_BOOT_IC ||
                 mode == DB_BOOT_MOB);

  for (;;) {
    fl.GetLine(line, 256);

    if (is_new? !str_cmp(line, "END") : *line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%ld", &nr) != 1) {
        log("Format error in %s, line %d",
            fl.Filename(), fl.LineNumber());
        shutdown();
      }
      if (nr >= 99999999)
        return;
      else
        switch (mode) {
        case DB_BOOT_WLD:
          parse_room(fl, nr);
          break;
        case DB_BOOT_MOB:
          parse_mobile(fl, nr);
          break;
        case DB_BOOT_OBJ:
          parse_object(fl, nr);
          break;
        case DB_BOOT_VEH:
          parse_veh(fl, nr);
          break;
        case DB_BOOT_QST:
          parse_quest(fl, nr);
          break;
        case DB_BOOT_SHP:
          parse_shop(fl, nr);
          break;
        case DB_BOOT_MTX:
          parse_host(fl, nr);
          break;
        case DB_BOOT_IC:
          parse_ic(fl, nr);
          break;
        }
    } else {
      log("Format error in %s, line %d",
          fl.Filename(), fl.LineNumber());
      shutdown();
    }
  }
}

long asciiflag_conv(char *flag)
{
  long flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return flags;
}

/* Ok around here somewhere,
 * we will need somebody to
 * code up the function to
 * load the vehicles to the
 * mud... void parse_room()
 * and stuff... I'm too new
 * at this to write anything
 * as complex as what is
 * written below for similar
 * purposes.  --Dunk */
void parse_veh(File &fl, long virtual_nr)
{
  static int veh_nr = 0, zone = 0;
  int t[6];
  char r[256];
  char line[256];

  veh_index[veh_nr].vnum = virtual_nr;
  veh_index[veh_nr].number = 0;
  veh_index[veh_nr].func = NULL;

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    fprintf(stderr, "Veh #%ld is below zone %d.\n", virtual_nr, zone_table[zone].number);
    shutdown();
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      fprintf(stderr, "Veh %ld is outside of any zone.\n", virtual_nr);
      shutdown();
    }

  veh_proto[veh_nr].veh_number = veh_nr;

  veh_proto[veh_nr].name = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].name)
    veh_proto[veh_nr].name = str_dup("An unnamed vehicle");

  veh_proto[veh_nr].short_description = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].short_description)
    veh_proto[veh_nr].short_description = str_dup("A rather nondescript vehicle.");

  veh_proto[veh_nr].description = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].description)
    veh_proto[veh_nr].description = str_dup("A rather nondescript vehicle.");

  veh_proto[veh_nr].long_description = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].long_description)
    veh_proto[veh_nr].long_description = str_dup("A rather nondescript vehicle.");

  veh_proto[veh_nr].inside_description = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].inside_description)
    veh_proto[veh_nr].inside_description = str_dup("A rather nondescript vehicle.");

  veh_proto[veh_nr].leave = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].leave)
    veh_proto[veh_nr].leave = str_dup("leaves");

  veh_proto[veh_nr].arrive = str_dup(fl.ReadString());
  if (!veh_proto[veh_nr].arrive)
    veh_proto[veh_nr].arrive = str_dup("arrives");

  // we check for the old version too
  if (!fl.GetLine(line, 256) || sscanf(line, "%d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4,
                                       t + 5) < 6) {
    fprintf(stderr, "Old format error in vehicle #%ld\n", virtual_nr);
    shutdown();
  }

  veh_proto[veh_nr].handling = t[0];
  veh_proto[veh_nr].speed = t[1];
  veh_proto[veh_nr].accel = t[2];
  veh_proto[veh_nr].body = t[3];
  veh_proto[veh_nr].armor = t[4];
  if (t[5] >= 0)
    veh_proto[veh_nr].pilot = t[5];
  else
    veh_proto[veh_nr].pilot = 0;
  if (!fl.GetLine(line, 256) || sscanf(line, "%d %d %d %d %d %d %s", t, t + 1, t + 2, t + 3, t + 4,
                                       t + 5, r) < 5) {
    fprintf(stderr, "Old format error in vehicle #%ld\n", virtual_nr);
    shutdown();
  }

  veh_proto[veh_nr].sig = t[0];
  veh_proto[veh_nr].autonav = t[1];
  veh_proto[veh_nr].seating = t[2];
  veh_proto[veh_nr].load = (float)t[3];
  veh_proto[veh_nr].cost = t[4];
  veh_proto[veh_nr].type = t[5];
  veh_proto[veh_nr].contents = NULL;
  veh_proto[veh_nr].people = NULL;
  veh_proto[veh_nr].damage = 0;
  top_of_veht = veh_nr++;
}

void parse_host(File &fl, long nr)
{
  static DBIndex::rnum_t rnum = 0, zone = 0;
  char field[64];
  if (nr <= (zone ? zone_table[zone - 1].top : -1)) {
    log("Host #%d is below zone %d.\n", nr, zone_table[zone].number);
    shutdown();
  }
  while (nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("Room %d is outside of any zone.\n", nr);
      shutdown();
    }
  host_data *host = matrix+rnum;

  VTable data;
  data.Parse(&fl);

  host->vnum = nr;
  host->rnum = rnum;
  host->parent = data.GetLong("Parent", 0);
  host->name = str_dup(data.GetString("Name", "An empty host"));
  host->desc = str_dup(data.GetString("Description", "This host is empty!\n"));
  host->keywords = str_dup(data.GetString("Keywords", "host"));
  host->shutdown_start = str_dup(data.GetString("ShutdownStart", "A deep echoing voice announces a host shutdown.\r\n"));
  host->shutdown_stop = str_dup(data.GetString("ShutdownStop", "A deep echoing voice announces the shutdown has been aborted.\r\n"));
  host->colour = data.GetInt("Colour", 0);
  host->security = data.GetInt("Security", 0);
  host->intrusion = data.GetInt("Difficulty", 0);
  host->stats[ACCESS][0] = data.GetLong("Access", 0);
  host->stats[ACCESS][2] = data.GetLong("AccessScramble", 0);
  host->stats[ACCESS][5] = data.GetLong("AccessTrapdoor", 0);
  if (host->stats[ACCESS][2])
    host->stats[ACCESS][1] = 1;
  host->stats[CONTROL][0] = data.GetLong("Control", 0);
  host->stats[CONTROL][5] = data.GetLong("ControlTrapdoor", 0);
  host->stats[INDEX][0] = data.GetLong("Index", 0);
  host->stats[INDEX][5] = data.GetLong("IndexTrapdoor", 0);
  host->stats[FILES][0] = data.GetLong("Files", 0);
  host->stats[FILES][2] = data.GetLong("FilesScramble", 0);
  host->stats[FILES][5] = data.GetLong("FilesTrapdoor", 0);
  if (host->stats[FILES][2])
    host->stats[FILES][1] = 1;
  host->stats[SLAVE][0] = data.GetLong("Slave", 0);
  host->stats[SLAVE][2] = data.GetLong("SlaveScramble", 0);
  host->stats[SLAVE][5] = data.GetLong("SlaveTrapdoor", 0);
  if (host->stats[SLAVE][2])
    host->stats[SLAVE][1] = 1;
  host->type = data.LookupInt("Type", host_type, 0);
  int num_fields = data.NumSubsections("EXITS");
  for (int x = 0; x < num_fields; x++) {
    const char *name = data.GetIndexSection("EXITS", x);
    exit_data *exit = new exit_data;
    sprintf(field, "%s/Exit", name);
    exit->host = data.GetLong(field, 0);
    sprintf(field, "%s/Number", name);
    exit->number = str_dup(data.GetString(field, "000"));
    if (host->exit)
      exit->next = host->exit;
    host->exit = exit;
  }
  num_fields = data.NumSubsections("TRIGGERS");
  for (int x = 0; x < num_fields; x++) {
    const char *name = data.GetIndexSection("TRIGGERS", x);
    trigger_step *trigger = new trigger_step;
    sprintf(field, "%s/Step", name);
    trigger->step = data.GetInt(field, 0);
    sprintf(field, "%s/Alert", name);
    trigger->alert = data.LookupInt(field, alerts, 0);
    sprintf(field, "%s/IC", name);
    trigger->ic = data.GetLong(field, 0);
    if (host->trigger) {
      struct trigger_step *last = NULL, *temp;
      for (temp = host->trigger; temp && trigger->step < temp->step; temp = temp->next)
        last = temp;
      if (temp) {
        trigger->next = temp->next;
        temp->next = trigger;
      } else
        last->next = trigger;
    } else
      host->trigger = trigger;
  }
  if (!host->type)
    switch (host->colour) {
    case 0:
      host->found = number(1, 6) - 1;
      break;
    case 1:
      host->found = number(1, 6) + number(1, 6) - 2;
      break;
    case 2:
      host->found = number(1, 6) + number(1, 6);
      break;
    case 3:
    case 4:
      host->found = number(1, 6) + number(1, 6) + 2;
      break;
    }
  top_of_matrix = rnum++;
}
void parse_ic(File &fl, long nr)
{
  static DBIndex::rnum_t rnum = 0, zone = 0;
  ic_index[rnum].vnum = nr;
  ic_index[rnum].number = 0;
  ic_index[rnum].func = NULL;

  matrix_icon *ic = ic_proto+rnum;
  clear_icon(ic);
  ic->number = rnum;

  if (nr <= (zone ? zone_table[zone - 1].top : -1)) {
    log("IC #%d is below zone %d.\n", nr, zone);
    shutdown();
  }
  while (nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("IC%d is outside of any zone.\n", nr);
      shutdown();
    }

  VTable data;
  data.Parse(&fl);

  ic->name = str_dup(data.GetString("Name", "An unfinished IC"));
  ic->look_desc = str_dup(data.GetString("LongDesc", "An unfinished IC guards the node."));
  ic->long_desc = str_dup(data.GetString("Description", "This IC is unfinished.\r\n"));
  ic->ic.rating = data.GetInt("Rating", 0);
  ic->ic.type = data.LookupInt("Type", ic_type, 0);
  ic->ic.subtype = data.GetInt("SubType", 0);
  ic->ic.trap = data.GetLong("Trap", 0);
  ic->ic.expert = data.GetInt("Expert", 0);
  ic->ic.options.FromString(data.GetString("Flags", "0"));
  top_of_ic = rnum++;
}
/* load the rooms */
void parse_room(File &fl, long nr)
{
  static DBIndex::rnum_t rnum = 0, zone = 0;

  if (nr <= (zone ? zone_table[zone - 1].top : -1)) {
    log("Room #%d is below zone %d.\n", nr, zone_table[zone].number);
    shutdown();
  }
  while (nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("Room %d is outside of any zone.\n", nr);
      shutdown();
    }

  room_data *room = world+rnum;

  VTable data;
  data.Parse(&fl);

  room->zone = zone;
  room->number = nr;

  room->name = str_dup(data.GetString("Name", "An empty room"));
  room->description =
    str_dup(data.GetString("Desc", "This room is empty!  Boo!\n"));

  room->room_flags.FromString(data.GetString("Flags", "0"));
  if (room->room_flags.IsSet(ROOM_PEACEFUL))
    room->peaceful = 1;
  room->sector_type = data.LookupInt("SecType", sector_types, SECT_INSIDE);
  room->matrix = data.GetLong("MatrixExit", 0);
  room->io = data.GetInt("IO", 0);
  room->bandwidth = data.GetInt("Bandwidth", 0);
  room->access = data.GetInt("Access", 0);
  room->trace = data.GetInt("Trace", 0);
  room->rtg = data.GetLong("RTG", 1100);
  room->jacknumber = data.GetLong("JackID", 0);
  room->address = str_dup(data.GetString("Address", "An undisclosed location"));
  room->spec = data.GetInt("POINTS/SpecIdx", 0);
  room->rating = data.GetInt("POINTS/Rating", 0);

  // read in directions
  int i;
  for (i = 0; *fulldirs[i] != '\n'; i++) {
    char sect[16];
    sprintf(sect, "EXIT %s", fulldirs[i]);

    room->dir_option[i] = NULL;

    if (data.DoesSectionExist(sect)) {
      char field[64];

      sprintf(field, "%s/ToVnum", sect);
      int to_vnum = data.GetInt(field, -1);

      if (to_vnum < 0) {
        log("Room #%d's %s exit had invalid destination -- skipping",
            nr, fulldirs[i]);
        continue;
      }

      room->dir_option[i] = new room_direction_data;
      room_direction_data *dir = room->dir_option[i];

      dir->to_room = to_vnum;
      // dir->to_room_vnum will be set in renum_world

      sprintf(field, "%s/Keywords", sect);
      dir->keyword = str_dup(data.GetString(field, NULL));

      sprintf(field, "%s/Desc", sect);
      dir->general_description = str_dup(data.GetString(field, NULL));

      sprintf(field, "%s/Flags", sect);
      int flags = data.GetInt(field, 0);

      if (flags == 1)
        dir->exit_info = EX_ISDOOR;
      else if (flags == 2)
        dir->exit_info = EX_ISDOOR | EX_PICKPROOF;
      else
        dir->exit_info = 0;

      sprintf(field, "%s/Material", sect);
      dir->material = data.LookupInt(field, material_names, 5);

      sprintf(field, "%s/Barrier", sect);
      dir->barrier = data.GetInt(field, 4);
      dir->condition = dir->barrier;

      sprintf(field, "%s/KeyVnum", sect);
      dir->key = data.GetInt(field, -1);

      sprintf(field, "%s/LockRating", sect);
      dir->key_level = data.GetInt(field, 0);

      sprintf(field, "%s/HiddenRating", sect);
      dir->hidden = data.GetInt(field, 0);
    }
  }

  room->ex_description = NULL;

  // finally, read in extra descriptions
  for (i = 0; true; i++) {
    char sect[16];
    sprintf(sect, "EXTRADESC %d", i);

    if (data.NumFields(sect) > 0) {
      char field[64];

      sprintf(field, "%s/Keywords", sect);
      char *keywords = str_dup(data.GetString(field, NULL));

      if (!*keywords) {
        log("Room #%d's extra description #%d had no keywords -- skipping",
            nr, i);
        continue;
      }


      extra_descr_data *desc = new extra_descr_data;
      desc->keyword = str_dup(keywords);
      delete [] keywords;
      sprintf(field, "%s/Desc", sect);
      desc->description = str_dup(data.GetString(field, NULL));
      desc->next = room->ex_description;
      room->ex_description = desc;
    } else
      break;
  }

  top_of_world = rnum++;
}

/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int t[7];
  char line[256];
  int retval;

  sprintf(buf2, "room #%ld, direction D%d", world[room].number, dir);

  world[room].dir_option[dir] = new room_direction_data;
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    shutdown();
  }
  if ((retval = sscanf(line, " %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3,
                       t + 4, t + 5, t + 6)) < 4) {
    fprintf(stderr, "Format error, %s\n", buf2);
    shutdown();
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = t[1];
  world[room].dir_option[dir]->to_room = MAX(0, t[2]);
  world[room].dir_option[dir]->key_level = t[3];

  world[room].dir_option[dir]->material = (retval > 4) ? t[4] : 5;
  world[room].dir_option[dir]->barrier = (retval > 5) ? t[5] : 4;
  world[room].dir_option[dir]->condition = (retval > 5) ? t[5] : 4;
  world[room].dir_option[dir]->hidden = (retval > 6) ? t[6] : 0;
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  extern vnum_t mortal_start_room;
  extern vnum_t immort_start_room;
  extern vnum_t frozen_start_room;
  extern vnum_t newbie_start_room;

  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    shutdown();
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
  if ((r_newbie_start_room = real_room(newbie_start_room)) < 0) {
    log("SYSERR:  Warning: Newbie start room does not exist.  Change in config.c.");
    r_newbie_start_room = r_mortal_start_room;
  }
}

/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  register int room, door;

  /* before renumbering the exits, copy them to to_room_vnum */
  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door]) {
        room_direction_data *dir = world[room].dir_option[door];
        dir->to_room_vnum = dir->to_room;
        dir->to_room = real_room(dir->to_room_vnum);
      }
}

#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int zone, cmd_no, a, b;

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; cmd_no < zone_table[zone].num_cmds; cmd_no++) {
      a = b = 0;
      switch (ZCMD.command) {
      case 'M':
        a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
        b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'H':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        if (ZCMD.arg3 != NOWHERE)
          b = ZCMD.arg3 = real_host(ZCMD.arg3);
        break;
      case 'O':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        if (ZCMD.arg3 != NOWHERE)
          b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'V': /* Vehicles */
        a = ZCMD.arg1 = real_vehicle(ZCMD.arg1);
        b = ZCMD.arg3 = real_room(ZCMD.arg3);
        break;
      case 'S':
        a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
        break;
      case 'G':
      case 'C':
      case 'U':
      case 'I':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      case 'E':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      case 'P':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        b = ZCMD.arg3 = real_object(ZCMD.arg3);
        break;
      case 'D':
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
        b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      case 'N':
        a = ZCMD.arg1 = real_object(ZCMD.arg1);
        break;
      }
      if (a < 0 || b < 0) {
        log_zone_error(zone, cmd_no, "Invalid vnum, cmd disabled");
        ZCMD.command = '*';
      }
    }
}

void parse_mobile(File &in, long nr)
{
  static DBIndex::rnum_t rnum = 0;

  char_data *mob = mob_proto+rnum;

  MOB_VNUM_RNUM(rnum) = nr;
  mob_index[rnum].number = 0;
  mob_index[rnum].func = NULL;

  clear_char(mob);

  mob->player_specials = &dummy_mob;

  // heheh....sweeeeet
  VTable data;
  data.Parse(&in);

  mob->player.physical_text.keywords =
    str_dup(data.GetString("Keywords", "mob unnamed"));
  mob->player.physical_text.name =
    str_dup(data.GetString("Name", "an unnamed mob"));
  mob->player.physical_text.room_desc =
    str_dup(data.GetString("RoomDesc", "An unfinished mob stands here.\n"));
  mob->player.physical_text.look_desc =
    str_dup(data.GetString("LookDesc", "He looks pretty unfinished.\n"));

  mob->mob_specials.arrive =
    str_dup(data.GetString("ArriveMsg", "arrives from"));
  mob->mob_specials.leave =
    str_dup(data.GetString("LeaveMsg", "leaves"));
  GET_TITLE(mob) = NULL;

  MOB_FLAGS(mob).FromString(data.GetString("MobFlags", "0"));
  AFF_FLAGS(mob).FromString(data.GetString("AffFlags", "0"));

  GET_RACE(mob) = data.LookupInt("Race", npc_classes, CLASS_OTHER);
  GET_SEX(mob) = data.LookupInt("Gender", genders, SEX_NEUTRAL);

  GET_POS(mob) = data.LookupInt("Position", position_types, POS_STANDING);
  GET_DEFAULT_POS(mob) =
    data.LookupInt("DefaultPos", position_types, POS_STANDING);
  mob->mob_specials.attack_type = TYPE_HIT +
                                  data.LookupInt("AttackType", attack_types, 0);

  GET_REAL_BOD(mob) = data.GetInt("ATTRIBUTES/Bod", 1);
  GET_REAL_QUI(mob) = data.GetInt("ATTRIBUTES/Qui", 1);
  GET_REAL_STR(mob) = data.GetInt("ATTRIBUTES/Str", 1);
  GET_REAL_CHA(mob) = data.GetInt("ATTRIBUTES/Cha", 1);
  GET_REAL_INT(mob) = data.GetInt("ATTRIBUTES/Int", 1);
  GET_REAL_WIL(mob) = data.GetInt("ATTRIBUTES/Wil", 1);
  GET_REAL_REA(mob) = (GET_REAL_QUI(mob) + GET_REAL_INT(mob)) / 2;


  mob->real_abils.mag = data.GetInt("ATTRIBUTES/Mag", 0) * 100;
  mob->real_abils.rmag = mob->real_abils.mag;
  mob->real_abils.ess = 600;
  mob->real_abils.bod_index =GET_REAL_BOD(mob) * 100;

  mob->aff_abils = mob->real_abils;

  GET_HEIGHT(mob) = data.GetInt("POINTS/Height", 100);
  GET_WEIGHT(mob) = data.GetInt("POINTS/Weight", 5);
  GET_LEVEL(mob) = data.GetInt("POINTS/Level", 0);
  GET_MAX_PHYSICAL(mob) = data.GetInt("POINTS/MaxPhys", 10*100);
  GET_MAX_MENTAL(mob) = data.GetInt("POINTS/MaxMent", 10*100);
  GET_BALLISTIC(mob) = data.GetInt("POINTS/Ballistic", 0);
  GET_IMPACT(mob) = data.GetInt("POINTS/Impact", 0);
  GET_NUYEN(mob) = data.GetInt("POINTS/Cash", 0);
  GET_BANK(mob) = data.GetInt("POINTS/Bank", 0);
  GET_KARMA(mob) = data.GetInt("POINTS/Karma", 0);

  GET_PHYSICAL(mob) = GET_MAX_PHYSICAL(mob);
  GET_MENTAL(mob) = GET_MAX_MENTAL(mob);
  GET_SUSTAINED(mob) = 0;
  GET_GRADE(mob) = 0;

  /* set pools to 0 initially, affect total will correct them */
  mob->real_abils.astral_pool = 0;
  mob->real_abils.defense_pool = 0;
  mob->real_abils.combat_pool = 0;
  mob->real_abils.offense_pool = 0;
  mob->real_abils.hacking_pool = 0;
  mob->real_abils.magic_pool = 0;

  int j;
  for (j = 0; j < 10; j++)
    mob->mob_specials.mob_skills[j] = 0;

  int num_skills = data.NumFields("SKILLS");
  for (j = 0; j < num_skills; j++) {
    const char *skill_name = data.GetIndexField("SKILLS", j);
    int idx;

    for (idx = 0; idx <= MAX_SKILLS; idx++)
      if (!str_cmp(skills[idx].name, skill_name))
        break;
    if (idx > 0 || idx <= MAX_SKILLS) {
      GET_SKILL(mob, idx) = data.GetIndexInt("SKILLS", j, 0);
      mob->mob_specials.mob_skills[j * 2] = idx;
      mob->mob_specials.mob_skills[j * 2 + 1] = data.GetIndexInt("SKILLS", j, 0);
    }
  }

  for (j = 0; j < 3; j++)
    GET_COND(mob, j) = -1;

  for (j = 0; j < NUM_WEARS; j++)
    mob->equipment[j] = NULL;

  mob->cyberware = NULL;
  mob->bioware = NULL;
  mob->nr = rnum;
  mob->desc = NULL;

  if ( 1 ) {
    extern int calc_karma(struct char_data *ch, struct char_data *vict);

    int old = GET_KARMA(mob);

    GET_KARMA(mob) = 0; // calc_karma prolly relies on this

    GET_KARMA(mob) = MIN(old, calc_karma(NULL, mob));
  }

  top_of_mobt = rnum++;
}

void parse_object(File &fl, long nr)
{
  static DBIndex::rnum_t rnum = 0;

  OBJ_VNUM_RNUM(rnum) = nr;
  obj_index[rnum].number = 0;
  obj_index[rnum].func = NULL;

  clear_object(obj_proto + rnum);

  obj_data *obj = obj_proto+rnum;

  obj->in_room = NOWHERE;
  obj->item_number = rnum;

  VTable data;
  data.Parse(&fl);

  obj->text.keywords = str_dup(data.GetString("Keywords", "item unnamed"));
  obj->text.name = str_dup(data.GetString("Name", "an unnamed item"));
  obj->text.room_desc =
    str_dup(data.GetString("RoomDesc", "An unfinished item is here.\n"));
  obj->text.look_desc =
    str_dup(data.GetString("LookDesc", "It looks pretty unfinished.\n"));

  GET_OBJ_TYPE(obj) = data.LookupInt("Type", item_types, ITEM_OTHER);

  GET_OBJ_WEAR(obj).FromString(data.GetString("WearFlags", "0"));
  GET_OBJ_EXTRA(obj).FromString(data.GetString("ExtraFlags", "0"));
  GET_OBJ_AFFECT(obj).FromString(data.GetString("AffFlags", "0"));
  GET_OBJ_AVAILTN(obj) = data.GetInt("POINTS/AvailTN", 0);
  GET_OBJ_AVAILDAY(obj) = data.GetFloat("POINTS/AvailDay", 0);

  obj->obj_flags.material = data.LookupInt("Material", material_names, 5);

  GET_OBJ_WEIGHT(obj) = data.GetFloat("POINTS/Weight", 5);
  GET_OBJ_BARRIER(obj) = data.GetInt("POINTS/Barrier", 3);
  GET_OBJ_CONDITION(obj) = GET_OBJ_BARRIER(obj);

  GET_OBJ_COST(obj) = data.GetInt("POINTS/Cost", 0);

  obj->obj_flags.quest_id = 0;

  int i;
  for (i = 0; i < 10; i++) {
    char field[32];
    sprintf(field, "VALUES/Val%d", i);

    GET_OBJ_VAL(obj, i) = data.GetInt(field, 0);
  }

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
    if (GET_OBJ_VAL(obj, 3) < TYPE_HIT)
      GET_OBJ_VAL(obj, 3) += TYPE_HIT;
    if (GET_OBJ_VAL(obj, 4) > 100)
      GET_OBJ_VAL(obj, 4) -= 100;
  }
  // read in affects
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    char sect[16];
    sprintf(sect, "AFFECT %d", i);

    obj->affected[i].location = APPLY_NONE;
    obj->affected[i].modifier = 0;

    if (data.NumFields(sect) > 1) {
      char field[64];
      sprintf(field, "%s/Location", sect);

      int loc = data.LookupInt(field, apply_types, APPLY_NONE);

      if (loc == APPLY_NONE) {
        log("Item #%d's affect #%d had no location -- skipping", nr, i);
        continue;
      }

      obj->affected[i].location = loc;

      sprintf(field, "%s/Modifier", sect);
      obj->affected[i].modifier = data.GetInt(field, 0);
    }
  }

  // finally, read in extra descriptions
  for (i = 0; true; i++) {
    char sect[16];
    sprintf(sect, "EXTRADESC %d", i);

    if (data.NumFields(sect) > 0) {
      char field[64];

      sprintf(field, "%s/Keywords", sect);
      char *keywords = str_dup(data.GetString(field, NULL));

      if (!keywords) {
        log("Room #%d's extra description #%d had no keywords -- skipping",
            nr, i);
        continue;
      }

      extra_descr_data *desc = new extra_descr_data;
      desc->keyword = str_dup(keywords);
      delete [] keywords;
      sprintf(field, "%s/Desc", sect);
      desc->description = str_dup(data.GetString(field, NULL));

      desc->next = obj->ex_description;
      obj->ex_description = desc;
    } else
      break;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_PROGRAM) {
    int mult;
    if (GET_OBJ_VAL(obj, 0) == SOFT_ATTACK)
      mult = attack_multiplier[GET_OBJ_VAL(obj, 3) - 1];
    else
      mult = program_multiplier[GET_OBJ_VAL(obj, 0)];
    GET_OBJ_VAL(obj, 2) = (GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 1)) * mult;
    if (GET_OBJ_VAL(obj, 1) < 4)
      GET_OBJ_COST(obj) = GET_OBJ_VAL(obj, 2) * 100;
    else if (GET_OBJ_VAL(obj, 1) < 7)
      GET_OBJ_COST(obj) = GET_OBJ_VAL(obj, 2) * 200;
    else if (GET_OBJ_VAL(obj, 1) < 10)
      GET_OBJ_COST(obj) = GET_OBJ_VAL(obj, 2) * 500;
    else
      GET_OBJ_COST(obj) = GET_OBJ_VAL(obj, 2) * 1000;
  }
  top_of_objt = rnum++;
}

void parse_quest(File &fl, long virtual_nr)
{
  static long quest_nr = 0;
  long j, t[12];
  char line[256];

  quest_table[quest_nr].vnum = virtual_nr;

  fl.GetLine(line, 256);
  if (sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
             t, t + 1, t + 2, t + 3, t+4, t+5,
             t + 6, t + 7, t + 8, t + 9, t + 10, t + 11) != 12) {
    fprintf(stderr, "Format error in quest #%ld, expecting # # # # # # # # #\n",
            quest_nr);
    shutdown();
  }
  quest_table[quest_nr].johnson = t[0];
  quest_table[quest_nr].time = t[1];
  quest_table[quest_nr].min_rep = t[2];
  quest_table[quest_nr].max_rep = t[3];
  quest_table[quest_nr].nuyen = t[4];
  quest_table[quest_nr].karma = t[5];
  quest_table[quest_nr].reward = (real_object(t[6]) > -1 ? t[6] : -1);
  quest_table[quest_nr].num_objs = t[7];
  quest_table[quest_nr].num_mobs = t[8];
  quest_table[quest_nr].s_time = t[9];
  quest_table[quest_nr].e_time = t[10];
  quest_table[quest_nr].s_room = t[11];

  if (quest_table[quest_nr].num_objs > 0) {
    quest_table[quest_nr].obj = new quest_om_data[quest_table[quest_nr].num_objs];
    for (j = 0; j < quest_table[quest_nr].num_objs; j++) {
      fl.GetLine(line, 256);
      if (sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3,
                 t + 4, t + 5, t + 6, t + 7) != 8) {
        fprintf(stderr, "Format error in quest #%ld, obj #%ld\n", quest_nr, j);
        shutdown();
      }
      quest_table[quest_nr].obj[j].vnum = t[0];
      quest_table[quest_nr].obj[j].nuyen = t[1];
      quest_table[quest_nr].obj[j].karma = t[2];
      quest_table[quest_nr].obj[j].load = (byte) t[3];
      quest_table[quest_nr].obj[j].objective = (byte) t[4];
      quest_table[quest_nr].obj[j].l_data = t[5];
      quest_table[quest_nr].obj[j].l_data2 = t[6];
      quest_table[quest_nr].obj[j].o_data = t[7];
    }
  } else
    quest_table[quest_nr].obj = NULL;

  if (quest_table[quest_nr].num_mobs > 0) {
    quest_table[quest_nr].mob = new quest_om_data[quest_table[quest_nr].num_mobs];
    for (j = 0; j < quest_table[quest_nr].num_mobs; j++) {
      fl.GetLine(line, 256);
      if (sscanf(line, "%ld %ld %ld %ld %ld %ld %ld %ld", t, t + 1, t + 2, t + 3,
                 t + 4, t + 5, t + 6, t + 7) != 8) {
        fprintf(stderr, "Format error in quest #%ld, mob #%ld\n", quest_nr, j);
        shutdown();
      }
      quest_table[quest_nr].mob[j].vnum = t[0];
      quest_table[quest_nr].mob[j].nuyen = t[1];
      quest_table[quest_nr].mob[j].karma = t[2];
      quest_table[quest_nr].mob[j].load = (byte) t[3];
      quest_table[quest_nr].mob[j].objective = (byte) t[4];
      quest_table[quest_nr].mob[j].l_data = t[5];
      quest_table[quest_nr].mob[j].l_data2 = t[6];
      quest_table[quest_nr].mob[j].o_data = t[7];
    }
  } else
    quest_table[quest_nr].mob = NULL;

  quest_table[quest_nr].intro = fl.ReadString();
  quest_table[quest_nr].decline = fl.ReadString();
  quest_table[quest_nr].quit = fl.ReadString();
  quest_table[quest_nr].finish = fl.ReadString();
  quest_table[quest_nr].info = fl.ReadString();
  quest_table[quest_nr].s_string = fl.ReadString();
  quest_table[quest_nr].e_string = fl.ReadString();
  quest_table[quest_nr].done = fl.ReadString();

  quest_nr++;
  top_of_questt = quest_nr;
}

void parse_shop(File &fl, long virtual_nr)
{
  static long shop_nr = 0;
  char line[256];
  long t[5], i, max;
  float buy, sell;

  shop_table[shop_nr].vnum = virtual_nr;

  fl.GetLine(line, 256);
  if (sscanf(line, "%ld %ld %ld", t, t + 1, t + 2) != 3) {
    fprintf(stderr, "Format error in shop #%ld, expecting # # #\r\n", shop_nr);
    shutdown();
  }

  shop_table[shop_nr].num_producing = t[0];
  shop_table[shop_nr].num_buy_types = t[1];
  shop_table[shop_nr].num_rooms = t[2];

  max = MAX(t[0], MAX(t[1], t[2]));

  if (shop_table[shop_nr].num_producing > 0)
    shop_table[shop_nr].producing = new long[shop_table[shop_nr].num_producing];
  else
    shop_table[shop_nr].producing = NULL;

  if (shop_table[shop_nr].num_buy_types > 0)
    shop_table[shop_nr].type = new int[shop_table[shop_nr].num_buy_types];
  else
    shop_table[shop_nr].type = NULL;

  if (shop_table[shop_nr].num_rooms > 0)
    shop_table[shop_nr].in_room = new long[shop_table[shop_nr].num_rooms];
  else
    shop_table[shop_nr].in_room = NULL;

  for (i = 0; i < max; i++) {
    fl.GetLine(line, 256);
    if (sscanf(line, "%ld %ld %ld", t, t + 1, t + 2) != 3) {
      fprintf(stderr, "Format error in shop #%ld, p/t/r line #%ld", shop_nr, i);
      shutdown();
    }
    if (t[0] > 0 && i < shop_table[shop_nr].num_producing)
      shop_table[shop_nr].producing[i] = t[0];
    if (t[1] > 0 && i < shop_table[shop_nr].num_buy_types)
      shop_table[shop_nr].type[i] = t[1];
    if (t[2] > 0 && i < shop_table[shop_nr].num_rooms)
      shop_table[shop_nr].in_room[i] = t[2];
  }

  fl.GetLine(line, 256);
  if (sscanf(line, "%f %f %ld %ld %ld %ld %ld", &buy, &sell, t, t + 1, t + 2,
             t + 3, t + 4) != 7) {
    fprintf(stderr, "Format error in shop #%ld, expecting # # # # # # # Line#%ld\r\n", virtual_nr, i);
    shutdown();
  }

  shop_table[shop_nr].profit_buy = buy;
  shop_table[shop_nr].profit_sell = sell;
  shop_table[shop_nr].percentage = t[0],
                                   shop_table[shop_nr].temper = t[1];
  shop_table[shop_nr].bitvector = t[2];
  shop_table[shop_nr].keeper = t[3];
  shop_table[shop_nr].with_who = t[4];

  fl.GetLine(line, 256);
  if (sscanf(line, "%ld %ld %ld %ld", t, t + 1, t + 2, t + 3) != 4) {
    fprintf(stderr, "Format error in shop #%ld, expecting # # # #", shop_nr);
    shutdown();
  }

  shop_table[shop_nr].open1 = t[0];
  shop_table[shop_nr].close1 = t[1];
  shop_table[shop_nr].open2 = t[2];
  shop_table[shop_nr].close2 = t[3];

  shop_table[shop_nr].no_such_item1 = fl.ReadString();
  shop_table[shop_nr].no_such_item2 = fl.ReadString();
  shop_table[shop_nr].do_not_buy = fl.ReadString();
  shop_table[shop_nr].missing_cash1 = fl.ReadString();
  shop_table[shop_nr].missing_cash2 = fl.ReadString();
  shop_table[shop_nr].message_buy = fl.ReadString();
  shop_table[shop_nr].message_sell = fl.ReadString();

  SHOP_BANK(shop_nr) = 0;
  SHOP_SORT(shop_nr) = 0;
  SHOP_LASTTIME(shop_nr) = 0;

  shop_nr++;
  top_of_shopt = shop_nr;
}

#define Z       zone_table[zone]

/* load the zone table and command tables */
void load_zones(File &fl)
{
  static int zone = 0;
  int cmd_no = 0, tmp, error;
  char *ptr, buf[256];

  Z.num_cmds = 0;
  while (fl.GetLine(buf, 256) && *buf != '$')
    Z.num_cmds++;

  // subtract the first 4 lines
  Z.num_cmds -= 4;

  fl.Rewind();

  if (Z.num_cmds == 0) {
    fprintf(stderr, "%s is empty.\n", fl.Filename());
    Z.cmd = NULL;
    // exit(0);   -- it is okay, we can deal with empty zones
  } else
    Z.cmd = new struct reset_com[Z.num_cmds];

  fl.GetLine(buf, 256);

  if (sscanf(buf, "#%d", &Z.number) != 1) {
    fprintf(stderr, "Format error in %s, line %d\n",
            fl.Filename(), fl.LineNumber());
    shutdown();
  }
  //sprintf(buf2, "beginning of zone #%d", Z.number);

  fl.GetLine(buf, 256);
  if ((ptr = strchr((const char *)buf, '~')) != NULL)   /* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = str_dup(buf);

  fl.GetLine(buf, 256);
  if (sscanf(buf, " %d %d %d %d %d %d",
             &Z.top, &Z.lifespan, &Z.reset_mode,
             &Z.security, &Z.connected, &Z.juridiction) < 5) {
    fprintf(stderr, "Format error in 5-constant line of %s", fl.Filename());
    shutdown();
  }

  fl.GetLine(buf, 256);
  // This next section reads in the id nums of the players that can edit
  // this zone.
  if (sscanf(buf, "%d %d %d %d %d", &Z.editor_ids[0], &Z.editor_ids[1],
             &Z.editor_ids[2], &Z.editor_ids[3], &Z.editor_ids[4]) != 5) {
    fprintf(stderr, "Format error in editor id list of %s", fl.Filename());
    shutdown();
  }

  cmd_no = 0;

  while (cmd_no < Z.num_cmds) {
    if (!fl.GetLine(buf, 256)) {
      fprintf(stderr, "Format error in %s - premature end of file\n",
              fl.Filename());
      exit(1);
    }

    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    error = 0;
    if (strchr((const char *)"MVOENPDHL", ZCMD.command) == NULL) { /* a 3-arg command */
      if (sscanf(ptr, " %d %ld %ld ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
        error = 1;
    } else {
      if (sscanf(ptr, " %d %ld %ld %ld ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
                 &ZCMD.arg3) != 4)
        error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error) {
      fprintf(stderr, "Format error in %s, line %d: '%s'\n",
              fl.Filename(), fl.LineNumber(), buf);
      exit(1);
    }
    if (ZCMD.command == 'E') {
      switch (ZCMD.arg3) {
      case OLD_WEAR_LIGHT:
        ZCMD.arg3 = WEAR_LIGHT;
        break;
      case OLD_WEAR_FINGER_R:
        ZCMD.arg3 = WEAR_FINGER_R;
        break;
      case OLD_WEAR_FINGER_L:
        ZCMD.arg3 = WEAR_FINGER_L;
        break;
      case OLD_WEAR_NECK_1:
        ZCMD.arg3 = WEAR_NECK_1;
        break;
      case OLD_WEAR_NECK_2:
        ZCMD.arg3 = WEAR_NECK_2;
        break;
      case OLD_WEAR_BODY:
        ZCMD.arg3 = WEAR_BODY;
        break;
      case OLD_WEAR_HEAD:
        ZCMD.arg3 = WEAR_HEAD;
        break;
      case OLD_WEAR_LEGS:
        ZCMD.arg3 = WEAR_LEGS;
        break;
      case OLD_WEAR_FEET:
        ZCMD.arg3 = WEAR_FEET;
        break;
      case OLD_WEAR_HANDS:
        ZCMD.arg3 = WEAR_HANDS;
        break;
      case OLD_WEAR_ARMS:
        ZCMD.arg3 = WEAR_ARMS;
        break;
      case OLD_WEAR_SHIELD:
        ZCMD.arg3 = WEAR_SHIELD;
        break;
      case OLD_WEAR_ABOUT:
        ZCMD.arg3 = WEAR_ABOUT;
        break;
      case OLD_WEAR_WAIST:
        ZCMD.arg3 = WEAR_WAIST;
        break;
      case OLD_WEAR_WRIST_R:
        ZCMD.arg3 = WEAR_WRIST_R;
        break;
      case OLD_WEAR_WRIST_L:
        ZCMD.arg3 = WEAR_WRIST_L;
        break;
      case OLD_WEAR_WIELD:
        ZCMD.arg3 = WEAR_WIELD;
        break;
      case OLD_WEAR_HOLD:
        ZCMD.arg3 = WEAR_HOLD;
        break;
      case OLD_WEAR_EYES:
        ZCMD.arg3 = WEAR_EYES;
        break;
      case OLD_WEAR_EAR:
        ZCMD.arg3 = WEAR_EAR;
        break;
      case OLD_WEAR_EAR2:
        ZCMD.arg3 = WEAR_EAR2;
        break;
      case OLD_WEAR_UNDER:
        ZCMD.arg3 = WEAR_UNDER;
        break;
      case OLD_WEAR_BACK:
        ZCMD.arg3 = WEAR_BACK;
        break;
      case OLD_WEAR_LANKLE:
        ZCMD.arg3 = WEAR_LANKLE;
        break;
      case OLD_WEAR_RANKLE:
        ZCMD.arg3 = WEAR_RANKLE;
        break;
      case OLD_WEAR_SOCK:
        ZCMD.arg3 = WEAR_SOCK;
        break;
      case OLD_WEAR_FINGER3:
        ZCMD.arg3 = WEAR_FINGER3;
        break;
      case OLD_WEAR_FINGER4:
        ZCMD.arg3 = WEAR_FINGER4;
        break;
      case OLD_WEAR_FINGER5:
        ZCMD.arg3 = WEAR_FINGER5;
        break;
      case OLD_WEAR_FINGER6:
        ZCMD.arg3 = WEAR_FINGER6;
        break;
      case OLD_WEAR_FINGER7:
        ZCMD.arg3 = WEAR_FINGER7;
        break;
      case OLD_WEAR_FINGER8:
        ZCMD.arg3 = WEAR_FINGER8;
        break;
      case OLD_WEAR_BELLY:
        ZCMD.arg3 = WEAR_BELLY;
        break;
      case OLD_WEAR_LARM:
        ZCMD.arg3 = WEAR_LARM;
        break;
      case OLD_WEAR_RARM:
        ZCMD.arg3 = WEAR_RARM;
        break;
      case OLD_WEAR_PATCH:
        ZCMD.arg3 = WEAR_PATCH;
        break;


      }
    }
    if (ZCMD.command == 'L')
      ZCMD.command = 'E';
    ZCMD.line = fl.LineNumber();
    cmd_no++;
  }

  top_of_zone_table = zone++;
}

#undef Z

/*************************************************************************
*  procedures for resetting, both play-time and boot-time                *
*********************************************************************** */

int vnum_mobile_karma(char *searchname, struct char_data * ch)
{
  SPECIAL(shop_keeper);
  extern int calc_karma(struct char_data *ch, struct char_data *vict);
  int nr, found = 0;
  int karma;
  int i;
  int highest_karma;

  for (i = 10000; i > 0; i = highest_karma)
  {
    highest_karma = 0;
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].func == shop_keeper
          ||mob_index[nr].sfunc == shop_keeper)
        continue;
      karma = calc_karma(NULL, &mob_proto[nr]);
      if ( karma < i ) {
        if ( karma > highest_karma )
          highest_karma = karma;
        continue;
      }
      if ( i != 10000 && karma > i )
        continue;
      if (from_ip_zone(MOB_VNUM_RNUM(nr)))
        continue;
      ++found;
      sprintf(buf, "[%5ld] %4.2f (%4.2f) %4dx %6.2f %s\r\n",
              MOB_VNUM_RNUM(nr),
              (float)karma/100.0,
              (float)(GET_KARMA(&mob_proto[nr]))/100.0,
              mob_proto[nr].mob_specials.count_death,
              (float)(mob_proto[nr].mob_specials.value_death_karma)/100.0,
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_mobile_bonuskarma(char *searchname, struct char_data * ch)
{
  SPECIAL(shop_keeper);
  extern int calc_karma(struct char_data *ch, struct char_data *vict);
  int nr, found = 0;
  int karma;
  int i;
  int highest_karma;

  for (i = 10000; i > 0; i = highest_karma )
  {
    highest_karma = 0;
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].func == shop_keeper
          ||mob_index[nr].sfunc == shop_keeper)
        continue;
      if ( GET_KARMA(&mob_proto[nr]) < i ) {
        if ( GET_KARMA(&mob_proto[nr]) > highest_karma )
          highest_karma = GET_KARMA(&mob_proto[nr]);
        continue;
      }
      if ( i != 10000 && GET_KARMA(&mob_proto[nr]) > i )
        continue;
      if (from_ip_zone(MOB_VNUM_RNUM(nr)))
        continue;
      karma = calc_karma(NULL, &mob_proto[nr]);
      ++found;
      sprintf(buf, "[%5ld] %4.2f (%4.2f)%4dx %6.2f %s\r\n",
              MOB_VNUM_RNUM(nr),
              (float)karma/100.0,
              (float)(GET_KARMA(&mob_proto[nr]))/100.0,
              mob_proto[nr].mob_specials.count_death,
              (float)(mob_proto[nr].mob_specials.value_death_karma)/100.0,
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_mobile_nuyen(char *searchname, struct char_data * ch)
{
  SPECIAL(shop_keeper);
  extern int calc_karma(struct char_data *ch, struct char_data *vict);
  int nr, found = 0;
  int karma;
  int i;
  int highest_nuyen;
  int nuyen;

  for (i = 1000 * 1000 * 1000; i > 0; i = highest_nuyen)
  {
    highest_nuyen = 0;
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].func == shop_keeper
          ||mob_index[nr].sfunc == shop_keeper)
        continue;
      nuyen = GET_NUYEN(&mob_proto[nr]) + GET_BANK(&mob_proto[nr]);
      if ( nuyen < i ) {
        if ( nuyen > highest_nuyen )
          highest_nuyen = nuyen;
        continue;
      }
      if ( i != 1000*1000*1000 && nuyen > i )
        continue;
      if (from_ip_zone(MOB_VNUM_RNUM(nr)))
        continue;
      karma = calc_karma(NULL, &mob_proto[nr]);
      ++found;
      sprintf(buf, "[%5ld] %4.2f (%4.2f)%4dx [%6d] %s\r\n",
              MOB_VNUM_RNUM(nr),
              (float)karma/100.0,
              (float)(GET_KARMA(&mob_proto[nr]))/100.0,
              mob_proto[nr].mob_specials.count_death,
              nuyen,
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_mobile_valuedeathkarma(char *searchname, struct char_data * ch)
{
  SPECIAL(shop_keeper);
  extern int calc_karma(struct char_data *ch, struct char_data *vict);
  extern struct char_data *mob_proto;
  int nr, found = 0;
  int karma;
  int i;
  int highest_karma;

  for (i = 10000; i > 0; i = highest_karma)
  {
    highest_karma = 0;
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].func == shop_keeper
          ||mob_index[nr].sfunc == shop_keeper)
        continue;
      if ( mob_proto[nr].mob_specials.value_death_karma < i ) {
        if ( mob_proto[nr].mob_specials.value_death_karma > highest_karma )
          highest_karma = mob_proto[nr].mob_specials.value_death_karma;
        continue;
      }
      if ( i != 10000 && mob_proto[nr].mob_specials.value_death_karma > i )
        continue;
      karma = calc_karma(NULL, &mob_proto[nr]);
      ++found;
      sprintf(buf, "[%5ld] %4.2f (%4.2f)%4dx %6.2f %s\r\n",
              MOB_VNUM_RNUM(nr),
              (float)karma/100.0,
              (float)(GET_KARMA(&mob_proto[nr]))/100.0,
              mob_proto[nr].mob_specials.count_death,
              (float)(mob_proto[nr].mob_specials.value_death_karma)/100.0,
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_mobile_valuedeath(char *searchname, struct char_data * ch)
{
  SPECIAL(shop_keeper);
  extern int calc_karma(struct char_data *ch, struct char_data *vict);
  int nr, found = 0;
  int karma;
  int highest_death;
  int i;

  for (i = 1000*1000*1000; i > 0; i = highest_death )
  {
    highest_death = 0;
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].func == shop_keeper
          ||mob_index[nr].sfunc == shop_keeper)
        continue;
      if ( (mob_proto[nr].mob_specials.value_death_nuyen
            + mob_proto[nr].mob_specials.value_death_items) < i ) {
        if ( (mob_proto[nr].mob_specials.value_death_nuyen
              + mob_proto[nr].mob_specials.value_death_items) > highest_death )
          highest_death = mob_proto[nr].mob_specials.value_death_nuyen
                          + mob_proto[nr].mob_specials.value_death_items;
        continue;
      }
      if ( (mob_proto[nr].mob_specials.value_death_nuyen
            + mob_proto[nr].mob_specials.value_death_items) > i )
        continue;
      karma = calc_karma(NULL, &mob_proto[nr]);
      ++found;
      sprintf(buf, "[%5ld] (%4.2f %4.2f)%4dx [%7d %7d] %s\r\n",
              MOB_VNUM_RNUM(nr),
              (float)karma/100.0,
              (float)(GET_KARMA(&mob_proto[nr]))/100.0,
              mob_proto[nr].mob_specials.count_death,
              mob_proto[nr].mob_specials.value_death_nuyen,
              mob_proto[nr].mob_specials.value_death_items,
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_mobile_valuedeathnuyen(char *searchname, struct char_data * ch)
{
  SPECIAL(shop_keeper);
  extern int calc_karma(struct char_data *ch, struct char_data *vict);
  int nr, found = 0;
  int karma;
  int highest_death;
  int i;


  for (i = 1000*1000*1000; i > 0; i = highest_death )
  {
    highest_death = 0;
    for (nr = 0; nr <= top_of_mobt; nr++) {
      if (mob_index[nr].func == shop_keeper
          ||mob_index[nr].sfunc == shop_keeper)
        continue;
      if ( (mob_proto[nr].mob_specials.value_death_nuyen) < i ) {
        if ( (mob_proto[nr].mob_specials.value_death_nuyen) > highest_death )
          highest_death = mob_proto[nr].mob_specials.value_death_nuyen;
        continue;
      }
      if ( (mob_proto[nr].mob_specials.value_death_nuyen) > i )
        continue;
      karma = calc_karma(NULL, &mob_proto[nr]);
      ++found;
      sprintf(buf, "[%5ld] (%4.2f %4.2f) [%7d %7d] %s\r\n",
              MOB_VNUM_RNUM(nr),
              (float)karma/100.0,
              (float)(GET_KARMA(&mob_proto[nr]))/100.0,
              mob_proto[nr].mob_specials.value_death_nuyen,
              mob_proto[nr].mob_specials.value_death_items,
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_vehicles(char *searchname, struct char_data * ch)
{
  int nr, found = 0;
  for (nr = 0; nr <= top_of_veht; nr++)
  {
    if (isname(searchname, veh_proto[nr].name)
        ||isname(searchname, veh_proto[nr].short_description)) {
      sprintf(buf, "%3d. [%5ld] %s\r\n", ++found,
              veh_index[nr].vnum,
              veh_proto[nr].short_description == NULL ? "(BUG)" :
              veh_proto[nr].short_description );
      send_to_char(buf, ch);
    }
  }

  return (found);
}

int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  if (!strcmp(searchname,"karmalist"))
    return vnum_mobile_karma(searchname,ch);
  if (!strcmp(searchname,"bonuskarmalist"))
    return vnum_mobile_bonuskarma(searchname,ch);
  if (!strcmp(searchname,"nuyenlist"))
    return vnum_mobile_nuyen(searchname,ch);
  if (!strcmp(searchname,"valuedeathkarma"))
    return vnum_mobile_valuedeathkarma(searchname,ch);
  if (!strcmp(searchname,"valuedeathnuyen"))
    return vnum_mobile_valuedeathnuyen(searchname,ch);
  if (!strcmp(searchname,"valuedeath"))
    return vnum_mobile_valuedeath(searchname,ch);
  for (nr = 0; nr <= top_of_mobt; nr++)
  {
    if (isname(searchname, mob_proto[nr].player.physical_text.keywords) ||
        isname(searchname, mob_proto[nr].player.physical_text.name)) {
      sprintf(buf, "%3d. [%5ld] %s\r\n", ++found,
              MOB_VNUM_RNUM(nr),
              mob_proto[nr].player.physical_text.name == NULL ? "(BUG)" :
              mob_proto[nr].player.physical_text.name);
      send_to_char(buf, ch);
    }
  }

  return (found);
}

int vnum_object_weapons(char *searchname, struct char_data * ch)
{
  char xbuf[MAX_STRING_LENGTH];
  extern const char *wound_arr[];
  int nr, found = 0;
  int power, severity, strength;


  for( severity = 4; severity >= 1; severity -- )
    for( power = 21; power >= 0; power-- )
      for( strength = 5; strength >= 0; strength-- )
      {
        for (nr = 0; nr <= top_of_objt; nr++) {
          if (GET_OBJ_TYPE(&obj_proto[nr]) != ITEM_WEAPON)
            continue;
          if (GET_OBJ_VAL(&obj_proto[nr],0) < power && power != 0)
            continue;
          if (GET_OBJ_VAL(&obj_proto[nr],1) < severity && severity != 1)
            continue;
          if (GET_OBJ_VAL(&obj_proto[nr],2) < strength && strength != 0)
            continue;
          if (GET_OBJ_VAL(&obj_proto[nr],0) > power && power != 21)
            continue;
          if (GET_OBJ_VAL(&obj_proto[nr],1) > severity && severity != 4)
            continue;
          if (GET_OBJ_VAL(&obj_proto[nr],2) > strength && strength != 5)
            continue;
          if (IS_OBJ_STAT(&obj_proto[nr], ITEM_GODONLY))
            continue;
          if (from_ip_zone(OBJ_VNUM_RNUM(nr)))
            continue;

          sprint_obj_mods( &obj_proto[nr], xbuf );

          ++found;
          sprintf(buf, "[%5ld -%2d] %2d%s +%d %s%s\r\n",
                  OBJ_VNUM_RNUM(nr),
                  ObjList.CountObj(nr),
                  GET_OBJ_VAL(&obj_proto[nr], 0),
                  wound_arr[GET_OBJ_VAL(&obj_proto[nr], 1)],
                  GET_OBJ_VAL(&obj_proto[nr], 2),
                  obj_proto[nr].text.name,
                  xbuf);
          send_to_char(buf, ch);
        }
      }
  return (found);
}

int vnum_object_armors(char *searchname, struct char_data * ch)
{
  char xbuf[MAX_STRING_LENGTH];
  int nr, found = 0;
  int ballistic, impact;

  for( ballistic = 11; ballistic >= -1; ballistic-- )
    for( impact = 11; impact >= -1;  impact-- )
    {
      for (nr = 0; nr <= top_of_objt; nr++) {
        if (GET_OBJ_TYPE(&obj_proto[nr]) != ITEM_WORN)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],0) < ballistic && ballistic != -1)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],1) < impact && impact != 1)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],0) > ballistic && ballistic != 11)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],1) > impact && impact != 11)
          continue;
        if (IS_OBJ_STAT(&obj_proto[nr], ITEM_GODONLY))
          continue;
        if (from_ip_zone(OBJ_VNUM_RNUM(nr)))
          continue;

        sprint_obj_mods( &obj_proto[nr], xbuf );

        ++found;
        sprintf(buf, "[%5ld -%2d] %2d %d %s%s\r\n",
                OBJ_VNUM_RNUM(nr),
                ObjList.CountObj(nr),
                GET_OBJ_VAL(&obj_proto[nr], 0),
                GET_OBJ_VAL(&obj_proto[nr], 1),
                obj_proto[nr].text.name,
                xbuf);
        send_to_char(buf, ch);
      }
    }
  return (found);
}

int vnum_object_clips(char *searchname, struct char_data * ch)
{
  int nr, found = 0;
  int capacity, type;

  for( type = 13; type <= 38; type++ )
    for( capacity = 101; capacity >= 0; capacity-- )
    {
      for (nr = 0; nr <= top_of_objt; nr++) {
        if (GET_OBJ_TYPE(&obj_proto[nr]) != ITEM_GUN_CLIP)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],0) < capacity && capacity != 0)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],1) < type && type != 13)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],0) > capacity && capacity != 101)
          continue;
        if (GET_OBJ_VAL(&obj_proto[nr],1) > type && type != 38)
          continue;
        if (IS_OBJ_STAT(&obj_proto[nr], ITEM_GODONLY))
          continue;
        if (from_ip_zone(OBJ_VNUM_RNUM(nr)))
          continue;

        ++found;
        sprintf(buf, "[%5ld -%2d wt %f] %2d %3d %s\r\n",
                OBJ_VNUM_RNUM(nr),
                ObjList.CountObj(nr),
                GET_OBJ_WEIGHT(&obj_proto[nr]),
                GET_OBJ_VAL(&obj_proto[nr], 1),
                GET_OBJ_VAL(&obj_proto[nr], 0),
                obj_proto[nr].text.name);
        send_to_char(buf, ch);
      }
    }
  return (found);
}

int vnum_object_foci(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
  {
    if (GET_OBJ_TYPE(&obj_proto[nr]) == ITEM_FOCUS
        && !from_ip_zone(OBJ_VNUM_RNUM(nr))) {
      sprintf(buf, "%3d. [%5ld -%2d] %s %d +%2d %s\r\n", ++found,
              OBJ_VNUM_RNUM(nr),
              ObjList.CountObj(nr),
              from_ip_zone(OBJ_VNUM_RNUM(nr)) ? " " : "*",
              GET_OBJ_VAL(&obj_proto[nr], VALUE_FOCUS_TYPE),
              GET_OBJ_VAL(&obj_proto[nr], VALUE_FOCUS_RATING),
              obj_proto[nr].text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_object_type(int type, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
  {
    if (GET_OBJ_TYPE(&obj_proto[nr]) == type) {
      ++found;
      sprintf(buf, "[%5ld -%2d] %s %s\r\n",
              OBJ_VNUM_RNUM(nr),
              ObjList.CountObj(nr),
              from_ip_zone(OBJ_VNUM_RNUM(nr)) ? " " : "*",
              obj_proto[nr].text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}

int vnum_object_affectloc(int type, struct char_data * ch)
{
  char xbuf[MAX_STRING_LENGTH];
  int nr, found = 0, mod;

  for( mod = 11; mod >= -11; mod -- )
    for (nr = 0; nr <= top_of_objt; nr++)
    {
      if (IS_OBJ_STAT(&obj_proto[nr], ITEM_GODONLY))
        continue;
      if (from_ip_zone(OBJ_VNUM_RNUM(nr)))
        continue;

      for (register int i = 0; i < MAX_OBJ_AFFECT; i++)
        if (obj_proto[nr].affected[i].location == type ) {
          if (obj_proto[nr].affected[i].modifier < mod && mod != -11)
            continue;
          if (obj_proto[nr].affected[i].modifier > mod && mod != 11)
            continue;

          sprint_obj_mods( &obj_proto[nr], xbuf );

          ++found;
          sprintf(buf, "[%5ld -%2d] %s%s\r\n",
                  OBJ_VNUM_RNUM(nr),
                  ObjList.CountObj(nr),
                  obj_proto[nr].text.name,
                  xbuf);
          send_to_char(buf, ch);
          break;
        }
    }
  return (found);
}

int vnum_object(char *searchname, struct char_data * ch)
{
  int nr, found = 0;
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];

  two_arguments(searchname, arg1, arg2);

  if (!strcmp(searchname,"weaponslist"))
    return vnum_object_weapons(searchname,ch);
  if (!strcmp(searchname,"armorslist"))
    return vnum_object_armors(searchname,ch);
  if (!strcmp(searchname,"clipslist"))
    return vnum_object_clips(searchname,ch);
  if (!strcmp(searchname,"focilist"))
    return vnum_object_foci(searchname,ch);
  if (!strcmp(arg1,"objtype"))
    return vnum_object_type(atoi(arg2),ch);
  if (!strcmp(arg1,"affectloc"))
    return vnum_object_affectloc(atoi(arg2),ch);
  for (nr = 0; nr <= top_of_objt; nr++)
  {
    if (isname(searchname, obj_proto[nr].text.keywords) ||
        isname(searchname, obj_proto[nr].text.name)) {
      sprintf(buf, "%3d. [%5ld -%2d] %s %s\r\n", ++found,
              OBJ_VNUM_RNUM(nr),
              ObjList.CountObj(nr),
              from_ip_zone(OBJ_VNUM_RNUM(nr)) ? " " : "*",
              obj_proto[nr].text.name);
      send_to_char(buf, ch);
    }
  }
  return (found);
}


struct veh_data *read_vehicle(int nr, int type)
{
  int i;
  struct veh_data *veh;

  if (type == VIRTUAL)
  {
    if ((i = real_vehicle(nr)) < 0) {
      sprintf(buf, "Vehicle (V) %d does not exist in database.", nr);
      return (0);
    }
  } else
    i = nr;
  veh = Mem->GetVehicle();
  *veh = veh_proto[i];
  veh->next = veh_list;
  veh_list = veh;
  veh_index[i].number++;
  return veh;
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(int nr, int type)
{
  int i;
  struct char_data *mob;

  if (type == VIRTUAL)
  {
    if ((i = real_mobile(nr)) < 0) {
      sprintf(buf, "Mobile (V) %d does not exist in database.", nr);
      return (0);
    }
  } else
    i = nr;

  mob = Mem->GetCh();
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  mob->points.physical = mob->points.max_physical;
  mob->points.mental = mob->points.max_mental;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);
  mob->char_specials.saved.left_handed = (!number(0, 9) ? 1 : 0);
  GET_WIELDED(mob, 0) = 0;
  GET_WIELDED(mob, 1) = 0;

  mob_index[i].number++;

  affect_total(mob);

  if (GET_MOB_SPEC(mob) && !MOB_FLAGGED(mob, MOB_SPEC))
    MOB_FLAGS(mob).SetBit(MOB_SPEC);

  return mob;
}

/* create a new mobile from a prototype */
struct matrix_icon *read_ic(int nr, int type)
{
  int i;
  struct matrix_icon *ic;

  if (type == VIRTUAL)
  {
    if ((i = real_ic(nr)) < 0) {
      sprintf(buf, "IC (V) %d does not exist in database.", nr);
      return (0);
    }
  } else
    i = nr;

  ic = Mem->GetIcon();
  *ic = ic_proto[i];
  ic_index[i].number++;
  ic->condition = 10;
  ic->idnum = number(0, 20000);
  ic->next = icon_list;
  icon_list = ic;

  return ic;
}
/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  obj = Mem->GetObject();
  ObjList.ADD(obj);

  return obj;
}

/* create a new object from a prototype */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0)
  {
    log("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL)
  {
    if ((i = real_object(nr)) < 0) {
      sprintf(buf, "Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  obj = Mem->GetObject();
  *obj = obj_proto[i];
  ObjList.ADD(obj);
  obj_index[i].number++;
  if (GET_OBJ_TYPE(obj) == ITEM_PHONE)
  {
    switch (GET_OBJ_VAL(obj, 0)) {
    case 0:
      GET_OBJ_VAL(obj, 8) = 1102;
      sprintf(buf, "%d206", number(0, 9));
      break;
    case 1:
      GET_OBJ_VAL(obj, 8) = 1102;
      sprintf(buf, "%d321", number(0, 9));
      break;
    case 2:
      GET_OBJ_VAL(obj, 8) = 1103;
      sprintf(buf, "%d503", number(0, 9));
      break;
    }
    GET_OBJ_VAL(obj, 0) = atoi(buf);
    GET_OBJ_VAL(obj, 1) = number(0, 9999);
  } else if (GET_OBJ_TYPE(obj) == ITEM_CYBERWARE && GET_OBJ_VAL(obj, 2) == 4)
  {
    switch (GET_OBJ_VAL(obj, 3)) {
    case 0:
      GET_OBJ_VAL(obj, 8) = 1102;
      sprintf(buf, "%d206", number(0, 9));
      break;
    case 1:
      GET_OBJ_VAL(obj, 8) = 1102;
      sprintf(buf, "%d321", number(0, 9));
      break;
    case 2:
      GET_OBJ_VAL(obj, 8) = 1103;
      sprintf(buf, "%d503", number(0, 9));
      break;
    }
    GET_OBJ_VAL(obj, 3) = atoi(buf);
    GET_OBJ_VAL(obj, 6) = number(0, 9999);
  } else if (GET_OBJ_TYPE(obj) == ITEM_GUN_CLIP)
    GET_OBJ_VAL(obj, 9) = GET_OBJ_VAL(obj, 0);

  return obj;
}

void spec_update(void)
{
  int i;

  for (i = 0; i < top_of_world; i++)
    if (world[i].func != NULL)
      world[i].func (NULL, world + i, 0, "");

  ObjList.CallSpec();
}

#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  static int timer = 0;
  /* Alot of good things came from 1992, like my next door neighbour's little sister for example.
     The original version of this function, however, was not one of those things - Che */
  /* jelson 10/22/92 */
  //   ^-- Retard

  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    timer = 0;
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
          zone_table[i].reset_mode)
        (zone_table[i].age)++;

      if (zone_table[i].age >= MAX(zone_table[i].lifespan,5) &&
          zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode &&
          (zone_table[i].reset_mode == 2 ||
           is_empty(i))) {
        reset_zone(i, 0);
        sprintf(buf, "Auto zone reset: %s",
                zone_table[i].name);
        mudlog(buf, NULL, LOG_ZONELOG, FALSE);
      }
    }
  }
}

void log_zone_error(int zone, int cmd_no, char *message)
{
  char buf[256];

  sprintf(buf, "error in zone file: %s", message);
  mudlog(buf, NULL, LOG_ZONELOG, TRUE);

  sprintf(buf, " ...offending cmd: '%c' cmd in zone #%d, line %d, cmd %d",
          ZCMD.command, zone_table[zone].number, ZCMD.line, cmd_no);
  mudlog(buf, NULL, LOG_ZONELOG, TRUE);
}

#define ZONE_ERROR(message) {log_zone_error(zone, cmd_no, message); last_cmd = 0;}

/* execute the reset command table of a given zone */
void reset_zone(int zone, int reboot)
{
  SPECIAL(fixer);
  int cmd_no, j, last_cmd = 0, found = 0, no_mob = 0;
  int sig = 0, load = 0;
  static int i;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to, *check;
  struct veh_data *veh = NULL;

  for (cmd_no = 0; cmd_no < zone_table[zone].num_cmds; cmd_no++) {
    if (ZCMD.if_flag && !last_cmd)
      continue;
    found = 0;
    switch (ZCMD.command) {
    case '$':
      last_cmd = 0;
      break;
    case '*':                 /* ignore command */
      last_cmd = 0;
      break;
    case 'M':                 /* read a mobile */
      if ((mob_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        mob = read_mobile(ZCMD.arg1, REAL);
        char_to_room(mob, ZCMD.arg3);
        last_cmd = 1;
      } else {
        if (ZCMD.arg2 == 0 && !reboot)
          no_mob = 1;
        else
          no_mob = 0;
        last_cmd = 0;
        mob = NULL;
      }
      break;
    case 'S':                 /* read a mobile */
      if (!veh)
        break;
      if ((mob_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        mob = read_mobile(ZCMD.arg1, REAL);
        if (!veh->people) {
          AFF_FLAGS(mob).SetBit(AFF_PILOT);
          veh->cspeed = SPEED_CRUISING;
        }
        char_to_veh(veh, mob);
        last_cmd = 1;
      } else {
        if (ZCMD.arg2 == 0 && !reboot)
          no_mob = 1;
        else
          no_mob = 0;
        last_cmd = 0;
        mob = NULL;
      }
      break;
    case 'U':                 /* mount/upgrades a vehicle with object */
      if (!veh)
        break;
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        obj = read_object(ZCMD.arg1, REAL);
        if (GET_OBJ_VAL(obj, 0) == MOD_MOUNT) {
          switch (GET_OBJ_VAL(obj, 1)) {
          case 1:
            sig = 1;
          case 0:
            load = 10;
            break;
          case 3:
            sig = 1;
          case 2:
            load = 10;
            break;
          case 4:
            sig = 1;
            load = 100;
            break;
          case 5:
            sig = 1;
            load = 25;
            break;
          }
          veh->load -= load;
          veh->sig -= sig;

          if (veh->mount)
            obj->next_content = veh->mount;
          veh->mount = obj;
        } else {
          GET_MOD(veh, GET_OBJ_VAL(obj, 0)) = obj;
          veh->load -= GET_OBJ_VAL(obj, 1);
          for (int j = 0; j < MAX_OBJ_AFFECT; j++)
            affect_veh(veh, obj->affected[j].location, obj->affected[j].modifier);
        }
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'I':                 /* puts an item into vehicle */
      if (!veh)
        break;
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        obj = read_object(ZCMD.arg1, REAL);
        obj_to_veh(obj, veh);
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'V':
      if ((veh_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        veh = read_vehicle(ZCMD.arg1, REAL);
        veh_to_room(veh, ZCMD.arg3);
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'H':
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        obj = read_object(ZCMD.arg1, REAL);
        obj->next_content = matrix[ZCMD.arg3].file;
        matrix[ZCMD.arg3].file = obj;
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'O':                 /* read an object */
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        obj = read_object(ZCMD.arg1, REAL);
        obj_to_room(obj, ZCMD.arg3);
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'P':                 /* object to object */
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        obj = read_object(ZCMD.arg1, REAL);
        if (!(obj_to = ObjList.FindObj(ZCMD.arg3))) {
          ZONE_ERROR("target obj not found");
          break;
        }
        if (obj != obj_to)
          obj_to_obj(obj, obj_to);
        if (GET_OBJ_TYPE(obj_to) == ITEM_HOLSTER)
          GET_OBJ_VAL(obj_to, 3) = 1;
        if (!from_ip_zone(GET_OBJ_VNUM(obj)) && !zone_table[zone].connected)
          GET_OBJ_EXTRA(obj).SetBit(ITEM_VOLATILE);
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'G':                 /* obj_to_char */
      if (!mob) {
        if (!no_mob)
          ZONE_ERROR("attempt to give obj to non-existant mob");
        break;
      }
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        obj = read_object(ZCMD.arg1, REAL);
        obj_to_char(obj, mob);
        if (!from_ip_zone(GET_OBJ_VNUM(obj)) && !zone_table[zone].connected)
          GET_OBJ_EXTRA(obj).SetBit(ITEM_VOLATILE);
        last_cmd = 1;
      } else
        last_cmd = 0;
      break;
    case 'E':                 /* object to equipment list */
      if (!mob) {
        if (!no_mob)
          ZONE_ERROR("trying to equip non-existant mob");
        break;
      }
      if ((obj_index[ZCMD.arg1].number < ZCMD.arg2) || (ZCMD.arg2 == -1) ||
          (ZCMD.arg2 == 0 && reboot)) {
        if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
          ZONE_ERROR("invalid equipment pos number");
        } else {
          obj = read_object(ZCMD.arg1, REAL);
          equip_char(mob, obj, ZCMD.arg3);
          if (!from_ip_zone(GET_OBJ_VNUM(obj)) && !zone_table[zone].connected)
            GET_OBJ_EXTRA(obj).SetBit(ITEM_VOLATILE);
          last_cmd = 1;
        }
      } else
        last_cmd = 0;
      break;
    case 'N':  // give x number of items to a mob
      if (!mob) {
        if (!no_mob)
          ZONE_ERROR("attempt to give obj to non-existant mob");
        break;
      }
      last_cmd = 0;
      for (i = 0; (i < ZCMD.arg3) && ((obj_index[ZCMD.arg1].number < ZCMD.arg2) ||
                                      (ZCMD.arg2 == -1) || (ZCMD.arg2 == 0 && reboot)); ++i) {
        obj = read_object(ZCMD.arg1, REAL);
        obj_to_char(obj, mob);
        if (!from_ip_zone(GET_OBJ_VNUM(obj)) && !zone_table[zone].connected)
          GET_OBJ_EXTRA(obj).SetBit(ITEM_VOLATILE);
        last_cmd = 1;
      }
      break;
    case 'C': // give mob bio/cyberware
      if (!mob) {
        if (!no_mob)
          ZONE_ERROR("attempt to give obj to non-existant mob");
        break;
      }
      obj = read_object(ZCMD.arg1, REAL);
      if (!ZCMD.arg2) {
        if (GET_OBJ_TYPE(obj) != ITEM_CYBERWARE) {
          ZONE_ERROR("attempt to install non-cyberware to mob");
          break;
        }
        if (GET_ESS(mob) < GET_OBJ_VAL(obj, 1))
          break;
        for (check = mob->cyberware; check && !found; check = check->next_content) {
          if ((GET_OBJ_VNUM(check) == GET_OBJ_VNUM(obj)))
            found = 1;
          if (GET_OBJ_VAL(check, 2) == GET_OBJ_VAL(obj, 2))
            found = 1;
        }
        if (GET_OBJ_VAL(obj, 2) == 23 || GET_OBJ_VAL(obj, 2) == 30 ||
            GET_OBJ_VAL(obj, 2) == 20)
          for (check = mob->bioware; check && !found; check = check->next_content) {
            if (GET_OBJ_VAL(check, 2) == 2 && GET_OBJ_VAL(obj, 2) == 23)
              found = 1;
            if (GET_OBJ_VAL(check, 2) == 8 && GET_OBJ_VAL(obj, 2) == 30)
              found = 1;
            if (GET_OBJ_VAL(check, 2) == 10 && GET_OBJ_VAL(obj, 2) == 20)
              found = 1;
          }
        if (found)
          break;
        obj_to_cyberware(obj, mob);
      } else {
        if (GET_OBJ_TYPE(obj) != ITEM_BIOWARE) {
          ZONE_ERROR("attempt to install non-bioware to mob");
          break;
        }
        if (GET_INDEX(mob) < GET_OBJ_VAL(obj, 1))
          break;
        for (check = mob->bioware; check && !found; check = check->next_content) {
          if ((GET_OBJ_VNUM(check) == GET_OBJ_VNUM(obj)))
            found = 1;
          if (GET_OBJ_VAL(check, 2) == GET_OBJ_VAL(obj, 2))
            found = 1;
        }
        if (GET_OBJ_VAL(obj, 2) == 2 || GET_OBJ_VAL(obj, 2) == 8 || GET_OBJ_VAL(obj, 2) == 10)
          for (check = mob->cyberware; check; check = check->next_content) {
            if (GET_OBJ_VAL(check, 2) == 23 && GET_OBJ_VAL(obj, 2) == 2)
              found = 1;
            if (GET_OBJ_VAL(check, 2) == 30 && GET_OBJ_VAL(obj, 2) == 8)
              found = 1;
            if (GET_OBJ_VAL(check, 2) == 20 && GET_OBJ_VAL(obj, 2) == 10)
              found = 1;
          }
        if (found)
          break;
        if (GET_OBJ_VAL(obj, 2) == 0)
          GET_OBJ_VAL(obj, 5) = 24;
        obj_to_bioware(obj, mob);
      }
      last_cmd = 1;
      break;
    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL) {
        obj_from_room(obj);
        extract_obj(obj);
      }
      last_cmd = 1;
      break;
    case 'D':                 /* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
          (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
        ZONE_ERROR("door does not exist");
      } else {
        bool ok = FALSE;
        int opposite = MAX(0, world[ZCMD.arg1].dir_option[ZCMD.arg2]->to_room);
        if (!world[opposite].dir_option[rev_dir[ZCMD.arg2]] || (ZCMD.arg1 !=
            world[opposite].dir_option[rev_dir[ZCMD.arg2]]->to_room)) {
          sprintf(buf, "Note: Exits from %ld to %ld do not coincide (zone %d, line %d, cmd %d)",
                  world[opposite].number, world[ZCMD.arg1].number, zone_table[zone].number,
                  ZCMD.line, cmd_no);
          mudlog(buf, NULL, LOG_ZONELOG, FALSE);
        } else
          ok = TRUE;
        // here I set the hidden flag for the door if hidden > 0
        if (world[ZCMD.arg1].dir_option[ZCMD.arg2]->hidden)
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
        if (ok && world[opposite].dir_option[rev_dir[ZCMD.arg2]]->hidden)
          SET_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_HIDDEN);
        // repair all damage
        REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_DESTROYED);
        world[ZCMD.arg1].dir_option[ZCMD.arg2]->condition =
          world[ZCMD.arg1].dir_option[ZCMD.arg2]->barrier;
        if (ok) {
          world[opposite].dir_option[rev_dir[ZCMD.arg2]]->material =
            world[ZCMD.arg1].dir_option[ZCMD.arg2]->material;
          world[opposite].dir_option[rev_dir[ZCMD.arg2]]->barrier =
            world[ZCMD.arg1].dir_option[ZCMD.arg2]->barrier;
          world[opposite].dir_option[rev_dir[ZCMD.arg2]]->condition =
            world[ZCMD.arg1].dir_option[ZCMD.arg2]->condition;
          REMOVE_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_DESTROYED);
        }
        switch (ZCMD.arg3) {
          // you now only have to set one side of a door
        case 0:
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
          if (ok) {
            REMOVE_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_LOCKED);
            REMOVE_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_CLOSED);
          }
          break;
        case 1:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
          REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
          if (ok) {
            SET_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_CLOSED);
            REMOVE_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_LOCKED);
          }
          break;
        case 2:
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
          SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
          if (ok) {
            SET_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_LOCKED);
            SET_BIT(world[opposite].dir_option[rev_dir[ZCMD.arg2]]->exit_info, EX_CLOSED);
          }
          break;
        }
      }
      last_cmd = 1;
      break;
    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;
  if (zone_table[zone].alert != 4)
    zone_table[zone].alert = 0;
  if (zone == real_zone(31))
    for (j = 0; j <= top_of_zone_table; j++)
      zone_table[j].alert = 0;
}

/* for use in reset_zone; return TRUE if zone 'nr' is Free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
        return 0;

  return 1;
}


/************************************************************************
*  funcs of a (more or less) general utility nature                     *
********************************************************************** */

// These were added for OLC purposes.  They will allocate a new array of
// the old size + 100 elements, copy over, and Free up the old one.
// They return TRUE if it worked, FALSE if it did not.  I could add in some
// checks and report to folks using OLC that currently there is not enough
// room to allocate.  Obviously these belong in an object once I convert
// completely over to C++.
bool resize_world_array()
{
  int counter;
  struct room_data *new_world;

  new_world = new struct room_data[top_of_world_array + world_chunk_size];

  if (!new_world) {
    mudlog("Unable to allocate new world array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;  // disallow any olc from here on
    return FALSE;
  }

  // remember that top_of_world is the actual rnum in the array
  for (counter = 0; counter <= top_of_world; counter++)
    new_world[counter] = world[counter];

  top_of_world_array += world_chunk_size;

  delete [] world;
  world = new_world;

  sprintf(buf, "World array resized to %d.", top_of_world_array);
  mudlog(buf, NULL, LOG_SYSLOG, TRUE);
  return TRUE;
}

bool resize_mob_array()
{
  int counter;
  struct char_data *new_mob;
  struct index_data *new_mob_index;

  new_mob = new struct char_data[top_of_mob_array + mob_chunk_size];

  if (!new_mob) {
    mudlog("Unable to allocate new mob array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;
    return FALSE;
  }

  new_mob_index = new struct index_data[top_of_mob_array + mob_chunk_size];

  if (!new_mob_index) {
    mudlog("Unable to allocate new mob index array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;
    return FALSE;
  }

  // do both mob_proto and mob_index
  for (counter = 0; counter <= top_of_mobt; counter++) {
    new_mob[counter] = mob_proto[counter];
    new_mob_index[counter] = mob_index[counter];
  }

  top_of_mob_array += mob_chunk_size;

  delete [] mob_proto;
  delete [] mob_index;
  mob_proto = new_mob;
  mob_index = new_mob_index;

  sprintf(buf, "Mob and Mob Index arrays resized to %d", top_of_mob_array);
  mudlog(buf, NULL, LOG_SYSLOG, TRUE);
  return TRUE;
}

bool resize_obj_array()
{
  int counter;
  struct obj_data *new_obj;
  struct index_data *new_obj_index;

  new_obj = new struct obj_data[top_of_obj_array + obj_chunk_size];

  if (!new_obj) {
    mudlog("Unable to allocate new obj array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;
    return FALSE;
  }

  new_obj_index = new struct index_data[top_of_obj_array + obj_chunk_size];

  if (!new_obj_index) {
    mudlog("Unable to allocate new obj index array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;
    return FALSE;
  }

  // do for both obj_proto and obj_index
  for (counter = 0; counter <= top_of_objt; counter++) {
    new_obj[counter] = obj_proto[counter];
    new_obj_index[counter] = obj_index[counter];
  }

  top_of_obj_array += obj_chunk_size;

  delete [] obj_proto;
  delete [] obj_index;
  obj_proto = new_obj;
  obj_index = new_obj_index;

  sprintf(buf, "Obj and Obj Index arrays resized to %d", top_of_obj_array);
  mudlog(buf, NULL, LOG_SYSLOG, TRUE);
  return TRUE;
}

bool resize_qst_array(void)
{
  int counter;
  struct quest_data *new_qst;

  new_qst = new struct quest_data[top_of_quest_array + quest_chunk_size];

  if (!new_qst) {
    mudlog("Unable to allocate new quest array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;
    return FALSE;
  }

  for (counter = 0; counter <= top_of_questt; counter++)
    new_qst[counter] = quest_table[counter];

  top_of_quest_array += quest_chunk_size;

  delete [] quest_table;
  quest_table = new_qst;

  sprintf(buf, "Quest array resized to %d", top_of_quest_array);
  mudlog(buf, NULL, LOG_SYSLOG, TRUE);
  return TRUE;
}

bool resize_shp_array(void)
{
  int counter;
  struct shop_data *new_shp;

  new_shp = new struct shop_data[top_of_shop_array + shop_chunk_size];

  if (!new_shp) {
    mudlog("Unable to allocate new shop array.", NULL, LOG_SYSLOG, TRUE);
    mudlog("OLC temporarily unavailable.", NULL, LOG_SYSLOG, TRUE);
    olc_state = 0;
    return FALSE;
  }

  for (counter = 0; counter <= top_of_shopt; counter++)
    new_shp[counter] = shop_table[counter];

  top_of_shop_array += shop_chunk_size;

  delete [] shop_table;
  shop_table = new_shp;

  sprintf(buf, "Shop array resized to %d", top_of_shop_array);
  mudlog(buf, NULL, LOG_SYSLOG, TRUE);
  return TRUE;
}

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH+3], tmp[512+3], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  /* FULLY initialize the buffer array. This is important, because you
  can't have garbage being read in. Doing the first byte isn't really good
  enough, and using memset or bzero is something I don't like. */
  for (int x=0; x <= MAX_STRING_LENGTH; x++) {
    buf[x] = '\0';
  }

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n", error);
      shutdown();
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr((const char *)tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
      /* Instead of an unconditional 'else', we only replace on what we want
      to replace, instead of acting blindly. */
    } else if ((point = strchr((const char *)tmp, '\n')) != NULL) {
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if ((length + templength + 3) >= MAX_STRING_LENGTH) {
      fprintf(stderr, "SYSERR: fread_string: string too large at or near %s\n", error);
      log("SYSERR: fread_string: string too large (db.c)");
      shutdown();
    } else {

      /* Last but not least, we copy byte by byte from array to array. It's
      really safer this way, not to mention much more portable. */
      for (int y=0; y <= templength; y++) {
        buf[length + y] = tmp[y];
      }
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    rslt = new char[length + 1];
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}

/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int i;
  struct alias *a, *temp, *next;
  struct remem *b, *nextr;
  extern void free_alias(struct alias * a);

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob)
  {
    // we have to delete these here before we delete the struct
    for (a = GET_ALIASES(ch); a; a = next) {
      next = a->next;
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }

    for (b = GET_MEMORY(ch); b; b = nextr) {
      nextr = b->next;
      delete [] b->mem;
      delete b;
    }

    if (ch->player_specials->mob_complete)
      delete [] ch->player_specials->mob_complete;
    if (ch->player_specials->obj_complete)
      delete [] ch->player_specials->obj_complete;
    if (ch->player_specials->gname)
      delete [] ch->player_specials->gname;

    if (IS_NPC(ch))
      log("SYSERR: Mob had player_specials allocated!");
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1))
  {
    if (ch->player.prompt)
      delete [] ch->player.prompt;
    if (ch->player.poofin)
      delete [] ch->player.poofin;
    if (ch->player.poofout)
      delete [] ch->player.poofout;
    if (ch->player.title)
      delete [] ch->player.title;
    if (ch->player.pretitle)
      delete [] ch->player.pretitle;
    if (ch->player.whotitle)
      delete [] ch->player.whotitle;

    {
      text_data *tab[3] = {
                            &ch->player.physical_text,
                            &ch->player.astral_text,
                            &ch->player.matrix_text
                          };

      for (int i = 0; i < 3; i++) {
        text_data *ptr = tab[i];

        if (ptr->keywords) {
          ptr->keywords = NULL;
          delete [] ptr->keywords;
        }

        if (ptr->name) {
          ptr->name = NULL;
          delete [] ptr->name;
        }

        if (ptr->room_desc) {
          ptr->room_desc = NULL;
          delete [] ptr->room_desc;
        }

        if (ptr->look_desc) {
          ptr->look_desc = NULL;
          delete [] ptr->look_desc;
        }
      }
    }

    if(!IS_NPC(ch))
      if (ch->player.host) {
        ch->player.host = NULL;
        delete [] ch->player.host;
      }
  } else if ((i = GET_MOB_RNUM(ch)) > -1 &&
             GET_MOB_VNUM(ch) != 20 && GET_MOB_VNUM(ch) != 22)
  {
    /* otherwise, Free strings only if the string is not pointing at proto */
    text_data *tab[3] = {
                          &ch->player.physical_text,
                          &ch->player.astral_text,
                          &ch->player.matrix_text
                        };

    char_data *proto = mob_proto+i;

    text_data *proto_tab[3] = {
                                &proto->player.physical_text,
                                &proto->player.astral_text,
                                &proto->player.matrix_text
                              };

    for (int i = 0; i < 3; i++) {
      text_data *ptr = tab[i];
      text_data *proto_ptr = proto_tab[i];

      if (ptr->keywords && ptr->keywords != proto_ptr->keywords) {
        delete [] ptr->keywords;
        ptr->keywords = NULL;
      }

      if (ptr->name && ptr->name != proto_ptr->name) {
        delete [] ptr->name;
        ptr->name = NULL;
      }

      if (ptr->room_desc && ptr->room_desc != proto_ptr->room_desc) {
        delete [] ptr->room_desc;
        ptr->room_desc = NULL;
      }

      if (ptr->look_desc && ptr->look_desc != proto_ptr->look_desc) {
        delete [] ptr->look_desc;
        ptr->look_desc = NULL;
      }
    }
  }
  while (ch->affected)
    affect_remove(ch, ch->affected, 0);

  clear_char(ch);
}

void free_room(struct room_data *room)
{
  struct extra_descr_data *This, *next_one;

  // first free up the strings
  if (room->name)
    delete [] room->name;
  if (room->description)
    delete [] room->description;

  // then free up the exits
  for (int counter = 0; counter < NUM_OF_DIRS; counter++)
  {
    if (room->dir_option[counter]) {
      if (room->dir_option[counter]->general_description)
        delete [] room->dir_option[counter]->general_description;
      if (room->dir_option[counter]->keyword)
        delete [] room->dir_option[counter]->keyword;
      delete room->dir_option[counter];
    }
  }
  // now the extra descriptions
  if (room->ex_description)
    for (This = room->ex_description; This; This = next_one)
    {
      next_one = This->next;
      if (This->keyword)
        delete [] This->keyword;
      if (This->description)
        delete [] This->description;
      delete This;
    }

  clear_room(room);
}

void free_veh(struct veh_data * veh)
{
  clear_vehicle(veh);
}

void free_host(struct host_data * host)
{
  if (host->name)
    delete [] host->name;
  if (host->keywords)
    delete [] host->keywords;
  if (host->desc)
    delete [] host->desc;
  if (host->shutdown_start)
    delete [] host->shutdown_start;
  if (host->shutdown_stop)
    delete [] host->shutdown_stop;
  clear_host(host);
}

void free_icon(struct matrix_icon * icon)
{
  if (icon->name && !icon->number)
    delete [] icon->name;
  if (icon->long_desc && !icon->number)
    delete [] icon->long_desc;
  if (icon->look_desc && !icon->number)
    delete [] icon->look_desc;
  clear_icon(icon);
}
/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *this1, *next_one;
  if ((nr = GET_OBJ_RNUM(obj)) == -1)
  {
    if (obj->text.keywords)
      delete [] obj->text.keywords;
    if (obj->text.name)
      delete [] obj->text.name;
    if (obj->text.room_desc)
      delete [] obj->text.room_desc;
    if (obj->text.look_desc)
      delete [] obj->text.look_desc;

    if (obj->ex_description)
      for (this1 = obj->ex_description; this1; this1 = next_one) {
        next_one = this1->next;
        if (this1->keyword)
          delete [] this1->keyword;
        if (this1->description)
          delete [] this1->description;
        delete this1;
      }
  } else
  {
    if (obj->text.keywords &&
        obj->text.keywords != obj_proto[nr].text.keywords)
      delete [] obj->text.keywords;

    if (obj->text.name &&
        obj->text.name != obj_proto[nr].text.name)
      delete [] obj->text.name;

    if (obj->text.room_desc &&
        obj->text.room_desc != obj_proto[nr].text.room_desc)
      delete [] obj->text.room_desc;

    if (obj->text.look_desc &&
        obj->text.look_desc != obj_proto[nr].text.look_desc)
      delete [] obj->text.look_desc;

    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
      for (this1 = obj->ex_description; this1; this1 = next_one) {
        next_one = this1->next;
        if (this1->keyword)
          delete [] this1->keyword;
        if (this1->description)
          delete [] this1->description;
        delete this1;
      }
  }
  if (obj->restring)
    delete [] obj->restring;
  if (obj->photo)
    delete [] obj->photo;
  clear_object(obj);
}

void free_quest(struct quest_data *quest)
{
  if (quest->obj)
    delete [] quest->obj;
  if (quest->mob)
    delete [] quest->mob;
  if (quest->intro)
    delete [] quest->intro;
  if (quest->decline)
    delete [] quest->decline;
  if (quest->quit)
    delete [] quest->quit;
  if (quest->finish)
    delete [] quest->finish;
  if (quest->info)
    delete [] quest->info;
  if (quest->done)
    delete [] quest->done;
}

void free_shop(struct shop_data *shop)
{
  if (shop->no_such_item1)
    delete [] shop->no_such_item1;
  if (shop->no_such_item2)
    delete [] shop->no_such_item2;
  if (shop->missing_cash1)
    delete [] shop->missing_cash1;
  if (shop->missing_cash2)
    delete [] shop->missing_cash2;
  if (shop->do_not_buy)
    delete [] shop->do_not_buy;
  if (shop->message_buy)
    delete [] shop->message_buy;
  if (shop->message_sell)
    delete [] shop->message_sell;
  if (shop->producing)
    delete [] shop->producing;
  if (shop->type)
    delete [] shop->type;
  if (shop->in_room)
    delete [] shop->in_room;
}

/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];

  if (file_to_string(name, temp) < 0)
    return -1;

  if (*buf)
    delete [] *buf;

  *buf = str_dup(temp);

  return 0;
}

/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
{
  FILE *fl;
  char tmp[128];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    sprintf(tmp, "Error reading %s", name);
    perror(tmp);
    return (-1);
  }
  do {
    fgets(tmp, 128, fl);
    tmp[strlen(tmp) - 1] = '\0';/* take off the trailing \n */
    strcat(tmp, "\r\n");

    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
        log("SYSERR: fl->strng: string too big (db.c, file_to_string)");
        *buf = '\0';
        return (-1);
      }
      strcat(buf, tmp);
    }
  } while (!feof(fl));

  fclose(fl);

  return (0);
}

/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  ch->in_veh = NULL;
  ch->followers = NULL;
  ch->master = NULL;
  ch->in_room = NOWHERE;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  calc_weight(ch);

  if (GET_PHYSICAL(ch) < 100)
    GET_PHYSICAL(ch) = 100;
  if (GET_MENTAL(ch) < 100)
    GET_MENTAL(ch) = 100;
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_veh = NULL;
  ch->in_room = NOWHERE;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  GET_BALLISTIC(ch) = 0;                /* Basic Armor */
  GET_IMPACT(ch) = 0;
  if (ch->points.max_mental < 1000)
    ch->points.max_mental = 1000;
}

/* Clear ALL the vars of an object; don't free up space though */
// we do this because generally, objects which are created are off of
// prototypes, and we do not want to delete the prototype strings
void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));
  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
}

void clear_room(struct room_data *room)
{
  memset((char *) room, 0, sizeof(struct room_data));
}

void clear_vehicle(struct veh_data *veh)
{
  memset((char *) veh, 0, sizeof(struct veh_data));
  veh->in_room = NOWHERE;
}

void clear_host(struct host_data *host)
{
  memset((char *) host, 0, sizeof(struct host_data));
}

void clear_icon(struct matrix_icon *icon)
{
  memset((char *) icon, 0, sizeof(struct matrix_icon));
  icon->in_host = NOWHERE;
}

/* returns the real number of the room with given virtual number */
long real_room(long virt)
{
  long bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) >> 1;

    if ((world + mid)->number == virt)
      return mid;
    if (bot >= top)
      return -1;
    if ((world + mid)->number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* returns the real number of the monster with given virtual number */
long real_mobile(long virt)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

long real_quest(long virt)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_questt;

  for (;;) {
    mid = (bot + top) >> 1;

    if ((quest_table + mid)->vnum == virt)
      return mid;
    if (bot >= top)
      return -1;
    if ((quest_table + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

long real_shop(long virt)
{
  int bot, top, mid;

  bot = 0;
  top = (top_of_shopt - 1);

  for (;;) {
    mid = (bot + top) / 2;

    if ((shop_table + mid)->vnum == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((shop_table + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

long real_zone(long virt)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_zone_table;

  /* perform binary search on zone-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((zone_table + mid)->number > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

long real_vehicle(long virt)
{
  int bot, top, mid;
  bot = 0;
  top = top_of_veht;
  for (;;) {
    mid = (bot + top) / 2;
    if ((veh_index + mid)->vnum == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((veh_index + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;

  }
}

long real_host(long virt)
{
  int bot, top, mid;
  bot = 0;
  top = top_of_matrix;
  for (;;) {
    mid = (bot + top) / 2;
    if ((matrix + mid)->vnum == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((matrix + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;

  }
}

long real_ic(long virt)
{
  int bot, top, mid;
  bot = 0;
  top = top_of_ic;
  for (;;) {
    mid = (bot + top) / 2;
    if ((ic_index + mid)->vnum == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((ic_index + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;

  }
}

/* returns the real number of the object with given virtual number */
long real_object(long virt)
{
  long bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == virt)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

char *short_object(int virt, int what)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == virt)
      /* 1-namelist, 2-shortdescription */
      if (what == 1)
        return (obj_proto[mid].text.keywords);
      else if (what == 2)
        return (obj_proto[mid].text.name);
    if (bot >= top)
      return (NULL);
    if ((obj_index + mid)->vnum > virt)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* remove ^M's from file output */
/* There may be a similar function in Oasis (and I'm sure
   it's part of obuild).  Remove this if you get a
   multiple definition error or if it you want to use a
   substitute
*/
void kill_ems(char *str)
{
  char *ptr1, *ptr2, *tmp;

  tmp = str;
  ptr1 = str;
  ptr2 = str;

  while(*ptr1) {
    if((*(ptr2++) = *(ptr1++)) == '\r')
      if(*ptr1 == '\r')
        ptr1++;
  }
  *ptr2 = '\0';
}

void load_saved_veh()
{
  FILE *fl;
  struct veh_data *veh = NULL;
  long vnum, owner;
  struct obj_data *obj, *last_obj = NULL;

  if (!(fl = fopen("veh/vfile", "r"))) {
    log("SYSERR: Could not open vfile for reading.");
    return;
  }
  if (!get_line(fl, buf)) {
    log("SYSERR: Invalid Entry In Vfile.");
    return;
  }
  fclose(fl);
  int num_veh = atoi(buf);
  for (int i = 0; i < num_veh; i++) {
    File file;
    sprintf(buf, "veh/%07d", i);
    if (!(file.Open(buf, "r")))
      continue;

    VTable data;
    data.Parse(&file);
    file.Close();

    owner = data.GetLong("VEHICLE/Owner", 0);
    if (!playerDB.DoesExist(owner))
      continue;

    if ((vnum = data.GetLong("VEHICLE/Vnum", 0)))
      veh = read_vehicle(vnum, VIRTUAL);
    else
      continue;
    veh->damage = data.GetInt("VEHICLE/Damage", 0);
    veh->owner = owner;
    veh->locked = TRUE;
    veh->sub = data.GetLong("VEHICLE/Subscribed", 0);
    veh_to_room(veh, real_room(data.GetLong("VEHICLE/InRoom", 0)));

    int inside = 0, last_in = 0;
    int num_objs = data.NumSubsections("CONTENTS");
    struct phone_data *k, *j;
    for (int i = 0; i < num_objs; i++) {
      const char *sect_name = data.GetIndexSection("CONTENTS", i);
      sprintf(buf, "%s/Vnum", sect_name);
      vnum = data.GetLong(buf, 0);
      if (vnum > 0 && (obj = read_object(vnum, VIRTUAL))) {
        sprintf(buf, "%s/Name", sect_name);
        obj->restring = str_dup(data.GetString(buf, NULL));
        sprintf(buf, "%s/Photo", sect_name);
        obj->photo = str_dup(data.GetString(buf, NULL));
        for (int x = 0; x < 10; x++) {
          sprintf(buf, "%s/Value %d", sect_name, x);
          GET_OBJ_VAL(obj, x) = data.GetInt(buf, GET_OBJ_VAL(obj, x));
        }
        if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2)) {
          bool found = FALSE;
          sprintf(buf, "%d%d", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
          for (j = phone_list; j; j = j->next)
            if (j->number == atoi(buf)) {
              found = TRUE;
              break;
            }
          if (!found) {
            k = new phone_data;
            k->number = atoi(buf);
            k->phone = obj;
            if (phone_list)
              k->next = phone_list;
            phone_list = k;
          }
        }
        sprintf(buf, "%s/ExtraFlags", sect_name);
        GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/AffFlags", sect_name);
        obj->obj_flags.bitvector.FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/Condition", sect_name);
        GET_OBJ_CONDITION(obj) = data.GetInt(buf, GET_OBJ_CONDITION(obj));
        for (int c = 0; c < MAX_OBJ_AFFECT; c++) {
          sprintf(buf, "%s/Affect%dLoc", sect_name, c);
          obj->affected[c].location = data.GetInt(buf, 0);
          sprintf(buf, "%s/Affect%dmod", sect_name, c);
          obj->affected[c].modifier = data.GetInt(buf, 0);
        }
        sprintf(buf, "%s/Inside", sect_name);
        inside = data.GetInt(buf, 0);
        if (inside > 0) {
          if (inside == last_in)
            last_obj = last_obj->in_obj;
          else if (inside < last_in)
            while (inside <= last_in) {
              last_obj = last_obj->in_obj;
              last_in--;
            }
          obj_to_obj(obj, last_obj);
        } else
          obj_to_veh(obj, veh);
        last_in = inside;
        last_obj = obj;
      }
    }
    int num_mods = data.NumFields("MODS");
    for (int i = 0; i < num_mods; i++) {
      sprintf(buf, "MODS/Mod%d", i);
      obj = read_object(data.GetLong(buf, 0), VIRTUAL);
      GET_MOD(veh, GET_OBJ_VAL(obj, 0)) = obj;
      veh->load -= GET_OBJ_VAL(obj, 1);
      for (int l = 0; l < MAX_OBJ_AFFECT; l++)
        affect_veh(veh, obj->affected[l].location, obj->affected[l].modifier);
    }
    num_mods = data.NumSubsections("GRIDGUIDE");
    for (int i = 0; i < num_mods; i++) {
      struct grid_data *grid = new grid_data;
      const char *sect_name = data.GetIndexSection("GRIDGUIDE", i);
      sprintf(buf, "%s/Name", sect_name);
      grid->name = str_dup(data.GetString(buf, NULL));
      sprintf(buf, "%s/Room", sect_name);
      grid->room = data.GetLong(buf, 0);
      grid->next = veh->grid;
      veh->grid = grid;
    }
    num_mods = data.NumSubsections("MOUNTS");
    for (int i = 0; i < num_mods; i++) {
      const char *sect_name = data.GetIndexSection("MOUNTS", i);
      sprintf(buf, "%s/MountNum", sect_name);
      obj = read_object(data.GetLong(buf, 0), VIRTUAL);
      sprintf(buf, "%s/Ammo", sect_name);
      GET_OBJ_VAL(obj, 9) = data.GetInt(buf, 0);
      sprintf(buf, "%s/Vnum", sect_name);
      int gun = data.GetLong(buf, 0);
      if (gun) {
        struct obj_data *weapon = read_object(gun, VIRTUAL);
        sprintf(buf, "%s/ExtraFlags", sect_name);
        GET_OBJ_EXTRA(weapon).FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/AffFlags", sect_name);
        weapon->obj_flags.bitvector.FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/Condition", sect_name);
        GET_OBJ_CONDITION(weapon) = data.GetInt(buf, GET_OBJ_CONDITION(weapon));
        for (int c = 0; c < MAX_OBJ_AFFECT; c++) {
          sprintf(buf, "%s/Affect%dLoc", sect_name, c);
          weapon->affected[c].location = data.GetInt(buf, 0);
          sprintf(buf, "%s/Affect%dmod", sect_name, c);
          weapon->affected[c].modifier = data.GetInt(buf, 0);
        }
        weapon->restring = str_dup(data.GetString(buf, NULL));
        for (int x = 0; x < 10; x++) {
          sprintf(buf, "%s/Value %d", sect_name, x);
          GET_OBJ_VAL(weapon, x) = data.GetInt(buf, GET_OBJ_VAL(weapon, x));
        }
        obj_to_obj(weapon, obj);
      }
      int subbed = 0, damage = 0;
      switch (GET_OBJ_VAL(obj, 1)) {
      case 1:
        subbed = 1;
      case 0:
        damage = 10;
        break;
      case 3:
        subbed  = 1;
      case 2:
        damage = 10;
        break;
      case 4:
        subbed  = 1;
        damage = 100;
        break;
      case 5:
        subbed  = 1;
        damage = 25;
        break;
      }
      veh->load -= damage;
      veh->sig -= subbed;
      if (veh->mount)
        obj->next_content = veh->mount;
      veh->mount = obj;
    }
  }
}

void load_consist(void)
{
  File file, fl;
  if (!(file.Open("etc/consist", "r"))) {
    log("CONSISTENCY FILE NOT FOUND");
    return;
  }

  VTable data;
  data.Parse(&file);
  file.Close();
  market[0] = data.GetInt("MARKET/Blue", 5000);
  market[1] = data.GetInt("MARKET/Green", 5000);
  market[2] = data.GetInt("MARKET/Orange", 5000);
  market[3] = data.GetInt("MARKET/Red", 5000);
  market[4] = data.GetInt("MARKET/Black", 5000);
  for (int nr = 0; nr < top_of_world; nr++)
    if (ROOM_FLAGGED(nr, ROOM_STORAGE)) {
      sprintf(buf, "storage/%ld", world[nr].number);
      if (!(fl.Open(buf, "r")))
        continue;
      VTable data;
      data.Parse(&fl);
      struct obj_data *obj = NULL, *last_obj = NULL;
      struct phone_data *k, *j;
      int inside = 0, last_in = 0, num_objs = data.NumSubsections("HOUSE");
      ;
      long vnum;
      for (int i = 0; i < num_objs; i++) {
        const char *sect_name = data.GetIndexSection("HOUSE", i);
        sprintf(buf, "%s/Vnum", sect_name);
        vnum = data.GetLong(buf, 0);
        if (vnum > 0 && (obj = read_object(vnum, VIRTUAL))) {
          sprintf(buf, "%s/Name", sect_name);
          obj->restring = str_dup(data.GetString(buf, NULL));
          sprintf(buf, "%s/Photo", sect_name);
          obj->photo = str_dup(data.GetString(buf, NULL));
          for (int x = 0; x < 10; x++) {
            sprintf(buf, "%s/Value %d", sect_name, x);
            GET_OBJ_VAL(obj, x) = data.GetInt(buf, GET_OBJ_VAL(obj, x));
          }
          if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2)) {
            bool found = FALSE;
            sprintf(buf, "%d%d", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
            for (j = phone_list; j; j = j->next)
              if (j->number == atoi(buf)) {
                found = TRUE;
                break;
              }
            if (!found) {
              k = new phone_data;
              k->number = atoi(buf);
              k->phone = obj;
              if (phone_list)
                k->next = phone_list;
              phone_list = k;
            }
          }
          sprintf(buf, "%s/ExtraFlags", sect_name);
          GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
          sprintf(buf, "%s/AffFlags", sect_name);
          obj->obj_flags.bitvector.FromString(data.GetString(buf, "0"));
          sprintf(buf, "%s/Condition", sect_name);
          GET_OBJ_CONDITION(obj) = data.GetInt(buf, GET_OBJ_CONDITION(obj));
          sprintf(buf, "%s/Timer", sect_name);
          GET_OBJ_TIMER(obj) = data.GetInt(buf, GET_OBJ_TIMER(obj));
          for (int c = 0; c < MAX_OBJ_AFFECT; c++) {
            sprintf(buf, "%s/Affect%dLoc", sect_name, c);
            obj->affected[c].location = data.GetInt(buf, 0);
            sprintf(buf, "%s/Affect%dmod", sect_name, c);
            obj->affected[c].modifier = data.GetInt(buf, 0);
          }
          sprintf(buf, "%s/Inside", sect_name);
          inside = data.GetInt(buf, 0);
          if (inside > 0) {
            if (inside == last_in)
              last_obj = last_obj->in_obj;
            else if (inside < last_in)
              while (inside <= last_in) {
                last_obj = last_obj->in_obj;
                last_in--;
              }
            obj_to_obj(obj, last_obj);
          } else
            obj_to_room(obj, nr);

          last_in = inside;
          last_obj = obj;
        }
      }



    }
}


// file: newdb.cpp
// author: Andrew Hynek
// contents: implementation of the index classes


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "structs.h"
#include "newdb.h"
#include "db.h"
#include "comm.h"       // for shutdown()
#include "file.h"
#include "utils.h"
#include "memory.h"
#include "handler.h"
#include "vtable.h"
#include "constants.h"
#include "interpreter.h" // for alias
#include "class.h" // for pc_race_types and allergy_names

extern void kill_ems(char *);
static const char *const INDEX_FILENAME = "etc/pfiles/index";
extern char *cleanup(char *dest, const char *src);

// ____________________________________________________________________________
//
// global variables
// ____________________________________________________________________________

PCIndex playerDB;

// ____________________________________________________________________________
//
// PCIndex -- the PC database class: implementation
// ____________________________________________________________________________

int entry_compare(const void *one, const void *two)
{
  PCIndex::entry *ptr1 = (PCIndex::entry *)one;
  PCIndex::entry *ptr2 = (PCIndex::entry *)two;

  if (ptr1->id < ptr2->id)
    return -1;
  else if (ptr1->id > ptr2->id)
    return 1;

  return 0;
}

static void init_char(struct char_data * ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
  {
    ch->player_specials = new player_special_data;
  }

  ch->player_specials->saved.freeze_level = 0;
  ch->player_specials->saved.invis_level  = 0;
  ch->player_specials->saved.incog_level  = 0;
  ch->player_specials->saved.wimp_level   = 0;
  ch->player_specials->saved.bad_pws      = 0;

  set_title(ch, NULL);

  GET_LOADROOM(ch) = NOWHERE;
  GET_WAS_IN(ch) = NOWHERE;

  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  ch->player.weight = (int)gen_size(GET_RACE(ch), 0, 3, GET_SEX(ch));
  ch->player.height = (int)gen_size(GET_RACE(ch), 1, 3, GET_SEX(ch));

  ch->points.max_mental = 1000;
  ch->points.max_physical = 1000;
  ch->points.mental = GET_MAX_MENTAL(ch);
  ch->points.physical = GET_MAX_PHYSICAL(ch);
  GET_BALLISTIC(ch) = GET_TOTALBAL(ch) = GET_IMPACT(ch) = GET_TOTALIMP(ch) = 0;
  ch->points.sustained = 0;
  ch->points.grade = 0;

  if (access_level(ch, LVL_VICEPRES))
  {
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  } else
  {
    GET_COND(ch,FULL) = 24;
    GET_COND(ch, THIRST) = 24;
    GET_COND(ch, DRUNK) = 0;
  }

  if (!access_level(ch, LVL_VICEPRES))
    for (i = 1; i <= MAX_SKILLS; i++)
    {
      SET_SKILL(ch, i, 0)
    }
  else
    for (i = 1; i <= MAX_SKILLS; i++)
    {
      SET_SKILL(ch, i, 100);
    }

  ch->char_specials.saved.affected_by.Clear();
}

static void init_char_strings(char_data *ch)
{
  // crap...race isn't set when this is called
  //  const char *race = pc_race_types[GET_RACE(ch)];

  if (ch->player.physical_text.keywords)
    delete [] ch->player.physical_text.keywords;

  size_t len = strlen(GET_CHAR_NAME(ch)) + 1; // + strlen(race) + 2;
  ch->player.physical_text.keywords = new char[len];

  strcpy(ch->player.physical_text.keywords, GET_CHAR_NAME(ch));
  *(ch->player.physical_text.keywords) =
    LOWER(*ch->player.physical_text.keywords);

  if (ch->player.physical_text.name)
    delete [] ch->player.physical_text.name;

  if (ch->player.physical_text.room_desc)
    delete [] ch->player.physical_text.room_desc;

  {
    char temp[256];

    sprintf(temp, "A %s %s", genders[(int)GET_SEX(ch)], pc_race_types[(int)GET_RACE(ch)]);
    ch->player.physical_text.name = str_dup(temp);

    sprintf(temp, "A %s %s voice.", genders[(int)GET_SEX(ch)], pc_race_types[(int)GET_RACE(ch)]);
    ch->player.physical_text.room_desc = str_dup(temp);

    if (ch->player.physical_text.look_desc)
      delete [] ch->player.physical_text.look_desc;

    sprintf(temp, "A fairly nondescript thing.\n");
    ch->player.physical_text.look_desc = str_dup(temp);
  }

  set_title(ch, "^y(Newbie)^n");
  set_pretitle(ch, NULL);
  set_whotitle(ch, " New ");

  switch(GET_RACE(ch)) {
  case RACE_HUMAN:
    set_whotitle(ch, "Human");
    break;
  case RACE_DWARF:
    set_whotitle(ch, "Dwarf");
    break;
  case RACE_ELF:
    set_whotitle(ch, "Elf");
    break;
  case RACE_ORK:
    set_whotitle(ch, "Ork");
    break;
  case RACE_TROLL:
    set_whotitle(ch, "Troll");
    break;
  case RACE_CYCLOPS:
    set_whotitle(ch, "Cyclops");
    break;
  case RACE_KOBOROKURU:
    set_whotitle(ch, "Koborokuru");
    break;
  case RACE_FOMORI:
    set_whotitle(ch, "Fomori");
    break;
  case RACE_MENEHUNE:
    set_whotitle(ch, "Menehune");
    break;
  case RACE_HOBGOBLIN:
    set_whotitle(ch, "Hobgoblin");
    break;
  case RACE_GIANT:
    set_whotitle(ch, "Giant");
    break;
  case RACE_GNOME:
    set_whotitle(ch, "Gnome");
    break;
  case RACE_ONI:
    set_whotitle(ch, "Oni");
    break;
  case RACE_WAKYAMBI:
    set_whotitle(ch, "Wakyambi");
    break;
  case RACE_OGRE:
    set_whotitle(ch, "Ogre");
    break;
  case RACE_MINOTAUR:
    set_whotitle(ch, " Minotaur");
    break;
  case RACE_SATYR:
    set_whotitle(ch, "Satyr");
    break;
  case RACE_NIGHTONE:
    set_whotitle(ch, "Night-One");
    break;
  case RACE_DRYAD:
    set_whotitle(ch, "Dryad");
    break;
  case RACE_DRAGON:
    set_whotitle(ch, "Dragon");
    break;
  default:
    mudlog("No race found at set_whotitle in class.cc", NULL, LOG_SYSLOG, TRUE);
    set_whotitle(ch, "CHKLG"); /* Will set incase the players */
  }        /* race is undeterminable      */

  GET_PROMPT(ch) = str_dup("< @pP @mM > ");
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  void advance_level(struct char_data * ch);

  GET_LEVEL(ch) = 1;
  GET_KARMA(ch) = 0;
  GET_REP(ch) = 0;
  GET_NOT(ch) = 0;
  GET_TKE(ch) = 0;

  ch->points.max_physical = 1000;
  ch->points.max_mental = 1000;

  ch->char_specials.saved.left_handed = (!number(0, 9) ? 1 : 0);
  GET_WIELDED(ch, 0) = 0;
  GET_WIELDED(ch, 1) = 0;

  advance_level(ch);

  GET_PHYSICAL(ch) = GET_MAX_PHYSICAL(ch);
  GET_MENTAL(ch) = GET_MAX_MENTAL(ch);
  //GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;
  GET_LOADROOM(ch) = 8039;

  PLR_FLAGS(ch).SetBit(PLR_NEWBIE);
  PRF_FLAGS(ch).SetBit(PRF_AUTOEXIT);
  PLR_FLAGS(ch).SetBit(PLR_AUTH);

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  //  GET_NUYEN(ch) = 0;
}

/* Gain maximum in various points */
void advance_level(struct char_data * ch)
{
  int i;

  if (IS_SENATOR(ch))
  {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    GET_KARMA(ch) = 0;
    GET_REP(ch) = 0;
    GET_TKE(ch) = 0;
    GET_NOT(ch) = 0;
    if (PRF_FLAGGED(ch, PRF_PKER))
      PRF_FLAGS(ch).RemoveBit(PRF_PKER);
    PRF_FLAGS(ch).SetBits(PRF_HOLYLIGHT, PRF_ROOMFLAGS,
                          PRF_NOHASSLE, ENDBIT);
  }

  playerDB.SaveChar(ch);
  playerDB.Update(ch);

  sprintf(buf, "%s advanced to %s.",
          GET_CHAR_NAME(ch), status_ratings[(int)GET_LEVEL(ch)]);
  mudlog(buf, ch, LOG_MISCLOG, TRUE);
}

static bool load_char(const char *name, char_data *ch, bool logon)
{
  File fl;
  int i;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH], bits[MAX_STRING_LENGTH];

  for (i = 0;(*(bits + i) = LOWER(*(name + i))); i++)
    ;

  sprintf(filename, "%s%s%c%s%s%s", PLR_PREFIX, SLASH, *bits,
          SLASH, bits, PLR_SUFFIX);

  fl.Open(filename, "r");

  if (!fl.IsOpen()) {
    sprintf(buf, "SYSERR: Couldn't open player file %s", filename);
    mudlog(buf, NULL, LOG_SYSLOG, TRUE);
    return false;
  }

  VTable data;
  data.Parse(&fl);

  fl.Close();

  /* character initializations */
  /* initializations necessary to keep some things straight */
  init_char(ch);

  ch->affected = NULL;
  for(i = 1; i <= MAX_SKILLS; i++)
    GET_SKILL(ch, i) = 0;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  GET_BALLISTIC(ch) = GET_TOTALBAL(ch) = GET_IMPACT(ch) = GET_TOTALIMP(ch) = 0;
  ch->points.init_dice = 0;
  ch->points.init_roll = 0;
  ch->points.sustained = 0;

  GET_LAST_TELL(ch) = NOBODY;

  if (!data.GetString("Character", NULL)) {
    sprintf(buf, "Error: no character name in %s", filename);
    mudlog(buf, NULL, LOG_SYSLOG, true);
    return false;
  } else if (!data.GetString("Password", NULL)) {
    sprintf(buf, "Error: no password in %s", filename);
    mudlog(buf, NULL, LOG_SYSLOG, true);
    return false;
  }

  ch->player.char_name = str_dup(data.GetString("Character", NULL));

  memset(GET_PASSWD(ch), 0, MAX_PWD_LENGTH);
  strncpy(GET_PASSWD(ch), data.GetString("Password", NULL), MAX_PWD_LENGTH);

  GET_RACE(ch) = data.LookupInt("Race", pc_race_types, RACE_HUMAN);
  GET_SEX(ch) = data.LookupInt("Gender", genders, SEX_NEUTRAL);

  GET_LEVEL(ch) = data.LookupInt("Rank", status_ratings, 0);

  AFF_FLAGS(ch).FromString(data.GetString("AffFlags", "0"));
  PLR_FLAGS(ch).FromString(data.GetString("PlrFlags", "0"));
  PRF_FLAGS(ch).FromString(data.GetString("PrfFlags", "0"));

  { // player text
    text_data *text_tab[3] = {
                               &ch->player.physical_text,
                               &ch->player.astral_text,
                               &ch->player.matrix_text
                             };

    const char *section_tab[3] = {
                                   "PHYSICAL TEXT",
                                   "ASTRAL TEXT",
                                   "MATRIX TEXT"
                                 };

    for (int i = 0; i < 3; i++) {
      if (data.DoesSectionExist(section_tab[i])) {
        text_data *ptr = text_tab[i];
        char field[128], defawlt[128];

        sprintf(field, "%s/Keywords", section_tab[i]);
        ptr->keywords = str_dup(data.GetString(field, GET_CHAR_NAME(ch)));

        sprintf(field, "%s/Name", section_tab[i]);
        ptr->name = str_dup(data.GetString(field, GET_CHAR_NAME(ch)));

        sprintf(defawlt, "An internal error occurred, but %s is here.\n",
                GET_CHAR_NAME(ch));

        sprintf(field, "%s/RoomDesc", section_tab[i]);
        ptr->room_desc = str_dup(data.GetString(field, defawlt));

        sprintf(field, "%s/LookDesc", section_tab[i]);
        ptr->look_desc = str_dup(data.GetString(field, defawlt));
      }
    }
  }

  { // miscellaneous text
    POOFIN(ch)  = str_dup(data.GetString("MISC TEXT/PoofIn", NULL));
    POOFOUT(ch) = str_dup(data.GetString("MISC TEXT/PoofOut", NULL));

    GET_TITLE(ch) = str_dup(data.GetString("MISC TEXT/Title", NULL));
    GET_PRETITLE(ch) = str_dup(data.GetString("MISC TEXT/Pretitle", NULL));
    GET_WHOTITLE(ch) = str_dup(data.GetString("MISC TEXT/Whotitle", NULL));

    GET_PROMPT(ch) = str_dup(data.GetString("MISC TEXT/Prompt", NULL));
    ch->player.host = str_dup(data.GetString("MISC TEXT/Host", NULL));
  }

  { // attributes
    GET_REAL_BOD(ch) = data.GetInt("ATTRIBUTES/Bod", 1);
    GET_REAL_QUI(ch) = data.GetInt("ATTRIBUTES/Qui", 1);
    GET_REAL_STR(ch) = data.GetInt("ATTRIBUTES/Str", 1);
    GET_REAL_CHA(ch) = data.GetInt("ATTRIBUTES/Cha", 1);
    GET_REAL_INT(ch) = data.GetInt("ATTRIBUTES/Int", 1);
    GET_REAL_WIL(ch) = data.GetInt("ATTRIBUTES/Wil", 1);
    ch->real_abils.mag = data.GetInt("ATTRIBUTES/Mag", 0);
    ch->real_abils.rmag = data.GetInt("ATTRIBUTES/RealMag", 0);
    GET_PP(ch) = data.GetInt("ATTRIBUTES/PowerPoints", ch->real_abils.rmag);
    ch->real_abils.ess = data.GetInt("ATTRIBUTES/Essence", 600);
    ch->real_abils.bod_index = data.GetInt("ATTRIBUTES/BodIndex",
                                           GET_REAL_BOD(ch) * 100);
    GET_REAL_REA(ch) = (GET_REAL_QUI(ch) + GET_REAL_INT(ch)) / 2;
  }

  { // pools
    ch->real_abils.astral_pool = data.GetInt("POOLS/ASTRAL/Total", 0);

    ch->real_abils.combat_pool = data.GetInt("POOLS/COMBAT/Total", 0);
    ch->real_abils.defense_pool = data.GetInt("POOLS/COMBAT/Defense", 0);
    ch->real_abils.offense_pool = data.GetInt("POOLS/COMBAT/Offense", 0);

    ch->real_abils.control_pool = data.GetInt("POOLS/CONTROL/Total", 0);

    ch->real_abils.hacking_pool = data.GetInt("POOLS/HACKING/Total", 0);

    ch->real_abils.magic_pool = data.GetInt("POOLS/MAGIC/Total", 0);
  }

  ch->aff_abils = ch->real_abils;

  { // points
    GET_HEIGHT(ch) = data.GetInt("POINTS/Height", 150);
    GET_WEIGHT(ch) = data.GetInt("POINTS/Weight", 70);

    GET_TRADITION(ch) = data.GetInt("POINTS/Tradition", TRAD_MUNDANE);

    GET_TOTEM(ch) = data.GetInt("POINTS/Totem", TOTEM_UNDEFINED);

    GET_NUYEN(ch) = data.GetInt("POINTS/Cash", 0);
    GET_BANK(ch) = data.GetInt("POINTS/Bank", 0);

    GET_KARMA(ch) = data.GetInt("POINTS/Karma", 0);
    GET_REP(ch) = data.GetInt("POINTS/Rep", 0);
    GET_TKE(ch) = data.GetInt("POINTS/TKE", 0);
    GET_NOT(ch) = data.GetInt("Points/Notoriety", 0);

    GET_GRADE(ch) = data.GetInt("POINTS/Grade", 0);
    ch->points.extrapp = data.GetInt("POINTS/ExtraPower", 0);
    GET_MAX_PHYSICAL(ch) = data.GetInt("POINTS/MaxPhys", 1000);
    GET_PHYSICAL(ch) = data.GetInt("POINTS/Physical", 1000);
    GET_MAX_MENTAL(ch) = data.GetInt("POINTS/MaxMent", 1000);
    GET_MENTAL(ch) = data.GetInt("POINTS/Mental", 1000);

    GET_ATT_POINTS(ch) = data.GetInt("POINTS/AttPoints", 0);
    GET_SKILL_POINTS(ch) = data.GetInt("POINTS/SkillPoints", 0);

    ch->player_specials->saved.zonenum = data.GetInt("POINTS/ZoneNumber", 0);
    ch->player_specials->saved.wimp_level = data.GetInt("POINTS/WimpLevel", 0);
    ch->player_specials->saved.freeze_level = data.GetInt("POINTS/FreezeLevel", 0);
    ch->player_specials->saved.invis_level = data.GetInt("POINTS/InvisLevel", 0);
    ch->player_specials->saved.incog_level = data.GetInt("POINTS/IncogLevel", 0);
    ch->player_specials->saved.bad_pws = data.GetInt("POINTS/BadPWs", 0);
    ch->player_specials->saved.load_room =
      data.GetLong("POINTS/LoadRoom", NOWHERE);
    ch->player_specials->saved.last_in =
      data.GetLong("POINTS/LastRoom", NOWHERE);
    GET_LASTROOM(ch) = data.GetLong("POINTS/Home", NOWHERE);

    ch->char_specials.saved.left_handed = data.GetInt("POINTS/LeftHanded", 0);
    GET_LANGUAGE(ch) = data.GetInt("POINTS/CurLang", 0);

    if (GET_LANGUAGE(ch) == 0) {
      GET_SKILL(ch, SKILL_ENGLISH) = 10;
      GET_LANGUAGE(ch) = SKILL_ENGLISH;
    }

    ch->player.time.logon = data.GetLong("POINTS/Last", time(NULL));
    ch->player.time.birth = data.GetLong("POINTS/Born", time(NULL));
    ch->player.time.played = data.GetInt("POINTS/Played", 0);
  }

  { // conditions
    GET_COND(ch, FULL) = data.GetInt("CONDITIONS/Hunger", 0);
    GET_COND(ch, THIRST) = data.GetInt("CONDITIONS/Thirst", 0);
    GET_COND(ch, DRUNK) = data.GetInt("CONDITIONS/Drunk", 0);
  }

  { // skills
    const int num_skills = data.NumFields("SKILLS");

    for (int j = 0; j < num_skills; j++) {
      const char *skill_name = data.GetIndexField("SKILLS", j);
      int idx;

      for (idx = 0; idx <= MAX_SKILLS; idx++)
        if (!str_cmp(skills[idx].name, skill_name))
          break;

      if (idx > 0 && idx <= MAX_SKILLS)
        GET_SKILL(ch, idx) = data.GetIndexInt("SKILLS", j, 0);
    }
  }
  {
    const int num_skills = data.NumFields("POWERS");

    for (int j = 0; j < num_skills; j++) {
      const char *skill_name = data.GetIndexField("POWERS", j);
      int idx;
      for (idx = 1; idx <= ADEPT_NUMPOWER; idx++)
        if (!str_cmp(adept_powers[idx], skill_name))
          break;
      GET_POWER(ch, idx) = data.GetIndexInt("POWERS", j, 0);
    }
  }
  { // spells
    const int num_spells = data.NumSubsections("SPELLS");

    for (int j = 0; j < num_spells; j++) {
      const char *sect_name = data.GetIndexSection("SPELLS", j);
      const char *name;
      char field[128];
      int idx;

      if (sscanf(sect_name, " SPELL %d ", &idx) != 1) {
        log("%s's file had an invalid spell index (spell #%d); skipping", j);
        continue;
      }

      sprintf(field, "%s/Name", sect_name);

      if ((name = data.GetString(field, NULL)) == NULL) {
        log("%s's file had a spell with no name (spell #%d); skipping", j);
        continue;
      }

      spell_t *spell = new spell_t;
      spell->name = new char[strlen(name) + 1];
      strcpy(spell->name, name);

      sprintf(field, "%s/Physical", sect_name);
      spell->type = idx;
      spell->physical = data.GetInt(field, 0);

      sprintf(field, "%s/Category", sect_name);
      spell->category = data.GetInt(field, 0);

      sprintf(field, "%s/Force", sect_name);
      spell->force = data.GetInt(field, 1);

      sprintf(field, "%s/Target", sect_name);
      spell->target = data.GetInt(field, 0);

      sprintf(field, "%s/Drain", sect_name);
      spell->drain = data.GetInt(field, 12);

      sprintf(field, "%s/Damage", sect_name);
      spell->damage = data.GetInt(field, 0);

      sprintf(field, "%s/Effect", sect_name);
      spell->effect = data.GetInt(field, 0);

      spell->next = ch->spells;
      ch->spells = spell;
    }
  }

  AFF_FLAGS(ch).RemoveBits(AFF_MANNING, AFF_RIG, AFF_PILOT, AFF_DESIGN, AFF_PROGRAM, ENDBIT);
  PLR_FLAGS(ch).RemoveBits(PLR_REMOTE, PLR_SWITCHED, PLR_MATRIX, PLR_PROJECT, PLR_PACKING, ENDBIT);

  { // aliases
    const int num_aliaii = data.NumFields("ALIASES");

    for (int j = 0; j < num_aliaii; j++) {
      alias *a = new alias;

      a->command = str_dup(data.GetIndexField("ALIASES", j));
      a->replacement = str_dup(data.GetIndexString("ALIASES", j, NULL));

      if (strchr(a->replacement, ALIAS_SEP_CHAR) ||
          strchr(a->replacement, ALIAS_VAR_CHAR))
        a->type = ALIAS_COMPLEX;
      else
        a->type = ALIAS_SIMPLE;

      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
    }
  }

  {
    const int num_mem = data.NumFields("MEMORY");

    for (int j = 0; j < num_mem; j++) {
      remem *a = new remem;

      a->mem = str_dup(data.GetIndexField("MEMORY", j));
      a->idnum = atol(data.GetIndexString("MEMORY", j, NULL));
      a->next = GET_MEMORY(ch);
      GET_MEMORY(ch) = a;
    }
  }

  {
    char field[128];
    for (int j = 0; j <= QUEST_TIMER - 1; j++) {
      sprintf(field, "QUESTS/Quest %d", j);
      GET_LQUEST(ch, j) = data.GetInt(field, 0);
    }
  }

  {
    struct obj_data *obj = NULL, *last_obj = NULL;
    struct phone_data *k;
    long vnum;
    int inside = 0, last_in = 0;
    int num_objs = data.NumSubsections("WORN");
    for (int i = 0; i < num_objs; i++) {
      const char *sect_name = data.GetIndexSection("WORN", i);
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
        if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2) && !logon) {
          sprintf(buf, "%04d%04d", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
          k = new phone_data;
          k->number = atoi(buf);
          k->phone = obj;
          k->rtg = GET_OBJ_VAL(obj, 8);
          if (phone_list)
            k->next = phone_list;
          phone_list = k;
        } else if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2))
          GET_OBJ_VAL(obj, 9) = 1;
        sprintf(buf, "%s/ExtraFlags", sect_name);
        GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/AffFlags", sect_name);
        obj->obj_flags.bitvector.FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/Condition", sect_name);
        GET_OBJ_CONDITION(obj) = data.GetInt(buf, GET_OBJ_CONDITION(obj));
        sprintf(buf, "%s/Cost", sect_name);
        GET_OBJ_COST(obj) = data.GetInt(buf, 1);
        sprintf(buf, "%s/Timer", sect_name);
        GET_OBJ_TIMER(obj) = data.GetInt(buf, 0);
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

        } else {
          sprintf(buf, "%s/Worn", sect_name);
          equip_char(ch, obj, data.GetInt(buf, 0));
        }
        last_in = inside;
        last_obj = obj;
      }
    }
  }

  {
    struct obj_data *last_obj = NULL, *obj;
    struct phone_data *k;
    int vnum = 0;
    int inside = 0, last_in = 0;
    int num_objs = data.NumSubsections("INV");
    for (int j = 0; j < num_objs; j++) {
      const char *sect_name = data.GetIndexSection("INV", j);
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
        if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2) && !logon) {
          sprintf(buf, "%04d%04d", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
          k = new phone_data;
          k->number = atoi(buf);
          k->phone = obj;
          k->rtg = GET_OBJ_VAL(obj, 8);
          if (phone_list)
            k->next = phone_list;
          phone_list = k;
        } else if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2))
          GET_OBJ_VAL(obj, 9) = 1;
        sprintf(buf, "%s/ExtraFlags", sect_name);
        GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/AffFlags", sect_name);
        obj->obj_flags.bitvector.FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/Condition", sect_name);
        GET_OBJ_CONDITION(obj) = data.GetInt(buf, GET_OBJ_CONDITION(obj));
        sprintf(buf, "%s/Cost", sect_name);
        GET_OBJ_COST(obj) = data.GetInt(buf, 1);
        sprintf(buf, "%s/Timer", sect_name);
        GET_OBJ_TIMER(obj) = data.GetInt(buf, 0);
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
          obj_to_char(obj, ch);
        last_in = inside;
        last_obj = obj;
      }
    }
  }

  {
    struct obj_data *obj;
    int vnum = 0;
    int num_objs = data.NumSubsections("BIOWARE");
    for (int j = 0; j < num_objs; j++) {
      const char *sect_name = data.GetIndexSection("BIOWARE", j);
      sprintf(buf, "%s/Vnum", sect_name);
      vnum = data.GetLong(buf, 0);
      if (vnum > 0 && (obj = read_object(vnum, VIRTUAL))) {
        for (int x = 0; x < 10; x++) {
          sprintf(buf, "%s/Value %d", sect_name, x);
          GET_OBJ_VAL(obj, x) = data.GetInt(buf, GET_OBJ_VAL(obj, x));
        }
        sprintf(buf, "%s/ExtraFlags", sect_name);
        GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/Cost", sect_name);
        GET_OBJ_COST(obj) = data.GetInt(buf, 1);
        for (int c = 0; c < MAX_OBJ_AFFECT; c++) {
          sprintf(buf, "%s/Affect%dLoc", sect_name, c);
          obj->affected[c].location = data.GetInt(buf, 0);
          sprintf(buf, "%s/Affect%dmod", sect_name, c);
          obj->affected[c].modifier = data.GetInt(buf, 0);
        }
        obj_to_bioware(obj, ch);
      }
    }
  }

  {
    struct obj_data *obj;
    struct phone_data *k;
    int vnum = 0;
    int num_objs = data.NumSubsections("CYBERWARE");
    for (int j = 0; j < num_objs; j++) {
      const char *sect_name = data.GetIndexSection("CYBERWARE", j);
      sprintf(buf, "%s/Vnum", sect_name);
      vnum = data.GetLong(buf, 0);
      if (vnum > 0 && (obj = read_object(vnum, VIRTUAL))) {
        for (int x = 0; x < 10; x++) {
          sprintf(buf, "%s/Value %d", sect_name, x);
          GET_OBJ_VAL(obj, x) = data.GetInt(buf, GET_OBJ_VAL(obj, x));
        }
        if (GET_OBJ_VAL(obj, 2) == 4 && GET_OBJ_VAL(obj, 7) && !logon) {
          sprintf(buf, "%04d%04d", GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 6));
          k = new phone_data;
          k->number = atoi(buf);
          k->phone = obj;
          k->rtg = GET_OBJ_VAL(obj, 8);
          if (phone_list)
            k->next = phone_list;
          phone_list = k;
        } else if (GET_OBJ_VAL(obj, 2) == 4 && GET_OBJ_VAL(obj, 7))
          GET_OBJ_VAL(obj, 9) = 1;
        sprintf(buf, "%s/ExtraFlags", sect_name);
        GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
        sprintf(buf, "%s/Cost", sect_name);
        GET_OBJ_COST(obj) = data.GetInt(buf, 1);
        for (int c = 0; c < MAX_OBJ_AFFECT; c++) {
          sprintf(buf, "%s/Affect%dLoc", sect_name, c);
          obj->affected[c].location = data.GetInt(buf, 0);
          sprintf(buf, "%s/Affect%dmod", sect_name, c);
          obj->affected[c].modifier = data.GetInt(buf, 0);
        }
        obj_to_cyberware(obj, ch);
      }
    }
  }
  for (struct obj_data *jack = ch->cyberware; jack; jack = jack->next_content)
    if (!GET_OBJ_VAL(jack, 2) && GET_OBJ_VAL(jack, 3)) {
      for (struct obj_data *wire = ch->cyberware; wire; wire = wire->next_content)
        if (GET_OBJ_VAL(wire, 2) == 28) {
          struct obj_data *chip = read_object(GET_OBJ_VAL(jack, 3), VIRTUAL);
          ch->char_specials.saved.skills[GET_OBJ_VAL(chip, 0)][1] = MIN(GET_OBJ_VAL(wire, 0), GET_OBJ_VAL(chip, 1));
          break;
        }
      break;
    }

  affect_total(ch);

  if (!IS_AFFECTED(ch, AFF_POISON) &&
      (((long) (time(0) - ch->player.time.logon)) >= SECS_PER_REAL_HOUR)) {
    GET_PHYSICAL(ch) = GET_MAX_PHYSICAL(ch);
    GET_MENTAL(ch) = GET_MAX_MENTAL(ch);
  }
  if ( !IS_SENATOR(ch) )
    PRF_FLAGS(ch).RemoveBit(PRF_ROLLS);

  if (((long) (time(0) - ch->player.time.logon) >= SECS_PER_REAL_HOUR * 2) ||
      (GET_LAST_IN(ch) > 599 && GET_LAST_IN(ch) < 700)) {
    GET_LAST_IN(ch) = GET_LOADROOM(ch);
    GET_PHYSICAL(ch) = GET_MAX_PHYSICAL(ch);
    GET_MENTAL(ch) = GET_MAX_MENTAL(ch);
  }

  // initialization for imms
  if(IS_SENATOR(ch)) {
    GET_COND(ch, FULL) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }

  return true;
}

static bool save_char(char_data *player, DBIndex::vnum_t loadroom)
{
  FILE *fl;
  char outname[40], buf[MAX_STRING_LENGTH];
  char bits[128];
  int i;
  struct obj_data *char_eq[NUM_WEARS];
  int wield[2];
  struct obj_data *temp, *next_obj;
  struct affected_type *af;

  struct affected_type affected[MAX_AFFECT];

  if (IS_NPC(player))
    return false;

  for (i = 0;(*(bits + i) = LOWER(*(GET_CHAR_NAME(player) + i))); i++)
    ;
  sprintf(outname, "%s%s%c%s%s%s", PLR_PREFIX, SLASH, *bits, SLASH, bits, PLR_SUFFIX);

  if (!(fl = fopen(outname, "w"))) {
    sprintf(buf, "SYSERR: Couldn't open player file (%s) %s for write",
            outname, GET_CHAR_NAME(player));
    mudlog(buf, NULL, LOG_SYSLOG, TRUE);
    return false;
  }

  /**************************************************/
  /** Do In-Memory Copy **/
  wield[0] = GET_WIELDED(player, 0);
  wield[1] = GET_WIELDED(player, 1);

  /* Unaffect everything a character can be affected by */

  /* worn eq */
  for (i = 0; i < NUM_WEARS; i++) {
    if (player->equipment[i])
      char_eq[i] = unequip_char(player, i);
    else
      char_eq[i] = NULL;
  }

  GET_WIELDED(player, 0) = wield[0];
  GET_WIELDED(player, 1) = wield[1];

  /* cyberware */
  for (temp = player->cyberware; temp; temp = next_obj) {
    next_obj = temp->next_content;
    obj_from_cyberware(temp);
    obj_to_char(temp, player);
  }

  /* bioware */
  for (temp = player->bioware; temp; temp = next_obj) {
    next_obj = temp->next_content;
    obj_from_bioware(temp);
    obj_to_char(temp, player);
  }

  for (af = player->affected, i = 0; i < MAX_AFFECT; i++) {
    affected[i].type = 0; /* Zero signifies not used */
    affected[i].duration = 0;
    affected[i].modifier = 0;
    affected[i].location = 0;
    affected[i].bitvector = 0;
    affected[i].next = NULL;
  }
  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (player->affected)
    affect_remove(player, player->affected, 0);

  if ((i >= MAX_AFFECT) && af && af->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  /**************************************************/
  if (loadroom == NOWHERE)
    loadroom = GET_LOADROOM(player);

  if (player->in_room != NOWHERE)
    if ( world[player->in_room].number <= 1 ) {
      GET_LAST_IN(player) = world[player->was_in_room].number;
    } else {
      GET_LAST_IN(player) = world[player->in_room].number;
    }


  /***************************************/
  /* Code Here for the actual Save Washu */
  /***************************************/

  fprintf(fl, "Character:\t%s\n", player->player.char_name);
  fprintf(fl, "Password:\t%s\n", GET_PASSWD(player));
  fprintf(fl, "Race:\t%s\n", pc_race_types[(int)GET_RACE(player)]);
  fprintf(fl, "Gender:\t%s\n", genders[(int)GET_SEX(player)]);
  fprintf(fl, "Rank:\t%s\n", status_ratings[(int)GET_LEVEL(player)]);

  fprintf(fl,
          "AffFlags:\t%s\n"
          "PlrFlags:\t%s\n"
          "PrfFlags:\t%s\n",
          AFF_FLAGS(player).ToString(),
          PLR_FLAGS(player).ToString(),
          PRF_FLAGS(player).ToString());

  { // player text
    text_data *text_tab[3] = {
                               &player->player.physical_text,
                               &player->player.astral_text,
                               &player->player.matrix_text
                             };

    const char *section_tab[3] = {
                                   "PHYSICAL TEXT",
                                   "ASTRAL TEXT",
                                   "MATRIX TEXT"
                                 };

    for (int i = 0; i < 3; i++) {
      text_data *ptr = text_tab[i];

      fprintf(fl, "[%s]\n", section_tab[i]);

      fprintf(fl,
              "\tKeywords:\t%s\n"
              "\tName:\t%s\n",
              ptr->keywords? ptr->keywords : player->player.char_name,
              ptr->name? ptr->name : player->player.char_name);

      if (ptr->room_desc) {
        strcpy(buf, ptr->room_desc);
        kill_ems(buf);
        fprintf(fl, "\tRoomDesc:$\n%s~\n", buf);
      } else {
        char race[32];

        strcpy(race, pc_race_types[(int)GET_RACE(player)]);
        *race = tolower(*race);

        fprintf(fl, "\tRoomDesc:$\nA %s %s voice~\n",
                genders[(int)GET_SEX(player)], race);
      }

      if (ptr->look_desc) {
        strcpy(buf, ptr->look_desc);
        kill_ems(buf);
        fprintf(fl, "\tLookDesc:$\n%s~\n", buf);
      } else {
        char race[32];

        strcpy(race, pc_race_types[(int)GET_RACE(player)]);
        *race = tolower(*race);

        fprintf(fl, "\tLookDesc:$\nYou see an unimaginative %s.~\n", race);
      }
    }
  }

  { // miscellaneous text
    fprintf(fl, "[MISC TEXT]\n");

    if(POOFIN(player))
      fprintf(fl, "\tPoofIn:\t%s\n", POOFIN(player));

    if(POOFOUT(player))
      fprintf(fl, "\tPoofOut:\t%s\n", POOFOUT(player));

    if(GET_TITLE(player))
      fprintf(fl, "\tTitle:\t%s\n", GET_TITLE(player));

    if(GET_PRETITLE(player))
      fprintf(fl, "\tPretitle:\t%s\n", GET_PRETITLE(player));

    if(GET_WHOTITLE(player))
      fprintf(fl, "\tWhotitle:\t%s\n", GET_WHOTITLE(player));
    else
      fprintf(fl, "\tWhotitle:\t%s\n", pc_race_types[(int)GET_RACE(player)]);

    if (player->player.prompt)
      fprintf(fl, "\tPrompt:\t%s\n", GET_PROMPT(player));

    if (player->player.host)
      fprintf(fl, "\tHost:\t%s\n", player->player.host);
  }

  { // attributes
    fprintf(fl, "[ATTRIBUTES]\n");

    fprintf(fl,
            "\tBod:\t%d\n"
            "\tQui:\t%d\n"
            "\tStr:\t%d\n"
            "\tCha:\t%d\n"
            "\tInt:\t%d\n"
            "\tWil:\t%d\n"
            "\tMag:\t%d\n"
            "\tRealMag:\t%d\n"
            "\tEssence:\t%d\n"
            "\tBodIndex:\t%d\n"
            "\tPowerPoints:\t%d\n",
            GET_REAL_BOD(player), GET_REAL_QUI(player), GET_REAL_STR(player),
            GET_REAL_CHA(player), GET_REAL_INT(player), GET_REAL_WIL(player),
            GET_REAL_MAG(player), GET_MAX_MAG(player), GET_REAL_ESS(player),
            GET_INDEX(player), GET_PP(player));
  }

  { // pools
    fprintf(fl, "[POOLS]\n");

    fprintf(fl,
            "\t[ASTRAL]\n"
            "\t\tTotal:\t%d\n",
            GET_ASTRAL(player));

    fprintf(fl,
            "\t[COMBAT]\n"
            "\t\tTotal:\t%d\n"
            "\t\tDefense:\t%d\n"
            "\t\tOffense:\t%d\n",
            GET_COMBAT(player),
            GET_DEFENSE(player),
            GET_OFFENSE(player));

    fprintf(fl,
            "\t[CONTROL]\n"
            "\t\tTotal:\t%d\n",
            GET_CONTROL(player));

    fprintf(fl,
            "\t[HACKING]\n"
            "\t\tTotal:\t%d\n",
            GET_HACKING(player));

    fprintf(fl,
            "\t[MAGIC]\n"
            "\t\tTotal:\t%d\n",
            GET_MAGIC(player));
  }

  { // points
    fprintf(fl, "[POINTS]\n");

    fprintf(fl, "\tHeight:\t%d\n", GET_HEIGHT(player));
    fprintf(fl, "\tWeight:\t%d\n", GET_WEIGHT(player));

    if (player->player.tradition != TRAD_MUNDANE)
      fprintf(fl, "\tTradition:\t%d\n", GET_TRADITION(player));

    if (GET_TOTEM(player))
      fprintf(fl, "\tTotem:\t%d\n", GET_TOTEM(player));

    fprintf(fl, "\tCash:\t%d\n", GET_NUYEN(player));
    fprintf(fl, "\tBank:\t%d\n", GET_BANK(player));
    fprintf(fl, "\tKarma:\t%d\n", GET_KARMA(player));
    fprintf(fl, "\tRep:\t%d\n", GET_REP(player));
    fprintf(fl, "\tNotoriety:\t%d\n", GET_NOT(player));
    fprintf(fl, "\tTKE:\t%d\n", GET_TKE(player));
    fprintf(fl, "\tGrade:\t%d\n", GET_GRADE(player));
    fprintf(fl, "\tExtraPower:\t%d\n", player->points.extrapp);
    fprintf(fl, "\tPhysical:\t%d\n", GET_PHYSICAL(player));
    fprintf(fl, "\tMaxPhys:\t%d\n", GET_MAX_PHYSICAL(player));

    fprintf(fl, "\tMental:\t%d\n", GET_MENTAL(player));
    fprintf(fl, "\tMaxMent:\t%d\n", GET_MAX_MENTAL(player));

    if (GET_ATT_POINTS(player))
      fprintf(fl, "\tAttPoints:\t%d\n", GET_ATT_POINTS(player));

    if (GET_SKILL_POINTS(player))
      fprintf(fl, "\tSkillPoints:\t%d\n", GET_SKILL_POINTS(player));

    if (GET_LEVEL(player) >= LVL_BUILDER)
      fprintf(fl, "\tZoneNumber:\t%d\n", player->player_specials->saved.zonenum);

    fprintf(fl, "\tLeftHanded:\t%d\n", player->char_specials.saved.left_handed);

    fprintf(fl, "\tCurLang:\t%d\n", GET_LANGUAGE(player));

    fprintf(fl, "\tWimpLevel:\t%d\n", GET_WIMP_LEV(player));

    if (GET_FREEZE_LEV(player))
      fprintf(fl, "\tFreezeLevel:\t%d\n", GET_FREEZE_LEV(player));

    if (GET_INVIS_LEV(player))
      fprintf(fl, "\tInvisLevel:\t%d\n", GET_INVIS_LEV(player));

    if (GET_INCOG_LEV(player))
      fprintf(fl, "\tIncogLevel:\t%d\n", GET_INCOG_LEV(player));

    if (GET_BAD_PWS(player))
      fprintf(fl, "\tBadPWs:\t%d\n", GET_BAD_PWS(player));

    fprintf(fl, "\tLoadRoom:\t%ld\n", GET_LOADROOM(player));
    fprintf(fl, "\tLastRoom:\t%ld\n", GET_LAST_IN(player));
    fprintf(fl, "\tLast:\t%ld\n", time(NULL));
    fprintf(fl, "\tBorn:\t%ld\n", player->player.time.birth);
    fprintf(fl, "\tPlayed:\t%d\n", player->player.time.played);
  }

  { // conditions
    fprintf(fl,
            "[CONDITIONS]\n"
            "\tHunger:\t%d\n"
            "\tThirst:\t%d\n"
            "\tDrunk:\t%d\n",
            GET_COND(player, FULL),
            GET_COND(player, THIRST),
            GET_COND(player, DRUNK));
  }

  { // skills
    fprintf(fl, "[SKILLS]\n");

    for (int i = 0; i <= MAX_SKILLS; i++)
      if (GET_SKILL(player, i))
        fprintf(fl, "\t%s:\t%d\n",
                skills[i].name, player->char_specials.saved.skills[i][0]);
  }
  { // adept powers
    if (GET_TRADITION(player) == TRAD_ADEPT) {
      fprintf(fl, "[POWERS]\n");
      for (int i = 1; i < ADEPT_NUMPOWER; i++)
        if (GET_POWER(player, i))
          fprintf(fl, "\t%s:\t%d\n", adept_powers[i], GET_POWER(player, i));
    }
  }
  /* add spell and eq affections back in now */
  for (temp = player->carrying; temp; temp = next_obj) {
    next_obj = temp->next_content;
    if ((GET_OBJ_TYPE(temp) == ITEM_CYBERWARE)) {
      obj_from_char(temp);
      obj_to_cyberware(temp, player);
    }
    /* bioware next */
    if (GET_OBJ_TYPE(temp) == ITEM_BIOWARE) {
      obj_from_char(temp);
      obj_to_bioware(temp, player);
    }
  }

  for (i = 0; i < MAX_AFFECT; i++) {
    if (affected[i].type)
      affect_to_char(player, &affected[i]);
  }

  // then worn eq
  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(player, char_eq[i], i);
  }

  affect_total(player);

  { // spells
    if (player->spells) {
      fprintf(fl, "[SPELLS]\n");

      for (spell_t *temp = player->spells; temp; temp = temp->next) {
        fprintf(fl,
                "\t[SPELL %d]\n"
                "\t\tName:\t%s\n"
                "\t\tForce:\t%d\n"
                "\t\tDrain:\t%d\n"
                "\t\tCategory:\t%d\n"
                "\t\tDamage:\t%d\n"
                "\t\tPhysical:\t%d\n"
                "\t\tTarget:\t%d\n"
                "\t\tEffect:\t%d\n",
                temp->type, temp->name, temp->force, temp->drain,
                temp->category, temp->damage, temp->physical,
                temp->target, temp->effect);
      }
    }
  }

  { // aliases
    fprintf(fl, "[ALIASES]\n");

    for (struct alias *a = GET_ALIASES(player); a; a = a->next) {
      fprintf(fl, "\t%s:\t%s\n",
              a->command, a->replacement);
    }
  }

  fprintf(fl, "[MEMORY]\n");
  for (struct remem *b = GET_MEMORY(player); b; b = b->next)
    if (b->idnum)
      fprintf(fl, "\t%s:\t%ld\n", b->mem, b->idnum);

  fprintf(fl, "[QUESTS]\n");
  for (int i = 0; i <= QUEST_TIMER - 1; i++)
    fprintf(fl, "\tQuest %d:\t%d\n", i, GET_LQUEST(player, i));

  struct obj_data *obj = NULL, *last_obj = NULL;
  int o = 0, level = 0;
  fprintf(fl, "[WORN]\n");
  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(player, i)) && !IS_OBJ_STAT(obj, ITEM_NORENT))
      break;

  while (obj && i < NUM_WEARS) {
    if (!IS_OBJ_STAT(obj, ITEM_NORENT)) {
      fprintf(fl, "\t[WORN %d]\n", o);
      fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj));
      if (level)
        fprintf(fl, "\t\tInside:\t%d\n", level);
      fprintf(fl, "\t\tWorn:\t%d\n", i);
      if (GET_OBJ_TYPE(obj) == ITEM_PHONE)
        for (int x = 0; x < 9; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      else
        for (int x = 0; x < 10; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj).ToString());
      fprintf(fl, "\t\tAffFlags:\t%s\n", obj->obj_flags.bitvector.ToString());
      fprintf(fl, "\t\tCondition:\t%d\n", GET_OBJ_CONDITION(obj));
      fprintf(fl, "\t\tCost:\t%d\n", GET_OBJ_COST(obj));
      fprintf(fl, "\t\tTimer:\t%d\n", GET_OBJ_TIMER(obj));
      for (int c = 0; c < MAX_OBJ_AFFECT; c++)
        if (obj->affected[c].location != APPLY_NONE && obj->affected[c].modifier != 0)
          fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                  c, obj->affected[c].location, c, obj->affected[c].modifier);
      if (obj->restring)
        fprintf(fl, "\t\tName:\t%s\n", obj->restring);
      if (obj->photo)
        fprintf(fl, "\t\tPhoto:$\n%s~\n", cleanup(buf2, obj->photo));
      last_obj = GET_EQ(player, i);
      o++;
    }

    if (obj->contains && !IS_OBJ_STAT(obj, ITEM_NORENT)) {
      obj = obj->contains;
      level++;
      continue;
    } else if (!obj->next_content && obj->in_obj)
      while (obj && !obj->next_content && level >= 0) {
        obj = obj->in_obj;
        level--;
      }

    if (!obj || !obj->next_content)
      while (i < NUM_WEARS) {
        i++;
        if ((obj = GET_EQ(player, i)) && !IS_OBJ_STAT(obj, ITEM_NORENT)) {
          level = 0;
          break;
        }
      }
    else
      obj = obj->next_content;
  }

  level = 0;
  o = 0;
  fprintf(fl, "[INV]\n");
  for (obj = player->carrying; obj;) {
    if (!IS_OBJ_STAT(obj, ITEM_NORENT)) {
      fprintf(fl, "\t[Object %d]\n", o);
      o++;
      fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj));
      if (level)
        fprintf(fl, "\t\tInside:\t%d\n", level);
      if (GET_OBJ_TYPE(obj) == ITEM_PHONE)
        for (int x = 0; x < 9; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      else
        for (int x = 0; x < 10; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj).ToString());
      fprintf(fl, "\t\tAffFlags:\t%s\n", obj->obj_flags.bitvector.ToString());
      fprintf(fl, "\t\tCondition:\t%d\n", GET_OBJ_CONDITION(obj));
      fprintf(fl, "\t\tCost:\t%d\n", GET_OBJ_COST(obj));
      fprintf(fl, "\t\tTimer:\t%d\n", GET_OBJ_TIMER(obj));
      if (obj->restring)
        fprintf(fl, "\t\tName:\t%s\n", obj->restring);
      if (obj->photo)
        fprintf(fl, "\t\tPhoto:$\n%s\n~\n", cleanup(buf2, obj->photo));
      for (int c = 0; c < MAX_OBJ_AFFECT; c++)
        if (obj->affected[c].location != APPLY_NONE && obj->affected[c].modifier != 0)
          fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                  c, obj->affected[c].location, c, obj->affected[c].modifier);
    }

    if (obj->contains && !IS_OBJ_STAT(obj, ITEM_NORENT)) {
      obj = obj->contains;
      level++;
      continue;
    } else if (!obj->next_content && obj->in_obj)
      while (obj && !obj->next_content && level >= 0) {
        obj = obj->in_obj;
        level--;
      }

    if (obj)
      obj = obj->next_content;
  }

  o = 0;
  fprintf(fl, "[BIOWARE]\n");
  for (obj = player->bioware; obj; obj = obj->next_content) {
    if (!IS_OBJ_STAT(obj, ITEM_NORENT)) {
      fprintf(fl, "\t[BIOWARE %d]\n", o);
      o++;
      fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj));
      for (int x = 0; x < 10; x++)
        fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj).ToString());
      fprintf(fl, "\t\tCost:\t%d\n", GET_OBJ_COST(obj));
      for (int c = 0; c < MAX_OBJ_AFFECT; c++)
        if (obj->affected[c].location != APPLY_NONE && obj->affected[c].modifier != 0)
          fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                  c, obj->affected[c].location, c, obj->affected[c].modifier);
    }
  }

  o = 0;
  fprintf(fl, "[CYBERWARE]\n");
  for (obj = player->cyberware; obj; obj = obj->next_content) {
    if (!IS_OBJ_STAT(obj, ITEM_NORENT)) {
      fprintf(fl, "\t[CYBER %d]\n", o);
      o++;
      fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj));
      if (GET_OBJ_VAL(obj, 2) == 4) {
        fprintf(fl, "\t\tValue 3:\t%d\n", GET_OBJ_VAL(obj, 3));
        fprintf(fl, "\t\tValue 6:\t%d\n", GET_OBJ_VAL(obj, 6));
        fprintf(fl, "\t\tValue 7:\t%d\n", GET_OBJ_VAL(obj, 7));
        fprintf(fl, "\t\tValue 8:\t%d\n", GET_OBJ_VAL(obj, 8));
      } else
        for (int x = 0; x < 10; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj).ToString());
      fprintf(fl, "\t\tCost:\t%d\n", GET_OBJ_COST(obj));
      for (int c = 0; c < MAX_OBJ_AFFECT; c++)
        if (obj->affected[c].location != APPLY_NONE && obj->affected[c].modifier != 0)
          fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                  c, obj->affected[c].location, c, obj->affected[c].modifier);
    }
  }

  fclose(fl);

  return true;
}

PCIndex::PCIndex()
{
  tab = NULL;
  entry_cnt = entry_size = 0;
  needs_save = false;
}

PCIndex::~PCIndex()
{
  reset();
}

bool PCIndex::Save()
{
  if (!needs_save)
    return true;

  FILE *index = fopen(INDEX_FILENAME, "w");

  if (!tab) {
    if (entry_cnt > 0)
      log("--Error: no table when there should be?!  That's fucked up, man.");
    return false;
  }

  if (!index) {
    log("--Error: Could not open player index file %s\n",
        INDEX_FILENAME);
    return false;
  }

  fprintf(index,
          "* Player index file\n"
          "* Generated automatically by PCIndex::Save\n\n");

  for (int i = 0; i < entry_cnt; i++) {
    entry *ptr = tab+i;

    fprintf(index,
            "%ld %d %u %lu %s\n",
            ptr->id, ptr->level, ptr->flags, ptr->last, ptr->name);
  }

  fprintf(index, "END\n");
  fclose(index);

  needs_save = false;

  return true;
}

char_data *PCIndex::CreateChar(char_data *ch)
{
  if (entry_cnt >= entry_size)
    resize_table();

  entry info;

  if (strlen(GET_CHAR_NAME(ch)) >= MAX_NAME_LENGTH) {
    log("--Fatal error: Could not fit name into player index..\n"
        "             : Inrease MAX_NAME_LENGTH");
    shutdown();
  }

  // if this is the first character, make him president
  if (entry_cnt < 1) {
    log("%s promoted to president, by virtue of first-come, first-serve.",
        GET_CHAR_NAME(ch));

    for (int i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;

    GET_KARMA(ch) = 0;
    GET_LEVEL(ch) = LVL_PRESIDENT;
    GET_REP(ch) = 0;
    GET_NOT(ch) = 0;
    GET_TKE(ch) = 0;

    PLR_FLAGS(ch).RemoveBit(PLR_NEWBIE);
    PLR_FLAGS(ch).RemoveBit(PLR_AUTH);
    PRF_FLAGS(ch).SetBits(PRF_HOLYLIGHT, PRF_CONNLOG, PRF_ROOMFLAGS,
                          PRF_NOHASSLE, PRF_AUTOINVIS, PRF_AUTOEXIT, ENDBIT);
  } else {
    PLR_FLAGS(ch).SetBit(PLR_AUTH);
  }
  init_char(ch);
  init_char_strings(ch);

  strcpy(info.name, GET_CHAR_NAME(ch));
  info.id = find_open_id();
  info.level = GET_LEVEL(ch);
  info.flags = (PLR_FLAGGED(ch, PLR_NODELETE) ? NODELETE : 0);
  info.active_data = ch;
  info.instance_cnt = 1;

  // sync IDs
  GET_IDNUM(ch) = info.id;

  // update the logon time
  time(&info.last);

  // insert the new info in the correct place
  sorted_insert(&info);

  if (!needs_save)
    needs_save = true;

  Save();

  return ch;
}

char_data *PCIndex::LoadChar(const char *name, bool logon)
{
  rnum_t idx = get_idx_by_name(name);

  if (idx < 0 || idx >= entry_cnt)
    return NULL;

  entry *ptr = tab+idx;

  // load the character data
  char_data *ch = Mem->GetCh();

  ch->player_specials = new player_special_data;
  memset(ch->player_specials, 0, sizeof(player_special_data));

  load_char(name, ch, logon);

  // sync IDs
  GET_IDNUM(ch) = ptr->id;

  ptr->instance_cnt++;

  if (logon) {
    ptr->active_data = NULL;
    // set the active data thing
    // ptr->active_data = ch;

    // update the last logon time
    time(&ptr->last);
  }

  return ch;
}

bool PCIndex::SaveChar(char_data *ch, vnum_t loadroom)
{
  if (IS_NPC(ch))
    return false;

  bool ret = save_char(ch, loadroom);

  Update(ch);

  return ret;
}

bool PCIndex::StoreChar(char_data *ch, bool save)
{
  bool result = true;

  if (IS_NPC(ch))
    return false;

  if (PLR_FLAGGED(ch, PLR_DELETED)) {
    playerDB.DeleteChar(GET_IDNUM(ch));
    return true;
  }
  /*  if (save && !SaveChar(ch, GET_LOADROOM(ch))) {
      char msg[256];
      sprintf(msg, "Error: could not save PC %s before storing",
              GET_CHAR_NAME(ch));
      mudlog(msg, ch, LOG_SYSLOG, true);
   
      result = false;
    }
  */
  rnum_t idx = get_idx_by_name(GET_CHAR_NAME(ch));

  update_by_idx(idx, ch);

  entry *ptr = tab+idx;

  ptr->instance_cnt--;

  //if (!ptr->instance_cnt || ptr->instance_cnt < 0)
  ptr->active_data = NULL;

  if (ch->desc)
    free_editing_structs(ch->desc);

  extract_char(ch);

  return result;
}

void PCIndex::Update(char_data *ch)
{
  rnum_t idx = get_idx_by_name(GET_CHAR_NAME(ch));
  update_by_idx(idx, ch);
}

void PCIndex::reset()
{
  if (tab != NULL) {
    delete [] tab;
    tab = NULL;
  }

  entry_cnt = entry_size = 0;
  needs_save = false;
}

bool PCIndex::load()
{
  File index(INDEX_FILENAME, "r");

  if (!index.IsOpen()) {
    log("--WARNING: Could not open player index file %s...\n"
        "         : Starting game with NO PLAYERS!!!\n",
        INDEX_FILENAME);

    FILE *test = fopen(INDEX_FILENAME, "r");
    if (test) {
      log("_could_ open it normally, tho");
      fclose(test);
    }

    return false;
  }

  reset();

  char line[512];
  int line_num = 0;
  int idx = 0;

  entry_cnt = count_entries(&index);
  resize_table();

  line_num += index.GetLine(line, 512);
  while (!index.EoF() && strcmp(line, "END")) {
    entry *ptr = tab+idx;
    char temp[512];           // for paranoia

    if (sscanf(line, " %ld %d %u %lu %s ",
               &ptr->id, &ptr->level, &ptr->flags, &ptr->last, temp) != 5) {
      log("--Fatal error: syntax error in player index file, line %d",
          line_num);
      shutdown();
    }

    strncpy(ptr->name, temp, MAX_NAME_LENGTH);
    *(ptr->name+MAX_NAME_LENGTH-1) = '\0';

    ptr->active_data = NULL;

    line_num += index.GetLine(line, 512);
    idx++;
  }

  if (entry_cnt != idx)
    entry_cnt = idx;

  log("--Successfully loaded %d entries from the player index file.",
      entry_cnt);

  sort_by_id();
  clear_by_time();
  return true;
}

int  PCIndex::count_entries(File *index)
{
  char line[512];
  int count = 0;

  index->GetLine(line, 512);
  while (!index->EoF() && strcmp(line, "END")) {
    count++;
    index->GetLine(line, 512);
  }

  index->Rewind();

  return count;
}

void PCIndex::sort_by_id()
{
  qsort(tab, entry_cnt, sizeof(entry), entry_compare);
}

void PCIndex::resize_table(int empty_slots)
{
  entry_size = entry_cnt + empty_slots;
  entry *new_tab = new entry[entry_size];

  if (tab) {
    for (int i = 0; i < entry_cnt; i++)
      new_tab[i] = tab[i];

    // fill empty slots with zeroes
    memset(new_tab+entry_cnt, 0, sizeof(entry)*(entry_size-entry_cnt));

    // delete the old table
    delete [] tab;
  } else {
    // fill entire table with zeros
    memset(new_tab, 0, sizeof(entry)*entry_size);
  }

  // finally, update the pointer
  tab = new_tab;
}

void PCIndex::sorted_insert(const entry *info)
{
  int i;

  // whee!! linear!

  // first, find the correct index
  for (i = 0; i < entry_cnt; i++)
    if (tab[i].id > info->id)
      break;

  // create an empty space for the new entry
  for (int j = entry_cnt; j > i; j--)
    tab[j] = tab[j-1];

  tab[i] = *info;

  log("--Inserting %s's (id#%d) entry into position %d",
      info->name, info->id, i);

  // update entry_cnt, and we're done
  entry_cnt++;
  needs_save = true;
}

DBIndex::vnum_t PCIndex::find_open_id()
{
  if (entry_cnt < 1)
    return 1;

  /* this won't work right now since id#s are used to store
     owners of foci, etc.  So once there's a universal id# invalidator,
     this should be put in
  */
  /*
  for (int i = 1; i < entry_cnt; i++)
    if (tab[i].id > (tab[i-1].id+1))
      return (tab[i-1].id+1);
  */

  return (tab[entry_cnt-1].id+1);
}
void PCIndex::clear_by_time()
{
  for (int i = 0; i < entry_cnt; i++) {
    entry *ptr = tab+i;
    if ((time(0) - ptr->last) > (SECS_PER_REAL_DAY*30)) {
      playerDB.DeleteChar(ptr->id);
    }
  }
}
DBIndex::rnum_t PCIndex::get_idx_by_name(const char *name)
{
  for (int i = 0; i < entry_cnt; i++) {
    entry *ptr = tab+i;

    if (!str_cmp(name, ptr->name))
      return i;
  }

  return -1;
}

DBIndex::rnum_t PCIndex::get_idx_by_id(const vnum_t virt)
{
  // binary search
  int low = 0, high = entry_cnt - 1;
  int mid = high / 2;

  while (low <= high) {
    entry *ptr = tab+mid;

    if (ptr->id == virt)
      return mid;

    if (ptr->id < virt) {
      low = mid+1;
    } else {
      high = mid-1;
    }

    mid = (low + high) / 2;
  }

  return -1;
}

DBIndex::vnum_t PCIndex::get_id_by_idx(const rnum_t idx)
{
  if (idx >= 0 && idx < entry_cnt)
    return (tab[idx].id);

  return -1;
}

const char *PCIndex::get_name_by_idx(const rnum_t idx)
{
  if (idx >= 0 && idx < entry_cnt)
    return (tab[idx].name);

  return NULL;
}

char_data *PCIndex::get_char_by_idx(const rnum_t idx)
{
  if (idx >= 0 && idx < entry_cnt)
    return tab[idx].active_data;

  return NULL;
}

bool PCIndex::delete_by_idx(rnum_t idx)
{
  entry *ptr = tab+idx;

  if (ptr->flags & NODELETE)
    return false;

  // delete the character's files
  log("PCLEAN: %s (ID#%d): level=%d, last=%s",
      ptr->name, ptr->id, ptr->level,
      asctime(localtime(&(ptr->last))));

  char bits[128];
  for (int i = 0;(*(bits + i) = LOWER(*(ptr->name + i))); i++)
    ;
  sprintf(buf, "%s%s%c%s%s%s", PLR_PREFIX, SLASH, *bits, SLASH, bits, PLR_SUFFIX);
  unlink(buf);

  for (int i = idx+1; i < entry_cnt; i++)
    tab[i-1] = tab[i];

  entry_cnt--;

  entry *zero = tab+entry_cnt;

  memset(zero->name, 0, MAX_NAME_LENGTH);
  zero->id = -1;
  zero->level = 0;
  zero->last = 0;
  zero->active_data = NULL;
  zero->flags = 0;

  if (!needs_save)
    needs_save = true;

  Save();

  return true;
}

void PCIndex::update_by_idx(rnum_t idx, char_data *ch)
{
  entry *ptr = tab+idx;
  bool changed = false;

  if (ptr->id != GET_IDNUM(ch)) {
    char msg[256];

    sprintf(msg,
            "Warning: %s's cached id didn't match up (%d != %ld); fixing",
            GET_CHAR_NAME(ch), GET_IDNUM(ch), ptr->id);
    mudlog(msg, ch, LOG_SYSLOG, true);

    GET_IDNUM(ch) = ptr->id;
  }

  if (ptr->level != GET_LEVEL(ch)) {
    ptr->level = GET_LEVEL(ch);
    changed = true;
  }

  if ((PLR_FLAGGED(ch, PLR_NODELETE) && !(ptr->flags & NODELETE)) ||
      (!PLR_FLAGGED(ch, PLR_NODELETE) && (ptr->flags & NODELETE))) {
    ptr->flags = 0;

    if (PLR_FLAGGED(ch, PLR_NODELETE))
      ptr->flags |= NODELETE;

    changed = true;
  }

  if (changed && !needs_save)
    needs_save = true;
}


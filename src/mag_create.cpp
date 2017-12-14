/* ***********************************************************************
*  File: mag_create.cc                                                   *
*  authors: Christopher J. Dickey and Andrew Hynek                       *
*  purpose: this defines all the spell creation functions                *
*  (c)2001 Christopher J. Dickey, Andrew Hynek, and the AwakeMUD         *
*  Consortium                                                            *
*********************************************************************** */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#include "structs.h"
#include "awake.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "utils.h"
#include "olc.h"
#include "mag_create.h"
#include "newmagic.h"
#include "spells.h"
#include "screen.h"

#define SPELL_OBJECT_VNUM               1
#define MAX_COMBAT_EFFECTS              7
#define MAX_DETECTION_EFFECTS           7
#define MAX_HEALTH_EFFECTS              20
#define MAX_ILLUSION_EFFECTS            4
#define MAX_MANIPULATION_EFFECTS        11

extern struct room_data *world;
extern const char *wound_arr[];
extern const char *wound_name[];
extern void create_program(struct char_data *ch);

void sedit_disp_menu(struct descriptor_data *d);

// spell constants

const char *spell_category[] =
  {
    "Combat",
    "Detection",
    "Health",
    "Illusion",
    "Manipulation",
    "\n"
  };

const char *spell_target[] =
  {
    "Caster",
    "Touch",
    "Ranged",
    "Area",
    "\n"
  };

const char *elements[] =
  {
    "Non-elemental",
    "Acid",
    "Air",
    "Earth",
    "Fire",
    "Ice",
    "Lightning",
    "Water",
    "\n"
  };

const char *spell_detection[] =
  {
    "Analyze Magical Object",
    "Analyze Device",
    "Analyze Person",
    "Clairvoyance",
    "Combat Sense",
    "Detect Alignment",
    "Detect Invisibility",
    "Detect Magic",
    "\n"
  };

const char *spell_health[] =
  {
    "Antidote",
    "Cure",
    "Decrease Bod",
    "Decrease Charisma",
    "Decrease Intellect",
    "Decrease Quickness",
    "Decrease Reaction",
    "Decrease Strength",
    "Decrease Willpower",
    "Heal wounds",
    "Increase Bod",
    "Increase Charisma",
    "Increase Intellect",
    "Increase Quickness",
    "Increase Reaction",
    "Increase Reflexes",
    "Increase Strength",
    "Increase Willpower",
    "Resist Pain",
    "Stabilize",
    "\n"
  };

const char *spell_illusion[] =
  {
    "Chaos",
    "Confusion",
    "Improved Invisibility",
    "Invisibility",
    "Overstimulation",
    "\n"
  };

const char *spell_manipulation[] =
  {
    "None",
    "Acid",
    "Air",
    "Earth",
    "Fire",
    "Ice",
    "Lightning",
    "Water",
    "Armor",
    "Light",
    "Influence",
    "\n"
  };

int determine_type(spell_t *spell)
{
  int temp = 0;

  switch (spell->category) {
  case SPELL_CATEGORY_COMBAT:
    if (spell->target == AREA) {
      if (spell->physical == 1) {
        if (spell->damage == LIGHT )
          temp = SPELL_POWER_CLOUD;
        else if (spell->damage == MODERATE )
          temp = SPELL_POWERBALL;
        else if (spell->damage == SERIOUS )
          temp = SPELL_POWERBLAST;
        else
          temp = SPELL_POWER_BURST;
      } else if (spell->physical == 0) {
        if (spell->damage == LIGHT )
          temp = SPELL_MANA_CLOUD;
        else if (spell->damage == MODERATE )
          temp = SPELL_MANABALL;
        else if (spell->damage == SERIOUS )
          temp = SPELL_MANABLAST;
        else
          temp = SPELL_MANA_BURST;
      } else if (spell->physical == -1) {
        if (spell->damage == LIGHT )
          temp = SPELL_STUN_CLOUD;
        else if (spell->damage == MODERATE )
          temp = SPELL_STUNBALL;
        else if (spell->damage == SERIOUS )
          temp = SPELL_STUNBLAST;
        else
          temp = SPELL_STUN_BURST;
      }
    } else {
      if (spell->physical == 1) {
        if (spell->damage == LIGHT)
          temp = SPELL_POWER_DART;
        else if (spell->damage == MODERATE)
          temp = SPELL_POWER_MISSILE;
        else if (spell->damage == SERIOUS )
          temp = SPELL_POWER_BOLT;
        else
          temp = SPELL_POWER_SHAFT;
      } else if (spell->physical == 0) {
        if (spell->damage == LIGHT)
          temp = SPELL_MANA_DART;
        else if (spell->damage == MODERATE)
          temp = SPELL_MANA_MISSILE;
        else if (spell->damage == SERIOUS )
          temp = SPELL_MANA_BOLT;
        else
          temp = SPELL_MANA_SHAFT;
      } else if (spell->physical == -1) {
        if (spell->damage == LIGHT)
          temp = SPELL_STUN_DART;
        else if (spell->damage == MODERATE)
          temp = SPELL_STUN_MISSILE;
        else if (spell->damage == SERIOUS )
          temp = SPELL_STUN_BOLT;
        else
          temp = SPELL_STUN_SHAFT;
      }
    }
    return temp;
  case SPELL_CATEGORY_DETECTION:
    switch (spell->effect) {
    case SPELL_EFFECT_ANALYZE_MAGIC:
      temp = SPELL_ANALYZE_MAGIC;
      break;
    case SPELL_EFFECT_ANALYZE_DEVICE:
      temp = SPELL_ANALYZE_DEVICE;
      break;
    case SPELL_EFFECT_ANALYZE_PERSON:
      temp = SPELL_ANALYZE_PERSON;
      break;
    case SPELL_EFFECT_CLAIRVOYANCE:
      temp = SPELL_CLAIRVOYANCE;
      break;
    case SPELL_EFFECT_COMBAT_SENSE:
      temp = SPELL_COMBAT_SENSE;
      break;
    case SPELL_EFFECT_DETECT_ALIGN:
      temp = SPELL_DETECT_ALIGNMENT;
      break;
    case SPELL_EFFECT_DETECT_INVIS:
      temp = SPELL_DETECT_INVIS;
      break;
    case SPELL_EFFECT_DETECT_MAGIC:
      temp = SPELL_DETECT_MAGIC;
      break;
    }
    return temp;
  case SPELL_CATEGORY_HEALTH:
    if (spell->effect == SPELL_EFFECT_ANTIDOTE)
      temp = SPELL_ANTIDOTE;
    else if (spell->effect == SPELL_EFFECT_CURE)
      temp = SPELL_CURE_DISEASE;
    else if (spell->effect <= SPELL_EFFECT_DECREASE_WIL)
      temp = SPELL_DECREASE_ATTRIB;
    else if (spell->effect == SPELL_EFFECT_HEAL)
      temp = SPELL_HEAL;
    else if (spell->effect == SPELL_EFFECT_INCREASE_REFLEX)
      temp = SPELL_INCREASE_REFLEXES;
    else if (spell->effect < SPELL_EFFECT_RESIST_PAIN)
      temp = SPELL_INCREASE_ATTRIB;
    else if (spell->effect == SPELL_EFFECT_RESIST_PAIN)
      temp = SPELL_RESIST_PAIN;
    else
      temp = SPELL_STABILIZE;
    return temp;
  case SPELL_CATEGORY_ILLUSION:
    switch (spell->effect) {
    case SPELL_EFFECT_CHAOS:
      temp = SPELL_CHAOS;
      break;
    case SPELL_EFFECT_CONFUSION:
      temp = SPELL_CONFUSION;
      break;
    case SPELL_EFFECT_IMP_INVIS:
      temp = SPELL_IMPROVED_INVIS;
      break;
    case SPELL_EFFECT_INVIS:
      temp = SPELL_INVISIBILITY;
      break;
    case SPELL_EFFECT_OVERSTIM:
      temp = SPELL_OVERSTIMULATION;
      break;
    }
    return temp;
  case SPELL_CATEGORY_MANIPULATION:
    if (spell->effect < SPELL_EFFECT_ARMOR) {
      if (spell->target == AREA) {
        if (spell->damage < DEADLY)
          temp = SPELL_ELEMENT_CLOUD;
        else
          temp = SPELL_ELEMENTBALL;
      } else {
        if (spell->damage == LIGHT)
          temp = SPELL_ELEMENT_DART;
        else if (spell->damage == MODERATE)
          temp = SPELL_ELEMENT_MISSILE;
        else
          temp = SPELL_ELEMENT_BOLT;
      }
    } else if (spell->effect == SPELL_EFFECT_ARMOR)
      temp = SPELL_ARMOR;
    else if (spell->effect == SPELL_EFFECT_LIGHT)
      temp = SPELL_LIGHT;
    else
      temp = SPELL_INFLUENCE;
    return temp;
  }
  return 0;
}

void spell_to_obj(spell_t *spell, struct descriptor_data *d)
{
  struct obj_data *obj;

  for (obj = d->character->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_RNUM(obj) == 1 && !GET_OBJ_VAL(obj, 6)) // it is empty
      break;

  if (!obj)
    return;

  // change the name of the formula to include the name of the spell
  sprintf(buf1, "formula spell %s", spell->name);
  obj->text.keywords = str_dup(buf1);

  // change the name of the formula to include the name of the spell
  sprintf(buf1, "a spell formula for %s", spell->name);
  obj->text.name = str_dup(buf1);

  sprintf(buf1, "This %s spell formula is titled '%s', and appears to be a\r\n"
          " force %d %s spell.\r\n", (d->edit_number2 ? "mage" : "shaman"),
          spell->name, spell->force, spell_category[spell->category]);

  obj->text.look_desc = str_dup(buf1);

  // copy over the values from the spell
  GET_OBJ_VAL(obj, 0) = spell->physical > 0 ? 1 : 0;
  GET_OBJ_VAL(obj, 1) = spell->category;
  GET_OBJ_VAL(obj, 2) = spell->force;
  GET_OBJ_VAL(obj, 3) = spell->target;
  GET_OBJ_VAL(obj, 4) = spell->drain;
  GET_OBJ_VAL(obj, 5) = spell->damage;
  GET_OBJ_VAL(obj, 6) = spell->type;
  GET_OBJ_VAL(obj, 7) = d->edit_number2;
  GET_OBJ_VAL(obj, 8) = spell->effect;
}

void calculate_drain(spell_t *spell)
{
  int drain = 0, level = LIGHT;

  switch (spell->category) {
  case SPELL_CATEGORY_COMBAT:
    level = MAX(1, spell->damage);
    if (spell->physical > 0)
      drain++;
    if (spell->physical < 0)
      drain--;
    if (spell->effect != SPELL_EFFECT_NONE)
      level++;
    if (spell->target < SPELL_TARGET_RANGE) {
      drain--;
      level--;
    } else if (spell->target == SPELL_TARGET_AREA) {
      level++;
    }
    level = MAX(1, level);
    break;
  case SPELL_CATEGORY_DETECTION:
    switch (spell->effect) {
    case SPELL_EFFECT_DETECT_ALIGN:
    case SPELL_EFFECT_DETECT_INVIS:
    case SPELL_EFFECT_DETECT_MAGIC:
      level = LIGHT;
      break;
    case SPELL_EFFECT_CLAIRVOYANCE:
      level = MODERATE;
      break;
    case SPELL_EFFECT_ANALYZE_MAGIC:
    case SPELL_EFFECT_ANALYZE_PERSON:
    case SPELL_EFFECT_ANALYZE_DEVICE:
    case SPELL_EFFECT_COMBAT_SENSE:
      level = SERIOUS;
      if (spell->effect == SPELL_EFFECT_ANALYZE_DEVICE)
        drain = 1;
      break;
    }
    if (spell->physical > 0)
      drain++;
    if (spell->target < SPELL_TARGET_RANGE) {
      drain--;
      level--;
    } else if (spell->target == SPELL_TARGET_AREA)
      level++;
    break;
  case SPELL_CATEGORY_HEALTH:
    level = 0;
    switch (spell->effect) {
    case SPELL_EFFECT_ANTIDOTE:
    case SPELL_EFFECT_CURE:
    case SPELL_EFFECT_HEAL:
      if (spell->target >= SPELL_TARGET_RANGE)
        spell->drain = 10;
      else
        spell->drain = 0;
      return;
    case SPELL_EFFECT_RESIST_PAIN:
      if (spell->target >= SPELL_TARGET_RANGE)
        spell->drain = 11;
      else
        spell->drain = 1;
      return;
    case SPELL_EFFECT_STABILIZE:
      if (spell->target >= SPELL_TARGET_RANGE)
        spell->drain = 20 + SERIOUS;
      else
        spell->drain = 10 + SERIOUS;
      return;
    case SPELL_EFFECT_INCREASE_REFLEX:
      drain = 2;
      level = MAX(2, spell->damage + 1);
      break;
    default:
      drain = 1;
      level = MAX(1, spell->damage);
      break;
    }
    if (spell->physical > 0)
      drain++;
    if (spell->target < SPELL_TARGET_RANGE) {
      drain--;
      level--;
    }
    break;
  case SPELL_CATEGORY_ILLUSION:
    if (spell->effect == SPELL_EFFECT_OVERSTIM)
      drain = 2;
    else
      drain = 1;
    if (spell->effect == SPELL_EFFECT_IMP_INVIS ||
        spell->effect == SPELL_EFFECT_INVIS ||
        spell->effect == SPELL_EFFECT_CONFUSION)
      level = SERIOUS;
    else
      level = MODERATE;
    if (spell->physical > 0)
      drain++;
    if (spell->target < SPELL_TARGET_RANGE) {
      drain--;
      level--;
    } else if (spell->target == SPELL_TARGET_AREA)
      level++;
    break;
  case SPELL_CATEGORY_MANIPULATION:
    if (spell->effect < SPELL_EFFECT_ARMOR)
      level = MAX(2, spell->damage + 1);
    else if (spell->effect == SPELL_EFFECT_ARMOR) {
      drain = 1;
      level = MODERATE;
    } else {
      drain = 2;
      level = SERIOUS;
    }
    if (spell->physical > 0)
      drain++;
    if (spell->target < SPELL_TARGET_RANGE) {
      drain--;
      level--;
    } else if (spell->target == SPELL_TARGET_AREA)
      level++;
    level = MAX(1, level);
    break;
  default:
    drain = 0;
    level = 0;
  }
  if (level > DEADLY) {
    drain += (2 * (level - DEADLY));
    level = DEADLY;
  }
  spell->drain = (drain * 10) + MAX(0, level);
}

void check_values(spell_t *spell)
{
  spell->force = MAX(1, spell->force);
  switch (spell->category) {
  case SPELL_CATEGORY_COMBAT:
    // if (spell->effect != SPELL_EFFECT_NONE)
    //   spell->physical = 1;
    spell->effect = SPELL_EFFECT_NONE;
    if (spell->target < SPELL_TARGET_TOUCH)
      spell->target = SPELL_TARGET_RANGE;
    spell->damage = MAX(1, spell->damage);
    break;
  case SPELL_CATEGORY_DETECTION:
    spell->target = MIN(spell->target, SPELL_TARGET_RANGE);
    if (spell->effect == SPELL_EFFECT_ANALYZE_DEVICE ||
        spell->effect == SPELL_EFFECT_COMBAT_SENSE) {
      spell->physical = 1;
      spell->force = MAX(3, spell->force);
    } else
      spell->physical = 0;
    break;
  case SPELL_CATEGORY_HEALTH:
    spell->target = MIN(spell->target, SPELL_TARGET_RANGE);
    if (spell->effect == SPELL_EFFECT_HEAL
        || spell->effect == SPELL_EFFECT_RESIST_PAIN
        || spell->effect == SPELL_EFFECT_STABILIZE)
      spell->physical = 0;
    else
      spell->physical = 1;
    if (spell->effect == SPELL_EFFECT_HEAL
        ||spell->effect > SPELL_EFFECT_INCREASE_WIL
        ||spell->effect < SPELL_EFFECT_DECREASE_BOD)
      spell->damage = 0;
    break;
  case SPELL_CATEGORY_ILLUSION:
    if (spell->effect == SPELL_CHAOS ||
        spell->effect == SPELL_IMPROVED_INVIS)
      spell->physical = 1;
    else
      spell->physical = 0;
    if (spell->effect == SPELL_EFFECT_INVIS ||
        spell->effect == SPELL_EFFECT_IMP_INVIS)
      spell->target = MIN(spell->target, SPELL_TARGET_RANGE);
    break;
  case SPELL_CATEGORY_MANIPULATION:
    if (spell->effect <= SPELL_EFFECT_LIGHT)
      spell->physical = 1;
    else
      spell->physical = 0;
    if (spell->effect <= SPELL_EFFECT_ARMOR) {
      spell->target = MIN(spell->target, SPELL_TARGET_RANGE);
    }
    break;
  }
}

ACMD(do_create)
{
#if 0
  if (!access_level(ch, LVL_VICEPRES)) {
    send_to_char("Spell creation is disabled until the fabric of magic is"
                 " completely woven.  --Flynn (=\r\n", ch);
    return;
  }
#endif
  if (FIGHTING(ch)) {
    send_to_char("You can't create spells while fighting!\r\n", ch);
    return;
  }
  if (IS_NPC(ch)) {
    send_to_char("Sure...you do that.\r\n", ch);
    return;
  }

  argument = any_one_arg(argument, buf1);

  if (is_abbrev(buf1, "program")) {
    if (GET_SKILL(ch, SKILL_COMPUTER) < 1)  {
      send_to_char("You must learn computer skills to create programs.\r\n", ch);
    } else
      create_program(ch);
    return;
  } else if (is_abbrev(buf1, "shaman"))
    if (GET_SKILL(ch, SKILL_SHAMANIC_STUDIES) < 1)  {
      send_to_char("You must learn shamanic studies in order to create shamanic spells.\r\n", ch);
      return;
    } else
      ch->desc->edit_number2 = 1;
  else if (is_abbrev(buf1, "mage")) {
    if (GET_SKILL(ch, SKILL_MAGICAL_THEORY) < 1) {
      send_to_char("You must learn magical theory in order to create mage spells.\r\n", ch);
      return;
    } else
      ch->desc->edit_number2 = 0;
  } else {
    send_to_char("You can only create mage or shaman spells.\r\n", ch);
    return;
  }

  // this makes sure they are in the right place and have the right
  // skills to create spells
  switch (GET_TRADITION(ch)) {
  case TRAD_SHAMANIC:
    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_MEDICINE_LODGE)) {
      send_to_char("You must be in a medicine lodge to create spells.\r\n", ch);
      return;
    }
    break;
  case TRAD_HERMETIC:
    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HERMETIC_LIBRARY)) {
      send_to_char("You must be in a hermetic library to create spells.\r\n", ch);
      return;
    }
    break;
  default:
    send_to_char("You don't understand the ways of spell weaving.\r\n", ch);
    return;
  }

  int found = 0;
  for (struct obj_data *obj = ch->carrying; obj && !found; obj = obj->next_content)
    if (GET_OBJ_RNUM(obj) == 1 && !GET_OBJ_VAL(obj, 6)) // it is empty
      found = 1;

  if (!found) {
    send_to_char("You need a blank spell formula to create a spell.\r\n", ch);
    return;
  }

  PLR_FLAGS(ch).SetBit(PLR_SPELL_CREATE);
  STATE(ch->desc) = CON_SPELL_CREATE;

  ch->desc->edit_spell = new spell_t;
  memset((char *) ch->desc->edit_spell, 0, sizeof(spell_t));

  // init any vars here
  ch->desc->edit_spell->name = str_dup("none");

#if 0

  send_to_char("Do you wish to create a new spell?\r\n", ch);
  ch->desc->edit_mode = SEDIT_CONFIRM_EDIT;
#endif

  sedit_disp_menu(ch->desc);
}

void sedit_disp_menu(struct descriptor_data *d)
{
  CLS(CH);
  // we update drain and check values everytime it returns here
  check_values(SPELL);
  calculate_drain(SPELL);

  send_to_char(CH, "   Drain: %s+%d%s%s (%d)\r\n", CCCYN(CH, C_CMP),
               MAX(2,DRAIN_POWER(SPELL->drain) + (SPELL->force>>1)),
               DRAIN_LEVEL(SPELL->drain) > 0
               ? wound_arr[DRAIN_LEVEL(SPELL->drain)]
               : "variable",
               CCNRM(CH, C_CMP), SPELL->drain);
  send_to_char(CH, "1) Spell Name: %s%s%s\r\n", CCCYN(CH, C_CMP),
               SPELL->name, CCNRM(CH, C_CMP));
  send_to_char(CH, "2) Type: %s%s%s\r\n", CCCYN(CH, C_CMP),
               (SPELL->physical > 0 ? "Physical" :
                (SPELL->physical == 0 ? "Mana" : "Stun")),
               CCNRM(CH, C_CMP));
  send_to_char(CH, "3) Category: %s%s%s\r\n", CCCYN(CH, C_CMP),
               spell_category[SPELL->category], CCNRM(CH, C_CMP));
  send_to_char(CH, "4) Force: %s%d%s\r\n", CCCYN(CH, C_CMP), SPELL->force,
               CCNRM(CH, C_CMP));
  send_to_char(CH, "5) Target: %s%s%s\r\n", CCCYN(CH, C_CMP),
               spell_target[SPELL->target], CCNRM(CH, C_CMP));
  switch (SPELL->category)
  {
  case SPELL_CATEGORY_COMBAT:
    sprinttype(SPELL->effect, elements, buf1);
    break;
  case SPELL_CATEGORY_DETECTION:
    sprinttype(SPELL->effect, spell_detection, buf1);
    break;
  case SPELL_CATEGORY_HEALTH:
    sprinttype(SPELL->effect, spell_health, buf1);
    break;
  case SPELL_CATEGORY_ILLUSION:
    sprinttype(SPELL->effect, spell_illusion, buf1);
    break;
  case SPELL_CATEGORY_MANIPULATION:
    sprinttype(SPELL->effect, spell_manipulation, buf1);
    break;
  default:
    strcpy(buf1, "None");
    break;
  }
  send_to_char(CH, "6) type: %s%s%s\r\n", CCCYN(CH, C_CMP), buf1, CCNRM(CH, C_CMP));

  if (SPELL->category == SPELL_CATEGORY_COMBAT
      || (SPELL->category == SPELL_CATEGORY_MANIPULATION
          && SPELL->effect < SPELL_EFFECT_ARMOR))
    send_to_char(CH, "7) Base Damage: %s%s%s\r\n", CCRED(CH, C_CMP), wound_name[SPELL->damage],
                 CCNRM(CH, C_CMP));
  else if (SPELL->category == SPELL_CATEGORY_HEALTH &&
           SPELL->effect != SPELL_EFFECT_HEAL &&
           SPELL->effect >= SPELL_EFFECT_DECREASE_BOD &&
           SPELL->effect <= SPELL_EFFECT_INCREASE_WIL)
    send_to_char(CH, "7) Modifier: %s+%d%s\r\n", CCCYN(CH, C_CMP),
                 SPELL->damage, CCNRM(CH, C_CMP));

  send_to_char("a) Abort.\r\nd) Done.\r\nEnter your choice:\r\n", CH);
  d->edit_mode = SEDIT_MAIN_MENU;
}

void sedit_disp_type_menu(struct descriptor_data *d)
{
  CLS(CH);
  send_to_char("1) Physical spell\r\n", CH);
  send_to_char("2) Mana spell\r\n", CH);
  switch (SPELL->category)
  {
  case SPELL_CATEGORY_COMBAT:
    send_to_char("3) Stun spell\r\n", CH);
    break;
  default:
    break;
  }
  send_to_char("Enter your choice, 0 to quit: ", CH);
}

void sedit_disp_category_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *spell_category[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, spell_category[i]);
  send_to_char("Enter your choice, 0 to quit: ", CH);
}

void sedit_disp_target_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *spell_target[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, spell_target[i]);
  send_to_char("Enter your choice, 0 to quit: ", CH);
}

void sedit_disp_base_menu(struct descriptor_data *d)
{
  CLS(CH);

  if (SPELL->category != SPELL_CATEGORY_HEALTH)
  {
    for (int i = 1; *wound_name[i] != '\n'; i++)
      send_to_char(CH, "%d) %s\r\n", i, wound_name[i]);
    send_to_char("Enter your choice: ", CH);
  } else
    send_to_char(CH, "Enter modifier (1-3): ");
}

void sedit_disp_combat_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *elements[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, elements[i]);
  send_to_char(CH, "Spell effect: %s%s%s\r\n"
               "Enter selection, 0 to quit: ", CCCYN(CH, C_CMP),
               elements[SPELL->effect], CCNRM(CH, C_CMP));
}

void sedit_disp_detection_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *spell_detection[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, spell_detection[i]);
  send_to_char(CH, "Spell effect: %s%s%s\r\n"
               "Enter selection, 0 to quit: ", CCCYN(CH, C_CMP),
               spell_detection[SPELL->effect], CCNRM(CH, C_CMP));
}

void sedit_disp_health_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *spell_health[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, spell_health[i]);
  send_to_char(CH, "Spell effect: %s%s%s\r\n"
               "Enter selection, 0 to quit: ", CCCYN(CH, C_CMP),
               spell_health[SPELL->effect], CCNRM(CH, C_CMP));
}

void sedit_disp_illusion_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *spell_illusion[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, spell_illusion[i]);
  send_to_char(CH, "Spell effect: %s%s%s\r\n"
               "Enter selection, 0 to quit: ", CCCYN(CH, C_CMP),
               spell_illusion[SPELL->effect], CCNRM(CH, C_CMP));
}

void sedit_disp_manipulation_menu(struct descriptor_data *d)
{
  CLS(CH);

  for (register int i = 0; *spell_manipulation[i] != '\n'; i++)
    send_to_char(CH, "%d) %s\r\n", i + 1, spell_manipulation[i]);
  send_to_char(CH, "Spell effect: %s%s%s\r\n"
               "Enter selection, 0 to quit: ", CCCYN(CH, C_CMP),
               spell_manipulation[SPELL->effect], CCNRM(CH, C_CMP));
}


// kind of the main routine--it sends the descriptor to the appropriate place
void sedit_parse(struct descriptor_data *d, char *arg)
{
  int number;

  switch (d->edit_mode)
  {
  case SEDIT_CONFIRM_EDIT:
    switch (*arg) {
    case 'y':
    case 'Y':
      sedit_disp_menu(d);
      break;
    case 'n':
    case 'N':
      STATE(d) = CON_PLAYING;
      if (SPELL->name)
        delete [] SPELL->name;
      if (SPELL)
        delete SPELL;
      SPELL = NULL;
      d->edit_mode = 0;
      PLR_FLAGS(CH).SetBit(PLR_SPELL_CREATE);
      break;
    default:
      send_to_char("That's not a valid choice.\r\n", CH);
      send_to_char("Do you wish to create a new spell? (y/n) \r\n", CH);
      break;
    }
    break;
  case SEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
    case 'n':
    case 'N':
    default:
      send_to_char("Invalid choice, please enter yes or no.\r\n"
                   "Do you wish to write this formula?\r\n", CH);
      break;
    }
    break;
  case SEDIT_MAIN_MENU:
    switch (*arg) {
    case 'd':
    case 'D':
      if (!strcmp(SPELL->name,"")
          || !strcmp(SPELL->name,"none")) {
        send_to_char("You must name your spell first.\r\n", CH);
        send_to_char("Enter the new name for the spell: ", CH);
        d->edit_mode = SEDIT_EDIT_NAME;
        break;
      }
      // do the writing to an object here...
      // the rnum of a spell formula is always 1
      SPELL->type = determine_type(SPELL);
      spell_to_obj(SPELL, d);
      send_to_char("Done.\r\n", CH);
      // free up the spell now
      if (SPELL->name)
        delete [] SPELL->name;
      if (SPELL)
        delete SPELL;
      SPELL = NULL;
      PLR_FLAGS(d->character).RemoveBit(PLR_SPELL_CREATE);
      d->edit_mode = 0;
      STATE(d) = CON_PLAYING;
      break;
    case 'a':
    case 'A':
      send_to_char("Spell aborted--not scribed.\r\n", CH);
      if (SPELL->name)
        delete [] SPELL->name;
      if (SPELL)
        delete SPELL;
      SPELL = NULL;
      PLR_FLAGS(d->character).RemoveBit(PLR_SPELL_CREATE);
      d->edit_mode = 0;
      STATE(d) = CON_PLAYING;
      break;
    case '1':
      send_to_char("Enter the new name for the spell: ", CH);
      d->edit_mode = SEDIT_EDIT_NAME;
      break;
    case '2':
      sedit_disp_type_menu(d);
      d->edit_mode = SEDIT_EDIT_TYPE;
      break;
    case '3':
      sedit_disp_category_menu(d);
      d->edit_mode = SEDIT_EDIT_CATEGORY;
      break;
    case '4':
      send_to_char("Enter the force of the spell: ", CH);
      d->edit_mode = SEDIT_EDIT_FORCE;
      break;
    case '5':
      sedit_disp_target_menu(d);
      d->edit_mode = SEDIT_EDIT_TARGET;
      break;
    case '6':
      switch (SPELL->category) {
      case SPELL_CATEGORY_COMBAT:
#if 1

        sedit_disp_menu(d);
#else

        sedit_disp_combat_menu(d);
        d->edit_mode = SEDIT_EDIT_COMBAT;
#endif

        break;
      case SPELL_CATEGORY_DETECTION:
        sedit_disp_detection_menu(d);
        d->edit_mode = SEDIT_EDIT_DETECTION;
        break;
      case SPELL_CATEGORY_HEALTH:
        sedit_disp_health_menu(d);
        d->edit_mode = SEDIT_EDIT_HEALTH;
        break;
      case SPELL_CATEGORY_ILLUSION:
        sedit_disp_illusion_menu(d);
        d->edit_mode = SEDIT_EDIT_ILLUSION;
        break;
      case SPELL_CATEGORY_MANIPULATION:
        sedit_disp_manipulation_menu(d);
        d->edit_mode = SEDIT_EDIT_MANIPULATION;
        break;
      default:
        sedit_disp_menu(d);
        break;
      }
      break;
    case '7':
      if (SPELL->category == SPELL_CATEGORY_COMBAT ||
          (SPELL->category == SPELL_CATEGORY_MANIPULATION &&
           SPELL->effect < SPELL_EFFECT_ARMOR) ||
          (SPELL->category == SPELL_CATEGORY_HEALTH &&
           SPELL->effect != SPELL_EFFECT_HEAL &&
           SPELL->effect >= SPELL_EFFECT_DECREASE_BOD &&
           SPELL->effect <= SPELL_EFFECT_INCREASE_WIL)) {
        sedit_disp_base_menu(d);
        d->edit_mode = SEDIT_EDIT_BASE_DAMAGE;
      } else
        sedit_disp_menu(d);
      break;
    default:
      sedit_disp_menu(d);
      break;
    }
    break;
  case SEDIT_EDIT_NAME:
    if (*arg) {
      if (SPELL->name)
        delete [] SPELL->name;
      SPELL->name = str_dup(arg);
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_TYPE:
    switch (*arg) {
    case '1':
      SPELL->physical = 1;
      break;
    case '2':
      SPELL->physical = 0;
      break;
    case '3':
      SPELL->physical = -1;
      break;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_CATEGORY:
    number = atoi(arg);
    if ((number > 0) && (number < 6)) {
      SPELL->category = number - 1;
      SPELL->effect = 0;
      SPELL->damage = 0;
      SPELL->type = 0;
      SPELL->physical = 0;
      SPELL->target = 0;
    } else if (number != 0) {
      sedit_disp_category_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_TARGET:
    number = atoi(arg);
    if ((number > 0) && (number < 5))
      SPELL->target = number - 1;
    else if (number != 0) {
      sedit_disp_target_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_FORCE:
    number = atoi(arg);
    if (number < 1)
      sedit_disp_menu(d);
    else if (number > GET_LIBRARY_RATING(CH->in_room))
      send_to_char(CH, "\r\n"
                   "%d is the maximum force you can create in this %s.\r\n"
                   "Enter the force of the spell: ",
                   GET_LIBRARY_RATING(CH->in_room),
                   (ROOM_FLAGGED(CH->in_room, ROOM_HERMETIC_LIBRARY) ?
                    "library" : "medicine lodge"));
    else if (d->edit_number2 == 0
             && number > GET_SKILL(CH, SKILL_MAGICAL_THEORY))
      send_to_char(CH, "\r\n%d is the maximum force you can create based on "
                   "your skill in magical theory.\r\n"
                   "Enter the force of the spell: ",
                   GET_SKILL(CH, SKILL_MAGICAL_THEORY));
    else if (d->edit_number2 == 1
             && number > GET_SKILL(CH, SKILL_SHAMANIC_STUDIES))
      send_to_char(CH, "\r\n%d is the maximum force you can create based on "
                   "your skill in shamanic studies.\r\n"
                   "Enter the force of the spell: ",
                   GET_SKILL(CH, SKILL_SHAMANIC_STUDIES));
    else {
      SPELL->force = number;
      sedit_disp_menu(d);
    }
    break;
  case SEDIT_EDIT_COMBAT:
    number = atoi(arg);
    if ((number > 0) && (number <= MAX_COMBAT_EFFECTS))
      SPELL->effect = number - 1;
    else if (number != 0) {
      sedit_disp_combat_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_DETECTION:
    number = atoi(arg);
    if ((number > 0) && (number <= MAX_DETECTION_EFFECTS)) {
      SPELL->effect = 0; // this resets it for different sub-categories
      SPELL->effect = number - 1;
    } else if (number != 0) {
      sedit_disp_detection_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_HEALTH:
    number = atoi(arg);
    if ((number > 0) && (number <= MAX_HEALTH_EFFECTS))
      SPELL->effect = number - 1;
    else if (number != 0) {
      sedit_disp_health_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_ILLUSION:
    number = atoi(arg);
    if ((number > 0) && (number <= MAX_ILLUSION_EFFECTS))
      SPELL->effect = number - 1;
    else if (number != 0) {
      sedit_disp_illusion_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_MANIPULATION:
    number = atoi(arg);
    if ((number > 0) && (number <= MAX_MANIPULATION_EFFECTS))
      SPELL->effect = number - 1;
    else if (number != 0) {
      sedit_disp_manipulation_menu(d);
      return;
    }
    sedit_disp_menu(d);
    break;
  case SEDIT_EDIT_BASE_DAMAGE:
    number = atoi(arg);
    if (SPELL->category == SPELL_CATEGORY_HEALTH &&
        SPELL->effect == SPELL_EFFECT_INCREASE_REFLEX &&
        (number < 1 || number > 3))
      sedit_disp_base_menu(d);
    else if ((number < 1) && (number > 4))
      sedit_disp_base_menu(d);
    else {
      SPELL->damage = number;
      sedit_disp_menu(d);
    }
    break;
  default: // this should probably be more robust just in case (=
    break;
  }
}

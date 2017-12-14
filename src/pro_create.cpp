// Sorry, but *snicker* - Che

#include "structs.h"
#include "awake.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "utils.h"
#include "screen.h"
#include "constants.h"
#include "olc.h"

#define CH d->character
#define PEDIT_MENU 0
#define PEDIT_TYPE 1
#define PEDIT_NAME 2
#define PEDIT_RATING 3
#define PEDIT_WOUND 4


void pedit_disp_menu(struct descriptor_data *d)
{
  CLS(CH);
  send_to_char(CH, "1) Name: ^c%s^n\r\n", d->edit_obj->restring);
  send_to_char(CH, "2) Type: ^c%s^n\r\n", program_types[GET_OBJ_VAL(d->edit_obj, 0)]);
  send_to_char(CH, "3) Rating: ^c%d^n\r\n", GET_OBJ_VAL(d->edit_obj, 1));
  if (GET_OBJ_VAL(d->edit_obj, 0) == 5)
  {
    send_to_char(CH, "4) Damage: ^c%s^n\r\n", wound_name[GET_OBJ_VAL(d->edit_obj, 2)]);
    send_to_char(CH, "Size: ^c%d^n\r\n", (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * attack_multiplier[GET_OBJ_VAL(d->edit_obj, 2)]);
  } else
    send_to_char(CH, "Size: ^c%d^n\r\n", (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * program_multiplier[GET_OBJ_VAL(d->edit_obj, 0)]);
  send_to_char(CH, "q) Quit\r\nEnter your choice: ");
  d->edit_mode = PEDIT_MENU;
}

void pedit_disp_program_menu(struct descriptor_data *d)
{
  CLS(CH);
  for (int counter = 1; counter < NUM_PROGRAMS; counter += 3)
  {
    send_to_char(CH, "  %2d) %-15s %2d) %-15s %2d) %-15s\r\n",
                 counter, program_types[counter],
                 counter + 1, counter + 1 < NUM_PROGRAMS ?
                 program_types[counter + 1] : "", counter + 2, counter + 2 < NUM_PROGRAMS ?
                 program_types[counter + 2] : "");
  }
  send_to_char("Program type: ", d->character);
  d->edit_mode = PEDIT_TYPE;
}
void pedit_parse(struct descriptor_data *d, char *arg)
{
  int number = atoi(arg);
  switch(d->edit_mode)
  {
  case PEDIT_MENU:
    switch (*arg) {
    case '1':
      send_to_char(CH, "What do you want to call this program: ");
      d->edit_mode = PEDIT_NAME;
      break;
    case '2':
      pedit_disp_program_menu(d);
      break;
    case '3':
      if (!GET_OBJ_VAL(d->edit_obj, 0))
        send_to_char(CH, "Choose a program type first!\r\n");
      else {
        send_to_char(CH,"Enter Rating: ");
        d->edit_mode = PEDIT_RATING;
      }
      break;
    case '4':
      if (GET_OBJ_VAL(d->edit_obj, 0) != 5)
        send_to_char(CH, "Invalid option!\r\n");
      else {
        CLS(CH);
        send_to_char(CH, "1) Light\r\n2) Moderate\r\n3) Serious\r\n4) Deadly\r\nEnter your choice: ");
        d->edit_mode = PEDIT_WOUND;
      }
      break;
    case 'q':
      send_to_char(CH, "Design saved!\r\n");
      if (GET_OBJ_VAL(d->edit_obj, 0) == 5) {
        GET_OBJ_VAL(d->edit_obj, 4) = GET_OBJ_VAL(d->edit_obj, 1) * attack_multiplier[GET_OBJ_VAL(d->edit_obj, 2)];
        GET_OBJ_VAL(d->edit_obj, 6) = (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * attack_multiplier[GET_OBJ_VAL(d->edit_obj, 2)];
      } else {
        GET_OBJ_VAL(d->edit_obj, 4) = GET_OBJ_VAL(d->edit_obj, 1) * program_multiplier[GET_OBJ_VAL(d->edit_obj, 0)];
        GET_OBJ_VAL(d->edit_obj, 6) = (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * program_multiplier[GET_OBJ_VAL(d->edit_obj, 0)];
      }
      GET_OBJ_VAL(d->edit_obj, 4)  *= 20;
      GET_OBJ_TIMER(d->edit_obj) = GET_OBJ_VAL(d->edit_obj, 4);
      GET_OBJ_VAL(d->edit_obj, 9) = GET_IDNUM(CH);
      obj_to_char(d->edit_obj, CH);
      STATE(d) = CON_PLAYING;
      d->edit_obj = NULL;
      break;
    default:
      send_to_char(CH, "Invalid option!\r\n");
      break;
    }
    break;
  case PEDIT_RATING:
    if (GET_OBJ_VAL(d->edit_obj, 0) <= SOFT_SENSOR && number > GET_SKILL(CH, SKILL_COMPUTER) * 1.5)
      send_to_char(CH, "You can't create a persona program of a higher rating than your computer skill times one and a half.\r\n"
                   "Enter Rating: ");
    else if (GET_OBJ_VAL(d->edit_obj, 0) == SOFT_EVALUATE && number > GET_SKILL(CH, SKILL_DATA_BROKERAGE))
      send_to_char(CH, "You can't create an evaluate program of a higher rating than your data brokerage skill.\r\n"
                   "Enter Rating: ");
    else if (GET_OBJ_VAL(d->edit_obj, 0) > SOFT_SENSOR && number > GET_SKILL(CH, SKILL_COMPUTER))
      send_to_char(CH, "You can't create a program of a higher rating than your computer skill.\r\n"
                   "Enter Rating: ");
    else {
      GET_OBJ_VAL(d->edit_obj, 1) = number;
      pedit_disp_menu(d);
    }
    break;
  case PEDIT_NAME:
    d->edit_obj->restring = str_dup(arg);
    pedit_disp_menu(d);
    break;
  case PEDIT_WOUND:
    if (number < 1 || number > 4)
      send_to_char(CH, "Not a valid option!\r\nEnter your choice: ");
    else {
      GET_OBJ_VAL(d->edit_obj, 2) = number;
      pedit_disp_menu(d);
    }
    break;
  case PEDIT_TYPE:
    if (number < 1 || number >= NUM_PROGRAMS)
      send_to_char(CH, "Not a valid option!\r\nEnter your choice: ");
    else {
      GET_OBJ_VAL(d->edit_obj, 0) = number;
      GET_OBJ_VAL(d->edit_obj, 1) = 0;
      pedit_disp_menu(d);
    }
    break;
  }
}

void create_program(struct char_data *ch)
{
  struct obj_data *design = read_object(107, VIRTUAL);
  STATE(ch->desc) = CON_PRO_CREATE;
  design->restring = str_dup("A blank program");
  ch->desc->edit_obj = design;
  pedit_disp_menu(ch->desc);
}


ACMD(do_design)
{
  struct obj_data *comp, *prog;
  if (!*argument) {
    if (AFF_FLAGGED(ch, AFF_DESIGN)) {
      AFF_FLAGS(ch).RemoveBit(AFF_DESIGN);
      send_to_char(ch, "You stop working on %s.\r\n", ch->char_specials.programming->restring);
      ch->char_specials.programming = NULL;
    } else
      send_to_char(ch, "Design which program?\r\n");
    return;
  }
  if (GET_POS(ch) > POS_SITTING) {
    send_to_char(ch, "You have to be sitting to do that.\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_DESIGN) || AFF_FLAGGED(ch, AFF_PROGRAM)) {
    send_to_char(ch, "You are already working on a program.\r\n");
    return;
  }
  skip_spaces(&argument);
  for (comp = world[ch->in_room].contents; comp; comp = comp->next_content)
    if (GET_OBJ_TYPE(comp) == ITEM_DECK_ACCESSORY && GET_OBJ_VAL(comp, 0) == 2)
      break;
  if (!comp) {
    send_to_char(ch, "There is no computer here for you to make a program design on.\r\n");
    return;
  }
  for (struct char_data *vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (AFF_FLAGS(vict).AreAnySet(AFF_PROGRAM, AFF_DESIGN, ENDBIT)) {
      send_to_char(ch, "Someone is already using the computer.\r\n");
      return;
    }
  for (prog = comp->contains; prog; prog = prog->next_content)
    if ((isname(argument, prog->text.keywords) || isname(argument, prog->restring)) && GET_OBJ_TYPE(prog) == ITEM_DESIGN)
      break;
  if (!prog) {
    send_to_char(ch, "The program design isn't on that computer.\r\n");
    return;
  }
  if (GET_OBJ_VAL(prog, 3) || GET_OBJ_VAL(prog, 5)) {
    send_to_char(ch, "You can no longer make a design on that project.\r\n");
    return;
  }
  if (GET_OBJ_VAL(prog, 9) && GET_OBJ_VAL(prog, 9) != GET_IDNUM(ch)) {
    send_to_char(ch, "Someone else has already started on this program.\r\n");
    return;
  }
  int skill = 0;
  switch (GET_OBJ_VAL(prog,0)) {
  case SOFT_BOD:
  case SOFT_EVASION:
  case SOFT_MASKING:
  case SOFT_SENSOR:
    skill = GET_SKILL(ch, SKILL_PROGRAM_CYBERTERM);
    break;
  case SOFT_ATTACK:
  case SOFT_SLOW:
    skill = GET_SKILL(ch, SKILL_PROGRAM_COMBAT);
    break;
  case SOFT_CLOAK:
  case SOFT_LOCKON:
  case SOFT_MEDIC:
  case SOFT_ARMOUR:
    skill = GET_SKILL(ch, SKILL_PROGRAM_DEFENSIVE);
    break;
  case SOFT_BATTLETEC:
  case SOFT_COMPRESSOR:
  case SOFT_SLEAZE:
  case SOFT_TRACK:
  case SOFT_SUITE:
    skill = GET_SKILL(ch, SKILL_PROGRAM_SPECIAL);
    break;
  case SOFT_CAMO:
  case SOFT_CRASH:
  case SOFT_DEFUSE:
  case SOFT_EVALUATE:
  case SOFT_VALIDATE:
  case SOFT_SWERVE:
  case SOFT_SNOOPER:
  case SOFT_ANALYZE:
  case SOFT_DECRYPT:
  case SOFT_DECEPTION:
  case SOFT_RELOCATE:
  case SOFT_SCANNER:
  case SOFT_BROWSE:
  case SOFT_READ:
  case SOFT_COMMLINK:
    skill = GET_SKILL(ch, SKILL_PROGRAM_OPERATIONAL);
    break;
  }
  if (!skill) {
    send_to_char(ch, "You have no idea how to go about creating a program design for that.\r\n");
    return;
  }
  if (!GET_OBJ_VAL(prog, 9)) {
    send_to_char(ch, "You begin designing %s.\r\n", prog->restring);
    GET_OBJ_VAL(prog, 8) = skill;
  } else
    send_to_char(ch, "You continue to design %s.\r\n", prog->restring);
  AFF_FLAGS(ch).SetBit(AFF_DESIGN);
  ch->char_specials.programming = prog;
}

ACMD(do_program)
{
  struct obj_data *comp, *prog;
  if (!*argument) {
    if (AFF_FLAGGED(ch, AFF_PROGRAM)) {
      AFF_FLAGS(ch).RemoveBit(AFF_PROGRAM);
      send_to_char(ch, "You stop working on %s.\r\n", ch->char_specials.programming->restring);
      ch->char_specials.programming = NULL;
    } else
      send_to_char(ch, "Program What?\r\n");
    return;
  }
  if (GET_POS(ch) > POS_SITTING) {
    send_to_char(ch, "You have to be sitting to do that.\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_DESIGN) || AFF_FLAGGED(ch, AFF_PROGRAM)) {
    send_to_char(ch, "You are already working on a program.\r\n");
    return;
  }
  skip_spaces(&argument);
  for (struct char_data *vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (AFF_FLAGS(vict).AreAnySet(AFF_PROGRAM, AFF_DESIGN, ENDBIT)) {
      send_to_char(ch, "Someone is already using the computer.\r\n");
      return;
    }
  for (comp = world[ch->in_room].contents; comp; comp = comp->next_content)
    if (GET_OBJ_TYPE(comp) == ITEM_DECK_ACCESSORY && GET_OBJ_VAL(comp, 0) == 2)
      break;
  if (!comp) {
    send_to_char(ch, "There is no computer here for you to program on.\r\n");
    return;
  }
  for (prog = comp->contains; prog; prog = prog->next_content)
    if ((isname(argument, prog->text.keywords) || isname(argument, prog->restring)) && GET_OBJ_TYPE(prog) == ITEM_DESIGN)
      break;
  if (!prog) {
    send_to_char(ch, "The program design isn't on that computer.\r\n");
    return;
  }
  if (GET_OBJ_VAL(prog, 9) && GET_OBJ_VAL(prog, 9) != GET_IDNUM(ch)) {
    send_to_char(ch, "Someone else has already started on this program.\r\n");
    return;
  }
  if (!GET_OBJ_VAL(prog, 5)) {
    send_to_char(ch, "You begin to program %s.\r\n", prog->restring);
    int target = GET_OBJ_VAL(prog, 1);
    if (GET_OBJ_VAL(comp, 1) >= GET_OBJ_VAL(prog, 6) * 2)
      target -= 2;
    if (!GET_OBJ_VAL(prog, 3) < 1)
      target += 2;
    else
      target -= GET_OBJ_VAL(prog, 3);
    int skill = GET_SKILL(ch, SKILL_COMPUTER);
    int success = success_test(skill, target);
    for (struct obj_data *suite = comp->contains; suite; suite = suite->next_content)
      if (GET_OBJ_TYPE(suite) == ITEM_PROGRAM && GET_OBJ_VAL(suite, 0) == SOFT_SUITE) {
        skill += MIN(GET_SKILL(ch, SKILL_COMPUTER), GET_OBJ_VAL(suite, 1));
        break;
      }
    success += (int)(success_test(skill, target) / 2);
    if (success)
      GET_OBJ_VAL(prog, 5) = GET_OBJ_VAL(prog, 6) / success;
    else {
      GET_OBJ_VAL(prog, 5) = number(1, 6) + number(1, 6);
      GET_OBJ_VAL(prog, 7) = 1;
    }
    GET_OBJ_VAL(prog, 5) *= 60;
    GET_OBJ_TIMER(prog) = GET_OBJ_VAL(prog, 5);
  } else
    send_to_char(ch, "You continue to work on %s.\r\n", prog->restring);
  AFF_FLAGS(ch).SetBit(AFF_PROGRAM);
  ch->char_specials.programming = prog;
}

#define PROG desc->character->char_specials.programming
void update_programs(void)
{
  for (struct descriptor_data *desc = descriptor_list; desc; desc = desc->next)
    if (desc->character)
      if (AFF_FLAGGED(desc->character, AFF_DESIGN)) {
        if (--GET_OBJ_VAL(PROG, 4) < 1) {
          send_to_char(desc->character, "You complete the design plan for %s.\r\n", PROG->restring);
          int target = 4;
          if (GET_OBJ_VAL(PROG, 1) < 5)
            target--;
          else if (GET_OBJ_VAL(PROG, 1) > 9)
            target++;
          GET_OBJ_VAL(PROG, 3) = success_test(GET_OBJ_VAL(PROG, 8), target);
          PROG = NULL;
          AFF_FLAGS(desc->character).RemoveBit(AFF_DESIGN);
        }
        if (ROOM_FLAGGED(desc->character->in_room, ROOM_HOUSE))
          ROOM_FLAGS(desc->character->in_room).SetBit(ROOM_HOUSE_CRASH);
      } else if (AFF_FLAGGED(desc->character, AFF_PROGRAM)) {
        if (--GET_OBJ_VAL(PROG, 5) < 1) {
          if (GET_OBJ_VAL(PROG, 7))
            send_to_char(desc->character, "You realise programming %s is a lost cause.\r\n", PROG->restring);
          else {
            send_to_char(desc->character, "You complete programming %s.\r\n", PROG->restring);
            struct obj_data *newp = read_object(108, VIRTUAL);
            newp->restring = str_dup(PROG->restring);
            GET_OBJ_VAL(newp, 0) = GET_OBJ_VAL(PROG, 0);
            GET_OBJ_VAL(newp, 1) = GET_OBJ_VAL(PROG, 1);
            GET_OBJ_VAL(newp, 2) = GET_OBJ_VAL(PROG, 6);
            GET_OBJ_VAL(newp, 3) = GET_OBJ_VAL(PROG, 2);
            obj_to_char(newp, desc->character);
          }
          GET_OBJ_VAL(PROG->in_obj, 3) -= GET_OBJ_VAL(PROG, 6) + (GET_OBJ_VAL(PROG, 6) / 10);
          extract_obj(PROG);
          PROG = NULL;
          AFF_FLAGS(desc->character).RemoveBit(AFF_PROGRAM);
        }
        if (ROOM_FLAGGED(desc->character->in_room, ROOM_HOUSE))
          ROOM_FLAGS(desc->character->in_room).SetBit(ROOM_HOUSE_CRASH);
      }
}

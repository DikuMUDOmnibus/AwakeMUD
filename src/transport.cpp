/* **********************************************************************
*  file: transport.cc                                                   *
*  author: Andrew Hynek                                                 *
*  (original monorail code, now deleted, written by Chris Dickey)       *
*  purpose: contains all routines for time- and command-based transport *
*  Copyright (c) 1998 by Andrew Hynek                                   *
*  (c) 2001 The AwakeMUD Consortium                                     *
********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#define strcasecmp(x, y) _stricmp(x,y)
#else
#endif

#include "structs.h"
#include "awake.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "transport.h"
#include "utils.h"
#include "constants.h"

SPECIAL(call_elevator);
SPECIAL(elevator_spec);
extern int find_first_step(vnum_t src, vnum_t target);
// ----------------------------------------------------------------------------

// ______________________________
//
// static vars
// ______________________________

static struct elevator_data *elevator = NULL;
static int num_elevators = 0;

static const int NUM_SEATTLE_STATIONS = 12;
static const int NUM_SEATAC_STATIONS = 2;
static bool enable_seatac_monorail = true;

static struct dest_data destinations[] =
  {
    { "cinema", "Le Cinema Vieux", 32588, 1, NORTH },
    { "vieux", "Le Cinema Vieux", 32588, 1, NORTH },
    { "reaper", "The Reaper", 32517, 4, SOUTH },
    { "platinum", "Platinum Club", 32685, 1, SOUTH },
    { "penumbra", "Club Penumbra", 32587, 1, WEST },
    { "big", "The Big Rhino", 32635, 1, SOUTH },
    { "Yoshi", "Yoshi's Sushi Bar", 32751, 1, NORTHWEST },
    { "aztech",  "Aztechnology Pyramid", 30539, 1, SOUTHEAST },
    { "garage", "Seattle Parking Garage", 32720, 1, SOUTHEAST },
    { "formal", "Seattle Formal Wear", 32746, 1, SOUTHEAST },
    { "dante", "Dante's Inferno", 32661, 1, SOUTH},
    { "quinns", "Quinn's", 32521, 1, SOUTHWEST },
    { "shintaru", "Shintaru", 32513, 1, SOUTHWEST },
    { "docks", "Seattle Dockyards", 32500, 1, NORTHWEST },
    { "modern", "Modern Magik", 30514, 1, NORTH },
    { "\n", "", 0, 0, 0 } // this MUST be last
  };
/*
static struct dest_data port_destinations[] =
  {
    { "tacoma", "Tacoma", 2000, 3, SOUTH
    }
 
  };
*/
struct transport_type
{
  vnum_t transport;
  dir_t to;
  vnum_t room;
  dir_t from;
};

static struct transport_type seatac[NUM_SEATAC_STATIONS] =
  {
    {
      501, WEST, 3018, EAST
    },
    { 501, EAST, 9823, WEST }
  };

// ______________________________
//
// func prototypes
// ______________________________

SPECIAL(taxi);
static int process_elevator(struct room_data *room,
                            struct char_data *ch,
                            int cmd,
                            char *argument);

// ____________________________________________________________________________
//
// Taxi
// ____________________________________________________________________________

// ______________________________
//
// utility funcs
// ______________________________

static void open_taxi_door(vnum_t room, int dir, int taxi)
{
  world[room].dir_option[dir] = new room_direction_data;
  memset((char *) world[room].dir_option[dir], 0,
         sizeof (struct room_direction_data));
  world[room].dir_option[dir]->to_room = taxi;
  world[room].dir_option[dir]->barrier = 8;
  world[room].dir_option[dir]->condition = 8;
  world[room].dir_option[dir]->material = 8;

  dir = rev_dir[dir];

  world[taxi].dir_option[dir] = new room_direction_data;
  memset((char *) world[taxi].dir_option[dir], 0,
         sizeof (struct room_direction_data));
  world[taxi].dir_option[dir]->to_room = room;
  world[taxi].dir_option[dir]->barrier = 8;
  world[taxi].dir_option[dir]->condition = 8;
  world[taxi].dir_option[dir]->material = 8;
}

static void close_taxi_door(int room, int dir, int taxi)
{
  if (world[room].dir_option[dir]->keyword)
    delete [] world[room].dir_option[dir]->keyword;
  if (world[room].dir_option[dir]->general_description)
    delete [] world[room].dir_option[dir]->general_description;
  delete world[room].dir_option[dir];
  world[room].dir_option[dir] = NULL;

  dir = rev_dir[dir];

  if (world[taxi].dir_option[dir]->keyword)
    delete [] world[taxi].dir_option[dir]->keyword;
  if (world[taxi].dir_option[dir]->general_description)
    delete [] world[taxi].dir_option[dir]->general_description;
  delete world[taxi].dir_option[dir];
  world[taxi].dir_option[dir] = NULL;
}

void taxi_leaves(void)
{
  int i, j, found = 0, to;
  struct char_data *temp;
  for (j = real_room(FIRST_CAB); j <= real_room(LAST_CAB); j++) {
    found = 0;
    for (temp = world[j].people; temp; temp = temp->next_in_room)
      if (!(GET_MOB_SPEC(temp) && GET_MOB_SPEC(temp) == taxi)) {
        found = 1;
        break;
      }
    if (found)
      continue;
    for (i = NORTH; i < UP; i++)
      if (world[j].dir_option[i]) {
        to = world[j].dir_option[i]->to_room;
        close_taxi_door(to, rev_dir[i], j);
        if (world[to].people) {
          act("The taxi door slams shut as its wheels churn up a "
              "cloud of dirt.", FALSE, world[to].people, 0, 0, TO_ROOM);
          act("The taxi door slams shut as its wheels churn up a "
              "cloud of dirt.", FALSE, world[to].people, 0, 0, TO_CHAR);
        }
        if (world[j].people)
          act("The door automatically closes.",
              FALSE, world[j].people, 0, 0, TO_CHAR);
      }
  }
}

// ______________________________
//
// hail command
// ______________________________

ACMD(do_hail)
{
  struct char_data *temp;
  int cab, dir, first, last, i = -1;
  bool found = FALSE, empty = FALSE;
  SPECIAL(taxi);

  for (dir = NORTH; dir < UP; dir++)
    if (!world[ch->in_room].dir_option[dir])
      empty = TRUE;

  if ( IS_ASTRAL(ch) ) {
    send_to_char("Magically active cab drivers...now I've heard everything...\n\r",ch);
    return;
  }

  if (world[ch->in_room].sector_type != SECT_CITY || !empty ||
      ROOM_FLAGGED(ch->in_room, ROOM_INDOORS)) {
    send_to_char("There doesn't seem to be any cabs in the area.\r\n", ch);
    return;
  }

  if ( (i = IN_ROOM(ch)) > -1 ) {
    switch (zone_table[world[i].zone].number) {
    case 40:
    case 305:
    case 325:
    case 74:
    case 13:
    case 15:
    case 20:
    case 194:
    case 22:
    case 30:
    case 29:
    case 25:
    case 32:
    case 143:
    case 23:
    case 24:
    case 26:
    case 49:
    case 38:
    case 45:
    case 42:
    case 301:
    case 293:
    case 290:
    case 707:
      break;

    default:
      /* Cab doesn't service the area */
      send_to_char("There don't seem to be any cabs in the area.\n\r",ch);
      return;
    }
  }

  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_IMP_INVIS)) {
    send_to_char("A cab almost stops, but guns it at the last second, splashing you...\n\r",ch);
    return;
  }


  first = real_room(FIRST_CAB);
  last = real_room(LAST_CAB);

  for (cab = first; cab <= last; cab++) {
    for (temp = world[cab].people; temp; temp = temp->next_in_room)
      if (!(GET_MOB_SPEC(temp) && GET_MOB_SPEC(temp) == taxi))
        break;
    if (!temp) {
      found = TRUE;
      for (dir = NORTH; dir < UP; dir++)
        if (world[cab].dir_option[dir])
          found = FALSE;
      if (found)
        break;
    }
  }

  if (!found) {
    send_to_char("Hail as you might, no cab answers.\r\n", ch);
    return;
  }

  for (dir = number(NORTH, UP - 1);; dir = number(NORTH, UP - 1))
    if (!world[ch->in_room].dir_option[dir]) {
      open_taxi_door(ch->in_room, dir, cab);
      sprintf(buf, "A beat-up yellow cab screeches to a halt, "
              "and its door opens to the %s.", fulldirs[dir]);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
}

// ______________________________
//
// driver spec
// ______________________________

SPECIAL(taxi)
{
  extern bool memory(struct char_data *ch, struct char_data *vict);
  ACMD(do_say);
  ACMD(do_action);

  struct char_data *temp = NULL, *driver = (struct char_data *) me;
  int comm = CMD_NONE, i = 0, j;
  char say[80];
  vnum_t dest = 0;
  if (!cmd) {
    for (temp = world[driver->in_room].people; temp; temp = temp->next_in_room)
      if (temp != driver && memory(driver, temp))
        break;
    if (!temp) {
      GET_SPARE1(driver) = 0;
      GET_SPARE2(driver) = 0;
      GET_ACTIVE(driver) = ACT_AWAIT_CMD;
    } else
      switch (GET_ACTIVE(driver)) {
      case ACT_REPLY_DEST:
        sprintf(say, "%s?  Yeah, sure...it'll cost ya %d nuyen, whaddya say?",
                destinations[GET_SPARE2(driver)].str, (int)GET_SPARE1(driver));
        do_say(driver, say, 0, 0);
        GET_ACTIVE(driver) = ACT_AWAIT_YESNO;
        break;
      case ACT_REPLY_NOTOK:
        do_say(driver, "Ya ain't got the nuyen!", 0, 0);
        forget(driver, temp);
        GET_SPARE1(driver) = 0;
        GET_SPARE2(driver) = 0;
        GET_ACTIVE(driver) = ACT_AWAIT_CMD;
        break;
      case ACT_REPLY_TOOBAD:
        do_say(driver, "Whatever, chum.", 0, 0);
        forget(driver, temp);
        GET_SPARE1(driver) = 0;
        GET_SPARE2(driver) = 0;
        GET_ACTIVE(driver) = ACT_AWAIT_CMD;
        break;
      case ACT_DRIVING:
        if (GET_SPARE1(driver) > 0)
          GET_SPARE1(driver)--;
        else {
          do_say(driver, "Ok, heres we are.", 0, 0);
          forget(driver, temp);
          dest = real_room(GET_SPARE2(driver));
          GET_SPARE2(driver) = 0;
          GET_ACTIVE(driver) = ACT_AWAIT_CMD;
          for (j = number(NORTH, NORTHWEST);; j = number(NORTH, NORTHWEST))
            if (!world[dest].dir_option[j]) {
              open_taxi_door(dest, j, driver->in_room);
              if (world[dest].people) {
                act("A taxi pulls to a stop, its door sliding open.",
                    FALSE, world[dest].people, 0, 0, TO_ROOM);
                act("A taxi pulls to a stop, its door sliding open.",
                    FALSE, world[dest].people, 0, 0, TO_CHAR);
              }
              sprintf(buf, "The door, rather noisily, slides open to the %s.",
                      fulldirs[rev_dir[j]]);
              act(buf, FALSE, driver, 0, 0, TO_ROOM);
              act(buf, FALSE, driver, 0, 0, TO_CHAR);
              break;
            }
        }
        break;
      }
    return FALSE;
  }

  if (!IS_NPC(ch) && memory(driver, ch) && (CMD_IS("north") ||
      CMD_IS("east") || CMD_IS("west") || CMD_IS("south") || CMD_IS("ne") ||
      CMD_IS("se") || CMD_IS("sw") || CMD_IS("nw") || CMD_IS("northeast") ||
      CMD_IS("southeast") || CMD_IS("southwest") || CMD_IS("northwest")) && GET_ACTIVE(driver) != ACT_DRIVING) {
    forget(driver, ch);
    return FALSE;
  }

  if (!CAN_SEE(driver, ch) || IS_NPC(ch) ||
      (GET_ACTIVE(driver) != ACT_AWAIT_CMD &&
       GET_ACTIVE(driver) != ACT_AWAIT_YESNO))
    return FALSE;

  skip_spaces(&argument);

  if (CMD_IS("say") || CMD_IS("'")) {
    bool found = FALSE;
    if (GET_ACTIVE(driver) == ACT_AWAIT_CMD)
      for (dest = 0; *destinations[dest].keyword != '\n'; dest++)
        if ( str_str((const char *)argument, destinations[dest].keyword)) {
          comm = CMD_DEST;
          found = TRUE;
          break;
        }
    if (!found) {
      if (strstr(argument, "yes") || strstr(argument, "sure") ||
          strstr(argument, "yea"))
        comm = CMD_YES;
      else if (strstr(argument, "no"))
        comm = CMD_NO;
    }
    do_say(ch, argument, 0, 0);
  } else if (CMD_IS("nod")) {
    comm = CMD_YES;
    do_action(ch, argument, cmd, 0);
  } else if (CMD_IS("shake") && !*argument) {
    comm = CMD_NO;
    do_action(ch, argument, cmd, 0);
  } else
    return FALSE;

  if (comm == CMD_DEST && !memory(driver, ch) &&
      (i = real_room(GET_LASTROOM(ch))) > -1 &&
      GET_ACTIVE(driver) == ACT_AWAIT_CMD) {
    for (i = NORTH; i < UP; i++)
      if (world[ch->in_room].dir_option[i]) {
        i = world[ch->in_room].dir_option[i]->to_room;
        break;
      }
    int dist = 0;
    while (i != -1) {
      int x = find_first_step(i, real_room(destinations[dest].vnum));
      if (x == -2)
        break;
      else if (x < 0) {
        i = -1;
        break;
      }
      i = world[i].dir_option[x]->to_room;
      dist++;
    }
    if (i == -1)
      GET_SPARE1(driver) = 500;
    else
      GET_SPARE1(driver) = 10 + (int)(dist * 2);
    GET_SPARE2(driver) = dest;
    GET_ACTIVE(driver) = ACT_REPLY_DEST;
    remember(driver, ch);
  } else if (comm == CMD_YES && memory(driver, ch) &&
             GET_ACTIVE(driver) == ACT_AWAIT_YESNO) {
    if (GET_NUYEN(ch) < GET_SPARE1(driver) && !IS_SENATOR(ch)) {
      GET_ACTIVE(driver) = ACT_REPLY_NOTOK;
      return TRUE;
    }
    if (!IS_SENATOR(ch))
      GET_NUYEN(ch) -= GET_SPARE1(driver);
    GET_SPARE1(driver) = (int)(GET_SPARE1(driver) / 50);
    GET_SPARE2(driver) = destinations[GET_SPARE2(driver)].vnum;
    GET_ACTIVE(driver) = ACT_DRIVING;

    for (i = NORTH; i < UP; i++)
      if (world[ch->in_room].dir_option[i]) {
        dest = world[ch->in_room].dir_option[i]->to_room;
        close_taxi_door(dest, rev_dir[i], ch->in_room);
        if (world[dest].people) {
          act("The taxi door slams shut as its wheels churn up a cloud "
              "of dirt.", FALSE, world[dest].people, 0, 0, TO_ROOM);
          act("The taxi door slams shut as its wheels churn up a cloud "
              "of dirt.", FALSE, world[dest].people, 0, 0, TO_CHAR);
        }
        act("The door shuts as the taxi begins to accelerate.",
            FALSE, ch, 0, 0, TO_ROOM);
        act("The door shuts as the taxi begins to accelerate.",
            FALSE, ch, 0, 0, TO_CHAR);
      }
  } else if (comm == CMD_NO && memory(driver, ch) &&
             GET_ACTIVE(driver) == ACT_AWAIT_YESNO)
    GET_ACTIVE(driver) = ACT_REPLY_TOOBAD;

  return TRUE;
}

// ______________________________
//
// Elevators
// ______________________________

// ______________________________
//
// utility funcs
// ______________________________

static void init_elevators(void)
{
  FILE *fl;
  char line[256];
  int i, j, t[3];
  vnum_t room[1], rnum;
  if (!(fl = fopen(ELEVATOR_FILE, "r"))) {
    log("Error opening elevator file.");
    shutdown();
  }

  if (!get_line(fl, line) || sscanf(line, "%d", &num_elevators) != 1) {
    log("Error at beginning of elevator file.");
    shutdown();
  }

  elevator = new struct elevator_data[num_elevators];

  for (i = 0; i < num_elevators && !feof(fl); i++) {
    get_line(fl, line);
    if (sscanf(line, "%ld %d %d %d", room, t, t + 1, t + 2) != 4) {
      fprintf(stderr, "Format error in elevator #%d, expecting # # # #\n", i);
      shutdown();
    }
    elevator[i].room = room[0];
    if ((rnum = real_room(elevator[i].room)) > -1) {
      world[rnum].func = elevator_spec;
      world[rnum].rating = 0;
    } else
      log("Nonexistent elevator.");
    elevator[i].columns = t[0];
    elevator[i].time_left = 0;
    elevator[i].dir = -1;
    elevator[i].destination = 0;
    elevator[i].num_floors = t[1];
    elevator[i].start_floor = t[2];

    if (elevator[i].num_floors > 0) {
      elevator[i].floor = new struct floor_data[elevator[i].num_floors];
      for (j = 0; j < elevator[i].num_floors; j++) {
        get_line(fl, line);
        if (sscanf(line, "%ld %d", room, t + 1) != 2) {
          fprintf(stderr, "Format error in elevator #%d, floor #%d\n", i, j);
          shutdown();
        }
        elevator[i].floor[j].vnum = room[0];
        if ((rnum = real_room(elevator[i].floor[j].vnum)) > -1)
          world[rnum].func = call_elevator;
        else {
          elevator[i].floor[j].vnum = -1;
          log("Nonexistent elevator destination -- blocking.");
        }
        elevator[i].floor[j].doors = t[1];
      }
    } else
      elevator[i].floor = NULL;
  }
  fclose(fl);
}

static void open_elevator_doors(struct room_data *room, int num, int floor)
{
  int dir;
  long rnum;

  rnum = real_room(elevator[num].floor[floor].vnum);
  dir = elevator[num].floor[floor].doors;

  room->dir_option[dir] = new room_direction_data;
  memset((char *) room->dir_option[dir], 0, sizeof (struct room_direction_data));
  room->dir_option[dir]->to_room = rnum;
  room->dir_option[dir]->barrier = 8;
  room->dir_option[dir]->condition = 8;
  room->dir_option[dir]->material = 8;

  dir = rev_dir[dir];

  world[rnum].dir_option[dir] = new room_direction_data;
  memset((char *) world[rnum].dir_option[dir], 0, sizeof (struct room_direction_data));
  world[rnum].dir_option[dir]->to_room = real_room(room->number);
  world[rnum].dir_option[dir]->barrier = 8;
  world[rnum].dir_option[dir]->condition = 8;
  world[rnum].dir_option[dir]->material = 8;

  elevator[num].dir = UP - 1;
  if (world[rnum].people)
  {
    sprintf(buf, "The elevator doors open to the %s.", fulldirs[dir]);
    act(buf, FALSE, world[rnum].people, 0, 0, TO_ROOM);
    act(buf, FALSE, world[rnum].people, 0, 0, TO_CHAR);
  }
}

static void close_elevator_doors(struct room_data *room, int num, int floor)
{
  long rnum;
  int dir;

  dir = elevator[num].floor[floor].doors;
  rnum = room->dir_option[dir]->to_room;

  if (room->dir_option[dir]->keyword)
    delete [] room->dir_option[dir]->keyword;
  if (room->dir_option[dir]->general_description)
    delete [] room->dir_option[dir]->general_description;
  delete room->dir_option[dir];
  room->dir_option[dir] = NULL;

  dir = rev_dir[dir];

  if (world[rnum].dir_option[dir]->keyword)
    delete [] world[rnum].dir_option[dir]->keyword;
  if (world[rnum].dir_option[dir]->general_description)
    delete [] world[rnum].dir_option[dir]->general_description;
  delete world[rnum].dir_option[dir];
  world[rnum].dir_option[dir] = NULL;

  if (world[rnum].people)
  {
    act("The elevator doors close.",
        FALSE, world[rnum].people, 0, 0, TO_ROOM);
    act("The elevator doors close.",
        FALSE, world[rnum].people, 0, 0, TO_CHAR);
  }
}

// ______________________________
//
// elevator lobby / call-button spec
// ______________________________

SPECIAL(call_elevator)
{
  int i = 0, j, index = -1;
  long rnum;
  if (!cmd)
    return FALSE;

  for (i = 0; i < num_elevators && index < 0; i++)
    for (j = 0; j < elevator[i].num_floors && index < 0; j++)
      if (elevator[i].floor[j].vnum == world[ch->in_room].number)
        index = i;

  if (CMD_IS("push")) {
    skip_spaces(&argument);
    if (!*argument || !(!strcasecmp("elevator", argument) ||
                        !strcasecmp("button", argument)))
      send_to_char("Press what?\r\n", ch);
    else {
      if (IS_ASTRAL(ch)) {
        send_to_char("You can't do that in your current state.\r\n", ch);
        return TRUE;
      }
      if (index < 0 || elevator[index].destination) {
        send_to_char("You press the call button, "
                     "but nothing seems to happen.\r\n", ch);
        return TRUE;
      }
      rnum = real_room(elevator[index].room);
      for (i = 0; i < UP; i++)
        if (world[rnum].dir_option[i] &&
            world[rnum].dir_option[i]->to_room == ch->in_room) {
          send_to_char("The door is already open!\r\n", ch);
          elevator[index].destination = 0;
          return TRUE;
        }
      send_to_char("You press the call button, "
                   "and the small light turns on.\r\n", ch);
      elevator[index].destination = world[ch->in_room].number;
    }
    return TRUE;
  }

  if (CMD_IS("look")) {
    one_argument(argument, arg);
    if (!*arg || index < 0 || !(!strcasecmp("panel", arg) ||
                                !strcasecmp("elevator", arg)))
      return FALSE;

    rnum = real_room(elevator[index].room);

    i = world[rnum].rating + 1 - elevator[index].num_floors - elevator[index].start_floor;
    if (i > 0)
      send_to_char(ch, "The floor indicator shows that the elevator is "
                   "currently at B%d.\r\n", i);
    else if (i == 0)
      send_to_char(ch, "The floor indicator shows that the elevator is "
                   "currently at the ground floor.\r\n");
    else
      send_to_char(ch, "The floor indicator shows that the elevator is "
                   "current at floor %d.\r\n", 0 - i);
    return TRUE;
  }

  return FALSE;
}

// ______________________________
//
// elevator spec
// ______________________________

SPECIAL(elevator_spec)
{
  struct room_data *room = (struct room_data *) me;

  if (cmd && process_elevator(room, ch, cmd, argument))
    return TRUE;

  return FALSE;
}

// ______________________________
//
// processing funcs
// ______________________________

static int process_elevator(struct room_data *room,
                            struct char_data *ch,
                            int cmd,
                            char *argument)
{
  int num, temp, number, floor = 0;

  for (num = 0; num < num_elevators; num++)
    if (elevator[num].room == room->number)
      break;

  if (num >= num_elevators)
    return 0;

  if (!cmd)
  {
    if (elevator[num].destination && elevator[num].dir == -1) {
      for (temp = 0; temp <= elevator[num].num_floors; temp++)
        if (elevator[num].floor[temp].vnum == elevator[num].destination)
          floor = temp;
      if (floor >= room->rating) {
        elevator[num].dir = DOWN;
        elevator[num].time_left = floor - room->rating;
      } else if (floor < room->rating) {
        elevator[num].dir = UP;
        elevator[num].time_left = room->rating - floor;
      }
      elevator[num].destination = 0;
    }
    if (elevator[num].time_left > 0) {
      elevator[num].time_left--;
      if (elevator[num].dir == DOWN)
        room->rating++;
      else
        room->rating--;
    } else if (elevator[num].dir == UP || elevator[num].dir == DOWN) {
      temp = room->rating + 1 - elevator[num].num_floors - elevator[num].start_floor;
      if (temp > 0)
        sprintf(buf, "The elevator stops at B%d, "
                "and the doors open to the %s.", temp,
                fulldirs[elevator[num].floor[room->rating].doors]);
      else if (temp == 0)
        sprintf(buf, "The elevator stops at the ground floor, "
                "and the doors open to the %s.", fulldirs[elevator[num].floor[room->rating].doors]);
      else
        sprintf(buf, "The elevator stops at floor %d, "
                "and the doors open to the %s.", 0 - temp,
                fulldirs[elevator[num].floor[room->rating].doors]);
      if (room->people) {
        act(buf, FALSE, room->people, 0, 0, TO_ROOM);
        act(buf, FALSE, room->people, 0, 0, TO_CHAR);
      }
      open_elevator_doors(room, num, room->rating);
    } else if (elevator[num].dir > 0)
      elevator[num].dir--;
    else if (!elevator[num].dir) {
      if (room->people) {
        act("The elevator doors close.", FALSE, room->people, 0, 0, TO_ROOM);
        act("The elevator doors close.", FALSE, room->people, 0, 0, TO_CHAR);
      }
      close_elevator_doors(room, num, room->rating);
      elevator[num].dir = -1;
    }
  } else if (CMD_IS("push"))
  {
    if (IS_ASTRAL(ch)) {
      send_to_char("You can't do that in your current state.\r\n", ch);
      return TRUE;
    }
    if (!*argument) {
      send_to_char("Press which button?\r\n", ch);
      return TRUE;
    }

    if (elevator[num].dir >= UP) {
      send_to_char("The elevator has already been activated.\r\n", ch);
      return TRUE;
    }

    skip_spaces(&argument);
    if (LOWER(*argument) == 'b' && (number = atoi(argument + 1)) > 0)
      number = elevator[num].num_floors + elevator[num].start_floor + number - 1;
    else if (LOWER(*argument) == 'g' && elevator[num].start_floor <= 0)
      number = elevator[num].num_floors + elevator[num].start_floor - 1;
    else if ((number = atoi(argument)) > 0)
      number = elevator[num].num_floors + elevator[num].start_floor - 1 - number;
    else
      number = -1;

    if (number < 0 || number >= elevator[num].num_floors ||
        elevator[num].floor[number].vnum < 0)
      send_to_char(ch, "'%s' isn't a button.\r\n", argument);
    else if (number == room->rating) {
      if (room->dir_option[elevator[num].floor[number].doors]) {
        temp = room->rating + 1 - elevator[num].num_floors - elevator[num].start_floor;
        if (temp > 0)
          send_to_char(ch, "You are already at B%d!\r\n", temp);
        else if (temp == 0)
          send_to_char(ch, "You are already at the ground floor!\r\n");
        else
          send_to_char(ch, "You are already at floor %d!\r\n", 0 - temp);
      } else {
        sprintf(buf, "The elevator doors open to the %s.",
                fulldirs[elevator[num].floor[room->rating].doors]);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
        open_elevator_doors(room, num, room->rating);
      }
    } else {
      if (number > room->rating) {
        elevator[num].dir = DOWN;
        elevator[num].time_left = MAX(1, number - room->rating);
      } else {
        elevator[num].dir = UP;
        elevator[num].time_left = MAX(1, room->rating - number);
      }
      if (!room->dir_option[elevator[num].floor[room->rating].doors])
        sprintf(buf, "The elevator begins to %s.",
                (elevator[num].dir == UP ? "ascend" : "descend"));
      else {
        sprintf(buf, "The elevator doors close and it begins to %s.",
                (elevator[num].dir == UP ? "ascend" : "descend"));
        close_elevator_doors(room, num, room->rating);
      }
      act(buf, FALSE, ch, 0, 0, TO_ROOM);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
    return TRUE;
  } else if (CMD_IS("look"))
  {
    one_argument(argument, arg);
    if (!*arg || !(!strcasecmp("panel", arg) || !strcasecmp("elevator", arg)
                   || !strcasecmp("buttons", arg)))
      return FALSE;

    strcpy(buf, "The elevator panel displays the following buttons:\r\n");
    number = 0;
    for (floor = 0; floor < elevator[num].num_floors; floor++)
      if (elevator[num].floor[floor].vnum > -1) {
        temp = elevator[num].start_floor + floor;
        if (temp < 0)
          sprintf(buf + strlen(buf), "  B%-2d", 0 - temp);
        else if (temp == 0)
          sprintf(buf + strlen(buf), "  G ");
        else
          sprintf(buf + strlen(buf), "  %-2d", temp);
        number++;
        if (!(number % elevator[num].columns))
          strcat(buf, "\r\n");
      }
    if ((number % elevator[num].columns))
      strcat(buf, "\r\n");
    temp = room->rating + 1 - elevator[num].num_floors - elevator[num].start_floor;
    if (temp > 0)
      sprintf(buf + strlen(buf), "The floor indicator shows that the "
              "elevator is currently at B%d.\r\n", temp);
    else if (temp == 0)
      sprintf(buf + strlen(buf), "The floor indicator shows that the "
              "elevator is currently at the ground floor.\r\n");
    else
      sprintf(buf + strlen(buf), "The floor indicator shows that the "
              "elevator is currently at floor %d.\r\n", 0 - temp);
    send_to_char(buf, ch);
    return TRUE;
  }
  return FALSE;
}

void ElevatorProcess(void)
{
  int i, rnum;

  for (i = 0; i < num_elevators; i++)
    if (elevator && (rnum = real_room(elevator[i].room)) > -1)
      process_elevator(&world[rnum], NULL, 0, "");
}

// ______________________________
//
// Escalators
// ______________________________

// ______________________________
//
// spec
// ______________________________


SPECIAL(escalator)
{
  return FALSE;
}

// ______________________________
//
// processing funcs
// ______________________________

void EscalatorProcess(void)
{
  int i, dir;
  struct char_data *temp, *next;

  for (i = 0; i <= top_of_world; i++)
    if (world[i].func && world[i].func == escalator)
      for (temp = world[i].people; temp; temp = next) {
        next = temp->next_in_room;
        if (GET_LASTROOM(temp) > 0 || GET_LASTROOM(temp) < -3)
          GET_LASTROOM(temp) = -3;
        else if (GET_LASTROOM(temp) < 0)
          GET_LASTROOM(temp)++;
        else
          for (dir = NORTH; dir <= DOWN; dir++)
            if (world[i].dir_option[dir] &&
                world[i].dir_option[dir]->to_room > 0) {
              act("As you reach the end, you step off the escalator.",
                  FALSE, temp, 0, 0, TO_CHAR);
              act("$n steps off of the escalator.", TRUE, temp, 0, 0, TO_ROOM);
              char_from_room(temp);
              GET_LASTROOM(temp) = world[i].number;
              char_to_room(temp, world[i].dir_option[dir]->to_room);
              if (temp->desc)
                look_at_room(temp, 0);
              break;
            }
      }
}

// ______________________________
//
// Monorail
// ______________________________

// ______________________________
//
// utility funcs
// ______________________________

static void open_doors(int car, int to, int room, int from)
{
  if (!world[car].dir_option[to]) {
    world[car].dir_option[to] = new room_direction_data;
    memset((char *) world[car].dir_option[to], 0,
           sizeof(struct room_direction_data));
    world[car].dir_option[to]->to_room = room;
    world[car].dir_option[to]->to_room_vnum = world[room].number;
    world[car].dir_option[to]->barrier = 8;
    world[car].dir_option[to]->condition = 8;
    world[car].dir_option[to]->material = 8;
  }
  if (!world[room].dir_option[from]) {
    world[room].dir_option[from] = new room_direction_data;
    memset((char *) world[room].dir_option[from], 0,
           sizeof(struct room_direction_data));
    world[room].dir_option[from]->to_room = car;
    world[room].dir_option[from]->to_room_vnum = world[car].number;
    world[room].dir_option[from]->barrier = 8;
    world[room].dir_option[from]->condition = 8;
    world[room].dir_option[from]->material = 8;
  }
  send_to_room("The monorail stops and the doors open.\r\n", car);
  send_to_room("The monorail stops and the doors open.\r\n", room);
}

static void close_doors(int car, int to, int room, int from)
{
  if (world[car].dir_option[to]->keyword)
    delete [] world[car].dir_option[to]->keyword;
  if (world[car].dir_option[to]->general_description)
    delete [] world[car].dir_option[to]->general_description;
  delete world[car].dir_option[to];
  world[car].dir_option[to] = NULL;

  if (world[room].dir_option[from]->keyword)
    delete [] world[room].dir_option[from]->keyword;
  if (world[room].dir_option[from]->general_description)
    delete [] world[room].dir_option[from]->general_description;
  delete world[room].dir_option[from];
  world[room].dir_option[from] = NULL;

  send_to_room("The monorail doors close and it begins accelerating.\r\n", car);
  send_to_room("The monorail doors close and it begins accelerating.\r\n", room);
}

// ______________________________
//
// processing funcs
// ______________________________

void process_seatac_monorail(void)
{
  int carnum, roomnum, ind;
  static int where = 0;

  if (where >= 8)
    where = 0;

  ind = (int)(where / 4);

  carnum = real_room(seatac[ind].transport);
  roomnum = real_room(seatac[ind].room);

  switch (where) {
  case 0:
  case 4:
    send_to_room("Lights flash along the runway as the monorail approaches.\r\n",
                 roomnum);
    break;
  case 1:
  case 5:
    open_doors(carnum, seatac[ind].to, roomnum, seatac[ind].from);
    break;
  case 2:
  case 6:
    close_doors(carnum, seatac[ind].to, roomnum, seatac[ind].from);
    break;
  case 3:
    send_to_room("A voice announces, \"Next stop: Seattle.\"\r\n", carnum);
    break;
  case 7:
    send_to_room("A voice announces, \"Next stop: Tacoma.\"\r\n", carnum);
    break;
  }

  where++;
}

// ______________________________
//
// Ferries and busses
// ______________________________

/* Seattle/Tacoma Ferry */

struct transport_type seattle_ferry[2] =
  {
    {
      12620, SOUTH, 12613, NORTH
    },
    {
      12620, SOUTHEAST, 2007, NORTHWEST
    },
  };

void extend_walkway_st(int ferry, int to, int room, int from)
{
  if (!world[ferry].dir_option[to]) {
    world[ferry].dir_option[to] = new room_direction_data;
    memset((char *) world[ferry].dir_option[to], 0,
           sizeof(struct room_direction_data));
    world[ferry].dir_option[to]->to_room = room;
    world[ferry].dir_option[to]->to_room_vnum = world[room].number;
    world[ferry].dir_option[to]->barrier = 8;
    world[ferry].dir_option[to]->condition = 8;
    world[ferry].dir_option[to]->material = 8;
  }
  if (!world[room].dir_option[from]) {
    world[room].dir_option[from] = new room_direction_data;
    memset((char *) world[room].dir_option[from], 0,
           sizeof(struct room_direction_data));
    world[room].dir_option[from]->to_room = ferry;
    world[room].dir_option[from]->to_room_vnum = world[ferry].number;
    world[room].dir_option[from]->barrier = 8;
    world[room].dir_option[from]->condition = 8;
    world[room].dir_option[from]->material = 8;
  }
  send_to_room("The ferry docks at the pier, and extends its walkway.\r\n", room);
  send_to_room("The ferry docks at the pier, and extends its walkway.\r\n", ferry);
}
void contract_walkway_st(int ferry, int to, int room, int from)
{
  if (world[ferry].dir_option[to]->keyword)
    delete [] world[ferry].dir_option[to]->keyword;
  if (world[ferry].dir_option[to]->general_description)
    delete [] world[ferry].dir_option[to]->general_description;
  delete world[ferry].dir_option[to];
  world[ferry].dir_option[to] = NULL;
  if (world[room].dir_option[from]->keyword)
    delete [] world[room].dir_option[from]->keyword;
  if (world[room].dir_option[from]->general_description)
    delete [] world[room].dir_option[from]->general_description;
  delete world[room].dir_option[from];
  world[room].dir_option[from] = NULL;
  send_to_room("The walkway recedes, and the ferry departs.\r\n", room);
  send_to_room("The walkway recedes, and the ferry departs.\r\n", ferry);
}

void process_seattle_ferry(void)
{
  static int where = 0;
  int ferry, dock, ind;

  if (where >= 26)
    where = 0;

  ind = (where >= 13 ? 1 : 0);

  ferry = real_room(seattle_ferry[ind].transport);
  dock = real_room(seattle_ferry[ind].room);

  switch (where) {
  case 0:
    send_to_room("The ferry approaches, gliding across the bay towards "
                 "the dock.\r\n", dock);
    break;
  case 1:
  case 14:
    extend_walkway_st(ferry, seattle_ferry[ind].to, dock, seattle_ferry[ind].from);
    break;
  case 4:
  case 17:
    contract_walkway_st(ferry, seattle_ferry[ind].to, dock, seattle_ferry[ind].from);
    break;
  case 5:
    send_to_room("A voice announces through a rusting speaker, "
                 "\"Next stop: Tacoma.\"\r\n", ferry);
    break;
  case 13:
    send_to_room("The ferry approaches, gliding across the bay towards "
                 "the dock.\r\n", dock);
    break;
  case 18:
    send_to_room("A voice announces through a rusting speaker, "
                 "\"Next stop: Seattle.\"\r\n", ferry);
    break;
  }

  where++;
}

/* Hellhound Bus Spec
 * Bus from Seattle to Portland and back
 */



struct transport_type hellhound[2] =
  {
    {
      670, NORTHWEST, 32763, SOUTHEAST
    },
    {  670, SOUTHWEST, 14700, NORTHEAST
    },
  };

void open_busdoor(int bus, int to, int room, int from)
{
  if (!world[bus].dir_option[to]) {
    world[bus].dir_option[to] = new room_direction_data;
    memset((char *) world[bus].dir_option[to], 0,
           sizeof(struct room_direction_data));
    world[bus].dir_option[to]->to_room = room;
    world[bus].dir_option[to]->to_room_vnum = world[room].number;
    world[bus].dir_option[to]->barrier = 8;
    world[bus].dir_option[to]->condition = 8;
    world[bus].dir_option[to]->material = 8;
  }
  if (!world[room].dir_option[from]) {
    world[room].dir_option[from] = new room_direction_data;
    memset((char *) world[room].dir_option[from], 0,
           sizeof(struct room_direction_data));
    world[room].dir_option[from]->to_room = bus;
    world[room].dir_option[from]->to_room_vnum = world[bus].number;
    world[room].dir_option[from]->barrier = 8;
    world[room].dir_option[from]->condition = 8;
    world[room].dir_option[from]->material = 8;
  }
  send_to_room("The bus rolls up to the platform, and the door opens.\r\n", room);
  send_to_room("The bus rolls up to the platform, and the door opens.\r\n", bus);
}

void close_busdoor(int bus, int to, int room, int from)
{
  if (world[bus].dir_option[to]->keyword)
    delete [] world[bus].dir_option[to]->keyword;
  if (world[bus].dir_option[to]->general_description)
    delete [] world[bus].dir_option[to]->general_description;
  delete world[bus].dir_option[to];
  world[bus].dir_option[to] = NULL;
  if (world[room].dir_option[from]->keyword)
    delete [] world[room].dir_option[from]->keyword;
  if (world[room].dir_option[from]->general_description)
    delete [] world[room].dir_option[from]->general_description;
  delete world[room].dir_option[from];
  world[room].dir_option[from] = NULL;
  send_to_room("The bus door shuts, the driver yells \"^Wall aboard!^n\", and begins driving.\r\n", room);
  send_to_room("The bus door shuts, the driver yells \"^Wall aboard!^n\", and begins driving.\r\n", bus);
}

void process_hellhound_bus(void)
{
  static int where = 0;
  int bus, stop, ind;

  if (where >= 52)
    where = 0;

  ind = (where >= 26 ? 1 : 0);

  bus = real_room(hellhound[ind].transport);
  stop = real_room(hellhound[ind].room);

  switch (where) {
  case 0:
    send_to_room("The bus pulls into the garage, and slowly moves to the platform.\r\n", stop);
    break;
  case 1:
  case 28:
    open_busdoor(bus, hellhound[ind].to, stop, hellhound[ind].from);
    break;
  case 8:
  case 34:
    close_busdoor(bus, hellhound[ind].to, stop, hellhound[ind].from);
    break;
  case 23:
    send_to_room("The driver shouts from the front, \"Next stop: Portland\"\r\n", bus);
    break;
  case 26:
    send_to_room("The bus pulls into the garage, and slowly moves to the platform.\r\n", stop);
    break;
  case 49:
    send_to_room("The driver shouts from the front, \"Next stop: Seattle\".\r\n", bus);
    break;
  }
  where++;
}

//FILEPOINTER
//Downtown Portlan, 60th Street Hospital, Gresham, 60th Street back from Gresham
//Lightrail
struct transport_type lightrail[4] =
  {
    {
      14799, SOUTH, 14701, NORTH
    }
    , //Downtown Portlan
    { 14799, EAST, 14702, WEST }, //60thStreet/Hospital
    { 14799, WEST, 14711, EAST }, //Gresham
    { 14799, EAST, 14702, WEST }, //60thStreet/BackfromGresham
  };

void open_lightraildoor(int lightrail, int to, int room, int from)
{
  if (!world[lightrail].dir_option[to]) {
    world[lightrail].dir_option[to] = new room_direction_data;
    memset((char *) world[lightrail].dir_option[to], 0,
           sizeof(struct room_direction_data));
    world[lightrail].dir_option[to]->to_room = room;
    world[lightrail].dir_option[to]->to_room_vnum = world[room].number;
    world[lightrail].dir_option[to]->barrier = 8;
    world[lightrail].dir_option[to]->condition = 8;
    world[lightrail].dir_option[to]->material = 8;
  }
  if (!world[room].dir_option[from]) {
    world[room].dir_option[from] = new room_direction_data;
    memset((char *) world[room].dir_option[from], 0,
           sizeof(struct room_direction_data));
    world[room].dir_option[from]->to_room = lightrail;
    world[room].dir_option[from]->to_room_vnum = world[lightrail].number;
    world[room].dir_option[from]->barrier = 8;
    world[room].dir_option[from]->condition = 8;
    world[room].dir_option[from]->material = 8;
  }
  send_to_room("The incoming lightrail grinds to a halt and its doors slide open with a hiss.\r\n", room);
  send_to_room("The lightrail grinds to a halt and the doors hiss open.\r\n", lightrail);
}

void close_lightraildoor(int lightrail, int to, int room, int from)
{
  if (world[lightrail].dir_option[to]->keyword)
    delete [] world[lightrail].dir_option[to]->keyword;
  if (world[lightrail].dir_option[to]->general_description)
    delete [] world[lightrail].dir_option[to]->general_description;
  delete world[lightrail].dir_option[to];
  world[lightrail].dir_option[to] = NULL;
  if (world[room].dir_option[from]->keyword)
    delete [] world[room].dir_option[from]->keyword;
  if (world[room].dir_option[from]->general_description)
    delete [] world[room].dir_option[from]->general_description;
  delete world[room].dir_option[from];
  world[room].dir_option[from] = NULL;
  send_to_room("The lightrail's doors slide shut and a tone eminates around the platform, signaling its departure.", room);
  send_to_room("The lightrail's doors slide shut and a tone signals as it begins moving.\r\n", lightrail);
}

void process_lightrail_train(void)
{
  static int where = 0;
  int train, stop;
  static int ind = 0;
  if (where >= 40)
    where = 0;

  //  if (where <= 0)
  //    ind = 0;
  //  if (where >= 10)
  //    ind = 1;
  //  if (where >= 20)
  //    ind = 2;
  //  if (where >= 30)
  //    ind = 3;

  train = real_room(lightrail[ind].transport);
  stop = real_room(lightrail[ind].room);

  switch (where) {
    //Downtown Stop Stuff
  case 39:
    send_to_room("An LCD Panel Flashes: \"Next Stop: Downtown Portland\".\r\n", train);
    break;
  case 0:
    send_to_room("The lightrail emits a loud grind as it brakes into the station.\r\n", stop);
    break;
  case 1:
    open_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    break;
  case 5:
    close_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    ind = 1;
    break;
    //60th Stop Stuff (1)
  case 9:
    send_to_room("An LCD Panel Flashes: \"Next Stop: 60th Street\".\r\n", train);
    break;
  case 10:
    send_to_room("The lightrail emits a loud grind as it brakes into the station.\r\n", stop);
    break;
  case 11:
    open_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    break;
  case 15:
    close_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    ind = 2;
    break;
    //Gresham Stop Stuff
  case 19:
    send_to_room("An LCD Panel Flashes: \"Next Stop: Gresham\".\r\n", train);
    break;
  case 20:
    send_to_room("The lightrail emits a loud grind as it brakes into the station.\r\n", stop);
    break;
  case 21:
    open_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    break;
  case 25:
    close_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    ind = 3;
    break;
    //To60th
  case 29:
    send_to_room("An LCD Panel Flashes: \"Next Stop: 60th Street\".\r\n", train);
    break;
  case 30:
    send_to_room("The lightrail emits a loud grind as it brakes into the station.\r\n", stop);
    break;
  case 31:
    open_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    break;
  case 35:
    close_lightraildoor(train, lightrail[ind].to, stop, lightrail[ind].from);
    ind = 0;
    break;
  }
  where++;
}


struct transport_type tacsea[2] =
  {
    {
      2099, SOUTH, 2007, NORTH
    },
    { 2099, NORTH, 14500, SOUTH }
  };

void extend_walkway(int ferry, int to, int room, int from)
{
  if (!world[ferry].dir_option[to]) {
    world[ferry].dir_option[to] = new room_direction_data;
    memset((char *) world[ferry].dir_option[to], 0,
           sizeof(struct room_direction_data));
    world[ferry].dir_option[to]->to_room = room;
    world[ferry].dir_option[to]->to_room_vnum = world[room].number;
    world[ferry].dir_option[to]->barrier = 8;
    world[ferry].dir_option[to]->condition = 8;
    world[ferry].dir_option[to]->material = 8;
  }
  if (!world[room].dir_option[from]) {
    world[room].dir_option[from] = new room_direction_data;
    memset((char *) world[room].dir_option[from], 0,
           sizeof(struct room_direction_data));
    world[room].dir_option[from]->to_room = ferry;
    world[room].dir_option[from]->to_room_vnum = world[ferry].number;
    world[room].dir_option[from]->barrier = 8;
    world[room].dir_option[from]->condition = 8;
    world[room].dir_option[from]->material = 8;
  }
  send_to_room("The ferry docks, and the walkway extends.\r\n", room);
  send_to_room("The ferry docks, and the walkway extends.\r\n", ferry);
}

void contract_walkway(int ferry, int to, int room, int from)
{
  if (world[ferry].dir_option[to]->keyword)
    delete [] world[ferry].dir_option[to]->keyword;
  if (world[ferry].dir_option[to]->general_description)
    delete [] world[ferry].dir_option[to]->general_description;
  delete world[ferry].dir_option[to];
  world[ferry].dir_option[to] = NULL;
  if (world[room].dir_option[from]->keyword)
    delete [] world[room].dir_option[from]->keyword;
  if (world[room].dir_option[from]->general_description)
    delete [] world[room].dir_option[from]->general_description;
  delete world[room].dir_option[from];
  world[room].dir_option[from] = NULL;
  send_to_room("The walkway recedes, and the ferry departs.\r\n", room);
  send_to_room("The walkway recedes, and the ferry departs.\r\n", ferry);
}

void process_seatac_ferry(void)
{
  static int where = 0;
  int ferry, dock, ind;

  if (where >= 26)
    where = 0;

  ind = (where >= 13 ? 1 : 0);

  ferry = real_room(tacsea[ind].transport);
  dock = real_room(tacsea[ind].room);

  switch (where) {
  case 0:
    send_to_room("The ferry approaches, gliding across the bay towards "
                 "the dock.\r\n", dock);
    break;
  case 1:
  case 14:
    extend_walkway(ferry, tacsea[ind].to, dock, tacsea[ind].from);
    break;
  case 4:
  case 17:
    contract_walkway(ferry, tacsea[ind].to, dock, tacsea[ind].from);
    break;
  case 5:
    send_to_room("A voice announces through a rusting speaker, "
                 "\"Next stop: Bradenton.\"\r\n", ferry);
    break;
  case 13:
    send_to_room("The ferry approaches, gliding across the bay towards "
                 "the dock.\r\n", dock);
    break;
  case 18:
    send_to_room("A voice announces through a rusting speaker, "
                 "\"Next stop: Tacoma.\"\r\n", ferry);
    break;
  }

  where++;
}

// ______________________________
//
// external interface funcs
// ______________________________

void TransportInit()
{
  int i;

  init_elevators();

  for (i = 0; i < NUM_SEATAC_STATIONS; i++) {
    if (real_room(seatac[i].room) < 0) {
      log("--Could not find SeaTac monorail room #%d -- disabling",
          seatac[i].room);
      enable_seatac_monorail = false;
      break;
    } else if (real_room(seatac[i].transport) < 0) {
      log("--Could not find SeaTac monorail room #%d -- disabling",
          seatac[i].transport);
      enable_seatac_monorail = false;
      break;
    }
  }

  // add ferry reality check here
}

void MonorailProcess(void)
{
//  if (enable_seatac_monorail)
//    process_seatac_monorail();

//  process_seattle_ferry();
// process_seatac_ferry();
//  process_hellhound_bus();
//  process_lightrail_train();
}

void TransportEnd()
{}

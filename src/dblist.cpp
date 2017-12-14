//  file: dblist.cc
//  authors: Chris Dickey, Andrew Hynek
//  purpose: contains the ObjList functions
//  Copyright (c) 1996 by Chris Dickey,
//  some parts Copyright (c) 1998 by Andrew Hynek

#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "structs.h"
#include "awake.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "dblist.h"
#include "handler.h"

// extern vars
extern class helpList Help;
extern class helpList WizHelp;
//extern struct room_data *world;

// extern funcs
extern void print_object_location(int, struct obj_data *, struct char_data *, int);

int objList::PrintList(struct char_data *ch, const char *arg)
{
  register nodeStruct<struct obj_data *> *temp = head;
  register int num = 0;

  for (;temp; temp = temp->next)
    if (temp->data && CAN_SEE_OBJ(ch, temp->data)
        && isname((char *)arg, temp->data->text.keywords))
      print_object_location(++num, temp->data, ch, TRUE);

  return num;
}

void objList::Traverse(void (*func)(struct obj_data *))
{
  for (nodeStruct<struct obj_data *> *temp = head; temp; temp = temp->
       next)
    func(temp->data);
}

// this function searches through the list and returns a count of objects
// with the specified virtual number.
int objList::CountObj(int num)
{
  int counter = 0;
  register nodeStruct<struct obj_data *> *temp;
  for (temp = head; temp; temp = temp->next)
    if (num == GET_OBJ_RNUM(temp->data))
      counter++;

  return counter;
}

// this function searches through the list and returns a pointer to the
// object whose object rnum matches num
struct obj_data *objList::FindObj(int num)
{
  register nodeStruct<struct obj_data *> *temp;
  for (temp = head; temp; temp = temp->next)
    if (num == GET_OBJ_RNUM(temp->data))
      return temp->data;

  return NULL;
}

// this function searches through the list and returns a pointer to the
// object whose name matches 'name' and who is the 'num'th object in the
// list
struct obj_data *objList::FindObj(struct char_data *ch, char *name, int num)
{
  register nodeStruct<struct obj_data *> *temp = head;
  register int i = 0;

  while (temp && (i <= num))
  {
    if (isname(name, temp->data->text.keywords) &&
        CAN_SEE_OBJ(ch, temp->data) &&
        (++i == num))
      return temp->data;
    temp = temp->next;
  }

  return NULL;
}

// this function updates pointers to this particular prototype--necessary
// for OLC so objects on the mud get updated with the correct values
void objList::UpdateObjs(const struct obj_data *proto, int rnum)
{
  static nodeStruct<struct obj_data *> *temp;
  static struct obj_data old;

  for (temp = head; temp; temp = temp->next)
  {
    if (temp->data->item_number == rnum) {
      old = *temp->data;
      *temp->data = *proto;
      temp->data->in_room = old.in_room;
      temp->data->item_number = rnum;
      temp->data->carried_by = old.carried_by;
      temp->data->worn_by = old.worn_by;
      temp->data->worn_on = old.worn_on;
      temp->data->in_obj = old.in_obj;
      temp->data->contains = old.contains;
      temp->data->next_content = old.next_content;
      temp->data->obj_flags.condition = old.obj_flags.condition;
      temp->data->restring = old.restring;
      temp->data->photo = old.photo;
      if (temp->data->carried_by)
        affect_total(temp->data->carried_by);
      else if (temp->data->worn_by)
        affect_total(temp->data->worn_by);
    }
  }
}

// this function runs through the list and checks the timers of each
// object, extracting them if their timers hit 0
void objList::UpdateCounters(void)
{
  static nodeStruct<struct obj_data *> *temp, *next;

  for (temp = head; temp; temp = next) {
    next = temp->next;
    if (GET_OBJ_TYPE(temp->data) == ITEM_PROGRAM && GET_OBJ_VAL(temp->data, 0) == SOFT_EVALUATE) {
      if (!GET_OBJ_VAL(temp->data, 5)) {
        GET_OBJ_VAL(temp->data, 5) = time(0);
        GET_OBJ_VAL(temp->data, 6) = GET_OBJ_VAL(temp->data, 5);
      } else if (GET_OBJ_VAL(temp->data, 5) < time(0) - SECS_PER_REAL_DAY / 2 && !(temp->data->carried_by && IS_NPC(temp->data->carried_by))) {
        GET_OBJ_VAL(temp->data, 1) -= number(0, 3);
        GET_OBJ_VAL(temp->data, 5) = time(0);
        if (GET_OBJ_VAL(temp->data, 1) < 0)
          GET_OBJ_VAL(temp->data, 1) = 0;
      }
      continue;
    }
    if (GET_OBJ_TYPE(temp->data) == ITEM_WORKSHOP && GET_OBJ_VAL(temp->data, 2)) {
      struct char_data *ch;
      for (ch = world[temp->data->in_room].people; ch; ch = ch->next_in_room)
        if (PLR_FLAGGED(ch, PLR_PACKING)) {
          GET_OBJ_VAL(temp->data, 2)--;
          if (!GET_OBJ_VAL(temp->data, 2)) {
            if (GET_OBJ_VAL(temp->data, 1)) {
              send_to_char(ch, "You finish packing up %s.\r\n", temp->data->text.name);
              act("$n finishes packing up $o", FALSE, ch, 0, temp->data, TO_ROOM);
              GET_OBJ_VAL(temp->data, 1)--;
            } else {
              send_to_char(ch, "You finish setting up %s.\r\n", temp->data->text.name);
              act("$n finishes setting up $o", FALSE, ch, 0, temp->data, TO_ROOM);
              GET_OBJ_VAL(temp->data, 1)++;
            }
            PLR_FLAGS(ch).RemoveBit(PLR_PACKING);
          }
          break;
        }
      if (ch)
        continue;
      GET_OBJ_VAL(temp->data, 2) = 0;
    }
    /* anti-twink measure...no decay until there's no eq in it */
    if ( IS_OBJ_STAT(temp->data, ITEM_CORPSE) && GET_OBJ_VAL(temp->data, 4)
         && temp->data->contains != NULL )
      continue;

    if (IS_OBJ_STAT(temp->data, ITEM_CORPSE)) {
      if (GET_OBJ_TIMER(temp->data) > 1) {
        GET_OBJ_TIMER(temp->data)--;
      } else {
        if (temp->data->carried_by)
          act("$p decays in your hands.", FALSE, temp->data->carried_by,
              temp->data, 0, TO_CHAR);
        else if (temp->data->worn_by)
          act("$p decays in your hands.", FALSE, temp->data->worn_by,
              temp->data, 0, TO_CHAR);
        else if ((temp->data->in_room != NOWHERE) &&
                 (world[temp->data->in_room].people)) {
          act("$p crumbles into dust.", TRUE, world[temp->data->in_room].people,
              temp->data, 0, TO_ROOM);
          act("$p crumbles into dust.", TRUE, world[temp->data->in_room].people,
              temp->data, 0, TO_CHAR);
        }
        // here we make sure to remove all items from the object
        struct obj_data *next_thing, *temp2;
        for (temp2 = temp->data->contains; temp2; temp2 = next_thing) {
          next_thing = temp2->next_content;     /*Next in inventory */
          extract_obj(temp2);
        }
        next = temp->next;
        extract_obj(temp->data);
      }
    }
  }
}

// this function updates the objects in the list whose real numbers are
// greater than num--necessary for olc--but maybe obsolete once the new
// structures come into effect
void objList::UpdateNums(int num)
{
  register nodeStruct<struct obj_data *> *temp;
  // just loop through the list and update
  for (temp = head; temp; temp = temp->next)
    if (GET_OBJ_RNUM(temp->data) >= num)
      GET_OBJ_RNUM(temp->data)++;
}

/* this function goes through each object, and if it has a spec, calls it */
void objList::CallSpec()
{
  nodeStruct<struct obj_data *> *temp;

  for (temp = head; temp; temp = temp->next)
    if (GET_OBJ_SPEC(temp->data) != NULL)
      GET_OBJ_SPEC(temp->data) (NULL, temp->data, 0, "");
}

void objList::RemoveObjNum(int num)
{
  nodeStruct<struct obj_data *> *temp, *next;

  for (temp = head; temp; temp = next) {
    next = temp->next;

    if (GET_OBJ_RNUM(temp->data) == num) {
      if (temp->data->carried_by)
        act("$p disintegrates.", FALSE, temp->data->carried_by,
            temp->data, 0, TO_CHAR);
      else if (temp->data->worn_by)
        act("$p disintegrates.", FALSE, temp->data->carried_by,
            temp->data, 0, TO_CHAR);
      else if (temp->data->in_room != NOWHERE && world[temp->data->in_room].people) {
        act("$p disintegrates.", TRUE, world[temp->data->in_room].people,
            temp->data, 0, TO_ROOM);
        act("$p disintegrates.", TRUE, world[temp->data->in_room].people,
            temp->data, 0, TO_CHAR);
      }
      extract_obj(temp->data);
    }
  }
}

void objList::RemoveQuestObjs(int id)
{
  nodeStruct<struct obj_data *> *temp, *next;

  for (temp = head; temp; temp = next) {
    next = temp->next;

    if (temp->data->obj_flags.quest_id == id)
      extract_obj(temp->data);
  }
}

void helpList::CreateIndex(bool wiz)
{
  DIR *directory;
  FILE *fl;
  struct dirent *dirEntry;

  if (wiz)
    directory = opendir(WIZHELP_FILE);
  else
    directory = opendir(HELP_PAGE_FILE);
  if (!directory) {
    log("Help directory not found. Continuing anyway.");
    return;
  }
  while ((dirEntry = readdir(directory))) {
    if (dirEntry->d_name[0] != '.') {
      struct help_index_data *help;
      sprintf(buf, "%s/%s", wiz ? WIZHELP_FILE : HELP_PAGE_FILE, dirEntry->d_name);
      help = new help_index_data;
      fl = fopen(buf, "r");
      get_line(fl, buf2);
      help->keyword = str_dup(buf2);
      fclose(fl);
      help->filename = str_dup(buf);
      if (wiz)
        WizHelp.ADD(help);
      else
        Help.ADD(help);
    }
  }
  closedir(directory);
}

void helpList::RebootIndex(bool wiz)
{
  nodeStruct<struct help_index_data *> *temp, *next;
  for (temp = head; temp; temp = next) {
    next = temp->next;
    if (wiz)
      WizHelp.RemoveItem(temp);
    else
      Help.RemoveItem(temp);
  }
  if (wiz)
    WizHelp.CreateIndex(TRUE);
  else
    Help.CreateIndex(FALSE);
}

void helpList::ListIndex(struct char_data *ch, char *letter)
{
  nodeStruct<struct help_index_data *> *temp;
  FILE *fl;
  int i = 0;

  sprintf(buf2, "The following help topics are available:\r\n");
  for (temp = head; temp; temp = temp->next)
  {
    fl = fopen(temp->data->filename, "r");
    get_line(fl, buf);
    fclose(fl);
    sprintf(buf2, "%s%30s", buf2, buf);
    if (++i == 3) {
      send_to_char(ch, buf2);
      send_to_char(ch, "\r\n");
      buf2[0] = '\0';
      i = 0;
    }
  }
}

bool helpList::FindTopic(char *help, char *arg)
{
  nodeStruct<struct help_index_data *> *temp;
  FILE *fl;
  for (temp = head; temp; temp = temp->next) {
    if (isname(arg, temp->data->keyword)) {
      fl = fopen(temp->data->filename, "r");
      if (fl) {
        get_line(fl, help);
        sprintf(buf, "^W%s^n\r\n", temp->data->keyword);
        while (get_line(fl, buf2)) {
          strcat(buf, buf2);
          strcat(buf, "\r\n");
        }
        strcpy(help, buf);
        fclose(fl);
        return TRUE;
      }
    }
  }
  return FALSE;
}


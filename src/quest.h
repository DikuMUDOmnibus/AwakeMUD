//  file: quest.h
//  author: Andrew Hynek
//  purpose: contains defines and structs for autoquest system
//  Copyright (c) 1997, 1998 by Andrew Hynek

#ifndef _quest_h_
#define _quest_h_

#define QUEST_NONE        0

#define QOL_JOHNSON       1
#define QOL_TARMOB_I      2
#define QOL_TARMOB_E      3
#define QOL_TARMOB_C      4
#define QOL_LOCATION      5
#define QOL_HOST          6

#define QML_LOCATION      1
#define QML_FOLQUESTER    2

#define QOO_JOHNSON       1
#define QOO_TAR_MOB       2
#define QOO_LOCATION      3
#define QOO_DSTRY_ONE     4
#define QOO_DSTRY_MANY    5
#define QOO_RETURN_PAY	  6
#define QOO_UPLOAD	  7

#define QMO_LOCATION      1
#define QMO_KILL_ONE      2
#define QMO_KILL_MANY     3
#define QMO_KILL_ESCORTEE 4

#define QMAX_OBJS                25
#define QMAX_MOBS                25

struct quest_om_data
{
  long vnum;
  int nuyen, karma;
  byte load;
  byte objective;
  int l_data, l_data2;
  int o_data;
};

struct quest_data
{
  long vnum, johnson;
  sh_int time, num_objs, num_mobs, min_rep, max_rep, s_time, e_time, s_room;
  int nuyen, karma, reward;
  struct quest_om_data *obj;
  struct quest_om_data *mob;
  char *intro;
  char *decline;
  char *quit;
  char *finish;
  char *info;
  char *s_string;
  char *e_string;
  char *done;

  quest_data() :
      johnson(-1), time(0), num_objs(0), num_mobs(0), min_rep(0),
      max_rep(0), s_time(0), e_time(0), s_room(0), nuyen(0),
      karma(0), reward(-1), obj(NULL), mob(NULL), intro(NULL),
      decline(NULL), quit(NULL), finish(NULL), info(NULL),
      s_string(NULL), e_string(NULL), done(NULL)
  {}
}
;

#define CMD_NONE  0
#define CMD_QUIT  1
#define CMD_DONE  2
#define CMD_START  3
#define CMD_YES  4
#define CMD_NO  5

#endif

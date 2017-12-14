/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _spells_h_
#define _spells_h_

/* Most of the contents of the original spells.h have been moved to
*  awake.h --Dunk
*/


struct attack_hit_type
{
  char *singular;
  char *plural;
  char *different;
};

int find_skill_num(char *name);

void mag_objectmagic(struct char_data *ch, struct obj_data *obj, char *argument);

void mob_cast(struct char_data *ch, struct char_data *tch,
              struct obj_data *tobj, int spellnum, int level);

#endif

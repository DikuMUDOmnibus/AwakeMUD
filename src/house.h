#ifndef _house_h_
#define _house_h_

#define HOUSE_PRIVATE 0
#define MAX_GUESTS 10

#include "types.h"
#include "bitfield.h"

struct house_control_rec
{
  vnum_t vnum;                 /* vnum of this house           */
  vnum_t atrium;               /* vnum of atrium               */
  vnum_t key;
  dir_t exit_num;             /* direction of house's exit    */
  int mode;                    /* mode of ownership            */
  long owner;                  /* idnum of house's owner       */
  long date;                   /* date rent is payed to        */
  int num_of_guests;           /* how many guests for house    */
  long guests[MAX_GUESTS];     /* idnums of house's guests     */
  char *name;  
  struct house_control_rec *next;  

  house_control_rec() :
    vnum(NOWHERE), atrium(NOWHERE), key(0), owner(0), num_of_guests(0), name(NULL), next(NULL)
  {}   
};

struct landlord 
{
  vnum_t vnum;     /* vnum of landlord mob */
  Bitfield race;   /* Races landlord will NOT deal with */
  int basecost;    /* Base cost for low lifestyle rooms */  
  int num_room;
  struct house_control_rec *rooms;
  struct landlord *next;

  landlord() :
    vnum(NOWHERE), race(0), rooms(NULL), next(NULL)
  {}
};

#define NOELF	1
#define NOORK	2
#define NOTROLL	3
#define NOHUMAN	4
#define NODWARF	5

#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
                            world[room].dir_option[dir]->to_room : NOWHERE)

bool House_can_enter(struct char_data *ch, vnum_t house);
void House_listrent(struct char_data *ch, vnum_t vnum);
void House_boot(void);
void House_save_all(void);
void House_crashsave(vnum_t vnum);
void House_list_guests(struct char_data *ch, struct house_control_rec *i, int quiet);

#endif

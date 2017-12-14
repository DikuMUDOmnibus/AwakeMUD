/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _handler_h_
#define _handler_h_

/* handling the affected-structures */
void  affect_total_veh(struct veh_data *veh);
void    affect_total(struct char_data *ch);
void    affect_modify(struct char_data *ch,
                      byte loc,
                      sbyte mod,
                      const Bitfield &bitv,
                      bool add
                       );
void    affect_to_char(struct char_data *ch, struct affected_type *af);
void    affect_remove(struct char_data *ch, struct affected_type *af, int message);
void    affect_from_char(struct char_data *ch, sh_int type);
int             affected_by_spell(struct char_data *ch, sh_int type);
void    affect_join(struct char_data *ch, struct affected_type *af,
                    bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
void    affect_veh(struct veh_data *veh, byte loc, sbyte mod);
/* utility */
char *money_desc(int amount);
struct obj_data *create_money(int amount);
int isname(char *str, char *namelist);
char    *fname(char *namelist);
int     get_number(char **name);
int from_ip_zone(int vnum);
struct remem *found_mem(struct remem *mem, struct char_data *ch);
int recog(struct char_data *ch, struct char_data *i, char *name);
void phone_check();
/* ******** objects *********** */

void    obj_to_char(struct obj_data *object, struct char_data *ch);
void    obj_from_char(struct obj_data *object);

void    equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);

struct obj_data *get_obj_in_list(char *name, struct obj_data *list);
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj(char *name);
struct obj_data *get_obj_num(int nr);

void    obj_to_veh(struct obj_data *object, struct veh_data *veh);
void    obj_to_room(struct obj_data *object, int room);
void    obj_from_room(struct obj_data *object);
void    obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void    obj_from_obj(struct obj_data *obj);
void    object_list_new_owner(struct obj_data *list, struct char_data *ch);

void obj_from_cyberware(struct obj_data *cyber);
void obj_from_bioware(struct obj_data *bio);
void obj_to_cyberware(struct obj_data *object, struct char_data *ch);
void obj_to_bioware(struct obj_data *object, struct char_data *ch);

void    extract_obj(struct obj_data *obj);
void    extract_veh(struct veh_data *veh);
void    extract_icon(struct matrix_icon *icon);
void    clear_queue(struct veh_data *veh);
/* ******* characters ********* */

struct char_data *get_char_room(char *name, int room);
struct char_data *get_char_num(int nr);
struct char_data *get_char(char *name);

void    char_from_room(struct char_data *ch);
void    char_to_room(struct char_data *ch, long room);
void    extract_char(struct char_data *ch);
void    char_to_veh(struct veh_data *veh, struct char_data *ch);
void veh_from_room(struct veh_data *veh);
void veh_to_room(struct veh_data *veh, int room);
int veh_skill(struct char_data *ch, struct veh_data *veh);
void	icon_from_host(struct matrix_icon *icon);
void	icon_to_host(struct matrix_icon *icon, vnum_t to_host);

/* find if character can see */
struct char_data *get_char_room_vis(struct char_data *ch, char *name);
struct char_data *get_player_vis(struct char_data *ch, char *name, int inroom);
struct char_data *get_char_vis(struct char_data *ch, char *name);
struct char_data *get_char_veh(struct char_data *ch, char *name, struct veh_data *veh);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name);
struct obj_data *get_object_in_equip_vis(struct char_data *ch,
          char *arg, struct obj_data *equipment[], int *j);

struct veh_data *get_veh_list(char *name, struct veh_data *list);

/* find all dots */

int     find_all_dots(char *arg);

#define FIND_INDIV      0
#define FIND_ALL        1
#define FIND_ALLDOT     2


/* Generic Find */

int     generic_find(char *arg, int bitvector, struct char_data *ch,
                     struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM     1
#define FIND_CHAR_WORLD    2
#define FIND_OBJ_INV       4
#define FIND_OBJ_ROOM      8
#define FIND_OBJ_WORLD    16
#define FIND_OBJ_EQUIP    32

/* prototypes from fight.c */
void    set_fighting(struct char_data *ch, struct char_data *victim, ...);
void    set_fighting(struct char_data *ch, struct veh_data *victim);
void    stop_fighting(struct char_data *ch);
void    stop_follower(struct char_data *ch);
void    hit(struct char_data *ch, struct char_data *victim, int type);
void    forget(struct char_data *ch, struct char_data *victim);
void    remember(struct char_data *ch, struct char_data *victim);
bool    damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype,
               bool is_physical);
bool    skill_message(int dam, struct char_data *ch, struct char_data *vict,
                      int attacktype);

void    vcombat(struct char_data *ch, struct veh_data *veh);
void    vram(struct veh_data *veh, struct char_data *ch, struct veh_data *tveh);
void    process_vcombat(struct veh_data *veh);
void    chkdmg(struct veh_data * veh);
void    disp_mod(struct veh_data *veh, struct char_data *ch, int i);
void    stop_chase(struct veh_data *veh);
#endif

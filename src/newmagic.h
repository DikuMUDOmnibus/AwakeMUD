// newmagic.h  -- header for newmagic.cc
//                                           -Chris 12/95
#ifndef _newmagic_h_
#define _newmagic_h_

#define COMBAT 0
#define DETECT 1
#define HEALTH 2
#define ILLUSI 3
#define MANIPU 4

#define CASTER 0
#define TOUCH  1
#define RANGED 2
#define AREA   3

#define LIGHT 1
#define MODERATE 2
#define SERIOUS 3
#define DEADLY 4

#define SPELL_TARGET_CASTER     0
#define SPELL_TARGET_TOUCH      1
#define SPELL_TARGET_RANGE      2
#define SPELL_TARGET_AREA       3

#define SPELL_CATEGORY_COMBAT           0
#define SPELL_CATEGORY_DETECTION        1
#define SPELL_CATEGORY_HEALTH           2
#define SPELL_CATEGORY_ILLUSION         3
#define SPELL_CATEGORY_MANIPULATION     4

#define SPELL_ELEMENT_NONE                      0
#define SPELL_ELEMENT_ACID                      1
#define SPELL_ELEMENT_AIR                       2
#define SPELL_ELEMENT_EARTH             3
#define SPELL_ELEMENT_FIRE                      4
#define SPELL_ELEMENT_ICE                       5
#define SPELL_ELEMENT_LIGHTNING         6
#define SPELL_ELEMENT_WATER             7

// for door/obj damaging
#define DAMOBJ_NONE                     0
#define DAMOBJ_ACID                     1
#define DAMOBJ_AIR                      2
#define DAMOBJ_EARTH            3
#define DAMOBJ_FIRE                     4
#define DAMOBJ_ICE                      5
#define DAMOBJ_LIGHTNING        6
#define DAMOBJ_WATER            7
#define DAMOBJ_EXPLODE          8
#define DAMOBJ_PROJECTILE       9
#define DAMOBJ_CRUSH            10
#define DAMOBJ_SLASH            11
#define DAMOBJ_PIERCE           12
#define DAMOBJ_MANIPULATION     32

// typedefs
typedef struct char_data char_t;
typedef struct obj_data obj_t;
typedef unsigned char u_char;

typedef struct spell_struct
{
  u_char physical;
  u_char category;
  u_char target;
  sh_int drain;
  sh_int damage;    // damage code for combat spells
}
spell_a;

#ifndef _newmagic_cc_
// function prototypes
extern spell_t *find_spell(char_t *ch, char *name);
extern int spell_bonus(char_t *ch, spell_t *spell);
void check_spell_drain( struct char_data *ch, struct spell_data *spell );
#else
bool process_combat_target(char_t *, spell_t *, char *, int);
void process_combat_spell(char_t *, char_t *, obj_t *, spell_t *, int);
bool process_detection_target(char_t *, spell_t *, char *, int);
void process_detection_spell(char_t *, char_t *, obj_t *, spell_t *, int);
int process_health_target(char_t *, spell_t *, char *, int);
void process_health_spell(char_t *, char_t *, int level, spell_t *, int);
bool process_illusion_target(char_t *, spell_t *, char *, int);
void process_illusion_spell(char_t *, char_t *, obj_t *, spell_t *, int);
bool process_manipulation_target(char_t *, spell_t *, char *, int);
void process_manipulation_spell(char_t *, char_t *, obj_t *, spell_t *, int);
void elemental_damage(char_t *ch, char_t *vict, spell_t *spell, int force, int success);
spell_t *find_spell(char_t *ch, char *name);
int spell_bonus(char_t *ch, spell_t *spell);
void affect_update(void);
int mag_can_see(struct char_data *ch, int id);
void sustain_spell(int force, struct char_data * ch, struct char_data *
                   victim, spell_t *spell, int level);
void mob_cast(struct char_data * ch, struct char_data * tch, struct
              obj_data * tobj, int spellnum, int level);
int totem_bonus(struct char_data *ch, struct spell_data *spell);
int foci_bonus(struct char_data *ch, struct spell_data *spell,
               int force, bool fCast);
int magic_pool_bonus(struct char_data *ch, struct spell_data *spell,
                     int force, bool fCast);

#endif

#endif

#ifndef _mag_create_h_
#define _mag_create_h_


#define CH              d->character
#define SPELL           d->edit_spell
#define CLS(ch)         send_to_char("\033[H\033[J", ch)

#define SEDIT_CONFIRM_EDIT              0
#define SEDIT_MAIN_MENU                 1
#define SEDIT_CONFIRM_SAVESTRING        2
#define SEDIT_EDIT_NAME                 3
#define SEDIT_EDIT_TYPE                 4
#define SEDIT_EDIT_CATEGORY             5
#define SEDIT_EDIT_FORCE                6
#define SEDIT_EDIT_TARGET               7
#define SEDIT_EDIT_COMBAT               8
#define SEDIT_EDIT_DETECTION            9
#define SEDIT_EDIT_HEALTH               10
#define SEDIT_EDIT_ILLUSION             11
#define SEDIT_EDIT_MANIPULATION         12
#define SEDIT_EDIT_BASE_DAMAGE          13

#endif

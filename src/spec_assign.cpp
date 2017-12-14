/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>

#include "structs.h"
#include "awake.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

void ASSIGNMOB(int mob, SPECIAL(fname));

/* arrays for trainers/teachers/adepts/masters */

/* order: vnum, attributes, 1 - newbie trainer (else 0)     */
/* to have multiple attributes, separate them with |      */
train_t trainers[] = {
                       { 2506, TSTR, 0 },
                       { 4255, TBOD, 0 },
                       { 60500, TBOD | TQUI | TSTR | TCHA | TINT | TWIL, 1 },
                       { 17104, TBOD | TQUI | TSTR, 0 },
                       { 9413, TCHA | TINT | TWIL, 0 },
                       { 0, 0, 0 } /* this MUST be last */
                     };

/* order: vnum, skill1, skill2, skill3, skill4, skill5, learn message, level  */
/* see spells.h for the list of skills.        */
/* don't have them teach anything above SKILL_ELF_ETTIQUETTE (except SKILL_CLIMBING)*/
/* if they teach < 5 skills, put a 0 in the remaining skill slots    */
/* available levels are NEWBIE (4), AMATEUR (6/7), and ADVANCED (10/11)   */
teach_t teachers[] = {
                       { 2508, SKILL_BIOTECH, 0, 0, 0, 0, 0, 0, 0, "After hours of medical research and instruction, you begin "
                         "to\r\nunderstand more of the basic biotech procedures.\r\n", NEWBIE },
                       { 2701, SKILL_ATHLETICS, SKILL_STEALTH, SKILL_UNARMED_COMBAT, SKILL_EDGED_WEAPONS,
                         SKILL_WHIPS_FLAILS, SKILL_PROJECTILES, SKILL_THROWING_WEAPONS, 0,
                         "Toh Li gives you the workout of your life, but you come out more learned.", ADVANCED },
                       { 3722, SKILL_ATHLETICS, SKILL_RIFLES, SKILL_PISTOLS, 0, 0, 0, 0, 0,
                         "After hours of study and physical practice, you feel like you've "
                         "learned\r\nsomething.\r\n", AMATEUR },
                       { 4101, SKILL_SHOTGUNS, SKILL_PISTOLS, SKILL_RIFLES, SKILL_SMG, SKILL_ASSAULT_RIFLES, 0, 0, 0,
                         "After hours of study and target practice, you feel like you've "
                         "learned\r\nsomething.\r\n", ADVANCED },
                       { 4102, SKILL_CLUBS, SKILL_EDGED_WEAPONS, SKILL_POLE_ARMS, SKILL_WHIPS_FLAILS,
                         SKILL_PILOT_CAR, 0, 0, 0, "After hours of study and practice, you feel like you've "
                         "learned\r\nsomething.\r\n", AMATEUR },
                       { 4103, SKILL_SHOTGUNS, SKILL_PISTOLS, SKILL_RIFLES, SKILL_SMG, SKILL_ASSAULT_RIFLES,
                         SKILL_MACHINE_GUNS, SKILL_MISSILE_LAUNCHERS, SKILL_ASSAULT_CANNON,
                         "After hours of study and target practice, you feel like you've "
                         "learned\r\nsomething.\r\n", ADVANCED },
                       { 4104, SKILL_CLUBS, SKILL_EDGED_WEAPONS, SKILL_POLE_ARMS, SKILL_WHIPS_FLAILS, 0, 0, 0,
                         0, "After hours of study and melee practice, you feel like you've "
                         "learned\r\nsomething.\r\n", ADVANCED },
                       { 4250, SKILL_SORCERY, SKILL_MAGICAL_THEORY, SKILL_CONJURING, SKILL_AURA_READING, 0, 0, 0, 0,
                         "After hours of study and magical practice, you feel like you've "
                         "learned\r\nsomething.\r\n", ADVANCED },
                       { 4251, SKILL_SHAMANIC_STUDIES, SKILL_CONJURING, SKILL_SORCERY, SKILL_AURA_READING, 0, 0, 0, 0,
                         "After hours of study and magical practice, you feel like you've "
                         "learned\r\nsomething.\r\n", ADVANCED },
                       { 4257, SKILL_ASSAULT_RIFLES, SKILL_TASERS, 0, 0, 0, 0, 0, 0,
                         "After hours of study and weapon practice, you feel like you've "
                         "learned\r\nsomething.\r\n", AMATEUR },
                       { 60501, SKILL_SORCERY, SKILL_CONJURING, SKILL_SHAMANIC_STUDIES,
                         SKILL_MAGICAL_THEORY, SKILL_AURA_READING, 0, 0, 0, "After hours of study and practice, you "
                         "feel like you've learned\r\nsomething.\r\n", NEWBIE },
                       { 60502, SKILL_STEALTH, SKILL_ATHLETICS, SKILL_COMPUTER, SKILL_BR_COMPUTER,
                         SKILL_ELECTRONICS, SKILL_BIOTECH, SKILL_NEGOTIATION, 0, "After hours of study and practice, you "
                         "feel like you've learned\r\nsomething.\r\n", NEWBIE },
                       { 60503, SKILL_ENGLISH, SKILL_JAPANESE, SKILL_CHINESE, SKILL_KOREAN,
                         SKILL_SPERETHIEL, SKILL_SALISH, SKILL_SIOUX, SKILL_MAKAW, "Von Richter runs through basic "
                         "conjugation and sentance structure with you.\r\n", NEWBIE },
                       { 60504,
                         SKILL_PILOT_CAR, SKILL_PILOT_BIKE, SKILL_PILOT_TRUCK, SKILL_BR_BIKE,
                         SKILL_BR_DRONE, SKILL_BR_CAR, SKILL_BR_TRUCK, 0, "After hours of study and practice, "
                         "you feel like you've learned\r\nsomething.\r\n", NEWBIE },
                       { 60505, SKILL_EDGED_WEAPONS, SKILL_WHIPS_FLAILS, SKILL_POLE_ARMS, SKILL_CLUBS,
                         SKILL_PROJECTILES, SKILL_THROWING_WEAPONS, SKILL_UNARMED_COMBAT, SKILL_CYBER_IMPLANTS,
                         "After hours of study and practice, you feel like you've learned\r\nsomething.\r\n", NEWBIE },
                       { 60506, SKILL_PISTOLS, SKILL_RIFLES, SKILL_SHOTGUNS, SKILL_SMG, SKILL_ASSAULT_RIFLES,
                         SKILL_GUNNERY, SKILL_MACHINE_GUNS, SKILL_PROJECTILES,  "After hours of study and practice, "
                         "you feel like you've learned\r\nsomething.\r\n", NEWBIE },
                       { 3109, SKILL_GERMAN, SKILL_RUSSIAN, SKILL_SPANISH, SKILL_ITALIAN, SKILL_SIOUX,
                         0, 0, 0, "After a mixture of lectures, tests and conversation workshops, you gain proficiency in the language.", ADVANCED },
                       { 30700, SKILL_ENGLISH, SKILL_JAPANESE, SKILL_CHINESE, SKILL_KOREAN,
                         SKILL_SPERETHIEL, SKILL_SALISH, SKILL_ITALIAN, SKILL_NEGOTIATION, "Socrates shows you the intricities "
                         "of the language and you emerge with a greater understanding.\r\n", ADVANCED },
                       { 13499, SKILL_COMPUTER, SKILL_ELECTRONICS, SKILL_BR_COMPUTER, SKILL_BIOTECH, SKILL_PROGRAM_COMBAT, SKILL_PROGRAM_DEFENSIVE, SKILL_PROGRAM_CYBERTERM, 0, "Brian explains some concepts you had yet to understand "
                         "and\r\nyou feel like you've learned something.\r\n", ADVANCED },
                       { 14608, SKILL_PILOT_CAR, SKILL_GRENADE_LAUNCHERS, SKILL_MACHINE_GUNS,
                         SKILL_MISSILE_LAUNCHERS, SKILL_ASSAULT_CANNON, SKILL_PILOT_BIKE, SKILL_PILOT_TRUCK, SKILL_GUNNERY,
                         "Hal shows you a trick or two, and the rest just falls into place.\r\n", ADVANCED },
                       { 14638, SKILL_COMPUTER, SKILL_ELECTRONICS, SKILL_BR_COMPUTER, SKILL_BIOTECH, SKILL_BR_DRONE, SKILL_PROGRAM_SPECIAL, SKILL_PROGRAM_OPERATIONAL, SKILL_DATA_BROKERAGE, "Gargamel looks at you, hands you some parts, drops you a clue, and you get it.\r\n", ADVANCED },
                       { 7223, SKILL_UNARMED_COMBAT, SKILL_THROWING_WEAPONS, SKILL_STEALTH, SKILL_ATHLETICS,
                         SKILL_PROJECTILES, SKILL_CYBER_IMPLANTS, 0, 0,
                         "Shing gives you a work out like none you've ever had, "
                         "but\r\nyou feel like you've learned something.\r\n", AMATEUR },
                       { 24806, SKILL_SORCERY, SKILL_CONJURING, SKILL_MAGICAL_THEORY, SKILL_EDGED_WEAPONS, SKILL_ATHLETICS, SKILL_SHAMANIC_STUDIES, SKILL_NEGOTIATION, SKILL_AURA_READING, "Hermes imparts his wisdom upon you.\r\n", ADVANCED },
                       { 37500, SKILL_BR_CAR, SKILL_BR_BIKE, SKILL_BR_DRONE, SKILL_BR_TRUCK, 0, 0, 0, 0, "Marty shows you a few tricks of the "
                         "trade and you emerge more skilled than before.\r\n", AMATEUR },
                       { 0, 0, 0, 0, 0, 0, 0, 0, 0, "Report this bug!\r\n", 0 } // this MUST be last
                     };

/* order: vnum, perception, combat sense, inc. reflexes, killing hands, pain res.,
          1 - newbie trainer (else 0)        */
/* max ratings - astral perception: 1, combat sense: 3, inc. reflexes: 3,  */
/* killing hands: 4, pain resistance: 10        */
adept_t adepts[] = {
                     { 60507,{ 0, 1, 3, 1, 1, 4, 1, 1, 1, 3, 6, 6, 6, 6, 6, 6, 3, 1, 1, 1, 3, 10, 3, 3, 3, 1, 1, 6, 6, 6, 6 }, 1 },
                     { 7223, { 0, 1, 3, 0, 1, 4, 1, 0, 0, 3, 0, 6, 0, 0, 6, 0, 6, 0, 0, 0, 0, 10, 0, 0, 6, 1, 1, 2, 0, 6, 0 }, 0 },
                     { 2701, { 0, 1, 3, 0, 0, 4, 0, 1, 1, 0, 6, 0, 6, 6, 0, 6, 0, 1, 1, 1, 4, 0, 3, 3, 0, 0, 0, 4, 2, 0, 0 }, 0 },
                     { 3603, { 0, 0, 3, 1, 1, 4, 1, 1, 1, 3, 6, 0, 0, 0, 0, 6, 0, 1, 1, 1, 4, 10, 6, 0, 0, 0, 0, 4, 0, 0, 6 }, 0 },
                     { 20326,{ 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 4, 5, 2, 2, 2, 0, 1, 2, 1, 2, 6 }, 0 },
                     { 0, { 0, 0, 0, 0 }, 0 } // this MUST be last
                   };

/* order: vnum, first spell, first max force, second spell, second max force,
          1 - mage teacher (0 - shaman teacher), 1 - newbie teacher (else 0)  */
/* see spells.h for the list of spells        */
/* don't have them teach anything above SPELL_TOXIC_WAVE     */
/* neither force shouldn't exceed 6, as no one should have a magic rating > 6  */
/* if they don't teach two spells, put a 0 in the fourth and fifth slots  */
master_t masters[] = {
                       { 60508, SPELL_POWER_DART, 4,   SPELL_MANA_DART, 4, 1, 1 },
                       { 60509, SPELL_POWER_DART, 4,   SPELL_MANA_DART, 4, 0, 1 },
                       { 9413, SPELL_INVISIBILITY, 6, SPELL_ARMOR,     6, 0, 0 },
                       { 0, 0, 0, 0, 0 } // this MUST be last
                     };

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname))
{
  int rnum;

  if ((rnum = real_mobile(mob)) >= 0) {
    mob_index[rnum].sfunc = mob_index[rnum].func;
    mob_index[rnum].func = fname;
  } else
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(int obj, SPECIAL(fname))
{
  if (real_object(obj) >= 0)
    obj_index[real_object(obj)].func = fname;
  else
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNWEAPON(int weapon, WSPEC(fname))
{
  if (real_object(weapon) >= 0)
    obj_index[real_object(weapon)].wfunc = fname;
  else
    log("SYSERR: Attempt to assign spec to non-existant weapon #%d", weapon);
}

void ASSIGNROOM(int room, SPECIAL(fname))
{
  if (real_room(room) >= 0)
    world[real_room(room)].func = fname;
  else
    log("SYSERR: Attempt to assign spec to non-existant rm. #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  int i;

  SPECIAL(postmaster);
  SPECIAL(generic_guard);
  SPECIAL(receptionist);
  SPECIAL(cryogenicist);
  SPECIAL(teacher);
  SPECIAL(trainer);
  SPECIAL(adept_trainer);
  SPECIAL(spell_master);
  SPECIAL(puff);
  SPECIAL(janitor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user);
  SPECIAL(pike);
  SPECIAL(jeff);
  SPECIAL(captain);
  SPECIAL(rodgers);
  SPECIAL(delaney);
  SPECIAL(lone_star_park);
  SPECIAL(mugger_park);
  SPECIAL(gate_guard_park);
  SPECIAL(squirrel_park);
  SPECIAL(sick_ork);
  SPECIAL(mage_messenger);
  SPECIAL(malinalxochi);
  SPECIAL(adept_guard);
  SPECIAL(takehero_tsuyama);
  SPECIAL(bio_secretary);
  SPECIAL(pool);
  SPECIAL(harlten);
  SPECIAL(branson);
  SPECIAL(bio_guard);
  SPECIAL(worker);
  SPECIAL(dwarf);
  SPECIAL(elf);
  SPECIAL(george);
  SPECIAL(wendigo);
  SPECIAL(pimp);
  SPECIAL(prostitute);
  SPECIAL(heinrich);
  SPECIAL(ignaz);
  SPECIAL(waitress);
  SPECIAL(dracula);
  SPECIAL(pandemonia);
  SPECIAL(tunnel_rat);
  SPECIAL(saeder_guard);
  SPECIAL(atomix);
  SPECIAL(fixer);
  SPECIAL(hacker);
  SPECIAL(fence);
  SPECIAL(taxi);
  SPECIAL(crime_mall_guard);
  SPECIAL(doctor_scriptshaw);
  SPECIAL(huge_troll);
  SPECIAL(roots_receptionist);
  SPECIAL(aegnor);
  SPECIAL(purple_haze_bartender);
  SPECIAL(yukiya_dahoto);
  SPECIAL(smelly);
  //Clan SpecMobs
  SPECIAL(backstage_barkeep);
  SPECIAL(backstage_mechanic);
  SPECIAL(backstage_fixer);
  SPECIAL(backstage_russian);
  SPECIAL(terell_davis);
  SPECIAL(smiths_bouncer);
  ASSIGNMOB(65200, backstage_barkeep);
  ASSIGNMOB(65201, backstage_mechanic);
  ASSIGNMOB(65202, backstage_russian);
  ASSIGNMOB(65203, backstage_fixer);

  /* trainers */
  for (i = 0; trainers[i].vnum != 0; i++)
    ASSIGNMOB(trainers[i].vnum, trainer);

  /* teachers */
  for (i = 0; teachers[i].vnum != 0; i++)
    ASSIGNMOB(teachers[i].vnum, teacher);

  /* adept trainer */
  for (i = 0; adepts[i].vnum != 0; i++)
    ASSIGNMOB(adepts[i].vnum, adept_trainer);

  for (i = 0; masters[i].vnum != 0; i++)
    ASSIGNMOB(masters[i].vnum, spell_master);

  /* Personal rooms */
  ASSIGNMOB(1, puff);
  /* cab drivers */
  ASSIGNMOB(600, taxi);

  /* Immortal HQ */
  ASSIGNMOB(1002, janitor);
  ASSIGNMOB(1005, postmaster);

  ASSIGNMOB(1151, terell_davis);
  /* SWU */
  ASSIGNMOB(60526, postmaster);

  /* Various Tacoma */
  ASSIGNMOB(1823, aegnor);
  ASSIGNMOB(1832, fixer);
  ASSIGNMOB(1833, purple_haze_bartender);
  ASSIGNMOB(1900, postmaster);
  ASSIGNMOB(1902, receptionist);
  ASSIGNMOB(1916, generic_guard);

  /* Tacoma */
  ASSIGNMOB(2013, generic_guard);
  ASSIGNMOB(2020, hacker);
  ASSIGNMOB(2022, janitor);
  ASSIGNMOB(2023, fence);

  /* club penumbra */
  ASSIGNMOB(2112, generic_guard);
  ASSIGNMOB(2113, magic_user);
  ASSIGNMOB(2115, receptionist);

  /* Puyallup */
  ASSIGNMOB(2306, pike);
  ASSIGNMOB(2305, jeff);
  ASSIGNMOB(2403, atomix);

  /*Lone Star 17th Precinct*/
  ASSIGNMOB(3700, captain);
  ASSIGNMOB(3708, rodgers);
  ASSIGNMOB(3709, delaney);
  ASSIGNMOB(3703, generic_guard);
  ASSIGNMOB(3720, generic_guard);
  ASSIGNMOB(3829, janitor);
  ASSIGNMOB(3712, janitor);

  /*Seattle State Park*/
  ASSIGNMOB(4001, lone_star_park);
  ASSIGNMOB(4003, mugger_park);
  ASSIGNMOB(4008, janitor);
  ASSIGNMOB(4009, gate_guard_park);
  ASSIGNMOB(4012, squirrel_park);
  ASSIGNMOB(4013, sick_ork);
  ASSIGNMOB(4016, adept_guard);
  ASSIGNMOB(4019, takehero_tsuyama);

  /* Various - zone 116 */
  ASSIGNMOB(4100, postmaster);

  /*BioHyde Complex*/
  ASSIGNMOB(4202, bio_secretary);
  ASSIGNMOB(4206, harlten);
  ASSIGNMOB(4209, branson);
  ASSIGNMOB(4200, bio_guard);
  ASSIGNMOB(4203, worker);

  /*Titty Twister*/
  ASSIGNMOB(4500, wendigo);
  ASSIGNMOB(4501, pimp);
  ASSIGNMOB(4502, prostitute);
  ASSIGNMOB(4504, heinrich);
  ASSIGNMOB(4506, ignaz);
  ASSIGNMOB(4507, waitress);
  ASSIGNMOB(4508, dracula);
  ASSIGNMOB(4509, pandemonia);
  ASSIGNMOB(4513, tunnel_rat);

  /* Tarislar */
  ASSIGNMOB(4912, saeder_guard);

  /* Seattle */
  ASSIGNMOB(5100, generic_guard);
  ASSIGNMOB(5101, janitor);

  /* Neophytic Guild */
  ASSIGNMOB(8010, postmaster);
  ASSIGNMOB(60523, receptionist);

  /* Council Island */
  ASSIGNMOB(9202, doctor_scriptshaw);
  ASSIGNMOB(9410, huge_troll);

  /* Ork Underground */
  ASSIGNMOB(9913, receptionist);

  /* Bellevue */
  ASSIGNMOB(20342, receptionist);

  /* Crime Mall */
  ASSIGNMOB(10022, crime_mall_guard);

  /* Bradenton */
  ASSIGNMOB(14515, hacker);
  ASSIGNMOB(14516, fence);

  /* Portland */
  ASSIGNMOB(14605, receptionist);

  /* Mitsuhama */
  ASSIGNMOB(17112, yukiya_dahoto);

  /* Seattle 2.0 */
  ASSIGNMOB(30500, receptionist);

  /* Telestrian */
  ASSIGNMOB(18902, smelly);
  ASSIGNMOB(18910, receptionist);
  /* Smiths Pub */
  ASSIGNMOB(32784, postmaster);
  ASSIGNMOB(38027, smiths_bouncer);
  ASSIGNMOB(70775, receptionist);

}

/* assign special procedures to objects */

void assign_objects(void)
{
  SPECIAL(bank);
  SPECIAL(gen_board);
  SPECIAL(vendtix);
  SPECIAL(clock);
  SPECIAL(vending_machine);
  SPECIAL(hand_held_scanner);
  SPECIAL(anticoagulant);
  SPECIAL(rng);
  SPECIAL(toggled_invis);
  SPECIAL(desktop);

  ASSIGNOBJ(3, gen_board);  /* Rift's Board */
  ASSIGNOBJ(4, gen_board);  /* Pook's Board */
  ASSIGNOBJ(12, gen_board);  /* Dunk's Board */
  ASSIGNOBJ(22, gen_board);   /* old Changelogs */
  ASSIGNOBJ(26, gen_board);  /* RP Board */
  ASSIGNOBJ(28, gen_board);  /* Quest Board */
  ASSIGNOBJ(31, gen_board);  /* War Room Board */
  ASSIGNOBJ(50, gen_board);  /* Harl's Board */
  ASSIGNOBJ(66, gen_board);             /* Lofwyr's Board */
  ASSIGNOBJ(1006, gen_board);  /* Builder Board  */
  ASSIGNOBJ(1038, gen_board);  /* Coders Board */
  ASSIGNOBJ(1074, gen_board);  /* Administration Board */
  ASSIGNOBJ(65126, gen_board);
  ASSIGNOBJ(26150, gen_board);
  ASSIGNOBJ(1117, desktop);
  ASSIGNOBJ(1118, desktop);
  ASSIGNOBJ(1119, desktop);
  ASSIGNOBJ(1120, desktop);
  ASSIGNOBJ(1121, desktop);
  ASSIGNOBJ(1122, desktop);
  ASSIGNOBJ(1123, desktop);
  ASSIGNOBJ(1124, desktop);
  ASSIGNOBJ(1125, desktop);
  ASSIGNOBJ(1126, desktop);
  ASSIGNOBJ(1845, desktop);
  ASSIGNOBJ(1846, desktop);
  ASSIGNOBJ(1847, desktop);
  ASSIGNOBJ(1848, desktop);
  ASSIGNOBJ(1904, clock);  /* clock */
  ASSIGNOBJ(1946, bank);  /* atm */
  ASSIGNOBJ(2104, gen_board);     /* mortal board   */
  ASSIGNOBJ(2106, gen_board);      /* immortal board */
  ASSIGNOBJ(2107, gen_board);      /* freeze board   */
  ASSIGNOBJ(2108, bank);         /* atm  */
  ASSIGNOBJ(2506, anticoagulant);
  ASSIGNOBJ(3012, vendtix);
  ASSIGNOBJ(3114, gen_board);  /* Matrix Board */
  ASSIGNOBJ(4006, clock);
  ASSIGNOBJ(4216, clock);
  ASSIGNOBJ(8001, gen_board);  /* newbie board */
  ASSIGNOBJ(8002, gen_board);  /* newbie board */
  ASSIGNOBJ(8003, gen_board);  /* newbie board */
  ASSIGNOBJ(8004, gen_board);  /* newbie board */
  ASSIGNOBJ(8013, gen_board);  /* newbie board */
  ASSIGNOBJ(9329, vending_machine);
  ASSIGNOBJ(10108, vending_machine);
  ASSIGNOBJ(9406, hand_held_scanner);
  ASSIGNOBJ(12114, vending_machine);
  ASSIGNOBJ(29997, gen_board);  /* Loki's Board */
  ASSIGNOBJ(22445, bank);
  ASSIGNOBJ(14645, clock);
  ASSIGNOBJ(16234, clock);
  ASSIGNOBJ(18884, clock);
  ASSIGNOBJ(18950, bank);
  ASSIGNOBJ(14726, bank);
  ASSIGNOBJ(64986, clock);
  ASSIGNOBJ(64900, gen_board); /* RPE Board */
  ASSIGNOBJ(65207, gen_board); /* Backstage Group's Board */
  ASSIGNOBJ(4603, gen_board);
  ASSIGNOBJ(50301, toggled_invis);
  ASSIGNOBJ(50305, desktop);
  ASSIGNOBJ(8458, desktop);
  ASSIGNOBJ(8459, desktop);
  ASSIGNOBJ(65214, desktop);
  ASSIGNOBJ(4607, desktop);
  ASSIGNOBJ(35026, clock);
  ASSIGNOBJ(29340, bank);
  ASSIGNOBJ(42118, clock);
  ASSIGNOBJ(70703, bank);
  WSPEC(monowhip);

  ASSIGNWEAPON(660, monowhip);
  ASSIGNWEAPON(4905, monowhip);
  ASSIGNWEAPON(10011, monowhip);
}

/* assign special procedures to rooms */

void assign_rooms(void)
{
  SPECIAL(car_dealer);
  SPECIAL(oceansounds);
  SPECIAL(aztec_one);
  SPECIAL(escalator);
  SPECIAL(neophyte_entrance);
  SPECIAL(simulate_bar_fight);
  SPECIAL(crime_mall_blockade);
  SPECIAL(waterfall);
  SPECIAL(climb_down_junk_pile);
  SPECIAL(climb_up_junk_pile);
  SPECIAL(junk_pile_fridge);
  SPECIAL(slave);
  SPECIAL(roots_office);
  SPECIAL(circulation_fan);
  SPECIAL(traffic);
  SPECIAL(newbie_car);
  SPECIAL(auth_room);
  SPECIAL(room_damage_radiation);
  SPECIAL(bouncy_castle);
  SPECIAL(rpe_room);

  /* Limbo/God Rooms */
  ASSIGNROOM(8, oceansounds);
  ASSIGNROOM(9, oceansounds);
  ASSIGNROOM(15, roots_office);
  //ASSIGNROOM(5, room_damage_radiation);
  /* Traffic */
  ASSIGNROOM(2000, traffic);
  ASSIGNROOM(2001, traffic);
  ASSIGNROOM(2003, traffic);
  ASSIGNROOM(2010, traffic);
  ASSIGNROOM(2011, traffic);
  ASSIGNROOM(2018, traffic);
  ASSIGNROOM(2019, traffic);
  ASSIGNROOM(2021, traffic);
  ASSIGNROOM(2031, traffic);
  ASSIGNROOM(2033, traffic);
  ASSIGNROOM(2040, traffic);
  ASSIGNROOM(2046, traffic);
  ASSIGNROOM(2047, traffic);
  ASSIGNROOM(2048, traffic);
  ASSIGNROOM(2049, traffic);
  ASSIGNROOM(2050, traffic);
  ASSIGNROOM(2060, traffic);
  ASSIGNROOM(2061, traffic);
  ASSIGNROOM(2062, traffic);
  ASSIGNROOM(2063, traffic);
  ASSIGNROOM(2072, traffic);
  ASSIGNROOM(2084, traffic);
  ASSIGNROOM(30502, traffic);
  ASSIGNROOM(30503, traffic);
  ASSIGNROOM(30504, traffic);
  ASSIGNROOM(30505, traffic);
  ASSIGNROOM(30506, traffic);
  ASSIGNROOM(30507, traffic);
  ASSIGNROOM(30508, traffic);
  ASSIGNROOM(30509, traffic);
  ASSIGNROOM(30510, traffic);
  ASSIGNROOM(30511, traffic);
  ASSIGNROOM(30512, traffic);
  ASSIGNROOM(30513, traffic);
  ASSIGNROOM(30514, traffic);
  ASSIGNROOM(30515, traffic);
  ASSIGNROOM(30516, traffic);
  ASSIGNROOM(30517, traffic);
  ASSIGNROOM(30518, traffic);
  ASSIGNROOM(30519, traffic);
  ASSIGNROOM(30520, traffic);
  ASSIGNROOM(30521, traffic);
  ASSIGNROOM(30522, traffic);
  ASSIGNROOM(30523, traffic);
  ASSIGNROOM(30524, traffic);
  ASSIGNROOM(30525, traffic);
  ASSIGNROOM(30526, traffic);
  ASSIGNROOM(30527, traffic);
  ASSIGNROOM(30528, traffic);
  ASSIGNROOM(30529, traffic);
  ASSIGNROOM(30530, traffic);
  ASSIGNROOM(30531, traffic);
  ASSIGNROOM(30532, traffic);
  ASSIGNROOM(30533, traffic);
  ASSIGNROOM(30534, traffic);
  ASSIGNROOM(30535, traffic);
  ASSIGNROOM(30536, traffic);
  ASSIGNROOM(30537, traffic);
  ASSIGNROOM(30538, traffic);
  ASSIGNROOM(30539, traffic);
  ASSIGNROOM(30540, traffic);
  ASSIGNROOM(30541, traffic);
  ASSIGNROOM(30542, traffic);
  ASSIGNROOM(30543, traffic);
  ASSIGNROOM(30544, traffic);
  ASSIGNROOM(30545, traffic);
  ASSIGNROOM(30546, traffic);
  ASSIGNROOM(30547, traffic);
  ASSIGNROOM(30548, traffic);
  ASSIGNROOM(30549, traffic);
  ASSIGNROOM(30550, traffic);
  ASSIGNROOM(30551, traffic);
  ASSIGNROOM(30552, traffic);
  ASSIGNROOM(30553, traffic);
  ASSIGNROOM(30554, traffic);
  ASSIGNROOM(30555, traffic);
  ASSIGNROOM(30556, traffic);
  ASSIGNROOM(30557, traffic);
  ASSIGNROOM(30558, traffic);
  ASSIGNROOM(30559, traffic);
  ASSIGNROOM(30560, traffic);
  ASSIGNROOM(30561, traffic);
  ASSIGNROOM(30562, traffic);
  ASSIGNROOM(30563, traffic);
  ASSIGNROOM(30564, traffic);
  ASSIGNROOM(30565, traffic);
  ASSIGNROOM(30566, traffic);
  ASSIGNROOM(30567, traffic);
  ASSIGNROOM(30568, traffic);
  ASSIGNROOM(30569, traffic);
  ASSIGNROOM(30570, traffic);
  ASSIGNROOM(30571, traffic);
  ASSIGNROOM(30572, traffic);
  ASSIGNROOM(30573, traffic);
  ASSIGNROOM(30574, traffic);
  ASSIGNROOM(30575, traffic);
  ASSIGNROOM(30576, traffic);
  ASSIGNROOM(30577, traffic);
  ASSIGNROOM(30578, traffic);
  ASSIGNROOM(30579, traffic);
  ASSIGNROOM(30580, traffic);
  ASSIGNROOM(30581, traffic);
  ASSIGNROOM(30582, traffic);
  ASSIGNROOM(30583, traffic);
  ASSIGNROOM(30584, traffic);
  ASSIGNROOM(30585, traffic);
  ASSIGNROOM(30586, traffic);
  ASSIGNROOM(30587, traffic);
  ASSIGNROOM(30588, traffic);
  ASSIGNROOM(30589, traffic);
  ASSIGNROOM(30590, traffic);
  ASSIGNROOM(30591, traffic);
  ASSIGNROOM(30592, traffic);
  ASSIGNROOM(30593, traffic);
  ASSIGNROOM(30594, traffic);
  ASSIGNROOM(30595, traffic);
  ASSIGNROOM(30596, traffic);
  ASSIGNROOM(30597, traffic);
  ASSIGNROOM(30598, traffic);
  ASSIGNROOM(30599, traffic);
  ASSIGNROOM(30600, traffic);


  /* Carbanado */
  ASSIGNROOM(4477, waterfall);

  /* Aztechnology */
  ASSIGNROOM(7401, aztec_one);

  /* Neophytic guild */
  ASSIGNROOM(60585, neophyte_entrance);
  ASSIGNROOM(60586, newbie_car);
  ASSIGNROOM(60562, auth_room);

  /* Ork Underground */
  ASSIGNROOM(9978, simulate_bar_fight);

  /* Tacoma Mall */
  ASSIGNROOM(1904, escalator);
  ASSIGNROOM(1920, escalator);
  ASSIGNROOM(1923, escalator);
  ASSIGNROOM(1937, escalator);

  /* Crime Mall */
  ASSIGNROOM(10075, crime_mall_blockade);
  ASSIGNROOM(10077, crime_mall_blockade);

  /* Mr. BetterWrench Auto Repair */
  ASSIGNROOM(12246, climb_down_junk_pile);
  ASSIGNROOM(12248, junk_pile_fridge);
  ASSIGNROOM(12278, climb_up_junk_pile);

  ASSIGNROOM(1399, car_dealer);
  ASSIGNROOM(14798, car_dealer);
  ASSIGNROOM(14796, car_dealer);
  ASSIGNROOM(37505, car_dealer);
  ASSIGNROOM(37507, car_dealer);
  ASSIGNROOM(37509, car_dealer);
  ASSIGNROOM(37511, car_dealer);
  ASSIGNROOM(37513, car_dealer);

  /* Mitsuhama */
  ASSIGNROOM(17171, circulation_fan);

  /* Portland */
  ASSIGNROOM(18860, escalator);
  ASSIGNROOM(18859, escalator);

  /* Kuroda's Bouncy Castle */
  ASSIGNROOM(65075, bouncy_castle);
  ASSIGNROOM(35611, rpe_room);
}

/*
 * UGH! game logic - C API (C++20 library without dependencies, re/notes/rewrite-design.md).
 *
 * The whole game from a new game to its end: the level caption, the play (copter physics, passengers, enemies,
 * bonus items, water, rain, keyboard, random numbers), the end of a level, game over. No drawing and no sound: the
 * logic reports events for those. Nothing of the state can be set from outside.
 */
#ifndef UGH_LOGIC_H
#define UGH_LOGIC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_logic ugh_logic;

/** Loads the game data (assets/sim/ugh-data.ugd). NULL on failure, the reason in err. */
ugh_logic* ugh_logic_create(const char* data_path, char* err, size_t err_size);
void ugh_logic_destroy(ugh_logic* logic);

/** What a new game starts with. */
typedef struct {
    int players;            /* 1, or 2 for the team mode */
    int difficulty;         /* 0 easy, 1 medium, 2 hard */
    int first_level;        /* from 0 in the order of the mode */
    uint16_t random_seed[4];   /* the state of the random numbers */
    int rain_floor_row;     /* the row where raindrops start again (180 after the program start) */
} ugh_logic_settings;

/** A new game; 0 when a setting is out of range. */
int ugh_logic_new_game(ugh_logic* logic, const ugh_logic_settings* settings);

/** A scancode of the PC keyboard, between two frames. */
void ugh_logic_scancode(ugh_logic* logic, int scancode);

enum { UGH_LOGIC_CONTINUE = 0, UGH_LOGIC_GAME_OVER = 1, UGH_LOGIC_ALL_LEVELS_DONE = 2 };

/** One frame (70.086 Hz): UGH_LOGIC_CONTINUE while the game goes on. */
int ugh_logic_step(ugh_logic* logic);

/** What happened (ugh_logic_take_events). */
enum {
    UGH_LOGIC_EVENT_LEVEL_CAPTION = 1,   /* the caption of a level shows */
    UGH_LOGIC_EVENT_COPTER_CRASHED,      /* player; the attempt ends */
    UGH_LOGIC_EVENT_LEVEL_DONE,          /* the last passenger is delivered */
    UGH_LOGIC_EVENT_PASSENGER_BOARDED,   /* player, entity = passenger */
    UGH_LOGIC_EVENT_PASSENGER_PAID,      /* player, entity = passenger, value = points */
    UGH_LOGIC_EVENT_QUICK_DELIVERY,      /* player, entity = passenger: a bonus item drops */
    UGH_LOGIC_EVENT_PASSENGER_DROPPED,   /* player lets the hanging passenger go, entity = passenger */
    UGH_LOGIC_EVENT_PASSENGER_IN_WATER,  /* entity = passenger */
    UGH_LOGIC_EVENT_FLYER_SCREECH,       /* entity = enemy */
    UGH_LOGIC_EVENT_FLYER_FLAP_START,    /* entity = enemy: the flapping until FLYER_FLAP_STOP */
    UGH_LOGIC_EVENT_FLYER_FLAP_STOP,     /* entity = enemy */
    UGH_LOGIC_EVENT_BLOWER_BLOW,         /* entity = enemy */
    UGH_LOGIC_EVENT_ENEMY_STUNNED,       /* entity = enemy, value = points */
    UGH_LOGIC_EVENT_TREE_DROP,           /* entity = enemy: a bonus item drops */
    UGH_LOGIC_EVENT_BONUS_COLLECTED      /* player, entity = bonus slot, value = 0 energy, 1 life, 2 multiplier */
};

typedef struct {
    int kind;     /* UGH_LOGIC_EVENT_... */
    int player;   /* -1 = none */
    int entity;   /* -1 = none */
    int value;
} ugh_logic_event;

/** Calls back with the events since the last call (and forgets them). */
void ugh_logic_take_events(ugh_logic* logic, void (*callback)(void* ctx, const ugh_logic_event* event), void* ctx);

#ifdef __cplusplus
}
#endif

#endif

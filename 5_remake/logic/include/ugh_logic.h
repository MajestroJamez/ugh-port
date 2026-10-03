/*
 * UGH! game logic - C API (C++20 library without dependencies, docs/rewrite-design.md).
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

/** Loads the game data (assets/logic/ugh-data.ugd). NULL on failure, the reason in err. */
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

/** One frame (70.086 Hz): UGH_LOGIC_CONTINUE while the game goes on (UGH_LOGIC_GAME_OVER before a new game). */
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

/* ------------------------------------------------------------------ what to draw */

/** Where the game is. */
enum {
    UGH_LOGIC_PHASE_START = 0,          /* a new game before its first frame */
    UGH_LOGIC_PHASE_BETWEEN_LEVELS,     /* the black screen before the first caption */
    UGH_LOGIC_PHASE_CAPTION,            /* the caption of a level */
    UGH_LOGIC_PHASE_SETUP,              /* the black screen before the play */
    UGH_LOGIC_PHASE_PLAY                /* the play (also while it fades in and out) */
};

/** What a sprite on the screen is. */
enum { UGH_LOGIC_ENTITY_PASSENGER = 1, UGH_LOGIC_ENTITY_ENEMY, UGH_LOGIC_ENTITY_BONUS_ITEM };

/** A sprite of the level: positions are top left corners in 1/32 px; sprites are assets/sprites/NNN.png, -1 none. */
typedef struct {
    int kind;      /* UGH_LOGIC_ENTITY_... */
    int index;     /* in its list (the events name it so); the slot of a bonus item */
    int x, y;
    int sprite;
    int bubble;    /* a passenger's speech bubble, -1 none */
} ugh_logic_entity;

/** A copter. */
typedef struct {
    int x, y;            /* 1/32 px */
    int rotor_sprite;
    int cargo_look;      /* who sits in it (or hangs below), 0 nobody */
    int destination;     /* the number of the pad its passenger wants to go to, 0 none, -1 a passenger hangs below */
    int fare;
} ugh_logic_copter;

enum { UGH_LOGIC_MAX_ENTITIES = 40, UGH_LOGIC_RAINDROPS = 193 };

/** Everything a frontend draws of a frame. */
typedef struct {
    int phase;              /* UGH_LOGIC_PHASE_... */
    int level;              /* the number of the level in the order of the mode, from 0 */
    int level_id;           /* the level in the data (its map), -1 before the first level is loaded */
    int lives, multiplier;
    unsigned score;
    int energy;             /* 0 .. 23099 */
    int fade;               /* 0 black .. 256 */
    int water_level;        /* the water surface, 1/32 px */
    int water_frame;        /* the animation of the surface, 0 .. 2 */
    int copter_count;
    ugh_logic_copter copters[2];
    int entity_count;
    ugh_logic_entity entities[UGH_LOGIC_MAX_ENTITIES];
    int raindrop_count;     /* 0 without wind */
    int raindrops[UGH_LOGIC_RAINDROPS][2];   /* x, y in pixels of a page 384 px wide (x >= 320 is off the screen) */
} ugh_logic_view;

/** Fills `view` with the state after the last step. */
void ugh_logic_get_view(const ugh_logic* logic, ugh_logic_view* view);

#ifdef __cplusplus
}
#endif

#endif

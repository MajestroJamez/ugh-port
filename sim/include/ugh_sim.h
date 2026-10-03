/*
 * UGH! game logic - C API of the C++ core (C++20, no dependencies).
 *
 * The whole game from a new game to its end: level setup and caption, the play frames (copter physics, passengers,
 * enemies, bonus items, water, rain, keyboard, random numbers), level end, game over. Left out: drawing, sound
 * (the core reports events instead), the menus and pause (P).
 *
 * The state can be read and set as the semantic game state of the golden replays (format "UGR 0", see
 * verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt): named fields such as "copter.0.xf" with the same values
 * and units as the original (positions in 1/32 px, data by their DGROUP offsets).
 */
#ifndef UGH_SIM_H
#define UGH_SIM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_sim ugh_sim;

/**
 * Loads the data exported by the extractor (assets/sim/ugh-sim.bin). NULL on failure, reason in err. The state is
 * the program start: one player, medium difficulty, level 1.
 */
ugh_sim* ugh_sim_create(const char* data_path, char* err, size_t err_size);
void ugh_sim_destroy(ugh_sim* sim);

/**
 * Back to the program start. Every field of the replay state gets a pattern from `fill` (the memory the core was
 * never given, like what the attract mode left); the replay player runs two cores with different patterns and
 * trusts only the fields they agree on.
 */
void ugh_sim_reset(ugh_sim* sim, int fill);

/** Forgets the replay state: empty level lists, no bonus item, every other field filled from `fill`. */
void ugh_sim_clear(ugh_sim* sim, int fill);

/** 1 if the core has the stage of the play frame (the B lines of the replays: passengers, objects, bonuses). */
int ugh_sim_has_stage(const char* stage);

/** Sets a field from its replay text. 1 = set, 0 = not a field of the core (ignored), -1 = bad value. */
int ugh_sim_set(ugh_sim* sim, const char* field, const char* value);

/** Calls back with what the core met and does not support since the last call (and forgets it). */
void ugh_sim_take_problems(ugh_sim* sim, void (*callback)(void* ctx, const char* problem), void* ctx);

/** Calls back with every field of the state (the level lists up to their ends, the bonus items in use). */
void ugh_sim_fields(const ugh_sim* sim, void (*callback)(void* ctx, const char* field, const char* value), void* ctx);

/** What happened, for sounds and effects (ugh_sim_take_events). */
enum {
    UGH_SIM_EVENT_LEVEL_CAPTION = 1,      /* the caption of a level shows (its jingle) */
    UGH_SIM_EVENT_COPTER_CRASHED,         /* player: too hard a bounce, or caught by the flyer; the attempt ends */
    UGH_SIM_EVENT_LEVEL_DONE,             /* the last passenger is delivered */
    UGH_SIM_EVENT_PASSENGER_BOARDED,      /* player, entity = passenger */
    UGH_SIM_EVENT_PASSENGER_PAID,         /* player, entity = passenger, value = points */
    UGH_SIM_EVENT_QUICK_DELIVERY,         /* player, entity = passenger: a bonus item drops */
    UGH_SIM_EVENT_PASSENGER_DROPPED,      /* player lets the hanging passenger go, entity = passenger */
    UGH_SIM_EVENT_PASSENGER_IN_WATER,     /* entity = passenger */
    UGH_SIM_EVENT_FLYER_SCREECH,          /* entity = enemy, before it flies */
    UGH_SIM_EVENT_FLYER_FLAP_START,       /* entity = enemy: the flapping until FLYER_FLAP_STOP */
    UGH_SIM_EVENT_FLYER_FLAP_STOP,        /* entity = enemy */
    UGH_SIM_EVENT_BLOWER_BLOW,            /* entity = enemy */
    UGH_SIM_EVENT_ENEMY_STUNNED,          /* entity = enemy, value = points: a passenger fell on it */
    UGH_SIM_EVENT_TREE_DROP,              /* entity = enemy: a bonus item drops out of the tree */
    UGH_SIM_EVENT_BONUS_COLLECTED         /* player, entity = bonus slot, value = 0 energy, 1 life, 2 multiplier */
};

typedef struct {
    int kind;     /* UGH_SIM_EVENT_... */
    int player;   /* -1 = none */
    int entity;   /* -1 = none */
    int value;
} ugh_sim_event;

/** Calls back with the events since the last call (and forgets them). */
void ugh_sim_take_events(ugh_sim* sim, void (*callback)(void* ctx, const ugh_sim_event* event), void* ctx);

/** A scancode delivered between frames (keyboard handler 113b:4567). */
void ugh_sim_key(ugh_sim* sim, int scancode);

/**
 * The game: from the start of a new game (113b:0c61, the state set before the first call) it runs to the next
 * place where the original waits for the vertical retrace - one frame of the original (70.086 Hz). Deliver
 * the keys with ugh_sim_key between the calls. Returns UGH_SIM_CONTINUE while the game goes on, else how it
 * ended (game over, all levels done; no further frame).
 */
enum { UGH_SIM_CONTINUE = 0, UGH_SIM_GAME_OVER = 1, UGH_SIM_ALL_LEVELS_DONE = 2 };
int ugh_sim_step(ugh_sim* sim);

/* Single transitions from a state set from outside (the replay player checks them one by one). */

/** 113b:3961: 3 lives, multiplier 1, score 0. */
void ugh_sim_new_game(ugh_sim* sim);

/** After a level (113b:0fa7): next level or one life less. */
int ugh_sim_level_end(ugh_sim* sim);

/** Level setup up to the caption: state, level load, the caption's water row. */
void ugh_sim_level_start(ugh_sim* sim);

/** One frame of the level play (113b:0c7d .. 0fa4, without the retrace wait). */
void ugh_sim_play_frame(ugh_sim* sim);

#ifdef __cplusplus
}
#endif

#endif

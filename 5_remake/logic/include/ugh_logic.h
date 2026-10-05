/*
 * UGH! game logic - C API (C++20 library without dependencies, docs/rewrite-design.md).
 *
 * The whole game from a new game to its end: the level caption, the play (copter physics, passengers, enemies,
 * bonus items, water, rain, keys, random numbers), the end of a level, game over. No drawing and no sound: the
 * logic reports events for those. Nothing of the state can be set from outside.
 */
#ifndef UGH_LOGIC_H
#define UGH_LOGIC_H

#include <stddef.h>
#include <stdint.h>

/*
 * In a DLL (the Unreal Engine module UghLogic in the editor) the functions are exported by the DLL that defines
 * UGH_LOGIC_EXPORTS and imported by the ones that define UGH_LOGIC_IMPORTS; in a static library they are plain.
 */
#if defined(UGH_LOGIC_EXPORTS)
#define UGH_LOGIC_API __declspec(dllexport)
#elif defined(UGH_LOGIC_IMPORTS)
#define UGH_LOGIC_API __declspec(dllimport)
#else
#define UGH_LOGIC_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_logic ugh_logic;

/** Loads the game data (assets/logic/ugh-data.ugd). NULL on failure, the reason in err. */
UGH_LOGIC_API ugh_logic* ugh_logic_create(const char* data_path, char* err, size_t err_size);
UGH_LOGIC_API void ugh_logic_destroy(ugh_logic* logic);

/**
 * What a new game starts with: what the menu chose, and two values the original takes from the screens before the
 * game (the replays set them; a frontend keeps the defaults).
 */
typedef struct {
    int players;            /* 1, or 2 for the team mode */
    int difficulty;         /* 0 easy, 1 medium, 2 hard */
    int first_level;        /* from 0 in the order of the mode (a password starts later) */
    uint16_t random_seed[4];   /* the state of the random numbers: any (a frontend may take the time) */
    int rain_floor_row;     /* the row where raindrops start again before the first windy level, 0 .. 255 */
} ugh_logic_settings;

/**
 * The settings of a one-player game on medium from the first level: the random seed 0 (any seed plays a fair game)
 * and the rain row as after the program start.
 */
UGH_LOGIC_API void ugh_logic_default_settings(ugh_logic_settings* settings);

/** A new game; 0 when a setting is out of range. */
UGH_LOGIC_API int ugh_logic_new_game(ugh_logic* logic, const ugh_logic_settings* settings);

/** The keys a pilot flies with. */
enum { UGH_LOGIC_KEY_UP = 0, UGH_LOGIC_KEY_DOWN, UGH_LOGIC_KEY_LEFT, UGH_LOGIC_KEY_RIGHT, UGH_LOGIC_KEY_FIRE };

/** A pilot's key (UGH_LOGIC_KEY_...) pressed (1) or released (0), between two frames; player 0 or 1. */
UGH_LOGIC_API void ugh_logic_key(ugh_logic* logic, int player, int key, int pressed);

/** The keys the game loop sees besides the pilots' keys. */
enum { UGH_LOGIC_MENU_ESCAPE = 0, UGH_LOGIC_MENU_PAUSE, UGH_LOGIC_MENU_OTHER };

/**
 * A key event the game loop sees, between two frames: Esc pressed (it gives the game up in every frame of the play
 * until the next key event), P pressed (pause, not supported), or any other key pressed or released - also a
 * pilot's key. A caption waits for one.
 */
UGH_LOGIC_API void ugh_logic_menu_key(ugh_logic* logic, int key);

enum { UGH_LOGIC_CONTINUE = 0, UGH_LOGIC_GAME_OVER = 1, UGH_LOGIC_ALL_LEVELS_DONE = 2 };

/** The frame rate of the original (VGA): 70.086 frames a second, as frames per 1000 s. */
enum { UGH_LOGIC_FRAMES_PER_1000_S = 70086 };

/** One frame: UGH_LOGIC_CONTINUE while the game goes on (UGH_LOGIC_GAME_OVER before a new game). */
UGH_LOGIC_API int ugh_logic_step(ugh_logic* logic);

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
UGH_LOGIC_API void ugh_logic_take_events(ugh_logic* logic, void (*callback)(void* ctx, const ugh_logic_event* event),
                                         void* ctx);

/* ------------------------------------------------------------------ what to draw */

/** Where the game is. */
enum {
    UGH_LOGIC_PHASE_START = 0,          /* a new game before its first frame */
    UGH_LOGIC_PHASE_BETWEEN_LEVELS,     /* only the black screen before the first caption (the replays name it so) */
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
    int look;      /* a passenger: who it is, as ugh_logic_copter.cargo_look (it rides so); 0 for the others */
    int stunned;   /* 1 while a passenger knocked an enemy out: a walker or a blower stunned, a flyer falling */
} ugh_logic_entity;

/** A copter. */
typedef struct {
    int x, y;            /* 1/32 px */
    int rotor_sprite;
    int cargo_look;      /* who sits in it (or hangs below), 0 nobody */
    int destination;     /* the number of the pad its passenger wants to go to, 0 none, -1 a passenger hangs below */
    int fare;
} ugh_logic_copter;

enum {
    UGH_LOGIC_MAX_ENTITIES = 40, UGH_LOGIC_RAINDROPS = 193,
    UGH_LOGIC_FULL_ENERGY = 23099,   /* a full tank */
    UGH_LOGIC_FADE_SHOWN = 256       /* the level fully shown */
};

/** Everything a frontend draws of a frame. */
typedef struct {
    int phase;              /* UGH_LOGIC_PHASE_... */
    int level;              /* the number of the level in the order of the mode, from 0 */
    int level_id;           /* the level in the data (its map), -1 before the first level is loaded */
    int lives, multiplier;
    unsigned score;
    int energy;             /* UGH_LOGIC_FULL_ENERGY full; below 0 it goes on and wraps like the original's 16 bits */
    int fade;               /* 0 black .. UGH_LOGIC_FADE_SHOWN fully shown; it may overshoot by 2, and ends below 0 */
    int water_level;        /* the water surface, 1/32 px */
    int water_frame;        /* the animation of the surface, 0 .. 2 */
    int wind;               /* where the wind of the level blows: -1 to the left, 1 to the right, 0 none (no rain) */
    int copter_count;
    ugh_logic_copter copters[2];
    int entity_count;
    ugh_logic_entity entities[UGH_LOGIC_MAX_ENTITIES];
    int raindrop_count;     /* the raindrops on the screen; 0 without wind */
    int raindrops[UGH_LOGIC_RAINDROPS][2];   /* x, y in pixels of the screen */
} ugh_logic_view;

/** Fills `view` with the state after the last step. */
UGH_LOGIC_API void ugh_logic_get_view(const ugh_logic* logic, ugh_logic_view* view);

/**
 * The screen of a level in pixels, and the positions of the view: 1/32 px. A copter's body (what a sprite touches)
 * is from BODY_LEFT to BODY_RIGHT across and BODY_HEIGHT high, in pixels from its top left corner.
 */
enum {
    UGH_LOGIC_SCREEN_WIDTH = 320, UGH_LOGIC_SCREEN_HEIGHT = 192, UGH_LOGIC_SUBPIXELS = 32,
    UGH_LOGIC_COPTER_BODY_LEFT = 5, UGH_LOGIC_COPTER_BODY_RIGHT = 26, UGH_LOGIC_COPTER_BODY_HEIGHT = 20
};

/**
 * What a sprite of the entities is in the data (the names of assets/logic/ugh-data.ugd), so that a frontend knows what
 * a figure does: a frame of an animation of a kind ("kind1.walkLeft": the kind, a dot, the animation; "kind1-water" is
 * that passenger in the water), else a sprite of the rules ("standingPassenger", "droppedPassenger",
 * "bouncedPassenger", "shakenTree") or a kind of bonus item ("energy3", "multiplier"). A sprite of two things is the
 * animation's (a flyer falling shows a frame of its flight, a stunned blower one of its blowing: see `stunned`).
 * A passenger's speech bubble (ugh_logic_entity.bubble) is "destinationBubble" (frame: the index of the pad it wants
 * to go to - the board with its number; the last frame for that pad and all after it, a blank board) or
 * "impatientBubble" (a question mark: the copter left without it).
 */
typedef struct {
    char name[32];
    int frame;    /* the first frame of the animation that shows the sprite; 0 for a single sprite */
    int frames;   /* the frames of the animation; 1 for a single sprite */
} ugh_logic_sprite;

/** Fills `info` with what `sprite` is; 0 when no entity of the data shows it (a rotor, an unused one). */
UGH_LOGIC_API int ugh_logic_get_sprite(const ugh_logic* logic, int sprite, ugh_logic_sprite* info);

/* ------------------------------------------------------------------ the background of the level being played */

/** A pad, in pixels. */
typedef struct {
    int left, right;   /* the landing area */
    int y;             /* its surface */
    int number;        /* shown in the bubbles */
} ugh_logic_pad;

/** The pads of the level being played (ugh_logic_view.level_id); 0 before the first level is loaded. */
UGH_LOGIC_API int ugh_logic_pad_count(const ugh_logic* logic);

/** Fills `pad` with pad `index` (0 .. count - 1) of the level being played; 0 when there is none. */
UGH_LOGIC_API int ugh_logic_get_pad(const ugh_logic* logic, int index, ugh_logic_pad* pad);

/** 1 when pixel x, y of the level being played is solid (its collision mask); 0 if not, outside, before a level. */
UGH_LOGIC_API int ugh_logic_solid(const ugh_logic* logic, int x, int y);

#ifdef __cplusplus
}
#endif

#endif

/*	SCCS Id: @(#)flag.h	3.4	2002/08/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you change the flag structure make sure you increment EDITLEVEL in   */
/* patchlevel.h if needed.  Changing the instance_flags structure does	   */
/* not require incrementing EDITLEVEL.					   */

#ifndef FLAG_H
#define FLAG_H

/*
 * Persistent flags that are saved and restored with the game.
 *
 */

struct flag {
#ifdef AMIFLUSH
	boolean  altmeta;	/* use ALT keys as META */
	boolean  amiflush;	/* kill typeahead */
#endif
#ifdef	MFLOPPY
	boolean  asksavedisk;
#endif
	boolean  autodig;       /* MRKR: Automatically dig */
	boolean  autoquiver;	/* Automatically fill quiver */
	boolean  autounlock;	/* automatically apply unlocking tools */
	boolean  beginner;
#ifdef MAIL
	boolean  biff;		/* enable checking for mail */
#endif
	boolean  botl;		/* partially redo status line */
	boolean  botlx;		/* print an entirely new bottom line */
	boolean  debug;		/* in debugging mode */
#define wizard	 flags.debug
	boolean  end_own;	/* list all own scores */
	boolean  explore;	/* in exploration mode */
#ifdef OPT_DISPMAP
	boolean  fast_map;	/* use optimized, less flexible map display */
#endif
#define discover flags.explore
	boolean  female;
	boolean  forcefight;
	boolean  friday13;	/* it's Friday the 13th */
	boolean  help;		/* look in data file for info about stuff */
	boolean  ignintr;	/* ignore interrupts */
#ifdef INSURANCE
	boolean  ins_chkpt;	/* checkpoint as appropriate */
#endif
	boolean  invlet_constant; /* let objects keep their inventory symbol */
	boolean  legacy;	/* print game entry "story" */
	boolean  lit_corridor;	/* show a dark corr as lit if it is in sight */
	boolean  made_amulet;
	boolean  makelev_closerooms;	/* allow rooms to be placed very close to each other, causing overlaps and merges*/
	boolean  mon_moving;	/* monsters' turn to move */
	boolean  run_timers;	/* run timers as soon as possible (probably to desummon items) */
	boolean  drgn_brth;		/* for use with breath weapons, indicates that a dragon is breathing */
	boolean  phasing; /* Etherealoid phasing in or out*/
	boolean  mv;		/* player is doing a multi-tile movement */
	boolean  bypasses;	/* bypass flag is set on at least one fobj */
	boolean  nap;		/* `timed_delay' option for display effects */
	boolean  nopick;	/* do not pickup objects (as when running) */
	boolean  null;		/* OK to send nulls to the terminal */
#ifdef MAC
	boolean  page_wait;	/* put up a --More-- after a page of messages */
#endif
	boolean  perm_invent;	/* keep full inventories up until dismissed */
	boolean  pickup;	/* whether you pickup or move and look */

	boolean  pushweapon;	/* When wielding, push old weapon into second slot */
	boolean  rest_on_space; /* space means rest */
	boolean  safe_dog;	/* give complete protection to the dog */
#ifdef EXP_ON_BOTL
	boolean  showexp;	/* show experience points */
#endif
#ifdef SCORE_ON_BOTL
	boolean  showscore;	/* show score */
#endif
	boolean  silent;	/* whether the bell rings or not */
	boolean  sortpack;	/* sorted inventory */
	boolean  soundok;	/* ok to tell about sounds heard */
	boolean  sparkle;	/* show "resisting" special FX (Scott Bigham) */
	boolean  standard_polearms;	/* use the old polearm targeting system */
	boolean  petsafe_polearms;	/* don't suggest pets as polearm targets */
	boolean  peacesafe_polearms;	/* don't suggest peacefuls as polearm targets */
	boolean  relative_polearms;	/* letter targets starting with a, not based on location. */
	boolean  standout;	/* use standout for --More-- */
	boolean  suppress_hurtness;	/* hides "uninjured" "bloody" "damaged" monster prefixes */
	boolean  time;		/* display elapsed 'time' */
	boolean  timeoutOrder;	/* display spirits and powers in order of timeout */
	boolean  tombstone;	/* print tombstone */
	boolean  toptenwin;	/* ending list in window instead of stdout */
	boolean  verbose;	/* max battle info */
	boolean  prayconfirm;	/* confirm before praying */
	boolean  tm_hour;	/* hour of the day (updated once per global turn) */

	int move;	/* type[s] of action taken by player's last input/action */
	int movetoprint;
	int movetoprintcost;
#define MOVE_DEFAULT				0x04000	/* equivalent to Standard unless another move is layered overtop, in which case it is ignored */
#define MOVE_CANCELLED				0x08000	/* Like move instant, but does not update the bottom line. Use for cancelled actions and non-functional menus, etc */
#define MOVE_FINISHED_OCCUPATION	0x10000	/* finished an occupation; does not affect action time */
#define MOVE_STANDARD				0x00001	/* player did a general action -- takes 1 standard turn */
#define MOVE_INSTANT				0x00002	/* player did an action that should take no time */
#define MOVE_PARTIAL				0x00004	/* player did a general action -- takes no time for the first instance, 1 standard turn after, resets on non-instant action */ 
#define MOVE_MOVED					0x00008	/* player moved */
#define MOVE_ATTACKED				0x00010	/* player made a weapon attack */
#define MOVE_QUAFFED				0x00020	/* player quaffed a potion (or sink/fountain) */
#define MOVE_ZAPPED					0x00040	/* player zapped a wand */
#define MOVE_READ					0x00080	/* player read a book, scroll, or other readable */
#define MOVE_CASTSPELL				0x00100	/* player cast a spell */
#define MOVE_ATE					0x00200	/* player ate food */
#define MOVE_FIRED					0x00400	/* player properly fired ammo, using a launcher or intrinsic launching means, NOT a standard thrown object. */
#define MOVE_CONTAINER				0x00800	/* player used a container */

	int	 end_top, end_around;	/* describe desired score list */
	unsigned ident;		/* social security number for each monster */
	unsigned moonphase;
	unsigned long suppress_alert;
#define NEW_MOON	0
#define FULL_MOON	4
#define HUNTING_MOON	8
	unsigned no_of_wizards; /* 0, 1 or 2 (wizard and his shadow) */
	boolean  travel;	/* find way automatically to u.tx,u.ty */
	unsigned run;		/* 0: h (etc), 1: H (etc), 2: fh (etc) */
				/* 3: FH, 4: ff+, 5: ff-, 6: FF+, 7: FF- */
				/* 8: travel */
	unsigned long warntypem; /* warn_of_mon monster type MM */
	unsigned long warntypet; /* warn_of_mon monster type MT */
	unsigned long warntypeb; /* warn_of_mon monster type MB */
	unsigned long warntypeg; /* warn_of_mon monster type MG */
	unsigned long warntypea; /* warn_of_mon monster type MA */
	unsigned long warntypev; /* warn_of_mon monster type MV */
	unsigned long long montype; /* warn_of_mon monster type bitshifted S_ */
	int	 warnlevel;
	int	 djinni_count, ghost_count;	/* potion effect tuning */
	int	 pickup_burden;		/* maximum burden before prompt */
	char	 inv_order[MAXOCLASSES];
	char	 pickup_types[MAXOCLASSES];
#define NUM_DISCLOSURE_OPTIONS		5
#define DISCLOSE_PROMPT_DEFAULT_YES	'y'
#define DISCLOSE_PROMPT_DEFAULT_NO	'n'
#define DISCLOSE_YES_WITHOUT_PROMPT	'+'
#define DISCLOSE_NO_WITHOUT_PROMPT	'-'
	char	 end_disclose[NUM_DISCLOSURE_OPTIONS + 1];  /* disclose various info
								upon exit */
	char	 menu_style;	/* User interface style setting */
#ifdef AMII_GRAPHICS
	int numcols;
	unsigned short amii_dripens[ 20 ]; /* DrawInfo Pens currently there are 13 in v39 */
	AMII_COLOR_TYPE amii_curmap[ AMII_MAXCOLORS ]; /* colormap */
#endif

	int	 questvar;	/* quest variant */
#define	questprogress	questvar
	boolean	 stag;	/* turned stag during the quest, re-used to track if the Anachrononaut has completed their extra task */
	boolean leader_backstab;		/* your leader is attacking you */
	boolean made_bell;		/* the bell of opening has been created */
	int spriest_level;		/* the current level has a priest of the serpent on it */

	
	Bitfield(spore_level, 1);		/* the current level has a spore-spreading monster */
	Bitfield(slime_level, 1);		/* the current level has a slime-spreading monster */
	Bitfield(walky_level, 1);		/* the current level has a undead-raising monster */
	Bitfield(shade_level, 1);		/* the current level has a shade-casting monster */
	Bitfield(yello_level, 1);		/* the current level has the attention of the King in Yellow */
	Bitfield(goldka_level, 1);		/* the current level has a gold kamerel golem on it */
	Bitfield(silence_level, 1);		/* the current level has an avatar of The Silence on it */
	
	Bitfield(made_first, 1);		/* the first word slab has been created */
	Bitfield(made_divide, 1);		/* the dividing word slab has been created */
	Bitfield(made_life, 1);			/* the nurturing word slab has been created */
	Bitfield(made_know, 1);			/* the word of knowledge slab has been created */

	Bitfield(made_twin, 1);			/* your yog sothoth twin has been created */

	Bitfield(disp_inv, 1);			/* currently displaying inventory, use separate obuf list */

	/* KMH, role patch -- Variables used during startup.
	 *
	 * If the user wishes to select a role, race, gender, and/or alignment
	 * during startup, the choices should be recorded here.  This
	 * might be specified through command-line options, environmental
	 * variables, a popup dialog box, menus, etc.
	 *
	 * These values are each an index into an array.  They are not
	 * characters or letters, because that limits us to 26 roles.
	 * They are not booleans, because someday someone may need a neuter
	 * gender.  Negative values are used to indicate that the user
	 * hasn't yet specified that particular value.	If you determine
	 * that the user wants a random choice, then you should set an
	 * appropriate random value; if you just left the negative value,
	 * the user would be asked again!
	 *
	 * These variables are stored here because the u structure is
	 * cleared during character initialization, and because the
	 * flags structure is restored for saved games.  Thus, we can
	 * use the same parameters to build the role entry for both
	 * new and restored games.
	 *
	 * These variables should not be referred to after the character
	 * is initialized or restored (specifically, after role_init()
	 * is called).
	 */
	int	 initrole;	/* starting role      (index into roles[])   */
	int	 initrace;	/* starting race      (index into races[])   */
	int	 initgend;	/* starting gender    (index into genders[]) */
	int	 initalign;	/* starting alignment (index into aligns[])  */
	int	 initspecies;	/* starting species (index into species[])  */
	int	 descendant;	/* start as descendant */
	int	 chaosvar;	/* Set chaos variant */
	int	 randomall;	/* randomly assign everything not specified */
	int	 pantheon;	/* deity selection for priest character */
	int	 racial_pantheon;	/* racial deity selection */
	int	 panLgod;	/* deity selection for binder character */
	int	 panNgod;	/* deity selection for binder character */
	int	 panCgod;	/* deity selection for binder character */
	int	 panVgod;	/* deity selection for binder character */
	int  HDbreath;	/* half-dragon breath weapon type*/
	int altrace;
};

/*
 * Flags that are set each time the game is started.
 * These are not saved with the game.
 *
 */

/* values for iflags.attack_mode */
#define ATTACK_MODE_PACIFIST  'p'
#define ATTACK_MODE_CHAT      'c'
#define ATTACK_MODE_ASK       'a'
#define ATTACK_MODE_FIGHT_ALL 'f'

/* values for iflags.pokedex */
#define POKEDEX_SHOW_STATS		0x0001
#define POKEDEX_SHOW_GENERATION 0x0002
#define POKEDEX_SHOW_WEIGHT		0x0004
#define POKEDEX_SHOW_RESISTS	0x0008
#define POKEDEX_SHOW_CONVEYS	0x0010
#define POKEDEX_SHOW_MM			0x0020
#define POKEDEX_SHOW_MT			0x0040
#define POKEDEX_SHOW_MB			0x0080
#define POKEDEX_SHOW_MG			0x0100
#define POKEDEX_SHOW_MA			0x0200
#define POKEDEX_SHOW_MV			0x0400
#define POKEDEX_SHOW_ATTACKS	0x0800
#define POKEDEX_SHOW_CRITICAL	0x1000
#define POKEDEX_SHOW_WARDS		0x2000
#define MAX_POKEDEX_SHOW	POKEDEX_SHOW_WARDS
#define POKEDEX_SHOW_DEFAULT	(MAX_POKEDEX_SHOW-1) | MAX_POKEDEX_SHOW

struct instance_flags {
	boolean debug_fuzzer;  /* fuzz testing */
	char attack_mode;         /* attack, refrain or ask to attack monsters */
	boolean  cbreak;	/* in cbreak mode, rogue format */
#ifdef CURSES_GRAPHICS
	boolean  classic_status;	/* What kind of horizontal statusbar to use */
	boolean  classic_colors; 	/* Use traditional curses colors (normally terminal settings)? */
	boolean  cursesgraphics;	/* Use portable curses extended characters */
#endif
	boolean  DECgraphics;	/* use DEC VT-xxx extended character set */
	boolean  echo;		/* 1 to echo characters */
	boolean  IBMgraphics;	/* use IBM extended character set */
	boolean  UTF8graphics;	/* use UTF-8 characters */
	int supports_utf8;	/* if the terminal supports utf8 */
	unsigned msg_history;	/* hint: # of top lines to save */
	boolean  num_pad;	/* use numbers for movement commands */
	boolean  news;		/* print news */
	boolean  window_inited; /* true if init_nhwindows() completed */
	boolean  vision_inited; /* true if vision is ready */
	boolean  menu_tab_sep;	/* Use tabs to separate option menu fields */
	boolean  mod_turncount;	/* Mod the turncount by 10 so it doesn't take up so much space */
	boolean  menu_requested; /* Flag for overloaded use of 'm' prefix
				  * on some non-move commands */
	uchar num_pad_mode;

    int bones;

	int	menu_headings;	/* ATR for menu headings */
	int      purge_monsters;	/* # of dead monsters still on fmon list */
	int *opt_booldup;	/* for duplication of boolean opts in config file */
	int *opt_compdup;	/* for duplication of compound opts in config file */
	uchar	bouldersym;	/* symbol for boulder display */
	boolean travel1;	/* first travel step */
	coord	travelcc;	/* coordinates for travel_cache */
	boolean  door_opened;	/* set to true if door was opened during test_move */
#ifdef QWERTZ
	boolean  qwertz_movement; /* replace y with z for this key layout */
#endif
#ifdef SIMPLE_MAIL
	boolean simplemail;	/* simple mail format $NAME:$MESSAGE */
#endif
#ifdef WIZARD
	boolean  sanity_check;	/* run sanity checks */
	boolean  mon_polycontrol;	/* debug: control monster polymorphs */
#endif
#ifdef TTY_GRAPHICS
	char prevmsg_window;	/* type of old message window to use */
#endif
#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS)
	boolean  extmenu;	/* extended commands use menu interface */
#endif
#ifdef MENU_COLOR
	boolean use_menu_color;	/* use color in menus; only if wc_color */
#endif
#ifdef WIN_EDGE
	boolean  win_edge;	/* are the menus aligned left&top */
#endif
#if defined(STATUS_COLORS) && defined(TEXTCOLOR)
    boolean use_status_colors; /* use color in status line; only if wc_color */
#endif
    boolean hitpointbar;
#ifdef MFLOPPY
	boolean  checkspace;	/* check disk space before writing files */
				/* (in iflags to allow restore after moving
				 * to >2GB partition) */
#endif
#ifdef MICRO
	boolean  BIOS;		/* use IBM or ST BIOS calls when appropriate */
#endif
#if defined(MICRO) || defined(WIN32)
	boolean  rawio;		/* whether can use rawio (IOCTL call) */
#endif
#ifdef MAC_GRAPHICS_ENV
	boolean  MACgraphics;	/* use Macintosh extended character set, as
				   as defined in the special font HackFont */
	unsigned  use_stone;		/* use the stone ppats */
#endif
#if defined(MSDOS) || defined(WIN32)
	boolean hassound;	/* has a sound card */
	boolean usesound;	/* use the sound card */
	boolean usepcspeaker;	/* use the pc speaker */
	boolean tile_view;
	boolean over_view;
	boolean traditional_view;
#endif
#ifdef MSDOS
	boolean hasvga;		/* has a vga adapter */
	boolean usevga;		/* use the vga adapter */
	boolean grmode;		/* currently in graphics mode */
#endif
#ifdef LAN_FEATURES
	boolean lan_mail;	/* mail is initialized */
	boolean lan_mail_fetched; /* mail is awaiting display */
#endif
#ifdef SHOW_BORN
	boolean show_born;	/* show numbers of created monsters */
#endif
#ifdef SORTLOOT
	char sortloot;          /* sort items to loot alphabetically */
#endif
#ifdef PARANOID
	boolean  paranoid_self_cast; /* Ask for 'yes' when casting certain spells at yourself (using . ) */
	boolean  paranoid_hit;  /* Ask for 'yes' when hitting peacefuls */
	boolean  paranoid_quit; /* Ask for 'yes' when quitting */
	boolean  paranoid_remove; /* Always show menu for 'T' and 'R' */
	boolean  paranoid_swim; /* Require 'm' prefix to move into water/lava/air unless it's safe */
	boolean  paranoid_wand_break; /* Ask for 'yes' when breaking a wand */
	boolean  no_forget_map; /* Amnesia doesn't blank map layouts. */
#endif
#ifdef USE_TILES
	boolean  vt_nethack;
#endif
	boolean  autoopen;	/* open doors by walking into them */
	boolean  quiver_fired;
       boolean  pickup_thrown;
    boolean msgtype_regex;
    boolean ape_regex;
    boolean menucolor_regex;
    boolean querytype_regex;
#ifdef USER_SOUNDS
    boolean usersound_regex;
#endif
    boolean show_shop_prices;
    boolean item_use_menu;
    boolean notice_walls;
    boolean use_menu_glyphs;
    boolean hilite_hidden_stairs;
    boolean hilite_obj_piles;
	
    boolean role_obj_names;
    boolean obscure_role_obj_names;
    boolean dnethack_start_text;
    boolean artifact_descriptors;
	boolean force_artifact_names;
    boolean dnethack_dungeon_colors;
    boolean invweight;
	boolean quick_m_abilities;
	boolean default_template_hilite;
	long long statuseffects;
	int statuslines;

	int pokedex;	/* default monster stats to show in the pokedex */
	
	boolean save_uinwater; /* tracks if we're actually buried etc. for #terrain*/
	boolean save_uburied;
	boolean save_uswallow;
	
	boolean autodescribe;
/*
 * Window capability support.
 */
	boolean wc_color;		/* use color graphics                  */
	boolean wc_hilite_pet;		/* hilight pets (blue)                    */
	boolean wc_hilite_peaceful;		/* hilight peaceful monsters (brown)   */
	boolean wc_zombie_z;		/* show zombies as Z of monster's color    */
	boolean wc_hilite_detected;		/* hilight detected monsters (magenta)   */
	boolean wc_ascii_map;		/* show map using traditional ascii    */
	boolean wc_tiled_map;		/* show map using tiles                */
	boolean wc_preload_tiles;	/* preload tiles into memory           */
	int	wc_tile_width;		/* tile width                          */
	int	wc_tile_height;		/* tile height                         */
	char	*wc_tile_file;		/* name of tile file;overrides default */
	boolean wc_inverse;		/* use inverse video for some things   */
	int	wc_align_status;	/*  status win at top|bot|right|left   */
	int	wc_align_message;	/* message win at top|bot|right|left   */
	int     wc_vary_msgcount;	/* show more old messages at a time    */
	char    *wc_foregrnd_menu;	/* points to foregrnd color name for menu win   */
	char    *wc_backgrnd_menu;	/* points to backgrnd color name for menu win   */
	char    *wc_foregrnd_message;	/* points to foregrnd color name for msg win    */
	char    *wc_backgrnd_message;	/* points to backgrnd color name for msg win    */
	char    *wc_foregrnd_status;	/* points to foregrnd color name for status win */
	char    *wc_backgrnd_status;	/* points to backgrnd color name for status win */
	char    *wc_foregrnd_text;	/* points to foregrnd color name for text win   */
	char    *wc_backgrnd_text;	/* points to backgrnd color name for text win   */
	char    *wc_font_map;		/* points to font name for the map win */
	char    *wc_font_message;	/* points to font name for message win */
	char    *wc_font_status;	/* points to font name for status win  */
	char    *wc_font_menu;		/* points to font name for menu win    */
	char    *wc_font_text;		/* points to font name for text win    */
	int     wc_fontsiz_map;		/* font size for the map win           */
	int     wc_fontsiz_message;	/* font size for the message window    */
	int     wc_fontsiz_status;	/* font size for the status window     */
	int     wc_fontsiz_menu;	/* font size for the menu window       */
	int     wc_fontsiz_text;	/* font size for text windows          */
	int	wc_scroll_amount;	/* scroll this amount at scroll_margin */
	int	wc_scroll_margin;	/* scroll map when this far from
						the edge */
	int	wc_map_mode;		/* specify map viewing options, mostly
						for backward compatibility */
	int	wc_player_selection;	/* method of choosing character */
	boolean	wc_splash_screen;	/* display an opening splash screen or not */
	boolean	wc_popup_dialog;	/* put queries in pop up dialogs instead of
				   		in the message window */
	boolean wc_eight_bit_input;	/* allow eight bit input               */
	boolean wc_mouse_support;	/* allow mouse support */
	boolean wc2_fullscreen;		/* run fullscreen */
	boolean wc2_softkeyboard;	/* use software keyboard */
	boolean wc2_wraptext;		/* wrap text */
    int     wc2_term_cols;      /* terminal width, in characters */
    int     wc2_term_rows;      /* terminal height, in characters */
    int     wc2_windowborders;  /* display borders on NetHack windows */
    int     wc2_petattr;        /* points to text attributes for pet */
    boolean wc2_guicolor;       /* allow colors in GUI (outside map) */
	boolean wc2_darkgray;		/* try to use PC dark-gray color
					 * to represent black object */

  boolean botl_updates;
  boolean hp_notify;
  char *hp_notify_fmt;
  boolean old_C_behaviour;
        boolean show_buc;
	boolean show_obj_sym;
	boolean  cmdassist;	/* provide detailed assistance for some commands */
	boolean	 obsolete;	/* obsolete options can point at this, it isn't used */
	/* Items which belong in flags, but are here to allow save compatibility */
	boolean  lootabc;	/* use "a/b/c" rather than "o/i/b" when looting */
	boolean  showrace;	/* show hero glyph by race rather than by role */
	boolean  travelcmd;	/* allow travel command */
	int  travelplus;/* how far travel command should attempt to path through the unknown */
	int	 runmode;	/* update screen display during run moves */
	int delay_length;	/* length of delay for delay_output */
	int wizlevelport;	/* options for ^V in wizmode */
	int wizcombatdebug;	/* options for combat debug messages (damage, accuracy) */
#ifdef AUTOPICKUP_EXCEPTIONS
	struct autopickup_exception *autopickup_exceptions[2];
#define AP_LEAVE 0
#define AP_GRAB	 1
#endif
#ifdef WIN32CON
#define MAX_ALTKEYHANDLER 25
	char	 altkeyhandler[MAX_ALTKEYHANDLER];
#endif
#ifdef REALTIME_ON_BOTL
  boolean  showrealtime; /* show actual elapsed time */
#endif
	struct {
		int set;
		int fg;
		int bg;
		char symbol;
	} monstertemplate[MAXTEMPLATE];
};

/*
 * Old deprecated names
 */
#ifdef TTY_GRAPHICS
#define eight_bit_tty wc_eight_bit_input
#endif
#ifdef TEXTCOLOR
#define use_color wc_color
#endif
#define hilite_pet wc_hilite_pet
#define hilite_peaceful wc_hilite_peaceful
#define hilite_detected wc_hilite_detected
#define use_inverse wc_inverse
#ifdef MAC_GRAPHICS_ENV
#define large_font obsolete
#endif
#ifdef MAC
#define popup_dialog wc_popup_dialog
#endif
#define preload_tiles wc_preload_tiles

extern NEARDATA struct flag flags;
extern NEARDATA struct instance_flags iflags;

/* runmode options */
#define RUN_TPORT	0	/* don't update display until movement stops */
#define RUN_LEAP	1	/* update display every 7 steps */
#define RUN_STEP	2	/* update display every single step */
#define RUN_CRAWL	3	/* walk w/ extra delay after each update */

/* wizlevelport options */
#define WIZLVLPORT_TRADITIONAL		0	/* Select levels directly */
#define WIZLVLPORT_TWOMENU			1	/* Select a branch, then a level in that branch */
#define WIZLVLPORT_BRANCHES_FIRST	2	/* With TWOMENU, don't show levels in each branch during 1st selection */
#define WIZLVLPORT_SELECTED_DUNGEON	4	/* With TWOMENU, only show the levels of the selected branch during 2nd selection */

/* wizcombatdebug options */
#define WIZCOMBATDEBUG_NONE		0x00
#define WIZCOMBATDEBUG_DMG		0x01
#define WIZCOMBATDEBUG_FULLDMG	0x02
#define WIZCOMBATDEBUG_ACCURACY	0x04
#define WIZCOMBATDEBUG_UVM		0x08
#define WIZCOMBATDEBUG_MVU		0x10
#define WIZCOMBATDEBUG_MVM		0x20
#define WIZCOMBATDEBUG_APPLIES(magr, mdef)	\
	(iflags.wizcombatdebug & ( WIZCOMBATDEBUG_MVM |    \
	(((magr) == &youmonst) ? WIZCOMBATDEBUG_UVM : 0) | \
	(((mdef) == &youmonst) ? WIZCOMBATDEBUG_MVU : 0)   \
	))

/* monstertemplate options */
#define MONSTERTEMPLATE_FOREGROUND	0x1
#define MONSTERTEMPLATE_BACKGROUND	0x2
#define MONSTERTEMPLATE_SYMBOL		0x4

#endif /* FLAG_H */

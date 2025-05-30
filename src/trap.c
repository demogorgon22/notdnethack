/*	SCCS Id: @(#)trap.c	3.4	2003/10/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "xhity.h"


extern const char * const destroy_strings[];	/* from xhityhelpers.c */

static boolean rolling_boulder_in_progress;

STATIC_DCL void FDECL(dofiretrap, (struct obj *));
STATIC_DCL void NDECL(domagictrap);
STATIC_DCL boolean FDECL(emergency_disrobe,(boolean *));
STATIC_DCL int FDECL(untrap_prob, (struct trap *ttmp));
STATIC_DCL void FDECL(move_into_trap, (struct trap *));
STATIC_DCL int FDECL(try_disarm, (struct trap *,BOOLEAN_P));
STATIC_DCL int FDECL(disarm_holdingtrap, (struct trap *));
STATIC_DCL int FDECL(disarm_rust_trap, (struct trap *));
STATIC_DCL int FDECL(disarm_fire_trap, (struct trap *));
STATIC_DCL int FDECL(disarm_magic_trap, (struct trap *));
STATIC_DCL int FDECL(disarm_landmine, (struct trap *));
STATIC_DCL int FDECL(disarm_squeaky_board, (struct trap *));
STATIC_DCL int FDECL(disarm_shooting_trap, (struct trap *));
STATIC_DCL boolean FDECL(try_lift, (struct monst *, struct trap *, int, BOOLEAN_P));
STATIC_DCL int FDECL(help_monster_out, (struct monst *, struct trap *));
STATIC_DCL boolean FDECL(thitm, (struct monst *,int,BOOLEAN_P));
STATIC_DCL int FDECL(mkroll_launch,
			(struct trap *,XCHAR_P,XCHAR_P,SHORT_P,long));
STATIC_DCL boolean FDECL(isclearpath,(coord *, int, SCHAR_P, SCHAR_P));
#ifdef STEED
STATIC_OVL int FDECL(steedintrap, (struct trap *, struct obj *));
STATIC_OVL boolean FDECL(keep_saddle_with_steedcorpse,
			(unsigned, struct obj *, struct obj *));
#endif

#ifndef OVLB
STATIC_VAR const char *a_your[2];
STATIC_VAR const char *A_Your[2];
STATIC_VAR const char tower_of_flame[];
STATIC_VAR const char *A_gush_of_water_hits;
STATIC_VAR const char * const blindgas[6];

#else

STATIC_VAR const char * const a_your[2] = { "a", "your" };
STATIC_VAR const char * const A_Your[2] = { "A", "Your" };
STATIC_VAR const char tower_of_flame[] = "tower of flame";
STATIC_VAR const char * const A_gush_of_water_hits = "A gush of water hits";
STATIC_VAR const char * const blindgas[6] = 
	{"humid", "odorless", "pungent", "chilling", "acrid", "biting"};

#endif /* OVLB */

#ifdef OVLB

/* called when you're hit by fire (dofiretrap,buzz,zapyourself,explode) */
boolean			/* returns TRUE if hit on torso */
burnarmor(victim, candestroy)
struct monst *victim;
boolean candestroy;
{
    struct obj *item;
    char buf[BUFSZ];
    int mat_idx;
    
    if (!victim) return 0;
	if(UseInvFire_res(victim)) return 0;
#define burn_dmg(obj,descr) rust_dmg(obj, descr, 0, FALSE, victim, candestroy)
    while (1) {
		switch (rn2(5)) {
			case 0:
				item = (victim == &youmonst) ? uarmh : which_armor(victim, W_ARMH);
				if (item) {
					Sprintf(buf,"%s helmet", material_name(item, FALSE));
				}
				if (!burn_dmg(item, item ? buf : "helmet")) continue;
				break;
			case 1:
				item = (victim == &youmonst) ? uarmc : which_armor(victim, W_ARMC);
				if (item) {
				(void) burn_dmg(item, cloak_simple_name(item));
				return TRUE;
				}
				item = (victim == &youmonst) ? uarm : which_armor(victim, W_ARM);
				if (item && (arm_blocks_upper_body(item->otyp) || !((victim == &youmonst) ? uarmu : which_armor(victim, W_ARMU)) || rn2(2))) {
				(void) burn_dmg(item, xname(item));
				return TRUE;
				}
				item = (victim == &youmonst) ? uarmu : which_armor(victim, W_ARMU);
				if (item)
				(void) burn_dmg(item, "shirt");
				return TRUE;
			case 2:
				item = (victim == &youmonst) ? uarms : which_armor(victim, W_ARMS);
				if (!burn_dmg(item, "shield")) continue;
				break;
			case 3:
				item = (victim == &youmonst) ? uarmg : which_armor(victim, W_ARMG);
				if (!burn_dmg(item, "gloves")) continue;
				break;
			case 4:
				item = (victim == &youmonst) ? uarmf : which_armor(victim, W_ARMF);
				if (!burn_dmg(item, "boots")) continue;
				break;
		}
		break; /* Out of while loop */
    }
    return FALSE;
#undef burn_dmg
}

/* Generic rust-armor function.  Returns TRUE if a message was printed;
 * "print", if set, means to print a message (and thus to return TRUE) even
 * if the item could not be rusted; otherwise a message is printed and TRUE is
 * returned only for rustable items.
 */
boolean
rust_dmg(otmp, ostr, type, print, victim, candestroy)
register struct obj *otmp;
register const char *ostr;
int type;
boolean print;
struct monst *victim;
boolean candestroy;
{
	static NEARDATA const char * const action[] = { "smolder", "rust", "rot", "corrode" };
	static NEARDATA const char * const msg[] =  { "burnt", "rusted", "rotten", "corroded" };
	static NEARDATA const char * const destruction[] = { "burn", "rust", "rot", "corrode" };
	boolean vulnerable = FALSE;
	boolean grprot = FALSE;
	boolean is_primary = TRUE;
	boolean vismon = (victim != &youmonst) && canseemon(victim);
	int erosion;

	if (otmp && otmp->otyp == find_ogloves())
		return FALSE;/* Old gloves are already as damaged as they're going to get */

	if (!otmp) return(FALSE);
	switch(type) {
		case 0: vulnerable = is_flammable(otmp);
			break;
		case 1: vulnerable = is_rustprone(otmp);
			grprot = TRUE;
			break;
		case 2: vulnerable = is_rottable(otmp);
			is_primary = FALSE;
			break;
		case 3: vulnerable = is_corrodeable(otmp);
			grprot = TRUE;
			is_primary = FALSE;
			break;
	}
	erosion = is_primary ? otmp->oeroded : otmp->oeroded2;
	
	if(vulnerable && !otmp->oerodeproof && !otmp->oartifact && erosion == MAX_ERODE && candestroy){
		if (victim == &youmonst)
		    Your("%s %s away!", ostr, vtense(ostr, destruction[type]));
		else if (vismon)
		    pline("%s's %s %s away!", Monnam(victim), ostr,
			  vtense(ostr, destruction[type]));
		if (victim == &youmonst)
			useup(otmp);
		else
			m_useup(victim, otmp);
		return TRUE;
	}
	
	if (!print && (!vulnerable || otmp->oerodeproof || erosion == MAX_ERODE))
		return FALSE;

	if (!vulnerable) {
	    if (flags.verbose) {
		if (victim == &youmonst)
		    Your("%s %s not affected.", ostr, vtense(ostr, "are"));
		else if (vismon)
		    pline("%s's %s %s not affected.", Monnam(victim), ostr,
			  vtense(ostr, "are"));
	    }
	} else if (erosion < MAX_ERODE) {
	    if (grprot && otmp->greased) {
		grease_protect(otmp,ostr,victim);
	    } else if (otmp->oerodeproof || (otmp->blessed && rnl(100) < 25)) {
		if (flags.verbose) {
		    if (victim == &youmonst)
			pline("Somehow, your %s %s not affected.",
			      ostr, vtense(ostr, "are"));
		    else if (vismon)
			pline("Somehow, %s's %s %s not affected.",
			      mon_nam(victim), ostr, vtense(ostr, "are"));
		}
	    } else {
		if (victim == &youmonst)
		    Your("%s %s%s!", ostr,
			 vtense(ostr, action[type]),
			 erosion+1 == MAX_ERODE ? " completely" :
			    erosion ? " further" : "");
		else if (vismon)
		    pline("%s's %s %s%s!", Monnam(victim), ostr,
			vtense(ostr, action[type]),
			erosion+1 == MAX_ERODE ? " completely" :
			  erosion ? " further" : "");
		if (is_primary)
		    otmp->oeroded++;
		else
		    otmp->oeroded2++;
		update_inventory();
	    }
	} else {
	    if (flags.verbose) {
		if (victim == &youmonst)
		    Your("%s %s completely %s.", ostr,
			 vtense(ostr, Blind ? "feel" : "look"),
			 msg[type]);
		else if (vismon)
		    pline("%s's %s %s completely %s.",
			  Monnam(victim), ostr,
			  vtense(ostr, "look"), msg[type]);
	    }
	}
	return(TRUE);
}

void
grease_protect(otmp,ostr,victim)
register struct obj *otmp;
register const char *ostr;
struct monst *victim;
{
	static const char txt[] = "protected by the layer of grease!";
	boolean vismon = victim && (victim != &youmonst) && canseemon(victim);

	if (ostr) {
	    if (victim == &youmonst)
		Your("%s %s %s", ostr, vtense(ostr, "are"), txt);
	    else if (vismon)
		pline("%s's %s %s %s", Monnam(victim),
		    ostr, vtense(ostr, "are"), txt);
	} else {
	    if (victim == &youmonst)
		Your("%s %s",aobjnam(otmp,"are"), txt);
	    else if (vismon)
		pline("%s's %s %s", Monnam(victim), aobjnam(otmp,"are"), txt);
	}
	if (!rn2(otmp->blessed ? 4 : 2)) {
	    if(victim == &youmonst && u.utats & TAT_HAMMER && rn2(2)) return;
	    otmp->greased = 0;
	    if (carried(otmp)) {
		pline_The("grease on %s dissolves.",the(xname(otmp)));
		update_inventory();
	    }
	}
}

struct trap *
maketrap(x,y,typ)
register int x, y, typ;
{
	register struct trap *ttmp;
	register struct rm *lev;
	register boolean oldplace;
	struct obj *otmp;

	if ((ttmp = t_at(x,y)) != 0) {
	    if (ttmp->ttyp == MAGIC_PORTAL) return (struct trap *)0;
	    oldplace = TRUE;
	    if (u.utrap && (x == u.ux) && (y == u.uy) &&
	      ((u.utraptype == TT_BEARTRAP && typ != BEAR_TRAP) ||
	      (u.utraptype == TT_WEB && typ != WEB) ||
	      (u.utraptype == TT_FLESH_HOOK && typ != FLESH_HOOK) ||
	      (u.utraptype == TT_PIT && typ != PIT && typ != SPIKED_PIT)))
		    u.utrap = 0;
	} else {
	    oldplace = FALSE;
	    ttmp = newtrap();
		memset(ttmp, 0, sizeof(struct trap));
	    ttmp->tx = x;
	    ttmp->ty = y;
	    ttmp->launch.x = -1;	/* force error if used before set */
	    ttmp->launch.y = -1;
		ttmp->statueid = 0;		/* one option of the union */
		ttmp->ammo = (struct obj *)0;
	}

	if (oldplace && ttmp->ttyp != typ && ttmp->ammo) {
		remove_trap_ammo(ttmp);
		/* have to re-call maketrap, because remove_trap_ammo() deleted the trap */
		return maketrap(x, y, typ);
	}

	ttmp->ttyp = typ;
	switch(typ) {
	    case STATUE_TRAP:	    /* create a "living" statue */
	      { struct monst *mtmp;
		struct obj *otmp, *statue;

		statue = mkcorpstat(STATUE, (struct monst *)0,
					&mons[rndmonnum()], x, y, FALSE);
		mtmp = makemon(&mons[statue->corpsenm], x, y, MM_ADJACENTOK);
		if (!mtmp) break; /* should never happen */
		while(mtmp->minvent) {
		    otmp = mtmp->minvent;
		    otmp->owornmask = 0;
		    obj_extract_self(otmp);
		    (void) add_to_container(statue, otmp);
		}
		statue->owt = weight(statue);
		mongone(mtmp);
		ttmp->statueid = statue->o_id;
		break;
	      }
	    case ROLLING_BOULDER_TRAP:	/* boulder will roll towards trigger */
		(void) mkroll_launch(ttmp, x, y, BOULDER, 1L);
		break;

		case DART_TRAP:
		case ARROW_TRAP:
			otmp = mksobj(((Role_if(PM_MONK) && In_quest(&u.uz)) ? SHURIKEN : typ == ARROW_TRAP ? ARROW : DART), NO_MKOBJ_FLAGS);
			otmp->quan = 15 + rnd(20);
			// material special cases: role quests
			if (In_quest(&u.uz))
			{
				if (Race_if(PM_DROW))
					set_material(otmp, OBSIDIAN_MT);
				else if (Role_if(PM_ARCHEOLOGIST) && !Is_qstart(&u.uz))
					set_material(otmp, !rn2(3) ? BONE : rn2(2) ? OBSIDIAN_MT : MINERAL);
				else if (Role_if(PM_ANACHRONONAUT))
					set_material(otmp, PLASTIC);
				else if (!rn2(3) && (Race_if(PM_ELF) && (Role_if(PM_RANGER) || Role_if(PM_WIZARD) || Role_if(PM_PRIEST) || Role_if(PM_NOBLEMAN))))
					set_material(otmp, BONE);
				else if (!rn2(3) && (Race_if(PM_GNOME) && Role_if(PM_RANGER)))
					set_material(otmp, MINERAL);
			}
			// material special cases: demon lairs
			if (Is_mammon_level(&u.uz))				set_material(otmp, GOLD);
			else if (Is_lolth_level(&u.uz))			set_material(otmp, OBSIDIAN_MT);
			else if (Is_orcus_level(&u.uz))			set_material(otmp, BONE);
			else if (Is_night_level(&u.uz))			set_material(otmp, BONE);
			// material special cases: other
			if (In_outlands(&u.uz))							set_material(otmp, rn2(3) ? LEAD : rn2(2) ? IRON : rn2(2) ? COPPER : rn2(4) ? SILVER : GOLD);
			else if (!rn2(3) && Is_sunsea(&u.uz))			set_material(otmp, GOLD);
			else if (!rn2(3) && Is_knox(&u.uz))				set_material(otmp, !rn2(3) ? GOLD : !rn2(2) ? PLATINUM : SILVER);
			else if (!rn2(3) && In_moloch_temple(&u.uz))	set_material(otmp, BONE);
			else if (!rn2(3) && In_mines(&u.uz))			set_material(otmp, MINERAL);
			else if (!rn2(3) && In_hell(&u.uz))				set_material(otmp, GREEN_STEEL);
			// poisons
			if (rn2(level_difficulty()) &&
				(typ == DART_TRAP || !rn2(5)))
			{
				if (Is_juiblex_level(&u.uz))			otmp->opoisoned = OPOISON_ACID;
				else if (Is_zuggtmoy_level(&u.uz))		otmp->opoisoned = OPOISON_FILTH;
				else if (Is_baphomet_level(&u.uz))		otmp->opoisoned = OPOISON_ACID;
				else if (Is_grazzt_level(&u.uz))		otmp->opoisoned = OPOISON_ACID;
				else if (Is_malcanthet_level(&u.uz))	otmp->opoisoned = OPOISON_BASIC;
				else if (Is_lolth_level(&u.uz))			otmp->opoisoned = OPOISON_SLEEP;
				else if (Is_leviathan_level(&u.uz))		otmp->opoisoned = OPOISON_AMNES;
				else if (Is_lilith_level(&u.uz))		otmp->opoisoned = OPOISON_BASIC;
				else if (Is_baalzebub_level(&u.uz))		otmp->opoisoned = OPOISON_BASIC;
				else if (Is_demogorgon_level(&u.uz))	otmp->opoisoned = OPOISON_FILTH;
				else if (Is_lamashtu_level(&u.uz))		otmp->opoisoned = OPOISON_FILTH;
				else if (Is_arcadia_woods(&u.uz))		otmp->opoisoned = OPOISON_ACID;
				else if (In_depths(&u.uz) && !rn2(10))	otmp->opoisoned = OPOISON_AMNES;
				else if (Is_rlyeh(&u.uz) && !rn2(3))	otmp->opoisoned = OPOISON_AMNES;
				else									otmp->opoisoned = OPOISON_BASIC;
			}
			set_trap_ammo(ttmp, otmp);
			break;
		case BEAR_TRAP:
			set_trap_ammo(ttmp, mksobj(BEARTRAP, 0));
			break;
		case FLESH_HOOK:
			set_trap_ammo(ttmp, mksobj(HOOK, MKOBJ_NOINIT));
			break;
		case LANDMINE:
			set_trap_ammo(ttmp, mksobj(LAND_MINE, 0));
			break;
		case FIRE_TRAP:
			otmp = mksobj(POT_OIL, NO_MKOBJ_FLAGS);
			otmp->quan = rnd(3);
			set_trap_ammo(ttmp, otmp);
			break;
		case ROCKTRAP:
			if(In_quest(&u.uz) && Pantheon_if(PM_SALAMANDER)){
				otmp = mksobj(OBSIDIAN, NO_MKOBJ_FLAGS);
				otmp->oknapped = KNAPPED_SPEAR;
			} else
				otmp = mksobj(ROCK, NO_MKOBJ_FLAGS);
			otmp->quan = 5 + rnd(10);
			set_trap_ammo(ttmp, otmp);
			break;
	    case RUST_TRAP:
		if (!rn2(4)) {
		    del_engr_at(x, y);
		    levl[x][y].typ = PUDDLE;
		    water_damage(level.objects[x][y], FALSE, TRUE, level.flags.lethe, 0);
		    newsym(x, y);
		}
		break;
	    case HOLE:
	    case PIT:
	    case SPIKED_PIT:
	    case TRAPDOOR:
		{
		struct monst *mtmp = m_at(x,y);
		lev = &levl[x][y];
		if (*in_rooms(x, y, SHOPBASE) &&
			((typ == HOLE || typ == TRAPDOOR) ||
			 IS_DOOR(lev->typ) || IS_WALL(lev->typ)))
		    add_damage(x, y,		/* schedule repair */
			       ((IS_DOOR(lev->typ) || IS_WALL(lev->typ))
				&& !flags.mon_moving) ? 200L : 0L);
		lev->doormask = 0;	/* subsumes altar_num, icedpool... */
		if (IS_ROOM(lev->typ) 
			&& lev->typ != SAND
			&& lev->typ != SOIL
		){ /* && !IS_AIR(lev->typ) */
			if(lev->typ == GRASS) lev->typ = SOIL;
		    else lev->typ = ROOM;
		}

		/*
		 * some cases which can happen when digging
		 * down while phazing thru solid areas
		 */
		else if (lev->typ == STONE || lev->typ == SCORR)
		    lev->typ = CORR;
		else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
		    lev->typ = level.flags.is_maze_lev ? ROOM :
			       level.flags.is_cavernous_lev ? CORR : DOOR;
		
		if(!does_block(x,y,lev) && (!mtmp || !opaque(mtmp->data)))
			unblock_point(x,y);

		unearth_objs(x, y);
		}break;
	}
	if (ttmp->ttyp == HOLE) ttmp->tseen = 1;  /* You can't hide a hole */
	else if (ttmp->ttyp == MAGIC_PORTAL && visible_portals(&u.uz))
		ttmp->tseen = 1;  /* Just make portals known */
	else ttmp->tseen = 0;
	ttmp->once = 0;
	ttmp->madeby_u = 0;
	ttmp->dst.dnum = -1;
	ttmp->dst.dlevel = -1;
	if (!oldplace) {
	    ttmp->ntrap = ftrap;
	    ftrap = ttmp;
	}
	return(ttmp);
}

/* Assign obj to be the ammo of trap. Deletes any ammo currently in the trap. */
void
set_trap_ammo(trap, obj)
struct trap *trap;
struct obj *obj;
{
	while (trap->ammo) {
		struct obj* oldobj = trap->ammo;
		extract_nobj(oldobj, &trap->ammo);
		obfree(oldobj, (struct obj *) 0);
	}
	if (obj->where != OBJ_FREE) {
		panic("putting non-free object into trap");
	}
	obj->where = OBJ_INTRAP;
	obj->nobj = 0;
	obj->otrap = trap;
	trap->ammo = obj;
	return;
}

void
fall_through(td)
boolean td;	/* td == TRUE : trap door or hole */
{
	d_level dtmp;
	char msgbuf[BUFSZ];
	const char *dont_fall = 0;
	register int newlevel = dunlev(&u.uz);

	/* KMH -- You can't escape the Sokoban level traps */
	if(Blind && Levitation && !In_sokoban(&u.uz)) return;

	do {
	    newlevel++;
	} while(!rn2(4) && newlevel < dunlevs_in_dungeon(&u.uz));

	if(td) {
	    struct trap *t=t_at(u.ux,u.uy);
	    seetrap(t);
	    if (!In_sokoban(&u.uz)) {
		if (t->ttyp == TRAPDOOR)
			pline("A trap door opens up under you!");
		else 
			pline("There's a gaping hole under you!");
	    }
	} else pline_The("%s opens up under you!", surface(u.ux,u.uy));

	if (In_sokoban(&u.uz) && Can_fall_thru(&u.uz))
	    ;	/* KMH -- You can't escape the Sokoban level traps */
	else if(Levitation || u.ustuck || !Can_fall_thru(&u.uz)
	   || Flying || is_clinger(youracedata)
	   || (Role_if(PM_ARCHEOLOGIST) && uwep && 
			(uwep->otyp == BULLWHIP || uwep->otyp == VIPERWHIP || uwep->otyp == FORCE_WHIP || uwep->otyp == WHIP_SAW))
	   || (Inhell && !u.uevent.invoked &&
					newlevel == (dunlevs_in_dungeon(&u.uz) - 1))/*seal off sanctum and square level until the invocation is performed*/
		) {
		if (Role_if(PM_ARCHEOLOGIST) && uwep && 
			(uwep->otyp == BULLWHIP || uwep->otyp == VIPERWHIP || uwep->otyp == FORCE_WHIP || uwep->otyp == WHIP_SAW)
		)            
		pline("But thanks to your trusty whip ...");
	    dont_fall = "don't fall in.";
	} else if (youracedata->msize >= MZ_HUGE) {
	    dont_fall = "don't fit through.";
	} else if (!next_to_u()) {
	    dont_fall = "are jerked back by your pet!";
	}
	if (dont_fall) {
	    You1(dont_fall);
	    /* hero didn't fall through, but any objects here might */
	    impact_drop((struct obj *)0, u.ux, u.uy, 0, TRUE);
	    if (!td) {
		display_nhwindow(WIN_MESSAGE, FALSE);
		pline_The("opening under you closes up.");
	    }
	    return;
	}

	if(*u.ushops) shopdig(1);
	if (Is_stronghold(&u.uz)) {
	    find_hell(&dtmp);
	} else if(Role_if(PM_RANGER) && Race_if(PM_GNOME) && In_mines(&u.uz) && !(u.uevent.qexpelled)){
		dtmp.dnum = qstart_level.dnum;
		dtmp.dlevel = 1;
	} else {
	    dtmp.dnum = u.uz.dnum;
	    dtmp.dlevel = newlevel;
	}
	if (!td)
	    Sprintf(msgbuf, "The hole in the %s above you closes up.",
		    ceiling(u.ux,u.uy));
	schedule_goto(&dtmp, FALSE, TRUE, 0,
		      (char *)0, !td ? msgbuf : (char *)0, 0, 0);
}

/*
 * Animate the given statue.  May have been via shatter attempt, trap,
 * or stone to flesh spell.  Return a monster if successfully animated.
 * If the monster is animated, the object is deleted.  If fail_reason
 * is non-null, then fill in the reason for failure (or success).
 *
 * The cause of animation is:
 *
 *	ANIMATE_NORMAL  - hero "finds" the monster
 *	ANIMATE_SHATTER - hero tries to destroy the statue
 *	ANIMATE_SPELL   - stone to flesh spell hits the statue
 *
 * Perhaps x, y is not needed if we can use get_obj_location() to find
 * the statue's location... ???
 */
struct monst *
animate_statue(statue, x, y, cause, fail_reason)
struct obj *statue;
xchar x, y;
int cause;
int *fail_reason;
{
	struct permonst *mptr;
	struct monst *mon = 0;
	struct obj *item;
	coord cc;
	boolean historic = (Role_if(PM_ARCHEOLOGIST) && !flags.mon_moving && (statue->spe & STATUE_HISTORIC));
	char statuename[BUFSZ], grateful = FALSE;

	Strcpy(statuename,the(xname(statue)));

	if (get_ox(statue, OX_EMON)) {
	    cc.x = x,  cc.y = y;
	    mon = montraits(statue, &cc);
	    // if (mon && get_mx(mon, MX_EDOG))
		// wary_dog(mon, TRUE);
	} else {
	    /* statue of any golem hit with stone-to-flesh becomes flesh golem */
	    if (is_golem(&mons[statue->corpsenm]) && cause == ANIMATE_SPELL)
	    	mptr = &mons[PM_FLESH_GOLEM];
	    else
			mptr = &mons[statue->corpsenm];
		
	    if((mptr->geno & G_UNIQ) || mptr->msound == MS_GUARDIAN){
			/* Statues of quest guardians or unique monsters
			* will not stone-to-flesh as the real thing.
			*/
			if(statue->spe&STATUE_FACELESS){
				mon = makemon_full(&mons[PM_DOPPELGANGER], x, y,
					NO_MINVENT|MM_NOCOUNTBIRTH|MM_ADJACENTOK, ILLUMINATED, -1);
			} else {
				mon = makemon(&mons[PM_DOPPELGANGER], x, y,
					NO_MINVENT|MM_NOCOUNTBIRTH|MM_ADJACENTOK);
			}
			if (mon) {
				/* makemon() will set mon->cham to
				 * CHAM_ORDINARY if hero is wearing
				 * ring of protection from shape changers
				 * when makemon() is called, so we have to
				 * check the field before calling newcham().
				 */
				if(statue->spe&STATUE_EPRE && dungeon_topology.eprecursor_typ == PRE_POLYP)
					mon->ispolyp = TRUE;
				if (mon->cham == CHAM_DOPPELGANGER)
					(void) newcham(mon, monsndx(mptr), FALSE, FALSE);
			}
	    } else {
			if(statue->spe&STATUE_FACELESS){
				mon = makemon_full(mptr, x, y, NO_MINVENT|MM_ADJACENTOK, ILLUMINATED, -1);
			} else {
				mon = makemon(mptr, x, y, (NO_MINVENT | MM_ADJACENTOK));
			}
			if(mon && statue->spe&STATUE_EPRE && dungeon_topology.eprecursor_typ == PRE_POLYP)
				mon->ispolyp = TRUE;
		}
	}

	if (!mon) {
	    if (fail_reason) *fail_reason = AS_NO_MON;
	    return (struct monst *)0;
	}
	
	if(mon && ((cause == ANIMATE_SPELL 
		&& ((In_quest(&u.uz) && Role_if(PM_HEALER) && (mon->mtyp == PM_IASOIAN_ARCHON || mon->mtyp == PM_PANAKEIAN_ARCHON || mon->mtyp == PM_HYGIEIAN_ARCHON || mon->mtyp == PM_IKSH_NA_DEVA))
			||  rnd(!always_hostile(mon->data) ? 12 : 20) < ACURR(A_CHA)
		) && !(is_animal(mon->data) || mindless_mon(mon))
		)
		|| statue->spe&STATUE_LOYAL)
	){
		struct monst *newmon;
		newmon = tamedog(mon, (struct obj *)0);
		if(newmon) mon = newmon;

		if(canspotmon(mon) && mon->mtame)
			grateful = TRUE;

		if((In_quest(&u.uz) && Role_if(PM_HEALER) && (mon->mtyp == PM_IASOIAN_ARCHON || mon->mtyp == PM_PANAKEIAN_ARCHON || mon->mtyp == PM_HYGIEIAN_ARCHON || mon->mtyp == PM_IKSH_NA_DEVA))
			|| (statue->spe&STATUE_LOYAL)
		){
			if(Role_if(PM_HEALER) && (mon->mtyp == PM_IASOIAN_ARCHON || mon->mtyp == PM_PANAKEIAN_ARCHON || mon->mtyp == PM_HYGIEIAN_ARCHON || mon->mtyp == PM_IKSH_NA_DEVA))
				set_template(mon, PLAGUE_TEMPLATE);
			if(get_mx(mon, MX_EDOG))
				EDOG(mon)->loyal = TRUE;
		}
	}

	/* in case statue is wielded and hero zaps stone-to-flesh at self */
	if (statue->owornmask) remove_worn_item(statue, TRUE);

	/* allow statues to be of a specific gender */
	if (statue->spe & STATUE_MALE)
	    mon->female = FALSE;
	else if (statue->spe & STATUE_FEMALE)
	    mon->female = TRUE;
	/* if statue has been named, give same name to the monster */
	if (get_ox(statue, OX_ENAM))
	    mon = christen_monst(mon, ONAME(statue));
	/* transfer any statue contents to monster's inventory */
	while ((item = statue->cobj) != 0) {
	    obj_extract_self(item);
	    (void) add_to_minv(mon, item);
	}
	m_dowear(mon, TRUE);
	init_mon_wield_item(mon);
	m_level_up_intrinsic(mon);
	delobj(statue);

	/* mimic statue becomes seen mimic; other hiders won't be hidden */
	if (mon->m_ap_type) seemimic(mon);
	else mon->mundetected = FALSE;
	if (get_mx(mon, MX_ESUM)) {
		if (cansee(x,y))
			pline_The("statue crumbles to dust.");
	}
	else if ((x == u.ux && y == u.uy) || cause == ANIMATE_SPELL) {
	    const char *comes_to_life = nonliving(mon->data) ?
					"moves" : "comes to life"; 
	    if (cause == ANIMATE_SPELL){
	    	if(cansee(x,y)) pline("%s %s!", upstart(statuename),
	    		canspotmon(mon) ? comes_to_life : "disappears");
	    } else if(cansee(x,y)) pline_The("statue %s!",
			canspotmon(mon) ? comes_to_life : "disappears");
		if(grateful){
			if(has_template(mon, PLAGUE_TEMPLATE))
				pline("%s is glad to see you, but too sick to help!", Monnam(mon));
			else pline("%s is incredibly grateful!", Monnam(mon));
		}
	    if (historic) {
		    You_feel("guilty that the historic statue is now gone.");
		    adjalign(-1);
	    }
	} else if (cause == ANIMATE_SHATTER){
	    if(cansee(x,y)) pline("Instead of shattering, the statue suddenly %s!",
			canspotmon(mon) ? "comes to life" : "disappears");
	} else { /* cause == ANIMATE_NORMAL */
	    if(cansee(x,y)) You("spot %s posing as a statue.",
			canspotmon(mon) ? a_monnam(mon) : something);
	    stop_occupation();
	}
	/* avoid hiding under nothing */
	if (x == u.ux && y == u.uy &&
		hides_under(youracedata) && !OBJ_AT(x, y))
	    u.uundetected = 0;

	if (fail_reason) *fail_reason = AS_OK;
	return mon;
}

/*
 * You've either stepped onto a statue trap's location or you've triggered a
 * statue trap by searching next to it or by trying to break it with a wand
 * or pick-axe.
 */
struct monst *
activate_statue_trap(trap, x, y, shatter)
struct trap *trap;
xchar x, y;
boolean shatter;
{
	struct monst *mtmp = (struct monst *)0;
	struct obj *otmp;
	struct obj *nobj;
	int fail_reason;
	unsigned int oid = trap->statueid;

	/*
	 * Try to animate the first valid statue.  Stop the loop when we
	 * actually create something or the failure cause is not because
	 * the mon was unique.
	 */
	deltrap(trap);
	for (otmp = level.objects[x][y]; otmp; otmp = nobj)
	{
		nobj = otmp->nexthere;
		if (otmp && otmp->o_id == oid)
		{
			mtmp = animate_statue(otmp, x, y, shatter ? ANIMATE_SHATTER : ANIMATE_NORMAL, &fail_reason);
		}
	}

	if (Blind) feel_location(x, y);
	else newsym(x, y);
	return mtmp;
}

#ifdef STEED
STATIC_OVL boolean
keep_saddle_with_steedcorpse(steed_mid, objchn, saddle)
unsigned steed_mid;
struct obj *objchn, *saddle;
{
	if (!saddle) return FALSE;
	while(objchn) {
		if(objchn->otyp == CORPSE && get_ox(objchn, OX_EMON)) {
			struct monst *mtmp = EMON(objchn);
			if (mtmp->m_id == steed_mid) {
				/* move saddle */
				xchar x,y;
				if (get_obj_location(objchn, &x, &y, 0)) {
					obj_extract_self(saddle);
					place_object(saddle, x, y);
					stackobj(saddle);
				}
				return TRUE;
			}
		}
		if (Has_contents(objchn) &&
		    keep_saddle_with_steedcorpse(steed_mid, objchn->cobj, saddle))
			return TRUE;
		objchn = objchn->nobj;
	}
	return FALSE;
}
#endif /*STEED*/

void
dotrap(trap, trflags)
register struct trap *trap;
unsigned trflags;
{
	register int ttype = trap->ttyp;
	register struct obj *otmp;
	boolean shienuse = FALSE;
	boolean already_seen = trap->tseen;
	boolean webmsgok = (!(trflags & NOWEBMSG));
	boolean forcebungle = (trflags & FORCEBUNGLE);

	if(
		uwep && is_lightsaber(uwep) && litsaber(uwep) && 
			((activeFightingForm(FFORM_SHIEN) && rnd(3) < FightingFormSkillLevel(FFORM_SHIEN)) || 
			 (activeFightingForm(FFORM_SORESU) && rnd(3) < FightingFormSkillLevel(FFORM_SORESU))
			)
	){
		shienuse = TRUE;
	}
	
	/* KMH -- You can't escape the Sokoban level traps */
	if (In_sokoban(&u.uz) &&
			(ttype == PIT || ttype == SPIKED_PIT || ttype == HOLE ||
			ttype == TRAPDOOR)) {
	    /* The "air currents" message is still appropriate -- even when
	     * the hero isn't flying or levitating -- because it conveys the
	     * reason why the player cannot escape the trap with a dexterity
	     * check, clinging to the ceiling, etc.
	     */
	    pline("Air currents pull you down into %s %s!",
	    	a_your[trap->madeby_u],
	    	defsyms[trap_to_defsym(ttype)].explanation);
	    /* then proceed to normal trap effect */
	} else if (already_seen || ((ttype == HOLE || ttype == TRAPDOOR || ttype == PIT || ttype == SPIKED_PIT) && u.sealsActive&SEAL_SIMURGH)) {
	    if ((Levitation || Flying) &&
		    (ttype == PIT || ttype == SPIKED_PIT || ttype == HOLE ||
		    ttype == BEAR_TRAP)) {
		You("%s over %s %s.",
		    Levitation ? "float" : "fly",
		    a_your[trap->madeby_u],
		    defsyms[trap_to_defsym(ttype)].explanation);
		return;
	    }
	    if(!Fumbling && ttype != MAGIC_PORTAL &&
		ttype != ANTI_MAGIC && !forcebungle &&
		(!rn2(5) ||
	    ((ttype == PIT || ttype == SPIKED_PIT) && is_clinger(youracedata)) ||
		((ttype == HOLE || ttype == TRAPDOOR || ttype == PIT || ttype == SPIKED_PIT) && u.sealsActive&SEAL_SIMURGH)
		)) {
		You("escape %s %s.",
		    ((ttype == ARROW_TRAP || ttype == VIVI_TRAP) && !trap->madeby_u) ? "an" :
			a_your[trap->madeby_u],
		    defsyms[trap_to_defsym(ttype)].explanation);
		nomul(0, NULL);
		return;
	    }
	}

#ifdef STEED
	if (u.usteed) u.usteed->mtrapseen |= (1 << (ttype-1));
#endif

	switch(ttype) {
	    case ARROW_TRAP:
		case DART_TRAP:
		if (!trap->ammo) {
			You_hear("a loud click!");
			deltrap(trap);
			newsym(u.ux, u.uy);
			nomul(0, NULL);
			break;
		}
		otmp = trap->ammo;
		if (trap->ammo->quan > 1) {
			otmp = splitobj(trap->ammo, 1);
		}
		extract_nobj(otmp, &trap->ammo);
		seetrap(trap);
		pline("%s shoots out at you!", An(xname(otmp)));

		projectile((struct monst *)0, otmp, trap, HMON_PROJECTILE|HMON_FIRED|HMON_TRAP, trap->tx, trap->ty, 0, 0, 0, 0, FALSE, FALSE, FALSE);
		break;

	    case ROCKTRAP:
		if (!(otmp = trap->ammo)) {
		    pline("A trap door in %s opens, but nothing falls out!",
			  the(ceiling(u.ux,u.uy)));
		    deltrap(trap);
		    newsym(u.ux,u.uy);
			nomul(0, NULL);
			break;
		}
		otmp = trap->ammo;
		if (trap->ammo->quan > 1) {
			otmp = splitobj(trap->ammo, 1);
		}
		extract_nobj(otmp, &trap->ammo);
		seetrap(trap);
		pline("A trap door in %s opens and %s falls!",
			the(ceiling(u.ux, u.uy)),
			an(xname(otmp))
			);
		projectile((struct monst *)0, otmp, trap, HMON_PROJECTILE|HMON_FIRED|HMON_TRAP, trap->tx, trap->ty, 0, 0, 0, 0, FALSE, FALSE, FALSE);
		break;

	    case SQKY_BOARD:	    /* stepped on a squeaky board */
		if (Levitation || Flying) {
		    if (!Blind) {
			seetrap(trap);
			if (Hallucination)
				You("notice a crease in the linoleum.");
			else
				You("notice a loose board below you.");
			if (!already_seen)
				nomul(0, NULL);
		    }
		} else {
		    seetrap(trap);
		    pline("A board beneath you squeaks loudly.");
		    wake_nearby_noisy();
			nomul(0, NULL);
		}
		break;

		case MUMMY_TRAP:	    /* step onto the mummy trap */
			seetrap(trap);
			register int cnt = rnd(3)+2;
			pline("Mummies suddenly appear around you!");
			if (!Free_action) {                        
				pline("You freeze for a moment, terrified!");
				nomul(-rnd(2), "frozen by a trap");
				exercise(A_DEX, FALSE);
				nomovemsg = You_can_move_again;
			} else {
				You("are mildly startled.");
				nomul(0, NULL);
			}
			
			while(cnt--)
				(void) makemon(mkclass(S_MUMMY, G_NOHELL), u.ux, u.uy, NO_MM_FLAGS);

			deltrap(trap);
			
		break;
		
	    case SWITCH_TRAP:
			if(Role_if(PM_NOBLEMAN) && Race_if(PM_HALF_DRAGON) && flags.initgend){
				int ix, iy;
				for(ix = 1; ix < COLNO; ix++){
					for(iy = 0; iy < ROWNO; iy++){
						if(IS_DOOR(levl[ix][iy].typ) && artifact_door(ix,iy)){
							You_hear("a door open.");
							levl[ix][iy].typ = ROOM;
							unblock_point(ix,iy);
						}
					}
				}
				nomul(0, NULL);
			} else {
				impossible("dotrap: You triggered an unhandled switch trap");
			}
		break;
	    case VIVI_TRAP:
			if(trap->tseen){
				You("shove through the delicate equipment, ruining it!");
			} else {
				You("blunder into some delicate equipment, ruining it!");
			}
			deltrap(trap);
			newsym(u.ux,u.uy);	/* get rid of trap symbol */
			nomul(0, NULL);
		break;

	    case BEAR_TRAP:
		if(Levitation || Flying) break;
		seetrap(trap);
		if(amorphous(youracedata) || is_whirly(youracedata) ||
						    unsolid(youracedata)) {
		    pline("%s %s closes harmlessly through you.",
			    A_Your[trap->madeby_u],
				xname(trap->ammo));
			nomul(0, NULL);
		    break;
		}
		if(
#ifdef STEED
		   !u.usteed &&
#endif
		   youracedata->msize < MZ_SMALL) {
		    pline("%s %s closes harmlessly over you.",
			    A_Your[trap->madeby_u],
				xname(trap->ammo));
			nomul(0, NULL);
		    break;
		}
		u.utrap = rn1(4, 4);
		u.utraptype = TT_BEARTRAP;
#ifdef STEED
		if (u.usteed) {
			pline("%s %s closes on %s %s!",
				A_Your[trap->madeby_u], xname(trap->ammo), s_suffix(mon_nam(u.usteed)),
				mbodypart(u.usteed, FOOT));
			hmon_with_trap(u.usteed, &(trap->ammo), trap, HMON_WHACK, rnd(20));
		}
		else
#endif
		{
		    long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;
		    pline("%s %s closes on your %s!",
				A_Your[trap->madeby_u], xname(trap->ammo), body_part(FOOT));

			hmon_with_trap(&youmonst, &(trap->ammo), trap, HMON_WHACK, rnd(20));

		    if(u.umonnum == PM_OWLBEAR || u.umonnum == PM_BUGBEAR)
			You("howl in anger!");
	#ifdef STEED
			if (!u.usteed)
	#endif
			{
				if (!(uarmf && uarmf->otyp == find_jboots())) set_wounded_legs(side, rn1(45, 21));
			}
		}
		exercise(A_DEX, FALSE);
		nomul(0, NULL);
		break;

	    case FLESH_HOOK:
		seetrap(trap);
		if(amorphous(youracedata) || is_whirly(youracedata) ||
						    unsolid(youracedata)) {
		    pline("%s %s passes harmlessly through you.",
			    A_Your[trap->madeby_u],
				xname(trap->ammo));
		    break;
		}
		u.utrap = rn1(4, 4);
		u.utraptype = TT_FLESH_HOOK;
#ifdef STEED
		if (u.usteed) {
			pline("%s %s grabs %s %s!",
				A_Your[trap->madeby_u], xname(trap->ammo), s_suffix(mon_nam(u.usteed)),
				mbodypart(u.usteed, FOOT));
			hmon_with_trap(u.usteed, &(trap->ammo), trap, HMON_WHACK, rnd(20));
		}
		else
#endif
		{
		    pline("%s %s grabs you!",
				A_Your[trap->madeby_u], xname(trap->ammo));

			hmon_with_trap(&youmonst, &(trap->ammo), trap, HMON_WHACK, rnd(20));
		}
		exercise(A_DEX, FALSE);
		nomul(0, NULL);
		break;

	    case SLP_GAS_TRAP:
		seetrap(trap);
		if(Sleep_resistance || breathless(youracedata)) {
		    You("are enveloped in a cloud of gas!");
			nomul(0, NULL);
		    break;
		}
		pline("A cloud of gas puts you to sleep!");
		fall_asleep(-rnd(25), TRUE);
#ifdef STEED
		(void) steedintrap(trap, (struct obj *)0);
#endif
		nomul(0, NULL);
		break;

	    case RUST_TRAP:
		seetrap(trap);
		if(Waterproof){
		    pline("Water gushes around you!");
			break;
		}
		if (is_iron(youracedata)) {
		    int dam = u.mhmax;

		    pline("%s you!", A_gush_of_water_hits);
		    You("are covered with rust!");
			dam = reduce_dmg(&youmonst,dam,TRUE,FALSE);
		    losehp(dam, "rusting away", KILLED_BY);
			nomul(0, NULL);
		    break;
		} else if (u.umonnum == PM_FLAMING_SPHERE) {
		    int dam = u.mhmax;

		    pline("%s you!", A_gush_of_water_hits);
		    You("are extinguished!");
			dam = reduce_dmg(&youmonst,dam,TRUE,FALSE);
		    losehp(dam, "drenching", KILLED_BY);
			nomul(0, NULL);
		    break;
		} else if (u.umonnum == PM_GREMLIN && rn2(3)) {
		    pline("%s you!", A_gush_of_water_hits);
		    (void)split_mon(&youmonst, (struct monst *)0);
			nomul(0, NULL);
		    break;
		}

	    /* Unlike monsters, traps cannot aim their rust attacks at
	     * you, so instead of looping through and taking either the
	     * first rustable one or the body, we take whatever we get,
	     * even if it is not rustable.
	     */
		switch (rn2(5)) {
		    case 0:
			pline("%s you on the %s!", A_gush_of_water_hits,
				    body_part(HEAD));
			(void) rust_dmg(uarmh, "helmet", 1, TRUE, &youmonst, FALSE);
			break;
		    case 1:
			pline("%s your left %s!", A_gush_of_water_hits,
				    body_part(ARM));
			if (rust_dmg(uarms, "shield", 1, TRUE, &youmonst, FALSE))
			    break;
			if (u.twoweap || (uwep && bimanual(uwep,youracedata)))
			    erode_obj(u.twoweap ? uswapwep : uwep, FALSE, TRUE);
glovecheck:		(void) rust_dmg(uarmg, "gauntlets", 1, TRUE, &youmonst, FALSE);
			/* Not "metal gauntlets" since it gets called
			 * even if it's leather for the message
			 */
			break;
		    case 2:
			pline("%s your right %s!", A_gush_of_water_hits,
				    body_part(ARM));
			erode_obj(uwep, FALSE, TRUE);
			goto glovecheck;
		    default:
			pline("%s you!", A_gush_of_water_hits);
			for (otmp=invent; otmp; otmp = otmp->nobj)
				    (void) snuff_lit(otmp);
			if (uarmc)
			    (void) rust_dmg(uarmc, cloak_simple_name(uarmc),
						1, TRUE, &youmonst, FALSE);
			else if (uarm && (arm_blocks_upper_body(uarm->otyp) || rn2(2)))
			    (void) rust_dmg(uarm, "armor", 1, TRUE, &youmonst, FALSE);
#ifdef TOURIST
			else if (uarmu)
			    (void) rust_dmg(uarmu, "shirt", 1, TRUE, &youmonst, FALSE);
#endif
		}
		update_inventory();
		nomul(0, NULL);
		break;

	    case FIRE_TRAP:
		if (!(otmp = trap->ammo)) {
			You_hear("a soft click!");
			deltrap(trap);
			newsym(u.ux, u.uy);
			nomul(0, NULL);
			break;
		}
		if (!(Is_firelevel(&u.uz)) &&	/* never useup on plane of fire */
			!(Inhell && rn2(5)) &&		/* useup 80% less often in gehennom */
			!(rn2(2))) {				/* useup only 50% of the time base */
			if (otmp->quan > 1)
				otmp->quan--;
			else {
				extract_nobj(otmp, &(trap->ammo));
				delobj(otmp);
			}
		}
		seetrap(trap);
		dofiretrap((struct obj *)0);
		nomul(0, NULL);
		break;

	    case PIT:
	    case SPIKED_PIT:
		/* KMH -- You can't escape the Sokoban level traps */
		if (!In_sokoban(&u.uz) && (Levitation || Flying)) break;
		seetrap(trap);
		if (!In_sokoban(&u.uz) && is_clinger(youracedata)) {
			if(ttype == SPIKED_PIT && In_outlands(&u.uz)){
				if(trap->tseen) {
					You("see %s shard-filled pit below you.", a_your[trap->madeby_u]);
				} else {
					pline("%s pit full of mirror-shards opens up under you!",
						A_Your[trap->madeby_u]);
					You("don't fall in!");
				}
			} else {
				if(trap->tseen) {
					You("see %s %spit below you.", a_your[trap->madeby_u],
						ttype == SPIKED_PIT ? "spiked " : "");
				} else {
					pline("%s pit %sopens up under you!",
						A_Your[trap->madeby_u],
						ttype == SPIKED_PIT ? "full of spikes " : "");
					You("don't fall in!");
				}
			}
			if (!already_seen)
				nomul(0, NULL);
		    break;
		}
		if (!In_sokoban(&u.uz)) {
		    char verbbuf[BUFSZ];
#ifdef STEED
		    if (u.usteed) {
		    	if ((trflags & RECURSIVETRAP) != 0)
			    Sprintf(verbbuf, "and %s fall",
				x_monnam(u.usteed,
				    M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
				    (char *)0, SUPPRESS_SADDLE, FALSE));
			else
			    Sprintf(verbbuf,"lead %s",
				x_monnam(u.usteed,
					 M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
				 	 "poor", SUPPRESS_SADDLE, FALSE));
		    } else
#endif
		    Strcpy(verbbuf,"fall");
		    You("%s into %s pit!", verbbuf, a_your[trap->madeby_u]);
		}
		/* wumpus reference */
		if (Role_if(PM_RANGER) && !Race_if(PM_DROW) && !Race_if(PM_ELF) && !Race_if(PM_GNOME) && !trap->madeby_u && !trap->once &&
			In_quest(&u.uz) && Is_qlocate(&u.uz)) {
		    pline("Fortunately it has a bottom after all...");
		    trap->once = 1;
		} else if (u.umonnum == PM_PIT_VIPER ||
			u.umonnum == PM_PIT_FIEND)
		    pline("How pitiful.  Isn't that the pits?");
		if (ttype == SPIKED_PIT) {
			if(In_outlands(&u.uz)){
				const char *predicament = "on a set of sharp mirror-shards";
#ifdef STEED
				if (u.usteed) {
				pline("%s lands %s!",
					upstart(x_monnam(u.usteed,
						 M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
						 "poor", SUPPRESS_SADDLE, FALSE)),
					  predicament);
				} else
#endif
				{
					You("land %s!", predicament);
					if (hates_silver(youracedata))
						pline("The silver shards sear your flesh!");
				}
			} else {
				const char *predicament = "on a set of sharp iron spikes";
#ifdef STEED
				if (u.usteed) {
				pline("%s lands %s!",
					upstart(x_monnam(u.usteed,
						 M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
						 "poor", SUPPRESS_SADDLE, FALSE)),
					  predicament);
				} else
#endif
				{
					You("land %s!", predicament);
					if (hates_iron(youracedata))
						pline("The cold-iron sears your flesh!");
				}
			}
		}
		if (!Passes_walls)
		    u.utrap = rn1(6,2);
		u.utraptype = TT_PIT;
#ifdef STEED
		if (!steedintrap(trap, (struct obj *)0)) {
#endif
		if (ttype == SPIKED_PIT) {
			if(In_outlands(&u.uz)){
				if (!hates_silver(youracedata)){
					losehp(rnd(12), "fell into a pit of mirror-shards",
					NO_KILLER_PREFIX);
				}
				else{//silver damage
					losehp((rnd(12) + rnd(20)), "fell into a pit of silver mirror-shards",
					NO_KILLER_PREFIX);
				}
				//Note: Never poisoned
			} else {
				if (!hates_iron(youracedata)){
					losehp(rnd(10), "fell into a pit of iron spikes",
					NO_KILLER_PREFIX);
				}
				else{//cold-iron damage
					losehp((rnd(10) + rnd(u.ulevel)), "fell into a pit of cold-iron spikes",
					NO_KILLER_PREFIX);
				}
				if (!rn2(6))
				poisoned("spikes", A_STR, "fall onto poison spikes", 8, FALSE);
			}
		} else
		    losehp(rnd(6),"fell into a pit", NO_KILLER_PREFIX);
		if (Punished && !carried(uball)) {
		    unplacebc();
		    ballfall();
		    placebc();
		}
		selftouch("Falling, you");
		vision_full_recalc = 1;	/* vision limits change */
		exercise(A_STR, FALSE);
		exercise(A_DEX, FALSE);
#ifdef STEED
		}
#endif
		nomul(0, NULL);
		break;
	    case HOLE:
	    case TRAPDOOR:
		if (!Can_fall_thru(&u.uz)) {
		    seetrap(trap);	/* normally done in fall_through */
		    impossible("dotrap: %ss cannot exist on this level.",
			       defsyms[trap_to_defsym(ttype)].explanation);
			if (!already_seen)
				nomul(0, NULL);
		    break;		/* don't activate it after all */
		}
		if(u.sealsActive&SEAL_SIMURGH) unbind(SEAL_SIMURGH,TRUE);
		fall_through(TRUE);
		nomul(0, NULL);
		break;

	    case TELEP_TRAP:
		seetrap(trap);
		tele_trap(trap);
		nomul(0, NULL);
		break;
	    case LEVEL_TELEP:
		seetrap(trap);
		level_tele_trap(trap, FALSE);
		nomul(0, NULL);
		break;

	    case WEB: /* Our luckless player has stumbled into a web. */
		seetrap(trap);
		if (amorphous(youracedata) || is_whirly(youracedata) ||
						    unsolid(youracedata)) {
		    if (acidic(youracedata) || u.umonnum == PM_GELATINOUS_CUBE ||
			u.umonnum == PM_FIRE_ELEMENTAL) {
				if (webmsgok)
					You("%s %s spider web!",
					(u.umonnum == PM_FIRE_ELEMENTAL) ? "burn" : "dissolve",
					a_your[trap->madeby_u]);
				if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
					deltrap(trap);
					newsym(u.ux,u.uy);
				}
				nomul(0, NULL);
				break;
		    }
		    if (webmsgok) You("flow through %s spider web.",
			    a_your[trap->madeby_u]);
		    break;
		}
		if (webmaker(youracedata) || u.sealsActive&SEAL_CHUPOCLOPS || (uarm && uarm->oartifact==ART_SPIDERSILK)) {
		    if (webmsgok)
		    	pline(trap->madeby_u ? "You take a walk on your web."
					 : "There is a spider web here.");
		    break;
		}
		if (webmsgok) {
		    char verbbuf[BUFSZ];
		    verbbuf[0] = '\0';
#ifdef STEED
		    if (u.usteed && !webmaker(u.usteed->data)){
				Sprintf(verbbuf,"lead %s",
					x_monnam(u.usteed,
						 M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
						 "poor", SUPPRESS_SADDLE, FALSE));
		    } else
#endif
			
		    Sprintf(verbbuf, "%s", Levitation ? (const char *)"float" :
		      		locomotion(&youmonst, "stumble"));
		    You("%s into %s spider web!",
			verbbuf, a_your[trap->madeby_u]);
		}
		u.utraptype = TT_WEB;

		/* Time stuck in the web depends on your/steed strength. */
		{
		    register int str = ACURR(A_STR);

#ifdef STEED
		    /* If mounted, the steed gets trapped.  Use mintrap
		     * to do all the work.  If mtrapped is set as a result,
		     * unset it and set utrap instead.  In the case of a
		     * strongmonst and mintrap said it's trapped, use a
		     * short but non-zero trap time.  Otherwise, monsters
		     * have no specific strength, so use player strength.
		     * This gets skipped for webmsgok, which implies that
		     * the steed isn't a factor.
		     */
		    if (u.usteed && webmsgok) {
			/* mtmp location might not be up to date */
			u.usteed->mx = u.ux;
			u.usteed->my = u.uy;

			/* mintrap currently does not return 2(died) for webs */
			if (mintrap(u.usteed)) {
			    u.usteed->mtrapped = 0;
			    if (strongmonst(u.usteed->data)) str = 17;
			} else {
			    break;
			}

			webmsgok = FALSE; /* mintrap printed the messages */
		    }
#endif
		    if (str <= 3) u.utrap = rn1(6,6);
		    else if (str < 6) u.utrap = rn1(6,4);
		    else if (str < 9) u.utrap = rn1(4,4);
		    else if (str < 12) u.utrap = rn1(4,2);
		    else if (str < 15) u.utrap = rn1(2,2);
		    else if (str < 18) u.utrap = rnd(2);
		    else if (str < STR18(51)) u.utrap = 1;
		    else {
			u.utrap = 0;
			if (webmsgok)
			    You("tear through %s web!", a_your[trap->madeby_u]);
			if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
				deltrap(trap);
				newsym(u.ux,u.uy);	/* get rid of trap symbol */
			}
		    }
		}
		nomul(0, NULL);
		break;

	    case STATUE_TRAP:
		(void) activate_statue_trap(trap, trap->tx, trap->ty, FALSE);
		nomul(0, NULL);
		break;

	    case MAGIC_TRAP:	    /* A magic trap. */
		seetrap(trap);
		if (!rn2(30)) {
		    deltrap(trap);
		    newsym(u.ux,u.uy);	/* update position */
		    You("are caught in a magical explosion!");
		    losehp(rnd(10), "magical explosion", KILLED_BY_AN);
		    Your("body absorbs some of the magical energy!");
		    u.uenbonus += 2;
			calc_total_maxen();
			u.uen = min(u.uen+400, u.uenmax);
		} else domagictrap();
#ifdef STEED
		(void) steedintrap(trap, (struct obj *)0);
#endif
		nomul(0, NULL);
		break;

	    case ANTI_MAGIC:
		seetrap(trap);
		if(Antimagic && !Race_if(PM_INCANTIFIER)) {
		    shieldeff(u.ux, u.uy);
		    You_feel("momentarily lethargic.");
		} else drain_en(rnd(u.ulevel) + 1);
		nomul(0, NULL);
		break;

	    case POLY_TRAP: {
	        char verbbuf[BUFSZ];
		seetrap(trap);
#ifdef STEED
		if (u.usteed)
			Sprintf(verbbuf, "lead %s",
				x_monnam(u.usteed,
					 M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
				 	 (char *)0, SUPPRESS_SADDLE, FALSE));
		else
#endif
		 Sprintf(verbbuf,"%s",
		    Levitation ? (const char *)"float" :
		    locomotion(&youmonst, "step"));
		You("%s onto a polymorph trap!", verbbuf);
		if(Antimagic || Unchanging) {
		    shieldeff(u.ux, u.uy);
		    You_feel("momentarily different.");
		    /* Trap did nothing; don't remove it --KAA */
		} else {
#ifdef STEED
		    (void) steedintrap(trap, (struct obj *)0);
#endif
		    deltrap(trap);	/* delete trap before polymorph */
		    newsym(u.ux,u.uy);	/* get rid of trap symbol */
		    You_feel("a change coming over you.");
		    polyself(FALSE);
		}
		nomul(0, NULL);
		break;
	    }
	    case LANDMINE: {
#ifdef STEED
		unsigned steed_mid = 0;
		struct obj *saddle = 0;
#endif
		if (Levitation || Flying) {
		    if (!already_seen && rn2(3)) break;
		    seetrap(trap);
		    pline("%s %s in a pile of soil below you.",
			    already_seen ? "There is" : "You discover",
			    trap->madeby_u ? "the trigger of your mine" :
					     "a trigger");
		    if (already_seen && rn2(3)) break;
		    pline("KAABLAMM!!!  %s %s%s off!",
			  forcebungle ? "Your inept attempt sets" :
					"The air currents set",
			    already_seen ? a_your[trap->madeby_u] : "",
			    already_seen ? " land mine" : "it");
		} else {
#ifdef STEED
		    /* prevent landmine from killing steed, throwing you to
		     * the ground, and you being affected again by the same
		     * mine because it hasn't been deleted yet
		     */
		    static boolean recursive_mine = FALSE;

		    if (recursive_mine) break;
#endif
		    seetrap(trap);
		    pline("KAABLAMM!!!  You triggered %s land mine!",
					    a_your[trap->madeby_u]);
#ifdef STEED
		    if (u.usteed) steed_mid = u.usteed->m_id;
		    recursive_mine = TRUE;
		    (void) steedintrap(trap, (struct obj *)0);
		    recursive_mine = FALSE;
		    saddle = sobj_at(SADDLE,u.ux, u.uy);
#endif
		    set_wounded_legs(LEFT_SIDE, rn1(35, 41));
		    set_wounded_legs(RIGHT_SIDE, rn1(35, 41));
		    exercise(A_DEX, FALSE);
		}
		blow_up_landmine(trap);
#ifdef STEED
		if (steed_mid && saddle && !u.usteed)
			(void)keep_saddle_with_steedcorpse(steed_mid, fobj, saddle);
#endif
		newsym(u.ux,u.uy);		/* update trap symbol */
		losehp(rnd(16), "land mine", KILLED_BY_AN);
		/* fall recursively into the pit... */
		if ((trap = t_at(u.ux, u.uy)) != 0) dotrap(trap, RECURSIVETRAP);
		fill_pit(u.ux, u.uy);
		nomul(0, NULL);
		break;
	    }
	    case ROLLING_BOULDER_TRAP: 
		if (!rolling_boulder_in_progress){
		int style = ROLL | (trap->tseen ? LAUNCH_KNOWN : 0);

		seetrap(trap);
		pline("Click! You trigger a rolling boulder trap!");
		nomul(0, NULL);
		rolling_boulder_in_progress = TRUE;
		if(!launch_obj(BOULDER, trap, style)) {
		    deltrap(trap);
		    newsym(u.ux,u.uy);	/* get rid of trap symbol */
		    pline("Fortunately for you, no boulder was released.");
		}
		rolling_boulder_in_progress = FALSE;
	    }
		break;
	    case MAGIC_PORTAL:
		seetrap(trap);
		domagicportal(trap);
		nomul(0, NULL);
		break;

	    default:
		seetrap(trap);
		impossible("You hit a trap of type %u", trap->ttyp);
	}
}

#ifdef STEED
STATIC_OVL int
steedintrap(trap, otmp)
struct trap *trap;
struct obj *otmp;
{
	struct monst *mtmp = u.usteed;
	struct permonst *mptr;
	int tt;
	boolean in_sight;
	boolean trapkilled = FALSE;
	boolean steedhit = FALSE;

	if (!u.usteed || !trap) return 0;
	mptr = mtmp->data;
	tt = trap->ttyp;
	mtmp->mx = u.ux;
	mtmp->my = u.uy;

	in_sight = !Blind;
	switch (tt) {
		case SLP_GAS_TRAP:
		    if (!resists_sleep(mtmp) && !breathless_mon(mtmp) &&
				!mtmp->msleeping && mtmp->mcanmove) {
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = rnd(25);
			    if (in_sight) {
				pline("%s suddenly falls asleep!",
				      Monnam(mtmp));
			    }
			}
			steedhit = TRUE;
			break;
		case LANDMINE:
			if (thitm(mtmp, rnd(16), FALSE))
			    trapkilled = TRUE;
			steedhit = TRUE;
			break;
		case PIT:
		case SPIKED_PIT:
			if(In_outlands(&u.uz)){
				if (in_sight && hates_silver(mtmp->data) && tt == SPIKED_PIT) {
					pline("The silver mirror-shards sear %s!",
						mon_nam(mtmp));
				}
				if (mtmp->mhp <= 0 ||
					thitm(mtmp, rnd((tt == PIT) ? 6 : 12) + ((tt == SPIKED_PIT && hates_silver(mtmp->data)) ? rnd(20) : 0), FALSE))
					trapkilled = TRUE;
				steedhit = TRUE;
			} else {
				if (in_sight && hates_iron(mtmp->data) && tt == SPIKED_PIT) {
					pline("The cold-iron sears %s!", 	//half cold-iron damage
						mon_nam(mtmp));
				}
				if(hates_iron(mtmp->data) && tt == SPIKED_PIT){
					mtmp->mironmarked = TRUE;
				}
				if (mtmp->mhp <= 0 ||
					thitm(mtmp, rnd((tt == PIT) ? 6 : 10) + ((tt == SPIKED_PIT && hates_iron(mtmp->data)) ? rnd(mtmp->m_lev) : 0), FALSE))
					trapkilled = TRUE;
				steedhit = TRUE;
			}
			break;
		case POLY_TRAP: 
			if (!resists_magm(mtmp) && !resists_poly(mtmp->data)) {
				if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
					(void) newcham(mtmp, NON_PM, FALSE, FALSE);
					if (!can_saddle(mtmp, which_armor(mtmp, W_SADDLE)) || !can_ride(mtmp)) {
						dismount_steed(DISMOUNT_POLY);
					} else {
						You("have to adjust yourself in the saddle on %s.",
						x_monnam(mtmp,
						M_HAS_NAME(mtmp) ? ARTICLE_NONE : ARTICLE_A,
						(char *)0, SUPPRESS_SADDLE, FALSE));
					}
				}
				steedhit = TRUE;
			}
		break;
		default:
		return 0;
	}
	if(trapkilled) {
		dismount_steed(DISMOUNT_POLY);
		return 2;
	}
	else if(steedhit) return 1;
	else return 0;
}
#endif /*STEED*/

/* some actions common to both player and monsters for triggered landmine */
void
blow_up_landmine(trap)
struct trap *trap;
{
	struct monst * shkp =  (struct monst *)0;
	boolean costly = trap->madeby_u && costly_spot(trap->tx, trap->ty) &&
				   ((shkp = shop_keeper(*in_rooms(trap->tx, trap->ty, SHOPBASE))) != (struct monst *)0);
	boolean insider = (*u.ushops && inside_shop(u.ux, u.uy) && *in_rooms(trap->tx, trap->ty, SHOPBASE) == *u.ushops);
	long loss = 0L;

	(void)scatter(trap->tx, trap->ty, 4,
		MAY_DESTROY | MAY_HIT | MAY_FRACTURE | VIS_EFFECTS,
		(struct obj *)0, &loss, shkp);
	/* may have caused shk loss */
	if (costly && loss) {
		if(insider)
			You("owe %ld %s for objects destroyed.",
					loss, currency(loss));
		else {
			You("caused %ld %s worth of damage!",
					loss, currency(loss));
			make_angry_shk(shkp, trap->tx, trap->ty);
		}
	}
	del_engr_ward_at(trap->tx, trap->ty);
	wake_nearto_noisy(trap->tx, trap->ty, 400);
	/* ALI - artifact doors from Slash'em */
	if (IS_DOOR(levl[trap->tx][trap->ty].typ) &&
		!artifact_door(trap->tx, trap->ty))
	    levl[trap->tx][trap->ty].doormask = D_BROKEN;
	/* TODO: destroy drawbridge if present */
	/* caller may subsequently fill pit, e.g. with a boulder */
	trap->ttyp = PIT;		/* explosion creates a pit */
	trap->madeby_u = FALSE;		/* resulting pit isn't yours */
	seetrap(trap);			/* and it isn't concealed */
}

#endif /* OVLB */
#ifdef OVL3

/*
 * Move object of type otyp from one set of trap's launch coordinates to other.
 *
 * Return 0 if no object was launched.
 *        1 if an object was launched and placed somewhere.
 *        2 if an object was launched, but used up.
 */
int
launch_obj(otyp, trap, style)
short otyp;
struct trap * trap;
int style;
{
	register struct monst *mtmp;
	register struct obj *otmp, *otmp2;
	register int dx,dy;
	struct obj *singleobj;
	boolean used_up = FALSE;
	boolean otherside = FALSE;
	int dist;
	int tmp;
	int delaycnt = 0;
	int x1 = trap->launch.x,
		x2 = trap->launch2.x,
		y1 = trap->launch.y,
		y2 = trap->launch2.y;

	otmp = sobj_at(otyp, x1, y1);
	/* Try the other side too, for rolling boulder traps */
	if (!otmp && otyp == BOULDER) { /*only boulders*/
		otherside = TRUE;
		otmp = sobj_at(otyp, x2, y2);
	}
	if (!otmp) return 0;
	if (otherside) {	/* swap 'em */
		int tx, ty;

		tx = x1; ty = y1;
		x1 = x2; y1 = y2;
		x2 = tx; y2 = ty;
	}

	if (otmp->quan == 1L) {
	    obj_extract_self(otmp);
	    singleobj = otmp;
	    otmp = (struct obj *) 0;
	} else {
	    singleobj = splitobj(otmp, 1L);
	    obj_extract_self(singleobj);
	}
	newsym(x1,y1);
	/* in case you're using a pick-axe to chop the boulder that's being
	   launched (perhaps a monster triggered it), destroy context so that
	   next dig attempt never thinks you're resuming previous effort */
	if ((otyp == BOULDER || otyp == STATUE) &&
	    singleobj->ox == digging.pos.x && singleobj->oy == digging.pos.y)
	    (void) memset((genericptr_t)&digging, 0, sizeof digging);

	dist = distmin(x1,y1,x2,y2);
	bhitpos.x = x1;
	bhitpos.y = y1;
	dx = sgn(x2 - x1);
	dy = sgn(y2 - y1);
	switch (style) {
	    case ROLL|LAUNCH_UNSEEN:
			if (otyp == BOULDER) {
			    You_hear(Hallucination ?
				     "someone bowling." :
				     "rumbling in the distance.");
			}
			style &= ~LAUNCH_UNSEEN;
			goto roll;
	    case ROLL|LAUNCH_KNOWN:
			/* use otrapped as a flag to ohitmon */
			singleobj->otrapped = 1;
			style &= ~LAUNCH_KNOWN;
			/* fall through */
	    roll:
	    case ROLL:
			delaycnt = 2;
			/* fall through */
	    default:
			if (!delaycnt) delaycnt = 1;
			if (!cansee(bhitpos.x,bhitpos.y)) curs_on_u();
			tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
			tmp_at(bhitpos.x, bhitpos.y);
	}

	/* Set the object in motion */
	while(dist-- > 0 && !used_up) {
		struct trap *t;
		tmp_at(bhitpos.x, bhitpos.y);
		tmp = delaycnt;

		/* dstage@u.washington.edu -- Delay only if hero sees it */
		if (cansee(bhitpos.x, bhitpos.y))
			while (tmp-- > 0) delay_output();

		bhitpos.x += dx;
		bhitpos.y += dy;
		t = t_at(bhitpos.x, bhitpos.y);
		
		if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
			if (otyp == BOULDER && throws_rocks(mtmp->data)) {
			    if (rn2(3)) {
				if (cansee(bhitpos.x, bhitpos.y))
					pline("%s snatches the boulder.",
						Monnam(mtmp));
				else
				    You_hear("a rumbling stop abruptly.");
				singleobj->otrapped = 0;
				(void) mpickobj(mtmp, singleobj);
				used_up = TRUE;
				break;
			    }
			}
			/* boulder may hit creature */
			int dieroll = rnd(20);
			int hitvalu = tohitval((struct monst *)0, mtmp, (struct attack *)0, singleobj, trap, HMON_PROJECTILE|HMON_FIRED|HMON_TRAP, 0, (int *) 0);
			if (hitvalu > dieroll || (dieroll == 1 && hitvalu > -10)) {
				struct obj ** sobj_p = &singleobj;
				hmon_with_trap(mtmp, sobj_p, trap, HMON_PROJECTILE|HMON_FIRED, dieroll);
				if(!(*sobj_p)) used_up = TRUE;
			}
			else if (cansee(bhitpos.x, bhitpos.y))
				miss(xname(singleobj), mtmp);
			if (used_up)
				break;
		}
		else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
			if (multi) nomul(0, NULL);
			if (!Invulnerable){
				/* boulder may hit you */
				int dieroll = rnd(20);
				int hitvalu = tohitval((struct monst *)0, &youmonst, (struct attack *)0, singleobj, trap, HMON_PROJECTILE|HMON_FIRED|HMON_TRAP, 0, (int *) 0);
				if (hitvalu > dieroll || (dieroll == 1 && hitvalu > -10)) {
					killer = "rolling boulder trap";
					killer_format = KILLED_BY_AN;
					struct obj ** sobj_p = &singleobj;
					hmon_with_trap(&youmonst, sobj_p, trap, HMON_PROJECTILE|HMON_FIRED, dieroll);
					if(!(*sobj_p)) used_up = TRUE;
				}
				else if (!Blind)
					pline("%s missses!", The(xname(singleobj)));
				else
					pline("It misses.");
				if (used_up)
					break;
				stop_occupation();
			}
		}
		if (style == ROLL) {
		    if (down_gate(bhitpos.x, bhitpos.y) != -1) {
		        if(ship_object(singleobj, bhitpos.x, bhitpos.y, FALSE)){
				used_up = TRUE;
				break;
			}
		    }
		    if (t && otyp == BOULDER) {
			switch(t->ttyp) {
			case LANDMINE:
			    if (rn2(10) > 2) {
			  	pline(
				  "KAABLAMM!!!%s",
				  cansee(bhitpos.x, bhitpos.y) ?
					" The rolling boulder triggers a land mine." : "");
				deltrap(t);
				del_engr_ward_at(bhitpos.x,bhitpos.y);
				place_object(singleobj, bhitpos.x, bhitpos.y);
				singleobj->otrapped = 0;
				break_boulder(singleobj);
				(void)scatter(bhitpos.x,bhitpos.y, 4,
					MAY_DESTROY|MAY_HIT|MAY_FRACTURE|VIS_EFFECTS,
					(struct obj *)0, (long *)0, (struct monst *)0);
				if (cansee(bhitpos.x,bhitpos.y))
					newsym(bhitpos.x,bhitpos.y);
			        used_up = TRUE;
			    }
			    break;		
			case LEVEL_TELEP:
			case TELEP_TRAP:
			    if (cansee(bhitpos.x, bhitpos.y))
			    	pline("Suddenly the rolling boulder disappears!");
			    else
			    	You_hear("a rumbling stop abruptly.");
			    singleobj->otrapped = 0;
			    if (t->ttyp == TELEP_TRAP)
				rloco(singleobj);
			    else {
				int newlev = random_teleport_level();
				d_level dest;

				if (newlev == depth(&u.uz) || In_endgame(&u.uz))
				    continue;
				add_to_migration(singleobj);
				get_level(&dest, newlev);
				singleobj->ox = dest.dnum;
				singleobj->oy = dest.dlevel;
				singleobj->owornmask = (long)MIGR_RANDOM;
			    }
		    	    seetrap(t);
			    used_up = TRUE;
			    break;
			case PIT:
			case SPIKED_PIT:
			case HOLE:
			case TRAPDOOR:
			    /* the boulder won't be used up if there is a
			       monster in the trap; stop rolling anyway */
			    x2 = bhitpos.x,  y2 = bhitpos.y;  /* stops here */
			    if (flooreffects(singleobj, x2, y2, "fall"))
				used_up = TRUE;
			    dist = -1;	/* stop rolling immediately */
			    break;
			}
			if (used_up || dist == -1) break;
		    }
		    if (flooreffects(singleobj, bhitpos.x, bhitpos.y, "fall")) {
			used_up = TRUE;
			break;
		    }
		    if (otyp == BOULDER &&
		       (otmp2 = sobj_at(BOULDER, bhitpos.x, bhitpos.y)) != 0) {/*only actual boulders*/
			const char *bmsg =
				     " as one boulder sets another in motion";

			if (!isok(bhitpos.x + dx, bhitpos.y + dy) || !dist ||
			    IS_ROCK(levl[bhitpos.x + dx][bhitpos.y + dy].typ))
			    bmsg = " as one boulder hits another";

			You_hear("a loud crash%s!",
				cansee(bhitpos.x, bhitpos.y) ? bmsg : "");
			obj_extract_self(otmp2);
			/* pass off the otrapped flag to the next boulder */
			otmp2->otrapped = singleobj->otrapped;
			singleobj->otrapped = 0;
			place_object(singleobj, bhitpos.x, bhitpos.y);
			singleobj = otmp2;
			otmp2 = (struct obj *)0;
			wake_nearto_noisy(bhitpos.x, bhitpos.y, 10*10);
		    }
		}
		if (otyp == BOULDER && closed_door(bhitpos.x,bhitpos.y)) {
			if (cansee(bhitpos.x, bhitpos.y))
				pline_The("boulder crashes through a door.");
			levl[bhitpos.x][bhitpos.y].doormask = D_BROKEN;
			if (dist) unblock_point(bhitpos.x, bhitpos.y);
		}

		/* if about to hit iron bars, do so now */
		if (dist > 0 && isok(bhitpos.x + dx,bhitpos.y + dy) &&
			levl[bhitpos.x + dx][bhitpos.y + dy].typ == IRONBARS) {
		    x2 = bhitpos.x,  y2 = bhitpos.y;	/* object stops here */
		    if (hits_bars(&singleobj, x2, y2, !rn2(20), 0)) {
			if (!singleobj) used_up = TRUE;
			break;
		    }
		}
	}
	tmp_at(DISP_END, 0);
	if (!used_up) {
		singleobj->otrapped = 0;
		place_object(singleobj, x2,y2);
		newsym(x2,y2);
		return 1;
	} else
		return 2;
}

#endif /* OVL3 */
#ifdef OVLB

void
seetrap(trap)
	register struct trap *trap;
{
	if(!trap->tseen) {
	    trap->tseen = 1;
	    newsym(trap->tx, trap->ty);
	}
}

#endif /* OVLB */
#ifdef OVL3

STATIC_OVL int
mkroll_launch(ttmp, x, y, otyp, ocount)
struct trap *ttmp;
xchar x,y;
short otyp;
long ocount;
{
	struct obj *otmp;
	register int tmp;
	schar dx,dy;
	int distance;
	coord cc;
	coord bcc;
	int trycount = 0;
	boolean success = FALSE;
	int mindist = 4;

	if (ttmp->ttyp == ROLLING_BOULDER_TRAP) mindist = 2;
	distance = rn1(5,4);    /* 4..8 away */
	tmp = rn2(8);		/* randomly pick a direction to try first */
	while (distance >= mindist) {
		dx = xdir[tmp];
		dy = ydir[tmp];
		cc.x = x; cc.y = y;
		/* Prevent boulder from being placed on water */
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP
				&& is_pool(x+distance*dx,y+distance*dy, FALSE))
			success = FALSE;
		else success = isclearpath(&cc, distance, dx, dy);
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
			boolean success_otherway;
			bcc.x = x; bcc.y = y;
			success_otherway = isclearpath(&bcc, distance,
						-(dx), -(dy));
			if (!success_otherway) success = FALSE;
		}
		if (success) break;
		if (++tmp > 7) tmp = 0;
		if ((++trycount % 8) == 0) --distance;
	}
	if (!success) {
	    /* create the trap without any ammo, launch pt at trap location */
		cc.x = bcc.x = x;
		cc.y = bcc.y = y;
	} else {
		otmp = mksobj(otyp, NO_MKOBJ_FLAGS);
		otmp->quan = ocount;
		otmp->owt = weight(otmp);
		place_object(otmp, cc.x, cc.y);
		stackobj(otmp);
	}
	ttmp->launch.x = cc.x;
	ttmp->launch.y = cc.y;
	if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		ttmp->launch2.x = bcc.x;
		ttmp->launch2.y = bcc.y;
	} else
		ttmp->launch_otyp = otyp;
	newsym(ttmp->launch.x, ttmp->launch.y);
	return 1;
}

STATIC_OVL boolean
isclearpath(cc,distance,dx,dy)
coord *cc;
int distance;
schar dx,dy;
{
	uchar typ;
	xchar x, y;

	x = cc->x;
	y = cc->y;
	while (distance-- > 0) {
		x += dx;
		y += dy;
		typ = levl[x][y].typ;
		if (!isok(x,y) || !ZAP_POS(typ) || closed_door(x,y))
			return FALSE;
	}
	cc->x = x;
	cc->y = y;
	return TRUE;
}
#endif /* OVL3 */
#ifdef OVL1

boolean 
spire_fall_mon(mtmp)
register struct monst *mtmp;
{
	if(Is_sigil(&u.uz) && levl[mtmp->mx][mtmp->my].typ == AIR && (!mon_resistance(mtmp, FLYING) && !mon_resistance(mtmp, LEVITATION))){
		if(canseemon(mtmp)) pline("%s falls from the spire!",Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&spire_level),
				      MIGR_RANDOM, (coord *)0);	
		return TRUE;
	}
	return FALSE;
}

int
mintrap(mtmp)
struct monst *mtmp;
{
	register struct trap *trap = t_at(mtmp->mx, mtmp->my);
	boolean trapkilled = FALSE;
	struct permonst *mptr = mtmp->data;
	struct obj *otmp;

	if (!trap) {
	    mtmp->mtrapped = 0;	/* perhaps teleported? */
	} else if (mtmp->mtrapped) {	/* is currently in the trap */
	    if (!trap->tseen &&
		cansee(mtmp->mx, mtmp->my) && canseemon(mtmp) &&
		(trap->ttyp == SPIKED_PIT || trap->ttyp == BEAR_TRAP || 
		 trap->ttyp == FLESH_HOOK ||
		 trap->ttyp == HOLE || trap->ttyp == PIT ||
		 trap->ttyp == WEB)) {
		/* If you come upon an obviously trapped monster, then
		 * you must be able to see the trap it's in too.
		 */
		seetrap(trap);
	    }
		
	    if (!rn2(40)) {
		if (boulder_at(mtmp->mx, mtmp->my) &&
			(trap->ttyp == PIT || trap->ttyp == SPIKED_PIT)) {
		    if (!rn2(2)) {
				mtmp->mtrapped = 0;
				if (canseemon(mtmp))
					pline("%s pulls free...", Monnam(mtmp));
				fill_pit(mtmp->mx, mtmp->my);
		    }
		} else {
		    mtmp->mtrapped = 0;
		}
	    } else if (metallivorous(mptr)) {
		if (trap->ttyp == BEAR_TRAP && is_metallic(trap->ammo)) {
			if (canseemon(mtmp)) {
				pline("%s eats %s!",
					Monnam(mtmp),
					an(xname(trap->ammo)));
			}
		    deltrap(trap);
		    mtmp->meating = 5;
		    mtmp->mtrapped = 0;
		} else if (trap->ttyp == SPIKED_PIT && !In_outlands(&u.uz)) {
		    if (canseemon(mtmp))
			pline("%s munches on some spikes!", Monnam(mtmp));
		    trap->ttyp = PIT;
		    mtmp->meating = 5;
		}
	    }
	} else {
	    register int tt = trap->ttyp;
	    boolean in_sight, tear_web, see_it,
		    inescapable = (((tt == HOLE || tt == PIT) &&
				   In_sokoban(&u.uz) && !trap->madeby_u) || tt == STATUE_TRAP);
	    const char *fallverb;

	    /* true when called from dotrap, inescapable is not an option */
	    if (mtmp == u.usteed) inescapable = TRUE;

	    /* web-rippers always rip webs */
		if(species_tears_webs(mtmp->data) && tt == WEB)
			inescapable = TRUE;

	    if (!inescapable &&
		    ((mtmp->mtrapseen & (1 << (tt-1))) != 0 ||
			(tt == HOLE && !mindless_mon(mtmp)))) {
		/* it has been in such a trap - perhaps it escapes */
		if(rn2(4)) return(0);
	    } else {
		mtmp->mtrapseen |= (1 << (tt-1));
	    }
	    /* Monster is aggravated by being trapped by you.
	       Recognizing who made the trap isn't completely
	       unreasonable; everybody has their own style. */
	    if (trap->madeby_u && rnl(100) >= 20) setmangry(mtmp);

	    in_sight = canseemon(mtmp);
	    see_it = cansee(mtmp->mx, mtmp->my);
#ifdef STEED
	    /* assume hero can tell what's going on for the steed */
	    if (mtmp == u.usteed) in_sight = TRUE;
#endif
	    switch (tt) {
		case ARROW_TRAP:
		case DART_TRAP:
			if (!trap->ammo) {
				if (in_sight && see_it)
					pline("%s triggers a trap but nothing happens.",
					Monnam(mtmp));
				deltrap(trap);
				newsym(mtmp->mx, mtmp->my);
				break;
			}
			otmp = trap->ammo;
			if (trap->ammo->quan > 1) {
				otmp = splitobj(trap->ammo, 1);
			}
			extract_nobj(otmp, &trap->ammo);
			if (in_sight)
				seetrap(trap);
			if (projectile((struct monst *)0, otmp, trap, HMON_PROJECTILE|HMON_FIRED|HMON_TRAP, trap->tx, trap->ty, 0, 0, 0, 0, FALSE, FALSE, FALSE)&MM_DEF_DIED)
				trapkilled = TRUE;
			break;
		case ROCKTRAP:
			if (!(otmp = trap->ammo)) {
			    if (in_sight && see_it)
				pline("A trap door above %s opens, but nothing falls out!",
				      mon_nam(mtmp));
			    deltrap(trap);
			    newsym(mtmp->mx, mtmp->my);
			    break;
			}
			if (trap->ammo->quan > 1) {
				otmp = splitobj(trap->ammo, 1);
			}
			extract_nobj(otmp, &trap->ammo);
			if (in_sight) seetrap(trap);

			if (projectile((struct monst *)0, otmp, trap, HMON_PROJECTILE|HMON_FIRED|HMON_TRAP, trap->tx, trap->ty, 0, 0, 0, 0, FALSE, FALSE, FALSE)&MM_DEF_DIED)
			    trapkilled = TRUE;
			break;

		case SQKY_BOARD:
			if(mon_resistance(mtmp,FLYING)) break;
			/* stepped on a squeaky board */
			if (in_sight) {
			    pline("A board beneath %s squeaks loudly.", mon_nam(mtmp));
			    seetrap(trap);
			} else
			   You_hear("a distant squeak.");
			/* wake up nearby monsters */
			wake_nearto_noisy(mtmp->mx, mtmp->my, 40);
			break;

		case BEAR_TRAP:
			if(mptr->msize > MZ_SMALL &&
				!amorphous(mptr) && !mon_resistance(mtmp,FLYING) &&
				!is_whirly(mptr) && !unsolid(mptr)) {
			    mtmp->mtrapped = 1;
			    if(in_sight) {
				pline("%s is caught in %s %s!",
				      Monnam(mtmp), a_your[trap->madeby_u], xname(trap->ammo));
				seetrap(trap);
			    } else {
				if((mptr->mtyp == PM_OWLBEAR
				    || mptr->mtyp == PM_BUGBEAR)
				   && flags.soundok)
				    You_hear("the roaring of an angry bear!");
			    }

				if (hmon_with_trap(mtmp, &(trap->ammo), trap, HMON_WHACK, rnd(20))&MM_DEF_DIED)
					trapkilled = TRUE;
			}
			break;

		case FLESH_HOOK:
			if(	!amorphous(mptr) &&
				!is_whirly(mptr) && !unsolid(mptr)) {
			    mtmp->mtrapped = 1;
			    if(in_sight) {
					pline("%s is caught by %s %s!",
						  Monnam(mtmp), a_your[trap->madeby_u], xname(trap->ammo));
					seetrap(trap);
			    }

				if (hmon_with_trap(mtmp, &(trap->ammo), trap, HMON_WHACK, rnd(20))&MM_DEF_DIED)
					trapkilled = TRUE;
			}
			break;

		case SLP_GAS_TRAP:
		    if (!resists_sleep(mtmp) && !breathless_mon(mtmp) &&
				!mtmp->msleeping && mtmp->mcanmove) {
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = rnd(25);
			    if (in_sight) {
				pline("%s suddenly falls asleep!",
				      Monnam(mtmp));
				seetrap(trap);
			    }
			}
			break;

		case RUST_TRAP:
		    {
			struct obj *target;

			if (in_sight)
			    seetrap(trap);
			if(mon_resistance(mtmp, WATERPROOF)){
			    if (in_sight)
					pline("Water gushes around %s!", mon_nam(mtmp));
				break;
			}
			switch (rn2(5)) {
			case 0:
			    if (in_sight)
				pline("%s %s on the %s!", A_gush_of_water_hits,
				    mon_nam(mtmp), mbodypart(mtmp, HEAD));
			    target = which_armor(mtmp, W_ARMH);
			    (void) rust_dmg(target, "helmet", 1, TRUE, mtmp, FALSE);
			    break;
			case 1:
			    if (in_sight)
				pline("%s %s's left %s!", A_gush_of_water_hits,
				    mon_nam(mtmp), mbodypart(mtmp, ARM));
			    target = which_armor(mtmp, W_ARMS);
			    if (rust_dmg(target, "shield", 1, TRUE, mtmp, FALSE))
				break;
			    target = MON_WEP(mtmp);
			    if (target && bimanual(target,mtmp->data))
				erode_obj(target, FALSE, TRUE);
glovecheck:		    target = which_armor(mtmp, W_ARMG);
			    (void) rust_dmg(target, "gauntlets", 1, TRUE, mtmp, FALSE);
			    break;
			case 2:
			    if (in_sight)
				pline("%s %s's right %s!", A_gush_of_water_hits,
				    mon_nam(mtmp), mbodypart(mtmp, ARM));
			    erode_obj(MON_WEP(mtmp), FALSE, TRUE);
			    goto glovecheck;
			default:
			    if (in_sight)
				pline("%s %s!", A_gush_of_water_hits,
				    mon_nam(mtmp));
			    for (otmp=mtmp->minvent; otmp; otmp = otmp->nobj)
				(void) snuff_lit(otmp);
			    target = which_armor(mtmp, W_ARMC);
			    if (target)
				(void) rust_dmg(target, cloak_simple_name(target),
						 1, TRUE, mtmp, FALSE);
			    else {
				target = which_armor(mtmp, W_ARM);
				if (target)
				    (void) rust_dmg(target, "armor", 1, TRUE, mtmp, FALSE);
#ifdef TOURIST
				else {
				    target = which_armor(mtmp, W_ARMU);
				    (void) rust_dmg(target, "shirt", 1, TRUE, mtmp, FALSE);
				}
#endif
			    }
			}
			if (is_iron(mptr)) {
				if (in_sight)
				    pline("%s falls to pieces!", Monnam(mtmp));
				else if(mtmp->mtame)
				    pline("May %s rust in peace.",
								mon_nam(mtmp));
				mondied(mtmp);
				if (mtmp->mhp <= 0)
					trapkilled = TRUE;
			} else if (mptr->mtyp == PM_FLAMING_SPHERE) {
				if (in_sight)
				    pline("%s is extinguished!", Monnam(mtmp));
				mondied(mtmp);
				if (mtmp->mhp <= 0)
					trapkilled = TRUE;
			} else if (mptr->mtyp == PM_GREMLIN && rn2(3)) {
				(void)split_mon(mtmp, (struct monst *)0);
			}
			break;
		    }
		case FIRE_TRAP:
			if (!(otmp = trap->ammo)) {
				if (in_sight && see_it)
					pline("%s triggers a trap but nothing happens.", Monnam(mtmp));
				deltrap(trap);
				newsym(mtmp->mx, mtmp->my);
				break;
			}
			if (!(Is_firelevel(&u.uz)) &&	/* never useup on plane of fire */
				!(Inhell && rn2(5)) &&		/* useup 80% less often in gehennom */
				!(rn2(2))) {				/* useup only 50% of the time base */
				if (otmp->quan > 1)
					otmp->quan--;
				else {
					extract_nobj(otmp, &(trap->ammo));
					delobj(otmp);
				}
			}
 mfiretrap:
			if (IS_PUDDLE(levl[mtmp->mx][mtmp->my].typ)) {
			    if (in_sight)
			        pline("A cascade of steamy bubbles erupts from the %s under %s!",
				  surface(mtmp->mx,mtmp->my), mon_nam(mtmp));
			    else if (see_it) You("see a cascade of steamy bubbles erupt from the %s!",
				surface(mtmp->mx,mtmp->my));
			    if(rn2(2)) {
				if (in_sight) pline_The("water evaporates!");
				levl[mtmp->mx][mtmp->my].typ = ROOM;
			    }
			    if (resists_fire(mtmp)) {
				if (in_sight) {
				    shieldeff(mtmp->mx,mtmp->my);
				    pline("%s is uninjured.", Monnam(mtmp));
				}
			    } else if (thitm(mtmp, rnd(3), FALSE))
				trapkilled = TRUE;
			    if (see_it) seetrap(trap);
			    break;
			}
			if (in_sight)
			    pline("A %s erupts from the %s under %s!",
				  tower_of_flame,
				  surface(mtmp->mx,mtmp->my), mon_nam(mtmp));
			else if (see_it)  /* evidently `mtmp' is invisible */
			    You("see a %s erupt from the %s!",
				tower_of_flame, surface(mtmp->mx,mtmp->my));

			if (resists_fire(mtmp)) {
			    if (in_sight) {
				shieldeff(mtmp->mx,mtmp->my);
				pline("%s is uninjured.", Monnam(mtmp));
			    }
			} else {
			    int num = d(2,4), alt;
			    boolean immolate = FALSE;

			    /* paper burns very fast, assume straw is tightly
			     * packed and burns a bit slower */
			    switch (monsndx(mtmp->data)) {
				case PM_SPELL_GOLEM:
			    case PM_PAPER_GOLEM:   
				case PM_MIGO_WORKER:
						   immolate = TRUE;
						   alt = mtmp->mhpmax; break;
			    case PM_STRAW_GOLEM:   alt = mtmp->mhpmax / 2; break;
			    case PM_WOOD_GOLEM:    alt = mtmp->mhpmax / 4; break;
			    case PM_GROVE_GUARDIAN:    alt = mtmp->mhpmax / 4; break;
			    case PM_LIVING_LECTERN:    alt = mtmp->mhpmax / 4; break;
			    case PM_LEATHER_GOLEM: alt = mtmp->mhpmax / 8; break;
			    default: alt = 0; break;
			    }
			    if (alt > num) num = alt;

			    if (thitm(mtmp, num, immolate))
				trapkilled = TRUE;
			    else
				/* we know mhp is at least `num' below mhpmax,
				   so no (mhp > mhpmax) check is needed here */
				mtmp->mhpmax -= rn2(num + 1);
			}
			if (burnarmor(mtmp, FALSE) || rn2(3)) {
			    (void) destroy_item(mtmp, SCROLL_CLASS, AD_FIRE);
			    (void) destroy_item(mtmp, SPBOOK_CLASS, AD_FIRE);
			    (void) destroy_item(mtmp, POTION_CLASS, AD_FIRE);
			}
			if (burn_floor_paper(mtmp->mx, mtmp->my, see_it, FALSE) &&
				!see_it && distu(mtmp->mx, mtmp->my) <= 3*3)
			    You("smell smoke.");
			if (is_ice(mtmp->mx,mtmp->my))
			    melt_ice(mtmp->mx,mtmp->my);
			if (see_it) seetrap(trap);
			break;

		case PIT:
		case SPIKED_PIT:
			fallverb = "falls";
			if (mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION) ||
				(mtmp->wormno && count_wsegs(mtmp) > 5) ||
				is_clinger(mptr)) {
			    if (!inescapable) break;	/* avoids trap */
			    fallverb = "is dragged";	/* sokoban pit */
			}
			if (!mon_resistance(mtmp,PASSES_WALLS))
			    mtmp->mtrapped = 1;
			if (in_sight) {
			    pline("%s %s into %s pit!",
				  Monnam(mtmp), fallverb,
				  a_your[trap->madeby_u]);
			    if (mptr->mtyp == PM_PIT_VIPER || mptr->mtyp == PM_PIT_FIEND)
				pline("How pitiful.  Isn't that the pits?");
			    seetrap(trap);
			}
			if(In_outlands(&u.uz)){
				if (in_sight && hates_silver(mtmp->data) && tt == SPIKED_PIT) {
					pline("The silver mirror-shards sear %s!",
						mon_nam(mtmp));
				}
				mselftouch(mtmp, "Falling, ", FALSE);
				if (mtmp->mhp <= 0 ||
					thitm(mtmp, rnd((tt == PIT) ? 6 : 12) + ((tt == SPIKED_PIT && hates_silver(mtmp->data)) ? rnd(20) : 0), FALSE))
					trapkilled = TRUE;
			} else {
				if (in_sight && hates_iron(mtmp->data) && tt == SPIKED_PIT) {
					pline("The cold-iron sears %s!",
						mon_nam(mtmp));
				}
				if(hates_iron(mtmp->data) && tt == SPIKED_PIT){
					mtmp->mironmarked = TRUE;
				}
				mselftouch(mtmp, "Falling, ", FALSE);
				if (mtmp->mhp <= 0 ||
					thitm(mtmp, rnd((tt == PIT) ? 6 : 10) + ((tt == SPIKED_PIT && hates_iron(mtmp->data)) ? rnd(mtmp->m_lev) : 0), FALSE))
					trapkilled = TRUE;
			}
			break;
		case HOLE:
		case TRAPDOOR:
			if (!Can_fall_thru(&u.uz)) {
			 impossible("mintrap: %ss cannot exist on this level.",
				    defsyms[trap_to_defsym(tt)].explanation);
			    break;	/* don't activate it after all */
			}
			if(mtmp->mhp <= 0) break;
			if (mon_resistance(mtmp,FLYING) || mon_resistance(mtmp,LEVITATION) ||
				mptr->mtyp == PM_WUMPUS ||
				(mtmp->wormno && count_wsegs(mtmp) > 5) ||
				mptr->msize >= MZ_HUGE) {
			    if (inescapable) {	/* sokoban hole */
				if (in_sight) {
				    pline("%s seems to be yanked down!",
					  Monnam(mtmp));
				    /* suppress message in mlevel_tele_trap() */
				    in_sight = FALSE;
				    seetrap(trap);
				}
			    } else
				break;
			}
			/* Fall through */
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
			{
			    int mlev_res;
				if(mtmp->mhp <= 0) break;
			    mlev_res = mlevel_tele_trap(mtmp, trap,
							inescapable, in_sight);
			    if (mlev_res) return(mlev_res);
			}
			break;

		case TELEP_TRAP:
			mtele_trap(mtmp, trap, in_sight);
			break;

		case WEB:
			/* Monster in a web. */
			if (webmaker(mptr) || (Is_lolth_level(&u.uz) && !mtmp->mpeaceful)) break;
			if (amorphous(mptr) || is_whirly(mptr) || unsolid(mptr)){
			    if(acidic(mptr) ||
			       mptr->mtyp == PM_GELATINOUS_CUBE ||
			       mptr->mtyp == PM_FIRE_ELEMENTAL) {
				if (in_sight)
				    pline("%s %s %s spider web!",
					  Monnam(mtmp),
					  (mptr->mtyp == PM_FIRE_ELEMENTAL) ?
					    "burns" : "dissolves",
					  a_your[trap->madeby_u]);
				if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
					deltrap(trap);
					newsym(mtmp->mx, mtmp->my);
				}
				break;
			    }
			    if (in_sight) {
				pline("%s flows through %s spider web.",
				      Monnam(mtmp),
				      a_your[trap->madeby_u]);
				seetrap(trap);
			    }
			    break;
			}
			{
				struct obj *mwep;
				mwep = MON_WEP(mtmp);
				if(mwep && (mwep->oartifact == ART_STING || mwep->oartifact == ART_LIECLEAVER)){
					pline("%s slices through %s spider web.",
						  Monnam(mtmp),
						  a_your[trap->madeby_u]);
					if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
						deltrap(trap);
						newsym(mtmp->mx, mtmp->my);
					}
					break;
				}
			}
			tear_web = FALSE;
			/* this list is fairly arbitrary; it deliberately
			   excludes wumpus & giant/ettin zombies/mummies */
			if(species_tears_webs(mptr))
				tear_web = TRUE;
			switch (monsndx(mptr)) {
			    case PM_OWLBEAR: /* Eric Backus */
			    case PM_BUGBEAR:
				if (!in_sight) {
				    You_hear("the roaring of a confused bear!");
				    mtmp->mtrapped = 1;
				    break;
				}
				/* fall though */
			    default:
				if ((mtmp->wormno && count_wsegs(mtmp) > 5)) {
				    tear_web = TRUE;
				} else if (in_sight) {
				    pline("%s is caught in %s spider web.",
					  Monnam(mtmp),
					  a_your[trap->madeby_u]);
				    seetrap(trap);
				}
				mtmp->mtrapped = tear_web ? 0 : 1;
				break;
			}
			if (tear_web) {
			    if (in_sight)
				pline("%s tears through %s spider web!",
				      Monnam(mtmp), a_your[trap->madeby_u]);
				if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
					deltrap(trap);
					newsym(mtmp->mx, mtmp->my);
				}
			}
			break;

	    case VIVI_TRAP:
			if (in_sight)
				pline("%s smashes through the delicate equipment!",
					  Monnam(mtmp));
			deltrap(trap);
			newsym(mtmp->mx, mtmp->my);
		break;
	    case STATUE_TRAP:
			(void) activate_statue_trap(trap, trap->tx, trap->ty, FALSE);
		break;

		case MAGIC_TRAP:
			/* A magic trap.  Monsters usually immune. */
			if (!rn2(21)) goto mfiretrap;
			break;
		case ANTI_MAGIC:
			break;

		case LANDMINE:
			if(rn2(3))
				break; /* monsters usually don't set it off */
			if(mon_resistance(mtmp,FLYING)) {
				boolean already_seen = trap->tseen;
				if (in_sight && !already_seen) {
	pline("A trigger appears in a pile of soil below %s.", mon_nam(mtmp));
					seetrap(trap);
				}
				if (rn2(3)) break;
				if (in_sight) {
					newsym(mtmp->mx, mtmp->my);
					pline_The("air currents set %s off!",
					  already_seen ? "a land mine" : "it");
				}
			} else if(in_sight) {
			    newsym(mtmp->mx, mtmp->my);
			    pline("KAABLAMM!!!  %s triggers %s land mine!",
				Monnam(mtmp), a_your[trap->madeby_u]);
			}
			if (!in_sight)
				pline("Kaablamm!  You hear an explosion in the distance!");
			blow_up_landmine(trap);
			if (thitm(mtmp, rnd(16), FALSE))
				trapkilled = TRUE;
			else {
				/* monsters recursively fall into new pit */
				if (mintrap(mtmp) == 2) trapkilled=TRUE;
			}
			/* a boulder may fill the new pit, crushing monster */
			fill_pit(trap->tx, trap->ty);
			if (mtmp->mhp <= 0) trapkilled = TRUE;
			if (unconscious()) {
				multi = -1;
				nomovemsg="The explosion awakens you!";
			}
			break;

		case POLY_TRAP:
		    if (resists_magm(mtmp) || resists_poly(mtmp->data)) {
			shieldeff(mtmp->mx, mtmp->my);
		    } else if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
			(void) newcham(mtmp, NON_PM,
				       FALSE, FALSE);
			if (in_sight) seetrap(trap);
		    }
		    break;

		case ROLLING_BOULDER_TRAP:
		    if (!mon_resistance(mtmp,FLYING) && !rolling_boulder_in_progress) {
			int style = ROLL | (in_sight ? 0 : LAUNCH_UNSEEN);

		        newsym(mtmp->mx,mtmp->my);
			if (in_sight)
			    pline("Click! %s triggers %s.", Monnam(mtmp),
				  trap->tseen ?
				  "a rolling boulder trap" :
				  something);
			rolling_boulder_in_progress = TRUE;
			if (launch_obj(BOULDER, trap, style)) {
			    if (in_sight) trap->tseen = TRUE;
			    if (mtmp->mhp <= 0) trapkilled = TRUE;
			} else {
			    deltrap(trap);
			    newsym(mtmp->mx,mtmp->my);
			}
			rolling_boulder_in_progress = FALSE;
		    }
		    break;
		case SWITCH_TRAP:
		case MUMMY_TRAP:
		// monsters can't activate these bad boys
		break;
		
		default:
			impossible("Some monster encountered a strange trap of type %d.", tt);
	    }
	}
	if(trapkilled) return 2;
	return mtmp->mtrapped;
}

#endif /* OVL1 */
#ifdef OVLB

/* Combine cockatrice checks into single functions to avoid repeating code. */
void
instapetrify(str)
const char *str;
{
	if (Stone_resistance) return;
	if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))
	    return;
	You("turn to stone...");
	killer_format = KILLED_BY;
	killer = str;
	done(STONING);
}

void
minstapetrify(mon,byplayer)
struct monst *mon;
boolean byplayer;
{
	if (resists_ston(mon)) return;
	if (poly_when_stoned(mon->data)) {
		mon_to_stone(mon);
		return;
	}
	else if(is_delouseable(mon->data)){
		mon = delouse(mon, AD_STON);
		return;
	}

	/* give a "<mon> is slowing down" message and also remove
	   intrinsic speed (comparable to similar effect on the hero) */
	mon_adjust_speed(mon, -3, (struct obj *)0, TRUE);

	if (cansee(mon->mx, mon->my))
		pline("%s turns to stone.", Monnam(mon));
	if (byplayer) {
		stoned = TRUE;
		xkilled(mon,0);
		stoned = FALSE;
	} else monstone(mon);
}

void
minstagoldify(mon,byplayer)
struct monst *mon;
int byplayer;
{
	if (resists_ston(mon)) return;
	if (poly_when_golded(mon->data)) {
		mon_to_gold(mon);
		return;
	}

	/* give a "<mon> is slowing down" message and also remove
	   intrinsic speed (comparable to similar effect on the hero) */
	mon_adjust_speed(mon, -3, (struct obj *)0, TRUE);

	if (cansee(mon->mx, mon->my))
		pline("%s turns to gold.", Monnam(mon));
	if (byplayer) {
		golded = TRUE;
		xkilled(mon,0);
		golded = FALSE;
	} else mongolded(mon);
}

void
minstaglass(mon,byplayer)
struct monst *mon;
boolean byplayer;
{
	/* give a "<mon> is slowing down" message and also remove
	   intrinsic speed (comparable to similar effect on the hero) */
	mon_adjust_speed(mon, -3, (struct obj *)0, TRUE);

	if (cansee(mon->mx, mon->my))
		pline("%s turns to glass.", Monnam(mon));
	if (byplayer) {
		glassed = TRUE;
		xkilled(mon,0);
		glassed = FALSE;
	} else monglassed(mon);
}

void
selftouch(arg)
const char *arg;
{
	char kbuf[BUFSZ];

	if(uwep && uwep->otyp == CORPSE && touch_petrifies(&mons[uwep->corpsenm])
			&& !Stone_resistance) {
		pline("%s touch the %s corpse.", arg,
		        mons[uwep->corpsenm].mname);
		Sprintf(kbuf, "%s corpse", an(mons[uwep->corpsenm].mname));
		instapetrify(kbuf);
	}
	/* Or your secondary weapon, if wielded */
	if(u.twoweap && uswapwep && uswapwep->otyp == CORPSE &&
			touch_petrifies(&mons[uswapwep->corpsenm]) && !Stone_resistance){
		pline("%s touch the %s corpse.", arg,
		        mons[uswapwep->corpsenm].mname);
		Sprintf(kbuf, "%s corpse", an(mons[uswapwep->corpsenm].mname));
		instapetrify(kbuf);
	}
}

void
mselftouch(mon,arg,byplayer)
struct monst *mon;
const char *arg;
boolean byplayer;
{
	struct obj *mwep = MON_WEP(mon);

	if (mwep && mwep->otyp == CORPSE && touch_petrifies(&mons[mwep->corpsenm])) {
		if (cansee(mon->mx, mon->my)) {
			pline("%s%s touches the %s corpse.",
			    arg ? arg : "", arg ? mon_nam(mon) : Monnam(mon),
			    mons[mwep->corpsenm].mname);
		}
		minstapetrify(mon, byplayer);
	}
}

void
float_up()
{
	if(u.utrap) {
		if(u.utraptype == TT_PIT) {
			u.utrap = 0;
			You("float up, out of the pit!");
			vision_full_recalc = 1;	/* vision limits change */
			fill_pit(u.ux, u.uy);
		} else if (u.utraptype == TT_INFLOOR) {
			Your("body pulls upward, but your %s are still stuck.",
			     makeplural(body_part(LEG)));
		} else {
			You("float up, only your %s is still stuck.",
				body_part(LEG));
		}
	}
	else if(Is_waterlevel(&u.uz))
		pline("It feels as though you've lost some weight.");
	else if(u.uinwater)
		spoteffects(TRUE);
	else if(u.uswallow)
		You(is_animal(u.ustuck->data) ?
			"float away from the %s."  :
			"spiral up into %s.",
		    is_animal(u.ustuck->data) ?
			surface(u.ux, u.uy) :
			mon_nam(u.ustuck));
	else if (Hallucination)
		pline("Up, up, and awaaaay!  You're walking on air!");
	else if(Weightless)
		You("gain control over your movements.");
	else
		You("start to float in the air!");
#ifdef STEED
	if (u.usteed && !mon_resistance(u.usteed,LEVITATION) &&
						!mon_resistance(u.usteed,FLYING)) {
	    if (Lev_at_will)
	    	pline("%s magically floats up!", Monnam(u.usteed));
	    else {
	    	You("cannot stay on %s.", mon_nam(u.usteed));
	    	dismount_steed(DISMOUNT_GENERIC);
	    }
	}
#endif
	return;
}

void
m_float_up(mon, silently)
struct monst *mon;
boolean silently;
{
	boolean seen = canseemon(mon);

	if (mon->mtrapped) {
		struct trap* ttmp = t_at(mon->mx, mon->my);
		if (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) {
			mon->mtrapped = 0;
			if (seen && !silently)
				pline("%s floats up, out of the pit!", Monnam(mon));
			fill_pit(mon->mx, mon->my);
		}
		else if (ttmp->ttyp == BEAR_TRAP || ttmp->ttyp == FLESH_HOOK || ttmp->ttyp == WEB || ttmp->ttyp == VIVI_TRAP) {
			if (seen && !silently)
				pline("%s floats upward, but is still stuck.", Monnam(mon));
		}
	}
	else if (seen && !silently)
		pline("%s starts to float in the air!", Monnam(mon));
	return;
}

void
fill_pit(x, y)
int x, y;
{
	struct obj *otmp;
	struct trap *t;

	if ((t = t_at(x, y)) &&
	    ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT)) &&
	    (otmp = sobj_at(BOULDER, x, y))) {
		obj_extract_self(otmp);
		(void) flooreffects(otmp, x, y, "settle");
	}
}

int
float_down(hmask, emask)
long hmask, emask;     /* might cancel timeout */
{
	register struct trap *trap = (struct trap *)0;
	d_level current_dungeon_level;
	boolean no_msg = FALSE;

	HLevitation &= ~hmask;
	ELevitation &= ~emask;
	if(Levitation) return(0); /* maybe another ring/potion/boots */
	
    /* Unmaintain the levitation spell if applicable */
    if (spell_maintained(SPE_LEVITATION))
        spell_unmaintain(SPE_LEVITATION);

	if(u.uswallow) {
	    You("float down, but you are still %s.",
		is_animal(u.ustuck->data) ? "swallowed" : "engulfed");
	    return(1);
	}

	if (Punished && !carried(uball) &&
	    (is_pool(uball->ox, uball->oy, FALSE) ||
	     ((trap = t_at(uball->ox, uball->oy)) &&
	      ((trap->ttyp == PIT) || (trap->ttyp == SPIKED_PIT) ||
	       (trap->ttyp == TRAPDOOR) || (trap->ttyp == HOLE))))) {
			u.ux0 = u.ux;
			u.uy0 = u.uy;
			u.ux = uball->ox;
			u.uy = uball->oy;
			movobj(uchain, uball->ox, uball->oy);
			newsym(u.ux0, u.uy0);
			vision_full_recalc = 1;	/* in case the hero moved. */
	}
	/* check for falling into pool - added by GAN 10/20/86 */
	if(!Flying) {
		if (!u.uswallow && u.ustuck) {
			if (sticks(&youmonst))
				You("aren't able to maintain your hold on %s.",
					mon_nam(u.ustuck));
			else
				pline("Startled, %s can no longer hold you!",
					mon_nam(u.ustuck));
			u.ustuck = 0;
		}
		/* kludge alert:
		 * drown() and lava_effects() print various messages almost
		 * every time they're called which conflict with the "fall
		 * into" message below.  Thus, we want to avoid printing
		 * confusing, duplicate or out-of-order messages.
		 * Use knowledge of the two routines as a hack -- this
		 * should really be handled differently -dlc
		 */
		if(is_pool(u.ux,u.uy, FALSE) && !Wwalking && !Swimming && !u.uinwater)
			no_msg = drown();

		if(is_lava(u.ux,u.uy)) {
			(void) lava_effects(TRUE);
			no_msg = TRUE;
		}
	}
	if (!trap) {
	    trap = t_at(u.ux,u.uy);
	    if(Weightless)
			You("begin to tumble in place.");
	    else if (Is_waterlevel(&u.uz) && !no_msg)
			You_feel("heavier.");
	    /* u.uinwater msgs already in spoteffects()/drown() */
	    else if (!u.uinwater && !no_msg) {
#ifdef STEED
		if (!(emask & W_SADDLE))
#endif
		{
		    boolean sokoban_trap = (In_sokoban(&u.uz) && trap);
		    if (Hallucination)
			pline("Bummer!  You've %s.",
			      is_pool(u.ux,u.uy, TRUE) ?
			      "splashed down" : sokoban_trap ? "crashed" :
			      "hit the ground");
		    else {
			if (!sokoban_trap)
			    You("float gently to the %s.",
				surface(u.ux, u.uy));
			else {
			    /* Justification elsewhere for Sokoban traps
			     * is based on air currents. This is
			     * consistent with that.
			     * The unexpected additional force of the
			     * air currents once leviation
			     * ceases knocks you off your feet.
			     */
			    You("fall over.");
			    losehp(rnd(2), "dangerous winds", KILLED_BY);
#ifdef STEED
			    if (u.usteed) dismount_steed(DISMOUNT_FELL);
#endif
			    selftouch("As you fall, you");
			}
		    }
		}
	    }
	}

	/* can't rely on u.uz0 for detecting trap door-induced level change;
	   it gets changed to reflect the new level before we can check it */
	assign_level(&current_dungeon_level, &u.uz);

	if(!Levitation && !Flying && In_quest(&u.uz) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && levl[u.ux][u.uy].typ == AIR){
		if(on_level(&u.uz, &qstart_level) && !ok_to_quest()){
			pline("A mysterious force prevents you from falling.");
		} else {
			struct d_level target_level;
			target_level.dnum = u.uz.dnum;
			target_level.dlevel = qlocate_level.dlevel+1;
			int dist = qlocate_level.dlevel+1 - u.uz.dlevel;
			schedule_goto(&target_level, FALSE, TRUE, FALSE, "You plummet through the cavern air!", "You slam into the rocky floor!", d(dist*5,6), 0);
		}
	}
	else if(trap)
		switch(trap->ttyp) {
		case STATUE_TRAP:
			(void) activate_statue_trap(trap, trap->tx, trap->ty, FALSE);
		break;
		case HOLE:
		case TRAPDOOR:
			if(!Can_fall_thru(&u.uz) || u.ustuck)
				break;
			/* fall into next case */
		default:
			if (!u.utrap) /* not already in the trap */
				dotrap(trap, 0);
	}

	if (!Weightless && !Is_waterlevel(&u.uz) && !u.uswallow &&
		/* falling through trap door calls goto_level,
		   and goto_level does its own pickup() call */
		on_level(&u.uz, &current_dungeon_level))
	    (void) pickup(1);
	return 1;
}

/* assumes that the monster is now neither flying nor levitating */
void
m_float_down(mon, silently)
struct monst *mon;
boolean silently;
{
	register struct trap *trap = (struct trap *)0;
	boolean seen = canseemon(mon);

	if (is_pool(mon->mx, mon->my, TRUE) || is_lava(mon->mx, mon->my))
	{
		if (seen && !silently)
			pline("%s splashes down into the %s.", Monnam(mon), surface(mon->mx, mon->my));
		silently = TRUE;	/* disable other messages from this function */
		minliquid(mon);
	}
	trap = t_at(mon->mx, mon->my);
	boolean sokoban_trap = (In_sokoban(&u.uz) && trap);

	if (!sokoban_trap)
	{
		if (seen && !silently)
			pline("%s floats gently to the %s.", Monnam(mon), surface(mon->mx, mon->my));
	}
	else {
		if (seen && !silently)
			pline("%s falls over.", Monnam(mon));
		thitm(mon, rnd(2), FALSE);
		mselftouch(mon, "Falling, ", FALSE);
	}

	if (trap) {
		mintrap(mon);
	}

	return;
}


STATIC_OVL void
dofiretrap(box)
struct obj *box;	/* null for floor trap */
{
	boolean see_it = !Blind;
	int num, alt;

/* Bug: for box case, the equivalent of burn_floor_paper() ought
 * to be done upon its contents.
 */

	if ((box && !carried(box)) ? (is_pool(box->ox, box->oy, FALSE)) :
				     (Underwater || IS_PUDDLE(levl[u.ux][u.uy].typ))) {
	    pline("A cascade of steamy bubbles erupts from %s!",
		    the(box ? xname(box) : surface(u.ux,u.uy)));
	    if (Fire_resistance) You("are uninjured.");
	    else losehp(rnd(3), "boiling water", KILLED_BY);
	    if (IS_PUDDLE(levl[u.ux][u.uy].typ) && rn2(2)) {
			pline_The("water evaporates!");
			levl[u.ux][u.uy].typ = ROOM;
	    }
	    return;
	}
	pline("A %s %s from %s!", tower_of_flame,
	      box ? "bursts" : "erupts",
	      the(box ? xname(box) : surface(u.ux,u.uy)));
	if (Fire_resistance) {
	    shieldeff(u.ux, u.uy);
	    num = rn2(2);
	} else if (Upolyd) {
	    num = d(2,4);
	    switch (u.umonnum) {
	    case PM_SPELL_GOLEM:   alt = u.mhmax; break;
	    case PM_PAPER_GOLEM:   alt = u.mhmax; break;
	    case PM_STRAW_GOLEM:   alt = u.mhmax / 2; break;
	    case PM_WOOD_GOLEM:    alt = u.mhmax / 4; break;
	    case PM_GROVE_GUARDIAN:    alt = u.mhmax / 4; break;
	    case PM_LIVING_LECTERN:alt = u.mhmax / 4; break;
	    case PM_LEATHER_GOLEM: alt = u.mhmax / 8; break;
	    default: alt = 0; break;
	    }
	    if (alt > num) num = alt;
	    if (u.mhmax > mons[u.umonnum].mlevel)
		u.mhmax -= rn2(min(u.mhmax,num + 1)), flags.botl = 1;
	} else {
	    num = d(2,4);
	    if (u.uhpmax > u.ulevel){
			u.uhpmod -= rn2(min(u.uhpmax,num + 1));
			calc_total_maxhp();
		}
	}
	if (!num) You("are uninjured.");
	else losehp(num, tower_of_flame, KILLED_BY_AN);
	
	burn_away_slime();
	melt_frozen_air();

	if (burnarmor(&youmonst, FALSE) || (rn2(3) && !UseInvFire_res(&youmonst))) {
	    destroy_item(&youmonst, SCROLL_CLASS, AD_FIRE);
	    destroy_item(&youmonst, SPBOOK_CLASS, AD_FIRE);
	    destroy_item(&youmonst, POTION_CLASS, AD_FIRE);
	}
	if (!box && burn_floor_paper(u.ux, u.uy, see_it, TRUE) && !see_it)
	    You("smell paper burning.");
	if (is_ice(u.ux, u.uy))
	    melt_ice(u.ux, u.uy);
}

STATIC_OVL void
domagictrap()
{
	register int fate = rnd(20);

	/* What happened to the poor sucker? */

	if (fate < 10) {
	  /* Most of the time, it creates some monsters. */
	  register int cnt = rnd(4);

	  if (!resists_blnd(&youmonst)) {
		You("are momentarily blinded by a flash of light!");
		make_blinded((long)rn1(5,10),FALSE);
		if (!Blind) Your1(vision_clears);
	  } else if (!Blind) {
		You("see a flash of light!");
	  }  else
		You_hear("a deafening roar!");
	  while(cnt--)
		(void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
	}
	else
	  switch (fate) {

	     case 10:
	     case 11:
		      /* sometimes nothing happens */
			break;
	     case 12: /* a flash of fire */
			dofiretrap((struct obj *)0);
			break;

	     /* odd feelings */
	     case 13:	pline("A shiver runs up and down your %s!",
			      body_part(SPINE));
			break;
	     case 14:	You_hear(Hallucination ?
				"the moon howling at you." :
				"distant howling.");
			break;
	     case 15:	if (on_level(&u.uz, &qstart_level))
			    You_feel("%slike the prodigal son.",
			      (flags.female || (is_neuter(youracedata))) ?
				     "oddly " : "");
			else
			    You("suddenly yearn for %s.",
				Hallucination ? "Cleveland" :
			    (In_quest(&u.uz) || at_dgn_entrance("The Quest")) ?
						"your nearby homeland" :
						"your distant homeland");
			break;
	     case 16:   Your("pack shakes violently!");
			break;
	     case 17:	You(Hallucination ?
				"smell hamburgers." :
				"smell charred flesh.");
			break;
	     case 18:	You_feel("tired.");
			break;

	     /* very occasionally something nice happens. */

	     case 19:
		    /* tame nearby monsters */
		   {   register int i,j;
		       register struct monst *mtmp;

		       (void) adjattrib(A_CHA,1,FALSE);
		       for(i = -1; i <= 1; i++) for(j = -1; j <= 1; j++) {
			   if(!isok(u.ux+i, u.uy+j)) continue;
			   mtmp = m_at(u.ux+i, u.uy+j);
			   if(mtmp)
			       (void) tamedog(mtmp, (struct obj *)0);
		       }
		       break;
		   }

	     case 20:
		    /* uncurse stuff */
		   {	struct obj pseudo;
			long save_conf = HConfusion;

			pseudo = zeroobj;   /* neither cursed nor blessed */
			pseudo.otyp = SCR_REMOVE_CURSE;
			HConfusion = 0L;
			(void) seffects(&pseudo);
			HConfusion = save_conf;
			break;
		   }
	     default: break;
	  }
}

/*
 * Scrolls, spellbooks, potions, and flammable items
 * may get affected by the fire.
 *
 * Return number of objects destroyed. --ALI
 */
int
fire_damage_chain(chain, force, here, x, y)
struct obj *chain;
boolean force, here;
xchar x, y;
{
    struct obj *obj, *nobj;
	int num = 0;

    /* erode_obj() relies on bhitpos if target objects aren't carried by
       the hero or a monster, to check visibility controlling feedback */
    bhitpos.x = x, bhitpos.y = y;

	for (obj = chain; obj; obj = nobj) {
		nobj = here ? obj->nexthere : obj->nobj;
		if (fire_damage(obj, force, x, y))
			num++;
	}
    if (num && (Blind && !couldsee(x, y)))
        You("smell smoke.");
    return num;
}

int
fire_damage(obj, force, x, y)
struct obj *obj;
boolean force;
xchar x, y;
{
    int chance;
    struct obj *otmp, *ncobj;
    int in_sight = !Blind && couldsee(x, y);	/* Don't care if it's lit */
    int dindx;

	/* object might light in a controlled manner */
	if (catch_lit(obj))
		return FALSE;

	 /* special BotD feedback - should it be the "dark red" message?*/
	if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
		if (in_sight)
			pline("Smoke rises from %s.", the(xname(obj)));
		return FALSE;
	}

	if (!is_flammable(obj) || obj->oerodeproof)
		return FALSE;

	if (Is_container(obj)) {
	    switch (obj->otyp) {
	    case ICE_BOX:
		return FALSE;		/* Immune */
		/*NOTREACHED*/
		break;
	    case CHEST:
		chance = 40;
		break;
	    case BOX:
	    case SARCOPHAGUS:
		chance = 30;
		break;
	    default:
		chance = 20;
		break;
	    }
	    if (!force && (Luck + 5) > rn2(chance))
			return FALSE;
	    /* Container is burnt up - dump contents out */
	    if (in_sight)
			pline("%s catches fire and burns.", Yname2(obj));
	    if (Has_contents(obj)) {
			if (in_sight) pline("Its contents fall out.");
			for (otmp = obj->cobj; otmp; otmp = ncobj) {
				ncobj = otmp->nobj;
				obj_extract_self(otmp);
				if (!flooreffects(otmp, x, y, ""))
					place_object(otmp, x, y);
			}
	    }
		if(obj->where != OBJ_FREE)
			obj_extract_and_unequip_self(obj);
	    delobj(obj);
	    return TRUE;
	} else if (!force && (Luck + 5) > rn2(20)) {
	    /*  chance per item of sustaining damage:
	     *	max luck (full moon):	 5%
	     *	max luck (elsewhen):	10%
	     *	avg luck (Luck==0):	75%
	     *	awful luck (Luck<-4):  100%
	     */
	    return FALSE;
	} else if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS) {
	    if (obj->otyp == SCR_FIRE || obj->otyp == SCR_RESISTANCE || obj->otyp == SPE_FIREBALL || obj->oartifact)
			return FALSE;
	    dindx = (obj->oclass == SCROLL_CLASS) ? 2 : 3;
	    if (in_sight)
			pline("%s %s.", Yname2(obj), (obj->quan > 1) ?
				  destroy_strings[dindx*3 + 1] : destroy_strings[dindx*3]);

		if(obj->where != OBJ_FREE)
			obj_extract_and_unequip_self(obj);
	    delobj(obj);
	    return TRUE;
	} else if (obj->oclass == POTION_CLASS) {
	    dindx = 1;
	    if (in_sight)
			pline("%s %s.", Yname2(obj), (obj->quan > 1) ?
				  destroy_strings[dindx*3 + 1] : destroy_strings[dindx*3]);

		if(obj->where != OBJ_FREE)
			obj_extract_and_unequip_self(obj);
	    delobj(obj);
	    return TRUE;
	} else if (obj->oeroded < MAX_ERODE &&
		!(obj->blessed && rnl(100) < 25)
	) {
	    if (in_sight) {
		pline("%s %s%s.", Yname2(obj), otense(obj, "burn"),
		      obj->oeroded+1 == MAX_ERODE ? " completely" :
		      obj->oeroded ? " further" : "");
	    }
	    obj->oeroded++;
		return TRUE;
	}

    return FALSE;
}

/* returns TRUE if obj is destroyed */
boolean
water_damage(obj, force, here, modifiers, owner)
struct obj *obj;
boolean force, here;
uchar modifiers;
struct monst *owner;
{
	/* Dips in the Lethe are a very poor idea - Lethe patch*/
	boolean lethe = modifiers&WD_LETHE;
	boolean blood = modifiers&WD_BLOOD;
	int luckpenalty = lethe ? 7 : 0;
	struct obj *otmp;
	struct obj *obj_original = obj;
	boolean obj_destroyed = FALSE;
	int is_lethe = lethe;
	if(owner && ProtectItems(owner) &&
		(obj->oclass == POTION_CLASS
		 || obj->oclass == SCROLL_CLASS
		 || obj->oclass == WAND_CLASS
	))
		return 0;
	if(owner == &youmonst){
		if(Waterproof) {
			return 0;
		}
		if(uarmc && uarmc->greased) {
			if (force || !rn2(uarmc->blessed ? 4 : 2)){
				uarmc->greased = 0;
				pline("The layer of grease on your %s dissolves.", xname(uarmc));
			}
			return 0;
		}
	} else if(owner){ //Monster
		struct obj *cloak = which_armor(owner, W_ARMC);
		if(mon_resistance(owner, WATERPROOF)){
			return 0;
		} else if (cloak && cloak->greased) {
			if (force || !rn2(cloak->blessed ? 4 : 2)){
				cloak->greased = 0;
				if(canseemon(owner)) pline("The layer of grease on %s's %s dissolves.", mon_nam(owner), xname(cloak));
			}
			return 0;
		}
	}
	/* else: Scrolls, spellbooks, potions, weapons and
	   pieces of armor may get affected by the water */
	for (; obj; obj = otmp) {
		otmp = here ? obj->nexthere : obj->nobj;

		(void) snuff_lit(obj);

		if(obj->otyp == CAN_OF_GREASE && obj->spe > 0) {
			continue;
		} else if (!force && (20-(Luck - luckpenalty + 5)) < rn2(100)) {
			/*  chance per item of sustaining damage:
			 *	max luck (full moon):	 5%
			 *	max luck (elsewhen):	10%
			 *	avg luck (Luck==0):	75%
			 *	awful luck (Luck<-4):  100%
			 *  If this is the Lethe, things are much worse.
			 */
			continue;
		/* An oil skin cloak protects your body armor  */
		} else if(obj->greased) {
			if (force || !rn2(obj->blessed ? 4 : 2)){
				obj->greased = 0;
				pline("The layer of grease on %s dissolves.", the(xname(obj)));
			}
		} else if(Is_container(obj) && !Is_box(obj) &&
			(obj->otyp != OILSKIN_SACK || (obj->cursed && !rn2(3)))) {
			water_damage(obj->cobj, force, FALSE, lethe, (struct monst *) 0);
		} else {
		    /* The Lethe strips blessed and cursed status... */
		    if (is_lethe) {
				uncurse(obj);
				unbless(obj);
		    }
			if(blood){
				if(obj->blessed)
					unbless(obj);
				else if(!obj->cursed)
					curse(obj);
			}

		    switch (obj->oclass) {
		    case SCROLL_CLASS:
			if(obj->oartifact || obj->otyp == SCR_RESISTANCE) break;
#ifdef MAIL
		    if (obj->otyp != SCR_MAIL)
#endif
		    {
			    /* The Lethe sometimes does a little rewrite */
			    obj->otyp = (is_lethe && !rn2(10)) ?
					SCR_AMNESIA : SCR_BLANK_PAPER;
			obj->spe = 0;
			obj->oward = 0;
		    }
			break;
		    case SPBOOK_CLASS:
			/* Spell books get blanked... */
			if (obj->otyp == SPE_BOOK_OF_THE_DEAD)
				pline("Steam rises from %s.", the(xname(obj)));
			else if (obj->oartifact) /*do nothing*/;
			else {
				obj->otyp = SPE_BLANK_PAPER;
				obj->obj_color = objects[SPE_BLANK_PAPER].oc_color;
				remove_oprop(obj, OPROP_TACTB);
			}
			break;
		    case POTION_CLASS:
			if (obj->otyp == POT_ACID) {
				/* damage player/monster? */
				if (cansee(obj->ox,obj->oy) &&
				  obj->where != OBJ_CONTAINED)
				    pline("%s!", aobjnam(obj, "explode"));
				delobj(obj);
				obj_destroyed = (obj == obj_original);
				continue;
			} else
			/* Potions turn to water or amnesia... */
			if (is_lethe) {
				if (obj->otyp == POT_STARLIGHT)
					end_burn(obj, FALSE);
			    if (obj->otyp == POT_WATER)
					obj->otyp = POT_AMNESIA;
			    else if (obj->otyp != POT_AMNESIA) {
					obj->otyp = POT_WATER;
				obj->odiluted = 0;
				set_object_color(obj);
			    }
			} else if (blood) {
			    if (obj->otyp != POT_BLOOD){
					if(obj->odiluted){
						if (obj->otyp == POT_STARLIGHT)
							end_burn(obj, FALSE);
						obj->otyp = POT_BLOOD;
						obj->corpsenm = PM_HUMAN;
						obj->odiluted = 0;
						set_object_color(obj);
					}
					else obj->odiluted = TRUE;
				}
				else obj->odiluted = 0;
			} else if (obj->odiluted || obj->otyp == POT_AMNESIA) {
				if (obj->otyp == POT_STARLIGHT)
					end_burn(obj, FALSE);
				obj->otyp = POT_WATER;
				obj->blessed = obj->cursed = 0;
				obj->odiluted = 0;
				set_object_color(obj);
			} else if (obj->otyp != POT_WATER)
				obj->odiluted++;
			break;
		    case GEM_CLASS:
			// if (is_lethe && (obj->otyp == LUCKSTONE
					// || obj->otyp == LOADSTONE
					// || obj->otyp == TOUCHSTONE))
			    // obj->otyp = FLINT;
			break;
		    case TOOL_CLASS:
			// if (is_lethe) {
			    // switch (obj->otyp) {
			    // case MAGIC_LAMP:
				// obj->otyp = OIL_LAMP;
				// break;
			    // case MAGIC_WHISTLE:
				// obj->otyp = WHISTLE;
				// break;	
			    // case MAGIC_FLUTE:
				// obj->otyp = FLUTE;
				// obj->spe  = 0;
				// break;	
			    // case MAGIC_HARP:
				// obj->otyp = HARP;
				// obj->spe  = 0;
				// break;
			    // case FIRE_HORN:
			    // case FROST_HORN:
			    // case HORN_OF_PLENTY:
				// obj->otyp = TOOLED_HORN;
				// obj->spe  = 0;
				// break;
			    // case DRUM_OF_EARTHQUAKE:
				// obj->otyp = DRUM;
				// obj->spe  = 0;
				// break;
			    // }
			// }

			/* Drop through */
			/* Weapons, armor and tools may be disenchanted... */
			/* Wands and rings lose a charge... */
		    case WEAPON_CLASS:
		    case ARMOR_CLASS:
		    case WAND_CLASS:
		    case RING_CLASS:
			if ( is_lethe
					&& ( obj->oclass == WEAPON_CLASS
						|| obj->oclass == ARMOR_CLASS
						|| obj->oclass == WAND_CLASS
						|| obj->oclass == RING_CLASS
						|| is_weptool(obj) )) {

			    /* Shift enchantment one step closer to 0 */
			    if (obj->spe > 0) drain_item(obj);
			}

			/* Magic markers run... */
			if ( is_lethe
					&& obj->otyp == MAGIC_MARKER ) {
			    obj->spe -= (3 + rn2(10));
			    if (obj->spe < 0) obj->spe = 0;
			}

			/* Drop through for rusting effects... */
			/* Weapons, armor, tools and other things may rust... */
		    default:
			if (is_rustprone(obj) && obj->oeroded < MAX_ERODE &&
					!(obj->oerodeproof || 
					 (obj->blessed && rnl(100) < 25)))
				obj->oeroded++;
			/* The Lethe may unfooproof the item... */
			// if (is_lethe
					// && obj->oerodeproof && !rn2(5))
			    // obj->oerodeproof = FALSE;
		    }
		}
		obj_destroyed = FALSE;
	}
	return obj_destroyed;
}

/*
 * This function is potentially expensive - rolling
 * inventory list multiple times.  Luckily it's seldom needed.
 * Returns TRUE if disrobing made player unencumbered enough to
 * crawl out of the current predicament.
 */
STATIC_OVL boolean
emergency_disrobe(lostsome)
boolean *lostsome;
{
	int invc = inv_cnt();

	while (near_capacity() > (Punished ? UNENCUMBERED : SLT_ENCUMBER)) {
	    register struct obj *obj, *otmp = (struct obj *)0;
	    register int i;

	    /* Pick a random object */
	    if (invc > 0) {
		i = rn2(invc);
		for (obj = invent; obj; obj = obj->nobj) {
		    /*
		     * Undroppables are: body armor, boots, gloves,
		     * amulets, and rings because of the time and effort
		     * in removing them + loadstone and other cursed stuff
		     * for obvious reasons.
		     */
		    if (!((obj->otyp == LOADSTONE && obj->cursed) ||
			  obj == uamul || obj == uleft || obj == uright ||
			  obj == ublindf || obj == uarm || obj == uarmc ||
			  obj == uarmg || obj == ubelt || obj == uarmf ||
			  obj == uarmu ||
			  (obj->cursed && !Weldproof && (obj == uarmh || obj == uarms)) ||
			  welded(obj)))
			otmp = obj;
		    /* reached the mark and found some stuff to drop? */
		    if (--i < 0 && otmp) break;

		    /* else continue */
		}
	    }
#ifndef GOLDOBJ
	    if (!otmp) {
		/* Nothing available left to drop; try gold */
		if (u.ugold) {
		    pline("In desperation, you drop your purse.");
		    /* Hack: gold is not in the inventory, so make a gold object
		     * and put it at the head of the inventory list.
		     */
		    obj = mkgoldobj(u.ugold);    /* removes from u.ugold */
		    obj->in_use = TRUE;
		    u.ugold = obj->quan;         /* put the gold back */
		    assigninvlet(obj);           /* might end up as NOINVSYM */
		    obj->nobj = invent;
		    invent = obj;
		    *lostsome = TRUE;
		    dropx(obj);
		    continue;                    /* Try again */
		}
		/* We can't even drop gold! */
		return (FALSE);
	    }
#else
	    if (!otmp) return (FALSE); /* nothing to drop! */	
#endif
	    if (otmp->owornmask) remove_worn_item(otmp, FALSE);
	    *lostsome = TRUE;
	    dropx(otmp);
	    invc--;
	}
	return(TRUE);
}

/*
 *  return(TRUE) == player relocated
 */
boolean
drown()
{
	boolean inpool_ok = FALSE, crawl_ok;
	int i, x, y;
	const char *sparkle = level.flags.lethe? "sparkling " : "";
	
	if(u.sealsActive&SEAL_OSE) unbind(SEAL_OSE,TRUE);
	
	/* happily wading in the same contiguous pool */
	if (u.uinwater && is_pool(u.ux-u.dx,u.uy-u.dy, FALSE) &&
		!(!is_3dwater(u.ux-u.dx,u.uy-u.dy) && is_3dwater(u.ux,u.uy)) &&
	    ((Swimming && (u.divetimer>0 || !u.usubwater)) || (Amphibious && u.usubwater))) {
		/* water effects on objects every now and then */
		if (!rn2(5)) inpool_ok = TRUE;
		else return(FALSE);
	}
	
	if (!u.uinwater || (!is_3dwater(u.ux-u.dx,u.uy-u.dy) && is_3dwater(u.ux,u.uy))) {
	    You("%s into the %swater%c",
		Is_waterlevel(&u.uz) ? "plunge" : Flying ? "fly" : Levitation ? "hover" : "fall",
		sparkle,
		Amphibious || Swimming ? '.' : '!');
	}
	if (!u.usubwater && !Swimming && !Is_waterlevel(&u.uz) && !(Flying||Levitation))
		You("sink like %s.",
		Hallucination ? "the Titanic" : "a rock");

	if (level.flags.lethe) {
	    /* Bad idea */
	    You_feel("the sparkling waters of the Lethe sweep away your "
			    "cares!");
	    forget(10);
	}

	water_damage(invent, FALSE, FALSE, level.flags.lethe, &youmonst);

	if (u.umonnum == PM_GREMLIN && rn2(3))
	    (void)split_mon(&youmonst, (struct monst *)0);
	else if (is_iron(youracedata) && !(u.sealsActive&SEAL_EDEN)) {
	    You("rust!");
	    i = d(2,6);
	    if (u.mhmax > i) u.mhmax -= i;
	    losehp(i, "rusting away", KILLED_BY);
	}
	if (inpool_ok) return(FALSE);

	if ((i = number_leashed()) > 0) {
		pline_The("leash%s slip%s loose.",
			(i > 1) ? "es" : "",
			(i > 1) ? "" : "s");
		unleash_all();
	}
	
	if (Amphibious || (Swimming && !(u.divetimer == 0 && u.usubwater))) {
		u.uinwater = 1;
		if(uclockwork) u.uboiler = MAX_BOILER;
		if (Swimming && !is_3dwater(u.ux,u.uy) && u.divetimer > 0 && !Is_waterlevel(&u.uz) && yn("Dive underwater?")=='y') {
			u.usubwater = 1;
		} else if(is_3dwater(u.ux,u.uy)){
			u.usubwater = 1;
		} else if(Amphibious && !Swimming){
			u.usubwater = 1;
		}
		if (u.usubwater) {
			if (flags.verbose)
				if(!Swimming) pline("But you aren't drowning.");
			if (!Is_waterlevel(&u.uz)) {
				if (Hallucination)
					Your("keel hits the bottom.");
				else
					You("touch bottom.");
				under_water(1);
			}
				
		} else { //canswim and are swiming
			You("swim on the surface.");
		}
		if (Punished) {
			unplacebc();
			placebc();
		}
		vision_recalc(2);	/* unsee old position */
		vision_full_recalc = 1;
		return(FALSE);
	}
	You("are drowning!");
	if ((Teleportation || mon_resistance(&youmonst,TELEPORT)) &&
		    !u.usleep && (Teleport_control || rn2(3) < Luck+2)) {
		You("attempt a teleport spell.");	/* utcsri!carroll */
		if (!notel_level()) {
			(void) dotele();
			if(!is_pool(u.ux,u.uy, FALSE))
				return(TRUE);
		} else pline_The("attempted teleport spell fails.");
	}
#ifdef STEED
	if (u.usteed) {
		dismount_steed(DISMOUNT_GENERIC);
		if(!is_pool(u.ux,u.uy, FALSE))
			return(TRUE);
	}
#endif
	crawl_ok = FALSE;
	x = y = 0;		/* lint suppression */
	/* if sleeping, wake up now so that we don't crawl out of water
	   while still asleep; we can't do that the same way that waking
	   due to combat is handled; note unmul() clears u.usleep */
	if (u.usleep) unmul("Suddenly you wake up!");
	/* can't crawl if unable to move (crawl_ok flag stays false) */
	if (multi < 0 || (!youracedata->mmove)) goto crawl;
	/* look around for a place to crawl to */
	for (i = 0; i < 100; i++) {
		x = rn1(3,u.ux - 1);
		y = rn1(3,u.uy - 1);
		if (goodpos(x, y, &youmonst, 0)) {
			crawl_ok = TRUE;
			goto crawl;
		}
	}
	/* one more scan */
	for (x = u.ux - 1; x <= u.ux + 1; x++)
		for (y = u.uy - 1; y <= u.uy + 1; y++)
			if (goodpos(x, y, &youmonst, 0)) {
				crawl_ok = TRUE;
				goto crawl;
			}
 crawl:
	if (crawl_ok) {
		boolean lost = FALSE;
		/* time to do some strip-tease... */
		boolean succ = Is_waterlevel(&u.uz) ? TRUE :
				emergency_disrobe(&lost);

		if(Is_waterlevel(&u.uz)) You("try to flounder into a bubble.");
		else You("try to crawl out of the water.");
		if (lost)
			You("dump some of your gear to lose weight...");
		if (succ) {
			pline("Pheew!  That was close.");
			if (u.usubwater) {
				u.usubwater = 0;
				vision_recalc(2);
				vision_full_recalc = 1;
				doredraw();
			}
			teleds(x,y,TRUE);
			return(TRUE);
		}
		/* still too much weight */
		pline("But in vain.");
	}
	u.uinwater = 1;
	if (!u.usubwater) {
		u.usubwater = 1;
		vision_recalc(2);
		vision_full_recalc = 1;
		doredraw();
	}
	if (u.divetimer > 0) {
		u.divetimer--;
		return FALSE;
	}

	You("drown.");
	/* [ALI] Vampires return to vampiric form on drowning.
	 */
	if (Upolyd && !Unchanging && Race_if(PM_VAMPIRE)) {
		rehumanize();
		change_gevurah(1); //cheated death.
		if(!is_3dwater(u.ux, u.uy)){
			u.uinwater = 0;
			u.usubwater = 0;
			vision_recalc(2);	/* unsee old position */
			vision_full_recalc = 1;
			doredraw();
			You("fly up out of the water!");
		}
		return (TRUE);
	}
	killer_format = KILLED_BY_AN;
	killer = (levl[u.ux][u.uy].typ == POOL || Is_medusa_level(&u.uz)) ?
	    "pool of water" : "moat";
	done(DROWNING);
	//Survived in the water, no longer drowning. Return False since we're still swimming happily
	if(u.divetimer > 0){
		return FALSE;
	}
	//Probably impossible to go past this point as things currently stand.
	// All forms of life saving refill the dive timer.
	/* oops, we're still alive.  better get out of the water. */
	while (!safe_teleds(TRUE) && u.divetimer <= 0) {
		pline("You're still drowning.");
		done(DROWNING);
	}
	if (u.uinwater) {
	    u.uinwater = 0;
	    u.usubwater = 0;
		vision_recalc(2);	/* unsee old position */
		vision_full_recalc = 1;
		doredraw();
	    You("find yourself back %s.", Is_waterlevel(&u.uz) ?
		"in an air bubble" : "on land");
	}
	return(TRUE);
}

int
dodeepswim()
{
	if((u.uinwater && Swimming) || ((Flying || Wwalking) && (Swimming || Amphibious) && !Levitation && is_pool(u.ux, u.uy, FALSE))){
		if(u.usubwater){
			if(is_3dwater(u.ux, u.uy)){
				pline("There is no surface!");
				return MOVE_CANCELLED;
			} else {
				if (Flying)
					You("fly out of the water.");
				else if (Wwalking)
					You("slowly rise above the surface.");
				else
					You("swim up to the surface.");
				u.usubwater = 0;
				if (Flying || Wwalking) u.uinwater = 0;
				vision_recalc(2);	/* unsee old position */
				vision_full_recalc = 1;
				doredraw();
				return MOVE_STANDARD;
			}
		} else {
			if(ACURR(A_CON) > 5 || Amphibious){
				if(Is_waterlevel(&u.uz)){
					You("are already under water!");
					return MOVE_CANCELLED;
				} else {
					You("dive below the surface.");
					u.usubwater = 1;
					if (Flying || Wwalking) u.uinwater = 1;
					under_water(1);
					vision_recalc(2);	/* unsee old position */
					vision_full_recalc = 1;
					return MOVE_STANDARD;
				}
			} else You("can't hold your breath for very long.");
			return MOVE_CANCELLED;
		}
	} else {
		if(!is_pool(u.ux, u.uy, FALSE)) You("can't dive into the %s!", surface(u.ux, u.uy));
		else if(!Swimming) You("can't swim!");
		else if(Levitation) You("can't reach the water!");
		return MOVE_CANCELLED;
	}
}

static const char antimagic_killer[] = "antimagic";

void
drain_en(n)
register int n;
{
	if (!u.uenmax) return;
	You_feel("your magical energy drain away!");
	u.uen -= n;
	if(u.uen < 0)  {
		if(Race_if(PM_INCANTIFIER)){
			killer_format = KILLED_BY;
			killer = antimagic_killer;
			done(DIED);
		}
		u.uenbonus += u.uen/3;
		calc_total_maxen();
		u.uen = 0;
	}
	flags.botl = 1;
}

int
dountrap()	/* disarm a trap */
{
	if (near_capacity() >= HVY_ENCUMBER) {
	    pline("You're too strained to do that.");
	    return MOVE_CANCELLED;
	}
	if (((nohands(youracedata) || !freehand()) && !(webmaker(youracedata) || u.sealsActive&SEAL_CHUPOCLOPS || (uarm && uarm->oartifact==ART_SPIDERSILK))) || !youracedata->mmove) {
	    pline("And just how do you expect to do that?");
	    return MOVE_CANCELLED;
	} else if (u.ustuck && sticks(&youmonst)) {
	    pline("You'll have to let go of %s first.", mon_nam(u.ustuck));
	    return MOVE_CANCELLED;
	}
	if (u.ustuck || (welded(uwep) && bimanual(uwep,youracedata))) {
	    Your("%s seem to be too busy for that.",
		 makeplural(body_part(HAND)));
	    return MOVE_CANCELLED;
	}
	return untrap((struct obj *)0);
}
#endif /* OVLB */
#ifdef OVL2

/* Probability of disabling a trap.  Helge Hafting */
STATIC_OVL int
untrap_prob(ttmp)
struct trap *ttmp;
{
	int chance = 3;

	/* Only spiders know how to deal with webs reliably */
	if (ttmp->ttyp == WEB && !(webmaker(youracedata) || u.sealsActive&SEAL_CHUPOCLOPS || (uarm && uarm->oartifact==ART_SPIDERSILK)))
	 	chance = 30;
	if (Confusion || Hallucination) chance++;
	if (Blind) chance++;
	if (Stunned) chance += 2;
	if (Fumbling) chance *= 2;
	/* Your own traps are better known than others. */
	if (ttmp && ttmp->madeby_u) chance--;
	if (Role_if(PM_ROGUE)) {
	    if (rn2(2 * MAXULEV) < u.ulevel) chance--;
	    if (u.uhave.questart && chance > 1) chance--;
	} else if (Role_if(PM_RANGER) && chance > 1) chance--;
	return rn2(chance);
}

/* Extract a trap's ammo, and place it on the trap's location. Helge Hafting */
void
remove_trap_ammo(ttmp)
struct trap *ttmp;
{
	struct obj *otmp;
	while (ttmp->ammo) {
		otmp = ttmp->ammo;
		extract_nobj(otmp, &ttmp->ammo);
		otmp->otrap = 0;
		place_object(otmp, ttmp->tx, ttmp->ty);
		/* Sell your own traps only... */
		if (ttmp->madeby_u)
			sellobj(otmp, ttmp->tx, ttmp->ty);
		stackobj(otmp);
	}
	newsym(ttmp->tx, ttmp->ty);
	deltrap(ttmp);
}

/* Extract an object from a trap, and leave free for future use 
	If this resutls in an empty trap, destroy the trap.
*/
void
obj_extract_self_from_trap(struct obj *obj)
{
	struct obj *otmp;
	struct obj **ppointer;

	struct trap *ttmp = obj->otrap;
	
	for (ppointer = &(ttmp->ammo), otmp = ttmp->ammo; otmp; ppointer = &(otmp->nobj), otmp = otmp->nobj) {
		if(otmp == obj)
			break;
	}
	if(!otmp)
		panic("obj_extract_self_from_trap: obj is not contained by the trap indicated by its own otrap field");
	extract_nobj(otmp, ppointer);
	otmp->otrap = 0;
	if(!ttmp->ammo){
		newsym(ttmp->tx, ttmp->ty);
		deltrap(ttmp);
	}
}

void
rloc_trap(ttmp)
struct trap *ttmp;
{
	int x = rn2(COLNO);
	int y = rn2(ROWNO);
	int tries = 0;
	while(tries++ < 500000 && (!isok(x,y) || (levl[x][y].typ != ROOM && levl[x][y].typ != GRASS && levl[x][y].typ != SOIL && levl[x][y].typ != SAND))){
		x = rn2(COLNO);
		y = rn2(ROWNO);
	}
	if(tries < 500000){
		ttmp->tx = x;
		ttmp->ty = y;
	}
}

/* while attempting to disarm an adjacent trap, we've fallen into it */
STATIC_OVL void
move_into_trap(ttmp)
struct trap *ttmp;
{
	int bc;
	xchar x = ttmp->tx, y = ttmp->ty, bx, by, cx, cy;
	boolean unused;

	/* we know there's no monster in the way, and we're not trapped */
	if (!Punished || drag_ball(x, y, &bc, &bx, &by, &cx, &cy, &unused,
		TRUE)) {
	    u.ux0 = u.ux,  u.uy0 = u.uy;
	    u.ux = x,  u.uy = y;
	    u.umoved = TRUE;
	    newsym(u.ux0, u.uy0);
	    vision_recalc(1);
	    check_leash(u.ux0, u.uy0);
	    if (Punished) move_bc(0, bc, bx, by, cx, cy);
	    spoteffects(FALSE);	/* dotrap() */
	    exercise(A_WIS, FALSE);
	}
}

/* 0: doesn't even try
 * 1: tries and fails
 * 2: succeeds
 */
STATIC_OVL int
try_disarm(ttmp, force_failure)
struct trap *ttmp;
boolean force_failure;
{
	struct monst *mtmp = m_at(ttmp->tx,ttmp->ty);
	int ttype = ttmp->ttyp;
	boolean under_u = (!u.dx && !u.dy);
	boolean holdingtrap = (ttype == BEAR_TRAP || ttype == FLESH_HOOK || ttype == WEB);
	
	/* Test for monster first, monsters are displayed instead of trap. */
	if (mtmp && (!mtmp->mtrapped || !holdingtrap)) {
		pline("%s is in the way.", Monnam(mtmp));
		return 0;
	}
	/* We might be forced to move onto the trap's location. */
	if (sobj_at(BOULDER, ttmp->tx, ttmp->ty)
				&& !Passes_walls && !under_u) {
		There("is a boulder in your way.");
		return 0;
	}
	/* duplicate tight-space checks from test_move */
	if (u.dx && u.dy &&
	    bad_rock(&youmonst,u.ux,ttmp->ty) &&
	    bad_rock(&youmonst,ttmp->tx,u.uy)) {
	    if ((invent && (inv_weight() + weight_cap() > 600)) ||
		bigmonst(youracedata)) {
		/* don't allow untrap if they can't get thru to it */
		You("are unable to reach the %s!",
		    defsyms[trap_to_defsym(ttype)].explanation);
		return 0;
	    }
	}
	/* untrappable traps are located on the ground. */
	if (!can_reach_floor()) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
			You("aren't skilled enough to reach from %s.",
				mon_nam(u.usteed));
		else
#endif
		You("are unable to reach the %s!",
			defsyms[trap_to_defsym(ttype)].explanation);
		return 0;
	}

	/* Will our hero succeed? */
	if (force_failure || untrap_prob(ttmp)) {
		if (rnl(100) >= 20) {
		    pline("Whoops...");
		    if (mtmp) {		/* must be a trap that holds monsters */
			if (ttype == BEAR_TRAP || ttype == FLESH_HOOK) {
			    if (mtmp->mtame) abuse_dog(mtmp);
			    if ((mtmp->mhp -= rnd(4)) <= 0) killed(mtmp);
			} else if (ttype == WEB) {
			    if (!(webmaker(youracedata) || u.sealsActive&SEAL_CHUPOCLOPS || (uarm && uarm->oartifact==ART_SPIDERSILK))) {
				struct trap *ttmp2 = maketrap(u.ux, u.uy, WEB);
				if (ttmp2) {
				    pline_The("webbing sticks to you. You're caught too!");
				    dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
				    if (u.usteed && u.utrap) {
					/* you, not steed, are trapped */
					dismount_steed(DISMOUNT_FELL);
				    }
#endif
				}
			    } else
				pline("%s remains entangled.", Monnam(mtmp));
			}
		    } else if (under_u) {
			dotrap(ttmp, 0);
		    } else {
			move_into_trap(ttmp);
		    }
		} else {
		    pline("%s %s is difficult to %s.",
			  ttmp->madeby_u ? "Your" : under_u ? "This" : "That",
			  defsyms[trap_to_defsym(ttype)].explanation,
			  (ttype == WEB) ? "remove" : "disarm");
		}
		return 1;
	}
	return 2;
}

void
reward_untrap(ttmp, mtmp)
struct trap *ttmp;
struct monst *mtmp;
{
	if (!ttmp->madeby_u) {
		if(ttmp->ttyp == VIVI_TRAP && !mindless_mon(mtmp)){
			struct monst *newmon;
			if(canspotmon(mtmp))
				pline("%s is incredibly grateful!", Monnam(mtmp));
			newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
			if(newmon){
				mtmp = newmon;
				newsym(mtmp->mx, mtmp->my);
			}
		} else {
		    if (rnl(100) < 80 && !mtmp->mpeaceful &&
			    !mtmp->msleeping && !mtmp->mfrozen &&
			    !mindless_mon(mtmp) &&
			    mtmp->data->mlet != S_HUMAN) {
			mtmp->mpeaceful = 1;
			set_malign(mtmp);	/* reset alignment */
			pline("%s is grateful.", Monnam(mtmp));
		    }
		    /* Helping someone out of a trap is a nice thing to do,
		     * A lawful may be rewarded, but not too often.  */
		    if (!rn2(3) && rnl(100) < 16 && u.ualign.type == A_LAWFUL) {
				adjalign(1);
				You_feel("that you did the right thing.");
		    }
		}
	}
}

STATIC_OVL int
disarm_holdingtrap(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
	struct monst *mtmp;
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return (fails==0 ? MOVE_CANCELLED : MOVE_STANDARD);

	/* ok, disarm it. */

	/* untrap the monster, if any.
	   There's no need for a cockatrice test, only the trap is touched */
	if ((mtmp = m_at(ttmp->tx,ttmp->ty)) != 0) {
		mtmp->mtrapped = 0;
		You("remove %s %s from %s.", the_your[ttmp->madeby_u],
			(ttmp->ttyp == BEAR_TRAP || ttmp->ttyp == FLESH_HOOK) ? xname(ttmp->ammo) : "webbing",
			mon_nam(mtmp));
		reward_untrap(ttmp, mtmp);
	} else {
		if (ttmp->ttyp == BEAR_TRAP) {
			You("disarm %s %s.", the_your[ttmp->madeby_u], xname(ttmp->ammo));
			remove_trap_ammo(ttmp);
		} else if (ttmp->ttyp == FLESH_HOOK) {
			You("yank out %s %s.", the_your[ttmp->madeby_u], xname(ttmp->ammo));
			remove_trap_ammo(ttmp);
		} else if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)) /* if (ttmp->ttyp == WEB) */ {
			You("succeed in removing %s web.", the_your[ttmp->madeby_u]);
			deltrap(ttmp);
		}
	}
	newsym(u.ux + u.dx, u.uy + u.dy);
	return MOVE_STANDARD;
}

void
unshackle_mon(mtmp)
struct monst * mtmp;
{
	if (!mtmp || mtmp->entangled_otyp != SHACKLES) {
		impossible("%s not shackled?", m_monnam(mtmp));
		return;
	}

	mtmp->entangled_otyp = 0;
	mtmp->entangled_oid = 0;
	You("unlock the shackles imprisoning %s.", mon_nam(mtmp));
	if (mtmp->mtame){
		verbalize("Thank you for rescuing me!");
	}
	else if (rnd(20) < ACURR(A_CHA) && !(is_animal(mtmp->data) || mindless_mon(mtmp))){
		struct monst *newmon;
		pline("%s is very grateful!", Monnam(mtmp));
		newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
		if (newmon) mtmp = newmon;
		if (!mtmp->mtame)
			pline("But, apparently not grateful enough to join you.");
		if (mtmp->mpeaceful)
			return;
		//Otherwise check for peacefulness
	}
	if (mtmp->mpeaceful){
		pline("%s is grateful for the assistance, but makes no move to help you in return.", Monnam(mtmp));
	}
	else if (!mtmp->mpeaceful && rnd(10) < ACURR(A_CHA) && !(is_animal(mtmp->data) || mindless_mon(mtmp))){
		mtmp->mpeaceful = 1;
		pline("%s is thankful enough for the rescue to not attack you, at least.", Monnam(mtmp));
	}
	else {
		mtmp->movement += 12;
		pline("That might have been a mistake.");
	}
	return;
}

STATIC_OVL int
disarm_landmine(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return (fails==0 ? MOVE_CANCELLED : MOVE_STANDARD);
	You("disarm %s land mine.", the_your[ttmp->madeby_u]);
	remove_trap_ammo(ttmp);
	return MOVE_STANDARD;
}

STATIC_OVL int
disarm_rust_trap(ttmp) /* Paul Sonier */
struct trap *ttmp;
{
	xchar trapx = ttmp->tx, trapy = ttmp->ty;
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return (fails==0 ? MOVE_CANCELLED : MOVE_STANDARD);
	You("disarm the water trap!");
	deltrap(ttmp);
	levl[trapx][trapy].typ = FOUNTAIN;
	newsym(trapx, trapy);
	level.flags.nfountains++;
	return 1;
}

STATIC_OVL int
disarm_magic_trap(ttmp) /* Paul Sonier */
struct trap *ttmp;
{
	xchar trapx = ttmp->tx, trapy = ttmp->ty;
	if(!Race_if(PM_INCANTIFIER)){
		You("cannot disable %s trap.", (u.dx || u.dy) ? "that" : "this");
		return 0;
	} //else
	You("drain the trap's magical energy!");
	u.uconduct.food++;
	deltrap(ttmp);
	newsym(trapx, trapy);
	lesshungry(10*INC_BASE_NUTRITION);
	return 1;
}

static int webxprime = 0, webyprime = 0;

void
dowebgush(cx,cy,radius)
	int cx,cy,radius;
{
	int madeweb = 0;
	
	webxprime = cx;
	webyprime = cy;
	do_clear_area(cx, cy, radius, webgush, (genericptr_t)&madeweb);
}

void
webgush(cx, cy, poolcnt)
int cx, cy;
genericptr_t poolcnt;
{
	register struct monst *mtmp;
	register struct trap *ttmp;
	
	if (( (((webxprime-cx)+(webyprime-cy))%2) && 
			distmin(webxprime, webyprime, cx, cy)%2)  ||
	    !(SPACE_POS(levl[cx][cy].typ))
	) return;
		
	if ((ttmp = t_at(cx, cy)) != 0)
		return;
	
	/* Put a web at cx, cy */
	ttmp = maketrap(cx, cy, WEB);
	if(ttmp){
		ttmp->madeby_u = 0;
		ttmp->tseen = cansee(cx, cy) ? 1 : 0;
		newsym(cx, cy);
		if (*in_rooms(cx,cy,SHOPBASE)) {
			add_damage(cx, cy, 0L);		/* schedule removal */
		}
		if(cx==u.ux && cy==u.uy) dotrap(ttmp, NOWEBMSG);
		else if((mtmp = m_at(cx,cy))) mintrap(mtmp);
	}
	reset_trapset();
}

/* getobj will filter down to cans of grease and known potions of oil */
static NEARDATA const char oil[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS, 0 };
static NEARDATA const char disarmpotion[] = { ALL_CLASSES, POTION_CLASS, 0 };

/* water disarms, oil will explode */
STATIC_OVL int
disarm_fire_trap(ttmp) /* Paul Sonier */
struct trap *ttmp;
{
	int fails;
	struct obj *obj;
	boolean bad_tool;

	obj = getobj(disarmpotion, "untrap with");
	if (!obj) return 0;

	if (obj->otyp == POT_OIL)
	{
		Your("potion of oil explodes!");
		splatter_burning_oil(ttmp->tx,ttmp->ty);
		delobj(obj);
		return 1;
	}

	bad_tool = (obj->cursed ||
				(obj->otyp != POT_WATER));
	fails = try_disarm(ttmp, bad_tool);
	if (fails < 2) return (fails==0 ? MOVE_CANCELLED : MOVE_STANDARD);

	useup(obj);
	makeknown(POT_WATER);
	You("manage to extinguish the pilot light!");
	remove_trap_ammo(ttmp);
	more_experienced(1, 5);
	newexplevel();
	return 1;
}

/* it may not make much sense to use grease on floor boards, but so what? */
STATIC_OVL int
disarm_squeaky_board(ttmp)
struct trap *ttmp;
{
	struct obj *obj;
	boolean bad_tool;
	int fails;

	obj = getobj(oil, "untrap with");
	if (!obj) return 0;

	bad_tool = (obj->cursed ||
			((obj->otyp != POT_OIL || obj->lamplit) &&
			 (obj->otyp != CAN_OF_GREASE || !obj->spe)));

	fails = try_disarm(ttmp, bad_tool);
	if (fails < 2) return (fails==0 ? MOVE_CANCELLED : MOVE_STANDARD);

	/* successfully used oil or grease to fix squeaky board */
	if (obj->otyp == CAN_OF_GREASE) {
	    consume_obj_charge(obj, TRUE);
	} else {
	    useup(obj);	/* oil */
	    makeknown(POT_OIL);
	}
	You("repair the squeaky board.");	/* no madeby_u */
	deltrap(ttmp);
	newsym(u.ux + u.dx, u.uy + u.dy);
	more_experienced(1, 5);
	newexplevel();
	return 1;
}

/* removes traps that shoot arrows, darts, etc. */
STATIC_OVL int
disarm_shooting_trap(ttmp)
struct trap *ttmp;
{
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return (fails==0 ? MOVE_CANCELLED : MOVE_STANDARD);
	You("disarm %s trap.", the_your[ttmp->madeby_u]);
	remove_trap_ammo(ttmp);
	return MOVE_STANDARD;
}

/* Is the weight too heavy?
 * Formula as in near_capacity() & check_capacity() */
STATIC_OVL boolean
try_lift(mtmp, ttmp, wt, stuff)
struct monst *mtmp;
struct trap *ttmp;
int wt;
boolean stuff;
{
	int wc = weight_cap();

	if (((wt * 2) / wc) >= HVY_ENCUMBER) {
	    pline("%s is %s for you to lift.", Monnam(mtmp),
		  stuff ? "carrying too much" : "too heavy");
	    if (!ttmp->madeby_u && !mtmp->mpeaceful && mtmp->mcanmove &&
		    !mindless_mon(mtmp) &&
		    mtmp->data->mlet != S_HUMAN && rnl(100) < 30) {
			mtmp->mpeaceful = 1;
			set_malign(mtmp);		/* reset alignment */
			pline("%s thinks it was nice of you to try.", Monnam(mtmp));
			return FALSE;
	    }
	    return FALSE;
	}
	return TRUE;
}

/* Help trapped monster (out of a (spiked) pit) */
STATIC_OVL int
help_monster_out(mtmp, ttmp)
struct monst *mtmp;
struct trap *ttmp;
{
	int wt;
	struct obj *otmp;
	boolean uprob;

	/*
	 * This works when levitating too -- consistent with the ability
	 * to hit monsters while levitating.
	 *
	 * Should perhaps check that our hero has arms/hands at the
	 * moment.  Helping can also be done by engulfing...
	 *
	 * Test the monster first - monsters are displayed before traps.
	 */
	if (!mtmp->mtrapped) {
		pline("%s isn't trapped.", Monnam(mtmp));
		return MOVE_CANCELLED;
	}
	/* Do you have the necessary capacity to lift anything? */
	if (check_capacity((char *)0)) return MOVE_STANDARD;

	/* Will our hero succeed? */
	if ((uprob = untrap_prob(ttmp)) && !mtmp->msleeping && mtmp->mcanmove) {
		You("try to reach out your %s, but %s backs away skeptically.",
			makeplural(body_part(ARM)),
			mon_nam(mtmp));
		return MOVE_STANDARD;
	}


	/* is it a cockatrice?... */
	if (touch_petrifies(mtmp->data) && !uarmg && !Stone_resistance) {
		You("grab the trapped %s using your bare %s.",
				mtmp->data->mname, makeplural(body_part(HAND)));

		if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))
			display_nhwindow(WIN_MESSAGE, FALSE);
		else {
			char kbuf[BUFSZ];

			Sprintf(kbuf, "trying to help %s out of a pit",
					an(mtmp->data->mname));
			instapetrify(kbuf);
			return MOVE_STANDARD;
		}
	}
	/* need to do cockatrice check first if sleeping or paralyzed */
	if (uprob) {
	    You("try to grab %s, but cannot get a firm grasp.",
		mon_nam(mtmp));
	    if (mtmp->msleeping) {
		mtmp->msleeping = 0;
		pline("%s awakens.", Monnam(mtmp));
	    }
	    return MOVE_STANDARD;
	}

	You("reach out your %s and grab %s.",
	    makeplural(body_part(ARM)), mon_nam(mtmp));

	if (mtmp->msleeping) {
	    mtmp->msleeping = 0;
	    pline("%s awakens.", Monnam(mtmp));
	} else if (mtmp->mfrozen && !rn2(mtmp->mfrozen)) {
	    /* After such manhandling, perhaps the effect wears off */
	    mtmp->mcanmove = 1;
	    mtmp->mfrozen = 0;
	    pline("%s stirs.", Monnam(mtmp));
	}

	/* is the monster too heavy? */
	wt = inv_weight() + mtmp->data->cwt;
	if (!try_lift(mtmp, ttmp, wt, FALSE))
		return MOVE_STANDARD;

	/* is the monster with inventory too heavy? */
	for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		wt += otmp->owt;
	if (!try_lift(mtmp, ttmp, wt, TRUE))
		return MOVE_STANDARD;

	You("pull %s out of the pit.", mon_nam(mtmp));
	mtmp->mtrapped = 0;
	fill_pit(mtmp->mx, mtmp->my);
	reward_untrap(ttmp, mtmp);
	return MOVE_STANDARD;
}

int
you_remove_jrt_fang(mtmp, tool)
struct monst *mtmp;
struct obj *tool;
{
	int dmg = 0;
	if(tool && tool->oartifact){
		if(Role_if(PM_HEALER) || u.sealsActive&SEAL_BUER){
			You("teleport the fang out of %s heart, treating the wound after you do.", s_suffix(mon_nam(mtmp)));
			set_template(mtmp, 0);
			struct monst *newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
			if(newmon){
				mtmp = newmon;
				newsym(mtmp->mx, mtmp->my);
				pline("%s comes to %s senses, and is incredibly grateful for the aid!", Monnam(mtmp), mhis(mtmp));
				if(get_mx(mtmp, MX_EDOG)){
					EDOG(mtmp)->loyal = 1;
				}
			}
		}
		else if(mtmp->mhp > (dmg = d(10,4))){
			You("teleport the fang out of %s heart, but are unable to provide proper medical care afterwards.", s_suffix(mon_nam(mtmp)));
			pline("Luckily, %s survives the process!", mhe(mtmp));
			set_template(mtmp, 0);
			mtmp->mhp -= dmg;
			if(rnd(20) > (ACURR(A_CHA)+2)){
				pline("%s comes to %s senses, but makes no move to aid you in return.", Monnam(mtmp), mhis(mtmp));
				mtmp->mpeaceful = TRUE;
				set_malign(mtmp);
			}
			else {
				struct monst *newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
				if(newmon){
					mtmp = newmon;
					newsym(mtmp->mx, mtmp->my);
					pline("%s comes to %s senses, and is incredibly grateful for the aid!", Monnam(mtmp), mhis(mtmp));
					if(get_mx(mtmp, MX_EDOG)){
						EDOG(mtmp)->loyal = 1;
					}
				}
			}
		}
		else {
			You("teleport the fang out of %s heart, but are unable to provide proper medical care afterwards.", s_suffix(mon_nam(mtmp)));
			pline("Unfortunately, the wound is fatal.");
			set_template(mtmp, 0);
			mondied(mtmp);
		}
		struct obj *fang = mksobj(FANG_OF_APEP, MKOBJ_NOINIT);
		hold_another_object(fang, "You drop %s!", doname(fang), (const char *)0);
		return MOVE_STANDARD;
	}
	else if(uwep && uwep->otyp == SCALPEL){
		if(Role_if(PM_HEALER) || u.sealsActive&SEAL_BUER){
			You("extract the fang from %s heart, treating the wound as you do.", s_suffix(mon_nam(mtmp)));
			set_template(mtmp, 0);
			struct monst *newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
			if(newmon){
				mtmp = newmon;
				newsym(mtmp->mx, mtmp->my);
				pline("%s comes to %s senses, and is incredibly grateful for the aid!", Monnam(mtmp), mhis(mtmp));
				if(get_mx(mtmp, MX_EDOG)){
					EDOG(mtmp)->loyal = 1;
				}
			}
		}
		else if(mtmp->mhp > (dmg = d(20,4))){
			You("extract the fang from %s heart without providing medical care.", s_suffix(mon_nam(mtmp)));
			pline("Luckily, %s survives the process!", mhe(mtmp));
			set_template(mtmp, 0);
			mtmp->mhp -= dmg;
			if(rnd(20) > ACURR(A_CHA)){
				pline("%s comes to %s senses, but makes no move to aid you in return.", Monnam(mtmp), mhis(mtmp));
				mtmp->mpeaceful = TRUE;
				set_malign(mtmp);
			}
			else {
				struct monst *newmon = tamedog_core(mtmp, (struct obj *)0, TRUE);
				if(newmon){
					mtmp = newmon;
					newsym(mtmp->mx, mtmp->my);
					pline("%s comes to %s senses, and is incredibly grateful for the aid!", Monnam(mtmp), mhis(mtmp));
					if(get_mx(mtmp, MX_EDOG)){
						EDOG(mtmp)->loyal = 1;
					}
				}
			}
		}
		else {
			You("cut the fang out of %s heart without providing medical care.", s_suffix(mon_nam(mtmp)));
			pline("Unfortunately, the process is fatal.");
			set_template(mtmp, 0);
			// xkilled(mtmp,0); //Breaks pacifist
			mondied(mtmp);
		}
		struct obj *fang = mksobj(FANG_OF_APEP, MKOBJ_NOINIT);
		hold_another_object(fang, "You drop %s!", doname(fang), (const char *)0);
		return MOVE_STANDARD;
	}
	else if(yn("You lack an appropriate medical tool. Attempt to remove the fang anyway?")=='y'){
		You("try to extract the fang from %s heart.", s_suffix(mon_nam(mtmp)));
		pline("Unfortunately, the process is fatal.");
		set_template(mtmp, 0);
		xkilled(mtmp,0); //Breaks pacifist (you got a warning)
		struct obj *fang = mksobj(FANG_OF_APEP, MKOBJ_NOINIT);
		hold_another_object(fang, "You drop %s!", doname(fang), (const char *)0);
		return MOVE_STANDARD;
	}
	return MOVE_CANCELLED;
}

int
untrap(tool)
struct obj * tool;
{
	register struct obj *otmp;
	register boolean confused = (Confusion > 0 || Hallucination > 0);
	register int x,y;
	int ch;
	struct trap *ttmp;
	struct monst *mtmp;
	boolean trap_skipped = FALSE;
	boolean box_here = FALSE;
	boolean deal_with_floor_trap = FALSE;
	boolean force = (tool && tool->oartifact);
	char the_trap[BUFSZ], qbuf[QBUFSZ];
	int containercnt = 0;

	if(!getdir((char *)0)) return MOVE_CANCELLED;
	x = u.ux + u.dx;
	y = u.uy + u.dy;
	if(!isok(x,y)) return MOVE_CANCELLED;
	
	if((mtmp = m_at(x,y)) && mtmp->mtyp == PM_JRT_NETJER && has_template(mtmp, POISON_TEMPLATE) && canseemon(mtmp)){
		struct obj *armor;
		if((armor = which_armor(mtmp, W_ARM)) && arm_blocks_upper_body(armor->otyp))
			/*no message*/;
		else if((armor = which_armor(mtmp, W_ARMU)) && arm_blocks_upper_body(armor->otyp))
			/*no message*/;
		else {
			pline("A broken-off fang is embedded in %s chest. It seems to have pierced %s heart!", s_suffix(mon_nam(mtmp)), mhis(mtmp));
			if(!helpless_still(mtmp) && !TimeStop){
				pline("%s moves too quickly for you to grasp the fang.", Monnam(mtmp));
			}
			else if(yn("Attempt to remove the fang?")=='y'){
				/* Don't really want the solution to be wages of sloth */
				if(TimeStop){
					pline("The flesh around %s wound is too unyielding in your accelerated time frame.", s_suffix(mon_nam(mtmp)));
				}
				else {
					int res = you_remove_jrt_fang(mtmp, tool);
					if(res != MOVE_CANCELLED)
						return res;
				}
			}
		}
	}
	
	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
		if(Is_box(otmp) && !u.dx && !u.dy) {
			box_here = TRUE;
			containercnt++;
			if (containercnt > 1) break;
		}
		if(otmp->oartifact == ART_CLARENT && (levl[x][y].typ <= SCORR)) {
		    Strcpy(the_trap, doname(otmp));

		    Strcat(the_trap, " here, embedded in ");
		    if (IS_TREE(levl[x][y].typ))
			Strcat(the_trap, "a tree");
		    else if (IS_WALL(levl[x][y].typ) || levl[x][y].typ == SDOOR)
			Strcat(the_trap, "a wall");
		    else if (closed_door(x,y))
			Strcat(the_trap, "a door");
		    else
			Strcat(the_trap, "stone");
		    
		    You("see %s.", the_trap);
		    switch (ynq("Try to pull it out?")) {
			case 'q': return MOVE_STANDARD;
			case 'n': trap_skipped = TRUE;  continue;
		    }

		    if(touch_artifact(otmp, &youmonst, FALSE) && u.ualign.type == A_LAWFUL && u.ualign.record >= 14) {
			pline("It slides out easily!");
			(void) pick_obj(otmp);
		    } else {
			pline("It is stuck fast!");
		    }

		    return MOVE_STANDARD;
		}
	}

	if ((ttmp = t_at(x,y)) && ttmp->tseen) {
		deal_with_floor_trap = TRUE;
		Strcpy(the_trap, the(defsyms[trap_to_defsym(ttmp->ttyp)].explanation));
		if (box_here) {
			if (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) {
			    You_cant("do much about %s%s.",
					the_trap, u.utrap ?
					" that you're stuck in" :
					" while standing on the edge of it");
			    trap_skipped = TRUE;
			    deal_with_floor_trap = FALSE;
			} else {
			    Sprintf(qbuf, "There %s and %s here. %s %s?",
				(containercnt == 1) ? "is a container" : "are containers",
				an(defsyms[trap_to_defsym(ttmp->ttyp)].explanation),
				ttmp->ttyp == WEB ? "Remove" : "Disarm", the_trap);
			    switch (ynq(qbuf)) {
				case 'q': return MOVE_CANCELLED;
				case 'n': trap_skipped = TRUE;
					  deal_with_floor_trap = FALSE;
					  break;
			    }
			}
		}
		if (deal_with_floor_trap) {
		    if (u.utrap) {
			You("cannot deal with %s while trapped%s!", the_trap,
				(x == u.ux && y == u.uy) ? " in it" : "");
			return MOVE_STANDARD;
		    }
		    switch(ttmp->ttyp) {
			case FLESH_HOOK:
			case BEAR_TRAP:
			case WEB:
				return disarm_holdingtrap(ttmp);
			case LANDMINE:
				return disarm_landmine(ttmp);
			case SQKY_BOARD:
				return disarm_squeaky_board(ttmp);
			case DART_TRAP:
				return disarm_shooting_trap(ttmp);
			case ARROW_TRAP:
				return disarm_shooting_trap(ttmp);
			case RUST_TRAP:
				return disarm_rust_trap(ttmp);
			case PIT:
			case SPIKED_PIT:
				if (!u.dx && !u.dy) {
				    You("are already on the edge of the pit.");
				    return MOVE_CANCELLED;
				}
				if (!(mtmp = m_at(x,y))) {
				    pline("Try filling the pit instead.");
				    return MOVE_CANCELLED;
				}
				return help_monster_out(mtmp, ttmp);
			case VIVI_TRAP:
				if((mtmp = m_at(x,y))){
					if(Role_if(PM_HEALER) || u.sealsActive&SEAL_BUER){
						You("free %s from the delicate equipment that imprisons %s, carefully tending to %s wounds as you do.", mon_nam(mtmp), himherit(mtmp), hisherits(mtmp));
						mtmp->mtrapped = 0;
						reward_untrap(ttmp, mtmp);
					} else {
						You("try to free %s from the delicate equipment that imprisons %s.", mon_nam(mtmp), himherit(mtmp));
						pline("Unfortunately, that equipment was the only thing keeping %s %s.", himherit(mtmp), nonliving(mtmp->data) ? "intact" : "alive");
						// xkilled(mtmp,1); //Breaks pacifist
						mondied(mtmp);
					}
					if(Is_illregrd(&u.uz)){
						u.uevent.uaxus_foe = 1;
						pline("An alarm sounds!");
						aggravate();
					}
				} else {
					You("smash the delicate equipment.");
				}
				deltrap(ttmp);
				newsym(u.ux + u.dx, u.uy + u.dy);
				return MOVE_STANDARD;
			case TELEP_TRAP:
			case LEVEL_TELEP:
			case MAGIC_TRAP:
			case POLY_TRAP:
				return disarm_magic_trap(ttmp);
			case FIRE_TRAP:
				if(!Is_firelevel(&u.uz)) return disarm_fire_trap(ttmp);
				// else fall through
			default:
				You("cannot disable %s trap.", (u.dx || u.dy) ? "that" : "this");
				return MOVE_CANCELLED;
		    }
		}
	} /* end if */
	else if((mtmp = m_at(x,y)) && mtmp->entangled_oid){
		if(mtmp->entangled_otyp == SHACKLES){
			unshackle_mon(mtmp);
		}
		else {
			struct obj *obj, *nobj;
			You("disentangle %s.", mon_nam(mtmp));
			for(obj = mtmp->minvent; obj; obj = nobj){
				nobj = obj->nobj;
				if(obj->o_id == mtmp->entangled_oid){
					obj->spe = 0;
					obj_extract_self(obj);
					place_object(obj, mtmp->mx, mtmp->my);
					stackobj(obj);
				}
			}
			mtmp->entangled_otyp = 0;
			mtmp->entangled_oid = 0;
		}
		return MOVE_STANDARD;
	}

	if(!u.dx && !u.dy) {
		for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
			if (otmp->otyp == MAGIC_CHEST && otmp->obolted) {
				Sprintf(qbuf, "There is %s bolted down here. Unbolt it?",
					safe_qbuf("", sizeof("There is  bolted down here. Unbolt it?"),
					doname(otmp), an(simple_typename(otmp->otyp)), "a box"));
				switch (ynq(qbuf)) {
				case 'q': return MOVE_CANCELLED;
				case 'n': continue;
				}
#ifdef STEED
				if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
					You("aren't skilled enough to reach from %s.",
						mon_nam(u.usteed));
					return MOVE_CANCELLED;
				}
#endif
				if (!(tool && tool->oartifact == ART_MASTER_KEY_OF_THIEVERY)) {
					pline("The bolts are seemingly magical and impossible to budge.");
					return MOVE_CANCELLED;
				}
				else {
					pline("The bolts release, and %s locks itself!", the(xname(otmp)));
					otmp->olocked = 1;
					otmp->obolted = 0;
					otmp->owt = weight(otmp);
					return MOVE_STANDARD;
				}
			}
			if (Is_box(otmp)) {
				Sprintf(qbuf, "There is %s here. Check it for traps?",
					safe_qbuf("", sizeof("There is  here. Check it for traps?"),
					doname(otmp), an(simple_typename(otmp->otyp)), "a box"));
				switch (ynq(qbuf)) {
				case 'q': return MOVE_CANCELLED;
				case 'n': continue;
				}
#ifdef STEED
				if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
					You("aren't skilled enough to reach from %s.",
						mon_nam(u.usteed));
					return MOVE_CANCELLED;
				}
#endif
				if ((otmp->otrapped && otmp->otyp != MAGIC_CHEST &&
					(force || (!confused && rn2(MAXULEV + 1 - u.ulevel) < 10)))
					|| (!force && confused && !rn2(3))) {
					You("find a trap on %s!", the(xname(otmp)));
					if (!confused) exercise(A_WIS, TRUE);

					switch (ynq("Disarm it?")) {
					case 'q': return MOVE_STANDARD;
					case 'n': trap_skipped = TRUE;  continue;
					}

					if (otmp->otrapped && otmp->otyp != MAGIC_CHEST) {
						exercise(A_DEX, TRUE);
						ch = ACURR(A_DEX) + u.ulevel;
						if (Role_if(PM_ROGUE)) ch *= 2;
						if (!force && (confused || Fumbling ||
							rnd(75 + level_difficulty() / 2) > ch)) {
							(void)chest_trap(otmp, FINGER, TRUE);
						}
						else {
							You("disarm it!");
							otmp->otrapped = 0;
						}
					}
					else pline("That %s was not trapped.", xname(otmp));
					return MOVE_STANDARD;
				}
				else {
					You("find no traps on %s.", the(xname(otmp)));
					return MOVE_STANDARD;
				}
			}
		}

	    You(trap_skipped ? "find no other traps here."
			     : "know of no traps here.");
	    return MOVE_CANCELLED;
	}

	if ((mtmp = m_at(x,y))				&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return MOVE_STANDARD;
	}

	if (!IS_DOOR(levl[x][y].typ)) {
	    if ((ttmp = t_at(x,y)) && ttmp->tseen)
		You("cannot disable that trap.");
	    else
		You("know of no traps there.");
	    return MOVE_CANCELLED;
	}

	switch (levl[x][y].doormask) {
	    case D_NODOOR:
		You("%s no door there.", Blind ? "feel" : "see");
		return MOVE_CANCELLED;
	    case D_ISOPEN:
		pline("This door is safely open.");
		return MOVE_CANCELLED;
	    case D_BROKEN:
		pline("This door is broken.");
		return MOVE_CANCELLED;
	}

	if ((levl[x][y].doormask & D_TRAPPED
	     && (force ||
		 (!confused && rn2(MAXULEV - u.ulevel + 11) < 10)))
	    || (!force && confused && !rn2(3))) {
		You("find a trap on the door!");
		exercise(A_WIS, TRUE);
		if (ynq("Disarm it?") != 'y') return MOVE_STANDARD;
		if (levl[x][y].doormask & D_TRAPPED) {
		    ch = 15 + (Role_if(PM_ROGUE) ? u.ulevel*3 : u.ulevel);
		    exercise(A_DEX, TRUE);
		    if(!force && (confused || Fumbling ||
				     rnd(75+level_difficulty()/2) > ch)) {
			You("set it off!");
			b_trapped("door", FINGER);
			levl[x][y].doormask = D_NODOOR;
			unblock_point(x, y);
			newsym(x, y);
			/* (probably ought to charge for this damage...) */
			if (*in_rooms(x, y, SHOPBASE)) add_damage(x, y, 0L);
		    } else {
			You("disarm it!");
			levl[x][y].doormask &= ~D_TRAPPED;
		    }
		} else pline("This door was not trapped.");
		return MOVE_STANDARD;
	} else {
		You("find no traps on the door.");
		return MOVE_STANDARD;
	}
}
#endif /* OVL2 */
#ifdef OVLB

/* only called when the player is doing something to the chest directly */
boolean
chest_trap(obj, bodypart, disarm)
register struct obj *obj;
register int bodypart;
boolean disarm;
{
	register struct obj *otmp = obj, *otmp2;
	char	buf[80];
	const char *msg;
	coord cc;

	if (get_obj_location(obj, &cc.x, &cc.y, 0))	/* might be carried */
	    obj->ox = cc.x,  obj->oy = cc.y;

	otmp->otrapped = 0;	/* trap is one-shot; clear flag first in case
				   chest kills you and ends up in bones file */
	You(disarm ? "set it off!" : "trigger a trap!");
	display_nhwindow(WIN_MESSAGE, FALSE);
	if (Luck > -13 && rn2(13+Luck) > 7) {	/* saved by luck */
	    /* trap went off, but good luck prevents damage */
	    switch (rn2(13)) {
		case 12:
		case 11:  msg = "explosive charge is a dud";  break;
		case 10:
		case  9:  msg = "electric charge is grounded";  break;
		case  8:
		case  7:  msg = "flame fizzles out";  break;
		case  6:
		case  5:
		case  4:  msg = "poisoned needle misses";  break;
		case  3:
		case  2:
		case  1:
		case  0:  msg = "gas cloud blows away";  break;
		default:  impossible("chest disarm bug");  msg = (char *)0;
			  break;
	    }
	    if (msg) pline("But luckily the %s!", msg);
	} else {
	    switch(rn2(20) ? ((Luck >= 13) ? 0 : rn2(13-Luck)) : rn2(26)) {
		case 25:
		case 24:
		case 23:
		case 22:
		case 21: 
			if(!obj->oartifact){
				struct monst *shkp = 0;
				long loss = 0L;
				boolean costly, insider;
				register xchar ox = obj->ox, oy = obj->oy;

				/* the obj location need not be that of player */
				costly = (costly_spot(ox, oy) &&
				   (shkp = shop_keeper(*in_rooms(ox, oy,
				    SHOPBASE))) != (struct monst *)0);
				insider = (*u.ushops && inside_shop(u.ux, u.uy) &&
				    *in_rooms(ox, oy, SHOPBASE) == *u.ushops);

				pline("%s!", Tobjnam(obj, "explode"));
				Sprintf(buf, "exploding %s", xname(obj));

				struct obj *otmp;
				while((otmp = obj->cobj)){
					obj_extract_self(otmp);
					if(costly && breaktest(otmp)){
						loss += stolen_value(otmp, ox, oy,
								(boolean)shkp->mpeaceful, TRUE);
						breakobj(otmp, ox, oy, TRUE, FALSE);
					}
					place_object(otmp, ox, oy);
					stackobj(otmp);
				}
				if(costly)
					loss += stolen_value(obj, obj->ox,
						obj->oy, (boolean)shkp->mpeaceful,
						TRUE);
				delobj(obj);
				/* we're about to scatter all things at this location,
				 * which could include the ball & chain.
				 * If we attempt to call unpunish() in the
				 * for-loop below we can end up with otmp2
				 * being invalid once the chain is gone.
				 * Deal with ball & chain right now instead.
				 */
				if (Punished && !carried(uball) &&
					((uchain->ox == u.ux && uchain->oy == u.uy) ||
					 (uball->ox == u.ux && uball->oy == u.uy))
				)
					unpunish();

				scatter(ox, oy, 4, VIS_EFFECTS|MAY_HIT|MAY_DESTROY|MAY_FRACTURE, (struct obj *)0, &loss, shkp);
				wake_nearby_noisy();
				losehp(d(6,6), buf, KILLED_BY_AN);
				exercise(A_STR, FALSE);
				if(costly && loss) {
					if(insider)
					You("owe %ld %s for objects destroyed.",
							loss, currency(loss));
					else {
					You("caused %ld %s worth of damage!",
							loss, currency(loss));
					make_angry_shk(shkp, ox, oy);
					}
				}
				return TRUE;
			}
		case 20:
		case 19:
		case 18:
		case 17:
			pline("A cloud of noxious gas billows from %s.",
							the(xname(obj)));
			poisoned("gas cloud", A_STR, "cloud of poison gas",15, FALSE);
			exercise(A_CON, FALSE);
			break;
		case 16:
		case 15:
		case 14:
		case 13:
			You_feel("a needle prick your %s.",body_part(bodypart));
			poisoned("needle", A_CON, "poisoned needle",10, FALSE);
			exercise(A_CON, FALSE);
			break;
		case 12:
		case 11:
		case 10:
		case 9:
			dofiretrap(obj);
			break;
		case 8:
		case 7:
		case 6: {
			int dmg;

			You("are jolted by a surge of electricity!");
			if(Shock_resistance)  {
			    shieldeff(u.ux, u.uy);
			    You("don't seem to be affected.");
			    dmg = 0;
			} else {
			    dmg = d(4, 4);
			}
			if(!UseInvShock_res(&youmonst)){
				destroy_item(&youmonst, RING_CLASS, AD_ELEC);
				destroy_item(&youmonst, WAND_CLASS, AD_ELEC);
			}
			if (dmg) losehp(dmg, "electric shock", KILLED_BY_AN);
			break;
		      }
		case 5:
		case 4:
		case 3:
			if (!Free_action) {                        
			pline("Suddenly you are frozen in place!");
			nomul(-d(5, 6), "frozen by a trap");
			exercise(A_DEX, FALSE);
			nomovemsg = You_can_move_again;
			} else You("momentarily stiffen.");
			break;
		case 2:
		case 1:
		case 0:
			pline("A cloud of %s gas billows from %s.",
				Blind ? blindgas[rn2(SIZE(blindgas))] :
				rndcolor(), the(xname(obj)));
			if(!Stunned) {
			    if (Hallucination)
				pline("What a groovy feeling!");
			    else if (Blind)
				You("%s and get dizzy...",
				    stagger(&youmonst, "stagger"));
			    else
				You("%s and your vision blurs...",
				    stagger(&youmonst, "stagger"));
			}
			make_stunned(HStun + rn1(7, 16),FALSE);
			(void) make_hallucinated(HHallucination + rn1(5, 16),FALSE,0L);
			break;
		default: impossible("bad chest trap");
			break;
	    }
	    bot();			/* to get immediate botl re-display */
	}

	return FALSE;
}

#endif /* OVLB */
#ifdef OVL0

struct trap *
t_at(x,y)
register int x, y;
{
	register struct trap *trap = ftrap;
	while(trap) {
		if(trap->tx == x && trap->ty == y) return(trap);
		trap = trap->ntrap;
	}
	return((struct trap *)0);
}

#endif /* OVL0 */
#ifdef OVLB

void
deltrap(trap)
register struct trap *trap;
{
	register struct trap *ttmp;

	if (!trap) return;
	if(trap == ftrap)
		ftrap = ftrap->ntrap;
	else {
		for(ttmp = ftrap; ttmp && ttmp->ntrap != trap; ttmp = ttmp->ntrap) ;
		if (!ttmp) return;
		ttmp->ntrap = trap->ntrap;
	}
	while (trap->ammo) {
		struct obj* otmp = trap->ammo;
		extract_nobj(otmp, &trap->ammo);
		obfree(otmp, (struct obj *) 0);
	}
	if (cansee(trap->tx, trap->ty))
		newsym(trap->tx, trap->ty);
	dealloc_trap(trap);
}

boolean
delfloortrap(ttmp)
register struct trap *ttmp;
{
	/* Destroy a trap that emanates from the floor. */
	/* some of these are arbitrary -dlc */
	if (ttmp && ((ttmp->ttyp == SQKY_BOARD) ||
		     (ttmp->ttyp == BEAR_TRAP) ||
		     (ttmp->ttyp == FLESH_HOOK) ||
		     (ttmp->ttyp == LANDMINE) ||
		     (ttmp->ttyp == FIRE_TRAP) ||
		     (ttmp->ttyp == PIT) ||
		     (ttmp->ttyp == SPIKED_PIT) ||
		     (ttmp->ttyp == VIVI_TRAP) ||
		     (ttmp->ttyp == MUMMY_TRAP) ||
		     (ttmp->ttyp == HOLE) ||
		     (ttmp->ttyp == TRAPDOOR) ||
		     (ttmp->ttyp == TELEP_TRAP) ||
		     (ttmp->ttyp == LEVEL_TELEP) ||
		     (ttmp->ttyp == WEB) ||
		     (ttmp->ttyp == MAGIC_TRAP) ||
		     (ttmp->ttyp == ANTI_MAGIC))) {
	    register struct monst *mtmp;

	    if (ttmp->tx == u.ux && ttmp->ty == u.uy) {
		u.utrap = 0;
		u.utraptype = 0;
	    } else if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
		mtmp->mtrapped = 0;
		if(ttmp->ttyp == VIVI_TRAP){
			mondied(mtmp);
		}
	    }
	    deltrap(ttmp);
	    return TRUE;
	} else
	    return FALSE;
}

/* used for doors (also tins).  can be used for anything else that opens. */
void
b_trapped(item, bodypart)
register const char *item;
register int bodypart;
{
	register int lvl = level_difficulty();
	int dmg = rnd(5 + (lvl < 5 ? lvl : 2+lvl/2));

	pline("KABOOM!!  %s was booby-trapped!", The(item));
	wake_nearby_noisy();
	losehp(dmg, "explosion", KILLED_BY_AN);
	exercise(A_STR, FALSE);
	if (bodypart) exercise(A_CON, FALSE);
	make_stunned(HStun + dmg, TRUE);
}

/* Monster is hit by basic-damage-dealing trap. */
STATIC_OVL boolean
thitm(mon, dam, nocorpse)
struct monst *mon;
int dam;
boolean nocorpse;
{
	boolean trapkilled = FALSE;

	if ((mon->mhp -= dam) <= 0) {
		int xx = mon->mx;
		int yy = mon->my;

		monkilled(mon, "", nocorpse ? -AD_RBRE : AD_PHYS);
		if (mon->mhp <= 0) {
			newsym(xx, yy);
			trapkilled = TRUE;
		}
	}
	return trapkilled;
}

boolean
unconscious()
{
	return((boolean)(multi < 0 && (!nomovemsg ||
		u.usleep ||
		!strncmp(nomovemsg,"You regain con", 14) ||
		!strncmp(nomovemsg,"You are consci", 14))));
}

static const char lava_killer[] = "molten lava";

boolean
lava_effects(initialize)
boolean initialize;
{
    register struct obj *obj, *obj2;
    int dmg;
    boolean usurvive;

    burn_away_slime();
    melt_frozen_air();

	if (uarmf && uarmf->oartifact == ART_FROST_TREADS) {
		zap_over_floor(u.ux, u.uy, AD_COLD, WAND_CLASS, FALSE, NULL);
		return FALSE;
	}

    if (likes_lava(youracedata)) return FALSE;
	
    if (!Fire_resistance) {
		if(Wwalking) {
			dmg = d(6,6);
			pline_The("lava here burns you!");
			if(dmg < u.uhp) {
			losehp(dmg, lava_killer, KILLED_BY);
			goto burn_stuff;
			}
		} else if(initialize)
			You("fall into the lava!");

		usurvive = Lifesaved || discover;
#ifdef WIZARD
		if (wizard) usurvive = TRUE;
#endif
		for(obj = invent; obj; obj = obj2) {
			obj2 = obj->nobj;
			if(is_organic(obj) && !obj->oerodeproof) {
				if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
					if (!Blind && usurvive)
						pline("%s glows a strange %s, but remains intact.",
						  The(xname(obj)), hcolor("dark red"));
					continue;
				}
				if(obj->owornmask) {
					if (usurvive)
					Your("%s into flame!", aobjnam(obj, "burst"));

					if(obj == uarm) (void) Armor_gone();
					else if(obj == uarmc) (void) Cloak_off();
					else if(obj == uarmh) (void) Helmet_off();
					else if(obj == uarms) (void) Shield_off();
					else if(obj == uarmg) (void) Gloves_off();
					else if(obj == uarmf) (void) Boots_off();
					else if(obj == uarmu) setnotworn(obj);
					else if(obj == uleft) Ring_gone(obj);
					else if(obj == uright) Ring_gone(obj);
					else if(obj == ublindf) Blindf_off(obj);
					else if(obj == uamul) Amulet_off();
					else if(obj == ubelt) Belt_off();
					else if(obj == uwep) uwepgone();
					else if (obj == uquiver) uqwepgone();
					else if (obj == uswapwep) uswapwepgone();
				}
				useupall(obj);
			}
		}
		/* s/he died... */
		u.uhp = -1;
		killer_format = KILLED_BY;
		killer = lava_killer;
		You("burn to a crisp...");
		done(BURNING);
		while (!safe_teleds(TRUE)) {
			pline("You're still burning.");
			done(BURNING);
		}
		You("find yourself back on solid %s.", surface(u.ux, u.uy));
		return(TRUE);
    }

    if (!Wwalking) {
		if(initialize){
			u.utrap = rn1(4, 4) + (rn1(4, 12) << 8);
			u.utraptype = TT_LAVA;
			You("sink into the lava, but it only burns slightly!");
		}
		if (u.uhp > 1)
			losehp(1, lava_killer, KILLED_BY);
    }
    /* just want to burn boots, not all armor; destroy_item doesn't work on
       armor anyway */
burn_stuff:
    if(uarmf && !uarmf->oerodeproof && is_organic(uarmf)) {
		if ((uarmf->oeroded<3) || (uarmf->oartifact)) {
			rust_dmg(uarmf, "boots", 0, FALSE, &youmonst, FALSE);
		}
		else {
			/* save uarmf value because Boots_off() sets uarmf to null */
			obj = uarmf;
			Your("%s bursts into flame!", xname(obj));
			(void) Boots_off();
			useup(obj);
		}
    }
	if(!(Wwalking || UseInvFire_res(&youmonst))){
		burnarmor(&youmonst, TRUE);
		destroy_item(&youmonst, SCROLL_CLASS, AD_FIRE);
		destroy_item(&youmonst, SPBOOK_CLASS, AD_FIRE);
		destroy_item(&youmonst, POTION_CLASS, AD_FIRE);
		burnarmor(&youmonst, TRUE);
	}
    return(FALSE);
}

/* obj has been thrown or dropped into lava; damage is worse than mere fire */
boolean
lava_damage(obj, x, y)
struct obj *obj;
xchar x, y;
{
    int otyp = obj->otyp, oart = obj->oartifact;

    /* the Amulet, invocation items, and Rider corpses are never destroyed
       (let Book of the Dead fall through to fire_damage() to get feedback) */
    if (obj_resists(obj, 0, 100) && otyp != SPE_BOOK_OF_THE_DEAD)
        return FALSE;
    /* destroy liquid (venom), wax, veggy, flesh, paper (except for scrolls
       and books--let fire damage deal with them), cloth, leather, wood, bone
       unless it's inherently or explicitly fireproof or contains something;
       note: potions are glass so fall through to fire_damage() and boil */
    if (is_flammable(obj)
        /* assumes oerodeproof isn't overloaded for some other purpose on
           non-eroding items */
        && !obj->oerodeproof
        /* fire_damage() knows how to deal with containers and contents */
        && !Has_contents(obj)
	) {
        if (cansee(x, y)) {
            /* this feedback is pretty clunky and can become very verbose
               when former contents of a burned container get here via
               flooreffects() */
            // if (obj == thrownobj || obj == kickedobj)
                // pline("%s %s up!", is_plural(obj) ? "They" : "It",
                      // otense(obj, "burn"));
            if (IS_FORGE(levl[u.ux][u.uy].typ)) {
                if (((obj->owornmask & W_ARM) && (obj == uarm))
                    || ((obj->owornmask & W_ARMC) && (obj == uarmc))
                    || ((obj->owornmask & W_ARMU) && (obj == uarmu))
                    || ((obj->owornmask & W_ARMG) && (obj == uarmg))
                    || ((obj->owornmask & W_ARMH) && (obj == uarmh))
                    || ((obj->owornmask & W_ARMF) && (obj == uarmf))
                    || ((obj->owornmask & W_ARMS) && (obj == uarms))
				) {
                    You("were still wearing your %s!", xname(obj));
                    losehp(d(6, 6),
                           "dipping a worn object into a forge", KILLED_BY);
                }
                /* use the() to properly handle artifacts */
                pline_The("molten lava in the forge incinerates %s.",
                          the(xname(obj)));
            } else
                You("see %s hit lava and burn up!", the(xname(obj)));
        }
		if(obj->where != OBJ_FREE)
			obj_extract_and_unequip_self(obj);
		delobj(obj);
        return TRUE;
    }

    return fire_damage(obj, TRUE, x, y);
}

#define ATTRSCALE 4
int
ubreak_entanglement()
{
	struct obj *obj;
	int breakcheck = (youracedata->msize*ATTRSCALE + ACURRSTR);
	if(u.uentangled_otyp == ROPE_OF_ENTANGLING){
		if(breakcheck*2 <= rn2(100*ATTRSCALE))
			return FALSE;
	} else if(u.uentangled_otyp == BANDS){
		if(ACURRSTR < 15 && youracedata->msize != MZ_GIGANTIC) return FALSE;
		if(breakcheck <= rn2(200*ATTRSCALE))
			return FALSE;
	} else if(u.uentangled_otyp == RAZOR_WIRE){
		if(breakcheck <= rn2(100*ATTRSCALE))
			return FALSE;
	} else {
		u.uentangled_oid = 0;
		u.uentangled_otyp = 0;
		return TRUE;
	}
	for(obj = invent; obj; obj = obj->nobj){
		if(obj->o_id == u.uentangled_oid && !obj->oartifact){
			You("break the restraining %s!", xname(obj));
			useup(obj);
			break;
		}
	}
	for(obj = invent; obj; obj = obj->nobj){
		if(obj->o_id == u.uentangled_oid){
			return FALSE;
		}
	}
	// else
	u.uentangled_oid = 0;
	u.uentangled_otyp = 0;
	return TRUE;
}

void
entangle_effects(mdef)
struct monst *mdef;
{
	struct obj *obj;
	boolean youdef = mdef == &youmonst;
	int entangle_oid = youdef ? u.uentangled_oid : mdef->entangled_oid;
	for(obj = youdef ? invent : mdef->minvent; obj; obj = obj->nobj){
		if(obj->o_id == entangle_oid){
			if(obj->oartifact == ART_JIN_GANG_ZUO)
				cancel_monst(mdef, obj, FALSE, FALSE, FALSE, 0);
		}
	}
}

int
uescape_entanglement()
{
	struct obj *obj;
	int escapecheck = ((7-youracedata->msize)*ATTRSCALE + ACURR(A_DEX));
	if(Free_action){ /*Somehow gained free action while entangled, dump all entangling items.*/
		struct obj *nobj;
		for(obj = invent; obj; obj = nobj){
			nobj = obj->nobj;
			if(obj->o_id == u.uentangled_oid){
				You("slip loose from the entangling %s!", xname(obj));
				obj->spe = 0;
				obj_extract_self(obj);
				dropy(obj);
			}
		}
		u.uentangled_oid = 0;
		u.uentangled_otyp = 0;
		return TRUE;
	}
	obj = outermost_armor(&youmonst);
	if(obj && (obj->greased || obj->otyp == OILSKIN_CLOAK));//Slip free
	else if(u.uentangled_otyp == ROPE_OF_ENTANGLING){
		if(escapecheck <= rn2(20*ATTRSCALE)+rn2(20*ATTRSCALE))
			return FALSE;
	} else if(u.uentangled_otyp == BANDS){
		if(escapecheck <= rn2(20*ATTRSCALE))
			return FALSE;
	} else if(u.uentangled_otyp == RAZOR_WIRE){
		if(escapecheck <= rn2(20*ATTRSCALE)+rn2(20*ATTRSCALE))
			return FALSE;
	} else {
		u.uentangled_oid = 0;
		u.uentangled_otyp = 0;
		return TRUE;
	}
	for(obj = invent; obj; obj = obj->nobj){
		if(obj->o_id == u.uentangled_oid){
			//Very hard to escape from the diamond snare
			if(obj->oartifact == ART_JIN_GANG_ZUO && rn2(20))
				break;
			You("slip loose from the entangling %s!", xname(obj));
			obj->spe = 0;
			obj_extract_self(obj);
			dropy(obj);
			obj = outermost_armor(&youmonst);
			if(obj && obj->greased){
				if (!rn2(obj->blessed ? 4 : 2)){
					obj->greased = 0;
					pline("The layer of grease on your %s wears off.", xname(obj));
				}
			}
			break;
		}
	}
	for(obj = invent; obj; obj = obj->nobj){
		if(obj->o_id == u.uentangled_oid){
			return FALSE;
		}
	}
	// else
	u.uentangled_oid = 0;
	u.uentangled_otyp = 0;
	return TRUE;
}

#endif /* OVLB */

/*trap.c*/

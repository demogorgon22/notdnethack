/*	SCCS Id: @(#)hack.c	3.4	2003/04/30	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include <math.h>
#include "hack.h"
#include "xhity.h"

#ifdef OVL1
#endif /*OVL1*/
STATIC_DCL int NDECL(moverock);
STATIC_DCL void NDECL(sigilfloat);
STATIC_DCL int FDECL(still_chewing,(XCHAR_P,XCHAR_P));
STATIC_DCL int FDECL(invocation_distmin,(int, int));
#ifdef SINKS
STATIC_DCL void NDECL(dosinkfall);
#endif
STATIC_DCL boolean FDECL(findtravelpath, (BOOLEAN_P));
STATIC_DCL boolean FDECL(monstinroom, (struct permonst *,int));

STATIC_DCL void FDECL(move_update, (BOOLEAN_P));
STATIC_DCL struct obj * FDECL(all_items, (boolean, int *, boolean));

#define IS_SHOP(x)	(rooms[x].rtype >= SHOPBASE)

#ifdef OVL2

int
min_ints(i1, i2)
int i1, i2;
{
	if(i1 < i2) return i1;
	else return i2;
}

int
max_ints(i1, i2)
int i1, i2;
{
	if(i1 > i2) return i1;
	else return i2;
}

boolean
revive_nasty(x, y, msg)
int x,y;
const char *msg;
{
    register struct obj *otmp, *otmp2;
    struct monst *mtmp;
    coord cc;
    boolean revived = FALSE;

    for(otmp = level.objects[x][y]; otmp; otmp = otmp2) {
	otmp2 = otmp->nexthere;
	if (otmp->otyp == CORPSE &&
	    (is_rider(&mons[otmp->corpsenm]) ||
	     otmp->corpsenm == PM_WIZARD_OF_YENDOR)) {
	    /* move any living monster already at that location */
	    if((mtmp = m_at(x,y)) && enexto(&cc, x, y, mtmp->data))
		rloc_to(mtmp, cc.x, cc.y);
	    if(msg) Norep("%s", msg);
	    revived = revive_corpse(otmp, REVIVE_MONSTER);
	}
    }

    /* this location might not be safe, if not, move revived monster */
    if (revived) {
	mtmp = m_at(x,y);
	if (mtmp && !goodpos(x, y, mtmp, 0) &&
	    enexto(&cc, x, y, mtmp->data)) {
	    rloc_to(mtmp, cc.x, cc.y);
	}
	/* else impossible? */
    }

    return (revived);
}

STATIC_OVL int
moverock()
{
    register xchar rx, ry, sx, sy;
    register struct obj *otmp;
    register struct trap *ttmp;
    register struct monst *mtmp;

    sx = u.ux + u.dx,  sy = u.uy + u.dy;	/* boulder starting position */
    while ((otmp = boulder_at(sx, sy)) != 0) {
	/* make sure that this boulder is visible as the top object */
	if (otmp != level.objects[sx][sy]) movobj(otmp, sx, sy);

	rx = u.ux + 2 * u.dx;	/* boulder destination position */
	ry = u.uy + 2 * u.dy;
	nomul(0, NULL);
	if (Levitation || Weightless) {
	    if (Blind) feel_location(sx, sy);
	    You("don't have enough leverage to push %s.", the(xname(otmp)));
	    /* Give them a chance to climb over it? */
	    return -1;
	}
	if (verysmall(youracedata)
#ifdef STEED
		 && !u.usteed
#endif
				    ) {
	    if (Blind) feel_location(sx, sy);
	    pline("You're too small to push that %s.", xname(otmp));
	    goto cannot_push;
	}
	if (isok(rx,ry) && !IS_ROCK(levl[rx][ry].typ) &&
	    levl[rx][ry].typ != IRONBARS &&
	    (!IS_DOOR(levl[rx][ry].typ) || !(u.dx && u.dy) || (
#ifdef REINCARNATION
		!Is_rogue_level(&u.uz) &&
#endif
		(levl[rx][ry].doormask & ~D_BROKEN) == D_NODOOR)) &&
	    !boulder_at(rx, ry)) {
	    ttmp = t_at(rx, ry);
	    mtmp = m_at(rx, ry);

		/* KMH -- Sokoban doesn't let you push boulders diagonally */
	    if (In_sokoban(&u.uz) && u.dx && u.dy) {
	    	if (Blind) feel_location(sx,sy);
	    	pline("%s won't roll diagonally on this %s.",
	        		The(xname(otmp)), surface(sx, sy));
	    	goto cannot_push;
	    }

	    if (revive_nasty(rx, ry, "You sense movement on the other side."))
		return (-1);

	    if (mtmp && !noncorporeal(mtmp->data) &&
		    (!mtmp->mtrapped ||
			 !(ttmp && ((ttmp->ttyp == PIT) ||
				    (ttmp->ttyp == SPIKED_PIT))))) {
		if (Blind) feel_location(sx, sy);
		if (canspotmon(mtmp))
		    pline("There's %s on the other side.", a_monnam(mtmp));
		else {
		    You_hear("a monster behind %s.", the(xname(otmp)));
		    map_invisible(rx, ry);
		}
		if (flags.verbose)
		    pline("Perhaps that's why %s cannot move it.",
#ifdef STEED
				u.usteed ? y_monnam(u.usteed) :
#endif
				"you");
		goto cannot_push;
	    }

	    if (ttmp)
		switch(ttmp->ttyp) {
		case LANDMINE:
		    if (rn2(10)) {
			obj_extract_self(otmp);
			place_object(otmp, rx, ry);
			unblock_point(sx, sy);
			newsym(sx, sy);
			pline("KAABLAMM!!!  %s %s land mine.",
			      Tobjnam(otmp, "trigger"),
			      ttmp->madeby_u ? "your" : "a");
			blow_up_landmine(ttmp);
			/* if the boulder remains, it should fill the pit */
			fill_pit(u.ux, u.uy);
			if (cansee(rx,ry)) newsym(rx,ry);
			continue;
		    }
		    break;
		case SPIKED_PIT:
		case PIT:
		    obj_extract_self(otmp);
		    /* vision kludge to get messages right;
		       the pit will temporarily be seen even
		       if this is one among multiple boulders */
		    if (!Blind) viz_array[ry][rx] |= IN_SIGHT;
		    if (!flooreffects(otmp, rx, ry, "fall")) {
			place_object(otmp, rx, ry);
		    }
		    if (mtmp && !Blind) newsym(rx, ry);
		    continue;
		case HOLE:
		case TRAPDOOR:
		    if (Blind)
			pline("Kerplunk!  You no longer feel %s.",
				the(xname(otmp)));
		    else
			pline("%s%s and %s a %s in the %s!",
			  Tobjnam(otmp,
			   (ttmp->ttyp == TRAPDOOR) ? "trigger" : "fall"),
			  (ttmp->ttyp == TRAPDOOR) ? nul : " into",
			  otense(otmp, "plug"),
			  (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole",
			  surface(rx, ry));
		    deltrap(ttmp);
			if(otmp->otyp == MASS_OF_STUFF){
				place_object(otmp, rx, ry);
				separate_mass_of_stuff(otmp, FALSE);
				otmp = (struct obj *) 0;
			}
			bury_objs(rx, ry); //Crate handling: Bury everything here (inc boulder item) then free the boulder after
			if(otmp && otmp->otyp == MASSIVE_STONE_CRATE){
				struct obj *item;
				if(Blind) pline("Click!");
				else pline("The crate pops open as it lands.");
				/* drop any objects contained inside the crate */
				while ((item = otmp->cobj) != 0) {
					obj_extract_self(item);
					place_object(item, rx, ry);
				}
			}
		    if(otmp) delobj(otmp);
		    if (cansee(rx,ry)) newsym(rx,ry);
		    continue;
		case LEVEL_TELEP:
		case TELEP_TRAP:
#ifdef STEED
		    if (u.usteed)
			pline("%s pushes %s and suddenly it disappears!",
			      upstart(y_monnam(u.usteed)), the(xname(otmp)));
		    else
#endif
		    You("push %s and suddenly it disappears!",
			the(xname(otmp)));
		    if (ttmp->ttyp == TELEP_TRAP)
			rloco(otmp);
		    else {
			int newlev = random_teleport_level();
			d_level dest;

			if (newlev == depth(&u.uz) || In_endgame(&u.uz))
			    continue;
			obj_extract_self(otmp);
			add_to_migration(otmp);
			get_level(&dest, newlev);
			otmp->ox = dest.dnum;
			otmp->oy = dest.dlevel;
			otmp->owornmask = (long)MIGR_RANDOM;
		    }
		    seetrap(ttmp);
		    continue;
		}
	    if (closed_door(rx, ry))
		goto nopushmsg;
	    if (boulder_hits_pool(otmp, rx, ry, TRUE))
		continue;
	    /*
	     * Re-link at top of fobj chain so that pile order is preserved
	     * when level is restored.
	     */
	    if (otmp != fobj) {
		remove_object(otmp);
		place_object(otmp, otmp->ox, otmp->oy);
	    }

	    {
#ifdef LINT /* static long lastmovetime; */
		long lastmovetime;
		lastmovetime = 0;
#else
		/* note: reset to zero after save/restore cycle */
		static NEARDATA long lastmovetime;
#endif
#ifdef STEED
		if (!u.usteed) {
#endif
		  if (moves > lastmovetime+2 || moves < lastmovetime)
		    pline("With %s effort you move %s.",
			  (throws_rocks(youracedata) || (u.sealsActive&SEAL_YMIR)) ? "little" : "great",
			  the(xname(otmp)));
		  exercise(A_STR, TRUE);
#ifdef STEED
		} else 
		    pline("%s moves %s.",
			  upstart(y_monnam(u.usteed)), the(xname(otmp)));
#endif
		lastmovetime = moves;
	    }

	    /* Move the boulder *after* the message. */
	    if (glyph_is_invisible(levl[rx][ry].glyph))
		unmap_object(rx, ry);
	    movobj(otmp, rx, ry);	/* does newsym(rx,ry) */
	    if (Blind) {
		feel_location(rx,ry);
		feel_location(sx, sy);
	    } else {
		newsym(sx, sy);
	    }
	} else {
	nopushmsg:
#ifdef STEED
	  if (u.usteed)
	    pline("%s tries to move %s, but cannot.",
		  upstart(y_monnam(u.usteed)), the(xname(otmp)));
	  else
#endif
	    You("try to move %s, but in vain.", the(xname(otmp)));
	    if (Blind) feel_location(sx, sy);
	cannot_push:
		if(u.sealsActive&SEAL_MARIONETTE){
			if (yn("Your fingers stretch and grow like roots. Fracture the boulder?") != 'y') return (-1);
			Your("fingers dig into %s like roots!", the(xname(otmp)));
			break_boulder(otmp);
			break;
		} else if (throws_rocks(youracedata) || (u.sealsActive&SEAL_YMIR)) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC) {
		    You("aren't skilled enough to %s %s from %s.",
			(flags.pickup && !In_sokoban(&u.uz))
			    ? "pick up" : "push aside",
			the(xname(otmp)), y_monnam(u.usteed));
		} else
#endif
		{
		    pline("However, you can easily %s.",
			(flags.pickup && !In_sokoban(&u.uz))
			    ? "pick it up" : "push it aside");
			if (yn("Do it?") != 'y') return (-1);
		    if (In_sokoban(&u.uz))
			change_luck(-1);	/* Sokoban guilt */
		    break;
		}
		break;
	    }

	    if (
#ifdef STEED
		!u.usteed &&
#endif	    
		(((!invent || inv_weight() <= -850) &&
		(!(uarm && uarm->otyp == POWER_ARMOR)) && 
		 (!u.dx || !u.dy || (IS_ROCK(levl[u.ux][sy].typ)
				     && IS_ROCK(levl[sx][u.uy].typ))))
		|| verysmall(youracedata))) {
		if (yn("However, you can squeeze yourself into a small opening. Do it?") != 'y') return (-1);

		if (In_sokoban(&u.uz))
		    change_luck(-1);	/* Sokoban guilt */
		break;
	    } else
		return (-1);
	}
    }
    return (0);
}

/*
 *  still_chewing()
 *
 *  Chew on a wall, door, or boulder.  Returns TRUE if still eating, FALSE
 *  when done.
 */
STATIC_OVL int
still_chewing(x,y)
    xchar x, y;
{
    struct rm *lev = &levl[x][y];
    struct obj *boulder = boulder_at(x,y);
    const char *digtxt = (char *)0, *dmgtxt = (char *)0;

    if (digging.down)		/* not continuing previous dig (w/ pick-axe) */
	(void) memset((genericptr_t)&digging, 0, sizeof digging);

    if (!boulder && IS_ROCK(lev->typ) && !may_dig(x,y)) {
	You("hurt your teeth on the %s.",
	    IS_TREES(lev->typ) ? "tree" : "hard stone");
	nomul(0, NULL);
	return 1;
    } else if (digging.pos.x != x || digging.pos.y != y ||
		!on_level(&digging.level, &u.uz)) {
	digging.down = FALSE;
	digging.chew = TRUE;
	digging.warned = FALSE;
	digging.pos.x = x;
	digging.pos.y = y;
	assign_level(&digging.level, &u.uz);
	/* solid rock takes more work & time to dig through */
	digging.effort =
	    (IS_ROCK(lev->typ) && !IS_TREES(lev->typ) ? 30 : 60) + u.udaminc + aeshbon();
	You("start chewing %s.",
	    boulder ? (boulder_at(x,y))->otyp==STATUE ? "on a statue" : "on a boulder" :
	    lev->typ == IRONBARS ? "on the iron bars" :
	    IS_TREES(lev->typ) ? "on a tree" :
	    IS_ROCK(lev->typ) ? "a hole in the rock" :
	    "a hole in the door");
	watch_dig((struct monst *)0, x, y, FALSE);
	return 1;
    } else if ((digging.effort += (30 + u.udaminc + aeshbon())) <= 100)  {
	if (flags.verbose)
	    You("%s chewing on the %s.",
		digging.chew ? "continue" : "begin",
		boulder ? "boulder" :
		lev->typ == IRONBARS ? "bars" :
		IS_TREES(lev->typ) ? "tree" :
		IS_ROCK(lev->typ) ? "rock" : "door");
	digging.chew = TRUE;
	watch_dig((struct monst *)0, x, y, FALSE);
	return 1;
    }

    /* Okay, you've chewed through something */
    u.uconduct.food++;
	if(Race_if(PM_INCANTIFIER)) u.uen += rnd(20);
	else u.uhunger += rnd(20);

    if (boulder) {
	delobj(boulder);		/* boulder goes bye-bye */
	You("eat the %s.",xname(boulder));	/* yum */

	/*
	 *  The location could still block because of
	 *	1. More than one boulder
	 *	2. Boulder stuck in a wall/stone/door.
	 *
	 *  [perhaps use does_block() below (from vision.c)]
	 */
	if (IS_ROCK(lev->typ) || closed_door(x,y) || boulder_at(x,y)) {
	    block_point(x,y);	/* delobj will unblock the point */
	    /* reset dig state */
	    (void) memset((genericptr_t)&digging, 0, sizeof digging);
	    return 1;
	}

    } else if (IS_WALL(lev->typ)) {
	if (*in_rooms(x, y, SHOPBASE)) {
	    add_damage(x, y, 10L * ACURRSTR);
	    dmgtxt = "damage";
	}
	digtxt = "chew a hole in the wall.";
	if (level.flags.is_maze_lev) {
	    lev->typ = ROOM;
	} else if (level.flags.is_cavernous_lev && !in_town(x, y)) {
	    lev->typ = CORR;
	} else {
	    lev->typ = DOOR;
	    lev->doormask = D_NODOOR;
	}
    } else if (IS_TREE(lev->typ)) {
	digtxt = "chew through the tree.";
	lev->typ = ROOM;
    } else if (lev->typ == SDOOR) {
	if (lev->doormask & D_TRAPPED) {
	    lev->doormask = D_NODOOR;
	    b_trapped("secret door", 0);
	} else {
	    digtxt = "chew through the secret door.";
	    lev->doormask = D_BROKEN;
	}
	lev->typ = DOOR;

    } else if (IS_DOOR(lev->typ)) {
	if (*in_rooms(x, y, SHOPBASE)) {
	    add_damage(x, y, 400L);
	    dmgtxt = "break";
	}
	if (lev->doormask & D_TRAPPED) {
	    lev->doormask = D_NODOOR;
	    b_trapped("door", 0);
	} else {
	    digtxt = "chew through the door.";
	    lev->doormask = D_BROKEN;
	}

    } else { /* STONE or SCORR */
	digtxt = "chew a passage through the rock.";
	lev->typ = CORR;
    }

    unblock_point(x, y);	/* vision */
    newsym(x, y);
    if (digtxt) You1(digtxt);	/* after newsym */
    if (dmgtxt) pay_for_damage(dmgtxt, FALSE);
    (void) memset((genericptr_t)&digging, 0, sizeof digging);
    return 0;
}

#endif /* OVL2 */
#ifdef OVLB

void
movobj(obj, ox, oy)
register struct obj *obj;
register xchar ox, oy;
{
	/* optimize by leaving on the fobj chain? */
	remove_object(obj);
	newsym(obj->ox, obj->oy);
	place_object(obj, ox, oy);
	newsym(ox, oy);
}

#ifdef SINKS
static NEARDATA const char fell_on_sink[] = "fell onto a sink";

STATIC_OVL void
dosinkfall()
{
	register struct obj *obj;

	if (mon_resistance(&youmonst,LEVITATION) || (HLevitation & FROMOUTSIDE)) {
	    You("wobble unsteadily for a moment.");
	} else {
	    long save_ELev = ELevitation, save_HLev = HLevitation;

	    /* fake removal of levitation in advance so that final
	       disclosure will be right in case this turns out to
	       be fatal; fortunately the fact that rings and boots
	       are really still worn has no effect on bones data */
	    ELevitation = HLevitation = 0L;
	    You("crash to the floor!");
	    losehp(rn1(8, 25 - (int)ACURR(A_CON)),
		   fell_on_sink, NO_KILLER_PREFIX);
	    exercise(A_DEX, FALSE);
	    selftouch("Falling, you");
	    for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere)
		if (obj->oclass == WEAPON_CLASS || is_weptool(obj)) {
		    You("fell on %s.", doname(obj));
		    losehp(rnd(3), fell_on_sink, NO_KILLER_PREFIX);
		    exercise(A_CON, FALSE);
		}
	    ELevitation = save_ELev;
	    HLevitation = save_HLev;
	}

	ELevitation &= ~W_ARTI;
	HLevitation &= ~(I_SPECIAL|TIMEOUT);
	HLevitation++;
	if(uleft && uleft->otyp == RIN_LEVITATION) {
	    obj = uleft;
	    Ring_off(obj);
	    off_msg(obj);
	}
	if(uright && uright->otyp == RIN_LEVITATION) {
	    obj = uright;
	    Ring_off(obj);
	    off_msg(obj);
	}
	if(uarmf && uarmf->otyp == FLYING_BOOTS) {
	    obj = uarmf;
	    (void)Boots_off();
	    off_msg(obj);
	}
	HLevitation--;
}
#endif

boolean
may_dig(x,y)
register xchar x,y;
/* intended to be called only on ROCKs and trees */
{
    return (boolean)!((levl[x][y].wall_info & W_NONDIGGABLE)
		&&((IS_STWALL(levl[x][y].typ) ||
			IS_TREES(levl[x][y].typ)
		)));
}

boolean
may_passwall(x,y)
register xchar x,y;
{
   return (boolean)!((levl[x][y].wall_info & W_NONPASSWALL)
		&&((IS_STWALL(levl[x][y].typ) ||
			IS_TREES(levl[x][y].typ)
		)));
}

#endif /* OVLB */
#ifdef OVL1

boolean
bad_rock(mon,x,y)
struct monst *mon;
register xchar x,y;
{
	boolean isyou = mon==&youmonst;
	struct permonst *mdat = mon->data;
	return((boolean) ((In_sokoban(&u.uz) && boulder_at(x,y)) ||
	       (IS_ROCK(levl[x][y].typ)
		    && (!tunnels(mdat) || needspick(mdat) || !may_dig(x,y))
		    && !((isyou ? Passes_walls : mon_resistance(mon,PASSES_WALLS)) && may_passwall(x,y)))));
}

boolean
invocation_pos(x, y)
xchar x, y;
{
	return((boolean)(Invocation_lev(&u.uz) && x == inv_pos.x && y == inv_pos.y));
}

STATIC_OVL int
invocation_distmin(x, y)
xchar x, y;
{
	if(Invocation_lev(&u.uz))
		return distmin(x, y, inv_pos.x, inv_pos.y);
	else
		return 1000;
}

#endif /* OVL1 */
#ifdef OVL3

/* return TRUE if (dx,dy) is an OK place to move
 * mode is one of DO_MOVE, TEST_MOVE or TEST_TRAV
 */
boolean 
test_move(ux, uy, dx, dy, mode)
int ux, uy, dx, dy;
int mode;
{
    int x = ux+dx;
    int y = uy+dy;
    register struct rm *tmpr = &levl[x][y];
    register struct rm *ust;

	iflags.door_opened = FALSE;
    /*
     *  Check for physical obstacles.  First, the place we are going.
     */
    if (IS_ROCK(tmpr->typ) || tmpr->typ == IRONBARS) {
	if ((Blind || LightBlind) && mode == DO_MOVE) feel_location(x,y);
	if (Passes_walls && may_passwall(x,y)) {
	    ;	/* do nothing */
	} else if (tmpr->typ == IRONBARS && !Is_illregrd(&u.uz) && mode == DO_MOVE) {
	    if ((dmgtype(youracedata, AD_RUST) ||
			dmgtype(youracedata, AD_CORR))) {
			You("eat through the bars.");
			dissolve_bars(x,y);
			if(youracedata->mtyp == PM_RUST_MONSTER)
				lesshungry(objects[BAR].oc_nutrition);
	    }
	    if (!(Passes_walls || passes_bars(&youmonst)))
		return FALSE;
	} else if (tunnels(youracedata) && !needspick(youracedata)) {
	    /* Eat the rock. */
	    if (mode == DO_MOVE && still_chewing(x,y)) return FALSE;
	} else if (flags.autodig && !flags.run && !flags.nopick &&
		   ((uwep && (is_pick(uwep) || (is_lightsaber(uwep) && litsaber(uwep)) || (uwep->otyp == SEISMIC_HAMMER))) ||
			(uarmg && is_pick(uarmg)))) {
	/* MRKR: Automatic digging when wielding the appropriate tool */
	    if (mode == DO_MOVE){
			if(uwep && (is_pick(uwep) || (is_lightsaber(uwep) && litsaber(uwep)) || (uwep->otyp == SEISMIC_HAMMER))) (void) use_pick_axe2(uwep);
			else if(uarmg && is_pick(uarmg)) (void) use_pick_axe2(uarmg);
		}
	    return FALSE;
	} else {
	    if (mode == DO_MOVE) {
		if (Is_stronghold(&u.uz) && is_db_wall(x,y))
		    pline_The("drawbridge is up!");
		if (Passes_walls && !may_passwall(x,y) && In_sokoban(&u.uz))
		    pline_The("Sokoban walls resist your ability.");
		else if (iflags.notice_walls)
		    pline("It's a wall.");
	    }
	    return FALSE;
	}
    } else if (IS_DOOR(tmpr->typ)) {
	if (closed_door(x,y)) {
	    if (Blind && mode == DO_MOVE) feel_location(x,y);
	    /* ALI - artifact doors from slash'em */
	    if (artifact_door(x, y)) {
		if (mode == DO_MOVE) {
		    if (amorphous(youracedata))
			You("try to ooze under the door, but the gap is too small.");
		    else if (tunnels(youracedata) && !needspick(youracedata))
			You("hurt your teeth on the re-enforced door.");
		    else if (x == u.ux || y == u.uy) {
			if (Blind || Stunned || ACURR(A_DEX) < 10 || Fumbling) {
				pline("Ouch!  You bump into a heavy door.");
			    exercise(A_DEX, FALSE);
			} else pline("That door is closed.");
		    }
		}
		return FALSE;
	    }	    if (Passes_walls)
		;	/* do nothing */
	    else if (can_ooze(&youmonst)) {
		if (mode == DO_MOVE) You("ooze under the door.");
	    } else if (tunnels(youracedata) && !needspick(youracedata)) {
		/* Eat the door. */
		if (mode == DO_MOVE && still_chewing(x,y)) return FALSE;
	    } else {
		if (mode == DO_MOVE) {
		    if (amorphous(youracedata))
			You("try to ooze under the door, but can't squeeze your possessions through.");
			if (iflags.autoopen && !flags.run && !Confusion && !Stunned && !Fumbling) {
				iflags.door_opened = !(doopen_indir(x, y) & (MOVE_CANCELLED|MOVE_INSTANT));
		    } else if (x == ux || y == uy) {
				if (Blind || Stunned || ACURR(A_DEX) < 10 || Fumbling) {
#ifdef STEED
			    if (u.usteed) {
				You_cant("lead %s through that closed door.",
				         y_monnam(u.usteed));
			    } else
#endif
			    {
			        pline("Ouch!  You bump into a door.");
			        exercise(A_DEX, FALSE);
			    }
			} else pline("That door is closed.");
		    }
		} else if (mode == TEST_TRAV) goto testdiag;
		return FALSE;
	    }
	} else {
	testdiag:
	    if (dx && dy && !Passes_walls
		&& ((tmpr->doormask & ~D_BROKEN)
#ifdef REINCARNATION
		    || Is_rogue_level(&u.uz)
#endif
		    || block_door(x,y))) {
		/* Diagonal moves into a door are not allowed. */
		if (Blind && mode == DO_MOVE)
		    feel_location(x,y);
		return FALSE;
	    }
	}
    }
    if (dx && dy
	    && bad_rock(&youmonst,ux,y) && bad_rock(&youmonst,x,uy)) {
	/* Move at a diagonal. */
	if (In_sokoban(&u.uz)) {
	    if (mode == DO_MOVE)
		You("cannot pass that way.");
	    return FALSE;
	}
	if (bigmonst(youracedata) && !(u.sealsActive&SEAL_ANDREALPHUS) && !amorphous(youracedata)) {
	    if (mode == DO_MOVE)
		Your("body is too large to fit through.");
	    return FALSE;
	}
	if(uarm && uarm->otyp == POWER_ARMOR && !(u.sealsActive&SEAL_ANDREALPHUS)){
	    if (mode == DO_MOVE)
		Your("power armor is too bulky to fit through.");
	    return FALSE;
	}
	if (invent && (inv_weight() + weight_cap() > 600) && !(u.sealsActive&SEAL_ANDREALPHUS)
		&& !(uarmc && (uarmc->otyp == OILSKIN_CLOAK || uarmc->greased))
		&& !(!uarmc && uarm && uarm->greased)
		&& !(!uarmc && !uarm && uarmu && uarmu->greased)
	) {
	    if (mode == DO_MOVE)
#ifdef CONVICT
        if (!Passes_walls)
#endif /* CONVICT */
		You("are carrying too much to get through.");
	    return FALSE;
	}
    }
    /* Pick travel path that does not require crossing a trap.
     * Avoid water and lava using the usual running rules.
     * (but not u.ux/u.uy because findtravelpath walks toward u.ux/u.uy) */
    if (flags.run == 8 && mode != DO_MOVE && (x != u.ux || y != u.uy)) {
	struct trap* t = t_at(x, y);

	if ((t && t->tseen) ||
        (((!Levitation && !Flying &&
         !is_clinger(youracedata)) || is_3dwater(x, y)) &&
         (is_pool(x, y, TRUE) || is_lava(x, y)) && levl[x][y].seenv))
        return FALSE;
    }

    ust = &levl[ux][uy];

    /* Now see if other things block our way . . */
    if (dx && dy && !Passes_walls
		     && (IS_DOOR(ust->typ) && ((ust->doormask & ~D_BROKEN)
#ifdef REINCARNATION
			     || Is_rogue_level(&u.uz)
#endif
			     || block_entry(x, y))
			 )) {
	/* Can't move at a diagonal out of a doorway with door. */
	return FALSE;
    }

    if (boulder_at(x,y) && (In_sokoban(&u.uz) || !Passes_walls)) {
	if (!(Blind || Hallucination) && (flags.run >= 2) && mode != TEST_TRAV)
	    return FALSE;
	if (mode == DO_MOVE) {
	    /* tunneling monsters will chew before pushing */
	    if (tunnels(youracedata) && !needspick(youracedata) &&
		!In_sokoban(&u.uz)) {
		if (still_chewing(x,y)) return FALSE;
	    } else
		if (flags.run || moverock() < 0) return FALSE;
	} else if (mode == TEST_TRAV) {
	    struct obj* obj;

	    /* don't pick two boulders in a row, unless there's a way thru */
	    if (boulder_at(ux,uy) && !In_sokoban(&u.uz)) {
		if (!Passes_walls &&
		    !(tunnels(youracedata) && !needspick(youracedata)) &&
		    !carrying(PICK_AXE) && !carrying(DWARVISH_MATTOCK) &&
		    !((obj = carrying(WAN_DIGGING)) &&
		      !objects[obj->otyp].oc_name_known))
		    return FALSE;
	    }
	}
	/* assume you'll be able to push it when you get there... */
    }

    /* OK, it is a legal place to move. */
    return TRUE;
}

/*
 * Find a path from the destination (u.tx,u.ty) back to (u.ux,u.uy).
 * A shortest path is returned.  If guess is TRUE, consider various
 * inaccessible locations as valid intermediate path points.
 * Returns TRUE if a path was found.
 */
static boolean
findtravelpath(guess)
boolean guess;
{
    /* if travel to adjacent, reachable location, use normal movement rules */
    if (!guess && iflags.travel1 && distmin(u.ux, u.uy, u.tx, u.ty) == 1) {
	flags.run = 0;
	if (test_move(u.ux, u.uy, u.tx-u.ux, u.ty-u.uy, TEST_MOVE)) {
	    u.dx = u.tx-u.ux;
	    u.dy = u.ty-u.uy;
	    nomul(0, NULL);
	    iflags.travelcc.x = iflags.travelcc.y = -1;
	    return TRUE;
	}
	flags.run = 8;
    }
	if (u.itx == u.ux && u.ity == u.uy)
		u.itx = u.ity = 0;
    if (u.tx != u.ux || u.ty != u.uy) {
	xchar travel[COLNO][ROWNO];		/* radius to get to this xy coord */
	xchar suretravel[COLNO][ROWNO];	/* we have a seenv-only path to this coord */
	xchar travelstepx[2][COLNO*ROWNO];
	xchar travelstepy[2][COLNO*ROWNO];
	xchar tx, ty, ux, uy;
	int n = 1;			/* max offset in travelsteps */
	int set = 0;			/* two sets current and previous */
	int radius = 1;			/* search radius */
	int i;

	/* If guessing, first find an "obvious" goal location.  The obvious
	 * goal is the position the player knows of, or might figure out
	 * (couldsee) that is closest to the target on a straight path.
	 */
	if (guess) {
	    tx = u.ux; ty = u.uy; ux = u.tx; uy = u.ty;
	} else {
	    tx = u.tx; ty = u.ty; ux = u.ux; uy = u.uy;
	}

    noguess:
	(void) memset((genericptr_t)travel, 0, sizeof(travel));
	(void)memset((genericptr_t)suretravel, 0, sizeof(suretravel));
	travelstepx[0][0] = tx;
	travelstepy[0][0] = ty;
	suretravel[tx][ty] = 1;
	while (n != 0) {
	    int nn = 0;

	    for (i = 0; i < n; i++) {
		int dir;
		int x = travelstepx[set][i];
		int y = travelstepy[set][i];
		static int ordered[] = { 0, 2, 4, 6, 1, 3, 5, 7 };
		/* no diagonal movement for grid bugs */
		int dirmax = (u.umonnum == PM_GRID_BUG || u.umonnum == PM_BEBELITH) ? 4 : 8;

		for (dir = 0; dir < dirmax; ++dir) {
		    int nx = x+xdir[ordered[dir]];
		    int ny = y+ydir[ordered[dir]];

			/* don't go out of bounds */
		    if (!isok(nx, ny)) continue;

			/* don't go through places we know we can't */
			if (!test_move(x, y, nx-x, ny-y, TEST_TRAV) && levl[nx][ny].seenv)
				continue;

		    if ((!Passes_walls && !can_ooze(&youmonst) &&
			closed_door(x, y)) || boulder_at(x, y)) {
			/* closed doors and boulders usually
			 * cause a delay, so prefer another path */
			if (travel[x][y] > radius - 3) {
			    travelstepx[1-set][nn] = x;
			    travelstepy[1-set][nn] = y;
			    /* don't change travel matrix! */
			    nn++;
			    continue;
			}
		    }
			/* travelplus is a bit adventerous, and hopes that unseen locations are pathable */
			if ((iflags.travelplus && (suretravel[x][y] || radius<iflags.travelplus)) || levl[nx][ny].seenv) {
			if (nx == ux && ny == uy) {
			    if (!guess) {
				u.dx = x-ux;
				u.dy = y-uy;
				if (x == u.tx && y == u.ty) {
				    nomul(0, NULL);
				    /* reset run so domove run checks work */
				    flags.run = 8;
				    iflags.travelcc.x = iflags.travelcc.y = -1;
				}
				return TRUE;
			    }
			}
			else if (!suretravel[nx][ny] && levl[nx][ny].seenv && suretravel[x][y]) {
				/* we are now sure of this step (because the previous step was) */
				suretravel[nx][ny] = radius;
				/* we may have already been able to reach this place by guessing through unexplored area */
				if (!travel[nx][ny]) {
					travelstepx[1-set][nn] = nx;
					travelstepy[1-set][nn] = ny;
					travel[nx][ny] = radius;
					nn++;
				}
			}
			else if (!travel[nx][ny]) {
				travelstepx[1 - set][nn] = nx;
				travelstepy[1 - set][nn] = ny;
				travel[nx][ny] = radius;
				nn++;
			}
		    }
		}
	    }
	    n = nn;
	    set = 1-set;
	    radius++;
	}

	/* if guessing, find best location in travel matrix and go there */
	if (guess) {
	    int px = tx, py = ty;	/* pick location */
	    int dist, idist, nxtdist, d2, id2, nd2;

	    dist = distmin(ux, uy, tx, ty);
	    d2 = dist2(ux, uy, tx, ty);

		/* we may already have an intermediary location picked out */
		if (u.itx || u.ity) {
			idist = distmin(ux, uy, u.itx, u.ity);
			id2 = dist2(ux, uy, u.itx, u.ity);
		}
		else {
			idist = dist;
			id2 = d2;
		}

	    for (tx = 1; tx < COLNO; ++tx)
		for (ty = 0; ty < ROWNO; ++ty)
		    if (travel[tx][ty]) {
			nxtdist = distmin(ux, uy, tx, ty);
			if (nxtdist == dist && (couldsee(tx, ty) || iflags.travelplus)) {
			    nd2 = dist2(ux, uy, tx, ty);
			    if (nd2 < d2) {
				/* prefer non-zigzag path */
				px = tx; py = ty;
				d2 = nd2;
			    }
			} else if (nxtdist < dist && (couldsee(tx, ty) || iflags.travelplus)) {
			    px = tx; py = ty;
			    dist = nxtdist;
			    d2 = dist2(ux, uy, tx, ty);
			}
		    }

	    if (px == u.ux && py == u.uy) {
		/* no guesses, just go in the general direction */
		u.dx = sgn(u.tx - u.ux);
		u.dy = sgn(u.ty - u.uy);
		if (test_move(u.ux, u.uy, u.dx, u.dy, TEST_MOVE))
		    return TRUE;
		goto found;
	    }
		/* which is better - our best in LoS, or our saved intermediate? */
		if (!(u.itx || u.ity) ||
			(dist == idist && d2 < id2) ||
			(dist < idist)
			) {
			u.itx = px;
			u.ity = py;
		}
		else {
			px = u.itx;
			py = u.ity;
		}
	    tx = px;
	    ty = py;
	    ux = u.ux;
	    uy = u.uy;
	    set = 0;
	    n = radius = 1;
	    guess = FALSE;
	    goto noguess;
	}
	return FALSE;
    }

found:
    u.dx = 0;
    u.dy = 0;
    nomul(0, NULL);
    return FALSE;
}

void
domove()
{
	register struct monst *mtmp;
	register struct rm *tmpr;
	register xchar x,y;
	struct trap *trap;
	int wtcap;
	boolean on_ice;
	xchar chainx, chainy, ballx, bally;	/* ball&chain new positions */
	int bc_control;				/* control for ball&chain */
	boolean cause_delay = FALSE, wasblind;	/* dragging ball will skip a move */
	const char *predicament;
	boolean displacer = FALSE;	/* defender attempts to displace you */

	u_wipe_engr(rnd(5));

	if (flags.travel) {
	    if (!findtravelpath(FALSE))
		(void) findtravelpath(TRUE);
	    iflags.travel1 = 0;
	}

	if(((wtcap = near_capacity()) >= OVERLOADED
	    || (wtcap > SLT_ENCUMBER &&
		(Upolyd ? (u.mh < 5 && u.mh != u.mhmax)
			: (u.uhp < 10 && u.uhp != u.uhpmax))))
	   && !Weightless) {
	    if(wtcap < OVERLOADED) {
		You("don't have enough stamina to move.");
		exercise(A_CON, FALSE);
	    } else
		You("collapse under your load.");
	    nomul(0, NULL);
	    return;
	}
	
	if(u.uentangled_oid){
		//Any movement attempt (whether true move or bump attack) tries to break the entangling item.
		if(!ubreak_entanglement()){
			if(u.uentangled_otyp == RAZOR_WIRE){
				int dmg = d(1,6);
				int beat;
				if(hates_silver(youracedata) && entangle_material(&youmonst, SILVER))
					dmg += rnd(20);
				if(hates_iron(youracedata) && (entangle_material(&youmonst, IRON) || entangle_material(&youmonst, GREEN_STEEL)))
					dmg += rnd(u.ulevel);
				if(hates_unholy(youracedata) && entangle_material(&youmonst, GREEN_STEEL))
					dmg += d(2,9);
				beat = entangle_beatitude(&youmonst, -1);
				if(hates_unholy(youracedata) && beat)
					dmg += beat == 2 ? d(2,9) : rnd(9);
				beat = entangle_beatitude(&youmonst, 0);
				if(hates_unblessed(youracedata) && beat)
					dmg += beat == 2 ? d(2,8) : rnd(8);
				beat = entangle_beatitude(&youmonst, 1);
				if(hates_holy(youracedata) && beat)
					dmg += beat == 2 ? rnd(20) : rnd(4);
				losehp(dmg, "being sliced to ribbons by razor wire", KILLED_BY);
			}
		}
	}
	
	if(u.uswallow) {
		if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
			You("pass right through %s!", mon_nam(u.ustuck));
			expels(u.ustuck, u.ustuck->data, 0);
			u.lastmoved = monstermoves;
			flags.move |= MOVE_MOVED;
			return;
		} else {
			extern coord save_d;
			save_d.x = u.dx;
			save_d.y = u.dy;
			u.dx = u.dy = 0;
			u.ux = x = u.ustuck->mx;
			u.uy = y = u.ustuck->my;
			mtmp = u.ustuck;
		}
	}else if(u.ustuck && !flags.nopick
		&& !(u.ustuck->mpeaceful && !Hallucination)
		&& !(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20)
		&& distmin(u.ux, u.uy, u.ustuck->mx, u.ustuck->my) <= 1) {
		u.dx = u.ustuck->mx - u.ux;
		u.dy = u.ustuck->my - u.uy;
		x = u.ustuck->mx;
		y = u.ustuck->my;
		mtmp = u.ustuck;
	}else {
		if (Weightless && rn2(4) &&
			!Levitation && !Flying) {
		    switch(rn2(3)) {
		    case 0:
			You("tumble in place.");
			exercise(A_DEX, FALSE);
			break;
		    case 1:
			You_cant("control your movements very well."); break;
		    case 2:
			pline("It's hard to walk in thin air.");
			exercise(A_DEX, TRUE);
			break;
		    }
		    return;
		}

		/* check slippery ice */
		on_ice = !Levitation && !Flying && (is_ice(u.ux, u.uy) || mad_turn(MAD_COLD_NIGHT));
		if (on_ice) {
			if ((uarmf && (uarmf->otyp == find_skates() || uarmf->oartifact == ART_FROST_TREADS))
			    || resists_cold(&youmonst)
			    || mon_resistance(&youmonst,LEVITATION) || is_clinger(youracedata)
			    || is_whirly(youracedata) || In_icecaves(&u.uz) || (In_tower(&u.uz) && dunlev(&u.uz) == 4 && u.ubranch == ICE_CAVES))
			on_ice = FALSE;
		    else if (!rn2(Cold_resistance ? 3 : 2)) {
			HFumbling &= ~TIMEOUT;
			HFumbling += 1;  /* slip on next move */
		    }
		}

		x = u.ux + u.dx;
		y = u.uy + u.dy;
		if(Stunned || (Confusion && !rn2(5))) {
			register int tries = 0;

			do {
				if(tries++ > 50) {
					nomul(0, NULL);
					return;
				}
				confdir();
				x = u.ux + u.dx;
				y = u.uy + u.dy;
			} while(!isok(x, y) || bad_rock(&youmonst, x, y));
		}
		/* turbulence might alter your actual destination */
		if (u.uinwater && !(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20)) {
			water_friction();
			if (!u.dx && !u.dy) {
				nomul(0, NULL);
				return;
			}
			x = u.ux + u.dx;
			y = u.uy + u.dy;
		}
		if(!isok(x, y)) {
			nomul(0, NULL);
			flags.move |= MOVE_CANCELLED;
			return;
		}
		if (((trap = t_at(x, y)) && trap->tseen) ||
		    (Blind && !Levitation && !Flying &&
		     !is_clinger(youracedata) &&
		     (is_pool(x, y, TRUE) || is_lava(x, y)) && levl[x][y].seenv)) {
			if(flags.run >= 2) {
				nomul(0, NULL);
				flags.move |= MOVE_CANCELLED;
				return;
			} else
				nomul(0, NULL);
		}

		if (u.ustuck && (x != u.ustuck->mx || y != u.ustuck->my)) {
		    if (distu(u.ustuck->mx, u.ustuck->my) > 2) {
			/* perhaps it fled (or was teleported or ... ) */
			u.ustuck = 0;
		    } else if (sticks(&youmonst)) {
			/* When polymorphed into a sticking monster,
			 * u.ustuck means it's stuck to you, not you to it.
			 */
			You("release %s.", mon_nam(u.ustuck));
			u.ustuck = 0;
		    } else {
			/* If holder is asleep or paralyzed:
			 *	37.5% chance of getting away,
			 *	12.5% chance of waking/releasing it;
			 * otherwise:
			 *	 7.5% chance of getting away.
			 * [strength ought to be a factor]
			 * If holder is tame and there is no conflict,
			 * guaranteed escape.
			 */
			switch (rn2(!u.ustuck->mcanmove ? 8 : 40)) {
			case 0: case 1: case 2:
			pull_free:
			    You("pull free from %s.", mon_nam(u.ustuck));
			    u.ustuck = 0;
			    break;
			case 3:
			    if (!u.ustuck->mcanmove) {
				/* it's free to move on next turn */
				u.ustuck->mfrozen = 1;
				u.ustuck->msleeping = 0;
			    }
			    /*FALLTHRU*/
			default:
			    if ((u.ustuck->mtame &&
				!Conflict && !u.ustuck->mberserk && !u.ustuck->mconf) || u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20)
					goto pull_free;
			    You("cannot escape from %s!", mon_nam(u.ustuck));
			    nomul(0, NULL);
			    return;
			}
		    }
		}

		mtmp = m_at(x,y);
		if (mtmp) {
			/* Don't attack if you're running, and can see it */
			/* We should never get here if forcefight */
			if (flags.run &&
			    ((!Blind && mon_visible(mtmp) &&
			      ((mtmp->m_ap_type != M_AP_FURNITURE &&
				mtmp->m_ap_type != M_AP_OBJECT) ||
			       Protection_from_shape_changers)) ||
			     sensemon(mtmp))) {
				nomul(0, NULL);
				flags.move |= MOVE_CANCELLED;
				return;
			}
		}
	}

	u.ux0 = u.ux;
	u.uy0 = u.uy;
	bhitpos.x = x;
	bhitpos.y = y;
	tmpr = &levl[x][y];

	/* attack monster */
	if(flags.forcefight && x==u.ux && y==u.uy){
		if(yn("Really attack yourself?")){
			attack2(&youmonst);
			flags.move |= MOVE_ATTACKED;
		}
		else {
			flags.move |= MOVE_CANCELLED;
		}
		return;
	}
	else if(mtmp) {
	    nomul(0, NULL);
	    /* only attack if we know it's there */
	    /* or if we used the 'F' command to fight blindly */
	    /* or if it hides_under, in which case we call attack() to print
	     * the Wait! message.
	     * This is different from ceiling hiders, who aren't handled in
	     * attack().
	     */

	    /* If they used a 'm' command, trying to move onto a monster
	     * prints the below message and wastes a turn.  The exception is
	     * if the monster is unseen and the player doesn't remember an
	     * invisible monster--then, we fall through to attack() and
	     * attack_check(), which still wastes a turn, but prints a
	     * different message and makes the player remember the monster.		     */
	    if(flags.nopick &&
		  (canspotmon(mtmp) || glyph_is_invisible(levl[x][y].glyph))){
			if((mtmp->m_ap_type && mtmp->m_ap_type != M_AP_MONSTER) && !Protection_from_shape_changers
								&& !sensemon(mtmp))
				stumble_onto_mimic(mtmp);
			else if (mtmp->mpeaceful && !Hallucination)
				pline("Pardon me, %s.", m_monnam(mtmp));
			else
				You("move right into %s.", mon_nam(mtmp));
			return;
	    }
	    if(flags.forcefight || !mtmp->mundetected || sensemon(mtmp) ||
		    ((hides_under(mtmp->data) || is_underswimmer(mtmp->data)) &&
			!is_safepet(mtmp))){
		gethungry();
		if(wtcap >= HVY_ENCUMBER && moves%3) {
		    if (Upolyd && u.mh > 1) {
			u.mh--;
		    } else if (!Upolyd && u.uhp > 1) {
			u.uhp--;
		    } else {
			You("pass out from exertion!");
			exercise(A_CON, FALSE);
			fall_asleep(-10, FALSE);
		    }
		}
		if(multi < 0) return;	/* we just fainted */
		/* new displacer beast thingie -- by [Tom] */
		/* sometimes, instead of attacking, you displace it. */
		/* Good joke, huh? */
		/* Good joke, but players find it irritating */
		// if (mon_resistance(mtmp,DISPLACED) && !rn2(2)) displacer = TRUE;
		if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20) displacer = TRUE;
		/* try to attack; note that it might evade */
		/* also, we don't attack tame when _safepet_ */
		else if (attack2(mtmp)){
			u.uattked = TRUE;
			if(uwep && is_lightsaber(uwep) && litsaber(uwep) && activeFightingForm(FFORM_ATARU)){
				coord cc;
				if(!u.utrap && tt_findadjacent(&cc, mtmp) && (cc.x != u.ux || cc.y != u.uy)){
					You("somersault to a new location!");
					teleds(cc.x, cc.y, FALSE);
				}
			}
			if(has_spear_point(uwep, TURQUOISE)){
				coord cc;
				if(!u.utrap && tt_findadjacent(&cc, mtmp) && (cc.x != u.ux || cc.y != u.uy)){
					You("warp to a new location!");
					teleds(cc.x, cc.y, FALSE);
				}
			}
			if(!(mtmp->mpeaceful) && (!uwep || (!uswapwep && u.twoweap)) && u.umabil & FLICKER_PUNCH && u.uen >= 1){
				coord cc;
				if(!u.utrap && tt_findadjacent(&cc, mtmp) && (cc.x != u.ux || cc.y != u.uy)){
					losepw(1);
					You("warp to a new location!");
					teleds(cc.x, cc.y, FALSE);
				}

			}
			flags.move |= MOVE_ATTACKED;
			return;
		}
	    }
	}

	/* specifying 'F' with no monster wastes a turn */
	if (flags.forcefight ||
	    /* remembered an 'I' && didn't use a move command */
	    (glyph_is_invisible(levl[x][y].glyph) && !flags.nopick)) {
		boolean expl = (Upolyd && attacktype(youracedata, AT_EXPL));
	    	char buf[BUFSZ];
		Sprintf(buf,"a vacant spot on the %s", surface(x,y));
		You("%s %s.",
		    expl ? "explode at" : "attack",
		    !Underwater ? "thin air" :
		    is_pool(x,y, FALSE) ? "empty water" : buf);
		{
			struct attack attkbuff = {0};
			struct attack *attk;
			int i = 0;
			int result = 0;
			struct obj *otmp;
			attk = mon_get_attacktype(&youmonst, AT_WEAP, &attkbuff);
			otmp = uwep;
			if(attk) do {
				/* Streaming mercurial weapons hit an aditional target if your insight is high enough */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && otmp && is_streaming_merc(otmp)){
					if(mlev(&youmonst) > 20 && (Insight > 20 && YOU_MERC_SPECIAL)){
						result |= hit_with_streaming(&youmonst, otmp, x, y, 0, attk);
					}
				}
				/* Rakuyo hit additional targets, if your insight is high enough to percieve the blood */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 20 && otmp && rakuyo_prop(otmp)){
					result |= hit_with_rblood(&youmonst, otmp, x, y, 0, attk);
				}
				/* Chikage launch blood iff you DON'T have a primary target, if your insight is high enough to percieve the blood */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 20 && otmp && otmp->otyp == CHIKAGE && otmp->obj_material == HEMARGYOS){
					result |= hit_with_cblood(&youmonst, otmp, x, y, 0, attk);
				}
				/* Club-claw insight weapons strike additional targets if your insight is high enough to perceive the claw */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 15 && otmp && is_cclub_able(otmp)){
					result |= hit_with_cclaw(&youmonst, otmp, x, y, 0, attk);
				}
				/* Isamusei hit additional targets, if your insight is high enough to percieve the distortions */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 22 && otmp && otmp->otyp == ISAMUSEI){
					result |= hit_with_iwarp(&youmonst, otmp, x, y, 0, attk);
				}
				/* Dancers hit additional targets */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && is_dancer(&youmonst)){
					result |= hit_with_dance(&youmonst, otmp, x, y, 0, attk);
				}
				/* Rejection antenae hit additional targets (last) */
				if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && otmp && check_oprop(otmp, OPROP_ANTAW) && check_reanimation(ANTENNA_REJECT)){
					result |= hit_with_rreject(&youmonst, otmp, x, y, 0, attk);
				}
				
				if(!u.twoweap && !(uwep && (uwep->otyp == STILETTOS || uwep->otyp == WIND_AND_FIRE_WHEELS)))
					break;
				attk = mon_get_attacktype(&youmonst, AT_XWEP, &attkbuff);
				otmp = uswapwep;
				i++;
			} while(i < 2 && attk);
		}
		// unmap_object(x, y); /* known empty -- remove 'I' if present */
		if (glyph_is_invisible(levl[x][y].glyph)) {
			unmap_object(x, y);
			newsym(x, y);
		}
		newsym(x, y);
		nomul(0, NULL);
		if (expl) {
		    u.mh = -1;		/* dead in the current form */
		    rehumanize();
			change_gevurah(1); //cheated death.
		}
		flags.move |= MOVE_ATTACKED;
		return;
	}
	if (glyph_is_invisible(levl[x][y].glyph)) {
	    unmap_object(x, y);
	    newsym(x, y);
	}
	/* not attacking an animal, so we try to move */
	if (!displacer) {

#ifdef STEED
	if (u.usteed && !u.usteed->mcanmove && (u.dx || u.dy)) {
		pline("%s won't move!", upstart(y_monnam(u.usteed)));
		nomul(0, NULL);
		return;
	} else
#endif
	if(!youracedata->mmove) {
		You("are rooted %s.",
		    Levitation || Weightless || Is_waterlevel(&u.uz) ?
		    "in place" : "to the ground");
		nomul(0, NULL);
		return;
	}
	if(u.uentangled_oid && !u.usteed){
		You("struggle against your bindings!");
		nomul(0, NULL);
		return;
	}
	if(u.usteed && u.usteed->entangled_oid){
		pline("Your steed struggles against its bindings!");
		flags.move |= MOVE_CANCELLED;
		nomul(0, NULL);
		return;
	}
	if(u.utrap) {
		boolean usedmove = TRUE;
		if(u.utraptype == TT_PIT) {
			if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
				You("phase through the wall of the pit.");
				u.utrap=0;
		    } else {
			if (!rn2(2) && boulder_at(u.ux, u.uy) && !(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20)) {
				Your("%s gets stuck in a crevice.", body_part(LEG));
				display_nhwindow(WIN_MESSAGE, FALSE);
				clear_nhwindow(WIN_MESSAGE);
				You("free your %s.", body_part(LEG));
		    } else if (!(--u.utrap)) {
				You("%s to the edge of the pit.",
					(In_sokoban(&u.uz) && Levitation) ?
					"struggle against the air currents and float" :
#ifdef STEED
					u.usteed ? "ride" :
#endif
					"crawl");
				fill_pit(u.ux, u.uy);
				vision_full_recalc = 1;	/* vision limits change */
		    } else if(uwep && is_lightsaber(uwep) && litsaber(uwep)){
				trap = t_at(u.ux,u.uy);
				u.utrap = 0;
				pline("The energy blade burns handholds in the side of the pit!");
				if(is_lightsaber(uwep) && uwep->oartifact != ART_INFINITY_S_MIRRORED_ARC && uwep->otyp != KAMEREL_VAJRA) uwep->age -= 200;
				fill_pit(u.ux, u.uy);
				vision_full_recalc = 1;	/* vision limits change */
		    } else if (flags.verbose) {
#ifdef STEED
			if (u.usteed)
			    Norep("%s is still in a pit.",
				  upstart(y_monnam(u.usteed)));
			else
#endif
			Norep( (Hallucination && !rn2(5)) ?
				"You've fallen, and you can't get up." :
				"You are still in a pit." );
		    }
			}
		} else if (u.utraptype == TT_LAVA) {
			if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
				You("phase through the lava.");
				u.utrap = 0;
			} else {
		    if(flags.verbose) {
				predicament = "stuck in the lava";
#ifdef STEED
				if (u.usteed)
					Norep("%s is %s.", upstart(y_monnam(u.usteed)),
					  predicament);
				else
#endif
				Norep("You are %s.", predicament);
		    }
		    if(!is_lava(x,y)) {
			u.utrap--;
			if((u.utrap & 0xff) == 0) {
#ifdef STEED
			    if (u.usteed)
				You("lead %s to the edge of the lava.",
				    y_monnam(u.usteed));
			    else
#endif
			     You("pull yourself to the edge of the lava.");
			    u.utrap = 0;
			}
		    }
		    u.umoved = TRUE;
			}
		} else if (u.utraptype == TT_WEB) {
			if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
				You("phase through the web.");
				u.utrap=0;
		    } else if(uwep && 
				(uwep->oartifact == ART_STING || uwep->oartifact == ART_LIECLEAVER || 
					(is_lightsaber(uwep) && litsaber(uwep))
				)
			){
				trap = t_at(u.ux,u.uy);
				u.utrap = 0;
				pline("%s through the web!", is_lightsaber(uwep) ? "The energy blade burns" : 
									uwep->oartifact == ART_LIECLEAVER ? "Liecleaver cuts" : "Sting cuts");
				if(is_lightsaber(uwep) && uwep->oartifact != ART_INFINITY_S_MIRRORED_ARC && uwep->otyp != KAMEREL_VAJRA) uwep->age -= 100;
				if(trap->ttyp == WEB){
					if(!Is_lolth_level(&u.uz) && !(u.specialSealsActive&SEAL_BLACK_WEB)){
						deltrap(trap);
						newsym(u.ux,u.uy);
					}
				}
				usedmove = FALSE;
			} else {
		    if(--u.utrap) {
			if(flags.verbose) {
			    predicament = "stuck to the web";
#ifdef STEED
			    if (u.usteed)
				Norep("%s is %s.", upstart(y_monnam(u.usteed)),
				      predicament);
			    else
#endif
			    Norep("You are %s.", predicament);
			}
		    } else {
#ifdef STEED
			if (u.usteed)
			    pline("%s breaks out of the web.",
				  upstart(y_monnam(u.usteed)));
			else
#endif
			You("disentangle yourself.");
		    }
			}
		} else if (u.utraptype == TT_SALIVA) {
			if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
				You("phase free from the gooey saliva.");
				u.utrap=0;
			} else {
		    if(--u.utrap) {
			if(flags.verbose) {
			    predicament = "glued down";
#ifdef STEED
			    if (u.usteed)
				Norep("%s is %s.", upstart(y_monnam(u.usteed)),
				      predicament);
			    else
#endif
			    Norep("You are %s.", predicament);
			}
		    } else {
#ifdef STEED
			if (u.usteed)
			    pline("%s pulls loose from the gooey saliva.",
				  upstart(y_monnam(u.usteed)));
			else
#endif
			You("unstick yourself.");
		    }
			}
		} else if (u.utraptype == TT_INFLOOR) {
			if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
				You("phase out of the floor.");
				u.utrap = 0;
			} else {
		    if(--u.utrap) {
			if(flags.verbose) {
			    predicament = "stuck in the";
#ifdef STEED
			    if (u.usteed)
				Norep("%s is %s %s.",
				      upstart(y_monnam(u.usteed)),
				      predicament, surface(u.ux, u.uy));
			    else
#endif
			    Norep("You are %s %s.", predicament,
				  surface(u.ux, u.uy));
			}
		    } else {
#ifdef STEED
			if (u.usteed)
			    pline("%s finally wiggles free.",
				  upstart(y_monnam(u.usteed)));
			else
#endif
			You("finally wiggle free.");
		    }
			}
		} else {
			if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
				You("phase through %s.", the(xname(t_at(u.ux, u.uy)->ammo)));
				u.utrap = 0;
		    } else if(uwep && 
				(is_lightsaber(uwep) && litsaber(uwep))
			){
				trap = t_at(u.ux,u.uy);
				u.utrap = 0;
				pline("The energy blade burns through %s!", the(xname(trap->ammo)));
				if(is_lightsaber(uwep) && uwep->oartifact != ART_INFINITY_S_MIRRORED_ARC && uwep->otyp != KAMEREL_VAJRA) uwep->age -= 100;
				if(trap->ttyp == BEAR_TRAP || trap->ttyp == FLESH_HOOK){
					deltrap(trap);
					newsym(u.ux,u.uy);
				}
				usedmove = FALSE;
			} else {
		    if(flags.verbose) {
				char buf[BUFSZ];
				Sprintf(buf, "caught in %s", an(xname(t_at(u.ux, u.uy)->ammo)));
				predicament = buf;
#ifdef STEED
			if (u.usteed)
			    Norep("%s is %s.", upstart(y_monnam(u.usteed)),
				  predicament);
			else
#endif
			Norep("You are %s.", predicament);
		    }
		    if((u.dx || u.dy) || !rn2(5)) u.utrap--; //was dx && dy, I think this was a typo
			}
		}
		if(!(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20) && usedmove) return;
	}

	if (!test_move(u.ux, u.uy, x-u.ux, y-u.uy, DO_MOVE)) {
		if (!iflags.door_opened) {
		    flags.move |= MOVE_INSTANT;
		    nomul(0, NULL);
		}
	    return;
	}

	} else if (!test_move(u.ux, u.uy, x-u.ux, y-u.uy, TEST_MOVE)) {
	    /*
	     * If a monster attempted to displace us but failed
	     * then we are entitled to our normal attack.
	     */
	    if (!attack2(mtmp)) {
			flags.move |= MOVE_INSTANT;
			nomul(0, NULL);
	    }
		else {
			flags.move |= MOVE_ATTACKED;
		}
	    return;
	}

#ifdef PARANOID
	/* credit - Ron Nazarov via IRC/discord, applied with some minor changes*/
	/* If no 'm' prefix and paranoid_swim enabled, don't allow dangerous moves.  */
	if (iflags.paranoid_swim && (!flags.nopick || flags.run)) {
		static int last_messaged;

	    /* Almost the same conditions as teleportation, except
	     * that lava is allowed if you have wwalking and fireproof
	     * boots and your steed matters.
	     */
	    boolean safe_air = Levitation || Flying;
	    /* The behaviour of air varies between different levels.  */
	    boolean safe_air_level = In_endgame(&u.uz) || (In_FF_quest(&u.uz) && Is_chaos_level(&u.uz));
	    boolean safe_inwater = (Amphibious || Swimming)
		&& !(u.sealsActive&SEAL_OSE) && Waterproof && !level.flags.lethe &&
		/* If you try to ride into water while riding a non-flying steed, you'll fall off.  */
		(!u.usteed || (amphibious_mon(u.usteed) && mon_resistance(u.usteed, FLYING)));

		/* wwalking has to be visible, to prevent identifying via a message prompt
		 * assumes that HWWalking is known always - i.e. level-up (monk) or similar where it messages
		 * checks extrinsic from carry/invoke artifacts as well, but not worn :( those are W_WORN not W_ART(I)
		 * this also assumes the only "object" that grants ww is ww boots
		 */
		boolean ww_boots = (uarmf && uarmf->otyp == WATER_WALKING_BOOTS && objects[WATER_WALKING_BOOTS].oc_name_known);
		boolean visible_ww = ww_boots || u.sealsActive&SEAL_EURYNOME || HWwalking ||
			(u.uprops[WWALKING].extrinsic & W_ARTI) || (u.uprops[WWALKING].extrinsic & W_ART) ||
			(uleft && uleft->oartifact == ART_NENYA) || (uright && uright->oartifact == ART_NENYA);

	    /* Going into 3D water with limited breath can be dangerous. */
	    boolean safe_3dwater = safe_inwater && Breathless;
	    boolean safe_water = safe_inwater || safe_air || (!u.usteed && visible_ww);
	    boolean safe_lava = safe_air ||	(!u.usteed &&
			(likes_lava(youracedata) || (visible_ww && Fire_resistance && (!uarmf || uarmf->oerodeproof || !is_flammable(uarmf)))));

	    if ((!safe_air && !safe_air_level && levl[x][y].typ == AIR && levl[u.ux][u.uy].typ != AIR) ||
			(!safe_water && is_pool(x, y, FALSE) && !is_pool(u.ux, u.uy, FALSE)) ||
			(!safe_3dwater && is_3dwater(x, y) && !is_3dwater(u.ux, u.uy)) ||
			(!safe_lava && is_lava(x, y) && !is_lava(u.ux, u.uy))
		){
			You("avoid %s into the %s.", ing_suffix(locomotion(&youmonst, "step")), levl[x][y].typ == AIR ? "open air" : waterbody_name(x, y));

			if (!last_messaged || (moves - last_messaged) >= 11)
				last_messaged = moves;

			if (last_messaged >= moves){
				pline("(Use 'm' prefix to step in if you really want to.)");
				last_messaged = moves - 1;
			}

			flags.move |= MOVE_CANCELLED;
			nomul(0, NULL);
			return;
	    }
	}
#endif
	
	/* Move ball and chain.  */
	if (Punished){
		if(u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20){
			You("slip out of the iron chain.");
			unpunish();
		} else {
			if (!drag_ball(x,y, &bc_control, &ballx, &bally, &chainx, &chainy,
				&cause_delay, TRUE))
			return;
		}
	}
	
	/* Check regions entering/leaving */
	if (!in_out_region(x,y)) {
#if 0
	    /* [ALI] This can't happen at present, but if it did we would
	     * also need to worry about the call to drag_ball above.
	     */
	    if (displacer) (void)attack2(mtmp);
#endif
	    return;
	}

 	/* now move the hero */
	mtmp = m_at(x, y);
	u.lastmoved = monstermoves;
	u.ux += u.dx;
	u.uy += u.dy;
#ifdef STEED
	/* Move your steed, too */
	if (u.usteed) {
		u.usteed->mx = u.ux;
		u.usteed->my = u.uy;
		exercise_steed();
	}
#endif

	if (displacer) {
	    char pnambuf[BUFSZ];

	    u.utrap = 0;			/* A lucky escape */
	    /* save its current description in case of polymorph */
	    Strcpy(pnambuf, mon_nam(mtmp));
		if(mtmp->wormno) remove_worm(mtmp);
		else remove_monster(x, y);
	    place_monster(mtmp, u.ux0, u.uy0);
		if(mtmp->wormno) place_worm_tail_randomly(mtmp,u.ux0,u.uy0);
	    /* check for displacing it into pools and traps */
	    switch (minliquid(mtmp) ? 2 : mintrap(mtmp)) {
		case 0:
		    You("displaced %s.", pnambuf);
		    break;
		case 1:
		case 3:
		    break;
		case 2:
		    u.uconduct.killer++;
			if(mtmp->mtyp == PM_CROW && u.sealsActive&SEAL_MALPHAS) unbind(SEAL_MALPHAS,TRUE);
		    break;
	    }
	}

	/*
	 * If safepet at destination then move the pet to the hero's
	 * previous location using the same conditions as in attack().
	 * there are special extenuating circumstances:
	 * (1) if the pet dies then your god angers,
	 * (2) if the pet gets trapped then your god may disapprove,
	 * (3) if the pet was already trapped and you attempt to free it
	 * not only do you encounter the trap but you may frighten your
	 * pet causing it to go wild!  moral: don't abuse this privilege.
	 *
	 * Ceiling-hiding pets are skipped by this section of code, to
	 * be caught by the normal falling-monster code.
	 */
	if (is_safepet(mtmp) && !(is_hider(mtmp->data) && mtmp->mundetected)) {
	    mtmp->mundetected = 0;
	    if (mtmp->m_ap_type) seemimic(mtmp);

	    if (mtmp->mtrapped &&
		    (trap = t_at(mtmp->mx, mtmp->my)) != 0 && ((
				(trap->ttyp == PIT || trap->ttyp == SPIKED_PIT) &&
				boulder_at(trap->tx, trap->ty)
			) || (
				(trap->ttyp == VIVI_TRAP)
			))
		) {
			/* can't swap places with pet pinned in a pit by a boulder, or one stuck in an essence trap */
			u.ux = u.ux0,  u.uy = u.uy0;	/* didn't move after all */
	    } else if (u.ux0 != x && u.uy0 != y &&
		       bad_rock(mtmp, x, u.uy0) &&
		       bad_rock(mtmp, u.ux0, y) &&
		       (bigmonst(mtmp->data) || (curr_mon_load(mtmp) > 600))
		) {
			/* can't swap places when pet won't fit thru the opening */
			u.ux = u.ux0,  u.uy = u.uy0;	/* didn't move after all */
			You("stop.  %s won't fit through.", upstart(y_monnam(mtmp)));
	    } else if (mtmp->mpeaceful && !mtmp->mtame
		    && (!goodpos(u.ux0, u.uy0, mtmp, 0)
			|| t_at(u.ux0, u.uy0) != NULL
			|| mtmp->m_id == quest_status.leader_m_id)
		) {
			u.ux = u.ux0, u.uy = u.uy0; /* didn't move after all */
			You("stop. %s doesn't want to swap places.",
				upstart(y_monnam(mtmp)));

	    } else if (mtmp->mpeaceful && !mtmp->mtame
		    && (!goodpos(u.ux0, u.uy0, mtmp, 0)
			|| t_at(u.ux0, u.uy0) != NULL
			|| mtmp->m_id == quest_status.leader_m_id)
		) {
			u.ux = u.ux0, u.uy = u.uy0; /* didn't move after all */
			You("stop. %s doesn't want to swap places.",
				upstart(y_monnam(mtmp)));

	    } else {
			char pnambuf[BUFSZ];
			extern const int clockwisex[8];
			extern const int clockwisey[8];
			int newx = 0;
			int newy = 0;

			/* goodpos for pet? otherwise we shunt somewhere else
			 * pet knowledge of pets checks tseen / searching. polytraps and */

			boolean pet_goodpos = goodpos(u.ux0, u.uy0, mtmp, 0);
			
			if ((trap = t_at(x, y)) != NULL && (trap->tseen || mon_resistance(mtmp, SEARCHING)) &&
				!(trap->ttyp == MAGIC_PORTAL || trap->ttyp == POLY_TRAP))
					pet_goodpos = FALSE;

			/* save its current description in case of polymorph */
			Strcpy(pnambuf, y_monnam(mtmp));

			int i = rnd(8), j;
#define tmpx clockwisex[(i + j) % 8] + x
#define tmpy clockwisey[(i + j) % 8] + y
			if (!pet_goodpos){
				/* clear for goodpos */
				u.ux = u.ux0, u.uy = u.uy0;
				for (j = 8; j >= 1; j--){
					if (!goodpos(tmpx, tmpy, mtmp, 0))
						continue;
					
					trap = t_at(tmpx, tmpy);
					if (trap != NULL && ((trap && trap->tseen) || mon_resistance(mtmp, SEARCHING)))
						continue;

					newx = tmpx;
					newy = tmpy;
					break;
				}
			} else {
				newx = u.ux0;
				newy = u.uy0;
			}
#undef tmpx
#undef tmpy
			if (newx && newy){
				u.ux = x, u.uy = y; /* re-move, cleared to check goodpos */
				mtmp->mtrapped = 0;
				remove_monster(x, y);
				place_monster(mtmp, newx, newy);
				newsym(newx, newy);
				newsym(x, y);
				newsym(u.ux0, u.uy0);
			}
			
			if (mtmp->mx == x && mtmp->my == y){
				You("stop. %s doesn't want to swap places.", upstart(y_monnam(mtmp)));
			} else {
				/* check for displacing it into pools and traps. 
				 * pools should never happen due to goodpos - but just in case, not removing handling */
				trap = t_at(u.ux0, u.uy0);
				int switchcase = minliquid(mtmp) ? 2 : mintrap(mtmp);
				switch (switchcase) {
					case 0:
						You("%s %s.", mtmp->mpeaceful ? "displaced" : "frightened", pnambuf);
						break;
					case 1:		/* trapped */
					case 3:		/* changed levels */
						/* there's already been a trap message, reinforce it */
						if(!(switchcase == 3 && trap->ttyp == MAGIC_PORTAL)){
							abuse_dog(mtmp);
							adjalign(-3);
						}
						break;
					case 2:
						/* it may have drowned or died.  that's no way to
						 * treat a pet!  your god gets angry.
						 */
						if (rn2(4)) {
							You_feel("guilty about losing your pet like this.");
							if(!Role_if(PM_ANACHRONOUNBINDER)) godlist[u.ualign.god].anger++;
							adjalign(-15);
						}
						/* you killed your pet by direct action.
						 * minliquid and mintrap don't know to do this
						 */
						u.uconduct.killer++;
						if(mtmp->mtyp == PM_CROW && u.sealsActive&SEAL_MALPHAS) unbind(SEAL_MALPHAS,TRUE);
						break;
					default:
						pline("that's strange, unknown mintrap result!");
						break;
				}
			}
	    }
	}

	reset_occupations();
	if (flags.run) {
	    if ( flags.run < 8 )
		if (IS_DOOR(tmpr->typ) || IS_ROCK(tmpr->typ) ||
			IS_FURNITURE(tmpr->typ))
		    nomul(0, NULL);
	}

	if (hides_under(youracedata))
	    u.uundetected = OBJ_AT(u.ux, u.uy);
	else if (is_underswimmer(youracedata))
	    u.uundetected = is_pool(u.ux, u.uy, FALSE) && !Is_waterlevel(&u.uz);
	else if (u.dx || u.dy)
	    u.uundetected = 0;

	/*
	 * Mimics (or whatever) become noticeable if they move and are
	 * imitating something that doesn't move.  We could extend this
	 * to non-moving monsters...
	 */
	if ((u.dx || u.dy) && (youmonst.m_ap_type == M_AP_OBJECT
				|| youmonst.m_ap_type == M_AP_FURNITURE))
	    youmonst.m_ap_type = M_AP_NOTHING;

	check_leash(u.ux0,u.uy0);

	if(u.ux0 != u.ux || u.uy0 != u.uy) {
	    u.umoved = TRUE;
	    /* Clean old position -- vision_recalc() will print our new one. */
	    newsym(u.ux0,u.uy0);
	    /* Since the hero has moved, adjust what can be seen/unseen. */
	    vision_recalc(1);	/* Do the work now in the recover time. */
	    invocation_message();
	}

	if (Punished){				/* put back ball and chain */
	    move_bc(0,bc_control,ballx,bally,chainx,chainy);
	}

	spoteffects(TRUE);
	
	if(u.specialSealsActive&SEAL_BLACK_WEB && u.spiritPColdowns[PWR_WEAVE_BLACK_WEB] > moves+20){
		weave_black_web((struct monst *)0);
	}
	
	/* delay next move because of ball dragging */
	/* must come after we finished picking up, in spoteffects() */
	if (cause_delay) {
	    nomul(-2, "dragging an iron ball");
	    nomovemsg = "";
	}

	if (flags.run && iflags.runmode != RUN_TPORT) {
		static unsigned long runmoves = 0;
		runmoves++;
	    /* display every step or every 7th step depending upon mode */
	    if (iflags.runmode != RUN_LEAP || !(runmoves % 7L)) {
		if (flags.time) flags.botl = 1;
		curs_on_u();
		delay_output();
		if (iflags.runmode == RUN_CRAWL) {
		    delay_output();
		    delay_output();
		    delay_output();
		    delay_output();
		}
	    }
	}
	flags.move |= MOVE_MOVED;
}

void
invocation_message()
{
	/* a special clue-msg when on the Invocation position */
	if(invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy)) {
	    char buf[BUFSZ];
	    struct obj *otmp = carrying(CANDELABRUM_OF_INVOCATION);

	    nomul(0, NULL);		/* stop running or travelling */
#ifdef STEED
	    if (u.usteed) Sprintf(buf, "beneath %s", y_monnam(u.usteed));
	    else
#endif
	    if (Levitation || Flying) Strcpy(buf, "beneath you");
	    else Sprintf(buf, "under your %s", makeplural(body_part(FOOT)));

	    You_feel("a strange vibration %s.", buf);
		u.uevent.found_square = 1;
	    if (otmp && otmp->spe == 7 && otmp->lamplit)
		pline("%s %s!", The(xname(otmp)),
		    Blind ? "throbs palpably" : "glows with a strange light");
	} else if(!u.uevent.found_square && invocation_distmin(u.ux, u.uy) <= 2) {
	    char buf[BUFSZ];
	    nomul(0, NULL);		/* stop running or travelling */
#ifdef STEED
	    if (u.usteed) Sprintf(buf, "beneath %s", y_monnam(u.usteed));
	    else
#endif
	    if (Levitation || Flying) Strcpy(buf, "beneath you");
	    else Sprintf(buf, "under your %s", makeplural(body_part(FOOT)));

	    if(invocation_distmin(u.ux, u.uy) == 2) You_feel("a slight vibration %s.", buf);
	    else You_feel("a vibration %s.", buf);
	}
}

#endif /* OVL3 */
#ifdef OVL2

void
spoteffects(pick)
boolean pick;
{
	register struct monst *mtmp;

	if(u.uinwater) {
		int was_underwater;

		if (!is_pool(u.ux,u.uy, FALSE)) {
			if (Is_waterlevel(&u.uz))
				You("pop into an air bubble.");
			else if (is_lava(u.ux, u.uy))
				if(likes_lava(youracedata)) You("swim into lava.");
				else You("leave the water...");	/* oops! */
			else
				You("are on solid %s again.",
				    is_ice(u.ux, u.uy) ? "ice" : "land");
		}
		else if (is_3dwater(u.ux, u.uy))
			goto stillinwater;
		else if (Levitation)
			You("pop out of the water like a cork!");
		else if (Flying && !Underwater)
			You("fly out of the water.");
		else if (uarmf && uarmf->oartifact == ART_FROST_TREADS)
			You("climb stairs of ice out of the water.");
		else if (Wwalking && !Underwater)
			You("slowly rise above the surface.");
		else
			goto stillinwater;
		was_underwater = Underwater;
		u.uinwater = 0;		/* leave the water */
		u.usubwater = 0;		/* leave the water */
		if (was_underwater) {	/* restore vision */
			docrt();
			vision_full_recalc = 1;
		}
	}
stillinwater:;
	if (((!Levitation && !Flying) || is_3dwater(u.ux, u.uy)) && !u.ustuck) {
	    /* limit recursive calls through teleds() */
	    if (is_pool(u.ux, u.uy, FALSE) || is_lava(u.ux, u.uy)) {
#ifdef STEED
		if (u.usteed && !mon_resistance(u.usteed,FLYING) &&
			!mon_resistance(u.usteed,LEVITATION) &&
			!is_clinger(u.usteed->data)) {
		    dismount_steed(Underwater ?
			    DISMOUNT_FELL : DISMOUNT_GENERIC);
		    /* dismount_steed() -> float_down() -> pickup() */
		    if (!Weightless && !Is_waterlevel(&u.uz))
			pick = FALSE;
		} else
#endif
		if (is_lava(u.ux, u.uy)) {
		    if (lava_effects(TRUE)) return;
		} else if (!Wwalking && drown())
		    return;
	    } else if (IS_PUDDLE(levl[u.ux][u.uy].typ) && !Wwalking) {

			/*You("%s through the shallow water.",
				verysmall(youracedata) ? "wade" : "splash");
			if (!verysmall(youracedata) && !rn2(4)) wake_nearby();*/

			if(u.umonnum == PM_GREMLIN)
				(void)split_mon(&youmonst, (struct monst *)0);
			else if (is_iron(youracedata) &&
				/* mud boots keep the feet dry */
				(!uarmf || strncmp(OBJ_DESCR(objects[uarmf->otyp]), "mud ", 4))
			) {
				int dam = rnd(6);
				Your("%s rust!", makeplural(body_part(FOOT)));
				if (u.mhmax > dam) u.mhmax -= dam;
				losehp(dam, "rusting away", KILLED_BY);
			// } else if (is_longworm(youracedata)) { /* water is lethal to Shai-Hulud */
				// int dam = d(3,12);
				// if (u.mhmax > dam) u.mhmax -= (dam+1) / 2;
					// pline_The("water burns your flesh!");
				// losehp(dam,"contact with water",KILLED_BY);
			}
			if (!u.usteed){
				static long rustmessage_turn = 0L;
				if (verysmall(youracedata)){
					water_damage(invent, FALSE,FALSE,FALSE,(struct monst *) 0);
				}
				else if(rustmessage_turn < monstermoves || (uarmf && is_rustprone(uarmf) && !uarmf->oerodeproof && uarmf->oeroded != MAX_ERODE)){
					(void)rust_dmg(uarmf, "boots", 1, TRUE, &youmonst, FALSE);
					rustmessage_turn = monstermoves + 100;
				}
			}
		}
		if (uarmf && uarmf->oartifact == ART_FROST_TREADS
			&& is_pool(u.ux, u.uy, TRUE) && !is_3dwater(u.ux, u.uy) && !Is_waterlevel(&u.uz)) {
			zap_over_floor(u.ux, u.uy, AD_COLD, WAND_CLASS, FALSE, NULL);
		}
	}
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
	check_special_room(FALSE);
#ifdef SINKS
	if(IS_SINK(levl[u.ux][u.uy].typ) && Levitation)
		dosinkfall();
#endif
	if (!in_steed_dismounting) { /* if dismounting, we'll check again later */
		struct trap *trap = t_at(u.ux, u.uy);
		boolean pit;
		pit = (trap && (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT));
		if (trap && pit)
			dotrap(trap, 0);	/* fall into pit */
		if (pick) (void) pickup(1);
		if (trap && !pit)
			dotrap(trap, 0);	/* fall into arrow trap, etc. */
		else
			sigilfloat();	
	}
	if((mtmp = m_at(u.ux, u.uy)) && !u.uswallow) {
		mtmp->mundetected = mtmp->msleeping = 0;
		switch(mtmp->data->mlet) {
		    case S_PIERCER:
			pline("%s suddenly drops from the %s!",
			      Amonnam(mtmp), ceiling(u.ux,u.uy));
			if(mtmp->mtame) /* jumps to greet you, not attack */
			    ;
			else if(uarmh && is_hard(uarmh)){
			    int dmg;
			    pline("Its blow glances off your helmet.");
				if(((mtmp->m_lev) - 8) > 0){
				    dmg = d((mtmp->m_lev) - 5,3);
					dmg = reduce_dmg(&youmonst,dmg,TRUE,FALSE);
				    mdamageu(mtmp, dmg);
				}
			}
			else if(umechanoid){
			    int dmg;
			    pline("Its blow glances off your head.");
				if(((mtmp->m_lev) - 8) > 0){
				    dmg = d((mtmp->m_lev) - 5,3);
					dmg = reduce_dmg(&youmonst,dmg,TRUE,FALSE);
				    mdamageu(mtmp, dmg);
				}
			} else if (u.uac + 3 <= rnd(20))
			    You("are almost hit by %s!",
				x_monnam(mtmp, ARTICLE_A, "falling", 0, TRUE));
			else {
			    int dmg;
			    You("are hit by %s!",
				x_monnam(mtmp, ARTICLE_A, "falling", 0, TRUE));
			    dmg = d(mtmp->m_lev,6);
				dmg = reduce_dmg(&youmonst,dmg,TRUE,FALSE);
			    mdamageu(mtmp, dmg);
			}
			break;
		    default:	/* monster surprises you. */
			if(mtmp->mtame)
			    pline("%s jumps near you from the %s.",
					Amonnam(mtmp), ceiling(u.ux,u.uy));
			else if(mtmp->mpeaceful) {
				You("surprise %s!",
				    Blind && !sensemon(mtmp) ?
				    something : a_monnam(mtmp));
				mtmp->mpeaceful = 0;
			} else
			    pline("%s attacks you by surprise!",
					Amonnam(mtmp));
			break;
		}
		if (mon_attacktype(mtmp, AT_ENGL)) {
			/* engulf the player */
			(void)xengulfhity(mtmp, &youmonst, attacktype_fordmg(mtmp->data, AT_ENGL, AD_ANY), 6);	/* vis == VIS_MDEF|VIS_NONE */
		}
		else {
			mnexto(mtmp); /* have to move the monster */
		}
	}
}

void sigilfloat()
{
	if(!Is_sigil(&u.uz)) return;
	if(levl[u.ux][u.uy].typ != AIR) return;
	if(Flying || Levitation) return;
	if(u.uhave.amulet){
		pline("The amulet holds you above the ground below.");
		return;
	}
	schedule_goto(&spire_level, FALSE, FALSE, 0,
		      "You fall from the spire! A mysterious force pads your landing.",
		      (char *)0, 0, 0);

}

STATIC_OVL boolean
monstinroom(mdat,roomno)
struct permonst *mdat;
int roomno;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(!DEADMONSTER(mtmp) && mtmp->data == mdat &&
		   index(in_rooms(mtmp->mx, mtmp->my, 0), roomno + ROOMOFFSET))
			return(TRUE);
	return(FALSE);
}

char *
in_rooms(x, y, typewanted)
register xchar x, y;
register int typewanted;
{
	static char buf[5];
	char rno, *ptr = &buf[4];
	int typefound, min_x, min_y, max_x, max_y_offset, step;
	register struct rm *lev;

#define goodtype(rno) (!typewanted || \
	     ((typefound = rooms[rno - ROOMOFFSET].rtype) == typewanted) || \
	     ((typewanted == SHOPBASE) && (typefound > SHOPBASE))) \

	switch (rno = levl[x][y].roomno) {
		case NO_ROOM:
			return(ptr);
		case SHARED:
			step = 2;
			break;
		case SHARED_PLUS:
			step = 1;
			break;
		default:			/* i.e. a regular room # */
			if (goodtype(rno))
				*(--ptr) = rno;
			return(ptr);
	}

	min_x = x - 1;
	max_x = x + 1;
	if (x < 1)
		min_x += step;
	else
	if (x >= COLNO)
		max_x -= step;

	min_y = y - 1;
	max_y_offset = 2;
	if (min_y < 0) {
		min_y += step;
		max_y_offset -= step;
	} else
	if ((min_y + max_y_offset) >= ROWNO)
		max_y_offset -= step;

	for (x = min_x; x <= max_x; x += step) {
		lev = &levl[x][min_y];
		y = 0;
		if (((rno = lev[y].roomno) >= ROOMOFFSET) &&
		    !index(ptr, rno) && goodtype(rno))
			*(--ptr) = rno;
		y += step;
		if (y > max_y_offset)
			continue;
		if (((rno = lev[y].roomno) >= ROOMOFFSET) &&
		    !index(ptr, rno) && goodtype(rno))
			*(--ptr) = rno;
		y += step;
		if (y > max_y_offset)
			continue;
		if (((rno = lev[y].roomno) >= ROOMOFFSET) &&
		    !index(ptr, rno) && goodtype(rno))
			*(--ptr) = rno;
	}
	return(ptr);
}

/* is (x,y) in a town? */
boolean
in_town(x, y)
register int x, y;
{
	s_level *slev = Is_special(&u.uz);
	register struct mkroom *sroom;
	boolean has_subrooms = FALSE;

	if (!slev || !slev->flags.town) return FALSE;

	/*
	 * See if (x,y) is in a room with subrooms, if so, assume it's the
	 * town.  If there are no subrooms, the whole level is in town.
	 */
	for (sroom = &rooms[0]; sroom->hx > 0; sroom++) {
	    if (sroom->nsubrooms > 0) {
		has_subrooms = TRUE;
		if (inside_room(sroom, x, y)) return TRUE;
	    }
	}

	return !has_subrooms;
}

STATIC_OVL void
move_update(newlev)
register boolean newlev;
{
	char *ptr1, *ptr2, *ptr3, *ptr4;

	Strcpy(u.urooms0, u.urooms);
	Strcpy(u.ushops0, u.ushops);
	if (newlev) {
		u.urooms[0] = '\0';
		u.uentered[0] = '\0';
		u.ushops[0] = '\0';
		u.ushops_entered[0] = '\0';
		Strcpy(u.ushops_left, u.ushops0);
		return;
	}
	Strcpy(u.urooms, in_rooms(u.ux, u.uy, 0));

	for (ptr1 = &u.urooms[0],
	     ptr2 = &u.uentered[0],
	     ptr3 = &u.ushops[0],
	     ptr4 = &u.ushops_entered[0];
	     *ptr1; ptr1++) {
		if (!index(u.urooms0, *ptr1))
			*(ptr2++) = *ptr1;
		if (IS_SHOP(*ptr1 - ROOMOFFSET)) {
			*(ptr3++) = *ptr1;
			if (!index(u.ushops0, *ptr1))
				*(ptr4++) = *ptr1;
		}
	}
	*ptr2 = '\0';
	*ptr3 = '\0';
	*ptr4 = '\0';

	/* filter u.ushops0 -> u.ushops_left */
	for (ptr1 = &u.ushops0[0], ptr2 = &u.ushops_left[0]; *ptr1; ptr1++)
		if (!index(u.ushops, *ptr1))
			*(ptr2++) = *ptr1;
	*ptr2 = '\0';
}

void
check_special_room(newlev)
register boolean newlev;
{
	register struct monst *mtmp;
	char *ptr;

	move_update(newlev);

	if (*u.ushops0)
	    u_left_shop(u.ushops_left, newlev);

	if (!*u.uentered && !*u.ushops_entered)		/* implied by newlev */
	    return;		/* no entrance messages necessary */

	/* Did we just enter a shop? */
	if (*u.ushops_entered) {
		int i = 0;
		while(u.ushops_entered[i]) {
	    	u_entered_shop(&u.ushops_entered[i]);
			i++;
		}
	}

	for (ptr = &u.uentered[0]; *ptr; ptr++) {
	    register int roomno = *ptr - ROOMOFFSET, rt = rooms[roomno].rtype;
		boolean wake = FALSE;

	    /* Did we just enter some other special room? */
	    /* vault.c insists that a vault remain a VAULT,
		 * BEEHIVEs, GARDENs have special colouration,
	     * and temples should remain TEMPLEs,
	     * but everything else gives a message only the first time */
	    switch (rt) {
		case ZOO:
		    pline("Welcome to David's treasure zoo!");
			wake = TRUE;
	    break;
		case GARDEN:
		    if (Blind) pline_The("air here smells nice and fresh!");
		    else You("enter a beautiful garden.");
			rt = 0;
			wake = TRUE;
	    break;
		case LIBRARY:
		    if (Blind) pline_The("air here smells of mildew.");
		    else You("enter a musty, water damaged-library.");
	    break;
		case SWAMP:
		    pline("It %s rather %s down here.",
			  Blind ? "feels" : "looks",
			  Blind ? "humid" : "muddy");
			rt = 0;
			wake = TRUE;
	    break;
		case COURT:
		    You("enter an opulent throne room!");
			rt = 0;
			wake = TRUE;
	    break;
		case LEPREHALL:
		    You("enter a leprechaun hall!");
			rt = 0;
	    break;
		case MORGUE:
		    if(midnight()) {
				const char *run = locomotion(&youmonst, "Run");
				pline("%s away!  %s away!", run, run);
		    } else
				You("have an uncanny feeling...");
			rt = 0;
			wake = TRUE;
	    break;
		case BEEHIVE:
			if (monstinroom(&mons[PM_QUEEN_BEE], roomno)) {
				You("enter a giant beehive!");
			}
			rt = 0;
			wake = TRUE;
		break;
		case COCKNEST:
		    You("enter a disgusting nest!");
			rt = 0;
	    break;
		case ANTHOLE:
		    You("enter an anthole!");
			rt = 0;
			wake = TRUE;
	    break;
		case BARRACKS:
			if (Is_lolth_level(&u.uz))
				if (monstinroom(&mons[PM_GNOLL], roomno))
					You("enter a gnoll barracks.");
				else
					You("enter an abandoned barracks.");
			else if (In_outlands(&u.uz))
				if (monstinroom(&mons[PM_FERRUMACH_RILMANI], roomno) ||
					monstinroom(&mons[PM_IRON_GOLEM], roomno) ||
					monstinroom(&mons[PM_ARGENTUM_GOLEM], roomno) ||
					monstinroom(&mons[PM_CUPRILACH_RILMANI], roomno))
					You("enter a rilmani barracks!");
				else
					You("enter an abandoned barracks.");
			else if (In_hell(&u.uz))
				if (monstinroom(&mons[PM_LEGION_DEVIL_GRUNT], roomno) ||
					monstinroom(&mons[PM_LEGION_DEVIL_SOLDIER], roomno) ||
					monstinroom(&mons[PM_LEGION_DEVIL_SERGEANT], roomno) ||
					monstinroom(&mons[PM_LEGION_DEVIL_CAPTAIN], roomno))
					You("enter a legion barracks!");
				else
					You("enter an abandoned barracks.");
			else if (In_dismalswamp(&u.uz))
				if (monstinroom(&mons[PM_KOBOLD], roomno) ||
					monstinroom(&mons[PM_KOBOLD_BRUTE], roomno) ||
					monstinroom(&mons[PM_KOBOLD_LORD], roomno) ||
					monstinroom(&mons[PM_KOBOLD_SHAMAN], roomno))
					You("enter a kobold bastion!");
				else
					You("enter an abandoned bastion.");
			else
				if(monstinroom(&mons[PM_SOLDIER], roomno) ||
				monstinroom(&mons[PM_SERGEANT], roomno) ||
				monstinroom(&mons[PM_LIEUTENANT], roomno) ||
				monstinroom(&mons[PM_CAPTAIN], roomno))
					You("enter a military barracks!");
				else
					You("enter an abandoned barracks.");
			rt = 0;
	    break;
		case ARMORY:
			You("enter a dilapidated armory.");
	    break;
		case DELPHI:
		    if(monstinroom(&mons[PM_ORACLE], roomno))
			verbalize("%s, %s, welcome to Delphi!",
					Hello((struct monst *) 0), plname);
		    break;
		case TEMPLE:
		    intemple(roomno + ROOMOFFSET);
		    /* fall through */
		default:
		    rt = 0;
	    }

	    if (rt != 0) {
			rooms[roomno].rtype = OROOM;
			if (!search_special(rt)) {
				/* No more room of that type */
				switch(rt) {
					case COURT:
					level.flags.has_court = 0;
					break;
					case SWAMP:
					level.flags.has_swamp = 0;
					break;
					case MORGUE:
					level.flags.has_morgue = 0;
					break;
					case ZOO:
					level.flags.has_zoo = 0;
					break;
					case ARMORY:
					level.flags.has_armory = 0;
					break;
					case BARRACKS:
					level.flags.has_barracks = 0;
					break;
					case TEMPLE:
					level.flags.has_temple = 0;
					break;
					case BEEHIVE:
					level.flags.has_beehive = 0;
					break;
				}
			}
	    }
		if (wake)
		    for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
				if (!DEADMONSTER(mtmp) && !Stealth && !rn2(3)) mtmp->msleeping = 0;
	}

	return;
}

#endif /* OVL2 */
#ifdef OVLB

int
dopickup()
{
	int count;
	struct trap *traphere = t_at(u.ux, u.uy);
 	/* awful kludge to work around parse()'s pre-decrement */
	count = (multi || (save_cm && *save_cm == ',')) ? multi + 1 : 0;
	multi = 0;	/* always reset */
	/* uswallow case added by GAN 01/29/87 */
	if(u.uswallow) {
	    if (!u.ustuck->minvent) {
		if (is_animal(u.ustuck->data)) {
		    You("pick up %s tongue.", s_suffix(mon_nam(u.ustuck)));
		    pline("But it's kind of slimy, so you drop it.");
		} else
		    You("don't %s anything in here to pick up.",
			  Blind ? "feel" : "see");
		return MOVE_STANDARD;
	    } else {
	    	int tmpcount = -count;
		return loot_mon(u.ustuck, &tmpcount, (boolean *)0);
	    }
	}
	if(is_pool(u.ux, u.uy, FALSE) && !is_3dwater(u.ux, u.uy) && !Is_waterlevel(&u.uz)) {//pools (bubble interior) on water level are special
	    if (Wwalking || mon_resistance(&youmonst,LEVITATION) || is_clinger(youracedata)
			|| (Flying && !Breathless)) {
		You("cannot dive into the water to pick things up.");
		return MOVE_CANCELLED;
	    } else if (!Underwater) {
		You_cant("even see the bottom, let alone pick up %s.",
				something);
		return MOVE_CANCELLED;
	    }
	}
	if (is_lava(u.ux, u.uy)) {
	    if (((Wwalking || (Flying && !Breathless)) && !likes_lava(youracedata)) 
			    || (mon_resistance(&youmonst,LEVITATION) || is_clinger(youracedata))
			) {
		You_cant("reach the bottom to pick things up.");
		return MOVE_CANCELLED;
	    } else if (!likes_lava(youracedata)) {
		You("would burn to a crisp trying to pick things up.");
		return MOVE_CANCELLED;
	    }
	}
	if(!OBJ_AT(u.ux, u.uy)) {
		There("is nothing here to pick up.");
		return MOVE_CANCELLED;
	}
	if (!can_reach_floor()) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
		    You("aren't skilled enough to reach from %s.",
			y_monnam(u.usteed));
		else
#endif
		You("cannot reach the %s.", surface(u.ux,u.uy));
		return MOVE_CANCELLED;
	}

 	if (traphere && traphere->tseen) {
		/* Allow pickup from holes and trap doors that you escaped from
		 * because that stuff is teetering on the edge just like you, but
		 * not pits, because there is an elevation discrepancy with stuff
		 * in pits.
		 */
		if ((traphere->ttyp == PIT || traphere->ttyp == SPIKED_PIT) &&
		     (!u.utrap || (u.utrap && u.utraptype != TT_PIT)) && !Flying) {
			You("cannot reach the bottom of the pit.");
			return MOVE_CANCELLED;
		}
	}

	return (pickup(-count)) ? MOVE_STANDARD : MOVE_CANCELLED;
}

#endif /* OVLB */
#ifdef OVL2

/* stop running if we see something interesting */
/* turn around a corner if that is the only way we can proceed */
/* do not turn left or right twice */
void
lookaround()
{
    register int x, y, i, x0 = 0, y0 = 0, m0 = 1, i0 = 9;
    register int corrct = 0, noturn = 0;
    register struct monst *mtmp;
    register struct trap *trap;

    /* Grid bugs stop if trying to move diagonal, even if blind.  Maybe */
    /* they polymorphed while in the middle of a long move. */
    if ((u.umonnum == PM_GRID_BUG || u.umonnum == PM_BEBELITH) && u.dx && u.dy) {
	nomul(0, NULL);
	return;
    }

    if(Blind || flags.run == 0) return;
    for(x = u.ux-1; x <= u.ux+1; x++) for(y = u.uy-1; y <= u.uy+1; y++) {
	if(!isok(x,y)) continue;

	if((u.umonnum == PM_GRID_BUG || u.umonnum == PM_BEBELITH) && x != u.ux && y != u.uy) continue;

	if(x == u.ux && y == u.uy) continue;

	if((mtmp = m_at(x,y)) &&
		    mtmp->m_ap_type != M_AP_FURNITURE &&
		    mtmp->m_ap_type != M_AP_OBJECT &&
			(!mtmp->minvis || See_invisible(mtmp->mx, mtmp->my)) && !mtmp->mundetected) {
	    if((flags.run != 1 && !mtmp->mtame)
					|| (x == u.ux+u.dx && y == u.uy+u.dy))
		goto stop;
	}

	if (levl[x][y].typ == STONE) continue;
	if (x == u.ux-u.dx && y == u.uy-u.dy) continue;

	if (IS_ROCK(levl[x][y].typ) 
		|| (levl[x][y].typ == ROOM) 
		|| (levl[x][y].typ == GRASS) 
		|| (levl[x][y].typ == SOIL) 
		|| (levl[x][y].typ == SAND) 
		|| IS_AIR(levl[x][y].typ)
	)
	    continue;
	else if (closed_door(x,y) ||
		 (mtmp && mtmp->m_ap_type == M_AP_FURNITURE &&
		  (mtmp->mappearance == S_hcdoor ||
		   mtmp->mappearance == S_vcdoor))) {
	    if(x != u.ux && y != u.uy) continue;
	    if(flags.run != 1 && !flags.travel) goto stop;
	    goto bcorr;
	} else if (levl[x][y].typ == CORR) {
bcorr:
	    if(levl[u.ux][u.uy].typ != ROOM
		&& levl[u.ux][u.uy].typ != GRASS
		&& levl[u.ux][u.uy].typ != SOIL
		&& levl[u.ux][u.uy].typ != SAND
		) {
		if(flags.run == 1 || flags.run == 3 || flags.run == 8) {
		    i = dist2(x,y,u.ux+u.dx,u.uy+u.dy);
		    if(i > 2) continue;
		    if(corrct == 1 && dist2(x,y,x0,y0) != 1)
			noturn = 1;
		    if(i < i0) {
			i0 = i;
			x0 = x;
			y0 = y;
			m0 = mtmp ? 1 : 0;
		    }
		}
		corrct++;
	    }
	    continue;
	} else if ((trap = t_at(x,y)) && trap->tseen) {
	    if(flags.run == 1) goto bcorr;	/* if you must */
	    if(x == u.ux+u.dx && y == u.uy+u.dy) goto stop;
	    continue;
	} else if (is_pool(x,y, TRUE) || is_lava(x,y)) {
	    /* water and lava only stop you if directly in front, and stop
	     * you even if you are running
	     */
	    if(!Levitation && !Flying && !is_clinger(youracedata) &&
				x == u.ux+u.dx && y == u.uy+u.dy)
			/* No Wwalking check; otherwise they'd be able
			 * to test boots by trying to SHIFT-direction
			 * into a pool and seeing if the game allowed it
			 */
			goto stop;
	    continue;
	} else {		/* e.g. objects or trap or stairs */
	    if(flags.run == 1) goto bcorr;
	    if(flags.run == 8) continue;
	    if(mtmp) continue;		/* d */
	    if(((x == u.ux - u.dx) && (y != u.uy + u.dy)) ||
	       ((y == u.uy - u.dy) && (x != u.ux + u.dx)))
	       continue;
	}
stop:
	nomul(0, NULL);
	return;
    } /* end for loops */

    if(corrct > 1 && flags.run == 2) goto stop;
    if((flags.run == 1 || flags.run == 3 || flags.run == 8) &&
	!noturn && !m0 && i0 && (corrct == 1 || (corrct == 2 && i0 == 1)))
    {
	/* make sure that we do not turn too far */
	if(i0 == 2) {
	    if(u.dx == y0-u.uy && u.dy == u.ux-x0)
		i = 2;		/* straight turn right */
	    else
		i = -2;		/* straight turn left */
	} else if(u.dx && u.dy) {
	    if((u.dx == u.dy && y0 == u.uy) || (u.dx != u.dy && y0 != u.uy))
		i = -1;		/* half turn left */
	    else
		i = 1;		/* half turn right */
	} else {
	    if((x0-u.ux == y0-u.uy && !u.dy) || (x0-u.ux != y0-u.uy && u.dy))
		i = 1;		/* half turn right */
	    else
		i = -1;		/* half turn left */
	}

	i += u.last_str_turn;
	if(i <= 2 && i >= -2) {
	    u.last_str_turn = i;
	    u.dx = x0-u.ux;
	    u.dy = y0-u.uy;
	}
    }
}

/* something like lookaround, but we are not running */
/* react only to monsters that might hit us */
int
monster_nearby()
{
	register int x,y;
	register struct monst *mtmp;

	/* Also see the similar check in dochugw() in monmove.c */
	for(x = u.ux-1; x <= u.ux+1; x++)
	    for(y = u.uy-1; y <= u.uy+1; y++) {
		if(!isok(x,y)) continue;
		if(x == u.ux && y == u.uy) continue;
		if((mtmp = m_at(x,y)) &&
		   mtmp->m_ap_type != M_AP_FURNITURE &&
		   mtmp->m_ap_type != M_AP_OBJECT &&
		   (!mtmp->mpeaceful || Hallucination) &&
		   (!is_hider(mtmp->data) || !mtmp->mundetected) &&
		   !noattacks(mtmp->data) &&
		   mtmp->mcanmove && !mtmp->msleeping &&  /* aplvax!jcn */
		   !onscary(u.ux, u.uy, mtmp) &&
		   canspotmon(mtmp))
			return(1);
	}
	return(0);
}

void
nomul(nval, txt)
	register int nval;
	const char *txt;
{
	if(multi < nval) return;	/* This is a bug fix by ab@unido */
	u.uinvulnerable = FALSE;	/* Kludge to avoid ctrl-C bug -dlc */
	u.usleep = 0;
	u.puzzle_time = 0;
	if(!flags.forcefight) multi = nval;
	flags.travel = iflags.travel1 = flags.mv = flags.run = 0;
	if (txt && txt[0])
	    (void) strncpy(multi_txt, txt, BUFSZ);
	else
	    (void) memset(multi_txt, 0, BUFSZ);
}

/* called when a non-movement, multi-turn action has completed */
void
unmul(msg_override)
const char *msg_override;
{
	multi = 0;	/* caller will usually have done this already */
	(void) memset(multi_txt, 0, BUFSZ);
	if (msg_override) nomovemsg = msg_override;
	else if (!nomovemsg) nomovemsg = You_can_move_again;
	if (*nomovemsg) pline1(nomovemsg);
	nomovemsg = 0;
	struct obj *puzzle = get_most_complete_puzzle();
	if(u.puzzle_time && puzzle){
		if((monstermoves - u.usleep) >= u.puzzle_time){
			int difficulty = puzzle->ovar1_puzzle_steps + 1;
			difficulty *= 6;
			if(objects[HYPERBOREAN_DIAL].oc_name_known)
				difficulty -= 6;
			difficulty -= Insight;
			difficulty -= ACURR(A_INT);
			if(rnd(20) >= difficulty && !(u.veil && puzzle->ovar1_puzzle_steps >= 5)){
				if(u.uhyperborean_steps < 6){
					if(puzzle->ovar1_puzzle_steps == u.uhyperborean_steps){
						more_experienced(6*pow(10,u.uhyperborean_steps), 0);
						newexplevel();
						u.uhyperborean_steps++;
						if(u.uhyperborean_steps == 6){
							You("have solved the sixth and final ring of %s!", the(xname(puzzle)));
							/*With appologies to "Through the Gates of the Silver Key" and "The Dunwich Horror" by H. P. Lovecraft. */
							pline("The hexagonal pegs are now oddly arranged, seeming to follow the symmetries of some cosmic geometry quite unknown to Earth.");
							pline("You have the faintest sense, as though from a memory within a dream, of strange shapes surmounting hexagonal pillars, and a voice that spoke without speaking.");
							pline("A seal is engraved into your mind!");
							u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
						}
						else {
							You("have solved the next ring of %s!", the(xname(puzzle)));
						}
					}
					else You("have solved the next ring of %s.", the(xname(puzzle)));
					puzzle->ovar1_puzzle_steps++;
				}
			}
			else You("haven't made any headway on the puzzle.");
		}
		else You("awaken before you can make any serious attempt on the puzzle.");
	}
	u.usleep = 0;
	u.puzzle_time = 0;
	if (afternmv) (*afternmv)();
	afternmv = 0;
}

#endif /* OVL2 */
#ifdef OVL1

void
maybe_wail()
{
    static short powers[] = { TELEPORT, SEE_INVIS, POISON_RES, COLD_RES,
			      SHOCK_RES, FIRE_RES, SLEEP_RES, DISINT_RES,
			      TELEPORT_CONTROL, STEALTH, FAST, INVIS };

    if (moves <= wailmsg + 50) return;

    wailmsg = moves;
    if (Role_if(PM_WIZARD) || Race_if(PM_ELF) || Role_if(PM_VALKYRIE)) {
	const char *who;
	int i, powercnt;

	who = (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ?
		urole.name.m : "Elf";
	if (u.uhp == 1) {
	    pline("%s is about to die.", who);
	} else {
	    for (i = 0, powercnt = 0; i < SIZE(powers); ++i)
		if (u.uprops[powers[i]].intrinsic & INTRINSIC) ++powercnt;

	    pline(powercnt >= 4 ? "%s, all your powers will be lost..."
				: "%s, your life force is running out.", who);
	}
    } else {
	You_hear(u.uhp == 1 ? "the wailing of the Banshee..."
			    : "the howling of the CwnAnnwn...");
    }
}

/** Print the amount n of damage inflicted,
 * if the Mother is bound.
 */
void
showdmg(n,you)
int n; /**< amount of damage inflicted */
boolean you; /**< true, if you are hit */
{
	if (u.sealsActive & SEAL_MOTHER && n > 0) {
		if (you)
			pline("[%d pts.]", n);
		else
			pline("(%d pts.)", n);
	}
}

void
losehp(n, knam, k_format)
register int n;
register const char *knam;
boolean k_format;
{
	u.total_damage += n;
	if (Upolyd) {
		u.mh -= n;
		if (u.mhmax < u.mh) u.mh = u.mhmax;
		flags.botl = 1;
		if (u.mh < 1){
		    rehumanize();
			change_gevurah(1); //cheated death.
		}
		else if (n > 0 && u.mh*10 < u.mhmax && Unchanging)
		    maybe_wail();
		return;
	}

	u.uhp -= n;
	if(u.uhp > u.uhpmax) u.uhp = u.uhpmax;	/* perhaps n was negative */
	flags.botl = 1;
	/* the golden knight saves you from dying from hp loss */
	if (uarms && uarms->oartifact == ART_GOLDEN_KNIGHT && u.uhp < 1 && (u.uhp*-2 < u.uen) && !Upolyd)
	{	
		Your("power pours into your shield, and your mortal wounds close!");
		healup(u.uen, 0, FALSE, FALSE); losepw(u.uen);
		change_gevurah(1); //cheated death.
	}
	if(u.uhp < 1) {
		killer_format = k_format;
		killer = knam;		/* the thing that killed you */
		You("die...");
		done(DIED);
	} else if (n > 0 && u.uhp*10 < u.uhpmax) {
		maybe_wail();
	}
}

void
losepw(n)
int n;
{
	u.uen -= n;
	if(u.uen < 0 && !Race_if(PM_INCANTIFIER))
		u.uen = 0;
	flags.botl = 1;
}

boolean
adjacent_mon()
{
	int i, j;
	if(u.ustuck)
		return TRUE;
	for(i = -1; i < 2; i++)
		for(j = -1; j < 2; j++)
			if(i != 0 || j != 0)
				if(isok(u.ux+i, u.uy+j) && m_at(u.ux+i, u.uy+j) && !(m_at(u.ux+i, u.uy+j)->mpeaceful))
					return TRUE;
	return FALSE;
}

void
mdamageu(mtmp, n)	/* mtmp hits you for n points damage */
register struct monst *mtmp;
register int n;
{
	flags.botl = 1;
	//ifdef BARD
	if (n > 0){
		if(!(Nightmare && u.umadness&MAD_RAGE))
			n += mtmp->encouraged;
		if(flags.spriest_level && is_demon(mtmp->data) && is_lawful_mon(mtmp) && !mtmp->mpeaceful)
			n += 9;
		if (uwep && uwep->oartifact == ART_SINGING_SWORD && !mindless_mon(mtmp) && !is_deaf(mtmp)){
			if (uwep->osinging == OSING_DIRGE && !mtmp->mtame){
				n -= uwep->spe + 1;
			}
		}
		if(mtmp->mtame){
			if(uring_art(ART_NARYA))
				n += narya();
		}
		if (n < 0) n = 0;
	}
	//endif
	if (n > 0) mtmp->mhurtu = TRUE;
	if (Upolyd) {
		u.mh -= n;
		if (u.mh < 1){
			rehumanize();
			change_gevurah(1); //cheated death.
		}
	}
	else {
		u.uhp -= n;
		if (u.uhp < 1) done_in_by(mtmp);
	}
}

int
weight_cap()
{
	long carrcap = 0, maxcap = MAX_CARR_CAP;

	carrcap = 25*(ACURRSTR + ACURR(A_CON)) + 50;
	
	struct permonst *mdat = youracedata;
	
	struct obj *cloak = uarmc;
	struct obj *bodyarmor = uarm;
	struct obj *underarmor = uarmu;
	struct obj *boots = uarmf;
	struct obj *belt = ubelt;
	
	/*If mounted your steed is doing the carrying, use its data instead*/
	if(u.usteed && u.usteed->data){
		long mstr = mon_str(u.usteed);
		long mcon = mon_con(u.usteed);
		// carrcap = 25L*(acurrstr((int)(u.usteed->mstr)) + u.usteed->mcon) + 50L;
		cloak = which_armor(u.usteed, W_ARMC);
		bodyarmor = which_armor(u.usteed, W_ARM);
		underarmor = which_armor(u.usteed, W_ARMU);
		boots = which_armor(u.usteed, W_ARMF);
		belt = which_armor(u.usteed, W_BELT);
		
		carrcap = 25L*(mstr + mcon) + 50L;
		mdat = u.usteed->data;
	}
	if (!mdat->cwt)
		maxcap = (maxcap * (long)mdat->msize) / MZ_HUMAN;
	else if (!strongmonst(mdat)
		|| (strongmonst(mdat) && (mdat->cwt > WT_HUMAN)))
		maxcap = (maxcap * (long)mdat->cwt / WT_HUMAN);

	if(Race_if(PM_ORC) && !Upolyd && !u.usteed){
		maxcap = 2*maxcap/3 + u.ulevel*maxcap/30;
	}
	/* consistent with can_carry() in mon.c */
	if (mdat->mlet == S_NYMPH){
		carrcap = max(carrcap, MAX_CARR_CAP);
		maxcap = max(maxcap, MAX_CARR_CAP);
	}
	else if (!mdat->cwt)
		carrcap = (carrcap * (long)mdat->msize) / MZ_HUMAN;
	else if (!strongmonst(mdat)
		|| (strongmonst(mdat) && (mdat->cwt > WT_HUMAN)))
		carrcap = (carrcap * (long)mdat->cwt / WT_HUMAN);
	if (!u.usteed && Race_if(PM_ORC) && !Upolyd){
		carrcap = 2 * carrcap / 3 + u.ulevel*carrcap / 30;
	}

	if (Levitation || Weightless)    /* pugh@cornell */
		carrcap = maxcap;
	if(!Weightless) {
		if (carrcap > maxcap)
			carrcap = maxcap;

		/* these carrcap modifiers only make sense if you have feet on the ground */
		if (belt && belt->otyp == BELT_OF_CARRYING){
			if(belt->blessed)
				carrcap += carrcap/4;
			else if(belt->cursed)
				carrcap -= carrcap/4;
			else
				carrcap += carrcap/8;
		}
		
		if (boots && boots->otyp == find_hboots()) carrcap += maxcap/10;

		if (boots && check_oprop(boots, OPROP_RBRD)
			&& u.ualign.record >= 20 && u.ualign.type != A_CHAOTIC && u.ualign.type != A_NEUTRAL
		)
			carrcap += max(200, maxcap/5);
		
		if (!u.usteed && !Flying) {
			if(EWounded_legs & LEFT_SIDE) carrcap -= 100;
			if(EWounded_legs & RIGHT_SIDE) carrcap -= 100;
		}
		if (carrcap < 0) carrcap = 0;
	}
	if(u.usteed && P_SKILL(P_RIDING) > P_UNSKILLED){
		carrcap += 100 * (P_SKILL(P_RIDING) - P_UNSKILLED);
	}
	
	carrcap += u.ucarinc;
	if(u.sealsActive&SEAL_FAFNIR) carrcap *= 1+((double) u.ulevel)/100;
	if(active_glyph(COMMUNION)) carrcap *= 1.25;
	if(active_glyph(LUMEN)) carrcap *= 1.064;
	if(animaloid(mdat) || naoid(mdat)){
		carrcap *= 1.5;
	}
	if(centauroid(mdat) || mdat->mtyp == PM_BLESSED){
		carrcap *= 1.25;
	}
	if(Race_if(PM_HALF_DRAGON) && flags.HDbreath == AD_FIRE && u.ulevel >= 15){
		carrcap *= 1.2;
	}
	if(arti_lighten(bodyarmor, FALSE)){
		if(bodyarmor->blessed) carrcap *= 1.5;
		else if(!bodyarmor->cursed) carrcap *= 1.25;
		else carrcap *= .75;
	}
	if(arti_lighten(cloak, FALSE)){
		if(cloak->blessed) carrcap *= 1.5;
		else if(!cloak->cursed) carrcap *= 1.25;
		else carrcap *= .75;
	}
	if(arti_lighten(underarmor, FALSE)){
		if(underarmor->blessed) carrcap *= 1.5;
		else if(!underarmor->cursed) carrcap *= 1.25;
		else carrcap *= .75;
	}

	if (carrcap < 1) carrcap = 1;
	
	return((int) carrcap);
}

static int wc;	/* current weight_cap(); valid after call to inv_weight() */

/* returns how far beyond the normal capacity the player is currently. */
/* inv_weight() is negative if the player is below normal capacity. */
int
inv_weight()
{
	struct obj *otmp = invent;
	int wt = 0;
	int objwt;
	boolean nymph = youracedata->mlet == S_NYMPH;
	int wtmod = mlev(&youmonst)*5;

#ifndef GOLDOBJ
	/* when putting stuff into containers, gold is inserted at the head
	   of invent for easier manipulation by askchain & co, but it's also
	   retained in u.ugold in order to keep the status line accurate; we
	   mustn't add its weight in twice under that circumstance */
	objwt = (otmp && otmp->oclass == COIN_CLASS) ? 0 :
		gold_weight(u.ugold);
	if(nymph)
		objwt = max(0, objwt - wtmod);
	wt += objwt;
#endif
	while (otmp) {
		//Correct artifact weights before adding them.  Because that code isn't being run.
		if(otmp->oartifact) otmp->owt = weight(otmp);
#ifndef GOLDOBJ
		if (!is_boulder(otmp) || !(throws_rocks(youracedata) || u.sealsActive&SEAL_YMIR))
#else
		if (otmp->oclass == COIN_CLASS){
			objwt += gold_weight(u.ugold);
			if(nymph)
				objwt = max(0, objwt - wtmod);
			wt += objwt;
		}
		else if (!is_boulder(otmp) || !(throws_rocks(youracedata) || u.sealsActive&SEAL_YMIR))
#endif
		{
			if(otmp->otyp == BELT_OF_WEIGHT && otmp == ubelt)
				wt += 500;
			else {
				objwt = otmp->owt;
				if(nymph)
					objwt = max(0, objwt - wtmod);
				wt += objwt;
			}
		}

		if(otmp->oartifact == ART_IRON_BALL_OF_LEVITATION)
			wt -= 2*otmp->owt;
		if(uarm && uarm == otmp && otmp->otyp == POWER_ARMOR && otmp->lamplit)
			wt -= otmp->owt + 100;
		
		if(u.uleadamulet && (otmp->otyp == AMULET_OF_YENDOR || otmp->otyp == FAKE_AMULET_OF_YENDOR))
			wt += 24*otmp->owt; /* Same as loadstone by default. Only affects fake amulets in open inventory */
		otmp = otmp->nobj;
	}
	
	if(u.usteed){
		nymph = u.usteed->data->mlet == S_NYMPH;
		wtmod = mlev(u.usteed)*5;
		otmp = u.usteed->minvent;
		while (otmp){
			if(otmp->oartifact) otmp->owt = weight(otmp);
			objwt = otmp->owt;
			if(nymph)
				objwt = max(0, objwt - wtmod);
			wt += objwt;
			otmp = otmp->nobj;
		}
	}
	
	wc = weight_cap();
	return (wt - wc);
}

/*
 * Returns 0 if below normal capacity, or the number of "capacity units"
 * over the normal capacity the player is loaded.  Max is 5.
 */
int
calc_capacity(xtra_wt)
int xtra_wt;
{
    int cap, wt = inv_weight() + xtra_wt;

    if (wt <= 0) return UNENCUMBERED;
    if (wc <= 1) return OVERLOADED;
    cap = (wt*2 / wc) + 1;
    return min(cap, OVERLOADED);
}

int
near_capacity()
{
    return calc_capacity(0);
}

int
max_capacity()
{
    int wt = inv_weight();

    return (wt - (2 * wc));
}

boolean
check_capacity(str)
const char *str;
{
    if(near_capacity() >= EXT_ENCUMBER) {
	if(str)
	    pline1(str);
	else
	    You_cant("do that while carrying so much stuff.");
	return 1;
    }
    return 0;
}

#endif /* OVL1 */
#ifdef OVLB

int
inv_cnt()
{
	register struct obj *otmp = invent;
	register int ct = 0;

	while(otmp){
		ct++;
		otmp = otmp->nobj;
	}
	return(ct);
}

#ifdef GOLDOBJ
/* Counts the money in an object chain. */
/* Intended use is for your or some monsters inventory, */
/* now that u.gold/m.gold is gone.*/
/* Counting money in a container might be possible too. */
long
money_cnt(otmp)
struct obj *otmp;
{
        while(otmp) {
	        /* Must change when silver & copper is implemented: */
 	        if (otmp->oclass == COIN_CLASS) return otmp->quan;
  	        otmp = otmp->nobj;
	}
	return 0;
}
#endif
#endif /* OVLB */

/* starts all_items() */
/* initializes all_items to look in where_to_look chains */
struct obj *
start_all_items(where_to_look)
int * where_to_look;
{
	return all_items(TRUE, where_to_look, FALSE);
}

/* gets next object in all_items() */
/* uses foundwhere to return where the returned object was found */
/* -> helpful for contained objects, whose obj->where doesn't reflect minvent/migratingminvent/etc */
struct obj *
next_all_items(foundwhere)
int * foundwhere;
{
	return all_items(FALSE, foundwhere, FALSE);
}

/* prematurely ends all_items */
void
end_all_items()
{
	all_items(FALSE, NULL, TRUE);
	return;
}

/* finds all items, specify where you want them from */
struct obj *
all_items(start, wherefrom, endnow)
boolean start;	/* call with TRUE when first calling this function to start it */
int * wherefrom;	/* bitfield, (1L << obj->where) */
boolean endnow;
{
	static boolean in_progress = FALSE;
	static int curwhere;
	static struct obj * prevobj;
	static struct obj * curobj;
	static struct trap * curtrap;
	static struct monst * curmonst;
	static boolean did_migratingmons;
	static int magic_chest_index;
	static boolean allow_floor;
	static boolean allow_contained;
	static boolean allow_invent;
	static boolean allow_minvent;
	static boolean allow_migrating;
	static boolean allow_buried;
	static boolean allow_magic;
	static boolean allow_intrap;

	if (endnow) {
		in_progress = FALSE;
		return (struct obj *)0;
	}

	if (start == in_progress) {
		if (start)
			impossible("all_items() called with start=TRUE while in progress!");
		else
			impossible("all_items() called with start=FALSE while not started!");
		return (struct obj *)0;
	}

	if (start) {
		/* fresh start */
		in_progress = TRUE;
		curwhere = OBJ_FREE;
		prevobj = (struct obj *)0;
		curobj = (struct obj *)0;
		curtrap = (struct trap *)0;
		curmonst = (struct monst *)0;
		did_migratingmons = FALSE;
		magic_chest_index = 0;
		allow_floor     = !wherefrom || !!((*wherefrom)&(1 << OBJ_FLOOR));
		allow_contained = !wherefrom || !!((*wherefrom)&(1 << OBJ_CONTAINED));
		allow_invent    = !wherefrom || !!((*wherefrom)&(1 << OBJ_INVENT));
		allow_minvent   = !wherefrom || !!((*wherefrom)&(1 << OBJ_MINVENT));
		allow_migrating = !wherefrom || !!((*wherefrom)&(1 << OBJ_MIGRATING));
		allow_buried    = !wherefrom || !!((*wherefrom)&(1 << OBJ_BURIED));
		allow_magic     = !wherefrom || !!((*wherefrom)&(1 << OBJ_MAGIC_CHEST));
		allow_intrap    = !wherefrom || !!((*wherefrom)&(1 << OBJ_INTRAP));
	}

	do {
		if (!curobj) {
			/* get the start of a new chain */
			if (curwhere == OBJ_FREE) {
				/* we literally cannot search OBJ_FREE */
			}
			else if (curwhere == OBJ_FLOOR && allow_floor) {
				curobj = fobj;
				if (wherefrom) *wherefrom = (1 << OBJ_FLOOR);
			}
			else if (curwhere == OBJ_INVENT && allow_invent) {
				curobj = invent;
				if (wherefrom) *wherefrom = (1 << OBJ_INVENT);
			}
			else if (curwhere == OBJ_MINVENT && allow_minvent) {
				do {
					if (!curmonst) {
						curmonst = fmon;
					}
					else
						curmonst = curmonst->nmon;
					if (curmonst)
						curobj = curmonst->minvent;
				} while (curmonst && !curobj);
				if (wherefrom) *wherefrom = (1 << OBJ_MINVENT);
			}
			else if (curwhere == OBJ_MIGRATING && allow_minvent && allow_migrating && !did_migratingmons) {
				do {
					if (!curmonst) {
						curmonst = migrating_mons;
					}
					else
						curmonst = curmonst->nmon;
					if (curmonst)
						curobj = curmonst->minvent;
				} while (curmonst && !curobj);
				if (wherefrom) *wherefrom = (1 << OBJ_MINVENT) | (1 << OBJ_MIGRATING);
			}
			else if (curwhere == OBJ_MIGRATING && allow_migrating) {
				curobj = migrating_objs;
				if (wherefrom) *wherefrom = (1 << OBJ_MIGRATING);
			}
			else if (curwhere == OBJ_BURIED && allow_buried) {
				curobj = level.buriedobjlist;
				if (wherefrom) *wherefrom = (1 << OBJ_BURIED);
			}
			else if (curwhere == OBJ_ONBILL) {
				/* we NEVER look on bills */
			}
			else if (curwhere == OBJ_MAGIC_CHEST && allow_magic) {
				do {
					curobj = magic_chest_objs[magic_chest_index];
					magic_chest_index++;
				} while (!curobj && magic_chest_index < 10);
				if (wherefrom) *wherefrom = (1 << OBJ_MAGIC_CHEST);
			}
			else if (curwhere == OBJ_INTRAP && allow_intrap) {
				do {
					if (!curtrap) {
						curtrap = ftrap;
					}
					else
						curtrap = curtrap->ntrap;
					if (curtrap)
						curobj = curtrap->ammo;
				} while (curtrap && !curobj);
				*wherefrom = (1 << OBJ_INTRAP);
			}
		}
		else {
			prevobj = curobj;
			curobj = curobj->nobj;
		}

		/* contained items handling */
		if (allow_contained) {
			if (curobj) {
				/* drop to the bottom of the contained-obj stack */
				while (curobj->cobj)
					curobj = curobj->cobj;
			}
			else if (prevobj && prevobj->where == OBJ_CONTAINED) {
				/* if we reached the end of a contained chain, bump up to container */
				curobj = prevobj->ocontainer;
			}
		}

		/* for the "where"s that can have many chains, we need to stay in that state until all their chains are exhausted */
		if (!curobj && (
			(curwhere == OBJ_MINVENT && allow_minvent && curmonst) ||
			(curwhere == OBJ_MIGRATING && allow_minvent && allow_migrating && curmonst && !did_migratingmons) ||
			(curwhere == OBJ_MIGRATING && allow_migrating && !curmonst && !did_migratingmons && (did_migratingmons=TRUE)) ||	/* set did_migratingmons to then do migrating objs */
			(curwhere == OBJ_MAGIC_CHEST && allow_magic && (magic_chest_index < 10)) ||
			(curwhere == OBJ_INTRAP && allow_intrap && curtrap)
			)) {
			curwhere--;	/* predecrement curwhere to stay in current state */
		}

	} while (!curobj && curwhere++ < NOBJ_STATES);

	if (!curobj) {
		/* finally done */
		in_progress = FALSE;
	}

	return curobj;
}


/*hack.c*/

/*	SCCS Id: @(#)polyself.c	3.4	2003/01/08	*/
/*	Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Polymorph self routine.
 *
 * Note:  the light source handling code assumes that both youmonst.m_id
 * and youmonst.mx will always remain 0 when it handles the case of the
 * player polymorphed into a light-emitting monster.
 */

#include "hack.h"

#ifdef OVLB
STATIC_DCL void FDECL(polyman, (const char *,const char *));
STATIC_DCL void NDECL(uunstick);
STATIC_DCL int FDECL(armor_to_dragon,(int));
STATIC_DCL void NDECL(newman);
STATIC_DCL short NDECL(doclockmenu);
STATIC_DCL short NDECL(dodroidmenu);
STATIC_DCL void FDECL(worddescriptions, (int));

/* assumes u.umonnum is set already */
void
init_uasmon()
{
	upermonst = mons[(flags.female && urace.femalenum != NON_PM) ? urace.femalenum : urace.malenum];
	set_uasmon();
}

/* update the youmonst.data structure pointer */
void
set_uasmon()
{
	set_mon_data(&youmonst, maybe_polyd(u.umonnum, ((flags.female && urace.femalenum != NON_PM) ? urace.femalenum : urace.malenum)));
}

/** Returns true if the player monster is genocided. */
boolean
is_playermon_genocided()
{
	return ((mvitals[urole.malenum].mvflags & G_GENOD && !In_quest(&u.uz)) ||
			(urole.femalenum != NON_PM &&
			(mvitals[urole.femalenum].mvflags & G_GENOD && !In_quest(&u.uz))) ||
			(mvitals[urace.malenum].mvflags & G_GENOD && !In_quest(&u.uz)) ||
			(urace.femalenum != NON_PM &&
			(mvitals[urace.femalenum].mvflags & G_GENOD && !In_quest(&u.uz))));
}

/* make a (new) human out of the player */
STATIC_OVL void
polyman(fmt, arg)
const char *fmt, *arg;
{
	boolean sticky = sticks(&youmonst) && u.ustuck && !u.uswallow,
		was_mimicking = (youmonst.m_ap_type == M_AP_OBJECT);
	boolean could_pass_walls = Passes_walls;
	boolean was_blind = !!Blind;
	int starting_hungermax;
	double starting_hungersizemod;

	starting_hungermax = get_uhungermax();
	starting_hungersizemod = get_uhungersizemod();
	
	if (Upolyd) {
		u.acurr = u.macurr;	/* restore old attribs */
		u.amax = u.mamax;
		u.umonnum = u.umonster;
		flags.female = u.mfemale;
	}
	set_uasmon();

	u.mh = u.mhmax = 0;
	u.mtimedone = 0;
	skinback(FALSE);
	u.uundetected = 0;

	if (sticky) uunstick();
	find_ac();
	if (was_mimicking) {
	    if (multi < 0) unmul("");
	    youmonst.m_ap_type = M_AP_NOTHING;
	}

	newsym(u.ux,u.uy);

	You(fmt, arg);
	/* check whether player foolishly genocided self while poly'd */
	if (is_playermon_genocided()) {
	    /* intervening activity might have clobbered genocide info */
	    killer = delayed_killer;
	    if (!killer || !strstri(killer, "genocid")) {
		killer_format = KILLED_BY;
		killer = "self-genocide";
	    }
	    done(GENOCIDED);
	}

	if (u.twoweap && !could_twoweap(youmonst.data))
	    untwoweapon();

	if (u.utraptype == TT_PIT) {
	    if (could_pass_walls) {	/* player forms cannot pass walls */
		u.utrap = rn1(6,2);
	    }
	}
	if (was_blind && !Blind) {	/* reverting from eyeless */
	    Blinded = 1L;
	    make_blinded(0L, TRUE);	/* remove blindness */
	}

	if(!Levitation && !u.ustuck &&
	   (is_pool(u.ux,u.uy, TRUE) || is_lava(u.ux,u.uy)))
		spoteffects(TRUE);

	int new_hungermax = get_uhungermax();
	if(starting_hungermax != new_hungermax){
		u.uhunger = u.uhunger * new_hungermax/starting_hungermax;
	}
	newuhs(get_uhungersizemod() < starting_hungersizemod); //May result in a message for gnomes, as the starvation thresholds will move.

	see_monsters();
}

void
change_sex()
{
	/* setting u.umonster for caveman/cavewoman or priest/priestess
	   swap unintentionally makes `Upolyd' appear to be true */
	boolean already_polyd = (boolean) Upolyd;

	/* Some monsters are always of one sex and their sex can't be changed */
	/* succubi/incubi can change, but are handled below */
	/* !already_polyd check necessary because is_male() and is_female()
           are true if the player is a priest/priestess */
	if (!is_male(youracedata) && !is_female(youracedata) && !is_neuter(youracedata))
	    flags.female = !flags.female;
	if (already_polyd)	/* poly'd: also change saved sex */
	    u.mfemale = !u.mfemale;
	max_rank_sz();		/* [this appears to be superfluous] */
	if ((already_polyd ? u.mfemale : flags.female) && urole.name.f)
	    Strcpy(pl_character, urole.name.f);
	else
	    Strcpy(pl_character, urole.name.m);
	u.umonster = ((already_polyd ? u.mfemale : flags.female) && urole.femalenum != NON_PM) ?
			urole.femalenum : urole.malenum;
	if (!already_polyd) {
	    u.umonnum = u.umonster;
	} else if (u.umonnum == PM_SUCCUBUS || u.umonnum == PM_INCUBUS) {
	    flags.female = !flags.female;
	    /* change monster type to match new sex */
	    u.umonnum = (u.umonnum == PM_SUCCUBUS) ? PM_INCUBUS : PM_SUCCUBUS;
	    set_uasmon();
	}
}

STATIC_OVL void
newman()
{
	int tmp, tmpen, oldlvl;

	tmp = u.uhpmax;
	tmpen = u.uenmax;
	oldlvl = u.ulevel;
	if(!Role_if(PM_ANACHRONOUNBINDER)) u.ulevel = u.ulevel + rn1(5, -2);
	if (u.ulevel > 127 || u.ulevel < 1) { /* level went below 0? */
	    u.ulevel = oldlvl; /* restore old level in case they lifesave */
	    goto dead;
	}
	if (u.ulevel > MAXULEV) u.ulevel = MAXULEV;
	/* If your level goes down, your peak level goes down by
	   the same amount so that you can't simply use blessed
	   full healing to undo the decrease.  But if your level
	   goes up, your peak level does *not* undergo the same
	   adjustment; you might end up losing out on the chance
	   to regain some levels previously lost to other causes. */
	if (u.ulevel < oldlvl) u.ulevelmax -= (oldlvl - u.ulevel);
	if (u.ulevelmax < u.ulevel) u.ulevelmax = u.ulevel;

	if (!rn2(10)) change_sex();

	adjabil(oldlvl, (int)u.ulevel);
	reset_rndmonst(NON_PM);	/* new monster generation criteria */

	/* random experience points for the new experience level */
	u.uexp = rndexp(FALSE);

	/* 
	 * Kludge up a reroll of all dice
	 */
	u.uhprolled = d(2*u.ulevel, max(u.uhprolled/u.ulevel, 1));
	u.uenrolled = d(2*u.ulevel, max(u.uenrolled/u.ulevel, 1));

	redist_attr();

	calc_total_maxhp();
	calc_total_maxen();

	tmp = u.uhpmax - tmp; //Note: final - initial
	u.uhp = u.uhp + tmp;
	if(u.uhp < 1) u.uhp = 1;

	tmpen = u.uenmax - tmpen; //Note: final - initial
	u.uen = u.uen + tmpen;
	if(u.uen < 1) u.uen = 1;

	if(Race_if(PM_INCANTIFIER)) u.uen = min(u.uenmax, rn1(500,500));
	else u.uhunger = rn1(500,500) * get_uhungersizemod();
	if (Sick) make_sick(0L, (char *) 0, FALSE, SICK_ALL);
	Stoned = 0;
	Golded = 0;
	Salted = 0;
	delayed_killer = 0;
	if (u.uhp <= 0 || u.uhpmax <= 0) {
		if (Polymorph_control) {
		    if (u.uhp <= 0) u.uhp = 1;
		} else {
dead: /* we come directly here if their experience level went to 0 or less */
		    Your("new form doesn't seem healthy enough to survive.");
		    killer_format = KILLED_BY_AN;
		    killer="unsuccessful polymorph";
		    done(DIED);
		    newuhs(FALSE);
		    return; /* lifesaved */
		}
	}
	newuhs(FALSE);
	polyman("feel like a new %s!",
		(flags.female && urace.individual.f) ? urace.individual.f :
		(urace.individual.m) ? urace.individual.m : urace.noun);
	if (Slimed) {
		Your("body transforms, but there is still slime on you.");
		Slimed = 10L;
	}
	flags.botl = 1;
	see_monsters();
	(void) encumber_msg();
}

void
polyself(forcecontrol)
boolean forcecontrol;
{
	char buf[BUFSZ];
	int old_light, new_light;
	int mntmp = NON_PM;
	int tries=0;
	boolean draconian = (uarm &&
				uarm->otyp >= GRAY_DRAGON_SCALE_MAIL &&
				uarm->otyp <= YELLOW_DRAGON_SCALES);
	boolean leonine = (uarmc && uarmc->otyp == LEO_NEMAEUS_HIDE);
	boolean iswere = (u.ulycn >= LOW_PM || is_were(youmonst.data));
	boolean isvamp = (is_vampire(youracedata));
	boolean hasmask = (ublindf && ublindf->otyp==MASK && ublindf->corpsenm != NON_PM && polyok(&mons[ublindf->corpsenm]));
	boolean was_floating = (Levitation || Flying);
	boolean allow_selfrace_poly = (wizard || (u.specialSealsActive&SEAL_ALIGNMENT_THING));
	boolean allow_nopoly_poly = FALSE;

	if(!Polymorph_control && !forcecontrol && !draconian && !iswere && !isvamp && !hasmask && !uskin) {
	    if (rn2(20) > ACURR(A_CON)) {
		You("%s", shudder_for_moment);
		losehp(rnd(30), "system shock", KILLED_BY_AN);
		exercise(A_CON, FALSE);
		return;
	    }
	}
	old_light = Upolyd ? emits_light(youmonst.data) : 0;

	if (Polymorph_control || forcecontrol) {
		do {
			getlin("Become what kind of monster? [type the name]",
				buf);
			mntmp = name_to_mon(buf);
			if (mntmp < LOW_PM)
				pline("I've never heard of such monsters.");
			else if (mntmp == (flags.female ? urace.femalenum : urace.malenum)) {
				/* newman() */
				newman();
				goto made_change;
			}
			/* Note:  humans are illegal as monsters, but an
			 * illegal monster forces newman(), which is what we
			 * want if they specified a human.... */
			else if ((!polyok(&mons[mntmp]) && (!(allow_nopoly_poly = (wizard && yn("Poly into forbidden form") == 'y')))) ||
				(your_race(&mons[mntmp]) && !allow_selfrace_poly))
				You("cannot polymorph into that.");
			else break;
		} while(++tries < 5);
		if (tries==5) pline("%s", thats_enough_tries);
		/* allow skin merging, even when polymorph is controlled */
		if (draconian &&
		    (mntmp == armor_to_dragon(uarm->otyp) || tries == 5))
		    goto do_merge;
		if (leonine &&
		    (mntmp == PM_SON_OF_TYPHON || tries == 5))
		    goto do_lion_merge;
	} else if (uskin) {
		newman();
		goto made_change;
	} else if (draconian || leonine || iswere || hasmask || isvamp) {
		/* special changes that don't require polyok() */
		if (draconian) {
		    do_merge:
			mntmp = armor_to_dragon(uarm->otyp);
			if (!(mvitals[mntmp].mvflags & G_GENOD && !In_quest(&u.uz))) {
				/* allow G_EXTINCT */
				You("merge with your scaly armor.");
				uskin = uarm;
				uarm = (struct obj *)0;
				/* save/restore hack */
				uskin->owornmask |= W_SKIN;
			}
		}
		else if(leonine) {
			do_lion_merge:
			mntmp = PM_SON_OF_TYPHON;
			if (!(mvitals[mntmp].mvflags & G_GENOD && !In_quest(&u.uz))) {
				/* allow G_EXTINCT */
				You("merge with lion skin cloak.");
				uskin = uarmc;
				uarmc = (struct obj *)0;
				/* save/restore hack */
				uskin->owornmask |= W_SKIN;
			}
		} else if (hasmask) {
			if ((youmonst.data) == &mons[ublindf->corpsenm])
				mntmp = PM_HUMAN; /* Illegal; force newman() */
			else
				mntmp = ublindf->corpsenm;
		} else if (iswere) {
			if (is_were(youmonst.data))
				mntmp = PM_HUMAN; /* Illegal; force newman() */
			else
				mntmp = u.ulycn;
		} else if (isvamp) {
			if (u.umonnum != PM_VAMPIRE_BAT)
				mntmp = PM_VAMPIRE_BAT;
			else
				mntmp = PM_HUMAN; /* newman() */
		}
		/* if polymon fails, "you feel" message has been given
		   so don't follow up with another polymon or newman */
		if (mntmp == PM_HUMAN) newman();	/* werecritter */
		else (void) polymon(mntmp);
		goto made_change;    /* maybe not, but this is right anyway */
	}

	if (mntmp < LOW_PM) {
		tries = 0;
		do {
			/* randomly pick an "ordinary" monster */
			mntmp = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
		} while((!polyok(&mons[mntmp]) || is_placeholder(&mons[mntmp]))
				&& tries++ < 200);
	}

	/* The below polyok() fails either if everything is genocided, or if
	 * we deliberately chose something illegal to force newman().
	 */
	if ((!allow_nopoly_poly   && !polyok(&mons[mntmp])) ||
		(!allow_selfrace_poly && !rn2(5)) ||
		(!allow_selfrace_poly && your_race(&mons[mntmp]))
	) newman();
	else if(!polymon(mntmp)) return;

	if (!uarmg) selftouch("No longer petrification-resistant, you");

 made_change:
	new_light = Upolyd ? emits_light(youmonst.data) : 0;
	if (old_light != new_light) {
	    del_light_source((&youmonst)->light);
	    if (new_light == 1) ++new_light;  /* otherwise it's undetectable */
	    if (new_light)
			new_light_source(LS_MONSTER, (genericptr_t)&youmonst, new_light);
	}
	if (is_pool(u.ux,u.uy, FALSE) && was_floating && !(Levitation || Flying) &&
		!breathless(youmonst.data) && !amphibious(youmonst.data) &&
		!Swimming) drown();
}

/* (try to) make a mntmp monster out of the player */
int
polymon(mntmp)	/* returns 1 if polymorph successful */
int	mntmp;
{
	boolean sticky = sticks(&youmonst) && u.ustuck && !u.uswallow,
		was_blind = !!Blind, dochange = FALSE;
	boolean could_pass_walls = Passes_walls;
	int mlvl;
	int starting_hungermax;
	double starting_hungersizemod;
	const char *s;

	if (mvitals[mntmp].mvflags & G_GENOD && !In_quest(&u.uz)) {	/* allow G_EXTINCT */
		You_feel("rather %s-ish.",mons[mntmp].mname);
		exercise(A_WIS, TRUE);
		return(0);
	}

	/* KMH, conduct */
	u.uconduct.polyselfs++;
	
	starting_hungermax = get_uhungermax();
	starting_hungersizemod = get_uhungersizemod();
	
	if (!Upolyd) {
		/* Human to monster; save human stats */
		u.macurr = u.acurr;
		u.mamax = u.amax;
		u.mfemale = flags.female;
	} else {
		/* Monster to monster; restore human stats, to be
		 * immediately changed to provide stats for the new monster
		 */
		u.acurr = u.macurr;
		u.amax = u.mamax;
		flags.female = u.mfemale;
	}

	if (youmonst.m_ap_type) {
	    /* stop mimicking immediately */
	    if (multi < 0) unmul("");
	} else if (mons[mntmp].mlet != S_MIMIC) {
	    /* as in polyman() */
	    youmonst.m_ap_type = M_AP_NOTHING;
	}
	if (is_male(&mons[mntmp])) {
		if(flags.female) dochange = TRUE;
	} else if (is_female(&mons[mntmp])) {
		if(!flags.female) dochange = TRUE;
	} else if (!is_neuter(&mons[mntmp]) && mntmp != u.ulycn) {
		if(!rn2(10)) dochange = TRUE;
	}
	if (dochange) {
		flags.female = !flags.female;
		You("%s%s %s%s!",
		    (u.umonnum != mntmp) ? "turn into" : "feel like a new",
			(!type_is_pname(&mons[mntmp])) ? " a" : "",
		    (is_male(&mons[mntmp]) || is_female(&mons[mntmp])) ? "" :
			flags.female ? "female " : "male ",
		    mons[mntmp].mname);
	} else {
		if (u.umonnum != mntmp) {
			if (type_is_pname(&mons[mntmp]))
				You("turn into %s!", mons[mntmp].mname);
			else
				You("turn into %s!", an(mons[mntmp].mname));
		}
		else
			You_feel("like a new %s!", mons[mntmp].mname);
	}
	if (Stoned && poly_when_stoned(&mons[mntmp])) {
		/* poly_when_stoned already checked stone golem genocide */
		You("turn to stone!");
		mntmp = PM_STONE_GOLEM;
		Stoned = 0;
		delayed_killer = 0;
	}
	if (Golded && poly_when_golded(&mons[mntmp])) {
		/* poly_when_golded already checked gold golem genocide */
		You("turn to gold!");
		mntmp = PM_GOLD_GOLEM;
		Golded = 0;
		delayed_killer = 0;
	}
	if (uarmc && (s = OBJ_DESCR(objects[uarmc->otyp])) != (char *)0 &&
	   !strcmp(s, "opera cloak") &&
	   is_vampire(youracedata)) {
		ABON(A_CHA) -= 1;
		flags.botl = 1;
	}

	u.mtimedone = rn1(500, 500);
	u.umonnum = mntmp;
	set_uasmon();

	/* New stats for monster, to last only as long as polymorphed.
	 * Currently only strength gets changed.
	 */
	if(strongmonst(&mons[mntmp])) ABASE(A_STR) = AMAX(A_STR) = STR18(100);

	if (uarmc && (s = OBJ_DESCR(objects[uarmc->otyp])) != (char *)0 &&
	   !strcmp(s, "opera cloak") &&
	   is_vampire(youracedata)) {
		You("%s very impressive in your %s.", Blind ||
				(Invis && !See_invisible(u.ux,u.uy)) ? "feel" : "look",
				OBJ_DESCR(objects[uarmc->otyp]));
		ABON(A_CHA) += 1;
		flags.botl = 1;
	}

	if (Stone_resistance && Stoned) { /* parnes@eniac.seas.upenn.edu */
		Stoned = 0;
		delayed_killer = 0;
		You("no longer seem to be petrifying.");
	}
	if (Stone_resistance && Golded) {
		Golded = 0;
		delayed_killer = 0;
		You("no longer seem to be turning to gold.");
	}
	if (Stone_resistance && Salted) {
		Salted = 0;
		delayed_killer = 0;
		You("no longer seem to be turning to salt.");
	}
	if (Sick_resistance && Sick) {
		make_sick(0L, (char *) 0, FALSE, SICK_ALL);
		You("no longer feel sick.");
	}
	if (Slimed) {
	    if (flaming(youmonst.data)) {
		pline_The("slime burns away!");
		Slimed = 0L;
		flags.botl = 1;
	    } else if (mntmp == PM_GREEN_SLIME) {
		/* do it silently */
		Slimed = 0L;
		flags.botl = 1;
	    }
	}
	if(FrozenAir){
	    if (flaming(youmonst.data)) {
			pline_The("frozen air vaporizes!");
			FrozenAir = 0L;
			flags.botl = 1;
		}
	}
	
	if (nohands(youmonst.data) || nolimbs(youmonst.data)) Glib = 0;

	/* Combine both current level & monster base level when factoring initial polyform hp,
	 * this is to allow for a 'correct' hp value for all players at lvl X and polyform Y to have,
	 * regardless of when they polymorphed into that monster (since players gain polyform hp
	 * from leveling up, this needs to be factored in).
	 */
	mlvl = (int)mons[mntmp].mlevel + (int)u.ulevel;
	int hds = hd_size(&mons[mntmp]);

	if (youmonst.data->mlet == S_DRAGON && mntmp >= PM_GRAY_DRAGON) {
		u.mhrolled = In_endgame(&u.uz) ? (hds*mlvl) : ((hds/2)*mlvl + d(mlvl,(hds/2)));
	} else if (is_golem(youmonst.data)) {
		u.mhrolled = golemhp(mntmp);
	} else {
		if (!mlvl) u.mhrolled = rnd(hds/2);
		else u.mhrolled = d(mlvl, hds);
		if (is_home_elemental(&mons[mntmp])) u.mhrolled *= 3;
	}
	calc_total_maxhp();
	u.mh = u.mhmax;

	if (u.ulevel < mlvl) {
	/* Low level characters can't become high level monsters for long */
#ifdef DUMB
		/* DRS/NS 2.2.6 messes up -- Peter Kendell */
		int mtd = u.mtimedone, ulv = u.ulevel;

		u.mtimedone = mtd * ulv / mlvl;
#else
		u.mtimedone = u.mtimedone * u.ulevel / mlvl;
#endif
	}

	if (uskin && mntmp != armor_to_dragon(uskin->otyp))
		skinback(FALSE);
	break_armor();
	drop_weapon(1);
	if (hides_under(youmonst.data))
		u.uundetected = OBJ_AT(u.ux, u.uy);
	else if (is_underswimmer(youmonst.data))
		u.uundetected = is_pool(u.ux, u.uy, FALSE);
	else
		u.uundetected = 0;

	if (u.utraptype == TT_PIT) {
	    if (could_pass_walls && !Passes_walls) {
		u.utrap = rn1(6,2);
	    } else if (!could_pass_walls && Passes_walls) {
		u.utrap = 0;
	    }
	}
	if (was_blind && !Blind) {	/* previous form was eyeless */
	    Blinded = 1L;
	    make_blinded(0L, TRUE);	/* remove blindness */
	}
	newsym(u.ux,u.uy);		/* Change symbol */

	if (!sticky && !u.uswallow && u.ustuck && sticks(&youmonst)) u.ustuck = 0;
	else if (sticky && !sticks(&youmonst)) uunstick();
#ifdef STEED
	if (u.usteed) {
	    if (touch_petrifies(u.usteed->data) &&
	    		!Stone_resistance && rnl(100) >= 33) {
	    	char buf[BUFSZ];

	    	pline("No longer petrifying-resistant, you touch %s.",
	    			mon_nam(u.usteed));
	    	Sprintf(buf, "riding %s", an(u.usteed->data->mname));
	    	instapetrify(buf);
 	    }
	    if (!can_ride(u.usteed)) dismount_steed(DISMOUNT_POLY);
	}
#endif

	if (flags.verbose) {
	    static const char use_thec[] = "Use the command #%s to %s.";
	    static const char monsterc[] = "monster";
#ifdef YOUMONST_SPELL
	    if (attacktype(youmonst.data, AT_MAGC))
		pline(use_thec,monsterc,"cast monster spells");
#endif /* YOUMONST_SPELL */
	    if (is_drow(youmonst.data))
		pline(use_thec,monsterc,"invoke darkness");
	    if (uclockwork)
		pline(use_thec,monsterc,"adjust your clockspeed");
	    if (uandroid)
		pline(use_thec,monsterc,"use your abilities");
	    if (can_breathe(youmonst.data))
		pline(use_thec,monsterc,"use your breath weapon");
	    if (attacktype(youmonst.data, AT_SPIT))
		pline(use_thec,monsterc,"spit venom");
	    if (youmonst.data->mlet == S_NYMPH)
		pline(use_thec,monsterc,"remove an iron ball");
	    if (attacktype(youmonst.data, AT_GAZE))
		pline(use_thec,monsterc,"gaze at monsters");
	    if (is_hider(youmonst.data))
		pline(use_thec,monsterc,"hide");
	    if (is_were(youmonst.data) || gates_in_help(youmonst.data))
		pline(use_thec,monsterc,"summon help");
	    if (webmaker(youmonst.data))
		pline(use_thec,monsterc,"spin a web");
	    if (u.umonnum == PM_GREMLIN)
		pline(use_thec,monsterc,"multiply in a fountain");
	    if (is_unicorn(youmonst.data) || youmonst.data->mtyp == PM_KI_RIN)
		pline(use_thec,monsterc,"use your horn");
	    if (is_mind_flayer(youmonst.data) || Role_if(PM_MADMAN))
		pline(use_thec,monsterc,"emit a mental blast");
	    if (youmonst.data->msound == MS_SHRIEK || youmonst.data->msound == MS_SHOG) /* worthless, actually */
		pline(use_thec,monsterc,"shriek");
	    if (youmonst.data->msound == MS_JUBJUB)
		pline(use_thec,monsterc,"scream");
	    if (youmonst.data->mtyp == PM_TOVE)
		pline(use_thec,monsterc,"gimble a hole in the ground");
		if (attacktype(youracedata, AT_LNCK) || attacktype(youracedata, AT_LRCH))
		pline(use_thec,monsterc,"attack a distant target");
	    if (lays_eggs(youmonst.data) && flags.female)
		pline(use_thec,"sit","lay an egg");
	}
	/* you now know what an egg of your type looks like */
	if (lays_eggs(youmonst.data)) {
	    learn_egg_type(u.umonnum);
	    /* make queen bees recognize killer bee eggs */
	    learn_egg_type(egg_type_from_parent(u.umonnum, TRUE));
	}
	find_ac();
	if((!Levitation && !u.ustuck && !Flying &&
	    (is_pool(u.ux,u.uy, TRUE) || is_lava(u.ux,u.uy))) ||
	   (Underwater && !Swimming))
	    spoteffects(TRUE);
	if (Passes_walls && u.utrap && u.utraptype == TT_INFLOOR) {
	    u.utrap = 0;
	    pline_The("rock seems to no longer trap you.");
	} else if (likes_lava(youmonst.data) && u.utrap && u.utraptype == TT_LAVA) {
	    u.utrap = 0;
	    pline_The("lava now feels soothing.");
	}
	if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
	    if (Punished) {
		You("slip out of the iron chain.");
		unpunish();
	    }
	}
	if (u.utrap && (u.utraptype == TT_WEB || u.utraptype == TT_SALIVA || u.utraptype == TT_BEARTRAP || u.utraptype == TT_FLESH_HOOK) &&
		(amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data) ||
		  (youmonst.data->msize <= MZ_SMALL && u.utraptype == TT_BEARTRAP))) {
	    You("are no longer stuck in the %s.",
		    u.utraptype == TT_WEB ? "web" : u.utraptype == TT_SALIVA ? "gooey saliva" :u.utraptype == TT_FLESH_HOOK ? "flesh hook" : "bear trap");
	    /* probably should burn webs too if PM_FIRE_ELEMENTAL */
	    u.utrap = 0;
	}
	if (webmaker(youmonst.data) && u.utrap && u.utraptype == TT_WEB) {
	    You("orient yourself on the web.");
	    u.utrap = 0;
	}
	int new_hungermax = get_uhungermax();
	if(starting_hungermax != new_hungermax){
		u.uhunger = u.uhunger * new_hungermax/starting_hungermax;
	}
	newuhs(get_uhungersizemod() < starting_hungersizemod); //May result in a message for gnomes, as the starvation thresholds will move.
	flags.botl = 1;
	vision_full_recalc = 1;
	see_monsters();
	exercise(A_CON, FALSE);
	exercise(A_WIS, TRUE);
	(void) encumber_msg();
	return(1);
}

void
break_armor()
{
    register struct obj *otmp;
#define special_armor(a) (a->oartifact || is_imperial_elven_armor(a))
	if ((otmp = uarm) != 0) {
		if(!arm_size_fits(youracedata,otmp) || !arm_match(youracedata,otmp) || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)){
			if (donning(otmp)) cancel_don();
			if(special_armor(otmp) || otmp->objsize > youracedata->msize || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)){
				Your("armor falls around you!");
				(void) Armor_gone();
				dropx(otmp);
			} else {
				You("break out of your armor!");
				exercise(A_STR, FALSE);
				(void) Armor_gone();
				useup(otmp);
			}
		}
	}
	if ((otmp = uarmc) != 0) {
		if(abs(otmp->objsize - youracedata->msize) > 1
				 || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)
		){
			if (donning(otmp)) cancel_don();
			if(special_armor(otmp) || otmp->objsize > youracedata->msize || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)) {
				Your("%s falls off!", cloak_simple_name(otmp));
				(void) Cloak_off();
				dropx(otmp);
			} else {
				Your("%s tears apart!", cloak_simple_name(otmp));
				(void) Cloak_off();
				useup(otmp);
			}
		}
	}
	if ((otmp = uarmu) != 0) {
		if(otmp->objsize != youracedata->msize
				|| !shirt_match(youracedata,otmp) || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)
		){
			if (donning(otmp)) cancel_don();
			if(special_armor(otmp) || otmp->objsize > youracedata->msize || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)) {
				Your("shirt falls off!");
				(void) Shirt_off();
		// setworn((struct obj *)0, otmp->owornmask & W_ARMU);
				dropx(otmp);
			} else {
				Your("shirt rips to shreds!");
				(void) Shirt_off();
				useup(otmp);
			}
		}
    }
	if ((otmp = uarmh) != 0){
		boolean hat = is_hat(otmp);
		if((!helm_match(youracedata, uarmh) && !hat)
			|| (!has_head_mon(&youmonst) && !hat)
			|| !helm_size_fits(youracedata, uarmh)
			|| (has_horns(youracedata) && !(otmp->otyp == find_gcirclet() || is_flimsy(otmp)))
			|| is_gaseous_noequip(youracedata)
			|| noncorporeal(youracedata)
		) {
			if (donning(otmp)) cancel_don();
			Your("helmet falls to the %s!", surface(u.ux, u.uy));
			(void) Helmet_off();
			dropx(otmp);
	    } else if (is_flimsy(otmp) && !donning(otmp) && has_horns(youracedata)) {
			char hornbuf[BUFSZ], yourbuf[BUFSZ];
			/* Future possiblities: This could damage/destroy helmet */
			Sprintf(hornbuf, "horn%s", plur(num_horns(youracedata)));
			Your("%s %s through %s %s.", hornbuf, vtense(hornbuf, uarmh->otyp == find_gcirclet() ? "pass" : "pierce"),
				 shk_your(yourbuf, otmp), xname(otmp));
		}
    }
	if ((otmp = uarmg) != 0) {
		if(nogloves(youracedata) 
			|| nolimbs(youracedata) 
			|| youracedata->msize != otmp->objsize
			|| is_gaseous_noequip(youracedata)
			|| noncorporeal(youracedata)
		){
			if (donning(otmp)) cancel_don();
			/* Drop weapon along with gloves */
			You("drop your gloves%s!", uwep ? " and weapon" : "");
			drop_weapon(0);
			(void) Gloves_off();
			dropx(otmp);
		}
	}
	if ((otmp = uarms) != 0) {
		if(nohands(youracedata) || nolimbs(youracedata) || bimanual(uwep,youracedata) || is_gaseous_noequip(youracedata) || noncorporeal(youracedata)){
			if (donning(otmp)) cancel_don();
			You("can no longer hold your shield!");
			(void) Shield_off();
			dropx(otmp);
		}
	}
	if ((otmp = uarmf) != 0) {
		if(noboots(youracedata)
			|| (!humanoid(youracedata) && !can_wear_boots(youracedata))
			|| !boots_size_fits(youracedata, otmp)
			|| is_gaseous_noequip(youracedata)
			|| noncorporeal(youracedata)
		){
			if (donning(otmp)) cancel_don();
			if (is_gaseous_noequip(youracedata))
				Your("boots fall away!");
			else Your("boots %s off your feet!",
				youracedata->msize < otmp->objsize ? "slide" : "are pushed");
			(void) Boots_off();
			dropx(otmp);
		}
	}
}

void
drop_weapon(alone)
int alone;
{
    struct obj *otmp;
    struct obj *otmp2;

    if ((otmp = uwep) != 0) {
	/* !alone check below is currently superfluous but in the
	 * future it might not be so if there are monsters which cannot
	 * wear gloves but can wield weapons
	 */
	if (!alone || you_cantwield(youracedata)) {
	    struct obj *wep = uwep;

	    if (alone) You("find you must drop your weapon%s!",
			   	u.twoweap ? "s" : "");
	    otmp2 = u.twoweap ? uswapwep : 0;
	    uwepgone();
	    if (!wep->cursed || wep->otyp != LOADSTONE)
		dropx(otmp);
	    if (otmp2 != 0) {
		uswapwepgone();
		if (!otmp2->cursed || otmp2->otyp != LOADSTONE)
		    dropx(otmp2);
	    }
	    untwoweapon();
	} else if (!could_twoweap(youmonst.data)) {
	    untwoweapon();
	}
    }
}

void
rehumanize()
{
	/* You can't revert back while unchanging */
	if (Unchanging && (u.mh < 1)) {
		killer_format = NO_KILLER_PREFIX;
		killer = "killed while stuck in creature form";
		done(DIED);
		return;	/* you do NOT get to rehumanize if you are unchanging and lifesaving */
	}

	/* delete any ls attached to you */
	del_light_source((&youmonst)->light);

	polyman("return to %s form!", urace.adj);

	if (u.uhp < 1) {
	    char kbuf[256];

	    Sprintf(kbuf, "reverting to unhealthy %s form", urace.adj);
	    killer_format = KILLED_BY;
	    killer = kbuf;
	    done(DIED);
	}
	if (!uarmg) selftouch("No longer petrify-resistant, you");
	nomul(0, NULL);

	flags.botl = 1;
	vision_full_recalc = 1;
	(void) encumber_msg();
}

int
dobreathe(mdat)
struct permonst *mdat;
{
	struct attack mattk;
	int powermult = 100;

	if (!getdir((char *)0)) return MOVE_CANCELLED;

	{
		struct attack * aptr;
		aptr = attacktype_fordmg(mdat, AT_BREA, AD_ANY);
		if (!aptr) aptr = attacktype_fordmg(mdat, AT_BRSH, AD_ANY);
		if (!aptr && Race_if(PM_HALF_DRAGON)) aptr = attacktype_fordmg(&mons[PM_HALF_DRAGON], AT_BREA, AD_ANY);

		if (!aptr) {
			impossible("bad breath attack?");
			return MOVE_CANCELLED;
		}
		mattk = *aptr;
	}

	if (Race_if(PM_HALF_DRAGON) && (mattk.adtyp == AD_HDRG || flags.HDbreath == mattk.adtyp)) {
		/* HalfDragons get a +0.5x bonus per armor piece that matches their default breath */
		int mtyp;
		switch (flags.HDbreath)
		{
		case AD_MAGM: mtyp = PM_GRAY_DRAGON;   break;
		case AD_FIRE: mtyp = PM_RED_DRAGON;    break;
		case AD_COLD: mtyp = PM_WHITE_DRAGON;  break;
		case AD_ELEC: mtyp = PM_BLUE_DRAGON;   break;
		case AD_DRST: mtyp = PM_GREEN_DRAGON;  break;
		case AD_SLEE: mtyp = PM_ORANGE_DRAGON; break;
		case AD_ACID: mtyp = PM_YELLOW_DRAGON; break;
		case AD_RBRE: mtyp = PM_SHIMMERING_DRAGON; break;
		case AD_DISN: mtyp = PM_BLACK_DRAGON;  break;
		default:
			impossible("bad HDbreath %d", flags.HDbreath);
			return MOVE_CANCELLED;
		}
		if (uarm && Dragon_armor_matches_mtyp(uarm, mtyp))
			powermult += 50;

		if (uarms && Dragon_armor_matches_mtyp(uarms, mtyp))
			powermult += 50;

		/* handled in xbreathey for monsters */
		if (flags.HDbreath == AD_FIRE && u.ulevel >= 14)
			mattk.damn += 2;
	}
	if(carrying_art(ART_DRAGON_S_HEART_STONE))
		powermult *= 2;

	/* use powermult to increase the damage dealt */
	mattk.damd = (mattk.damd ? mattk.damd : 6) * powermult / 100;

	/* use xbreathey to do the attack */
	return xbreathey(&youmonst, &mattk, 0, 0) ? MOVE_STANDARD : MOVE_CANCELLED;
}

int
domakewhisperer()
{
	const char *petname;
	struct monst *mtmp;
	int duration;
	if (u.uen < (10+min(Insight, 45))) {
	    You("concentrate but lack the energy to maintain doing so.");
	    return MOVE_CANCELLED;
	}
	
	duration = ACURR(A_CHA) + 1;
	
	if(Insight >= 20)
		duration = 2*ACURR(A_CHA);
	
	u.whisperturn = moves+duration+14;
	losepw(10+min(Insight, 45));
	flags.botl = 1;
	
	// makedog();
	mtmp = makemon(&mons[PM_SECRET_WHISPERER], u.ux, u.uy, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH|MM_EDOG|MM_ESUM);

	if(!mtmp) return MOVE_CANCELLED; /* pets were genocided */

	mark_mon_as_summoned(mtmp, &youmonst, duration, 0);
	for(int i = min(45, (Insight - mtmp->m_lev)); i > 0; i--){
		grow_up(mtmp, (struct monst *) 0);
		//Technically might grow into a genocided form.
		if(DEADMONSTER(mtmp))
			return MOVE_CANCELLED;
	}
	mtmp->mspec_used = 0;

	if(mtmp->m_lev) mtmp->mhpmax = 8*(mtmp->m_lev-1)+rnd(8);
	mtmp->mhp = mtmp->mhpmax;

	petname = whisperername;
	if (*petname)
		mtmp = christen_monst(mtmp, petname);
	
	initedog(mtmp);
	EDOG(mtmp)->loyal = TRUE;
	EDOG(mtmp)->waspeaceful = TRUE;
	mtmp->mpeacetime = 0;
	return MOVE_STANDARD;
}

int
dokiai()
{
	const char *petname;
	struct monst *mtmp;
	int duration;

	pline("Hyah!");
	song_noise(u.ulevel * 3);
	duration = xlev_to_rank(u.ulevel);
	if(ACURR(A_CHA) > 13){
		duration += rnd(ACURR(A_CHA)-13);
	}
	u.uencouraged = max(duration, u.uencouraged);
	incr_itimeout(&HAggravate_monster, duration*4);
	u.kiaiturn = moves+duration*8+rnz(100);
	flags.botl = 1;
	
	return MOVE_PARTIAL;
}

int
doelementalbreath()
{
	struct monst *mon = 0;
	int type;
	
	if (Strangled) {
	    You_cant("breathe.  Sorry.");
	    return MOVE_CANCELLED;
	}
	if (u.uen < 45) {
	    You("don't have enough energy to sing an elemental!");
	    return MOVE_CANCELLED;
	}
	losepw(45);
	flags.botl = 1;

	type = flags.HDbreath;
	switch(type){
		case AD_FIRE:
			mon = makemon(&mons[PM_FIRE_ELEMENTAL], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
		case AD_COLD:
			mon = makemon(&mons[PM_ICE_PARAELEMENTAL], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
		case AD_ELEC:
			mon = makemon(&mons[PM_LIGHTNING_PARAELEMENTAL], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
		case AD_ACID:
			mon = makemon(&mons[PM_ACID_PARAELEMENTAL], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
		case AD_DRST:
			mon = makemon(&mons[PM_POISON_PARAELEMENTAL], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
		case AD_SLEE:
		case AD_RBRE:
			mon = makemon(&mons[PM_DREAM_QUASIELEMENTAL], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
		case AD_MAGM:
			mon = makemon(&mons[PM_ENERGY_VORTEX], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		break;
	}
	if(mon){
		initedog(mon);
		mon->m_lev = (mon->m_lev+u.ulevel)/2;
		mon->mhpmax = (mon->m_lev * 8) - 4;
		mon->mhp =  mon->mhpmax;
	}
	return MOVE_STANDARD;
}

int
dospit()
{
	struct obj *otmp;

	if (!getdir((char *)0))
		return MOVE_CANCELLED;
	else {
		xspity(&youmonst, attacktype_fordmg(youracedata, AT_SPIT, AD_ANY), 0, 0);
	}
	return MOVE_STANDARD;
}

int
doremove()
{
	if (!Punished) {
		You("are not chained to anything!");
		return MOVE_CANCELLED;
	}
	unpunish();
	return MOVE_STANDARD;
}

int
dospinweb()
{
	register struct trap *ttmp = t_at(u.ux,u.uy);

	if (Levitation || Weightless
	    || Underwater || Is_waterlevel(&u.uz)) {
		You("must be on the ground to spin a web.");
		return MOVE_CANCELLED;
	}
	if (u.uswallow) {
		You("release web fluid inside %s.", mon_nam(u.ustuck));
		if (is_animal(u.ustuck->data)) {
			expels(u.ustuck, u.ustuck->data, TRUE);
			return MOVE_INSTANT;
		}
		if (is_whirly(u.ustuck->data)) {
			int i;

			for (i = 0; i < NATTK; i++)
				if (u.ustuck->data->mattk[i].aatyp == AT_ENGL)
					break;
			if (i == NATTK)
			       impossible("Swallower has no engulfing attack?");
			else {
				char sweep[30];

				sweep[0] = '\0';
				switch(u.ustuck->data->mattk[i].adtyp) {
					case AD_FIRE:
						Strcpy(sweep, "ignites and ");
						break;
					case AD_ELEC:
						Strcpy(sweep, "fries and ");
						break;
					case AD_COLD:
						Strcpy(sweep,
						      "freezes, shatters and ");
						break;
				}
				pline_The("web %sis swept away!", sweep);
			}
			return MOVE_INSTANT;
		}		     /* default: a nasty jelly-like creature */
		pline_The("web dissolves into %s.", mon_nam(u.ustuck));
		return MOVE_INSTANT;
	}
	if (u.utrap) {
		You("cannot spin webs while stuck in a trap.");
		return MOVE_CANCELLED;
	}
	exercise(A_DEX, TRUE);
	if (ttmp) switch (ttmp->ttyp) {
		case PIT:
		case SPIKED_PIT: You("spin a web, covering up the pit.");
			deltrap(ttmp);
			bury_objs(u.ux, u.uy);
			newsym(u.ux, u.uy);
			return MOVE_STANDARD;
		case SQKY_BOARD: pline_The("squeaky board is muffled.");
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return MOVE_STANDARD;
		case TELEP_TRAP:
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
			Your("webbing vanishes!");
			return MOVE_INSTANT;
		case WEB: You("make the web thicker.");
			return MOVE_STANDARD;
		case HOLE:
		case TRAPDOOR:
			You("web over the %s.",
			    (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole");
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return MOVE_STANDARD;
		case ROLLING_BOULDER_TRAP:
			You("spin a web, jamming the trigger.");
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return MOVE_STANDARD;
		case VIVI_TRAP:
			You("spin a web, ruining the delicate machinery.");
			deltrap(ttmp);
			newsym(u.ux, u.uy);
			return MOVE_STANDARD;
		case ARROW_TRAP:
		case DART_TRAP:
		case BEAR_TRAP:
		case FLESH_HOOK:
		case ROCKTRAP:
		case FIRE_TRAP:
		case LANDMINE:
		case SLP_GAS_TRAP:
		case RUST_TRAP:
		case MAGIC_TRAP:
		case ANTI_MAGIC:
		case POLY_TRAP:
		case MUMMY_TRAP:
			You("have triggered a trap!");
			dotrap(ttmp, 0);
			return MOVE_STANDARD;
		default:
			impossible("Webbing over trap type %d?", ttmp->ttyp);
			return MOVE_CANCELLED;
		}
	else if (On_stairs(u.ux, u.uy)) {
	    /* cop out: don't let them hide the stairs */
	    Your("web fails to impede access to the %s.",
		 (levl[u.ux][u.uy].typ == STAIRS) ? "stairs" : "ladder");
	    return MOVE_STANDARD;
		 
	}
	ttmp = maketrap(u.ux, u.uy, WEB);
	if (ttmp) {
		ttmp->tseen = 1;
		ttmp->madeby_u = 1;
	}
	newsym(u.ux, u.uy);
	return MOVE_STANDARD;
}

int
dosummon()
{
	int placeholder;
	if (u.uen < 10) {
	    You("lack the energy to send forth a call for help!");
	    return MOVE_CANCELLED;
	}
	losepw(10);
	flags.botl = 1;

	You("call upon your brethren for help!");
	exercise(A_WIS, TRUE);
	if (!were_summon(&youmonst, &placeholder, (char *)0))
		pline("But none arrive.");
	return MOVE_STANDARD;
}

int
dodemonpet()
{
	int i;
	struct permonst *pm;
	struct monst *dtmp;

	if (u.uen < 10) {
		You("lack the energy to call for help!");
		return MOVE_CANCELLED;
	}
	else if (youmonst.summonpwr >= youmonst.data->mlevel) {
		You("don't have the authority to call for any more help!");
		return MOVE_CANCELLED;
	}
	losepw(10);
	flags.botl = 1;

	i = (is_demon(youracedata) && !rn2(6)) 
	     ? ndemon(u.ualign.type) : NON_PM;
	pm = i != NON_PM ? &mons[i] : youracedata;
	if(pm->geno&G_UNIQ) {
		pm = &mons[ndemon(A_NONE)];
	}
	if(is_ancient(pm)) {
	    pm = rn2(4) ? &mons[PM_METAMORPHOSED_NUPPERIBO] : &mons[PM_ANCIENT_NUPPERIBO];
	}
	if ((dtmp = makemon(pm, u.ux, u.uy, MM_ESUM)) != 0) {
		pline("Some hell-p has arrived!");
	    (void)tamedog(dtmp, (struct obj *)0);
		mark_mon_as_summoned(dtmp, &youmonst, 250, 0);
		exercise(A_WIS, TRUE);
	}
	else {
		pline("No help arrived.");
	}
	return MOVE_STANDARD;
}

static NEARDATA const char food_types[] = { FOOD_CLASS, 0 };

int
dovampminion()
{
	struct obj *otmp;
	struct obj *corpse = (struct obj *)0;
	boolean onfloor = FALSE;
	char qbuf[QBUFSZ];
	char c;
	
	for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
		if(otmp->otyp==CORPSE && otmp->odrained
			&& ((peek_at_iced_corpse_age(otmp) + 20) >= monstermoves)) {
		
			Sprintf(qbuf, "There %s %s here; feed blood to %s?",
				otense(otmp, "are"),
				doname(otmp),
				(otmp->quan == 1L) ? "it" : "one");
			if((c = yn_function(qbuf,ynqchars,'n')) == 'y'){
				corpse = otmp;
				onfloor = TRUE;
				break;
			}
			else if(c == 'q')
				break;
		}
	}
	if (!corpse) corpse = getobj(food_types, "feed blood to");
	if (!corpse) return MOVE_CANCELLED;

	struct permonst *pm = &mons[corpse->corpsenm];
	if (!has_blood(pm)){
		pline("You can't put blood in a monster that didn't start with blood!");
		return MOVE_CANCELLED;
	} else {
		struct monst * mtmp = revive(corpse, FALSE);
		if (mtmp) {
			set_template(mtmp, VAMPIRIC);
			tamedog_core(mtmp, (struct obj *)0, TRUE);
			losexp("donating blood", TRUE, TRUE, FALSE);
		}
		else {
			pline1(nothing_happens);
		}
	}
	
	return MOVE_STANDARD;
}

int
dotinker()
{
	if (u.uen < 10) {
	    You("lack the energy to tinker.");
	    return MOVE_CANCELLED;
	}
	losepw(10);
	flags.botl = 1;
	/*Make tinkered friend*/
	struct monst *mlocal;
	
	
	if(monsndx(youracedata) == PM_HOOLOOVOO){
		if(rn2(4)) mlocal = makemon(&mons[PM_GOLDEN_HEART], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_ADJACENTSTRICT|MM_NOCOUNTBIRTH);
		else mlocal = makemon(&mons[PM_ID_JUGGERNAUT], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_ADJACENTSTRICT|MM_NOCOUNTBIRTH);
	} else {
		if(u.ulevel > 10 && !rn2(10)) mlocal = makemon(&mons[PM_FIREWORK_CART], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_ADJACENTSTRICT|MM_NOCOUNTBIRTH);
		else mlocal = makemon(&mons[PM_CLOCKWORK_SOLDIER+rn2(3)], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_ADJACENTSTRICT|MM_NOCOUNTBIRTH);
	}
	
	if(mlocal){
		mlocal->mvar_vector = rn2(8);
		initedog(mlocal);
		mlocal->mtame = 10;
		mlocal->mpeaceful = 1;
		newsym(mlocal->mx,mlocal->my);
	}
	return MOVE_STANDARD;
}

int
dogaze()
{
	register struct monst *mtmp;
	int result = 0;

	if (Blind) {
		You_cant("see anything to gaze at.");
		return MOVE_CANCELLED;
	}
	if (u.uen < 15) {
		You("lack the energy to use your special gaze!");
		return MOVE_CANCELLED;
	}
	if (!throwgaze()) {
		/* player cancelled targetting or picked a not-allowed location */
		return MOVE_CANCELLED;
	}
	else {
		losepw(15);
		flags.botl = 1;

		if ((mtmp = m_at(u.dx, u.dy)) && canseemon(mtmp)) {
			struct attack *a;

			for (a = &youracedata->mattk[0]; a < &youracedata->mattk[NATTK]; a++){
				if (a->aatyp == AT_GAZE) 
					result |= xgazey(&youmonst, mtmp, a, -1);
			}
			if(!Upolyd && check_vampire(VAMPIRE_GAZE)){
				struct attack gaze = {AT_GAZE, AD_PLYS, 1, 4};
				result |= xgazey(&youmonst, mtmp, &gaze, -1);
			}

			if (!result) {
				pline("%s seemed not to notice.", Monnam(mtmp));
			}


			/* deliberately gazing at some things is dangerous. This is inconsistent with monster agr gazes, but whatever */

			/* For consistency with passive() in uhitm.c, this only
			* affects you if the monster is still alive.
			*/
			if (!DEADMONSTER(mtmp) &&
				(mtmp->mtyp == PM_FLOATING_EYE) && !mtmp->mcan) {
				if (!Free_action) {
					You("are frozen by %s gaze!",
						s_suffix(mon_nam(mtmp)));
					nomul((u.ulevel > 6 || rn2(4)) ?
						-d((int)mtmp->m_lev + 1,
						(int)mtmp->data->mattk[0].damd)
						: -200, "frozen by a monster's gaze");
					return MOVE_STANDARD;
				}
				else
					You("stiffen momentarily under %s gaze.",
					s_suffix(mon_nam(mtmp)));
			}
			/* Technically this one shouldn't affect you at all because
			* the Medusa gaze is an active monster attack that only
			* works on the monster's turn, but for it to *not* have an
			* effect would be too weird.
			*/
			if (!DEADMONSTER(mtmp) &&
				(mtmp->mtyp == PM_MEDUSA) && !mtmp->mcan) {
				pline(
					"Gazing at the awake %s is not a very good idea.",
					l_monnam(mtmp));
				/* as if gazing at a sleeping anything is fruitful... */
				You("turn to stone...");
				killer_format = KILLED_BY;
				killer = "deliberately meeting Medusa's gaze";
				done(STONING);
			}
		}
		else {
			You("gaze at empty space.");
		}
	}
	return MOVE_STANDARD;
}
#if 0
{
	register struct monst *mtmp;
	int looked = 0;
	char qbuf[QBUFSZ];
	int i;
	int dmg = 0;
	uchar adtyp = 0;
	uchar damn = 0;
	uchar damd = 0;
	const int elementalgaze[] = {AD_FIRE,AD_COLD,AD_ELEC};

	for (i = 0; i < NATTK; i++) {
	    if(youracedata->mattk[i].aatyp == AT_GAZE) {
		adtyp = youracedata->mattk[i].adtyp;
		damn = youracedata->mattk[i].damn;
		damd = youracedata->mattk[i].damd;
		break;
	    }
	}

	if (Blind) {
	    You_cant("see anything to gaze at.");
	    return 0;
	}
	if (u.uen < 15) {
	    You("lack the energy to use your special gaze!");
	    return(0);
	}
	losepw(15);
	flags.botl = 1;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
		if(ward_at(mtmp->mx,mtmp->my) == HAMSA) continue;
	    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
		looked++;
		if (Invis && !mon_resistance(mtmp,SEE_INVIS))
		    pline("%s seems not to notice your gaze.", Monnam(mtmp));
		else if (mtmp->minvis && !See_invisible(mtmp->mx,mtmp->my))
		    You_cant("see where to gaze at %s.", Monnam(mtmp));
		else if (mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT) {
		    looked--;
		    continue;
		} else if (flags.safe_dog && !Confusion && !Hallucination
		  && mtmp->mtame) {
		    You("avoid gazing at %s.", y_monnam(mtmp));
		} else {
		    if (iflags.attack_mode != ATTACK_MODE_FIGHT_ALL && mtmp->mpeaceful && !Confusion
							&& !Hallucination) {
			Sprintf(qbuf, "Really %s %s?",
			    (adtyp == AD_CONF) ? "confuse" : "attack",
			    mon_nam(mtmp));
			if (yn(qbuf) != 'y') continue;
			setmangry(mtmp);
		    }
		    if (!mtmp->mcanmove || !mtmp->mnotlaugh || mtmp->mstun || mtmp->msleeping ||
				    is_blind(mtmp) || !haseyes(mtmp->data)) {
			looked--;
			continue;
		    }
		    /* No reflection check for consistency with when a monster
		     * gazes at *you*--only medusa gaze gets reflected then.
		     */
		    if(adtyp == AD_RETR)
			adtyp = elementalgaze[rn2(SIZE(elementalgaze))];	//flat random member of elementalgaze
		    switch(adtyp){
		   	 case AD_CONF:
		   	     if (!mtmp->mconf)
		   	         Your("gaze confuses %s!", mon_nam(mtmp));
		   	     else
		   	         pline("%s is getting more and more confused.",
		   	     				Monnam(mtmp));
		   	     mtmp->mconf = 1;
		   	 break;
		   	 case AD_FIRE:
		   	     dmg = d(damn,damd);
		   	     You("attack %s with a fiery gaze!", mon_nam(mtmp));
		   	     if (resists_fire(mtmp)) {
		   	         pline_The("fire doesn't burn %s!", mon_nam(mtmp));
		   	         dmg = 0;
		   	     }
		   	     if((int) u.ulevel > rn2(20))
		   	         (void) destroy_item(mtmp, SCROLL_CLASS, AD_FIRE);
		   	     if((int) u.ulevel > rn2(20))
		   	         (void) destroy_item(mtmp, POTION_CLASS, AD_FIRE);
		   	     if((int) u.ulevel > rn2(25))
		   	         (void) destroy_item(mtmp, SPBOOK_CLASS, AD_FIRE);
		   	 break;
		   	 case AD_COLD:
		   	     dmg = d(damn,damd);
		   	     You("attack %s with a cold gaze!", mon_nam(mtmp));
		   	     if (resists_cold(mtmp)) {
		   	         pline_The("cold doesn't freeze %s!", mon_nam(mtmp));
		   	         dmg = 0;
		   	     }
		   	     if((int) u.ulevel > rn2(20))
		   	         (void) destroy_item(mtmp, POTION_CLASS, AD_COLD);
		   	 break;
		   	 case AD_ELEC:
		   	     dmg = d(damn,damd);
		   	     You("attack %s with an electrifying gaze!", mon_nam(mtmp));
		   	     if (resists_elec(mtmp)) {
		   	         pline_The("electricity doesn't shock %s!", mon_nam(mtmp));
		   	         dmg = 0;
		   	     }
		   	 break;
		   	 case AD_DARK:
		   	     dmg = d(damn,damd);
		   	     You("attack %s with a dark gaze!", mon_nam(mtmp));
		   	     if (dark_immune(mtmp)) {
		   	         pline_The("dark doesn't bother %s!", mon_nam(mtmp));
		   	         dmg = 0;
		   	     } else if(mortal_race(mtmp)){
		   	         dmg *= 2;
				 }
		   	 break;
			 case AD_STDY:
				You("study %s carefully.",mon_nam(mtmp));
				mtmp->mstdy = max(d(damn,damd),mtmp->mstdy);
			 break;
			 case AD_PLYS:{
				int plys;
				if(damn == 0 || damd == 0) 
					plys = rnd(10);
				else plys = d(damn, damd);
		    		mtmp->mcanmove = 0;
		    		mtmp->mfrozen = plys;
				You("mesmerize %s!", mon_nam(mtmp));
			     }
			 break;
			 case AD_SPOR:
			 case AD_MIST:{
				int n;
				int mndx;
				struct monst *mtmp2;
				struct monst *mtmp3;
				if(youracedata->mtyp ==  PM_MIGO_PHILOSOPHER){
					n = rnd(4);
					pline("Whirling snow swirls out from around you.");
					mndx = PM_ICE_VORTEX;
				} else if(youracedata->mtyp == PM_MIGO_QUEEN){
					n = rnd(2);
					pline("Scalding steam swirls out from around you.");
					mndx = PM_STEAM_VORTEX;
				} else if(youracedata->mtyp == PM_SWAMP_FERN){
					n = 1;
					You("release a spore.");
					mndx = PM_SWAMP_FERN_SPORE;
				} else if(youracedata->mtyp == PM_BURNING_FERN){
					n = 1;
					You("release a spore.");
					mndx = PM_BURNING_FERN_SPORE;
				} else if(adtyp == AD_SPOR){
					n = 1;
					You("release a spore.");
					mndx = PM_DUNGEON_FERN_SPORE;
				} else {
					n = rnd(4);
					pline("fog billows out from around you.");
					mndx = PM_FOG_CLOUD;
				}
				for(i=0;i<n;i++){
					mtmp3 = makemon(&mons[mndx], u.ux, u.uy, MM_ADJACENTOK|MM_ADJACENTSTRICT|MM_ESUM);
				 	if (mtmp3 && (mtmp2 = tamedog(mtmp3, (struct obj *)0)) != 0){
						mtmp3 = mtmp2;
						mtmp3->mtame = 30;
						mark_mon_as_summoned(mtmp3, &youmonst, 10, 0);
					} else mongone(mtmp3);
				}
			 }
			 break;
			 case AD_BLND:{
			 	register unsigned rnd_tmp;
				if (!is_blind(mtmp))
			 	   pline("%s is blinded.", Monnam(mtmp));
				rnd_tmp = d(damn, damd);
				pline("%d",rnd_tmp);
				if ((rnd_tmp += mtmp->mblinded) > 127) rnd_tmp = 127;
				mtmp->mblinded = rnd_tmp;
				mtmp->mcansee = 0;
				mtmp->mstrategy &= ~STRAT_WAITFORU;
			 }
			 break;
			 default:
	    			impossible("gaze attack %d?", adtyp);
	    			return 0;
			 break;
		     }
 		    if (dmg && !DEADMONSTER(mtmp)) mtmp->mhp -= dmg;
		    if (mtmp->mhp <= 0) killed(mtmp);
		    /* For consistency with passive() in uhitm.c, this only
		     * affects you if the monster is still alive.
		     */
		    if (!DEADMONSTER(mtmp) &&
			  (mtmp->mtyp==PM_FLOATING_EYE) && !mtmp->mcan) {
			if (!Free_action) {
			    You("are frozen by %s gaze!",
					     s_suffix(mon_nam(mtmp)));
			    nomul((u.ulevel > 6 || rn2(4)) ?
				    -d((int)mtmp->m_lev+1,
					    (int)mtmp->data->mattk[0].damd)
				    : -200, "frozen by a monster's gaze");
			    return 1;
			} else
			    You("stiffen momentarily under %s gaze.",
				    s_suffix(mon_nam(mtmp)));
		    }
		    /* Technically this one shouldn't affect you at all because
		     * the Medusa gaze is an active monster attack that only
		     * works on the monster's turn, but for it to *not* have an
		     * effect would be too weird.
		     */
		    if (!DEADMONSTER(mtmp) &&
			    (mtmp->mtyp == PM_MEDUSA) && !mtmp->mcan) {
			pline(
			 "Gazing at the awake %s is not a very good idea.",
			    l_monnam(mtmp));
			/* as if gazing at a sleeping anything is fruitful... */
			You("turn to stone...");
			killer_format = KILLED_BY;
			killer = "deliberately meeting Medusa's gaze";
			done(STONING);
		    }
		}
	    }
	}
	if (!looked) You("gaze at no place in particular.");
	return 1;
}
#endif

int
dohide()
{
	boolean ismimic = youracedata->mlet == S_MIMIC;

	if (u.uundetected || (ismimic && youmonst.m_ap_type != M_AP_NOTHING)) {
		You("are already hiding.");
		return MOVE_CANCELLED;
	}
	if (ismimic) {
		/* should bring up a dialog "what would you like to imitate?" */
		youmonst.m_ap_type = M_AP_OBJECT;
		youmonst.mappearance = STRANGE_OBJECT;
	} else
		u.uundetected = 1;
	newsym(u.ux,u.uy);
	return MOVE_STANDARD;
}

int
domindblast()
{
	struct monst *mtmp, *nmon;
	int dice = 1, mfdmg;
	int twin_dice = 0;
	int round_dice;
	boolean hit;
	boolean twin = (check_mutation(TWIN_DREAMS) && u.specialSealsActive&SEAL_YOG_SOTHOTH) && (Role_if(PM_MADMAN) || has_mind_blast(youracedata));
	boolean twinround;

	if (u.uen < 10) {
	    You("concentrate but lack the energy to maintain doing so.");
	    return MOVE_CANCELLED;
	}
	losepw(10);
	flags.botl = 1;

	if(Role_if(PM_MADMAN))
		dice += u.ulevel/14;
	if(check_mutation(TWIN_DREAMS) && u.specialSealsActive&SEAL_YOG_SOTHOTH)
		twin_dice += (u.ulevel+7)/14;
	
	if(!twin)
		dice += twin_dice;

	You("concentrate.");
	pline("A wave of psychic energy pours out.");
	for(mtmp=fmon; mtmp; mtmp = nmon) {
		int u_sen;

		nmon = mtmp->nmon;
		if (DEADMONSTER(mtmp))
			continue;
		// if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
			// continue;
		if(mtmp->mpeaceful)
			continue;
		if(mindless_mon(mtmp))
			continue;
		twinround = twin;
		round_dice = dice;
		u_sen = (mon_resistance(mtmp,TELEPAT) && (is_blind(mtmp) || species_blind_telepathic(mtmp->data))) || rlyehiansight(mtmp->data);
		if(is_tettigon(mtmp->data))
			u_sen = FALSE;
		hit = u_sen || (mon_resistance(mtmp,TELEPAT) && rn2(2)) || !rn2(10) || (Role_if(PM_ANACHRONOUNBINDER) && !rn2(10-(int)(u.ulevel/3))) ;
		if(!hit && twinround){
			twinround = FALSE;
			//reroll
			hit = (mon_resistance(mtmp,TELEPAT) && rn2(2)) || !rn2(10);
		}
		if(is_tettigon(mtmp->data) && rn2(10))
			hit = FALSE;
		if (hit) {
			You("lock in on %s %s.", s_suffix(mon_nam(mtmp)),
				u_sen ? "telepathy" :
				mon_resistance(mtmp,TELEPAT) ? "latent telepathy" :
				"mind");
			if(twinround)
				round_dice += twin_dice;

			mfdmg = Role_if(PM_ANACHRONOUNBINDER)?d(round_dice,15)*(int)(u.ulevel/6):d(round_dice,15);
			mtmp->mhp -= mfdmg;
			mtmp->mstrategy &= ~STRAT_WAITFORU;
			if (mtmp->mhp <= 0)
				killed(mtmp);
			else {
				
				if(round_dice >= 3){
					mtmp->mstdy = max(mfdmg, mtmp->mstdy);
					mtmp->encouraged = min(-1*mfdmg, mtmp->encouraged);
				}
				if(round_dice >= 5){
					mtmp->mstun = 1;
					mtmp->mconf = 1;
				}
			}
		}
	}
	return MOVE_STANDARD;
}

void
domindblast_strong()
{
	struct monst *mtmp, *nmon;
	int mfdmg;
	
	pline("A wave of psychic energy pours out from you.");
	for(mtmp=fmon; mtmp; mtmp = nmon) {
		int u_sen;

		nmon = mtmp->nmon;
		if (DEADMONSTER(mtmp))
			continue;
		if(mtmp->mpeaceful)
			continue;
		if(mindless_mon(mtmp))
			continue;
		u_sen = (mon_resistance(mtmp,TELEPAT) && is_blind(mtmp)) || rlyehiansight(mtmp->data);
		if (u_sen || (mon_resistance(mtmp,TELEPAT) && rn2(2)) || !rn2(10)) {
			You("lock in on %s %s.", s_suffix(mon_nam(mtmp)),
				u_sen ? "telepathy" :
				mon_resistance(mtmp,TELEPAT) ? "latent telepathy" :
				"mind");
			mfdmg = d(3,15);
			mtmp->mhp -= mfdmg;
			mtmp->mstdy = max(mfdmg, mtmp->mstdy);
			if (mtmp->mhp <= 0)
				killed(mtmp);
		}
	}
}

int
dodarken()
{

	if (u.uen < 10 && u.uen<u.uenmax) {
	    You("lack the energy to invoke the darkness.");
	    return MOVE_CANCELLED;
	}
	u.uen = max(u.uen-10,0);
	flags.botl = 1;
	You("invoke the darkness.");
	litroom(FALSE,NULL);
	
	return MOVE_STANDARD;
}

int
doclockspeed()
{
	short newspeed = doclockmenu();
	if(newspeed == PHASE_ENGINE){
		if(u.phasengn){
			u.phasengn = 0;
			You("switch off your phase engine.");
		} else {
			u.phasengn = 1;
			You("activate your phase engine.");
		}
	} else if(newspeed != u.ucspeed){
		if(newspeed == HIGH_CLOCKSPEED && u.uhs < WEAK && !(u.clockworkUpgrades&EFFICIENT_SWITCH)) morehungry(10);
		/*Note: that adjustment may have put you at weak*/
		if(newspeed == HIGH_CLOCKSPEED && u.uhs >= WEAK){
			pline("There is insufficient tension left in your mainspring for you to move at high speed.");
		}
		else if(newspeed == SLOW_CLOCKSPEED && u.uhs == SATIATED){
			pline("There is too much tension in your mainspring for you to move at low speed.");
		}
		else{
			switch(newspeed){
			case HIGH_CLOCKSPEED:
				You("increase your clock to high speed.");
				u.ucspeed = newspeed;
			break;
			case NORM_CLOCKSPEED:
				You("%s your clock to normal speed.",u.ucspeed== HIGH_CLOCKSPEED ? "decrease" : "increase");
				u.ucspeed = newspeed;
			break;
			case SLOW_CLOCKSPEED:
				You("decrease your clock to low speed.");
				u.ucspeed = newspeed;
			break;
			}
		}
		if(u.clockworkUpgrades&FAST_SWITCH) return MOVE_INSTANT;
		else return MOVE_STANDARD;
	} else{
		You("leave your clock at its current speed.");
		return MOVE_CANCELLED;
	}
	return MOVE_CANCELLED;
}

int
doandroid()
{
	short newspeed = dodroidmenu();
	if(newspeed == PHASE_ENGINE){
		if(u.phasengn){
			u.phasengn = 0;
			You("deactivate your phase engine.");
		} else {
			u.phasengn = 1;
			You("activate your phase engine.");
		}
		return MOVE_INSTANT;
	} else if(newspeed == HIGH_CLOCKSPEED){
		You("activate emergency high speed.");
		u.ucspeed = HIGH_CLOCKSPEED;
		return MOVE_INSTANT;
	} else if(newspeed == NORM_CLOCKSPEED){
		You("reduce speed to normal.");
		u.ucspeed = NORM_CLOCKSPEED;
		return MOVE_STANDARD;
	} else if(newspeed == SLOW_CLOCKSPEED){
		int mult = HEALCYCLE/u.ulevel;
		int duration = (u.uenmax - u.uen)*mult*2/3+30, i, lim;
		You("enter sleep mode.");
		u.ucspeed = NORM_CLOCKSPEED;
		for (i = 0; i < A_MAX; i++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
				ABASE(i)++;
				flags.botl = 1;
			}
		}
		u.nextsleep = moves+rnz(350)+duration;
		u.lastslept = moves;
		fall_asleep(-rn1(duration+1, duration+1), TRUE);
		return MOVE_STANDARD;
	} else if(newspeed == RECHARGER){
		static const char recharge_type[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
	    struct obj *otmp = getobj(recharge_type, "charge");

	    if (!otmp) {
			return MOVE_CANCELLED;
	    }
	    if(!recharge(otmp, 0))
			You("recharged %s.", the(xname(otmp)));
		losepw(10);
	    update_inventory();
		return MOVE_STANDARD;
	} else if (newspeed == ANDROID_COMBO) {
		return android_combo();	/* in xhity.c */
	}
	return MOVE_CANCELLED;
}

int
dowords(splaction)
int splaction;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	
	do {
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		any.a_void = 0;		/* zero out all bits */

		Sprintf(buf, "Words of Power");
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
		if (u.ufirst_light && (splaction == SPELLMENU_DESCRIBE || u.ufirst_light_timeout <= moves)){
			Sprintf(buf, "speak the First Word");
			any.a_int = FIRST_LIGHT + 1;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'q', 0, ATR_NONE, buf, MENU_UNSELECTED);
		}
		if (u.ufirst_sky && (splaction == SPELLMENU_DESCRIBE || u.ufirst_sky_timeout <= moves)){
			Sprintf(buf, "speak the Dividing Word");
			any.a_int = PART_WATER + 1;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'w', 0, ATR_NONE, buf, MENU_UNSELECTED);
		}
		if (u.ufirst_life && (splaction == SPELLMENU_DESCRIBE || u.ufirst_life_timeout <= moves)){
			Sprintf(buf, "speak the Nurturing Word");
			any.a_int = OVERGROW + 1;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'e', 0, ATR_NONE, buf, MENU_UNSELECTED);
		}
		if (u.ufirst_know && (splaction == SPELLMENU_DESCRIBE || u.ufirst_know_timeout <= moves)){
			Sprintf(buf, "speak the Word of Knowledge");
			any.a_int = APPLE_WORD + 1;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'r', 0, ATR_NONE, buf, MENU_UNSELECTED);
		}
		if (splaction != SPELLMENU_DESCRIBE && splaction < 0){
			Sprintf(buf, "Describe a word instead");
			any.a_int = SPELLMENU_DESCRIBE;
			add_menu(tmpwin, NO_GLYPH, &any,
				'?', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		if (splaction != SPELLMENU_CAST && splaction < 0) {
			Sprintf(buf, "Speak a word instead");
			any.a_int = SPELLMENU_CAST;
			add_menu(tmpwin, NO_GLYPH, &any,
				'!', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		end_menu(tmpwin, "Select Word");

		how = PICK_ONE;
		n = select_menu(tmpwin, how, &selected);
		destroy_nhwindow(tmpwin);


		if (n > 0){
			int s_no = selected[0].item.a_int - 1;

			if (selected[0].item.a_int < 0){
				splaction = selected[0].item.a_int;
				continue;
			}
			else {
				switch (splaction)
				{
				case SPELLMENU_CAST:
					return (wordeffects(selected[0].item.a_int - 1));

				case SPELLMENU_DESCRIBE:
					worddescriptions(selected[0].item.a_int - 1);
					continue;

				} // switch(splaction)
			} // doing something allowable
			free(selected);
		} // menu item was selected
		/* else end menu, nothing was selected */
		break;
	} while (TRUE);

	return MOVE_CANCELLED;
}

STATIC_OVL void
worddescriptions(spellID)
int spellID;
{
	struct obj *pseudo;

	winid datawin;
	char name[20];
	char stats[30];
	char fail[20];
	char known[20];
	
	char desc1[80] = " ";
	char desc2[80] = " ";
	char desc3[80] = " ";
	char desc4[80] = " ";

	switch (spellID){
	case FIRST_LIGHT:
		strcat(desc1, "Creates a lit field over your entire line-of-sight.");
		strcat(desc2, "Damages all hostile monsters in your line of sight.");
		strcat(desc3, "Deals heavy damage to undead, demons, and wraiths.");
		strcat(desc4, "Takes less than 1 turn to cast.");
		break;
	case PART_WATER:
		strcat(desc1, "Creates a directed plane of partitioning force.");
		strcat(desc2, "Parts water and knocks hostile monsters aside.");
		strcat(desc3, "Deals light damage, but may bisect targets.");
		strcat(desc4, "Takes less than 1 turn to cast");
		break;
	case OVERGROW:
		strcat(desc1, "Creates a field of rapid plant growth in your line-of-sight.");
		strcat(desc2, "Heavily damages hostile elementals, undead, and some golems.");
		strcat(desc3, "Destroyed enemies may leave trees behind.");
		strcat(desc4, "Takes less than 1 turn to cast");
		break;
	case APPLE_WORD:
		strcat(desc1, "Creates a field of terrible knowledge in your line-of-sight.");
		strcat(desc2, "Causes all monsters to become crazed and begin to flee.");
		strcat(desc3, "Intelligent enemies may also disrobe.");
		strcat(desc4, "Takes less than 1 turn to cast");
		break;
	}
	datawin = create_nhwindow(NHW_TEXT);
	if(spellID < MAXSPELL){
		putstr(datawin, 0, "");
		putstr(datawin, 0, name);
		putstr(datawin, 0, "");
		putstr(datawin, 0, stats);
		putstr(datawin, 0, "");
		putstr(datawin, 0, fail);
		putstr(datawin, 0, known);
		putstr(datawin, 0, "");
	}
	if (desc1[3] != 0) putstr(datawin, 0, desc1);
	if (desc2[3] != 0) putstr(datawin, 0, desc2);
	if (desc3[3] != 0) putstr(datawin, 0, desc3);
	if (desc4[3] != 0) putstr(datawin, 0, desc4);
	display_nhwindow(datawin, FALSE);
	destroy_nhwindow(datawin);
	return;
}

STATIC_OVL void
uunstick()
{
	pline("%s is no longer in your clutches.", Monnam(u.ustuck));
	u.ustuck = 0;
}

void
skinback(silently)
boolean silently;
{
	if (uskin) {
		struct obj * otmp = (struct obj *)0;
		struct obj ** otmp_p;
		const char * msg = "Your skin returns to its original form.";
		if(uskin->otyp == MASK) {
			otmp_p = &ublindf;
			msg = "Your mask unmelds from your face.";
		}
		else if(uskin->otyp == LEO_NEMAEUS_HIDE){
			otmp_p = &uarmc;
		} else {
			otmp_p = &uarm;
		}
		if (!silently) pline1(msg);
		if ((otmp = *otmp_p) != (struct obj *)0) {
			if (donning(otmp)) cancel_don();
			if (otmp == ublindf) Blindf_off(ublindf);
			if (otmp == uarmc) Cloak_off();
			if (otmp == uarm) Armor_off();
		}

		*otmp_p = uskin;
		/* undo save/restore hack */
		uskin->owornmask &= ~W_SKIN;
		uskin = (struct obj *)0;
		/* undo save/restore hack */
		(*otmp_p)->owornmask &= ~W_SKIN;
	}
}

#endif /* OVLB */
#ifdef OVL1

const char *
mbodypart(struct monst *mon, int part)
{
	struct permonst *mptr = mon->data;
	return ptrbodypart(mptr, part, mon);
}

const char *
ptrbodypart(struct permonst *mptr, int part, struct monst *mon)
{
	static NEARDATA const char
	*humanoid_parts[] = { 
		"arm",			"eye",		"face",			"finger",
		"fingertip",	"foot",		"hand",			"handed", 
		"head", 		"leg",		"light headed", "neck",
		"spine",		"toe",		"hair", 		"blood",
		"lung",			"nose", 	"stomach",		"heart",
		"skin",			"flesh",	"beat",			"bones",
		"ear", 			"ears",		"tongue",		"brain",
		"creak",		"crack"},
	*uvuudaum_parts[] = { 
		"arm",			"eye",		"headspike",	"finger",
		"fingertip",	"hand",		"hand",			"handed", 
		"tentacle", 	"arm",		"addled", 		"tentacle-base",
		"spine",		"finger",	"headspike", 	"ichor",
		"pore",			"pore", 	"stomach",		"heart",
		"skin",			"flesh",	"beat",			"bones",
		"clairaudience", "clairaudience","fingertip","brain",
		"creak",	"crack"},
	*clockwork_parts[] = { 
		"arm", 			"photoreceptor",	"face",			"grasping digit",
		"digit-tip",	"foot",				"manipulator",	"manipulatored",
		"head",			"leg",				"addled",		"neck",
		"chassis",		"toe",				"doll-hair",	"oil",
		"gear",			"chemoreceptor",	"keyhole",		"mainspring",
		"foil skin",	"brass structure",	"tick",			"armature",
		"phonoreceptor","phonoreceptors",	"spring",		"card deck",
		"creak",		"bend"},
	*doll_parts[] = { 
		"arm", 			"glass eye",		"face",			"finger",
		"fingertip",	"foot",				"hand",			"handed",
		"head",			"leg",				"addled",		"neck",
		"trunk",		"toe",				"doll-hair",	"ichor",
		"lip",			"nose",				"wood",			"wood",
		"painted skin",	"wood",				"...it doesn't sound like much", "wood",
		"ear",			"ears",				"cloth tongue",	"seawater",
		"creak",		"crack"},
	*android_parts[] = { 
		"arm", 			"photoreceptor",	"face",			"finger",
		"fingertip",	"foot",				"hand",			"handed",
		"head",			"leg",				"buggy",		"neck",
		"dorsal wiring","toe",				"doll-hair",	"oily red liquid",
		"vocal pump",	"chemoreceptor",	"black box",	"heart",
		"cosmetic layer","plasteel",		"pump",			"armature",
		"phonoreceptor","phonoreceptors",	"wire",			"CPU housing",
		"flex",			"crack"},
	*assessor_parts[] = {
		"arm", 			"eye", 				"central eye", 	"grasping digit",
		"digit-tip",	"foot",				"manipulator",	"manipulatored",
		"head", 		"leg",				"addled", 		"shoulders",
		"chassis", 		"toe", 				"topspike",		"oil",
		"valve",		"olfactory nerve",	"gearbox",		"eternal core",
		"armor",		"brass structure",	"tick",			"armature",
		"phonoreceptor","phonoreceptors",	"tongue",		"brain",
		"creak",		"bend"},
	*audient_parts[] = {
		"distal limb",	"photoreceptor",	"front",		"articulated distal spike",
		"spike-tip",	"ventral needle",	"distal spike",	"spiked",
		"cap",			"ventral limb",		"addled",		"stalk",
		"chassis",		"needle-tip",		"spores",		"oil",
		"gear",			"gill",				"hyphal network","eternal core",
		"armor",		"brass structure",	"tick",			"armature",
		"phonoreceptor horn","phonoreceptor horn","hypha",	"stolon",
		"creak",	"bend"},
	*jelly_parts[] = {
		"pseudopod",		"dark spot",		"front",		"pseudopod extension",
		"pseudopod extremity","pseudopod root", "grasp", 		"grasped", 
		"cerebral area",	"lower pseudopod",	"viscous",		"middle",
		"centriole",		"pseudopod extremity","ripples",	"plasm",
		"tiny cilia",		"chemosensor",		"vacuoles",		"cytoskeletal structure",
		"membrane",			"cortex",			"shift",		"cytoskeletal filaments",
		"membrane",			"membrane",			"pseudopod",	"nucleus",
		"creak",			"crack" },
	*animal_parts[] = {
		"forelimb", 		"eye", 				"face", 		"foreclaw",
		"claw tip",			"rear claw", 		"foreclaw", 	"clawed", 
		"head", 			"rear limb",		"light headed", "neck",
		"spine", 			"rear claw tip",	"fur", 			"blood", 
		"lung", 			"nose", 			"stomach",		"heart",
		"skin",				"flesh",			"beat",			"bones",
		"ear",				"ears",				"tongue",		"brain",
		"creak",			"crack" },
	*insect_parts[] = { 
		"forelimb",			"compound eye",		"face",			"foreclaw",
		"claw tip",			"rear claw", 		"foreclaw", 	"clawed", 
		"head", 			"rear limb",		"light headed", "neck", 
		"notochord", 		"rear claw tip",	"setae", 		"haemolymph", 
		"spriacle", 		"antenna", 			"stomach",		"dorsal vessel",
		"exoskeleton",		"chitin",			"pulse",		"apodeme",
		"tympanum",			"tympana",			"haustellum",	"brain",
		"creak",		"tear" },
	*bird_parts[] = { 
		"wing", 			"eye", 				"face", 		"wing", 
		"wing tip",			"foot", 			"wing", 		"winged", 
		"head", 			"leg",				"light headed", "neck", 
		"spine", 			"toe",				"feathers", 	"blood",
		"lung", 			"bill", 			"stomach",		"heart",
		"skin",				"flesh",			"beat",			"bones",
		"ear",				"ears",				"tongue",		"brain",
		"creak",			"crack" },
	*horse_parts[] = {
		"foreleg", 			"eye", 				"face", 		"forehoof",
		"hoof tip",			"rear hoof", 		"foreclaw", 	"hooved", 
		"head", 			"rear leg",			"light headed",	"neck",
		"backbone", 		"rear hoof tip",	"mane", 		"blood", 
		"lung", 			"nose", 			"stomach",		"heart",
		"skin",				"flesh",			"beat",			"bones",
		"ear",				"ears",				"tongue",		"brain",
		"creak",			"crack"},
	*sphere_parts[] = { 
		"appendage", 		"optic nerve", 		"body", 		"tentacle", 
		"tentacle tip", 	"lower appendage",	"tentacle",		"tentacled",
		"body",				"lower tentacle", 	"rotational",	"equator",
		"body", 			"lower tentacle tip","surface",		"life force",
		"retina",			"olfactory nerve",	"interior",		"core",
		"surface",			"subsurface layers","pulse",		"auras",
		"tympanic membrane","tympanic membranes","tentacle",	"brain",
		"flicker",			"blink out"},
	*spore_parts[] = { 
		"stalk", 			"visual area", 		"front", 		"stalk", 
		"stalk tip", 		"stalk",			"stalk",		"stalked",
		"annulus",			"stalk", 			"addled",		"equator",
		"body", 			"stalk tip",		"surface",		"sap",
		"lip",				"lip",				"interior",		"spores",
		"annulus",			"flesh",			"...they don't sound like much","cells",
		"tympanic area",	"tympanic area",	"hypha",		"spore",
		"flex",				"crack"},
	*fungus_parts[] = {
		"mycelium", 		"visual area", 		"front", 					"hypha",
		"hypha", 			"root", 			"strand", 					"stranded",
		"cap area",			"rhizome", 			"sporulated", 				"stalk", 
		"root", 			"rhizome tip",		"spores", 					"juice", 
		"gill", 			"gill", 			"interior",					"hyphal network",
		"cuticle",			"flesh",			"...it doesn't sound like much","hyphae",
		"tympanic area",	"tympanic area",	"hypha",					"stolon",
		"stretch",					"tear" },
	*tree_parts[] = { 
		"limb", 	"visual area",	"front",						"leaf",
		"leaftip",	"taproot",		"twig",							"twigged",
		"crown", 	"root", 		"wilty", 						"trunk",
		"heartwood","root-tip", 	"leaves", 						"sap",
		"stoma", 	"stoma",		"xylem",						"phloem",
		"bark", 	"sapwood",		"...it doesn't sound like much","wood",
		"tympanic area","tympanic area","tendril",					"apical meristem",
		"creak",	"crack" },
	*vipertree_parts[] = { 
		"coil", 	"eye",			"face",				"mouth",
		"fang",		"taproot",		"viper head",		"headed",
		"crown", 	"root", 		"disordered", 		"trunk",
		"heartwood","root-tip", 	"scales", 			"blood",
		"lung", 	"nose",			"stomach",			"heart",
		"scales", 	"sapwood",		"beat",				"wood",
		"ear",		"ears",			"forked tongue",	"apical brain",
		"creak",	"crack" },
	*blackflower_parts[] = {
		"arm",			"blank eye",	"face",			"finger",
		"fingertip",	"petal"			"hand",			"handed", 
		"head", 		"flower"	,	"light headed", "neck",
		"spine",		"petal-tip", 	"hair", 		"pale fluid",
		"lung",			"nose", 		"stomach",		"heart",
		"skin",			"flesh",		"beat",			"bones",
		"ear",			"ears",			"tongue",		"brain",
		"creak",		"crack" },
	*plant_parts[] = {
		"shoot", 		"visual area",	"front",						"leaf",
		"leaftip",		"lateral root",	"twig",							"twigged",
		"apical bud",	"primary root", "wilty", 						"stem",
		"vascular tissue","root-tip", 	"leaves", 						"sap",
		"stoma", 		"stoma",		"xylem",						"phloem",
		"epidermis",	"flesh",		"...it doesn't sound like much","stem",
		"tympanic area","tympanic area","tendril",						"apical bud",
		"stretch",		"tear" },
	*mandrake_parts[] = { 
		"arm-root", 	"eye spot",		"root-face",					"arm-root tip",
		"arm-root hair","leg-root tip",	"arm-root end",					"rooted",
		"leaves", 		"leg-root", 	"wilty", 						"ground tissue",
		"spine",		"leg-root hair","apical bud", 					"bloody sap",
		"stoma", 		"nose spots",	"xylem",						"phloem",
		"epidermis",	"flesh",		"...it doesn't sound like much","stem",
		"ear spot",		"ear spots",	"tendril",						"apical bud",
		"stretch",		"tear" },
	*willow_parts[] = { 
		"limb", 	"visual area",	"front",						"leaf",
		"leaftip",	"taproot",		"twig",							"twigged",
		"crown", 	"root", 		"wilty", 						"trunk",
		"spine",	"root-tip", 	"leaves", 						"blood",
		"stoma", 	"stoma",		"xylem",						"phloem",
		"bark", 	"flesh",		"...it doesn't sound like much","wood",
		"tympanic area","tympanic area","tendril",					"brain",
		"creak",	"crack" },
	*birch_parts[] = { 
		"limb", 		"eye",			"face",			"thorn",
		"thorn-tip",	"crawling-root","scaffold",		"scaffolded",
		"head", 		"bud union", 	"light headed", "neck",
		"spine",		"root-tip", 	"hair", 		"sap",
		"lung", 		"nose",			"stomach",		"heart",
		"bark", 		"sapwood",		"beat",			"wood",
		"ear",			"ears",			"tongue",		"brain",
		"creak",		"crack" },
	*vortex_parts[] = {
		"region",			"eye",				"front",		"minor current",
		"minor current",	"lower current",	"swirl",		"swirled",
		"central core",		"lower current",	"addled",		"center",
		"currents",			"edge",				"currents",		"life force",
		"center",			"leading edge", 	"interior",		"core",
		"vaporous currents","subsurface currents","pulse",		"currents",
		"vapor",			"vapor",			"swirl",		"core",
		"weaken",		"falter" },
	*snake_parts[] = {
		"vestigial limb", 	"eye",				"face",			"large scale",
		"large scale tip",	"rear region",		"scale gap",	"scale gapped",
		"head",				"rear region",		"light headed",	"neck",
		"length",			"rear scale",		"scales",		"blood",
		"lung",				"forked tongue",	"stomach",		"heart",
		"scales",			"flesh",			"beat",			"bones",
		"ear",				"ears",				"forked tongue","brain",
		"creak",		"crack" },
	*naunet_parts[] = {
		"watery tentacles", "eye",				"face",			"tentacle",
		"tentacle tip",		"rear region",		"tentacle",		"tentacled",
		"head",				"rear region",		"light headed",	"neck",
		"length",			"rear surface",		"watery surface","blood",
		"foamy depths",		"forked tongue",	"hungry depths","swirling depths",
		"watery surface",	"waters",			"flow",			"waters",
		"ear",				"ears",				"forked tongue","brain",
		"bubble",		"boil" },
	*fish_parts[] = {
		"fin",				"eye",				"premaxillary",	"pelvic axillary",
		"pelvic fin",		"anal fin",			"pectoral fin", "finned",
		"head",				"peduncle",			"played out",	"gills",
		"dorsal fin",		"caudal fin",		"scales",		"blood",
		"gill",				"nostril",			"stomach",		"heart",
		"scales",			"flesh",			"beat",			"bones",
		"ear",				"ears",				"tongue",		"brain",
		"creak",		"crack" },
	*snakeleg_humanoid_parts[] = {
		"arm",				"eye",				"face",			"finger",
		"fingertip",		"serpentine lower body","hand",		"handed", 
		"head",				"rear region",		"light headed",	"neck",
		"spine",			"tail-tip",			"hair",			"blood",
		"lung",				"nose", 			"stomach",		"heart",
		"scales",			"flesh",			"beat",			"bones",
		"ear",				"ears",				"tongue",		"brain",
		"creak",		"crack" },
	*dracae_parts[] = {
		"arm",				"eye",				"face",			"finger",
		"claw tip",			"gooey proleg",		"hand",			"handed", 
		"head",				"gooey caterpilloid lower body","light headed",	"neck",
		"notochord",		"tentacle-tip",		"tendrils",		"sol",
		"spongiform jelly",	"chemopores", 		"vacuoles",		"heart",
		"mucous membrane",	"protoplasm",		"beat",			"cytoskeletal filaments",
		"tympanic membrane","tympanic membranes","tongue",		"brain",
		"creak",		"crack" },
	*centauroid_parts[] = {
		"arm", 				"eye", 				"face", 		"finger",
		"fingertip", 		"hoof", 			"hand", 		"handed",
		"head", 			"front leg",		"light headed", "neck", 
		"spine", 			"hoof-nail", 		"hair",			"blood", 
		"lung", 			"nose", 			"stomach",		"heart",
		"skin",				"flesh",			"beat",			"bones",
		"ear",				"ears",				"tongue",		"brain",
		"creak",		"crack" },
	*luminous_parts[] = {
		"arm", 				"eye", 				"face", 		"finger",
		"fingertip", 		"leg spike", 		"claw", 		"clawed",
		"head", 			"front leg",		"light headed", "neck", 
		"spine", 			"spike-tip", 		"setae",		"swarm", 
		"swarm center", 	"swarm antenna", 	"interior",		"vital core",
		"outer swarm",		"swarm currents",	"pulse",		"latice",
		"swarm tympanum",	"swarm tympana",	"swarm haustellum",	"ego core",
		"weaken",		"falter" };
	/* claw attacks are overloaded in mons[]; most humanoids with
	   such attacks should still reference hands rather than claws */
	static const char not_claws[] = {
		S_HUMAN, S_MUMMY, S_ZOMBIE, S_LAW_ANGEL, S_NEU_ANGEL, S_CHA_ANGEL,
		S_NYMPH, S_LEPRECHAUN, S_QUANTMECH, S_VAMPIRE,
		S_ORC, S_GIANT,		/* quest nemeses */
		'\0'		/* string terminator; assert( S_xxx != 0 ); */
	};

	//Specific overrides
	if ((mptr->mtyp == PM_MUMAK || mptr->mtyp == PM_MASTODON) &&
		part == NOSE)
	    return "trunk";
	if (mptr->mtyp == PM_SHARK && part == HAIR)
	    return "skin";	/* sharks don't have scales */
	if (mptr->mtyp == PM_JELLYFISH && (part == ARM || part == FINGER ||
	    part == HAND || part == FOOT || part == TOE))
	    return "tentacle";
	if (mptr->mtyp == PM_FLOATING_EYE && part == EYE)
	    return "cornea";
	if (mptr->mtyp == PM_SUNFLOWER || mptr->mtyp == PM_MIRRORED_MOONFLOWER){
		if(part == HEAD)
			return "flower";
		if(part == FACE)
			return "mirrored corolla";
	}
	if (mptr->mtyp == PM_DREADBLOSSOM_SWARM){
		if(part == HEAD)
			return "flower";
		if(part == FACE)
			return "corolla";
	}
	if (is_fern(mptr)){
		if(part == HEAD)
			return "crosier";
	}
	if (mptr->mtyp == PM_DREADBLOSSOM_SWARM && part == LEG)
	    return "root-thorn";
	if ((mptr->mtyp == PM_DREADBLOSSOM_SWARM || mptr->mtyp == PM_SUNFLOWER || mptr->mtyp == PM_MIRRORED_MOONFLOWER) && part == HAIR)
	    return "petals";
	if (is_delouseable(mptr)){
		if(mptr->mtyp == PM_PARASITIZED_DOLL){
			if(part == HEAD)
				return "nightmarish flesh mass";
			if(part == FACE)
				return "gnawing mouths";
			if(part == EYE)
				return "winking eye";
			if(part == BLOOD)
				return "seething blood";
		} else {
			if(part == FACE)
				return "squid";
			if(part == EAR)
				return "skin";
			if(part == EARS)
				return "skin";
			if(part == EYE)
				return "eye";
		}
	}

	//PM-based part lists
	if (mptr->mtyp == PM_RAVEN || mptr->mtyp == PM_CROW)
	    return bird_parts[part];
	if (mptr->mtyp == PM_DAUGHTER_OF_NAUNET)
	    return naunet_parts[part];
	if (mptr->mtyp == PM_APHANACTONAN_ASSESSOR)
	    return assessor_parts[part];
	if (mptr->mtyp == PM_APHANACTONAN_AUDIENT)
	    return audient_parts[part];
	if (mptr->mtyp == PM_MANDRAKE)
	    return mandrake_parts[part];
	if (mptr->mtyp == PM_CANDLE_TREE)
	    return birch_parts[part];
	if (mptr->mtyp == PM_WEEPING_WILLOW)
	    return willow_parts[part];
	if (mptr->mtyp == PM_VIPER_TREE)
	    return vipertree_parts[part];
	if (mptr->mtyp == PM_BLACK_FLOWER)
	    return blackflower_parts[part];
	if (is_fern_spore(mptr))
	    return spore_parts[part];
	if (mptr->mtyp == PM_WARDEN_TREE)
	    return tree_parts[part];
	if (mptr->mtyp == PM_LIVING_DOLL)
	    return doll_parts[part];
	if (is_android(mptr))
	    return android_parts[part];
	if (mptr->mtyp == PM_UVUUDAUM)
	    return uvuudaum_parts[part];
	if (mptr->mtyp == PM_DRACAE_ELADRIN)
	    return dracae_parts[part];
	if (mptr->mtyp == PM_LUMINESCENT_SWARM)
	    return luminous_parts[part];

	//S-based part lists
	if (mptr->mlet == S_PLANT)
	    return plant_parts[part];
	if (mptr->mlet == S_UNICORN ||
		(mptr->mtyp == PM_ROTHE && part != HAIR))
	    return horse_parts[part];
	if (mptr->mlet == S_LIGHT) {
		if (part == HANDED) return "rayed";
		else if (part == ARM || part == FINGER ||
				part == FINGERTIP || part == HAND) return "ray";
		else return "beam";
	}
	if (mptr->mlet == S_EYE && !is_auton(mptr))
	    return sphere_parts[part];
	if (mptr->mlet == S_JELLY || mptr->mlet == S_PUDDING ||
		mptr->mlet == S_BLOB || mptr->mtyp == PM_JELLYFISH)
	    return jelly_parts[part];
	if (mptr->mlet == S_VORTEX || mptr->mlet == S_ELEMENTAL)
	    return vortex_parts[part];
	if (mptr->mlet == S_FUNGUS)
	    return fungus_parts[part];
	if (mptr->mlet == S_EEL && mptr->mtyp != PM_JELLYFISH)
	    return fish_parts[part];
	if (mptr->mlet == S_ANT)
		return insect_parts[part];

	//General body shape-based
	if (part == HAND || part == HANDED) {	/* some special cases */
	    if (mptr->mlet == S_DOG || mptr->mlet == S_FELINE ||
		    mptr->mlet == S_YETI)
		return part == HAND ? "paw" : "pawed";
		if(mon ? (mon == &youmonst && youracedata->mtyp == PM_HALF_DRAGON) : mptr->mtyp == PM_HALF_DRAGON)
			return part == HAND ? "claw" : "clawed";
	    if (humanoid(mptr) && attacktype(mptr, AT_CLAW) &&
		    !index(not_claws, mptr->mlet) &&
		    mptr->mtyp != PM_STONE_GOLEM &&
		    mptr->mtyp != PM_SENTINEL_OF_MITHARDIR &&
		    mptr->mtyp != PM_INCUBUS && mptr->mtyp != PM_SUCCUBUS)
		return part == HAND ? "claw" : "clawed";
	}
	if (serpentine(mptr) || (mptr->mlet == S_DRAGON && part == HAIR))
	    return snake_parts[part];
	if (snakemanoid(mptr))
	    return snakeleg_humanoid_parts[part];
	if (centauroid(mptr))
	    return centauroid_parts[part];
	if (mon ? ((mon != &youmonst && is_clockwork(mptr)) || (mon == &youmonst && uclockwork)) : is_clockwork(mptr))
	    return clockwork_parts[part];
	if (humanoid(mptr))
	    return humanoid_parts[part];
	return animal_parts[part];
}

const char *
body_part(part)
int part;
{
	return mbodypart(&youmonst, part);
}

#endif /* OVL1 */
#ifdef OVL0

int
poly_gender()
{
/* Returns gender of polymorphed player; 0/1=same meaning as flags.female,
 * 2=none.
 */
	if (is_neuter(youracedata) || !humanoid(youracedata)) return 2;
	return flags.female;
}

#endif /* OVL0 */
#ifdef OVLB

void
ugolemeffects(damtype, dam)
int damtype, dam;
{
	int heal = 0;
	/* We won't bother with "slow"/"haste" since players do not
	 * have a monster-specific slow/haste so there is no way to
	 * restore the old velocity once they are back to human.
	 */
	if (u.umonnum != PM_FLESH_GOLEM
	 && u.umonnum != PM_IRON_GOLEM
	 && u.umonnum != PM_GREEN_STEEL_GOLEM
	 && u.umonnum != PM_CHAIN_GOLEM
	 && u.umonnum != PM_ARGENTUM_GOLEM
	)
		return;
	switch (damtype) {
		case AD_EELC:
		case AD_ELEC: if (u.umonnum == PM_FLESH_GOLEM)
				heal = dam / 6; /* Approx 1 per die */
			break;
		case AD_EFIR:
		case AD_FIRE: if (u.umonnum == PM_IRON_GOLEM || u.umonnum == PM_GREEN_STEEL_GOLEM || u.umonnum == PM_CHAIN_GOLEM || u.umonnum == PM_ARGENTUM_GOLEM)
				heal = dam;
			break;
	}
	if (heal && (u.mh < u.mhmax)) {
		u.mh += heal;
		if (u.mh > u.mhmax) u.mh = u.mhmax;
		flags.botl = 1;
		pline("Strangely, you feel better than before.");
		exercise(A_STR, TRUE);
	}
}

STATIC_OVL int
armor_to_dragon(atyp)
int atyp;
{
	switch(atyp) {
	    case LEO_NEMAEUS_HIDE:
		return PM_SON_OF_TYPHON;
	    case GRAY_DRAGON_SCALE_MAIL:
	    case GRAY_DRAGON_SCALES:
		return PM_GRAY_DRAGON;
	    case SILVER_DRAGON_SCALE_MAIL:
	    case SILVER_DRAGON_SCALES:
		return PM_SILVER_DRAGON;
	    case SHIMMERING_DRAGON_SCALE_MAIL:
	    case SHIMMERING_DRAGON_SCALES:
		return PM_SHIMMERING_DRAGON;
	    case RED_DRAGON_SCALE_MAIL:
	    case RED_DRAGON_SCALES:
		return PM_RED_DRAGON;
	    case ORANGE_DRAGON_SCALE_MAIL:
	    case ORANGE_DRAGON_SCALES:
		return PM_ORANGE_DRAGON;
	    case WHITE_DRAGON_SCALE_MAIL:
	    case WHITE_DRAGON_SCALES:
		return PM_WHITE_DRAGON;
	    case BLACK_DRAGON_SCALE_MAIL:
	    case BLACK_DRAGON_SCALES:
		return PM_BLACK_DRAGON;
	    case BLUE_DRAGON_SCALE_MAIL:
	    case BLUE_DRAGON_SCALES:
		return PM_BLUE_DRAGON;
	    case GREEN_DRAGON_SCALE_MAIL:
	    case GREEN_DRAGON_SCALES:
		return PM_GREEN_DRAGON;
	    case YELLOW_DRAGON_SCALE_MAIL:
	    case YELLOW_DRAGON_SCALES:
		return PM_YELLOW_DRAGON;
	    default:
		return -1;
	}
}


STATIC_OVL short
doclockmenu()
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	char incntlet = 'a';
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "To what speed will you set your clock?");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	if(u.clockworkUpgrades&EFFICIENT_SWITCH)
		Sprintf(buf, "High speed (efficient switch)");
	else
		Sprintf(buf, "High speed");
	any.a_int = HIGH_CLOCKSPEED;	/* must be non-zero */
	incntlet = 'a';
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	
	Sprintf(buf, "Normal speed");
	incntlet = 'b';
	any.a_int = NORM_CLOCKSPEED;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	
	
	Sprintf(buf, "Low speed");
	incntlet = 'c';
	any.a_int = SLOW_CLOCKSPEED;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	
	if(u.clockworkUpgrades&PHASE_ENGINE && !u.phasengn){
		Sprintf(buf, "Activate phase engine");
		incntlet = 'd';
		any.a_int = PHASE_ENGINE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(u.clockworkUpgrades&PHASE_ENGINE && u.phasengn){
		Sprintf(buf, "Switch off phase engine");
		incntlet = 'd';
		any.a_int = PHASE_ENGINE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	
	end_menu(tmpwin, "Change your clock-speed");

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = (short) selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return (short)u.ucspeed;
}

STATIC_OVL short
dodroidmenu()
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	char incntlet = 'a';
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Use what ability?");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	if(moves >= u.nextsleep){
		Sprintf(buf, "Sleep and regenerate");
		any.a_int = SLOW_CLOCKSPEED;	/* must be non-zero */
		incntlet = 's';
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	} else {
		Sprintf(buf, "Last rest began on turn %ld", u.lastslept);
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, 0, buf, MENU_UNSELECTED);
	}
	
	if(u.ucspeed == HIGH_CLOCKSPEED){
		Sprintf(buf, "Emergency speed (active)");
		any.a_int = NORM_CLOCKSPEED;	/* must be non-zero */
	incntlet = 'e';
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	} else if(u.uen > 0){
		Sprintf(buf, "Emergency speed");
		any.a_int = HIGH_CLOCKSPEED;	/* must be non-zero */
		incntlet = 'e';
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(u.uen >= 10){
		Sprintf(buf, "Recharger");
		incntlet = 'r';
		any.a_int = RECHARGER;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(((uwep && (!is_lightsaber(uwep) || litsaber(uwep)) && P_SKILL(weapon_type(uwep)) >= P_BASIC) || ((!uwep || (is_lightsaber(uwep) && !litsaber(uwep))) && P_SKILL(P_MARTIAL_ARTS) >= P_BASIC)) && u.uen > 0 && !u.twoweap){
		Sprintf(buf, "Combo");
		incntlet = 'c';
		any.a_int = ANDROID_COMBO;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(u.clockworkUpgrades&PHASE_ENGINE && !u.phasengn){
		Sprintf(buf, "Phase engine (active)");
		incntlet = 'p';
		any.a_int = PHASE_ENGINE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(u.clockworkUpgrades&PHASE_ENGINE && !u.phasengn){
		Sprintf(buf, "Phase engine");
		incntlet = 'p';
		any.a_int = PHASE_ENGINE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	
	end_menu(tmpwin, "Select android ability");

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

#endif /* OVLB */

/*polyself.c*/

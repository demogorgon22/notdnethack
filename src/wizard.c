/*	SCCS Id: @(#)wizard.c	3.4	2003/02/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* wizard code - inspired by rogue code from Merlyn Leroy (digi-g!brian) */
/*	       - heavily modified to give the wiz balls.  (genat!mike)   */
/*	       - dewimped and given some maledictions. -3. */
/*	       - generalized for 3.1 (mike@bullns.on01.bull.ca) */

#include "hack.h"
#include "qtext.h"
#include "xhity.h"


extern const int monstr[];

#ifdef OVLB

STATIC_DCL short FDECL(which_arti, (long int));
STATIC_DCL struct monst *FDECL(other_mon_has_arti, (struct monst *,SHORT_P));
STATIC_DCL boolean FDECL(rightful_owner, (struct monst *,SHORT_P, struct monst *));
STATIC_DCL struct obj *FDECL(on_ground, (SHORT_P));
STATIC_DCL boolean FDECL(you_have, (long int));
STATIC_DCL long FDECL(target_on, (long int,struct monst *));
STATIC_DCL long FDECL(strategy, (struct monst *));
STATIC_DCL void FDECL(wizgush, (int, int, genericptr_t));
STATIC_DCL void NDECL(dowizgush);
STATIC_DCL void FDECL(aglaopesong, (struct monst *));

static int xprime = 0, yprime = 0;

static NEARDATA const int nasties[] = {
	/* neutral */
	PM_COCKATRICE, PM_ETTIN, PM_STALKER, PM_MINOTAUR, 
	PM_OWLBEAR, PM_PURPLE_WORM, PM_XAN, PM_UMBER_HULK, 
	PM_XORN, PM_LEOCROTTA, PM_BALUCHITHERIUM, PM_CARNIVOROUS_APE,
	PM_MASTER_MIND_FLAYER, PM_FIRE_ELEMENTAL, PM_JABBERWOCK, PM_DUNGEON_FERN,
	PM_IRON_GOLEM, PM_OCHRE_JELLY, PM_GREEN_SLIME, PM_JUBJUB_BIRD, 
	PM_BANDERSNATCH, PM_GIANT_TURTLE, PM_WATER_ELEMENTAL,
	
	/* chaotic */
	PM_RED_DRAGON, PM_BLACK_DRAGON, PM_ROCK_TROLL, PM_GREMLIN, 
	PM_VAMPIRE_LORD, PM_VAMPIRE_LADY, PM_WINGED_GARGOYLE, PM_ARCH_LICH, PM_SON_OF_TYPHON, 
	PM_OGRE_KING, PM_OLOG_HAI, PM_DISENCHANTER, PM_DISPLACER_BEAST, 
	PM_MANTICORE, PM_GNOLL, PM_SCRAP_TITAN, PM_GUG, 
	PM_ANUBAN_JACKAL, PM_BEBELITH, PM_WEREWOLF, PM_WERERAT,
	PM_DAUGHTER_OF_BEDLAM, PM_WALKING_DELIRIUM, PM_DAUGHTER_OF_NAUNET,
	
	/* lawful */
	PM_GREEN_DRAGON, PM_YELLOW_DRAGON, PM_ORANGE_DRAGON, PM_CAPTAIN,
	PM_ANCIENT_NAGA, PM_GUARDIAN_NAGA, PM_GOLDEN_NAGA, PM_SERPENT_NECKED_LIONESS, 
	PM_EDDERKOP, PM_LEGION_DEVIL_SERGEANT, PM_LEGION_DEVIL_CAPTAIN, PM_HELLFIRE_COLOSSUS, 
	PM_GREEN_STEEL_GOLEM, PM_MAID, PM_HELLCAT
	};

static NEARDATA const unsigned wizapp[] = {
	PM_HUMAN, PM_MARID, PM_VAMPIRE,
	PM_RED_DRAGON, PM_TROLL, PM_UMBER_HULK,
	PM_XORN, PM_XAN, PM_COCKATRICE,
	PM_FLOATING_EYE,
	PM_GUARDIAN_NAGA,
	PM_TRAPPER
};

#endif /* OVLB */
#ifdef OVL0

/* If you've found the Amulet, make the Wizard appear after some time */
/* Also, give hints about portal locations, if amulet is worn/wielded -dlc */
void
amulet()
{
	struct monst *mtmp;
	struct trap *ttmp;
	struct obj *amu;

#if 0		/* caller takes care of this check */
	if (!u.uhave.amulet)
		return;
#endif
	if ((((amu = uamul) != 0 && amu->otyp == AMULET_OF_YENDOR) ||
	     ((amu = uwep) != 0 && amu->otyp == AMULET_OF_YENDOR))
	    && !rn2(15)) {
	    for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
		if(ttmp->ttyp == MAGIC_PORTAL) {
		    int du = distu(ttmp->tx, ttmp->ty);
		    if (du <= 9)
			pline("%s hot!", Tobjnam(amu, "feel"));
		    else if (du <= 64)
			pline("%s very warm.", Tobjnam(amu, "feel"));
		    else if (du <= 144)
			pline("%s warm.", Tobjnam(amu, "feel"));
		    /* else, the amulet feels normal */
		    break;
		}
	    }
	}

	if (!flags.no_of_wizards)
		return;
	/* find Wizard, and wake him if necessary */
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp) && mtmp->iswiz && mtmp->msleeping && !rn2(40)) {
		mtmp->msleeping = 0;
		if (distu(mtmp->mx,mtmp->my) > 2)
		    You(
    "get the creepy feeling that somebody noticed your taking the Amulet."
		    );
		return;
	    }
}

#endif /* OVL0 */
#ifdef OVLB

int
mon_has_amulet(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == AMULET_OF_YENDOR) return(1);
	return(0);
}

int
mon_has_special(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == AMULET_OF_YENDOR ||
			is_quest_artifact(otmp) ||
			is_asc_obj(otmp)) return(1);
	return(0);
}

/*
 *	New for 3.1  Strategy / Tactics for the wiz, as well as other
 *	monsters that are "after" something (defined via mflag3).
 *
 *	The strategy section decides *what* the monster is going
 *	to attempt, the tactics section implements the decision.
 */
#define STRAT(w, x, y, typ) (w | ((long)(x)<<16) | ((long)(y)<<8) | (long)typ)

#define M_Wants(mask)	(mtmp->data->mflagst & (mask))

STATIC_OVL short
which_arti(mask)
	register long int mask;
{
	switch(mask) {
	    case MT_WANTSAMUL:	return(AMULET_OF_YENDOR);
	    case MT_WANTSBELL:	return(BELL_OF_OPENING);
	    case MT_WANTSCAND:	return(CANDELABRUM_OF_INVOCATION);
	    case MT_WANTSBOOK:	return(SPE_BOOK_OF_THE_DEAD);
	    default:		break;	/* 0 signifies quest artifact */
	}
	return(0);
}

/*
 *	If "otyp" is zero, it triggers a check for the quest_artifact,
 *	since bell, book, candle, and amulet are all objects, not really
 *	artifacts right now.	[MRS]
 */
boolean
mon_has_arti(mtmp, otyp)
	register struct monst *mtmp;
	register short	otyp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
	    if(otyp) {
			if(otmp->otyp == otyp)
				return(1);
	    }
	    else if(is_quest_artifact(otmp))
			return(1);
	}
	return(0);

}

STATIC_OVL struct monst *
other_mon_has_arti(mtmp, otyp)
	struct monst *mtmp;
	int otyp;
{
	register struct monst *mtmp2;

	for(mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon)
	    /* no need for !DEADMONSTER check here since they have no inventory */
	    if(mtmp2 != mtmp)
		if(mon_has_arti(mtmp2, otyp)) return(mtmp2);

	return((struct monst *)0);
}

STATIC_OVL boolean
rightful_owner(holder, otyp, seeker)
	register struct monst *holder;
	register short	otyp;
	register struct monst *seeker;
{
	switch(otyp){
		case AMULET_OF_YENDOR:
			//Note: the Wizard won't steal from the EP (but demon lords will)
			// Monsters won't attack the wizard (but the wizard will squable with himself)
			if((holder->mtyp == PM_ELDER_PRIEST && seeker->iswiz) 
			  || (holder->iswiz && !seeker->iswiz)
			)
				return TRUE;
		break;
		case BELL_OF_OPENING:
			if(holder->data->msound == MS_NEMESIS)
				return TRUE;
			if(is_Rebel(holder->data) && is_Rebel(seeker->data))
				return TRUE;
		break;
		case CANDELABRUM_OF_INVOCATION:
			if(holder->mtyp == PM_VLAD_THE_IMPALER)
				return TRUE;
		break;
		case SPE_BOOK_OF_THE_DEAD:
			if(holder->iswiz)
				return TRUE;
		break;
	}
	return FALSE;
}

STATIC_OVL struct obj *
on_ground(otyp)
	register short	otyp;
{
	register struct obj *otmp;

	for (otmp = fobj; otmp; otmp = otmp->nobj){
	    if (otyp) {
			if (otmp->otyp == otyp)
				return(otmp);
	    }
		else if (is_quest_artifact(otmp))
			return(otmp);
	}
	return((struct obj *)0);
}

STATIC_OVL boolean
you_have(mask)
	register long int mask;
{
	switch(mask) {
	    case MT_WANTSAMUL:	return(boolean)(u.uhave.amulet);
	    case MT_WANTSBELL:	return(boolean)(u.uhave.bell);
	    case MT_WANTSCAND:	return(boolean)(u.uhave.menorah);
	    case MT_WANTSBOOK:	return(boolean)(u.uhave.book);
	    case MT_WANTSARTI:	return(boolean)(u.uhave.questart);
	    default:		break;
	}
	return(0);
}

STATIC_OVL long
target_on(mask, mtmp)
	register long int mask;
	register struct monst *mtmp;
{
	int otyp;
	struct obj *otmp;
	struct monst *mtmp2;

	if(!M_Wants(mask))	return(STRAT_NONE);

	otyp = which_arti(mask);
	if(!mon_has_arti(mtmp, otyp)) {
	    if(you_have(mask))
		return(STRAT(STRAT_PLAYER, u.ux, u.uy, mask));
	    else if((otmp = on_ground(otyp))){
			return(STRAT(STRAT_GROUND, otmp->ox, otmp->oy, mask));
	    } else if((mtmp2 = other_mon_has_arti(mtmp, otyp)) && !rightful_owner(mtmp2, otyp, mtmp))
		return(STRAT(STRAT_MONSTR, mtmp2->mx, mtmp2->my, mask));
	}
	return(STRAT_NONE);
}

STATIC_OVL long
strategy(mtmp)
	register struct monst *mtmp;
{
	long strat, dstrat;

	if (!(is_covetous(mtmp->data) || mtmp->mtyp == PM_GREAT_CTHULHU) ||
		/* perhaps a shopkeeper has been polymorphed into a master
		   lich; we don't want it teleporting to the stairs to heal
		   because that will leave its shop untended */
		(mtmp->isshk && inhishop(mtmp)))
	    return STRAT_NONE;

	switch((mtmp->mhp*3)/mtmp->mhpmax) {	/* 0-3 */

	   default:
	    case 0:	/* panic time - mtmp is almost snuffed */
			return(STRAT_HEAL);

	    case 1:	/* the wiz is less cautious */
			if(mtmp->mtyp != PM_WIZARD_OF_YENDOR)
			    return(STRAT_HEAL);
			/* else fall through */

	    case 2:	dstrat = STRAT_HEAL;
			break;

	    case 3:	dstrat = STRAT_NONE;
			break;
	}
	
	//I don't think the MT_ matters (other than it can't be 0), but the bell is thematic for Cthulhu anyway
	if(mtmp->mtyp == PM_GREAT_CTHULHU){
		if(rn2(4) && rn2(mtmp->mhpmax) < mtmp->mhp)
			return STRAT(STRAT_PLAYER, u.ux, u.uy, MT_WANTSBELL);
		else
			return dstrat;
	}
	
	if(flags.made_amulet)
	    if((strat = target_on(MT_WANTSAMUL, mtmp)) != STRAT_NONE)
			return(strat);

	if(u.uevent.invoked) {		/* priorities change once gate opened */

	    if((strat = target_on(MT_WANTSARTI, mtmp)) != STRAT_NONE)
			return(strat);
	    if((strat = target_on(MT_WANTSBOOK, mtmp)) != STRAT_NONE)
			return(strat);
	    if((strat = target_on(MT_WANTSBELL, mtmp)) != STRAT_NONE)
			return(strat);
	    if((strat = target_on(MT_WANTSCAND, mtmp)) != STRAT_NONE)
			return(strat);
	} else {

	    if((strat = target_on(MT_WANTSBOOK, mtmp)) != STRAT_NONE)
			return(strat);
	    if((strat = target_on(MT_WANTSBELL, mtmp)) != STRAT_NONE)
			return(strat);
	    if((strat = target_on(MT_WANTSCAND, mtmp)) != STRAT_NONE)
			return(strat);
	    if((strat = target_on(MT_WANTSARTI, mtmp)) != STRAT_NONE)
			return(strat);
	}
	return(dstrat);
}

int
tactics(mtmp)
	register struct monst *mtmp;
{
	long strat = strategy(mtmp);

	mtmp->mstrategy = (mtmp->mstrategy & STRAT_WAITMASK) | strat;

	switch (strat) {
	    case STRAT_HEAL:	/* hide and recover */
		if(mtmp->mtyp == PM_GREAT_CTHULHU){
			if(canseemon(mtmp))
				pline("Noxious gasses swirl around %s!", mon_nam(mtmp));
			create_gas_cloud(mtmp->mx, mtmp->my, 2, 30, FALSE);
			mtmp->mhp = mtmp->mhpmax;
			mtmp->mflee = 0;
			mtmp->mfleetim = 0;
			return 1;
		}
		else if(mtmp->mtyp == PM_JRT_NETJER && !mtmp->mpeaceful && !mtmp->mcan && distmin(u.ux, u.uy, mtmp->mx, mtmp->my) > 1){
		// else if(mtmp->mtyp == PM_JRT_NETJER && !mtmp->mpeaceful && !mtmp->mcan && distmin(u.ux, u.uy, mtmp->mx, mtmp->my) > 1 && !rn2(3)){
			if(!Blind)
				pline("Stars swirl around you.");
			int type;
			int ox = mtmp->mx, oy = mtmp->my;
			struct monst *tmpm;
			if(rn2(6)){
				type = PM_MOTE_OF_LIGHT;
			}
			else {
				if(dungeon_topology.alt_tulani == CAILLEA_CASTE)
					type = PM_MOONSHADOW;
				else
					type = PM_BALL_OF_RADIANCE;
			}
			tmpm = makemon(&mons[type], u.ux, u.uy, NO_MINVENT|MM_ADJACENTOK|MM_NOCOUNTBIRTH);
			if(tmpm){
				struct obj *otmp;
				int ctype = counter_were(type);
				if(canseemon(tmpm))
					pline("The stars coalesce into %s!", an(mons[type].mname));
				otmp = mksobj(KHOPESH, MKOBJ_NOINIT);
				set_material_gm(otmp, COPPER);
				add_oprop(otmp, OPROP_HOLYW);
				otmp->objsize = mons[ctype].msize;
				fix_object(otmp);
				(void) mpickobj(tmpm, otmp);


				if(type != PM_BALL_OF_RADIANCE){
					otmp = mksobj(KHOPESH, MKOBJ_NOINIT);
					set_material_gm(otmp, COPPER);
					add_oprop(otmp, OPROP_HOLYW);
					otmp->objsize = mons[ctype].msize;
					fix_object(otmp);
					(void) mpickobj(tmpm, otmp);
				}
				otmp = mksobj(ARCHAIC_HELM, MKOBJ_NOINIT);
				set_material_gm(otmp, COPPER);
				add_oprop(otmp, OPROP_HOLY);
				otmp->objsize = mons[ctype].msize;
				fix_object(otmp);
				(void) mpickobj(tmpm, otmp);
				
				otmp = mksobj(WAISTCLOTH, MKOBJ_NOINIT);
				set_material_gm(otmp, CLOTH);
				otmp->obj_color = CLR_WHITE;
				add_oprop(otmp, OPROP_HOLY);
				otmp->objsize = mons[ctype].msize;
				fix_object(otmp);
				(void) mpickobj(tmpm, otmp);
			}
			if(has_template(mtmp, POISON_TEMPLATE))
				set_template(tmpm, POISON_TEMPLATE);
			else if(has_template(mtmp, MAD_TEMPLATE))
				set_template(tmpm, FALLEN_TEMPLATE);
			else if(has_template(mtmp, FALLEN_TEMPLATE))
				set_template(tmpm, FALLEN_TEMPLATE);
			if(mtmp->mfaction)
				set_faction(tmpm, mtmp->mfaction);
			return 1;
		}
		/* if wounded, hole up on or near the stairs (to block them) */
		/* unless, of course, there are no stairs (e.g. endlevel) */
		mtmp->mavenge = 1; /* covetous monsters attack while fleeing */
		if(mtmp->mtyp == PM_AGLAOPE){
			//don't move from current location, post-theft warp does that for you.
		} else {
			if(flags.stag && In_quest(&u.uz)){
				if (In_W_tower(mtmp->mx, mtmp->my, &u.uz) ||
					(mtmp->iswiz && !xdnstair && !mon_has_amulet(mtmp))) {
					if (!rn2(3 + mtmp->mhp/10)) (void) rloc(mtmp, TRUE);
				} else if (xdnstair &&
					 (mtmp->mx != xdnstair || mtmp->my != ydnstair)) {
					(void) mnearto(mtmp, xdnstair, ydnstair, TRUE);
				}
			} else {
				if (In_W_tower(mtmp->mx, mtmp->my, &u.uz) ||
					(mtmp->iswiz && !xupstair && !mon_has_amulet(mtmp))) {
					if (!rn2(3 + mtmp->mhp/10)) (void) rloc(mtmp, TRUE);
				} else if (xupstair &&
					 (mtmp->mx != xupstair || mtmp->my != yupstair)) {
					(void) mnearto(mtmp, xupstair, yupstair, TRUE);
				}
			}
		}
		/* if you're not around, cast healing spells */
		if (distu(mtmp->mx,mtmp->my) > (BOLT_LIM * BOLT_LIM) && !mtmp->mcan)
		    if(mtmp->mhp <= mtmp->mhpmax - 8) {
			mtmp->mhp += rnd(8);
			return(1);
		    }
		/* fall through :-) */

	    case STRAT_NONE:	/* harrass */
		if(mtmp->mtyp == PM_GREAT_CTHULHU){
			if(!DimensionalLock){
				struct monst *spawn;
				if(!Blind)
					pline("Noxious gasses swirl around you!");
				create_gas_cloud(u.ux, u.uy, 2, 30, FALSE);
				spawn = makemon(&mons[PM_STAR_SPAWN], u.ux, u.uy, MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
				if(spawn){
					mark_mon_as_summoned(spawn, mtmp, ESUMMON_PERMANENT, 0);
				}
			}
			return 0;
		}
		if (!rn2((!mtmp->mflee || mtmp->mtyp == PM_BANDERSNATCH) ? 5 : 33)){
			if(mtmp->mtyp == PM_AGLAOPE){
				coord cc;
				aglaopesong(mtmp);
				pline("%s's song warps space to draw you together.",Monnam(mtmp));
				if( tt_findadjacent(&cc, mtmp) ){
					teleds(cc.x, cc.y, FALSE);
					return(0);
				}
			}
			if(mtmp->mtyp == PM_CHAOS){
				mnexto(mtmp);
			}
			else if(mtmp->mtame){
				mnexto(mtmp);
			}
			else if(mtmp->mtyp == PM_GRAND_MASTER
				|| mtmp->mtyp == PM_MASTER_KAEN
			){
				monline(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			}
			else if(mtmp->mtyp == PM_JRT_NETJER){
				if(!mtmp->mcan && !rn2(3))
					mofflin(mtmp);
				else if(!mtmp->mcan && rn2(2))
					mofflin_close(mtmp);
				else
					mnexto(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			}
			else if((attacktype_fordmg(mtmp->data, AT_BREA, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_SPIT, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_ARRW, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_BEAM, AD_ANY) )
				&& !mtmp->mcan && !mtmp->mspec_used
			){
				if(attacktype_fordmg(mtmp->data, AT_ARRW, AD_SLVR)
					&& !has_object_type(mtmp->minvent, ARROW)
					&& !has_object_type(mtmp->minvent, SILVER_ARROW)
					&& !has_object_type(mtmp->minvent, GOLDEN_ARROW)
					&& !has_object_type(mtmp->minvent, ANCIENT_ARROW)
				)
					mnexto(mtmp);
				else {
					monline(mtmp);
					if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
				}
			} else if(attacktype_fordmg(mtmp->data, AT_LRCH, AD_ANY)
				|| attacktype_fordmg(mtmp->data, AT_LNCK, AD_ANY)
				|| attacktype_fordmg(mtmp->data, AT_BRSH, AD_ANY)
			){
				mofflin_close(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			} else if((attacktype_fordmg(mtmp->data, AT_MMGC, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_MAGC, AD_ANY) )
				&& !mtmp->mcan && !mtmp->mspec_used
			){
				mofflin(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			} else {
				mnexto(mtmp);
			}
		}
		return(0);

	    default:		/* kill, maim, pillage! */
	    {
		long  where = (strat & STRAT_STRATMASK);
		xchar tx = STRAT_GOALX(strat),
		      ty = STRAT_GOALY(strat);
		int   targ = strat & STRAT_GOAL;
		struct obj *otmp;

		if(!targ) { /* simply wants you to close */
		    return(0);
		}
		if((u.ux == tx && u.uy == ty) || where == STRAT_PLAYER) {
		    /* player is standing on it (or has it) */
			if(mtmp->mtyp == PM_AGLAOPE){
				coord cc;
				aglaopesong(mtmp);
				pline("%s's song warps space to draw you together.",Monnam(mtmp));
				if( tt_findadjacent(&cc, mtmp) ){
					teleds(cc.x, cc.y, FALSE);
					return(0);
				}
			} else if(mtmp->mtyp == PM_GREAT_CTHULHU){
				if(DimensionalLock) /* Block Cthulhu's relocation */
					return 0;
				boolean saw = FALSE;
				if(sensemon(mtmp) || canseemon(mtmp)){
					pline("%s steps through strange angles.",Monnam(mtmp));
					u.umadness |= MAD_NON_EUCLID;
					saw = TRUE;
				}
				mofflin(mtmp);
				if(!saw && (sensemon(mtmp) || canseemon(mtmp))){
					pline("%s steps through strange angles.",Monnam(mtmp));
					u.umadness |= MAD_NON_EUCLID;
				}
				return 1;
			}
			if(mtmp->mtyp == PM_CHAOS){
				mnexto(mtmp);
			}
			else if(mtmp->mtame){
				mnexto(mtmp);
			}
			else if(mtmp->mtyp == PM_GRAND_MASTER
				|| mtmp->mtyp == PM_MASTER_KAEN
			){
				monline(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			}
			else if(mtmp->mtyp == PM_JRT_NETJER){
				if(!mtmp->mcan && !rn2(3))
					mofflin(mtmp);
				else if(!mtmp->mcan && rn2(2))
					mofflin_close(mtmp);
				else
					mnexto(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			}
			else if((attacktype_fordmg(mtmp->data, AT_BREA, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_SPIT, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_ARRW, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_BEAM, AD_ANY) )
				&& !mtmp->mcan && !mtmp->mspec_used
			){
				if(attacktype_fordmg(mtmp->data, AT_ARRW, AD_SLVR)
					&& !has_object_type(mtmp->minvent, ARROW)
					&& !has_object_type(mtmp->minvent, SILVER_ARROW)
					&& !has_object_type(mtmp->minvent, GOLDEN_ARROW)
					&& !has_object_type(mtmp->minvent, ANCIENT_ARROW)
				)
					mnexto(mtmp);
				else {
					monline(mtmp);
					if(!mon_can_see_you(mtmp)) mnexto(mtmp);
				}
			} else if(attacktype_fordmg(mtmp->data, AT_LRCH, AD_ANY)
				|| attacktype_fordmg(mtmp->data, AT_LNCK, AD_ANY)
				|| attacktype_fordmg(mtmp->data, AT_BRSH, AD_ANY)
			){
				mofflin_close(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			} else if((attacktype_fordmg(mtmp->data, AT_MMGC, AD_ANY) ||
				attacktype_fordmg(mtmp->data, AT_MAGC, AD_ANY) )
				&& !mtmp->mcan && !mtmp->mspec_used
			){
				mofflin(mtmp);
				if(!mon_can_see_you(mtmp) || !couldsee(mtmp->mx, mtmp->my)) mnexto(mtmp);
			} else {
				mnexto(mtmp);
			}
		    return(0);
		}
		if(where == STRAT_GROUND) {
		    if(!MON_AT(tx, ty) || (mtmp->mx == tx && mtmp->my == ty)) {
			/* teleport to it and pick it up */
			rloc_to(mtmp, tx, ty);	/* clean old pos */

			if ((otmp = on_ground(which_arti(targ))) != 0) {
			    if (cansee(mtmp->mx, mtmp->my))
				pline("%s picks up %s.",
				    Monnam(mtmp),
				    (distu(mtmp->mx, mtmp->my) <= 5) ?
				     doname(otmp) : distant_name(otmp, doname));
			    obj_extract_self(otmp);
			    (void) mpickobj(mtmp, otmp);
			    return(1);
			} else return(0);
		    } else {
			/* a monster is standing on it - cause some trouble */
			if (!rn2(5)) mnexto(mtmp);
			return(0);
		    }
	        } else { /* a monster has it - 'port beside it. */
		    (void) mnearto(mtmp, tx, ty, FALSE);
		    return(0);
		}
	    }
	}
	/*NOTREACHED*/
	return(0);
}

void
aggravate()
{
	register struct monst *mtmp;

	if(uarmc && uarmc->oartifact == ART_MANTLE_OF_WRATH)
		return;
	
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp)) {
			mtmp->msleeping = 0;
			if(!mtmp->mcanmove && !rn2(5)) {
				mtmp->mfrozen = 0;
				if(mtmp->mtyp != PM_GIANT_TURTLE || !(mtmp->mflee))
					mtmp->mcanmove = 1;
			}
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
	    }
}

void
clonewiz()
{
	register struct monst *mtmp2;

	if ((mtmp2 = makemon(&mons[PM_WIZARD_OF_YENDOR],
				u.ux, u.uy, NO_MM_FLAGS)) != 0) {
	    mtmp2->msleeping = mtmp2->mtame = mtmp2->mpeaceful = 0;
	    if (!u.uhave.amulet && rn2(2)) {  /* give clone a fake */
		(void) add_to_minv(mtmp2, mksobj(FAKE_AMULET_OF_YENDOR, NO_MKOBJ_FLAGS));
	    }
	    mtmp2->m_ap_type = M_AP_MONSTER;
	    mtmp2->mappearance = wizapp[rn2(SIZE(wizapp))];
	    newsym(mtmp2->mx,mtmp2->my);
	}
}

/* also used by newcham() */
int
pick_nasty()
{
	int tries = 0;
	int mndx;
	do{
		mndx = nasties[rn2(SIZE(nasties))];
	} while(mvitals[mndx].mvflags & G_GONE && tries++ < 500);
    /* To do?  Possibly should filter for appropriate forms when
       in the elemental planes or surrounded by water or lava. */
    return mndx;
}

/* create some nasty monsters, aligned or neutral with the caster */
/* a null caster is assumed to be Wizard intervention */
int
nasty(mcast)
	struct monst *mcast;
{
    register struct monst *mtmp;
    register int i, j, tmp;
    int castalign = (mcast ? mcast->data->maligntyp : -1);
    coord bypos;
    int count=0;

    if(!rn2(10) && Inhell) {
		/* creatures made this way are full monsters gated in, not summons tied to mcast */
		msummon(mcast, (struct permonst *) 0);	/* summons like WoY */
		count++;
    } else {
	tmp = (u.ulevel > 3) ? u.ulevel/3 : 1; /* just in case -- rph */
	/* if we don't have a casting monster, the nasties appear around you */
	bypos.x = u.ux;
	bypos.y = u.uy;
	for(i = rnd(tmp); i > 0; --i)
	    for(j=0; j<20; j++) {
			int makeindex;

			/* Don't create more spellcasters of the monsters' level or
			 * higher--avoids chain summoners filling up the level.
			 */
			do {
				makeindex = pick_nasty();
			} while(mcast && attacktype(&mons[makeindex], AT_MAGC) &&
				monstr[makeindex] >= monstr[mcast->mtyp]);
			/* do this after picking the monster to place */
			if (mcast &&
				!enexto(&bypos, mcast->mux, mcast->muy, &mons[makeindex]))
				continue;
			if(mvitals[makeindex].mvflags & G_GENOD)
				continue;
			if(castalign == A_NONE || mons[makeindex].maligntyp == A_NONE 
				|| sgn(mons[makeindex].maligntyp) == sgn(castalign)
				|| (!rn2(4) && abs(sgn(mons[makeindex].maligntyp) - sgn(castalign)) == 1)
			){
				if ((mtmp = makemon(&mons[makeindex],
							bypos.x, bypos.y, MM_ESUM)) != 0);
				else /* makemon failed for some reason */
					mtmp = makemon((struct permonst *)0,
							bypos.x, bypos.y, MM_ESUM);
				if(!mtmp) /* makemon still failed, abort */
					return count;
				mtmp->msleeping = 0;
				untame(mtmp, 0);
				set_malign(mtmp);
				mark_mon_as_summoned(mtmp, mcast, ESUMMON_PERMANENT, 0);
				count++;
				break;
			}
	    }
    }
    return count;
}

/* create some yellow sign monsters */
void
yellow_nasty()
{
    register struct monst *mtmp;
    register int i, j, tmp, makeindex, maketemplate = YELLOW_TEMPLATE;
    coord bypos;
	tmp = (u.ulevel > 3) ? u.ulevel/3 : 1; /* just in case -- rph */
	for(i = rnd(tmp); i > 0; --i){
		switch(rn2(11)){
			case 0:
				makeindex = PM_BYAKHEE;
				maketemplate = 0;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 1:
				makeindex = PM_COILING_BRAWN;
				maketemplate = 0;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 2:
				makeindex = PM_FUNGAL_BRAIN;
				maketemplate = 0;
				bypos.x = 0;
				bypos.y = 0;
			break;
			case 3:
				makeindex = PM_SKELETON;
				maketemplate = 0;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 4:
				makeindex = Race_if(PM_ELF) ? PM_STAR_ELF : PM_HUMAN;
				maketemplate = DREAM_LEECH;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 5:
				makeindex = rn2(2) ? PM_MADMAN : PM_MADWOMAN;
				maketemplate = YELLOW_TEMPLATE;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 6:
				makeindex = PM_PURPLE_WORM;
				maketemplate = PSURLON;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 7:
				makeindex = PM_LONG_WORM;
				maketemplate = PSURLON;
				bypos.x = 0;
				bypos.y = 0;
			break;
			case 8:
				makeindex = PM_FLAXEN_STARSHADOW;
				maketemplate = 0;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 9:
				makeindex = PM_BARD;
				maketemplate = YELLOW_TEMPLATE;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
			case 10:
				makeindex = PM_CARCOSAN_COURTIER;
				maketemplate = 0;
				bypos.x = u.ux;
				bypos.y = u.uy;
			break;
		}
		if ((mtmp = makemon(&mons[makeindex],
					bypos.x, bypos.y, 0)) != 0);
		else /* makemon failed for some reason */
			continue;
		if(maketemplate)
			set_template(mtmp, maketemplate);
		if(mtmp->m_lev < 15){
			mtmp->m_lev = 15;
			mtmp->mhpmax = d(15,hd_size(mtmp->data));
			mtmp->mhp = mtmp->mhpmax;
		}
		mtmp->msleeping = 0;
		untame(mtmp, 0);
		set_faction(mtmp, YELLOW_FACTION);
		set_malign(mtmp);
	}
}

/* create some yellow sign monsters */
void
yellow_dead()
{
    register struct monst *mtmp;
    register int makeindex, maketemplate = 0;
	int zombiepms[] ={PM_HUMAN, PM_ELF, PM_DROW, PM_DWARF, PM_HOBBIT, PM_HALF_DRAGON, PM_CHIROPTERAN, PM_KOBOLD, PM_GNOME, PM_ORC, PM_ETTIN, PM_GIANT};
    coord bypos;
	switch(rn2(6)){
		case 0:
			makeindex = PM_COILING_BRAWN;
			maketemplate = 0;
			bypos.x = u.ux;
			bypos.y = u.uy;
		break;
		case 1:
			makeindex = PM_FUNGAL_BRAIN;
			maketemplate = 0;
			bypos.x = 0;
			bypos.y = 0;
		break;
		case 2:
			makeindex = Race_if(PM_ELF) ? PM_STAR_ELF : PM_HUMAN;
			maketemplate = DREAM_LEECH;
			bypos.x = u.ux;
			bypos.y = u.uy;
		break;
		default:
			makeindex = zombiepms[rn2(SIZE(zombiepms))];
			maketemplate = YELLOW_TEMPLATE;
			bypos.x = u.ux;
			bypos.y = u.uy;
		break;
	}
	mtmp = makemon(&mons[makeindex], bypos.x, bypos.y, 0);
	/* makemon failed for some reason */
	if(!mtmp)
		return;
	if(maketemplate)
		set_template(mtmp, maketemplate);
	if(mtmp->m_lev < 15){
		mtmp->m_lev = 15;
		mtmp->mhpmax = d(15,hd_size(mtmp->data));
		mtmp->mhp = mtmp->mhpmax;
	}
	mtmp->msleeping = 0;
	mtmp->mpeaceful = 0;
	set_faction(mtmp, YELLOW_FACTION);
	set_malign(mtmp);
}

boolean
u_recognize_stars()
{
	if(Role_if(PM_MADMAN))
		return TRUE;
	// Particularly outdoors roles, those that navigate by the stars, or those that study roles that do.
	if(Role_if(PM_ARCHEOLOGIST) || Role_if(PM_PIRATE) || Role_if(PM_CAVEMAN) || Role_if(PM_BARBARIAN))
		return ACURR(A_INT) > 11;
	// Celestial roles and those that CAN navigate by the stars (asuming that they live above ground).
	if(Role_if(PM_WIZARD) || Role_if(PM_PRIEST) || Role_if(PM_RANGER))
		return (Race_if(PM_DROW) || Race_if(PM_DWARF) || Race_if(PM_GNOME)) ? ACURR(A_INT) > 18 : ACURR(A_INT) > 13;
	// Anas
	if(Role_if(PM_ANACHRONONAUT) || Role_if(PM_UNDEAD_HUNTER))
		return ACURR(A_INT) > 13;

	// Catch all
	return (Race_if(PM_DROW) || Race_if(PM_DWARF) || Race_if(PM_GNOME)) ? FALSE : ACURR(A_INT) > 15;
}

/* The king smites you in one of a variety of ways */
boolean
yellow_smite()
{
	int dmg;
	struct monst *magr;
	struct attack attkbuff = {AT_MAGC, AD_CLRC, 0, 6};
	for(magr = fmon; magr; magr = magr->nmon){
		if(magr->mpeaceful
			|| nonthreat(magr)
			|| magr->msleeping
			|| !magr->mcanmove
			|| (magr->mstrategy & STRAT_WAITMASK)
		)
			continue;
		if(mon_can_see_you(magr))
			break;
	}
	if(!magr){
		return FALSE;
	}
	if(canseemon(magr))
		You("glimpse %s over %s %s!", 
			u_recognize_stars() ? "the five flaxen sisters" : "five amber stars", 
			s_suffix(mon_nam(magr)), 
			mbodypart(magr, HEAD));

	switch(rnd(11)){
		case 1:
			if (!Punished) {
				punish((struct obj *)0);
			}
			else {
				Your("iron ball gets heavier!");
				uball->owt += 160;
			}
		break;
		case 2:
			if (!Sick_resistance && !umechanoid) {
				if(!Blind)
					You("are afflicted with jaundice!");
				else
					You("are afflicted with disease!");
				make_sick(Sick ? Sick / 3L + 1L : (long)rn1(ACURR(A_CON), 20),
					"the king in yellow", TRUE, SICK_NONVOMITABLE);
			}
			else {
				You_feel("slightly infectious.");
			}
		break;
		case 3:
			cast_spell(magr, &youmonst, &attkbuff, DRAIN_LIFE, u.ux, u.uy);
		break;
		case 4:
			//wounds
			dmg = d(10,5);
			if (Antimagic)
				dmg = (dmg + 1) / 2;
			dmg = reduce_dmg(&youmonst,dmg,FALSE,TRUE);
			if (dmg >= *hp(&youmonst)) {
				Your("body is covered with deadly wounds!");
				dmg = max(*hp(&youmonst) - 5, 0);	/* Cap the damage */
			}
			else if (dmg <= 5)
				Your("skin itches badly for a moment.");
			else if (dmg <= 15)
				pline("Wounds appear on your body!");
			else if (dmg <= 30)
				pline("Severe wounds appear on your body!");
			else
				Your("body is covered with deep, painful wounds!");
			losehp(dmg, "the king in yellow", KILLED_BY);
		break;
		case 5:
			//nightmare
			dmg = rnd(d(5,15));
			if (Antimagic)
				dmg = (dmg + 1) / 2;
			dmg = reduce_dmg(&youmonst,dmg,FALSE,TRUE);
			You_hear("menacing laughter as the world blurs around you...");
			make_confused(HConfusion + dmg * 10, FALSE);
			make_stunned(HStun + dmg, FALSE);
			make_hallucinated(HHallucination + dmg * 15, FALSE, 0L);
			stop_occupation();
		break;
		case 6:
			//death
			/* message */
			You_feel("a yellow death descend upon you!");
			/* check resistance cases and do effects */
			if (nonliving(youracedata) || is_demon(youracedata)) {
				You("seem no deader than before.");
				dmg = 0;
			}
			else if (ward_at(u.ux, u.uy) == CIRCLE_OF_ACHERON) {
				You("are already beyond Acheron.");
				dmg = 0; 
			}
			else if (u.sealsActive & SEAL_OSE) {
				shieldeff(u.ux, u.uy);
				dmg = 0;
			}
			else {
				/* split between vs player and vs monster */
				if (!Antimagic && (rn2(20) > 12)) {
					if (Hallucination) {
						You("have an out of body experience.");
						dmg = 0;
					}
					else if (*hp(&youmonst) >= 100){
						Your("%s stops!  When it finally beats again, it is weak and thready.", body_part(HEART));
						dmg = d(8, 8);
					}
					else {
						killer_format = KILLED_BY;
						killer = "the yellow death";
						dmg = *hp(&youmonst);
					}
				}
				else {
					if (Antimagic)
						shieldeff(u.ux, u.uy);
					Your("%s flutters!", body_part(HEART));
					dmg = 8; //you still take damage
				}
			}
			losehp(dmg, "the yellow death", KILLED_BY);
		break;
		case 7:{
			//buff monsters
			for(struct monst *mon = fmon; mon; mon = mon->nmon){
				if(mon->mpeaceful
					|| nonthreat(mon)
					|| mon->msleeping
					|| !mon->mcanmove
					|| DEADMONSTER(mon)
					|| !(mon->mstrategy & STRAT_WAITMASK)
				)
					continue;
				mon->encouraged += 5;
			}
		}break;
		case 8:
			//poison (Con)
			cast_spell(magr, &youmonst, &attkbuff, MON_POISON_GAS, u.ux, u.uy);
		break;
		case 9:
			//Acid rain
			cast_spell(magr, &youmonst, &attkbuff, ACID_RAIN, u.ux, u.uy);
		break;
		case 10:
			//Fire
			cast_spell(magr, &youmonst, &attkbuff, FIRE_PILLAR, u.ux, u.uy);
		break;
		case 11:
			//Cold
			cast_spell(magr, &youmonst, &attkbuff, ICE_STORM, u.ux, u.uy);
		break;
	}
	grow_up(magr,(struct monst *)0);
	return TRUE;
}

/*	Let's resurrect the wizard, for some unexpected fun.	*/
void
resurrect()
{
	struct monst *mtmp, **mmtmp;
	long elapsed;
	const char *verb;

	if (!flags.no_of_wizards) {
	    /* make a new Wizard */
	    verb = "kill";
	    mtmp = makemon(&mons[PM_WIZARD_OF_YENDOR], u.ux, u.uy, MM_NOWAIT);
	} else {
	    /* look for a migrating Wizard */
	    verb = "elude";
	    mmtmp = &migrating_mons;
	    while ((mtmp = *mmtmp) != 0) {
		if (mtmp->iswiz &&
			/* if he has the Amulet, he won't bring it to you */
			!mon_has_amulet(mtmp) &&
			(elapsed = monstermoves - mtmp->mlstmv) > 0L
		){
		    mon_catchup_elapsed_time(mtmp, elapsed);
		    if (elapsed >= LARGEST_INT) elapsed = LARGEST_INT - 1;
		    elapsed /= 50L;
		    if (mtmp->msleeping && rn2((int)elapsed + 1))
			mtmp->msleeping = 0;
		    if (mtmp->mfrozen == 1) /* would unfreeze on next move */
			mtmp->mfrozen = 0,  mtmp->mcanmove = 1;
		    if (mtmp->mcanmove && !mtmp->msleeping) {
			*mmtmp = mtmp->nmon;
			mon_arrive(mtmp, TRUE);
			/* note: there might be a second Wizard; if so,
			   he'll have to wait til the next resurrection */
			break;
		    }
		}
		mmtmp = &mtmp->nmon;
	    }
	}

	if (mtmp) {
		mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
		set_malign(mtmp);
		pline("A voice booms out...");
		verbalize("So thou thought thou couldst %s me, fool.", verb);
	}

}

void
illur_resurrect()
{
	struct monst *mtmp, **mmtmp;
	long elapsed;

	/* look for a Illurien on the same level */
	mmtmp = &fmon;
	while ((mtmp = *mmtmp) != 0) {
		if (mtmp->mtyp==PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES)
			return; /*Illurine is currently here */
		mmtmp = &mtmp->nmon;
	}
	/* look for a migrating Illurien */
	mmtmp = &migrating_mons;
	while ((mtmp = *mmtmp) != 0) {
		if (mtmp->mtyp==PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES) {
			if((elapsed = monstermoves - mtmp->mlstmv) > 0L){
				mon_catchup_elapsed_time(mtmp, elapsed);
				if (elapsed >= LARGEST_INT) elapsed = LARGEST_INT - 1;
				elapsed /= 50L;
			} else elapsed = 1;
			if (mtmp->msleeping && rn2((int)elapsed + 1))
				mtmp->msleeping = 0;
			if (mtmp->mfrozen == 1) /* would unfreeze on next move */
				mtmp->mfrozen = 0,  mtmp->mcanmove = 1;
			if (mtmp->mcanmove && !mtmp->msleeping) {
				*mmtmp = mtmp->nmon;
				mon_arrive(mtmp, TRUE);
				break;
			}
		}
		mmtmp = &mtmp->nmon;
	}
	
	if(!mtmp) mtmp = makemon(&mons[PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES], u.ux, u.uy, MM_NOWAIT|MM_NOCOUNTBIRTH);
	
	if (mtmp) {
		mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
		set_malign(mtmp);
		pline("A voice booms out...");
		verbalize("You thought to steal memories from ME, she of the Myriad Glimpses!?");
	}

}

void
coa_arrive()
{
	struct monst *mtmp, **mmtmp;
	long elapsed;

	/* look for a migrating CoAll */
	mmtmp = &migrating_mons;
	while ((mtmp = *mmtmp) != 0) {
		if (mtmp->mtyp==PM_CENTER_OF_ALL) {
			if((elapsed = monstermoves - mtmp->mlstmv) > 0L){
				mon_catchup_elapsed_time(mtmp, elapsed);
				if (elapsed >= LARGEST_INT) elapsed = LARGEST_INT - 1;
				elapsed /= 50L;
			} else elapsed = 1;
			mtmp->msleeping = 0;
			if (mtmp->mfrozen == 1) /* would unfreeze on next move */
				mtmp->mfrozen = 0,  mtmp->mcanmove = 1;
			if (mtmp->mcanmove) {
				*mmtmp = mtmp->nmon;
				mtmp->mtrack[0].x = MIGR_RANDOM;
				mon_arrive(mtmp, FALSE);
				break;
			}
		}
		mmtmp = &mtmp->nmon;
	}
	
	if(!mtmp && mvitals[PM_CENTER_OF_ALL].born == 0) mtmp = makemon(&mons[PM_CENTER_OF_ALL], 0, 0, MM_NOWAIT);
	
	if (mtmp && (In_endgame(&u.uz) || Insight > 8)) {
		mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
		set_malign(mtmp);
	}

}

STATIC_PTR void
dowizdarken()
{
	register struct monst *mtmp;
	register struct trap *ttmp;
	int cx, cy;
	
	for(cx = 0; cx < COLNO; cx++){
		for(cy = 0; cy < ROWNO; cy++){
			levl[cx][cy].lit = 0;
			snuff_light_source(cx, cy);
			if(!rn2(3)) switch(rn2(5)) {
				case 2:(void) makemon(&mons[PM_SHADE], cx, cy, NO_MM_FLAGS);break;
				case 3:(void) makemon(&mons[PM_BLACK_LIGHT], cx, cy, NO_MM_FLAGS);break;
				case 4:(void) makemon(&mons[PM_WRAITH], cx, cy, NO_MM_FLAGS);break;
				case 5:(void) makemon(rn2(10) ? &mons[PM_SHADE] : &mons[PM_DARKNESS_GIVEN_HUNGER], cx, cy, NO_MM_FLAGS);break;
				default:(void) makemon(mkclass(S_WRAITH, G_NOHELL|G_HELL), cx, cy, NO_MM_FLAGS);break;
			}
		}
	}
	if (!Blind) {
	    vision_recalc(2);

	    /* replace ball&chain */
	    if (Punished)
		move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
	}
}

void
dowizgush() /* Gushing forth along LOS from (u.ux, u.uy) and random other spots */
{
	int madepool = 0,cx,cy;
	
	xprime = u.ux;
	yprime = u.uy;
	do_clear_area(u.ux, u.uy, 9, wizgush, (genericptr_t)&madepool);
	pline("Water sprays all over you.");
	water_damage(invent, FALSE, FALSE, level.flags.lethe, &youmonst);
	for(cx = 0; cx < COLNO; cx++){
		for(cy = 0; cy < ROWNO; cy++){
			if(!rn2(100)){
				madepool = 0;
				xprime = cx;
				yprime = cy;
				do_clear_area(cx, cy, 9, wizgush, (genericptr_t)&madepool);
			}
		}
	}
}

STATIC_PTR void
wizgush(cx, cy, poolcnt)
int cx, cy;
genericptr_t poolcnt;
{
	register struct monst *mtmp;
	register struct trap *ttmp;

	if (((cx+cy)%2) || 
	    (rn2(1 + distmin(xprime, yprime, cx, cy)))  ||
	    (levl[cx][cy].typ != ROOM) ||
	    (boulder_at(cx, cy)) || nexttodoor(cx, cy))
		return;

	if ((ttmp = t_at(cx, cy)) != 0 && !delfloortrap(ttmp))
		return;

	/* Put a pool at cx, cy */
	levl[cx][cy].typ = POOL;
	/* No kelp! */
	del_engr_ward_at(cx, cy);
	water_damage(level.objects[cx][cy], FALSE, TRUE, level.flags.lethe, (struct monst *) 0);

	if ((mtmp = m_at(cx, cy)) != 0){
		(void) minliquid(mtmp);
	}else{
		if(!rn2(3)) switch(rn2(In_hell(&u.uz) ? 7 : 5)) {
			case 2:(void) makemon(&mons[PM_KRAKEN], cx, cy, NO_MM_FLAGS);
			case 3:(void) makemon(&mons[PM_WATER_ELEMENTAL], cx, cy, NO_MM_FLAGS);
			case 4:(void) makemon(&mons[PM_DEEPER_ONE], cx, cy, NO_MM_FLAGS);
			case 5:(void) makemon(&mons[PM_MARID], cx, cy, NO_MM_FLAGS);
			case 6:(void) makemon(&mons[PM_UMBER_HULK], cx, cy, NO_MM_FLAGS);
			default:(void) makemon(mkclass(S_EEL, G_NOHELL|G_HELL), cx, cy, NO_MM_FLAGS);
		}
		newsym(cx,cy);
	}
}

/*	Here, we make trouble for the poor shmuck who actually	*/
/*	managed to do in the Wizard.				*/
void
intervene()
{
	if(In_void(&u.uz)) return;
	int which = Is_astralevel(&u.uz) ? rnd(4) : rn2(6);
	/* cases 0 and 5 don't apply on the Astral level */
	switch (which) {
	    case 0:
	    case 1:	You_feel("vaguely nervous.");
			break;
	    case 2:	if (!Blind)
			    You("notice a %s glow surrounding you.",
				  hcolor(NH_BLACK));
			rndcurse();
			break;
	    case 3:	aggravate();
			break;
	    case 4:	(void)nasty((struct monst *)0);
			break;
	    case 5:	resurrect();
			break;
	}
}

void
illur_intervene()
{
	if(Is_astralevel(&u.uz)) return;
	if(In_void(&u.uz)) return;
	switch (rnd(4)) {
	    case 1:	
	    case 2:
			You_feel("vaguely nervous.");
		break;
	    case 3:
			aggravate();
		break;
	    case 4:
			illur_resurrect();
		break;
	}
}

boolean
yello_intervene()
{
	switch (rnd(5)) {
	    case 1:	
			You_feel("vaguely nervous.");
			return TRUE;
	    case 2:
			if (!resists_blnd(&youmonst)) {
				You("are momentarily blinded by a flash of lemony light!");
				make_blinded((long)rn1(6,50),FALSE);
				if (!Blind) Your1(vision_clears);
			} else if (!Blind) {
				You("see a flash of lemony light!");
			}  else
				You_hear("a deafening roar!");
			yellow_nasty();
			return TRUE;
	    case 3:
			aggravate();
			return TRUE;
	    case 4:
			if (!Blind)
			    You("notice a %s glow surrounding you.",
				  hcolor(NH_YELLOW));
			rndcurse();
			return TRUE;
	    case 5:
			return yellow_smite();
	}
	return FALSE;
}

void
wizdead()
{
	flags.no_of_wizards--;
	if (!u.uevent.udemigod) {
		u.uevent.udemigod = TRUE;
		u.udg_cnt = rn1(250, 50);
	}
}

const char * const random_insult[] = {
	"antic",
	"blackguard",
	"caitiff",
	"chucklehead",
	"coistrel",
	"craven",
	"cretin",
	"cur",
	"dastard",
	"demon fodder",
	"dimwit",
	"dolt",
	"fool",
	"footpad",
	"imbecile",
	"knave",
	"maledict",
	"miscreant",
	"niddering",
	"poltroon",
	"rattlepate",
	"reprobate",
	"scapegrace",
	"varlet",
	"villein",	/* (sic.) */
	"wittol",
	"worm",
	"wretch",
};

const char * const random_malediction[] = {
	"Hell shall soon claim thy remains,",
	"I chortle at thee, thou pathetic",
	"Prepare to die, thou",
	"Resistance is useless,",
	"Surrender or die, thou",
	"There shall be no mercy, thou",
	"Thou shalt repent of thy cunning,",
	"Thou art as a flea to me,",
	"Thou art doomed,",
	"Thy fate is sealed,",
	"Verily, thou shalt be one dead"
};

const char * const random_garlandism[] = {
	"Cornelia will be mine!",
	"Thou art but a worthless lapdog of the King!",
	"Do you have any idea who you're messing with!?",
	"You really think you have what it takes to cross swords with ME?",
	"I, Garland, will knock you all down!!"
};
const char * const random_chaos_garlandism[] = {
	"Do you remember me!?",
	"Two thousand years from now...you killed me.\n  Now, it's my turn!",
	"I!  AM!  GAR! LAAAAND!",
	"Oh, you did defeat me then.  But now I shall kill you, and live forever!",
	"In two thousand years, I will remember none of this.",
	"I, Garland, will knock you all down!!"
};
const char * const random_chaosism[] = {
	"Do you remember me!?",																	//0
	"Two thousand years from now...you killed me.\n  Now, YOU will die!",					//1
	"Oh, you did defeat me then.  But now I shall kill you, and live FOREVER.",				//2
	"In two thousand years, I will remember none of this.",									//3
	"I, Chaos, will knock you all down!!",													//4
																							///
	"Many times have you kill me hence!",													//5
	"But I have ever been reborn here!",													//6
	"Many times have you followed me!",														//7
	"Many times, have you DIED!",															//8
	"So even as you die again and again, I shall return! Born forever into this endless circle that I have created, here, writ with your BLOOD!"	//9

};
const char * const random_apollyon[] = {
	"Tremble, mortal!", /*0*/
	"Tremble, mortal!", /*1*/
	"Tremble, mortal!", /*2*/
	"Kneel.", /*3*/
	"Bow before me!", /*4*/
	"You viper!", /*5*/
	"Cower, mortal!", /*6*/
	"Cower, mortal!", /*7*/
	"Tremble, mortal, you face the Angel of the Pit!", /*8*/
	"Bow before me, mortal.  For I am Apollyon.", /*9*/
	"You viper.  I shall sentence you to Gehennom!", /*10*/
	"Cower, mortal.  I am the Jailer of Heaven!" /*11*/
};
const char * const random_angeldiction[] = {
	"Weep, for %s has abandoned you!", /*0: Special handling needed (God name)*/
	"Your unholy soul shall be %s!", /*1: Special handling needed (cleansed/purged)*/
	"Tremble, mortal!", /*2*/
	"Kneel.", /*3*/
	"Bow before me!", /*4*/
	"You viper!", /*5*/
	"Cower, mortal!", /*6*/
	"Repent, sinner!", /*7*/
	"Kneel, or thou shalt be knelt!", /*8 (Wheel of Time reference?)*/
	"Repent and thou shalt be saved."  /*9*/
};
const char * const random_mirkwood[] = {
	"A sharp struggle, but worth it, I'll wager!", /*0*/
	"What nasty thick skin you have!", /*1*/
	"I'll wager there is good juice inside you!", /*2*/
	"You'll make fine eating, when you've been hung a bit!", /*3*/
	"You're not as fat as you might be. Been feeding none to well of late?", /*4*/
	"Alive and kicking? I'll soon put an end to that!", /*5*/
	"I'll kill you, and hang your corpse to soften!", /*6*/
	"You nasty little creature! I'll eat you and leave your bones and skin hanging on a tree!", /*7*/
	"Got a sting, have you? Well, I'll get you all the same!", /*8*/
	"I'll hang you head down for a day or two, that will take the fight out of you!" /*9*/
};
const char * const random_Ixoth[] = {
	"Hah!  Another puny knight seeks death.  I shall dine well tonight, then tomorrow, Arthur shall fall!",
	"Thou challengest me, knight?  So be it.  Thou wilt die here.",
	"Thou art truly foolish, sir.  I shall dispatch thee anon.",
	"A mere knight can never withstand me!",
	"I shall kill thee now, and feast!",
	"Puny knight.  What manner of death dost thou wish?",
	"First thee, knight, then I shall feast upon Arthur.",
	"Hah!  Thou hast failed, sir.  Now thou shalt die.",
	"Die, knight.  Thou art as nothing against my might.",
	"I shall suck the marrow from thy bones, knight.",
	"Let's see...  Baked?  No.  Fried?  Nay.  Broiled?  Yea verily, that is the way I like my knights for dinner.",
	"Thy strength waneth, sir.  The time of thy death draweth near.",
	"Call upon thy precious god, sir.  It shall not avail thee."
};
const char * const random_szcult[] = {
	"Derettash neeb evah sdog eth fo staruggiz eth!",
	"Hgin si dne eth!",
	"Dlrow eth fo esab eth ta Erutcarf a si ereth!",
	"Uoy emusnoc llash Erutcarf eth!",
	"Lla emusnoc llash Erutcarf eth!",
	"Gnidne si Noitatnemal fo emit eth!",
	"Snruter Tsol eth!",
	"Nrober neeb sah Tsol eth!"
};
/* Insult or intimidate the player */
void
cuss(mtmp)
register struct monst	*mtmp;
{
	if (mtmp->iswiz) {
	    if (!rn2(5))  /* typical bad guy action */
		pline("%s laughs fiendishly.", Monnam(mtmp));
	    else
		if (u.uhave.amulet && !rn2(SIZE(random_insult)))
		    verbalize("Relinquish the amulet, %s!",
			  random_insult[rn2(SIZE(random_insult))]);
		else if (u.uhp < 5 && !rn2(2))	/* Panic */
		    verbalize(rn2(2) ?
			  "Even now thy life force ebbs, %s!" :
			  "Savor thy breath, %s, it be thy last!",
			  random_insult[rn2(SIZE(random_insult))]);
		else if (mtmp->mhp < 5 && !rn2(2))	/* Parthian shot */
		    verbalize(rn2(2) ?
			      "I shall return." :
			      "I'll be back.");
		else
		    verbalize("%s %s!",
			  random_malediction[rn2(SIZE(random_malediction))],
			  random_insult[rn2(SIZE(random_insult))]);
	} else if(mtmp->mtyp == PM_CHAOS){
		if(mtmp->mvar_conversationTracker<5){
			verbalize("%s", random_chaosism[mtmp->mvar_conversationTracker+5]);
			mtmp->mvar_conversationTracker++;
		}
		else verbalize("%s", random_chaosism[rn2(5)]);
	} else if(mtmp->mtyp == PM_GARLAND){
		verbalize("%s", random_chaos_garlandism[rn2(SIZE(random_chaos_garlandism))]);
	} else if(mtmp->mtyp == PM_SIR_GARLAND){
		verbalize("%s", random_garlandism[rn2(SIZE(random_garlandism))]);
	} else if(mtmp->mtyp == PM_APOLLYON){
		verbalize("%s", random_apollyon[rn2(SIZE(random_apollyon))]);
	} else if(is_angel(mtmp->data) && !(is_lminion(mtmp) && rn2(10))){
		int t = rn2(SIZE(random_angeldiction));
		if(t == 0) //Contains %s for god
			verbalize(random_angeldiction[t], godname(u.ualign.god));
		else if(t == 1) //Contains %s for cleansed/purged
			verbalize(random_angeldiction[t], 
				(sgn(u.ualign.type) == sgn(mtmp->data->maligntyp) &&  u.ualign.type != A_VOID) ? 
					"cleansed" : "purged"
			);
		else verbalize("%s", random_angeldiction[t]);
	} else if(mtmp->mtyp == PM_IXOTH){
		verbalize("%s", random_Ixoth[rn2(SIZE(random_Ixoth))]);
	} else if(is_szcultist(mtmp->data)){
		verbalize("%s", random_szcult[rn2(SIZE(random_szcult))]);
	} else if(is_spider(mtmp->data)){
		verbalize("%s", random_mirkwood[rn2(SIZE(random_mirkwood))]);
	} else if(is_lminion(mtmp)) {
		com_pager(rn2(QTN_ANGELIC - 1 + (Hallucination ? 1 : 0)) +
			      QT_ANGELIC);
	} else {
	    if (!rn2(5))
			pline("%s casts aspersions on your %s.", Monnam(mtmp), Hallucination ? "asparagus" : "ancestry");
	    else
	        com_pager(rn2(QTN_DEMONIC) + QT_DEMONIC);
	}
}

STATIC_PTR void
aglaopesong(mtmp)
	struct monst *mtmp;
{
		struct monst *tmpm;
		struct trap *ttmp;
		int ix, iy, i;
		switch(rnd(3)){
			case 1:
				pline("%s sings a song of courage.", Monnam(mtmp));
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(tmpm != mtmp && !DEADMONSTER(tmpm)){
						if(!mindless_mon(tmpm)){
							if ( mtmp->mpeaceful == tmpm->mpeaceful ) {
								if (tmpm->encouraged < BASE_DOG_ENCOURAGED_MAX)
									tmpm->encouraged = min_ints(BASE_DOG_ENCOURAGED_MAX, tmpm->encouraged + rnd(mtmp->m_lev/3+1));
								if (tmpm->mflee) tmpm->mfleetim = 0;
								if (canseemon(tmpm)) {
									if (Hallucination) {
										if(canspotmon(tmpm)) pline("%s looks %s!", Monnam(tmpm),
											  tmpm->encouraged >= BASE_DOG_ENCOURAGED_MAX ? "way cool" :
											  tmpm->encouraged > (BASE_DOG_ENCOURAGED_MAX/2) ? "cooler" : "cool");
									} else {
										if(canspotmon(tmpm)) pline("%s looks %s!", Monnam(tmpm),
											  tmpm->encouraged >= BASE_DOG_ENCOURAGED_MAX ? "berserk" :
											  tmpm->encouraged > (BASE_DOG_ENCOURAGED_MAX/2) ? "wilder" : "wild");
									}
								}
							}
						}
					}
				}
			break;
			case 2:
				pline("%s sings a song of good health.", Monnam(mtmp));
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(tmpm != mtmp && !DEADMONSTER(tmpm)){
						if(!mindless_mon(tmpm)){
							if ( mtmp->mpeaceful == tmpm->mpeaceful ) {
								set_mcan(tmpm, FALSE);
								tmpm->mspec_used = 0;
								if(!tmpm->mnotlaugh && tmpm->mlaughing){
									tmpm->mnotlaugh = 1;
									tmpm->mlaughing = 0;
								}
								if(!tmpm->mcansee && tmpm->mblinded){
									tmpm->mcansee = 1;
									tmpm->mblinded = 0;
								}
								tmpm->mberserk = 0;
								if(tmpm->mhp < tmpm->mhpmax) tmpm->mhp = min(tmpm->mhp + tmpm->m_lev,tmpm->mhpmax);
								if(!tmpm->mcanmove && tmpm->mfrozen){
									tmpm->mcanmove = 1;
									tmpm->mfrozen = 0;
								}
								if(tmpm->mstdy > 0) tmpm->mstdy = 0;
								tmpm->mstun = 0;
								tmpm->mconf = 0;
								tmpm->msleeping = 0;
								tmpm->mflee = 0;
								tmpm->mfleetim = 0;
							}
						}
					}
				}
			break;
			case 3:
				pline("%s sings a song of haste.", Monnam(mtmp));
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(tmpm != mtmp && !DEADMONSTER(tmpm)){
						if(!mindless_mon(tmpm) && tmpm->data->mmove){
							if ( mtmp->mpeaceful == tmpm->mpeaceful ) {
								tmpm->movement += 12;
								tmpm->permspeed = MFAST;
								tmpm->mspeed = MFAST;
								if(canspotmon(tmpm)) pline("%s moves quickly to attack.", Monnam(tmpm));
							}
						}
					}
				}
			break;
			case 4:
				pline("%s sings a dirge.", Monnam(mtmp));
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(tmpm != mtmp && !DEADMONSTER(tmpm)){
						if(!mindless_mon(tmpm)){
							if ( mtmp->mpeaceful != tmpm->mpeaceful && !mm_resist(tmpm, mtmp, 0, FALSE) ) {
								if (tmpm->encouraged > -1*BASE_DOG_ENCOURAGED_MAX)
									tmpm->encouraged = max_ints(-1*BASE_DOG_ENCOURAGED_MAX, tmpm->encouraged - rnd(mtmp->m_lev/3+1));
								if (tmpm->mflee) tmpm->mfleetim = 0;
								if (canseemon(tmpm)) {
									if (Hallucination) {
										if(canspotmon(tmpm)) pline("%s looks %s!", Monnam(tmpm),
											  tmpm->encouraged <= -1*BASE_DOG_ENCOURAGED_MAX ? "peaced out" :
											  tmpm->encouraged < (-1*BASE_DOG_ENCOURAGED_MAX/2) ? "mellower" : "mellow");
									} else {
										if(canspotmon(tmpm)) pline("%s looks %s!", Monnam(tmpm),
											  tmpm->encouraged <= -1*BASE_DOG_ENCOURAGED_MAX ? "inconsolable" :
											  tmpm->encouraged < -1*(BASE_DOG_ENCOURAGED_MAX/2) ? "depressed" : "a bit sad");
									}
								}
							}
						}
					}
				}
			break;
			case 5:
				pline("%s sings a slow march.", Monnam(mtmp));
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(tmpm != mtmp && !DEADMONSTER(tmpm)){
						if(!mindless_mon(tmpm) && tmpm->data->mmove){
							if ( mtmp->mpeaceful != tmpm->mpeaceful && !mm_resist(tmpm, mtmp, 0, FALSE) ) {
								tmpm->movement -= 12;
								tmpm->permspeed = MSLOW;
								tmpm->mspeed = MSLOW;
							}
						}
					}
				}
			break;
		}

}
#endif /* OVLB */

/*wizard.c*/

/*	SCCS Id: @(#)steal.c	3.4	2003/12/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(stealarm);

#ifdef OVLB
STATIC_DCL const char *FDECL(equipname, (struct obj *));

STATIC_OVL const char *
equipname(otmp)
register struct obj *otmp;
{
	return (
#ifdef TOURIST
		(otmp == uarmu) ? "shirt" :
#endif
		(otmp == uarmf) ? "boots" :
		(otmp == uarms) ? "shield" :
		(otmp == uarmg) ? "gloves" :
		(otmp == uarmc) ? cloak_simple_name(otmp) :
		(otmp == uarmh) ? "helmet" : "armor");
}

#ifndef GOLDOBJ
long		/* actually returns something that fits in an int */
somegold()
{
#ifdef LINT	/* long conv. ok */
	return(0L);
#else
	return (long)( (u.ugold < 100) ? u.ugold :
		(u.ugold > 10000) ? rnd(10000) : rnd((int) u.ugold) );
#endif
}

void
stealgold(mtmp)
register struct monst *mtmp;
{
	register struct obj *gold = g_at(u.ux, u.uy);
	register long tmp;

	if (gold && ( !u.ugold || gold->quan > u.ugold || !rn2(5))) {
	    mtmp->mgold += gold->quan;
	    delobj(gold);
	    newsym(u.ux, u.uy);
	    pline("%s quickly snatches some gold from between your %s!",
		    Monnam(mtmp), makeplural(body_part(FOOT)));
	    if(!u.ugold || !rn2(5)) {
		if (!tele_restrict(mtmp)) (void) rloc(mtmp, TRUE);
		/* do not set mtmp->mavenge here; gold on the floor is fair game */
		monflee(mtmp, 0, FALSE, FALSE);
	    }
	} else if(u.ugold) {
	    u.ugold -= (tmp = somegold());
	    Your("purse feels lighter.");
		IMPURITY_UP(u.uimp_theft)
	    mtmp->mgold += tmp;
	if(mtmp->mtyp != PM_FAFNIR){
		if (!tele_restrict(mtmp)) (void) rloc(mtmp, TRUE);
		mtmp->mavenge = 1;
		monflee(mtmp, 0, FALSE, FALSE);
	}
	    flags.botl = 1;
	}
}

#else /* !GOLDOBJ */

long		/* actually returns something that fits in an int */
somegold(umoney)
long umoney;
{
#ifdef LINT	/* long conv. ok */
	return(0L);
#else
	return (long)( (umoney < 100) ? umoney :
		(umoney > 10000) ? rnd(10000) : rnd((int) umoney) );
#endif
}

/*
Find the first (and hopefully only) gold object in a chain.
Used when leprechaun (or you as leprechaun) looks for
someone else's gold.  Returns a pointer so the gold may
be seized without further searching.
May search containers too.
Deals in gold only, as leprechauns don't care for lesser coins.
*/
struct obj *
findgold(chain)
register struct obj *chain;
{
        while (chain && chain->otyp != GOLD_PIECE) chain = chain->nobj;
        return chain;
}

/* 
Steal gold coins only.  Leprechauns don't care for lesser coins.
*/
void
stealgold(mtmp)
register struct monst *mtmp;
{
	register struct obj *fgold = g_at(u.ux, u.uy);
	register struct obj *ygold;
	register long tmp;

        /* skip lesser coins on the floor */        
        while (fgold && fgold->otyp != GOLD_PIECE) fgold = fgold->nexthere; 

        /* Do you have real gold? */
        ygold = findgold(invent);

	if (fgold && ( !ygold || fgold->quan > ygold->quan || !rn2(5))) {
            obj_extract_self(fgold);
	    add_to_minv(mtmp, fgold);
	    newsym(u.ux, u.uy);
	    pline("%s quickly snatches some gold from between your %s!",
		    Monnam(mtmp), makeplural(body_part(FOOT)));
	    if(!ygold || !rn2(5)) {
		if (!tele_restrict(mtmp)) (void) rloc(mtmp, TRUE);
		monflee(mtmp, 0, FALSE, FALSE);
	    }
	} else if(ygold) {
            const int gold_price = objects[GOLD_PIECE].oc_cost;
	    tmp = (somegold(money_cnt(invent)) + gold_price - 1) / gold_price;
	    tmp = min(tmp, ygold->quan);
            if (tmp < ygold->quan) ygold = splitobj(ygold, tmp);
            freeinv(ygold);
            add_to_minv(mtmp, ygold);
	    Your("purse feels lighter.");
	    if (!tele_restrict(mtmp)) (void) rloc(mtmp, TRUE);
	    monflee(mtmp, 0, FALSE, FALSE);
	    flags.botl = 1;
	}
}
#endif /* GOLDOBJ */

/* steal armor after you finish taking it off */
unsigned int stealoid;		/* object to be stolen */
unsigned int stealmid;		/* monster doing the stealing */

STATIC_PTR int
stealarm(VOID_ARGS)
{
	register struct monst *mtmp;
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj) {
	    if(otmp->o_id == stealoid) {
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if(mtmp->m_id == stealmid) {
			if(DEADMONSTER(mtmp)) impossible("stealarm(): dead monster stealing"); 
			if(!dmgtype(mtmp->data, AD_SITM)) /* polymorphed */
			    goto botm;
			if(otmp->unpaid)
			    subfrombill(otmp, shop_keeper(*u.ushops));
			freeinv(otmp);
			pline("%s steals %s!", Monnam(mtmp), doname(otmp));
			(void) mpickobj(mtmp,otmp);	/* may free otmp */
			/* Implies seduction, "you gladly hand over ..."
			   so we don't set mavenge bit here. */
			monflee(mtmp, 0, FALSE, FALSE);
			if (!tele_restrict(mtmp)) (void) rloc(mtmp, TRUE);
			if(roll_madness(MAD_TALONS)){
				You("panic after having your property stolen!!");
				HPanicking += 1+rnd(6);
			}
		        break;
		    }
		}
		break;
	    }
	}
botm:   stealoid = 0;
	return 0;
}

/* An object you're wearing has been taken off by a monster (theft or
   seduction).  Also used if a worn item gets transformed (stone to flesh). */
void
remove_worn_item(obj, unchain_ball)
struct obj *obj;
boolean unchain_ball;	/* whether to unpunish or just unwield */
{
	if (donning(obj))
	    cancel_don();
	if (!obj->owornmask)
	    return;

	if (obj->owornmask & W_ARMOR) {
	    if (obj == uskin) {
		impossible("Removing embedded scales?");
		skinback(TRUE);		/* uarm = uskin; uskin = 0; */
	    }
	    if (obj == uarm) (void) Armor_off();
	    else if (obj == uarmc) (void) Cloak_off();
	    else if (obj == uarmf) (void) Boots_off();
	    else if (obj == uarmg) (void) Gloves_off();
	    else if (obj == uarmh) (void) Helmet_off();
	    else if (obj == uarms) (void) Shield_off();
#ifdef TOURIST
	    else if (obj == uarmu) (void) Shirt_off();
#endif
	    /* catchall -- should never happen */
	    else setworn((struct obj *)0, obj->owornmask & W_ARMOR);
	} else if (obj->owornmask & W_AMUL) {
	    Amulet_off();
	} else if (obj->owornmask & W_BELT) {
	    Belt_off();
	} else if (obj->owornmask & W_RING) {
	    Ring_gone(obj);
	} else if (obj->owornmask & W_TOOL) {
	    Blindf_off(obj);
	} else if (obj->owornmask & (W_WEP|W_SWAPWEP|W_QUIVER)) {
	    if (obj == uwep)
		uwepgone();
	    if (obj == uswapwep)
		uswapwepgone();
	    if (obj == uquiver)
		uqwepgone();
	}

	if (obj->owornmask & (W_BALL|W_CHAIN)) {
	    if (unchain_ball) unpunish();
	} else if (obj->owornmask) {
	    /* catchall */
	    setnotworn(obj);
	}
}

/* Returns 1 when something was stolen (or at least, when N should flee now)
 * Returns -1 if the monster died in the attempt
 * Avoid stealing the object stealoid
 */
int
steal(mtmp, objnambuf, artifact, monkey_business)
struct monst *mtmp;
char *objnambuf;
int artifact;
int monkey_business; /* true iff an animal is doing the thievery */
{
	struct obj *otmp;
	int tmp, could_petrify, named = 0, armordelay;
	if(!mtmp)
		return 0;
	boolean charms = (is_neuter(mtmp->data) || flags.female == mtmp->female);
	boolean mi_only = is_chuul(mtmp->data);
	if(mtmp->mtyp == PM_ALRUNES) charms = !charms;
	else if(mtmp->mtyp == PM_FIERNA) charms = TRUE;
	else if(mtmp->mtyp == PM_BEAUTEOUS_ONE) charms = FALSE;

	if (objnambuf) *objnambuf = '\0';
	/* the following is true if successful on first of two attacks. */
	if(!monnear(mtmp, u.ux, u.uy)) return(0);

	/* food being eaten might already be used up but will not have
	   been removed from inventory yet; we don't want to steal that,
	   so this will cause it to be removed now */
	if (occupation) (void) maybe_finished_meal(FALSE);

	if (!invent || (inv_cnt() == 1 && uskin)) {
nothing_to_steal:
	    /* Not even a thousand men in armor can strip a naked man. */
	    if(Blind){
	      if(!artifact) pline("Somebody tries to rob you, but finds nothing to steal.");
		}
	    else{
	      if(!artifact) pline("%s tries to rob you, but there is nothing to steal!", Monnam(mtmp));
		}
	    return(1);	/* let her flee */
	}

	// monkey_business = is_animal(mtmp->data);
	if (monkey_business) {
	    ;	/* skip ring special cases */
	} else if (Adornment & LEFT_RING) {
	    otmp = uleft;
	    goto gotobj;
	} else if (Adornment & RIGHT_RING) {
	    otmp = uright;
	    goto gotobj;
	}

	tmp = 0;
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if ((!uarm || otmp != uarmc) && otmp != uskin
				&& (!mi_only || is_magic_obj(otmp))
				&& !(ProtItems && (otmp->oclass == POTION_CLASS || otmp->oclass == SCROLL_CLASS || otmp->oclass == WAND_CLASS))
#ifdef INVISIBLE_OBJECTS
				&& (!otmp->oinvis || mon_resistance(mtmp,SEE_INVIS))
#endif
				)
		tmp += ((otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL)) ? 5 : 1);
	if (!tmp) goto nothing_to_steal;
	tmp = rn2(tmp);
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if ((!uarm || otmp != uarmc) && otmp != uskin
				&& (!mi_only || is_magic_obj(otmp))
				&& !(ProtItems && (otmp->oclass == POTION_CLASS || otmp->oclass == SCROLL_CLASS || otmp->oclass == WAND_CLASS))
#ifdef INVISIBLE_OBJECTS
				&& (!otmp->oinvis || mon_resistance(mtmp,SEE_INVIS))
#endif
			)
		if((tmp -= ((otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL)) ? 5 : 1)) < 0)
			break;
	if(!otmp) {
		impossible("Steal fails!");
		return(0);
	}
	/* can't steal gloves while wielding - so steal the wielded item. */
	if (otmp == uarmg && uwep)
	    otmp = uwep;
	/* can't steal armor while wearing cloak - so steal the cloak. */
	else if(otmp == uarm && uarmc) otmp = uarmc;
#ifdef TOURIST
	else if(otmp == uarmu && uarmc) otmp = uarmc;
	else if(otmp == uarmu && uarm && arm_blocks_upper_body(uarm->otyp)) otmp = uarm;
#endif
gotobj:
	if(otmp->o_id == stealoid) return(0);
	
	if(otmp->oartifact == ART_PEN_OF_THE_VOID && otmp->ovara_seals&SEAL_ANDROMALIUS){
		pline("%s tries to steal your weapon, but is prevented!",Monnam(mtmp));
		return 0;
	}

	/* animals can't overcome curse stickiness nor unlock chains */
	if (monkey_business) {
	    boolean ostuck;
	    /* is the player prevented from voluntarily giving up this item?
	       (ignores loadstones; the !can_carry() check will catch those) */
	    if (otmp == uball)
		ostuck = TRUE;	/* effectively worn; curse is implicit */
	    else if (otmp == uquiver || (otmp == uswapwep && !u.twoweap))
		ostuck = FALSE;	/* not really worn; curse doesn't matter */
	    else
		ostuck = (otmp->cursed && otmp->owornmask);

	    if (ostuck || !can_carry(mtmp, otmp)) {
		static const char * const how[] = { "steal","snatch","grab","take" };
 cant_take:
		pline("%s tries to %s your %s but gives up.",
		      Monnam(mtmp), how[rn2(SIZE(how))],
		      (otmp->owornmask & W_ARMOR) ? equipname(otmp) :
		       cxname(otmp));
		/* the fewer items you have, the less likely the thief
		   is going to stick around to try again (0) instead of
		   running away (1) */
		return !rn2(inv_cnt() / 5 + 2);
	    }
	}

	/*stealing is impure*/
	IMPURITY_UP(u.uimp_theft)

	if (otmp->otyp == LEASH && otmp->leashmon) {
	    if (monkey_business && otmp->cursed) goto cant_take;
	    o_unleash(otmp);
	}

	/* you're going to notice the theft... */
	stop_occupation();

	if((otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL))){
		switch(otmp->oclass) {
		case TOOL_CLASS:
		case AMULET_CLASS:
		case RING_CLASS:
		case BELT_CLASS:
		case FOOD_CLASS: /* meat ring */
		    remove_worn_item(otmp, TRUE);
		    break;
		case ARMOR_CLASS:
		    armordelay = objects[otmp->otyp].oc_delay;
		    /* Stop putting on armor which has been stolen. */
		    if (donning(otmp)) {
			remove_worn_item(otmp, TRUE);
			break;
		    } else if (monkey_business || artifact) {
			/* animals usually don't have enough patience
			   to take off items which require extra time */
			if (armordelay >= 1 && rn2(10)) goto cant_take;
			remove_worn_item(otmp, TRUE);
			break;
		    } else {
			int curssv = otmp->cursed;
			int slowly;
			boolean seen = canspotmon(mtmp);

			otmp->cursed = 0;
			/* can't charm you without first waking you */
			if (multi < 0 && is_fainted()) unmul((char *)0);
			slowly = (armordelay >= 1 || multi < 0);
			if(!artifact){
			if(mtmp->mtyp == PM_DEMOGORGON)
				pline("%s compels you.  You gladly %s your %s.",
				  Monnam(mtmp),
				  curssv ? "let him take" :
				  slowly ? "start removing" : "hand over",
				  equipname(otmp));
			else if(charms){
			    pline("%s charms you.  You gladly %s your %s.",
				  !seen ? SheHeIt(mtmp) : Monnam(mtmp),
				  curssv ? "let her take" :
				  slowly ? "start removing" : "hand over",
				  equipname(otmp));
				  if(!rn2(2+u.uimp_seduction)){
					IMPURITY_UP(u.uimp_seduction)
				  }
			}
			else {
			    pline("%s seduces you and %s off your %s.",
				  !seen ? SheHeIt(mtmp) : Adjmonnam(mtmp, "beautiful"),
				  curssv ? "helps you to take" :
				  slowly ? "you start taking" : "you take",
				  equipname(otmp));
				  IMPURITY_UP(u.uimp_seduction)
			}
			named++;
			}
			/* the following is to set multi for later on */
			nomul(-armordelay, "taking off clothes");
			remove_worn_item(otmp, TRUE);
			otmp->cursed = curssv;
			if(multi < 0){
				/*
				multi = 0;
				nomovemsg = 0;
				afternmv = 0;
				*/
				stealoid = otmp->o_id;
				stealmid = mtmp->m_id;
				afternmv = stealarm;
				return(0);
			}
		    }
		    break;
		default:
		    impossible("Tried to steal a strange worn thing. [%d]",
			       otmp->oclass);
		}
	}
	else if (otmp->owornmask)
	    remove_worn_item(otmp, TRUE);

	/* do this before removing it from inventory */
	if (objnambuf) Strcpy(objnambuf, yname(otmp));
	/* set mavenge bit so knights won't suffer an
	 * alignment penalty during retaliation;
	 */
	mtmp->mavenge = 1;

	freeinv(otmp);
	pline("%s stole %s.", Monnam(mtmp), doname(otmp));
	could_petrify = (otmp->otyp == CORPSE &&
			 touch_petrifies(&mons[otmp->corpsenm]));
	(void) mpickobj(mtmp,otmp);	/* may free otmp */
	if (could_petrify && !(mtmp->misc_worn_check & W_ARMG)) {
	    minstapetrify(mtmp, TRUE);
	}
	if(roll_madness(MAD_TALONS)){
		You("panic after having your property stolen!");
		HPanicking += 1+rnd(6);
	}
	//mtmp died, probably from taking a petrifying corpse
	return((mtmp->mhp <= 0) ? -1 : (multi < 0) ? 0 : 1);
}

#endif /* OVLB */
#ifdef OVL1

/* Returns 1 if otmp is free'd, 0 otherwise. */
int
mpickobj(mtmp,otmp)
register struct monst *mtmp;
register struct obj *otmp;
{
    int freed_otmp;

#ifndef GOLDOBJ
    if (otmp->oclass == COIN_CLASS) {
	mtmp->mgold += otmp->quan;
	obfree(otmp, (struct obj *)0);
	freed_otmp = 1;
	}
	else {
#endif
		boolean snuff_otmp = FALSE;
		/* Must do carrying effects on object prior to add_to_minv() */
		carry_obj_effects(otmp);

		/* extinguish burning objects -- it isn't strictly necessary to end_burn anymore, though */
		if (mon_attacktype(mtmp, AT_ENGL)) {
			/* burning objects should go out; */
			if (obj_is_burning(otmp) && u.uswallow && mtmp == u.ustuck) {
				if (!Blind)
					pline("%s out.", Tobjnam(otmp, "go"));
				end_burn(otmp, TRUE);
			}
		}

		/* add_to_minv() might free otmp [if merged with something else],
		   so we have to call it after doing the object checks */
		freed_otmp = add_to_minv(mtmp, otmp);
//		/* and we had to defer this until object is in mtmp's inventory */
//		if (snuff_otmp) snuff_light_source(mtmp->mx, mtmp->my);
#ifndef GOLDOBJ
	}
#endif
    return freed_otmp;
}

#endif /* OVL1 */
#ifdef OVLB

void
stealamulet(mtmp)
struct monst *mtmp;
{
    struct obj *otmp = (struct obj *)0;
    int real=0, fake=0;

    /* select the artifact to steal */
    if(u.uhave.amulet) {
	real = AMULET_OF_YENDOR;
	fake = FAKE_AMULET_OF_YENDOR;
    } else if(u.uhave.bell) {
	real = BELL_OF_OPENING;
	fake = BELL;
    } else if(u.uhave.book) {
	real = SPE_BOOK_OF_THE_DEAD;
    } else if(u.uhave.menorah) {
	real = CANDELABRUM_OF_INVOCATION;
    } else return;	/* you have nothing of special interest */

    if (!otmp) {
	/* If we get here, real and fake have been set up. */
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if(otmp->otyp == real || (otmp->otyp == fake && !mtmp->iswiz))
			break;
    }

    if (otmp) { /* we have something to snatch */
	if (otmp->owornmask)
	    remove_worn_item(otmp, TRUE);
	freeinv(otmp);
	/* mpickobj wont merge otmp because none of the above things
	   to steal are mergable */
	(void) mpickobj(mtmp,otmp);	/* may merge and free otmp */
	pline("%s stole %s!", Monnam(mtmp), doname(otmp));
	if (mon_resistance(mtmp,TELEPORT) && !tele_restrict(mtmp))
	    (void) rloc(mtmp, TRUE);
    }
}

void
stealquestart(mtmp)
struct monst *mtmp;
{
    struct obj *otmp = (struct obj *)0;
    int real=0, fake=0;

    /* select the artifact to steal */
    if(u.uhave.amulet) {
	real = AMULET_OF_YENDOR;
	fake = FAKE_AMULET_OF_YENDOR;
    } else if(u.uhave.questart) {
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if(is_quest_artifact(otmp)) break;
	if (!otmp) return;	/* should we panic instead? */
    } else if(u.uhave.bell) {
	real = BELL_OF_OPENING;
	fake = BELL;
    } else if(u.uhave.book) {
	real = SPE_BOOK_OF_THE_DEAD;
    } else if(u.uhave.menorah) {
	real = CANDELABRUM_OF_INVOCATION;
    } else return;	/* you have nothing of special interest */

    if (!otmp) {
	/* If we get here, real and fake have been set up. */
	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if(otmp->otyp == real || (otmp->otyp == fake && !mtmp->iswiz))
			break;
    }

    if (otmp) { /* we have something to snatch */
	if (otmp->owornmask)
	    remove_worn_item(otmp, TRUE);
	freeinv(otmp);
	/* mpickobj wont merge otmp because none of the above things
	   to steal are mergable */
	(void) mpickobj(mtmp,otmp);	/* may merge and free otmp */
	pline("%s stole %s!", Monnam(mtmp), doname(otmp));
	if (mon_resistance(mtmp,TELEPORT) && !tele_restrict(mtmp))
	    (void) rloc(mtmp, TRUE);
    }
}

#endif /* OVLB */
#ifdef OVL0

/* drop one object taken from a (possibly dead) monster's inventory */
void
mdrop_obj(mon, obj, verbosely)
struct monst *mon;
struct obj *obj;
boolean verbosely;
{
    int omx = mon->mx, omy = mon->my;

    if (obj->owornmask) {
	/* always perform worn item handling */
	mon->misc_worn_check &= ~obj->owornmask;
	update_mon_intrinsics(mon, obj, FALSE, FALSE);
	/* obj_no_longer_held(obj); -- done by place_object */
	if (obj->owornmask & W_WEP){
		setmnotwielded(mon,obj);
		MON_NOWEP(mon);
	}
	if (obj->owornmask & W_SWAPWEP){
		setmnotwielded(mon,obj);
		MON_NOSWEP(mon);
	}
#ifdef STEED
	/* don't charge for an owned saddle on dead steed */
	if (mon->mhp <= 0 && mon->mtame && (obj->owornmask & W_SADDLE) && 
		!obj->unpaid && costly_spot(omx, omy)) {
	    obj->no_charge = 1;
#endif
	}
	obj->owornmask = 0L;
    }
    if (verbosely && cansee(omx, omy))
	pline("%s drops %s.", Monnam(mon), distant_name(obj, doname));
    if (!flooreffects(obj, omx, omy, "fall")) {
	place_object(obj, omx, omy);
	stackobj(obj);
    }
}

/* some monsters bypass the normal rules for moving between levels or
   even leaving the game entirely; when that happens, prevent them from
   taking the Amulet or invocation tools with them */
void
mdrop_special_objs(mon)
struct monst *mon;
{
    struct obj *obj, *otmp;

    for (obj = mon->minvent; obj; obj = otmp) {
		otmp = obj->nobj;
		/* the Amulet, invocation tools, and Rider corpses resist even when
		   artifacts and ordinary objects are given 0% resistance chance */
		if (obj_resists(obj, 0, 0)) {
			obj_extract_self(obj);
			mdrop_obj(mon, obj, FALSE);
		}
		/* Items that are entangling a monster don't vanish with it */
		else if(mon->entangled_oid == obj->o_id){
			obj_extract_self(obj);
			//Assumes that the only way the jin gang zuo can come into play is via crowning gift.
			if(obj->oartifact == ART_JIN_GANG_ZUO){
				hold_another_object(obj, "Oops!  The returning %s slips to the floor!", "snare", (const char *)0);
			}
			else {
				mdrop_obj(mon, obj, FALSE);
			}
		}
    }
}

static struct obj *propellor;

extern boolean FDECL(would_prefer_hwep,(struct monst *,struct obj *));
extern boolean FDECL(would_prefer_rwep,(struct monst *,struct obj *));

/* release the objects the creature is carrying */
void
relobj(mtmp,show,is_pet)
register struct monst *mtmp;
register int show;
boolean is_pet;		/* If true, pet should keep wielded/worn items */
{
	register struct obj *otmp;
	register int omx = mtmp->mx, omy = mtmp->my;

	if(is_pet){
		//THIS IS VERY EFFICIENT!!1!
		//(It starts over from the beginning of the inventory every time it drops an object)
		//(Because it's a linked list and it starts dropping from the head, it's still O(n))
		while((otmp = DROPPABLES(mtmp))){
			obj_extract_self(otmp);
			mdrop_obj(mtmp, otmp, is_pet && flags.verbose);
		}
	} else {
		//Drop everything, I'm dying :(
		while((otmp = mtmp->minvent)){
			obj_extract_and_unequip_self(otmp);
			//Assumes that the only way the jin gang zuo can come into play is via crowning gift.
			if(otmp->oartifact == ART_JIN_GANG_ZUO){
				hold_another_object(otmp, "Oops!  The returning %s slips to the floor!", "snare", (const char *)0);
			}
			else {
				mdrop_obj(mtmp, otmp, is_pet && flags.verbose);
			}
		}
	}

#ifndef GOLDOBJ
	if (mtmp->mgold) {
		register long g = mtmp->mgold;
		(void) mkgold_core(g, omx, omy, FALSE);
		if (is_pet && cansee(omx, omy) && flags.verbose)
			pline("%s drops %ld gold piece%s.", Monnam(mtmp),
				g, plur(g));
		mtmp->mgold = 0L;
	}
#endif
	
	if (show & cansee(omx, omy))
		newsym(omx, omy);
}

/* release the objects the creature is carrying */
void
relobj_envy(mtmp,show)
register struct monst *mtmp;
register int show;
{
	register struct obj *otmp;
	register int omx = mtmp->mx, omy = mtmp->my;

	//THIS IS VERY EFFICIENT!!1!
	//(It starts over from the beginning of the inventory every time it drops an object)
	//(Because it's a linked list and it starts dropping from the head, it's still O(n))
	while((otmp = drop_envy(mtmp))){
		obj_extract_self(otmp);
		mdrop_obj(mtmp, otmp, flags.verbose);
	}

#ifndef GOLDOBJ
	if (mtmp->mgold) {
		register long g = mtmp->mgold;
		(void) mkgold_core(g, omx, omy, FALSE);
		if (cansee(omx, omy) && flags.verbose)
			pline("%s drops %ld gold piece%s.", Monnam(mtmp),
				g, plur(g));
		mtmp->mgold = 0L;
	}
#endif
	
	if (show & cansee(omx, omy))
		newsym(omx, omy);
}

#endif /* OVL0 */

/*steal.c*/

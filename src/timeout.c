/*	SCCS Id: @(#)timeout.c	3.4	2002/12/17	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"

#include "hack.h"
#include "artifact.h"

#include "lev.h"	/* for checking save modes */

STATIC_DCL void NDECL(stoned_dialogue);
STATIC_DCL void NDECL(golded_dialogue);
STATIC_DCL void NDECL(gillyweed_dialogue);
STATIC_DCL void NDECL(salted_dialogue);
#ifdef CONVICT
STATIC_DCL void NDECL(phasing_dialogue);
#endif /* CONVICT */
STATIC_DCL void NDECL(vomiting_dialogue);
STATIC_DCL void NDECL(choke_dialogue);
STATIC_DCL void NDECL(slime_dialogue);
STATIC_DCL void NDECL(slip_or_trip);
STATIC_DCL void FDECL(see_lamp_flicker, (struct obj *, const char *));
STATIC_DCL void FDECL(lantern_message, (struct obj *));
STATIC_DCL void FDECL(cleanup_burn, (genericptr_t,long));

#ifdef OVLB

/* used by wizard mode #timeout and #wizintrinsic; order by 'interest'
   for timeout countdown, where most won't occur in normal play */
const struct propname {
    int prop_num;
    const char *prop_name;
} propertynames[] = {
    { SANCTUARY, "sanctuary" },
    { STONED, "petrifying" },
    { FROZEN_AIR, "frozen air" },
    { SLIMED, "becoming slime" },
    { STRANGLED, "strangling" },
    { SICK, "fatally sick" },
    { STUNNED, "stunned" },
    { DOUBT, "agnostic" },
    { CONFUSION, "confused" },
    { HALLUC, "hallucinating" },
    { BLINDED, "blinded" },
    { VOMITING, "vomiting" },
    { GLIB, "slippery fingers" },
    { WOUNDED_LEGS, "wounded legs" },
	{ BLIND_RES, "immune to blinding" },
	{ GAZE_RES, "immune to gazes" },
	{ SLEEPING, "sleeping" },
    { TELEPORT, "teleporting" },
    { POLYMORPH, "polymorphing" },
    { LEVITATION, "levitating" },
    { FAST, "very fast" },
    { CLAIRVOYANT, "clairvoyant" },
    { DETECT_MONSTERS, "monster detection" },
    { SEE_INVIS, "see invisible" },
    { INVIS, "invisible" },
    { FIRE_RES, "fire resistance" },
    { COLD_RES, "cold resistance" },
    { SLEEP_RES, "sleep resistance" },
    { DISINT_RES, "disintegration resistance" },
    { SHOCK_RES, "shock resistance" },
    { POISON_RES, "poison resistance" },
    { ACID_RES, "acid resistance" },
    { STONE_RES, "stoning resistance" },
    { DRAIN_RES, "drain resistance" },
    { SICK_RES, "sickness resistance" },
    { ANTIMAGIC, "magic resistance" },
    { HALLUC_RES, "hallucination resistance" },
	{ BLOCK_CONFUSION, "confusion resistance" },
    { FUMBLING, "fumbling" },
    { HUNGER, "voracious hunger" },
    { TELEPAT, "telepathic" },
    { WARNING, "dangersense" },
    { WARN_OF_MON, "warning of specific foes" },
    { SEARCHING, "searching" },
    { INFRAVISION, "infravision" },
    { ADORNED, "adorned (+/- Cha)" },
    { DISPLACED, "displaced" },
    { STEALTH, "stealthy" },
    { AGGRAVATE_MONSTER, "monster aggravation" },
    { CONFLICT, "conflict" },
    { JUMPING, "jumping" },
    { TELEPORT_CONTROL, "teleport control" },
    { FLYING, "flying" },
    { WWALKING, "water walking" },
    { SWIMMING, "swimming" },
    { MAGICAL_BREATHING, "magical breathing" },
    { PASSES_WALLS, "pass thru walls" },
    { SLOW_DIGESTION, "slow digestion" },
    { HALF_SPDAM, "half spell damage" },
    { HALF_PHDAM, "half physical damage" },
    { REGENERATION, "HP regeneration" },
    { ENERGY_REGENERATION, "energy regeneration" },
    { PROTECTION, "extra protection" },
    { PROT_FROM_SHAPE_CHANGERS, "protection from shape changers" },
    { POLYMORPH_CONTROL, "polymorph control" },
    { UNCHANGING, "unchanging" },
    { REFLECTING, "reflecting" },
    { FREE_ACTION, "free action" },
    { FIXED_ABIL, "fixed abilites" },
    { LIFESAVED, "life will be saved" },
	{ NULLMAGIC, "magic nullification" },
	{ DEADMAGIC, "dead magic zone" },
	{ CATAPSI, "psionic vortex" },
	{ MISOTHEISM, "divine-exclusion zone" },
    { WATERPROOF, "waterproofing" },
    { SHATTERING, "fracturing" },
    { DARKVISION_ONLY, "darksight-override" },
    { DIMENSION_LOCK, "dimensional lock" },
	{ CLEAR_THOUGHTS, "clear thoughts" },
    { EXTRAMISSION, "extramission" },
    {  0, 0 },
};

/* He is being petrified - dialogue by inmet!tower */
static NEARDATA const char * const stoned_texts[] = {
	"You are slowing down.",		/* 5 */
	"Your limbs are stiffening.",		/* 4 */
	"Your limbs have turned to stone.",	/* 3 */
	"You have turned to stone.",		/* 2 */
	"You are a statue."			/* 1 */
};

STATIC_OVL void
stoned_dialogue()
{
	register long i = (Stoned & TIMEOUT);

	if (i > 0L && i <= SIZE(stoned_texts)) {
		pline1(stoned_texts[SIZE(stoned_texts) - i]);
		nomul(0, NULL); /* fix for C343-74 */
	}
	if (i == 5L)
		HFast = 0L;
	if (i == 3L && !Free_action)
		nomul(-3, "getting stoned");
	exercise(A_DEX, FALSE);
}

static NEARDATA const char * const golded_texts[] = {
	"You are slowing down.",		/* 5 */
	"Your limbs are stiffening.",		/* 4 */
	"Your limbs have turned to gold.",	/* 3 */
	"You have turned to gold.",		/* 2 */
	"You are a gold statue."			/* 1 */
};

STATIC_OVL void
golded_dialogue()
{
	register long i = (Golded & TIMEOUT);

	if (i > 0L && i <= SIZE(golded_texts)) {
		pline1(golded_texts[SIZE(golded_texts) - i]);
		nomul(0, NULL); /* fix for C343-74 */
	}
	if (i == 5L)
		HFast = 0L;
	if (i == 3L && !Free_action)
		nomul(-3, "turning to gold");
	exercise(A_DEX, FALSE);
}

STATIC_OVL void
gillyweed_dialogue()
{
    if (HMagical_breathing == 15) {
        Your("gills are beginning to disappear.");
        stop_occupation();
    } else if (HMagical_breathing == 1) {
        Your("gills are gone.");
        stop_occupation();
    }
}

static NEARDATA const char * const salted_texts[] = {
	"You are slowing down.",		/* 5 */
	"Your limbs are stiffening.",		/* 4 */
	"Your limbs have turned to salt.",	/* 3 */
	"You have turned to salt.",		/* 2 */
	"You have become a pillar of salt."			/* 1 */
};

STATIC_OVL void
salted_dialogue()
{
	register long i = (Salted & TIMEOUT);

	if (i > 0L && i <= SIZE(salted_texts)) {
		pline1(salted_texts[SIZE(salted_texts) - i]);
		nomul(0, NULL); /* fix for C343-74 */
	}
	if (i == 5L)
		HFast = 0L;
	if (i == 3L && !Free_action)
		nomul(-3, "turning to salt");
	exercise(A_DEX, FALSE);
}

#ifdef CONVICT
STATIC_OVL void
phasing_dialogue()
{
    if (Phasing == 15) {
        if (!Hallucination) {
            Your("body is beginning to feel more solid.");
        } else {
            You_feel("more distant from the spirit world.");
        }
        stop_occupation();
    } else if (Phasing == 1) {
        if (!Hallucination) {
            Your("body is solid again.");
        } else {
            You_feel("totally separated from the spirit world.");
        }
        stop_occupation();
    }
}
#endif /* CONVICT */

/* He is getting sicker and sicker prior to vomiting */
static NEARDATA char * const vomiting_texts[] = {
	"are feeling mildly nauseated.",	/* 14 */
	"feel slightly confused.",		/* 11 */
	"can't seem to think straight.",	/* 8 */
	"feel incredibly sick.",		/* 5 */
	"suddenly vomit!"			/* 2 */
};

/* He is getting sicker and sicker prior to vomiting */
static NEARDATA char * const non_vomiting_texts[] = {
	"are somehow feeling nauseated.",	/* 14 */
	"feel slightly confused.",		/* 11 */
	"can't seem to think straight.",	/* 8 */
	"feel incredibly sick.",		/* 5 */
	"suddenly retch!"			/* 2 */
};

STATIC_OVL void
vomiting_dialogue()
{
	register long i = (Vomiting & TIMEOUT) / 3L;
	
	if ((((Vomiting & TIMEOUT) % 3L) == 2) && (i >= 0)
	    && (i < SIZE(vomiting_texts)))
		You("%s", umechanoid ? 
				non_vomiting_texts[SIZE(non_vomiting_texts) - i - 1] : 
				vomiting_texts[SIZE(vomiting_texts) - i - 1] );

	switch ((int) i) {
	case 0:
		vomit();
		morehungry(20*get_uhungersizemod());
		break;
	case 2:
		make_stunned(HStun + d(2,4), FALSE);
		/* fall through */
	case 3:
		make_confused(HConfusion + d(2,4), FALSE);
		break;
	}
	exercise(A_CON, FALSE);
}

static NEARDATA const char * const choke_texts[] = {
	"You find it hard to breathe.",
	"You're gasping for air.",
	"You can no longer breathe.",
	"You're turning %s.",
	"You suffocate."
};

static NEARDATA const char * const choke_texts2[] = {
	"Your %s is becoming constricted.",
	"Your blood is having trouble reaching your brain.",
	"The pressure on your %s increases.",
	"Your consciousness is fading.",
	"You suffocate."
};

static NEARDATA const char * const choke_texts3[] = {
	"You can't dislodge your meal.",
	"You're gasping for air.",
	"You can no longer breathe.",
	"You're turning %s.",
	"You choke to death."
};

STATIC_OVL void
choke_dialogue()
{
	register long i = u.divetimer;
	const char *str;

	if(i > 0 && i <= SIZE(choke_texts)) {
		if (HStrangled & TIMEOUT) {
			str = choke_texts3[SIZE(choke_texts) - i];
			goto printchokestr;
		}
	    else if (!rn2(10))
			pline(choke_texts2[SIZE(choke_texts2) - i], body_part(NECK));
		else {
			str = choke_texts[SIZE(choke_texts) - i];

printchokestr:
			if (index(str, '%'))
				pline(str, hcolor(NH_BLUE));
			else
				pline1(str);
		}
	}
	exercise(A_STR, FALSE);
}

static NEARDATA const char * const slime_texts[] = {
	"You are turning a little %s.",           /* 5 */
	"Your limbs are getting oozy.",              /* 4 */
	"Your skin begins to peel away.",            /* 3 */
	"You are turning into %s.",       /* 2 */
	"You have become %s."             /* 1 */
};

STATIC_OVL void
slime_dialogue()
{
	register long i = (Slimed & TIMEOUT) / 2L;

	if (((Slimed & TIMEOUT) % 2L) && i >= 0L
		&& i < SIZE(slime_texts)) {
	    const char *str = slime_texts[SIZE(slime_texts) - i - 1L];

	    if (index(str, '%')) {
		if (i == 4L) {	/* "you are turning green" */
		    if (!Blind)	/* [what if you're already green?] */
			pline(str, hcolor(NH_GREEN));
		} else
		    pline(str, an(Hallucination ? rndmonnam() : "green slime"));
	    } else
		pline1(str);
	}
	if (i == 3L) {	/* limbs becoming oozy */
	    HFast = 0L;	/* lose intrinsic speed */
	    stop_occupation();
	    if (multi > 0) nomul(0, NULL);
	}
	exercise(A_DEX, FALSE);
}

void
burn_away_slime()
{
	if (Slimed) {
	    pline_The("slime that covers you is burned away!");
	    Slimed = 0L;
	    flags.botl = 1;
		delayed_killer = 0;
	}
	if (youmonst.mcaterpillars) {
	    pline_The("parasitic caterpillars are burned off!");
	    youmonst.mcaterpillars = FALSE;
	}
	if (youmonst.momud) {
	    pline_The("writhing mud that covers you is burned away!");
	    youmonst.momud = FALSE;
	}
	return;
}

void
melt_frozen_air()
{
	if (FrozenAir) {
	    pline_The("frozen air around you vaporizes!");
	    FrozenAir = 0L;
	    flags.botl = 1;
		delayed_killer = 0;
	}
	return;
}


#endif /* OVLB */
#ifdef OVL0

void
unbind(spir,forced)
long spir;
boolean forced;
{
	unbind_core(spir,forced,FALSE);
}

void
unbind_lifesaving(spir)
long spir;
{
	unbind_core(spir,TRUE,TRUE);
}

void
unbind_core(spir,forced,lifesave_forced)
long spir;
boolean forced;
boolean lifesave_forced;
{
	int i;
	boolean found = FALSE, gnosis = (spir == u.spirit[GPREM_SPIRIT]);
	if(forced && Role_if(PM_ANACHRONOUNBINDER)) return;	
	if(spir==SEAL_JACK && forced && !gnosis && u.ulevel == 1){
		Your("life is saved!");
		pline("Unfortunately, your soul is torn to shreds.");
	}
	
	if(forced && gnosis){
		You("are shaken to your core!");
		return;
	}
	
	if(forced && u.voidChime && !lifesave_forced) return; //void chime allows you to keep spirits bound even if you break their taboos.
	
	if(spir == SEAL_ORTHOS && Hallucination) pline("Orthos has been eaten by a grue!");
	
	if(spir&SEAL_SPECIAL){
		for(i=0;i<(NUMINA-QUEST_SPIRITS);i++){
			if(((spir&(~SEAL_SPECIAL)) >> i) == 1L){
				Your("link with %s has %sbroken.", sealNames[i+(QUEST_SPIRITS-FIRST_SEAL)],forced?"been ":"");
				break;
			}
		}
		if(spir&SEAL_NUMINA) Your("links with the Numina have %sbroken.",forced?"been ":"");
	} else for(i=0;i<QUEST_SPIRITS;i++){
		if((spir >> i) == 1L){
			if(!Role_if(PM_ANACHRONOUNBINDER)) Your("link with %s has %sbroken.", sealNames[i],forced?"been ":"");
			break;
		}
	}
	
	if(!(spir&SEAL_SPECIAL)){
		int j;
		if(u.sealsActive&spir){
			u.sealsActive &= ~spir;
			if(spir&int_spirits) u.intSpirits--;
			else if(spir&wis_spirits) u.wisSpirits--;
			for(i=0; i<u.sealCounts; i++){
				if(u.spirit[i] == spir){
					found = TRUE;
				}
				if(found){
					if(i<u.sealCounts-1){
						u.spirit[i] = u.spirit[i+1];
						u.spiritT[i] = u.spiritT[i+1];
					} else {
						u.spirit[i]=0;
						u.spiritT[i]=0;
						u.sealCounts--;
					}
				}
			}
			if(!found){
				if(u.spirit[CROWN_SPIRIT] == spir){
					u.spirit[CROWN_SPIRIT] = 0;
				}
				else if(u.spirit[GPREM_SPIRIT] == spir){
					u.spirit[GPREM_SPIRIT] = 0;
				}
			}

			/* remove properties given by spirit */
			int * spiritprops = spirit_props(decode_sealID(spir));
			for (i = 0; spiritprops[i] != NO_PROP; i++)
				u.uprops[spiritprops[i]].extrinsic &= ~W_SPIRIT;

		} else if(uwep && uwep->oartifact == ART_PEN_OF_THE_VOID){
			uwep->ovara_seals &= ~spir;
			if(uwep->lamplit && !artifact_light(uwep)) end_burn(uwep, TRUE);
		} else if(uswapwep  && uswapwep->oartifact == ART_PEN_OF_THE_VOID){
			uswapwep->ovara_seals &= ~spir;
			if(uswapwep->lamplit && !artifact_light(uswapwep)) end_burn(uswapwep, TRUE);
		}
		if(u.spiritTineA == spir){
			u.spiritTineA = u.spiritTineB;
			u.spiritTineTA = u.spiritTineTB;
			u.spiritTineB = 0;
			u.spiritTineTB = 0;
		} else if(u.spiritTineB == spir){
			u.spiritTineB = 0;
			u.spiritTineTB = 0;
		}
	} else {
		if(spir&SEAL_DAHLVER_NAR) u.wisSpirits--;
		if(spir&SEAL_ACERERAK) u.intSpirits--;
		u.specialSealsActive &= ~spir;
		if(u.specialSealsActive) u.specialSealsActive |= SEAL_SPECIAL;
		if(u.spirit[QUEST_SPIRIT] == spir){
			u.spirit[QUEST_SPIRIT]=0;
			u.spiritT[QUEST_SPIRIT]=0;
		} else if(u.spirit[ALIGN_SPIRIT] == spir){
			u.spirit[ALIGN_SPIRIT]=0;
			u.spiritT[ALIGN_SPIRIT]=0;
		} else if(u.spirit[OTHER_SPIRIT] == spir){
			u.spirit[OTHER_SPIRIT]=0;
			u.spiritT[OTHER_SPIRIT]=0;
		} else if(u.spirit[OUTER_SPIRIT] == spir){
			u.spirit[OUTER_SPIRIT]=0;
			u.spiritT[OUTER_SPIRIT]=0;
		}
		/* remove properties given by spirit */
		int * spiritprops = spirit_props(decode_sealID(spir));
		for (i = 0; spiritprops[i] != NO_PROP; i++)
			u.uprops[spiritprops[i]].extrinsic &= ~W_SPIRIT;
	}
	stop_occupation();
	if(flags.run) nomul(0, NULL);
	vision_full_recalc = 1;	/* visible monsters may have changed */
	doredraw();
	
	//Do this last.  Needed to avoid looping forever on Jack level 1.
	if(forced && !gnosis){
		if(spir == SEAL_ORTHOS && Hallucination) losexp("being eaten by a grue",TRUE,TRUE,TRUE);
		else losexp("shredding of the soul",TRUE,TRUE,TRUE);
		if(flags.run) nomul(0, NULL);
		stop_occupation();
	}
	return;
}

char *spiritFadeTerms[] = {"starting to weaken","growing weaker","very faint","fading fast","about to break"};

void
nh_timeout()
{
	register struct prop *upp;
	int sleeptime, i, m_idx, baseluck = (flags.moonphase == HUNTING_MOON) ? 2 : (flags.moonphase == FULL_MOON) ? 1 : 0;
	
	if (flags.friday13) baseluck -= 1;
	
	if(u.uaesh_duration) u.uaesh_duration--;
	if(u.uhoon_duration) u.uhoon_duration--;
	if(u.ukrau_duration) u.ukrau_duration--;
	if(u.unaen_duration) u.unaen_duration--;
	if(u.uuur_duration) u.uuur_duration--;
	if(u.uvaul_duration) u.uvaul_duration--;
	
	if (u.uluck != baseluck) {
	    int timeout = 600;
	    int time_luck = stone_luck(FALSE);
		int stoneluck = stone_luck(TRUE);
	/* Cursed luckstones slow bad luck timing out; blessed luckstones
	 * slow good luck timing out; normal luckstones slow both;
	 * neither is affected if you don't have a luckstone.
	 * Luck is based at 0 usually, +1 if a full moon and -1 on Friday 13th
	 */
	    if (has_luckitem() && 
		    (!time_luck ||
		     (time_luck > 0 && u.uluck > baseluck) ||
		     (time_luck < 0 && u.uluck < baseluck))
		) {
			/* The slowed timeout depends on the distance between your 
			 * luck (not including luck bonuses) and your base luck.
			 * 
			 * distance	timeout
			 * --------------------
			 *  1		20000
			 *  2		18000
			 *  3		16000
			 *  4		14000
			 *  5		12000
			 *  6		10000
			 *  7		 8000
			 *  8		 6000
			 *  9		 4000
			 *  10		 2000
			 *  11		(600)
			 */ 
			int base_dist = u.uluck - baseluck;
			int slow_timeout;
			if (!Role_if(PM_CONVICT) || 
				stoneluck < 0 || 
				base_dist <= stoneluck*3
			){
				if(base_dist < 0) base_dist *= -1; /* magnitude only */
				slow_timeout = 22000 - 2000 * (base_dist);
				if (slow_timeout > timeout) timeout = slow_timeout;
			}
	    }

	    if (u.uhave.amulet || godlist[u.ualign.god].anger) timeout = timeout / 2;

	    if (moves >= u.luckturn + timeout) {
			if(u.uluck > baseluck)
			u.uluck--;
			else if(u.uluck < baseluck)
			u.uluck++;
			u.luckturn = moves;
	    }
	}
    if(HMagical_breathing) gillyweed_dialogue();
#ifdef CONVICT
    if(Phasing) phasing_dialogue();
#endif /* CONVICT */
	if(u.uprops[SANCTUARY].intrinsic&TIMEOUT){
		u.uprops[SANCTUARY].intrinsic--;
		if(!u.uprops[SANCTUARY].intrinsic){
			if(!Blind)
				pline("The shimmering light fades.");
			else You_feel("exposed to harm once more.");
		}
	}
	if(Invulnerable) return; /* things past this point could kill you */
	if(Stoned) stoned_dialogue();
	if(Golded) golded_dialogue();
	if(Salted) salted_dialogue();
	if(Slimed) slime_dialogue();
	if(Vomiting) vomiting_dialogue();
	if((Strangled || FrozenAir || BloodDrown) && !Breathless) choke_dialogue();
	if(u.mtimedone && !--u.mtimedone) {
		if (Unchanging)
			u.mtimedone = rnd(100*youmonst.data->mlevel + 1);
		else {
			rehumanize();
			change_gevurah(1); //cheated death.
		}
	}
	if(u.ucreamed) u.ucreamed--;

	/* Dissipate spell-based protection. */
	if (u.usptime) {
	    if (--u.usptime == 0 && u.uspellprot) {
		u.usptime = u.uspmtime;
		u.uspellprot--;
		find_ac();
		if (!Blind)
		    Norep("The %s haze around you %s.", hcolor(NH_GOLDEN),
			  u.uspellprot ? "becomes less dense" : "disappears");
	    }
	}
	
	if(u.sowdisc) u.sowdisc--; //timeout for discord
	if(u.voidChime){
		u.voidChime--;
		if(!u.voidChime){
			int i;
			int * spiritprops;
			if (u.spiritTineA) {
				u.sealsActive &= (~u.spiritTineA);
				if (u.spiritTineA&wis_spirits) u.wisSpirits--;
				if (u.spiritTineA&int_spirits) u.intSpirits--;
				for (i = 0, spiritprops = spirit_props(decode_sealID(u.spiritTineA)); spiritprops[i] != NO_PROP; i++)
					u.uprops[spiritprops[i]].extrinsic &= ~W_SPIRIT;
			}
			if (u.spiritTineB) {
				u.sealsActive &= (~u.spiritTineB);
				if (u.spiritTineB&wis_spirits) u.wisSpirits--;
				if (u.spiritTineB&int_spirits) u.intSpirits--;
				for (i = 0, spiritprops = spirit_props(decode_sealID(u.spiritTineB)); spiritprops[i] != NO_PROP; i++)
					u.uprops[spiritprops[i]].extrinsic &= ~W_SPIRIT;
			}
			if(uwep && uwep->oartifact==ART_PEN_OF_THE_VOID){
				uwep->ovara_seals = 0;
				uwep->ovara_seals |= u.spiritTineA;
				uwep->ovara_seals |= u.spiritTineB;
				if(artifact_light(uwep) && !uwep->lamplit) begin_burn(uwep);
				else if(!artifact_light(uwep) && uwep->lamplit) end_burn(uwep, TRUE);
			}
			else if(uswapwep && uswapwep->oartifact==ART_PEN_OF_THE_VOID){
				uswapwep->ovara_seals = 0;
				uswapwep->ovara_seals |= u.spiritTineA;
				uswapwep->ovara_seals |= u.spiritTineB;
				// if(artifact_light(uswapwep) && !uswapwep->lamplit) begin_burn(uswapwep);
				// else if(!artifact_light(uswapwep) && uswapwep->lamplit) end_burn(uswapwep, TRUE);
			}
		}
	}
	if(u.spirit[ALIGN_SPIRIT] & SEAL_YOG_SOTHOTH && carrying_art(ART_SILVER_KEY))
		u.spiritT[ALIGN_SPIRIT]++;
	if(!u.voidChime && !Role_if(PM_ANACHRONOUNBINDER)){
		while(u.spirit[0] && u.spiritT[0] < moves) unbind(u.spirit[0],0);
		if(u.spiritTineB && u.spiritTineTB < moves) unbind(u.spiritTineB,0);
		if(u.spiritTineA && u.spiritTineTA < moves) unbind(u.spiritTineA,0);
		if(u.spirit[QUEST_SPIRIT] && u.spiritT[QUEST_SPIRIT] < moves
			&& !((u.spirit[QUEST_SPIRIT] & SEAL_BLACK_WEB) && Role_if(PM_ANACHRONONAUT)))
			unbind(u.spirit[QUEST_SPIRIT],0);
		if(u.spirit[ALIGN_SPIRIT] && u.spiritT[ALIGN_SPIRIT] < moves) 
			unbind(u.spirit[ALIGN_SPIRIT],0);
		if(u.spirit[OTHER_SPIRIT] && u.spiritT[OTHER_SPIRIT] < moves) 
			unbind(u.spirit[OTHER_SPIRIT],0);
		if(u.spirit[CROWN_SPIRIT] && u.spiritT[CROWN_SPIRIT] < moves) 
			unbind(u.spirit[CROWN_SPIRIT],0);
		if(u.spirit[GPREM_SPIRIT] && u.spiritT[GPREM_SPIRIT] < moves) 
			unbind(u.spirit[GPREM_SPIRIT],0);
		//if(u.spirit[OUTER_SPIRIT] && u.spiritT[OUTER_SPIRIT] < moves) 
			//unbind(u.spirit[OUTER_SPIRIT]); Numina does not time out
	}
	if((u.sealsActive || u.specialSealsActive) && !Role_if(PM_ANACHRONOUNBINDER)){
		int remaining;
		for(i=0;i<OUTER_SPIRIT;i++){
			remaining = 0;
			if(u.spiritT[i] > moves + 625) continue;
			else if(u.spiritT[i] <= moves) continue;
			else if(u.spiritT[i] == moves + 625) remaining = 1;
			else if(u.spiritT[i] == moves + 125) remaining = 2;
			else if(u.spiritT[i] == moves + 25) remaining = 3;
			else if(u.spiritT[i] == moves + 5) remaining = 4;
			else if(u.spiritT[i] == moves + 1) remaining = 5;
			if(remaining){
				int j;
				if(u.spirit[i]&SEAL_SPECIAL){
					for(j=0;j<32;j++){
						if(((u.spirit[i]&(~SEAL_SPECIAL)) >> j) == 1L){
							Your("link with %s is %s.", sealNames[j+(QUEST_SPIRITS-FIRST_SEAL)], spiritFadeTerms[remaining-1]);
						}
					}
				} else {
					for(j=0;j<32;j++){
						if((u.spirit[i] >> j) == 1L){
							Your("link with %s is %s.", sealNames[j], spiritFadeTerms[remaining-1]);
						}
					}
				}
			}
		}
	}
	if(Strangled || FrozenAir || BloodDrown){
		if(BloodDrown){
			pline("Your lungs are full of blood!");
			water_damage(invent, FALSE, FALSE, WD_BLOOD, &youmonst);
		}
		if(Breathless);//Do nothing
		else if(u.divetimer > 1) u.divetimer--;
		else {
			killer_format = KILLED_BY;
			killer = ((HStrangled & TIMEOUT) && delayed_killer) ? delayed_killer
				: (u.uburied || FrozenAir) ? "suffocation"
				: BloodDrown ? "drowning in blood"
				: "strangulation";
			done(((HStrangled & TIMEOUT) && delayed_killer) ? CHOKING : DIED);
		}
	} else if(Drowning && u.divetimer > 0){
		u.divetimer--;
		if(u.divetimer<=3) You("are running short on air.");
		if(u.divetimer==1) You("are running out of air!");
	} else if (!u.usubwater){ /* limited duration dive, 2 turns to 6 turns naturally, 8 turns with magic */ 
		if(u.divetimer < (ACURR(A_CON))/3 && !u.ustuck && (u.divetimer == 0 || (!Babble && !Screaming))) u.divetimer++;
		else if(u.divetimer > (ACURR(A_CON))/3) u.divetimer--;
	}

	if((Babble || Screaming) && !Strangled && !FrozenAir && !BloodDrown && u.divetimer > 1)
		u.divetimer--;

	if(u.divetimer<=0){
		You("can't hold your breath any longer.");
		if((!Swimming && !Amphibious && is_pool(u.ux,u.uy, FALSE)) || is_3dwater(u.ux,u.uy)) {
			u.dx = u.dy = 0;
			drown();
		}
		u.usubwater = 0;
		vision_full_recalc = 1;
		vision_recalc(2);	/* unsee old position */
		doredraw();
	}
#ifdef STEED
	if (u.ugallop) {
	    if (--u.ugallop == 0L && u.usteed)
	    	pline("%s stops galloping.", Monnam(u.usteed));
	}
#endif

	for(upp = u.uprops; upp < u.uprops+SIZE(u.uprops); upp++){
		if(upp - u.uprops == SANCTUARY)
			continue; /*Already decremented above. This should be redundant as properties don't time out while invulnerable */
		if((youracedata->mtyp == PM_SHOGGOTH || youracedata->mtyp == PM_PRIEST_OF_GHAUNADAUR) && upp - u.uprops == BLINDED
			&&  upp->intrinsic & TIMEOUT
		){
			upp->intrinsic &= ~TIMEOUT;
			upp->intrinsic++;
			You("form new eyes.");
		}
		if((youracedata->mtyp == PM_BLASPHEMOUS_LURKER || youracedata->mtyp == PM_LURKING_ONE) && upp - u.uprops == BLINDED
			&&  upp->intrinsic & TIMEOUT
		){
			upp->intrinsic &= ~TIMEOUT;
			upp->intrinsic++;
			You("open more eyes.");
		}
		if (!(upp->intrinsic & TIMEOUT_INF)
			&& (upp->intrinsic & TIMEOUT)
			&& !(--upp->intrinsic & TIMEOUT)	/* decremented here */
			){
		switch(upp - u.uprops){
		case FIRE_RES:
			You_feel("warmer!");
			if(HFire_resistance){
				You(Hallucination ? "still be chillin', tho'." :
					"still feel a bit chill, though.");
			}
		break;
		case SLEEP_RES:
			You_feel("tired!");
			if(HSleep_resistance){
				You_feel("wide awake the next moment, though.");
			}
		break;
		case COLD_RES:
			You_feel("cooler!");
			if(HCold_resistance){
				You("still feel full of hot air, though.");
			}
		break;
		case SHOCK_RES:
			You_feel("conductive!");
			if(HShock_resistance){
				pline("...But only a bit.");
			}
		break;
		case ACID_RES:
			Your("skin feels more sensitive!");
			if(HAcid_resistance){
				pline("...But only a bit.");
			}
		break;
		case DRAIN_RES:
			You_feel("less energetic!");
			if(HDrain_resistance){
				pline("...But only a bit.");
			}
		break;
		case GAZE_RES:
			You_feel("self-conscious!");
			if(HGaze_immune){
				pline("...But only a bit.");
			}
		break;
		case DISPLACED:
			if(Hallucination){
				You("calm down");
				if(Displaced){
					pline("...But only a bit.");
				}
			}
			if(!Displaced) Your("outline ceases shimmering.");
			else Your("outline becomes more distinct.");
		break;
		case STONED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				/* leaving killer_format would make it
				   "petrified by petrification" */
				killer_format = NO_KILLER_PREFIX;
				killer = "killed by petrification";
			}
			if (!u.uconduct.killer){
				//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
				IMPURITY_UP(u.uimp_murder)
				IMPURITY_UP(u.uimp_bloodlust)
			}
			done(STONING);
			break;
		case GOLDED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				/* leaving killer_format would make it
				   "petrified by petrification" */
				killer_format = NO_KILLER_PREFIX;
				killer = "killed by turning to gold";
			}
			if (!u.uconduct.killer){
				//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
				IMPURITY_UP(u.uimp_murder)
				IMPURITY_UP(u.uimp_bloodlust)
			}
			done(GOLDING);
			break;
		case SALTED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				/* leaving killer_format would make it
				   "petrified by petrification" */
				killer_format = NO_KILLER_PREFIX;
				killer = "killed by turning to salt";
			}
			if (!u.uconduct.killer){
				//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
				IMPURITY_UP(u.uimp_murder)
				IMPURITY_UP(u.uimp_bloodlust)
			}
			done(SALTING);
			break;
		case FROZEN_AIR:
			pline("The frozen air surrounding you becomes vapor.");
		break;
		case SLIMED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				killer_format = NO_KILLER_PREFIX;
				killer = "turned into green slime";
			}
			if (!u.uconduct.killer){
				//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
				IMPURITY_UP(u.uimp_murder)
				IMPURITY_UP(u.uimp_bloodlust)
			}
			done(TURNED_SLIME);
			break;
		case VOMITING:
			make_vomiting(0L, TRUE);
			break;
		case SICK:
			You("die from your illness.");
			killer_format = KILLED_BY_AN;
			killer = u.usick_cause;
			if ((m_idx = name_to_mon(killer)) >= LOW_PM) {
			    if (type_is_pname(&mons[m_idx])) {
				killer_format = KILLED_BY;
			    } else if (mons[m_idx].geno & G_UNIQ) {
				killer = the(killer);
				Strcpy(u.usick_cause, killer);
				killer_format = KILLED_BY;
			    }
			}
			u.usick_type = 0;
			done(POISONING);
			break;
		case FAST:
			if (!Very_fast)
				You_feel("yourself slowing down%s.",
							Fast ? " a bit" : "");
			break;
		case CONFUSION:
			HConfusion = 1; /* So make_confused works properly */
			make_confused(0L, TRUE);
			stop_occupation();
			break;
		case STUNNED:
			HStun = 1;
			make_stunned(0L, TRUE);
			stop_occupation();
			break;
		case DOUBT:
			HDoubt = 1;
			make_doubtful(0L, TRUE);
			stop_occupation();
			break;
		case BLINDED:
			Blinded = 1;
			make_blinded(0L, TRUE);
			stop_occupation();
			break;
		case INVIS:
			newsym(u.ux,u.uy);
			if (!Invis && !BInvis && !Blind) {
			    You(!See_invisible(u.ux,u.uy) ?
				    "are no longer invisible." :
				    "can no longer see through yourself.");
			    stop_occupation();
			}
			break;
		case SEE_INVIS:
			set_mimic_blocking(); /* do special mimic handling */
			see_monsters();		/* make invis mons appear */
			newsym(u.ux,u.uy);	/* make self appear */
			stop_occupation();
			break;
		case WOUNDED_LEGS:
			heal_legs();
			stop_occupation();
			break;
		case HALLUC:
			HHallucination = 1;
			(void) make_hallucinated(0L, TRUE, 0L);
			stop_occupation();
			break;
		case SLEEPING:
			if (unconscious() || Sleep_resistance)
				HSleeping += rnd(100);
			else if (Sleeping) {
				You("fall asleep.");
				sleeptime = rnd(20);
				fall_asleep(-sleeptime, TRUE);
				HSleeping += sleeptime + rnd(100);
			}
			break;
		case LEVITATION:
			(void) float_down(I_SPECIAL|TIMEOUT, 0L);
			break;
		case STRANGLED:
			if(!Breathless && !Strangled) Your("throat opens up!");
		break;
		case FUMBLING:
			/* call this only when a move took place.  */
			/* otherwise handle fumbling msgs locally. */
			if (u.umoved && !Levitation) {
			    slip_or_trip();
			    nomul(-2, "fumbling");
			    nomovemsg = "";
			    /* The more you are carrying the more likely you
			     * are to make noise when you fumble.  Adjustments
			     * to this number must be thoroughly play tested.
			     */
			    if ((inv_weight() > -500)) {
				You("make a lot of noise!");
				wake_nearby();
			    }
			}
			if (Fumbling)
			    HFumbling += rnd(20);
			break;
		case INFRAVISION:
		case BLOODSENSE:
		case LIFESENSE:
		case SENSEALL:
		case OMNISENSE:
		case EARTHSENSE:
		case DETECT_MONSTERS:
			see_monsters();
			break;
		case NULLMAGIC:
			pline("The shimmering film around your body pops!");
		break;
		case CHASTITY:
			You_feel("less chaste.");
		break;
		case CLEAVING:
			//
		break;
		case GOOD_HEALTH:
			You_feel("less healthy.");
		break;
		case RAPID_HEALING:
			if((!Upolyd && u.uhp < u.uhpmax) ||
				(Upolyd && u.mh < u.mhmax)
			)
				pline("Your wounds stop knitting shut.");
		break;
		case DESTRUCTION:
			pline("You stop radiating waves of destruction.");
		break;
		case PRESERVATION:
			pline("The rubbery film around your body vanishes!");
		break;
		case QUICK_DRAW:
			pline("Your %s feel less swift.", makeplural(body_part(HAND)));
		break;
		case CLEAR_THOUGHTS:
			if(u.usanity < 100)
				pline("Your mind feels less clear.");
		break;
		case XRAY_VISION:
			if (Hallucination)
				pline("Your visions recede.");
			else
				pline("Your vision recedes.");
			vision_full_recalc = 1;
		break;
		case NORMALVISION:
		case LOWLIGHTSIGHT:
		case ELFSIGHT:
		case DARKSIGHT:
		case CATSIGHT:
		case EXTRAMISSION:
		case ECHOLOCATION:
		case DARKVISION_ONLY:
			vision_full_recalc = 1;
			see_monsters();
		break;
		}
		}
	}

	run_timers();
}

#endif /* OVL0 */
#ifdef OVL1

void
fall_asleep(how_long, wakeup_msg)
int how_long;
boolean wakeup_msg;
{
	if(u.sealsActive&SEAL_HUGINN_MUNINN){
		unbind(SEAL_HUGINN_MUNINN,TRUE);
		return; //expel and end
	}
	stop_occupation();
	nomul(how_long, "sleeping");
	/* generally don't notice sounds while sleeping */
	if (wakeup_msg && multi == how_long) {
	    /* caller can follow with a direct call to Hear_again() if
	       there's a need to override this when wakeup_msg is true */
	    flags.soundok = 0;
	    afternmv = Hear_again;	/* this won't give any messages */
	}
	/*Adjust Android timeouts*/
	u.nextsleep = max(u.nextsleep, monstermoves);
	u.lastslept = monstermoves;
	struct obj *puzzle;
	if(u.uhyperborean_steps < 6 && (puzzle = get_most_complete_puzzle())){
		u.puzzle_time = 6*(1+puzzle->ovar1_puzzle_steps)*(27-ACURR(A_INT))/2;
		if(ESleeping)
			u.puzzle_time = (u.puzzle_time + 1)/2; //Restful sleep
		Your("%s begin working the disks and pegs of %s!", makeplural(body_part(FINGER)), the(xname(puzzle)));
	}
	/* early wakeup from combat won't be possible until next monster turn */
	u.usleep = monstermoves;
	nomovemsg = wakeup_msg ? "You wake up." : You_can_move_again;
}

//#ifdef FIREARMS
/* Attach an explosion timeout to a given explosive device */
void
attach_bomb_blow_timeout(bomb, fuse, yours)
struct obj *bomb;
int fuse;
boolean yours;
{
	long expiretime;	

	if (bomb->cursed && !rn2(2)) return; /* doesn't arm if not armed */

	/* Now if you play with other people's property... */
	if (yours && ((!carried(bomb) && costly_spot(bomb->ox, bomb->oy) &&
		!bomb->no_charge) || bomb->unpaid)) {
	    verbalize("You play with it, you pay for it!");
	    bill_dummy_object(bomb);
	}

	expiretime = stop_timer(BOMB_BLOW, bomb->timed);
	if (expiretime > 0L) fuse = fuse - (expiretime - monstermoves);
	bomb->yours = yours;
	bomb->oarmed = TRUE;

	(void) start_timer((long)fuse, TIMER_OBJECT, BOMB_BLOW, (genericptr_t)bomb);
}

/* timer callback routine: detonate the explosives */
void
bomb_blow(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *bomb;
	xchar x,y;
	boolean silent, underwater;
	struct monst *mtmp = (struct monst *)0;

	bomb = (struct obj *) arg;

	silent = (timeout != monstermoves);     /* exploded while away */

	if (get_obj_location(bomb, &x, &y, BURIED_TOO | CONTAINED_TOO)) {
		switch(bomb->where) {		
		    case OBJ_MINVENT:
		    	mtmp = bomb->ocarry;
			if (bomb == MON_WEP(mtmp)) {
			    bomb->owornmask &= ~W_WEP;
			    MON_NOWEP(mtmp);
			}
			if (bomb == MON_SWEP(mtmp)) {
			    bomb->owornmask &= ~W_SWAPWEP;
			    MON_NOSWEP(mtmp);
			}
			if (!silent) {
			    if (canseemon(mtmp))
				You("see %s engulfed in an explosion!", mon_nam(mtmp));
			}
		    	mtmp->mhp -= d(2,5);
			if(mtmp->mhp < 1) {
				if(!bomb->yours) 
					monkilled(mtmp, 
						  (silent ? "" : "explosion"),
						  AD_PHYS);
				else xkilled(mtmp, !silent);
			}
			break;
		    case OBJ_INVENT:
		    	/* This shouldn't be silent! */
			pline("Something explodes inside your knapsack!");
			if (bomb == uwep) {
			    uwepgone();
			    stop_occupation();
			} else if (bomb == uswapwep) {
			    uswapwepgone();
			    stop_occupation();
			} else if (bomb == uquiver) {
			    uqwepgone();
			    stop_occupation();
			}
		    	losehp(d(2,5), "carrying live explosives", KILLED_BY);
		    	break;
		    case OBJ_FLOOR:
			underwater = is_pool(x, y, FALSE);
			if (!silent) {
			    if (x == u.ux && y == u.uy) {
				if (underwater && (Flying || Levitation))
				    pline_The("water boils beneath you.");
				else if (underwater && Wwalking)
				    pline_The("water erupts around you.");
				else pline("A bomb explodes under your %s!",
				  makeplural(body_part(FOOT)));
			    } else if (cansee(x, y))
				You(underwater ?
				    "see a plume of water shoot up." :
				    "see a bomb explode.");
			}
			if (underwater && (Flying || Levitation || Wwalking)) {
			    if (Wwalking && x == u.ux && y == u.uy) {
				struct trap trap;
				trap.ntrap = NULL;
				trap.tx = x;
				trap.ty = y;
				trap.launch.x = -1;
				trap.launch.y = -1;
				trap.ttyp = RUST_TRAP;
				trap.tseen = 0;
				trap.once = 0;
				trap.madeby_u = 0;
				trap.dst.dnum = -1;
				trap.dst.dlevel = -1;
				dotrap(&trap, 0);
			    }
			    goto free_bomb;
			}
		    	break;
		    default:	/* Buried, contained, etc. */
			if (!silent)
			    You_hear("a muffled explosion.");
			goto free_bomb;
			break;
		}
		grenade_explode(bomb, x, y, bomb->yours, silent ? 3 : 0);
		return;
	} /* Migrating grenades "blow up in midair" */

free_bomb:
	obj_extract_self(bomb);
	obfree(bomb, (struct obj *)0);
}
//#endif

/* Attach an egg hatch timeout to the given egg. */
void
attach_egg_hatch_timeout(egg)
struct obj *egg;
{
	int i;

	/* stop previous timer, if any */
	(void) stop_timer(HATCH_EGG, egg->timed);

	/*
	 * Decide if and when to hatch the egg.  The old hatch_it() code tried
	 * once a turn from age 151 to 200 (inclusive), hatching if it rolled
	 * a number x, 1<=x<=age, where x>150.  This yields a chance of
	 * hatching > 99.9993%.  Mimic that here.
	 */
	for (i = (MAX_EGG_HATCH_TIME-50)+1; i <= MAX_EGG_HATCH_TIME; i++)
	    if (rnd(i) > 150) {
		/* egg will hatch */
		(void) start_timer((long)i, TIMER_OBJECT,
						HATCH_EGG, (genericptr_t)egg);
		break;
	    }
}

/* prevent an egg from ever hatching */
void
kill_egg(egg)
struct obj *egg;
{
	/* stop previous timer, if any */
	(void) stop_timer(HATCH_EGG, egg->timed);
}

/* timer callback routine: hatch the given egg */
void
hatch_egg(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *egg;
	struct monst *mon, *mon2;
	coord cc;
	xchar x, y;
	boolean yours, silent, knows_egg = FALSE;
	boolean cansee_hatchspot = FALSE;
	int i, mtyp, hatchcount = 0;

	egg = (struct obj *) arg;
	/* sterilized while waiting */
	if (egg->corpsenm == NON_PM) return;

	mon = mon2 = (struct monst *)0;
	mtyp = big_to_little(egg->corpsenm);
	if(u.silvergrubs && !rn2(20)){
		set_silvergrubs(FALSE);
	}
	if(check_rot(ROT_KIN) && (u.silvergrubs || !rn2(100)) && !(mvitals[PM_MAN_FLY].mvflags&G_GONE && !In_quest(&u.uz))){
		mtyp = PM_MAN_FLY;
		set_silvergrubs(TRUE);
	}
	/* The identity of one's father is learned, not innate */
	yours = (egg->spe || (!flags.female && carried(egg) && !rn2(2)));
	silent = (timeout != monstermoves);	/* hatched while away */

	/* only can hatch when in INVENT, FLOOR, MINVENT */
	if (get_obj_location(egg, &x, &y, 0)) {
	    hatchcount = rnd((int)egg->quan);
	    cansee_hatchspot = cansee(x, y) && !silent;
	    if (!(mons[mtyp].geno & G_UNIQ) &&
		   !(mvitals[mtyp].mvflags & G_GONE && !In_quest(&u.uz))) {
		for (i = hatchcount; i > 0; i--) {
		    if (!enexto(&cc, x, y, &mons[mtyp]) ||
			 !(mon = makemon(&mons[mtyp], cc.x, cc.y, NO_MINVENT)))
			break;
		    /* tame if your own egg hatches while you're on the
		       same dungeon level, or any dragon egg (or metroid) which hatches
		       while it's in your inventory */
		    if ((yours && !silent) ||
			(carried(egg) && mon->data->mlet == S_DRAGON) ||
			(carried(egg) && mon->mtyp == PM_BABY_METROID) ) { //metroid egg
			if ((mon2 = tamedog(mon, (struct obj *)0)) != 0) {
			    mon = mon2;
			    if (carried(egg) && mon->data->mlet != S_DRAGON)
				mon->mtame = 20;
			}
		    }
		    if (mvitals[mtyp].mvflags & G_EXTINCT && !In_quest(&u.uz))
			break;	/* just made last one */
		    mon2 = mon;	/* in case makemon() fails on 2nd egg */
		}
		if (!mon) mon = mon2;
		hatchcount -= i;
		egg->quan -= (long)hatchcount;
	    }
	}
#if 0
	/*
	 * We could possibly hatch while migrating, but the code isn't
	 * set up for it...
	 */
	else if (obj->where == OBJ_MIGRATING) {
	    /*
	    We can do several things.  The first ones that come to
	    mind are:

	    + Create the hatched monster then place it on the migrating
	      mons list.  This is tough because all makemon() is made
	      to place the monster as well.    Makemon() also doesn't
	      lend itself well to splitting off a "not yet placed"
	      subroutine.

	    + Mark the egg as hatched, then place the monster when we
	      place the migrating objects.

	    + Or just kill any egg which gets sent to another level.
	      Falling is the usual reason such transportation occurs.
	    */
	    cansee_hatchspot = FALSE;
	    mon = ???
	    }
#endif

	if (mon) {
	    char monnambuf[BUFSZ], carriedby[BUFSZ];
	    boolean siblings = (hatchcount > 1), redraw = FALSE;

	    if (cansee_hatchspot) {
		Sprintf(monnambuf, "%s%s",
			siblings ? "some " : "",
			siblings ?
			makeplural(m_monnam(mon)) : an(m_monnam(mon)));
		/* we don't learn the egg type here because learning
		   an egg type requires either seeing the egg hatch
		   or being familiar with the egg already,
		   as well as being able to see the resulting
		   monster, checked below
		*/
	    }
	    switch (egg->where) {
		case OBJ_INVENT:
		    knows_egg = TRUE; /* true even if you are blind */
		    if (!cansee_hatchspot)
			You_feel("%s %s from your pack!", something,
			    locomotion(mon, "drop"));
		    else
			You("see %s %s out of your pack!",
			    monnambuf, locomotion(mon, "drop"));
		    if (yours) {
			pline("%s cries sound like \"%s%s\"",
			    siblings ? "Their" : "Its",
			    flags.female ? "mommy" : "daddy",
			    egg->spe ? "." : "?");
		    } else if (mon->data->mlet == S_DRAGON) {
			verbalize("Gleep!");		/* Mything eggs :-) */
		    }
		    break;

		case OBJ_FLOOR:
		    if (cansee_hatchspot) {
			knows_egg = TRUE;
			You("see %s hatch.", monnambuf);
			redraw = TRUE;	/* update egg's map location */
		    }
		    break;

		case OBJ_MINVENT:
		    if (cansee_hatchspot) {
			/* egg carring monster might be invisible */
			if (canseemon(egg->ocarry)) {
			    Sprintf(carriedby, "%s pack",
				     s_suffix(a_monnam(egg->ocarry)));
			    knows_egg = TRUE;
			}
			else if (is_pool(mon->mx, mon->my, FALSE))
			    Strcpy(carriedby, "empty water");
			else
			    Strcpy(carriedby, "thin air");
			You("see %s %s out of %s!", monnambuf,
			    locomotion(mon, "drop"), carriedby);
		    }
		    break;
#if 0
		case OBJ_MIGRATING:
		    break;
#endif
		default:
		    impossible("egg hatched where? (%d)", (int)egg->where);
		    break;
	    }

	    if (cansee_hatchspot && knows_egg)
		learn_egg_type(mtyp);

	    if (egg->quan > 0) {
		/* still some eggs left */
		attach_egg_hatch_timeout(egg);
		if (egg->timed) {
		    /* replace ordinary egg timeout with a short one */
		    (void) stop_timer(HATCH_EGG, egg->timed);
		    (void) start_timer((long)rnd(12), TIMER_OBJECT,
					HATCH_EGG, (genericptr_t)egg);
		}
	    } else if (carried(egg)) {
		useup(egg);
	    } else {
		/* free egg here because we use it above */
		obj_extract_self(egg);
		obfree(egg, (struct obj *)0);
	    }
	    if (redraw) newsym(x, y);
	}
}

/* Learn to recognize eggs of the given type. */
void
learn_egg_type(mtyp)
int mtyp;
{
	/* baby monsters hatch from grown-up eggs */
	mtyp = little_to_big(mtyp, rn2(2));
	mvitals[mtyp].mvflags |= MV_KNOWS_EGG;
	/* we might have just learned about other eggs being carried */
	update_inventory();
}

/* Attach a fig_transform timeout to the given figurine. */
void
attach_fig_transform_timeout(figurine)
struct obj *figurine;
{
	int i;

	/* stop previous timer, if any */
	(void) stop_timer(FIG_TRANSFORM, figurine->timed);

	/*
	 * Decide when to transform the figurine.
	 */
	i = rnd(9000) + 200;
	/* figurine will transform */
	(void) start_timer((long)i, TIMER_OBJECT,
				FIG_TRANSFORM, (genericptr_t)figurine);
}

/* give a fumble message */
STATIC_OVL void
slip_or_trip()
{
	struct obj *otmp = vobj_at(u.ux, u.uy);
	const char *what, *pronoun;
	char buf[BUFSZ];
	boolean on_foot = TRUE;
#ifdef STEED
	if (u.usteed) on_foot = FALSE;
#endif

	if (otmp && on_foot && !u.uinwater && is_pool(u.ux, u.uy, FALSE)) otmp = 0;

	if (otmp && on_foot) {		/* trip over something in particular */
	    /*
		If there is only one item, it will have just been named
		during the move, so refer to by via pronoun; otherwise,
		if the top item has been or can be seen, refer to it by
		name; if not, look for rocks to trip over; trip over
		anonymous "something" if there aren't any rocks.
	     */
	    pronoun = otmp->quan == 1L ? "it" : Hallucination ? "they" : "them";
	    what = !otmp->nexthere ? pronoun :
		  (otmp->dknown || !Blind) ? doname(otmp) :
		  ((otmp = sobj_at(ROCK, u.ux, u.uy)) == 0 ? something :
		  (otmp->quan == 1L ? "a rock" : "some rocks"));
	    if (Hallucination) {
		what = strcpy(buf, what);
		buf[0] = highc(buf[0]);
		pline("Egads!  %s bite%s your %s!",
			what, (!otmp || otmp->quan == 1L) ? "s" : "",
			body_part(FOOT));
	    } else {
		You("trip over %s.", what);
	    }
	} else if (rn2(3) && is_ice(u.ux, u.uy)) {
	    pline("%s %s%s on the ice.",
#ifdef STEED
		u.usteed ? upstart(x_monnam(u.usteed,
				M_HAS_NAME(u.usteed) ? ARTICLE_NONE : ARTICLE_THE,
				(char *)0, SUPPRESS_SADDLE, FALSE)) :
#endif
		"You", rn2(2) ? "slip" : "slide", on_foot ? "" : "s");
	} else {
	    if (on_foot) {
		switch (rn2(4)) {
		  case 1:
			You("trip over your own %s.", Hallucination ?
				"elbow" : makeplural(body_part(FOOT)));
			break;
		  case 2:
			You("slip %s.", Hallucination ?
				"on a banana peel" : "and nearly fall");
			break;
		  case 3:
			You("flounder.");
			break;
		  default:
			You("stumble.");
			break;
		}
	    }
#ifdef STEED
	    else {
		switch (rn2(4)) {
		  case 1:
			Your("%s slip out of the stirrups.", makeplural(body_part(FOOT)));
			break;
		  case 2:
			You("let go of the reins.");
			break;
		  case 3:
			You("bang into the saddle-horn.");
			break;
		  default:
			You("slide to one side of the saddle.");
			break;
		}
		dismount_steed(DISMOUNT_FELL);
	    }
#endif
	}
}

/* Print a lamp flicker message with tailer. */
STATIC_OVL void
see_lamp_flicker(obj, tailer)
struct obj *obj;
const char *tailer;
{
	switch (obj->where) {
	    case OBJ_INVENT:
	    case OBJ_MINVENT:
		pline("%s flickers%s.", Yname2(obj), tailer);
		break;
	    case OBJ_FLOOR:
		You("see %s flicker%s.", an(xname(obj)), tailer);
		break;
	}
}

/* Print a dimming message for brass lanterns. */
STATIC_OVL void
lantern_message(obj)
struct obj *obj;
{
	/* from adventure */
	switch (obj->where) {
	    case OBJ_INVENT:
		Your("lantern is getting dim.");
		if (Hallucination)
		    pline("Batteries have not been invented yet.");
		break;
	    case OBJ_FLOOR:
		You("see a lantern getting dim.");
		break;
	    case OBJ_MINVENT:
		pline("%s lantern is getting dim.",
		    s_suffix(Monnam(obj->ocarry)));
		break;
	}
}

/*
 * Timeout callback for for objects that are burning. E.g. lamps, candles.
 * See begin_burn() for meanings of obj->age and obj->spe.
 */
void
burn_object(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *obj = (struct obj *) arg;
	boolean canseeit, many, menorah, need_newsym;
	xchar x, y;
	char whose[BUFSZ];

	menorah = obj->otyp == CANDELABRUM_OF_INVOCATION;
	many = menorah ? obj->spe > 1 : obj->quan > 1L;

	/* timeout while away */
	if (timeout != monstermoves && (obj->where != OBJ_MINVENT || (
				(!is_dwarf(obj->ocarry->data) || obj->otyp != DWARVISH_HELM)
				&& obj->otyp != LANTERN_PLATE_MAIL
				&& (!is_gnome(obj->ocarry->data) || obj->otyp != GNOMISH_POINTY_HAT)
				&& (!is_szcultist(obj->ocarry->data) || !(obj->otyp == TORCH || obj->otyp == SHADOWLANDER_S_TORCH))
	))) {
	    long how_long = monstermoves - timeout;

		if (how_long >= obj->age){
			obj->age = 0;
			end_burn(obj, FALSE);
			if (menorah) {
				obj->spe = 0;	/* no more candles */
			}
			else if (Is_candle(obj) || obj->otyp == POT_OIL
				|| obj->otyp == SUNROD
				) {
				/* get rid of candles and burning oil potions */
				obj_extract_and_unequip_self(obj);
				obfree(obj, (struct obj *)0);
				obj = (struct obj *) 0;
				//#ifdef FIREARMS
			}
			else if (obj->otyp == SHADOWLANDER_S_TORCH || obj->otyp == TORCH) {
				/* torches may become burnt clubs */
				if (obj_resists(obj, 0, 90))
				{
					obj->otyp = CLUB;
					obj->oclass = WEAPON_CLASS;
					obj->age = monstermoves;
					if (is_flammable(obj) && !obj->oerodeproof)
						obj->oeroded = min(obj->oeroded + 1, 3);
					fix_object(obj);
				}
				else {
					obj_extract_and_unequip_self(obj);
					obfree(obj, (struct obj *)0);
					obj = (struct obj *) 0;
				}
			}
			else if (obj->otyp == STICK_OF_DYNAMITE) {
				bomb_blow((genericptr_t)obj, timeout);
				return;
				//#endif
			}
		}
		else {
			obj->age -= how_long;
		    begin_burn(obj);
	    }
	    return;
	}

	/* only interested in INVENT, FLOOR, and MINVENT */
	if (get_obj_location(obj, &x, &y, 0)) {
	    canseeit = !Blind && cansee(x, y);
	    /* set up `whose[]' to be "Your" or "Fred's" or "The goblin's" */
	    (void) Shk_Your(whose, obj);
	} else {
	    canseeit = FALSE;
	}
	need_newsym = FALSE;

	if(obj->oartifact == ART_HOLY_MOONLIGHT_SWORD){
	        if ((obj->where == OBJ_FLOOR) || 
		    (obj->where == OBJ_MINVENT && 
				((!MON_WEP(obj->ocarry) || MON_WEP(obj->ocarry) != obj)
				&& (!MON_SWEP(obj->ocarry) || MON_SWEP(obj->ocarry) != obj))) ||
		    (obj->where == OBJ_INVENT &&
		    	((!uwep || uwep != obj) &&
		    	 (!u.twoweap || !uswapwep || obj != uswapwep))))
	            lightsaber_deactivate(obj, FALSE);
			if (obj && obj->age && obj->lamplit) /* might be deactivated */
				begin_burn(obj);
	} else switch (obj->otyp) {
	    case POT_OIL:
		    /* this should only be called when we run out */
		    if (canseeit) {
			switch (obj->where) {
			    case OBJ_INVENT:
			    case OBJ_MINVENT:
				pline("%s potion of oil has burnt away.",
				    whose);
				break;
			    case OBJ_FLOOR:
				You("see a burning potion of oil go out.");
				need_newsym = TRUE;
				break;
			}
		    }
		    end_burn(obj, FALSE);	/* turn off light source */
		    obj_extract_self(obj);
		    obfree(obj, (struct obj *)0);
		    obj = (struct obj *) 0;
		    break;

	    case TONITRUS:
			/* even if blind you'll know if holding it */
			if (canseeit || obj->where == OBJ_INVENT) {
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
					pline("%s %s has gone out.",
					    whose, xname(obj));
				    break;
				case OBJ_FLOOR:
					You("see %s go out.",
					    an(xname(obj)));
				    break;
			    }
			}
			end_burn(obj, FALSE);
			break;
   	    case DWARVISH_HELM:
	    case LANTERN:
	    case LANTERN_PLATE_MAIL:
	    case OIL_LAMP:
		switch((int)obj->age) {
		    case 150:
		    case 100:
		    case 50:
			if (canseeit) {
			    if (obj->otyp == LANTERN
				 || obj->otyp == DWARVISH_HELM
				 || obj->otyp == LANTERN_PLATE_MAIL
				)
					lantern_message(obj);
			    else
				see_lamp_flicker(obj,
				    obj->age == 50L ? " considerably" : "");
			}
			//Dwarvish lamps don't go out in monster inventories
			if(obj->where == OBJ_MINVENT && 
				((is_dwarf(obj->ocarry->data) && obj->otyp == DWARVISH_HELM)
				 || obj->otyp == LANTERN_PLATE_MAIL)
			) obj->age = (long) rn1(250,250);
			break;

		    case 25:
			if (canseeit) {
			    if (obj->otyp == LANTERN 
				|| obj->otyp == DWARVISH_HELM
				|| obj->otyp == LANTERN_PLATE_MAIL
				)
				lantern_message(obj);
			    else {
				switch (obj->where) {
				    case OBJ_MINVENT:
				    case OBJ_INVENT:
					pline("%s %s seems about to go out.",
					    whose, xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s about to go out.",
					    an(xname(obj)));
					break;
				}
			    }
			}
			//Dwarvish lamps don't go out in monster inventories
			if(obj->where == OBJ_MINVENT && 
				((is_dwarf(obj->ocarry->data) && obj->otyp == DWARVISH_HELM)
				 || obj->otyp == LANTERN_PLATE_MAIL)
			) obj->age = (long) rn1(50,25);
			break;

		    case 0:
			/* even if blind you'll know if holding it */
			if (canseeit || obj->where == OBJ_INVENT) {
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
			    if (obj->otyp == LANTERN 
				|| obj->otyp == LANTERN_PLATE_MAIL
				|| obj->otyp == DWARVISH_HELM)
					pline("%s lantern has run out of power.",
					    whose);
				    else
					pline("%s %s has gone out.",
					    whose, xname(obj));
				    break;
				case OBJ_FLOOR:
			    if (obj->otyp == LANTERN 
				|| obj->otyp == LANTERN_PLATE_MAIL
				|| obj->otyp == DWARVISH_HELM)
					You("see a lantern run out of power.");
				    else
					You("see %s go out.",
					    an(xname(obj)));
				    break;
			    }
			}
			end_burn(obj, FALSE);
			break;

		    default:
			/*
			 * Someone added fuel to the lamp while it was
			 * lit.  Just fall through and let begin burn
			 * handle the new age.
			 */
			break;
		}

		if (obj->age)
		    begin_burn(obj);

		break;

	    case CANDELABRUM_OF_INVOCATION:
	    case TALLOW_CANDLE:
	    case WAX_CANDLE:
		case GNOMISH_POINTY_HAT:
		switch (obj->age) {
		    case 75:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s %scandle%s getting short.",
					whose,
					menorah ? "candelabrum's " : "",
					many ? "s are" : " is");
				    break;
				case OBJ_FLOOR:
				    You("see %scandle%s getting short.",
					    menorah ? "a candelabrum's " :
						many ? "some " : "a ",
					    many ? "s" : "");
				    break;
			    }
			break;

		    case 15:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline(
					"%s %scandle%s flame%s flicker%s low!",
					    whose,
					    menorah ? "candelabrum's " : "",
					    many ? "s'" : "'s",
					    many ? "s" : "",
					    many ? "" : "s");
				    break;
				case OBJ_FLOOR:
				    You("see %scandle%s flame%s flicker low!",
					    menorah ? "a candelabrum's " :
						many ? "some " : "a ",
					    many ? "s'" : "'s",
					    many ? "s" : "");
				    break;
			    }
			if(obj->where == OBJ_MINVENT &&
				is_gnome(obj->ocarry->data) &&
				obj->otyp == GNOMISH_POINTY_HAT){
				obj->age = (long) rn1(50,15);
			}
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
			    if (menorah) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s candelabrum's flame%s.",
					    whose,
					    many ? "s die" : " dies");
					break;
				    case OBJ_FLOOR:
					You("see a candelabrum's flame%s die.",
						many ? "s" : "");
					break;
				}
				}else if(obj->otyp == GNOMISH_POINTY_HAT){
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s pointy hat's candle dies.",
					    whose);
					break;
				    case OBJ_FLOOR:
						You("see a pointy hat's candle die.");
					break;
				}
			    } else {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s %s consumed!",
					    whose,
					    xname(obj),
					    many ? "are" : "is");
					break;
				    case OBJ_FLOOR:
					/*
					You see some wax candles consumed!
					You see a wax candle consumed!
					*/
					You("see %s%s consumed!",
					    many ? "some " : "",
					    many ? xname(obj):an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ?
					(many ? "They shriek!" :
						"It shrieks!") :
					Blind ? "" :
					    (many ? "Their flames die." :
						    "Its flame dies."));
			    }
			}
			end_burn(obj, FALSE);

			if (menorah) {
			    obj->spe = 0;
			} else if(obj->otyp != GNOMISH_POINTY_HAT){
			    obj_extract_self(obj);
			    obfree(obj, (struct obj *)0);
			    obj = (struct obj *) 0;
			}
			break;
		}
		
		if (obj && obj->age)
		    begin_burn(obj);

		break;

		case TORCH:
		switch (obj->age) {
		    case 150:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s torch is burning down.", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch burning down.");
				    break;
			    }
			break;

		    case 50:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline("%s torch's flame flickers low!", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch's flame flicker low!");
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s is consumed!",
					    whose,
					    xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s consumed!", an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ? 
						"It shrieks!" :
					Blind ? "" :
					    "Its flame dies.");
			}
			end_burn(obj, FALSE);

			/* torches may become burnt clubs */
			if (obj_resists(obj, 0, 90))
			{
				obj->otyp = CLUB;
				obj->oclass = WEAPON_CLASS;
				obj->age = monstermoves;
				if (is_flammable(obj) && !obj->oerodeproof)
					obj->oeroded = min(obj->oeroded + 1, 3);
				fix_object(obj);
				break;	/* don't do other torch things */
			}
			else {
				obj_extract_and_unequip_self(obj);
				obfree(obj, (struct obj *)0);
				obj = (struct obj *) 0;
			}
		}
		
		if (obj && obj->age && obj->otyp == TORCH){
			if(obj->where == OBJ_MINVENT &&
				is_szcultist(obj->ocarry->data)
			){
				obj->age = (long) rn1(150,150);
			}
			end_burn(obj, FALSE);
		    begin_burn(obj);
		}
		break;
		case SUNROD:
		switch (obj->age) {
		    case 150:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s sunrod is fading.", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a sunrod fading.");
				    break;
			    }
			break;

		    case 50:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline("%s sunrod's light flickers faintly!", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a sunrod's light flicker faintly!");
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s is consumed!",
					    whose,
					    xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s consumed!", an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ? 
						"It shrieks!" :
					Blind ? "" :
					    "Its faint light dies.");
			}
			end_burn(obj, FALSE);

			obj_extract_and_unequip_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		}
		
		if (obj && obj->age){
			end_burn(obj, FALSE);
		    begin_burn(obj);
		}
		break;
		case SHADOWLANDER_S_TORCH:
		switch (obj->age) {
		    case 150:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s torch is burning down.", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch burning down.");
				    break;
			    }
			break;

		    case 50:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline("%s torch's shadowy flame flickers low!", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch's shadowy flame flicker low!");
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s is consumed!",
					    whose,
					    xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s consumed!", an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ? 
						"It shrieks!" :
					Blind ? "" :
					    "Its shadowy flame dies.");
			}
			end_burn(obj, FALSE);

			/* torches may become burnt clubs */
			if (obj_resists(obj, 0, 90))
			{
				obj->otyp = CLUB;
				obj->oclass = WEAPON_CLASS;
				obj->age = monstermoves;
				if (is_flammable(obj) && !obj->oerodeproof)
					obj->oeroded = min(obj->oeroded + 1, 3);
				fix_object(obj);
				break;	/* don't do other torch things */
			}
			else {
				obj_extract_and_unequip_self(obj);
				obfree(obj, (struct obj *)0);
				obj = (struct obj *) 0;
			}
		}
		
		if (obj && obj->age && obj->otyp == SHADOWLANDER_S_TORCH){
			if(obj->where == OBJ_MINVENT &&
				is_szcultist(obj->ocarry->data)
			){
				obj->age = (long) rn1(150,150);
			}
			end_burn(obj, FALSE);
		    begin_burn(obj);
		}
		break;

	    case DOUBLE_LIGHTSABER:
	    	if (obj->altmode && obj->cursed && !rn2(125)) {
		    obj->altmode = FALSE;
		    pline("%s %s reverts to single blade mode!",
			    whose, xname(obj));
	    	}
	    case POWER_ARMOR:
	    case LIGHTSABER: 
	    case BEAMSWORD:
	    case ROD_OF_FORCE:
	        /* Callback is checked every 1 turns - 
	        	lightsaber automatically deactivates if not wielded */
	        if ((obj->cursed && obj->otyp != ROD_OF_FORCE && !rn2(250)) ||
	            (obj->where == OBJ_FLOOR) || 
		    (obj->where == OBJ_MINVENT && 
				((!MON_WEP(obj->ocarry) || MON_WEP(obj->ocarry) != obj)
				&& (!MON_SWEP(obj->ocarry) || MON_SWEP(obj->ocarry) != obj))) ||
		    (obj->where == OBJ_INVENT &&
		    	((!uwep || uwep != obj) &&
			(obj->otyp != POWER_ARMOR) &&
		    	 (!u.twoweap || !uswapwep || obj != uswapwep))))
	            lightsaber_deactivate(obj, FALSE);
			if(obj->age <= 0){
				obj->age = 0;//From hitting targets
				lightsaber_deactivate(obj, FALSE);
			}
			if (obj && obj->age && obj->lamplit) /* might be deactivated */
				begin_burn(obj);
		break;
//#ifdef FIREARMS
	    case STICK_OF_DYNAMITE:
		end_burn(obj, FALSE);
		bomb_blow((genericptr_t) obj, timeout);
		return;
//#endif
	    default:
		impossible("burn_object: unexpeced obj %s", xname(obj));
		break;
	}
	if (need_newsym) newsym(x, y);
}

/* lightsabers deactivate when they hit the ground/not wielded */
/* assumes caller checks for correct conditions */
void
lightsaber_deactivate (obj, timer_attached)
	struct obj *obj;
	boolean timer_attached;
{
	xchar x,y;
	char whose[BUFSZ];

	(void) Shk_Your(whose, obj);
	obj->lamplit = 0; //temp turn it off so that it prints the material instead of the color
	if (get_obj_location(obj, &x, &y, 0)) {
	    if (cansee(x, y)) {
		switch (obj->where) {
			case OBJ_INVENT:
			case OBJ_MINVENT:
			    pline("%s %s deactivates.",whose, xname(obj));
			    break;
			case OBJ_FLOOR:
			    You("see %s deactivate.", an(xname(obj)));
			    break;
		}
	    } else {
			if(obj->oartifact != ART_HOLY_MOONLIGHT_SWORD)
				You_hear("a %s deactivate.",obj->otyp == POWER_ARMOR?"power suit":"lightsaber");
	    }
	}
	obj->lamplit = 1; //turn back on for proper deactivation
	// if (obj->otyp == DOUBLE_LIGHTSABER)
	obj->altmode = FALSE;
	if ((obj == uwep) || (u.twoweap && obj != uswapwep)) unweapon = TRUE;
	end_burn(obj, timer_attached);
}

/* returns the correct light/dark radius for obj, if it were lit */
int
lightsource_radius(obj)
struct obj * obj;
{
	int radius = 0;

	if (obj->oartifact == ART_HOLY_MOONLIGHT_SWORD){
		radius = 1;
	}
	else switch (obj->otyp) 
	{
	case MAGIC_LAMP:
	case DWARVISH_HELM:
	case LANTERN:
	case LANTERN_PLATE_MAIL:
	case OIL_LAMP:
	case CANDLE_OF_INVOCATION:
		radius = 3;
		break;
	case GNOMISH_POINTY_HAT:
	case POT_STARLIGHT:
	case SUNLIGHT_MAGGOT:
	case TONITRUS:
		radius = 2;
		break;
	case CHUNK_OF_FOSSIL_DARK:
		radius = 2;
		break;
	case POT_OIL:
	case STICK_OF_DYNAMITE:
		radius = 1;     /* very dim light */
		break;
	case CANDELABRUM_OF_INVOCATION:
	case TALLOW_CANDLE:
	case WAX_CANDLE:
		radius = candle_light_range(obj);
		break;
	case MAGIC_TORCH:
		radius = 4;
		break;
	case SUNROD:
	case TORCH:
	case SHADOWLANDER_S_TORCH:
		/* magic times are 150, 50, and 0 */
		/* sunrods are more extreme: 5/3/1 vs 4/3/2 */
		if (obj->age > 150L){
			radius = 4 + (obj->otyp == SUNROD);
		}
		else if (obj->age > 50L){
			radius = 3;
		}
		else {
			radius = 2 - (obj->otyp == SUNROD);
		}
		break;
	case DOUBLE_LIGHTSABER:
	case LIGHTSABER:
	case BEAMSWORD:
		if (obj->cobj && obj->cobj->oartifact == obj->oartifact && arti_light(obj->cobj))
			radius = (obj->cobj->blessed ? 3 : (obj->cobj->cursed ? 1 : 2));
		else if (obj->cobj && (obj->cobj->otyp == JET || obj->cobj->otyp == BLACK_OPAL || obj->cobj->otyp == CATAPSI_VORTEX))
			radius = 0; // jet is "mirrored", black opal is "smoke", catapsi is "cloud"
		else radius = 1;
		break;
	case ROD_OF_FORCE:
	case POWER_ARMOR:
			radius = 0;
		break;
	default:
		/* [ALI] Support artifact light sources */
		if (artifact_light(obj) || arti_light(obj)) {
			radius = (obj->blessed ? 3 : (obj->cursed ? 1 : 2));
		}
		break;
	}
	return radius;
}

long
lightsource_turns(obj)
struct obj * obj;
{
	long turns = 0;

	if (obj->oartifact == ART_HOLY_MOONLIGHT_SWORD){
		turns = 1;
	}
	else switch (obj->otyp) {
	case DOUBLE_LIGHTSABER:
	case LIGHTSABER:
	case BEAMSWORD:
	case ROD_OF_FORCE:
	case POWER_ARMOR:
		turns = 1;
		break;

	case POT_OIL:
	case STICK_OF_DYNAMITE:
		turns = obj->age;
		break;

	case TONITRUS:
		turns = 7;
		break;

	case GNOMISH_POINTY_HAT:
		turns = obj->age;
		if (obj->age > 75L)
			turns = obj->age - 75L;
		else if (obj->age > 15L)
			turns = obj->age - 15L;
		break;

	case DWARVISH_HELM:
	case LANTERN:
	case LANTERN_PLATE_MAIL:
	case OIL_LAMP:
		/* magic times are 150, 100, 50, 25, and 0 */
		if (obj->age > 150L)
			turns = obj->age - 150L;
		else if (obj->age > 100L)
			turns = obj->age - 100L;
		else if (obj->age > 50L)
			turns = obj->age - 50L;
		else if (obj->age > 25L)
			turns = obj->age - 25L;
		else
			turns = obj->age;
		break;

	case CANDELABRUM_OF_INVOCATION:
	case TALLOW_CANDLE:
	case WAX_CANDLE:
		/* magic times are 75, 15, and 0 */
		if (obj->age > 75L)
			turns = obj->age - 75L;
		else if (obj->age > 15L)
			turns = obj->age - 15L;
		else
			turns = obj->age;
		break;

	case SUNROD:
	case TORCH:
	case SHADOWLANDER_S_TORCH:
		/* magic times are 150, 50, and 0 */
		if (obj->age > 150L){
			turns = obj->age - 150L;
		}
		else if (obj->age > 50L){
			turns = obj->age - 50L;
		}
		else {
			turns = obj->age;
		}
		break;

	default:
		turns = 0;
		break;
	}
	return turns;
}

boolean
lightsource_timed(obj)
struct obj * obj;
{
	return (obj && (
		(obj->oartifact == ART_HOLY_MOONLIGHT_SWORD) ||	/* ??? Chris: The timer's used to extinquish it when it's dropped. */
		(obj->otyp == TONITRUS) ||
		(obj->otyp == DOUBLE_LIGHTSABER) ||
		(obj->otyp == LIGHTSABER) ||
		(obj->otyp == BEAMSWORD) ||
		(obj->otyp == POWER_ARMOR) ||
		(obj->otyp == ROD_OF_FORCE) ||
		(obj->otyp == POT_OIL) ||
		(obj->otyp == STICK_OF_DYNAMITE) ||
		(obj->otyp == GNOMISH_POINTY_HAT) ||
		(obj->otyp == DWARVISH_HELM) ||
		(obj->otyp == LANTERN) ||
		(obj->otyp == LANTERN_PLATE_MAIL) ||
		(obj->otyp == OIL_LAMP) ||
		(obj->otyp == CANDELABRUM_OF_INVOCATION) ||
		(obj->otyp == TALLOW_CANDLE) ||
		(obj->otyp == WAX_CANDLE) ||
		(obj->otyp == SUNROD) ||
		(obj->otyp == TORCH) ||
		(obj->otyp == SHADOWLANDER_S_TORCH)));
}

/*
 * Start a burn timeout on the given object. If not "already lit" then
 * create a light source for the vision system.  There had better not
 * be a burn already running on the object.
 *
 * Magic lamps stay lit as long as there's a genie inside, so don't start
 * a timer.
 *
 * Burn rules:
 *	potions of oil, lamps & candles:
 *		age = # of turns of fuel left
 *		spe = <unused>
 *
 *	magic lamps:
 *		age = <unused>
 *		spe = 0 not lightable, 1 lightable forever
 *
 *	candelabrum:
 *		age = # of turns of fuel left
 *		spe = # of candles
 *
 * Once the burn begins, the age will be set to the amount of fuel
 * remaining _once_the_burn_finishes_.  If the burn is terminated
 * early then fuel is added back.
 *
 * This use of age differs from the use of age for corpses and eggs.
 * For the latter items, age is when the object was created, so we
 * know when it becomes "bad".
 *
 * This is a "silent" routine - it should not print anything out.
 */
void
begin_burn(obj)
	struct obj *obj;
{
	int radius;
	long turns = 0;
	boolean do_timer = TRUE;
	boolean already_lit = obj->lamplit;
	radius = 3;

	/* othere than these, lightsources don't work when age==0 */
	if (obj->age == 0 && 
		obj->otyp != MAGIC_LAMP && 
		obj->otyp != CANDLE_OF_INVOCATION &&
		obj->otyp != MAGIC_TORCH &&
		obj->otyp != POT_STARLIGHT && 
		obj->otyp != SUNLIGHT_MAGGOT && 
		obj->otyp != CHUNK_OF_FOSSIL_DARK && 
		obj->otyp != TONITRUS && 
		!artifact_light(obj) && 
		!arti_light(obj) && 
		obj->oartifact != ART_HOLY_MOONLIGHT_SWORD &&
		obj->oartifact != ART_ATMA_WEAPON
	) return;
	

	radius = lightsource_radius(obj);
	turns = lightsource_turns(obj);
	do_timer = lightsource_timed(obj);

	/* some things need to set lamplit on their own here */
	if (obj->otyp == MAGIC_LAMP ||
		obj->otyp == CANDLE_OF_INVOCATION ||
		obj->otyp == MAGIC_TORCH ||
		artifact_light(obj) ||
		obj_eternal_light(obj))
		obj->lamplit = TRUE;
	/* lightsaber charge and Atma Weapon special */
	if (obj->otyp == DOUBLE_LIGHTSABER ||
		obj->otyp == ROD_OF_FORCE ||
		obj->otyp == LIGHTSABER ||
		obj->otyp == BEAMSWORD
		) {
		if (obj->altmode && obj->age > 1)
			obj->age--; /* Double power usage */
		if (obj->oartifact == ART_ATMA_WEAPON){
			if (obj->age <= 0){
				if (Drain_resistance) return;
				losexp("life force drain", TRUE, TRUE, TRUE);
				obj->cursed = FALSE;
				obj->age = 150000;
			}
			else if (!Drain_resistance) obj->age++;
		}
	}
	
	if (do_timer) {
	    if (start_timer(turns, TIMER_OBJECT,
					BURN_OBJECT, (genericptr_t)obj)) {
		obj->lamplit = 1;
		obj->age -= turns;
		if (carried(obj) && !already_lit)
		    update_inventory();
	    }
	} else {
	    if (carried(obj) && !already_lit)
		update_inventory();
	}

	if (obj->lamplit) {
	    xchar x, y;
		if (already_lit)	/* to give an error if already_lit != actually had an ls */
			del_light_source(obj->light);
		if(radius) new_light_source(LS_OBJECT, (genericptr_t)obj, radius);
	}
}

/*
 * Stop a burn timeout on the given object if timer attached.  Darken
 * light source.
 */
void
end_burn(obj, timer_attached)
	struct obj *obj;
	boolean timer_attached;
{
	if (!obj->lamplit) {
	    impossible("end_burn: obj %s not lit", xname(obj));
	    return;
	}

	if (obj->otyp == MAGIC_LAMP 
		|| obj->otyp == CANDLE_OF_INVOCATION
		|| obj->otyp == MAGIC_TORCH
		|| obj->otyp == POT_STARLIGHT
		|| obj->otyp == SUNLIGHT_MAGGOT
		|| obj->otyp == CHUNK_OF_FOSSIL_DARK
		|| artifact_light(obj)
		|| arti_light(obj)
	) timer_attached = FALSE;

	if (!timer_attached) {
	    /* [DS] Cleanup explicitly, since timer cleanup won't happen */
	    if(obj->otyp != POWER_ARMOR) del_light_source(obj->light);
	    obj->lamplit = 0;
	    if (obj->where == OBJ_INVENT)
		update_inventory();
	} else if (!stop_timer(BURN_OBJECT, obj->timed)){
		obj->lamplit = 0;
	    impossible("end_burn: obj %s not timed!", xname(obj));
	}
	// if(darksight(youracedata) || catsight(youracedata)) doredraw();
	// So many different vision systems, just do a redraw
	//doredraw();
	vision_full_recalc = 1;
}

#endif /* OVL1 */
#ifdef OVL0

extern boolean saving_game;

/*
 * Cleanup a burning object if timer stopped.
 */
static void
cleanup_burn(arg, expire_time)
    genericptr_t arg;
    long expire_time;
{
    struct obj *obj = (struct obj *)arg;
    if (!obj->lamplit) {
	impossible("cleanup_burn: obj %s not lit", xname(obj));
	return;
    }
	if(obj->otyp != POWER_ARMOR) del_light_source(((struct obj *)arg)->light);

    /* restore unused time */
    obj->age += expire_time - monstermoves;

    obj->lamplit = 0;

    if (obj->where == OBJ_INVENT && !saving_game)
		update_inventory();
}

#endif /* OVL0 */
#ifdef OVL1

void
do_storms()
{
    int nstrike;
    register int x, y;
    int dirx, diry;
    int count;

    /* no lightning if not the air level or too often, even then */
    if(!Is_airlevel(&u.uz) || rn2(8))
	return;

    /* the number of strikes is 8-log2(nstrike) */
    for(nstrike = rnd(64); nstrike <= 64; nstrike *= 2) {
	count = 0;
	do {
	    x = rnd(COLNO-1);
	    y = rn2(ROWNO);
	} while (++count < 100 && levl[x][y].typ != CLOUD);

	if(count < 100) {
	    dirx = rn2(3) - 1;
	    diry = rn2(3) - 1;
	    if(dirx != 0 || diry != 0)
			zap((struct monst *)0, x, y, dirx, diry, rn1(7, 7), basiczap(0, AD_ELEC, ZAP_WAND, 8));
	}
    }

    if(levl[u.ux][u.uy].typ == CLOUD) {
	/* inside a cloud during a thunder storm is deafening */
	pline("Kaboom!!!  Boom!!  Boom!!");
	if(!Invulnerable) {
	    stop_occupation();
	    nomul(-3, "hiding from thunderstorm");
	}
    } else
	You_hear("a rumbling noise.");
}
#endif /* OVL1 */

struct obj *
update_skull_mon(mon, obj)
struct monst *mon;
struct obj *obj;
{
	if(!get_mx(mon, MX_ESUM) || !mon->mextra_p->esum_p->sm_o_id){
		impossible("Monster %s to update skull stats, but doesn't have an item id set!", noit_mon_nam(mon));
		return obj;
	}
	
	if(!obj){
		obj = find_oid(mon->mextra_p->esum_p->sm_o_id);
	}

	//Not an error, the skull may have been destroyed or left on another level or be otherwise unfindable.
	if(!obj){
		return obj;
	}

	if(mon->mextra_p->esum_p->sm_o_id != obj->o_id){
		impossible("Monster %s to update skull stats, but given wrong item %s!", noit_mon_nam(mon), xname(obj));
		return obj;
	}

	if(!get_ox(obj, OX_EMON)){
		impossible("Monster %s to update skull stats, but item %s has no attached monster!", noit_mon_nam(mon), xname(obj));
		return obj;
	}

	if(big_to_little(mon->mtyp) == big_to_little(EMON(obj)->mtyp)){
		EMON(obj)->data = mon->data;
		EMON(obj)->mhpmax = mon->mhpmax;
		EMON(obj)->mhp = mon->mhpmax;
		EMON(obj)->m_lev = mon->m_lev;
		for(int i = 0; i < MPROP_SIZE; i++){
			EMON(obj)->mintrinsics[i] = mon->mintrinsics[i];
		}
		EMON(obj)->acurr = mon->acurr;
		EMON(obj)->aexe = mon->aexe;
		EMON(obj)->abon = mon->abon;
		EMON(obj)->amax = mon->amax;
		EMON(obj)->atemp = mon->atemp;
		EMON(obj)->atime = mon->atime;
		EMON(obj)->female = mon->female;
	}
	EMON(obj)->mcrazed = mon->mcrazed;
	EMON(obj)->mnotlaugh = mon->mnotlaugh;
	EMON(obj)->mlaughing = mon->mlaughing;
	EMON(obj)->mdoubt = mon->mdoubt;
	EMON(obj)->mwounded_legs = mon->mwounded_legs;
	
	EMON(obj)->menvy = mon->menvy;
	EMON(obj)->msanctity = mon->msanctity;
	EMON(obj)->mgluttony = mon->mgluttony;
	EMON(obj)->mfrigophobia = mon->mfrigophobia;
	EMON(obj)->mrage = mon->mrage;
	EMON(obj)->margent = mon->margent;
	EMON(obj)->msuicide = mon->msuicide;
	EMON(obj)->mophidio = mon->mophidio;
	EMON(obj)->marachno = mon->marachno;
	EMON(obj)->mentomo = mon->mentomo;
	EMON(obj)->mthalasso = mon->mthalasso;
	EMON(obj)->mhelmintho = mon->mhelmintho;
	EMON(obj)->mparanoid = mon->mparanoid;
	EMON(obj)->mtalons = mon->mtalons;
	EMON(obj)->mdreams = mon->mdreams;
	EMON(obj)->msciaphilia = mon->msciaphilia;
	EMON(obj)->mapostasy = mon->mapostasy;
	EMON(obj)->mtoobig = mon->mtoobig;
	EMON(obj)->mrotting = mon->mrotting;
	EMON(obj)->mspores = mon->mspores;
	EMON(obj)->mformication = mon->mformication;
	EMON(obj)->mscorpions = mon->mscorpions;
	EMON(obj)->mcaterpillars = mon->mcaterpillars;
	
	
	EMON(obj)->encouraged = mon->encouraged;
	EMON(obj)->mtrapseen = mon->mtrapseen;
	EMON(obj)->mfaction = mon->mfaction;
	return obj;
}

/* callback procs to desummon monsters/objects */
void
desummon_mon(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct monst * mon = (struct monst *)arg;
	if (DEADMONSTER(mon)) {
		/* already dead, necessary cleanup will be done by cleanup_msummon() */
		return;
	}
	if(get_mx(mon, MX_ESUM) && mon->mextra_p->esum_p->permanent) {
		start_timer(9999, TIMER_MONSTER, DESUMMON_MON, arg);
		return;
	}
	if (get_mx(mon, MX_ESUM) && mon->mextra_p->esum_p->summoner) {
		mon->mextra_p->esum_p->summoner->summonpwr -= mon->mextra_p->esum_p->summonstr;
		mon->mextra_p->esum_p->summoner = (struct monst *)0;
		mon->mextra_p->esum_p->sm_id = 0;
		mon->mextra_p->esum_p->sm_o_id = 0;
	}

	/* Update crystal skull if it has one. Find the skull in the update function. */
	if(get_mx(mon, MX_ESUM) && mon->mextra_p->esum_p->sm_o_id){
		update_skull_mon(mon, (struct obj *) 0);
	}

	/* special case for vexing orbs -- awful */
	if (mon->mtyp == PM_VEXING_ORB) {
		boolean monmoving = flags.mon_moving;
		flags.mon_moving = TRUE;
		mondied(mon);
		flags.mon_moving = monmoving;
	}
	else
	{
		if (timeout == monstermoves && canseemon(mon)) {
			pline("%s vanishes.", Monnam(mon));
		}
		monvanished(mon);
	}
}

void
cleanup_msummon(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct monst * mon = (struct monst *)arg;
	/* if we are stopping the timer because mon died or vanished, reduce tax on summoner */
	if (get_mx(mon, MX_ESUM) && DEADMONSTER(mon) && mon->mextra_p->esum_p->summoner) {
		mon->mextra_p->esum_p->summoner->summonpwr -= mon->mextra_p->esum_p->summonstr;
	}
	/* Update crystal skull if it has one. Find the skull in the update function. */
	if(get_mx(mon, MX_ESUM) && mon->mextra_p->esum_p->sm_o_id){
		update_skull_mon(mon, (struct obj *) 0);
	}
}

void
desummon_obj(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj * otmp = (struct obj *)arg;
	if(get_ox(otmp, OX_ESUM) && otmp->oextra_p->esum_p->permanent) {
		start_timer(9999, TIMER_OBJECT, DESUMMON_OBJ, arg);
		return;
	}
	/* clean up some pointers that obj_extract_self and obfree don't catch. UGH. */
	if (otmp->where == OBJ_MINVENT) {
		if (otmp == MON_WEP(otmp->ocarry)) MON_NOWEP(otmp->ocarry);
		if (otmp == MON_SWEP(otmp->ocarry)) MON_NOSWEP(otmp->ocarry);
	}
	/* if in_use is set, then we know it'll be used up by the thing currently using it, and we'd be double-deleting it */
	if (!otmp->in_use) {
		obj_extract_and_unequip_self(otmp);
		newsym(otmp->ox, otmp->oy);
		obfree(otmp, (struct obj *)0);
	}
}

void
larvae_die(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj * otmp = (struct obj *)arg;
	if(otmp->olarva > 0){
		otmp->olarva--;
		otmp->odead_larva = min(3, otmp->odead_larva + 1);
	}
	if(otmp->obyak > 0){
		xchar x = 0, y = 0;
		otmp->obyak--;
		get_obj_location(otmp, &x, &y, INTRAP_TOO);
		if(x || y){
			struct monst *mtmp = makemon(&mons[PM_STRANGE_LARVA], x, y, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mvar_tanninType = PM_BYAKHEE;
			}
		}
		else {
			otmp->odead_larva = min(3, otmp->odead_larva + 1);
		}
	}
	if(otmp->olarva > 0 || otmp->obyak > 0) {
		start_timer(4+d(2,4), TIMER_OBJECT, LARVAE_DIE, arg);
	}
}

void
revert_object(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *obj = (struct obj *) arg;
	if(obj->owornmask&(W_WEP)){
		start_timer(1, TIMER_OBJECT,
					REVERT_OBJECT, (genericptr_t)obj);
	}
	else if((obj->owornmask&(W_SWAPWEP)) && (obj->where == OBJ_MINVENT || u.twoweap)){
		start_timer(1, TIMER_OBJECT,
					REVERT_OBJECT, (genericptr_t)obj);
	}
	else {
		if(obj->obj_material == HEMARGYOS){
			set_material_gm(obj, obj->ovar1_alt_mat);
			obj->oeroded = access_oeroded(obj->ovar2_alt_erosion);
			obj->oeroded2 = access_oeroded2(obj->ovar2_alt_erosion);
			obj->oeroded3 = access_oeroded3(obj->ovar2_alt_erosion);
			fix_object(obj);
			update_inventory();
		}
	}
}


void
revert_mercurial(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *obj = (struct obj *) arg;
	if(obj->obj_material != MERCURIAL){
		if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
			switch(obj->obj_material){
				case IRON:
					artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_IRON;
				break;
				case GREEN_STEEL:
					artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_GREEN;
				break;
				case SILVER:
					artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_SILVER;
				break;
				case GOLD:
					artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_GOLD;
				break;
				case PLATINUM:
					artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_PLATINUM;
				break;
				case MITHRIL:
					artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_MITHRIL;
				break;
			}
		}
		if(obj->where == OBJ_INVENT){
			Your("%s is tired of its rigid composition and melts back to silvery chaos.", xname(obj));
		}
		else if(obj->where == OBJ_FLOOR && cansee(obj->ox, obj->oy)){
			pline("%s is tired of its rigid composition and melts back to silvery chaos.", The(xname(obj)));
		}
		set_material_gm(obj, MERCURIAL);
		fix_object(obj);
		update_inventory();
	}
}


#ifdef OVL0
/* ------------------------------------------------------------------------- */
/*
 * Generic Timeout Functions.
 *
 * Interface:
 *
 * General:
 *	boolean start_timer(long timeout,short kind,short func_index,
 *							genericptr_t arg)
 *		Start a timer of kind 'kind' that will expire at time
 *		monstermoves+'timeout'.  Call the function at 'func_index'
 *		in the timeout table using argument 'arg'.  Return TRUE if
 *		a timer was started.  This places the timer on a list ordered
 *		"sooner" to "later".  If an object, increment the object's
 *		timer count.
 *
 *	long stop_timer(short func_index, genericptr_t arg)
 *		Stop a timer specified by the (func_index, arg) pair.  This
 *		assumes that such a pair is unique.  Return the time the
 *		timer would have gone off.  If no timer is found, return 0.
 *		If an object, decrement the object's timer count.
 *
 *	void copy_timers(struct timer *src_timer, int tmtype, genericptr_t dest)
 *		Duplicate all timers on src and attach them to dest.
 *
 *	void stop_all_timers(timer_element * tm)
 *		Stop all timers on the chain.
 *	void run_timers(void)
 *		Call timers that have timed out.
 *
 *
 * Save/Restore:
 *	void save_timers(timer_element * tm, int fd, int mode)
 *		Save chain of timers.
 * 
 *	void rest_timers(int tmtype, generic_ptr owner, timer_element * tm, int fd, boolean ghostly, long adjust)
 *		Restore chain of timers onto owner. If ghostly, adjust timers. 
 *
 */

#ifdef WIZARD
STATIC_DCL const char *FDECL(kind_name, (SHORT_P));
STATIC_DCL void FDECL(print_queue, (winid, timer_element *));
#endif
STATIC_DCL void FDECL(add_procchain_tm, (timer_element *));
STATIC_DCL void FDECL(rem_procchain_tm, (timer_element *));
STATIC_DCL void FDECL(rem_locchain_tm, (timer_element *, timer_element **));

/* ordered timer list */
static timer_element *timer_base;		/* head of timer procchain */
static timer_element *timer_paused;		/* helper pointer to first paused/migrating timer in procchain */
static timer_element *timer_last;		/* last timer in procchain */
static unsigned long timer_id = 1;


#define owner_tm(tmtype, owner) (\
	(tmtype) == TIMER_OBJECT  ? &(((struct obj *)(owner))->timed) : \
	(tmtype) == TIMER_MONSTER ? &(((struct monst *)(owner))->timed) : \
	(struct timer **)0)

/* If defined, then include names when printing out the timer queue */
#define VERBOSE_TIMER

typedef struct {
    timeout_proc f, cleanup;
#ifdef VERBOSE_TIMER
    const char *name;
# define TTAB(a, b, c) {a,b,c}
#else
# define TTAB(a, b, c) {a,b}
#endif
} ttable;

/* table of timeout functions */
static const ttable timeout_funcs[NUM_TIME_FUNCS] = {
	TTAB(rot_organic,		(timeout_proc)0,	"rot_organic"),
	TTAB(rot_corpse,		(timeout_proc)0,	"rot_corpse"),
	TTAB(moldy_corpse,		(timeout_proc)0,	"moldy_corpse"),
	TTAB(revive_mon,		(timeout_proc)0,	"revive_mon"),
	TTAB(burn_object,		cleanup_burn,		"burn_object"),
	TTAB(hatch_egg,			(timeout_proc)0,	"hatch_egg"),
	TTAB(fig_transform,		(timeout_proc)0,	"fig_transform"),
	TTAB(light_damage,		(timeout_proc)0,	"light_damage"),
	TTAB(slimy_corpse,		(timeout_proc)0,	"slimy_corpse"),
	TTAB(zombie_corpse,		(timeout_proc)0,	"zombie_corpse"),
	TTAB(shady_corpse,		(timeout_proc)0,	"shady_corpse"),
	TTAB(yellow_corpse,		(timeout_proc)0,	"yellow_corpse"),
	TTAB(bomb_blow,			(timeout_proc)0,	"bomb_blow"),
	TTAB(return_ammo,		(timeout_proc)0,	"return_ammo"),
	TTAB(desummon_mon,		cleanup_msummon,	"desummon_mon"),
	TTAB(desummon_obj,		(timeout_proc)0,	"desummon_obj"),
	TTAB(larvae_die,		(timeout_proc)0,	"larvae_die"),
	TTAB(revive_mon_pickup,	(timeout_proc)0,	"revive_mon_pickup"),
	TTAB(revert_object,		(timeout_proc)0,	"revert_object"),
	TTAB(revert_mercurial,	(timeout_proc)0,	"revert_mercurial"),
};
#undef TTAB


#if defined(WIZARD)

STATIC_OVL const char *
kind_name(kind)
short kind;
{
    switch (kind) {
	//case TIMER_LEVEL: return "level";
	//case TIMER_GLOBAL: return "global";
	case TIMER_OBJECT: return "object";
	case TIMER_MONSTER: return "monster";
    }
    return "unknown";
}

STATIC_OVL void
print_queue(win, base)
winid win;
timer_element *base;
{
    timer_element *curr;
    char buf[BUFSZ], arg_address[20];

    if (!base) {
	putstr(win, 0, "<empty>");
    } else {
	putstr(win, 0, "timeout  id   kind   call");
	for (curr = base; curr; curr = curr->next) {
#ifdef VERBOSE_TIMER
	    Sprintf(buf, " %4ld   %4ld  %-6s %s(%s) %d",
		curr->timeout, curr->tid, kind_name(curr->kind),
		timeout_funcs[curr->func_index].name,
		fmt_ptr((genericptr_t)curr->arg, arg_address),
		curr->timerflags);
#else
	    Sprintf(buf, " %4ld   %4ld  %-6s #%d(%s)",
		curr->timeout, curr->tid, kind_name(curr->kind),
		curr->func_index,
		fmt_ptr((genericptr_t)curr->arg, arg_address));
#endif
	    putstr(win, 0, buf);
	}
    }
}

int
wiz_timeout_queue()
{
    winid win;
    char buf[BUFSZ];
    const char *propname;
    long intrinsic;
	int i, p, count, longestlen, ln;

    win = create_nhwindow(NHW_MENU); /* corner text window */
    if (win == WIN_ERR)
        return 0;

    Sprintf(buf, "Current time = %ld.", monstermoves);
    putstr(win, 0, buf);
    putstr(win, 0, "");
    putstr(win, 0, "Active timeout queue:");
    putstr(win, 0, "");
    print_queue(win, timer_base);

    /* Timed properies:
     * check every one; the majority can't obtain temporary timeouts in
     * normal play but those can be forced via the #wizintrinsic command.
     */
    count = longestlen = 0;
    for (i = 0; (propname = propertynames[i].prop_name) != 0; ++i) {
        p = propertynames[i].prop_num;
        intrinsic = u.uprops[p].intrinsic;
        if (intrinsic & TIMEOUT) {
            ++count;
            if ((ln = (int) strlen(propname)) > longestlen)
                longestlen = ln;
        }
    }
    putstr(win, 0, "");
    if (!count) {
        putstr(win, 0, "No timed properties.");
    } else {
        putstr(win, 0, "Timed properties:");
        putstr(win, 0, "");
        for (i = 0; (propname = propertynames[i].prop_name) != 0; ++i) {
            p = propertynames[i].prop_num;
            intrinsic = u.uprops[p].intrinsic;
			if (intrinsic & TIMEOUT_INF) {
				Sprintf(buf, " %*s    inf", -longestlen, propname);
				putstr(win, 0, buf);
			}
            else if (intrinsic & TIMEOUT) {
                /* timeout value can be up to 16777215 (0x00ffffff) but
                   width of 6 digits should result in values lining up
                   almost all the time (if/when they don't, it won't
                   look nice but the information will still be accurate) */
                Sprintf(buf, " %*s %6ld", -longestlen, propname,
                        (intrinsic & TIMEOUT));
                putstr(win, 0, buf);
            }
        }
    }
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);

    return 0;
}

void
timer_sanity_check()
{
    timer_element *curr;
    char obj_address[20];

    /* this should be much more complete */
    for (curr = timer_base; curr; curr = curr->next)
	if (curr->kind == TIMER_OBJECT) {
	    struct obj *obj = (struct obj *) curr->arg;
	    if (obj->timed == 0) {
		pline("timer sanity: untimed obj %s, timer %ld",
		      fmt_ptr((genericptr_t)obj, obj_address), curr->tid);
	    }
	}
}

#endif /* WIZARD */

/* adds an existing timer to the processing chain */
/* does not affect local chain it should go on */
void
add_procchain_tm(tm)
timer_element * tm;
{
	timer_element *curr, *prev;

	/* check for duplicates */
	for (curr = timer_base; curr; curr = curr->next) {
		if (curr == tm) {
			impossible("tm already in processing chain");
			return;
		}
	}

	/* insert into procchain */
	if (tm->timerflags & (TIMERFLAG_PAUSED | TIMERFLAG_MIGRATING)) {
		/* timer is not executable, goes on very end, order independent */
		prev = timer_last;
		curr = (timer_element *)0;
		if (!timer_paused)
			timer_paused = tm;
	}
	else
	{
		/* insert in ordered place in processing loop */
		for (prev = 0, curr = timer_base; curr && curr != timer_paused; prev = curr, curr = curr->next)
		{
			if (curr->timeout >= tm->timeout) break;
		}
	}
    tm->next = curr;

    if (prev)
		prev->next = tm;
    else
		timer_base = tm;

	if (!curr)
		timer_last = tm;
	return;
}

/* removes a timer from the processing chain */
/* does not affect local chain it is on */
void
rem_procchain_tm(tm)
timer_element * tm;
{
	struct timer * tmtmp;

	if (timer_base == tm) {
		timer_base = tm->next;
		if (timer_paused == tm)
			timer_paused = tm->next;
		if (!tm->next)
			timer_last = timer_base;
		return;
	}
	else for (tmtmp = timer_base; tmtmp; tmtmp = tmtmp->next) {
		if (tmtmp->next == tm) {
			tmtmp->next = tm->next;
			if (timer_paused == tm)
				timer_paused = tm->next;
			if (!tm->next)
				timer_last = tmtmp;
			return;
		}
	}
	return;
}
/* removes and readds a timer from the procchain, so that it is in the correct place */
void
adj_procchain_tm(tm)
timer_element * tm;
{
	rem_procchain_tm(tm);
	add_procchain_tm(tm);
}

/* removes a timer from a local chain */
void
rem_locchain_tm(tm, chain_p)
timer_element * tm;
timer_element ** chain_p;
{
	struct timer * tmtmp;
	struct timer * chain_base = *chain_p;

	if (chain_base == tm) {
		*chain_p = tm->tnxt;
		return;
	}
	else for (tmtmp = chain_base; tmtmp; tmtmp = tmtmp->tnxt) {
		if (tmtmp->tnxt == tm) {
			tmtmp->tnxt = tm->tnxt;
			return;
		}
	}
	impossible("couldn't find tm in given chain");
	return;
}

/*
 * Pick off timeout elements from the global queue and call their functions.
 * Do this until their time is less than or equal to the move count.
 */
void
run_timers()
{
    timer_element *curr;
	flags.run_timers = FALSE;
    /*
     * Always use the first element.  Elements may be added or deleted at
     * any time.  The list is ordered, we are done when the first element
     * is in the future.
     */
    while (timer_base && timer_base->timeout <= monstermoves && timer_base != timer_paused) {
		curr = timer_base;
		timer_base = curr->next;
		rem_locchain_tm(curr, owner_tm(curr->kind, curr->arg));
		(*timeout_funcs[curr->func_index].f)(curr->arg, curr->timeout);
		free((genericptr_t) curr);
    }
}

/*
 * Start a timer.  Return TRUE if successful.
 */
timer_element *
start_timer(when, tmtype, func_index, owner)
long when;
short tmtype;
short func_index;
genericptr_t owner;
{
    timer_element *gnu;
	timer_element *curr;

	/* check that <owner> does not already have a <func_index> timer running via the processing loop; fail if so */
	for (curr = timer_base; curr; curr = curr->next)
	if (curr->arg == owner && curr->func_index == func_index) {
		impossible("Attempted to start 2nd %s timer, aborted.",
			timeout_funcs[func_index].name);
		return (timer_element *)0;
	}
    if (func_index < 0 || func_index >= NUM_TIME_FUNCS)
		panic("start_timer bad func_index");

    gnu = (timer_element *) alloc(sizeof(timer_element));
    gnu->tnxt = *owner_tm(tmtype, owner);	/* local chain */
    gnu->tid = timer_id++;
    gnu->timeout = monstermoves + when;
    gnu->kind = tmtype;
    gnu->func_index = func_index;
    gnu->arg = owner;
	gnu->timerflags = 0;

	/* add to owner */
	*owner_tm(tmtype, owner) = gnu;
	/* add to processing chain */
	add_procchain_tm(gnu);

    return gnu;
}

/*
 * Remove the timer from the current list and free it up.  Return the time
 * it would have gone off, 0 if not found.
 */
long
stop_timer(func_index, chain)
short func_index;
timer_element * chain;
{
    timer_element *doomed;
    long timeout;

	for (doomed = chain; doomed && doomed->func_index != func_index; doomed = doomed->tnxt);
	if (!doomed) {
		// couldn't find it
		return 0;
	}
	timeout = doomed->timeout;
	/* remove from processing loop */
	rem_procchain_tm(doomed);
	/* call cleanup function */
	if (timeout_funcs[doomed->func_index].cleanup)
		(*timeout_funcs[doomed->func_index].cleanup)(doomed->arg, timeout);
	/* remove from owner */
	rem_locchain_tm(doomed, owner_tm(doomed->kind, doomed->arg));
	free((genericptr_t) doomed);
	return timeout;
}
void
stop_all_timers(tm)
timer_element * tm;
{
	timer_element *curr, *next_timer = 0;
	for (curr = tm; curr; curr = next_timer) {
		next_timer = curr->tnxt;
		(void) stop_timer(curr->func_index, curr);
	}
}

/* temporarily pause processing of a single timer */
void
pause_timer(tm)
timer_element * tm;
{
	tm->timeout = tm->timeout - monstermoves;
	tm->timerflags |= TIMERFLAG_PAUSED;
	adj_procchain_tm(tm);
}
/* resume processing of a single timer */
void
resume_timer(tm)
timer_element * tm;
{
	tm->timeout = tm->timeout + monstermoves;
	tm->timerflags &= ~TIMERFLAG_PAUSED;
	adj_procchain_tm(tm);
	flags.run_timers = TRUE;
}

/* temporarily pause processing of all timers on chain until it they are resumed */
void
pause_timers(tm)
timer_element * tm;
{
	timer_element *curr;
	for (curr = tm; curr; curr = tm->tnxt)
		pause_timer(curr);
}

/* resume processing of all timers on chain */
void
resume_timers(tm)
timer_element * tm;
{
	timer_element *curr;
	for (curr = tm; curr; curr = tm->tnxt)
		resume_timer(curr);
}

/* set all timers on chain as migrating (do not execute) */
void
migrate_timers(tm)
timer_element * tm;
{
	timer_element *curr;
	for (curr = tm; curr; curr = tm->tnxt)
	{
		curr->timerflags |= TIMERFLAG_MIGRATING;
		adj_procchain_tm(curr);
	}
}
/* resume handling of migrating timers on chain */
void
receive_timers(tm)
timer_element * tm;
{
	timer_element *curr;
	for (curr = tm; curr; curr = tm->tnxt)
	{
		curr->timerflags &= ~TIMERFLAG_MIGRATING;
		adj_procchain_tm(curr);
	}
	flags.run_timers = TRUE;
}

void
save_timers(tm, fd, mode)
struct timer * tm;
int fd;
int mode;
{
	struct timer * curr;

	int count = 0;
	for (curr = tm; curr; curr = curr->tnxt) {
		if (perform_bwrite(mode))
			bwrite(fd, (genericptr_t)curr, sizeof(struct timer));
	}
	if (release_data(mode))
		stop_all_timers(tm);
	return;
}

void
rest_timers(tmtype, owner, tm, fd, ghostly, adjust)
int tmtype;
genericptr_t owner;
struct timer * tm;
int fd;
boolean ghostly;
long adjust;
{
	boolean hastnxt;

	*owner_tm(tmtype, owner) = (struct timer *)0;
	do {
		tm = (struct timer *)alloc(sizeof(struct timer));
		mread(fd, (genericptr_t) tm, sizeof(struct timer));
		add_procchain_tm(tm);
		hastnxt = tm->tnxt != (struct timer *)0;
		/* possibly adjust timer */
		if (ghostly)
	    	tm->timeout += adjust;
		/* relink owner */
		tm->arg = owner;
		tm->tnxt = *owner_tm(tmtype, owner);
		*owner_tm(tmtype, owner) = tm;
	} while(hastnxt);
	return;
}

long
timer_duration_remaining(tm)
timer_element * tm;
{
	if (!tm)
		return 0L;
	return tm->timeout - monstermoves;
}

void
adjust_timer_duration(tm, amt)
timer_element * tm;
long amt;
{
	if (!tm) {
		impossible("tm doesn't exist to adjust timer");
		return;
	}
	/* have to remove it and re-add it so the list remains ordered */
	rem_procchain_tm(tm);
	tm->timeout += amt;
	add_procchain_tm(tm);
}

timer_element *
get_timer(chain, func)
timer_element * chain;
short func;
{
	timer_element * tm;
	for (tm = chain; tm && tm->func_index != func; tm = tm->tnxt);
	return tm;
}

/* Duplicates a specific timer onto dest.
 */
void
copy_timer(src_timer, tmtype, dest)
timer_element *src_timer;
int tmtype;
genericptr_t dest;
{
	timer_element * tmp;
	if (src_timer) {
		tmp = start_timer(src_timer->timeout-monstermoves, tmtype, src_timer->func_index, dest);
		tmp->timerflags = src_timer->timerflags;
		adj_procchain_tm(tmp);
	}
}

/*
 * Duplicate all timers on the given chain onto dest.
 */
void
copy_timers(src_timer, tmtype, dest)
timer_element *src_timer;
int tmtype;
genericptr_t dest;
{
    timer_element *curr;
	timer_element *tmp;
	/* loop over src's local chain, which is safe as we add to dest's chain and the processing loop */
    for (curr = src_timer; curr; curr = curr->tnxt)
	{
		tmp = start_timer(curr->timeout-monstermoves, tmtype, curr->func_index, dest);
		tmp->timerflags = curr->timerflags;
		adj_procchain_tm(tmp);
    }
}


/* safely reduce remaining time on the summoned monster by amt */
void
abjure_summon(mon, amt)
struct monst * mon;
int amt;
{
	struct esum * esum = get_mx(mon, MX_ESUM);
	timer_element * tm;
	if (!esum) return;
	if (esum->permanent) return;
	if (!(tm = get_timer(mon->timed, DESUMMON_MON))) return;
	adjust_timer_duration(tm, -min(amt, tm->timeout - monstermoves));
	run_timers();
}
/* when a summoner dies or changes levels, all of its summons disappear */
void
summoner_gone(mon, travelling)
struct monst * mon;
boolean travelling;	/* if true, don't vanish summoned items in its inventory */
{
	if (!mon) return;
	timer_element * tm;
	struct esum * esum;
	for (tm = timer_base; tm; tm = tm->next) {
		if (
			(tm->func_index == DESUMMON_MON && (((struct monst *)tm->arg)->mextra_p) && (esum = ((struct monst *)tm->arg)->mextra_p->esum_p) && (mon == esum->summoner)) ||
			(tm->func_index == DESUMMON_OBJ && (((struct obj   *)tm->arg)->oextra_p) && (esum = ((struct obj   *)tm->arg)->oextra_p->esum_p) && (mon == esum->summoner))
			)
		{
			/* exception 1: summoned pets may follow the player between levels */
			if ((tm->func_index == DESUMMON_MON) && (mon == &youmonst)) {
				struct monst * mtmp;
				for (mtmp = mydogs; mtmp && mtmp != ((struct monst *)tm->arg); mtmp = mtmp->nmon);
				if (mtmp)
					continue;	/* don't desummon this monster */
			}
			/* exception 2: a summoner's summoned items should not disappear if they are within its inventory while travelling */
			if ((tm->func_index == DESUMMON_OBJ) && travelling) {
				struct obj * otmp = ((struct obj   *)tm->arg);
				if (otmp->where == OBJ_MINVENT && otmp->ocarry == mon)
					continue;	/* don't desummon this item */
				/* exception 2b: if they are a pet travelling with the player and the item is on another pet travelling with player or the player */
				struct monst * mtmp;
				for (mtmp = mydogs; mtmp && mtmp != mon; mtmp = mtmp->nmon);
				if (mtmp) {
					if (otmp->where == OBJ_INVENT)
						continue;	/* in player's inventory */
					if (otmp->where == OBJ_MINVENT) {
						for (mtmp = mydogs; mtmp && mtmp != otmp->ocarry; mtmp = mtmp->nmon);
						if (mtmp)
							continue;	/* in travelling pet's inventory */
					}
				}
			}

			adjust_timer_duration(tm, min(0, monstermoves - tm->timeout));
			flags.run_timers = TRUE;

			/* remove "permanent" flag from esum so it will despawn */
			esum->permanent = 0;
			/* remove pointer to summoner, who is gone (and may be freed soon) */
			/* adjusting summonpwr of summoner is moot */
			esum->summoner = (struct monst *)0;
		}
	}
}

/*
 * stops all corpse-related timers on otmp
 */
void
stop_corpse_timers(otmp)
struct obj * otmp;
{
	if(!otmp->timed)
		return;
	(void) stop_timer(ROT_CORPSE, otmp->timed);
	(void) stop_timer(MOLDY_CORPSE, otmp->timed);
	(void) stop_timer(REVIVE_MON, otmp->timed);
	(void) stop_timer(SLIMY_CORPSE, otmp->timed);
	(void) stop_timer(ZOMBIE_CORPSE, otmp->timed);
	(void) stop_timer(SHADY_CORPSE, otmp->timed);
	(void) stop_timer(YELLOW_CORPSE, otmp->timed);
}

#endif /* OVL0 */

/*timeout.c*/

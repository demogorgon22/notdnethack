/*	SCCS Id: @(#)spell.h	3.4	1995/06/01	*/
/* Copyright 1986, M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SPELL_H
#define SPELL_H

/* spellmenu arguments; 0 thru n-1 used as spl_book[] index when swapping */
#define SPELLMENU_QUIVER (-6)
#define SPELLMENU_PICK (-5)
#define SPELLMENU_MAINTAIN (-4)
#define SPELLMENU_DESCRIBE (-3)
#define SPELLMENU_CAST (-2)
#define SPELLMENU_VIEW (-1)

struct spell {
    short	sp_id;			/* spell id (== object.otyp) */
    xchar	sp_lev;			/* power level */
    int		sp_know;		/* knowlege of spell */
	boolean	sp_ext;			/* spell is externally provided (by an artifact) */
};

#define KEEN 20000

#define GOAT_SPELL 0x1L

#define incrnknow(spell)        spl_book[spell].sp_know = KEEN
#define percdecrnknow(spell, knw)        spl_book[spell].sp_know = max(0, spl_book[spell].sp_know - (KEEN*knw)/100)

#define spellev(spell)		spl_book[spell].sp_lev
#define spellname(spell)	OBJ_NAME(objects[spellid(spell)])
#define spellet(spell)	\
	((char)((spell < 26) ? ('a' + spell) : ('A' + spell - 26)))

#define decrnknow(spell)	spl_book[spell].sp_know--
#define ndecrnknow(spell, knw)        spl_book[spell].sp_know = max(0, spl_book[spell].sp_know - knw)
#define spellid(spell)		spl_book[spell].sp_id
#define spellknow(spell)	spl_book[spell].sp_know
#define spellext(spell)		spl_book[spell].sp_ext
#define emergency_spell(spell) (spellid(spell) == SPE_HEALING || spellid(spell) == SPE_EXTRA_HEALING || \
                                spellid(spell) == SPE_MASS_HEALING || \
							    spellid(spell) == SPE_CURE_BLINDNESS || spellid(spell) == SPE_FULL_HEALING || \
							    spellid(spell) == SPE_RESTORE_ABILITY || spellid(spell) == SPE_REMOVE_CURSE)
#define metal_blocks_spellcasting(otmp) (otmp && \
	(is_metallic(otmp) || otmp->oartifact == ART_DRAGON_PLATE) && \
	!(check_oprop(otmp, OPROP_BRIL) || otmp->otyp == HELM_OF_BRILLIANCE \
	  || (otmp->otyp == HELM_OF_TELEPATHY && base_casting_stat() == A_CHA) \
	  || (is_imperial_elven_armor(otmp) && !(check_imp_mod(otmp, IEA_TELEPAT) \
											|| check_imp_mod(otmp, IEA_BOLTS) \
											|| check_imp_mod(otmp, IEA_KICKING) \
											|| check_imp_mod(otmp, IEA_DEFLECTION) \
											) \
	 )) \
	)

#define FIRST_LIGHT	MAXSPELL+1
#define PART_WATER	MAXSPELL+2
#define OVERGROW	MAXSPELL+3
#define APPLE_WORD		MAXSPELL+4

#define MAX_BONUS_DICE	10

#endif /* SPELL_H */

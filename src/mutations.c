#include "hack.h"
#include "mutations.h"

/* mutation lists */
const int shubbie_mutation_list[] = {ABHORRENT_SPORE,
						 CRAWLING_FLESH,
						 SHUB_RADIANCE,
						 TENDRIL_HAIR,
						 SHIFTING_MIND,
						 SHUB_CLAWS,
						 MIND_STEALER,
						 SHUB_TENTACLES,
						 0
						};

const int yog_mutation_list[] = {YOG_GAZE_1,
					 YOG_GAZE_2,
					 TWIN_MIND,
					 TWIN_DREAMS,
					 TWIN_SAVE,
					 0
					};

const struct mutationtype mutationtypes[] =
{
	{ ABHORRENT_SPORE, -1, "abhorrent spore", "An abhorrent spore has taken root on your body." },
	{ CRAWLING_FLESH, -1, "crawling flesh", "Your flesh crawls, closing wounds with horrid swiftness." },
	{ SHUB_RADIANCE, -1, "illumination of Shub-Nugganoth", "HER light shines in your eyes!" },
	{ TENDRIL_HAIR, -1, "hairlike tendrils", "Your body is covered with hairlike tendrils." },
	{ SHIFTING_MIND, -1, "shifting mind", "Your mind has shifted."},
	{ SHUB_CLAWS, HAND, "pointed claws", "Your fingernails can fuse into your fingers and transform into long claws."},
	{ MIND_STEALER, TONGUE, "mind-stealing tongue", "Your long thin tongue can slip into others' thoughts."},
	{ YOG_GAZE_1, EYE, "eyes of Yog-Sothoth", "Your eyes burn with magenta fire."},
	{ YOG_GAZE_2, EYE, "frenzy of Yog-Sothoth", "Your eyes blaze with magenta fire."},
	{ TWIN_MIND, -1, "chanting tentacles", "Your waist-tentacles chant strange spells."},
	{ TWIN_DREAMS, -1, "mind blasts", "You emit deadly dreams."},
	// { BY_THE_SMELL, -1, "bladder of Yog-Sothoth", "A gas-filled bladder swells then vanishes. An unholy stench fills the air!"},
	{ TWIN_SAVE, -1, "empathic link", "You feel a strong connection to your twin."},
	{ SHUB_TENTACLES, -1, "tentacle valves", "You have valves in your skull, allowing HER to reach through your mind."},
	//Tiefling traits, these use the start forming names and descriptions
	// Smell Traits
	{ TT_POISON_CLOUD, SMELL_TRAIT, "stinking clouds", "You can emit stinking clouds.", "corpse stench", "You stink like a rotting corpse."},
	{ TT_FIRE_BLAST_1, SMELL_TRAIT, "fiery blasts", "You can emit fiery blasts.", "sulferous smell", "You stink of sulfur."},
	{ TT_FIRE_BLAST_2, SMELL_TRAIT, "firey blasts", "You can emit fiery blasts.", "ashy smell", "You smell of ash and burnt flesh."},
	{ TT_COLD_BLAST, SMELL_TRAIT, "icy blasts", "You can emit icy blasts.", "frosty smell", "You emit a crip odor that unpleasantly stings the nostrils."},
	{ TT_ACID_BLAST, SMELL_TRAIT, "acidic blasts", "You can emit acidic blasts.", "pungent smell", "You emit a pungent, corrosive odor."},
	// Branch from small scales
	{ TT_NA_SCALES, BODY_SKIN, "brittle scales", "Your skin is covered in hard yet brittle scales.", "small scales", "Your skin is covered in small scales."},
	{ TT_DR_SCALES, BODY_SKIN, "durable scales", "Your skin is covered in durable rubbery scales.", "small scales", "Your skin is covered in small scales."},
	{ TT_SCALES, BODY_SKIN, "metallic scales", "Your skin is covered in metallic scales.", "small scales", "Your skin is covered in small scales."}, 
	// Branch from chitin
	// Fiberous vs. glassy chitin
	{ TT_CHITIN, BODY_SKIN, "chitinous plating", "Your skin is covered in chitinous plating.", "small chitin plates", "Your skin is covered in small chitin plates."},
	// Other skin traits
	{ TT_SLIPPERY_SKIN, BODY_SKIN, "slippery skin", "Your skin is extremely slippery.", "clamy skin", "Your skin is disagreeably clammy."},
	{ TT_FUNGUS_SKIN, BODY_SKIN, "deadly spores", "Deadly spores fly from your skin when struck.", "blotchy skin", "Your skin is covered in blotchy patches."},
	// Branch from long canines
	{ TT_SNAKE_FANGS, MOUTH_TRAIT, "venomous fangs", "You have venomous fangs.", "elongated canines", "Your canines are slightly elongated."},
	{ TT_VAMPIRE_FANGS, MOUTH_TRAIT, "vampiric fangs", "You have blood-sucking fangs.", "elongated canines", "Your canines are slightly elongated."},
	// Branch from bulging cheeks
	{ TT_SPIDER_FANGS, MOUTH_TRAIT, "poisonous chelicerae", "You have poisonous chelicerae tucked inside your mouth.", "bulging cheeks", "Your cheeks bulge slightly."},
	{ TT_SERPENT, MOUTH_TRAIT, "serpentine tongue", "A serpent-coiled second jaw has replaced your tongue.", "bulging cheeks", "Your cheeks bulge slightly."},
	{ TT_BLINDING_VENOM, MOUTH_TRAIT, "blinding venom", "You can spit a blinding venom.", "bulging cheeks", "Your cheeks bulge slightly."},
	// Branch from cloudy breath
	{ TT_SMOKE, WINDPIPE, "smoke breath", "You can exhale a cloud of noxious smoke.", "cloudy breath", "Your breath hangs in the air like white clouds."},
	{ TT_COLD_CLOUD, WINDPIPE, "cold breath", "You can a cloud of freezing mist.", "cloudy breath", "Your breath hangs in the air like white clouds."},
	// Eye traits
	// Branch from writing in eyes
	{ TT_HATEFUL_VISION, EYE, "hateful vision", "You can see curses and the weaknesses of others.", "black writing", "The white of your eyes is covered in tiny black script."},
	{ TT_ODD_EYES_1, EYE, "odd eyes", "Your gaze can demoralize foes.", "black writing", "The white of your eyes is covered in tiny black script."},
	// Branch from blank white eyes
	{ TT_INFRAVISION_1, EYE, "infravision", "You can see heat signatures.", "blank white eyes", "Your eyes are blank and white."},
	{ TT_EXTRAMISSION_1, EYE, "extramission", "You can see in the dark and light.", "blank white eyes", "Your eyes are blank and white."},
	{ TT_ODD_EYES_2, EYE, "odd eyes", "Your blank gaze can demoralize foes.", "milky eyes", "Your eyes are blank and white."},
	// Branch from extra eyes
	{ TT_BEHOLDER, EYE, "beholder eyes", "You have extra eyes that emit baleful rays.", "extra eyes", "You have many extra eyes."},
	{ TT_DISCOVERY_1, EYE, "vigilant eyes", "You have extra eyes that can spot hidden things.", "extra eyes", "You have many extra eyes."},
	{ TT_ODD_EYES_3, EYE, "odd eyes", "Your multitudinous gaze can demoralize foes.", "extra eyes", "You have many extra eyes."},
	// Branch from glowing eyes
	{ TT_LIGHT, EYE, "light emission", "Your burning eyes light up your surroundings.", "glowing eyes", "Your eyes glow faintly."},
	{ TT_EXTRAMISSION_2, EYE, "extramission", "You can see in the dark and light.", "glowing eyes", "Your eyes glow faintly."},
	{ TT_DISCOVERY_2, EYE, "keen eyes", "You can spot hidden things.", "glowing eyes", "Your eyes glow faintly."},
	{ TT_ODD_EYES_4, EYE, "odd eyes", "Your burning gaze can demoralize foes.", "glowing eyes", "Your eyes glow faintly."},
	// Branch from one large eye
	{ TT_PARALYSIS_GAZE, EYE, "paralysis gaze", "You can paralyze foes with your gaze.", "large eye", "You have one large eye instead of two."},
	{ TT_CANCEL_GAZE, EYE, "cancellation gaze", "You can cancel foes with your gaze.", "large eye", "You have one large eye instead of two."},
	{ TT_PROBING_GAZE, EYE, "probing gaze", "You can examine foes with your gaze.", "large eye", "You have one large eye instead of two."},
	// Misc eye mutations
	{ TT_MESMERIZING_GAZE, EYE, "mesmerizing gaze", "You can mesmerize foes with your gaze.", "swirling irises", "Your irises swirl hypnotically."},
	{ TT_CLOCKWORK_EYES, EYE, "foresight", "Your clockwork eyes can see an instant into the future.", "clockwork eyes", "You have spinning clockwork instead of eyes."},
	{ TT_TEARS_OF_BLOOD, EYE, "tears of blood", "Your foes weep blood in your presence.", "bleeding eyes", "Your eyes weep blood."},
	// Finger traits
	{ TT_COLD_TOUCH, FINGER, "cold touch", "You have a cold touch attack.", "cold fingers", "Your fingers feel cold to the touch."},
	{ TT_DRAIN_TOUCH, FINGER, "drain touch", "You have a life-draining touch attack.", "cold fingers", "Your fingers feel cold to the touch."},
	{ TT_FIRE_TOUCH, FINGER, "fire touch", "You have a fire touch attack.", "warm fingers", "Your fingers feel warm to the touch."},
	{ TT_SHOCK_TOUCH, FINGER, "shock touch", "You have an electric touch attack.", "tingling fingers", "Your fingers tingle slightly."},
	{ TT_EXTRA_FINGERS, FINGER, "enhanced spellcasting", "Your extra fingers improve your spellcasting.", "extra fingers", "You have extra fingers on each hand."},
	{ TT_WEBS, FINGER, "swimming webs", "You can swim.", "webbed fingers", "Your fingers are webbed."},
	// Branching from claws
	{ TT_RAZOR_CLAWS, FINGER, "razor claws", "Your claws are razor sharp.", "small claws", "You have small claws."},
	{ TT_HARD_CLAWS, FINGER, "hard claws", "Your claws are extremely hard.", "small claws", "You have small claws."},
	{ TT_HOOKED_CLAWS, FINGER, "hooked claws", "Your claws are hooked like a bird of prey's.", "small claws", "You have small claws."},
	{ TT_TALONS, FINGER, "talons", "Your have long talons.", "small claws", "You have small claws."},
	// Shadow traits
	{ TT_SHADOW_SHRED, SHADOW_TRAIT, "shredding shadow", "Your tortured shadow lashes out at foes.", "tortured shadow", "Your shadow writhes and twitches as though in agony."},
	{ TT_WANDERING_SHADOW, SHADOW_TRAIT, "summon shade", "Your shadow can fight alongside you.", "wandering shadow", "Your shadow sometimes wanders away from you."},
	{ TT_SHADOW_CASTER, SHADOW_TRAIT, "shadow casting", "Your shadow can cast spells at foes.", "gesticulating shadow", "Your shadow makes hateful gestures quite on its own."},
	// Blood traits
	{ TT_FIRE_COUNTER, BLOOD, "fiery blood", "Your attackers are burned when they draw your blood.", "warm body", "Your body feels warm to the touch."},
	{ TT_COLD_COUNTER, BLOOD, "icy blood", "Your attackers are chilled when they draw your blood.", "cool body", "Your body feels cool to the touch."},
	{ TT_ACID_COUNTER, BLOOD, "acidic blood", "Your attackers are burned when they draw your blood.", "bilious veins", "Your veins look bilious and unhealthy."},
	{ TT_POISON_COUNTER, BLOOD, "poisonous blood", "Your attackers are poisoned when they draw your blood.", "sickly pallor", "Your skin has a sickly pallor."},
	// Sound traits
	{ TT_FROG_CROAK, SOUND_TRAIT, "deafening croak", "You can emit a deafening croak.", "baggy throat", "Your throat is baggy and frog-like."},
	{ TT_ECHOLOCATION, SOUND_TRAIT, "echolocation", "You can navigate via echolocation.", "bat-like ears", "Your ears are large and bat-like."},
	{ TT_SIREN_SONG, SOUND_TRAIT, "siren song", "You can sing a hypnotic siren song.", "musical voice", "Your voice has a musical quality."},
	// Hair traits
	{ TT_THORN_HAIR, HAIR, "thorny hair", "Your attackers are stabbed by your thorny hair.", "spiky hair", "Your hair is spiky and stiff."},
	{ TT_FLAMING_HAIR, HAIR, "fiery aura", "Your blazing hair burns nearby foes.", "fiery hair", "Your hair blazes with fire."},
	{ TT_FROSTY_HAIR, HAIR, "icy aura", "Your icy hair freezes nearby foes.", "frosty hair", "Your hair is covered in frost."},
	{ TT_BLINDING_HAIR, HAIR, "blinding aura", "Your attackers are blinded by your crystalline hair.", "shimmering hair", "Your hair shimmers with a crystalline sheen."},
	{ TT_BITING_HAIR, HAIR, "biting hair", "Your hair bites at nearby foes.", "writhing hair", "Your hair writhes like a ball of leaches."},
	// Horn traits
	{ TT_RAMS_HORN, HORN_TRAIT, "ram's horns", "You have sturdy ram's horns.", "small horns", "You have small horns."},
	{ TT_DEMON_HORN, HORN_TRAIT, "demon horns", "You have wicked demon horns.", "small horns", "You have small horns."},
	{ TT_UNICORN_HORN, HORN_TRAIT, "diseased horn", "You have a diseased unicorn horn.", "small horn", "You have a small horn."},
	{ TT_ANTLERS, HORN_TRAIT, "antlers", "You have majestic antlers.", "budding antlers", "You have budding antlers."},
	{ TT_BULL_HORNS, HORN_TRAIT, "bull horns", "You have massive bull horns.", "small horns", "You have small horns."},
	// Tail traits
	// Branch from bump at the base of your spine.
	{ TT_PREHENSILE_TAIL, TAIL_TRAIT, "prehensile tail", "You have a prehensile tail.", "tail bud", "You have a bump at the base of your spine."},
	{ TT_LASHING_TAIL, TAIL_TRAIT, "lashing tail", "You have a lashing tail.", "tail bud", "You have a bump at the base of your spine."},
	{ TT_STINGER_TAIL, TAIL_TRAIT, "stinger tail", "You have a stinger-tipped tail.", "tail bud", "You have a bump at the base of your spine."},
	{ TT_SNAKE_TAIL, TAIL_TRAIT, "snake-headed tail", "You have a snake-headed tail.", "tail bud", "You have a bump at the base of your spine."},
	{ TT_THIEVING_TAIL, TAIL_TRAIT, "thieving tail", "You have a thieving tail.", "tail bud", "You have a bump at the base of your spine."},
	{ TT_LIZARD_TAIL, TAIL_TRAIT, "regeneration", "Your lizard's tail regenerates your wounds.", "tail bud", "You have a bump at the base of your spine."},
	{ TT_SPIDER_SPINNERS, TAIL_TRAIT, "silk spinners", "You can spin webs", "tail bud", "You have a bump at the base of your spine."},
	// Wing traits
	{ TT_WINGS_1, WINGS_BP, "flying", "You fly on bat-like wings.", "vestigial wings", "You have tiny bat-like wings."},
	{ TT_WINGS_2, WINGS_BP, "flying", "You fly on black-feathered wings.", "vestigial wings", "You have tiny black-feathered wings."},
	{ TT_WINGS_3, WINGS_BP, "flying", "You fly on white-feathered wings.", "vestigial wings", "You have tiny white-feathered wings."},
	{ TT_WINGS_4, WINGS_BP, "flying", "You fly on red-feathered wings.", "vestigial wings", "You have tiny red-feathered wings."},
	{ TT_WINGS_5, WINGS_BP, "flying", "You fly on giant fly's wings.", "vestigial wings", "You have tiny fly's wings."},
	// Aura traits
	{ TT_NA_AURA, AURA_TRAIT, "unsetling aura", "Your unsettling aura turns away blows.", "unsettling aura", "You have an unsettling aura."},
	{ TT_DR_AURA, AURA_TRAIT, "resistant aura", "Your unsettling aura resists blows.", "unsettling aura", "You have an unsettling aura."},
	// Appearance traits
	{ TT_ATTRACTIVE_1, APPEARANCE_TRAIT, "charming appearance", "You can charm others into giving you gifts.", "attractive appearance", "You are supernaturally attractive."},
	{ TT_ATTRACTIVE_2, APPEARANCE_TRAIT, "disarming appearance", "You can seduce others into disarming themselves.", "attractive appearance", "You are supernaturally attractive."},
	// Misc non-bodypart mutations
	{ TT_MAGIC_BREATHING, -1, "magic breathing", "You don't need to breathe.", "drowned appearance", "You look like a drowned corpse."},
	// End marker
	{ 0 },
};

void
confer_mutation(mutation)
int mutation;
{
	int i;
	if(mutation == SHIFTING_MIND){
		reset_skills();
	}
	else add_mutation(mutation);
	
	if(mutation <= LAST_CULT_MUTATION || !Upolyd) for (i = 0; mutationtypes[i].mutation; i++){
		if(mutation == mutationtypes[i].mutation){
			pline("%s", mutationtypes[i].description);
		}
	}

	if(mutation == SHUB_RADIANCE){
		calc_total_maxhp();
		calc_total_maxen();
	}
}

boolean
any_mutation()
{
	for(int i = 0; i < MUTATION_LISTSIZE; i++)
		if(u.mutations[i])
			return TRUE;
	if(u.next_tiefling_mutation)
		return TRUE;
	return FALSE;
}
void
init_natural_mutations()
{
	if(!Race_if(PM_TIEFLING)) return;
	int mutation_blacklist[] = {TT_TEARS_OF_BLOOD, TT_BLINDING_HAIR, TT_WINGS_1, TT_WINGS_2, TT_WINGS_3, TT_WINGS_4, TT_WINGS_5, TT_MAGIC_BREATHING };
	int possible_mutations[LAST_MUTATION];
	int possible_count = 0;
	for(int j = 0; mutationtypes[j].mutation; j++){
		int mut = mutationtypes[j].mutation;
		if(mut <= LAST_CULT_MUTATION) continue;
		boolean blacklisted = FALSE;
		for(int k = 0; k < sizeof(mutation_blacklist)/sizeof(int); k++){
			if(mut == mutation_blacklist[k]){
				blacklisted = TRUE;
				break;
			}
		}
		if(blacklisted) continue;
		possible_mutations[possible_count++] = mut;
	}
	int chosen_mutation = possible_mutations[rn2(possible_count)];
	u.next_tiefling_mutation = chosen_mutation;
	u.next_mutation_level = 3;
}

void
check_natural_mutations()
{
	if(!Race_if(PM_TIEFLING)) return;
	if(u.next_tiefling_mutation && u.ulevel >= u.next_mutation_level){
		confer_mutation(u.next_tiefling_mutation);
		u.next_tiefling_mutation = 0;
	}
	boolean new_mutation = FALSE;
	for(int i = 0; i < SIZE(flags.tiefling_level); i++){
		if(u.ulevel == flags.tiefling_level[i]){
			new_mutation = TRUE;
			flags.tiefling_level[i] = -1; // Prevent getting the same mutation again
			break;
		}
	}
	if(!new_mutation) return;
	int type_used[MUTATION_TYPE_SIZE];
	memset(type_used, 0, sizeof(type_used));
	// Mark existing mutation types as used
	for(int j = 0; mutationtypes[j].mutation; j++){
		int i = mutationtypes[j].mutation;
		if(i > LAST_CULT_MUTATION && has_mutation(i) && mutationtypes[j].bodypart >= 0){
			type_used[mutationtypes[j].bodypart] = 1;
			break;
		}
	}
	// Pick a new mutation
	int possible_mutations[LAST_MUTATION];
	int possible_count = 0;
	for(int j = 0; mutationtypes[j].mutation; j++){
		int mut = mutationtypes[j].mutation;
		if(mut <= LAST_CULT_MUTATION) continue;
		if(type_used[mutationtypes[j].bodypart]) continue;
		if(has_mutation(mut)) continue;
		possible_mutations[possible_count++] = mut;
	}
	if(possible_count < 1){
		impossible("No possible tiefling mutations left!");
		return;
	}
	int chosen_mutation = possible_mutations[rn2(possible_count)];
	if(u.next_tiefling_mutation != 0){
		pline("Error recovery: next_tiefling_mutation already set to %d", u.next_tiefling_mutation);
		confer_mutation(u.next_tiefling_mutation);
	}
	u.next_tiefling_mutation = chosen_mutation;
	u.next_mutation_level = u.ulevel + 3;
	if(!Upolyd){
		for(int j = 0; mutationtypes[j].mutation; j++){
			if(chosen_mutation == mutationtypes[j].mutation){
				pline("%s", mutationtypes[j].start_forming);
				break;
			}
		}
	}
}

GAMEDIR = notdnethackdir

# only used for generating include/macromagic.h
PYTHON = python3

# gprof flags
# CFLAGS = -pg
CFLAGS = -g3

# gprof flags
# LDFLAGS += -pg -Wno-knr-promoted-parameter
LDFLAGS += -Wno-knr-promoted-parameter

GAMELIBS = -lncursesw -lm

-include local.mk

CPPFLAGS += -Wno-knr-promoted-parameter
CPPFLAGS += -Iinclude
CPPFLAGS += -DDLB
CPPFLAGS += -Wall
CPPFLAGS += -Wno-comment
CPPFLAGS += -Wno-unused-variable
CPPFLAGS += -Wno-misleading-indentation
CPPFLAGS += -Wno-unused-but-set-variable
CPPFLAGS += -Wno-unused-function
CPPFLAGS += -Wno-unused-label
CPPFLAGS += -Wno-unknown-pragmas
CPPFLAGS += -Wno-missing-braces
CPPFLAGS += -Wno-format-overflow
CPPFLAGS += -std=gnu17

.DELETE_ON_ERROR:

.PHONY: all
all: src/notdnethack util/recover dat/nhdat dat/license

ATOMIC_LN = ln $(1) $(2).new && mv $(2).new $(2)

.PHONY: install
install: all
	mkdir -p $(GAMEDIR)
	install src/notdnethack $(GAMEDIR)
	install util/recover $(GAMEDIR)
	install -m 644 dat/nhdat dat/license $(GAMEDIR)
	touch $(GAMEDIR)/perm
	touch $(GAMEDIR)/record
	touch $(GAMEDIR)/logfile
	touch $(GAMEDIR)/xlogfile
	touch $(GAMEDIR)/livelog
	mkdir -p $(GAMEDIR)/save

##### BINARIES #####

SRCOBJ = allmain.o alloc.o apply.o artifact.o astar.o attrib.o ball.o bones.o	\
         botl.o cmd.o crown.o dbridge.o decl.o detect.o dig.o display.o dlb.o	\
         do.o do_name.o do_wear.o dog.o dogmove.o dokick.o dothrow.o	\
         drawing.o dungeon.o eat.o end.o engrave.o enlighten.o exper.o explode.o	\
         extralev.o files.o fountain.o hack.o hacklib.o invent.o	\
         light.o lock.o mail.o makemon.o mapglyph.o mcastu.o 		\
         mextra.o minion.o mklev.o mkmap.o mkmaze.o mkobj.o mkroom.o	\
         mon.o mondata.o monmove.o monst.o monstr.o mplayer.o		\
         mthrowu.o muse.o music.o mutations.o o_init.o objects.o objnam.o		\
         oextra.o options.o pager.o pickup.o pline.o polyself.o potion.o	\
         pray.o priest.o projectile.o quest.o questpgr.o read.o 	\
         recover.o rect.o region.o research.o restore.o rip.o rnd.o role.o 	\
         rumors.o save.o seduce.o shk.o shknam.o sit.o sounds.o 	\
         sp_lev.o spell.o steal.o steed.o teleport.o testing.o 		\
         thoughtglyph.o tile.o timeout.o topten.o track.o trap.o 	\
         u_init.o unicode.o vault.o version.o vision.o weapon.o 	\
         were.o wield.o windows.o wizard.o worm.o worn.o write.o 	\
         xhity.o xhityhelpers.o zap.o 
SYSUNIXOBJ = unixmain.o unixres.o unixunix.o
SYSSHAREOBJ = ioctl.o unixtty.o
WINTTYOBJ = getline.o termcap.o topl.o wintty.o
WINCURSESOBJ = cursdial.o cursinit.o cursmain.o cursmesg.o cursmisc.o	\
               cursstat.o curswins.o cursinvt.o

GAME_O = $(SRCOBJ:%.o=src/%.o) $(SYSUNIXOBJ:%.o=sys/unix/%.o)	\
         $(SYSSHAREOBJ:%.o=sys/share/%.o)			\
         $(WINTTYOBJ:%.o=win/tty/%.o)				\
         $(WINCURSESOBJ:%.o=win/curses/%.o)
src/notdnethack: $(GAME_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(GAMELIBS) -o $@
AUTO_BIN += src/notdnethack

RECOVER_O = util/recover_main.o src/recover.o
util/recover: $(RECOVER_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
AUTO_BIN += util/recover

MAKEDEFS_O = util/makedefs.o src/alloc.o src/monst.o src/objects.o	\
             util/panic.o
util/makedefs: $(MAKEDEFS_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
AUTO_BIN += util/makedefs

DLB_O = util/dlb_main.o src/alloc.o src/dlb.o util/panic.o
util/dlb: $(DLB_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
AUTO_BIN += util/dlb

DGN_COMP_O = util/dgn_main.o util/dgn_lex.o util/dgn_yacc.o	\
             src/alloc.o util/panic.o
util/dgn_comp: $(DGN_COMP_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
AUTO_BIN += util/dgn_comp

LEV_COMP_O = util/lev_main.o util/lev_lex.o util/lev_yacc.o	\
             src/alloc.o src/decl.o src/drawing.o src/monst.o	\
             src/objects.o util/panic.o
util/lev_comp: $(LEV_COMP_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
AUTO_BIN += util/lev_comp

TILEMAP_O = win/share/tilemap.o
util/tilemap: $(TILEMAP_O)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
AUTO_BIN += util/tilemap

ALL_O = $(GAME_O) $(RECOVER_O) $(MAKEDEFS_O) $(DLB_O) $(DGN_COMP_O)	\
        $(LEV_COMP_O) $(TILEMAP_O)

##### BASIC RULES AND AUTOMATIC DEPENDENCY GENERATION #####

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c -o $@ $<

%.d:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM -MP -MG -MT $*.o -MF $@ $*.c

ALL_D = $(ALL_O:%.o=%.d)
-include $(ALL_D)

##### AUTOGENERATED SOURCE FILES #####

export COMMIT_DESC := $(shell git describe --always)
include/date.h: util/makedefs $(filter-out src/version.o,$(GAME_O))
	cd util && ./makedefs -a  # include/date.h
include/onames.h: util/makedefs
	cd util && ./makedefs -o  # include/onames.h
include/gnames.h: util/makedefs
	cd util && ./makedefs -g  # include/gnames.h
include/pm.h: util/makedefs
	cd util && ./makedefs -p  # include/pm.h
include/verinfo.h: util/makedefs
	cd util && ./makedefs -w  # include/verinfo.h
include/macromagic.h: util/MacroMagicMarker.py doc/macromagic.txt
	command -v $(PYTHON) >/dev/null && $(PYTHON) util/MacroMagicMarker.py || true  # include/macromagic.h
AUTO_H += include/date.h include/onames.h include/gnames.h include/pm.h include/verinfo.h include/macromagic.h

# generating the dependencies of source files requires them to exist,
# so the dependencies of the autogenerated source files have to be
# declared explicitly instead

src/monstr.c: util/makedefs
	cd util && ./makedefs -m  # src/monstr.c
AUTO_C += src/monstr.c

include/dgn_comp.h include/lev_comp.h: include/%_comp.h: util/%_yacc.c
util/dgn_yacc.c util/lev_yacc.c: util/%_yacc.c: util/%_comp.y
	bison --defines=include/$*_comp.h -o util/$*_yacc.c util/$*_comp.y
AUTO_H += include/dgn_comp.h include/lev_comp.h
AUTO_C += util/dgn_yacc.c util/lev_yacc.c

util/dgn_yacc.o: include/verinfo.h
util/lev_yacc.o: include/onames.h include/gnames.h include/pm.h

util/dgn_lex.c util/lev_lex.c: util/%_lex.c: util/%_comp.l
	flex -o$@ $<
AUTO_C += util/dgn_lex.c util/lev_lex.c

util/dgn_lex.o: include/dgn_comp.h
util/lev_lex.o: include/lev_comp.h include/onames.h include/gnames.h include/pm.h

src/tile.c: util/tilemap
	cd util && ./tilemap  # src/tile.c
AUTO_C += src/tile.c

src/tile.o: include/pm.h include/onames.h

# to bootstrap the dependency generation, we want to not depend on
# autogenerated header files or source files, but we'll still need the
# correct dependencies on and for those when compiling, at least for
# the first time; missing headers are added to the initial dependency
# files without the correct prefix, so we add those here, and we
# already specified the dependencies to the autogenerated header files
# from the autogenerated source files above, so we don't need to
# bootstrap the dependency files for the autogenerated source files
# because they'll be created whenever they're actually compiled
$(AUTO_H:include/%.h=%.h): %.h: include/%.h ;
$(AUTO_C:%.c=%.d): ;

##### DATA FILES #####

QUEST_DES = Arch.des Anachrononaut.des Android.des Barb.des Bard.des Binder.des Caveman.des 	\
            Convict.des Drow.des DrowNoble.des DrowHealer.des Elf.des Erebor.des GnomeRanger.des	\
            HalfDragonFemaleNoble.des Healer.des Hedrow.des HedrowNoble.des Knight.des \
			Monk.des Moria.des Madman.des Noble.des Pirate.des Priest.des Ranger.des Rogue.des	\
            Samurai.des Tourist.des UndeadHunter.des Valkyrie.des Wizard.des Anachronounbinder.des Salamander.des

SPEC_DES = bigroom.des blacktemple.des castle.des chaos.des chaos2.des chaos3.des	\
           endgame.des gehennom.des knox.des labr.des law.des		\
           medusa.des grue.des mines.des neutrality.des oracle.des sokoban.des	\
           storage.des sunlesssea.des tomb.des tower.des yendor.des \
	   void.des sacristy.des nowhere.des spire.des village.des \
	   icecave.des blackforest.des dismalswamp.des archipelago.des

ALL_TAG = $(QUEST_DES:%.des=dat/%.tag) $(SPEC_DES:%.des=dat/%.tag)

$(ALL_TAG): dat/%.tag: dat/%.des util/lev_comp
	cd dat && ../util/lev_comp $(<F) && touch $(@F)
AUTO_DAT += $(ALL_TAG)

DAT_NHDAT = cmdhelp data dungeon help hh history opthelp options	\
            oracles quest.dat rumors wizhelp
dat/nhdat: util/dlb $(DAT_NHDAT:%=dat/%) $(ALL_TAG)
	cd dat && ../util/dlb cf nhdat $(DAT_NHDAT) *.lev
AUTO_DAT += dat/nhdat

dat/data: dat/data.base util/makedefs
	cd util && ./makedefs -d  # dat/data
dat/options: util/makedefs
	cd util && ./makedefs -t  # dat/options
dat/oracles: dat/oracles.txt util/makedefs
	cd util && ./makedefs -h  # dat/oracles
dat/quest.dat: dat/quest.txt util/makedefs
	cd util && ./makedefs -q  # dat/quest.dat
dat/rumors: dat/rumors.tru dat/rumors.fal util/makedefs
	cd util && ./makedefs -r  # dat/rumors
AUTO_DAT += dat/data dat/options dat/oracles dat/quest.dat dat/rumors

dat/dungeon: dat/dungeon.pdf util/dgn_comp
	cd dat && ../util/dgn_comp dungeon.pdf  # dat/dungeon
dat/dungeon.pdf: dat/dungeon.def util/makedefs
	cd util && ./makedefs -e  # dat/dungeon.pdf

AUTO_DAT += dat/dungeon dat/dungeon.pdf

##### CLEANING UP #####

.PHONY: clean
clean:
	rm -f $(AUTO_BIN)
	rm -f $(ALL_O)

	rm -f $(filter-out include/macromagic.h,$(AUTO_H))
	rm -f $(AUTO_C)

	rm -f $(AUTO_DAT)
	rm -f dat/*.lev

.PHONY: cleandeps
cleandeps: clean
	rm -f $(ALL_D)

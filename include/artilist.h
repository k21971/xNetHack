/* NetHack 3.7  artilist.h      $NHDT-Date: 1710957374 2024/03/20 17:56:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.30 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#if defined(MAKEDEFS_C) || defined (MDLIB_C)
/* in makedefs.c, all we care about is the list of names */

#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, mat, bn) nam

static const char *const artifact_names[] = {

#elif defined(ARTI_ENUM)
#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, mat, bn)                            \
    ART_##bn

#elif defined(DUMP_ARTI_ENUM)
#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, mat, bn)                            \
    { ART_##bn, "ART_" #bn }
#else
/* in artifact.c, set up the actual artifact list structure;
   color field is for an artifact when it glows, not for the item itself */

#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, \
          gs, gv, cost, clr, mat, bn)                            \
    {                                                            \
        typ, nam, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac,   \
        gs, gv, cost, clr, mat                                   \
    }

/* clang-format off */
#define     NO_ATTK     {0,0,0,0}               /* no attack */
#define     NO_DFNS     {0,0,0,0}               /* no defense */
#define     NO_CARY     {0,0,0,0}               /* no carry effects */
#define     DFNS(c)     {0,c,0,0}
#define     CARY(c)     {0,c,0,0}
#define     PHYS(a,b)   {0,AD_PHYS,a,b}         /* physical */
#define     DRLI(a,b)   {0,AD_DRLI,a,b}         /* life drain */
#define     COLD(a,b)   {0,AD_COLD,a,b}
#define     FIRE(a,b)   {0,AD_FIRE,a,b}
#define     ELEC(a,b)   {0,AD_ELEC,a,b}         /* electrical shock */
#define     STUN(a,b)   {0,AD_STUN,a,b}         /* magical attack */
#define     POIS(a,b)   {0,AD_DRST,a,b}         /* poison */

#define DEFAULT_MAT 0 /* use base object's default material */
/* clang-format on */

static NEARDATA struct artifact artilist[] = {
#endif /* MAKEDEFS_C || MDLIB_C */

    /* Artifact cost rationale:
     * 1.  The more useful the artifact, the better its cost.
     * 2.  Quest artifacts are highly valued.
     * 3.  Chaotic artifacts are inflated due to scarcity (and balance).
     *
     * Artifact gen_spe rationale:
     * 1.  If the artifact is useful against most enemies, +0.
     * 2.  If the artifact is useful against only a few enemies, usually +2.
     *     This gives the artifact use to early-game characters who receive
     *     it as a gift or find it on the ground.
     * 3.  Role gift gen_spe is chosen to balance against the role's
     *     default starting weapon (it should be better, but need not be
     *     much better).
     * 4.  This can be modified as required for special cases.
     *     (In some cases, like Excalibur, the value is irrelevant.)
     * 5.  Nonweapon spe may have a special meaning, so gen_spe for
     *     nonweapons must always be 0.
     *
     * Artifact gift_value is chosen so that "endgame-quality" artifacts are
     * not gifted in the early game (so that characters don't grind on an
     * altar early-game until they have their endgame weapon, then use it to
     * carry them through the game).  Those artifacts have values ranging from
     * around 8 to 10, based on how good the artifact is.  Less powerful
     * artifacts have values in the 1 to 5 range.  Values in between are used
     * for conditionally good artifacts.  (Note that the value of a gift is
     * normally 1 higher than the difficulty of the monster.)
     */

    /*  dummy element #0, so that all interesting indices are non-zero */
    A("", STRANGE_OBJECT, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE,
      NON_PM, NON_PM,
      0, 0, 0L, NO_COLOR, DEFAULT_MAT, NONARTIFACT),

    A("Excalibur", LONG_SWORD, (SPFX_NOGEN | SPFX_RESTR | SPFX_SEEK
                                | SPFX_DEFN | SPFX_INTEL | SPFX_SEARCH),
      0, 0, PHYS(5, 10), DRLI(0, 0), NO_CARY, 0, A_LAWFUL, PM_KNIGHT, NON_PM,
      0, 10, 4000L, NO_COLOR, DEFAULT_MAT, EXCALIBUR),
    /*
     *      Stormbringer only has a 2 because it can drain a level,
     *      providing 8 more.
     */
    A("Stormbringer", RUNESWORD,
      (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN | SPFX_INTEL | SPFX_DRLI), 0, 0,
      DRLI(5, 2), DRLI(0, 0), NO_CARY, 0, A_CHAOTIC, NON_PM, NON_PM,
      0, 9, 8000L, NO_COLOR, DEFAULT_MAT, STORMBRINGER),
    /*
     *      Mjollnir can be thrown when wielded if hero has 25 Strength
     *      (usually via gauntlets of power but possible with rings of
     *      gain strength).  If the thrower is a Valkyrie, Mjollnir will
     *      usually (99%) return and then usually (separate 99%) be caught
     *      and automatically be re-wielded.  When returning Mjollnir is
     *      not caught, there is a 50:50 chance of hitting hero for damage
     *      and its lightning shock might destroy some wands and/or rings.
     *
     *      Monsters don't throw Mjollnir regardless of strength (not even
     *      fake-player valkyries).
     */
    A("Mjollnir", WAR_HAMMER, /* Mjo:llnir */
      (SPFX_RESTR | SPFX_ATTK), 0, 0, ELEC(5, 24), NO_DFNS, NO_CARY,
      LIGHTNING_BOLT, A_NEUTRAL, PM_VALKYRIE, NON_PM,
      0, 8, 4000L, NO_COLOR, DEFAULT_MAT, MJOLLNIR),

    A("Cleaver", BATTLE_AXE, SPFX_RESTR, 0, 0, PHYS(3, 6), NO_DFNS, NO_CARY,
      0, A_NEUTRAL, PM_BARBARIAN, NON_PM,
      0, 8, 1500L, NO_COLOR, DEFAULT_MAT, CLEAVER),

    /*
     *      Grimtooth glows in warning when elves are present, but its
     *      damage bonus applies to all targets rather than just elves
     *      (handled as special case in spec_dbon()).
     */
    A("Grimtooth", ORCISH_DAGGER, (SPFX_RESTR | SPFX_WARN | SPFX_DFLAG2),
      0, M2_ELF, PHYS(2, 6), POIS(0, 0),
      NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ORC,
      0, 5, 300L, CLR_RED, DEFAULT_MAT, GRIMTOOTH),
    /*
     *      Orcrist and Sting have same alignment as elves.
     *
     *      The combination of SPFX_WARN+SPFX_DFLAG2+M2_value will trigger
     *      EWarn_of_mon for all monsters that have the M2_value flag.
     *      Sting and Orcrist will warn of M2_ORC monsters.
     */
    A("Orcrist", ELVEN_BROADSWORD, (SPFX_WARN | SPFX_DFLAG2), 0, M2_ORC,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ELF,
      3, 4, 2000L, CLR_BRIGHT_BLUE, DEFAULT_MAT, ORCRIST), /* actually light blue */

    A("Sting", ELVEN_DAGGER, (SPFX_WARN | SPFX_DFLAG2), 0, M2_ORC,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ELF,
      3, 1, 800L, CLR_BRIGHT_BLUE, DEFAULT_MAT, STING),
    /*
     *      Magicbane is a bit different!  Its magic fanfare
     *      unbalances victims in addition to doing some damage.
     */
    A("Magicbane", ATHAME, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,
      STUN(3, 4), DFNS(AD_MAGM), NO_CARY, 0, A_NEUTRAL, PM_WIZARD, NON_PM,
      0, 7, 3500L, NO_COLOR, DEFAULT_MAT, MAGICBANE),

    A("Frost Brand", SHORT_SWORD, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,
      COLD(5, 0), COLD(0, 0), NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      0, 9, 3000L, NO_COLOR, DEFAULT_MAT, FROST_BRAND),

    A("Fire Brand", SHORT_SWORD, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,
      FIRE(5, 0), FIRE(0, 0), NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      0, 5, 3000L, NO_COLOR, DEFAULT_MAT, FIRE_BRAND),

    A("Mirror Brand", SHORT_SWORD, (SPFX_RESTR | SPFX_REFLECT), 0, 0,
      NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      1, 5, 2500L, NO_COLOR, GLASS, MIRROR_BRAND), /* special damage added in artifact_hit() */

    A("Dragonbane", DWARVISH_SPEAR,
      (SPFX_WARN | SPFX_RESTR | SPFX_DCLAS | SPFX_REFLECT), 0, S_DRAGON,
      PHYS(5, 10), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      2, 5, 2500L, NO_COLOR, DEFAULT_MAT, DRAGONBANE),

    A("Demonbane", MACE, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_DEMON,
      PHYS(5, 0), NO_DFNS, NO_CARY, BANISH, A_LAWFUL, PM_CLERIC, NON_PM,
      1, 3, 2500L, NO_COLOR, SILVER, DEMONBANE),

    A("Werebane", SABER, (SPFX_WARN | SPFX_RESTR | SPFX_DFLAG2), 0, M2_WERE,
      PHYS(5, 10), DFNS(AD_WERE), NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      1, 4, 1500L, NO_COLOR, SILVER, WEREBANE),

    A("Grayswandir", SABER, (SPFX_RESTR | SPFX_HALRES), 0, 0,
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_LAWFUL, NON_PM, NON_PM,
      0, 10, 8000L, NO_COLOR, SILVER, GRAYSWANDIR),

    A("Giantslayer", LONG_SWORD,
      (SPFX_WARN | SPFX_RESTR | SPFX_DFLAG2), 0, M2_GIANT,
      PHYS(5, 10), NO_DFNS, NO_CARY, 0, A_NEUTRAL, NON_PM, NON_PM,
      2, 4, 500L, NO_COLOR, DEFAULT_MAT, GIANTSLAYER),

    A("Ogresmasher", WAR_HAMMER, (SPFX_RESTR | SPFX_HEAVYHIT), 0, 0,
      PHYS(3, 8), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      2, 1, 1200L, NO_COLOR, DEFAULT_MAT, OGRESMASHER),

    A("Trollsbane", MORNING_STAR,
      (SPFX_WARN | SPFX_RESTR | SPFX_DCLAS | SPFX_REGEN), 0, S_TROLL,
      PHYS(5, 10), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      2, 1, 1000L, NO_COLOR, DEFAULT_MAT, TROLLSBANE),
    /*
     *      Two problems:  1) doesn't let trolls regenerate heads,
     *      2) doesn't give unusual message for 2-headed monsters (but
     *      allowing those at all causes more problems than worth the effort).
     */
    A("Vorpal Blade", LONG_SWORD, (SPFX_RESTR | SPFX_BEHEAD), 0, 0,
      PHYS(5, 8), NO_DFNS, NO_CARY, 0, A_NEUTRAL, NON_PM, NON_PM,
      1, 5, 4000L, NO_COLOR, DEFAULT_MAT, VORPAL_BLADE),
    /*
     *      Ah, never shall I forget the cry,
     *              or the shriek that shrieked he,
     *      As I gnashed my teeth, and from my sheath
     *              I drew my Snickersnee!
     *                      --Koko, Lord high executioner of Titipu
     *                        (From Sir W.S. Gilbert's "The Mikado")
     */
    A("Snickersnee", KATANA, SPFX_RESTR, 0, 0, PHYS(0, 8), NO_DFNS, NO_CARY,
      0, A_LAWFUL, PM_SAMURAI, NON_PM,
      0, 8, 1200L, NO_COLOR, DEFAULT_MAT, SNICKERSNEE),

    /* Sunsword emits light when wielded (handled in the core rather than
       via artifact fields), but that light has no particular color */
    A("Sunsword", LONG_SWORD, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_UNDEAD,
      PHYS(5, 0), DFNS(AD_BLND), NO_CARY, BLINDING_RAY, A_LAWFUL, NON_PM,
      NON_PM,
      0, 6, 1500L, NO_COLOR, GOLD, SUNSWORD),

    A("The Apple of Discord", APPLE, SPFX_RESTR, 0, 0, NO_ATTK, NO_DFNS,
      NO_CARY, CONFLICT, A_CHAOTIC, NON_PM, NON_PM,
      0, 7, 4000L, NO_COLOR, GOLD, APPLE_OF_DISCORD),

    A("The Amulet of Storms", AMULET_OF_FLYING,
      (SPFX_RESTR | SPFX_DEFN), 0, 0,
      NO_ATTK, DFNS(AD_ELEC), NO_CARY, 0, A_CHAOTIC, NON_PM, NON_PM,
      0, 2, 600L, NO_COLOR, DEFAULT_MAT, AMULET_OF_STORMS),

    /*
     *      The artifacts for the quest dungeon, all self-willed.
     *      gen_spe should be 0; gift_value irrelevant and set to 12.
     */

    A("Itlachiayaque", SHIELD_OF_REFLECTION,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), SPFX_WARN, 0,
      NO_ATTK, NO_DFNS, CARY(AD_FIRE), SMOKE_CLOUD, A_LAWFUL, PM_ARCHEOLOGIST,
      NON_PM,
      0, 12, 3500L, NO_COLOR, GOLD, ITLACHIAYAQUE),

    A("The Heart of Ahriman", LUCKSTONE,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), SPFX_STLTH, 0,
      /* this stone does double damage if used as a projectile weapon */
      PHYS(5, 0), NO_DFNS, NO_CARY, LEVITATION, A_NEUTRAL, PM_BARBARIAN,
      NON_PM,
      0, 12, 2500L, NO_COLOR, DEFAULT_MAT, HEART_OF_AHRIMAN),

    A("Big Stick", CLUB,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DEFN),
      SPFX_STLTH, 0, /* "speak softly..." */
      PHYS(5, 12), DFNS(AD_MAGM), NO_CARY, CONFLICT, A_CHAOTIC, PM_CAVE_DWELLER,
      NON_PM,
      0, 12, 2500L, NO_COLOR, DEFAULT_MAT, BIG_STICK),

#if 0   /* OBSOLETE -- from 3.1.0 to 3.2.x, this was quest artifact for the
         * Elf role; in 3.3.0 elf became a race available to several roles
         * and the Elf role along with its quest was eliminated; it's a bit
         * overpowered to be an ordinary artifact so leave it excluded */
    A("The Palantir of Westernesse", CRYSTAL_BALL,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL),
                                     (SPFX_ESP | SPFX_REGEN | SPFX_HSPDAM), 0,
      NO_ATTK, NO_DFNS, NO_CARY, TAMING, A_CHAOTIC, NON_PM, PM_ELF,
      0, 12, 8000L, NO_COLOR, PALANTIR_OF_WESTERNESSE),
#endif

    A("The Staff of Aesculapius", QUARTERSTAFF,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_ATTK | SPFX_INTEL | SPFX_DRLI
       | SPFX_REGEN),
      0, 0, DRLI(0, 0), DRLI(0, 0), NO_CARY, HEALING, A_NEUTRAL, PM_HEALER,
      NON_PM,
      0, 12, 5000L, NO_COLOR, DEFAULT_MAT, STAFF_OF_AESCULAPIUS),

    A("The Magic Mirror of Merlin", MIRROR,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_SPEAK), SPFX_ESP, 0,
      NO_ATTK, NO_DFNS, CARY(AD_MAGM), 0, A_LAWFUL, PM_KNIGHT, NON_PM,
      0, 12, 1500L, NO_COLOR, DEFAULT_MAT, MAGIC_MIRROR_OF_MERLIN),

    A("The Eyes of the Overworld", LENSES,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_XRAY), 0, 0, NO_ATTK,
      DFNS(AD_MAGM), NO_CARY, ENLIGHTENING, A_NEUTRAL, PM_MONK, NON_PM,
      0, 12, 2500L, NO_COLOR, DEFAULT_MAT, EYES_OF_THE_OVERWORLD),

    A("The Sceptre of Might", MACE,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DALIGN), 0, 0, PHYS(5, 0),
      DRLI(0,0), NO_CARY, ENERGY_BOOST, A_LAWFUL, PM_CLERIC, NON_PM,
      /* METAL so it doesn't hurt elvish Priests */
      0, 12, 2500L, NO_COLOR, METAL, SCEPTRE_OF_MIGHT),

    /* Removed for now. This isn't a bad artifact, but the Sceptre of Might
     * offers more interesting choices for Priests than this does.
    A("The Mitre of Holiness", HELM_OF_BRILLIANCE,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_DFLAG2 | SPFX_INTEL | SPFX_PROTECT), 0,
      M2_UNDEAD, NO_ATTK, NO_DFNS, CARY(AD_FIRE), ENERGY_BOOST, A_LAWFUL,
      PM_CLERIC, NON_PM,
      0, 12, 2000L, NO_COLOR, SILVER, MITRE_OF_HOLINESS),
     */

    A("The Longbow of Diana", BOW,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_REFLECT), SPFX_ESP, 0,
      PHYS(5, 0), NO_DFNS, NO_CARY, CREATE_AMMO, A_CHAOTIC, PM_RANGER, NON_PM,
      0, 12, 4000L, NO_COLOR, DEFAULT_MAT, LONGBOW_OF_DIANA),

    /* MKoT has an additional carry property if the Key is not cursed (for
       rogues) or blessed (for non-rogues):  #untrap of doors and chests
       will always find any traps and disarming those will always succeed */
    A("The Master Key of Thievery", SKELETON_KEY,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_SPEAK),
      (SPFX_WARN | SPFX_TCTRL | SPFX_HPHDAM), 0, NO_ATTK, NO_DFNS, NO_CARY,
      UNTRAP, A_CHAOTIC, PM_ROGUE, NON_PM,
      0, 12, 3500L, NO_COLOR, DEFAULT_MAT, MASTER_KEY_OF_THIEVERY),

    A("The Tsurugi of Muramasa", TSURUGI,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_BEHEAD | SPFX_LUCK
       | SPFX_PROTECT),
      0, 0, PHYS(0, 8), NO_DFNS, NO_CARY, 0, A_LAWFUL, PM_SAMURAI, NON_PM,
      0, 12, 4500L, NO_COLOR, DEFAULT_MAT, TSURUGI_OF_MURAMASA),

    A("The Platinum Yendorian Express Card", CREDIT_CARD,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DEFN),
      (SPFX_ESP | SPFX_HSPDAM), 0, NO_ATTK, NO_DFNS, CARY(AD_MAGM),
      CHARGE_OBJ, A_NEUTRAL, PM_TOURIST, NON_PM,
      0, 12, 7000L, NO_COLOR, PLATINUM, YENDORIAN_EXPRESS_CARD),

    /* While this is the Valkyrie artifact, it's chaotic because it belongs to
     * Lord Surtur. */
    A("Sol Valtiva", TWO_HANDED_SWORD,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_ATTK | SPFX_INTEL),
      (SPFX_HSPDAM | SPFX_HPHDAM), 0, FIRE(3, 5), NO_DFNS, NO_CARY,
      LEV_TELE, A_CHAOTIC, PM_VALKYRIE, NON_PM,
      0, 12, 5000L, NO_COLOR, MITHRIL, SOL_VALTIVA),

    A("The Eye of the Aethiopica", AMULET_OF_ESP,
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), (SPFX_EREGEN | SPFX_HSPDAM), 0,
      NO_ATTK, DFNS(AD_MAGM), NO_CARY, CREATE_PORTAL, A_NEUTRAL, PM_WIZARD,
      NON_PM,
      0, 12, 4000L, NO_COLOR, DEFAULT_MAT, EYE_OF_THE_AETHIOPICA),

#if !defined(ARTI_ENUM) && !defined(DUMP_ARTI_ENUM)
    /*
     *  terminator; otyp must be zero
     */
    A(0, 0, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM,
      0, 0, 0L, 0, 0, TERMINATOR) /* 0 is CLR_BLACK rather than NO_COLOR but it
                                     doesn't matter here */

}; /* artilist[] (or artifact_names[]) */
#endif

#undef A

#ifdef NO_ATTK
#undef NO_ATTK
#undef NO_DFNS
#undef DFNS
#undef PHYS
#undef DRLI
#undef COLD
#undef FIRE
#undef ELEC
#undef STUN
#endif

/*artilist.h*/

/* NetHack 3.7	were.c	$NHDT-Date: 1717570494 2024/06/05 06:54:54 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.36 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void
were_change(struct monst *mon)
{
    if (!is_were(mon->data))
        return;

    if (is_human(mon->data)) {
        if (!Protection_from_shape_changers
            && !rn2(night() ? (flags.moonphase == FULL_MOON ? 3 : 30)
                            : (flags.moonphase == FULL_MOON ? 10 : 50))) {
            new_were(mon); /* change into animal form */
            gw.were_changes++;
            if (!Deaf && !canseemon(mon)) {
                const char *howler;
                const char *howl = "howling";

                switch (monsndx(mon->data)) {
                case PM_WEREWOLF:
                    howler = "wolf";
                    break;
                case PM_WEREJACKAL:
                    howler = "jackal";
                    break;
                case PM_WERERAT:
                    if (Hallucination) {
                        howler = "mouse";
                        howl = "squeaking";
                    }
                    else {
                        howler = NULL;
                    }
                    break;
                default:
                    howler = (char *) 0;
                    break;
                }
                if (howler) {
                    Soundeffect(se_canine_howl, 50);
                    You_hear("a %s %s at the moon.", howler, howl);
                    wake_nearto(mon->mx, mon->my, 4 * 4);
                }
            }
        }
    } else if (!rn2(30) || Protection_from_shape_changers) {
        new_were(mon); /* change back into human form */
        gw.were_changes++;
    }
}

int
counter_were(int pm)
{
    switch (pm) {
    case PM_WEREWOLF:
        return PM_HUMAN_WEREWOLF;
    case PM_HUMAN_WEREWOLF:
        return PM_WEREWOLF;
    case PM_WEREJACKAL:
        return PM_HUMAN_WEREJACKAL;
    case PM_HUMAN_WEREJACKAL:
        return PM_WEREJACKAL;
    case PM_WERERAT:
        return PM_HUMAN_WERERAT;
    case PM_HUMAN_WERERAT:
        return PM_WERERAT;
    default:
        return NON_PM;
    }
}

/* convert monsters similar to werecritters into appropriate werebeast */
int
were_beastie(int pm)
{
    switch (pm) {
    case PM_WERERAT:
    case PM_SEWER_RAT:
    case PM_GIANT_RAT:
    case PM_RABID_RAT:
        return PM_WERERAT;
    case PM_WEREJACKAL:
    case PM_JACKAL:
    case PM_FOX:
    case PM_COYOTE:
        return PM_WEREJACKAL;
    case PM_WEREWOLF:
    case PM_WOLF:
    case PM_WARG:
    case PM_WINTER_WOLF:
        return PM_WEREWOLF;
    default:
        break;
    }
    return NON_PM;
}

void
new_were(struct monst *mon)
{
    int pm;

    /* neither hero nor werecreature can change from human form to
       critter form if hero has Protection_from_shape_changers extrinsic;
       if already in critter form, always change to human form for that */
    if (Protection_from_shape_changers && is_human(mon->data))
        return;

    pm = counter_were(monsndx(mon->data));
    if (pm < LOW_PM) {
        impossible("unknown lycanthrope %s.",
                    mon->data->pmnames[NEUTRAL]);
        return;
    }

    if (canseemon(mon) && !Hallucination)
        pline("%s changes into a %s.", Monnam(mon),
              is_human(&mons[pm]) ? "human"
                                  /* pmname()+4: skip past "were" prefix */
                                  : pmname(&mons[pm], Mgender(mon)) + 4);

    set_mon_data(mon, &mons[pm]);
    if (helpless(mon)) {
        /* transformation wakens and/or revitalizes */
        mon->msleeping = 0;
        mon->mfrozen = 0; /* not asleep or paralyzed */
        mon->mcanmove = 1;
    }
    /* regenerate by 1/4 of the lost hit points */
    healmon(mon, (mon->mhpmax - mon->mhp) / 4, 0);
    newsym(mon->mx, mon->my);
    mon_break_armor(mon, FALSE);
    possibly_unwield(mon, FALSE);

    /* vision capability isn't changing so we don't call set_apparxy() to
       update mon's idea of where hero is; peaceful check is redundant */
    if (svc.context.mon_moving && !mon->mpeaceful
        && onscary(mon->mux, mon->muy, mon)
        && monnear(mon, mon->mux, mon->muy))
        monflee(mon, rn1(9, 2), TRUE, TRUE); /* 2..10 turns */
}

/* were-creature (even you) summons a horde */
int
were_summon(
    struct permonst *ptr,
    boolean yours,
    int *visible, /* number of visible helpers created */
    char *genbuf)
{
    int i, typ, pm = monsndx(ptr);
    struct monst *mtmp;
    int total = 0;

    *visible = 0;
    if (Protection_from_shape_changers && !yours)
        return 0;
    for (i = rnd(5); i > 0; i--) {
        switch (pm) {
        case PM_WERERAT:
        case PM_HUMAN_WERERAT:
            typ = rn2(3) ? PM_SEWER_RAT
                         : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT;
            if (genbuf)
                Strcpy(genbuf, "rat");
            break;
        case PM_WEREJACKAL:
        case PM_HUMAN_WEREJACKAL:
            typ = rn2(7) ? PM_JACKAL : rn2(3) ? PM_COYOTE : PM_FOX;
            if (genbuf)
                Strcpy(genbuf, "jackal");
            break;
        case PM_WEREWOLF:
        case PM_HUMAN_WEREWOLF:
            typ = rn2(5) ? PM_WOLF : rn2(2) ? PM_WARG : PM_WINTER_WOLF;
            if (genbuf)
                Strcpy(genbuf, "wolf");
            break;
        default:
            continue;
        }
        mtmp = makemon(&mons[typ], u.ux, u.uy, NO_MM_FLAGS);
        if (mtmp) {
            total++;
            if (canseemon(mtmp))
                *visible += 1;
        }
        if (yours && mtmp)
            (void) tamedog(mtmp, (struct obj *) 0, FALSE, FALSE);
    }
    return total;
}

void
you_were(void)
{
    char qbuf[QBUFSZ];
    boolean controllable_poly = Polymorph_control && !(Stunned || Unaware);

    if (Unchanging || u.umonnum == u.ulycn)
        return;
    if (controllable_poly) {
        /* `+4' => skip "were" prefix to get name of beast */
        Sprintf(qbuf, "Do you want to change into %s?",
                an(mons[u.ulycn].pmnames[NEUTRAL] + 4));
        if (!paranoid_query(ParanoidWerechange, qbuf))
            return;
    }
    gw.were_changes++;
    (void) polymon(u.ulycn, POLYMON_ALL_MSGS);
}

void
you_unwere(boolean purify)
{
    boolean controllable_poly = Polymorph_control && !(Stunned || Unaware);

    if (purify) {
        You_feel("purified.");
        set_ulycn(NON_PM); /* cure lycanthropy */
    }
    if (!Unchanging && is_were(gy.youmonst.data)
        && (!controllable_poly
            || !paranoid_query(ParanoidWerechange, "Remain in beast form?")))
        rehumanize();
    else if (is_were(gy.youmonst.data) && !u.mtimedone)
        u.mtimedone = rn1(200, 200); /* 40% of initial were change */
}

/* lycanthropy is being caught or cured, but no shape change is involved */
void
set_ulycn(int which)
{
    u.ulycn = which;
    /* add or remove lycanthrope's innate intrinsics (Drain_resistance) */
    set_uasmon();
}

/*were.c*/

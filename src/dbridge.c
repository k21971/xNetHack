/* NetHack 3.7	dbridge.c	$NHDT-Date: 1702349063 2023/12/12 02:44:23 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.62 $ */
/*      Copyright (c) 1989 by Jean-Christophe Collet              */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the drawbridge manipulation (create, open, close,
 * destroy).
 *
 * Added comprehensive monster-handling, and the "entity" structure to
 * deal with players as well. - 11/89
 *
 * Any traps and/or engravings at either the portcullis or span location
 * are destroyed whenever the bridge is lowered, raised, or destroyed.
 * (Engraving handling could be extended to flag whether an engraving on
 * the DB_UNDER surface is hidden by the lowered bridge, or one on the
 * bridge itself is hidden because the bridge has been raised, but that
 * seems like an awful lot of effort for very little gain.)
 */

#include "hack.h"

staticfn void get_wall_for_db(coordxy *, coordxy *);
staticfn struct entity *e_at(coordxy, coordxy);
staticfn void m_to_e(struct monst *, coordxy, coordxy, struct entity *);
staticfn void u_to_e(struct entity *);
staticfn void set_entity(coordxy, coordxy, struct entity *);
staticfn const char *e_nam(struct entity *);
staticfn const char *E_phrase(struct entity *, const char *);
staticfn boolean e_survives_at(struct entity *, coordxy, coordxy);
staticfn void e_died(struct entity *, int, int);
staticfn boolean automiss(struct entity *);
staticfn boolean e_missed(struct entity *, boolean);
staticfn boolean e_jumps(struct entity *);
staticfn void do_entity(struct entity *);
staticfn boolean cerberus_blocked(void);
staticfn void nokiller(void);

boolean
is_waterwall(coordxy x, coordxy y)
{
    if (isok(x, y) && IS_WATERWALL(levl[x][y].typ))
        return TRUE;
    return FALSE;
}

boolean
is_pool(coordxy x, coordxy y)
{
    schar ltyp;

    if (!isok(x, y))
        return FALSE;
    ltyp = levl[x][y].typ;
    /* The ltyp == MOAT is not redundant with is_moat, because the
     * Juiblex level does not have moats, although it has MOATs. There
     * is probably a better way to express this. */
    if (ltyp == POOL || ltyp == MOAT || ltyp == WATER || is_moat(x, y))
        return TRUE;
    return FALSE;
}

boolean
is_lava(coordxy x, coordxy y)
{
    schar ltyp;

    if (!isok(x, y))
        return FALSE;
    ltyp = levl[x][y].typ;
    if (ltyp == LAVAPOOL || ltyp == LAVAWALL
        || (ltyp == DRAWBRIDGE_UP
            && (levl[x][y].drawbridgemask & DB_UNDER) == DB_LAVA))
        return TRUE;
    return FALSE;
}

boolean
is_pool_or_lava(coordxy x, coordxy y)
{
    if (is_pool(x, y) || is_lava(x, y))
        return TRUE;
    else
        return FALSE;
}

boolean
is_ice(coordxy x, coordxy y)
{
    schar ltyp;

    if (!isok(x, y))
        return FALSE;
    ltyp = levl[x][y].typ;
    if (ltyp == ICE || (ltyp == DRAWBRIDGE_UP
                        && (levl[x][y].drawbridgemask & DB_UNDER) == DB_ICE))
        return TRUE;
    return FALSE;
}

boolean
is_moat(coordxy x, coordxy y)
{
    schar ltyp;

    if (!isok(x, y))
        return FALSE;
    ltyp = levl[x][y].typ;
    if (!Is_juiblex_level(&u.uz)
        && (ltyp == MOAT
            || (ltyp == DRAWBRIDGE_UP
                && (levl[x][y].drawbridgemask & DB_UNDER) == DB_MOAT)))
        return TRUE;
    return FALSE;
}

/* If the space is empty air - a precipice or chasm that monsters and objects
 * will fall down into, not the gravity-less air found in the Planes */
boolean
is_open_air(coordxy x, coordxy y)
{
    if (!isok(x, y))
        return FALSE;
    return (levl[x][y].typ == AIR && !In_endgame(&u.uz));
    /* potential future extension -- add a DB_AIR for drawbridge over air if
     * such a thing ever has a reason to exist */
}

schar
db_under_typ(int mask)
{
    switch (mask & DB_UNDER) {
    case DB_ICE:
        return ICE;
    case DB_LAVA:
        return LAVAPOOL;
    case DB_MOAT:
        return MOAT;
    default:
        return STONE;
    }
}

/*
 * We want to know whether a wall (or a door) is the portcullis (passageway)
 * of an eventual drawbridge.
 *
 * Return value:  the direction of the drawbridge, or -1 if not valid
 */
int
is_drawbridge_wall(coordxy x, coordxy y)
{
    struct rm *lev;

    if (!isok(x, y))
        return -1;

    lev = &levl[x][y];
    if (lev->typ != DOOR && lev->typ != DBWALL)
        return -1;

    if (isok(x + 1, y) && IS_DRAWBRIDGE(levl[x + 1][y].typ)
        && (levl[x + 1][y].drawbridgemask & DB_DIR) == DB_WEST)
        return DB_WEST;
    if (isok(x - 1, y) && IS_DRAWBRIDGE(levl[x - 1][y].typ)
        && (levl[x - 1][y].drawbridgemask & DB_DIR) == DB_EAST)
        return DB_EAST;
    if (isok(x, y - 1) && IS_DRAWBRIDGE(levl[x][y - 1].typ)
        && (levl[x][y - 1].drawbridgemask & DB_DIR) == DB_SOUTH)
        return DB_SOUTH;
    if (isok(x, y + 1) && IS_DRAWBRIDGE(levl[x][y + 1].typ)
        && (levl[x][y + 1].drawbridgemask & DB_DIR) == DB_NORTH)
        return DB_NORTH;

    return -1;
}

/*
 * Use is_db_wall where you want to verify that a
 * drawbridge "wall" is UP in the location x, y
 * (instead of UP or DOWN, as with is_drawbridge_wall).
 */
boolean
is_db_wall(coordxy x, coordxy y)
{
    return (boolean) (levl[x][y].typ == DBWALL);
}

/*
 * Return true with x,y pointing to the drawbridge if x,y initially indicate
 * a drawbridge or drawbridge wall.
 */
boolean
find_drawbridge(coordxy *x, coordxy *y)
{
    int dir;

    if (IS_DRAWBRIDGE(levl[*x][*y].typ))
        return TRUE;
    dir = is_drawbridge_wall(*x, *y);
    if (dir >= 0) {
        switch (dir) {
        case DB_NORTH:
            (*y)++;
            break;
        case DB_SOUTH:
            (*y)--;
            break;
        case DB_EAST:
            (*x)--;
            break;
        case DB_WEST:
            (*x)++;
            break;
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * Find the drawbridge wall associated with a drawbridge.
 */
staticfn void
get_wall_for_db(coordxy *x, coordxy *y)
{
    switch (levl[*x][*y].drawbridgemask & DB_DIR) {
    case DB_NORTH:
        (*y)--;
        break;
    case DB_SOUTH:
        (*y)++;
        break;
    case DB_EAST:
        (*x)++;
        break;
    case DB_WEST:
        (*x)--;
        break;
    }
}

/*
 * Creation of a drawbridge at pos x,y.
 *     dir is the direction.
 *     flag must be put to TRUE if we want the drawbridge to be opened.
 */
boolean
create_drawbridge(coordxy x, coordxy y, int dir, boolean flag)
{
    coordxy x2, y2;
    boolean horiz;
    boolean lava = levl[x][y].typ == LAVAPOOL; /* assume initialized map */

    x2 = x;
    y2 = y;
    switch (dir) {
    case DB_NORTH:
        horiz = TRUE;
        y2--;
        break;
    case DB_SOUTH:
        horiz = TRUE;
        y2++;
        break;
    case DB_EAST:
        horiz = FALSE;
        x2++;
        break;
    default:
        impossible("bad direction in create_drawbridge");
        FALLTHROUGH;
        /*FALLTHRU*/
    case DB_WEST:
        horiz = FALSE;
        x2--;
        break;
    }
    if (!IS_WALL(levl[x2][y2].typ))
        return FALSE;
    if (flag) { /* We want the bridge open */
        levl[x][y].typ = DRAWBRIDGE_DOWN;
        levl[x2][y2].typ = DOOR;
        set_doorstate(&levl[x2][y2], D_NODOOR);
    } else {
        levl[x][y].typ = DRAWBRIDGE_UP;
        levl[x2][y2].typ = DBWALL;
        /* Drawbridges are non-diggable. */
        levl[x2][y2].wall_info = W_NONDIGGABLE;
    }
    levl[x][y].horizontal = !horiz;
    levl[x2][y2].horizontal = horiz;
    levl[x][y].drawbridgemask = dir;
    if (lava)
        levl[x][y].drawbridgemask |= DB_LAVA;
    return  TRUE;
}

staticfn struct entity *
e_at(coordxy x, coordxy y)
{
    int entitycnt;

    for (entitycnt = 0; entitycnt < ENTITIES; entitycnt++)
        if (go.occupants[entitycnt].edata
            && go.occupants[entitycnt].ex == x
            && go.occupants[entitycnt].ey == y)
            break;
    debugpline1("entitycnt = %d", entitycnt);
#ifdef D_DEBUG
    wait_synch();
#endif
    return (entitycnt == ENTITIES) ? (struct entity *) 0
                                   : &(go.occupants[entitycnt]);
}

staticfn void
m_to_e(struct monst *mtmp, coordxy x, coordxy y, struct entity *etmp)
{
    etmp->emon = mtmp;
    if (mtmp) {
        etmp->ex = x;
        etmp->ey = y;
        if (mtmp->wormno && (x != mtmp->mx || y != mtmp->my))
            etmp->edata = &mons[PM_LONG_WORM_TAIL];
        else
            etmp->edata = mtmp->data;
    } else {
        etmp->edata = (struct permonst *) 0;
        etmp->ex = etmp->ey = 0;
    }
}

staticfn void
u_to_e(struct entity *etmp)
{
    etmp->emon = &gy.youmonst;
    etmp->ex = u.ux;
    etmp->ey = u.uy;
    etmp->edata = gy.youmonst.data;
}

staticfn void
set_entity(
    coordxy x, coordxy y, /* location of span or portcullis */
    struct entity *etmp)  /* pointer to occupants[0] or occupants[1] */
{
    if (u_at(x, y))
        u_to_e(etmp);
    else /* m_at() might yield Null; that's ok */
        m_to_e(m_at(x, y), x, y, etmp);
}

#define is_u(etmp) (etmp->emon == &gy.youmonst)
#define e_canseemon(etmp) (is_u(etmp) || canseemon(etmp->emon))

/*
 * e_strg is a utility routine which is not actually in use anywhere, since
 * the specialized routines below suffice for all current purposes.
 */

/* #define e_strg(etmp, func) (is_u(etmp) ? (char *) 0 : func(etmp->emon)) */

staticfn const char *
e_nam(struct entity *etmp)
{
    return is_u(etmp) ? "you" : mon_nam(etmp->emon);
}

/*
 * Generates capitalized entity name, makes 2nd -> 3rd person conversion on
 * verb, where necessary.
 */
staticfn const char *
E_phrase(struct entity *etmp, const char *verb)
{
    static char wholebuf[80];

    Strcpy(wholebuf, is_u(etmp) ? "You" : Monnam(etmp->emon));
    if (!verb || !*verb)
        return wholebuf;
    Strcat(wholebuf, " ");
    if (is_u(etmp))
        Strcat(wholebuf, verb);
    else
        Strcat(wholebuf, vtense((char *) 0, verb));
    return wholebuf;
}

/*
 * Simple-minded "can it be here?" routine
 */
staticfn boolean
e_survives_at(struct entity *etmp, coordxy x, coordxy y)
{
    if (noncorporeal(etmp->edata))
        return TRUE;
    if (is_pool(x, y))
        return (boolean) ((is_u(etmp) && (Wwalking || Amphibious || Breathless
                                          || Swimming || Flying || Levitation))
                          || is_swimmer(etmp->edata)
                          || !grounded(etmp->edata));
    /* must force call to lava_effects in e_died if is_u */
    if (is_lava(x, y))
        return (boolean) ((is_u(etmp) && (Levitation || Flying))
                          || likes_lava(etmp->edata)
                          || is_flyer(etmp->edata));
    if (is_db_wall(x, y))
        return (boolean) (is_u(etmp) ? Passes_walls
                          : passes_walls(etmp->edata));
    return TRUE;
}

staticfn void
e_died(struct entity *etmp, int xkill_flags, int how)
{
    if (is_u(etmp)) {
        if (how == DROWNING) {
            svk.killer.name[0] = 0; /* drown() sets its own killer */
            (void) drown();
        } else if (how == BURNING) {
            svk.killer.name[0] = 0; /* lava_effects() sets own killer */
            (void) lava_effects();
        } else {
            coord xy;

            /* use more specific killer if specified */
            if (!svk.killer.name[0]) {
                svk.killer.format = KILLED_BY_AN;
                Strcpy(svk.killer.name, "falling drawbridge");
            }
            done(how);
            /* So, you didn't die */
            if (!e_survives_at(etmp, etmp->ex, etmp->ey)) {
                if (enexto(&xy, etmp->ex, etmp->ey, etmp->edata)) {
                    pline("A %s force teleports you away...",
                          Hallucination ? "normal" : "strange");
                    teleds(xy.x, xy.y, TELEDS_NO_FLAGS);
                }
                /* otherwise on top of the drawbridge is the
                 * only viable spot in the dungeon, so stay there
                 */
            }
        }
        /* we might have crawled out of the moat to survive */
        etmp->ex = u.ux, etmp->ey = u.uy;
    } else {
        int entitycnt;

        svk.killer.name[0] = 0;
/* fake "digested to death" damage-type suppresses corpse */
#define mk_message(dest) (((dest & XKILL_NOMSG) != 0) ? (char *) 0 : "")
#define mk_corpse(dest) (((dest & XKILL_NOCORPSE) != 0) ? AD_DGST : AD_PHYS)
        /* if monsters are moving, one of them caused the destruction */
        if (svc.context.mon_moving)
            monkilled(etmp->emon,
                      mk_message(xkill_flags), mk_corpse(xkill_flags));
        else /* you caused it */
            xkilled(etmp->emon, xkill_flags);
        etmp->edata = (struct permonst *) 0;

        /* dead long worm handling */
        for (entitycnt = 0; entitycnt < ENTITIES; entitycnt++) {
            if (etmp != &(go.occupants[entitycnt])
                && etmp->emon == go.occupants[entitycnt].emon)
                go.occupants[entitycnt].edata = (struct permonst *) 0;
        }
#undef mk_message
#undef mk_corpse
    }
}

/*
 * These are never directly affected by a bridge or portcullis.
 */
staticfn boolean
automiss(struct entity *etmp)
{
    return (boolean) ((is_u(etmp) ? Passes_walls : passes_walls(etmp->edata))
                      || noncorporeal(etmp->edata));
}

/*
 * Does falling drawbridge or portcullis miss etmp?
 */
staticfn boolean
e_missed(struct entity *etmp, boolean chunks)
{
    int misses;

    if (chunks) {
        debugpline0("Do chunks miss?");
    }
    if (automiss(etmp))
        return TRUE;

    if (is_flyer(etmp->edata)
        && (is_u(etmp) ? !Unaware
                       : !helpless(etmp->emon)))
        /* flying requires mobility */
        misses = 5; /* out of 8 */
    else if (is_floater(etmp->edata)
             || (is_u(etmp) && Levitation)) /* doesn't require mobility */
        misses = 3;
    else if (chunks && is_pool(etmp->ex, etmp->ey))
        misses = 2; /* sitting ducks */
    else
        misses = 0;

    if (is_db_wall(etmp->ex, etmp->ey))
        misses -= 3; /* less airspace */

    debugpline1("Miss chance = %d (out of 8)", misses);

    return (misses >= rnd(8)) ? TRUE : FALSE;
}

/*
 * Can etmp jump from death?
 */
staticfn boolean
e_jumps(struct entity *etmp)
{
    int tmp = 4; /* out of 10 */

    if (is_u(etmp) ? (Unaware || Fumbling)
                   : (helpless(etmp->emon)
                      || !etmp->edata->mmove || etmp->emon->wormno))
        return FALSE;

    if (is_u(etmp) ? Confusion : etmp->emon->mconf)
        tmp -= 2;

    if (is_u(etmp) ? Stunned : etmp->emon->mstun)
        tmp -= 3;

    if (is_db_wall(etmp->ex, etmp->ey))
        tmp -= 2; /* less room to maneuver */

    debugpline2("%s to jump (%d chances in 10)", E_phrase(etmp, "try"), tmp);
    return (tmp >= rnd(10)) ? TRUE : FALSE;
}

staticfn void
do_entity(struct entity *etmp)
{
    coordxy newx, newy, oldx, oldy;
    int at_portcullis;
    boolean must_jump = FALSE, relocates = FALSE, e_inview;
    struct rm *crm;

    if (!etmp->edata)
        return;

    e_inview = e_canseemon(etmp);
    oldx = etmp->ex;
    oldy = etmp->ey;
    at_portcullis = is_db_wall(oldx, oldy);
    crm = &levl[oldx][oldy];

    if (automiss(etmp) && e_survives_at(etmp, oldx, oldy)) {
        if (e_inview && (at_portcullis || IS_DRAWBRIDGE(crm->typ)))
            pline_The("%s passes through %s!",
                      at_portcullis ? "portcullis" : "drawbridge",
                      e_nam(etmp));
        if (is_u(etmp))
            spoteffects(FALSE);
        return;
    }
    if (e_missed(etmp, FALSE)) {
        if (at_portcullis) {
            pline_The("portcullis misses %s!", e_nam(etmp));
        } else {
            debugpline1("The drawbridge misses %s!", e_nam(etmp));
        }
        if (e_survives_at(etmp, oldx, oldy)) {
            return;
        } else {
            debugpline0("Mon can't survive here");
            if (at_portcullis)
                must_jump = TRUE;
            else
                relocates = TRUE; /* just ride drawbridge in */
        }
    } else {
        if (crm->typ == DRAWBRIDGE_DOWN) {
            if (is_u(etmp)) {
                svk.killer.format = NO_KILLER_PREFIX;
                Strcpy(svk.killer.name,
                       "crushed to death underneath a drawbridge");
            }
            pline("%s crushed underneath the drawbridge.",
                  E_phrase(etmp, "are"));             /* no jump */
            e_died(etmp,
                   XKILL_NOCORPSE | (e_inview ? XKILL_GIVEMSG : XKILL_NOMSG),
                   CRUSHING); /* no corpse */
            return;       /* Note: Beyond this point, we know we're  */
        }                 /* not at an opened drawbridge, since all  */
        must_jump = TRUE; /* *missable* creatures survive on the     */
    }                     /* square, and all the unmissed ones die.  */
    if (must_jump) {
        if (at_portcullis) {
            if (e_jumps(etmp)) {
                relocates = TRUE;
                debugpline0("Jump succeeds!");
            } else {
                if (e_inview) {
                    pline("%s crushed by the falling portcullis!",
                          E_phrase(etmp, "are"));
                } else if (!Deaf) {
                    Soundeffect(se_crushing_sound, 100);
                    You_hear("a crushing sound.");
                }
                e_died(etmp,
                       XKILL_NOCORPSE | (e_inview ? XKILL_GIVEMSG
                                                  : XKILL_NOMSG),
                       CRUSHING);
                /* no corpse */
                return;
            }
        } else { /* tries to jump off bridge to original square */
            relocates = !e_jumps(etmp);
            debugpline1("Jump %s!", (relocates) ? "fails" : "succeeds");
        }
    }

    /*
     * Here's where we try to do relocation.  Assumes that etmp is not
     * arriving at the portcullis square while the drawbridge is
     * falling, since this square would be inaccessible (i.e. etmp
     * started on drawbridge square) or unnecessary (i.e. etmp started
     * here) in such a situation.
     */
    debugpline0("Doing relocation.");
    newx = oldx;
    newy = oldy;
    (void) find_drawbridge(&newx, &newy);
    if ((newx == oldx) && (newy == oldy))
        get_wall_for_db(&newx, &newy);
    debugpline0("Checking new square for occupancy.");
    if (relocates && (e_at(newx, newy))) {
        /*
         * Standoff problem: one or both entities must die, and/or
         * both switch places.  Avoid infinite recursion by checking
         * first whether the other entity is staying put.  Clean up if
         * we happen to move/die in recursion.
         */
        struct entity *other;

        other = e_at(newx, newy);
        debugpline1("New square is occupied by %s", e_nam(other));
        if (e_survives_at(other, newx, newy) && automiss(other)) {
            relocates = FALSE; /* "other" won't budge */
            debugpline1("%s suicide.", E_phrase(etmp, "commit"));
        } else {
            debugpline1("Handling %s", e_nam(other));
            while ((e_at(newx, newy) != 0) && (e_at(newx, newy) != etmp))
                do_entity(other);
            debugpline1("Checking existence of %s", e_nam(etmp));
#ifdef D_DEBUG
            wait_synch();
#endif
            if (e_at(oldx, oldy) != etmp) {
                debugpline1("%s moved or died in recursion somewhere",
                            E_phrase(etmp, "have"));
#ifdef D_DEBUG
                wait_synch();
#endif
                return;
            }
        }
    }
    if (relocates && !e_at(newx, newy)) { /* if e_at() entity = worm tail */
        debugpline1("Moving %s", e_nam(etmp));
        if (!is_u(etmp)) {
            remove_monster(etmp->ex, etmp->ey);
            place_monster(etmp->emon, newx, newy);
            update_monster_region(etmp->emon);
        } else {
            u.ux = newx;
            u.uy = newy;
        }
        etmp->ex = newx;
        etmp->ey = newy;
        e_inview = e_canseemon(etmp);
    }
    debugpline1("Final disposition of %s", e_nam(etmp));
#ifdef D_DEBUG
    wait_synch();
#endif
    if (is_db_wall(etmp->ex, etmp->ey)) {
        debugpline1("%s in portcullis chamber", E_phrase(etmp, "are"));
#ifdef D_DEBUG
        wait_synch();
#endif
        if (e_inview) {
            if (is_u(etmp)) {
                You("tumble towards the closed portcullis!");
                if (automiss(etmp))
                    You("pass through it!");
                else
                    pline_The("drawbridge closes in...");
            } else
                pline("%s behind the drawbridge.",
                      E_phrase(etmp, "disappear"));
        }
        if (!e_survives_at(etmp, etmp->ex, etmp->ey)) {
            svk.killer.format = KILLED_BY_AN;
            Strcpy(svk.killer.name, "closing drawbridge");
            e_died(etmp, XKILL_NOMSG, CRUSHING);
            return;
        }
        debugpline1("%s in here", E_phrase(etmp, "survive"));
    } else {
        debugpline1("%s on drawbridge square", E_phrase(etmp, "are"));
        if (is_pool(etmp->ex, etmp->ey) && !e_inview)
            if (!Deaf) {
                Soundeffect(se_splash, 100);
                You_hear("a splash.");
            }
        if (e_survives_at(etmp, etmp->ex, etmp->ey)) {
            if (e_inview && grounded(etmp->edata))
                pline("%s from the bridge.", E_phrase(etmp, "fall"));
            return;
        }
        debugpline1("%s cannot survive on the drawbridge square",
                    E_phrase(etmp, NULL));
        if (is_pool(etmp->ex, etmp->ey) || is_lava(etmp->ex, etmp->ey))
            if (e_inview && !is_u(etmp)) {
                /* drown() will supply msgs if nec. */
                boolean lava = is_lava(etmp->ex, etmp->ey);

                if (Hallucination)
                    pline("%s the %s and disappears.",
                          E_phrase(etmp, "drink"), lava ? "lava" : "moat");
                else
                    pline("%s into the %s.", E_phrase(etmp, "fall"),
                          lava ? hliquid("lava") : "moat");
            }
        svk.killer.format = NO_KILLER_PREFIX;
        Strcpy(svk.killer.name, "fell from a drawbridge");
        e_died(etmp, /* CRUSHING is arbitrary */
               XKILL_NOCORPSE | (e_inview ? XKILL_GIVEMSG : XKILL_NOMSG),
               is_pool(etmp->ex, etmp->ey) ? DROWNING
                 : is_lava(etmp->ex, etmp->ey) ? BURNING
                   : CRUSHING); /*no corpse*/
        return;
    }
}

/* clear stale reason for death and both 'entities' before returning */
staticfn void
nokiller(void)
{
    svk.killer.name[0] = '\0';
    svk.killer.format = 0;
    m_to_e((struct monst *) 0, 0, 0, &go.occupants[0]);
    m_to_e((struct monst *) 0, 0, 0, &go.occupants[1]);
}

/*
 * Close the drawbridge located at x,y
 */
void
close_drawbridge(coordxy x, coordxy y)
{
    struct rm *lev1, *lev2;
    struct trap *t;
    coordxy x2, y2;

    lev1 = &levl[x][y];
    if (lev1->typ != DRAWBRIDGE_DOWN)
        return;
    x2 = x;
    y2 = y;
    get_wall_for_db(&x2, &y2);
    if (cansee(x, y) || cansee(x2, y2)) {
        You_see("a drawbridge %s up!",
                (((u.ux == x || u.uy == y) && !Underwater)
                 || distu(x2, y2) < distu(x, y))
                    ? "coming"
                    : "going");
    } else { /* "5 gears turn" for castle drawbridge tune */
        Soundeffect(se_chains_rattling_gears_turning, 75);
        You_hear("chains rattling and gears turning.");
    }
    lev1->typ = DRAWBRIDGE_UP;
    lev2 = &levl[x2][y2];
    lev2->typ = DBWALL;
    switch (lev1->drawbridgemask & DB_DIR) {
    case DB_NORTH:
    case DB_SOUTH:
        lev2->horizontal = TRUE;
        break;
    case DB_WEST:
    case DB_EAST:
        lev2->horizontal = FALSE;
        break;
    }
    lev2->wall_info = W_NONDIGGABLE;
    set_entity(x, y, &(go.occupants[0]));
    set_entity(x2, y2, &(go.occupants[1]));
    do_entity(&(go.occupants[0]));          /* Do set_entity after first */
    set_entity(x2, y2, &(go.occupants[1])); /* do_entity for worm tail */
    do_entity(&(go.occupants[1]));
    if (OBJ_AT(x, y) && !Deaf) {
        Soundeffect(se_smashing_and_crushing, 75);
        You_hear("smashing and crushing.");
    }
    (void) revive_nasty(x, y, (char *) 0);
    (void) revive_nasty(x2, y2, (char *) 0);
    delallobj(x, y);
    delallobj(x2, y2);
    if ((t = t_at(x, y)) != 0)
        deltrap(t);
    if ((t = t_at(x2, y2)) != 0)
        deltrap(t);
    del_engr_at(x, y);
    del_engr_at(x2, y2);
    newsym(x, y);
    newsym(x2, y2);
    block_point(x2, y2); /* vision */
    nokiller();
}

/*
 * Open the drawbridge located at x,y.
 * Return TRUE if it was opened successfully, FALSE if not (or if x, y is not a
 * drawbridge).
 */
boolean
open_drawbridge(coordxy x, coordxy y)
{
    struct rm *lev1, *lev2;
    struct trap *t;
    coordxy x2, y2;

    lev1 = &levl[x][y];
    if (lev1->typ != DRAWBRIDGE_UP)
        return FALSE;
    if (cerberus_blocked()) {
        if (cansee(x, y))
            pline("The drawbridge vibrates, but doesn't budge.");
        else
            You_hear("a brief mechanical sound, but then it stops.");
        return FALSE;
    }
    x2 = x;
    y2 = y;
    get_wall_for_db(&x2, &y2);
    if (cansee(x, y) || cansee(x2, y2)) {
        You_see("a drawbridge %s down!",
                (distu(x2, y2) < distu(x, y)) ? "going" : "coming");
    } else { /* "5 gears turn" for castle drawbridge tune */
        Soundeffect(se_gears_turning_chains_rattling, 100);
        You_hear("gears turning and chains rattling.");
    }
    lev1->typ = DRAWBRIDGE_DOWN;
    lev2 = &levl[x2][y2];
    lev2->typ = DOOR;
    set_doorstate(lev2, D_NODOOR);
    set_entity(x, y, &(go.occupants[0]));
    set_entity(x2, y2, &(go.occupants[1]));
    do_entity(&(go.occupants[0]));          /* do set_entity after first */
    set_entity(x2, y2, &(go.occupants[1])); /* do_entity for worm tails */
    do_entity(&(go.occupants[1]));
    (void) revive_nasty(x, y, (char *) 0);
    delallobj(x, y);
    if ((t = t_at(x, y)) != 0)
        deltrap(t);
    if ((t = t_at(x2, y2)) != 0)
        deltrap(t);
    del_engr_at(x, y);
    del_engr_at(x2, y2);
    newsym(x, y);
    newsym(x2, y2);
    unblock_point(x2, y2); /* vision */
    if (Is_stronghold(&u.uz))
        u.uevent.uopened_dbridge = TRUE;
    nokiller();
    return TRUE;
}

/*
 * Let's destroy the drawbridge located at x,y
 */
void
destroy_drawbridge(coordxy x, coordxy y)
{
    struct rm *lev1, *lev2;
    struct trap *t;
    struct obj *otmp;
    coordxy x2, y2;
    int i;
    boolean e_inview;
    struct entity *etmp1 = &(go.occupants[0]), *etmp2 = &(go.occupants[1]);

    lev1 = &levl[x][y];
    if (!IS_DRAWBRIDGE(lev1->typ))
        return;
    if (cerberus_blocked()) {
        if (cansee(x, y))
            pline("The drawbridge vibrates vigorously, but doesn't budge.");
        return;
    }
    x2 = x;
    y2 = y;
    get_wall_for_db(&x2, &y2);
    lev2 = &levl[x2][y2];
    if ((lev1->drawbridgemask & DB_UNDER) == DB_MOAT
        || (lev1->drawbridgemask & DB_UNDER) == DB_LAVA) {
        struct obj *otmp2;
        boolean lava = (lev1->drawbridgemask & DB_UNDER) == DB_LAVA;

        Soundeffect(se_loud_splash, 100);  /* Deaf-aware */
        if (lev1->typ == DRAWBRIDGE_UP) {
            if (cansee(x2, y2) || u_at(x2, y2))
                pline_The("portcullis of the drawbridge falls into the %s!",
                          lava ? hliquid("lava") : "moat");
            else
                You_hear("a loud *SPLASH*!");  /* Deaf-aware */
        } else {
            if (cansee(x, y) || u_at(x, y))
                pline_The("drawbridge collapses into the %s!",
                          lava ? hliquid("lava") : "moat");
            else
                You_hear("a loud *SPLASH*!");  /* Deaf-aware */
        }
        lev1->typ = lava ? LAVAPOOL : MOAT;
        lev1->drawbridgemask = 0;
        if ((otmp2 = sobj_at(BOULDER, x, y)) != 0) {
            obj_extract_self(otmp2);
            (void) flooreffects(otmp2, x, y, "fall");
        }
    } else {
        /* no moat beneath */
        Soundeffect(se_loud_crash, 100);  /* Deaf-aware */
        if (cansee(x, y) || u_at(x, y))
            pline_The("drawbridge disintegrates!");
        else
            You_hear("a loud *CRASH*!");  /* Deaf-aware */
        lev1->typ = ((lev1->drawbridgemask & DB_ICE) ? ICE : ROOM);
        lev1->icedpool = ((lev1->drawbridgemask & DB_ICE) ? ICED_MOAT : 0);
    }
    wake_nearto(x, y, 500);
    lev2->typ = DOOR;
    set_doorstate(lev2, D_NODOOR);
    if ((t = t_at(x, y)) != 0)
        deltrap(t);
    if ((t = t_at(x2, y2)) != 0)
        deltrap(t);
    del_engr_at(x, y);
    del_engr_at(x2, y2);
    for (i = rn2(6); i > 0; --i) { /* scatter some debris */
        /* doesn't matter if we happen to pick <x,y2> or <x2,y>;
           since drawbridges are never placed diagonally, those
           pairings will always match one of <x,y> or <x2,y2> */
        otmp = mksobj_at(IRON_CHAIN, rn2(2) ? x : x2, rn2(2) ? y : y2, TRUE,
                         FALSE);
        /* a force of 5 here would yield a radius of 2 for
           iron chain; anything less produces a radius of 1 */
        (void) scatter(otmp->ox, otmp->oy, 1, MAY_HIT, otmp);
    }
    newsym(x, y);
    newsym(x2, y2);
    if (!does_block(x2, y2, lev2))
        unblock_point(x2, y2); /* vision */
    vision_recalc(0);
    if (Is_stronghold(&u.uz))
        u.uevent.uopened_dbridge = TRUE;

    set_entity(x2, y2, etmp2); /* currently only automissers can be here */
    if (etmp2->edata) {
        e_inview = e_canseemon(etmp2);
        if (!automiss(etmp2)) {
            if (e_inview)
                pline("%s blown apart by flying debris.",
                      E_phrase(etmp2, "are"));
            svk.killer.format = KILLED_BY_AN;
            Strcpy(svk.killer.name, "exploding drawbridge");
            e_died(etmp2,
                   XKILL_NOCORPSE | (e_inview ? XKILL_GIVEMSG : XKILL_NOMSG),
                   CRUSHING); /*no corpse*/
        } /* nothing which is vulnerable can survive this */
    }
    set_entity(x, y, etmp1);
    if (etmp1->edata) {
        e_inview = e_canseemon(etmp1);
        if (e_missed(etmp1, TRUE)) {
            debugpline1("%s spared!", E_phrase(etmp1, "are"));
            /* if there is water or lava here, fall in now */
            if (is_u(etmp1))
                spoteffects(FALSE);
            else
                (void) minliquid(etmp1->emon);
        } else {
            if (e_inview) {
                if (!is_u(etmp1) && Hallucination)
                    pline("%s into some heavy metal!",
                          E_phrase(etmp1, "get"));
                else
                    pline("%s hit by a huge chunk of metal!",
                          E_phrase(etmp1, "are"));
            } else {
                if (!Deaf && !is_u(etmp1) && !is_pool(x, y)) {
                    Soundeffect(se_crushing_sound, 75);
                    You_hear("a crushing sound.");
                } else {
                    debugpline1("%s from shrapnel", E_phrase(etmp1, "die"));
                }
            }
            svk.killer.format = KILLED_BY_AN;
            Strcpy(svk.killer.name, "collapsing drawbridge");
            e_died(etmp1,
                   XKILL_NOCORPSE | (e_inview ? XKILL_GIVEMSG : XKILL_NOMSG),
                   CRUSHING); /*no corpse*/
            if (levl[etmp1->ex][etmp1->ey].typ == MOAT)
                do_entity(etmp1);
        }
    }
    nokiller();
    if (Is_stronghold(&u.uz))
        u.uevent.uheard_tune = 3; /* bridge is gone so tune is now useless */
}

/* The drawbridge at the Gate of Hell will not open while Cerberus guards it. */
staticfn boolean
cerberus_blocked(void)
{
    if (Is_hellgate(&u.uz)) {
        const struct monst *mtmp;
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (mtmp->data == &mons[PM_CERBERUS] && !mtmp->mpeaceful
                && !helpless(mtmp)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*dbridge.c*/

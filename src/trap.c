/* NetHack 3.7	trap.c	$NHDT-Date: 1720128169 2024/07/04 21:22:49 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.602 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char *const destroy_strings[][3]; /* from zap.c */

staticfn int dng_bottom(d_level *lev);
staticfn void hole_destination(d_level *);
staticfn boolean keep_saddle_with_steedcorpse(unsigned, struct obj *,
                                            struct obj *);
staticfn boolean mu_maybe_destroy_web(struct monst *, boolean, struct trap *);
staticfn boolean floor_trigger(int);
staticfn boolean check_in_air(struct monst *, unsigned);
staticfn int trapeffect_arrow_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_rocktrap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_sqky_board(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_bear_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_slp_gas_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_rust_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_fire_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_pit(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_hole(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_telep_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_level_telep(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_web(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_statue_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_magic_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_anti_magic(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_poly_trap(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_landmine(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_rolling_boulder_trap(struct monst *, struct trap *,
                                           unsigned);
staticfn int trapeffect_magic_portal(struct monst *, struct trap *, unsigned);
staticfn int trapeffect_vibrating_square(struct monst *, struct trap *,
                                           unsigned);
staticfn int trapeffect_selector(struct monst *, struct trap *, unsigned);
staticfn int choose_trapnote(struct trap *);
staticfn int steedintrap(struct trap *, struct obj *);
staticfn void launch_drop_spot(struct obj *, coordxy, coordxy);
staticfn boolean find_random_launch_coord(struct trap *, coord *);
staticfn int mkroll_launch(struct trap *, coordxy, coordxy, short, long);
staticfn boolean isclearpath(coord *, int, schar, schar);
staticfn boolean m_easy_escape_pit(struct monst *) NONNULLARG1;
staticfn void dofiretrap(struct obj *);
staticfn void domagictrap(void);
staticfn void pot_acid_damage(struct obj *, boolean, boolean);
staticfn boolean emergency_disrobe(boolean *);
staticfn int untrap_prob(struct trap *);
staticfn void move_into_trap(struct trap *);
staticfn int try_disarm(struct trap *, boolean);
staticfn void reward_untrap(struct trap *, struct monst *);
staticfn int disarm_holdingtrap(struct trap *);
staticfn int disarm_landmine(struct trap *);
staticfn int unsqueak_ok(struct obj *);
staticfn int disarm_squeaky_board(struct trap *);
staticfn int disarm_shooting_trap(struct trap *);
staticfn void clear_conjoined_pits(struct trap *);
staticfn boolean adj_nonconjoined_pit(struct trap *);
staticfn int try_lift(struct monst *, struct trap *, int, boolean);
staticfn int help_monster_out(struct monst *, struct trap *);
staticfn void disarm_box(struct obj *, boolean, boolean);
staticfn void untrap_box(struct obj *, boolean, boolean);
#if 0
staticfn void join_adjacent_pits(struct trap *);
#endif
staticfn boolean thitm(int, struct monst *, struct obj *, int,
                                                         boolean) NONNULLARG2;
staticfn void maybe_finish_sokoban(void);

static const char *const a_your[2] = { "a", "your" };
static const char *const A_Your[2] = { "A", "Your" };
static const char tower_of_flame[] = "tower of flame";
static const char *const A_gush_of_water_hits = "A gush of water hits";
static const char *const blindgas[6] = { "humid",   "odorless",
                                         "pungent", "chilling",
                                         "acrid",   "biting" };

/* called when you're hit by fire (dofiretrap,buzz,zapyourself,explode);
   returns TRUE if hit on torso */
boolean
burnarmor(struct monst *victim)
{
    struct obj *item;
    char buf[BUFSZ];
    int mat_idx, oldspe;
    boolean hitting_u;

    if (!victim)
        return 0;
    hitting_u = (victim == &gy.youmonst);

    /* burning damage may dry wet towel */
    item = hitting_u ? carrying(TOWEL) : m_carrying(victim, TOWEL);
    while (item) {
        if (is_wet_towel(item)) { /* True => (item->spe > 0) */
            oldspe = item->spe;
            dry_a_towel(item, rn2(oldspe + 1), TRUE);
            if (item->spe != oldspe)
                break; /* stop once one towel has been affected */
        }
        item = item->nobj;
    }

#define burn_dmg(obj, descr) erode_obj(obj, descr, ERODE_BURN, EF_GREASE)
    while (1) {
        switch (rn2(5)) {
        case 0:
            item = hitting_u ? uarmh : which_armor(victim, W_ARMH);
            if (item) {
                mat_idx = item->material;
                Sprintf(buf, "%s %s", materialnm[mat_idx],
                        helm_simple_name(item));
            }
            if (!burn_dmg(item, item ? buf : "helmet"))
                continue;
            break;
        case 1:
            item = hitting_u ? uarmc : which_armor(victim, W_ARMC);
            if (item) {
                (void) burn_dmg(item, cloak_simple_name(item));
                return TRUE;
            }
            item = hitting_u ? uarm : which_armor(victim, W_ARM);
            if (item) {
                (void) burn_dmg(item, xname(item));
                return TRUE;
            }
            item = hitting_u ? uarmu : which_armor(victim, W_ARMU);
            if (item)
                (void) burn_dmg(item, "shirt");
            return TRUE;
        case 2:
            item = hitting_u ? uarms : which_armor(victim, W_ARMS);
            if (!burn_dmg(item, "shield"))
                continue;
            break;
        case 3:
            item = hitting_u ? uarmg : which_armor(victim, W_ARMG);
            if (!burn_dmg(item, "gloves"))
                continue;
            break;
        case 4:
            item = hitting_u ? uarmf : which_armor(victim, W_ARMF);
            if (!burn_dmg(item, "boots"))
                continue;
            break;
        }
        break; /* Out of while loop */
    }
#undef burn_dmg

    return FALSE;
}

/* Generic erode-item function.
 * "ostr", if non-null, is an alternate string to print instead of the
 *   object's name.
 * "type" is an ERODE_* value for the erosion type
 * "flags" is an or-ed list of EF_* flags
 *
 * Returns an erosion return value (ER_*)
 */
int
erode_obj(
    struct obj *otmp,
    const char *ostr,
    int type,
    int ef_flags)
{
    static NEARDATA const char
        *const action[] = { "smoulder", "rust", "rot", "corrode", "crack" },
        *const msg[] = { "burnt", "rusted", "rotten", "corroded", "cracked" },
        *const bythe[] = { "heat", "oxidation", "decay", "corrosion",
                           "impact" }; /* this could use improvement... */
    boolean vulnerable = FALSE, is_primary = TRUE,
            check_grease = (ef_flags & EF_GREASE) ? TRUE : FALSE,
            print = (ef_flags & EF_VERBOSE) ? TRUE : FALSE,
            crackers = FALSE, /* True: different feedback if otmp destroyed */
            uvictim, vismon, visobj;
    int erosion, cost_type;
    struct monst *victim;

    if (!otmp)
        return ER_NOTHING;

    victim = carried(otmp) ? &gy.youmonst
             : mcarried(otmp) ? otmp->ocarry
               : (struct monst *) 0;
    uvictim = (victim == &gy.youmonst);
    vismon = victim && (victim != &gy.youmonst) && canseemon(victim);
    /* Is gb.bhitpos correct here? Ugh. */
    visobj = (!victim && cansee(gb.bhitpos.x, gb.bhitpos.y)
              && (!is_pool(gb.bhitpos.x, gb.bhitpos.y)
                  || (next2u(gb.bhitpos.x,gb.bhitpos.y) && Underwater)));

    switch (type) {
    case ERODE_BURN:
        if (victim && inventory_resistance_check(victim, AD_FIRE))
            return ER_NOTHING;
        vulnerable = is_flammable(otmp);
        check_grease = FALSE;
        cost_type = COST_BURN;
        break;
    case ERODE_RUST:
        vulnerable = is_rustprone(otmp);
        cost_type = COST_RUST;
        break;
    case ERODE_ROT:
        vulnerable = is_rottable(otmp);
        check_grease = FALSE;
        is_primary = FALSE;
        cost_type = COST_ROT;
        break;
    case ERODE_CORRODE:
        if (victim && inventory_resistance_check(victim, AD_ACID))
            return ER_NOTHING;
        vulnerable = is_corrodeable(otmp);
        is_primary = FALSE;
        cost_type = COST_CORRODE;
        break;
    case ERODE_CRACK: /* crystal armor */
        vulnerable = is_crackable(otmp);
        is_primary = TRUE;
        crackers = TRUE;
        cost_type = COST_CRACK;
        break;
    default:
        impossible("Invalid erosion type in erode_obj");
        return ER_NOTHING;
    }
    erosion = is_primary ? otmp->oeroded : otmp->oeroded2;

    if (!ostr)
        ostr = cxname(otmp);
    /* 'visobj' messages insert "the"; probably ought to switch to the() */
    if (visobj && !(uvictim || vismon) && !strncmpi(ostr, "the ", 4))
        ostr += 4;

    if (check_grease && otmp->greased) {
        grease_protect(otmp, ostr, victim);
        return ER_GREASED;
    } else if (!erosion_matters(otmp)) {
        return ER_NOTHING;
    } else if (!vulnerable || (otmp->oerodeproof && otmp->rknown)) {
        if (flags.verbose && print && (uvictim || vismon))
            pline("%s %s %s not affected by %s.",
                  uvictim ? "Your" : s_suffix(Monnam(victim)),
                  ostr, vtense(ostr, "are"), bythe[type]);
        return ER_NOTHING;
    } else if (otmp->oerodeproof || (otmp->blessed && !rnl(4))) {
        if (flags.verbose && (print || otmp->oerodeproof)
            && (uvictim || vismon || visobj))
            pline("Somehow, %s %s %s not affected by the %s.",
                  uvictim ? "your"
                  : !vismon ? "the" /* visobj */
                    : s_suffix(mon_nam(victim)),
                  ostr, vtense(ostr, "are"), bythe[type]);
        /* We assume here that if the object is protected because it
         * is blessed, it still shows some minor signs of wear, and
         * the hero can distinguish this from an object that is
         * actually proof against damage.
         */
        if (otmp->oerodeproof) {
            otmp->rknown = TRUE;
            if (victim == &gy.youmonst)
                update_inventory();
        }

        return ER_NOTHING;
    } else if (erosion < MAX_ERODE) {
        const char *adverb = (erosion + 1 == MAX_ERODE) ? " completely"
                             : erosion ? " further"
                               : "";

        if (uvictim || vismon || visobj)
            pline("%s %s %s%s!",
                  uvictim ? "Your"
                  : !vismon ? "The" /* visobj */
                    : s_suffix(Monnam(victim)),
                  ostr, vtense(ostr, action[type]), adverb);

        if (ef_flags & EF_PAY)
            costly_alteration(otmp, cost_type);

        if (is_primary)
            otmp->oeroded++;
        else
            otmp->oeroded2++;

        if (victim == &gy.youmonst)
            update_inventory();

        return ER_DAMAGED;
    } else if (ef_flags & EF_DESTROY) {
        otmp->in_use = 1; /* in case of hangup during message w/ --More-- */
        if (uvictim || vismon || visobj) {
            char actbuf[BUFSZ];

            if (!crackers)
                Sprintf(actbuf, "%s away", vtense(ostr, action[type]));
            else
                Sprintf(actbuf, "shatters");
            pline("%s %s %s!",
                  uvictim ? "Your"
                  : !vismon ? "The" /* visobj */
                    : s_suffix(Monnam(victim)),
                  ostr, actbuf);
        }
        if (ef_flags & EF_PAY)
            costly_alteration(otmp, cost_type);

        if (otmp->owornmask) {
            /* unwear otmp before deleting it */
            if (carried(otmp)) {
                /* otmp remains in hero's invent; if we get here because
                   it is being burned up by lava, we don't need to worry
                   about unwearing levitation boots and having that
                   trigger float_down to then fall in again; if such
                   were being worn, they wouldn't be in the lava now */
                remove_worn_item(otmp, TRUE); /* calls Cloak_off(),&c */
            } else if (mcarried(otmp)) {
                /* results in otmp->where==OBJ_FREE; delobj() doesn't care */
                extract_from_minvent(otmp->ocarry, otmp, TRUE, FALSE);
            } else { /* worn but not in hero invent or monster minvent ? */
                impossible(
            "erode_obj(%d): destroying strangely worn item [%d, 0x%08lx: %s]",
                           type,
                           otmp->where, otmp->owornmask, simpleonames(otmp));
                otmp->owornmask = 0L; /* otherwise a second complaint (about
                                       * deleting a worn item) will ensue */
            }
        }
        delobj(otmp);
        return ER_DESTROYED;
    } else {
        if (flags.verbose && print) {
            if (uvictim)
                Your("%s %s completely %s.",
                     ostr, vtense(ostr, Blind ? "feel" : "look"), msg[type]);
            else if (vismon || visobj)
                pline("%s %s %s completely %s.",
                      !vismon ? "The" : s_suffix(Monnam(victim)),
                      ostr, vtense(ostr, "look"), msg[type]);
        }
        return ER_NOTHING;
    }
}

/* Protect an item from erosion with grease. Returns TRUE if the grease
 * wears off.
 */
boolean
grease_protect(
    struct obj *otmp,
    const char *ostr,
    struct monst *victim)
{
    static const char txt[] = "protected by the layer of grease!";
    boolean vismon = victim && (victim != &gy.youmonst) && canseemon(victim);

    if (ostr) {
        if (victim == &gy.youmonst)
            Your("%s %s %s", ostr, vtense(ostr, "are"), txt);
        else if (vismon)
            pline("%s's %s %s %s", Monnam(victim),
                  ostr, vtense(ostr, "are"), txt);
    } else if (victim == &gy.youmonst || vismon) {
        pline("%s %s", Yobjnam2(otmp, "are"), txt);
    }
    if (!rn2(2)) {
        otmp->greased = 0;
        if (carried(otmp)) {
            pline_The("grease dissolves.");
            update_inventory();
        }
        return TRUE;
    }
    return FALSE;
}

/* create a "living" statue at x,y */
void
mk_trap_statue(coordxy x, coordxy y)
{
    struct monst *mtmp;
    struct obj *otmp, *statue;
    struct permonst *mptr;
    int trycount = 10;

    do { /* avoid ultimately hostile co-aligned unicorn */
        mptr = &mons[rndmonnum_adj(3, 6)];
    } while (--trycount > 0 && is_unicorn(mptr)
             && sgn(u.ualign.type) == sgn(mptr->maligntyp));
    statue = mkcorpstat(STATUE, (struct monst *) 0, mptr, x, y,
                        CORPSTAT_NONE);
    /* randomize the material */
    init_obj_material(statue);
    mtmp = makemon(&mons[statue->corpsenm], 0, 0, MM_NOCOUNTBIRTH|MM_NOMSG);
    if (!mtmp)
        return; /* should never happen */
    while (mtmp->minvent) {
        otmp = mtmp->minvent;
        otmp->owornmask = 0;
        obj_extract_self(otmp);
        (void) add_to_container(statue, otmp);
    }
    statue->owt = weight(statue);
    mongone(mtmp);
}

/* find "bottom" level of specified dungeon, stopping at quest locate */
staticfn int
dng_bottom(d_level *lev)
{
    int bottom = dunlevs_in_dungeon(lev);

    /* when in the upper half of the quest, don't fall past the
       middle "quest locate" level if hero hasn't been there yet */
    if (In_quest(lev)) {
        int qlocate_depth = qlocate_level.dlevel;

        /* deepest reached < qlocate implies current < qlocate */
        if (dunlev_reached(lev) < qlocate_depth)
            bottom = qlocate_depth; /* early cut-off */
    } else if (In_hell(lev)) {
        /* if the invocation hasn't been performed yet, the vibrating square
           level is effectively the bottom of Gehennom; the sanctum level is
           out of reach until after the invocation */
        if (!u.uevent.invoked)
            bottom -= 1;
    }
    return bottom;
}

/* destination dlevel for holes or trapdoors */
staticfn void
hole_destination(d_level *dst)
{
    int bottom = dng_bottom(&u.uz);

    dst->dnum = u.uz.dnum;
    dst->dlevel = dunlev(&u.uz);
    while (dst->dlevel < bottom) {
        dst->dlevel++;
        if (rn2(4))
            break;
    }
}

struct trap *
maketrap(coordxy x, coordxy y, int typ)
{
    static union vlaunchinfo zero_vl;
    boolean oldplace, was_ice, clear_flags;
    struct trap *ttmp;
    struct rm *lev = &levl[x][y];
    struct obj *otmp;

    if (typ == TRAPPED_DOOR || typ == TRAPPED_CHEST)
        return (struct trap *) 0;

    if ((ttmp = t_at(x, y)) != 0) {
        if (undestroyable_trap(ttmp->ttyp))
            return (struct trap *) 0;
        oldplace = TRUE;
        if (u.utrap && u_at(x, y)
            && ((u.utraptype == TT_BEARTRAP && typ != BEAR_TRAP)
                || (u.utraptype == TT_WEB && typ != WEB)
                || (u.utraptype == TT_PIT && !is_pit(typ))
                || (u.utraptype == TT_LAVA && !is_lava(x, y))))
            reset_utrap(FALSE);
        /* old <tx,ty> remain valid */
    } else if (!CAN_OVERWRITE_TERRAIN(lev->typ) /* stairs */
               || is_pool_or_lava(x, y)
               || (IS_FURNITURE(lev->typ) && (typ != PIT && typ != HOLE))
               || (lev->typ == DRAWBRIDGE_UP && typ == MAGIC_PORTAL)
               || (IS_AIR(lev->typ) && typ != MAGIC_PORTAL)
               || (typ == LEVEL_TELEP && single_level_branch(&u.uz))) {
        /* no trap on top of furniture (caller usually screens the
           location to inhibit this, but wizard mode wishing doesn't)
           and no level teleporter in branch with only one level */
        return (struct trap *) 0;
    } else {
        oldplace = FALSE;
        ttmp = newtrap();
        (void) memset((genericptr_t) ttmp, 0, sizeof(struct trap));
        ttmp->ntrap = 0;
        ttmp->tx = x;
        ttmp->ty = y;
    }
    /* [re-]initialize all fields except ntrap (handled below) and <tx,ty> */
    ttmp->vl = zero_vl;
    ttmp->launch.x = ttmp->launch.y = -1; /* force error if used before set */
    ttmp->dst.dnum = ttmp->dst.dlevel = -1;
    ttmp->madeby_u = 0;
    ttmp->once = 0;
    ttmp->tseen = unhideable_trap(typ);
    ttmp->ttyp = typ;
    set_trap_ammo(ttmp, NULL);

    switch (typ) {
    case SQKY_BOARD:
        ttmp->tnote = choose_trapnote(ttmp);
        break;
    case ROLLING_BOULDER_TRAP: /* boulder will roll towards trigger */
        (void) mkroll_launch(ttmp, x, y, BOULDER, 1L);
        break;
    case PIT:
    case SPIKED_PIT:
        ttmp->conjoined = 0;
        FALLTHROUGH;
        /*FALLTHRU*/
    case HOLE:
    case TRAPDOOR:
        if (is_hole(typ))
            hole_destination(&(ttmp->dst));
        if (*in_rooms(x, y, SHOPBASE)
            && (is_hole(typ) || IS_DOOR(lev->typ) || IS_WALL(lev->typ)))
            add_damage(x, y, /* schedule repair */
                       ((IS_DOOR(lev->typ) || IS_WALL(lev->typ))
                        && !svc.context.mon_moving) ? SHOP_HOLE_COST : 0L);

        clear_flags = TRUE; /* assume lev->flags needs to be reset */
        /* DRAWBRIDGE_UP passes the IS_ROOM() test so check it first;
           it also needs to retain lev->drawbridgemask */
        if (lev->typ == DRAWBRIDGE_UP) {
            /* bridge is closed and we're putting a hole or pit at the span
               spot; this trap will be deleted if/when the bridge is opened;
               terrain becomes room floor even if it was moat, lava, or ice */
            clear_flags = FALSE; /* keep lev->drawbridgemask */
            was_ice = (lev->drawbridgemask & DB_UNDER) == DB_ICE;
            lev->drawbridgemask &= ~DB_UNDER;
            lev->drawbridgemask |= DB_FLOOR;
            if (was_ice) {
                /* subset of set_levltyp() after changing ice to floor;
                   frozen corpses resume rotting, no more ice to melt away */
                obj_ice_effects(x, y, TRUE);
                spot_stop_timers(x, y, MELT_ICE_AWAY);
            }
        } else if (IS_ROOM(lev->typ)) {
            (void) set_levltyp(x, y, ROOM);

        /*
         * some cases which can happen when digging
         * down while phazing thru solid areas
         */
        } else if (lev->typ == STONE || lev->typ == SCORR) {
            (void) set_levltyp(x, y, CORR);
        } else if (IS_WALL(lev->typ) || lev->typ == SDOOR) {
            (void) set_levltyp(x, y, svl.level.flags.is_maze_lev ? ROOM
                                     : svl.level.flags.is_cavernous_lev ? CORR
                                       : DOOR);
        }
        if (clear_flags)
            lev->flags = 0; /* set_levltyp doesn't take care of this [yet?] */

        unearth_objs(x, y);
        recalc_block_point(x, y);
        break;
    case TELEP_TRAP:
        if (isok(gl.launchplace.x, gl.launchplace.y)) {
            ttmp->teledest.x = gx.xstart + gl.launchplace.x;
            ttmp->teledest.y = gy.ystart + gl.launchplace.y;
            if (ttmp->teledest.x == x && ttmp->teledest.y == y) {
                impossible("making fixed-dest tele trap pointing to itself");
            }
        }
        break;
    case ROCKTRAP:
        otmp = mksobj(ROCK, TRUE, FALSE);
        /* TODO: Scale this with depth. */
        otmp->quan = 5 + rnd(10);
        otmp->owt = weight(otmp);
        set_trap_ammo(ttmp, otmp);
        break;
    case DART_TRAP:
        otmp = mksobj(DART, TRUE, FALSE);
        otmp->quan = 15 + rnd(20);
        otmp->owt = weight(otmp);
        /* darts are poisoned 1/6 of the time on level 7+ */
        otmp->opoisoned = (level_difficulty() > 6 && !rn2(6));
        set_trap_ammo(ttmp, otmp);
        break;
    case ARROW_TRAP:
        otmp = mksobj(ARROW, TRUE, FALSE);
        otmp->quan = 15 + rnd(20);
        otmp->owt = weight(otmp);
        /* arrows are not poisoned */
        otmp->opoisoned = 0;
        set_trap_ammo(ttmp, otmp);
        break;
    case BEAR_TRAP:
        set_trap_ammo(ttmp, mksobj(BEARTRAP, TRUE, FALSE));
        break;
    case LANDMINE:
        set_trap_ammo(ttmp, mksobj(LAND_MINE, TRUE, FALSE));
        break;
    case VIBRATING_SQUARE:
        if (!Invocation_lev(&u.uz)) {
            impossible("creating vibrating square on wrong level");
        }
        else {
            svi.inv_pos.x = x;
            svi.inv_pos.y = y;
        }
        break;
    }

    if (!oldplace) {
        ttmp->ntrap = gf.ftrap;
        gf.ftrap = ttmp;
    } else {
        /* oldplace;
           it shouldn't be possible to override a sokoban pit or hole
           with some other trap, but we'll check just to be safe */
        if (Sokoban)
            maybe_finish_sokoban();
    }

    /* It's possible to create a trap on the same square as a monster which is
     * already hiding under an object (e.g. if the monster and object are
     * created as part of a themeroom, and the trap is just added as ordinary
     * room fill). This would cause it to fail a sanity check since the trap
     * makes it no longer a valid hiding position. Force it to un-hide. */
    maybe_unhide_at(x, y);

    return ttmp;
}

/* Assign obj to be the ammo of trap. Deletes any ammo currently in the trap.
 * obj can be set to NULL to delete the ammo without putting in anything else.
 */
void
set_trap_ammo(struct trap *trap, struct obj *obj)
{
    if (!trap) {
        impossible("set_trap_ammo: null trap!");
        return;
    }
    while (trap->ammo) {
        struct obj* oldobj = trap->ammo;
        extract_nobj(oldobj, &trap->ammo);
        if (oldobj->oartifact) {
            impossible("destroying artifact %d that was ammo of a trap",
                       oldobj->oartifact);
        }
        obfree(oldobj, (struct obj *) 0);
    }
    if (!obj) {
        trap->ammo = (struct obj *) 0;
        return;
    }
    if (obj->where != OBJ_FREE) {
        panic("putting non-free object into trap");
    }
    obj->where = OBJ_INTRAP;
    trap->ammo = obj;
}

/* limit the destination of a hole or trapdoor to the furthest level you
   should be able to fall to */
d_level *
clamp_hole_destination(d_level *dlev)
{
    int bottom = dng_bottom(dlev);

    dlev->dlevel = min(dlev->dlevel, bottom);
    return dlev;
}

void
fall_through(
    boolean td, /* td == TRUE : trap door or hole */
    unsigned ftflags)
{
    d_level dtmp;
    char msgbuf[BUFSZ];
    const char *dont_fall = 0;
    int newlevel;
    struct trap *t = (struct trap *) 0;
    boolean controlled_flight = FALSE;

    /* we'll fall even while levitating in Sokoban; otherwise, if we
       won't fall and won't be told that we aren't falling, give up now */
    if (Blind && Levitation && !Sokoban)
        return;

    newlevel = dunlev(&u.uz); /* current level */
    newlevel++;

    if (td) {
        t = t_at(u.ux, u.uy);
        feeltrap(t);
        if (!Sokoban && !(ftflags & TOOKPLUNGE)) {
            if (t->ttyp == TRAPDOOR)
                pline("A trap door opens up under you!");
            else
                pline("There's a gaping hole under you!");
        }
    } else
        pline_The("%s opens up under you!", surface(u.ux, u.uy));

    if (Sokoban && Can_fall_thru(&u.uz)) {
        ; /* KMH -- You can't escape the Sokoban level traps */
    } else if (Levitation || u.ustuck
             || (!Can_fall_thru(&u.uz) && !levl[u.ux][u.uy].candig)
             || ((Flying || !grounded(gy.youmonst.data)
                  || (ceiling_hider(gy.youmonst.data) && u.uundetected))
                 && !(ftflags & TOOKPLUNGE))
             || (In_main_gehennom(&u.uz) && !u.uevent.invoked
                 && newlevel == dng_bottom(&u.uz))) {
        dont_fall = "don't fall in.";
    } else if (gy.youmonst.data->msize >= MZ_HUGE) {
        dont_fall = "don't fit through.";
    } else if (!next_to_u()) {
        dont_fall = "are jerked back by your pet!";
    }
    if (dont_fall) {
        You1(dont_fall);
        /* hero didn't fall through, but any objects here might */
        impact_drop((struct obj *) 0, u.ux, u.uy, 0);
        if (!td) {
            display_nhwindow(WIN_MESSAGE, FALSE);
            pline_The("opening under you closes up.");
        }
        return;
    }
    if ((Flying || is_clinger(gy.youmonst.data))
        && (ftflags & TOOKPLUNGE) && td && t) {
        if (Flying)
            controlled_flight = TRUE;
        You("%s down %s!",
            Flying ? "swoop" : "deliberately drop",
            (t->ttyp == TRAPDOOR)
                ? "through the trap door"
                : "into the gaping hole");
    }

    if (*u.ushops)
        shopdig(1);
    if (Is_stronghold(&u.uz)) {
        find_hell(&dtmp);
    } else {
        int dist;

        if (t) {
            assign_level(&dtmp, &t->dst);
            /* don't fall beyond the bottom, in case this came from a bones
               file with different dungeon size  */
            (void) clamp_hole_destination(&dtmp);
        } else {
            dtmp.dnum = u.uz.dnum;
            dtmp.dlevel = newlevel;
        }
        dist = depth(&dtmp) - depth(&u.uz);
        if (dist > 1)
            You("%s down a %s%sshaft!",
                controlled_flight ? "fly" : "fall",
                dist > 3 ? "very " : "",
                dist > 2 ? "deep " : "");
    }
    if (!td)
        Sprintf(msgbuf, "The hole in the %s above you closes up.",
                ceiling(u.ux, u.uy));

    schedule_goto(&dtmp, !Flying ? UTOTYPE_FALLING : UTOTYPE_NONE, (char *) 0,
                  !td ? msgbuf : (char *) 0);
}

/*
 * Animate the given statue.  May have been via shatter attempt, trap,
 * or stone to flesh spell.  Return a monster if successfully animated.
 * If the monster is animated, the object is deleted.  If fail_reason
 * is non-null, then fill in the reason for failure (or success).
 *
 * The cause of animation is:
 *
 *      ANIMATE_NORMAL  - hero "finds" the monster
 *      ANIMATE_SHATTER - hero tries to destroy the statue
 *      ANIMATE_SPELL   - stone to flesh spell hits the statue
 *
 * Perhaps x, y is not needed if we can use get_obj_location() to find
 * the statue's location... ???
 *
 * Sequencing matters:
 *      create monster; if it fails, give up with statue intact;
 *      give "statue comes to life" message;
 *      if statue belongs to shop, have shk give "you owe" message;
 *      transfer statue contents to monster (after stolen_value());
 *      delete statue.
 *      [This ordering means that if the statue ends up wearing a cloak of
 *       invisibility or a mummy wrapping, the visibility checks might be
 *       wrong, but to avoid that we'd have to clone the statue contents
 *       first in order to give them to the monster before checking their
 *       shop status--it's not worth the hassle.]
 */
struct monst *
animate_statue(
    struct obj *statue,
    coordxy x,
    coordxy y,
    int cause,
    int *fail_reason)
{
    static const char
        historic_statue_is_gone[] = "that the historic statue is now gone";
    int mnum = statue->corpsenm;
    struct permonst *mptr = &mons[mnum];
    struct monst *mon = 0, *shkp;
    struct obj *item;
    coord cc;
    boolean historic = (Role_if(PM_ARCHEOLOGIST)
                        && (statue->spe & CORPSTAT_HISTORIC) != 0),
            golem_xform = FALSE, use_saved_traits;
    const char *comes_to_life;
    char statuename[BUFSZ], tmpbuf[BUFSZ];

    if (cant_revive(&mnum, TRUE, statue)) {
        /* mnum has changed; we won't be animating this statue as itself */
        if (mnum != PM_DOPPELGANGER)
            mptr = &mons[mnum];
        use_saved_traits = FALSE;
    } else if (is_golem(mptr) && cause == ANIMATE_SPELL) {
        /* statue of any golem hit by stone-to-flesh becomes flesh golem */
        golem_xform = (mptr != &mons[PM_FLESH_GOLEM]);
        mnum = PM_FLESH_GOLEM;
        mptr = &mons[PM_FLESH_GOLEM];
        use_saved_traits = (has_omonst(statue) && !golem_xform);
    } else {
        use_saved_traits = has_omonst(statue);
    }

    if (use_saved_traits) {
        /* restore a petrified monster */
        cc.x = x, cc.y = y;
        mon = montraits(statue, &cc, (cause == ANIMATE_SPELL));
        if (mon && mon->mtame && !mon->isminion)
            wary_dog(mon, TRUE);
    } else {
        int sgend = (statue->spe & CORPSTAT_GENDER);
        mmflags_nht mmflags = (NO_MINVENT | MM_NOMSG
                        | ((sgend == CORPSTAT_MALE) ? MM_MALE : 0)
                        | ((sgend == CORPSTAT_FEMALE) ? MM_FEMALE : 0));

        /* statues of unique monsters from bones or wishing end
           up here (cant_revive() sets mnum to be doppelganger;
           mptr reflects the original form for use by newcham()) */
        if ((mnum == PM_DOPPELGANGER && mptr != &mons[PM_DOPPELGANGER])
            /* block quest guards from other roles */
            || (mptr->msound == MS_GUARDIAN
                && quest_info(MS_GUARDIAN) != mnum)) {
            mmflags |= MM_NOCOUNTBIRTH | MM_ADJACENTOK;
            mon = makemon(&mons[PM_DOPPELGANGER], x, y, mmflags);
            /* if hero has protection from shape changers, cham field will
               be NON_PM; otherwise, set form to match the statue */
            if (mon && ismnum(mon->cham))
                (void) newcham(mon, mptr, NO_NC_FLAGS);
        } else {
            if (cause == ANIMATE_SPELL)
                mmflags |= MM_ADJACENTOK;
            mon = makemon(mptr, x, y, mmflags);
        }
    }

    if (!mon) {
        if (fail_reason)
            *fail_reason = unique_corpstat(&mons[statue->corpsenm])
                               ? AS_MON_IS_UNIQUE
                               : AS_NO_MON;
        return (struct monst *) 0;
    }

    /* if statue has been named, give same name to the monster */
    if (has_oname(statue) && !unique_corpstat(mon->data))
        mon = christen_monst(mon, ONAME(statue));
    /* mimic statue becomes seen mimic; other hiders won't be hidden */
    if (M_AP_TYPE(mon))
        seemimic(mon);
    else
        mon->mundetected = FALSE;
    mon->msleeping = 0;
    if (cause == ANIMATE_NORMAL || cause == ANIMATE_SHATTER) {
        /* trap always releases hostile monster */
        mon->mtame = 0; /* (might be petrified pet tossed onto trap) */
        mon->mpeaceful = 0;
        set_malign(mon);
    }

    comes_to_life = !canspotmon(mon) ? "disappears"
                    : golem_xform ? "turns into flesh"
                      : (nonliving(mon->data) || is_vampshifter(mon))
                        ? "moves"
                        : "comes to life";
    if (u_at(x, y) || cause == ANIMATE_SPELL) {
        /* "the|your|Manlobbi's statue [of a wombat]" */
        shkp = shop_keeper(*in_rooms(mon->mx, mon->my, SHOPBASE));
        Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                (cause == ANIMATE_SPELL
                 /* avoid "of a shopkeeper" if it's Manlobbi himself
                    (if carried, it can't be unpaid--hence won't be
                    described as "Manlobbi's statue"--because there
                    wasn't any living shk when statue was picked up) */
                 && (mon != shkp || carried(statue)))
                   ? xname(statue)
                   : "statue");
        pline("%s %s!", upstart(statuename), comes_to_life);
    } else if (Hallucination) { /* They don't know it's a statue */
        pline_The("%s suddenly seems more animated.", rndmonnam((char *) 0));
    } else if (cause == ANIMATE_SHATTER) {
        if (cansee(x, y))
            Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                    xname(statue));
        else
            Strcpy(statuename, "a statue");
        pline("Instead of shattering, %s suddenly %s!", statuename,
              comes_to_life);
    } else { /* cause == ANIMATE_NORMAL */
        set_msg_xy(x, y);
        You("find %s posing as a statue.",
            canspotmon(mon) ? a_monnam(mon) : something);
        if (!canspotmon(mon) && Blind)
            map_invisible(x, y);
        stop_occupation();
    }

    /* if this isn't caused by a monster using a wand of striking,
       there might be consequences for the hero */
    if (!svc.context.mon_moving) {
        /* if statue is owned by a shop, hero will have to pay for it;
           stolen_value gives a message (about debt or use of credit)
           which refers to "it" so needs to follow a message describing
           the object ("the statue comes to life" one above) */
        if (cause != ANIMATE_NORMAL && costly_spot(x, y)
            && (carried(statue) ? statue->unpaid : !statue->no_charge)
            && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
            /* avoid charging for Manlobbi's statue of Manlobbi
               if stone-to-flesh is used on petrified shopkeep */
            && mon != shkp)
            (void) stolen_value(statue, x, y, (boolean) shkp->mpeaceful,
                                FALSE);

        if (historic) {
            You_feel("guilty %s.", historic_statue_is_gone);
            adjalign(-1);
        }
    } else {
        if (historic && cansee(x, y))
            You_feel("regret %s.", historic_statue_is_gone);
        /* no alignment penalty */
    }

    /* transfer any statue contents to monster's inventory */
    while ((item = statue->cobj) != 0) {
        obj_extract_self(item);
        (void) mpickobj(mon, item);
    }
    m_dowear(mon, TRUE);
    /* in case statue is wielded and hero zaps stone-to-flesh at self */
    if (statue->owornmask)
        remove_worn_item(statue, TRUE);
    /* statue no longer exists */
    delobj(statue);

    /* avoid hiding under nothing */
    if (u_at(x, y) && Upolyd && hides_under(gy.youmonst.data)
        && !concealed_spot(&gy.youmonst, x, y))
        u.uundetected = 0;

    if (fail_reason)
        *fail_reason = AS_OK;
    return mon;
}

/*
 * You've either stepped onto a statue trap's location or you've triggered a
 * statue trap by searching next to it or by trying to break it with a wand
 * or pick-axe.
 */
struct monst *
activate_statue_trap(
    struct trap *trap,
    coordxy x,
    coordxy y,
    boolean shatter)
{
    struct monst *mtmp = (struct monst *) 0;
    struct obj *otmp = sobj_at(STATUE, x, y);
    int fail_reason;

    /*
     * Try to animate the first valid statue.  Stop the loop when we
     * actually create something or the failure cause is not because
     * the mon was unique.
     */
    deltrap(trap);
    while (otmp) {
        mtmp = animate_statue(otmp, x, y,
                              shatter ? ANIMATE_SHATTER : ANIMATE_NORMAL,
                              &fail_reason);
        if (mtmp || fail_reason != AS_MON_IS_UNIQUE)
            break;

        otmp = nxtobj(otmp, STATUE, TRUE);
    }

    feel_newsym(x, y);
    return mtmp;
}

staticfn boolean
keep_saddle_with_steedcorpse(
    unsigned steed_mid,
    struct obj *objchn,
    struct obj *saddle)
{
    if (!saddle)
        return FALSE;
    while (objchn) {
        if (objchn->otyp == CORPSE && has_omonst(objchn)) {
            struct monst *mtmp = OMONST(objchn);

            if (mtmp->m_id == steed_mid) {
                /* move saddle */
                coordxy x, y;
                if (get_obj_location(objchn, &x, &y, 0)) {
                    obj_extract_self(saddle);
                    place_object(saddle, x, y);
                    stackobj(saddle);
                }
                return TRUE;
            }
        }
        if (Has_contents(objchn)
            && keep_saddle_with_steedcorpse(steed_mid, objchn->cobj, saddle))
            return TRUE;
        objchn = objchn->nobj;
    }
    return FALSE;
}

/* monster or you go through and possibly destroy a web.
   return TRUE if could go through. */
staticfn boolean
mu_maybe_destroy_web(
    struct monst *mtmp,
    boolean domsg,
    struct trap *trap)
{
    boolean isyou = (mtmp == &gy.youmonst);
    struct permonst *mptr = mtmp->data;

    if (amorphous(mptr) || is_whirly(mptr) || flaming(mptr)
        || unsolid(mptr) || mptr == &mons[PM_GELATINOUS_CUBE]) {
        coordxy x = trap->tx;
        coordxy y = trap->ty;

        if (flaming(mptr) || acidic(mptr)) {
            if (domsg) {
                if (isyou)
                    You("%s %s spider web!",
                        (flaming(mptr)) ? "burn" : "dissolve",
                        a_your[trap->madeby_u]);
                else
                    pline_mon(mtmp,
                          "%s %s %s spider web!", Monnam(mtmp),
                          (flaming(mptr)) ? "burns" : "dissolves",
                          a_your[trap->madeby_u]);
            }
            deltrap(trap);
            newsym(x, y);
            return TRUE;
        }
        if (domsg) {
            if (isyou) {
                You("flow through %s spider web.", a_your[trap->madeby_u]);
            } else {
                pline_mon(mtmp,
                      "%s flows through %s spider web.", Monnam(mtmp),
                      a_your[trap->madeby_u]);
                seetrap(trap);
            }
        }
        return TRUE;
    }
    return FALSE;
}

void
set_utrap(unsigned int tim, unsigned int typ)
{
    /* if we get here through reset_utrap(), the caller of that might
       have already set u.utrap to 0 so this check won't be sufficient
       in that situation; caller will need to set context.botl itself */
    if (!u.utrap ^ !tim)
        disp.botl = TRUE;

    u.utrap = tim;
    u.utraptype = tim ? typ : TT_NONE;

    float_vs_flight(); /* maybe block Lev and/or Fly */
}

void
reset_utrap(boolean msg)
{
    boolean was_Lev = (Levitation != 0), was_Fly = (Flying != 0);

    set_utrap(0, 0);

    if (msg) {
        if (!was_Lev && Levitation)
            float_up();
        if (!was_Fly && Flying)
            You("can fly.");
    }
}

/* is trap type ttyp triggered by touching the floor? */
staticfn boolean
floor_trigger(int ttyp)
{
    switch (ttyp) {
    case ARROW_TRAP:
    case DART_TRAP:
    case ROCKTRAP:
    case SQKY_BOARD:
    case BEAR_TRAP:
    case LANDMINE:
    case ROLLING_BOULDER_TRAP:
    case SLP_GAS_TRAP:
    case RUST_TRAP:
    case FIRE_TRAP:
    case COLD_TRAP:
    case PIT:
    case SPIKED_PIT:
    case HOLE:
    case TRAPDOOR:
        return TRUE;
    default:
        return FALSE;
    }
}

/* return TRUE if monster mtmp is up in the air, considering trap flags
 * This has nothing to do with air terrain. */
staticfn boolean
check_in_air(struct monst *mtmp, unsigned trflags)
{
    boolean is_you = mtmp == &gy.youmonst,
            plunged = (trflags & (TOOKPLUNGE | VIASITTING)) != 0;

    return ((trflags & HURTLING) != 0
            || (is_you ? Levitation : is_floater(mtmp->data))
            || ((is_you ? Flying : is_flyer(mtmp->data)) && !plunged));
}

/* also used for dart traps - they behave the same */
staticfn int
trapeffect_arrow_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned trflags UNUSED)
{
    struct obj *otmp;

    if (mtmp == &gy.youmonst) {
        if (!trap->ammo) {
            Soundeffect(se_loud_click, 100);
            You_hear("a loud click!");
            deltrap(trap);
            newsym(u.ux, u.uy);
            return Trap_Is_Gone;
        }
        otmp = trap->ammo;
        if (trap->ammo->quan > 1) {
            otmp = splitobj(trap->ammo, 1);
        }
        extract_nobj(otmp, &trap->ammo);
        seetrap(trap);
        pline("%s shoots out at you!", An(xname(otmp)));

        if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) {
            ; /* nothing */
        } else if (thitu(8, dmgval(otmp, &gy.youmonst), &otmp, (const char *) 0)) {
            if (otmp) {
                if (otmp->opoisoned)
                    /* Poison can either deal extra HP damage or attribute loss.
                     * We'd rather do the latter for a trap, so don't count this
                     * missile as a "thrown weapon". */
                    poisoned("dart", A_CON, OBJ_NAME(objects[otmp->otyp]),
                             10, FALSE);
                /* TODO: use hero-missile ammo breakage formula rather than
                 * unconditionally destroying otmp? */
                obfree(otmp, (struct obj *) 0);
            }
        } else {
            place_object(otmp, u.ux, u.uy);
            if (!Blind)
                otmp->dknown = 1;
            stackobj(otmp);
            newsym(u.ux, u.uy);
        }
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean see_it = cansee(mtmp->mx, mtmp->my);
        boolean trapkilled = FALSE;

        if (!trap->ammo) {
            if (in_sight && see_it)
                pline_mon(mtmp,
                      "%s triggers a trap but nothing happens.",
                      Monnam(mtmp));
            deltrap(trap);
            newsym(mtmp->mx, mtmp->my);
            return Trap_Is_Gone;
        }
        otmp = trap->ammo;
        if (trap->ammo->quan > 1) {
            otmp = splitobj(trap->ammo, 1);
        }
        extract_nobj(otmp, &trap->ammo);
        if (in_sight)
            seetrap(trap);
        if (thitm(8, mtmp, otmp, 0, FALSE))
            trapkilled = TRUE;

        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_rocktrap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned trflags UNUSED)
{
    struct obj *otmp;
    boolean harmless = FALSE;

    if (mtmp == &gy.youmonst) {
        if (!trap->ammo) {
            pline("A trap door in %s opens, but nothing falls out!",
                  the(ceiling(u.ux, u.uy)));
            deltrap(trap);
            newsym(u.ux, u.uy);
        } else {
            int dmg = d(2, 6); /* should be std ROCK dmg? */

            otmp = trap->ammo;
            if (trap->ammo->quan > 1) {
                otmp = splitobj(trap->ammo, 1);
            }
            extract_nobj(otmp, &trap->ammo);
            feeltrap(trap);
            place_object(otmp, u.ux, u.uy);

            pline("A trap door in %s opens and %s falls on your %s!",
                  the(ceiling(u.ux, u.uy)), an(xname(otmp)), body_part(HEAD));
            if (uarmh) {
                /* normally passes_rocks() would protect against a falling
                   rock, but not when wearing a helmet */
                if (passes_rocks(gy.youmonst.data)) {
                    pline("Unfortunately, you are wearing %s.",
                          an(helm_simple_name(uarmh))); /* helm or hat */
                    dmg = 2;
                } else if (hard_helmet(uarmh)) {
                    pline("Fortunately, you are wearing a hard helmet.");
                    dmg = 2;
                } else if (flags.verbose) {
                    pline("%s does not protect you.", Yname2(uarmh));
                }
                crack_glass_obj(uarmh);
            } else if (passes_rocks(gy.youmonst.data)) {
                pline("It passes harmlessly through you.");
                harmless = TRUE;
            }
            if (!Blind)
                otmp->dknown = 1;
            stackobj(otmp);
            newsym(u.ux, u.uy); /* map the rock */

            if (!harmless) {
                losehp(Maybe_Half_Phys(dmg), "falling rock", KILLED_BY_AN);
                exercise(A_STR, FALSE);
            }
        }
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean see_it = cansee(mtmp->mx, mtmp->my);
        boolean trapkilled = FALSE;

        if (!trap->ammo) {
            if (in_sight && see_it)
                pline_mon(mtmp,
                      "A trap door above %s opens, but nothing falls out!",
                      mon_nam(mtmp));
            deltrap(trap);
            newsym(mtmp->mx, mtmp->my);
            return Trap_Is_Gone;
        }
        otmp = trap->ammo;
        if (trap->ammo->quan > 1) {
            otmp = splitobj(trap->ammo, 1);
        }
        extract_nobj(otmp, &trap->ammo);
        if (in_sight)
            seetrap(trap);
        if (thitm(0, mtmp, otmp, d(2, 6), FALSE))
            trapkilled = TRUE;

        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_sqky_board(
    struct monst *mtmp,
    struct trap *trap,
    unsigned trflags)
{
    enum sound_effect_entries tsnds[] = {
        se_squeak_C, se_squeak_D_flat, se_squeak_D,
        se_squeak_E_flat, se_squeak_E, se_squeak_F,
        se_squeak_F_sharp, se_squeak_G, se_squeak_G_sharp,
        se_squeak_A, se_squeak_B_flat, se_squeak_B,
    };
    boolean forcetrap = ((trflags & FORCETRAP) != 0
                         || (trflags & FAILEDUNTRAP) != 0
                         || (Flying && (trflags & VIASITTING) != 0));
    const char *board = Hallucination ? "chicken" : "board";
    const char *squeak = Hallucination ? "squawk" : "squeak";

    if (mtmp == &gy.youmonst) {
        if ((Levitation || Flying) && !forcetrap) {
            if (!Blind) {
                seetrap(trap);
                if (Hallucination)
                    You("notice a crease in the linoleum.");
                else
                    You("notice a loose board below you.");
            }
        } else {
            seetrap(trap);
            if (IndexOk(trap->tnote, tsnds)) {
                Soundeffect(tsnds[trap->tnote], 50);
            }
            pline("A %s beneath you %ss %s%s.", board,
                  Deaf ? "vibrate" : squeak,
                  Deaf ? "" : trapnote(trap, FALSE),
                  Deaf ? "" : " loudly");
            wake_nearby(FALSE);
            if (Is_wizpuzzle_lev(&u.uz)) {
                /* squeaky boards are the activation mechanism for the puzzle */
                wizpuzzle_activate_mechanism(u.ux, u.uy);
            }
        }
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);

        if (m_in_air(mtmp))
            return Trap_Effect_Finished;
        /* stepped on a squeaky board */
        if (in_sight) {
            if (!Deaf) {
                if (IndexOk(trap->tnote, tsnds)) {
                    Soundeffect(tsnds[trap->tnote], 50);
                }
                pline_mon(mtmp, "A %s beneath %s %ss %s loudly.",
                          board, mon_nam(mtmp), squeak, trapnote(trap, FALSE));
                seetrap(trap);
            } else if (!mindless(mtmp->data)) {
                pline_mon(mtmp,
                      "%s stops momentarily and appears to cringe.",
                      Monnam(mtmp));
            }
        } else {
            /* same near/far threshold as mzapmsg() */
            int range = couldsee(mtmp->mx, mtmp->my) /* 9 or 5 */
                ? (BOLT_LIM + 1) : (BOLT_LIM - 3);

            if (IndexOk(trap->tnote, tsnds)) {
                Soundeffect(tsnds[trap->tnote],
                             ((mdistu(mtmp) <= range * range)
                                ? 40 : 20));
            }
            You_hear("%s %s %s.", trapnote(trap, FALSE), squeak,
                     (distu(mtmp->mx, mtmp->my) <= range * range)
                     ? "nearby" : "in the distance");
        }
        /* wake up nearby monsters */
        wake_nearto(mtmp->mx, mtmp->my, 40);
        /* wake the hero if nearby */
        if (u.usleep && u.usleep < svm.moves
            && dist2(mtmp->mx, mtmp->my, u.ux, u.uy) < 40
            /* sleeping causes deafness so don't check regular Deaf here, only
             * stricter forms of it */
            && !(EDeaf || u.uroleplay.deaf)) {
            gm.multi = -1;
            gn.nomovemsg = "A squeak awakens you.";
        }
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_bear_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned trflags)
{
    boolean is_you = mtmp == &gy.youmonst,
            forcetrap = ((trflags & FORCETRAP) != 0
                         || (trflags & FAILEDUNTRAP) != 0
                         || (is_you && (trflags & VIASITTING) != 0));

    if (is_you) {
        int dmg = d(2, 4);

        if ((Levitation || Flying) && !forcetrap)
            return Trap_Effect_Finished;
        feeltrap(trap);
        if (amorphous(gy.youmonst.data) || is_whirly(gy.youmonst.data)
            || unsolid(gy.youmonst.data)) {
            pline("%s bear trap closes harmlessly through you.",
                  A_Your[trap->madeby_u]);
            return Trap_Effect_Finished;
        }
        if (!u.usteed && gy.youmonst.data->msize <= MZ_SMALL) {
            pline("%s bear trap closes harmlessly over you.",
                  A_Your[trap->madeby_u]);
            return Trap_Effect_Finished;
        }
        set_utrap((unsigned) rn1(4, 4), TT_BEARTRAP);
        if (u.usteed) {
            pline("%s bear trap closes on %s %s!", A_Your[trap->madeby_u],
                  s_suffix(mon_nam(u.usteed)), mbodypart(u.usteed, FOOT));
            if (thitm(0, u.usteed, (struct obj *) 0, dmg, FALSE))
                reset_utrap(TRUE); /* steed died, hero not trapped */
        } else {
            pline("%s bear trap closes on your %s!", A_Your[trap->madeby_u],
                  body_part(FOOT));
            set_wounded_legs(rn2(2) ? RIGHT_SIDE : LEFT_SIDE, rn1(10, 10));
            if (u.umonnum == PM_OWLBEAR || u.umonnum == PM_BUGBEAR)
                You("howl in anger!");
            losehp(Maybe_Half_Phys(dmg), "bear trap", KILLED_BY_AN);
        }
        exercise(A_DEX, FALSE);
    } else {
        struct permonst *mptr = mtmp->data;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean trapkilled = FALSE;

        if (mptr->msize > MZ_SMALL && !amorphous(mptr) && !m_in_air(mtmp)
            && !is_whirly(mptr) && !unsolid(mptr)) {
            mtmp->mtrapped = 1;
            if (in_sight) {
                pline_mon(mtmp,
                      "%s is caught in %s bear trap!", Monnam(mtmp),
                      a_your[trap->madeby_u]);
                seetrap(trap);
            } else {
                if (mptr == &mons[PM_OWLBEAR]
                    || mptr == &mons[PM_BUGBEAR]) {
                    Soundeffect(se_roar, 100);
                    You_hear("the roaring of an angry bear!");
                }
            }
        } else if (forcetrap) {
            if (in_sight) {
                pline_mon(mtmp,
                      "%s evades %s bear trap!", Monnam(mtmp),
                      a_your[trap->madeby_u]);
                seetrap(trap);
            }
        }
        if (mtmp->mtrapped)
            trapkilled = thitm(0, mtmp, (struct obj *) 0, d(2, 4), FALSE);

        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_slp_gas_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        seetrap(trap);
        if (Sleep_resistance || breathless(gy.youmonst.data)) {
            You("are enveloped in a cloud of gas!");
            monstseesu(M_SEEN_SLEEP);
        } else {
            pline("A cloud of gas puts you to sleep!");
            fall_asleep(-rnd(25), TRUE);
            monstunseesu(M_SEEN_SLEEP);
        }
        (void) steedintrap(trap, (struct obj *) 0);
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);

        if (!resists_sleep(mtmp) && !breathless(mtmp->data)
            && !helpless(mtmp)) {
            if (sleep_monst(mtmp, rnd(25), -1) && in_sight) {
                pline_mon(mtmp,
                          "%s suddenly falls asleep!", Monnam(mtmp));
                seetrap(trap);
            }
        }
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_rust_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    struct obj *otmp, *nextobj;

    if (mtmp == &gy.youmonst) {
        seetrap(trap);

        /* Unlike monsters, traps cannot aim their rust attacks at
         * you, so instead of looping through and taking either the
         * first rustable one or the body, we take whatever we get,
         * even if it is not rustable.
         */
        switch (rn2(5)) {
        case 0:
            pline("%s you on the %s!", A_gush_of_water_hits, body_part(HEAD));
            (void) water_damage(uarmh, helm_simple_name(uarmh), TRUE);
            break;
        case 1:
            pline("%s your left %s!", A_gush_of_water_hits, body_part(ARM));
            if (water_damage(uarms, "shield", TRUE) != ER_NOTHING)
                break;
            if (u.twoweap || (uwep && bimanual(uwep)))
                (void) water_damage(u.twoweap ? uswapwep : uwep, 0, TRUE);
 uglovecheck:
            (void) water_damage(uarmg, gloves_simple_name(uarmg), TRUE);
            break;
        case 2:
            pline("%s your right %s!", A_gush_of_water_hits, body_part(ARM));
            (void) water_damage(uwep, 0, TRUE);
            goto uglovecheck;
        default:
            pline("%s you!", A_gush_of_water_hits);
            /* note: exclude primary and secondary weapons from splashing
               because cases 1 and 2 target them [via water_damage()] */
            for (otmp = gi.invent; otmp; otmp = nextobj) {
                nextobj = otmp->nobj;
                if (otmp->lamplit && otmp != uwep
                    && (otmp != uswapwep || !u.twoweap))
                    (void) splash_lit(otmp);
            }
            if (uarmc)
                (void) water_damage(uarmc, cloak_simple_name(uarmc), TRUE);
            else if (uarm)
                (void) water_damage(uarm, suit_simple_name(uarm), TRUE);
            else if (uarmu)
                (void) water_damage(uarmu, "shirt", TRUE);
        }
        update_inventory();

        if (u.umonnum == PM_IRON_GOLEM) {
            int dam = u.mhmax;

            You("are covered with rust!");
            losehp(Maybe_Half_Phys(dam), "rusting away", KILLED_BY);
        } else if (u.umonnum == PM_GREMLIN && rn2(3)) {
            (void) split_mon(&gy.youmonst, (struct monst *) 0);
        }
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean trapkilled = FALSE;
        struct permonst *mptr = mtmp->data;
        struct obj *target;

        if (in_sight)
            seetrap(trap);
        switch (rn2(5)) {
        case 0:
            if (in_sight)
                pline_mon(mtmp,
                      "%s %s on the %s!", A_gush_of_water_hits,
                      mon_nam(mtmp), mbodypart(mtmp, HEAD));
            target = which_armor(mtmp, W_ARMH);
            (void) water_damage(target, helm_simple_name(target), TRUE);
            break;
        case 1:
            if (in_sight)
                pline_mon(mtmp,
                      "%s %s's left %s!", A_gush_of_water_hits,
                      mon_nam(mtmp), mbodypart(mtmp, ARM));
            target = which_armor(mtmp, W_ARMS);
            if (water_damage(target, "shield", TRUE) != ER_NOTHING)
                break;
            target = MON_WEP(mtmp);
            if (target && bimanual(target))
                (void) water_damage(target, 0, TRUE);
 mglovecheck:
            target = which_armor(mtmp, W_ARMG);
            (void) water_damage(target, gloves_simple_name(target), TRUE);
            break;
        case 2:
            if (in_sight)
                pline_mon(mtmp,
                      "%s %s's right %s!", A_gush_of_water_hits,
                      mon_nam(mtmp), mbodypart(mtmp, ARM));
            (void) water_damage(MON_WEP(mtmp), 0, TRUE);
            goto mglovecheck;
        default:
            if (in_sight)
                pline("%s %s!", A_gush_of_water_hits, mon_nam(mtmp));
            for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                if (otmp->lamplit
                    /* exclude weapon(s) because cases 1 and 2 do them */
                    && (otmp->owornmask & (W_WEP | W_SWAPWEP)) == 0)
                    (void) splash_lit(otmp);
            if ((target = which_armor(mtmp, W_ARMC)) != 0)
                (void) water_damage(target, cloak_simple_name(target),
                                    TRUE);
            else if ((target = which_armor(mtmp, W_ARM)) != 0)
                (void) water_damage(target, suit_simple_name(target),
                                    TRUE);
            else if ((target = which_armor(mtmp, W_ARMU)) != 0)
                (void) water_damage(target, "shirt", TRUE);
        }

        if (completelyrusts(mptr)) {
            if (in_sight)
                pline_mon(mtmp, "%s %s to pieces!", Monnam(mtmp),
                      !mlifesaver(mtmp) ? "falls" : "starts to fall");
            monkilled(mtmp, (const char *) 0, AD_RUST);
            if (DEADMONSTER(mtmp))
                trapkilled = TRUE;
        } else if (mptr == &mons[PM_GREMLIN] && rn2(3)) {
            (void) split_mon(mtmp, (struct monst *) 0);
        }

        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_fire_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        seetrap(trap);
        dofiretrap((struct obj *) 0);
    } else {
        coordxy tx = trap->tx, ty = trap->ty;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean see_it = cansee(tx, ty);
        boolean trapkilled = FALSE;
        struct permonst *mptr = mtmp->data;
        int orig_dmg = d(2, 4);

        if (in_sight)
            pline_mon(mtmp,
                 "A %s erupts from the %s under %s!", tower_of_flame,
                  surface(mtmp->mx, mtmp->my), mon_nam(mtmp));
        else if (see_it) { /* evidently `mtmp' is invisible */
            set_msg_xy(mtmp->mx, mtmp->my);
            You_see("a %s erupt from the %s!", tower_of_flame,
                    surface(mtmp->mx, mtmp->my));
        }
        if (resists_fire(mtmp)) {
            if (in_sight) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("%s is uninjured.", Monnam(mtmp));
            }
        } else {
            int num = orig_dmg, alt;
            boolean immolate = FALSE;

            /* paper burns very fast, assume straw is tightly packed
               and burns a bit slower
               (note: this is inconsistent with mattackm()'s AD_FIRE
               damage where completelyburns() includes straw golem) */
            switch (monsndx(mptr)) {
            case PM_PAPER_GOLEM:
                immolate = TRUE;
                alt = mtmp->mhpmax;
                break;
            case PM_STRAW_GOLEM:
                alt = mtmp->mhpmax / 2;
                break;
            case PM_WOOD_GOLEM:
                alt = mtmp->mhpmax / 4;
                break;
            case PM_LEATHER_GOLEM:
                alt = mtmp->mhpmax / 8;
                break;
            default:
                alt = 0;
                break;
            }
            if (alt > num)
                num = alt;

            if (thitm(0, mtmp, (struct obj *) 0, num, immolate))
                trapkilled = TRUE;
            else {
                mtmp->mhpmax -= rn2(num + 1);
                if (mtmp->mhp > mtmp->mhpmax)
                    mtmp->mhp = mtmp->mhpmax;
            }
        }
        if (burnarmor(mtmp) || rn2(3)) {
            int xtradmg = destroy_items(mtmp, AD_FIRE, orig_dmg);
            ignite_items(mtmp->minvent);
            if (!DEADMONSTER(mtmp)) {
                mtmp->mhp -= xtradmg;
                if (DEADMONSTER(mtmp)) { /* NOW it's dead */
                    monkilled(mtmp, "", AD_FIRE);
                    trapkilled = TRUE;
                }
            }
        }
        if (burn_floor_objects(tx, ty, see_it, FALSE)
            && !see_it && distu(tx, ty) <= 3 * 3)
            You("smell smoke.");
        if (is_ice(tx, ty))
            melt_ice(tx, ty, (char *) 0);
        if (DEADMONSTER(mtmp))
            trapkilled = TRUE;
        if (see_it && t_at(tx, ty))
            seetrap(t_at(tx, ty));

        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

/* Remove any intrinsic cold resistance from mtmp, which can be either the
 * player or a monster. Does not affect extrinsic cold resistance or resistance
 * inherent to a monster's form.
 * Return true if it was removed, false if mtmp did not have cold resistance. */
boolean
strip_cold_resistance(struct monst *mtmp)
{
    if (mtmp == &gy.youmonst) {
        if (HCold_resistance) {
            HCold_resistance = 0;
            You_feel("alarmingly cooler.");
            return TRUE;
        }
    }
    else {
        if (mtmp->mintrinsics & MR_COLD) {
            mtmp->mintrinsics &= ~MR_COLD;
            return TRUE;
        }
    }
    return FALSE;
}

static int
trapeffect_cold_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    /* things fire traps do that cold traps don't:
     * - freeze potions (zap_over_floor doesn't either)
     * - change terrain (assume no cold traps generate over water or lava; it
     *   also won't turn regular floor to ice (which could then turn to water)
     * - extra damage or instakilling to certain monsters (it would be possible
     *   to deal e.g. half max HP damage to water elementals and such, but that
     *   might raise more questions than it answers) */
    int dmg = d(4, 8);
    boolean lost_resistance = FALSE;
    if (mtmp == &gy.youmonst) {
        /* This is similar to dofiretrap(), but it doesn't need to be its own
         * function because it's not called from magic traps or container traps.
         */
        seetrap(trap);
        pline("Mist flash-freezes around you as your heat is sucked away!");
        if (Cold_resistance) {
            dmg = rn2(2);
            if (!rn2(3))
                lost_resistance = strip_cold_resistance(&gy.youmonst);
        }
        else {
            /* this is copied from fire trap code and may indicate we should
             * refactor hpmax gains/losses into its own function... */
            int uhpmin = minuhpmax(1), olduhpmax = u.uhpmax;

            if (u.uhpmax > uhpmin) {
                u.uhpmax -= rn2(min(u.uhpmax, dmg + 1));
                disp.botl = TRUE;
            } /* note: no 'else' here */
            if (u.uhpmax < uhpmin) {
                setuhpmax(min(olduhpmax, uhpmin), FALSE); /* sets disp.botl */
                if (!Drain_resistance)
                    losexp(NULL); /* never fatal when 'drainer' is Null */
            }
            if (u.uhp > u.uhpmax) {
                u.uhp = u.uhpmax;
                disp.botl = TRUE;
            }
        }
        if (!dmg) {
            if (!lost_resistance)
                You("are uninjured.");
        }
        else
            losehp(dmg, "flash freeze", KILLED_BY_AN); /* cold damage */
        if (rn2(3))
            (void) destroy_items(&gy.youmonst, AD_COLD, dmg);
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean see_it = cansee(mtmp->mx, mtmp->my);
        boolean trapkilled = FALSE;

        if (in_sight)
            pline("%s is caught in a cloud of flash-freezing mist!",
                  Monnam(mtmp));
        else if (see_it) /* evidently `mtmp' is invisible */
            pline("A cloud of mist condenses and flash-freezes!");

        if (resists_cold(mtmp)) {
            if (!rn2(3))
                lost_resistance = strip_cold_resistance(mtmp);
            if (in_sight) {
                shieldeff(mtmp->mx, mtmp->my);
                if (lost_resistance)
                    pline("%s momentarily %s.", Monnam(mtmp),
                          makeplural(locomotion(mtmp->data, "stumble")));
                else
                    pline("%s is uninjured.", Monnam(mtmp));
            }
        } else {
            if (thitm(0, mtmp, (struct obj *) 0, dmg, FALSE))
                trapkilled = TRUE;
            else {
                mtmp->mhpmax -= rn2(dmg + 1);
                if (mtmp->mhp > mtmp->mhpmax)
                    mtmp->mhp = mtmp->mhpmax;
            }
        }
        if (rn2(3)) {
            int xtradmg = destroy_items(mtmp, AD_COLD, dmg);
            if (!DEADMONSTER(mtmp)) {
                mtmp->mhp -= xtradmg;
                if (DEADMONSTER(mtmp)) { /* NOW it's dead */
                    monkilled(mtmp, "", AD_COLD);
                    trapkilled = TRUE;
                }
            }
        }
        if (see_it && t_at(mtmp->mx, mtmp->my))
            seetrap(trap);

        return trapkilled ? Trap_Killed_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_pit(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    int ttype = trap->ttyp;

    if (mtmp == &gy.youmonst) {
        boolean plunged = (trflags & TOOKPLUNGE) != 0;
        boolean viasitting = (trflags & VIASITTING) != 0;
        boolean conj_pit = conjoined_pits(trap, t_at(u.ux0, u.uy0), TRUE);
        boolean adj_pit = adj_nonconjoined_pit(trap);
        boolean already_known = trap->tseen ? TRUE : FALSE;
        boolean deliberate = FALSE;
        int steed_article = ARTICLE_THE;

        /* suppress article in various steed messages when using its
           name (which won't occur when hallucinating) */
        if (u.usteed && has_mgivenname(u.usteed) && !Hallucination)
            steed_article = ARTICLE_NONE;

        /* KMH -- You can't escape the Sokoban level traps */
        if (!Sokoban && (Levitation || (Flying && !plunged && !viasitting)))
            return Trap_Effect_Finished;
        feeltrap(trap);
        if (!Sokoban && !grounded(gy.youmonst.data) && !plunged) {
            if (already_known) {
                You_see("%s %spit below you.", a_your[trap->madeby_u],
                        ttype == SPIKED_PIT ? "spiked " : "");
            } else {
                pline("%s pit %sopens up under you!", A_Your[trap->madeby_u],
                      ttype == SPIKED_PIT ? "full of spikes " : "");
                You("don't fall in!");
            }
            return Trap_Effect_Finished;
        }
        /* is the pit an "open grave"? (if it's in a graveyard, yes) */
        boolean is_grave = (getroomtype(u.ux, u.uy) == MORGUE &&
                            ttype != SPIKED_PIT);
        if (!Sokoban) {
            char verbbuf[BUFSZ];

            *verbbuf = '\0';
            if (u.usteed) {
                if ((trflags & RECURSIVETRAP) != 0)
                    Sprintf(verbbuf, "and %s fall",
                            x_monnam(u.usteed, steed_article, (char *) 0,
                                     SUPPRESS_SADDLE, FALSE));
                else
                    Sprintf(verbbuf, "lead %s",
                            x_monnam(u.usteed, steed_article, "poor",
                                     SUPPRESS_SADDLE, FALSE));
            } else if (iflags.menu_requested && already_known) {
                You("carefully %s into the pit.",
                    u_locomotion("lower yourself"));
                deliberate = TRUE;
            } else if (conj_pit) {
                You("move into an adjacent pit.");
            } else if (adj_pit) {
                You("stumble over debris%s.",
                    !rn2(5) ? " between the pits" : "");
            } else {
                Strcpy(verbbuf,
                       !plunged ? "fall" : (Flying ? "dive" : "plunge"));
            }
            if (*verbbuf)
                You("%s into %s %s!", verbbuf, a_your[trap->madeby_u],
                    (is_grave ? "open grave" : "pit"));
        }
        /* wumpus reference */
        if (Role_if(PM_RANGER) && !trap->madeby_u && !trap->once
            && In_quest(&u.uz) && Is_qlocate(&u.uz)) {
            pline("Fortunately it has a bottom after all...");
            trap->once = 1;
        } else if (u.umonnum == PM_PIT_VIPER || u.umonnum == PM_PIT_FIEND) {
            pline("How pitiful.  Isn't that the pits?");
        } else if (is_grave) {
            if(is_undead(gy.youmonst.data)) {
                pline("It seems quite cozy down here.");
            }
            else {
                pline("It's a little early for that, isn't it?");
            }
        }
        if (ttype == SPIKED_PIT) {
            const char *predicament = "on a set of sharp iron spikes";

            if (u.usteed) {
                pline("%s %s %s!",
                      upstart(x_monnam(u.usteed, steed_article, "poor",
                                       SUPPRESS_SADDLE, FALSE)),
                      conj_pit ? "steps" : "lands", predicament);
            } else
                You("%s %s!", conj_pit ? "step" : "land", predicament);
        }
        /* FIXME:
         * if hero gets killed here, setting u.utrap in advance will
         * show "you were trapped in a pit" during disclosure's display
         * of enlightenment, but hero is dying *before* becoming trapped.
         */
        set_utrap((unsigned) rn1(6, 2), TT_PIT);
        if (!steedintrap(trap, (struct obj *) 0)) {
            if (ttype == SPIKED_PIT) {
                int dmg = rnd(conj_pit ? 4 : adj_pit ? 6 : 10);
                if (mon_hates_material(mtmp, IRON))
                    dmg += rnd(sear_damage(IRON));
                int oldumort = u.umortality;
                losehp(Maybe_Half_Phys(dmg),
                       /* note: these don't need locomotion() handling;
                          if fatal while poly'd and Unchanging, the
                          death reason will be overridden with
                          "killed while stuck in creature form" */
                       plunged
                       ? "deliberately plunged into a pit of iron spikes"
                       : (conj_pit || deliberate)
                         ? "stepped into a pit of iron spikes"
                         : adj_pit
                           ? "stumbled into a pit of iron spikes"
                           : "fell into a pit of iron spikes",
                       NO_KILLER_PREFIX);
                if (!rn2(6))
                    poisoned("spikes", A_STR,
                             (conj_pit || adj_pit || deliberate)
                             ? "stepping on poison spikes"
                             : "fall onto poison spikes",
                             /* if damage triggered life-saving,
                                poison is limited to attrib loss */
                             (u.umortality > oldumort) ? 0 : 8, FALSE);
            } else {
                /* plunging flyers take spike damage but not pit damage */
                if (!conj_pit && !deliberate
                    && !(plunged && (Flying || is_clinger(gy.youmonst.data))))
                    losehp(Maybe_Half_Phys(rnd(adj_pit ? 3 : 6)),
                           plunged ? "deliberately plunged into a pit"
                           : "fell into a pit",
                           NO_KILLER_PREFIX);
            }
            if (Punished && !carried(uball)) {
                unplacebc();
                ballfall();
                placebc();
            }
            if (!conj_pit)
                selftouch("Falling, you");
            gv.vision_full_recalc = 1; /* vision limits change */
            exercise(A_STR, FALSE);
            exercise(A_DEX, FALSE);
        }
    } else {
        int dmg;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean trapkilled = FALSE;
        boolean forcetrap = ((trflags & FORCETRAP) != 0);
        boolean inescapable = (forcetrap || (Sokoban && !trap->madeby_u));
        struct permonst *mptr = mtmp->data;
        const char *fallverb;

        fallverb = "falls";
        if (!grounded(mptr) || (mtmp->wormno && count_wsegs(mtmp) > 5)) {
            if (forcetrap && !Sokoban) {
                /* openfallingtrap; not inescapable here */
                if (in_sight) {
                    seetrap(trap);
                    pline_mon(mtmp,
                             "%s doesn't fall into the pit.", Monnam(mtmp));
                }
                return Trap_Effect_Finished;
            }
            if (!inescapable)
                return Trap_Effect_Finished; /* avoids trap */
            fallverb = "is dragged"; /* sokoban pit */
        }
        if (!passes_walls(mptr))
            mtmp->mtrapped = 1;
        if (in_sight) {
            pline_mon(mtmp,
                     "%s %s into %s pit!", Monnam(mtmp), fallverb,
                     a_your[trap->madeby_u]);
            if (mptr == &mons[PM_PIT_VIPER]
                || mptr == &mons[PM_PIT_FIEND])
                pline("How pitiful.  Isn't that the pits?");
            seetrap(trap);
        }
        mselftouch(mtmp, "Falling, ", FALSE);
        dmg = rnd((ttype == PIT) ? 6 : 10);
        if (ttype == SPIKED_PIT && mon_hates_material(mtmp, IRON))
            dmg += rnd(sear_damage(IRON));
        if (DEADMONSTER(mtmp) || thitm(0, mtmp, (struct obj *) 0,
                                       dmg, FALSE))
            trapkilled = TRUE;

        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_hole(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        if (!Can_fall_thru(&u.uz)) {
            seetrap(trap); /* normally done in fall_through */
            impossible("dotrap: %ss cannot exist on this level.",
                       trapname(trap->ttyp, TRUE));
            return Trap_Effect_Finished; /* don't activate it after all */
        }
        fall_through(TRUE, (trflags & TOOKPLUNGE));
    } else {
        int tt = trap->ttyp;
        struct permonst *mptr = mtmp->data;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean forcetrap = ((trflags & FORCETRAP) != 0);
        boolean inescapable = (forcetrap || (Sokoban && !trap->madeby_u));

        if (!Can_fall_thru(&u.uz)) {
            impossible("mintrap: %ss cannot exist on this level.",
                       trapname(tt, TRUE));
            return Trap_Effect_Finished; /* don't activate it after all */
        }
        if (!grounded(mptr) || (mtmp->wormno && count_wsegs(mtmp) > 5)
            || mptr->msize >= MZ_HUGE) {
            if (forcetrap && !Sokoban) {
                /* openfallingtrap; not inescapable here */
                if (in_sight) {
                    seetrap(trap);
                    if (tt == TRAPDOOR)
                        pline_mon(mtmp,
                            "A trap door opens, but %s doesn't fall through.",
                              mon_nam(mtmp));
                    else /* (tt == HOLE) */
                        pline_mon(mtmp,
                                 "%s doesn't fall through the hole.",
                                 Monnam(mtmp));
                }
                return Trap_Effect_Finished; /* inescapable = FALSE; */
            }
            if (inescapable) { /* sokoban hole */
                if (in_sight) {
                    pline_mon(mtmp,
                             "%s seems to be yanked down!", Monnam(mtmp));
                    seetrap(trap);
                }
            } else
                return Trap_Effect_Finished;
        }
        return trapeffect_level_telep(mtmp, trap, trflags);
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_telep_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        seetrap(trap);
        tele_trap(trap);
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);

        mtele_trap(mtmp, trap, in_sight);
        return Trap_Moved_Mon;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_level_telep(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        seetrap(trap);
        level_tele_trap(trap, trflags);
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean forcetrap = ((trflags & FORCETRAP) != 0);

        return mlevel_tele_trap(mtmp, trap, forcetrap, in_sight);
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_web(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        boolean webmsgok = (trflags & NOWEBMSG) == 0;
        boolean forcetrap = ((trflags & FORCETRAP) != 0
                             || (trflags & FAILEDUNTRAP) != 0);
        boolean viasitting = (trflags & VIASITTING) != 0;
        int steed_article = ARTICLE_THE;

        /* suppress article in various steed messages when using its
           name (which won't occur when hallucinating) */
        if (u.usteed && has_mgivenname(u.usteed) && !Hallucination)
            steed_article = ARTICLE_NONE;

        feeltrap(trap);
        if (mu_maybe_destroy_web(&gy.youmonst, webmsgok, trap))
            return Trap_Effect_Finished;
        if (webmaker(gy.youmonst.data)) {
            if (webmsgok)
                pline(trap->madeby_u ? "You take a walk on your web."
                      : "There is a spider web here.");
            return Trap_Effect_Finished;
        }
        if (webmsgok) {
            char verbbuf[BUFSZ];

            if (forcetrap || viasitting || svc.context.nopick) {
                Strcpy(verbbuf, "are caught by");
            } else if (u.usteed) {
                Sprintf(verbbuf, "lead %s into",
                        x_monnam(u.usteed, steed_article, "poor",
                                 SUPPRESS_SADDLE, FALSE));
            } else {
                Sprintf(verbbuf, "%s into", u_locomotion("stumble"));
            }
            You("%s %s spider web!", verbbuf, a_your[trap->madeby_u]);
        }

        /* time will be adjusted below */
        set_utrap(1, TT_WEB);

        /* Time stuck in the web depends on your/steed strength. */
        {
            int tim, str = ACURR(A_STR);

            /* If mounted, the steed gets trapped.  Use mintrap
             * to do all the work.  If mtrapped is set as a result,
             * unset it and set utrap instead.  In the case of a
             * strongmonst and mintrap said it's trapped, use a
             * short but non-zero trap time.  Otherwise, monsters
             * have no specific strength, so use player strength.
             * This gets skipped for webmsgok, which implies that
             * the steed isn't a factor.
             */
            if (u.usteed && webmsgok) {
                /* mtmp location might not be up to date */
                u.usteed->mx = u.ux;
                u.usteed->my = u.uy;

                /* mintrap currently does not return Trap_Killed_Mon
                   (mon died) for webs */
                if (mintrap(u.usteed, trflags) != Trap_Effect_Finished) {
                    u.usteed->mtrapped = 0;
                    if (strongmonst(u.usteed->data))
                        str = 17;
                } else {
                    reset_utrap(FALSE);
                    return Trap_Effect_Finished;
                }

                webmsgok = FALSE; /* mintrap printed the messages */
            }
            if (str <= 3)
                tim = rn1(6, 6);
            else if (str < 6)
                tim = rn1(6, 4);
            else if (str < 9)
                tim = rn1(4, 4);
            else if (str < 12)
                tim = rn1(4, 2);
            else if (str < 15)
                tim = rn1(2, 2);
            else if (str < 18)
                tim = rnd(2);
            else if (str < 69)
                tim = 1;
            else {
                tim = 0;
                if (webmsgok)
                    You("tear through %s web!", a_your[trap->madeby_u]);
                deltrap(trap);
                newsym(u.ux, u.uy); /* get rid of trap symbol */
            }
            set_utrap((unsigned) tim, TT_WEB);
        }
    } else {
        /* Monster in a web. */
        boolean tear_web;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean forcetrap = ((trflags & FORCETRAP) != 0);
        struct permonst *mptr = mtmp->data;

        if (webmaker(mptr))
            return Trap_Effect_Finished;
        if (mu_maybe_destroy_web(mtmp, in_sight, trap))
            return Trap_Effect_Finished;
        tear_web = FALSE;
        switch (monsndx(mptr)) {
        case PM_OWLBEAR: /* Eric Backus */
        case PM_BUGBEAR:
            if (!in_sight) {
                Soundeffect(se_roar, 60);
                You_hear("the roaring of a confused bear!");
                mtmp->mtrapped = 1;
                break;
            }
            FALLTHROUGH;
            /*FALLTHRU*/
        default:
            if (mptr->mlet == S_GIANT
                /* exclude baby dragons and relatively short worms */
                || (mptr->mlet == S_DRAGON && extra_nasty(mptr))
                || (mtmp->wormno && count_wsegs(mtmp) > 5)) {
                tear_web = TRUE;
            } else if (in_sight) {
                pline_mon(mtmp,
                      "%s is caught in %s spider web.", Monnam(mtmp),
                      a_your[trap->madeby_u]);
                seetrap(trap);
            }
            mtmp->mtrapped = tear_web ? 0 : 1;
            break;
            /* this list is fairly arbitrary; it deliberately
               excludes wumpus & giant/ettin zombies/mummies */
        case PM_TITANOTHERE:
        case PM_BALUCHITHERIUM:
        case PM_PURPLE_WORM:
        case PM_JABBERWOCK:
        case PM_IRON_GOLEM:
        case PM_BALROG:
        case PM_KRAKEN:
        case PM_MASTODON:
        case PM_ORION:
        case PM_NORN:
        case PM_CYCLOPS:
        case PM_LORD_SURTUR:
            tear_web = TRUE;
            break;
        }
        if (tear_web) {
            if (in_sight)
                pline_mon(mtmp,
                     "%s tears through %s spider web!", Monnam(mtmp),
                      a_your[trap->madeby_u]);
            deltrap(trap);
            newsym(mtmp->mx, mtmp->my);
        } else if (forcetrap && !mtmp->mtrapped) {
            if (in_sight) {
                pline_mon(mtmp,
                      "%s avoids %s spider web!", Monnam(mtmp),
                      a_your[trap->madeby_u]);
                seetrap(trap);
            }
        }
        return mtmp->mtrapped ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_statue_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        (void) activate_statue_trap(trap, u.ux, u.uy, FALSE);
    } else {
        /* monsters don't trigger statue traps */
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_magic_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        seetrap(trap);
        if (!rn2(30)) {
            deltrap(trap);
            newsym(u.ux, u.uy); /* update position */
            You("are caught in a magical explosion!");
            losehp(rnd(10), "magical explosion", KILLED_BY_AN);
            Your("body absorbs some of the magical energy!");
            u.uen = (u.uenmax += 2);
            if (u.uenmax > u.uenpeak)
                u.uenpeak = u.uenmax;
            return Trap_Effect_Finished;
        } else {
            domagictrap();
        }
        (void) steedintrap(trap, (struct obj *) 0);
    } else {
        /* A magic trap.  Monsters usually immune. */
        if (!rn2(21))
            return trapeffect_fire_trap(mtmp, trap, trflags);
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_anti_magic(
    struct monst *mtmp, /* monster, possibly youmonst */
    struct trap *trap,  /* trap->ttyp == ANTI_MAGIC */
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        int drain = (u.uen > 1) ? (rnd(u.uen / 2) + 2) : 4;

        seetrap(trap);
        if (Antimagic) {
            /* toting around anti-magic defenses exacerbates the effects of the
             * trap. If hero ever becomes able to be cancelled, this might be a
             * good effect to switch in rather than just additional drain */
            struct obj *otmp;

            drain += rnd(10); /* just for having Antimagic */
            /* Half_XXX_damage has opposite its usual effect (approx)
               but isn't cumulative if hero has more than one */
            if (Half_physical_damage || Half_spell_damage)
                drain += rnd(4);
            /* give Magicbane wielder dose of own medicine */
            if (u_wield_art(ART_MAGICBANE))
                drain += rnd(4);
            /* having an artifact--other than own quest one--which
               confers magic resistance simply by being carried
               also increases the effect */
            for (otmp = gi.invent; otmp; otmp = otmp->nobj)
                if (otmp->oartifact && !is_quest_artifact(otmp)
                    && defends_when_carried(AD_MAGM, otmp))
                    break;
            if (otmp)
                drain += rnd(4);
        }
        drain_en(drain, TRUE);
        if (Antimagic && u.uenmax >= 1) {
            u.uenmax--;
            u.uen = min(u.uenmax, u.uen);
        }
    } else {
        boolean trapkilled = FALSE;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean see_it = cansee(mtmp->mx, mtmp->my);
        struct permonst *mptr = mtmp->data;

        /* similar to hero's case, more or less */
        if (!resists_magm(mtmp)) { /* lose spell energy */
            if (!mtmp->mcan && (attacktype(mptr, AT_MAGC)
                                || attacktype(mptr, AT_BREA))) {
                mtmp->mspec_used += d(2, 6);
                if (in_sight) {
                    seetrap(trap);
                    pline_mon(mtmp, "%s seems lethargic.",
                              Monnam(mtmp));
                }
            }
        } else { /* take some damage */
            struct obj *otmp;
            int dmgval2 = rnd(4);

            if ((otmp = MON_WEP(mtmp)) != 0
                && is_art(otmp, ART_MAGICBANE))
                dmgval2 += rnd(4);
            for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                if (otmp->oartifact
                    && defends_when_carried(AD_MAGM, otmp))
                    break;
            if (otmp)
                dmgval2 += rnd(4);
            if (passes_walls(mptr))
                dmgval2 = (dmgval2 + 3) / 4;

            if (in_sight)
                seetrap(trap);
            mtmp->mhp -= dmgval2;
            if (DEADMONSTER(mtmp))
                monkilled(mtmp,
                          in_sight
                          ? "compression from an anti-magic field"
                          : (const char *) 0,
                          -AD_MAGM);
            if (DEADMONSTER(mtmp))
                trapkilled = TRUE;
            if (see_it)
                newsym(trap->tx, trap->ty);
        }
        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_poly_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        boolean viasitting = (trflags & VIASITTING) != 0;
        int steed_article = ARTICLE_THE;
        char verbbuf[BUFSZ];

        /* suppress article in various steed messages when using its
           name (which won't occur when hallucinating) */
        if (u.usteed && has_mgivenname(u.usteed) && !Hallucination)
            steed_article = ARTICLE_NONE;

        seetrap(trap);
        if (viasitting)
            Strcpy(verbbuf, "trigger"); /* follows "You sit down." */
        else if (u.usteed)
            Sprintf(verbbuf, "lead %s onto",
                    x_monnam(u.usteed, steed_article, (char *) 0,
                             SUPPRESS_SADDLE, FALSE));
        else
            Sprintf(verbbuf, "%s onto", u_locomotion("step"));
        You("%s a polymorph trap!", verbbuf);
        /* your Antimagic also protects your steed; however, your Unchanging
         * won't */
        if (Antimagic) {
            shieldeff(u.ux, u.uy);
            You_feel("momentarily different.");
            /* Trap did nothing; don't remove it --KAA */
        } else {
            (void) steedintrap(trap, (struct obj *) 0); /* may call deltrap() */
            if (!Unchanging) {
                if (t_at(u.ux, u.uy)) {
                    /* steed didn't call deltrap so must not have been affected */
                    deltrap(trap);      /* delete trap before polymorph */
                    newsym(u.ux, u.uy); /* get rid of trap symbol */
                }
                You_feel("a change coming over you.");
                polyself(POLY_NOFLAGS);
            }
        }
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);

        if (resists_magm(mtmp)) {
            shieldeff_mon(mtmp);
        } else if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
            if (newcham(mtmp, (struct permonst *) 0, NC_SHOW_MSG)) {
                deltrap(trap);
                trap = (struct trap *) 0;
            }
            if (trap && in_sight)
                seetrap(trap);
        }
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_landmine(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        boolean already_seen = trap->tseen;
        boolean forcetrap = ((trflags & FORCETRAP) != 0
                             || (trflags & FAILEDUNTRAP) != 0);
        boolean forcebungle = (trflags & FORCEBUNGLE) != 0;
        unsigned steed_mid = 0;
        struct obj *saddle = 0;

        if ((Levitation || Flying) && !forcetrap) {
            if (!already_seen && rn2(3))
                return Trap_Effect_Finished;
            feeltrap(trap);
            pline("%s %s in a pile of soil below you.",
                  already_seen ? "There is" : "You discover",
                  trap->madeby_u ? "the trigger of your mine" : "a trigger");
            if (already_seen && rn2(3))
                return Trap_Effect_Finished;
            Soundeffect(se_kaablamm_of_mine, 80);
            pline("KAABLAMM!!!  %s %s%s off!",
                  forcebungle ? "Your inept attempt sets"
                  : "The air currents set",
                  already_seen ? a_your[trap->madeby_u] : "",
                  already_seen ? " land mine" : "it");
        } else {
            /* prevent landmine from killing steed, throwing you to
             * the ground, and you being affected again by the same
             * mine because it hasn't been deleted yet
             */
            static boolean recursive_mine = FALSE;

            if (recursive_mine)
                return Trap_Effect_Finished;
            feeltrap(trap);
            pline("KAABLAMM!!!  You triggered %s land mine!",
                  a_your[trap->madeby_u]);
            if (u.usteed)
                steed_mid = u.usteed->m_id;
            recursive_mine = TRUE;
            (void) steedintrap(trap, (struct obj *) 0);
            recursive_mine = FALSE;
            saddle = sobj_at(SADDLE, u.ux, u.uy);
            set_wounded_legs(LEFT_SIDE, rn1(35, 41));
            set_wounded_legs(RIGHT_SIDE, rn1(35, 41));
            exercise(A_DEX, FALSE);
        }
        /* add a pit before calling losehp so bones won't keep the landmine;
           blow_up_landmine() will remove pit afterwards if inappropriate */
        trap->ttyp = PIT;
        trap->madeby_u = FALSE;
        losehp(Maybe_Half_Phys(rnd(16)), "land mine", KILLED_BY_AN);
        blow_up_landmine(trap);
        if (steed_mid && saddle && !u.usteed)
            (void) keep_saddle_with_steedcorpse(steed_mid, fobj, saddle);
        newsym(u.ux, u.uy); /* update trap symbol */
        /* fall recursively into the pit... */
        if ((trap = t_at(u.ux, u.uy)) != 0)
            dotrap(trap, RECURSIVETRAP);
        fill_pit(u.ux, u.uy);
    } else {
        boolean trapkilled = FALSE;
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        coordxy tx = trap->tx, ty = trap->ty;

        /* heavier monsters are more likely to set off a land mine; on the
           other hand, any mon lighter than the trigger weight is immune */
#define MINE_TRIGGER_WT (WT_ELF / 2U)
        if (rn2(mtmp->data->cwt + 1) < (int) MINE_TRIGGER_WT)
            return Trap_Effect_Finished;
        if (m_in_air(mtmp)) {
            boolean already_seen = trap->tseen;

            if (in_sight && !already_seen) {
                pline_mon(mtmp,
                     "A trigger appears in a pile of soil below %s.",
                      mon_nam(mtmp));
                seetrap(trap);
            }
            if (rn2(3))
                return Trap_Effect_Finished;
            if (in_sight) {
                newsym(mtmp->mx, mtmp->my);
                pline_The("air currents set %s off!",
                          already_seen ? "a land mine" : "it");
            }
        } else if (in_sight) {
            newsym(mtmp->mx, mtmp->my);
            pline_mon(mtmp,
                  "%s%s triggers %s land mine!",
                  !Deaf ? "KAABLAMM!!!  " : "", Monnam(mtmp),
                  a_your[trap->madeby_u]);
        }
        if (!in_sight && !Deaf)
            pline("Kaablamm!  %s an explosion in the distance!",
                  "You hear");  /* Deaf-aware */
        blow_up_landmine(trap);
        /* explosion might have destroyed a drawbridge; don't
           dish out more damage if monster is already dead */
        if (DEADMONSTER(mtmp)
            || thitm(0, mtmp, (struct obj *) 0, rnd(16), FALSE)) {
            trapkilled = TRUE;
        } else {
            /* monsters recursively fall into new pit */
            if (mintrap(mtmp, trflags | FORCETRAP) == Trap_Killed_Mon)
                trapkilled = TRUE;
        }
        /* a boulder may fill the new pit, crushing monster */
        fill_pit(tx, ty); /* thitm may have already destroyed the trap */
        if (DEADMONSTER(mtmp))
            trapkilled = TRUE;
        if (unconscious()) {
            gm.multi = -1;
            gn.nomovemsg = "The explosion awakens you!";
        }
        return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
            ? Trap_Caught_Mon : Trap_Effect_Finished;
    }
    return Trap_Effect_Finished;
}
#undef MINE_TRIGGER_WT

staticfn int
trapeffect_rolling_boulder_trap(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        int style = ROLL | (trap->tseen ? LAUNCH_KNOWN : 0);
        if (Levitation || Flying || !grounded(gy.youmonst.data))
            return 0;

        feeltrap(trap);
        pline("%sYou trigger a rolling boulder trap!",
              !Deaf ? "Click!  " : "");
        if (!launch_obj(BOULDER, trap->launch.x, trap->launch.y,
                        trap->launch2.x, trap->launch2.y, style)) {
            deltrap(trap);
            newsym(u.ux, u.uy); /* get rid of trap symbol */
            pline("Fortunately for you, no boulder was released.");
        }
    } else {
        if (!m_in_air(mtmp)) {
            boolean in_sight = (mtmp == u.usteed
                                || (cansee(mtmp->mx, mtmp->my)
                                    && canspotmon(mtmp)));
            int style = ROLL | (in_sight ? 0 : LAUNCH_UNSEEN);
            boolean trapkilled = FALSE;

            newsym(mtmp->mx, mtmp->my);
            if (in_sight)
                pline_mon(mtmp, "%s%s triggers %s.",
                      !Deaf ? "Click!  " : "", Monnam(mtmp),
                      trap->tseen ? "a rolling boulder trap" : something);
            if (launch_obj(BOULDER, trap->launch.x, trap->launch.y,
                           trap->launch2.x, trap->launch2.y, style)) {
                if (in_sight)
                    trap->tseen = TRUE;
                if (DEADMONSTER(mtmp))
                    trapkilled = TRUE;
            } else {
                deltrap(trap);
                newsym(mtmp->mx, mtmp->my);
            }
            return trapkilled ? Trap_Killed_Mon : mtmp->mtrapped
                ? Trap_Caught_Mon : Trap_Effect_Finished;
        }
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_magic_portal(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    if (mtmp == &gy.youmonst) {
        feeltrap(trap);
        domagicportal(trap);
    } else {
        return trapeffect_level_telep(mtmp, trap, trflags);
    }
    return Trap_Effect_Finished;
}

staticfn int
trapeffect_vibrating_square(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags UNUSED)
{
    if (mtmp == &gy.youmonst) {
        feeltrap(trap);
        /* messages handled elsewhere; the trap symbol is merely to mark the
           square for future reference */
    } else {
        boolean in_sight = canseemon(mtmp) || (mtmp == u.usteed);
        boolean see_it = cansee(mtmp->mx, mtmp->my);

        if (see_it && !Blind) {
            seetrap(trap); /* before messages */
            if (in_sight) {
                char buf[BUFSZ], *p, *monnm = mon_nam(mtmp);

                if (nolimbs(mtmp->data) || m_in_air(mtmp)) {
                    /* just "beneath <mon>" */
                    Strcpy(buf, monnm);
                } else {
                    Strcpy(buf, s_suffix(monnm));
                    p = eos(strcat(buf, " "));
                    Strcpy(p, makeplural(mbodypart(mtmp, FOOT)));
                    /* avoid "beneath 'rear paws'" or 'rear hooves' */
                    (void) strsubst(p, "rear ", "");
                }
                You_see("a strange vibration beneath %s.", buf);
            } else {
                /* notice something (hearing uses a larger threshold
                   for 'nearby') */
                You_see("the ground vibrate %s.",
                        (mdistu(mtmp) <= 2 * 2)
                           ? "nearby" : "in the distance");
            }
        }
    }
    return Trap_Effect_Finished;
}

/*
 * for PR#259 - paranoid_confirm:trap
 *
 * Will a monster suffer any adverse effects from a certain trap?
 * Note: does NOT mean "will a monster trigger a trap in the first place",
 * though if it won't that does imply that they'll not suffer adverse effects.
 * For example, an elf is considered immune to sleeping gas traps even though
 * they'll set the trap off.
 * Return value:
 *  TRAP_NOT_IMMUNE = not immune at the moment;
 *  TRAP_CLEARLY_IMMUNE = obviously immune (if player is polymorphed, assume
 *    they know which traps they are immune to in their current form);
 *  TRAP_HIDDEN_IMMUNE = immune but in non-obvious way such as an unidentified
 *    item or hidden intrinsic providing a resistance; the player should still
 *    be warned of this trap, while monsters implicitly know they're immune.
 */
int
immune_to_trap(struct monst *mon, unsigned ttype)
{
    struct permonst *pm;
    struct obj *obj;
    boolean is_you;

    if (!mon) {
        impossible("immune_to_trap: null monster");
        return TRAP_NOT_IMMUNE;
    }
    pm = mon->data;
    is_you = (mon == &gy.youmonst);

    switch (ttype) {
    case ARROW_TRAP:
    case DART_TRAP:
    case ROCKTRAP:
        /* can hit anything; even noncorporeal monsters might get a blessed
           projectile */
        return TRAP_NOT_IMMUNE;
    case BEAR_TRAP:
        if (pm->msize <= MZ_SMALL
            || amorphous(pm) || is_whirly(pm) || unsolid(pm))
            return TRAP_CLEARLY_IMMUNE;
        FALLTHROUGH;
        /*FALLTHRU*/
    case SQKY_BOARD:
    case LANDMINE:
    case ROLLING_BOULDER_TRAP:
    case HOLE:
    case TRAPDOOR:
    case PIT:
    case SPIKED_PIT:
        /* ground-based traps, which can be evaded by levitation, flying, or
           hanging to the ceiling */
        if (Sokoban && (is_pit(ttype) || is_hole(ttype)))
            return TRAP_NOT_IMMUNE;
        if (!grounded(pm))
            return TRAP_CLEARLY_IMMUNE;
        else if (is_you && (Levitation || Flying))
            return TRAP_CLEARLY_IMMUNE;
        return TRAP_NOT_IMMUNE;
    case SLP_GAS_TRAP:
        if (breathless(pm))
            return TRAP_CLEARLY_IMMUNE;
        else if (!is_you && resists_sleep(mon))
            return TRAP_CLEARLY_IMMUNE;
        else if (is_you && Sleep_resistance)
            return TRAP_HIDDEN_IMMUNE;
        return TRAP_NOT_IMMUNE;
    case LEVEL_TELEP:
    case TELEP_TRAP:
        /* consider unintended teleporting to be an adverse effect; if in
           the endgame or carrying the Amulet, the teleport trap won't work
           anyway, so anything hitting it is immune. */
        if (In_endgame(&u.uz) || mon_has_amulet(mon))
            return TRAP_CLEARLY_IMMUNE;
        return TRAP_NOT_IMMUNE;
    case POLY_TRAP:
        if (resists_magm(mon))
            /* covers Antimagic for player */
            return (is_you ? TRAP_HIDDEN_IMMUNE : TRAP_CLEARLY_IMMUNE);
        return TRAP_NOT_IMMUNE;
    case STATUE_TRAP:
        /* no effect on monsters, only affects players; only trap detection
           can let player know that this is a statue trap there ahead of time;
           in the rare case this happens, do consider it an adverse effect */
        if (!is_you)
            return TRAP_CLEARLY_IMMUNE;
        return TRAP_NOT_IMMUNE;
    case WEB:
        /* most of this code is lifted from mu_maybe_destroy_web */
        if (webmaker(pm) || amorphous(pm) || is_whirly(pm) || flaming(pm)
            || unsolid(pm) || pm == &mons[PM_GELATINOUS_CUBE])
            return TRAP_CLEARLY_IMMUNE;
        return TRAP_NOT_IMMUNE;
    case ANTI_MAGIC:
        /* doesn't hurt any non-magic-resistant monster with no magic */
        if (is_you) {
            if (Antimagic)
                return TRAP_NOT_IMMUNE;
            else if (u.uenmax == 0)
                /* player won't lose HP and can't lose more Pw */
                return TRAP_HIDDEN_IMMUNE;

        /* following conditional lifted from mintrap ANTI_MAGIC logic */
        } else if (!resists_magm(mon)
                   && (mon->mcan || (!attacktype(pm, AT_MAGC)
                                     && !attacktype(pm, AT_BREA)))) {
            return TRAP_CLEARLY_IMMUNE;
        }
        return TRAP_NOT_IMMUNE;
    case RUST_TRAP:
        /* harmful if wearing anything rustable or if mon is an iron golem */
        if (pm == &mons[PM_IRON_GOLEM])
            return TRAP_NOT_IMMUNE;

        for (obj = is_you ? gi.invent : mon->minvent; obj; obj = obj->nobj) {
            /* rust traps can currently hit only worn armor and weapons */
            if (is_rustprone(obj) && obj->owornmask) {
                if (is_you && (obj == uquiver
                               || (obj == uswapwep && !u.twoweap)))
                    continue;
                return TRAP_NOT_IMMUNE;
            }
        }
        return TRAP_CLEARLY_IMMUNE;
    case MAGIC_TRAP:
        /* for player, any number of bad effects;
           for monsters, only replicates fire trap, so fall through */
        if (is_you)
            return TRAP_NOT_IMMUNE;
        FALLTHROUGH;
        /*FALLTHRU*/
    case FIRE_TRAP: /* can always destroy items being carried */
        /* harmful if not resistant or if carrying anything that could burn */
        if (is_you ? !Fire_resistance : !resists_fire(mon))
            return TRAP_NOT_IMMUNE;

        for (obj = is_you ? gi.invent : mon->minvent; obj; obj = obj->nobj) {
            if (obj->oclass == SCROLL_CLASS || obj->oclass == POTION_CLASS
                || obj->oclass == SPBOOK_CLASS
                || (obj->owornmask && is_flammable(obj))) {
                if ((obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
                    /* mon knows scroll of fire or spellbook of fireball
                       won't be affected; hero knows iff this one has been
                       seen and its type has been discovered */
                    && (!is_you
                        || (obj->dknown && objects[obj->otyp].oc_name_known)))
                    continue;
                return TRAP_NOT_IMMUNE;
            }
        }
        return (is_you ? TRAP_HIDDEN_IMMUNE : TRAP_CLEARLY_IMMUNE);
    case COLD_TRAP:
        /* always potentially harmful to player */
        if (is_you)
            return TRAP_NOT_IMMUNE;
        /* but normally not harmful to monsters since they can't lose cold
         * resistance; however, they can lose it if it's acquired, so check for
         * that */
        else {
            if (mon->mintrinsics & MR_COLD)
                return TRAP_NOT_IMMUNE;
            return resists_cold(mon) ? TRAP_CLEARLY_IMMUNE : TRAP_NOT_IMMUNE;
        }
    case MAGIC_PORTAL:
        /* never hurts anything, but player is considered non-immune so they
           can be asked about entering it */
        if (!is_you)
            return TRAP_CLEARLY_IMMUNE;
        return TRAP_NOT_IMMUNE;
    case VIBRATING_SQUARE:
        /* no adverse effects */
        return TRAP_CLEARLY_IMMUNE;
    default:
        impossible("immune_to_trap: bad ttype %u", ttype);
        break;
    }
    return TRAP_NOT_IMMUNE;
}

staticfn int
trapeffect_selector(
    struct monst *mtmp,
    struct trap *trap,
    unsigned int trflags)
{
    switch (trap->ttyp) {
    case ARROW_TRAP:
    case DART_TRAP:
        return trapeffect_arrow_trap(mtmp, trap, trflags);
    case ROCKTRAP:
        return trapeffect_rocktrap(mtmp, trap, trflags);
    case SQKY_BOARD:
        return trapeffect_sqky_board(mtmp, trap, trflags);
    case BEAR_TRAP:
        return trapeffect_bear_trap(mtmp, trap, trflags);
    case SLP_GAS_TRAP:
        return trapeffect_slp_gas_trap(mtmp, trap, trflags);
    case RUST_TRAP:
        return trapeffect_rust_trap(mtmp, trap, trflags);
    case FIRE_TRAP:
        return trapeffect_fire_trap(mtmp, trap, trflags);
    case COLD_TRAP:
        return trapeffect_cold_trap(mtmp, trap, trflags);
    case PIT:
    case SPIKED_PIT:
        return trapeffect_pit(mtmp, trap, trflags);
    case HOLE:
    case TRAPDOOR:
        return trapeffect_hole(mtmp, trap, trflags);
    case LEVEL_TELEP:
        return trapeffect_level_telep(mtmp, trap, trflags);
    case MAGIC_PORTAL:
        return trapeffect_magic_portal(mtmp, trap, trflags);
    case TELEP_TRAP:
        return trapeffect_telep_trap(mtmp, trap, trflags);
    case WEB:
        return trapeffect_web(mtmp, trap, trflags);
    case STATUE_TRAP:
        return trapeffect_statue_trap(mtmp, trap, trflags);
    case MAGIC_TRAP:
        return trapeffect_magic_trap(mtmp, trap, trflags);
    case ANTI_MAGIC:
        return trapeffect_anti_magic(mtmp, trap, trflags);
    case LANDMINE:
        return trapeffect_landmine(mtmp, trap, trflags);
    case POLY_TRAP:
        return trapeffect_poly_trap(mtmp, trap, trflags);
    case ROLLING_BOULDER_TRAP:
        return trapeffect_rolling_boulder_trap(mtmp, trap, trflags);
    case VIBRATING_SQUARE:
        return trapeffect_vibrating_square(mtmp, trap, trflags);
    default:
        impossible("%s encountered a strange trap of type %d.",
                   (mtmp == &gy.youmonst) ? "You" : "Some monster",
                   trap->ttyp);
    }
    return Trap_Effect_Finished;
}

void
dotrap(struct trap *trap, unsigned trflags)
{
    int ttype = trap->ttyp;
    boolean already_seen = trap->tseen,
            forcetrap = ((trflags & FORCETRAP) != 0
                         || (trflags & FAILEDUNTRAP) != 0),
            forcebungle = (trflags & FORCEBUNGLE) != 0,
            plunged = (trflags & TOOKPLUNGE) != 0,
            conj_pit = conjoined_pits(trap, t_at(u.ux0, u.uy0), TRUE),
            adj_pit = adj_nonconjoined_pit(trap);

    nomul(0);

    /* Correct conj_pit and adj_pit if the player isn't moving; this function
     * can also be called by a pit fiend hurling you into a pit on its turn,
     * which has nothing to do with moving between pits */
    if (!svc.context.mon_moving) {
        conj_pit = adj_pit = FALSE;
    }

    if (fixed_tele_trap(trap)) {
        trflags |= FORCETRAP;
        forcetrap = TRUE;
    }

    /* KMH -- You can't escape the Sokoban level traps */
    if (Sokoban && (is_pit(ttype) || is_hole(ttype))) {
        /* The "air currents" message is still appropriate -- even when
         * the hero isn't flying or levitating -- because it conveys the
         * reason why the player cannot escape the trap with a dexterity
         * check, clinging to the ceiling, etc.
         */
        pline("Air currents pull you down into %s %s!",
              a_your[trap->madeby_u],
              trapname(ttype, TRUE)); /* do force "pit" while hallucinating */
        /* then proceed to normal trap effect */
    } else if (!forcetrap) {
        if (floor_trigger(ttype) && check_in_air(&gy.youmonst, trflags)) {
            if (already_seen) {
                You("%s over %s %s.", u_locomotion("step"),
                    (ttype == ARROW_TRAP && !trap->madeby_u)
                    ? "an" : a_your[trap->madeby_u],
                    trapname(ttype, FALSE));
            }
            return;
        }
        if (already_seen && !Fumbling && !undestroyable_trap(ttype)
            /* in wiztower puzzles, player wants to trigger the trap, so don't
             * frustrate them by randomly escaping it */
            && !(ttype == SQKY_BOARD && Is_wizpuzzle_lev(&u.uz))
            && !(ttype == TELEP_TRAP && Is_telemaze_lev(&u.uz))
            && ttype != ANTI_MAGIC && !forcebungle && !plunged
            && !conj_pit && !adj_pit
            && (!rn2(5) || (is_pit(ttype)
                            && !grounded(gy.youmonst.data)))) {
                You("escape %s %s.", (ttype == ARROW_TRAP && !trap->madeby_u)
                                     ? "an"
                                     : a_your[trap->madeby_u],
                trapname(ttype, FALSE));
            return;
        }
    }

    if (u.usteed)
        mon_learns_traps(u.usteed, ttype);
    mons_see_trap(trap);

    /*
     * Note:
     *  Most references to trap types here don't use trapname() for
     *  hallucination.  This could be considered to be a bug but doing
     *  that would hide the actual trap situation from the player which
     *  would be somewhat harsh for what's usually a minor impairment.
     */

    (void) trapeffect_selector(&gy.youmonst, trap, trflags);
}

char *
trapnote(struct trap* trap, boolean noprefix)
{
    static const char *const tnnames[] = {
        "C note",  "D flat", "D note",  "E flat",
                         "E note",  "F note", "F sharp", "G note",
        "G sharp", "A note", "B flat",  "B note",
    };
    static char tnbuf[12]; /* result buffer */
    const char *tn;

    tnbuf[0] = '\0';
    tn = tnnames[trap->tnote];
    if (!noprefix)
        (void) just_an(tnbuf, tn);
    return strcat(tnbuf, tn);
}

/* choose a note not used by any trap on current level,
   ignoring ttmp; if all are in use, pick a random one */
staticfn int
choose_trapnote(struct trap *ttmp)
{
    int tavail[12], tpick[12], tcnt = 0, k;
    struct trap *t;

    for (k = 0; k < 12; ++k)
        tavail[k] = tpick[k] = 0;
    for (t = gf.ftrap; t; t = t->ntrap)
        if (t->ttyp == SQKY_BOARD && t != ttmp)
            tavail[t->tnote] = 1;
    /* now populate tpick[] with the available indices */
    for (k = 0; k < 12; ++k)
        if (tavail[k] == 0)
            tpick[tcnt++] = k;
    /* choose an unused note; if all are in use, pick a random one */
    return ((tcnt > 0) ? tpick[rn2(tcnt)] : rn2(12));
}

staticfn int
steedintrap(struct trap *trap, struct obj *otmp)
{
    struct monst *steed = u.usteed;
    int tt;
    boolean trapkilled, steedhit;

    if (!steed || !trap)
        return Trap_Effect_Finished;
    tt = trap->ttyp;
    steed->mx = u.ux;
    steed->my = u.uy;
    trapkilled = steedhit = FALSE;

    switch (tt) {
    case ARROW_TRAP:
    case DART_TRAP:
        if (!otmp) {
            impossible("steed hit by non-existent arrow/dart?");
            return Trap_Effect_Finished;
        }
        trapkilled = thitm(8, steed, otmp, 0, FALSE);
        steedhit = TRUE;
        break;
    case SLP_GAS_TRAP:
        if (!resists_sleep(steed) && !breathless(steed->data)
            && !helpless(steed)) {
            if (sleep_monst(steed, rnd(25), -1))
                /* no in_sight check here; you can feel it even if blind */
                pline("%s suddenly falls asleep!", Monnam(steed));
        }
        steedhit = TRUE;
        break;
    case LANDMINE:
        trapkilled = thitm(0, steed, (struct obj *) 0, rnd(16), FALSE);
        steedhit = TRUE;
        break;
    case PIT:
    case SPIKED_PIT:
        trapkilled = (DEADMONSTER(steed)
                      || thitm(0, steed, (struct obj *) 0,
                               rnd((tt == PIT) ? 6 : 10), FALSE));
        steedhit = TRUE;
        break;
    case POLY_TRAP:
        if (!resists_magm(steed) && !resist(steed, WAND_CLASS, 0, NOTELL)) {
            deltrap(trap);
            newsym(steed->mx, steed->my); /* get rid of trap symbol */
            /* newcham() will probably end up calling poly_steed() */
            (void) newcham(steed, (struct permonst *) 0, NC_SHOW_MSG);
        }
        steedhit = TRUE;
        break;
    default:
        break;
    }

    if (trapkilled) {
        dismount_steed(DISMOUNT_POLY);
        return Trap_Killed_Mon;
    }
    return steedhit ? 1 : 0;
}

/* some actions common to both player and monsters for triggered landmine */
void
blow_up_landmine(struct trap *trap)
{
    coordxy x = trap->tx, y = trap->ty, dbx, dby;
    struct rm *lev = &levl[x][y];
    schar old_typ, typ;

    set_trap_ammo(trap, NULL); /* useup the land mine obj */
    old_typ = lev->typ;
    (void) scatter(x, y, 4,
                   MAY_DESTROY | MAY_HIT | MAY_FRACTURE | VIS_EFFECTS,
                   (struct obj *) 0);
    del_engr_at(x, y);
    wake_nearto(x, y, 400);
    if (IS_DOOR(lev->typ))
        set_doorstate(lev, D_BROKEN);
    /* destroy drawbridge if present */
    if (lev->typ == DRAWBRIDGE_DOWN || is_drawbridge_wall(x, y) >= 0) {
        dbx = x, dby = y;
        /* if under the portcullis, the bridge is adjacent */
        if (find_drawbridge(&dbx, &dby))
            destroy_drawbridge(dbx, dby);
    }
    trap = t_at(x, y); /* expected to be null after destruction */
    /* or could be null if scatter blew up oil which melted ice */
    /* convert landmine into pit */
    if (trap) {
        if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz)) {
            /* no pits here */
            deltrap(trap);
        } else {
            /* fill pit with water, if applicable */
            typ = fillholetyp(x, y, FALSE);
            if (typ != ROOM) {
                lev->typ = typ;
                liquid_flow(x, y, typ, trap,
                            cansee(x, y) ? "The hole fills with %s!"
                                         : (char *) 0);
            } else {
                trap->ttyp = PIT;       /* explosion creates a pit */
                trap->madeby_u = FALSE; /* resulting pit isn't yours */
                seetrap(trap);          /* and it isn't concealed */
            }
        }
    }
    fill_pit(x, y);
    maybe_dunk_boulders(x, y);
    recalc_block_point(x, y);
    spot_checks(x, y, old_typ);
}

staticfn void
launch_drop_spot(struct obj *obj, coordxy x, coordxy y)
{
    if (!obj) {
        gl.launchplace.obj = (struct obj *) 0;
        gl.launchplace.x = 0;
        gl.launchplace.y = 0;
    } else {
        gl.launchplace.obj = obj;
        gl.launchplace.x = x;
        gl.launchplace.y = y;
    }
}

boolean
launch_in_progress(void)
{
    if (gl.launchplace.obj)
        return TRUE;
    return FALSE;
}

void
force_launch_placement(void)
{
    if (gl.launchplace.obj) {
        gl.launchplace.obj->otrapped = 0;
        place_object(gl.launchplace.obj, gl.launchplace.x, gl.launchplace.y);
    }
}

/*
 * Move obj from (x1,y1) to (x2,y2)
 *
 * Return 0 if no object was launched.
 *        1 if an object was launched and placed somewhere.
 *        2 if an object was launched, but used up.
 */
int
launch_obj(
    short otyp,
    coordxy x1, coordxy y1,
    coordxy x2, coordxy y2,
    int style)
{
    struct monst *mtmp;
    struct obj *otmp, *otmp2;
    int dx, dy;
    coordxy x, y;
    struct obj *singleobj;
    boolean used_up = FALSE, otherside = FALSE;
    int dist, tmp, delaycnt = 0;

    otmp = sobj_at(otyp, x1, y1);
    /* Try the other side too, for rolling boulder traps */
    if (!otmp && otyp == BOULDER) {
        otherside = TRUE;
        otmp = sobj_at(otyp, x2, y2);
    }
    if (!otmp)
        return 0;
    if (otherside) { /* swap 'em */
        int tx, ty;

        tx = x1;
        ty = y1;
        x1 = x2;
        y1 = y2;
        x2 = tx;
        y2 = ty;
    }

    if (otmp->quan == 1L) {
        obj_extract_self(otmp);
        maybe_unhide_at(otmp->ox, otmp->oy);
        singleobj = otmp;
        otmp = (struct obj *) 0;
    } else {
        singleobj = splitobj(otmp, 1L);
        obj_extract_self(singleobj);
    }
    newsym(x1, y1);
    /* in case you're using a pick-axe to chop the boulder that's being
       launched (perhaps a monster triggered it), destroy context so that
       next dig attempt never thinks you're resuming previous effort */
    if ((otyp == BOULDER || otyp == STATUE)
        && singleobj->ox == svc.context.digging.pos.x
        && singleobj->oy == svc.context.digging.pos.y)
        (void) memset((genericptr_t) &svc.context.digging, 0,
                      sizeof(struct dig_info));

    dist = distmin(x1, y1, x2, y2);
    x = gb.bhitpos.x = x1;
    y = gb.bhitpos.y = y1;
    dx = sgn(x2 - x1);
    dy = sgn(y2 - y1);
    switch (style) {
    case ROLL | LAUNCH_UNSEEN:
        if (otyp == BOULDER) {
            if (cansee(x1, y1)) {
                You_see("%s start to roll.", an(xname(singleobj)));
            } else if (Hallucination) {
                Soundeffect(se_someone_bowling, 60);
                You_hear("someone bowling.");
            } else {
                Soundeffect(se_rumbling, 60);
                You_hear("rumbling %s.", (distu(x1, y1) <= 4 * 4) ? "nearby"
                                           : "in the distance");
            }
        }
        style &= ~LAUNCH_UNSEEN;
        goto roll;
    case ROLL | LAUNCH_KNOWN:
        /* use otrapped as a flag to ohitmon */
        singleobj->otrapped = 1;
        style &= ~LAUNCH_KNOWN;
        FALLTHROUGH;
    /*FALLTHRU*/
    case ROLL:
 roll:
        delaycnt = 2;
        FALLTHROUGH;
    /*FALLTHRU*/
    default:
        if (!delaycnt)
            delaycnt = 1;
        if (!cansee(x, y))
            curs_on_u();
        tmp_at(DISP_FLASH, obj_to_glyph(singleobj, rn2_on_display_rng));
        tmp_at(x, y);
    }
    /* Mark a spot to place object in bones files to prevent
     * loss of object. Use the starting spot to ensure that
     * a rolling boulder will still launch, which it wouldn't
     * do if left midstream. Unfortunately we can't use the
     * target resting spot, because there are some things/situations
     * that would prevent it from ever getting there (bars), and we
     * can't tell that yet.
     */
    launch_drop_spot(singleobj, x, y);

    /* Set the object in motion */
    while (dist-- > 0 && !used_up) {
        struct trap *t;

        tmp_at(x, y);
        tmp = delaycnt;

        /* dstage@u.washington.edu -- Delay only if hero sees it */
        if (cansee(x, y))
            while (tmp-- > 0)
                nh_delay_output();

        x = (gb.bhitpos.x += dx);
        y = (gb.bhitpos.y += dy);

        if ((mtmp = m_at(x, y)) != 0) {
            if (otyp == BOULDER && throws_rocks(mtmp->data)) {
                if (rn2(3)) {
                    if (cansee(x, y))
                        pline_mon(mtmp, "%s snatches the boulder.",
                                  Monnam(mtmp));
                    singleobj->otrapped = 0;
                    (void) mpickobj(mtmp, singleobj);
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                }
            }
            if (ohitmon(mtmp, singleobj, (style == ROLL) ? -1 : dist,
                        FALSE)) {
                used_up = TRUE;
                launch_drop_spot((struct obj *) 0, 0, 0);
                break;
            }
        } else if (u_at(x, y)) {
            if (gm.multi)
                nomul(0);
            if (thitu(9 + singleobj->spe, dmgval(singleobj, &gy.youmonst),
                      &singleobj, (char *) 0))
                stop_occupation();
        }
        if (style == ROLL) {
            if (otyp == BOULDER) {
                wake_nearto(x, y, 25);
            }
            if (down_gate(x, y) != -1) {
                if (ship_object(singleobj, x, y, FALSE)) {
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                }
            }
            if ((t = t_at(x, y)) != 0
                && otyp == BOULDER) {
                int newlev = 0;
                d_level dest;

                switch (t->ttyp) {
                case LANDMINE:
                    if (rn2(10) > 2) {
                        if (cansee(x, y))
                            set_msg_xy(x, y);
                        pline("KAABLAMM!!!%s",
                              cansee(x, y)
                               ? "  The rolling boulder triggers a land mine."
                               : "");
                        deltrap_with_ammo(t, DELTRAP_DESTROY_AMMO);
                        del_engr_at(x, y);
                        place_object(singleobj, x, y);
                        singleobj->otrapped = 0;
                        fracture_rock(singleobj);
                        (void) scatter(x, y, 4,
                                       MAY_DESTROY | MAY_HIT | MAY_FRACTURE
                                           | VIS_EFFECTS,
                                       (struct obj *) 0);
                        if (cansee(x, y))
                            newsym(x, y);
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                    }
                    break;
                case LEVEL_TELEP:
                    /* 20% chance of picking current level; 100% chance for
                       that if in single-level branch (Knox) or in endgame */
                    newlev = random_teleport_level();
                    /* if trap doesn't work, skip "disappears" message */
                    if (newlev == depth(&u.uz))
                        break;
                    FALLTHROUGH;
                    /*FALLTHRU*/
                case TELEP_TRAP:
                    if (cansee(x, y))
                        pline_xy(x, y,
                                 "Suddenly the rolling boulder disappears!");
                    else if (!Deaf)
                        You_hear("a rumbling stop abruptly.");
                    singleobj->otrapped = 0;
                    if (t->ttyp == TELEP_TRAP) {
                        (void) rloco(singleobj);
                    } else {
                        add_to_migration(singleobj);
                        get_level(&dest, newlev);
                        singleobj->ox = dest.dnum;
                        singleobj->oy = dest.dlevel;
                        singleobj->migrateflags = (long) MIGR_RANDOM;
                    }
                    seetrap(t);
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                case PIT:
                case SPIKED_PIT:
                case HOLE:
                case TRAPDOOR:
                    /* the boulder won't be used up if there is a
                       monster in the trap; stop rolling anyway */
                    x2 = x, y2 = y; /* stops here */
                    if (flooreffects(singleobj, x2, y2, "fall")) {
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                    }
                    dist = -1; /* stop rolling immediately */
                    break;
                default:
                    break;
                }

                if (used_up || dist == -1)
                    break; /* from 'while' loop */
            }
            if (flooreffects(singleobj, x, y, "fall")) {
                used_up = TRUE;
                launch_drop_spot((struct obj *) 0, 0, 0);
                break;
            }
            if (otyp == BOULDER && (otmp2 = sobj_at(BOULDER, x, y)) != 0) {
                const char *bmsg = " as one boulder sets another in motion";
                coordxy fx = x + dx, fy = y + dy;

                if (!isok(fx, fy) || !dist || IS_OBSTRUCTED(levl[fx][fy].typ))
                    bmsg = " as one boulder hits another";

                Soundeffect(se_loud_crash, 80);
                You_hear("a loud crash%s!", cansee(x, y) ? bmsg : "");
                obj_extract_self(otmp2);
                /* pass off the otrapped flag to the next boulder */
                otmp2->otrapped = singleobj->otrapped;
                singleobj->otrapped = 0;
                place_object(singleobj, x, y);
                singleobj = otmp2;
                otmp2 = (struct obj *) 0;
                wake_nearto(x, y, 10 * 10);
            }
        }
        if (otyp == BOULDER && closed_door(x, y)) {
            if (!(door_is_iron(&levl[x][y]) && door_is_locked(&levl[x][y]))) {
                if (cansee(x, y)) {
                    set_msg_xy(x, y);
                    pline_The("boulder crashes through a door.");
                }
                set_doorstate(&levl[x][y],
                              door_is_iron(&levl[x][y]) ? D_ISOPEN : D_BROKEN);
                if (dist)
                    recalc_block_point(x, y);
            }
            else {
                /* boulder hits locked iron door, stop the boulder */
                dist = 0;
            }
        }

        /* if about to hit something, do so now */
        if (dist > 0 && isok(x + dx, y + dy)) {
            coordxy fx = x + dx, fy = y + dy;
            uchar typ = levl[fx][fy].typ;

            if (typ == IRONBARS) {
                x2 = x, y2 = y; /* object stops here */
                if (hits_bars(&singleobj, x2, y2, fx, fy, !rn2(20), 0)) {
                    if (!singleobj) {
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                    }
                    break;
                }
            } else if (IS_STWALL(typ) || IS_TREE(typ)) {
                x2 = x, y2 = y; /* object stops here */
                if (!Deaf)
                    pline("Thump!");
                wake_nearto(x2, y2, 16);
                break;
            }
        }
    } /* while dist > 0 */
    tmp_at(DISP_END, 0);
    launch_drop_spot((struct obj *) 0, 0, 0);
    if (!used_up) {
        singleobj->otrapped = 0;
        place_object(singleobj, x2, y2);
        newsym(x2, y2);
        return 1;
    }
    return 2;
}

void
seetrap(struct trap *trap)
{
    if (!trap->tseen) {
        trap->tseen = 1;
        newsym(trap->tx, trap->ty);
    }
}

/* like seetrap() but overrides vision */
void
feeltrap(struct trap *trap)
{
    trap->tseen = 1;
    map_trap(trap, 1);
    /* in case it's beneath something, redisplay the something */
    newsym(trap->tx, trap->ty);
}

/* try to find a random coordinate where launching a rolling boulder
   could work. return TRUE if found, with coordinate in cc. */
staticfn boolean
find_random_launch_coord(struct trap *ttmp, coord *cc)
{
    int tmp;
    boolean success = FALSE;
    coord bcc = UNDEFINED_VALUES;
    int distance;
    int mindist = 4;
    int trycount = 0;
    coordxy dx, dy;
    coordxy x, y;

    if (!ttmp || !cc)
        return FALSE;

    x = ttmp->tx;
    y = ttmp->ty;

    bcc.x = ttmp->tx + gl.launchplace.x;
    bcc.y = ttmp->ty + gl.launchplace.y;
    if (isok(bcc.x, bcc.y) && linedup(ttmp->tx, ttmp->ty, bcc.x, bcc.y, 1)) {
        cc->x = bcc.x;
        cc->y = bcc.y;
        return TRUE;
    }

    if (ttmp->ttyp == ROLLING_BOULDER_TRAP)
        mindist = 2;
    distance = rn1(5, 4); /* 4..8 away */
    tmp = rn2(N_DIRS); /* randomly pick a direction to try first */
    while (distance >= mindist) {
        dx = xdir[tmp];
        dy = ydir[tmp];
        cc->x = x;
        cc->y = y;
        /* Prevent boulder from being placed on water */
        if (ttmp->ttyp == ROLLING_BOULDER_TRAP
            && is_pool_or_lava(x + distance * dx, y + distance * dy))
            success = FALSE;
        else
            success = isclearpath(cc, distance, dx, dy);
        if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
            boolean success_otherway;

            bcc.x = x;
            bcc.y = y;
            success_otherway = isclearpath(&bcc, distance, -(dx), -(dy));
            if (!success_otherway)
                success = FALSE;
        }
        if (success)
            break;
        if (++tmp > 7)
            tmp = 0;
        if ((++trycount % 8) == 0)
            --distance;
    }
    return success;
}

staticfn int
mkroll_launch(
    struct trap *ttmp,
    coordxy x,
    coordxy y,
    short otyp,
    long ocount)
{
    struct obj *otmp;
    coord cc = UNDEFINED_VALUES;
    boolean success = FALSE;

    success = find_random_launch_coord(ttmp, &cc);

    if (!success) {
        /* create the trap without any ammo, launch pt at trap location */
        cc.x = x;
        cc.y = y;
    } else {
        if (!rn2(20) && ttmp->ttyp == ROLLING_BOULDER_TRAP && otyp == BOULDER
            && !In_quest(&u.uz)) {
            /* somebody had a little accident */
            otmp = mkcorpstat(CORPSE, (struct monst *) 0,
                              &mons[PM_ARCHEOLOGIST], cc.x, cc.y,
                              CORPSTAT_INIT); /* places it */
            otmp = mksobj(FEDORA, TRUE, FALSE);
            if (otmp->spe < 2)
                otmp->spe = 2;
            place_object(otmp, cc.x, cc.y);
            if (!rn2(3)) {
                otmp = mksobj(BULLWHIP, TRUE, FALSE);
                if (otmp->spe < 2)
                    otmp->spe = 2;
                place_object(otmp, cc.x, cc.y);
            }
        }
        otmp = mksobj(otyp, TRUE, FALSE);
        otmp->quan = ocount;
        otmp->owt = weight(otmp);
        place_object(otmp, cc.x, cc.y);
        stackobj(otmp);
    }
    ttmp->launch.x = cc.x;
    ttmp->launch.y = cc.y;
    if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
        ttmp->launch2.x = x - (cc.x - x);
        ttmp->launch2.y = y - (cc.y - y);
    } else
        ttmp->launch_otyp = otyp;
    newsym(ttmp->launch.x, ttmp->launch.y);
    return 1;
}

staticfn boolean
isclearpath(
    coord *cc,
    int distance,
    schar dx,
    schar dy)
{
    struct trap *t;
    uchar typ;
    coordxy x, y;

    x = cc->x;
    y = cc->y;
    while (distance-- > 0) {
        x += dx;
        y += dy;
        if (!isok(x, y))
            return FALSE;
        typ = levl[x][y].typ;
        if (!ZAP_POS(typ) || closed_door(x, y))
            return FALSE;
        if ((t = t_at(x, y)) != 0
            && (is_pit(t->ttyp) || is_hole(t->ttyp) || is_xport(t->ttyp)))
            return FALSE;
    }
    cc->x = x;
    cc->y = y;
    return TRUE;
}

/* can monster escape from a pit easily */
staticfn boolean
m_easy_escape_pit(struct monst *mtmp)
{
    return (mtmp->data == &mons[PM_PIT_FIEND]
            || mtmp->data->msize >= MZ_HUGE);
}

int
mintrap(struct monst *mtmp, unsigned mintrapflags)
{
    struct trap *trap = t_at(mtmp->mx, mtmp->my);
    struct permonst *mptr = mtmp->data;
    int trap_result = Trap_Effect_Finished;

    if (!trap) {
        mtmp->mtrapped = 0;      /* perhaps teleported? */
    } else if (mtmp->mtrapped) { /* is currently in the trap */
        if (!trap->tseen && cansee(mtmp->mx, mtmp->my) && canseemon(mtmp)
            && (is_pit(trap->ttyp) || trap->ttyp == BEAR_TRAP
                || trap->ttyp == HOLE
                || trap->ttyp == WEB)) {
            /* If you come upon an obviously trapped monster, then
               you must be able to see the trap it's in too. */
            seetrap(trap);
        }

        if (!rn2(40) || (is_pit(trap->ttyp) && m_easy_escape_pit(mtmp))) {
            if (sobj_at(BOULDER, mtmp->mx, mtmp->my)
                && is_pit(trap->ttyp)) {
                if (!rn2(2)) {
                    mtmp->mtrapped = 0;
                    if (canseemon(mtmp))
                        pline_mon(mtmp, "%s pulls free...",
                                 Monnam(mtmp));
                    fill_pit(mtmp->mx, mtmp->my);
                }
            } else {
                if (canseemon(mtmp)) {
                    set_msg_xy(mtmp->mx, mtmp->my);
                    if (is_pit(trap->ttyp))
                        pline("%s climbs %sout of the pit.", Monnam(mtmp),
                              m_easy_escape_pit(mtmp) ? "easily " : "");
                    else if (trap->ttyp == BEAR_TRAP || trap->ttyp == WEB)
                        pline("%s pulls free of the %s.", Monnam(mtmp),
                              trapname(trap->ttyp, FALSE));
                }
                mtmp->mtrapped = 0;
            }
        } else if (metallivorous(mptr)) {
            if (trap->ttyp == BEAR_TRAP) {
                if (canseemon(mtmp))
                    pline_mon(mtmp, "%s eats a bear trap!",
                              Monnam(mtmp));
                deltrap_with_ammo(trap, DELTRAP_DESTROY_AMMO);
                mtmp->meating = 5;
                mtmp->mtrapped = 0;
            } else if (trap->ttyp == SPIKED_PIT) {
                if (canseemon(mtmp))
                    pline_mon(mtmp, "%s munches on some spikes!",
                              Monnam(mtmp));
                trap->ttyp = PIT;
                mtmp->meating = 5;
            }
        }
        trap_result = mtmp->mtrapped ? Trap_Caught_Mon : Trap_Effect_Finished;
    } else {
        int tt = trap->ttyp;
        boolean forcetrap = ((mintrapflags & FORCETRAP) != 0);
        boolean forcebungle = (mintrapflags & FORCEBUNGLE) != 0;
        /* monster has seen such a trap before */
        boolean already_seen = (mon_knows_traps(mtmp, tt)
                                || (tt == HOLE && !mindless(mptr)));

        if (fixed_tele_trap(trap)) {
            mintrapflags |= FORCETRAP;
            forcetrap = TRUE;
        }

        if (mtmp == u.usteed) {
            ; /* true when called from dotrap, inescapable is not an option */
        } else if (Sokoban && (is_pit(tt) || is_hole(tt))
                   && !trap->madeby_u) {
            ; /* nothing here, the trap effects will handle messaging */
        } else if (!forcetrap) {
            if (floor_trigger(tt) && check_in_air(mtmp, mintrapflags)) {
                return Trap_Effect_Finished;
            }
            if (already_seen && rn2(4) && !forcebungle)
                return Trap_Effect_Finished;
        }

        mon_learns_traps(mtmp, tt);
        mons_see_trap(trap);

        /* Monster is aggravated by being trapped by you.
           Recognizing who made the trap isn't completely
           unreasonable; everybody has their own style. */
        if (trap->madeby_u && rnl(5))
            setmangry(mtmp, FALSE);

        trap_result = trapeffect_selector(mtmp, trap, mintrapflags);

        /* mtmp can't stay hiding under an object if trapped in non-pit
           (mtmp hiding under object at armed bear trap location, hero
           zaps wand of locking or spell of wizard lock at spot triggering
           the trap and trapping mtmp there) */
        if (!DEADMONSTER(mtmp) && mtmp->mtrapped) {
            boolean alreadyspotted = canspotmon(mtmp);

            maybe_unhide_at(mtmp->mx, mtmp->my);
            if (!alreadyspotted && canseemon(mtmp))
                pline_mon(mtmp, "%s appears.", Amonnam(mtmp));
        }
    }
    return trap_result;
}

/* Combine cockatrice checks into single functions to avoid repeating code. */
void
instapetrify(const char *str)
{
    if (Stone_resistance)
        return;

    if (Hallucination) { /* you are already stoned :-) */
        /* This will follow messages such as "Touching a cockatrice corpse is a
         * fatal mistake" that imply the game is ending; contradict that */
        You("feel momentarily inflexible, but then you feel groovy again.");
        return;
    }
    if (poly_when_stoned(gy.youmonst.data)
        && polymon(PM_STONE_GOLEM, POLYMON_ALL_MSGS))
        return;
    urgent_pline("You turn to stone...");
    svk.killer.format = KILLED_BY;
    if (str != svk.killer.name)
        Strcpy(svk.killer.name, str ? str : "");
    done(STONING);
}

void
minstapetrify(struct monst *mon, boolean byplayer)
{
    if (resists_ston(mon))
        return;
    if (poly_when_stoned(mon->data)) {
        mon_to_stone(mon);
        return;
    }
    if (!vamp_stone(mon))
        return;

    /* give a "<mon> is slowing down" message and also remove
       intrinsic speed (comparable to similar effect on the hero) */
    mon_adjust_speed(mon, -3, (struct obj *) 0);

    if (cansee(mon->mx, mon->my))
        pline_mon(mon, "%s turns to stone.", Monnam(mon));
    if (byplayer) {
        gs.stoned = TRUE;
        xkilled(mon, XKILL_NOMSG);
    } else
        monstone(mon);
}

void
selftouch(const char *arg)
{
    char kbuf[BUFSZ];
    const char *corpse_pmname;

    if (uwep && uwep->otyp == CORPSE && touch_petrifies(&mons[uwep->corpsenm])
        && !Stone_resistance) {
        corpse_pmname = obj_pmname(uwep);
        pline("%s touch the %s corpse.", arg, corpse_pmname);
        Sprintf(kbuf, "%s corpse", an(corpse_pmname));
        instapetrify(kbuf);
        /* life-saved; unwield the corpse if we can't handle it */
        if (!uarmg && !Stone_resistance && !Hallucination)
            uwepgone();
    }
    /* Or your secondary weapon, if wielded [hypothetical; we don't
       allow two-weapon combat when either weapon is a corpse] */
    if (u.twoweap && uswapwep && uswapwep->otyp == CORPSE
        && touch_petrifies(&mons[uswapwep->corpsenm]) && !Stone_resistance) {
        corpse_pmname = obj_pmname(uswapwep);
        pline("%s touch the %s corpse.", arg, corpse_pmname);
        Sprintf(kbuf, "%s corpse", an(corpse_pmname));
        instapetrify(kbuf);
        /* life-saved; unwield the corpse */
        if (!uarmg && !Stone_resistance && !Hallucination)
            uswapwepgone();
    }
}

void
mselftouch(
    struct monst *mon,
    const char *arg,
    boolean byplayer)
{
    struct obj *mwep = MON_WEP(mon);

    if (mwep && mwep->otyp == CORPSE && touch_petrifies(&mons[mwep->corpsenm])
        && !resists_ston(mon)) {
        if (cansee(mon->mx, mon->my)) {
            pline_mon(mon, "%s%s touches %s.", arg ? arg : "",
                  arg ? mon_nam(mon) : Monnam(mon),
                  corpse_xname(mwep, (const char *) 0, CXN_PFX_THE));
        }
        minstapetrify(mon, byplayer);
        /* if life-saved, might not be able to continue wielding */
        if (!DEADMONSTER(mon)
            && !which_armor(mon, W_ARMG) && !resists_ston(mon))
            mwepgone(mon);
    }
}

/* start levitating */
void
float_up(void)
{
    disp.botl = TRUE;
    if (u.utrap) {
        if (u.utraptype == TT_PIT) {
            reset_utrap(FALSE);
            You("float up, out of the %s!", trapname(PIT, FALSE));
            gv.vision_full_recalc = 1; /* vision limits change */
            fill_pit(u.ux, u.uy);
        } else if (u.utraptype == TT_LAVA /* molten lava */
                   || u.utraptype == TT_INFLOOR) { /* solidified lava */
            Your("body pulls upward, but your %s are still stuck.",
                 makeplural(body_part(LEG)));
        } else if (u.utraptype == TT_BURIEDBALL) { /* tethered */
            coord cc;

            cc.x = u.ux, cc.y = u.uy;
            /* caveat: this finds the first buried iron ball within
               one step of the specified location, not necessarily the
               buried [former] uball at the original anchor point */
            (void) buried_ball(&cc);
            /* being chained to the floor blocks levitation from floating
               above that floor but not from enhancing carrying capacity */
            You("feel lighter, but your %s is still chained to the %s.",
                body_part(LEG),
                IS_ROOM(levl[cc.x][cc.y].typ) ? "floor" : "ground");
        } else if (u.utraptype == WEB) {
            You("float up slightly, but you are still stuck in the %s.",
                trapname(WEB, FALSE));
        } else { /* bear trap */
            You("float up slightly, but your %s is still stuck.",
                body_part(LEG));
        }
        /* when still trapped, float_vs_flight() below will block levitation */
#if 0
    } else if (Is_waterlevel(&u.uz)) {
        pline("It feels as though you've lost some weight.");
#endif
    } else if (u.uinwater) {
        spoteffects(TRUE);
    } else if (u.uswallow) {
        /* FIXME: this isn't correct for trapper/lurker above */
        if (is_animal(u.ustuck->data))
            You("float away from the %s.", surface(u.ux, u.uy));
        else
            You("spiral up into %s.", mon_nam(u.ustuck));
    } else if (Hallucination) {
        pline("Up, up, and awaaaay!  You're walking on air!");
    } else if (Is_airlevel(&u.uz)) {
        You("gain control over your movements.");
    } else {
        You("start to float in the air!");
    }
    if (u.usteed && !is_floater(u.usteed->data)
        && !is_flyer(u.usteed->data)) {
        if (Lev_at_will) {
            pline("%s magically floats up!", Monnam(u.usteed));
        } else {
            You("cannot stay on %s.", mon_nam(u.usteed));
            dismount_steed(DISMOUNT_GENERIC);
        }
    }
    if (Flying)
        You("are no longer able to control your flight.");
    float_vs_flight(); /* set BFlying, also BLevitation if still trapped */
    /* levitation gives maximum carrying capacity, so encumbrance
       state might be reduced */
    (void) encumber_msg();
    return;
}

/* a boulder fills a pit or a hole at x,y */
void
fill_pit(coordxy x, coordxy y)
{
    struct obj *otmp;
    struct trap *t;

    if ((t = t_at(x, y)) != 0 && (is_pit(t->ttyp) || is_hole(t->ttyp))
        && (otmp = sobj_at(BOULDER, x, y)) != 0) {
        obj_extract_self(otmp);
        (void) flooreffects(otmp, x, y, "settle");
    }
}

/* stop levitating */
int
float_down(
    long hmask,
    long emask) /* might cancel timeout */
{
    struct trap *trap = (struct trap *) 0;
    d_level current_dungeon_level;
    boolean no_msg = FALSE;

    HLevitation &= ~hmask;
    ELevitation &= ~emask;
    if (Levitation)
        return 0; /* maybe another ring/potion/boots */
    if (BLevitation) {
        /* if blocked by terrain, we haven't actually been levitating so
           we don't give any end-of-levitation feedback or side-effects,
           but if blocking is solely due to being trapped in/on floor,
           do give some feedback but skip other float_down() effects */
        boolean trapped = (BLevitation == I_SPECIAL);

        float_vs_flight();
        if (trapped && u.utrap) /* u.utrap => paranoia */
            You("are no longer trying to float up from the %s.",
                (u.utraptype == TT_BEARTRAP) ? "trap's jaws"
                  : (u.utraptype == TT_WEB) ? "web"
                      : (u.utraptype == TT_BURIEDBALL) ? "chain"
                          : (u.utraptype == TT_LAVA) ? "lava"
                              : "ground"); /* TT_INFLOOR */
        (void) encumber_msg(); /* carrying capacity might have changed */
        return 0;
    }
    disp.botl = TRUE;
    nomul(0); /* stop running or resting */
    if (BFlying) {
        /* controlled flight no longer overridden by levitation */
        float_vs_flight(); /* clears BFlying & I_SPECIAL
                            * unless hero is stuck in floor */
        if (Flying) {
            You("have stopped levitating and are now flying.");
            (void) encumber_msg(); /* carrying capacity might have changed */
            return 1;
        }
    }
    if (u.uswallow) {
        You("float down, but you are still %s.",
            digests(u.ustuck->data) ? "swallowed" : "engulfed");
        (void) encumber_msg();
        return 1;
    }

    if (Punished && !carried(uball) && !m_at(uball->ox, uball->oy)
        && (is_pool(uball->ox, uball->oy) || is_open_air(uball->ox, uball->oy)
            || ((trap = t_at(uball->ox, uball->oy))
                && (is_pit(trap->ttyp) || is_hole(trap->ttyp))))) {
        u.ux0 = u.ux;
        u.uy0 = u.uy;
        u.ux = uball->ox;
        u.uy = uball->oy;
        movobj(uchain, uball->ox, uball->oy);
        newsym(u.ux0, u.uy0);
        gv.vision_full_recalc = 1; /* in case the hero moved. */
    }
    /* check for falling into pool - added by GAN 10/20/86 */
    if (!Flying) {
        if (!u.uswallow && u.ustuck) {
            if (sticks(gy.youmonst.data))
                You("aren't able to maintain your hold on %s.",
                    mon_nam(u.ustuck));
            else
                pline("Startled, %s can no longer hold you!",
                      mon_nam(u.ustuck));
            set_ustuck((struct monst *) 0);
        }
        /* kludge alert:
         * drown() and lava_effects() print various messages almost
         * every time they're called which conflict with the "fall
         * into" message below.  Thus, we want to avoid printing
         * confusing, duplicate or out-of-order messages.
         * Use knowledge of the two routines as a hack -- this
         * should really be handled differently -dlc
         */
        if (is_pool(u.ux, u.uy) && !Swimming && !u.uinwater)
            no_msg = drown();

        if (is_lava(u.ux, u.uy) && !iflags.in_lava_effects) {
            (void) lava_effects();
            no_msg = TRUE;
        }

        if (is_open_air(u.ux, u.uy)) {
            u_aireffects();
            no_msg = TRUE;
        }
    }
    if (!trap) {
        trap = t_at(u.ux, u.uy);
        if (Is_airlevel(&u.uz)) {
            You("begin to tumble in place.");
        } else if (Is_waterlevel(&u.uz) && !no_msg) {
            You_feel("heavier.");
        /* u.uinwater msgs already in spoteffects()/drown() */
        } else if (!u.uinwater && !no_msg) {
            if (!(emask & W_SADDLE)) {
                if (Sokoban && trap) {
                    /* Justification elsewhere for Sokoban traps is based
                     * on air currents.  This is consistent with that.
                     * The unexpected additional force of the air currents
                     * once levitation ceases knocks you off your feet.
                     */
                    if (Hallucination)
                        pline("Bummer!  You've crashed.");
                    else
                        You("fall over.");
                    losehp(rnd(2), "dangerous winds", KILLED_BY);
                    if (u.usteed)
                        dismount_steed(DISMOUNT_FELL);
                    selftouch("As you fall, you");
                } else if (u.usteed && (is_floater(u.usteed->data)
                                        || is_flyer(u.usteed->data))) {
                    You("settle more firmly in the saddle.");
                } else if (Hallucination) {
                    pline("Bummer!  You've %s.",
                          is_pool(u.ux, u.uy)
                             ? "splashed down"
                             : "hit the ground");
                } else {
                    You("float gently to the %s.", surface(u.ux, u.uy));
                }
            }
        }
    }

    /* levitation gives maximum carrying capacity, so having it end
       potentially triggers greater encumbrance; do this after
       'come down' messages, before trap activation or autopickup */
    (void) encumber_msg();

    /* can't rely on u.uz0 for detecting trap door-induced level change;
       it gets changed to reflect the new level before we can check it */
    assign_level(&current_dungeon_level, &u.uz);
    if (trap) {
        switch (trap->ttyp) {
        case STATUE_TRAP:
            break;
        case HOLE:
        case TRAPDOOR:
            if (!Can_fall_thru(&u.uz) || u.ustuck)
                break;
            FALLTHROUGH;
            /*FALLTHRU*/
        default:
            if (!u.utrap) /* not already in the trap */
                dotrap(trap, NO_TRAP_FLAGS);
        }
    }
    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !u.uswallow
        /* falling through trap door calls goto_level,
           and goto_level does its own pickup() call */
        && on_level(&u.uz, &current_dungeon_level))
        (void) pickup(1);
    return 1;
}

/* shared code for climbing out of a pit */
void
climb_pit(void)
{
    const char *pitname;

    if (!u.utrap || u.utraptype != TT_PIT)
        return;

    pitname = trapname(PIT, FALSE);
    if (Passes_walls) {
        /* marked as trapped so they can pick things up */
        You("ascend from the %s.", pitname);
        reset_utrap(FALSE);
        fill_pit(u.ux, u.uy);
        gv.vision_full_recalc = 1; /* vision limits change */
    } else if (!rn2(2) && sobj_at(BOULDER, u.ux, u.uy)) {
        Your("%s gets stuck in a crevice.", body_part(LEG));
        display_nhwindow(WIN_MESSAGE, FALSE);
        clear_nhwindow(WIN_MESSAGE);
        You("free your %s.", body_part(LEG));
    } else if ((Flying || is_clinger(gy.youmonst.data)) && !Sokoban) {
        /* eg fell in pit, then poly'd to a flying monster;
           or used '>' to deliberately enter it */
        You("%s from the %s.", u_locomotion("climb"), pitname);
        reset_utrap(FALSE);
        fill_pit(u.ux, u.uy);
        gv.vision_full_recalc = 1; /* vision limits change */
    } else if (!(--u.utrap) || m_easy_escape_pit(&gy.youmonst)) {
        reset_utrap(FALSE);
        You("%s to the edge of the %s.",
            (Sokoban && Levitation)
                ? "struggle against the air currents and float"
                : u.usteed ? "ride" : "crawl",
            pitname);
        fill_pit(u.ux, u.uy);
        gv.vision_full_recalc = 1; /* vision limits change */
    } else if (u.dz || flags.verbose) {
        /* these should use 'pitname' rather than "pit" for hallucination
           but that would nullify Norep (this message can be repeated
           many times without further user intervention by using a run
           attempt to keep retrying to escape from the pit) */
        if (u.usteed)
            Norep("%s is still in a pit.", YMonnam(u.usteed));
        else
            Norep((Hallucination && !rn2(5))
                      ? "You've fallen, and you can't get up."
                      : "You are still in a pit.");
    }
}

staticfn void
dofiretrap(
    struct obj *box) /* null for floor trap */
{
    boolean see_it = !Blind;
    int orig_dmg, num, alt;
    orig_dmg = num = d(2, 4);

    /* Bug: for box case, the equivalent of burn_floor_objects() ought
     * to be done upon its contents.
     */

    if ((box && !carried(box)) ? is_pool(box->ox, box->oy) : Underwater) {
        pline("A cascade of steamy bubbles erupts from %s!",
              the(box ? xname(box) : surface(u.ux, u.uy)));
        if (Fire_resistance)
            You("are uninjured.");
        else
            losehp(rnd(3), "boiling water", KILLED_BY);
        return;
    }
    pline("A %s %s from %s!", tower_of_flame, box ? "bursts" : "erupts",
          the(box ? xname(box) : surface(u.ux, u.uy)));
    if (Fire_resistance) {
        shieldeff(u.ux, u.uy);
        monstseesu(M_SEEN_FIRE);
        num = rn2(2);
    } else if (Upolyd) {
        switch (u.umonnum) {
        case PM_PAPER_GOLEM:
            alt = u.mhmax;
            break;
        case PM_STRAW_GOLEM:
            alt = u.mhmax / 2;
            break;
        case PM_WOOD_GOLEM:
            alt = u.mhmax / 4;
            break;
        case PM_LEATHER_GOLEM:
            alt = u.mhmax / 8;
            break;
        default:
            alt = 0;
            break;
        }
        if (alt > num)
            num = alt;
        if (u.mhmax > mons[u.umonnum].mlevel)
            u.mhmax -= rn2(min(u.mhmax, num + 1)), disp.botl = TRUE;
        if (u.mh > u.mhmax)
            u.mh = u.mhmax, disp.botl = TRUE;
        monstunseesu(M_SEEN_FIRE);
    } else {
        int uhpmin = minuhpmax(1), olduhpmax = u.uhpmax;

        if (u.uhpmax > uhpmin) {
            u.uhpmax -= rn2(min(u.uhpmax, num + 1)), disp.botl = TRUE;
        } /* note: no 'else' here */
        if (u.uhpmax < uhpmin) {
            setuhpmax(min(olduhpmax, uhpmin), FALSE); /* sets disp.botl */
            if (!Drain_resistance)
                losexp(NULL); /* never fatal when 'drainer' is Null */
        }
        if (u.uhp > u.uhpmax)
            u.uhp = u.uhpmax, disp.botl = TRUE;
        monstunseesu(M_SEEN_FIRE);
    }
    if (!num)
        You("are uninjured.");
    else
        losehp(num, tower_of_flame, KILLED_BY_AN); /* fire damage */
    burn_away_slime();

    if (burnarmor(&gy.youmonst) || rn2(3)) {
        (void) destroy_items(&gy.youmonst, AD_FIRE, orig_dmg);
        ignite_items(gi.invent);
    }
    if (!box && burn_floor_objects(u.ux, u.uy, see_it, TRUE) && !see_it)
        You("smell paper burning.");
    if (is_ice(u.ux, u.uy))
        melt_ice(u.ux, u.uy, (char *) 0);
}

staticfn void
domagictrap(void)
{
    int fate = rnd(20);

    /* What happened to the poor sucker? */

    if (fate < 10) {
        /* Most of the time, it creates some monsters. */
        int cnt = rnd(4);

        /* blindness effects */
        if (!resists_blnd(&gy.youmonst)) {
            You("are momentarily blinded by a flash of light!");
            make_blinded((long) rn1(5, 10), FALSE);
            if (!Blind)
                Your1(vision_clears);
        } else if (!Blind) {
            You_see("a flash of light!");
        }

        /* deafness effects */
        if (!Deaf) {
            Soundeffect(se_deafening_roar_atmospheric, 100);
            You_hear("a deafening roar!");
            incr_itimeout(&HDeaf, rn1(20, 30));
            disp.botl = TRUE;
        } else {
            /* magic vibrations still hit you */
            You_feel("rankled.");
            incr_itimeout(&HDeaf, rn1(5, 15));
            disp.botl = TRUE;
        }
        while (cnt--)
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
        /* roar: wake monsters in vicinity, after placing trap-created ones */
        wake_nearto(u.ux, u.uy, 7 * 7);
        /* [flash: should probably also hit nearby gremlins with light] */
    } else {
        switch (fate) {
        case 10:
            /* sometimes nothing happens */
            break;
        case 11: /* temporary intrinsic invisibility, or remove it if it's there */
            Soundeffect(se_low_hum, 100);
            You_hear("a low hum.");
            if (!Invis) {
                if (!Blind)
                    self_invis_message();
            } else if (!EInvis && !pm_invisible(gy.youmonst.data)) {
                if (!Blind) {
                    if (!See_invisible)
                        You("can see yourself again!");
                    else
                        You_cant("see through yourself anymore.");
                }
            } else {
                /* If we're invisible from another source */
                You_feel("a little more %s now.",
                         HInvis ? "obvious" : "hidden");
            }
            set_itimeout(&HInvis, HInvis ? 0 : rnd(200) + 200);
            newsym(u.ux, u.uy);
            break;
        case 12: /* a flash of fire */
            dofiretrap((struct obj *) 0);
            break;

        /* odd feelings */
        case 13:
            pline("A shiver runs up and down your %s!", body_part(SPINE));
            break;
        case 14:
            You_hear(Hallucination ? "the moon howling at you."
                                   : "distant howling.");
            break;
        case 15:
            if (on_level(&u.uz, &qstart_level))
                You_feel(
                    "%slike the prodigal son.",
                    (flags.female || (Upolyd && is_neuter(gy.youmonst.data)))
                        ? "oddly "
                        : "");
            else if (Role_if(PM_VALKYRIE) && Hallucination)
                You("pine for the fjords.");
            else
                You("suddenly yearn for %s.",
                    Hallucination
                        ? "Cleveland"
                        : (In_quest(&u.uz) || at_dgn_entrance("The Quest"))
                              ? "your nearby homeland"
                              : "your distant homeland");
            break;
        case 16:
            Your("pack shakes violently!");
            break;
        case 17:
            You(Hallucination ? "smell hamburgers." : "smell charred flesh.");
            break;
        case 18:
            You_feel("tired.");
            break;

        /* very occasionally something nice happens. */
        case 19: { /* tame nearby monsters */
            int i, j;
            struct monst *mtmp;

            (void) adjattrib(A_CHA, 1, AA_YESMSG);
            for (i = -1; i <= 1; i++)
                for (j = -1; j <= 1; j++) {
                    if (!isok(u.ux + i, u.uy + j))
                        continue;
                    mtmp = m_at(u.ux + i, u.uy + j);
                    if (mtmp)
                        (void) tamedog(mtmp, (struct obj *) 0, TRUE, TRUE);
                }
            break;
        }
        case 20: { /* uncurse stuff */
            struct obj pseudo;
            long save_conf = HConfusion;

            pseudo = cg.zeroobj; /* force 'uncursed' and zero out oextra */
            /* used to be SCR_REMOVE_CURSE but that could cause seffects()
               to have hero discover scroll of remove curse */
            pseudo.otyp = SPE_REMOVE_CURSE;
            pseudo.oclass = SPBOOK_CLASS;
            HConfusion = 0L;
            (void) seffects(&pseudo);
            HConfusion = save_conf;
            break;
        }
        default:
            break;
        }
    }
}

/* Set an item on fire.  Return whether the object was destroyed. */
boolean
fire_damage(
    struct obj *obj,
    boolean force, /* if True, skip luck-based protection check */
    coordxy x, coordxy y) /* where to place contents of burned up container */
{
    int chance;
    struct obj *otmp, *ncobj;
    int in_sight = !Blind && couldsee(x, y); /* Don't care if it's lit */
    int dindx;

    /* object might light in a controlled manner */
    if (catch_lit(obj))
        return FALSE;

    if (Is_container(obj) || obj->otyp == STATUE) {
        if (!is_flammable(obj)) {
            return FALSE; /* immune */
        }
        switch (obj->otyp) {
        case STATUE:
        case ICE_BOX:
            return FALSE; /* Immune */
        case CHEST:
            chance = 40;
            break;
        case LARGE_BOX:
            chance = 30;
            break;
        default:
            chance = 20;
            break;
        }
        if ((!force && (Luck + 5) > rn2(chance))
            /* note: containers aren't subject to erosion so are never
               marked fireproof/corrodeproof/&c */
            /*|| (is_flammable(obj) && obj->oerodeproof)*/
            ) {
            return FALSE;
        }
        /* Container is burnt up - dump contents out */
        if (in_sight)
            pline("%s catches fire and burns.", Yname2(obj));
        if (Has_contents(obj)) {
            if (in_sight)
                pline("Its contents fall out.");
            for (otmp = obj->cobj; otmp; otmp = ncobj) {
                ncobj = otmp->nobj;
                obj_extract_self(otmp);
                if (!flooreffects(otmp, x, y, ""))
                    place_object(otmp, x, y);
            }
        }
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (!force && (Luck + 5) > rn2(20)) {
        /*  chance per item of sustaining damage:
          *     max luck (Luck==13):    10%
          *     avg luck (Luck==0):     75%
          *     awful luck (Luck<-4):  100%
          */
        return FALSE;
    } else if (obj->otyp == EGG && obj->corpsenm == PM_PHOENIX) {
        revive_egg(obj);
        return FALSE;
    } else if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS) {
        if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
            return FALSE;
        if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
            if (in_sight)
                pline("Smoke rises from %s.", the(xname(obj)));
            return FALSE;
        }
        dindx = (obj->oclass == SCROLL_CLASS) ? 3 : 4;
        if (in_sight)
            pline("%s %s.", Yname2(obj),
                  destroy_strings[dindx][(obj->quan > 1L)]);
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (obj->oclass == POTION_CLASS) {
        dindx = (obj->otyp != POT_OIL) ? 1 : 2;
        if (in_sight)
            pline("%s %s.", Yname2(obj),
                  destroy_strings[dindx][(obj->quan > 1L)]);
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (erode_obj(obj, (char *) 0, ERODE_BURN, EF_DESTROY)
               == ER_DESTROYED) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Apply fire_damage() to an entire chain.
 *
 * Return number of objects destroyed. --ALI
 */
int
fire_damage_chain(
    struct obj *chain,
    boolean force,
    boolean here,
    coordxy x, coordxy y)
{
    struct obj *obj, *nobj;
    int num = 0;

    /* erode_obj() relies on bhitpos if target objects aren't carried by
       the hero or a monster, to check visibility controlling feedback */
    gb.bhitpos.x = x, gb.bhitpos.y = y;

    for (obj = chain; obj; obj = nobj) {
        nobj = here ? obj->nexthere : obj->nobj;
        if (fire_damage(obj, force, x, y))
            ++num;
    }

    if (num && (Blind && !couldsee(x, y)))
        You("smell smoke.");
    return num;
}

/* obj has been thrown or dropped into lava; damage is worse than mere fire */
boolean
lava_damage(struct obj *obj, coordxy x, coordxy y)
{
    int otyp = obj->otyp, ocls = obj->oclass;

    /* the Amulet, invocation items, and Rider corpses are never destroyed
       (let Book of the Dead fall through to fire_damage() to get feedback) */
    if (obj_resists(obj, 0, 0) && otyp != SPE_BOOK_OF_THE_DEAD)
        return FALSE;
    if (obj->otyp == EGG && obj->corpsenm == PM_PHOENIX) {
        revive_egg(obj);
        return FALSE;
    }
    /* destroy liquid (venom), wax, veggy, flesh, paper (except for scrolls
       and books--let fire damage deal with them), cloth, leather, wood, bone
       unless it's inherently or explicitly fireproof or contains something;
       note: potions are glass so fall through to fire_damage() and boil */
    if (obj->material < DRAGON_HIDE
        && ocls != SCROLL_CLASS && ocls != SPBOOK_CLASS
        && objects[otyp].oc_oprop != FIRE_RES
        && otyp != WAN_FIRE && otyp != FIRE_HORN
        /* assumes oerodeproof isn't overloaded for some other purpose on
           non-eroding items */
        && !obj->oerodeproof
        /* fire_damage() knows how to deal with containers and contents */
        && !Has_contents(obj)) {
        if (cansee(x, y)) {
            /* this feedback is pretty clunky and can become very verbose
               when former contents of a burned container get here via
               flooreffects() */
            if (obj == gt.thrownobj || obj == gk.kickedobj)
                pline("%s %s up!", is_plural(obj) ? "They" : "It",
                      otense(obj, "burn"));
            else
                You_see("%s hit lava and burn up!", doname(obj));
        }
        if (carried(obj)) { /* shouldn't happen */
            remove_worn_item(obj, TRUE);
            useupall(obj);
        } else
            delobj(obj);
        return TRUE;
    }
    return fire_damage(obj, TRUE, x, y);
}

void
acid_damage(struct obj *obj)
{
    /* Scrolls but not spellbooks can be erased by acid. */
    struct monst *victim;
    boolean vismon;

    if (!obj)
        return;

    victim = carried(obj) ? &gy.youmonst : mcarried(obj) ? obj->ocarry : NULL;
    vismon = victim && (victim != &gy.youmonst) && canseemon(victim);

    if (victim && inventory_resistance_check(victim, AD_ACID))
        return;

    if (obj->greased) {
        grease_protect(obj, (char *) 0, victim);
    } else if (obj->oclass == SCROLL_CLASS && obj->otyp != SCR_BLANK_PAPER) {
        if (obj->otyp != SCR_BLANK_PAPER
#ifdef MAIL_STRUCTURES
            && obj->otyp != SCR_MAIL
#endif
            ) {
            if (!Blind) {
                if (victim == &gy.youmonst)
                    Your("%s.", aobjnam(obj, "fade"));
                else if (vismon)
                    pline("%s %s.", s_suffix(Monnam(victim)),
                          aobjnam(obj, "fade"));
            }
        }
        obj->otyp = SCR_BLANK_PAPER;
        obj->spe = 0;
        obj->dknown = 0;
    } else
        erode_obj(obj, (char *) 0, ERODE_CORRODE, EF_GREASE | EF_VERBOSE);
}

staticfn void
pot_acid_damage(
    struct obj *obj,
    boolean in_invent,
    boolean described)
{
    char *bufp;
    boolean one, exploded;

    one = (obj->quan == 1L);
    exploded = FALSE;

    if (Blind && !in_invent)
        obj->dknown = 0;
    if (ga.acid_ctx.ctx_valid)
        exploded = ((obj->dknown ? ga.acid_ctx.dkn_boom
                                 : ga.acid_ctx.unk_boom) > 0);
    if (described) {
        /* just gave "The grease washes off your potion of acid."
            or "...your <color> potion." (or just "...your potion.");
            don't re-describe potion here; if we used "It explodes!"
            then "it" might be misconstrued as applying to "grease" */
        pline_The("potion%s %s!",
                    plur(obj->quan), otense(obj, "explode"));
    } else {
        /* First message is
            * "a [potion|<color> potion|potion of acid] explodes"
            * depending on obj->dknown (potion has been seen) and
            * objects[POT_ACID].oc_name_known (fully discovered),
            * or "some {plural version} explode" when relevant.
            * Second and subsequent messages for same chain and
            * matching dknown status are
            * "another [potion|<color> &c] explodes" or plural
            * variant.
            */
        bufp = simpleonames(obj);
        pline("%s%s %s!", /* "A potion explodes!" */
                !exploded ? (one ? "A " : "Some ")
                        : (one ? "Another " : "More "),
            bufp, vtense(bufp, "explode"));
    }
    if (ga.acid_ctx.ctx_valid) {
        if (obj->dknown)
            ga.acid_ctx.dkn_boom++;
        else
            ga.acid_ctx.unk_boom++;
    }
    if (obj->dknown)
        makeknown(POT_ACID);
    setnotworn(obj);
    delobj(obj);
    if (in_invent)
        update_inventory();
}

/* Get an object wet and damage it appropriately.
   Returns an erosion return value (ER_*). */
int
water_damage(
    struct obj *obj,  /* might be Null; return ER_NOTHING if so */
    const char *ostr, /* if non-Null, use instead of cxname() in messages */
    boolean force)    /* if True, skip luck-based protection check */
{
    boolean in_invent = obj && carried(obj), described = FALSE;

    if (!obj)
        return ER_NOTHING;

    if (splash_lit(obj))
        return ER_DAMAGED;

    if (!ostr)
        ostr = cxname(obj);

    if (obj->otyp == CAN_OF_GREASE && obj->spe > 0) {
        return ER_NOTHING;
    } else if (obj->otyp == TOWEL && obj->spe < 7) {
        /* a negative change induces a reverse increment, adding abs(change);
           spe starts 0..6, arg passed to rnd() is 1..7, change is -7..-1,
           final spe is 1..7 and always greater than its starting value */
        wet_a_towel(obj, -rnd(7 - obj->spe), TRUE);
        return ER_NOTHING;
    } else if (obj->greased) {
        if (!rn2(2)) {
            obj->greased = 0;
            if (in_invent) {
                pline_The("grease on %s washes off.", yname(obj));
                described = TRUE; /* used to modify potion feedback */
                update_inventory();
            }
            /* ungreased potions of acid will always be destroyed by water */
            if (obj->otyp == POT_ACID) {
                pot_acid_damage(obj, in_invent, described);
                return ER_DESTROYED;
            }
        }
        return ER_GREASED;
    } else if (obj->oerodeproof) {
        return ER_NOTHING;
    } else if (Is_container(obj)
               && (!Waterproof_container(obj) || (obj->cursed && !rn2(3)))) {
        if (in_invent) {
            pline("Some %s gets into your %s!", hliquid("water"), ostr);
            gm.mentioned_water = !Hallucination;
        }
        /* assume that if we're getting water into a container, further water
         * damage should also get inside nested containers */
        water_damage_chain(obj->cobj, FALSE, 0, TRUE);
        return ER_DAMAGED; /* contents were damaged */
    } else if (Waterproof_container(obj)) {
        if (in_invent) {
            pline_The("%s slides right off your %s.", hliquid("water"), ostr);
            gm.mentioned_water = !Hallucination;
            makeknown(obj->otyp); /* if an oilskin sack, discover it; doesn't
                                   * matter for chest, large box, ice box */
        }
        /* not actually damaged, but because we /didn't/ get the "water
           gets into!" message, the player now has more information and
           thus we need to waste any potion they may have used (also,
           flavourwise the water is now on the floor) */
        return ER_DAMAGED;
    } else if (!force && (Luck + 5) > rn2(20)) {
        /*  chance per item of sustaining damage:
            *   max luck:               10%
            *   avg luck (Luck==0):     75%
            *   awful luck (Luck<-4):  100%
            */
        return ER_NOTHING;
    } else if (obj->oclass == SCROLL_CLASS) {
        if (obj->otyp == SCR_BLANK_PAPER || obj->otyp == SCR_WATER
#ifdef MAIL_STRUCTURES
            || obj->otyp == SCR_MAIL
#endif
           ) return 0;
        if (in_invent)
            Your("%s %s.", ostr, vtense(ostr, "fade"));

        obj->otyp = SCR_BLANK_PAPER;
        obj->dknown = 0;
        obj->spe = 0;
        if (in_invent)
            update_inventory();
        return ER_DAMAGED;
    } else if (obj->oclass == SPBOOK_CLASS) {
        int otyp = obj->otyp;

        if (otyp == SPE_BOOK_OF_THE_DEAD) {
            coordxy ox = 0, oy = 0;

            /* note: The Book of the Dead can't be contained or buried */
            if (get_obj_location(obj, &ox, &oy, CONTAINED_TOO | BURIED_TOO))
                obj->ox = ox, obj->oy = oy;
            if (isok(ox, oy) && cansee(ox, oy))
                pline("Steam rises from %s.", the(xname(obj)));
            return 0;
        } else if (otyp == SPE_BLANK_PAPER) {
            return 0;
        }
        if (in_invent)
            Your("%s %s.", ostr, vtense(ostr, "fade"));

        obj->otyp = SPE_BLANK_PAPER;
        set_material(obj, PAPER); /* in case it was one of the LEATHER books */
        /* same re-init as over-reading or polymorph; matters if it gets
           polymorphed into non-blank; doesn't matter if eventually written
           on since that replaces it with new book and studied count of 0 */
        if (obj->spestudied)
            obj->spestudied = rn2(obj->spestudied);
        obj->dknown = 0;
        /* blanking a novel is more involved than blanking a spellbook */
        if (otyp == SPE_NOVEL) /* old type */
            blank_novel(obj);
        if (in_invent)
            update_inventory();
        return ER_DAMAGED;
    } else if (obj->oclass == POTION_CLASS) {
        if (obj->otyp == POT_ACID) {
            pot_acid_damage(obj, in_invent, described);
            return ER_DESTROYED;
        } else if (obj->odiluted) {
            if (in_invent)
                Your("%s %s further.", ostr, vtense(ostr, "dilute"));

            obj->otyp = POT_WATER;
            obj->dknown = 0;
            obj->blessed = obj->cursed = 0;
            obj->odiluted = 0;
            if (in_invent)
                update_inventory();
            return ER_DAMAGED;
        } else if (obj->otyp == POT_OIL) {
            pline("The water doesn't seem to mix with your %s.", ostr);
            makeknown(POT_OIL);
        } else if (obj->otyp != POT_WATER) {
            if (in_invent)
                Your("%s %s.", ostr, vtense(ostr, "dilute"));

            obj->odiluted++;
            if (in_invent)
                update_inventory();
            return ER_DAMAGED;
        }
    } else {
        return erode_obj(obj, ostr, ERODE_RUST, EF_NONE);
    }
    return ER_NOTHING;
}

/* Apply water damage to a chain (inventory, floor, monster inventory) of
 * objects.
 * here is TRUE if the chain represents objects on the floor.
 * count, if positive, is the number of items that will be damaged by
 * this function.
 * If trying to wet everything (e.g. falling into water), set count < 1.
 */
void
water_damage_chain(
    struct obj *obj,
    boolean here,
    int count,
    boolean do_container)
{
    struct obj *otmp = obj;
    struct obj *nobj;
    int i = 0, j;
    struct obj** to_damage = NULL;
    coordxy x, y;
    coord save_bhitpos;

    if (!obj)
        return;

    if (count >= 1) {
        /* reservoir sampling: setup */
        to_damage = (struct obj**) alloc(sizeof(struct obj*) * count);
        for (otmp = obj; otmp; otmp = (here ? otmp->nexthere : otmp->nobj)) {
            if (!do_container && Is_container(otmp))
                continue;
            to_damage[i] = otmp;
            i++;
            if (i == count)
                break;
        }
        if (i < count) {
            /* fewer than count items in chain, so just damage everything */
            free((genericptr_t) to_damage);
            count = -1; /* let the rest of this function handle it */
        }
    }

    /* initialize acid context: so far, neither seen (dknown) potions of
       acid nor unseen have exploded during this water damage sequence */
    ga.acid_ctx.dkn_boom = ga.acid_ctx.unk_boom = 0;
    ga.acid_ctx.ctx_valid = TRUE;
    /* we don't want to permanently overwrite bhitpos below, since we can get
       here from scenarios where it was in use up the call stack (e.g. thrown
       item hurtling the levitating hero into a wall of water) */
    save_bhitpos = gb.bhitpos;
    /* erode_obj() relies on bhitpos if target objects aren't carried by
       the hero or a monster, to check visibility controlling feedback */
    if (get_obj_location(obj, &x, &y, CONTAINED_TOO))
        gb.bhitpos.x = x, gb.bhitpos.y = y;

    i = 0;
    for (otmp = obj; otmp; otmp = nobj) {
        /* if acid explodes or other item destruction happens, otmp will be
         * deleted. Avoid reading garbage data from it. */
        nobj = (here ? otmp->nexthere : otmp->nobj);
        if (!do_container && Is_container(otmp))
            continue;
        if (count < 1) {
            water_damage(otmp, (char *) 0, FALSE);
        }
        else {
            /* reservoir sampling: replace elements with lowering probability */
            i++;
            if (i <= count) {
                /* skip the first count items of the object list since they're
                 * already in to_damage; this avoids putting the same object in
                 * to_damage twice */
                continue;
            }
            j = rn2(i);
            if (j < count) {
                to_damage[j] = otmp;
            }
        }
    }

    if (count >= 1) {
        for (i = 0; i < count; i++) {
            water_damage(to_damage[i], (char *) 0, FALSE);
        }
        free((genericptr_t) to_damage);
    }

    /* reset acid context and bhitpos */
    ga.acid_ctx.dkn_boom = ga.acid_ctx.unk_boom = 0;
    ga.acid_ctx.ctx_valid = FALSE;
    gb.bhitpos = save_bhitpos;
}

/*
 * This function is potentially expensive - rolling
 * inventory list multiple times.  Luckily it's seldom needed.
 * Returns TRUE if disrobing made player unencumbered enough to
 * crawl out of the current predicament.
 */
staticfn boolean
emergency_disrobe(boolean *lostsome)
{
    int invc = inv_cnt(TRUE);

    while (near_capacity() > (Punished ? UNENCUMBERED : SLT_ENCUMBER)) {
        struct obj *obj, *nextobj, *otmp = (struct obj *) 0;
        int i;

        /* Pick a random object */
        if (invc > 0) {
            i = rn2(invc);
            for (obj = gi.invent; obj; obj = nextobj) {
                nextobj = obj->nobj;
                /*
                 * Undroppables are: body armor, boots, gloves,
                 * amulets, and rings because of the time and effort
                 * in removing them and other cursed stuff
                 * for obvious reasons. Also, any item in the midst
                 * of being taken off or stolen.
                 */
                if (!(undroppable(obj) || obj == uamul
                      || obj == uleft || obj == uright || obj == ublindf
                      || obj == uarm || obj == uarmc || obj == uarmg
                      || obj == uarmf || obj == uarmu
                      || (obj->cursed && (obj == uarmh || obj == uarms))
                      || welded(obj)
                      || obj->o_id == gs.stealoid || obj->in_use))
                    otmp = obj;
                /* reached the mark and found some stuff to drop? */
                if (--i < 0 && otmp)
                    break;

                /* else continue */
            }
        }
        if (!otmp)
            return FALSE; /* nothing to drop! */
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        *lostsome = TRUE;
        dropx(otmp);
        invc--;
    }
    return TRUE;
}

/* pick a random goodpos() next to x,y for monster mtmp.
   mtmp could be &gy.youmonst, uses then crawl_destination().
   returns TRUE if any good position found, with the coord in x,y */
boolean
rnd_nextto_goodpos(coordxy *x, coordxy *y, struct monst *mtmp)
{
    int i, j;
    boolean is_u = (mtmp == &gy.youmonst);
    coordxy nx, ny, k, dirs[N_DIRS];

    for (i = 0; i < N_DIRS; ++i)
        dirs[i] = i;
    for (i = N_DIRS; i > 0; --i) {
        j = rn2(i);
        k = dirs[j];
        dirs[j] = dirs[i - 1];
        dirs[i - 1] = k;
    }
    for (i = 0; i < N_DIRS; ++i) {
        nx = *x + xdir[dirs[i]];
        ny = *y + ydir[dirs[i]];
        /* crawl_destination and goodpos both include an isok() check */
        if (is_u ? crawl_destination(nx, ny) : goodpos(nx, ny, mtmp, 0)) {
            *x = nx;
            *y = ny;
            return TRUE;
        }
    }
    return FALSE;
}

/* print a message about being back on the ground after leaving a pool */
void
back_on_ground(boolean rescued)
{
    const char *preposit = (Levitation || Flying) ? "over" : "on",
               *surf = surface(u.ux, u.uy), *you_are_back;
    char icebuf[QBUFSZ];

    if (is_ice(u.ux, u.uy)) {
        /* "on ice" */
        surf = ice_descr(u.ux, u.uy, icebuf);
    } else if (!strcmpi(surf, "floor") || !strcmpi(surf, "ground")) {
        /* "on solid ground" */
        surf = "solid ground";
    } else if (!strcmpi(surf, "bridge") || !strcmpi(surf, "altar")
               || !strcmpi(surf, "headstone")) {
        /* "on a bridge" */
        surf = an(surf);
    } else if (!strcmpi(surf, "stairs") || !strcmpi(surf, "lava")
               || !strcmpi(surf, "bottom")) {
        /* "on the stairs" */
        surf = the(surf);
    } else { /* "cloud", "air", "air bubble", "wall", "fountain", "doorway" */
        /* "in a cloud", "in the air" */
        surf = !strcmp(surf, "air") ? the(surf) : an(surf);
        preposit = "in";
    }
    if (rescued) {
        you_are_back = "You find yourself";
    } else {
        you_are_back = flags.verbose ? "You are back" : "Back";
    }
    pline("%s %s %s.", you_are_back, preposit, surf);
    iflags.last_msg = PLNMSG_BACK_ON_GROUND;
}

/* life-saving or divine rescue has attempted to get the hero out of hostile
   terrain and put hero in an unexpected spot or failed due to overfull level
   and just prevented death so "back on solid ground" may be inappropriate */
void
rescued_from_terrain(int how)
{
    static const char find_yourself[] = "find yourself";
    struct rm *lev = &levl[u.ux][u.uy];
    boolean mesggiven = FALSE;

    switch (how) {
    case DROWNING:
        if (is_pool(u.ux, u.uy)) {
            You("%s %s of %s.", find_yourself,
                (Is_waterlevel(&u.uz) || IS_WATERWALL(lev->typ))
                  ? "in the midst" : "on top",
                hliquid("water"));
            mesggiven = TRUE;
        } else if (IS_AIR(lev->typ)) {
            You("%s in %s.", find_yourself,
                Is_waterlevel(&u.uz) ? "an air bubble" : "mid air");
            mesggiven = TRUE;
        }
        break;
    case BURNING: /* moved onto lava without fire resistance */
    case DISSOLVED: /* sunk into lava while fire resistant */
        if (is_pool(u.ux, u.uy)) {
            You("%s %s %s.", find_yourself,
                u.uinwater ? "in" : "on", hliquid("water"));
            mesggiven = TRUE;
        } else if (is_lava(u.ux, u.uy)) {
            You("%s on top of %s.", find_yourself, hliquid("molten lava"));
            mesggiven = TRUE;
        }
        break;
    default:
        break;
    }
    if (!mesggiven)
        back_on_ground(TRUE);

    iflags.last_msg = PLNMSG_BACK_ON_GROUND; /* for describe_decor() */
    /* feedback just disclosed this */
    update_lastseentyp(u.ux, u.uy);
    iflags.prev_decor = svl.lastseentyp[u.ux][u.uy];
}

/* return TRUE iff player relocated */
boolean
drown(void)
{
    const char *pool_of_water;
    boolean inpool_ok = FALSE;
    int i;
    coordxy x, y;
    boolean is_solid = is_waterwall(u.ux, u.uy);

    if (Levitation || Flying) {
        /* shouldn't drown */
        return FALSE;
    }
    feel_newsym(u.ux, u.uy); /* in case Blind, map the water here */
    /* Note: drown() callers should NOT check !Wwalking as a condition of
     * calling it. If water walking boots prevent the player from falling in,
     * they should become identified. */
    if (Wwalking) {
        if (u.uinwater) {
            impossible("drown: in water but also water walking?");
        }
        /* maybe we were called because the hero moved or fell into a pool; if
         * so, assuming the only source of water walking is water walking
         * boots, identify them. */
        if (!objects[WATER_WALKING_BOOTS].oc_name_known) {
            Your("boots don't sink into the water!");
        }
        makeknown(WATER_WALKING_BOOTS);
        return FALSE;
    }

    /* happily wading in the same contiguous pool */
    if (u.uinwater && is_pool(u.ux - u.dx, u.uy - u.dy)
        && (Swimming || Amphibious || Breathless)) {
        /* water effects on objects every now and then */
        if (!rn2(5))
            inpool_ok = TRUE;
        else
            return FALSE;
    }

    if (!u.uinwater) {
        You("%s into the %s%c", is_solid ? "plunge" : "fall",
            waterbody_name(u.ux, u.uy),
            (Amphibious || Swimming || Breathless) ? '.' : '!');
        if (!Swimming && !is_solid)
            You("sink like %s.", Hallucination ? "the Titanic" : "a rock");
    }

    water_damage_chain(gi.invent, FALSE, 0, TRUE);

    if (u.umonnum == PM_GREMLIN && rn2(3)) {
        (void) split_mon(&gy.youmonst, (struct monst *) 0);
    } else if (u.umonnum == PM_IRON_GOLEM) {
        You("rust!");
        i = Maybe_Half_Phys(d(2, 6));
        if (u.mhmax > i)
            u.mhmax -= i;
        losehp(i, "rusting away", KILLED_BY);
    }
    if (inpool_ok)
        return FALSE;

    if ((i = number_leashed()) > 0) {
        pline_The("leash%s slip%s loose.", (i > 1) ? "es" : "",
                  (i > 1) ? "" : "s");
        unleash_all();
    }

    if (Amphibious || Breathless || Swimming) {
        if (Amphibious || Breathless) {
            if (flags.verbose)
                pline("But you aren't drowning.");
            if (!Is_waterlevel(&u.uz)) {
                if (Hallucination)
                    Your("keel hits the bottom.");
                else
                    You("touch bottom.");
            }
        }
        if (Punished) {
            unplacebc();
            placebc();
        }
        vision_recalc(2); /* unsee old position */
        set_uinwater(1); /* u.uinwater = 1 */
        under_water(1);
        gv.vision_full_recalc = 1;
        return FALSE;
    }
    if ((Teleportation || can_teleport(gy.youmonst.data)) && !Unaware
        && (Teleport_control || rn2(3) < Luck + 2)) {
        You("attempt a teleport spell."); /* utcsri!carroll */
        if (!noteleport_level(&gy.youmonst)) {
            (void) dotele(FALSE);
            if (!is_pool(u.ux, u.uy))
                return TRUE;
        } else
            pline_The("attempted teleport spell fails.");
    }
    if (u.usteed) {
        dismount_steed(DISMOUNT_GENERIC);
        if (!is_pool(u.ux, u.uy))
            return TRUE;
    }
    /* if sleeping, wake up now so that we don't crawl out of water
       while still asleep; we can't do that the same way that waking
       due to combat is handled; note unmul() clears u.usleep */
    if (u.usleep)
        unmul("Suddenly you wake up!");
    /* being doused will revive from fainting */
    if (is_fainted())
        reset_faint();

    x = u.ux, y = u.uy;
    /* have to be able to move in order to crawl */
    if (gm.multi >= 0 && gy.youmonst.data->mmove
        && rnd_nextto_goodpos(&x, &y, &gy.youmonst)) {
        boolean lost = FALSE;
        /* time to do some strip-tease... */
        boolean succ = Is_waterlevel(&u.uz) ? TRUE : emergency_disrobe(&lost);

        You("try to crawl out of the %s.", hliquid("water"));
        if (lost)
            You("dump some of your gear to lose weight...");
        if (succ) {
            pline("Pheew!  That was close.");
            teleds(x, y, TELEDS_ALLOW_DRAG);
            return TRUE;
        }
        /* still too much weight */
        pline("But in vain.");
    }
    set_uinwater(1); /* u.uinwater = 1 */
    urgent_pline("You drown.");
    /* first pass is survivable by using up an amulet of life-saving or by
       answering no to "Die?" in explore|wizard mode; second pass can only
       be survivable via the latter */
    for (i = 0; i < 2; i++) {
        /* killer format and name are reconstructed every iteration
           because lifesaving resets them */
        pool_of_water = waterbody_name(u.ux, u.uy);
        svk.killer.format = KILLED_BY_AN;
        /* avoid "drowned in [a] water" */
        if (!strcmp(pool_of_water, "water"))
            pool_of_water = "deep water", svk.killer.format = KILLED_BY;
        /* avoid "drowned in _a_ limitless water" on Plane of Water */
        else if (!strcmp(pool_of_water, "limitless water"))
            svk.killer.format = KILLED_BY;
        Strcpy(svk.killer.name, pool_of_water);
        done(DROWNING);
        /* oops, we're still alive.  better get out of the water. */
        if (safe_teleds(TELEDS_ALLOW_DRAG | TELEDS_TELEPORT))
            break; /* successful life-save */
        /* nowhere safe to land; repeat drowning loop... */
        pline("You're still drowning.");
    }

    if (u.uinwater)
        set_uinwater(0); /* u.uinwater = 0 */
    rescued_from_terrain(DROWNING);
    return TRUE;
}

void
drain_en(int n, boolean max_already_drained)
{
    const char *mesg;
    char punct = max_already_drained ? '!' : '.';

    /*
     * FIXME?
     *  u.uenmax should probably have a higher minimum than 0;
     *  perhaps u.ulevel or (u.ulevel + 1) / 2
     */
    if (u.uenmax < 1) {
        /* energy is completely gone */
        if (u.uen || u.uenmax) { /* paranoia */
            u.uen = u.uenmax = 0;
            disp.botl = TRUE;
        }
        mesg = "momentarily lethargic";
    } else {
        /* throttle further loss a bit when there's not much left to lose */
        if (n > (u.uen + u.uenmax) / 3)
            n = rnd(n);

        mesg = "your magical energy drain away";
        if (n > u.uen)
            punct = '!';

        u.uen -= n;
        if (u.uen < 0) {
            u.uenmax -= rnd(-u.uen);
            if (u.uenmax < 0)
                u.uenmax = 0;
            u.uen = 0;
        } else if (u.uen > u.uenmax) {
            /* uen might be greater than uenmax if caller reduced uenmax
               and then we throttled the loss being applied to current */
            u.uen = u.uenmax;
        }
        disp.botl = TRUE;
    }
    /* after manipulating u.uen,uenmax and setting context.botl, so
       that You_feel() -> pline() will update status before the message */
    You_feel("%s%c", mesg, punct);
}

/* the #untrap command - disarm a trap */
int
dountrap(void)
{
    if (!could_untrap(TRUE, FALSE))
        return ECMD_OK;

    return untrap(FALSE, 0, 0, (struct obj *) 0) ? ECMD_TIME : ECMD_OK;
}

/* preliminary checks for dountrap(); also used for autounlock */
int
could_untrap(boolean verbosely, boolean check_floor)
{
    char buf[BUFSZ];

    buf[0] = '\0';
    if (near_capacity() >= HVY_ENCUMBER) {
        Strcpy(buf, "You're too strained to do that.");
    } else if ((nohands(gy.youmonst.data) && !webmaker(gy.youmonst.data))
               || !gy.youmonst.data->mmove) {
        Strcpy(buf, "And just how do you expect to do that?");
    } else if (u.ustuck && sticks(gy.youmonst.data)) {
        Sprintf(buf, "You'll have to let go of %s first.", mon_nam(u.ustuck));
    } else if (u.ustuck || (welded(uwep) && bimanual(uwep))) {
        Sprintf(buf, "Your %s seem to be too busy for that.",
                makeplural(body_part(HAND)));
    } else if (check_floor && !can_reach_floor(FALSE)) {
        /* only checked here for autounlock of chest/box and that will
           be !verbosely so precise details of the message don't matter */
        Sprintf(buf, "You can't reach the %s.", surface(u.ux, u.uy));
    }
    if (buf[0]) {
        if (verbosely)
            pline("%s", buf);
        return 0;
    }
    return 1;
}

/* Probability of disabling a trap.  Helge Hafting;
   Returns 0 for success, non-0 for failure. */
staticfn int
untrap_prob(
    struct trap *ttmp) /* must not be Null */
{
    int chance = 3;

    /* non-spiders are less adept at dealing with webs */
    if (ttmp->ttyp == WEB) {
        /* this assumes that all fiery artifacts are blades; no need to
           make it more complicated unless/until that changes */
        struct obj *wep = (uwep && is_blade(uwep)) ? uwep
                          : (uswapwep && u.twoweap && is_blade(uswapwep))
                            ? uswapwep : NULL;

        /* FIXME? Forcefight of adjacent web works with bare-handed and
           martial arts but #untrap of same resorts to !webmaker() chance */
        if (wep && !m_at(ttmp->tx, ttmp->ty)) {
            /* primary or secondary weapon is a blade (which includes
               daggers but not axes or bladed polearms) */
            if (u_wield_art(ART_STING) || attacks(AD_FIRE, wep))
                chance = 1;
            /* else chance stays 3 */
        } else if (!webmaker(gy.youmonst.data)) {
            chance = 7; /* 3.7: used to be 30 */
        }
    }
    if (Confusion || Hallucination)
        chance++;
    if (Blind)
        chance++;
    if (Stunned)
        chance += 2;
    if (Fumbling)
        chance *= 2;
    /* Your own traps are better known than others. */
    if (ttmp->madeby_u)
        chance--;
    if (Role_if(PM_RANGER) && ttmp->ttyp == BEAR_TRAP && chance <= 3)
        return 0; /* always succeeds */
    if (Role_if(PM_ROGUE)) {
        if (rn2(2 * MAXULEV) < u.ulevel)
            chance--;
        if (u.uhave.questart && chance > 1)
            chance--;
    } else if (Role_if(PM_RANGER) && chance > 1)
        chance--;
    if (chance < 1)
        chance = 1;
    return rn2(chance);
}

/* whether moving to a trap location is moving "into" the trap or "onto" it */
boolean
into_vs_onto(int traptype)
{
    switch (traptype) {
    case BEAR_TRAP:
    case PIT:
    case SPIKED_PIT:
    case HOLE:
    case TELEP_TRAP:
    case LEVEL_TELEP:
    case MAGIC_PORTAL:
    case WEB:
        return TRUE;
    }
    return FALSE;
}

/* while attempting to disarm an adjacent trap, we've fallen into it */
staticfn void
move_into_trap(struct trap *ttmp)
{
    int bc = 0;
    coordxy x = ttmp->tx, y = ttmp->ty, bx, by, cx, cy;
    boolean unused;

    bx = by = cx = cy = 0; /* lint suppression */
    /* we know there's no monster in the way and we're not trapped, but
       need to make sure the move is not diagonally into or out of a
       doorway; the sgn() calls are redundant since ttmp is adjacent */
    if (test_move(u.ux, u.uy, sgn(x - u.ux), sgn(y - u.uy), TEST_MOVE)
        && (!Punished
            || drag_ball(x, y, &bc, &bx, &by, &cx, &cy, &unused, TRUE))) {
        /* move hero and update map */
        u.ux0 = u.ux, u.uy0 = u.uy;
        /* set u.ux,u.uy and u.usteed->mx,my plus handle CLIPPING */
        u_on_newpos(x, y);
        u.umoved = TRUE;
        newsym(u.ux0, u.uy0);
        vision_recalc(1);
        check_leash(u.ux0, u.uy0);
        if (Punished)
            move_bc(0, bc, bx, by, cx, cy);
        /* marking the trap unseen forces dotrap() to treat it like a new
           discovery and prevents pickup() -> look_here() -> check_here()
           from giving a redundant "there is a <trap> here" message when
           there are objects covering this trap */
        ttmp->tseen = 0; /* hack for check_here() */
        /* trigger the trap */
        iflags.failing_untrap++; /* spoteffects() -> dotrap(,FAILEDUNTRAP) */
        spoteffects(TRUE); /* pickup() + dotrap() */
        iflags.failing_untrap--;
        /* this should no longer be necessary; before the failing_untrap
           hack, Flying hero would not trigger an unseen bear trap and
           setting it not-yet-seen above resulted in leaving it hidden */
        if ((ttmp = t_at(u.ux, u.uy)) != 0)
            ttmp->tseen = 1;
        exercise(A_WIS, FALSE);
    } else {
        /* caller has just printed "Whoops..." so if hero is prevented from
           moving, a followup message is needed */
        pline("Fortunately, you don't move %s it.",
              into_vs_onto(ttmp->ttyp) ? "into" : "onto");
    }
}

/* 0: doesn't even try; 1: tries and fails; 2: succeeds */
staticfn int
try_disarm(
    struct trap *ttmp,
    boolean force_failure)
{
    struct monst *mtmp = m_at(ttmp->tx, ttmp->ty);
    int ttype = ttmp->ttyp;
    boolean under_u = (!u.dx && !u.dy);
    boolean holdingtrap = (ttype == BEAR_TRAP || ttype == WEB);

    /* Test for monster first, monsters are displayed instead of trap. */
    if (mtmp && (!mtmp->mtrapped || !holdingtrap)) {
        pline("%s is in the way.", Monnam(mtmp));
        return 0;
    }
    /* We might be forced to move onto the trap's location. */
    if (sobj_at(BOULDER, ttmp->tx, ttmp->ty) && !Passes_walls && !under_u) {
        There("is a boulder in your way.");
        return 0;
    }
    /* duplicate tight-space checks from test_move */
    if (u.dx && u.dy && bad_rock(gy.youmonst.data, u.ux, ttmp->ty)
        && bad_rock(gy.youmonst.data, ttmp->tx, u.uy)) {
        if ((gi.invent && (inv_weight() + weight_cap() > 600))
            || bigmonst(gy.youmonst.data)) {
            /* don't allow untrap if they can't get thru to it */
            You("are unable to reach the %s!", trapname(ttype, FALSE));
            return 0;
        }
    }
    /* untrappable traps are located on the ground. */
    if (!can_reach_floor(under_u)) {
        if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
            rider_cant_reach();
        else
            You("are unable to reach the %s!", trapname(ttype, FALSE));
        return 0;
    }

    /* Will our hero succeed? */
    if (force_failure || untrap_prob(ttmp)) {
        if (rnl(5)) {
            pline("Whoops...");
            if (mtmp) { /* must be a trap that holds monsters */
                if (ttype == BEAR_TRAP) {
                    if (mtmp->mtame)
                        abuse_dog(mtmp);
                    mtmp->mhp -= rnd(4);
                    if (DEADMONSTER(mtmp))
                        killed(mtmp);
                } else if (ttype == WEB) {
                    struct trap *ttmp2 = t_at(u.ux, u.uy);

                    if (!webmaker(gy.youmonst.data)
                        /* don't always try to spread the web */
                        && !rn2(3)
                        /* is there already a trap at hero's spot?
                           if so, don't clobber it with spreading web */
                        && (ttmp2
                            ? (ttmp2->ttyp == WEB)
                            /* make a new web to trap hero in */
                            : (ttmp2 = maketrap(u.ux, u.uy, WEB)) != 0)) {
                        pline_The("web sticks to you.  You're caught too!");
                        dotrap(ttmp2, NOWEBMSG);
                        if (u.usteed && u.utrap) {
                            /* you, not steed, are trapped */
                            dismount_steed(DISMOUNT_FELL);
                        }
                    }
                    if (mtmp->mtrapped)
                        pline("%s remains entangled.", Monnam(mtmp));
                }
            } else if (under_u) {
                /* [don't need the iflags.failing_untrap hack here] */
                dotrap(ttmp, FAILEDUNTRAP);
            } else {
                move_into_trap(ttmp);
            }
        } else {
            pline("%s %s is difficult to %s.",
                  ttmp->madeby_u ? "Your" : under_u ? "This" : "That",
                  trapname(ttype, FALSE),
                  (ttype == WEB) ? "remove" : "disarm");
        }
        return 1;
    }
    return 2;
}

staticfn void
reward_untrap(struct trap *ttmp, struct monst *mtmp)
{
    if (!ttmp->madeby_u) {
        if (rnl(10) < 8 && !mtmp->mpeaceful && !helpless(mtmp)
            && !mtmp->mfrozen && !mindless(mtmp->data)
            && !unique_corpstat(mtmp->data)
            && mtmp->data->mlet != S_HUMAN) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp); /* reset alignment */
            pline("%s is grateful.", Monnam(mtmp));
        }
        /* Helping someone out of a trap is a nice thing to do.
           A lawful may be rewarded, but not too often.  */
        if (!rn2(3) && !rnl(8) && u.ualign.type == A_LAWFUL) {
            adjalign(1);
            You_feel("that you did the right thing.");
        }
    }
}

/* Help a monster out of a bear trap or web, or if no monster is
   present, disarm a bear trap or destroy a web.  Helge Hafting */
staticfn int
disarm_holdingtrap(struct trap *ttmp)
{
    struct monst *mtmp;
    const char *which = the_your[ttmp->madeby_u];
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;

    /* ok, disarm it. */

    /* untrap the monster, if any.
       There's no need for a cockatrice test, only the trap is touched */
    if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
        mtmp->mtrapped = 0;
        You("extract %s from %s %s.", mon_nam(mtmp),
            which, (ttmp->ttyp == BEAR_TRAP) ? "bear trap" : "web");
        reward_untrap(ttmp, mtmp);
    } else if (ttmp->ttyp == BEAR_TRAP) {
        You("disarm %s bear trap.", which);
        deltrap_with_ammo(ttmp, DELTRAP_PLACE_AMMO);
    } else if (ttmp->ttyp == WEB) {
        struct obj *wep = (uwep && is_blade(uwep)) ? uwep
                          : (uswapwep && u.twoweap && is_blade(uswapwep))
                            ? uswapwep : NULL;

        if (wep && wep->oartifact
            && (u_wield_art(ART_STING) || attacks(AD_FIRE, wep)))
            pline("%s %s through %s web!", bare_artifactname(uwep),
                  u_wield_art(ART_STING) ? "cuts" : "burns", which);
        else if (wep)
            You("cut through %s web.", which);
        else
            You("succeed in removing %s web.", which);
        deltrap(ttmp);
    }
    newsym(u.ux + u.dx, u.uy + u.dy);
    return 1;
}

staticfn int
disarm_landmine(struct trap *ttmp) /* Helge Hafting */
{
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;
    You("disarm %s land mine.", the_your[ttmp->madeby_u]);
    deltrap_with_ammo(ttmp, DELTRAP_PLACE_AMMO);
    return 1;
}

/* getobj callback for object to disarm a squeaky board with */
staticfn int
unsqueak_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    if (obj->otyp == CAN_OF_GREASE)
        return GETOBJ_SUGGEST;

    if (obj->otyp == POT_OIL && obj->dknown
        && objects[POT_OIL].oc_name_known)
        return GETOBJ_SUGGEST;

    /* downplay all other potions, including unidentified oil
     * Potential extension: if oil is known, skip this and exclude all other
     * potions. */
    if (obj->oclass == POTION_CLASS)
        return GETOBJ_DOWNPLAY;

    return GETOBJ_EXCLUDE;
}

/* it may not make much sense to use grease on floor boards, but so what? */
staticfn int
disarm_squeaky_board(struct trap *ttmp)
{
    struct obj *obj;
    boolean bad_tool;
    int fails;

    obj = getobj("untrap with", unsqueak_ok, GETOBJ_PROMPT);
    if (!obj)
        return 0;

    bad_tool = (obj->cursed
                || ((obj->otyp != POT_OIL || obj->lamplit)
                    && (obj->otyp != CAN_OF_GREASE || !obj->spe)));
    fails = try_disarm(ttmp, bad_tool);
    if (fails < 2)
        return fails;

    /* successfully used oil or grease to fix squeaky board */
    if (obj->otyp == CAN_OF_GREASE) {
        consume_obj_charge(obj, TRUE);
    } else {
        useup(obj); /* oil */
        makeknown(POT_OIL);
    }
    You("repair the squeaky board."); /* no madeby_u */
    deltrap(ttmp);
    newsym(u.ux + u.dx, u.uy + u.dy);
    more_experienced(1, 5);
    newexplevel();
    return 1;
}

/* removes traps that shoot arrows, darts, etc. */
staticfn int
disarm_shooting_trap(struct trap* ttmp)
{
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;
    You("disarm %s trap.", the_your[ttmp->madeby_u]);
    deltrap_with_ammo(ttmp, DELTRAP_TAKE_AMMO);
    return 1;
}

/* trying to #untrap a monster from a pit; is the weight too heavy? */
staticfn int
try_lift(
    struct monst *mtmp, /* trapped monster */
    struct trap *ttmp, /* pit, possibly made by hero, or spiked pit */
    int xtra_wt, /* monster (corpse weight) + (stuff ? minvent weight : 0) */
    boolean stuff) /* False: monster w/o minvent; True: w/ minvent */
{
    if (calc_capacity(xtra_wt) >= HVY_ENCUMBER) {
        pline("%s is %s for you to lift.", Monnam(mtmp),
              stuff ? "carrying too much" : "too heavy");
        if (!ttmp->madeby_u && !mtmp->mpeaceful && mtmp->mcanmove
            && !mindless(mtmp->data) && mtmp->data->mlet != S_HUMAN
            && rnl(10) < 3) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp); /* reset alignment */
            pline("%s thinks it was nice of you to try.", Monnam(mtmp));
        }
        return 0;
    }
    return 1;
}

/* Help trapped monster (out of a (spiked) pit) */
staticfn int
help_monster_out(
    struct monst *mtmp,
    struct trap *ttmp)
{
    int xtra_wt;
    struct obj *otmp;
    boolean uprob;

    /*
     * This works when levitating too -- consistent with the ability
     * to hit monsters while levitating.
     *
     * Should perhaps check that our hero has arms/hands at the
     * moment.  Helping can also be done by engulfing...
     *
     * Test the monster first - monsters are displayed before traps.
     */
    if (!mtmp->mtrapped) {
        pline("%s isn't trapped.", Monnam(mtmp));
        return 0;
    }
    /* Do you have the necessary capacity to lift anything? */
    if (check_capacity((char *) 0))
        return 1;

    /* Will our hero succeed? */
    if ((uprob = untrap_prob(ttmp)) != 0 && !helpless(mtmp)) {
        You("try to reach out your %s, but %s backs away skeptically.",
            makeplural(body_part(ARM)), mon_nam(mtmp));
        return 1;
    }

    /* is it a cockatrice?... */
    if (touch_petrifies(mtmp->data) && !uarmg && !Stone_resistance) {
        const char *mtmp_pmname = mon_pmname(mtmp);

        You("grab the trapped %s using your bare %s.",
            mtmp_pmname, makeplural(body_part(HAND)));

        if (poly_when_stoned(gy.youmonst.data)
            && polymon(PM_STONE_GOLEM, POLYMON_ALL_MSGS)) {
            display_nhwindow(WIN_MESSAGE, FALSE);
        } else {
            char kbuf[BUFSZ];

            Sprintf(kbuf, "trying to help %s out of a pit", an(mtmp_pmname));
            instapetrify(kbuf);
            return 1;
        }
    }
    /* need to do cockatrice check first if sleeping or paralyzed */
    if (uprob) {
        You("try to grab %s, but cannot get a firm grasp.", mon_nam(mtmp));
        if (mtmp->msleeping) {
            wakeup(mtmp, FALSE, TRUE);
        }
        return 1;
    }

    You("reach out your %s and grab %s.", makeplural(body_part(ARM)),
        mon_nam(mtmp));

    if (mtmp->msleeping) {
        wakeup(mtmp, FALSE, TRUE);
    } else if (mtmp->mfrozen && !rn2(mtmp->mfrozen)) {
        /* After such manhandling, perhaps the effect wears off */
        mtmp->mcanmove = 1;
        mtmp->mfrozen = 0;
        pline("%s stirs.", Monnam(mtmp));
    }

    /* is the monster too heavy? */
    xtra_wt = mtmp->data->cwt;
    if (!try_lift(mtmp, ttmp, xtra_wt, FALSE))
        return 1;

    /* monster without its inventory isn't too heavy; if it carries
       anything, include that minvent weight and check again */
    if (mtmp->minvent) {
        for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
            xtra_wt += otmp->owt;
        if (!try_lift(mtmp, ttmp, xtra_wt, TRUE))
            return 1;
    }

    You("pull %s out of the pit.", mon_nam(mtmp));
    mtmp->mtrapped = 0;
    reward_untrap(ttmp, mtmp);
    fill_pit(mtmp->mx, mtmp->my);
    return 1;
}

staticfn void
disarm_box(struct obj *box, boolean force, boolean confused)
{
    if (box->otrapped) {
        int ch = ACURR(A_DEX) + u.ulevel;

        if (Role_if(PM_ROGUE))
            ch *= 2;
        if (!force && (confused || Fumbling
                       || rnd(75 + level_difficulty() / 2) > ch
                       || Is_mummychest(box))) {
            (void) chest_trap(box, FINGER, TRUE);
            /* 'box' might be gone now */
        } else {
            You("disarm it!");
            box->otrapped = 0;
            box->tknown = 0;
            more_experienced(8, 0);
            newexplevel();
        }
        exercise(A_DEX, TRUE);
    } else {
        pline("That %s was not trapped.", xname(box));
        box->tknown = 0;
    }
}

/* check a particular container for a trap and optionally disarm it */
staticfn void
untrap_box(
    struct obj *box,
    boolean force,
    boolean confused)
{
    if ((box->otrapped
         && (force || (!confused && rn2(MAXULEV + 1 - u.ulevel) < 10)))
        || box->tknown
        || (!force && confused && !rn2(3))) {
        if (!(box->tknown && box->dknown))
            You("find a trap on %s!", the(xname(box)));
        else
            pline("There's a trap on %s.", the(xname(box)));
        box->tknown = 1;
        box->dknown = 1;
        if (!confused)
            exercise(A_WIS, TRUE);

        if (ynq("Disarm it?") == 'y')
            disarm_box(box, force, confused);
    } else {
        You("find no traps on %s.", the(xname(box)));
    }
}

/* hero is able to attempt untrap, so do so */
int
untrap(
    boolean force,
    coordxy rx, coordxy ry,
    struct obj *container)
{
    struct obj *otmp;
    coordxy x, y;
    int ch;
    struct trap *ttmp;
    struct monst *mtmp;
    const char *trapdescr;
    boolean here, useplural, deal_with_floor_trap,
            confused = (Confusion || Hallucination),
            trap_skipped = FALSE, autounlock_door = FALSE;
    int boxcnt = 0;
    char the_trap[BUFSZ], qbuf[QBUFSZ];

    /* 'force' is true for #invoke; if carrying MKoT, make it be true
       for #untrap or autounlock */
    if (!force && has_magic_key(&gy.youmonst))
        force = TRUE;

    if (!rx && !container) {
        /* usual case */
        if (!getdir((char *) 0))
            return 0;
        x = u.ux + u.dx;
        y = u.uy + u.dy;
    } else {
        /* autounlock's untrap; skip most prompting */
        if (container) {
            untrap_box(container, force, confused);
            return 1;
        }
        /* levl[rx][ry] is a locked or trapped door */
        x = rx, y = ry;
        autounlock_door = TRUE;
    }
    if (!isok(x, y)) {
        pline_The("perils lurking there are beyond your grasp.");
        return 0;
    }

    ttmp = t_at(x, y);
    if (ttmp && !ttmp->tseen)
        ttmp = 0;
    trapdescr = ttmp ? trapname(ttmp->ttyp, FALSE) : 0;
    here = u_at(x, y); /* !u.dx && !u.dy */

    if (here) /* are there are one or more containers here? */
        for (otmp = svl.level.objects[x][y]; otmp; otmp = otmp->nexthere)
            if (Is_box(otmp)) {
                if (++boxcnt > 1)
                    break;
            }

    deal_with_floor_trap = can_reach_floor(FALSE);
    if (autounlock_door) {
        ; /* skip a bunch */
    } else if (!deal_with_floor_trap) {
        *the_trap = '\0';
        if (ttmp)
            Strcat(the_trap, an(trapdescr));
        if (ttmp && boxcnt)
            Strcat(the_trap, " and ");
        if (boxcnt)
            Strcat(the_trap, (boxcnt == 1) ? "a container" : "containers");
        useplural = ((ttmp && boxcnt > 0) || boxcnt > 1);
        /* note: boxcnt and useplural will always be 0 for !here case */
        if (ttmp || boxcnt)
            There("%s %s %s but you can't reach %s%s.",
                  useplural ? "are" : "is", the_trap, here ? "here" : "there",
                  useplural ? "them" : "it",
                  u.usteed ? " while mounted" : "");
        trap_skipped = (ttmp != 0);
    } else { /* deal_with_floor_trap */

        if (ttmp) {
            Strcpy(the_trap, the(trapdescr));
            if (boxcnt) {
                if (is_pit(ttmp->ttyp)) {
                    You_cant("do much about %s%s.", the_trap,
                             u.utrap ? " that you're stuck in"
                                     : " while standing on the edge of it");
                    trap_skipped = TRUE;
                    deal_with_floor_trap = FALSE;
                } else {
                    Snprintf(qbuf, sizeof(qbuf),
                             "There %s and %s here.  %s %s?",
                             (boxcnt == 1) ? "is a container"
                                           : "are containers",
                        an(trapdescr),
                             (ttmp->ttyp == WEB) ? "Remove"
                                                 : "Disarm",
                             the_trap);
                    switch (ynq(qbuf)) {
                    case 'q':
                        return 0;
                    case 'n':
                        trap_skipped = TRUE;
                        deal_with_floor_trap = FALSE;
                        break;
                    }
                }
            }
            if (deal_with_floor_trap) {
                if (u.utrap) {
                    You("cannot deal with %s while trapped%s!", the_trap,
                        u_at(x, y) ? " in it" : "");
                    return 1;
                }
                if ((mtmp = m_at(x, y)) != 0
                    && (M_AP_TYPE(mtmp) == M_AP_FURNITURE
                        || M_AP_TYPE(mtmp) == M_AP_OBJECT)) {
                    stumble_onto_mimic(mtmp);
                    return 1;
                }
                switch (ttmp->ttyp) {
                case BEAR_TRAP:
                case WEB:
                    return disarm_holdingtrap(ttmp);
                case LANDMINE:
                    return disarm_landmine(ttmp);
                case SQKY_BOARD:
                    return disarm_squeaky_board(ttmp);
                case DART_TRAP:
                case ARROW_TRAP:
                    return disarm_shooting_trap(ttmp);
                case PIT:
                case SPIKED_PIT:
                    if (here) {
                        You("are already on the edge of the pit.");
                        return 0;
                    }
                    if (!mtmp) {
                        pline("Try filling the pit instead.");
                        return 0;
                    }
                    return help_monster_out(mtmp, ttmp);
                default:
                    You("cannot disable %s trap.", !here ? "that" : "this");
                    return 0;
                }
            }
        } /* end if */

        if (boxcnt) {
            /* 3.7: this used to allow searching for traps on multiple
               containers on the same move and needed to keep track of
               whether any had been found but not attempted to untrap;
               now at most one per move may be checked and we only
               continue on to door handling if they are all declined */
            for (otmp = svl.level.objects[x][y]; otmp; otmp = otmp->nexthere) {
                if (!Is_box(otmp))
                    continue;
                if (otmp->tknown && otmp->dknown)
                    (void) safe_qbuf(qbuf, "Disarm this ", NULL,
                                     otmp, xname, ansimpleoname, "a box");
                else
                    (void) safe_qbuf(qbuf, "There is ",
                                     " here.  Check it for traps?", otmp,
                                     doname, ansimpleoname, "a box");
                switch (ynq(qbuf)) {
                    case 'q':
                        return 0;
                    case 'y':
                        if (otmp->tknown && otmp->dknown)
                            disarm_box(otmp, force, confused);
                        else
                            untrap_box(otmp, force, confused);
                        return 1; /* even for 'no' at "Disarm it?" prompt */
                    }
                    /* 'n' => continue to next box */
            }
            There("are no other chests or boxes here.");
        }

        if (stumble_on_door_mimic(x, y))
            return 1;
    } /* deal_with_floor_trap */

    /*
     * Doors can be manipulated even while levitating/unskilled riding.
     *
     * Ordinarily there won't be a closed or locked door at the same
     * location as a floor trap or a container.  However, there could
     * be a container at a closed/locked door spot if it was dropped
     * there by a monster or poly'd hero with Passes_walls capability,
     * and poly'd hero could move onto that spot and attempt #untrap
     * in direction '.' or '>'.  We'll get here for that situation if
     * player declines to check all containers for traps.
     *
     * The usual situation is #untrap toward an adjacent closed door.
     * No floor trap would be present and any containers would be
     * ignored because they're only checked when direction is '.'/'>'.
     */
    if (!IS_DOOR(levl[x][y].typ)) {
        if (!trap_skipped)
            You("know of no traps there.");
        return 0;
    }

    switch (doorstate(&levl[x][y])) {
    case D_NODOOR:
        You("%s no door there.", Blind ? "feel" : "see");
        return 0;
    case D_BROKEN:
        pline("This door is broken.");
        return 0;
    }

    if ((door_is_trapped(&levl[x][y])
         && (force || (!confused && rn2(MAXULEV - u.ulevel + 11) < 10)))
        || (!force && confused && !rn2(3))) {
        You("find a trap on the door!");
        exercise(A_WIS, TRUE);
        if (ynq("Disarm it?") != 'y')
            return 1;
        if (door_is_trapped(&levl[x][y])) {
            ch = 15 + (Role_if(PM_ROGUE) ? u.ulevel * 3 : u.ulevel);
            exercise(A_DEX, TRUE);
            if (!force && (confused || Fumbling
                           || rnd(75 + level_difficulty() / 2) > ch)) {
                You("slip up...");
                /* don't "set it off"; some traps have no message here */
                (void) alldoortrapped(x, y, &gy.youmonst, FINGER, -D_TRAPPED);
            } else {
                You("disarm it!");
                set_door_trap(&levl[x][y], FALSE);
                more_experienced(8, 0);
                newexplevel();
            }
        } else
            pline("This door was not trapped.");
        return 1;
    } else {
        You("find no traps on the door.");
        return 1;
    }
}

/* for magic unlocking; returns true if targeted monster (which might
   be hero) gets untrapped; the trap remains intact */
boolean
openholdingtrap(
    struct monst *mon,
    boolean *noticed) /* set to true iff hero notices the effect;
                       * otherwise left with its previous value intact */
{
    struct trap *t, tdummy;
    char buf[BUFSZ], whichbuf[20];
    const char *trapdescr = 0, *which = 0;
    boolean ishero = (mon == &gy.youmonst);

    if (!mon)
        return FALSE;
    if (mon == u.usteed)
        ishero = TRUE;

    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);

    if (ishero && u.utrap) { /* all u.utraptype values are holding traps */
        /* there might not be any trap at hero's spot for tt_buriedball;
           conversely, there might be an unrelated trap at that spot */
        if (!t) {
            t = &tdummy;
            (void) memset(t, 0, sizeof *t), t->ntrap = NULL;
            /* fallback 't' is now nonNull, t->tseen and t->madeby_u are 0 */
        }
        which = the_your[(!t || !t->tseen || !t->madeby_u) ? 0 : 1];

        switch (u.utraptype) {
        case TT_LAVA:
            trapdescr = "molten lava";
            break;
        case TT_INFLOOR:
            /* solidified lava, so not "floor" even if within a room */
            trapdescr = "ground";
            break;
        case TT_BURIEDBALL:
            trapdescr = "your anchor";
            which = "";
            break;
        case TT_BEARTRAP:
        case TT_PIT:
        case TT_WEB:
            trapdescr = defsyms[(u.utraptype == TT_WEB) ? S_web
                                : (u.utraptype == TT_PIT) ? S_pit
                                  : S_bear_trap].explanation;
            break;
        default:
            /* lint suppression in case 't' is unexpectedly Null
               or u.utraptype has new value we don't know about yet */
            trapdescr = "trap";
            break;
        }
    } else {
        /* if no trap here or it's not a holding trap, we're done */
        if (!t || (t->ttyp != BEAR_TRAP && t->ttyp != WEB))
            return FALSE;
        trapdescr = trapname(t->ttyp, FALSE);
    }
    assert(t != NULL);
    if (!which)
        which = t->tseen ? the_your[t->madeby_u]
                         : strchr(vowels, *trapdescr) ? "an" : "a";
    assert(which != 0);
    if (*which)
        which = strcat(strcpy(whichbuf, which), " ");

    if (ishero) {
        if (!u.utrap)
            return FALSE;
        *noticed = TRUE;
        if (!u.usteed)
            Strcpy(buf, "You are");
        else if (u.utraptype == TT_BURIEDBALL)
            Sprintf(buf, "You and %s are", y_monnam(u.usteed));
        else
            Sprintf(buf, "%s is", noit_Monnam(u.usteed));
        /* give release message before untrap in case it triggers a message */
        pline("%s released from %s%s.", buf, which, trapdescr);
        /* might float up if Levitation is being unblocked */
        gv.vision_full_recalc = 1; /* vision limits can change (pit escape) */
        reset_utrap(TRUE);
        if (gv.vision_full_recalc)
            vision_recalc(0);
    } else {
        if (!mon->mtrapped)
            return FALSE;
        mon->mtrapped = 0;
        if (canspotmon(mon)) {
            *noticed = TRUE;
            pline("%s is released from %s%s.", Monnam(mon), which,
                  trapdescr);
        } else if (cansee(t->tx, t->ty) && t->tseen) {
            *noticed = TRUE;
            if (t->ttyp == WEB)
                pline("%s is released from %s%s.", Something, which,
                      trapdescr);
            else /* BEAR_TRAP */
                pline("%s%s opens.", upstart(strcpy(buf, which)), trapdescr);
        }
        /* might pacify monster if adjacent */
        if (rn2(2) && m_next2u(mon))
            reward_untrap(t, mon);
    }
    return TRUE;
}

/* for magic locking; returns true if targeted monster (which might
   be hero) gets hit by a trap (might avoid actually becoming trapped) */
boolean
closeholdingtrap(
    struct monst *mon,
    boolean *noticed) /* set to true iff hero notices the effect;
                       * otherwise left with its previous value intact */
{
    struct trap *t;
    unsigned dotrapflags;
    boolean ishero = (mon == &gy.youmonst), result;

    if (!mon)
        return FALSE;
    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a holding trap, we're done */
    if (!t || (t->ttyp != BEAR_TRAP && t->ttyp != WEB))
        return FALSE;

    if (ishero) {
        if (u.utrap)
            return FALSE; /* already trapped */
        *noticed = TRUE;
        dotrapflags = FORCETRAP;
        /* dotrap calls mintrap when mounted hero encounters a web */
        if (u.usteed)
            dotrapflags |= NOWEBMSG;
        dotrap(t, dotrapflags | FORCETRAP);
        result = (u.utrap != 0);
    } else {
        if (mon->mtrapped)
            return FALSE; /* already trapped */
        /* you notice it if you see the trap close/tremble/whatever
           or if you sense the monster who becomes trapped */
        *noticed = cansee(t->tx, t->ty) || canspotmon(mon);
        result = (mintrap(mon, FORCETRAP) != Trap_Effect_Finished);
    }
    return result;
}

/* for magic unlocking; returns true if targeted monster (which might
   be hero) gets hit by a trap (target might avoid its effect) */
boolean
openfallingtrap(
    struct monst *mon,
    boolean trapdoor_only,
    boolean *noticed) /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    boolean ishero = (mon == &gy.youmonst), result;

    if (!mon)
        return FALSE;
    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a falling trap, we're done
       (note: falling rock traps have a trapdoor in the ceiling) */
    if (!t || ((t->ttyp != TRAPDOOR && t->ttyp != ROCKTRAP)
               && (trapdoor_only || (t->ttyp != HOLE && !is_pit(t->ttyp)))))
        return FALSE;

    if (ishero) {
        if (u.utrap)
            return FALSE; /* already trapped */
        *noticed = TRUE;
        dotrap(t, FORCETRAP);
        result = (u.utrap != 0);
    } else {
        if (mon->mtrapped)
            return FALSE; /* already trapped */
        /* you notice it if you see the trap close/tremble/whatever
           or if you sense the monster who becomes trapped */
        *noticed = cansee(t->tx, t->ty) || canspotmon(mon);
        /* monster will be angered; mintrap doesn't handle that */
        wakeup(mon, TRUE, TRUE);
        result = (mintrap(mon, FORCETRAP) != Trap_Effect_Finished);
        /* mon might now be on the migrating monsters list */
    }
    return result;
}

/* only called when the player is doing something to the chest directly */
boolean
chest_trap(
    struct obj *obj,
    int bodypart,
    boolean disarm)
{
    struct obj *otmp = obj, *otmp2;
    char buf[80];
    const char *msg;
    coord cc;

    if (get_obj_location(obj, &cc.x, &cc.y, 0)) /* might be carried */
        obj->ox = cc.x, obj->oy = cc.y;

    otmp->tknown = 0;
    otmp->otrapped = 0; /* trap is one-shot; clear flag first in case
                           chest kills you and ends up in bones file */
    You(disarm ? "set it off!" : "trigger a trap!");
    display_nhwindow(WIN_MESSAGE, FALSE);

    /* only trigger this if mummies are not all genocided/extinct; otherwise do
     * a normal chest trap */
    if (Is_mummychest(obj) && mkclass(S_MUMMY, 0)) {
        int i, mumnum = 4 + rn2(3);
        boolean seen = FALSE;
        obj->spe = 0; /* revert to normal chest in case it ever becomes possible
                         to re-trap chests */
        for (i = 0; i < mumnum; ++i) {
            struct permonst *mptr = mkclass(S_MUMMY, 0);
            struct monst *mon;
            if (mptr && mptr->difficulty < mons[PM_HUMAN_MUMMY].difficulty) {
                mptr = &mons[PM_HUMAN_MUMMY];
            }
            mon = makemon(mptr, u.ux, u.uy, MM_ADJACENTOK | MM_ANGRY);
            if (mon && canspotmon(mon)) {
                seen = TRUE;
            }
        }
        You_hear("a horrible groaning sound.");
        if (!Blind && seen) {
            pline("Mummies emerge from hidden recesses!");
        }
        return FALSE; /* obj not destroyed */
    }

    if (Luck > -13 && rn2(13 + Luck) > 7) { /* saved by luck */
        /* trap went off, but good luck prevents damage */
        switch (rn2(13)) {
        case 12:
        case 11:
            msg = "explosive charge is a dud";
            break;
        case 10:
        case 9:
            msg = "electric charge is grounded";
            break;
        case 8:
        case 7:
            msg = "flame fizzles out";
            break;
        case 6:
        case 5:
        case 4:
            msg = "poisoned needle misses";
            break;
        case 3:
        case 2:
        case 1:
        case 0:
            msg = "gas cloud blows away";
            break;
        default:
            impossible("chest disarm bug");
            msg = (char *) 0;
            break;
        }
        if (msg)
            pline("But luckily the %s!", msg);
    } else {
        switch (rn2(20) ? ((Luck >= 13) ? 0 : rn2(13 - Luck)) : rn2(26)) {
        case 25:
        case 24:
        case 23:
        case 22:
        case 21: {
            struct monst *shkp = 0;
            long loss = 0L;
            boolean costly, insider;
            coordxy ox = obj->ox, oy = obj->oy;

            /* the obj location need not be that of player */
            costly = (costly_spot(ox, oy)
                      && (shkp = shop_keeper(*in_rooms(ox, oy, SHOPBASE)))
                             != (struct monst *) 0);
            insider = (*u.ushops && inside_shop(u.ux, u.uy)
                       && *in_rooms(ox, oy, SHOPBASE) == *u.ushops);

            pline("%s!", Tobjnam(obj, "explode"));
            Sprintf(buf, "exploding %s", xname(obj));

            if (costly)
                loss += stolen_value(obj, ox, oy, (boolean) shkp->mpeaceful,
                                     TRUE);
            delete_contents(obj);
            /*
             * Note:  the explosion is taking place at the chest's
             * location, not necessarily at the hero's.  (Simplest
             * case: kicking it from one step away and getting the
             * chest_trap() outcome.)
             */
            /* unpunish() in advance if either ball or chain (or both)
               is going to be destroyed */
            if (Punished && ((uchain->ox == ox && uchain->oy == oy)
                             || (uball->where == OBJ_FLOOR
                                 && uball->ox == ox && uball->oy == oy)))
                unpunish();
            /* destroy everything at the spot (the Amulet, the
               invocation tools, and Rider corpses will remain intact) */
            for (otmp = svl.level.objects[ox][oy]; otmp; otmp = otmp2) {
                otmp2 = otmp->nexthere;
                if (costly)
                    loss += stolen_value(otmp, otmp->ox, otmp->oy,
                                         (boolean) shkp->mpeaceful, TRUE);
                delobj(otmp);
            }
            wake_nearby(FALSE);
            losehp(Maybe_Half_Phys(d(6, 6)), buf, KILLED_BY_AN);
            exercise(A_STR, FALSE);
            if (costly && loss) {
                if (insider)
                    You("owe %ld %s for objects destroyed.", loss,
                        currency(loss));
                else {
                    You("caused %ld %s worth of damage!", loss,
                        currency(loss));
                    make_angry_shk(shkp, ox, oy);
                }
            }
            return TRUE;
        } /* case 21 */
        case 20:
        case 19:
        case 18:
        case 17:
            pline("A cloud of noxious gas billows from %s.", the(xname(obj)));
            if (rn2(3))
                poisoned("gas cloud", A_STR, "cloud of poison gas", 15,
                         FALSE);
            else
                create_gas_cloud(obj->ox, obj->oy, 1, 8);
            exercise(A_CON, FALSE);
            break;
        case 16:
        case 15:
        case 14:
        case 13:
            You_feel("a needle prick your %s.", body_part(bodypart));
            poisoned("needle", A_CON, "poisoned needle", 10, FALSE);
            exercise(A_CON, FALSE);
            break;
        case 12:
        case 11:
        case 10:
        case 9:
            dofiretrap(obj);
            break;
        case 8:
        case 7:
        case 6: {
            int dmg = d(4, 4), orig_dmg = dmg;

            You("are jolted by a surge of electricity!");
            if (Shock_resistance) {
                shieldeff(u.ux, u.uy);
                You("don't seem to be affected.");
                monstseesu(M_SEEN_ELEC);
                dmg = 0;
            } else {
                monstunseesu(M_SEEN_ELEC);
            }
            (void) destroy_items(&gy.youmonst, AD_ELEC, orig_dmg);
            if (dmg)
                losehp(dmg, "electric shock", KILLED_BY_AN);
            break;
        } /* case 6 */
        case 5:
        case 4:
        case 3:
            make_paralyzed(d(5, 6), TRUE, "frozen by a trap");
            break;
        case 2:
        case 1:
        case 0:
            pline("A cloud of %s gas billows from %s.",
                  Blind ? ROLL_FROM(blindgas) : rndcolor(),
                  the(xname(obj)));
            if (!Stunned) {
                if (Hallucination)
                    pline("What a groovy feeling!");
                else
                    You("%s%s...", stagger(gy.youmonst.data, "stagger"),
                        Halluc_resistance ? ""
                                          : Blind ? " and get dizzy"
                                                  : " and your vision blurs");
            }
            make_stunned((HStun & TIMEOUT) + (long) rn1(7, 16), FALSE);
            (void) make_hallucinated(
                (HHallucination & TIMEOUT) + (long) rn1(5, 16), FALSE, 0L);
            break;
        default:
            impossible("bad chest trap");
            break;
        }
        bot(); /* to get immediate botl re-display */
    }

    return FALSE;
}

struct trap *
t_at(coordxy x, coordxy y)
{
    struct trap *trap = gf.ftrap;

    while (trap) {
        if (trap->tx == x && trap->ty == y)
            return trap;
        trap = trap->ntrap;
    }
    return (struct trap *) 0;
}

/* return number of traps of type ttyp on this level */
int
count_traps(int ttyp)
{
    int ret = 0;
    struct trap *trap = gf.ftrap;

    while (trap) {
        if ((int) trap->ttyp == ttyp)
            ret++;
        trap = trap->ntrap;
    }

    return ret;
}

void
deltrap(struct trap *trap)
{
    struct trap *ttmp;

    if (trap->ammo) {
        impossible("deleting trap (%d) containing ammo (%d)?",
                   trap->ttyp, trap->ammo->otyp);
        /* deltrap (here) -> deltrap_with_ammo (destroys ammo) -> deltrap */
        deltrap_with_ammo(trap, DELTRAP_DESTROY_AMMO);
        return;
    }
    clear_conjoined_pits(trap);
    if (trap == gf.ftrap) {
        gf.ftrap = gf.ftrap->ntrap;
    } else {
        for (ttmp = gf.ftrap; ttmp; ttmp = ttmp->ntrap)
            if (ttmp->ntrap == trap)
                break;
        if (!ttmp)
            panic("deltrap: no preceding trap!");
        ttmp->ntrap = trap->ntrap;
    }
    if (Sokoban && (trap->ttyp == PIT || trap->ttyp == HOLE))
        maybe_finish_sokoban();
    dealloc_trap(trap);
}

/* Delete a trap, but handle any ammo in it.
 * The values for do_what are the DELTRAP_*_AMMO constants.
 * If called with a trap without ammo, this should function like deltrap.
 * If called with DELTRAP_RETURN_AMMO, delete the trap but preserve the ammo as
 * an object chain, and return it. */
struct obj *
deltrap_with_ammo(struct trap *trap, int do_what)
{
    struct obj *otmp = (struct obj *) 0;
    struct obj *objchn = (struct obj *) 0;
    coordxy tx, ty;
    if (!trap) {
        impossible("deltrap_with_ammo: null trap!");
        return NULL;
    }
    tx = trap->tx;
    ty = trap->ty;
    while (trap->ammo) {
        otmp = trap->ammo;
        extract_nobj(otmp, &trap->ammo);
        if (objchn) {
            otmp->nobj = objchn;
        }
        objchn = otmp;
    }
    if (do_what == DELTRAP_DESTROY_AMMO) {
        set_trap_ammo(trap, (struct obj *) 0);
    }
    else if (do_what != DELTRAP_RETURN_AMMO) {
        struct obj *nobj;
        otmp = objchn;
        while (otmp) {
            nobj = otmp->nobj;
            switch (do_what) {
            default:
                impossible("Bad deltrap constant! Placing ammo instead");
                FALLTHROUGH;
                /* FALLTHRU */
            case DELTRAP_PLACE_AMMO:
                place_object(otmp, trap->tx, trap->ty);
                /* Sell your own traps only... */
                if (trap->madeby_u)
                    sellobj(otmp, trap->tx, trap->ty);
                stackobj(otmp);
                break;
            case DELTRAP_BURY_AMMO:
                place_object(otmp, trap->tx, trap->ty);
                (void) bury_an_obj(otmp, NULL);
                break;
            case DELTRAP_TAKE_AMMO:
                hold_another_object(otmp, "You remove, but drop, %s.",
                                    doname(otmp), NULL);
                break;
            }
            otmp = nobj;
        }
        objchn = NULL;
    }
    if (u.utrap && u_at(trap->tx, trap->ty))
        reset_utrap(TRUE);
    deltrap(trap);
    newsym(tx, ty);
    return objchn;
}

boolean
conjoined_pits(
    struct trap *trap2,
    struct trap *trap1,
    boolean u_entering_trap2)
{
    coordxy dx, dy;
    int diridx, adjidx;

    if (!trap1 || !trap2)
        return FALSE;
    if (!isok(trap2->tx, trap2->ty) || !isok(trap1->tx, trap1->ty)
        || !is_pit(trap2->ttyp)
        || !is_pit(trap1->ttyp)
        || (u_entering_trap2 && !(u.utrap && u.utraptype == TT_PIT)))
        return FALSE;
    dx = sgn(trap2->tx - trap1->tx);
    dy = sgn(trap2->ty - trap1->ty);
    diridx = xytod(dx, dy);
    if (diridx != DIR_ERR) {
        adjidx = DIR_180(diridx);
        if ((trap1->conjoined & (1 << diridx))
            && (trap2->conjoined & (1 << adjidx)))
            return TRUE;
    }
    return FALSE;
}

staticfn void
clear_conjoined_pits(struct trap *trap)
{
    int diridx, adjidx;
    coordxy x, y;
    struct trap *t;

    if (trap && is_pit(trap->ttyp)) {
        for (diridx = 0; diridx < N_DIRS; ++diridx) {
            if (trap->conjoined & (1 << diridx)) {
                x = trap->tx + xdir[diridx];
                y = trap->ty + ydir[diridx];
                if (isok(x, y)
                    && (t = t_at(x, y)) != 0
                    && is_pit(t->ttyp)) {
                    adjidx = DIR_180(diridx);
                    t->conjoined &= ~(1 << adjidx);
                }
                trap->conjoined &= ~(1 << diridx);
            }
        }
    }
}

staticfn boolean
adj_nonconjoined_pit(struct trap *adjtrap)
{
    struct trap *trap_with_u = t_at(u.ux0, u.uy0);

    if (trap_with_u && adjtrap && u.utrap && u.utraptype == TT_PIT
        && is_pit(trap_with_u->ttyp) && is_pit(adjtrap->ttyp)) {
        if (xytod(u.dx, u.dy) != DIR_ERR)
            return TRUE;
    }
    return FALSE;
}

#if 0
/*
 * Mark all neighboring pits as conjoined pits.
 * (currently not called from anywhere)
 */
staticfn void
join_adjacent_pits(struct trap *trap)
{
    struct trap *t;
    int diridx;
    coordxy x, y;

    if (!trap)
        return;
    for (diridx = 0; diridx < N_DIRS; ++diridx) {
        x = trap->tx + xdir[diridx];
        y = trap->ty + ydir[diridx];
        if (isok(x, y)) {
            if ((t = t_at(x, y)) != 0 && is_pit(t->ttyp)) {
                trap->conjoined |= (1 << diridx);
                join_adjacent_pits(t);
            } else
                trap->conjoined &= ~(1 << diridx);
        }
    }
}
#endif /*0*/

/*
 * Returns TRUE if you escaped a pit and are standing on the precipice.
 */
boolean
uteetering_at_seen_pit(struct trap *trap)
{
    return (trap && is_pit(trap->ttyp) && trap->tseen
            && u_at(trap->tx, trap->ty)
            && !(u.utrap && u.utraptype == TT_PIT));
}

/*
 * Returns TRUE if you didn't fall through a hole or didn't
 * release a trap door
 */
boolean
uescaped_shaft(struct trap *trap)
{
    return (trap && is_hole(trap->ttyp) && trap->tseen
            && u_at(trap->tx, trap->ty));
}

/* Destroy a trap that emanates from the floor. */
boolean
delfloortrap(struct trap *ttmp)
{
    /* some of these are arbitrary -dlc */
    if (ttmp && ((ttmp->ttyp == SQKY_BOARD) || (ttmp->ttyp == BEAR_TRAP)
                 || (ttmp->ttyp == LANDMINE) || (ttmp->ttyp == FIRE_TRAP)
                 || (ttmp->ttyp == COLD_TRAP)
                 || is_pit(ttmp->ttyp)
                 || is_hole(ttmp->ttyp)
                 || (ttmp->ttyp == TELEP_TRAP) || (ttmp->ttyp == LEVEL_TELEP)
                 || (ttmp->ttyp == WEB) || (ttmp->ttyp == MAGIC_TRAP)
                 || (ttmp->ttyp == ANTI_MAGIC))) {
        struct monst *mtmp;

        if (u_at(ttmp->tx, ttmp->ty)) {
            if (u.utraptype != TT_BURIEDBALL)
                reset_utrap(TRUE);
        } else if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
            mtmp->mtrapped = 0;
        }
        /* For the two types of ammo-bearing floor traps (land mine and bear
         * trap), it's ambiguous whether this should destroy the ammo or place
         * it. Since this is currently only called during gameplay (usually when
         * this space gets flooded), assume placing it; if this ever gets called
         * in level generation or something, it may result in the objects
         * getting left around the map where they shouldn't be. */
        deltrap_with_ammo(ttmp, DELTRAP_PLACE_AMMO);
        return TRUE;
    }
    return FALSE;
}

/* Given a door location, return the trap type associated with it. */
int
getdoortrap(int x, int y)
{
    int lvl = level_difficulty();
    int maxtrap = -1;
    const int trapminlevels[NUMDOORTRAPS][2] = {
        { HINGE_SCREECH,      1 }, /* keep these sorted */
        { SELF_LOCK,          1 },
        { STATIC_SHOCK,       2 },
        { WATER_BUCKET,       3 },
        { HINGELESS_FORWARD,  6 },
        { HINGELESS_BACKWARD, 8 },
        { ROCKFALL,          10 },
        { HOT_KNOB,          12 },
        { FIRE_BLAST,        15 }};

    /* Find the maximum possible effect for a trap, based on level. */
    int i;
    for (i = 0; i < NUMDOORTRAPS; ++i) {
        if (lvl >= trapminlevels[i][1]) {
            maxtrap = i;
        }
        else break;
    }
    if (maxtrap < 0) {
        impossible("getdoortrap: no valid traps");
        return 0;
    }

    /* trap is picked deterministically based on door's coordinates, so that
     * repeat traps on the same door will be the same trap.
     * In order to provide some extra scrambling that the player won't be able
     * to get out of the visible game state, involve birthday as well. */
    return (int) (coord_hash(x, y, ledger_no(&u.uz)) % (maxtrap + 1));
}

/* Interacting with a door triggers a trap.
 *
 * mon is the monster triggering the trap (youmonst means player; null means
 * caller doesn't know who the culprit is, which happens a lot if the trap is
 * triggered by someone zapping something, so make a best guess based on
 * context.mon_moving)
 *
 * bodypart is the body part used to touch the door, and can affect what the
 * trap does. Note: ARM is sometimes used to denote touching the door with an
 * object (i.e. chopping the door down).
 *
 * action is a doorstate that defines what the player is doing with the door:
 *   D_ISOPEN - opening it
 *   D_CLOSED - closing it, not wizard-locking it
 *   D_BROKEN - destroying it
 *   D_LOCKED - locking it
 *  -D_LOCKED - unlocking it
 *  -D_TRAPPED - untrapping it
 *   D_NODOOR - none of the above; this is for a trap that triggers when the
 *     player moves off the doorway onto another space. (Unimplemented.)
 * Unused ones that might be of some use at some point:
 *  -D_NODOOR - moving onto the doorway. (Note that D_NODOOR is 0 so this won't
 *    work.)
 *
 * "when" is a combination of DOOR_TRAP_[PRE|POST] flags documented in trap.h.
 *
 * Return value is one of the DOORTRAPPED_* constants documented in trap.h.
 */
xint8
doortrapped(int x, int y, struct monst * mon, int bodypart, int action,
            xint8 when)
{
    boolean before = (when & DOOR_TRAP_PRE);
    boolean after = (when & DOOR_TRAP_POST);
    boolean byu = (mon == &gy.youmonst || (mon == (struct monst *) 0
                                           && !svc.context.mon_moving));
    boolean touching = (bodypart != NO_PART);
    /* note that touching represents either you or the monster touching the
     * door and does not mean "the player touching". */
    boolean canseemon = ((byu || (mon && cansee(mon->mx, mon->my)))
                         && !Unaware);
    /* also assume that it's impossible for the player to trigger a door trap
     * while unaware, so assume byu implies !Unaware */
    boolean canseedoor = (cansee(x,y) && !Unaware);
    int selected_trap = getdoortrap(x, y);
    int lvl = level_difficulty();
    int dmg = 0;
    struct rm * door = &levl[x][y];
    uchar saved_doorstate = doorstate(door);

    if (door->typ != DOOR && door->typ != SDOOR) {
        impossible("doortrapped: called on a non-door");
        return DOORTRAPPED_NOCHANGE;
    }
    if (selected_trap < HINGE_SCREECH || selected_trap >= NUMDOORTRAPS) {
        impossible("doortrapped: bad trap %d", selected_trap);
        return DOORTRAPPED_NOCHANGE;
    }
    if (action != D_NODOOR && action != D_ISOPEN && action != D_BROKEN
        && action != D_CLOSED && action != -D_TRAPPED
        && action != -D_LOCKED && action != D_LOCKED) {
        impossible("doortrapped: bad action %d", action);
        return DOORTRAPPED_NOCHANGE;
    }
    /* ok to call this on a non-trapped door, since it lets us call the
     * function unconditionally whenever something interacts with a door */
    if (!door_is_trapped(door)) {
        return DOORTRAPPED_NOCHANGE;
    }

    if (mon == (struct monst *) 0) {
        if (byu) {
            /* useful for calling player/monster agnostic stuff later;
             * also ensures that after this block, !mon means there isn't a
             * monster who can be directly affected, so we can outright exclude
             * certain traps */
            mon = &gy.youmonst;
        }
    }
    else if (mon != &gy.youmonst) {
        mon->mtrapseen |= (1 << (TRAPPED_DOOR - 1));
    }

    /* It is safe to do a direct return in any of these conditions IF the
     * condition contains zero chance of destroying the door. */
    if (selected_trap == HINGE_SCREECH && after
        && (action == D_ISOPEN || action == D_CLOSED)) {
        int range = (11 * lvl) + (11 * lvl);
        if (!Deaf) {
            if (byu || canseedoor) {
                pline_xy(x, y, "The hinges screech loudly.");
            }
            else {
                You_hear("a %s screech.",
                        distu(x, y) < range ? "faint" : "nearby");
            }
        }
        wake_nearto(x, y, range);
        /* trap not disarmed */
    }
    else if (selected_trap == SELF_LOCK && after) {
        boolean do_lock = FALSE;
        if (saved_doorstate == D_BROKEN || action == D_BROKEN) {
            /* possible to hit this with a door already broken by digging;
             * obviously the trap won't do anything any longer */
            set_door_trap(door, FALSE);
            return DOORTRAPPED_NOCHANGE;
        }
        if (action == D_ISOPEN) {
            if (byu || canseedoor) {
                pline_xy(x, y, "But it swings closed again!");
            }
            do_lock = TRUE;
        }
        else if (action == D_CLOSED ||
                 (action == -D_TRAPPED && door_is_closed(door))) {
            do_lock = TRUE;
        }
        else if (action == D_NODOOR) {
            if (byu || canseedoor) {
                pline_xy(x, y, "The door swings itself shut!");
            }
            do_lock = TRUE;
        }
        else if (action == -D_LOCKED) {
            if (byu && bodypart == FINGER) {
                You("disarm a self-locking mechanism.");
            }
            set_door_trap(door, FALSE);
            return DOORTRAPPED_NOCHANGE; /* still closed */
        }
        /* no case for action == D_LOCKED
         * not much point doing anything when the player *wants* to lock it */
        if (do_lock) {
            if (!Deaf && canseedoor) {
                You_hear("the lock click by itself!");
            }
            set_doorstate(door, D_CLOSED);
            set_door_lock(door, TRUE);
            if (byu)
                feel_newsym(x, y); /* the hero knows it is closed */
            block_point(x, y); /* vision: no longer see there */
            return DOORTRAPPED_CHANGED;
        }
        return DOORTRAPPED_NOCHANGE;
    }
    else if (selected_trap == STATIC_SHOCK && before && bodypart == FINGER
             && mon && (action == D_ISOPEN || action == D_CLOSED
                        || action == -D_LOCKED || action == -D_TRAPPED)) {
        boolean resists = (byu ? Shock_resistance : resists_elec(mon));
        dmg = rnd(lvl * 2) / (resists ? 4 : 1);
        dmg += 1;
        if (byu) {
            pline_xy(x, y, "An electric spark from the doorknob zaps you!");
            losehp(dmg, "charged doorknob", KILLED_BY_AN);
            exercise(A_WIS, FALSE);
        }
        else {
            if (canseemon) {
                pline_mon(mon, "%s is zapped by a doorknob!", Monnam(mon));
            }
            mon->mhp -= dmg;
            if (DEADMONSTER(mon)) {
                monkilled(mon, "", AD_ELEC);
            }
        }
        /* trap goes away */
        set_door_trap(door, FALSE);
    }
    else if (selected_trap == WATER_BUCKET && after
             && (action == D_ISOPEN || action == D_BROKEN
                 || action == -D_TRAPPED)) {
        if (canseedoor) {
            pline_xy(x, y, "A bucket of water splashes down on %s!",
                     ((!touching || !mon) ? "the floor"
                                          : (byu ? "you" : mon_nam(mon))));
        }
        else {
            You_hear("a distant splash.");
        }
        if (touching) {
            if (byu) {
                /* TODO: no iron golem rust/gremlin multiplying as of yet,
                 * waiting to hear from DT on this */
                water_damage_chain(gi.invent, FALSE, (lvl/5)+1, FALSE);
                exercise(A_WIS, FALSE);
            }
            else if (mon) {
                water_damage_chain(mon->minvent, FALSE, (lvl/5)+1, FALSE);
            }
        }
        set_door_trap(door, FALSE); /* trap is gone */
    }
    else if ((selected_trap == HINGELESS_FORWARD && before
              && (action == D_ISOPEN || action == D_BROKEN
                  || action == -D_TRAPPED))
             /* if you're trying to *break* the door, and it doesn't have
              * hinges, it's going to fall backward regardless of which way it
              * was rigged. */
             || (selected_trap == HINGELESS_BACKWARD && before
                 && action == D_BROKEN)) {
        dmg = rnd((lvl/4) + 1);
        /* necessary to set this up front; otherwise we hurtle into the closed
         * door and don't actually move */
        set_doorstate(door, D_BROKEN);
        recalc_block_point(x, y);
        if (byu) {
            pline_xy(x, y, "The door %s forward off its hinges!",
                     (action == D_BROKEN ? "is knocked" : "falls"));
            if (touching) {
                You("crash on top of it!");
                /* move onto the door */
                hurtle(x - u.ux, y - u.uy, 1, FALSE);
                gm.multi_reason = "falling on top of a door";

                make_stunned((HStun & TIMEOUT) + d(2,4), FALSE);
                losehp(Maybe_Half_Phys(dmg), "falling on top of a door",
                        KILLED_BY);
                exercise(A_STR, FALSE);
            }
        }
        else {
            if (canseedoor) {
                You_see("a door %s off its hinges!",
                        (action == D_BROKEN ? "smashed" : "fall"));
            }
            if (mon && touching) {
                mhurtle(mon, x - mon->mx, y - mon->my, 1);
                if (canseedoor) { /* not canseemon: mon is on door now anyway */
                    pline_mon(mon, "%s crashes on top of it!", Monnam(mon));
                }
                else {
                    You_hear("a %s crash.", distu(x, y) > 7 * 7 ? "distant"
                                                                : "nearby");
                }
                mon->mstun = 1;
                mon->mhp -= dmg;
                if (DEADMONSTER(mon)) {
                    monkilled(mon, "", AD_PHYS);
                }
            }
        }
        set_door_trap(door, FALSE); /* trap is gone */
        wake_nearto(x, y, 8 * 8);
    }
    else if (selected_trap == HINGELESS_BACKWARD && before
             && (action == D_ISOPEN || action == -D_TRAPPED)) {
        dmg = rnd((lvl/2) + 1);
        if (byu) {
            pline_xy(x, y, "The door falls backward off its hinges!");
            if (touching) {
                You("are flattened by it!");
                losehp(Maybe_Half_Phys(dmg), "crushed by a falling door",
                        NO_KILLER_PREFIX);
                /* takes a while to get out from the door */
                nomul(-3);
                gm.multi_reason = "trapped under a door";
                exercise(A_STR, FALSE);
            }
        }
        else {
            if (canseedoor) {
                You_see("a door fall off its hinges!");
            }
            if (touching && mon) {
                if (canseemon) {
                    pline_mon(mon, "It crashes on top of %s!", Monnam(mon));
                }
                else {
                    You_hear("a nearby crash.");
                }
                mon->mcanmove = 0;
                mon->mfrozen = 3;
                mon->mhp -= dmg;
                if (DEADMONSTER(mon)) {
                    monkilled(mon, "", AD_PHYS);
                }
            }
        }
        set_door_trap(door, FALSE); /* trap is gone */
        set_doorstate(door, D_BROKEN);
        wake_nearto(x, y, 8 * 8);
    }
    else if (selected_trap == ROCKFALL && after
             && (action == D_ISOPEN || action == D_BROKEN
                 || action == -D_TRAPPED)) {
        if (canseedoor) {
            You("see a tripwire snap!");
        }
        You_hear("something rumbling in the ceiling.");
        if (touching && mon) {
            if (byu) {
                drop_boulder_on_player(FALSE, FALSE, FALSE, FALSE);
                exercise(A_STR, FALSE);
            }
            else {
                drop_boulder_on_monster(mon->mx, mon->my, FALSE, FALSE);
            }
        }
        else {
            /* drop on door square */
            drop_boulder_on_monster(x, y, FALSE, FALSE);
        }
        set_door_trap(door, FALSE); /* trap is gone */
    }
    else if (selected_trap == HOT_KNOB && before && bodypart == FINGER
             && mon && (action == D_ISOPEN || action == D_CLOSED
                        || action == -D_TRAPPED)) {
        dmg = rnd(lvl);
        struct obj* gloves = which_armor(mon, W_ARMG);
        if (byu ? Fire_resistance : resists_fire(mon)) {
            dmg /= 2;
            dmg += 1;
        }
        /* gloves give half damage but burn if burnable;
         * padded gloves are designed for heat and negate all damage */
        if (gloves) {
            dmg /= 2;
            if (objdescr_is(gloves, "padded gloves")) {
                dmg = 0;
            }
        }
        if (byu) {
            pline("Ouch!  The knob is red-hot!");
            if (dmg <= 0) {
                pline("Fortunately, your glove protects your %s.",
                      body_part(HAND));
            }
            else {
                losehp(dmg, "a red-hot doorknob", KILLED_BY);
            }
        }
        else {
            if (canseemon && dmg) {
                pline_mon(mon, "%s is burned by a red-hot doorknob!",
                          Monnam(mon));
            }
            mon->mhp -= dmg;
            if (DEADMONSTER(mon)) {
                monkilled(mon, "", AD_FIRE);
            }
        }

        /* put this after just so a death won't be preceded by a misleading
            * message about gloves burning */
        if (gloves) {
            erode_obj(gloves, NULL, ERODE_BURN, EF_GREASE);
        }
    }
    else if (selected_trap == FIRE_BLAST && after
             && (action == D_ISOPEN || action == D_BROKEN
                 || action == -D_TRAPPED)) {
        int dam = rnd(lvl);
        if (door_is_iron(door)) {
            /* more damage for iron doors, since flying pieces of metal are more
             * damaging */
            dam *= 3;
        }
        if (byu || canseedoor) {
            /* if !byu, this will be preceded by "You see a door open." or "Foo
             * opens a door." */
            Soundeffect(se_kaboom_door_explodes, 75);
            pline_xy(x, y, "KABOOM!!  The door was booby-trapped!");
        }
        else {
            Soundeffect(se_explosion, 75);
            You_hear("a %s explosion.",
                     (distu(x, y) > 7 * 7) ? "distant"
                                           : "nearby");
        }
        /* need to remove trap now because otherwise explode() will destroy the
         * door and call doortrapped() again */
        set_door_trap(door, FALSE); /* trap is gone */
        explode(x, y, 11, rnd(lvl), EXPLODING_DOOR, EXPL_FIERY);
        /* wooden doors will be consumed in flames and don't need to set
         * doorstate; however, iron ones won't burn so we need to set it */
        set_doorstate(door, D_NODOOR);
    }
    if ((saved_doorstate != D_NODOOR && saved_doorstate != D_BROKEN)
        && (doorstate(door) == D_NODOOR || doorstate(door) == D_BROKEN)) {
        /* door was destroyed during this function */
        if (*in_rooms(x, y, SHOPBASE)) {
            /* always add to shk fix list, but only add to player cost if the
             * player is responsible */
            add_damage(x, y, (byu ? SHOP_DOOR_COST : 0L));
        }
        newsym(x, y);
        recalc_block_point(x, y); /* can now see through it */
        return DOORTRAPPED_DESTROYED;
    }
    else if (saved_doorstate != doorstate(door)) {
        newsym(x, y);
        recalc_block_point(x, y); /* take care of vision */
        return DOORTRAPPED_CHANGED;
    }
    else {
        return DOORTRAPPED_NOCHANGE;
    }
}

/* Monster is hit by trap. */
staticfn boolean
thitm(
    int tlev,          /* missile's attack level */
    struct monst *mon, /* target */
    struct obj *obj,   /* missile; might be Null */
    int d_override,    /* non-zero: force hit for this amount of damage */
    boolean nocorpse)  /* True: a trap is completely burning up the target */
{
    int strike;
    boolean trapkilled = FALSE;

    if (d_override)
        strike = 1;
    else if (obj)
        strike = (find_mac(mon) + tlev + obj->spe <= rnd(20));
    else
        strike = (find_mac(mon) + tlev <= rnd(20));

    /* Actually more accurate than thitu, which doesn't take
     * obj->spe into account.
     */
    if (!strike) {
        if (obj && cansee(mon->mx, mon->my))
            pline_mon(mon, "%s is almost hit by %s!",
                      Monnam(mon), doname(obj));
    } else {
        int dam = 1;
        boolean harmless = (obj && stone_missile(obj)
                            && passes_rocks(mon->data));

        if (obj && cansee(mon->mx, mon->my))
            pline_mon(mon, "%s is hit by %s%s",
                      Monnam(mon), doname(obj),
                      harmless ? " but is not harmed." : "!");
        if (d_override) {
            dam = d_override;
        } else if (obj) {
            dam = dmgval(obj, mon);
            if (dam < 1)
                dam = 1;
            if (mon_hates_material(mon, obj->material)) {
                /* extra damage already applied by dmgval() */
                searmsg(NULL, mon, obj, TRUE);
            }
        }
        if (!harmless) {
            mon->mhp -= dam;
            if (mon->mhp <= 0) {
                int xx = mon->mx, yy = mon->my;

                /* If a monster dies in a trap on the player's turn (e.g. forced
                 * onto one by jousting or staggering blow), the player is probably
                 * responsible. */
                if (svc.context.mon_moving) {
                    monkilled(mon, "", nocorpse ? -AD_RBRE : AD_PHYS);
                }
                else {
                    xkilled(mon, nocorpse ? XKILL_NOCORPSE : 0);
                }
                if (DEADMONSTER(mon)) {
                    newsym(xx, yy);
                    trapkilled = TRUE;
                }
            }
        } else {
            strike = 0; /* harmless; don't use up the missile */
        }
    }
    if (obj && (!strike || d_override)) {
        place_object(obj, mon->mx, mon->my);
        stackobj(obj);
    } else if (obj)
        dealloc_obj(obj);

    return trapkilled;
}

boolean
unconscious(void)
{
    if (gm.multi >= 0)
        return FALSE;

    return (u.usleep
            || (gn.nomovemsg
                && (!strncmp(gn.nomovemsg, "You awake", 9)
                    || !strncmp(gn.nomovemsg, "You regain con", 14)
                    || !strncmp(gn.nomovemsg, "You are consci", 14))));
}

static const char lava_killer[] = "molten lava";

/* hero enters pool of molten lava; returns True if hero is killed and
   then life-saved (with teleport to safe spot), False for other survival;
   no return at all if hero dies and isn't life-saved */
boolean
lava_effects(void)
{
    struct obj *obj, *obj2, *nextobj;
    boolean usurvive, boil_away;
    unsigned protect_oid = 0;
    int burncount = 0, burnmesgcount = 0;
    const int dmg = d(6, 6); /* only applicable for water walking */

    if (iflags.in_lava_effects) {
        debugpline0("Skipping recursive lava_effects().");
        return FALSE;
    }
    feel_newsym(u.ux, u.uy); /* in case Blind, map the lava here */
    burn_away_slime();
    if (likes_lava(gy.youmonst.data))
        return FALSE;

    usurvive = Fire_resistance || (Wwalking && dmg < u.uhp);
    /*
     * A timely interrupt might manage to salvage your life
     * but not your gear.  For scrolls and potions this
     * will destroy whole stacks, where fire resistant hero
     * survivor only loses partial stacks via destroy_items().
     *
     * Flag items to be destroyed before any messages so
     * that player causing hangup at --More-- won't get an
     * emergency save file created before item destruction.
     */
    if (!usurvive) {
        for (obj = gi.invent; obj; obj = nextobj) {
            nextobj = obj->nobj;
            if (obj->in_use) { /* remove_worn_item() sets in_use */
                /* one item can be protected from burning up [accommodates
                   steal(AMULET_OF_FLYING) -> remove_worn_item() -> fall
                   into lava (which happens before item is transferred
                   from invent to thief->minvent)]; item will still be in
                   inventory when we return to caller or save bones (or
                   perform hangup save if that occurs) */
                if (!protect_oid) {
                    protect_oid = obj->o_id;
                    obj->in_use = 0;
                } else {
                    impossible(
                     "lava_effects: '%s' (#%u) is already in use; so is #%u.",
                               simpleonames(obj), obj->o_id, protect_oid);
                }
                continue;
            }
            /* set obj->in_use for items which will be destroyed below */
            if ((is_organic(obj) || obj->oclass == POTION_CLASS)
                && !obj->oerodeproof
                && objects[obj->otyp].oc_oprop != FIRE_RES
                && obj->otyp != SCR_FIRE && obj->otyp != SPE_FIREBALL
                && !obj_resists(obj, 0, 0)) /* for invocation items */
                obj->in_use = 1;
        }
    }

    /* Check whether we should burn away boots *first* so we know whether to
     * make the player sink into the lava. Assumption: water walking only
     * comes from boots.
     * (3.7: that assumption is no longer true, but having boots be the first
     * thing to come into contact with lava makes sense.)
     */
    if (uarmf && (uarmf->in_use
                  || (is_organic(uarmf) && !uarmf->oerodeproof))) {
        obj = uarmf;
        pline("%s into flame!", Yobjnam2(obj, "burst"));
        ++burnmesgcount;
        iflags.in_lava_effects++; /* (see above) */
        (void) Boots_off();
        if (obj->o_id != protect_oid)
            useup(obj);
        iflags.in_lava_effects--;
        ++burncount;
    }

    if (!Fire_resistance) {
        if (Wwalking) {
            /* Assume three things:
             * 1. The hero is wearing water walking boots (they are the only
             *    source of the water walking property).
             * 2. Water walking boots are always burnable.
             * 3. To be walking on lava, they must be fireproof.
             */
            if (!objects[WATER_WALKING_BOOTS].oc_name_known) {
                Your("boots don't sink into the lava!");
            }
            makeknown(WATER_WALKING_BOOTS);
            uarmf->rknown = 1;
            pline_The("%s here burns you!", hliquid("lava"));
            if (usurvive) {
                losehp(dmg, lava_killer, KILLED_BY); /* lava damage */
                goto burn_stuff;
            }
        } else
            You("fall into the %s!", waterbody_name(u.ux, u.uy));

        usurvive = Lifesaved || discover;
        if (wizard)
            usurvive = TRUE;

        /* prevent remove_worn_item() -> Boots_off(WATER_WALKING_BOOTS) ->
           spoteffects() -> lava_effects() recursion which would
           successfully delete (via useupall) the no-longer-worn boots;
           once recursive call returned, we would try to delete them again
           here in the outer call (and access stale memory, probably panic) */
        iflags.in_lava_effects++;

        for (obj = gi.invent; obj; obj = obj2) {
            obj2 = obj->nobj;
            if (obj->o_id == protect_oid) {
                /* skip protected item; caller expects to retain access */
                obj->in_use = 1; /* was cleared when setting protect_oid */
            } else if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
                if (usurvive && !Blind)
                    pline("%s glows a strange %s, but remains intact.",
                          The(xname(obj)), hcolor("dark red"));
            } else if (obj->in_use) {
                if (obj->owornmask) {
                    if (usurvive) {
                        pline("%s into flame!", Yobjnam2(obj, "burst"));
                        ++burnmesgcount;
                    }
                    remove_worn_item(obj, TRUE);
                }
                useupall(obj);
                ++burncount;
            }
        }
        if (usurvive && burncount > burnmesgcount)
            pline("%s item%s in your inventory %s been destroyed.",
                  (burnmesgcount > 0)
                    ? ((burncount - burnmesgcount == 1) ? "Another" : "Other")
                    : ((burncount == 1) ? "An" : "Some"),
                  plur(burncount - burnmesgcount),
                  (burncount - burnmesgcount == 1) ? "has" : "have");

        /* s/he died... */
        boil_away = (u.umonnum == PM_WATER_ELEMENTAL
                     || u.umonnum == PM_STEAM_VORTEX
                     || u.umonnum == PM_FOG_CLOUD);
        /* burn to death; if hero is life-saved on the first pass, try
           to teleport to safety; if that fails, burn all over again */
        for (burncount = 0; burncount < 2; ++burncount) {
            u.uhp = -1;
            /* killer format and name are reconstructed every iteration
               because lifesaving resets them */
            svk.killer.format = KILLED_BY;
            Strcpy(svk.killer.name, lava_killer);
            urgent_pline("You %s...", boil_away ? "boil away"
                                                : "burn to a crisp");
            done(BURNING);
            if (safe_teleds(TELEDS_ALLOW_DRAG | TELEDS_TELEPORT))
                break; /* successful life-save */
            /* nowhere safe to land; repeat burning loop */
            pline("You're still burning.");
        }

        iflags.in_lava_effects--;

        if (burncount == 2) {
            /* life-saved twice (second time must have been due to declining
               to die in wizard|explore mode) and failed to be teleported
               to safety both times; moveloop() will just drop the hero into
               the lava again on next move so take countermeasures to give
               the player--or the debug fuzzer--a chance to try something
               else instead of just immediately burning up all over again */
            if (!Fire_resistance)
                set_itimeout(&HFire_resistance, 5L);
            if (!Wwalking)
                set_itimeout(&HWwalking, 5L);
            goto burn_stuff;
        }
        rescued_from_terrain(BURNING);

        /* normally done via safe_teleds() -> teleds() -> spoteffects() but
           spoteffects() was no-op when called with nonzero in_lava_effects */
        spoteffects(FALSE); /* suppress auto-pickup for this landing... */

        return TRUE;
    } else if (!Wwalking && (!u.utrap || u.utraptype != TT_LAVA)) {
        boil_away = !Fire_resistance;
        /* if not fire resistant, sink_into_lava() will quickly be fatal;
           hero needs to escape immediately */
        set_utrap((unsigned) (rn1(4, 4) + ((boil_away ? 2
                                                      : rn1(4, 12)) << 8)),
                  TT_LAVA);
        You("sink into the %s%s!", waterbody_name(u.ux, u.uy),
            !boil_away ? ", but it only burns slightly"
                       : " and are about to be immolated");
        if (Fire_resistance)
            monstseesu(M_SEEN_FIRE);
        else
            monstunseesu(M_SEEN_FIRE);
        if (u.uhp > 1)
            losehp(!boil_away ? 1 : (u.uhp / 2), lava_killer,
                   KILLED_BY); /* lava damage */
    }

 burn_stuff:
    (void) destroy_items(&gy.youmonst, AD_FIRE, dmg);
    ignite_items(gi.invent);
    return FALSE;
}

/* called each turn when trapped in lava */
void
sink_into_lava(void)
{
    static const char sink_deeper[] = "You sink deeper into the lava.";

    if (!u.utrap || u.utraptype != TT_LAVA) {
        ; /* do nothing; this usually won't happen but could after
           * polymorphing from a flier into a ceiling hider and then hiding;
           * allmain() only checks whether the hero is at a lava location,
           * not whether he or she is currently sinking */
    } else if (!is_lava(u.ux, u.uy)) {
        reset_utrap(FALSE); /* this shouldn't happen either */
    } else if (!u.uinvulnerable) {
        /* ordinarily we'd have to be fire resistant to survive long
           enough to become stuck in lava, but it can happen without
           resistance if water walking boots allow survival and then
           get burned up; u.utrap time will be quite short in that case */
        if (!Fire_resistance)
            u.uhp = (u.uhp + 2) / 3;

        u.utrap -= (1 << 8);
        if (u.utrap < (1 << 8)) {
            svk.killer.format = KILLED_BY;
            Strcpy(svk.killer.name, "molten lava");
            urgent_pline("You sink below the surface and die.");
            burn_away_slime(); /* add insult to injury? */
            done(DISSOLVED);
            /* can only get here via life-saving; try to get away from lava */
            reset_utrap(TRUE);
            /* levitation or flight have become unblocked, otherwise Tport */
            if (!Levitation && !Flying)
                (void) safe_teleds(TELEDS_ALLOW_DRAG | TELEDS_TELEPORT);
        } else if (!u.umoved) {
            /* can't fully turn into slime while in lava, but might not
               have it be burned away until you've come awfully close */
            if (Slimed && rnd(10 - 1) >= (int) (Slimed & TIMEOUT)) {
                pline(sink_deeper);
                burn_away_slime();
            } else {
                Norep(sink_deeper);
            }
            u.utrap += rnd(4);
        }
    }
}

/* called when something has been done (breaking a boulder, for instance)
   which entails a luck penalty if performed on a sokoban level */
void
sokoban_guilt(void)
{
    if (Sokoban) {
        u.uconduct.sokocheat++;
        change_luck(-1);
        /*
         * TODO:
         *  Issue some feedback so that player can learn that whatever
         *  he/she just did is a naughty thing to do in sokoban and
         *  should probably be avoided in future....
         *
         *  Caveat:  doing this might introduce message sequencing
         *  issues, depending upon feedback during the various actions
         *  which trigger Sokoban luck penalties.
         */
    }
}

/* called when a trap has been deleted or had its ttyp replaced */
staticfn void
maybe_finish_sokoban(void)
{
    struct trap *t;

    if (Sokoban && !gi.in_mklev) {
        /* scan all remaining traps, ignoring any created by the hero;
           if this level has no more pits or holes, the current sokoban
           puzzle has been solved */
        for (t = gf.ftrap; t; t = t->ntrap) {
            if (t->madeby_u)
                continue;
            if (t->ttyp == PIT || t->ttyp == HOLE)
                break;
        }
        if (!t) {
            /* for livelog to report the sokoban depth in the way that
               players tend to think about it: 1 for entry level, 4 for top */
            int sokonum = svd.dungeons[u.uz.dnum].entry_lev - u.uz.dlevel + 1;

            /* we've passed the last trap without finding a pit or hole;
               clear the sokoban_rules flag so that luck penalties for
               things like breaking boulders or jumping will no longer
               be given, and restrictions on diagonal moves are lifted */
            Sokoban = 0; /* clear svl.level.flags.sokoban_rules */
            /*
             * TODO: give some feedback about solving the sokoban puzzle
             * (perhaps say "congratulations" in Japanese?).
             */

            /* log the completion event regardless of whether or not
               any normal in-game feedback has just been given */
            livelog_printf(LL_MINORAC | LL_DUMP,
                           "completed %d%s Sokoban level",
                           sokonum, ordin(sokonum));
        }
    }
}

/* Return the string name of the trap type passed in, unless the player is
   hallucinating, in which case return a random or hallucinatory trap name. */
const char *
trapname(
    int ttyp,
    boolean override) /* if True, ignore Hallucination */
{
    static const char *const halu_trapnames[] = {
        /* riffs on actual nethack traps */
        "bottomless pit", "polymorphism trap", "devil teleporter",
        "falling boulder trap", "anti-anti-magic field", "weeping gas trap",
        "queasy board", "electrified web", "owlbear trap", "sand mine",
        "vibrating triangle",
        /* some traps found in nethack variants */
        "death trap", "disintegration trap", "ice trap", "monochrome trap",
        /* plausible real-life traps */
        "axeblade trap", "pool of boiling oil", "pool of quicksand",
        "field of caltrops", "buzzsaw trap", "spiked floor", "revolving wall",
        "uneven floor", "finger trap", "jack-in-a-box", "yellow snow",
        "booby trap", "rat trap", "poisoned nail", "snare", "whirlpool",
        "trip wire", "roach motel (tm)", "pile of lego bricks",
        /* sci-fi */
        "negative space", "tensor field", "singularity", "imperial fleet",
        "black hole", "thermal detonator", "event horizon",
        "entoptic phenomenon",
        /* miscellaneous suggestions */
        "sweet-smelling gas vent", "phone booth", "exploding runes",
        "never-ending elevator", "slime pit", "warp zone", "illusory floor",
        "pile of poo", "honey trap", "tourist trap",
    };
    static char roletrap[33]; /* [17 + 5 + 1] should suffice */

    if (Hallucination && !override) {
        int total_names = TRAPNUM + SIZE(halu_trapnames),
            nameidx = rn2_on_display_rng(total_names + 1);

        if (nameidx == total_names) {
            boolean fem = Upolyd ? u.mfemale : flags.female;

            /* inspired by "tourist trap" */
            copynchars(roletrap,
                       rn2(3) ? ((fem && gu.urole.name.f) ? gu.urole.name.f
                                                          : gu.urole.name.m)
                              : rank_of(u.ulevel, Role_switch, fem),
                       (int) (sizeof roletrap - sizeof " trap"));
            Strcat(roletrap, " trap");
            return lcase(roletrap);
        } else if (nameidx >= TRAPNUM) {
            nameidx -= TRAPNUM;
            return halu_trapnames[nameidx];
        } /* else use an actual trap type */
        if (nameidx != NO_TRAP)
            ttyp = nameidx;
    }
    return defsyms[trap_to_defsym(ttyp)].explanation;
}

/* Ignite ignitable items (limited to light sources) in the given object
   chain, due to some external source of fire.  The object chain should
   be somewhere exposed, like someone's open inventory or the floor.
   This is modeled after destroy_items() somewhat and hopefully will be able to
   merge into it in the future. */
void
ignite_items(struct obj *objchn)
{
    struct obj *obj, *nextobj;
    boolean bynexthere = (objchn && objchn->where == OBJ_FLOOR);

    for (obj = objchn; obj; obj = bynexthere ? obj->nexthere : nextobj) {
        nextobj = obj->nobj;
        /* ignitable items like lamps and candles will catch fire */
        if (!obj->lamplit && !obj->in_use)
            catch_lit(obj);
    }
}

void
trap_ice_effects(coordxy x, coordxy y, boolean ice_is_melting)
{
    struct trap *ttmp = t_at(x, y);

    if (ttmp && ice_is_melting) {
        struct monst *mtmp;

        if (((mtmp = m_at(x, y)) != 0) && mtmp->mtrapped)
            mtmp->mtrapped = 0;
        if (ttmp->ttyp == LANDMINE || ttmp->ttyp == BEAR_TRAP) {
            /* landmine or bear trap set on top of the ice falls
               into the water */
            deltrap_with_ammo(ttmp, DELTRAP_PLACE_AMMO);
        } else if (ttmp->ammo) { /* shouldn't really happen but... */
            deltrap_with_ammo(ttmp, DELTRAP_DESTROY_AMMO);
        } else {
            if (!undestroyable_trap(ttmp->ttyp))
                deltrap(ttmp);
        }
    }
}

/* sanity check traps */
void
trap_sanity_check(void)
{
    struct trap *ttmp = gf.ftrap;

    while (ttmp) {
        if (!isok(ttmp->tx, ttmp->ty))
            impossible("trap sanity: location (%i,%i)", ttmp->tx, ttmp->ty);
        if (ttmp->ttyp <= NO_TRAP || ttmp->ttyp >= TRAPNUM)
            impossible("trap sanity: type (%i)", ttmp->ttyp);
        ttmp = ttmp->ntrap;
    }
}

/*trap.c*/

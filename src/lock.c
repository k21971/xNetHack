/* NetHack 3.7	lock.c	$NHDT-Date: 1718745135 2024/06/18 21:12:15 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.137 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* occupation callbacks */
staticfn int picklock(void);
staticfn int forcelock(void);

staticfn const char *lock_action(void);
staticfn boolean obstructed(coordxy, coordxy, boolean);
staticfn void chest_shatter_msg(struct obj *);
staticfn boolean demogorgon_special_key(struct obj *key) NONNULLARG1;

boolean
picking_lock(coordxy *x, coordxy *y)
{
    if (go.occupation == picklock) {
        *x = u.ux + u.dx;
        *y = u.uy + u.dy;
        return TRUE;
    } else {
        *x = *y = 0;
        return FALSE;
    }
}

boolean
picking_at(coordxy x, coordxy y)
{
    return (boolean) (go.occupation == picklock
                      && gx.xlock.door == &levl[x][y]);
}

/* produce an occupation string appropriate for the current activity */
staticfn const char *
lock_action(void)
{
    /* "unlocking"+2 == "locking" */
    static const char *const actions[] = {
        "unlocking the door",   /* [0] */
        "unlocking the chest",  /* [1] */
        "unlocking the box",    /* [2] */
        "picking the lock"      /* [3] */
    };

    /* if the target is currently unlocked, we're trying to lock it now */
    if (gx.xlock.door && !door_is_locked(gx.xlock.door))
        return actions[0] + 2; /* "locking the door" */
    else if (gx.xlock.box && !gx.xlock.box->olocked)
        return gx.xlock.box->otyp == CHEST ? actions[1] + 2 : actions[2] + 2;
    /* otherwise we're trying to unlock it */
    else if (gx.xlock.picktyp == LOCK_PICK)
        return actions[3]; /* "picking the lock" */
    else if (gx.xlock.picktyp == CREDIT_CARD)
        return actions[3]; /* same as lock_pick */
    else if (gx.xlock.door)
        return actions[0]; /* "unlocking the door" */
    else if (gx.xlock.box)
        return gx.xlock.box->otyp == CHEST ? actions[1] : actions[2];
    else
        return actions[3];
}

/* try to open/close a lock */
staticfn int
picklock(void)
{
    coordxy doorx = u.ux + u.dx, doory = u.uy + u.dy;
    coordxy behind_x = u.ux, behind_y = u.uy;
    boolean credit_card_slipped;

    if (gx.xlock.box) {
        if (gx.xlock.box->where != OBJ_FLOOR
            || gx.xlock.box->ox != u.ux || gx.xlock.box->oy != u.uy) {
            return ((gx.xlock.usedtime = 0)); /* you or it moved */
        }
    } else { /* door */
        if (gx.xlock.door != &(levl[doorx][doory])) {
            return ((gx.xlock.usedtime = 0)); /* you moved */
        }
        if (!door_is_closed(gx.xlock.door)) {
            switch (doorstate(gx.xlock.door)) {
            case D_NODOOR:
                pline("This doorway has no door.");
                return ((gx.xlock.usedtime = 0));
            case D_ISOPEN:
                You("cannot lock an open door.");
                return ((gx.xlock.usedtime = 0));
            case D_BROKEN:
                pline("This door is broken.");
                return ((gx.xlock.usedtime = 0));
            default:
                impossible("bad picklock door state %d",
                           gx.xlock.door->doormask);
            }
        }
        if (demogorgon_special_door(gx.xlock.door)) {
            if (gx.xlock.picktyp == CREDIT_CARD) {
                /* if in the future certain random doors are changed to not be
                 * pickable with a credit card, probably change this message to
                 * that */
                pline("The crack of this door is too narrow.");
                return (gx.xlock.usedtime = 0);
            }
            else if (gx.xlock.picktyp == LOCK_PICK) {
                pline("A ward on the keyhole repels your attempt to pick it.");
                return (gx.xlock.usedtime = 0);
            }
            else { /* key */
                if (!demogorgon_special_key(gx.xlock.pick)) {
                    pline(
                "This door's keyhole is strange and won't admit your key.");
                    return (gx.xlock.usedtime = 0);
                }
            }
        }
    }

    if (gx.xlock.door
        && predoortrapped(doorx, doory, &gy.youmonst, FINGER, -D_LOCKED)
           == DOORTRAPPED_DESTROYED) {
        return (gx.xlock.usedtime = 0);
    }

    if (gx.xlock.usedtime++ >= 50 || nohands(gy.youmonst.data)) {
        You("give up your attempt at %s.", lock_action());
        exercise(A_DEX, TRUE); /* even if you don't succeed */
        return ((gx.xlock.usedtime = 0));
    }

    if (rn2(100) >= gx.xlock.chance)
        return 1; /* still busy */

    if (gx.xlock.picktyp == CREDIT_CARD && (Fumbling || gx.xlock.pick->cursed)
        /* 1/(Dex/4) chance if Fumbling; 1/2*Dex chance otherwise */
        && !rn2(Fumbling ? ((ACURR(A_DEX) + 3) / 4)
                         : ACURR(A_DEX) * 2)) {
        if (gx.xlock.door) {
            behind_x = doorx + (gx.xlock.door->horizontal ? 0 : u.dx);
            behind_y = doory + (gx.xlock.door->horizontal ? u.dy : 0);
            credit_card_slipped = isok(behind_x, behind_y)
                                  && SPACE_POS(levl[behind_x][behind_y].typ);
        } else {
            credit_card_slipped = TRUE;
        }
        if (credit_card_slipped) {
            Your("%s slips through the crack!", xname(gx.xlock.pick));
            obj_extract_self(gx.xlock.pick);
            if (gx.xlock.door) {
                place_object(gx.xlock.pick, behind_x, behind_y);
            } else {
                add_to_container(gx.xlock.box, gx.xlock.pick);
            }
            return ((gx.xlock.usedtime = 0));
        }
    }

    /* using the Master Key of Thievery finds traps if its bless/curse
       state is adequate (non-cursed for rogues, blessed for others;
       checked when setting up 'xlock') */
    if ((!gx.xlock.door ? (int) gx.xlock.box->otrapped
                        : door_is_trapped(gx.xlock.door))
        && gx.xlock.magic_key) {
        gx.xlock.chance += 20; /* less effort needed next time */
        if (!gx.xlock.door) {
            if (!gx.xlock.box->tknown)
                You("find a trap!");
            gx.xlock.box->tknown = 1;
        }
        if (y_n("Do you want to try to disarm it?") == 'y') {
            const char *what;
            boolean alreadyunlocked;

            /* disarming while using magic key always succeeds */
            if (gx.xlock.door) {
                set_door_trap(gx.xlock.door, FALSE);
                what = "door";
                alreadyunlocked = !door_is_locked(gx.xlock.door);
            } else {
                gx.xlock.box->otrapped = 0;
                gx.xlock.box->tknown = 0;
                what = (gx.xlock.box->otyp == CHEST) ? "chest" : "box";
                alreadyunlocked = !gx.xlock.box->olocked;
            }
            You("succeed in disarming the trap.  The %s is still %slocked.",
                what, alreadyunlocked ? "un" : "");
            exercise(A_WIS, TRUE);
        } else {
            You("stop %s.", lock_action());
            exercise(A_WIS, FALSE);
        }
        return ((gx.xlock.usedtime = 0));
    }

    You("succeed in %s.", lock_action());
    if (gx.xlock.door) {
        int intended = (door_is_locked(gx.xlock.door) ? -D_LOCKED : D_LOCKED);
        if (postdoortrapped(doorx, doory, &gy.youmonst, FINGER, intended)
            == DOORTRAPPED_NOCHANGE) {
            set_door_lock(gx.xlock.door, !door_is_locked(gx.xlock.door));
        }
        if (demogorgon_special_door(gx.xlock.door)
            && demogorgon_special_key(gx.xlock.pick)
            && !door_is_locked(gx.xlock.door)) {
            pline("The door vibrates then vanishes, taking the key with it.");
            set_doorstate(gx.xlock.door, D_NODOOR);
            /* if the player decides to zap this back into a locked door, make
             * it a normal breakable wooden door, because otherwise they could
             * conceivably trap themselves in part of the level with no keys to
             * escape with */
            set_door_iron(gx.xlock.door, FALSE);
            newsym(doorx, doory);
            useup(gx.xlock.pick);
            reset_pick();
        }
    } else {
        gx.xlock.box->olocked = !gx.xlock.box->olocked;
        gx.xlock.box->lknown = 1;
        if (gx.xlock.box->otrapped)
            (void) chest_trap(gx.xlock.box, FINGER, FALSE);
    }
    exercise(A_DEX, TRUE);
    return ((gx.xlock.usedtime = 0));
}

void
breakchestlock(struct obj *box, boolean destroyit)
{
    if (is_metallic(box)) {
        destroyit = FALSE;
    }
    if (!destroyit) { /* bill for the box but not for its contents */
        struct obj *hide_contents = box->cobj;

        box->cobj = 0;
        costly_alteration(box, COST_BRKLCK);
        box->cobj = hide_contents;
        box->olocked = 0;
        box->obroken = 1;
        box->lknown = 1;
    } else { /* #force has destroyed this box (at <u.ux,u.uy>) */
        struct obj *otmp;
        struct monst *shkp = (*u.ushops && costly_spot(u.ux, u.uy))
                                 ? shop_keeper(*u.ushops)
                                 : 0;
        boolean costly = (boolean) (shkp != 0),
                peaceful_shk = costly && (boolean) shkp->mpeaceful;
        long loss = 0L;

        pline("In fact, you've totally destroyed %s.", the(xname(box)));
        /* Put the contents on ground at the hero's feet. */
        while ((otmp = box->cobj) != 0) {
            obj_extract_self(otmp);
            if (!rn2(3) || otmp->oclass == POTION_CLASS) {
                chest_shatter_msg(otmp);
                if (costly)
                    loss += stolen_value(otmp, u.ux, u.uy, peaceful_shk,
                                         TRUE);
                if (otmp->quan == 1L) {
                    obfree(otmp, (struct obj *) 0);
                    continue;
                }
                /* this works because we're sure to have at least 1 left;
                   otherwise it would fail since otmp is not in inventory */
                useup(otmp);
            }
            if (box->otyp == ICE_BOX && otmp->otyp == CORPSE) {
                otmp->age = svm.moves - otmp->age; /* actual age */
                start_corpse_timeout(otmp);
            }
            place_object(otmp, u.ux, u.uy);
            stackobj(otmp);
        }
        if (costly)
            loss += stolen_value(box, u.ux, u.uy, peaceful_shk, TRUE);
        if (loss)
            You("owe %ld %s for objects destroyed.", loss, currency(loss));
        delobj(box);
    }
}

/* try to force a locked chest */
staticfn int
forcelock(void)
{
    if ((gx.xlock.box->ox != u.ux) || (gx.xlock.box->oy != u.uy))
        return ((gx.xlock.usedtime = 0)); /* you or it moved */

    if (gx.xlock.usedtime++ >= 50 || !uwep || nohands(gy.youmonst.data)) {
        You("give up your attempt to force the lock.");
        if (gx.xlock.usedtime >= 50) /* you made the effort */
            exercise((gx.xlock.picktyp) ? A_DEX : A_STR, TRUE);
        return ((gx.xlock.usedtime = 0));
    }

    if (gx.xlock.picktyp) { /* blade */
        if (rn2(1000 - (int) uwep->spe) > (992 - greatest_erosion(uwep) * 10)
            && !uwep->cursed && !(uwep->material == GLASS && uwep->oerodeproof)
            && !obj_resists(uwep, 0, 99)) {
            /* for a +0 weapon, probability that it survives an unsuccessful
             * attempt to force the lock is (.992)^50 = .67
             */
            pline("%sour %s broke!", (uwep->quan > 1L) ? "One of y" : "Y",
                  xname(uwep));
            useup(uwep);
            You("give up your attempt to force the lock.");
            exercise(A_DEX, TRUE);
            return ((gx.xlock.usedtime = 0));
        }
    } else             /* blunt */
        wake_nearby(FALSE); /* due to hammering on the container */

    if (rn2(100) >= gx.xlock.chance)
        return 1; /* still busy */

    You("succeed in forcing the lock.");
    exercise(gx.xlock.picktyp ? A_DEX : A_STR, TRUE);
    /* breakchestlock() might destroy xlock.box; if so, xlock context will
       be cleared (delobj -> obfree -> maybe_reset_pick); but it might not,
       so explicitly clear that manually */
    breakchestlock(gx.xlock.box, (boolean) (!gx.xlock.picktyp && !rn2(3)));
    reset_pick(); /* lock-picking context is no longer valid */

    return 0;
}

void
reset_pick(void)
{
    gx.xlock.usedtime = gx.xlock.chance = gx.xlock.picktyp = 0;
    gx.xlock.magic_key = FALSE;
    gx.xlock.door = (struct rm *) 0;
    gx.xlock.box = gx.xlock.pick = (struct obj *) 0;
}

/* level change or object deletion; context may no longer be valid */
void
maybe_reset_pick(struct obj *container) /* passed from obfree() */
{
    /*
     * If a specific container, only clear context if it is for that
     * particular container (which is being deleted).  Other stuff on
     * the current dungeon level remains valid.
     * However if 'container' is Null, clear context if not carrying
     * gx.xlock.box (which might be Null if context is for a door).
     * Used for changing levels, where a floor container or a door is
     * being left behind and won't be valid on the new level but a
     * carried container will still be.  There might not be any context,
     * in which case redundantly clearing it is harmless.
     */
    if (container ? (container == gx.xlock.box)
                  : (!gx.xlock.box || !carried(gx.xlock.box)))
        reset_pick();
}

/* pick a tool for autounlock */
struct obj *
autokey(boolean opening) /* True: key, pick, or card; False: key or pick */
{
    struct obj *o, *key, *pick, *card, *akey, *apick, *acard;

    /* mundane item or regular artifact or own role's quest artifact */
    key = pick = card = (struct obj *) 0;
    /* other role's quest artifact (Rogue's Key or Tourist's Credit Card) */
    akey = apick = acard = (struct obj *) 0;
    for (o = gi.invent; o; o = o->nobj) {
        if (any_quest_artifact(o) && !is_quest_artifact(o)) {
            switch (o->otyp) {
            case SKELETON_KEY:
                if (!akey)
                    akey = o;
                break;
            case LOCK_PICK:
                if (!apick)
                    apick = o;
                break;
            case CREDIT_CARD:
                if (!acard)
                    acard = o;
                break;
            default:
                break;
            }
        } else {
            switch (o->otyp) {
            case SKELETON_KEY:
                if (!key || is_magic_key(&gy.youmonst, o))
                    key = o;
                break;
            case LOCK_PICK:
                if (!pick)
                    pick = o;
                break;
            case CREDIT_CARD:
                if (!card)
                    card = o;
                break;
            default:
                break;
            }
        }
    }
    if (!opening)
        card = acard = 0;
    /* only resort to other role's quest artifact if no other choice */
    if (!key && !pick && !card)
        key = akey;
    if (!pick && !card)
        pick = apick;
    if (!card)
        card = acard;
    return key ? key : pick ? pick : card ? card : 0;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* for doapply(); if player gives a direction or resumes an interrupted
   previous attempt then it usually costs hero a move even if nothing
   ultimately happens; when told "can't do that" before being asked for
   direction or player cancels with ESC while giving direction, it doesn't */
#define PICKLOCK_LEARNED_SOMETHING (-1) /* time passes */
#define PICKLOCK_DID_NOTHING 0          /* no time passes */
#define PICKLOCK_DID_SOMETHING 1

/* player is applying a key, lock pick, or credit card */
int
pick_lock(
    struct obj *pick,
    coordxy rx, coordxy ry, /* coordinates of door/container, for autounlock:
                             * doesn't prompt for direction if these are set */
    struct obj *container)  /* container, for autounlock */
{
    struct obj dummypick;
    int picktyp, c, ch;
    coord cc;
    struct rm *door;
    struct obj *otmp;
    char qbuf[QBUFSZ];
    boolean autounlock = (rx != 0 && ry != 0) || (container != NULL);
    static const char *cant_open_with_card =
        "It doesn't look like this lock can be opened with a credit card.";

    /* 'pick' might be Null [called by do_loot_cont() for AUTOUNLOCK_UNTRAP] */
    if (!pick) {
        dummypick = cg.zeroobj;
        pick = &dummypick; /* pick->otyp will be STRANGE_OBJECT */
    }
    picktyp = pick->otyp;

    /* check whether we're resuming an interrupted previous attempt */
    if (gx.xlock.usedtime && picktyp == gx.xlock.picktyp) {
        static char no_longer[] = "Unfortunately, you can no longer %s %s.";

        if (nohands(gy.youmonst.data)) {
            const char *what = (picktyp == LOCK_PICK) ? "pick" : "key";

            if (picktyp == CREDIT_CARD)
                what = "card";
            pline(no_longer, "hold the", what);
            reset_pick();
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (u.uswallow || (gx.xlock.box && !can_reach_floor(TRUE))) {
            pline(no_longer, "reach the", "lock");
            reset_pick();
            return PICKLOCK_LEARNED_SOMETHING;
        } else {
            const char *action = lock_action();

            You("resume your attempt at %s.", action);
            gx.xlock.magic_key = is_magic_key(&gy.youmonst, pick);
            set_occupation(picklock, action, 0);
            return PICKLOCK_DID_SOMETHING;
        }
    }

    if (nohands(gy.youmonst.data)) {
        You_cant("hold %s -- you have no hands!", doname(pick));
        return PICKLOCK_DID_NOTHING;
    } else if (u.uswallow) {
        You_cant("%sunlock %s.", (picktyp == CREDIT_CARD) ? "" : "lock or ",
                 mon_nam(u.ustuck));
        return PICKLOCK_DID_NOTHING;
    }

    if (pick != &dummypick && picktyp != SKELETON_KEY
        && picktyp != LOCK_PICK && picktyp != CREDIT_CARD) {
        impossible("picking lock with object %d?", picktyp);
        return PICKLOCK_DID_NOTHING;
    }
    ch = 0; /* lint suppression */

    if (rx != 0) { /* autounlock; caller has provided coordinates */
        cc.x = rx;
        cc.y = ry;
    } else if (!get_adjacent_loc((char *) 0, "Invalid location!",
                                 u.ux, u.uy, &cc)) {
        return PICKLOCK_DID_NOTHING;
    }

    if (u_at(cc.x, cc.y)) { /* pick lock on a container */
        const char *verb;
        char qsfx[QBUFSZ];
        boolean it;
        int count;

        /*
         * FIXME:
         *  (chest->otrapped && chest->tknown) is handled, to skip
         *  checking for a trap and continue with asking about disarm;
         *  (chest->tknown && !chest->otrapped) ignores tknown and will
         *  ask about checking for non-existant trap.
         */

        if (u.dz < 0 && !autounlock) { /* beware stale u.dz value */
            There("isn't any sort of lock up %s.",
                  Levitation ? "here" : "there");
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (is_lava(u.ux, u.uy)) {
            pline("Doing that would probably melt %s.", yname(pick));
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (is_pool(u.ux, u.uy) && !Underwater) {
            pline_The("%s has no lock.", hliquid("water"));
            return PICKLOCK_LEARNED_SOMETHING;
        }

        count = 0;
        c = 'n'; /* in case there are no boxes here */
        for (otmp = svl.level.objects[cc.x][cc.y]; otmp;
             otmp = otmp->nexthere) {
            /* autounlock on boxes: only the one that was just discovered to
               be locked; don't include any other boxes which might be here */
            if (autounlock && otmp != container)
                continue;
            if (Is_box(otmp)) {
                ++count;
                if (!can_reach_floor(TRUE)) {
                    You_cant("reach %s from up here.", the(xname(otmp)));
                    return PICKLOCK_LEARNED_SOMETHING;
                }
                it = 0;
                if (otmp->obroken)
                    verb = "fix";
                else if (!otmp->olocked)
                    verb = "lock", it = 1;
                else if (picktyp != LOCK_PICK)
                    verb = "unlock", it = 1;
                else
                    verb = "pick";

                if (autounlock && (flags.autounlock & AUTOUNLOCK_UNTRAP) != 0
                    && could_untrap(FALSE, TRUE)
                    && (c = ynq(safe_qbuf(qbuf, "Check ", " for a trap?",
                                          otmp, yname, ysimple_name, "this")))
                       != 'n') {
                    if (c == 'q')
                        return PICKLOCK_DID_NOTHING; /* c == 'q' */
                    /* c == 'y' */
                    untrap(FALSE, 0, 0, otmp);
                    return PICKLOCK_DID_SOMETHING; /* even if no trap found */
                } else if (autounlock
                          && (flags.autounlock & AUTOUNLOCK_APPLY_KEY) != 0) {
                    c = 'q';
                    if (pick != &dummypick) {
                        Sprintf(qbuf, "Unlock it with %s?", yname(pick));
                        c = ynq(qbuf);
                    }
                    if (c != 'y')
                        return PICKLOCK_DID_NOTHING;
                } else {
                    /* "There is <a box> here; <verb> <it|its lock>?" */
                    Sprintf(qsfx, " here; %s %s?",
                            verb, it ? "it" : "its lock");
                    (void) safe_qbuf(qbuf, "There is ", qsfx, otmp, doname,
                                     ansimpleoname, "a box");
                    otmp->lknown = 1;

                    c = ynq(qbuf);
                    if (c == 'q')
                        return PICKLOCK_DID_NOTHING;
                    if (c == 'n')
                        continue; /* try next box */
                }

                if (otmp->obroken) {
                    You_cant("fix its broken lock with %s.",
                             ansimpleoname(pick));
                    return PICKLOCK_LEARNED_SOMETHING;
                } else if (picktyp == CREDIT_CARD && !otmp->olocked) {
                    /* credit cards are only good for unlocking */
                    You_cant("do that with %s.",
                             an(simple_typename(picktyp)));
                    return PICKLOCK_LEARNED_SOMETHING;
                } else if (autounlock
                           && !touch_artifact(pick, &gy.youmonst)) {
                    /* note: for !autounlock, apply already did touch check */
                    return PICKLOCK_DID_SOMETHING;
                }
                switch (picktyp) {
                case CREDIT_CARD:
                    if (hash1(otmp->o_id) % 5 < 2) {
                        pline1(cant_open_with_card);
                        return PICKLOCK_LEARNED_SOMETHING;
                    }
                    ch = ACURR(A_DEX) + 20 * Role_if(PM_ROGUE);
                    break;
                case LOCK_PICK:
                    ch = 4 * ACURR(A_DEX) + 25 * Role_if(PM_ROGUE);
                    break;
                case SKELETON_KEY:
                    ch = 75 + ACURR(A_DEX);
                    break;
                default:
                    ch = 0;
                }
                if (otmp->cursed)
                    ch /= 2;

                gx.xlock.box = otmp;
                gx.xlock.door = 0;
                break;
            }
        }
        if (c != 'y') {
            if (!count)
                There("doesn't seem to be any sort of lock here.");
            return PICKLOCK_LEARNED_SOMETHING; /* decided against all boxes */
        }

    /* not the hero's location; pick the lock in an adjacent door */
    } else {
        struct monst *mtmp;

        if (u.utrap && u.utraptype == TT_PIT) {
            You_cant("reach over the edge of the pit.");
            /* this used to return PICKLOCK_LEARNED_SOMETHING but the
               #open command doesn't use a turn for similar situation */
            return PICKLOCK_DID_NOTHING;
        }

        door = &levl[cc.x][cc.y];
        mtmp = m_at(cc.x, cc.y);
        if (mtmp && canseemon(mtmp) && M_AP_TYPE(mtmp) != M_AP_FURNITURE
            && M_AP_TYPE(mtmp) != M_AP_OBJECT) {
            if (picktyp == CREDIT_CARD
                && (mtmp->isshk || mtmp->data == &mons[PM_ORACLE])) {
                SetVoice(mtmp, 0, 80, 0);
                verbalize("No checks, no credit, no problem.");
            } else {
                pline("I don't think %s would appreciate that.",
                      mon_nam(mtmp));
            }
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (mtmp && is_door_mappear(mtmp)) {
            /* "The door actually was a <mimic>!" */
            stumble_onto_mimic(mtmp);
            /* mimic might keep the key (50% chance, 10% for PYEC or MKoT) */
            maybe_absorb_item(mtmp, pick, 50, 10);
            return PICKLOCK_LEARNED_SOMETHING;
        }
        if (!IS_DOOR(door->typ)) {
            int res = PICKLOCK_DID_NOTHING, oldglyph = door->glyph;
            schar oldlastseentyp = update_mapseen_for(cc.x, cc.y);

            /* this is probably only relevant when blind */
            feel_location(cc.x, cc.y);
            if (door->glyph != oldglyph
                || svl.lastseentyp[cc.x][cc.y] != oldlastseentyp)
                res = PICKLOCK_LEARNED_SOMETHING;

            if (is_drawbridge_wall(cc.x, cc.y) >= 0)
                You("%s no lock on the drawbridge.", Blind ? "feel" : "see");
            else
                You("%s no door there.", Blind ? "feel" : "see");
            return res;
        }

        if (door_is_closed(door)) {
            if ((flags.autounlock & AUTOUNLOCK_UNTRAP) != 0
                && could_untrap(FALSE, FALSE)
                && (c = ynq("Check this door for a trap?")) != 'n') {
                if (c == 'q')
                    return PICKLOCK_DID_NOTHING;
                /* c == 'y' */
                untrap(FALSE, cc.x, cc.y, (struct obj *) 0);
                return PICKLOCK_DID_SOMETHING; /* even if no trap found */
            }
            /* credit cards are only good for unlocking */
            if (picktyp == CREDIT_CARD && !door_is_locked(door)) {
                You_cant("lock a door with a credit card.");
                return PICKLOCK_LEARNED_SOMETHING;
            }

            /* If not autounlocking, the player has already gone through several
             * keystrokes to get this far - apply command, select tool to apply,
             * direction to apply in. Suppress a further "Unlock it?" prompt. */
            if (autounlock) {
                Sprintf(qbuf, "%s it%s%s?",
                        (door->doormask & D_LOCKED) ? "Unlock" : "Lock",
                        autounlock ? " with " : "",
                        autounlock ? yname(pick) : "");
                c = ynq(qbuf);
                if (c != 'y')
                    return PICKLOCK_DID_NOTHING;

                /* note: for !autounlock, 'apply' already did touch check */
                if (!touch_artifact(pick, &gy.youmonst))
                    return PICKLOCK_DID_SOMETHING;
            }

            switch (picktyp) {
            case CREDIT_CARD:
                /* chance of unopenable door increases with depth;
                 * 10% on early floors, increasing to 60% or so by the Sanctum
                 */
                if (coord_hash(cc.x, cc.y, ledger_no(&u.uz)) % 10
                    < (unsigned int) (depth(&u.uz) / 7) + 1) {
                    pline1(cant_open_with_card);
                    return PICKLOCK_LEARNED_SOMETHING;
                }
                ch = 2 * ACURR(A_DEX) + 20 * Role_if(PM_ROGUE);
                break;
            case LOCK_PICK:
                ch = 3 * ACURR(A_DEX) + 30 * Role_if(PM_ROGUE);
                break;
            case SKELETON_KEY:
                ch = 70 + ACURR(A_DEX);
                break;
            default:
                ch = 0;
            }
            gx.xlock.door = door;
            gx.xlock.box = 0;
        } else {
            switch (doorstate(door)) {
            case D_NODOOR:
                pline("This doorway has no door.");
                return PICKLOCK_LEARNED_SOMETHING;
            case D_ISOPEN:
                You("cannot lock an open door.");
                return PICKLOCK_LEARNED_SOMETHING;
            case D_BROKEN:
                pline("This door is broken.");
                return PICKLOCK_LEARNED_SOMETHING;
            default:
                impossible("picking lock on door state %d", door->doormask);
            }
        }
    }
    svc.context.move = 0;
    gx.xlock.chance = ch;
    gx.xlock.picktyp = picktyp;
    gx.xlock.pick = pick;
    gx.xlock.magic_key = is_magic_key(&gy.youmonst, pick);
    gx.xlock.usedtime = 0;
    set_occupation(picklock, lock_action(), 0);
    return PICKLOCK_DID_SOMETHING;
}

/* is hero wielding a weapon that can #force? */
boolean
u_have_forceable_weapon(void)
{
    if (!uwep /* proper type test */
        || ((uwep->oclass == WEAPON_CLASS || is_weptool(uwep))
            ? (objects[uwep->otyp].oc_skill < P_DAGGER
               || objects[uwep->otyp].oc_skill == P_FLAIL
               || objects[uwep->otyp].oc_skill > P_LANCE)
            : uwep->oclass != ROCK_CLASS))
        return FALSE;
    return TRUE;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* the #force command - try to force a chest with your weapon */
int
doforce(void)
{
    struct obj *otmp;
    int c, picktyp;
    char qbuf[QBUFSZ];

    /*
     * TODO?
     *  allow force with edged weapon to be performed on doors.
     */

    if (u.uswallow) {
        You_cant("force anything from inside here.");
        return ECMD_OK;
    }
    if (!u_have_forceable_weapon()) {
        boolean use_plural = uwep && uwep->quan > 1;

        You_cant("force anything %s weapon%s.",
                 !uwep ? "when not wielding a"
                 : (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))
                   ? (use_plural ? "without proper" : "without a proper")
                   : (use_plural ? "with those" : "with that"),
                 use_plural ? "s" : "");
        return ECMD_OK;
    }
    if (!can_reach_floor(TRUE)) {
        cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return ECMD_OK;
    }

    picktyp = is_blade(uwep) && !is_pick(uwep);
    if (gx.xlock.usedtime && gx.xlock.box && picktyp == gx.xlock.picktyp) {
        You("resume your attempt to force the lock.");
        set_occupation(forcelock, "forcing the lock", 0);
        return ECMD_TIME;
    }

    /* A lock is made only for the honest man, the thief will break it. */
    gx.xlock.box = (struct obj *) 0;
    for (otmp = svl.level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
        if (Is_box(otmp)) {
            if (otmp->obroken || !otmp->olocked) {
                /* force doname() to omit known "broken" or "unlocked"
                   prefix so that the message isn't worded redundantly;
                   since we're about to set lknown, there's no need to
                   remember and then reset its current value */
                otmp->lknown = 0;
                There("is %s here, but its lock is already %s.",
                      doname(otmp), otmp->obroken ? "broken" : "unlocked");
                otmp->lknown = 1;
                continue;
            }
            (void) safe_qbuf(qbuf, "There is ", " here; force its lock?",
                             otmp, doname, ansimpleoname, "a box");
            otmp->lknown = 1;

            c = ynq(qbuf);
            if (c == 'q')
                return ECMD_OK;
            if (c == 'n')
                continue;

            if (picktyp)
                You("force %s into a crack and pry.", yname(uwep));
            else
                You("start bashing it with %s.", yname(uwep));
            gx.xlock.box = otmp;
            gx.xlock.chance = objects[uwep->otyp].oc_wldam * 2;
            gx.xlock.picktyp = picktyp;
            gx.xlock.magic_key = FALSE;
            gx.xlock.usedtime = 0;
            break;
        }

    if (gx.xlock.box)
        set_occupation(forcelock, "forcing the lock", 0);
    else
        You("decide not to force the issue.");
    return ECMD_TIME;
}

boolean
stumble_on_door_mimic(coordxy x, coordxy y)
{
    struct monst *mtmp;

    if ((mtmp = m_at(x, y)) && is_door_mappear(mtmp)
        && !Protection_from_shape_changers) {
        stumble_onto_mimic(mtmp);
        return TRUE;
    }
    return FALSE;
}

/* the #open command - try to open a door */
int
doopen(void)
{
    return doopen_indir(0, 0);
}

/* try to open a door in direction u.dx/u.dy */
int
doopen_indir(coordxy x, coordxy y)
{
    coord cc;
    struct rm *door;
    boolean portcullis;
    const char *dirprompt;
    int res = ECMD_OK;

    if (nohands(gy.youmonst.data)) {
        You_cant("open anything -- you have no hands!");
        return ECMD_OK;
    }

    dirprompt = NULL; /* have get_adjacent_loc() -> getdir() use default */
    if (u.utrap && u.utraptype == TT_PIT && container_at(u.ux, u.uy, FALSE))
        dirprompt = "Open where? [.>]";

    if (x > 0 && y >= 0) {
        /* nonzero <x,y> is used when hero in amorphous form tries to
           flow under a closed door at <x,y>; the test here was using
           'y > 0' but that would give incorrect results if doors are
           ever allowed to be placed on the top row of the map */
        cc.x = x;
        cc.y = y;
    } else if (!get_adjacent_loc(dirprompt, (char *) 0, u.ux, u.uy, &cc)) {
        return ECMD_OK;
    }

    /* open at yourself/up/down: switch to loot unless there is a closed
       door here (possible with Passes_walls) and direction isn't 'down' */
    if (u_at(cc.x, cc.y) && (u.dz > 0 || !closed_door(u.ux, u.uy)))
        return doloot();

    /* this used to be done prior to get_adjacent_loc() but doing so was
       incorrect once open at hero's spot became an alternate way to loot */
    if (u.utrap && u.utraptype == TT_PIT) {
        You_cant("reach over the edge of the pit.");
        return ECMD_OK;
    }

    if (stumble_on_door_mimic(cc.x, cc.y))
        return ECMD_TIME;

    /* when choosing a direction is impaired, use a turn
       regardless of whether a door is successfully targeted */
    if (Confusion || Stunned)
        res = ECMD_TIME;

    door = &levl[cc.x][cc.y];
    portcullis = (is_drawbridge_wall(cc.x, cc.y) >= 0);
    /* this used to be 'if (Blind)' but using a key skips that so we do too */
    {
        int oldglyph = door->glyph;
        schar oldlastseentyp = update_mapseen_for(cc.x, cc.y);

        newsym(cc.x, cc.y);
        if (door->glyph != oldglyph
            || svl.lastseentyp[cc.x][cc.y] != oldlastseentyp)
            res = ECMD_TIME; /* learned something */
    }

    if (portcullis || !IS_DOOR(door->typ)) {
        /* closed portcullis or spot that opened bridge would span */
        if (is_db_wall(cc.x, cc.y) || door->typ == DRAWBRIDGE_UP)
            There("is no obvious way to open the drawbridge.");
        else if (portcullis || door->typ == DRAWBRIDGE_DOWN)
            pline_The("drawbridge is already open.");
        else if (container_at(cc.x, cc.y, TRUE))
            pline("%s like something lootable over there.",
                  Blind ? "Feels" : "Seems");
        else
            You("%s no door there.", Blind ? "feel" : "see");
        return res;
    }

    if (!door_is_closed(door)) {
        const char *mesg = NULL;

        switch (doorstate(door)) {
        case D_BROKEN:
            mesg = " is broken";
            break;
        case D_NODOOR:
            mesg = "way has no door";
            break;
        case D_ISOPEN:
            mesg = " is already open";
            break;
        default:
            impossible("doopen_indir: bad door state %d", door->doormask);
        }
        set_msg_xy(cc.x, cc.y);
        pline("This door%s.", mesg);
        return res;
    }

    if (verysmall(gy.youmonst.data)) {
        pline("You're too small to pull the door open.");
        return res;
    }

    if (predoortrapped(cc.x, cc.y, &gy.youmonst, FINGER, D_ISOPEN)
        != DOORTRAPPED_NOCHANGE) {
        return res;
    }

    if (door_is_locked(door)) {
        pline("This door is locked.");
        if (flags.autounlock) {
            struct obj *unlocktool;

            u.dz = 0; /* should already be 0 since hero moved toward door */
            if ((flags.autounlock & AUTOUNLOCK_APPLY_KEY) != 0
                && (unlocktool = autokey(TRUE)) != 0) {
                res = pick_lock(unlocktool, cc.x, cc.y,
                                (struct obj *) 0) ? ECMD_TIME : ECMD_OK;
            } else if ((flags.autounlock & AUTOUNLOCK_KICK) != 0
                       && !u.usteed /* kicking is different when mounted */
                       && ynq("Kick it?") == 'y') {
                cmdq_add_ec(CQ_CANNED, dokick);
                cmdq_add_dir(CQ_CANNED,
                             sgn(cc.x - u.ux), sgn(cc.y - u.uy), 0);
                /* this was 'ECMD_TIME', but time shouldn't elapse until
                   the canned kick takes place */
                res = ECMD_OK;
            }
        }
        return res;
    }

    /* door is known to be CLOSED */
    pline_The("door opens.");
    set_msg_xy(cc.x, cc.y);
    if (postdoortrapped(cc.x, cc.y, &gy.youmonst, FINGER, D_ISOPEN)
        == DOORTRAPPED_NOCHANGE) {
        set_doorstate(door, D_ISOPEN);
        feel_newsym(cc.x, cc.y); /* the hero knows she opened it */
        recalc_block_point(cc.x, cc.y); /* vision: new see through there */
    }

    return ECMD_TIME;
}

staticfn boolean
obstructed(coordxy x, coordxy y, boolean quietly)
{
    struct monst *mtmp = m_at(x, y);

    if (mtmp && M_AP_TYPE(mtmp) != M_AP_FURNITURE) {
        if (M_AP_TYPE(mtmp) == M_AP_OBJECT)
            goto objhere;
        if (!quietly) {
            char *Mn = Some_Monnam(mtmp); /* Monnam, Someone or Something */

            if ((mtmp->mx != x || mtmp->my != y) && canspotmon(mtmp))
                /* s_suffix() returns a modifiable buffer */
                Mn = strcat(s_suffix(Mn), " tail");

            pline("%s blocks the way!", Mn);
        }
        if (!canspotmon(mtmp))
            map_invisible(x, y);
        return TRUE;
    }
    if (OBJ_AT(x, y)) {
 objhere:
        if (!quietly)
            pline("%s's in the way.", Something);
        return TRUE;
    }
    return FALSE;
}

/* the #close command - try to close a door */
int
doclose(void)
{
    coordxy x, y;
    struct rm *door;
    boolean portcullis;
    int res = ECMD_OK;

    if (nohands(gy.youmonst.data)) {
        You_cant("close anything -- you have no hands!");
        return ECMD_OK;
    }

    if (u.utrap && u.utraptype == TT_PIT) {
        You_cant("reach over the edge of the pit.");
        return ECMD_OK;
    }

    if (!getdir((char *) 0))
        return ECMD_CANCEL;

    x = u.ux + u.dx;
    y = u.uy + u.dy;
    if (u_at(x, y) && !Passes_walls) {
        You("are in the way!");
        return ECMD_TIME;
    }

    if (!isok(x, y))
        goto nodoor;

    if (stumble_on_door_mimic(x, y))
        return ECMD_TIME;

    /* when choosing a direction is impaired, use a turn
       regardless of whether a door is successfully targeted */
    if (Confusion || Stunned)
        res = ECMD_TIME;

    door = &levl[x][y];
    portcullis = (is_drawbridge_wall(x, y) >= 0);
    if (Blind) {
        int oldglyph = door->glyph;
        schar oldlastseentyp = update_mapseen_for(x, y);

        feel_location(x, y);
        if (door->glyph != oldglyph
            || svl.lastseentyp[x][y] != oldlastseentyp)
            res = ECMD_TIME; /* learned something */
    }

    if (portcullis || !IS_DOOR(door->typ)) {
        /* is_db_wall: closed portcullis */
        if (is_db_wall(x, y) || door->typ == DRAWBRIDGE_UP)
            pline_The("drawbridge is already closed.");
        else if (portcullis || door->typ == DRAWBRIDGE_DOWN)
            There("is no obvious way to close the drawbridge.");
        else {
 nodoor:
            You("%s no door there.", Blind ? "feel" : "see");
        }
        return res;
    }

    if (doorstate(door) == D_NODOOR) {
        pline("This doorway has no door.");
        return res;
    } else if (obstructed(x, y, FALSE)) {
        return res;
    } else if (doorstate(door) == D_BROKEN) {
        pline("This door is broken.");
        return res;
    } else if (door_is_locked(door)) {
        pline("This door is already closed.");
        return res;
    }

    if (predoortrapped(x, y, &gy.youmonst, FINGER, D_CLOSED)
        != DOORTRAPPED_NOCHANGE) {
        return res;
    }

    if (doorstate(door) == D_ISOPEN) {
        if (verysmall(gy.youmonst.data) && !u.usteed) {
            pline("You're too small to push the door closed.");
            return res;
        }
        pline_The("door closes.");
        if (postdoortrapped(x, y, &gy.youmonst, FINGER, D_CLOSED)
            == DOORTRAPPED_NOCHANGE) {
            set_doorstate(door, D_CLOSED);
            feel_newsym(x, y); /* the hero knows she closed it */
            block_point(x, y); /* vision:  no longer see there */
        }
    }

    return ECMD_TIME;
}

/* box obj was hit with spell or wand effect otmp;
   returns true if something happened */
boolean
boxlock(struct obj *obj, struct obj *otmp) /* obj *is* a box */
{
    boolean res = 0;

    switch (otmp->otyp) {
    case WAN_LOCKING:
    case SPE_WIZARD_LOCK:
        if (!obj->olocked) { /* lock it; fix if broken */
            Soundeffect(se_klunk, 50);
            pline("Klunk!");
            obj->olocked = 1;
            obj->obroken = 0;
            if (Role_if(PM_WIZARD))
                obj->lknown = 1;
            else
                obj->lknown = 0;
            res = 1;
        } /* else already closed and locked */
        break;
    case WAN_OPENING:
    case SPE_KNOCK:
        if (obj->olocked) { /* unlock; isn't broken so doesn't need fixing */
            Soundeffect(se_klick, 50);
            pline("Klick!");
            obj->olocked = 0;
            res = 1;
            if (Role_if(PM_WIZARD))
                obj->lknown = 1;
            else
                obj->lknown = 0;
        } else /* silently fix if broken */
            obj->obroken = 0;
        break;
    case WAN_POLYMORPH:
    case SPE_POLYMORPH:
        /* maybe start unlocking chest, get interrupted, then zap it;
           we must avoid any attempt to resume unlocking it */
        if (gx.xlock.box == obj)
            reset_pick();
        break;
    }
    return res;
}

/* Door/secret door was hit with spell or wand effect otmp;
   returns true if something happened */
boolean
doorlock(struct obj *otmp, struct monst *mon, coordxy x, coordxy y)
{
    struct rm *door = &levl[x][y];
    boolean res = TRUE;
    const char *msg = (const char *) 0;
    const char *dustcloud = "A cloud of dust";
    const char *quickly_dissipates = "quickly dissipates";
    boolean mysterywand = (otmp->oclass == WAND_CLASS && !otmp->dknown);

    if (door->typ == SDOOR) {
        switch (otmp->otyp) {
        case WAN_OPENING:
        case SPE_KNOCK:
        case WAN_STRIKING:
        case SPE_FORCE_BOLT:
            door->typ = DOOR;
            set_doorstate(door, D_CLOSED);
            newsym(x, y);
            if (cansee(x, y))
                pline("A door appears in the wall!");
            if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK)
                return TRUE;
            break; /* striking: continue door handling below */
        case WAN_LOCKING:
        case SPE_WIZARD_LOCK:
        default:
            return FALSE;
        }
    }

    switch (otmp->otyp) {
    case WAN_LOCKING:
    case SPE_WIZARD_LOCK:
        if (obstructed(x, y, mysterywand))
            return FALSE;
        /* Don't allow doors to close over traps.  This is for pits */
        /* & trap doors, but is it ever OK for anything else? */
        if (t_at(x, y)) {
            /* maketrap() clears doormask, so it should be NODOOR */
            pline("%s springs up in the doorway, but %s.", dustcloud,
                  quickly_dissipates);
            return FALSE;
        }

        switch (doorstate(door)) {
        case D_CLOSED:
            if (!door_is_locked(door))
                msg = "The door locks!";
            break;
        case D_ISOPEN:
            msg = "The door swings shut, and locks!";
            break;
        case D_BROKEN:
            msg = "The broken door reassembles and locks!";
            break;
        case D_NODOOR:
            msg =
               "A cloud of dust springs up and assembles itself into a door!";
            break;
        default:
            impossible("doorlock: bad door state %d", door->doormask);
            res = FALSE;
            break;
        }
        block_point(x, y);
        set_doorstate(door, D_CLOSED);
        set_door_lock(door, TRUE);
        newsym(x, y);
        break;
    case WAN_OPENING:
    case SPE_KNOCK:
        if (door_is_locked(door)
            && alldoortrapped(x, y, mon, NO_PART, -D_LOCKED)
               == DOORTRAPPED_NOCHANGE) {
            if (demogorgon_special_door(door)) {
                msg = "A shimmering shield on the door dissipates the beam.";
            }
            else {
                msg = "The door unlocks!";
                set_door_lock(door, FALSE);
            }
        } else
            res = FALSE;
        break;
    case WAN_STRIKING:
    case SPE_FORCE_BOLT:
        if (door_is_closed(door)) {
            /* sawit: closed door location is more visible than open */
            boolean sawit, seeit;
            if (door_is_iron(door)) {
                msg = "The reinforced door shudders.";
                break;
            }
            if (alldoortrapped(x, y, mon, NO_PART, D_BROKEN)
                != DOORTRAPPED_NOCHANGE) {
                break;
            }
            sawit = cansee(x, y);
            set_doorstate(door, D_BROKEN);
            recalc_block_point(x, y);
            seeit = cansee(x, y);
            newsym(x, y);
            if (flags.verbose) {
                if ((sawit || seeit) && !Unaware) {
                    pline_The("door crashes open!");
                } else if (!Deaf) {
                    Soundeffect(se_crashing_sound, 100);
                    You_hear("a crashing sound.");
                }
            }
            /* force vision recalc before printing more messages */
            if (gv.vision_full_recalc)
                vision_recalc(0);
            wake_nearto(x, y, 20);
            if (*in_rooms(x, y, SHOPBASE))
                add_damage(x, y, 0L);
        } else
            res = FALSE;
        break;
    default:
        impossible("magic (%d) attempted on door.", otmp->otyp);
        break;
    }
    if (msg && cansee(x, y))
        pline1(msg);

    if (res && picking_at(x, y)) {
        /* maybe unseen monster zaps door you're unlocking */
        stop_occupation();
        reset_pick();
    }
    return res;
}

staticfn void
chest_shatter_msg(struct obj *otmp)
{
    const char *disposition;
    const char *thing;
    long save_HBlinded, save_BBlinded;

    if (otmp->oclass == POTION_CLASS) {
        You("%s %s shatter!", Blind ? "hear" : "see", an(bottlename()));
        if (!breathless(gy.youmonst.data) || haseyes(gy.youmonst.data))
            potionbreathe(otmp);
        return;
    }
    /* We have functions for distant and singular names, but not one */
    /* which does _both_... */
    save_HBlinded = HBlinded,  save_BBlinded = BBlinded;
    HBlinded = 1L,  BBlinded = 0L;
    thing = singular(otmp, xname);
    HBlinded = save_HBlinded,  BBlinded = save_BBlinded;
    switch (otmp->material) {
    case PAPER:
        disposition = "is torn to shreds";
        break;
    case WAX:
        disposition = "is crushed";
        break;
    case VEGGY:
        disposition = "is pulped";
        break;
    case FLESH:
        disposition = "is mashed";
        break;
    case GLASS:
        disposition = "shatters";
        break;
    case WOOD:
        disposition = "splinters to fragments";
        break;
    default:
        disposition = "is destroyed";
        break;
    }
    pline("%s %s!", An(thing), disposition);
}

/* Some doors on Demogorgon's level are special and can only be unlocked with a
 * special type of key found there.
 * For now, this just uses iron doors because that's a nice visual indicator,
 * but it could be possible in the future to change to "glass doors" or some
 * such in the future. */
boolean
demogorgon_special_door(struct rm *door) {
    return (IS_DOOR(door->typ) && Is_demogorgon_level(&u.uz)
            && door_is_iron(door));
}

/* Same for keys - for now, these are just any bone key. It could later be
 * changed to "ornate bone key" or something, with its spe field set to
 * visually distinguish from an ordinary key, and only generate with spe set on
 * Demogorgon's level. */
staticfn boolean
demogorgon_special_key(struct obj *key) {
    if (key->otyp != SKELETON_KEY) {
        impossible("non key typ %d passed to demogorgon_special_key",
                   key->otyp);
        return FALSE;
    }
    return (key->material == BONE);
}

/*lock.c*/

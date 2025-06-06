/* NetHack 3.7	sp_lev.c	$NHDT-Date: 1737610109 2025/01/22 21:28:29 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.373 $ */
/*      Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the various functions that are related to the special
 * levels.
 *
 * It contains also the special level loader.
 */

#define IN_SP_LEV_C

#include "hack.h"
#include "sp_lev.h"

extern void mkmap(lev_init *);

staticfn void solidify_map(void);
staticfn void map_cleanup(void);
staticfn void lvlfill_swamp(schar, schar, schar);
staticfn void flip_dbridge_horizontal(struct rm *);
staticfn void flip_dbridge_vertical(struct rm *);
staticfn void flip_visuals(int, int, int, int, int);
staticfn int flip_encoded_dir_bits(int, int);
staticfn void flip_vault_guard(int, struct monst *,
                             coordxy, coordxy, coordxy, coordxy);
staticfn void sel_set_wall_property(coordxy, coordxy, genericptr_t);
staticfn void set_wall_property(coordxy, coordxy, coordxy, coordxy, int);
staticfn void remove_boundary_syms(void);
staticfn void set_door_orientation(int, int);
staticfn boolean shared_with_room(int, int, struct mkroom *);
staticfn void maybe_add_door(int, int, struct mkroom *);
staticfn void link_doors_rooms(void);
staticfn int rndtrap(void);
staticfn void get_location(coordxy *, coordxy *, getloc_flags_t,
                         struct mkroom *);
staticfn void set_ok_location_func(boolean (*)(coordxy, coordxy));
staticfn boolean is_ok_location(coordxy, coordxy, getloc_flags_t);
staticfn unpacked_coord get_unpacked_coord(long, int);
staticfn void get_room_loc(coordxy *, coordxy *, struct mkroom *);
staticfn void get_free_room_loc(coordxy *, coordxy *, struct mkroom *,
                              packed_coord);
staticfn boolean create_subroom(struct mkroom *, coordxy, coordxy, coordxy,
                              coordxy, xint16, xint16);
staticfn void create_door(room_door *, struct mkroom *);
staticfn void create_trap(spltrap *, struct mkroom *);
staticfn int noncoalignment(aligntyp);
staticfn boolean m_bad_boulder_spot(coordxy, coordxy);
staticfn int pm_to_humidity(struct permonst *);
staticfn unsigned int sp_amask_to_amask(unsigned int sp_amask);
staticfn void create_monster(monster *, struct mkroom *);
staticfn struct obj *create_object(object *, struct mkroom *);
staticfn void create_altar(altar *, struct mkroom *);
staticfn boolean search_door(struct mkroom *, coordxy *, coordxy *, xint16,
                             int) NONNULLPTRS;
staticfn void create_corridor(corridor *);
staticfn struct mkroom *build_room(room *, struct mkroom *);
staticfn void light_region(region *);
staticfn void maze1xy(coord *, int);
staticfn void fill_empty_maze(void);
staticfn void splev_initlev(lev_init *);
staticfn boolean generate_way_out_method(coordxy nx, coordxy ny,
                                       struct selectionvar *ov);
staticfn void l_push_wid_hei_table(lua_State *, int, int);
staticfn boolean good_stair_loc(coordxy, coordxy);
staticfn void ensure_way_out(void);

#if 0
/* macosx complains that these are unused */
staticfn long sp_code_jmpaddr(long, long);
staticfn void spo_room(struct sp_coder *);
staticfn void spo_trap(struct sp_coder *);
staticfn void spo_gold(struct sp_coder *);
staticfn void spo_corridor(struct sp_coder *);
staticfn void spo_feature(struct sp_coder *);
staticfn void spo_terrain(struct sp_coder *);
staticfn void spo_replace_terrain(struct sp_coder *);
staticfn void spo_levregion(struct sp_coder *);
staticfn void spo_region(struct sp_coder *);
staticfn void spo_drawbridge(struct sp_coder *);
staticfn void spo_mazewalk(struct sp_coder *);
staticfn void spo_wall_property(struct sp_coder *);
staticfn void spo_room_door(struct sp_coder *);
staticfn void spo_wallify(struct sp_coder *);
staticfn void sel_set_wallify(coordxy, coordxy, genericptr_t);
#endif
staticfn void spo_end_moninvent(void);
staticfn void spo_pop_container(void);
staticfn int l_create_stairway(lua_State *, boolean);
staticfn void spo_endroom(struct sp_coder *);
staticfn void l_table_getset_feature_flag(lua_State *, int, int, const char *,
                                        int);
staticfn void l_get_lregion(lua_State *, lev_region *);
staticfn void sel_set_lit(coordxy, coordxy, genericptr_t);
staticfn void add_doors_to_room(struct mkroom *);
staticfn void get_table_coords_or_region(lua_State *,
                             coordxy *, coordxy *, coordxy *, coordxy *);
staticfn void sel_set_ter(coordxy, coordxy, genericptr_t);
staticfn void sel_set_door(coordxy, coordxy, genericptr_t);
staticfn void sel_set_feature(coordxy, coordxy, genericptr_t);
staticfn void levregion_add(lev_region *);
staticfn void get_table_xy_or_coord(lua_State *, lua_Integer *,
                                    lua_Integer *) NONNULLPTRS;
staticfn int get_table_region(lua_State *, const char *, lua_Integer *,
                        lua_Integer *, lua_Integer *, lua_Integer *, boolean);
staticfn void set_wallprop_in_selection(lua_State *, int);
staticfn int floodfillchk_match_under(coordxy, coordxy);
staticfn int floodfillchk_match_accessible(coordxy, coordxy);
staticfn void l_push_mkroom_table(lua_State *, struct mkroom *);
staticfn int get_table_align(lua_State *);
staticfn int get_table_monclass(lua_State *);
staticfn int get_table_montype(lua_State *, int *);
staticfn lua_Integer get_table_int_or_random(lua_State *, const char *, int);
staticfn int get_table_buc(lua_State *);
staticfn int get_table_objclass(lua_State *);
staticfn int find_objtype(lua_State *, const char *);
staticfn int get_table_objtype(lua_State *);
staticfn const char *get_mkroom_name(int) NONNULL;
staticfn int get_table_roomtype_opt(lua_State *, const char *, int);
staticfn int get_table_traptype_opt(lua_State *, const char *, int);
staticfn int get_traptype_byname(const char *);
staticfn lua_Integer get_table_intarray_entry(lua_State *, int, int);
staticfn struct sp_coder *sp_level_coder_init(void);

/* lua_CFunction prototypes */
int lspo_altar(lua_State *);
int lspo_branch(lua_State *);
int lspo_corridor(lua_State *);
int lspo_door(lua_State *);
int lspo_drawbridge(lua_State *);
int lspo_engraving(lua_State *);
int lspo_feature(lua_State *);
int lspo_gold(lua_State *);
int lspo_grave(lua_State *);
int lspo_ladder(lua_State *);
int lspo_level_flags(lua_State *);
int lspo_level_init(lua_State *);
int lspo_levregion(lua_State *);
int lspo_exclusion(lua_State *);
int lspo_map(lua_State *);
int lspo_mazewalk(lua_State *);
int lspo_message(lua_State *);
int lspo_mineralize(lua_State *);
int lspo_monster(lua_State *);
int lspo_non_diggable(lua_State *);
int lspo_non_passwall(lua_State *);
int lspo_object(lua_State *);
int lspo_portal(lua_State *);
int lspo_random_corridors(lua_State *);
int lspo_region(lua_State *);
int lspo_replace_terrain(lua_State *);
int lspo_reset_level(lua_State *);
int lspo_finalize_level(lua_State *);
int lspo_room(lua_State *);
int lspo_stair(lua_State *);
int lspo_teleport_region(lua_State *);
int lspo_gas_cloud(lua_State *);
int lspo_terrain(lua_State *);
int lspo_trap(lua_State *);
int lspo_wall_property(lua_State *);
int lspo_wallify(lua_State *);

#define sq(x) ((x) * (x))

/* These are also defined in rect.c. If it's important that they have the same
 * values, shouldn't they be moved into a header like rect.h? */
#define XLIM 4
#define YLIM 3

#define New(type) (type *) alloc(sizeof (type))
#define NewTab(type, size) (type **) alloc(sizeof (type *) * (unsigned) size)
#define Free(ptr) \
    do {                                        \
        if (ptr)                                \
            free((genericptr_t) (ptr));         \
    } while (0)

    /*
     * No need for 'struct instance_globals g' to contain these.
     * sp_level_coder_init() always re-initializes them prior to use.
     */
static boolean splev_init_present, icedpools;
/* positions touched by level elements explicitly defined in the level */
static char SpLev_Map[COLNO][ROWNO];
#define MAX_CONTAINMENT 10
static int container_idx = 0; /* next slot in container_obj[] to use */
static struct obj *container_obj[MAX_CONTAINMENT];
static struct monst *invent_carrying_monster = (struct monst *) 0;
    /*
     * end of no 'g.'
     */

#define TYP_CANNOT_MATCH(typ) ((typ) == MAX_TYPE || (typ) == INVALID_TYPE)

void
reset_xystart_size(void)
{
    gx.xstart = 1; /* column [0] is off limits */
    gy.ystart = 0;
    gx.xsize = COLNO - 1; /* 1..COLNO-1 */
    gy.ysize = ROWNO; /* 0..ROWNO-1 */
}

/* Does typ match with levl[][].typ, considering special types
   MATCH_WALL and MAX_TYPE (aka transparency)? */
boolean
match_maptyps(xint16 typ, xint16 levltyp)
{
    if ((typ == MATCH_WALL) && !IS_STWALL(levltyp))
        return FALSE;
    if ((typ < MAX_TYPE) && (typ != levltyp))
        return FALSE;
    return TRUE;
}

struct mapfragment *
mapfrag_fromstr(char *str)
{
    struct mapfragment *mf = (struct mapfragment *) alloc(sizeof *mf);

    char *tmps;

    mf->data = dupstr(str);

    (void) stripdigits(mf->data);
    mf->wid = str_lines_maxlen(mf->data);
    mf->hei = 0;
    tmps = mf->data;
    while (tmps && *tmps) {
        char *s1 = strchr(tmps, '\n');

        if (mf->hei > MAP_Y_LIM) {
            free(mf->data);
            free(mf);
            return NULL;
        }
        if (s1)
            s1++;
        tmps = s1;
        mf->hei++;
    }
    return mf;
}

void
mapfrag_free(struct mapfragment **mf)
{
    if (mf && *mf) {
        free((*mf)->data);
        free(*mf);
        *mf = NULL;
    }
}

schar
mapfrag_get(struct mapfragment *mf, int x, int y)
{
    if (y < 0 || x < 0 || y > mf->hei - 1 || x > mf->wid - 1)
        panic("outside mapfrag (%i,%i), wanted (%i,%i)",
              mf->wid, mf->hei, x, y);
    return splev_chr2typ(mf->data[y * (mf->wid + 1) + x]);
}

boolean
mapfrag_canmatch(struct mapfragment *mf)
{
    return ((mf->wid % 2) && (mf->hei % 2));
}

const char *
mapfrag_error(struct mapfragment *mf)
{
    const char *res = NULL;

    if (!mf) {
        res = "mapfragment error";
    } else if (!mapfrag_canmatch(mf)) {
        mapfrag_free(&mf);
        res = "mapfragment needs to have odd height and width";
    } else if (TYP_CANNOT_MATCH(mapfrag_get(mf, mf->wid / 2, mf->hei / 2))) {
        mapfrag_free(&mf);
        res = "mapfragment center must be valid terrain";
    }
    return res;
}

boolean
mapfrag_match(struct mapfragment *mf,  int x, int y)
{
    int rx, ry;

    for (rx = -(mf->wid / 2); rx <= (mf->wid / 2); rx++)
        for (ry = -(mf->hei / 2); ry <= (mf->hei / 2); ry++) {
            schar mapc = mapfrag_get(mf, rx + (mf->wid / 2),
                                     ry + (mf->hei / 2));
            schar levc = isok(x+rx, y+ry) ? levl[x+rx][y+ry].typ : STONE;

            if (!match_maptyps(mapc, levc))
                return FALSE;
        }
    return TRUE;
}

staticfn void
solidify_map(void)
{
    coordxy x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (IS_STWALL(levl[x][y].typ) && !SpLev_Map[x][y])
                levl[x][y].wall_info |= (W_NONDIGGABLE | W_NONPASSWALL);
}

void
lvlfill_maze_grid(int x1, int y1, int x2, int y2, schar filling)
{
    int x, y;
    for (x = x1; x <= x2; x++) {
        for (y = y1; y <= y2; y++) {
            /* avoid overwriting maze rooms */
            if (levl[x][y].roomno == NO_ROOM) {
                if (y < 2 || ((x % 2) && (y % 2))) {
                    levl[x][y].typ = STONE;
                }
                else {
                    levl[x][y].typ = filling;
                }
            }
        }
    }
}

/* do a post-level-creation cleanup of map, such as
   removing boulders and traps from lava */
staticfn void
map_cleanup(void)
{
    struct obj *otmp;
    struct trap *ttmp;
    struct engr *etmp;
    coordxy x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            schar typ = levl[x][y].typ;

            if (IS_LAVA(typ) || IS_POOL(typ)) {
                /* in case any boulders are on liquid, delete them */
                while ((otmp = sobj_at(BOULDER, x, y)) != 0) {
                    obj_extract_self(otmp);
                    obfree(otmp, (struct obj *) 0);
                }

                /* traps on liquid? */
                if (((ttmp = t_at(x, y)) != 0)
                    && !undestroyable_trap(ttmp->ttyp))
                    deltrap(ttmp);

                /* engravings? */
                if ((etmp = engr_at(x, y)) != 0)
                  del_engr(etmp);
            }
        }
}

void
lvlfill_solid(schar filling, schar lit)
{
    int x, y;

    for (x = 2; x <= gx.x_maze_max; x++)
        for (y = 0; y <= gy.y_maze_max; y++) {
            if (!set_levltyp_lit(x, y, filling, lit))
                continue;
            /* TODO: consolidate this w lspo_map ? */
            levl[x][y].flags = 0;
            levl[x][y].horizontal = 0;
            levl[x][y].roomno = 0;
            levl[x][y].edge = 0;
        }
}

staticfn void
lvlfill_swamp(schar fg, schar bg, schar lit)
{
    int x, y;

    lvlfill_solid(bg, lit);

    /* "relaxed blockwise maze" algorithm, Jamis Buck */
    for (x = 2; x <= min(gx.x_maze_max, COLNO-2); x += 2)
        for (y = 0; y <= min(gy.y_maze_max, ROWNO-2); y += 2) {
            int c = 0;

            (void) set_levltyp_lit(x, y, fg, lit);
            if (levl[x + 1][y].typ == bg)
                ++c;
            if (levl[x][y + 1].typ == bg)
                ++c;
            if (levl[x + 1][y + 1].typ == bg)
                ++c;
            if (c == 3) {
                switch (rn2(3)) {
                case 0:
                    (void) set_levltyp_lit(x + 1,y, fg, lit);
                    break;
                case 1:
                    (void) set_levltyp_lit(x, y + 1, fg, lit);
                    break;
                case 2:
                    (void) set_levltyp_lit(x + 1, y + 1, fg, lit);
                    break;
                default:
                    break;
                }
            }
        }
}

staticfn void
flip_dbridge_horizontal(struct rm *lev)
{
    if (IS_DRAWBRIDGE(lev->typ)) {
        if ((lev->drawbridgemask & DB_DIR) == DB_WEST) {
            lev->drawbridgemask &= ~DB_WEST;
            lev->drawbridgemask |=  DB_EAST;
        } else if ((lev->drawbridgemask & DB_DIR) == DB_EAST) {
            lev->drawbridgemask &= ~DB_EAST;
            lev->drawbridgemask |=  DB_WEST;
        }
    }
}

staticfn void
flip_dbridge_vertical(struct rm *lev)
{
    if (IS_DRAWBRIDGE(lev->typ)) {
        if ((lev->drawbridgemask & DB_DIR) == DB_NORTH) {
            lev->drawbridgemask &= ~DB_NORTH;
            lev->drawbridgemask |=  DB_SOUTH;
        } else if ((lev->drawbridgemask & DB_DIR) == DB_SOUTH) {
            lev->drawbridgemask &= ~DB_SOUTH;
            lev->drawbridgemask |=  DB_NORTH;
        }
    }
}

/* for #wizfliplevel; not needed when flipping during level creation;
   update seen vector for whole flip area and glyph for known walls */
staticfn void
flip_visuals(int flp, int minx, int miny, int maxx, int maxy)
{
    struct rm *lev;
    int x, y, seenv;

    for (y = miny; y <= maxy; ++y) {
        for (x = minx; x <= maxx; ++x) {
            lev = &levl[x][y];
            seenv = lev->seenv & 0xff;
            /* locations which haven't been seen can be skipped */
            if (seenv == 0)
                continue;
            /* flip <x,y>'s seen vector; not necessary for locations seen
               from all directions (the whole level after magic mapping) */
            if (seenv != SVALL) {
                /* SV2 SV1 SV0 *
                 * SV3 -+- SV7 *
                 * SV4 SV5 SV6 */
                if (flp & 1) { /* swap top and bottom */
                    seenv = swapbits(seenv, 2, 4);
                    seenv = swapbits(seenv, 1, 5);
                    seenv = swapbits(seenv, 0, 6);
                }
                if (flp & 2) { /* swap left and right */
                    seenv = swapbits(seenv, 2, 0);
                    seenv = swapbits(seenv, 3, 7);
                    seenv = swapbits(seenv, 4, 6);
                }
                lev->seenv = (uchar) seenv;
            }
            /* if <x,y> is displayed as a wall, reset its display glyph so
               that remembered, out of view T's and corners get flipped */
            if ((IS_WALL(lev->typ) || lev->typ == SDOOR)
                && glyph_is_cmap(lev->glyph))
                lev->glyph = back_to_glyph(x, y);
        }
    }
}

/* transpose an encoded direction */
staticfn int
flip_encoded_dir_bits(int flp, int val)
{
    /* these depend on xdir[] and ydir[] order */
    if (flp & 1) {
        val = swapbits(val, 1, 7);
        val = swapbits(val, 2, 6);
        val = swapbits(val, 3, 5);
    }
    if (flp & 2) {
        val = swapbits(val, 1, 3);
        val = swapbits(val, 0, 4);
        val = swapbits(val, 7, 5);
    }

    return val;
}

#define FlipX(val) ((maxx - (val)) + minx)
#define FlipY(val) ((maxy - (val)) + miny)
#define inFlipArea(x,y) \
    ((x) >= minx && (x) <= maxx && (y) >= miny && (y) <= maxy)
#define Flip_coord(cc) \
    do {                                            \
        if ((cc).x && inFlipArea((cc).x, (cc).y)) { \
            if (flp & 1)                            \
                (cc).y = FlipY((cc).y);             \
            if (flp & 2)                            \
                (cc).x = FlipX((cc).x);             \
        }                                           \
    } while (0)

/* transpose top with bottom or left with right or both; sometimes called
   for new special levels, or for any level via the #wizfliplevel command */
void
flip_level(
    int flp,        /* mask for orientation(s) to transpose */
    boolean extras) /* False: level creation; True: #wizfliplevel is
                     * altering an active level so more needs to be done */
{
    int x, y, i, itmp;
    coordxy minx, miny, maxx, maxy;
    struct rm trm;
    struct trap *ttmp;
    struct obj *otmp;
    struct monst *mtmp;
    struct engr *etmp;
    struct mkroom *sroom;
    timer_element *timer;
    boolean ball_active = FALSE, ball_fliparea = FALSE;
    stairway *stway;
    struct exclusion_zone *ez;

    /* nothing to do unless (flp & 1) or (flp & 2) or both */
    if ((flp & 3) == 0)
        return;

    get_level_extends(&minx, &miny, &maxx, &maxy);
    /* get_level_extends() returns -1,-1 to COLNO,ROWNO at max */
    if (miny < 0)
        miny = 0;
    if (minx < 1)
        minx = 1;
    if (maxx >= COLNO)
        maxx = (COLNO - 1);
    if (maxy >= ROWNO)
        maxy = (ROWNO - 1);

    if (extras) {
        if (Punished && uball->where != OBJ_FREE) {
            ball_active = TRUE;
            /* if hero and ball and chain are all inside flip area,
               flip b&c coordinates along with other objects; if they
               are all outside, leave them to be rejected when flipping
               so that they stay as is; if some are inside and some are
               outside, un-place here and subsequently re-place them on
               hero's [possibly new] spot below */
            if (carried(uball))
                uball->ox = u.ux, uball->oy = u.uy;
            ball_fliparea = ((inFlipArea(uball->ox, uball->oy)
                              == inFlipArea(uchain->ox, uchain->oy))
                             && (inFlipArea(uball->ox, uball->oy)
                                 == inFlipArea(u.ux, u.uy)));
            if (!ball_fliparea)
                unplacebc();
        }
    }

    /* stairs and ladders */
    for (stway = gs.stairs; stway; stway = stway->next) {
        if (flp & 1)
            stway->sy = FlipY(stway->sy);
        if (flp & 2)
            stway->sx = FlipX(stway->sx);
    }

    /* traps */
    for (ttmp = gf.ftrap; ttmp; ttmp = ttmp->ntrap) {
        if (!inFlipArea(ttmp->tx, ttmp->ty))
            continue;
        if (flp & 1) {
            ttmp->ty = FlipY(ttmp->ty);
            if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
                ttmp->launch.y = FlipY(ttmp->launch.y);
                ttmp->launch2.y = FlipY(ttmp->launch2.y);
            } else if (is_pit(ttmp->ttyp) && ttmp->conjoined) {
                ttmp->conjoined = flip_encoded_dir_bits(flp, ttmp->conjoined);
            }
        }
        if (flp & 2) {
            ttmp->tx = FlipX(ttmp->tx);
            if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
                ttmp->launch.x = FlipX(ttmp->launch.x);
                ttmp->launch2.x = FlipX(ttmp->launch2.x);
            } else if (is_pit(ttmp->ttyp) && ttmp->conjoined) {
                ttmp->conjoined = flip_encoded_dir_bits(flp, ttmp->conjoined);
            }
        }
    }

    /* objects */
    for (otmp = fobj; otmp; otmp = otmp->nobj) {
        if (!inFlipArea(otmp->ox, otmp->oy))
            continue;
        if (flp & 1)
            otmp->oy = FlipY(otmp->oy);
        if (flp & 2)
            otmp->ox = FlipX(otmp->ox);
    }

    /* buried objects */
    for (otmp = svl.level.buriedobjlist; otmp; otmp = otmp->nobj) {
        if (!inFlipArea(otmp->ox, otmp->oy))
            continue;
        if (flp & 1)
            otmp->oy = FlipY(otmp->oy);
        if (flp & 2)
            otmp->ox = FlipX(otmp->ox);
    }

    /* monsters */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (mtmp->isgd) {
            if (extras) /* flip mtmp->mextra->egd */
                flip_vault_guard(flp, mtmp, minx, miny, maxx, maxy);
            if (mtmp->mx == 0) /* not on map so don't flip guard->mx,my */
                continue;
        }
        /* skip the occasional earth elemental outside the flip area */
        if (!inFlipArea(mtmp->mx, mtmp->my))
            continue;
        if (flp & 1)
            mtmp->my = FlipY(mtmp->my);
        if (flp & 2)
            mtmp->mx = FlipX(mtmp->mx);

        if (mtmp->ispriest) {
            Flip_coord(EPRI(mtmp)->shrpos);
        } else if (mtmp->isshk) {
            Flip_coord(ESHK(mtmp)->shk); /* shk's preferred spot */
            Flip_coord(ESHK(mtmp)->shd); /* shop door */
        } else if (mtmp->wormno) {
            if (flp & 1)
                flip_worm_segs_vertical(mtmp, miny, maxy);
            if (flp & 2)
                flip_worm_segs_horizontal(mtmp, minx, maxx);
        }
#if 0   /* not useful unless tracking also gets flipped */
        if (extras) {
            if (mtmp->tame && has_edog(mtmp))
                Flip_coord(EDOG(mtmp)->ogoal);
        }
#endif
    }
    if (extras) { /* #wizfliplevel rather than level creation */
        for (mtmp = gm.migrating_mons; mtmp; mtmp = mtmp->nmon) {
            if (mtmp->isgd && on_level(&u.uz, &EGD(mtmp)->gdlevel)) {
                flip_vault_guard(flp, mtmp, minx, miny, maxx, maxy); /* egd */
            } else if (mtmp->ispriest
                       && on_level(&u.uz, &EPRI(mtmp)->shrlevel)) {
                Flip_coord(EPRI(mtmp)->shrpos); /* priest's altar */
            } else if (mtmp->isshk
                       && on_level(&u.uz, &ESHK(mtmp)->shoplevel)) {
                Flip_coord(ESHK(mtmp)->shk); /* shk's preferred spot */
                Flip_coord(ESHK(mtmp)->shd); /* shop door */
            }
        }
    }

    /* engravings */
    for (etmp = head_engr; etmp; etmp = etmp->nxt_engr) {
        if (flp & 1)
            etmp->engr_y = FlipY(etmp->engr_y);
        if (flp & 2)
            etmp->engr_x = FlipX(etmp->engr_x);
    }

    /* level (teleport) regions */
    for (i = 0; i < gn.num_lregions; i++) {
        if (flp & 1) {
            gl.lregions[i].inarea.y1 = FlipY(gl.lregions[i].inarea.y1);
            gl.lregions[i].inarea.y2 = FlipY(gl.lregions[i].inarea.y2);
            if (gl.lregions[i].inarea.y1 > gl.lregions[i].inarea.y2) {
                itmp = gl.lregions[i].inarea.y1;
                gl.lregions[i].inarea.y1 = gl.lregions[i].inarea.y2;
                gl.lregions[i].inarea.y2 = itmp;
            }

            gl.lregions[i].delarea.y1 = FlipY(gl.lregions[i].delarea.y1);
            gl.lregions[i].delarea.y2 = FlipY(gl.lregions[i].delarea.y2);
            if (gl.lregions[i].delarea.y1 > gl.lregions[i].delarea.y2) {
                itmp = gl.lregions[i].delarea.y1;
                gl.lregions[i].delarea.y1 = gl.lregions[i].delarea.y2;
                gl.lregions[i].delarea.y2 = itmp;
            }
        }
        if (flp & 2) {
            gl.lregions[i].inarea.x1 = FlipX(gl.lregions[i].inarea.x1);
            gl.lregions[i].inarea.x2 = FlipX(gl.lregions[i].inarea.x2);
            if (gl.lregions[i].inarea.x1 > gl.lregions[i].inarea.x2) {
                itmp = gl.lregions[i].inarea.x1;
                gl.lregions[i].inarea.x1 = gl.lregions[i].inarea.x2;
                gl.lregions[i].inarea.x2 = itmp;
            }

            gl.lregions[i].delarea.x1 = FlipX(gl.lregions[i].delarea.x1);
            gl.lregions[i].delarea.x2 = FlipX(gl.lregions[i].delarea.x2);
            if (gl.lregions[i].delarea.x1 > gl.lregions[i].delarea.x2) {
                itmp = gl.lregions[i].delarea.x1;
                gl.lregions[i].delarea.x1 = gl.lregions[i].delarea.x2;
                gl.lregions[i].delarea.x2 = itmp;
            }
        }
    }

    /* regions (poison clouds, etc) */
    for (i = 0; i < svn.n_regions; i++) {
        int j, tmp1, tmp2;
        if (flp & 1) {
            tmp1 = FlipY(gr.regions[i]->bounding_box.ly);
            tmp2 = FlipY(gr.regions[i]->bounding_box.hy);
            gr.regions[i]->bounding_box.ly = min(tmp1, tmp2);
            gr.regions[i]->bounding_box.hy = max(tmp1, tmp2);
            for (j = 0; j < gr.regions[i]->nrects; j++) {
                tmp1 = FlipY(gr.regions[i]->rects[j].ly);
                tmp2 = FlipY(gr.regions[i]->rects[j].hy);
                gr.regions[i]->rects[j].ly = min(tmp1, tmp2);
                gr.regions[i]->rects[j].hy = max(tmp1, tmp2);
            }
        }
        if (flp & 2) {
            tmp1 = FlipX(gr.regions[i]->bounding_box.lx);
            tmp2 = FlipX(gr.regions[i]->bounding_box.hx);
            gr.regions[i]->bounding_box.lx = min(tmp1, tmp2);
            gr.regions[i]->bounding_box.hx = max(tmp1, tmp2);
            for (j = 0; j < gr.regions[i]->nrects; j++) {
                tmp1 = FlipX(gr.regions[i]->rects[j].lx);
                tmp2 = FlipX(gr.regions[i]->rects[j].hx);
                gr.regions[i]->rects[j].lx = min(tmp1, tmp2);
                gr.regions[i]->rects[j].hx = max(tmp1, tmp2);
            }
        }
    }

    /* rooms */
    for (sroom = &svr.rooms[0]; ; sroom++) {
        if (sroom->hx < 0)
            break;

        if (flp & 1) {
            sroom->ly = FlipY(sroom->ly);
            sroom->hy = FlipY(sroom->hy);
            if (sroom->ly > sroom->hy) {
                itmp = sroom->ly;
                sroom->ly = sroom->hy;
                sroom->hy = itmp;
            }
        }
        if (flp & 2) {
            sroom->lx = FlipX(sroom->lx);
            sroom->hx = FlipX(sroom->hx);
            if (sroom->lx > sroom->hx) {
                itmp = sroom->lx;
                sroom->lx = sroom->hx;
                sroom->hx = itmp;
            }
        }

        if (sroom->nsubrooms)
            for (i = 0; i < sroom->nsubrooms; i++) {
                struct mkroom *rroom = sroom->sbrooms[i];

                if (flp & 1) {
                    rroom->ly = FlipY(rroom->ly);
                    rroom->hy = FlipY(rroom->hy);
                    if (rroom->ly > rroom->hy) {
                        itmp = rroom->ly;
                        rroom->ly = rroom->hy;
                        rroom->hy = itmp;
                    }
                }
                if (flp & 2) {
                    rroom->lx = FlipX(rroom->lx);
                    rroom->hx = FlipX(rroom->hx);
                    if (rroom->lx > rroom->hx) {
                        itmp = rroom->lx;
                        rroom->lx = rroom->hx;
                        rroom->hx = itmp;
                    }
                }
            }
    }

    /* doors */
    for (i = 0; i < gd.doorindex; i++) {
        Flip_coord(svd.doors[i]);
    }

    /* the map */
    if (flp & 1) {
        for (x = minx; x <= maxx; x++)
            for (y = miny; y < (miny + ((maxy - miny + 1) / 2)); y++) {
                int ny = FlipY(y);

                flip_dbridge_vertical(&levl[x][y]);
                flip_dbridge_vertical(&levl[x][ny]);

                trm = levl[x][y];
                levl[x][y] = levl[x][ny];
                levl[x][ny] = trm;

                otmp = svl.level.objects[x][y];
                svl.level.objects[x][y] = svl.level.objects[x][ny];
                svl.level.objects[x][ny] = otmp;

                mtmp = svl.level.monsters[x][y];
                svl.level.monsters[x][y] = svl.level.monsters[x][ny];
                svl.level.monsters[x][ny] = mtmp;
            }
    }
    if (flp & 2) {
        for (x = minx; x < (minx + ((maxx - minx + 1) / 2)); x++)
            for (y = miny; y <= maxy; y++) {
                int nx = FlipX(x);

                flip_dbridge_horizontal(&levl[x][y]);
                flip_dbridge_horizontal(&levl[nx][y]);

                trm = levl[x][y];
                levl[x][y] = levl[nx][y];
                levl[nx][y] = trm;

                otmp = svl.level.objects[x][y];
                svl.level.objects[x][y] = svl.level.objects[nx][y];
                svl.level.objects[nx][y] = otmp;

                mtmp = svl.level.monsters[x][y];
                svl.level.monsters[x][y] = svl.level.monsters[nx][y];
                svl.level.monsters[nx][y] = mtmp;
            }
    }

    /* timed effects */
    for (timer = gt.timer_base; timer; timer = timer->next) {
        if (timer->func_index == MELT_ICE_AWAY) {
            long ty = timer->arg.a_long & 0xffff;
            long tx = (timer->arg.a_long >> 16) & 0xffff;

            if (flp & 1)
                ty = FlipY(ty);
            if (flp & 2)
                tx = FlipX(tx);
            timer->arg.a_long = ((tx << 16) | ty);
        }
    }

    /* exclusion zones */
    for (ez = sve.exclusion_zones; ez; ez = ez->next) {
        if (flp & 1) {
            ez->ly = FlipY(ez->ly);
            ez->hy = FlipY(ez->hy);
            if (ez->ly > ez->hy) {
                itmp = ez->ly;
                ez->ly = ez->hy;
                ez->hy = itmp;
            }
        }
        if (flp & 2) {
            ez->lx = FlipX(ez->lx);
            ez->hx = FlipX(ez->hx);
            if (ez->lx > ez->hx) {
                itmp = ez->lx;
                ez->lx = ez->hx;
                ez->hx = itmp;
            }
        }
    }

    if (extras) { /* for #wizfliplevel rather than during level creation */
        /* flip hero location only if inside the flippable area */
        if (inFlipArea(u.ux, u.uy)) {
            if (flp & 1)
                u.uy = FlipY(u.uy);
            if (flp & 2)
                u.ux = FlipX(u.ux);
            /* we could flip <ux0,uy0> too if it's inside the flip area,
               but have to resort to this if outside, so just do this */
            u.ux0 = u.ux, u.uy0 = u.uy;
        }
        if (ball_active && !ball_fliparea)
            placebc();
        Flip_coord(iflags.travelcc);
        Flip_coord(svc.context.digging.pos);
    }

    fix_wall_spines(1, 0, COLNO - 1, ROWNO - 1);
    if (extras && flp) {
        set_wall_state();
        /* after wall_spines; flips seenv and wall joins */
        flip_visuals(flp, minx, miny, maxx, maxy);
    }
    vision_reset();
}

/* for #wizfliplevel, flip guard's egd data; not needed for level creation */
staticfn void
flip_vault_guard(
    int flp, /* 1: transpose vertically, 2: transpose horizontally, 3: both */
    struct monst *grd, /* the vault guard, has monst->mextra->egd data */
    coordxy minx, coordxy miny, /* needed by FlipX(), FlipY(), */
    coordxy maxx, coordxy maxy) /* and inFlipArea() macros     */
{
    int i;
    struct egd *egd = EGD(grd);

    if (inFlipArea(egd->gdx, egd->gdy)) {
        if (flp & 1)
            egd->gdy = FlipY(egd->gdy);
        if (flp & 2)
            egd->gdx = FlipX(egd->gdx);
    }
    if (inFlipArea(egd->ogx, egd->ogy)) {
        if (flp & 1)
            egd->ogy = FlipY(egd->ogy);
        if (flp & 2)
            egd->ogx = FlipX(egd->ogx);
    }
    for (i = egd->fcbeg; i < egd->fcend; ++i) {
        coordxy fx = egd->fakecorr[i].fx, fy = egd->fakecorr[i].fy;

        if (inFlipArea(fx, fy)) {
            if (flp & 1)
                egd->fakecorr[i].fy = FlipY(fy);
            if (flp & 2)
                egd->fakecorr[i].fx = FlipX(fx);
        }
    }
    return;
}

#undef FlipX
#undef FlipY
#undef inFlipArea

/* randomly transpose top with bottom or left with right or both;
   caller controls which transpositions are allowed */
void
flip_level_rnd(int flp, boolean extras)
{
    int c = 0;

    /* TODO?
     *  Might change rn2(2) to !rn2(3) or (rn2(5) < 2) in order to bias
     *  the outcome towards the traditional orientation.
     */
    if ((flp & 1) && rn2(2))
        c |= 1;
    if ((flp & 2) && rn2(2))
        c |= 2;

    if (c)
        flip_level(c, extras);
}


staticfn void
sel_set_wall_property(coordxy x, coordxy y, genericptr_t arg)
{
    int prop = *(int *) arg;

    if (IS_STWALL(levl[x][y].typ) || IS_TREE(levl[x][y].typ)
        /* 3.6.2: made iron bars eligible to be flagged nondiggable
           (checked by chewing(hack.c) and zap_over_floor(zap.c)) */
        || levl[x][y].typ == IRONBARS)
        levl[x][y].wall_info |= prop;
}

/*
 * Make walls of the area (x1, y1, x2, y2) non diggable/non passwall-able
 */
staticfn void
set_wall_property(coordxy x1, coordxy y1, coordxy x2, coordxy y2, int prop)
{
    coordxy x, y;

    x1 = max(x1, 1);
    x2 = min(x2, COLNO - 1);
    y1 = max(y1, 0);
    y2 = min(y2, ROWNO - 1);
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++) {
            sel_set_wall_property(x, y, (genericptr_t) &prop);
        }
}

staticfn void
remove_boundary_syms(void)
{
    /*
     * If any CROSSWALLs are found, must change to ROOM after REGION's
     * are laid out.  CROSSWALLS are used to specify "invisible"
     * boundaries where DOOR syms look bad or aren't desirable.
     */
    coordxy x, y;
    boolean has_bounds = FALSE;

    for (x = 0; x < COLNO - 1; x++)
        for (y = 0; y < ROWNO - 1; y++)
            if (levl[x][y].typ == CROSSWALL) {
                has_bounds = TRUE;
                break;
            }
    if (has_bounds) {
        for (x = 0; x < gx.x_maze_max; x++)
            for (y = 0; y < gy.y_maze_max; y++)
                if ((levl[x][y].typ == CROSSWALL) && SpLev_Map[x][y])
                    levl[x][y].typ = ROOM;
    }
}

/* used by sel_set_door() and link_doors_rooms() */
staticfn void
set_door_orientation(int x, int y)
{
    boolean wleft, wright, wup, wdown;

    /* If there's a wall or door on either the left side or right
     * side (or both) of this secret door, make it be horizontal.
     *
     * It is feasible to put SDOOR in a corner, tee, or crosswall
     * position, although once the door is found and opened it won't
     * make a lot sense (diagonal access required).  Still, we try to
     * handle that as best as possible.  For top or bottom tee, using
     * horizontal is the best we can do.  For corner or crosswall,
     * either horizontal or vertical are just as good as each other;
     * we produce horizontal for corners and vertical for crosswalls.
     * For left or right tee, using vertical is best.
     *
     * A secret door with no adjacent walls is also feasible and makes
     * even less sense.  It will be displayed as a vertical wall while
     * hidden and become a vertical door when found.  Before resorting
     * to that, we check for solid rock which hasn't been wallified
     * yet (cf lower leftside of leader's room in Cav quest).
     */
    wleft  = (isok(x - 1, y) && (IS_WALL(levl[x - 1][y].typ)
                                 || IS_DOOR(levl[x - 1][y].typ)
                                 || levl[x - 1][y].typ == SDOOR));
    wright = (isok(x + 1, y) && (IS_WALL(levl[x + 1][y].typ)
                                 || IS_DOOR(levl[x + 1][y].typ)
                                 || levl[x + 1][y].typ == SDOOR));
    wup    = (isok(x, y - 1) && (IS_WALL(levl[x][y - 1].typ)
                                 || IS_DOOR(levl[x][y - 1].typ)
                                 || levl[x][y - 1].typ == SDOOR));
    wdown  = (isok(x, y + 1) && (IS_WALL(levl[x][y + 1].typ)
                                 || IS_DOOR(levl[x][y + 1].typ)
                                 || levl[x][y + 1].typ == SDOOR));
    if (!wleft && !wright && !wup && !wdown) {
        /* out of bounds is treated as implicit wall; should be academic
           because we don't expect to have doors so near the level's edge */
        wleft  = (!isok(x - 1, y) || IS_DOORJOIN(levl[x - 1][y].typ));
        wright = (!isok(x + 1, y) || IS_DOORJOIN(levl[x + 1][y].typ));
        wup    = (!isok(x, y - 1) || IS_DOORJOIN(levl[x][y - 1].typ));
        wdown  = (!isok(x, y + 1) || IS_DOORJOIN(levl[x][y + 1].typ));
    }
    levl[x][y].horizontal = ((wleft || wright) && !(wup && wdown)) ? 1 : 0;
}

/* is x,y right next to room droom? */
staticfn boolean
shared_with_room(int x, int y, struct mkroom *droom)
{
    int rmno = (droom - svr.rooms) + ROOMOFFSET;

    if (!isok(x,y))
        return FALSE;
    if ((int) levl[x][y].roomno == rmno && !levl[x][y].edge)
        return FALSE;
    if (isok(x-1, y) && (int) levl[x-1][y].roomno == rmno && x-1 <= droom->hx)
        return TRUE;
    if (isok(x+1, y) && (int) levl[x+1][y].roomno == rmno && x+1 >= droom->lx)
        return TRUE;
    if (isok(x, y-1) && (int) levl[x][y-1].roomno == rmno && y-1 <= droom->hy)
        return TRUE;
    if (isok(x, y+1) && (int) levl[x][y+1].roomno == rmno && y+1 >= droom->ly)
        return TRUE;
    return FALSE;
}

/* maybe add door at x,y to room droom */
staticfn void
maybe_add_door(int x, int y, struct mkroom *droom)
{
    if (droom->hx >= 0
        && ((!droom->irregular && inside_room(droom, x, y))
            || (int) levl[x][y].roomno == (droom - svr.rooms) + ROOMOFFSET
            || shared_with_room(x, y, droom))) {
        add_door(x, y, droom);
    }
}

/* link all doors in the map to their corresponding rooms */
staticfn void
link_doors_rooms(void)
{
    int x, y;
    int tmpi, m;

    for (y = 0; y < ROWNO; y++)
        for (x = 0; x < COLNO; x++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR) {
                /* in case this door was a '+' or 'S' from the
                   MAP...ENDMAP section without an explicit DOOR
                   directive, set/clear levl[][].horizontal for it */
                set_door_orientation(x, y);

                for (tmpi = 0; tmpi < svn.nroom; tmpi++) {
                    maybe_add_door(x, y, &svr.rooms[tmpi]);
                    for (m = 0; m < svr.rooms[tmpi].nsubrooms; m++) {
                        maybe_add_door(x, y, svr.rooms[tmpi].sbrooms[m]);
                    }
                }
            }
}

/*
 * Select a random trap
 */
staticfn int
rndtrap(void)
{
    int rtrap;

    do {
        rtrap = rnd(TRAPNUM - 1);
        switch (rtrap) {
        case HOLE: /* no random holes on special levels */
        case VIBRATING_SQUARE:
        case MAGIC_PORTAL:
            rtrap = NO_TRAP;
            break;
        case TRAPDOOR:
            if (!Can_dig_down(&u.uz))
                rtrap = NO_TRAP;
            break;
        case LEVEL_TELEP:
        case TELEP_TRAP:
            if (svl.level.flags.noteleport && !Is_telemaze_lev(&u.uz))
                rtrap = NO_TRAP;
            break;
        case ROCKTRAP:
            if (svl.level.flags.outdoors)
                rtrap = NO_TRAP;
            FALLTHROUGH;
            /* FALLTHRU */
        case ROLLING_BOULDER_TRAP:
            if (In_endgame(&u.uz))
                rtrap = NO_TRAP;
            break;
        }
    } while (rtrap == NO_TRAP);
    return rtrap;
}

/*
 * Translate a given coordinate from a special level definition into an actual
 * location on the map.
 *
 * If x or y is negative, we generate a random coordinate within the area. If
 * not negative, they are interpreted as relative to the last defined map or
 * room, and are output as absolute svl.level.locations coordinates.
 *
 * The "humidity" flag is used to ensure that engravings aren't created
 * underwater, or eels on dry land.
 */
staticfn void
get_location(
    coordxy *x, coordxy *y,
    getloc_flags_t humidity,
    struct mkroom *croom)
{
    int cpt = 0;
    int mx, my, sx, sy;

    if (croom) {
        mx = croom->lx;
        my = croom->ly;
        sx = croom->hx - mx + 1;
        sy = croom->hy - my + 1;
    } else {
        mx = gx.xstart;
        my = gy.ystart;
        sx = gx.xsize;
        sy = gy.ysize;
    }

    if (*x >= 0) { /* normal locations */
        *x += mx;
        *y += my;
    } else { /* random location */
        do {
            if (croom) { /* handle irregular areas */
                coord tmpc;
                (void) somexy(croom, &tmpc);
                *x = tmpc.x;
                *y = tmpc.y;
            } else {
                *x = mx + rn2((int) sx);
                *y = my + rn2((int) sy);
            }
            if (is_ok_location(*x, *y, humidity))
                break;
        } while (++cpt < 100);
        if (cpt >= 100) {
            int xx, yy;

            /* last try */
            for (xx = 0; xx < sx; xx++)
                for (yy = 0; yy < sy; yy++) {
                    *x = mx + xx;
                    *y = my + yy;
                    if (is_ok_location(*x, *y, humidity))
                        goto found_it;
                }
            if (!(humidity & NO_LOC_WARN)) {
                impossible("get_location:  can't find a place!");
            } else {
                *x = *y = -1;
            }
        }
    }
 found_it:
    ;

    if (!(humidity & ANY_LOC) && !isok(*x, *y)) {
        if (!(humidity & NO_LOC_WARN)) {
            /*warning("get_location:  (%d,%d) out of bounds", *x, *y);*/
            *x = gx.x_maze_max;
            *y = gy.y_maze_max;
        } else {
            *x = *y = -1;
        }
    }
}

static boolean (*is_ok_location_func)(coordxy, coordxy) = NULL;

staticfn void
set_ok_location_func(boolean (*func)(coordxy, coordxy))
{
    is_ok_location_func = func;
}

staticfn boolean
is_ok_location(coordxy x, coordxy y, getloc_flags_t humidity)
{
    int typ = levl[x][y].typ;

    if (Is_waterlevel(&u.uz))
        return TRUE; /* accept any spot */

    if (is_ok_location_func)
        return is_ok_location_func(x, y);

    /* TODO: Should perhaps check if wall is diggable/passwall? */
    if (humidity & ANY_LOC)
        return TRUE;

    if ((humidity & SOLID) && IS_OBSTRUCTED(typ))
        return TRUE;

    if (is_open_air(x, y))
        return (boolean) (humidity & NOFLOOR);

    if ((humidity & (DRY|SPACELOC)) && SPACE_POS(typ)) {
        boolean bould = (sobj_at(BOULDER, x, y) != NULL);

        if (!bould || (bould && (humidity & SOLID)))
            return TRUE;
    }
    if ((humidity & WET) && is_pool(x, y))
        return TRUE;
    if ((humidity & HOT) && is_lava(x, y))
        return TRUE;
    return FALSE;
}

boolean
pm_good_location(coordxy x, coordxy y, struct permonst *pm)
{
    return is_ok_location(x, y, pm_to_humidity(pm));
}

staticfn unpacked_coord
get_unpacked_coord(long loc, int defhumidity)
{
    static unpacked_coord c;

    if (loc & SP_COORD_IS_RANDOM) {
        c.x = c.y = -1;
        c.is_random = 1;
        c.getloc_flags = (getloc_flags_t)(loc & ~SP_COORD_IS_RANDOM);
        if (!c.getloc_flags)
            c.getloc_flags = defhumidity;
    } else {
        c.is_random = 0;
        c.getloc_flags = defhumidity;
        c.x = SP_COORD_X(loc);
        c.y = SP_COORD_Y(loc);
    }
    return c;
}

void
get_location_coord(
    coordxy *x, coordxy *y,
    int humidity,
    struct mkroom *croom,
    long crd)
{
    unpacked_coord c;

    c = get_unpacked_coord(crd, humidity);
    *x = c.x;
    *y = c.y;
    get_location(x, y, c.getloc_flags | (c.is_random ? NO_LOC_WARN : 0),
                 croom);

    if (*x == -1 && *y == -1 && c.is_random)
        get_location(x, y, humidity, croom);
}

/*
 * Get a relative position inside a room.
 * negative values for x or y means RANDOM!
 */
staticfn void
get_room_loc(coordxy *x, coordxy *y, struct mkroom *croom)
{
    coord c;

    if (*x < 0 && *y < 0) {
        if (somexy(croom, &c)) {
            *x = c.x;
            *y = c.y;
        } else
            panic("get_room_loc : can't find a place!");
    } else {
        if (*x < 0)
            *x = rn2(croom->hx - croom->lx + 1);
        if (*y < 0)
            *y = rn2(croom->hy - croom->ly + 1);
        *x += croom->lx;
        *y += croom->ly;
    }
}

/*
 * Get a relative position inside a room.
 * negative values for x or y means RANDOM!
 */
staticfn void
get_free_room_loc(
    coordxy *x, coordxy *y,
    struct mkroom *croom,
    packed_coord pos)
{
    coordxy try_x, try_y;
    int trycnt = 0;

    get_location_coord(&try_x, &try_y, DRY, croom, pos);
    if (levl[try_x][try_y].typ != ROOM) {
        do {
            try_x = *x, try_y = *y;
            get_room_loc(&try_x, &try_y, croom);
        } while (levl[try_x][try_y].typ != ROOM && ++trycnt <= 100);

        if (trycnt > 100)
            panic("get_free_room_loc:  can't find a place!");
    }
    *x = try_x, *y = try_y;
}

/* Check the area within XLIM and YLIM around a room to make sure it's valid
 * (containing only STONE terrain). Try to make the room smaller so that it's
 * valid if possible, and pass the changes to the caller.
 * lowx and lowy are the absolute coordinates of the top left of the room floor.
 * ddx and ddy are width-1 and height-1, respectively.
 * Return TRUE if the room was valid or has been changed to be valid, FALSE
 * otherwise.
 * Note that every time it finds some non-STONE terrain in the area around the
 * room, it will fail outright with 1/3 chance.
 * Also note that the most other pieces of room generation code use a +1 to
 * XLIM and YLIM to represent the walls, which aren't counted as part of the
 * area of the room. However, this function does not add 1 for the walls, for
 * some reason. */
boolean
check_room(
    coordxy *lowx, coordxy *ddx,
    coordxy *lowy, coordxy *ddy,
    boolean vault)
{
    int x, y, hix = *lowx + *ddx, hiy = *lowy + *ddy;
    struct rm *lev;
    int xlim, ylim, ymax;
    coordxy s_lowx, s_ddx, s_lowy, s_ddy;

    s_lowx = *lowx; s_ddx = *ddx;
    s_lowy = *lowy; s_ddy = *ddy;

    xlim = XLIM + (vault ? 1 : 0);
    ylim = YLIM + (vault ? 1 : 0);

    if (*lowx < 3)
        *lowx = 3;
    if (*lowy < 2)
        *lowy = 2;
    if (hix > COLNO - 3)
        hix = COLNO - 3;
    if (hiy > ROWNO - 3)
        hiy = ROWNO - 3;
 chk:
    if (hix <= *lowx || hiy <= *lowy)
        return FALSE;

    if (gi.in_mk_themerooms && (s_lowx != *lowx) && (s_ddx != *ddx)
        && (s_lowy != *lowy) && (s_ddy != *ddy))
        return FALSE;

    /* check area around room (and make room smaller if necessary) */
    for (x = *lowx - xlim; x <= hix + xlim; x++) {
        if (x <= 0 || x >= COLNO)
            continue;
        y = *lowy - ylim;
        ymax = hiy + ylim;
        if (y < 0)
            y = 0;
        if (ymax >= ROWNO)
            ymax = (ROWNO - 1);
        lev = &levl[x][y];
        for (; y <= ymax; y++) {
            if (lev++->typ != STONE) {
                if (!vault) {
                    debugpline2("strange area [%d,%d] in check_room.", x, y);
                }
                if (!rn2(3))
                    return FALSE;
                if (gi.in_mk_themerooms)
                    return FALSE;
                if (svl.level.flags.is_maze_lev)
                    return FALSE;
                if (x < *lowx)
                    *lowx = x + xlim + 1;
                else
                    hix = x - xlim - 1;
                if (y < *lowy)
                    *lowy = y + ylim + 1;
                else
                    hiy = y - ylim - 1;
                goto chk;
            }
        }
    }
    *ddx = hix - *lowx;
    *ddy = hiy - *lowy;

    if (gi.in_mk_themerooms && (s_lowx != *lowx) && (s_ddx != *ddx)
        && (s_lowy != *lowy) && (s_ddy != *ddy))
        return FALSE;

    return TRUE;
}

/*
 * Create a room with the specified position, dimensions, alignment, room type,
 * and lighting.
 * Any of these arguments can be made random by setting them to -1.
 * If they are all random, it will use a slightly different algorithm for
 * placing the room (this is intended for normal filler level rooms). This
 * algorithm looks for available rects to begin with, instead of setting the
 * room location first and then trying to find a rect it fits in.
 * Otherwise:
 *   x, y: Absolute coordinates for the room. If either is -1, it will choose
 *   a random value within the screen bounds.
 *   w, h: Width and height of the room. If _either_ is -1, width is set to
 *   d15 + 2 and height is set to d8 + 1.
 *   xal, yal: Room alignment (should be SPLEV_LEFT, SPLEV_RIGHT, SPLEV_CENTER /
 *     SPLEV_TOP, SPLEV_BOTTOM, SPLEV_CENTER). Define what point in the room is
 *     represented by x and y; e.g. using SPLEV_CENTER,SPLEV_CENTER means that
 *     (x,y) should be the center of the room. If either is -1 it will pick one
 *     of the three randomly.
 *   rtype: The room type. If -1, it will choose OROOM (*not* a random special
 *     room type).
 *   rlit: Whether to light the room. If -1, it will choose based on the level
 *     depth.
 *  Return TRUE if it successfully created the room, FALSE otherwise.
 */
boolean
create_room(
    coordxy x, coordxy y,
    coordxy w, coordxy h,
    coordxy xal, coordxy yal,
    xint16 rtype, xint16 rlit)
{
    coordxy xabs = 0, yabs = 0;
    int wtmp, htmp, xaltmp, yaltmp, xtmp, ytmp;
    NhRect *r1 = 0, r2;
    int trycnt = 0;
    boolean vault = FALSE;
    int xlim = XLIM, ylim = YLIM;

    /* If the type is random, set to OROOM; don't actually randomize */
    if (rtype == -1)
        rtype = OROOM;

    /* vaults apparently use an extra space of buffer */
    if (rtype == VAULT) {
        vault = TRUE;
        xlim++;
        ylim++;
    }

    /* on low levels the room is lit (usually) */
    /* some other rooms may require lighting */

    /* is light state random ? */
    rlit = litstate_rnd(rlit);

    /* Here we try to create a semi-random or totally random room. Try 100
     * times before giving up.
     * FIXME: if there are no random parameters and the room cannot be created
     * with those parameters, it tries 100 times anyway. */
    do {
        coordxy xborder, yborder;

        wtmp = w;
        htmp = h;
        xtmp = x;
        ytmp = y;
        xaltmp = xal;
        yaltmp = yal;

        /* First case : a totally random room */

        if ((xtmp < 0 && ytmp < 0 && wtmp < 0 && xaltmp < 0 && yaltmp < 0)
            || vault) {
            /* hx, hy, lx, ly: regular rectangle bounds
             * dx, dy: tentative room dimensions minus 1 */
            coordxy hx, hy, lx, ly, dx, dy;

            r1 = rnd_rect(); /* Get a random rectangle */

            if (!r1) { /* No more free rectangles ! */
                debugpline0("No more rects...");
                return FALSE;
            }
            /* set our boundaries to the rectangle's boundaries */
            hx = r1->hx;
            hy = r1->hy;
            lx = r1->lx;
            ly = r1->ly;
            if (vault) { /* always 2x2 */
                dx = dy = 1;
            } else {
                /* if in a very wide rectangle, allow room width to vary from
                 * 3 to 14, otherwise 3 to 10;
                 * vary room height from 3 to 6.
                 * Keeping in mind that dx and dy are the room dimensions
                 * minus 1. */
                dx = 2 + rn2((hx - lx > 28) ? 12 : 8);
                dy = 2 + rn2(4);
                /* force the room to be no more than size 50 */
                if (dx * dy > 50)
                    dy = 50 / dx;
            }

            /* is r1 big enough to contain this room with enough buffer space?
             * If it's touching one or more edges, we can have a looser bound
             * on the border since there won't be other rooms on one side of
             * the rectangle. */
            xborder = (lx > 0 && hx < COLNO - 1) ? 2 * xlim : xlim + 1;
            yborder = (ly > 0 && hy < ROWNO - 1) ? 2 * ylim : ylim + 1;

            /* The rect must have enough width to fit:
             *   1: the room width itself (dx + 1)
             *   2: the room walls (+2)
             *   3: the buffer space on one or both sides (xborder)
             *
             * Possible small bug? If the rectangle contains the hx column and
             * the hy row inclusive, then hx - lx actually returns the width of
             * the rectangle minus 1.
             * For example, if lx = 40 and hx = 50, the rectangle is 11 squares
             * wide. Say xborder is 4 and dx is 4 (room width 5). This rect
             * should be able to fit this room like "  |.....|  " with no spare
             * space. dx + 3 + xborder is the correct 11, but hx - lx is 10, so
             * it won't let the room generate.
             */
            if (hx - lx < dx + 3 + xborder || hy - ly < dy + 3 + yborder) {
                r1 = 0;
                continue;
            }

            /* Finalize the actual coordinates as (xabs, yabs), selecting them
             * uniformly from all possible valid locations to place the room
             * (respecting the xlim and extra wall space rules).
             * There are lots of shims here to make sure we never go below x=3
             * or y=2, why does the rectangle code even allow rectangles to
             * generate like that? */
            xabs = lx + (lx > 0 ? xlim : 3)
                   + rn2(hx - (lx > 0 ? lx : 3) - dx - xborder + 1);
            yabs = ly + (ly > 0 ? ylim : 2)
                   + rn2(hy - (ly > 0 ? ly : 2) - dy - yborder + 1);

            /* Some weird tweaks: if r1 spans the whole level vertically and
             * the bottom of this room would be below the middle of the level
             * vertically, with a 1/(existing rooms) chance, set yabs to a
             * value from 2 to 4.
             * Then, shrink the room width by 1 if we have less than 4 rooms
             * already and the room height >= 3.
             * These are probably to prevent a large vertically centered room
             * from being placed first, which would force the remaining top and
             * bottom rectangles to be fairly narrow and unlikely to generate
             * rooms. The overall effect would be to create a level which is
             * more or less just a horizontal string of rooms, which
             * occasionally does happen under this algorithm.
             */
            if (ly == 0 && hy >= ROWNO - 1
                && (!svn.nroom || !rn2(svn.nroom))
                && (yabs + dy > ROWNO / 2)) {
                yabs = rn1(3, 2);
                if (svn.nroom < 4 && dy > 1)
                    dy--;
            }

            /* If the room or part of the surrounding area are occupied by
             * something else, and we can't shrink the room to fit, abort. */
            if (!check_room(&xabs, &dx, &yabs, &dy, vault)) {
                r1 = 0;
                continue;
            }

            /* praise be, finally setting width and height variables properly */
            wtmp = dx + 1;
            htmp = dy + 1;

            /* Set up r2 with the full area of the room's footprint, including
             * its walls. */
            r2.lx = xabs - 1;
            r2.ly = yabs - 1;
            r2.hx = xabs + wtmp;
            r2.hy = yabs + htmp;
        } else { /* Only some parameters are random */
            xabs = (xtmp < 0) ? rn2(COLNO) : xtmp;
            yabs = (ytmp < 0) ? rn2(COLNO) : ytmp;
            coordxy dx, dy;

            if (wtmp < 0 || htmp < 0) { /* Size is RANDOM */
                wtmp = rn1(15, 3);
                htmp = rn1(8, 2);
            }
            if (xaltmp == -1) /* Horizontal alignment is RANDOM */
                xaltmp = rnd(3);
            if (yaltmp == -1) /* Vertical alignment is RANDOM */
                yaltmp = rnd(3);

            switch (xaltmp) {
            case SPLEV_LEFT:
                break;
            case SPLEV_RIGHT:
                xabs -= wtmp;
                break;
            case SPLEV_CENTER:
                xabs -= wtmp / 2;
                break;
            }
            switch (yaltmp) {
            case SPLEV_TOP:
                break;
            case SPLEV_BOTTOM:
                yabs -= htmp;
                break;
            case SPLEV_CENTER:
                yabs -= htmp / 2;
                break;
            }

            /* make sure room is staying in bounds */
            if (xabs + wtmp - 1 > COLNO - 2)
                xabs = COLNO - wtmp - 3;
            if (xabs < 2)
                xabs = 2;
            if (yabs + htmp - 1 > ROWNO - 2)
                yabs = ROWNO - htmp - 3;
            if (yabs < 2)
                yabs = 2;

            /* Try to find a rectangle that fits our room */
            r2.lx = xabs - 1;
            r2.ly = yabs - 1;
            r2.hx = xabs + wtmp;
            r2.hy = yabs + htmp;
            r1 = get_rect(&r2);
            dx = wtmp;
            dy = htmp;

            if (r1 && !check_room(&xabs, &dx, &yabs, &dy, vault)) {
                r1 = 0;
            }
        }
    } while (++trycnt <= 100 && !r1);

    if (!r1) { /* creation of room failed ? */
        return FALSE;
    }

    /* r2 is contained inside r1: remove r1 and split it into four smaller
     * rectangles representing the areas of r1 that don't intersect with r2. */
    split_rects(r1, &r2);

    if (!vault) {
        /* set this room's id number to be unique for joining purposes */
        gs.smeq[svn.nroom] = svn.nroom;
        /* actually add the room, setting the terrain properly */
        add_room(xabs, yabs, xabs + wtmp - 1, yabs + htmp - 1, rlit, rtype,
                 FALSE);
    } else {
        /* vaults are isolated so don't get added to smeq; also apparently
         * don't have add_room() called on them. The lx and ly is set so that
         * makerooms() can store them in vault_x and vault_y. */
        svr.rooms[svn.nroom].lx = xabs;
        svr.rooms[svn.nroom].ly = yabs;
    }
    return TRUE;
}

/*
 * Create a subroom in room proom at pos x,y with width w & height h.
 * x & y are relative to the parent room.
 */
staticfn boolean
create_subroom(
    struct mkroom *proom,
    coordxy x, coordxy y,
    coordxy w, coordxy h,
    xint16 rtype, xint16 rlit)
{
    coordxy width, height;

    width = proom->hx - proom->lx + 1;
    height = proom->hy - proom->ly + 1;

    /* There is a minimum size for the parent room */
    if (width < 4 || height < 4)
        return FALSE;

    /* Check for random position, size, etc... */

    if (w == -1)
        w = rnd(width - 3);
    if (h == -1)
        h = rnd(height - 3);
    if (x == -1)
        x = rnd(width - w);
    if (y == -1)
        y = rnd(height - h);
    if (x == 1)
        x = 0;
    if (y == 1)
        y = 0;
    if ((x + w + 1) == width)
        x++;
    if ((y + h + 1) == height)
        y++;
    if (rtype == -1)
        rtype = OROOM;
    rlit = litstate_rnd(rlit);
    add_subroom(proom, proom->lx + x, proom->ly + y, proom->lx + x + w - 1,
                proom->ly + y + h - 1, rlit, rtype, FALSE);
    return TRUE;
}

/*
 * Create a new door in a room.
 * It's placed on a wall (north, south, east or west).
 */
staticfn void
create_door(room_door *dd, struct mkroom *broom)
{
    int x = 0, y = 0;
    int trycnt;

    if (dd->wall == W_RANDOM)
        dd->wall = W_ANY; /* speeds things up in the below loop */

    for (trycnt = 0; trycnt < 100; ++trycnt) {
        int dwall = dd->wall, dpos = dd->pos;

        /* Convert wall and pos into an absolute coordinate! */
        switch (rn2(4)) {
        case 0:
            if (!(dwall & W_NORTH))
                continue;
            y = broom->ly - 1;
            x = broom->lx + ((dpos == -1) ? rn2(1 + broom->hx - broom->lx)
                                          : dpos);
            if (!isok(x, y - 1) || IS_OBSTRUCTED(levl[x][y - 1].typ))
                continue;
            break;
        case 1:
            if (!(dwall & W_SOUTH))
                continue;
            y = broom->hy + 1;
            x = broom->lx + ((dpos == -1) ? rn2(1 + broom->hx - broom->lx)
                                          : dpos);
            if (!isok(x, y + 1) || IS_OBSTRUCTED(levl[x][y + 1].typ))
                continue;
            break;
        case 2:
            if (!(dwall & W_WEST))
                continue;
            x = broom->lx - 1;
            y = broom->ly + ((dpos == -1) ? rn2(1 + broom->hy - broom->ly)
                                          : dpos);
            if (!isok(x - 1, y) || IS_OBSTRUCTED(levl[x - 1][y].typ))
                continue;
            break;
        case 3:
            if (!(dwall & W_EAST))
                continue;
            x = broom->hx + 1;
            y = broom->ly + ((dpos == -1) ? rn2(1 + broom->hy - broom->ly)
                                          : dpos);
            if (!isok(x + 1, y) || IS_OBSTRUCTED(levl[x + 1][y].typ))
                continue;
            break;
        default:
            /*NOTREACHED*/
            break;
        }

        if (okdoor(x, y))
            break;
    }
    if (trycnt >= 100) {
        impossible("create_door: Can't find a proper place!");
        return;
    }
    if (!set_levltyp(x, y, (dd->secret ? SDOOR : DOOR)))
        return;
    levl[x][y].doormask = dd->doormask;
    clear_nonsense_doortraps(x, y);
}

/*
 * Create a trap in a room.
 */
staticfn void
create_trap(spltrap *t, struct mkroom *croom)
{
    coordxy x = -1, y = -1;
    coord tm;
    unsigned mktrap_flags = MKTRAP_MAZEFLAG;

    if (croom) {
        get_free_room_loc(&x, &y, croom, t->coord);
    } else {
        int trycnt = 0;

        do {
            get_location_coord(&x, &y, DRY, croom, t->coord);
        } while ((levl[x][y].typ == STAIRS || levl[x][y].typ == LADDER)
                 && !t_at(x, y) && ++trycnt <= 100);
        if (trycnt > 100)
            return;
    }

    if (!t->spider_on_web)
        mktrap_flags |= MKTRAP_NOSPIDERONWEB;
    if (t->seen)
        mktrap_flags |= MKTRAP_SEEN;
    if (t->novictim)
        mktrap_flags |= MKTRAP_NOVICTIM;

    tm.x = x;
    tm.y = y;

    mktrap(t->type, mktrap_flags, (struct mkroom *) 0, &tm);
}

staticfn int
noncoalignment(aligntyp alignment)
{
    int k;

    k = rn2(2);
    if (!alignment)
        return (k ? -1 : 1);
    return (k ? -alignment : 0);
}

/* attempt to screen out locations where a mimic-as-boulder shouldn't occur */
staticfn boolean
m_bad_boulder_spot(coordxy x, coordxy y)
{
    struct rm *lev;

    /* avoid trap locations */
    if (t_at(x, y))
        return TRUE;
    /* try to avoid locations which already have a boulder (this won't
       actually work; we get called before objects have been placed...) */
    if (sobj_at(BOULDER, x, y))
        return TRUE;
    /* avoid closed doors */
    lev = &levl[x][y];
    if (IS_DOOR(lev->typ) && door_is_closed(lev))
        return TRUE;
    /* spot is ok */
    return FALSE;
}

staticfn int
pm_to_humidity(struct permonst *pm)
{
    int loc = DRY;

    if (!pm)
        return loc;
    if (pm->mlet == S_EEL || amphibious(pm) || is_swimmer(pm))
        loc = WET;
    if (!grounded(pm))
        loc |= (HOT | WET | NOFLOOR);
    if (passes_walls(pm) || noncorporeal(pm))
        loc |= SOLID;
    if (likes_fire(pm))
        loc |= HOT;
    return loc;
}

/*
 * Convert a special level alignment mask (an alignment mask with possible
 * extra values/flags) to a "normal" alignment mask (no extra flags).
 *
 * When random: there is an 80% chance that the altar will be co-aligned.
 */
staticfn unsigned int
sp_amask_to_amask(unsigned int sp_amask)
{
    unsigned int amask;

    if (sp_amask == AM_SPLEV_CO)
        amask = Align2amask(u.ualignbase[A_ORIGINAL]);
    else if (sp_amask == AM_SPLEV_NONCO)
        amask = Align2amask(noncoalignment(u.ualignbase[A_ORIGINAL]));
    else if (sp_amask == AM_SPLEV_RANDOM)
        amask = induced_align(80);
    else
        amask = sp_amask & AM_MASK;

    return amask;
}

/*
 * Create a monster in a room.
 */
staticfn void
create_monster(monster* m, struct mkroom* croom)
{
    struct monst *mtmp;
    coordxy x, y;
    char class;
    unsigned int amask;
    coord cc;
    struct permonst *pm;
    unsigned g_mvflags;

    if (m->class >= 0)
        class = (char) def_char_to_monclass((char) m->class);
    else
        class = 0;

    if (class == MAXMCLASSES) {
        impossible("create_monster: unknown monster class '%c'", m->class);
        return;
    }

    amask = sp_amask_to_amask(m->sp_amask);

    if (!class) {
        pm = (struct permonst *) 0;
    } else if (m->id != NON_PM) {
        pm = &mons[m->id];
        g_mvflags = (unsigned) svm.mvitals[monsndx(pm)].mvflags;
        if ((pm->geno & G_UNIQ) && (g_mvflags & G_EXTINCT))
            return;
        if (g_mvflags & G_GONE) /* genocided or extinct */
            pm = (struct permonst *) 0; /* make random monster */
    } else {
        if (m->sp_amask != AM_SPLEV_RANDOM) {
            /* overloading of the "align" field when a class is specified but a
             * specific species is not; produce a monster in this class with
             * this alignment, and avoid the part later where align is
             * interpreted to mean "create a minion of a god".
             * This makes it impossible to generate a minion of an indeterminate
             * species within a definite class, but this does not currently
             * happen and it's unlikely it ever would happen. */
            pm = mkclass_aligned(class, G_NOGEN, Amask2align(amask));
            m->sp_amask = AM_SPLEV_RANDOM;
        }
        else
            pm = mkclass(class, G_NOGEN);
            /* if we can't get a specific monster type (pm == 0) then the
               class has been genocided, so settle for a random monster */
    }
    if (In_mines(&u.uz) && pm && your_race(pm)
        && (Race_if(PM_DWARF) || Race_if(PM_GNOME)) && rn2(3))
        pm = (struct permonst *) 0;

    if (pm) {
        int loc = pm_to_humidity(pm);

        /* If water-liking monster, first try is without DRY */
        get_location_coord(&x, &y, loc | NO_LOC_WARN, croom, m->coord);
        if (x == -1 && y == -1) {
            loc |= DRY;
            get_location_coord(&x, &y, loc, croom, m->coord);
        }
    } else {
        get_location_coord(&x, &y, DRY, croom, m->coord);
    }

    /* try to find a close place if someone else is already there */
    if (MON_AT(x, y) && enexto(&cc, x, y, pm))
        x = cc.x, y = cc.y;

    if (croom && !inside_room(croom, x, y))
        return;

    if (m->sp_amask != AM_SPLEV_RANDOM)
        mtmp = mk_roamer(pm, Amask2align(amask), x, y, m->peaceful);
    else if (PM_ARCHEOLOGIST <= m->id && m->id <= PM_WIZARD)
        mtmp = mk_mplayer(pm, x, y, FALSE);
    else
        mtmp = makemon(pm, x, y, m->mm_flags);

    if (mtmp) {
        x = mtmp->mx, y = mtmp->my; /* sanity precaution */
        m->x = x, m->y = y;
        /* handle specific attributes for some special monsters */
        if (m->name.str)
            mtmp = christen_monst(mtmp, m->name.str);

        /*
         * This doesn't complain if an attempt is made to give a
         * non-mimic/non-shapechanger an appearance or to give a
         * shapechanger a non-monster shape, it just refuses to comply.
         */
        if (m->appear_as.str
            && ((mtmp->data->mlet == S_MIMIC)
                /* shapechanger (chameleons, et al, and vampires) */
                || (ismnum(mtmp->cham) && m->appear == M_AP_MONSTER))
            && !Protection_from_shape_changers) {
            int i;

            switch (m->appear) {
            case M_AP_NOTHING:
                impossible(
                 "create_monster: mon has an appearance, \"%s\", but no type",
                           m->appear_as.str);
                break;

            case M_AP_FURNITURE:
                for (i = 0; i < MAXPCHARS; i++)
                    if (!strcmp(defsyms[i].explanation, m->appear_as.str))
                        break;
                if (i == MAXPCHARS) {
                    impossible("create_monster: can't find feature \"%s\"",
                               m->appear_as.str);
                } else {
                    mtmp->m_ap_type = M_AP_FURNITURE;
                    mtmp->mappearance = i;
                }
                break;

            case M_AP_OBJECT:
                for (i = 0; i < NUM_OBJECTS; i++)
                    if (OBJ_NAME(objects[i])
                        && !strcmp(OBJ_NAME(objects[i]), m->appear_as.str))
                        break;
                if (i == NUM_OBJECTS) {
                    impossible("create_monster: can't find object \"%s\"",
                               m->appear_as.str);
                } else {
                    mtmp->m_ap_type = M_AP_OBJECT;
                    mtmp->mappearance = i;
                    /* try to avoid placing mimic boulder on a trap */
                    if (i == BOULDER && m->x < 0
                        && m_bad_boulder_spot(x, y)) {
                        int retrylimit = 10;

                        remove_monster(x, y);
                        do {
                            x = m->x;
                            y = m->y;
                            get_location(&x, &y, DRY, croom);
                            if (MON_AT(x, y) && enexto(&cc, x, y, pm))
                                x = cc.x, y = cc.y;
                        } while (m_bad_boulder_spot(x, y)
                                 && --retrylimit > 0);
                        place_monster(mtmp, x, y);
                        /* if we didn't find a good spot
                           then mimic something else */
                        if (!retrylimit)
                            set_mimic_sym(mtmp);
                    }
                }
                break;

            case M_AP_MONSTER: {
                int mndx, gender_name_var = NEUTRAL;

                if (!strcmpi(m->appear_as.str, "random"))
                    mndx = select_newcham_form(mtmp);
                else
                    mndx = name_to_mon(m->appear_as.str, &gender_name_var);

                if (mndx == NON_PM || (is_vampshifter(mtmp)
                                       && !validvamp(mtmp, &mndx, S_HUMAN))) {
                    impossible("create_monster: invalid %s (\"%s\")",
                               (mtmp->data->mlet == S_MIMIC)
                                 ? "mimic appearance"
                                 : (mtmp->data == &mons[PM_WIZARD_OF_YENDOR])
                                     ? "Wizard appearance"
                                     : is_vampshifter(mtmp)
                                         ? "vampire shape"
                                         : "chameleon shape",
                               m->appear_as.str);
                } else if (&mons[mndx] == mtmp->data) {
                    /* explicitly forcing a mimic to appear as itself */
                    mtmp->m_ap_type = M_AP_NOTHING;
                    mtmp->mappearance = 0;
                } else if (mtmp->data->mlet == S_MIMIC
                           || mtmp->data == &mons[PM_WIZARD_OF_YENDOR]) {
                    /* this is ordinarily only used for Wizard clones
                       and hasn't been exhaustively tested for mimics */
                    mtmp->m_ap_type = M_AP_MONSTER;
                    mtmp->mappearance = mndx;
                } else { /* chameleon or vampire */
                    struct permonst *mdat = &mons[mndx];
                    struct permonst *olddata = mtmp->data;

                    mgender_from_permonst(mtmp, mdat);
                    if (gender_name_var != NEUTRAL)
                        mtmp->female = gender_name_var;
                    set_mon_data(mtmp, mdat);
                    if (emits_light(olddata) != emits_light(mtmp->data)) {
                        /* used to give light, now doesn't, or vice versa,
                           or light's range has changed */
                        if (emits_light(olddata))
                            del_light_source(LS_MONSTER, monst_to_any(mtmp));
                        if (emits_light(mtmp->data))
                            new_light_source(mtmp->mx, mtmp->my,
                                             emits_light(mtmp->data),
                                             LS_MONSTER, monst_to_any(mtmp));
                    }
                    if (!mtmp->perminvis || pm_invisible(olddata))
                        mtmp->perminvis = pm_invisible(mdat);
                }
                break;
            }
            default:
                impossible(
                  "create_monster: unimplemented mon appear type [%d,\"%s\"]",
                           m->appear, m->appear_as.str);
                break;
            }
            if (does_block(x, y, &levl[x][y]))
                block_point(x, y);
        }

        mtmp->female = m->female;
        if (m->peaceful > BOOL_RANDOM) {
            mtmp->mpeaceful = m->peaceful;
            /* changed mpeaceful again; have to reset malign */
            set_malign(mtmp);
        }
        if (m->asleep > BOOL_RANDOM)
            mtmp->msleeping = m->asleep;
        if (m->seentraps)
            mtmp->mtrapseen = m->seentraps;
        if (m->cancelled)
            mtmp->mcan = 1;
        if (m->revived)
            mtmp->mrevived = 1;
        if (m->avenge)
            mtmp->mavenge = 1;
        if (m->stunned)
            mtmp->mstun = 1;
        if (m->confused)
            mtmp->mconf = 1;
        if (m->invis) {
            mtmp->minvis = mtmp->perminvis = 1;
        }
        if (m->blinded) {
            mtmp->mcansee = 0;
            mtmp->mblinded = (m->blinded % 127);
        }
        if (m->paralyzed) {
            mtmp->mcanmove = 0;
            mtmp->mfrozen = (m->paralyzed % 127);
        }
        if (m->fleeing) {
            mtmp->mflee = 1;
            mtmp->mfleetim = (m->fleeing % 127);
        }
        if (m->waiting) {
            mtmp->mstrategy |= STRAT_WAITFORU;
            /* if this is a vampire that got created already shifted into
               bat/fog/wolf form and the special level or theme room didn't
               explicitly request that, shift back to vampire */
            if (vampshifted(mtmp) && m->appear != M_AP_MONSTER)
                (void) newcham(mtmp, &mons[mtmp->cham], NO_NC_FLAGS);
        }
        if (!(m->has_invent & DEFAULT_INVENT)) {
            /* guard against someone accidentally specifying e.g. quest nemesis
             * with custom inventory that lacks Bell or quest artifact but
             * forgetting to flag them as receiving their default inventory */
            mdrop_special_objs(mtmp);
            discard_minvent(mtmp, TRUE);
        }
        if (m->has_invent & CUSTOM_INVENT) {
            invent_carrying_monster = mtmp;
        }
        if (m->dead) {
            mondied(mtmp);
            /* kludge for this: it didn't actually die while the player was
             * around, so revert mondead() incrementing this */
            svm.mvitals[monsndx(mtmp->data)].died--;
        }
        if (m->waiting) {
            mtmp->mstrategy |= STRAT_WAITFORU;
        }
    }
}

/*
 * Create an object in a room.
 */
staticfn struct obj *
create_object(object *o, struct mkroom *croom)
{
    struct obj *otmp;
    coordxy x, y;
    char c;
    boolean named; /* has a name been supplied in level description? */

    named = o->name.str ? TRUE : FALSE;

    get_location_coord(&x, &y, DRY, croom, o->coord);

    if (o->class >= 0)
        c = o->class;
    else
        c = 0;

    if (!c) {
        otmp = mkobj_at(RANDOM_CLASS, x, y, !named);
    } else if (o->id != -1) {
        otmp = mksobj_at(o->id, x, y, TRUE, !named);
    } else {
        /*
         * The special levels are compiled with the default "text" object
         * class characters.  We must convert them to the internal format.
         */
        char oclass = (char) def_char_to_objclass(c);

        if (oclass == MAXOCLASSES)
            panic("create_object:  unexpected object class '%c'", c);

        /* KMH -- Create piles of gold properly */
        if (oclass == COIN_CLASS)
            otmp = mkgold(0L, x, y);
        else
            otmp = mkobj_at(oclass, x, y, !named);
    }

    if (o->spe != -127) /* That means NOT RANDOM! */
        otmp->spe = (schar) o->spe;

    switch (o->curse_state) {
    case 1: /* blessed */
        bless(otmp);
        break;
    case 2: /* uncursed */
        unbless(otmp);
        uncurse(otmp);
        break;
    case 3: /* cursed */
        curse(otmp);
        break;
    case 4: /* not cursed */
        uncurse(otmp);
        break;
    case 5: /* not uncursed */
        blessorcurse(otmp, 1);
        break;
    case 6: /* not blessed */
        unbless(otmp);
        break;
    default: /* random */
        break; /* keep what mkobj gave us */
    }

    /* corpsenm is "empty" if -1, random if -2, otherwise specific */
    if (o->corpsenm != NON_PM) {
        if (o->corpsenm == NON_PM - 1)
            set_corpsenm(otmp, rndmonnum());
        else
            set_corpsenm(otmp, o->corpsenm);
    }
    /* set_corpsenm() took care of egg hatch and corpse timers */

    if (named) {
        otmp = oname(otmp, o->name.str, ONAME_LEVEL_DEF);
        if (otmp->otyp == SPE_NOVEL) {
            /* needs to be an existing title */
            (void) lookup_novel(o->name.str, &otmp->novelidx);
        }
    }
    if (o->eroded) {
        if (o->eroded < 0) {
            otmp->oerodeproof = 1;
        } else {
            otmp->oeroded = (o->eroded % 4);
            otmp->oeroded2 = ((o->eroded >> 2) % 4);
        }
    } else {
        otmp->oeroded = otmp->oeroded2 = 0;
        otmp->oerodeproof = 0;
    }
    if (o->recharged)
        otmp->recharged = (o->recharged % 8);
    if (o->locked == 0 || o->locked == 1) {
        otmp->olocked = o->locked;
    } else if (o->broken) {
        otmp->obroken = 1;
        otmp->olocked = 0; /* obj generation may set */
    }
    if (o->trapped == 0 || o->trapped == 1)
        otmp->otrapped = o->trapped;
    if (o->trapped && (o->tknown == 0 || o->tknown == 1))
        otmp->tknown = o->tknown;
    if (o->id == STATUE && o->trapped == 1) {
        otmp->otrapped = 0;
        maketrap(x, y, STATUE_TRAP);
    }
    otmp->greased = o->greased ? 1 : 0;

    if (o->material > 0)
        set_material(otmp, o->material);

    if (o->quan > 0 && objects[otmp->otyp].oc_merge) {
        otmp->quan = o->quan;
        otmp->owt = weight(otmp);
    }

    /* contents (of a container or monster's inventory) */
    if (o->containment & SP_OBJ_CONTENT || invent_carrying_monster) {
        if (!container_idx) {
            if (!invent_carrying_monster) {
                /*impossible("create_object: no container");*/
                /* don't complain, the monster may be gone legally
                   (eg. unique demon already generated)
                   TODO: In the case of unique demon lords, they should
                   get their inventories even when they get generated
                   outside the des-file.  Maybe another data file that
                   determines what inventories monsters get by default?
                 */
                ; /* ['otmp' remains on floor] */
            } else {
                remove_object(otmp);
                if (otmp->otyp == SADDLE && can_saddle(invent_carrying_monster))
                    put_saddle_on_mon(otmp, invent_carrying_monster);
                else
                    (void) mpickobj(invent_carrying_monster, otmp);
            }
        } else {
            struct obj *cobj = container_obj[container_idx - 1];

            remove_object(otmp);
            if (cobj) {
                otmp = add_to_container(cobj, otmp);
                cobj->owt = weight(cobj);
            } else {
                obj_extract_self(otmp);
                /* uncreate a random artifact created in a container */
                /* FIXME: it could be intentional rather than random */
                if (otmp->oartifact)
                    artifact_exists(otmp, safe_oname(otmp), FALSE,
                                    ONAME_NO_FLAGS); /* flags don't matter */
                obfree(otmp, NULL);
                return NULL;
            }
        }
    }
    /* container */
    if (o->containment & SP_OBJ_CONTAINER) {
        delete_contents(otmp);
        if (container_idx < MAX_CONTAINMENT) {
            container_obj[container_idx] = otmp;
            container_idx++;
        } else
            impossible("create_object: too deeply nested containers.");
    }

    /* Medusa level special case: statues are petrified monsters, so they
     * are not stone-resistant and have monster inventory.  They also lack
     * other contents, but that can be specified as an empty container.
     */
    if (o->id == STATUE && Is_medusa_level(&u.uz) && o->corpsenm == NON_PM) {
        struct monst *was = NULL;
        struct obj *obj;
        int wastyp;
        int i = 0; /* prevent endless loop in case makemon always fails */

        /* Named random statues are of player types, and aren't stone-
         * resistant (if they were, we'd have to reset the name as well as
         * setting corpsenm).
         */
        for (wastyp = otmp->corpsenm; i < 1000; i++, wastyp = rndmonnum()) {
            /* makemon without rndmonst() might create a group */
            was = makemon(&mons[wastyp], 0, 0, MM_NOCOUNTBIRTH|MM_NOMSG);
            if (was) {
                if (!resists_ston(was) && !poly_when_stoned(&mons[wastyp])) {
                    (void) propagate(wastyp, TRUE, FALSE);
                    break;
                }
                mongone(was);
                was = NULL;
            }
        }
        if (was) {
            set_corpsenm(otmp, wastyp);
            while (was->minvent) {
                obj = was->minvent;
                obj->owornmask = 0;
                obj_extract_self(obj);
                (void) add_to_container(otmp, obj);
            }
            otmp->owt = weight(otmp);
            mongone(was);
        }
    }

    if (o->achievement) {
        static const char prize_warning[] = "multiple prizes on %s level";

        if (Is_mineend_level(&u.uz)) {
            if (!svc.context.achieveo.mines_prize_oid) {
                svc.context.achieveo.mines_prize_oid = otmp->o_id;
                svc.context.achieveo.mines_prize_otyp = otmp->otyp;
                /* prevent stacking; cleared when achievement is recorded;
                   will be reset in addinv_core1() */
                otmp->nomerge = 1;
            } else {
                impossible(prize_warning, "mines end");
            }
        } else if (Is_sokoend_level(&u.uz)) {
            if (!svc.context.achieveo.soko_prize_oid) {
                svc.context.achieveo.soko_prize_oid = otmp->o_id;
                svc.context.achieveo.soko_prize_otyp = otmp->otyp;
                otmp->nomerge = 1; /* redundant; Sokoban prizes don't stack;
                                    * will be reset in addinv_core1() */
            } else {
                impossible(prize_warning, "sokoban end");
            }
        } else if (!iflags.lua_testing) {
            char lbuf[QBUFSZ];

            (void) describe_level(lbuf, 1 | 2);
            impossible("create_object: unknown achievement (%s\"%s\")",
                       lbuf, simpleonames(otmp));
        }
    }

    if (!(o->containment & SP_OBJ_CONTENT)) {
        stackobj(otmp);

        if (o->lit)
            begin_burn(otmp, FALSE);

        if (o->buried) {
            boolean dealloced;

            (void) bury_an_obj(otmp, &dealloced);
            if (dealloced) {
                if (container_idx)
                    container_obj[container_idx - 1] = NULL;
                otmp = NULL;
            }
        }
    }
    return otmp;
}

/*
 * Create an altar in a room.
 */
staticfn void
create_altar(altar *a, struct mkroom *croom)
{
    schar sproom;
    coordxy x = -1, y = -1;
    unsigned int amask;
    boolean croom_is_temple = TRUE;

    if (croom) {
        get_free_room_loc(&x, &y, croom, a->coord);
        if (croom->rtype != TEMPLE)
            croom_is_temple = FALSE;
    } else {
        get_location_coord(&x, &y, DRY, croom, a->coord);
        if ((sproom = (schar) *in_rooms(x, y, TEMPLE)) != 0)
            croom = &svr.rooms[sproom - ROOMOFFSET];
        else
            croom_is_temple = FALSE;
    }

    /* check for existing features */
    if (!set_levltyp(x, y, ALTAR))
        return;

    amask = sp_amask_to_amask(a->sp_amask);

    levl[x][y].altarmask = amask;

    if (a->shrine < 0)
        a->shrine = rn2(2); /* handle random case */

    if (!croom_is_temple || !a->shrine)
        return;

    if (a->shrine) { /* Is it a shrine  or sanctum? */
        priestini(&u.uz, croom, x, y, (a->shrine > 1));
        levl[x][y].altarmask |= AM_SHRINE;
        if (a->shrine == 2) /* high altar or sanctum */
            levl[x][y].altarmask |= AM_SANCTUM;
        svl.level.flags.has_temple = TRUE;
    }
}

/*
 * Search for a door in a room on a specified wall.
 */
staticfn boolean
search_door(
    struct mkroom *croom,
    coordxy *x, coordxy *y,
    xint16 wall, int cnt)
{
    int dx, dy;
    int xx, yy;

    switch (wall) {
    case W_SOUTH:
        dy = 0;
        dx = 1;
        xx = croom->lx;
        yy = croom->hy + 1;
        break;
    case W_NORTH:
        dy = 0;
        dx = 1;
        xx = croom->lx;
        yy = croom->ly - 1;
        break;
    case W_EAST:
        dy = 1;
        dx = 0;
        xx = croom->hx + 1;
        yy = croom->ly;
        break;
    case W_WEST:
        dy = 1;
        dx = 0;
        xx = croom->lx - 1;
        yy = croom->ly;
        break;
    default:
        panic("search_door: Bad wall!");
        /*NOTREACHED*/
    }
    while (xx <= croom->hx + 1 && yy <= croom->hy + 1) {
        if (IS_DOOR(levl[xx][yy].typ) || levl[xx][yy].typ == SDOOR) {
            *x = xx;
            *y = yy;
            if (cnt-- <= 0)
                return TRUE;
        }
        xx += dx;
        yy += dy;
    }
    return FALSE;
}

/*
 * Dig a corridor between two points, using terrain ftyp.
 * if nxcor is TRUE, he corridor may be blocked by a boulder,
 * or just end without reaching the destination.
 */
boolean
dig_corridor(
    coord *org, coord *dest,
    boolean nxcor,
    schar ftyp, schar btyp)
{
    int dx = 0, dy = 0, dix, diy, cct;
    struct rm *crm;
    int tx, ty, xx, yy;

    xx = org->x;
    yy = org->y;
    tx = dest->x;
    ty = dest->y;
    if (xx <= 0 || yy <= 0 || tx <= 0 || ty <= 0 || xx > COLNO - 1
        || tx > COLNO - 1 || yy > ROWNO - 1 || ty > ROWNO - 1) {
        debugpline4("dig_corridor: bad coords <%d,%d> <%d,%d>.",
                    xx, yy, tx, ty);
        return FALSE;
    }
    if (tx > xx)
        dx = 1;
    else if (ty > yy)
        dy = 1;
    else if (tx < xx)
        dx = -1;
    else
        dy = -1;

    xx -= dx;
    yy -= dy;
    cct = 0;
    while (xx != tx || yy != ty) {
        /* loop: dig corridor at [xx,yy] and find new [xx,yy] */
        if (cct++ > 500 || (nxcor && !rn2(35)))
            return FALSE;

        xx += dx;
        yy += dy;

        if (xx >= COLNO - 1 || xx <= 0 || yy <= 0 || yy >= ROWNO - 1)
            return FALSE; /* impossible */

        crm = &levl[xx][yy];
        if (crm->typ == btyp) {
            crm->typ = ftyp;
            if (nxcor && !rn2(50))
                (void) mksobj_at(BOULDER, xx, yy, TRUE, FALSE);
        } else if (crm->typ != ftyp && crm->typ != SCORR) {
            /* strange ... */
            return FALSE;
        }

        /* find next corridor position */
        dix = abs(xx - tx);
        diy = abs(yy - ty);

        if ((dix > diy) && diy && !rn2(dix - diy + 1)) {
            dix = 0;
        } else if ((diy > dix) && dix && !rn2(diy - dix + 1)) {
            diy = 0;
        }

        /* do we have to change direction ? */
        if (dy && dix > diy) {
            int ddx = (xx > tx) ? -1 : 1;

            crm = &levl[xx + ddx][yy];
            if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
                dx = ddx;
                dy = 0;
                continue;
            }
        } else if (dx && diy > dix) {
            int ddy = (yy > ty) ? -1 : 1;

            crm = &levl[xx][yy + ddy];
            if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
                dy = ddy;
                dx = 0;
                continue;
            }
        }

        /* continue straight on? */
        crm = &levl[xx + dx][yy + dy];
        if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
            continue;

        /* no, what must we do now?? */
        if (dx) {
            dx = 0;
            dy = (ty < yy) ? -1 : 1;
        } else {
            dy = 0;
            dx = (tx < xx) ? -1 : 1;
        }
        crm = &levl[xx + dx][yy + dy];
        if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
            continue;
        dy = -dy;
        dx = -dx;
    }
    return TRUE;
}

/*
 * Corridors always start from a door. But it can end anywhere...
 * Basically we search for door coordinates or for endpoints coordinates
 * (from a distance).
 */
staticfn void
create_corridor(corridor *c)
{
    coord org, dest;

    if (c->src.room == -1) {
        makecorridors(); /*makecorridors(c->src.door);*/
        return;
    }

    /* Safety railings - if there's ever a case where des.corridor() needs
     * to be called with src/destwall="random", that logic first needs to be
     * implemented in search_door. */
    if (c->src.wall == W_ANY || c->src.wall == W_RANDOM
        || c->dest.wall == W_ANY || c->dest.wall == W_RANDOM) {
        impossible("create_corridor to/from a random wall");
        return;
    }
    if (!search_door(&svr.rooms[c->src.room], &org.x, &org.y, c->src.wall,
                     c->src.door))
        return;
    if (c->dest.room != -1) {
        if (!search_door(&svr.rooms[c->dest.room],
                         &dest.x, &dest.y, c->dest.wall, c->dest.door))
            return;
        switch (c->src.wall) {
        case W_NORTH:
            org.y--;
            break;
        case W_SOUTH:
            org.y++;
            break;
        case W_WEST:
            org.x--;
            break;
        case W_EAST:
            org.x++;
            break;
        }
        switch (c->dest.wall) {
        case W_NORTH:
            dest.y--;
            break;
        case W_SOUTH:
            dest.y++;
            break;
        case W_WEST:
            dest.x--;
            break;
        case W_EAST:
            dest.x++;
            break;
        }
        (void) dig_corridor(&org, &dest, FALSE, CORR, STONE);
    }
}

/*
 * Fill a room (shop, zoo, etc...) with appropriate stuff.
 */
void
fill_special_room(struct mkroom *croom)
{
    int i;

    if (!croom)
        return;

    /* First recurse into subrooms. We don't want to block an ordinary room
     * with a special subroom from having the subroom filled, or an unfilled
     * outer room preventing a special subroom from being filled. */
    for (i = 0; i < croom->nsubrooms; ++i) {
        fill_special_room(croom->sbrooms[i]);
    }

    if (croom->rtype == OROOM || croom->rtype == THEMEROOM
        || croom->needfill == FILL_NONE)
        return;

    if (croom->needfill == FILL_NORMAL) {
        int x, y;

        /* Shop ? */
        if (croom->rtype >= SHOPBASE) {
            stock_room(croom->rtype - SHOPBASE, croom);
            svl.level.flags.has_shop = TRUE;
            return;
        }

        switch (croom->rtype) {
        case VAULT:
            for (x = croom->lx; x <= croom->hx; x++)
                for (y = croom->ly; y <= croom->hy; y++)
                    (void) mkgold((long) rn1(abs(depth(&u.uz)) * 100, 51),
                                  x, y);
            break;
        case COURT:
        case ZOO:
        case BEEHIVE:
        case ANTHOLE:
        case COCKNEST:
        case LEPREHALL:
        case MORGUE:
        case BARRACKS:
        case DEMONDEN:
        case LAVAROOM:
        case ABATTOIR:
            fill_zoo(croom);
            break;
        }
    }
    switch (croom->rtype) {
    case VAULT:
        svl.level.flags.has_vault = TRUE;
        break;
    case ZOO:
        svl.level.flags.has_zoo = TRUE;
        break;
    case COURT:
        svl.level.flags.has_court = TRUE;
        break;
    case MORGUE:
        svl.level.flags.has_morgue = TRUE;
        break;
    case BEEHIVE:
        svl.level.flags.has_beehive = TRUE;
        break;
    case BARRACKS:
        svl.level.flags.has_barracks = TRUE;
        break;
    case TEMPLE:
        svl.level.flags.has_temple = TRUE;
        break;
    case SWAMP:
        svl.level.flags.has_swamp = TRUE;
        break;
    }
}

staticfn struct mkroom *
build_room(room *r, struct mkroom *mkr)
{
    boolean okroom;
    struct mkroom *aroom;
    xint16 rtype = (!r->chance || rn2(100) < r->chance) ? r->rtype : OROOM;

    if (mkr) {
        aroom = &gs.subrooms[gn.nsubroom];
        okroom = create_subroom(mkr, r->x, r->y, r->w, r->h, rtype, r->rlit);
    } else {
        aroom = &svr.rooms[svn.nroom];
        okroom = create_room(r->x, r->y, r->w, r->h, r->xalign, r->yalign,
                             rtype, r->rlit);
    }

    if (okroom) {
#ifdef SPECIALIZATION
        topologize(aroom, FALSE); /* set roomno */
#else
        topologize(aroom); /* set roomno */
#endif
        aroom->needfill = r->needfill;
        aroom->needjoining = r->joined;
        return aroom;
    }
    return (struct mkroom *) 0;
}

/*
 * set lighting in a region that will not become a room.
 */
staticfn void
light_region(region *tmpregion)
{
    boolean litstate = tmpregion->rlit ? 1 : 0;
    int hiy = tmpregion->y2;
    int x, y;
    struct rm *lev;
    int lowy = tmpregion->y1;
    int lowx = tmpregion->x1, hix = tmpregion->x2;

    if (litstate) {
        /* adjust region size for walls, but only if lighted */
        lowx = max(lowx - 1, 1);
        hix = min(hix + 1, COLNO - 1);
        lowy = max(lowy - 1, 0);
        hiy = min(hiy + 1, ROWNO - 1);
    }
    for (x = lowx; x <= hix; x++) {
        lev = &levl[x][lowy];
        for (y = lowy; y <= hiy; y++) {
            lev->lit = IS_LAVA(lev->typ) ? 1 : litstate;
            lev++;
        }
    }
}

void
wallify_map(coordxy x1, coordxy y1, coordxy x2, coordxy y2)
{
    coordxy x, y, xx, yy, lo_xx, lo_yy, hi_xx, hi_yy;

    y1 = max(y1, 0);
    x1 = max(x1, 1);
    y2 = min(y2, ROWNO - 1);
    x2 = min(x2, COLNO - 1);
    for (y = y1; y <= y2; y++) {
        lo_yy = (y > 0) ? y - 1 : 0;
        hi_yy = (y < y2) ? y + 1 : y2;
        for (x = x1; x <= x2; x++) {
            if (!isok(x, y))
                /* not on the map anyway, can't be wallified */
                continue;
            if (levl[x][y].typ != STONE)
                continue;
            lo_xx = (x > 1) ? x - 1 : 1;
            hi_xx = (x < x2) ? x + 1 : x2;
            for (yy = lo_yy; yy <= hi_yy; yy++)
                for (xx = lo_xx; xx <= hi_xx; xx++)
                    if (IS_ROOM(levl[xx][yy].typ)
                        || levl[xx][yy].typ == CROSSWALL) {
                        levl[x][y].typ = (yy != y) ? HWALL : VWALL;
                        yy = hi_yy; /* end `yy' loop */
                        break;      /* end `xx' loop */
                    }
        }
    }
}

/*
 * Select a random coordinate in the maze.
 *
 * We want a place not 'touched' by the loader.  That is, a place in
 * the maze outside every part of the special level.
 */
staticfn void
maze1xy(coord *m, int humidity)
{
    int x, y, tryct = 2000;
    /* tryct:  normally it won't take more than ten or so tries due
       to the circumstances under which we'll be called, but the
       `humidity' screening might drastically change the chances */

    do {
        x = rn1(gx.x_maze_max - 3, 3);
        y = rn1(gy.y_maze_max - 3, 3);
        if (--tryct < 0)
            break; /* give up */
    } while (!(x % 2) || !(y % 2) || SpLev_Map[x][y]
             || !is_ok_location((coordxy) x, (coordxy) y, humidity));

    m->x = (coordxy) x, m->y = (coordxy) y;
}

/*
 * If there's a significant portion of maze unused by the special level,
 * we don't want it empty.
 *
 * Makes the number of traps, monsters, etc. proportional
 * to the size of the maze.
 */
staticfn void
fill_empty_maze(void)
{
    int mapcountmax, mapcount, mapfact;
    coordxy x, y;
    coord mm;

    mapcountmax = mapcount = (gx.x_maze_max - 2) * (gy.y_maze_max - 2);
    mapcountmax = mapcountmax / 2;

    for (x = 2; x < gx.x_maze_max; x++)
        for (y = 0; y < gy.y_maze_max; y++)
            if (SpLev_Map[x][y])
                mapcount--;

    if ((mapcount > (int) (mapcountmax / 10))) {
        mapfact = (int) ((mapcount * 100L) / mapcountmax);
        for (x = rnd((int) (20 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) mkobj_at(rn2(2) ? GEM_CLASS : RANDOM_CLASS, mm.x, mm.y,
                            TRUE);
        }
        for (x = rnd((int) (12 * mapfact) / 100); x; x--) {
            struct trap *ttmp;

            maze1xy(&mm, DRY);
            if ((ttmp = t_at(mm.x, mm.y)) != 0
                && (is_pit(ttmp->ttyp) || is_hole(ttmp->ttyp)))
                continue;
            (void) mksobj_at(BOULDER, mm.x, mm.y, TRUE, FALSE);
        }
        for (x = rn2(2); x; x--) {
            maze1xy(&mm, DRY);
            (void) makemon(&mons[PM_MINOTAUR], mm.x, mm.y, NO_MM_FLAGS);
        }
        for (x = rnd((int) (12 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) makemon((struct permonst *) 0, mm.x, mm.y, NO_MM_FLAGS);
        }
        for (x = rn2((int) (15 * mapfact) / 100); x; x--) {
            maze1xy(&mm, DRY);
            (void) mkgold(0L, mm.x, mm.y);
        }
        for (x = rn2((int) (15 * mapfact) / 100); x; x--) {
            int trytrap;

            maze1xy(&mm, DRY);
            trytrap = rndtrap();
            if (sobj_at(BOULDER, mm.x, mm.y))
                while (is_pit(trytrap) || is_hole(trytrap))
                    trytrap = rndtrap();
            (void) maketrap(mm.x, mm.y, trytrap);
            if (trytrap == STATUE_TRAP)
                mk_trap_statue(mm.x, mm.y);
        }
    }
}

staticfn void
splev_initlev(lev_init *linit)
{
    switch (linit->init_style) {
    default:
        impossible("Unrecognized level init style.");
        break;
    case LVLINIT_NONE:
        break;
    case LVLINIT_SOLIDFILL:
        if (linit->lit == BOOL_RANDOM)
            linit->lit = rn2(2);
        lvlfill_solid(linit->filling, linit->lit);
        break;
    case LVLINIT_MAZEGRID:
        lvlfill_maze_grid(2, 0, gx.x_maze_max, gy.y_maze_max, linit->bg);
        break;
    case LVLINIT_MAZE:
        create_maze(linit->corrwid, linit->wallthick, linit->rm_deadends);
        break;
    case LVLINIT_MINES:
        if (linit->lit == BOOL_RANDOM) {
            linit->lit = rn2(2);
            if (In_mines(&u.uz)) {
                /* Always bright above Minetown, always dark below */
                s_level *minetownslev = find_level("minetn");
                int diff = depth(&u.uz) - depth(&minetownslev->dlevel);
                if (diff > 0)
                    linit->lit = 0;
                if (diff < 0)\
                    linit->lit = 1;
                /* if diff == 0, we're in minetown, so let it stay random */
            }
        }
        if (linit->filling > -1)
            lvlfill_solid(linit->filling, 0);
        linit->icedpools = icedpools;
        mkmap(linit);
        break;
    case LVLINIT_SWAMP:
        if (linit->lit == BOOL_RANDOM)
            linit->lit = rn2(2);
        lvlfill_swamp(linit->fg, linit->bg, linit->lit);
        break;
    }
}

#if 0
staticfn long
sp_code_jmpaddr(long curpos, long jmpaddr)
{
    return (curpos + jmpaddr);
}
#endif


/*ARGUSED*/
staticfn void
spo_end_moninvent(void)
{
    if (invent_carrying_monster) {
        m_dowear(invent_carrying_monster, TRUE);
        /* replicate the behavior in makemon() for a monster that was created
         * with no weapon, but was then later given one in inventory */
        if (is_armed(invent_carrying_monster->data)
            && !MON_WEP(invent_carrying_monster)
            && !helpless(invent_carrying_monster)) {
            invent_carrying_monster->weapon_check = NEED_WEAPON;
            mon_wield_item(invent_carrying_monster);
        }
    }
    invent_carrying_monster = NULL;
}

/*ARGUSED*/
staticfn void
spo_pop_container(void)
{
    if (container_idx > 0) {
        container_idx--;
        container_obj[container_idx] = NULL;
    }
}

/* push a table on lua stack: {width=wid, height=hei} */
staticfn void
l_push_wid_hei_table(lua_State *L, int wid, int hei)
{
    lua_newtable(L);
    nhl_add_table_entry_int(L, "width", wid);
    nhl_add_table_entry_int(L, "height", hei);
}

/* push a table on lua stack containing room data */
staticfn void
l_push_mkroom_table(lua_State *L, struct mkroom *tmpr)
{
    lua_newtable(L);
    nhl_add_table_entry_int(L, "width", 1 + (tmpr->hx - tmpr->lx));
    nhl_add_table_entry_int(L, "height", 1 + (tmpr->hy - tmpr->ly));
    nhl_add_table_entry_region(L, "region", tmpr->lx, tmpr->ly,
                               tmpr->hx, tmpr->hy);
    nhl_add_table_entry_bool(L, "lit", (boolean) tmpr->rlit);
    nhl_add_table_entry_bool(L, "irregular", tmpr->irregular);
    nhl_add_table_entry_bool(L, "needjoining", tmpr->needjoining);
    nhl_add_table_entry_str(L, "type", get_mkroom_name(tmpr->rtype));
}

DISABLE_WARNING_UNREACHABLE_CODE

/* message("What a strange feeling!"); */
int
lspo_message(lua_State *L)
{
    char *levmsg;
    int old_n, n;
    const char *msg;

    int argc = lua_gettop(L);

    if (argc < 1) {
        nhl_error(L, "Wrong parameters");
        /*NOTREACHED*/
        return 0;
    }

    create_des_coder();

    msg = luaL_checkstring(L, 1);

    old_n = gl.lev_message ? (Strlen(gl.lev_message) + 1) : 0;
    n = Strlen(msg);

    levmsg = (char *) alloc(old_n + n + 1);
    if (old_n)
        levmsg[old_n - 1] = '\n';
    if (gl.lev_message)
        (void) memcpy((genericptr_t) levmsg, (genericptr_t) gl.lev_message,
                      old_n - 1);
    (void) memcpy((genericptr_t) &levmsg[old_n], msg, n);
    levmsg[old_n + n] = '\0';
    Free(gl.lev_message);
    gl.lev_message = levmsg;

    return 0;  /* number of results */
}

RESTORE_WARNING_UNREACHABLE_CODE

staticfn int
get_table_align(lua_State *L)
{
    static const char *const gtaligns[] = {
        "noalign", "law", "neutral", "chaos",
        "coaligned", "noncoaligned", "random", NULL
    };
    static const int aligns2i[] = {
        AM_NONE, AM_LAWFUL, AM_NEUTRAL, AM_CHAOTIC,
        AM_SPLEV_CO, AM_SPLEV_NONCO, AM_SPLEV_RANDOM, 0
    };

    int a = aligns2i[get_table_option(L, "align", "random", gtaligns)];

    return a;
}

staticfn int
get_table_monclass(lua_State *L)
{
    char *s = get_table_str_opt(L, "class", NULL);
    int ret = -1;

    if (s && strlen(s) == 1)
        ret = (int) *s;
    Free(s);
    return ret;
}

int
find_montype(
    lua_State *L UNUSED,
    const char *s,
    int *mgender)
{
    int i, mgend = NEUTRAL;

    i = name_to_monplus(s, (const char **) 0, &mgend);
    if (i >= LOW_PM && i < NUMMONS) {
        if (is_male(&mons[i]) || is_female(&mons[i]))
            mgend = is_female(&mons[i]) ? FEMALE : MALE;
        else
            mgend = (mgend == FEMALE) ? FEMALE
                        : (mgend == MALE) ? MALE : rn2(2);
        if (mgender)
            *mgender = mgend;
        return i;
    }
    if (mgender)
        *mgender = NEUTRAL;
    return NON_PM;
}

staticfn int
get_table_montype(lua_State *L, int *mgender)
{
    char *s = get_table_str_opt(L, "id", NULL);
    int ret = NON_PM;

    if (s) {
        ret = find_montype(L, s, mgender);
        Free(s);
        if (ret == NON_PM)
            nhl_error(L, "Unknown monster id");
    }
    return ret;
}

/* Get x and y values from a table (which the caller has already checked for
 * the existence of), handling both a table with x= and y= specified and a
 * table with coord= specified.
 * Returns absolute rather than map-relative coordinates; the caller of this
 * function must decide if it wants to interpret the coordinates as
 * map-relative and adjust accordingly. */
staticfn void
get_table_xy_or_coord(
    lua_State *L,
    lua_Integer *x,
    lua_Integer *y)
{
    lua_Integer mx = get_table_int_opt(L, "x", -1);
    lua_Integer my = get_table_int_opt(L, "y", -1);

    if (mx == -1 && my == -1) {
        lua_getfield(L, 1, "coord");
        (void) get_coord(L, -1, &mx, &my);
        lua_pop(L, 1);
    }

    *x = mx;
    *y = my;
}

/* monster(); */
/* monster("wood nymph"); */
/* monster("D"); */
/* monster("giant eel",11,06); */
/* monster("hill giant", {08,06}); */
/* monster({ id = "giant mimic", appear_as = "obj:boulder" }); */
/* monster({ class = "H", peaceful = 0 }); */
int
lspo_monster(lua_State *L)
{
    int argc = lua_gettop(L);
    monster tmpmons;
    lua_Integer mx = -1, my = -1;
    int mgend = NEUTRAL;
    char *mappear = NULL;

    create_des_coder();

    tmpmons.peaceful = -1;
    tmpmons.asleep = -1;
    tmpmons.name.str = NULL;
    tmpmons.appear = 0;
    tmpmons.appear_as.str = (char *) 0;
    tmpmons.sp_amask = AM_SPLEV_RANDOM;
    tmpmons.female = 0;
    tmpmons.invis = 0;
    tmpmons.cancelled = 0;
    tmpmons.revived = 0;
    tmpmons.avenge = 0;
    tmpmons.fleeing = 0;
    tmpmons.blinded = 0;
    tmpmons.paralyzed = 0;
    tmpmons.stunned = 0;
    tmpmons.confused = 0;
    tmpmons.seentraps = 0;
    tmpmons.has_invent = DEFAULT_INVENT;
    tmpmons.dead = 0;
    tmpmons.waiting = 0;
    tmpmons.mm_flags = NO_MM_FLAGS;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        const char *paramstr = luaL_checkstring(L, 1);

        if (strlen(paramstr) == 1) {
            tmpmons.class = *paramstr;
            tmpmons.id = NON_PM;
        } else {
            tmpmons.class = -1;
            tmpmons.id = find_montype(L, paramstr, &mgend);
            tmpmons.female = (mgend == FEMALE) ? FEMALE
                                : (mgend == MALE) ? MALE : rn2(2);
        }
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
               && lua_type(L, 2) == LUA_TTABLE) {
        const char *paramstr = luaL_checkstring(L, 1);

        (void) get_coord(L, 2, &mx, &my);

        if (strlen(paramstr) == 1) {
            tmpmons.class = *paramstr;
            tmpmons.id = NON_PM;
        } else {
            tmpmons.class = -1;
            tmpmons.id = find_montype(L, paramstr, &mgend);
            tmpmons.female = (mgend == FEMALE) ? FEMALE
                                : (mgend == MALE) ? MALE : rn2(2);
        }

    } else if (argc == 3) {
        const char *paramstr = luaL_checkstring(L, 1);

        mx = luaL_checkinteger(L, 2);
        my = luaL_checkinteger(L, 3);

        if (strlen(paramstr) == 1) {
            tmpmons.class = *paramstr;
            tmpmons.id = NON_PM;
        } else {
            tmpmons.class = -1;
            tmpmons.id = find_montype(L, paramstr, &mgend);
            tmpmons.female = (mgend == FEMALE) ? FEMALE
                                : (mgend == MALE) ? MALE : rn2(2);
        }
    } else {
        int keep_default_invent = -1; /* -1 = unspecified */
        lcheck_param_table(L);

        tmpmons.peaceful = get_table_boolean_opt(L, "peaceful", BOOL_RANDOM);
        tmpmons.asleep = get_table_boolean_opt(L, "asleep", BOOL_RANDOM);
        tmpmons.name.str = get_table_str_opt(L, "name", NULL);
        tmpmons.appear = 0;
        tmpmons.appear_as.str = (char *) 0;
        tmpmons.sp_amask = get_table_align(L);
        tmpmons.female = get_table_boolean_opt(L, "female", BOOL_RANDOM);
        tmpmons.invis = get_table_boolean_opt(L, "invisible", FALSE);
        tmpmons.cancelled = get_table_boolean_opt(L, "cancelled", FALSE);
        tmpmons.revived = get_table_boolean_opt(L, "revived", FALSE);
        tmpmons.avenge = get_table_boolean_opt(L, "avenge", FALSE);
        tmpmons.fleeing = get_table_int_opt(L, "fleeing", 0);
        tmpmons.blinded = get_table_int_opt(L, "blinded", 0);
        tmpmons.paralyzed = get_table_int_opt(L, "paralyzed", 0);
        tmpmons.stunned = get_table_boolean_opt(L, "stunned", FALSE);
        tmpmons.confused = get_table_boolean_opt(L, "confused", FALSE);
        tmpmons.waiting = get_table_boolean_opt(L, "waiting", FALSE);
        tmpmons.dead = get_table_boolean_opt(L, "dead", 0);
        tmpmons.seentraps = 0; /* TODO: list of trap names to bitfield */
        keep_default_invent =
            get_table_boolean_opt(L, "keep_default_invent", -1);

        if (!get_table_boolean_opt(L, "tail", TRUE))
            tmpmons.mm_flags |= MM_NOTAIL;
        if (!get_table_boolean_opt(L, "group", TRUE))
            tmpmons.mm_flags |= MM_NOGRP;
        if (get_table_boolean_opt(L, "adjacentok", FALSE))
            tmpmons.mm_flags |= MM_ADJACENTOK;
        if (get_table_boolean_opt(L, "ignorewater", FALSE))
            tmpmons.mm_flags |= MM_IGNOREWATER;
        if (!get_table_boolean_opt(L, "countbirth", TRUE))
            tmpmons.mm_flags |= MM_NOCOUNTBIRTH;

        mappear = get_table_str_opt(L, "appear_as", NULL);
        if (mappear) {
            if (!strncmp("obj:", mappear, 4)) {
                tmpmons.appear = M_AP_OBJECT;
            } else if (!strncmp("mon:", mappear, 4)) {
                tmpmons.appear = M_AP_MONSTER;
            } else if (!strncmp("ter:", mappear, 4)) {
                tmpmons.appear = M_AP_FURNITURE;
            } else {
                nhl_error(L, "Unknown appear_as type");
            }
            tmpmons.appear_as.str = dupstr(&mappear[4]);
            Free(mappear);
        }

        get_table_xy_or_coord(L, &mx, &my);

        tmpmons.id = get_table_montype(L, &mgend);
        /* get_table_montype will return a random gender if the species isn't
         * all-male or all-female; if the level designer specified a certain
         * gender, override that random one now, unless it *is* a one-gender
         * species, in which case don't override (don't permit creation of a
         * male nymph or female Nazgul, etc.) */
        if (mgend != NEUTRAL
            && (tmpmons.female == BOOL_RANDOM || is_female(&mons[tmpmons.id])
                || is_male(&mons[tmpmons.id])))
            tmpmons.female = mgend;
        /* safety net - if find_montype did not find a gender for this species
         * (should cause a lua error anyway) */
        if (tmpmons.female == BOOL_RANDOM)
            tmpmons.female = 0;

        tmpmons.class = get_table_monclass(L);

        lua_getfield(L, 1, "inventory");
        if (!lua_isnil(L, -1)) {
            /* overwrite DEFAULT_INVENT - most times inventory is specified,
             * the monster should not get its species' default inventory. Only
             * provide it if explicitly requested. */
            tmpmons.has_invent = CUSTOM_INVENT;
            if (keep_default_invent == TRUE)
                tmpmons.has_invent |= DEFAULT_INVENT;
        }
        else {
            /* if keep_default_invent was not specified (-1), keep has_invent as
             * DEFAULT_INVENT and provide the species' default inventory.
             * But if it was explicitly set to false, provide *no* inventory. */
            if (keep_default_invent == FALSE)
                tmpmons.has_invent = NO_INVENT;
        }
    }

    if (mx == -1 && my == -1)
        tmpmons.coord = SP_COORD_PACK_RANDOM(0);
    else
        tmpmons.coord = SP_COORD_PACK(mx, my);

    if (tmpmons.id != NON_PM && tmpmons.class == -1)
        tmpmons.class = monsym(&mons[tmpmons.id]);

    create_monster(&tmpmons, gc.coder->croom);

    if ((tmpmons.has_invent & CUSTOM_INVENT)
        && lua_type(L, -1) == LUA_TFUNCTION) {
        lua_remove(L, -2);
        nhl_pcall_handle(L, 0, 0, "lspo_monster", NHLpa_panic);
        spo_end_moninvent();
    } else
        lua_pop(L, 1);

    Free(tmpmons.name.str);
    Free(tmpmons.appear_as.str);

    return 0;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* the hash key 'name' is an integer or "random",
   or if not existent, also return rndval */
staticfn lua_Integer
get_table_int_or_random(lua_State *L, const char *name, int rndval)
{
    lua_Integer ret;
    char buf[BUFSZ];

    lua_getfield(L, 1, name);
    if (lua_type(L, -1) == LUA_TNIL) {
        lua_pop(L, 1);
        return rndval;
    }
    if (!lua_isnumber(L, -1)) {
        const char *tmp = lua_tostring(L, -1);

        if (tmp && !strcmpi("random", tmp)) {
            lua_pop(L, 1);
            return rndval;
        }
        Sprintf(buf, "Expected integer or \"random\" for \"%s\", got ", name);
        if (tmp)
            Sprintf(eos(buf), "\"%s\"", tmp);
        else
            Strcat(buf, "<Null>");
        nhl_error(L, buf);
        /*NOTREACHED*/
        lua_pop(L, 1);
        return 0;
    }
    ret = luaL_optinteger(L, -1, rndval);
    lua_pop(L, 1);
    return ret;
}

RESTORE_WARNING_UNREACHABLE_CODE

staticfn int
get_table_buc(lua_State *L)
{
    static const char *const bucs[] = {
        "random", "blessed", "uncursed", "cursed",
        "not-cursed", "not-uncursed", "not-blessed", NULL,
    };
    static const int bucs2i[] = { 0, 1, 2, 3, 4, 5, 6, 0 };
    int curse_state = bucs2i[get_table_option(L, "buc", "random", bucs)];

    return curse_state;
}

staticfn int
get_table_objclass(lua_State *L)
{
    char *s = get_table_str_opt(L, "class", NULL);
    int ret = -1;

    if (s && strlen(s) == 1)
        ret = (int) *s;
    Free(s);
    return ret;
}

staticfn int
find_objtype(lua_State *L, const char *s)
{
    if (s && *s) {
        int i;
        const char *objname;
        char class = 0;

        /* In objects.h, some item classes are defined without prefixes
           (such as "scroll of ") in their names, making some names (such
           as "teleportation") ambiguous.  Get the object class if it is
           specified, and only return an object of the matching class. */
        static struct objclasspfx {
            const char *prefix;
            char class;
        } class_prefixes[] = {
            { "ring of ", RING_CLASS },
            { "potion of ", POTION_CLASS },
            { "scroll of ", SCROLL_CLASS },
            { "spellbook of ", SPBOOK_CLASS },
            { "wand of ", WAND_CLASS },
            { NULL, 0 }
        };

        if (strstri(s, " of ")) {
            for (i = 0; class_prefixes[i].prefix; i++) {
                const char *p = class_prefixes[i].prefix;

                if (!strncmpi(s, p, strlen(p))) {
                    class = class_prefixes[i].class;
                    s = s + strlen(p);
                    break;
                }
            }
        }

        /* find by object name */
        for (i = 0; i < NUM_OBJECTS; i++) {
            objname = OBJ_NAME(objects[i]);
            if ((!class || class == objects[i].oc_class)
                && objname && !strcmpi(s, objname))
                return i;
        }

        /*
         * FIXME:
         *  If the file specifies "orange potion", the actual object
         *  description is just "orange" and won't match.  [There's a
         *  reason that wish handling is insanely complicated.]  And
         *  even if that gets fixed, if the file specifies "gray stone"
         *  it will start matching but would always pick the first one.
         *
         *  "orange potion" is an unlikely thing to have in a special
         *  level description but "gray stone" is not....
         */

        /* find by object description */
        for (i = 0; i < NUM_OBJECTS; i++) {
            objname = OBJ_DESCR(objects[i]);
            if (objname && !strcmpi(s, objname))
                return i;
        }

        nhl_error(L, "Unknown object id");
    }
    return STRANGE_OBJECT;
}

staticfn int
get_table_objtype(lua_State *L)
{
    char *s = get_table_str_opt(L, "id", NULL);
    int ret = find_objtype(L, s);

    Free(s);
    return ret;
}

/* object(); */
/* object("sack"); */
/* object("scimitar", 6,7); */
/* object("scimitar", {6,7}); */
/* object({ class = "%" }); */
/* object({ id = "boulder", x = 03, y = 12}); */
/* object({ id = "boulder", coord = {03,12} }); */
int
lspo_object(lua_State *L)
{
    static object zeroobject = {
            { 0 },   /* Str_or_len name */
            0,       /* corpsenm */
            0, 0,    /* id, spe */
            0,       /* coord */
            0, 0,    /* coordxy x,y */
            0, 0,    /* class, containment */
            0,       /* curse_state */
            0,       /* quan */
            0,       /* buried */
            0,       /* lit */
            0, 0, 0, 0, 0, /* eroded, locked, trapped, tknown, recharged */
            0, 0, 0, 0, /* invis, greased, broken, achievement */
            0,       /* material */
    };
#if 0
    int nparams = 0;
#endif
    long quancnt;
    object tmpobj;
    lua_Integer ox = -1, oy = -1;
    int argc = lua_gettop(L);
    int maybe_contents = 0;
    struct obj *otmp = NULL;

    create_des_coder();

    tmpobj = zeroobject;
    tmpobj.name.str = (char *) 0;
    tmpobj.spe = -127;
    tmpobj.quan = -1;
    tmpobj.trapped = -1;
    tmpobj.tknown = -1;
    tmpobj.locked = -1;
    tmpobj.corpsenm = NON_PM;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        const char *paramstr = luaL_checkstring(L, 1);

        if (strlen(paramstr) == 1) {
            tmpobj.class = *paramstr;
            tmpobj.id = STRANGE_OBJECT;
        } else {
            tmpobj.class = -1;
            tmpobj.id = find_objtype(L, paramstr);
        }
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
               && lua_type(L, 2) == LUA_TTABLE) {
        const char *paramstr = luaL_checkstring(L, 1);

        (void) get_coord(L, 2, &ox, &oy);

        if (strlen(paramstr) == 1) {
            tmpobj.class = *paramstr;
            tmpobj.id = STRANGE_OBJECT;
        } else {
            tmpobj.class = -1;
            tmpobj.id = find_objtype(L, paramstr);
        }
    } else if (argc == 3 && lua_type(L, 2) == LUA_TNUMBER
               && lua_type(L, 3) == LUA_TNUMBER) {
        const char *paramstr = luaL_checkstring(L, 1);

        ox = luaL_checkinteger(L, 2);
        oy = luaL_checkinteger(L, 3);

        if (strlen(paramstr) == 1) {
            tmpobj.class = *paramstr;
            tmpobj.id = STRANGE_OBJECT;
        } else {
            tmpobj.class = -1;
            tmpobj.id = find_objtype(L, paramstr);
        }
    } else {
        lcheck_param_table(L);

        tmpobj.spe = get_table_int_or_random(L, "spe", -127);
        tmpobj.curse_state = get_table_buc(L);
        tmpobj.corpsenm = NON_PM;
        tmpobj.name.str = get_table_str_opt(L, "name", (char *) 0);
        tmpobj.quan = get_table_int_or_random(L, "quantity", -1);
        tmpobj.buried = get_table_boolean_opt(L, "buried", 0);
        tmpobj.lit = get_table_boolean_opt(L, "lit", 0);
        tmpobj.eroded = get_table_int_opt(L, "eroded", 0);
        tmpobj.locked = get_table_boolean_opt(L, "locked", -1);
        tmpobj.trapped = get_table_boolean_opt(L, "trapped", -1);
        tmpobj.tknown = get_table_boolean_opt(L, "trap_known", -1);
        tmpobj.recharged = get_table_int_opt(L, "recharged", 0);
        tmpobj.greased = get_table_boolean_opt(L, "greased", 0);
        tmpobj.broken = get_table_boolean_opt(L, "broken", 0);
        tmpobj.achievement = get_table_boolean_opt(L, "achievement", 0);
        /* There is currently no way to say "use the default material"; leaving
         * material blank may give it various materials using the normal
         * formula, if the object is eligible. */
        tmpobj.material = get_table_option(L, "material", "mysterious", materialnm);

        get_table_xy_or_coord(L, &ox, &oy);

        tmpobj.id = get_table_objtype(L);
        tmpobj.class = get_table_objclass(L);
        maybe_contents = 1;
    }

    if (ox == -1 && oy == -1)
        tmpobj.coord = SP_COORD_PACK_RANDOM(0);
    else
        tmpobj.coord = SP_COORD_PACK(ox, oy);

    if (tmpobj.class == -1 && tmpobj.id > STRANGE_OBJECT)
        tmpobj.class = objects[tmpobj.id].oc_class;
    else if (tmpobj.class > -1 && tmpobj.id == STRANGE_OBJECT)
        tmpobj.id = -1;

    if (tmpobj.id == STATUE || tmpobj.id == EGG
        || tmpobj.id == CORPSE || tmpobj.id == TIN
        || tmpobj.id == FIGURINE) {
        struct permonst *pm = NULL;
        boolean nonpmobj = FALSE;
        int i;
        char *montype = get_table_str_opt(L, "montype", NULL);

        if (montype) {
            if ((tmpobj.id == TIN && (!strcmpi(montype, "spinach")
                /* id="tin",montype="empty" produces an empty tin */
                                      || !strcmpi(montype, "empty")))
                /* id="egg",montype="empty" produces a generic, unhatchable
                   egg rather than an "empty egg" */
                || (tmpobj.id == EGG && !strcmpi(montype, "empty"))) {
                tmpobj.corpsenm = NON_PM;
                tmpobj.spe = !strcmpi(montype, "spinach") ? 1 : 0;
                nonpmobj = TRUE;
            } else if (strlen(montype) == 1
                       && def_char_to_monclass(*montype) != MAXMCLASSES) {
                pm = mkclass(def_char_to_monclass(*montype),
                             G_NOGEN | G_IGNORE);
            } else {
                for (i = LOW_PM; i < NUMMONS; i++)
                    if (!strcmpi(mons[i].pmnames[NEUTRAL], montype)
                        || (mons[i].pmnames[MALE] != 0
                            && !strcmpi(mons[i].pmnames[MALE], montype))
                        || (mons[i].pmnames[FEMALE] != 0
                            && !strcmpi(mons[i].pmnames[FEMALE], montype))) {
                        pm = &mons[i];
                        break;
                    }
            }
            free((genericptr_t) montype);
            if (pm)
                tmpobj.corpsenm = monsndx(pm);
            else if (!nonpmobj)
                nhl_error(L, "Unknown montype");
        }
        if (tmpobj.id == STATUE || tmpobj.id == CORPSE) {
            int lflags = 0;

            if (get_table_boolean_opt(L, "historic", 0))
                lflags |= CORPSTAT_HISTORIC;
            if (get_table_boolean_opt(L, "male", 0))
                lflags |= CORPSTAT_MALE;
            if (get_table_boolean_opt(L, "female", 0))
                lflags |= CORPSTAT_FEMALE;
            tmpobj.spe = lflags;
        } else if (tmpobj.id == EGG) {
            tmpobj.spe = get_table_boolean_opt(L, "laid_by_you", 0) ? 1 : 0;
        } else if (!nonpmobj) { /* tmpobj.spe is already set for nonpmobj */
            tmpobj.spe = 0;
        }
    }

    quancnt = (tmpobj.id > STRANGE_OBJECT) ? tmpobj.quan : 0;

    if (container_idx)
        tmpobj.containment |= SP_OBJ_CONTENT;

    if (maybe_contents) {
        lua_getfield(L, 1, "contents");
        if (!lua_isnil(L, -1))
            tmpobj.containment |= SP_OBJ_CONTAINER;
    }

    do {
        otmp = create_object(&tmpobj, gc.coder->croom);
        quancnt--;
    } while ((quancnt > 0) && ((tmpobj.id > STRANGE_OBJECT)
                               && !objects[tmpobj.id].oc_merge));

    if (lua_type(L, -1) == LUA_TFUNCTION) {
        lua_remove(L, -2);
        nhl_push_obj(L, otmp);
        nhl_pcall_handle(L, 1, 0, "lspo_object", NHLpa_panic);
    } else
        lua_pop(L, 1);

    if ((tmpobj.containment & SP_OBJ_CONTAINER) != 0)
        spo_pop_container();

    Free(tmpobj.name.str);

    nhl_push_obj(L, otmp);

    return 1;
}

/* level_flags("noteleport", "mazelevel", ... ); */
int
lspo_level_flags(lua_State *L)
{
    int argc = lua_gettop(L);
    int i;

    create_des_coder();

    if (argc < 1)
        nhl_error(L, "expected string params");

    for (i = 1; i <= argc; i++) {
        const char *s = luaL_checkstring(L, i);

        if (!strcmpi(s, "noteleport"))
            svl.level.flags.noteleport = 1;
        else if (!strcmpi(s, "hardfloor"))
            svl.level.flags.hardfloor = 1;
        else if (!strcmpi(s, "nommap"))
            svl.level.flags.nommap = MAPPABLE_NEVER;
        else if (!strcmpi(s, "nommap-boss"))
            svl.level.flags.nommap = MAPPABLE_BOSSBLOCKED;
        else if (!strcmpi(s, "shortsighted"))
            svl.level.flags.shortsighted = 1;
        else if (!strcmpi(s, "arboreal"))
            svl.level.flags.arboreal = 1;
        else if (!strcmpi(s, "mazelevel"))
            svl.level.flags.is_maze_lev = 1;
        else if (!strcmpi(s, "shroud"))
            svl.level.flags.hero_memory = 1;
        else if (!strcmpi(s, "graveyard"))
            svl.level.flags.graveyard = 1;
        else if (!strcmpi(s, "icedpools"))
            icedpools = 1;
        else if (!strcmpi(s, "premapped"))
            gc.coder->premapped = 1;
        else if (!strcmpi(s, "solidify"))
            gc.coder->solidify = 1;
        else if (!strcmpi(s, "sokoban"))
            Sokoban = 1; /* svl.level.flags.sokoban_rules */
        else if (!strcmpi(s, "inaccessibles"))
            gc.coder->check_inaccessibles = 1;
        else if (!strcmpi(s, "outdoors"))
            svl.level.flags.outdoors = 1;
        else if (!strcmpi(s, "noflipx"))
            gc.coder->allow_flips &= ~2;
        else if (!strcmpi(s, "noflipy"))
            gc.coder->allow_flips &= ~1;
        else if (!strcmpi(s, "noflip"))
            gc.coder->allow_flips = 0;
        else if (!strcmpi(s, "temperate"))
            svl.level.flags.temperature = 0;
        else if (!strcmpi(s, "hot"))
            svl.level.flags.temperature = 1;
        else if (!strcmpi(s, "cold"))
            svl.level.flags.temperature = -1;
        else if (!strcmpi(s, "nomongen"))
            svl.level.flags.rndmongen = 0;
        else if (!strcmpi(s, "nodeathdrops"))
            svl.level.flags.deathdrops = 0;
        else if (!strcmpi(s, "noautosearch"))
            svl.level.flags.noautosearch = 1;
        else if (!strcmpi(s, "fumaroles"))
            svl.level.flags.fumaroles = 1;
        else if (!strcmpi(s, "stormy"))
            svl.level.flags.stormy = 1;
        else {
            char buf[BUFSZ];

            Sprintf(buf, "Unknown level flag %s", s);
            nhl_error(L, buf);
        }
    }

    return 0;
}

/* level_init({ style = "solidfill", fg = " " }); */
/* level_init({ style = "mines", fg = ".", bg = "}",
                smoothed=true, joined=true, lit=0 }) */
int
lspo_level_init(lua_State *L)
{
    static const char *const initstyles[] = {
        "solidfill", "mazegrid", "maze", "mines", "swamp", NULL
    };
    static const int initstyles2i[] = {
        LVLINIT_SOLIDFILL, LVLINIT_MAZEGRID, LVLINIT_MAZE, LVLINIT_MINES,
        LVLINIT_SWAMP, 0
    };
    lev_init init_lev;

    create_des_coder();

    lcheck_param_table(L);

    splev_init_present = TRUE;

    init_lev.init_style
        = initstyles2i[get_table_option(L, "style", "solidfill", initstyles)];
    init_lev.fg = get_table_mapchr_opt(L, "fg", ROOM);
    init_lev.bg = get_table_mapchr_opt(L, "bg", INVALID_TYPE);
    init_lev.smoothed = get_table_boolean_opt(L, "smoothed", FALSE);
    init_lev.joined = get_table_boolean_opt(L, "joined", FALSE);
    init_lev.lit = get_table_boolean_opt(L, "lit", BOOL_RANDOM);
    init_lev.walled = get_table_boolean_opt(L, "walled", FALSE);
    init_lev.filling = get_table_mapchr_opt(L, "filling", init_lev.fg);
    init_lev.corrwid = get_table_int_opt(L, "corrwid", -1);
    init_lev.wallthick = get_table_int_opt(L, "wallthick", -1);
    init_lev.rm_deadends = !get_table_boolean_opt(L, "deadends", TRUE);

    gc.coder->lvl_is_joined = init_lev.joined;

    if (init_lev.bg == INVALID_TYPE)
        init_lev.bg = (init_lev.init_style == LVLINIT_SWAMP) ? MOAT : STONE;

    splev_initlev(&init_lev);

    return 0;
}

/* engraving({ x = 1, y = 1, type="burn", text="Foo" }); */
/* engraving({ coord={1, 1}, type="burn", text="Foo" }); */
/* engraving({x,y}, "engrave", "Foo"); */
/* using "" as the engraving string means a random engraving is desired */
int
lspo_engraving(lua_State *L)
{
    static const char *const engrtypes[] = {
        "dust", "engrave", "burn", "mark", "blood", NULL
    };
    static const int engrtypes2i[] = {
        DUST, ENGRAVE, BURN, MARK, ENGR_BLOOD, 0
    };
    int etyp = DUST;
    char *txt = (char *) 0;
    long ecoord;
    coordxy x = -1, y = -1;
    int argc = lua_gettop(L);
    boolean random = FALSE;
    boolean guardobjs = FALSE;
    boolean wipeout = TRUE;
    struct engr *ep;

    create_des_coder();

    if (argc == 1) {
        lua_Integer ex, ey;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &ex, &ey);
        x = ex;
        y = ey;
        etyp = engrtypes2i[get_table_option(L, "type", "engrave", engrtypes)];
        txt = get_table_str(L, "text");
        wipeout = get_table_boolean_opt(L, "degrade", TRUE);
        guardobjs = get_table_boolean_opt(L, "guardobjects", FALSE);
    } else if (argc == 3) {
        lua_Integer ex, ey;
        (void) get_coord(L, 1, &ex, &ey);
        x = ex;
        y = ey;
        etyp = engrtypes2i[luaL_checkoption(L, 2, "engrave", engrtypes)];
        txt = dupstr(luaL_checkstring(L, 3));
    } else {
        nhl_error(L, "Wrong parameters");
    }

    if (strlen(txt) == 0) { /* "" indicates random engraving */
        random = TRUE;
    }
    if (random) {
        /* whether txt came from the luaL_checkstring or the get_table_str, it's
         * been dupstr'd at some point. */
        Free(txt);
        txt = (char*) alloc(BUFSZ);
        random_engraving(txt);
    }

    if (x == -1 && y == -1)
        ecoord = SP_COORD_PACK_RANDOM(0);
    else
        ecoord = SP_COORD_PACK(x, y);

    get_location_coord(&x, &y, DRY, gc.coder->croom, ecoord);
    make_engr_at(x, y, txt, 0L, etyp);
    Free(txt);
    ep = engr_at(x, y);
    if (ep) {
        ep->guardobjects = guardobjs;
        ep->nowipeout = !wipeout;
    }
    return 0;
}

int
lspo_mineralize(lua_State *L)
{
    int gem_prob, gold_prob, kelp_moat, kelp_pool;

    create_des_coder();

    lcheck_param_table(L);
    /* -1 produces default mineralize behavior */
    gem_prob = get_table_int_opt(L, "gem_prob", -1);
    gold_prob = get_table_int_opt(L, "gold_prob", -1);
    kelp_moat = get_table_int_opt(L, "kelp_moat", -1);
    kelp_pool = get_table_int_opt(L, "kelp_pool", -1);

    mineralize(kelp_pool, kelp_moat, gold_prob, gem_prob, TRUE);

    return 0;
}

static const struct {
    const char *name;
    int type;
} room_types[] = {
    { "ordinary", OROOM },
    { "themed", THEMEROOM },
    { "throne", COURT },
    { "swamp", SWAMP },
    { "vault", VAULT },
    { "beehive", BEEHIVE },
    { "morgue", MORGUE },
    { "barracks", BARRACKS },
    { "zoo", ZOO },
    { "delphi", DELPHI },
    { "temple", TEMPLE },
    { "anthole", ANTHOLE },
    { "cocknest", COCKNEST },
    { "leprehall", LEPREHALL },
    { "demon den", DEMONDEN },
    { "abattoir", ABATTOIR },
    { "submerged", SUBMERGED },
    { "lava room", LAVAROOM },
    { "statuary", STATUARY },
    { "seminary", SEMINARY },
    { "shop", SHOPBASE },
    { "armor shop", ARMORSHOP },
    { "scroll shop", SCROLLSHOP },
    { "potion shop", POTIONSHOP },
    { "weapon shop", WEAPONSHOP },
    { "food shop", FOODSHOP },
    { "ring shop", RINGSHOP },
    { "wand shop", WANDSHOP },
    { "tool shop", TOOLSHOP },
    { "book shop", BOOKSHOP },
    { "health food shop", FODDERSHOP },
    { "candle shop", CANDLESHOP },
    { 0, 0 }
};

staticfn const char *
get_mkroom_name(int rtype)
{
    int i;

    for (i = 0; room_types[i].name; i++)
        if (room_types[i].type == rtype)
            return room_types[i].name;

    impossible("get_mkroom_name unknown rtype %d", rtype);
    return "unknown"; /* not NULL */
}

staticfn int
get_table_roomtype_opt(lua_State *L, const char *name, int defval)
{
    char *roomstr = get_table_str_opt(L, name, emptystr);
    int i, res = defval;

    if (roomstr && *roomstr) {
        for (i = 0; room_types[i].name; i++)
            if (!strcmpi(roomstr, room_types[i].name)) {
                res = room_types[i].type;
                break;
            }
        if (!room_types[i].name)
            impossible("Unknown room type '%s'", roomstr);
    }
    Free(roomstr);
    return res;
}

/* room({ type="ordinary", lit=1, x=3,y=3, xalign="center",yalign="center",
 *        w=11,h=9 }); */
/* room({ lit=1, coord={3,3}, xalign="center",yalign="center", w=11,h=9 }); */
/* room({ coord={3,3}, xalign="center",yalign="center", w=11,h=9,
 *        contents=function(room) ... end }); */
int
lspo_room(lua_State *L)
{
    create_des_coder();

    if (gi.in_mk_themerooms && gt.themeroom_failed)
        return 0;

    lcheck_param_table(L);

    if (gc.coder->n_subroom > MAX_NESTED_ROOMS) {
        panic("Too deeply nested rooms?!");
    } else {
        static const char *const left_or_right[] = {
            "left", "half-left", "center", "half-right", "right",
            "none", "random", NULL
        };
        static const int l_or_r2i[] = {
            SPLEV_LEFT, SPLEV_H_LEFT, SPLEV_CENTER, SPLEV_H_RIGHT,
            SPLEV_RIGHT, -1, -1, -1
        };
        static const char *const top_or_bot[] = {
            "top", "center", "bottom", "none", "random", NULL
        };
        static const int t_or_b2i[] = { SPLEV_TOP, SPLEV_CENTER, SPLEV_BOTTOM, -1, -1, -1 };
        room tmproom;
        struct mkroom *tmpcr;
        lua_Integer rx, ry;

        get_table_xy_or_coord(L, &rx, &ry);
        tmproom.x = rx, tmproom.y = ry;
        if ((tmproom.x == -1 || tmproom.y == -1) && tmproom.x != tmproom.y)
            nhl_error(L, "Room must have both x and y");

        tmproom.w = get_table_int_opt(L, "w", -1);
        tmproom.h = get_table_int_opt(L, "h", -1);

        if ((tmproom.w == -1 || tmproom.h == -1) && tmproom.w != tmproom.h)
            nhl_error(L, "Room must have both w and h");

        tmproom.xalign = l_or_r2i[get_table_option(L, "xalign", "random",
                                                   left_or_right)];
        tmproom.yalign = t_or_b2i[get_table_option(L, "yalign", "random",
                                                   top_or_bot)];
        tmproom.rtype = get_table_roomtype_opt(L, "type", OROOM);
        tmproom.chance = get_table_int_opt(L, "chance", 100);
        tmproom.rlit = get_table_int_opt(L, "lit", -1);
        /* theme rooms default to unfilled */
        tmproom.needfill = get_table_int_opt(L, "filled",
                                             gi.in_mk_themerooms ? 0 : 1);
        tmproom.joined = get_table_boolean_opt(L, "joined", TRUE);

        if (!gc.coder->failed_room[gc.coder->n_subroom - 1]) {
            tmpcr = build_room(&tmproom, gc.coder->croom);
            if (tmpcr) {
                int n = gc.coder->n_subroom;

                gc.coder->tmproomlist[n] = tmpcr; /* TRUE to get here... */
                gc.coder->failed_room[n] = FALSE;
                /* added a subroom, make parent room irregular */
                if (gc.coder->tmproomlist[n - 1])
                    gc.coder->tmproomlist[n - 1]->irregular = TRUE;
                gc.coder->n_subroom++;
                update_croom();
                lua_getfield(L, 1, "contents");
                if (lua_type(L, -1) == LUA_TFUNCTION) {
                    lua_remove(L, -2);
                    l_push_mkroom_table(L, tmpcr);
                    nhl_pcall_handle(L, 1, 0, "lspo_room", NHLpa_panic);
                } else
                    lua_pop(L, 1);
                spo_endroom(gc.coder);
                add_doors_to_room(tmpcr);
                return 0;
            }
            if (gi.in_mk_themerooms)
                gt.themeroom_failed = TRUE;
        } /* failed to create parent room, so fail this too */
    }
    gc.coder->tmproomlist[gc.coder->n_subroom] = (struct mkroom *) 0;
    gc.coder->failed_room[gc.coder->n_subroom] = TRUE;
    gc.coder->n_subroom++;
    update_croom();
    spo_endroom(gc.coder);
    if (gi.in_mk_themerooms)
        gt.themeroom_failed = TRUE;

    return 0;
}

staticfn void
spo_endroom(struct sp_coder *coder UNUSED)
{
    if (gc.coder->n_subroom > 1) {
        gc.coder->n_subroom--;
        gc.coder->tmproomlist[gc.coder->n_subroom] = NULL;
        gc.coder->failed_room[gc.coder->n_subroom] = TRUE;
    } else {
        /* no subroom, get out of top-level room */
        /* Need to ensure xstart/ystart/xsize/ysize have something sensible,
           in case there's some stuff to be created outside the outermost
           room, and there's no MAP. */
        if (gx.xsize <= 1 && gy.ysize <= 1)
            reset_xystart_size();
    }
    update_croom();
}

/* callback for is_ok_location.
   stairs generated at random location shouldn't overwrite special terrain */
staticfn boolean
good_stair_loc(coordxy x, coordxy y)
{
    schar typ = levl[x][y].typ;

    return (typ == ROOM || typ == CORR || typ == ICE);
}

staticfn int
l_create_stairway(lua_State *L, boolean using_ladder)
{
    static const char *const stairdirs[] = { "down", "up", NULL };
    static const int stairdirs2i[] = { 0, 1 };
    int argc = lua_gettop(L);
    coordxy x = -1, y = -1;
    struct trap *badtrap;

    long scoord;
    int up = 0; /* default is down */
    int ltype = lua_type(L, 1);

    create_des_coder();

    if (argc == 1 && ltype == LUA_TTABLE) {
        lua_Integer ax = -1, ay = -1;
        lcheck_param_table(L);
        get_table_xy_or_coord(L, &ax, &ay);
        up = stairdirs2i[get_table_option(L, "dir", "down", stairdirs)];
        x = (coordxy) ax;
        y = (coordxy) ay;
    } else {
        lua_Integer ix = -1, iy = -1;
        if (argc > 0 && ltype == LUA_TSTRING) {
            up = stairdirs2i[luaL_checkoption(L, 1, "down", stairdirs)];
            lua_remove(L, 1);
        }
        nhl_get_xy_params(L, &ix, &iy);
        x = (coordxy) ix;
        y = (coordxy) iy;
    }

    if (x == -1 && y == -1) {
        set_ok_location_func(good_stair_loc);
        scoord = SP_COORD_PACK_RANDOM(0);
    } else
        scoord = SP_COORD_PACK(x, y);

    get_location_coord(&x, &y, DRY, gc.coder->croom, scoord);
    set_ok_location_func(NULL);
    if ((badtrap = t_at(x, y)) != 0)
        deltrap_with_ammo(badtrap, DELTRAP_DESTROY_AMMO);
    SpLev_Map[x][y] = 1;

    if (using_ladder) {
        levl[x][y].typ = LADDER;
        if (up) {
            d_level dest;

            dest.dnum = u.uz.dnum;
            dest.dlevel = u.uz.dlevel - 1;
            stairway_add(x, y, TRUE, TRUE, &dest);
            levl[x][y].ladder = LA_UP;
        } else {
            d_level dest;

            dest.dnum = u.uz.dnum;
            dest.dlevel = u.uz.dlevel + 1;
            stairway_add(x, y, FALSE, TRUE, &dest);
            levl[x][y].ladder = LA_DOWN;
        }
    } else {
        mkstairs(x, y, (char) up, gc.coder->croom,
                 !(scoord & SP_COORD_IS_RANDOM));
    }
    return 0;
}

/* stair("up"); */
/* stair({ dir = "down" }); */
/* stair({ dir = "down", x = 4, y = 7 }); */
/* stair({ dir = "down", coord = {x,y} }); */
/* stair("down", 4, 7); */
/* TODO: stair(selection, "down"); */
/* TODO: stair("up", {x,y}); */
int
lspo_stair(lua_State *L)
{
    return l_create_stairway(L, FALSE);
}

/* ladder("down"); */
/* ladder("up", 6,10); */
/* ladder({ x=11, y=05, dir="down" }); */
int
lspo_ladder(lua_State *L)
{
    return l_create_stairway(L, TRUE);
}

/* grave(); */
/* grave(x,y, "text"); */
/* grave({ x = 1, y = 1 }); */
/* grave({ x = 1, y = 1, text = "Foo" }); */
/* grave({ coord = {1, 1}, text = "Foo" }); */
int
lspo_grave(lua_State *L)
{
    int argc = lua_gettop(L);
    coordxy x, y;
    long scoord;
    lua_Integer ax,ay;
    char *txt;

    create_des_coder();

    if (argc == 3) {
        x = ax = luaL_checkinteger(L, 1);
        y = ay = luaL_checkinteger(L, 2);
        txt = dupstr(luaL_checkstring(L, 3));
    } else {
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &ax, &ay);
        x = ax, y = ay;
        txt = get_table_str_opt(L, "text", NULL);
    }

    if (x == -1 && y == -1)
        scoord = SP_COORD_PACK_RANDOM(0);
    else
        scoord = SP_COORD_PACK(ax, ay);

    get_location_coord(&x, &y, DRY, gc.coder->croom, scoord);

    if (isok(x, y) && !t_at(x, y)) {
        levl[x][y].typ = GRAVE;
        make_grave(x, y, txt); /* note: 'txt' might be Null */
    }
    Free(txt);
    return 0;
}

/* altar({ x=NN, y=NN, align=ALIGNMENT, type=SHRINE }); */
/* des.altar({ coord = {5, 10}, align="noalign", type="altar" }); */
int
lspo_altar(lua_State *L)
{
    static const char *const shrines[] = {
        "altar", "shrine", "sanctum", NULL
    };
    static const int shrines2i[] = { 0, 1, 2, 0 };

    altar tmpaltar;

    lua_Integer x, y;
    long acoord;
    int shrine;
    int al;

    create_des_coder();

    lcheck_param_table(L);

    get_table_xy_or_coord(L, &x, &y);

    al = get_table_align(L);
    shrine = shrines2i[get_table_option(L, "type", "altar", shrines)];

    if (x == -1 && y == -1)
        acoord = SP_COORD_PACK_RANDOM(0);
    else
        acoord = SP_COORD_PACK(x, y);

    tmpaltar.coord = acoord;
    tmpaltar.sp_amask = al;
    tmpaltar.shrine = shrine;

    create_altar(&tmpaltar, gc.coder->croom);

    return 0;
}

static const struct {
    const char *name;
    int type;
} trap_types[] = { { "arrow", ARROW_TRAP },
                   { "dart", DART_TRAP },
                   { "falling rock", ROCKTRAP },
                   { "board", SQKY_BOARD },
                   { "bear", BEAR_TRAP },
                   { "land mine", LANDMINE },
                   { "rolling boulder", ROLLING_BOULDER_TRAP },
                   { "sleep gas", SLP_GAS_TRAP },
                   { "rust", RUST_TRAP },
                   { "fire", FIRE_TRAP },
                   { "cold", COLD_TRAP },
                   { "pit", PIT },
                   { "spiked pit", SPIKED_PIT },
                   { "hole", HOLE },
                   { "trap door", TRAPDOOR },
                   { "teleport", TELEP_TRAP },
                   { "level teleport", LEVEL_TELEP },
                   { "magic portal", MAGIC_PORTAL },
                   { "web", WEB },
                   { "statue", STATUE_TRAP },
                   { "magic", MAGIC_TRAP },
                   { "anti magic", ANTI_MAGIC },
                   { "polymorph", POLY_TRAP },
                   { "vibrating square", VIBRATING_SQUARE },
                   { "random", -1 },
                   { 0, NO_TRAP } };

staticfn int
get_table_traptype_opt(lua_State *L, const char *name, int defval)
{
    char *trapstr = get_table_str_opt(L, name, emptystr);
    int i, res = defval;

    if (trapstr && *trapstr) {
        for (i = 0; trap_types[i].name; i++)
            if (!strcmpi(trapstr, trap_types[i].name)) {
                res = trap_types[i].type;
                break;
            }
    }
    Free(trapstr);
    return res;
}

const char *
get_trapname_bytype(int ttyp)
{
    int i;

    for (i = 0; trap_types[i].name; i++)
        if (ttyp == trap_types[i].type)
            return trap_types[i].name;

    return NULL;
}

staticfn int
get_traptype_byname(const char *trapname)
{
    int i;

    for (i = 0; trap_types[i].name; i++)
        if (!strcmpi(trapname, trap_types[i].name))
            return trap_types[i].type;

    return NO_TRAP;
}

/* trap({ type = "hole", x = 1, y = 1 }); */
/* trap({ type = "web", spider_on_web = 0 }); */
/* trap("hole", 3, 4); */
/* trap("level teleport", {5, 8}); */
/* trap("rust") */
/* trap(); */
int
lspo_trap(lua_State *L)
{
    spltrap tmptrap;
    lua_Integer x, y;
    int argc = lua_gettop(L);

    create_des_coder();

    tmptrap.spider_on_web = TRUE;
    tmptrap.seen = FALSE;
    tmptrap.novictim = FALSE;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        const char *trapstr = luaL_checkstring(L, 1);

        tmptrap.type = get_traptype_byname(trapstr);
        x = y = -1;
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
               && lua_type(L, 2) == LUA_TTABLE) {
        const char *trapstr = luaL_checkstring(L, 1);

        tmptrap.type = get_traptype_byname(trapstr);
        (void) get_coord(L, 2, &x, &y);
    } else if (argc == 3) {
        const char *trapstr = luaL_checkstring(L, 1);

        tmptrap.type = get_traptype_byname(trapstr);
        x = luaL_checkinteger(L, 2);
        y = luaL_checkinteger(L, 3);
    } else {
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &x, &y);
        tmptrap.type = get_table_traptype_opt(L, "type", -1);
        tmptrap.spider_on_web = get_table_boolean_opt(L, "spider_on_web", 1);
        tmptrap.seen = get_table_boolean_opt(L, "seen", FALSE);
        tmptrap.novictim = !get_table_boolean_opt(L, "victim", TRUE);

        lua_getfield(L, -1, "launchfrom");
        if (lua_type(L, -1) == LUA_TTABLE) {
            lua_Integer lx = -1, ly = -1;

            (void) get_coord(L, -1, &lx, &ly);
            lua_pop(L, 1);
            gl.launchplace.x = lx;
            gl.launchplace.y = ly;
        } else
            lua_pop(L, 1);

        lua_getfield(L, -1, "teledest");
        if (lua_type(L, -1) == LUA_TTABLE) {
            lua_Integer lx = -1, ly = -1;

            (void) get_coord(L, -1, &lx, &ly);
            lua_pop(L, 1);
            gl.launchplace.x = lx;
            gl.launchplace.y = ly;
        } else
            lua_pop(L, 1);
    }

    if (tmptrap.type == NO_TRAP)
        nhl_error(L, "Unknown trap type");

    if (x == -1 && y == -1)
        tmptrap.coord = SP_COORD_PACK_RANDOM(0);
    else
        tmptrap.coord = SP_COORD_PACK(x, y);

    create_trap(&tmptrap, gc.coder->croom);
    gl.launchplace.x = gl.launchplace.y = 0;

    return 0;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* gold(500, 3,5); */
/* gold(500, {5, 6}); */
/* gold({ amount = 500, x = 2, y = 5 });*/
/* gold({ amount = 500, coord = {2, 5} });*/
/* gold(); */
int
lspo_gold(lua_State *L)
{
    int argc = lua_gettop(L);
    coordxy x, y;
    long amount;
    long gcoord;
    lua_Integer gldx, gldy;

    create_des_coder();

    if (argc == 3) {
        amount = luaL_checkinteger(L, 1);
        x = gldx = luaL_checkinteger(L, 2);
        y = gldy = luaL_checkinteger(L, 2);
    } else if (argc == 2 && lua_type(L, 2) == LUA_TTABLE) {
        amount = luaL_checkinteger(L, 1);
        (void) get_coord(L, 2, &gldx, &gldy);
        x = gldx;
        y = gldy;
    } else if (argc == 0 || (argc == 1 && lua_type(L, 1) == LUA_TTABLE)) {
        lcheck_param_table(L);

        amount = get_table_int_opt(L, "amount", -1);
        get_table_xy_or_coord(L, &gldx, &gldy);
        x = gldx, y = gldy;
    } else {
        nhl_error(L, "Wrong parameters");
        /*NOTREACHED*/
        return 0;
    }

    if (x == -1 && y == -1)
        gcoord = SP_COORD_PACK_RANDOM(0);
    else
        gcoord = SP_COORD_PACK(gldx, gldy);

    get_location_coord(&x, &y, DRY, gc.coder->croom, gcoord);
    if (amount < 0)
        amount = rnd(200);
    mkgold(amount, x, y);

    return 0;
}

RESTORE_WARNING_UNREACHABLE_CODE

/* corridor({ srcroom=1, srcdoor=2, srcwall="north",
 *            destroom=2, destdoor=1, destwall="west" }); */
int
lspo_corridor(lua_State *L)
{
    static const char *const walldirs[] = {
        "all", "random", "north", "west", "east", "south", NULL
    };
    static const int walldirs2i[] = {
        W_ANY, W_RANDOM, W_NORTH, W_WEST, W_EAST, W_SOUTH, 0
    };
    corridor tc;

    create_des_coder();

    lcheck_param_table(L);

    tc.src.room = get_table_int(L, "srcroom");
    tc.src.door = get_table_int(L, "srcdoor");
    tc.src.wall = walldirs2i[get_table_option(L, "srcwall", "all", walldirs)];
    tc.dest.room = get_table_int(L, "destroom");
    tc.dest.door = get_table_int(L, "destdoor");
    tc.dest.wall = walldirs2i[get_table_option(L, "destwall", "all",
                                               walldirs)];

    create_corridor(&tc);

    return 0;
}

/* random_corridors(); */
int
lspo_random_corridors(lua_State *L UNUSED)
{
    corridor tc;

    create_des_coder();

    tc.src.room = -1;
    tc.src.door = -1;
    tc.src.wall = -1;
    tc.dest.room = -1;
    tc.dest.door = -1;
    tc.dest.wall = -1;

    create_corridor(&tc);

    return 0;
}

/* Choose a single random W_* direction. */
coordxy
random_wdir(void)
{
    static const coordxy wdirs[4] = { W_NORTH, W_SOUTH, W_EAST, W_WEST };
    return wdirs[rn2(4)];
}

static schar floodfillchk_match_under_typ;

staticfn int
floodfillchk_match_under(coordxy x, coordxy y)
{
    return (floodfillchk_match_under_typ == levl[x][y].typ);
}

void
set_floodfillchk_match_under(coordxy typ)
{
    floodfillchk_match_under_typ = typ;
    set_selection_floodfillchk(floodfillchk_match_under);
}

staticfn int
floodfillchk_match_accessible(coordxy x, coordxy y)
{
    return (ACCESSIBLE(levl[x][y].typ)
            || levl[x][y].typ == SDOOR
            || levl[x][y].typ == SCORR);
}

/* change map location terrain type during level creation */
staticfn void
sel_set_ter(coordxy x, coordxy y, genericptr_t arg)
{
    terrain terr;

    terr = *(terrain *) arg;
    if (!set_levltyp_lit(x, y, terr.ter, terr.tlit))
        return;
    /* TODO: move this below into set_levltyp? */
    /* handle doors and secret doors */
    if (levl[x][y].typ == SDOOR || IS_DOOR(levl[x][y].typ)) {
        if (levl[x][y].typ == SDOOR)
            set_doorstate(&levl[x][y], D_CLOSED);
        if (x && (IS_WALL(levl[x - 1][y].typ) || levl[x - 1][y].horizontal))
            levl[x][y].horizontal = 1;
    } else if (levl[x][y].typ == HWALL || levl[x][y].typ == IRONBARS) {
        levl[x][y].horizontal = 1;
    } else if (splev_init_present && levl[x][y].typ == ICE) {
        levl[x][y].icedpool = icedpools ? ICED_POOL : ICED_MOAT;
    } else if (levl[x][y].typ == CLOUD) {
        del_engr_at(x, y); /* clouds cannot have engravings */
    }
}

staticfn void
sel_set_feature(coordxy x, coordxy y, genericptr_t arg)
{
    if (!isok(x, y)) {
#ifdef EXTRA_SANITY_CHECKS
        impossible("sel_set_feature(%i,%i,%i) !isok", x, y, (*(int *) arg));
#endif /*EXTRA_SANITY_CHECKS*/
        return;
    }
    if (IS_FURNITURE(levl[x][y].typ))
        return;
    levl[x][y].typ = (*(int *) arg);
}

staticfn void
sel_set_door(coordxy dx, coordxy dy, genericptr_t arg)
{
    coordxy typ = *(coordxy *) arg;
    coordxy x = dx, y = dy;

    if (!IS_DOOR(levl[x][y].typ) && levl[x][y].typ != SDOOR)
        levl[x][y].typ = (typ & D_SECRET) ? SDOOR : DOOR;
    if (typ & D_SECRET) {
        typ &= ~D_SECRET;
        if (typ < D_CLOSED)
            typ = D_CLOSED;
    }
    set_door_orientation(x, y); /* set/clear levl[x][y].horizontal */
    levl[x][y].doormask = typ;
    clear_nonsense_doortraps(x, y);
    SpLev_Map[x][y] = 1;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* door({ x = 1, y = 1, state = "nodoor" }); */
/* door({ coord = {1, 1}, state = "nodoor" }); */
/* door({ wall = "north", pos = 3, state="secret", locked = 1, iron = 1 }); */
/* door("nodoor", 1, 2); */
/* The 3-arg form of door() cannot force a door to be trapped or iron (except
 * that a "random" door might randomly get any combination of locked, trapped,
 * and iron). If more specificity is needed, the table form must be used. */
int
lspo_door(lua_State *L)
{
    /* Note: in xNetHack locked and secret aren't door states, but we allow the
     * level compiler to specify it. They both imply state = "closed", and
     * locked also implies locked = 1.
     */
#define UNSPECIFIED -1
    static const char *const doorstates[] = {
        "random", "open", "closed", "locked", "nodoor", "broken",
        "secret", NULL
    };
    static const int doorstates2i[] = {
        UNSPECIFIED, D_ISOPEN, D_CLOSED, D_LOCKED, D_NODOOR, D_BROKEN, D_SECRET
    };
    int msk;
    /* ds is one of the values in doorstates2i. */
    int typ, ds, locked, trapped, iron;
    coordxy x, y;
    int argc = lua_gettop(L);

    typ = ds = locked = trapped = iron = UNSPECIFIED;

    /* The approach here for determining the door state and flags:
     * 1. Collect all specifications for the door from the lua file.
     * 2. Generate an entirely random door state.
     * 3. Override the parts of door state that were specified in part 1.
     * This approach enables doors to be "partially random" with the important
     * bits specified.
     */

    create_des_coder();

    if (argc == 3) {
        ds = doorstates2i[luaL_checkoption(L, 1, "random", doorstates)];
        x = luaL_checkinteger(L, 2);
        y = luaL_checkinteger(L, 3);
    } else {
        lua_Integer dx, dy;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &dx, &dy);
        x = dx, y = dy;
        ds = doorstates2i[get_table_option(L, "state", "random", doorstates)];
        locked = get_table_boolean_opt(L, "locked", UNSPECIFIED);
        trapped = get_table_boolean_opt(L, "trapped", UNSPECIFIED);
        iron = get_table_boolean_opt(L, "iron", UNSPECIFIED);
    }

    /* If the door was specified as "closed", and "locked" wasn't specified in
     * the table, that means NOT locked. The level designer has to put "locked"
     * or locked=1 if they want it locked. Otherwise, doors such as Minetown
     * shops that are specified with just "closed" can end up locked. */
    if (ds == D_CLOSED && locked == UNSPECIFIED)
        locked = 0;

    /* Determine if the door is specified as secret or not.
     * By existing convention, doors specified with any state besides "secret"
     * or "random" are not secret.
     * Random doors or doors where a state is not specified have a 50% chance of
     * being secret. */
    typ = DOOR;
    if (ds == D_SECRET || (ds == UNSPECIFIED && !rn2(2))) {
        typ = SDOOR;
        ds = D_CLOSED;
    }

    /* catch possible "locked" specified door in either non-table or table
     * syntax forms; just ensure the door is closed... */
    if (ds == D_LOCKED) {
        ds = D_CLOSED;
        locked = 1;
    }
    /* ds should now be either a valid value for doorstate() or UNSPECIFIED */

    /* shdoor = FALSE because we really don't have any idea whether this will be
     * a shop door. The Lua author has to be trusted to not specify
     * random/nodoor/broken shop doors. */
    msk = random_door_mask(typ, FALSE);

    /* now override parts of the random mask with specified things */
    if (ds != UNSPECIFIED) {
        msk &= ~D_STATEMASK; /* zero these bits */
        msk |= ds;
    }
    if (locked != UNSPECIFIED) {
        msk &= ~D_LOCKED;
        msk |= (locked ? D_LOCKED : 0);
    }
    if (trapped != UNSPECIFIED) {
        msk &= ~D_TRAPPED;
        msk |= (trapped ? D_TRAPPED : 0);
    }
    if (iron != UNSPECIFIED) {
        msk &= ~D_IRON;
        msk |= (iron ? D_IRON : 0);
    }
    if (typ == SDOOR) {
        msk |= D_SECRET;
    }

    if (x == -1 && y == -1) {
        static const char *const walldirs[] = {
            "all", "random", "north", "west", "east", "south", NULL
        };
        /* Note that "random" is also W_ANY, because create_door just wants a
         * mask of acceptable walls */
        static const int walldirs2i[] = {
            W_ANY, W_ANY, W_NORTH, W_WEST, W_EAST, W_SOUTH, 0
        };
        room_door tmpd;
        tmpd.doormask = msk;
        tmpd.secret = (typ == SDOOR);
        tmpd.pos = get_table_int_opt(L, "pos", -1);
        tmpd.wall = walldirs2i[get_table_option(L, "wall", "all", walldirs)];

        create_door(&tmpd, gc.coder->croom);
    } else {
        /*selection_iterate(sel, sel_set_door, (genericptr_t) &typ);*/
        get_location_coord(&x, &y, ANY_LOC, gc.coder->croom,
                           SP_COORD_PACK(x, y));
        if (!isok(x, y)) {
            nhl_error(L, "door coord not ok");
            /*NOTREACHED*/
            return 0;
        }
        sel_set_door(x, y, (genericptr_t) &msk);
    }

    return 0;
}
#undef UNSPECIFIED

RESTORE_WARNING_UNREACHABLE_CODE

staticfn void
l_table_getset_feature_flag(
    lua_State *L,
    int x, int y,
    const char *name,
    int flag)
{
    int val = get_table_boolean_opt(L, name, -2);

    if (val != -2) {
        if (val == -1)
            val = rn2(2);
        if (val)
            levl[x][y].flags |= flag;
        else
            levl[x][y].flags &= ~flag;
    }
}

/* guts of nhl_abs_coord; convert a coordinate relative to a map or room
 * into an absolute coordinate in svl.level.locations.
 *
 * If there is no enclosing map or room, the coordinates are assumed to be
 * absolute already.
 *
 * Part of the reason this is a function is to make it clearer in the calling
 * code that this conversion is what is intended.
 *
 * NOTE: if the coordinates are going to get passed to one of the get_location
 * family of functions, this should NOT be called; get_location already makes
 * an adjustment like this. (What this function supports which get_location
 * doesn't is the input coordinates being negative. get_location will treat
 * that as "level designer wants a random coordinate".) */
void
cvt_to_abscoord(coordxy *x, coordxy *y)
{
    /* since commit 99715e0, xstart and ystart are only relevant in mklev when
     * maps are being used, and 0 otherwise. It is possible in the future that
     * map positions and dimensions can be saved and retrieved outside of
     * mklev which would reintroduce nonzero xstart/ystart/xsiz/ysiz, but
     * this is not currently implemented, so this function can be assumed to
     * have no effect outside of mklev.
     */
    if (gc.coder && gc.coder->croom) {
        *x += gc.coder->croom->lx;
        *y += gc.coder->croom->ly;
    } else {
        *x += gx.xstart;
        *y += gy.ystart;
    }
}

/* inverse of cvt_to_abscoord; turn an absolute svl.level.locations coordinate
 * into one relative to the current map or room. */
void
cvt_to_relcoord(coordxy *x, coordxy *y)
{
    if (gc.coder && gc.coder->croom) {
        *x -= gc.coder->croom->lx;
        *y -= gc.coder->croom->ly;
    } else {
        *x -= gx.xstart;
        *y -= gy.ystart;
    }
}

DISABLE_WARNING_UNREACHABLE_CODE

/* convert map-relative coordinate to absolute.
  local ax,ay = nh.abscoord(rx, ry);
  local pt = nh.abscoord({ x = 10, y = 5 });
 */
int
nhl_abs_coord(lua_State *L)
{
    int argc = lua_gettop(L);
    coordxy x = -1, y = -1;

    if (argc == 2) {
        x = (coordxy) lua_tointeger(L, 1);
        y = (coordxy) lua_tointeger(L, 2);
        cvt_to_abscoord(&x, &y);
        lua_pushinteger(L, x);
        lua_pushinteger(L, y);
        return 2;
    } else if (argc == 1 && lua_type(L, 1) == LUA_TTABLE) {
        x = (coordxy) get_table_int(L, "x");
        y = (coordxy) get_table_int(L, "y");
        cvt_to_abscoord(&x, &y);
        lua_newtable(L);
        nhl_add_table_entry_int(L, "x", x);
        nhl_add_table_entry_int(L, "y", y);
        return 1;
    } else {
        nhl_error(L, "nhl_abs_coord: Wrong args");
        /*NOTREACHED*/
        return 0;
    }
}

/* feature("fountain", x, y); */
/* feature("fountain", {x,y}); */
/* feature({ type="fountain", x=NN, y=NN }); */
/* feature({ type="fountain", coord={NN, NN} }); */
/* feature({ type="tree", coord={NN, NN}, swarm=true, looted=false }); */
int
lspo_feature(lua_State *L)
{
    static const char *const features[] = { "fountain", "sink", "pool",
                                            "throne", "tree", NULL };
    static const int features2i[] = { FOUNTAIN, SINK, POOL,
                                      THRONE, TREE, STONE };
    coordxy x, y;
    int typ;
    int argc = lua_gettop(L);
    boolean can_have_flags = FALSE;
    long fcoord;
    int humidity;

    create_des_coder();

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        typ = features2i[luaL_checkoption(L, 1, NULL, features)];
        x = y = -1;
    } else if (argc == 2 && lua_type(L, 1) == LUA_TSTRING
        && lua_type(L, 2) == LUA_TTABLE) {
        lua_Integer fx, fy;
        typ = features2i[luaL_checkoption(L, 1, NULL, features)];
        (void) get_coord(L, 2, &fx, &fy);
        x = fx;
        y = fy;
    } else if (argc == 3) {
        typ = features2i[luaL_checkoption(L, 1, NULL, features)];
        x = luaL_checkinteger(L, 2);
        y = luaL_checkinteger(L, 3);
    } else {
        lua_Integer fx, fy;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &fx, &fy);
        x = fx, y = fy;
        typ = features2i[get_table_option(L, "type", NULL, features)];
        can_have_flags = TRUE;
    }

    if (x == -1 && y == -1) {
        fcoord = SP_COORD_PACK_RANDOM(0);
        humidity = DRY; /* pick a regular space, no rock or other furniture */
    }
    else {
        fcoord = SP_COORD_PACK(x, y);
        humidity = ANY_LOC; /* assume the author knows what they're doing */
    }
    get_location_coord(&x, &y, humidity, gc.coder->croom, fcoord);

    if (typ == STONE)
        impossible("feature has unknown type param.");
    else
        sel_set_feature(x, y, (genericptr_t) &typ);

    if (levl[x][y].typ != typ || !can_have_flags)
        return 0;

    switch (typ) {
    default:
        break;
    case FOUNTAIN:
        l_table_getset_feature_flag(L, x, y, "looted", F_LOOTED);
        l_table_getset_feature_flag(L, x, y, "warned", F_WARNED);
        break;
    case SINK:
        /* use LPUDDING as a temporary stand-in for the ring
         * note, the L means "looted", so this is rather backwards; setting
         * ring=true in vanilla actually means "set ring-has-already-been-looted
         * to true", thus no ring.
         * To keep this (weird) behavior the same as vanilla's, we also make it
         * backwards so that ring=true gets you no ring, and leaving the option
         * out entirely gives a ring like normal.
         */
        l_table_getset_feature_flag(L, x, y, "ring", S_LPUDDING);
        if (!(levl[x][y].flags & S_LPUDDING)) {
            bury_sink_ring(x, y);
        }
        levl[x][y].flags &= ~S_LPUDDING;

        l_table_getset_feature_flag(L, x, y, "pudding", S_LPUDDING);
        l_table_getset_feature_flag(L, x, y, "dishwasher", S_LDWASHER);
        break;
    case THRONE:
        l_table_getset_feature_flag(L, x, y, "looted", T_LOOTED);
        break;
    case TREE:
        l_table_getset_feature_flag(L, x, y, "looted", TREE_LOOTED);
        l_table_getset_feature_flag(L, x, y, "swarm", TREE_SWARM);
        break;
    }

    return 0;
}

/* gas_cloud({ selection=SELECTION }); */
/* gas_cloud({ selection=SELECTION, damage=N }); */
/* gas_cloud({ selection=SELECTION, damage=N, ttl=N }); */
int
lspo_gas_cloud(lua_State *L)
{
    coordxy x = 0, y = 0;
    struct selectionvar *sel = NULL;
    int argc = lua_gettop(L);
    int damage = 0;
    int ttl = -2;

    create_des_coder();

    if (argc == 1 && lua_type(L, 1) == LUA_TTABLE) {
        lua_Integer tx, ty;
        NhRegion *reg;

        lcheck_param_table(L);

        get_table_xy_or_coord(L, &tx, &ty);
        x = tx, y = ty;
        if (tx == -1 && ty == -1) {
            lua_getfield(L, 1, "selection");
            sel = l_selection_check(L, -1);
            lua_pop(L, 1);
        }
        damage = get_table_int_opt(L, "damage", 0);
        ttl = get_table_int_opt(L, "ttl", -2);
        if (!sel) {
            reg = create_gas_cloud(x, y, 1, damage);
        } else
            reg = create_gas_cloud_selection(sel, damage);
        if (ttl > -2)
            reg->ttl = ttl;
    } else {
        nhl_error(L, "wrong parameters");
    }

    return 0;
}


/*
 * [lit_state: 1 On, 0 Off, -1 random, -2 leave as-is]
 * terrain({ x=NN, y=NN, typ=MAPCHAR, lit=lit_state });
 * terrain({ coord={X, Y}, typ=MAPCHAR, lit=lit_state });
 * terrain({ selection=SELECTION, typ=MAPCHAR, lit=lit_state });
 * terrain( SELECTION, MAPCHAR [, lit_state ] );
 * terrain({x,y}, MAPCHAR);
 * terrain(x,y, MAPCHAR);
 */
int
lspo_terrain(lua_State *L)
{
    terrain tmpterrain;
    coordxy x = 0, y = 0;
    struct selectionvar *sel = NULL;
    int argc = lua_gettop(L);

    create_des_coder();
    tmpterrain.tlit = SET_LIT_NOCHANGE;
    tmpterrain.ter = INVALID_TYPE;

    if (argc == 1) {
        lua_Integer tx, ty;
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &tx, &ty);
        x = tx, y = ty;
        if (tx == -1 && ty == -1) {
            lua_getfield(L, 1, "selection");
            sel = l_selection_check(L, -1);
            lua_pop(L, 1);
        }
        tmpterrain.ter = get_table_mapchr(L, "typ");
        tmpterrain.tlit = get_table_int_opt(L, "lit", SET_LIT_NOCHANGE);
    } else if (argc == 2 && lua_type(L, 1) == LUA_TTABLE
               && lua_type(L, 2) == LUA_TSTRING) {
        lua_Integer tx, ty;
        tmpterrain.ter = check_mapchr(luaL_checkstring(L, 2));
        lua_pop(L, 1);
        (void) get_coord(L, 1, &tx, &ty);
        x = tx;
        y = ty;
    } else if (argc == 2) {
        sel = l_selection_check(L, 1);
        tmpterrain.ter = check_mapchr(luaL_checkstring(L, 2));
    } else if (argc == 3) {
        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);
        tmpterrain.ter = check_mapchr(luaL_checkstring(L, 3));
    } else {
        nhl_error(L, "wrong parameters");
    }

    if (tmpterrain.ter == INVALID_TYPE)
        nhl_error(L, "Erroneous map char");

    if (sel) {
        selection_iterate(sel, sel_set_ter, (genericptr_t) &tmpterrain);
    } else {
        get_location_coord(&x, &y, ANY_LOC, gc.coder->croom,
                           SP_COORD_PACK(x, y));
        if (!isok(x, y)) {
            nhl_error(L, "terrain coord not ok");
            /*NOTREACHED*/
            return 0;
        }
        sel_set_ter(x, y, (genericptr_t) &tmpterrain);
    }

    return 0;
}

/*
 * replace_terrain({ x1=NN,y1=NN, x2=NN,y2=NN, fromterrain=MAPCHAR,
 *                   toterrain=MAPCHAR, lit=N, chance=NN });
 * replace_terrain({ region={x1,y1, x2,y2}, fromterrain=MAPCHAR,
 *                   toterrain=MAPCHAR, lit=N, chance=NN });
 * replace_terrain({ selection=selection.area(2,5, 40,10),
 *                   fromterrain=MAPCHAR, toterrain=MAPCHAR });
 * replace_terrain({ selection=SEL, mapfragment=[[...]],
 *                   toterrain=MAPCHAR });
 */
int
lspo_replace_terrain(lua_State *L)
{
    coordxy totyp, fromtyp;
    struct mapfragment *mf = NULL;
    struct selectionvar *sel = NULL;
    boolean freesel = FALSE;
    coordxy x, y;
    lua_Integer x1, y1, x2, y2;
    int chance;
    int tolit;
    NhRect rect = cg.zeroNhRect;

    create_des_coder();

    lcheck_param_table(L);

    totyp = get_table_mapchr(L, "toterrain");

    if (totyp >= MAX_TYPE)
        return 0;

    fromtyp = get_table_mapchr_opt(L, "fromterrain", INVALID_TYPE);

    if (fromtyp == INVALID_TYPE) {
        const char *err;
        char *tmpstr = get_table_str(L, "mapfragment");
        mf = mapfrag_fromstr(tmpstr);
        free(tmpstr);

        if ((err = mapfrag_error(mf)) != NULL) {
            nhl_error(L, err);
            /*NOTREACHED*/
        }
    }

    chance = get_table_int_opt(L, "chance", 100);
    tolit = get_table_int_opt(L, "lit", SET_LIT_NOCHANGE);
    x1 = get_table_int_opt(L, "x1", -1);
    y1 = get_table_int_opt(L, "y1", -1);
    x2 = get_table_int_opt(L, "x2", -1);
    y2 = get_table_int_opt(L, "y2", -1);

    if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) {
        get_table_region(L, "region", &x1, &y1, &x2, &y2, TRUE);
    }

    if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) {
        lua_getfield(L, 1, "selection");
        if (lua_type(L, -1) != LUA_TNIL)
            sel = l_selection_check(L, -1);
        lua_pop(L, 1);
    }

    if (!sel) {
        sel = selection_new();
        freesel = TRUE;

        if (x1 == -1 && y1 == -1 && x2 == -1 && y2 == -1) {
            (void) selection_clear(sel, 1);
        } else {
            coordxy rx1, ry1, rx2, ry2;
            rx1 = x1, ry1 = y1, rx2 = x2, ry2 = y2;
            get_location(&rx1, &ry1, ANY_LOC, gc.coder->croom);
            get_location(&rx2, &ry2, ANY_LOC, gc.coder->croom);
            for (x = max(rx1, 0); x <= min(rx2, COLNO - 1); x++)
                for (y = max(ry1, 0); y <= min(ry2, ROWNO - 1); y++)
                    selection_setpoint(x, y, sel, 1);
        }
    }

    selection_getbounds(sel, &rect);

    for (x = max(1, rect.lx); x <= rect.hx; x++)
        for (y = rect.ly; y <= rect.hy; y++)
            if (selection_getpoint(x, y,sel)) {
                if (mf) {
                    if (mapfrag_match(mf, x, y) && (rn2(100)) < chance)
                        (void) set_levltyp_lit(x, y, totyp, tolit);
                } else {
                    if (((fromtyp == MATCH_WALL && IS_STWALL(levl[x][y].typ))
                         || levl[x][y].typ == fromtyp)
                        && rn2(100) < chance)
                        (void) set_levltyp_lit(x, y, totyp, tolit);
                }
            }

    if (freesel)
        selection_free(sel, TRUE);

    mapfrag_free(&mf);

    return 0;
}

staticfn boolean
generate_way_out_method(
    coordxy nx, coordxy ny,
    struct selectionvar *ov)
{
    static const int escapeitems[] = {
        PICK_AXE, DWARVISH_MATTOCK, WAN_DIGGING,
        WAN_TELEPORTATION, SCR_TELEPORTATION, RIN_TELEPORTATION
    };
    struct selectionvar *ov2 = selection_new(), *ov3;
    coordxy x, y;
    boolean res = TRUE;

    selection_floodfill(ov2, nx, ny, TRUE);
    ov3 = selection_clone(ov2);

    /* try to make a secret door */
    while (selection_rndcoord(ov3, &x, &y, TRUE)) {
        if (isok(x + 1, y) && !selection_getpoint(x + 1, y, ov)
            && IS_WALL(levl[x + 1][y].typ)
            && isok(x + 2, y) &&  selection_getpoint(x + 2, y, ov)
            && ACCESSIBLE(levl[x + 2][y].typ)) {
            levl[x + 1][y].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x - 1, y) && !selection_getpoint(x - 1, y, ov)
            && IS_WALL(levl[x - 1][y].typ)
            && isok(x - 2, y) && selection_getpoint(x - 2, y, ov)
            && ACCESSIBLE(levl[x - 2][y].typ)) {
            levl[x - 1][y].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x, y + 1) && !selection_getpoint(x, y + 1, ov)
            && IS_WALL(levl[x][y + 1].typ)
            && isok(x, y + 2) && selection_getpoint(x, y + 2, ov)
            && ACCESSIBLE(levl[x][y + 2].typ)) {
            levl[x][y + 1].typ = SDOOR;
            goto gotitdone;
        }
        if (isok(x, y - 1) && !selection_getpoint(x, y - 1, ov)
            && IS_WALL(levl[x][y - 1].typ)
            && isok(x, y - 2) && selection_getpoint(x, y - 2, ov)
            && ACCESSIBLE(levl[x][y - 2].typ)) {
            levl[x][y - 1].typ = SDOOR;
            goto gotitdone;
        }
    }

    /* try to make a hole or a trapdoor */
    if (Can_fall_thru(&u.uz)) {
        selection_free(ov3, TRUE);
        ov3 = selection_clone(ov2);
        while (selection_rndcoord(ov3, &x, &y, TRUE)) {
            if (maketrap(x, y, rn2(2) ? HOLE : TRAPDOOR))
                goto gotitdone;
        }
    }

    /* generate one of the escape items */
    if (selection_rndcoord(ov2, &x, &y, FALSE)) {
        mksobj_at(ROLL_FROM(escapeitems), x, y, TRUE, FALSE);
        goto gotitdone;
    }

    res = FALSE;
 gotitdone:
    selection_free(ov2, TRUE);
    selection_free(ov3, TRUE);
    return res;
}

staticfn void
ensure_way_out(void)
{
    struct selectionvar *ov = selection_new();
    struct trap *ttmp = gf.ftrap;
    coordxy x, y;
    boolean ret = TRUE;
    stairway *stway = gs.stairs;

    set_selection_floodfillchk(floodfillchk_match_accessible);

    while (stway) {
        if (stway->tolev.dnum == u.uz.dnum)
            selection_floodfill(ov, stway->sx, stway->sy, TRUE);
        stway = stway->next;
    }

    while (ttmp) {
        if ((undestroyable_trap(ttmp->ttyp) || is_hole(ttmp->ttyp))
            && !selection_getpoint(ttmp->tx, ttmp->ty, ov))
            selection_floodfill(ov, ttmp->tx, ttmp->ty, TRUE);
        ttmp = ttmp->ntrap;
    }

    do {
        ret = TRUE;
        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++)
                if (ACCESSIBLE(levl[x][y].typ)
                    && !selection_getpoint(x, y, ov)) {
                    if (generate_way_out_method(x, y, ov))
                        selection_floodfill(ov, x, y, TRUE);
                    ret = FALSE;
                    goto outhere;
                }
 outhere:
        ;
    } while (!ret);
    selection_free(ov, TRUE);
}

DISABLE_WARNING_UNREACHABLE_CODE

staticfn lua_Integer
get_table_intarray_entry(lua_State *L, int tableidx, int entrynum)
{
    lua_Integer ret = 0;
    if (tableidx < 0)
        tableidx--;

    lua_pushinteger(L, entrynum);
    lua_gettable(L, tableidx);
    if (lua_isnumber(L, -1)) {
        ret = lua_tointeger(L, -1);
    } else {
        char buf[BUFSZ];

        Sprintf(buf, "Array entry #%i is %s, expected number",
                1, luaL_typename(L, -1));
        nhl_error(L, buf);
    }
    lua_pop(L, 1);
    return ret;
}

staticfn int
get_table_region(
    lua_State *L,
    const char *name,
    lua_Integer *x1, lua_Integer *y1,
    lua_Integer *x2, lua_Integer *y2,
    boolean optional)
{
    lua_Integer arrlen;

    lua_getfield(L, 1, name);
    if (optional && lua_type(L, -1) == LUA_TNIL) {
        lua_pop(L, 1);
        return 1;
    }

    luaL_checktype(L, -1, LUA_TTABLE);

    lua_len(L, -1);
    arrlen = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (arrlen != 4) {
        nhl_error(L, "Not a region");
        /*NOTREACHED*/
        lua_pop(L, 1);
        return 0;
    }

    *x1 = get_table_intarray_entry(L, -1, 1);
    *y1 = get_table_intarray_entry(L, -1, 2);
    *x2 = get_table_intarray_entry(L, -1, 3);
    *y2 = get_table_intarray_entry(L, -1, 4);

    lua_pop(L, 1);
    return 1;
}

boolean
get_coord(lua_State *L, int i, lua_Integer *x, lua_Integer *y)
{
    boolean ret = FALSE;
    int ltyp = lua_type(L, i);

    if (ltyp == LUA_TTABLE) {
        int arrlen;
        boolean gotx = FALSE;

        lua_getfield(L, i, "x");
        if (!lua_isnil(L, -1)) {
            *x = luaL_checkinteger(L, -1);
            gotx = TRUE;
        }
        lua_pop(L, 1);

        if (gotx) {
            lua_getfield(L, i, "y");
            if (!lua_isnil(L, -1)) {
                *y = luaL_checkinteger(L, -1);
                lua_pop(L, 1);
                ret = TRUE;
            } else {
                nhl_error(L, "Not a coordinate");
                /*NOTREACHED*/
                return FALSE;
            }
        } else {
            lua_len(L, i);
            arrlen = lua_tointeger(L, -1);
            lua_pop(L, 1);
            if (arrlen != 2) {
                nhl_error(L, "Not a coordinate");
                /*NOTREACHED*/
                return FALSE;
            }

            *x = get_table_intarray_entry(L, i, 1);
            *y = get_table_intarray_entry(L, i, 2);

            return TRUE;
        }
    } else if (ltyp != LUA_TNIL) {
        /* non-existent coord is ok */
        nhl_error(L, "non-table coord specified");
    }
    return ret;
}

RESTORE_WARNING_UNREACHABLE_CODE

staticfn void
levregion_add(lev_region *lregion)
{
    if (!lregion->in_islev) {
        get_location(&lregion->inarea.x1, &lregion->inarea.y1, ANY_LOC,
                     (struct mkroom *) 0);
        get_location(&lregion->inarea.x2, &lregion->inarea.y2, ANY_LOC,
                     (struct mkroom *) 0);
    }

    if (!lregion->del_islev) {
        get_location(&lregion->delarea.x1, &lregion->delarea.y1,
                     ANY_LOC, (struct mkroom *) 0);
        get_location(&lregion->delarea.x2, &lregion->delarea.y2,
                     ANY_LOC, (struct mkroom *) 0);
    }
    if (gn.num_lregions) {
        /* realloc the lregion space to add the new one */
        lev_region *newl = (lev_region *) alloc(
            sizeof (lev_region) * (unsigned) (1 + gn.num_lregions));

        (void) memcpy((genericptr_t) (newl), (genericptr_t) gl.lregions,
                      sizeof (lev_region) * gn.num_lregions);
        Free(gl.lregions);
        gn.num_lregions++;
        gl.lregions = newl;
    } else {
        gn.num_lregions = 1;
        gl.lregions = (lev_region *) alloc(sizeof (lev_region));
    }
    (void) memcpy(&gl.lregions[gn.num_lregions - 1], lregion,
                  sizeof (lev_region));
}

/* get params from topmost lua hash:
   - region = {x1,y1,x2,y2}
   - exclude = {x1,y1,x2,y2} (optional)
   - region_islev=true, exclude_islev=true (optional)
   - negative x and y are invalid */
staticfn void
l_get_lregion(lua_State *L, lev_region *tmplregion)
{
    lua_Integer x1,y1,x2,y2;

    get_table_region(L, "region", &x1, &y1, &x2, &y2, FALSE);
    tmplregion->inarea.x1 = x1;
    tmplregion->inarea.y1 = y1;
    tmplregion->inarea.x2 = x2;
    tmplregion->inarea.y2 = y2;

    x1 = y1 = x2 = y2 = -1;
    get_table_region(L, "exclude", &x1, &y1, &x2, &y2, TRUE);
    tmplregion->delarea.x1 = x1;
    tmplregion->delarea.y1 = y1;
    tmplregion->delarea.x2 = x2;
    tmplregion->delarea.y2 = y2;

    tmplregion->in_islev = get_table_boolean_opt(L, "region_islev", 0);
    tmplregion->del_islev = get_table_boolean_opt(L, "exclude_islev", 0);

    /* if x1 is still negative, exclude wasn't specified, so we should treat
     * it as if there is no exclude region at all. Force exclude_islev to
     * true so the -1,-1,-1,-1 region is safely off the map and won't
     * interfere with branch or portal placement. */
    if (x1 < 0)
        tmplregion->del_islev = TRUE;
}

/* teleport_region({ region = { x1,y1, x2,y2 } }); */
/* teleport_region({ region = { x1,y1, x2,y2 }, [ region_islev = 1, ]
 *   exclude = { x1,y1, x2,y2 }, [ exclude_islen = 1, ] [ dir = "up" ] }); */
/* TODO: maybe allow using selection, with a new method "getextents()"? */
int
lspo_teleport_region(lua_State *L)
{
    static const char *const teledirs[] = { "both", "down", "up", NULL };
    static const int teledirs2i[] = { LR_TELE, LR_DOWNTELE, LR_UPTELE, -1 };
    lev_region tmplregion;

    create_des_coder();
    lcheck_param_table(L);
    l_get_lregion(L, &tmplregion);
    tmplregion.rtype = teledirs2i[get_table_option(L, "dir", "both",
                                                   teledirs)];
    tmplregion.padding = 0;
    tmplregion.rname.str = NULL;

    levregion_add(&tmplregion);

    return 0;
}

/* TODO: FIXME
   from lev_comp SPO_LEVREGION was called as:
   - STAIR:(x1,y1,x2,y2),(x1,y1,x2,y2),dir
   - PORTAL:(x1,y1,x2,y2),(x1,y1,x2,y2),string
   - BRANCH:(x1,y1,x2,y2),(x1,y1,x2,y2),dir
*/
/* levregion({ region = { x1,y1, x2,y2 }, exclude = { x1,y1, x2,y2 },
 *             type = "portal", name="air" }); */
/* TODO: allow region to be optional, defaulting to whole level */
int
lspo_levregion(lua_State *L)
{
    static const char *const regiontypes[] = {
        "stair-down", "stair-up", "portal", "branch",
        "teleport", "teleport-up", "teleport-down", NULL
    };
    static const int regiontypes2i[] = {
        LR_DOWNSTAIR, LR_UPSTAIR, LR_PORTAL, LR_BRANCH,
        LR_TELE, LR_UPTELE, LR_DOWNTELE, 0
    };
    lev_region tmplregion;

    create_des_coder();
    lcheck_param_table(L);
    l_get_lregion(L, &tmplregion);
    tmplregion.rtype = regiontypes2i[get_table_option(L, "type", "stair-down",
                                                      regiontypes)];
    tmplregion.padding = get_table_int_opt(L, "padding", 0);
    tmplregion.rname.str = get_table_str_opt(L, "name", NULL);

    levregion_add(&tmplregion);
    return 0;
}

/* exclusion({ type = "teleport", region = { x1,y1, x2,y2 } }); */
int
lspo_exclusion(lua_State *L)
{
    static const char *const ez_types[] = {
        "teleport", "teleport-up", "teleport-down", "monster-generation", NULL
    };
    static const int ez_types2i[] = {
        LR_TELE, LR_UPTELE, LR_DOWNTELE, LR_MONGEN, 0
    };
    struct exclusion_zone *ez = (struct exclusion_zone *) alloc(sizeof *ez);
    lua_Integer x1,y1,x2,y2;
    coordxy a1,b1,a2,b2;

    create_des_coder();
    lcheck_param_table(L);
    ez->zonetype = ez_types2i[get_table_option(L, "type", "teleport",
                                                      ez_types)];
    get_table_region(L, "region", &x1, &y1, &x2, &y2, FALSE);

    a1 = x1, b1 = y1;
    a2 = x2, b2 = y2;

    get_location_coord(&a1, &b1, ANY_LOC|NO_LOC_WARN, gc.coder->croom,
                       SP_COORD_PACK(a1, b1));
    get_location_coord(&a2, &b2, ANY_LOC|NO_LOC_WARN, gc.coder->croom,
                       SP_COORD_PACK(a2, b2));

    ez->lx = a1;
    ez->ly = b1;
    ez->hx = a2;
    ez->hy = b2;

    ez->next = sve.exclusion_zones;
    sve.exclusion_zones = ez;
    return 0;
}

staticfn void
sel_set_lit(coordxy x, coordxy y, genericptr_t arg)
{
     int lit = *(int *) arg;

     levl[x][y].lit = (IS_LAVA(levl[x][y].typ) || lit) ? 1 : 0;
}

/* Add to the room any doors within/bordering it */
staticfn void
add_doors_to_room(struct mkroom *croom)
{
    coordxy x, y;
    int i;

    for (x = croom->lx - 1; x <= croom->hx + 1; x++)
        for (y = croom->ly - 1; y <= croom->hy + 1; y++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR)
                maybe_add_door(x, y, croom);
    for (i = 0; i < croom->nsubrooms; i++)
        add_doors_to_room(croom->sbrooms[i]);
}

DISABLE_WARNING_UNREACHABLE_CODE

/* inside a lua table, get fields x1,y1,x2,y2 or region table */
staticfn void
get_table_coords_or_region(lua_State *L,
                           coordxy *dx1, coordxy *dy1,
                           coordxy *dx2, coordxy *dy2)
{
    *dx1 = get_table_int_opt(L, "x1", -1);
    *dy1 = get_table_int_opt(L, "y1", -1);
    *dx2 = get_table_int_opt(L, "x2", -1);
    *dy2 = get_table_int_opt(L, "y2", -1);

    if (*dx1 == -1 && *dy1 == -1 && *dx2 == -1 && *dy2 == -1) {
        lua_Integer rx1, ry1, rx2, ry2;

        get_table_region(L, "region", &rx1, &ry1, &rx2, &ry2, FALSE);
        *dx1 = rx1; *dy1 = ry1;
        *dx2 = rx2; *dy2 = ry2;
    }
}

/* region(selection, lit); */
/* region({ x1=NN, y1=NN, x2=NN, y2=NN, lit=BOOL, type=ROOMTYPE, joined=BOOL,
 *          irregular=BOOL, filled=NN [ , contents = FUNCTION ] }); */
/* region({ region={x1,y1, x2,y2}, type="ordinary" }); */
int
lspo_region(lua_State *L)
{
    coordxy dx1, dy1, dx2, dy2;
    struct mkroom *troom;
    boolean do_arrival_room = FALSE, room_not_needed,
            irregular = FALSE, joined = TRUE;
    int rtype = OROOM, rlit = 1, needfill = 0;
    int argc = lua_gettop(L);

    create_des_coder();

    if (argc <= 1) {
        lcheck_param_table(L);

        /* TODO: "unfilled" => filled=0, "filled" => filled=1, and
         * "lvflags_only" => filled=2, probably in a get_table_needfill_opt */
        needfill = get_table_int_opt(L, "filled", 0);
        irregular = get_table_boolean_opt(L, "irregular", 0);
        joined = get_table_boolean_opt(L, "joined", TRUE);
        do_arrival_room = get_table_boolean_opt(L, "arrival_room", 0);
        rtype = get_table_roomtype_opt(L, "type", OROOM);
        rlit = get_table_int_opt(L, "lit", -1);

        get_table_coords_or_region(L, &dx1, &dy1, &dx2, &dy2);

        if (dx1 == -1 && dy1 == -1 && dx2 == -1 && dy2 == -1) {
            nhl_error(L, "region needs region");
        }

    } else if (argc == 2) {
        /* region(selection, "lit"); */
        static const char *const lits[] = { "unlit", "lit", NULL };
        struct selectionvar *orig = l_selection_check(L, 1),
                            *sel = selection_clone(orig);

        rlit = luaL_checkoption(L, 2, "lit", lits);

        /*
    TODO: lit=random
        */
        if (rlit)
            selection_do_grow(sel, W_ANY);
        selection_iterate(sel, sel_set_lit, (genericptr_t) &rlit);

        selection_free(sel, TRUE);

        /* TODO: skip the rest of this function? */
        return 0;
    } else {
        nhl_error(L, "Wrong parameters");
        /*NOTREACHED*/
        return 0;
    }

    rlit = litstate_rnd(rlit);

    get_location(&dx1, &dy1, ANY_LOC, (struct mkroom *) 0);
    get_location(&dx2, &dy2, ANY_LOC, (struct mkroom *) 0);

    /* Many regions are simple, rectangular areas that just need to set
     * lighting in an area. In that case, we don't need to do anything
     * complicated by creating a room. The exceptions are:
     *  - Special rooms (which usually need to be filled).
     *  - Irregular regions (more convenient to use the room-making code).
     *  - Themed room regions (which often have contents).
     *  - When a room is desired to constrain the arrival of migrating
     *    monsters (see the mon_arrive function for details).
     */
    room_not_needed = (rtype == OROOM && !irregular
                       && !do_arrival_room && !gi.in_mk_themerooms);
    if (room_not_needed || svn.nroom >= MAXNROFROOMS) {
        region tmpregion;
        if (!room_not_needed)
            impossible("Too many rooms on new level!");
        tmpregion.rlit = rlit;
        tmpregion.x1 = dx1;
        tmpregion.y1 = dy1;
        tmpregion.x2 = dx2;
        tmpregion.y2 = dy2;
        light_region(&tmpregion);

        return 0;
    }

    troom = &svr.rooms[svn.nroom];

    /* mark rooms that must be filled, but do it later */
    troom->needfill = needfill;

    troom->needjoining = joined;

    if (irregular) {
        gm.min_rx = gm.max_rx = dx1;
        gm.min_ry = gm.max_ry = dy1;
        gs.smeq[svn.nroom] = svn.nroom;
        flood_fill_rm(dx1, dy1, svn.nroom + ROOMOFFSET, rlit, TRUE);
        add_room(gm.min_rx, gm.min_ry, gm.max_rx, gm.max_ry, FALSE, rtype,
                 TRUE);
        troom->rlit = rlit;
        troom->irregular = TRUE;
    } else {
        add_room(dx1, dy1, dx2, dy2, rlit, rtype, TRUE);
#ifdef SPECIALIZATION
        topologize(troom, FALSE); /* set roomno */
#else
        topologize(troom); /* set roomno */
#endif
    }

    if (!room_not_needed) {
        if (gc.coder->n_subroom > 1) {
            impossible("region as subroom");
        } else {
            gc.coder->tmproomlist[gc.coder->n_subroom] = troom;
            gc.coder->failed_room[gc.coder->n_subroom] = FALSE;
            gc.coder->n_subroom++;
            update_croom();
            lua_getfield(L, 1, "contents");
            if (lua_type(L, -1) == LUA_TFUNCTION) {
                lua_remove(L, -2);
                l_push_mkroom_table(L, troom);
                nhl_pcall_handle(L, 1, 0, "lspo_region", NHLpa_panic);
            } else {
                lua_pop(L, 1);
            }
            spo_endroom(gc.coder);
            add_doors_to_room(troom);
        }
    }

    return 0;
}

/* drawbridge({ dir="east", state="closed", x=05,y=08 }); */
/* drawbridge({ dir="east", state="closed", coord={05,08} }); */
int
lspo_drawbridge(lua_State *L)
{
    static const char *const mwdirs[] = {
        "north", "south", "west", "east", "random", NULL
    };
    static const int mwdirs2i[] = {
        DB_NORTH, DB_SOUTH, DB_WEST, DB_EAST, -1, -2
    };
    static const char *const dbopens[] = {
        "open", "closed", "random", NULL
    };
    static const int dbopens2i[] = { 1, 0, -1, -2 };
    coordxy x, y;
    lua_Integer mx, my;
    int dir;
    int db_open;
    long dcoord;

    create_des_coder();

    lcheck_param_table(L);

    get_table_xy_or_coord(L, &mx, &my);

    dir = mwdirs2i[get_table_option(L, "dir", "random", mwdirs)];
    dcoord = SP_COORD_PACK(mx, my);
    db_open = dbopens2i[get_table_option(L, "state", "random", dbopens)];
    x = mx;
    y = my;

    get_location_coord(&x, &y, DRY | WET | HOT, gc.coder->croom, dcoord);
    if (!isok(mx, my)) {
        nhl_error(L, "drawbridge coord not ok");
        /*NOTREACHED*/
        return 0;
    }
    if (db_open == -1)
        db_open = !rn2(2);
    if (!create_drawbridge(x, y, dir, db_open ? TRUE : FALSE))
        impossible("Cannot create drawbridge.");
    SpLev_Map[x][y] = 1;

    return 0;
}

/* mazewalk({ x = NN, y = NN, typ = ".", dir = "north", stocked = 0 }); */
/* mazewalk({ coord = {XX, YY}, typ = ".", dir = "north", stocked = 0 }); */
/* mazewalk(x,y,dir); */
int
lspo_mazewalk(lua_State *L)
{
    static const char *const mwdirs[] = {
        "north", "south", "east", "west", "random", NULL
    };
    static const int mwdirs2i[] = {
        W_NORTH, W_SOUTH, W_EAST, W_WEST, W_RANDOM, -2
    };
    coordxy x, y;
    lua_Integer mx, my;
    coordxy ftyp = ROOM;
    int fstocked = 1, dir = -1;
    long mcoord;
    int argc = lua_gettop(L);

    create_des_coder();

    if (argc == 3) {
        mx = luaL_checkinteger(L, 1);
        my = luaL_checkinteger(L, 2);
        dir = mwdirs2i[luaL_checkoption(L, 3, "random", mwdirs)];
    } else {
        lcheck_param_table(L);

        get_table_xy_or_coord(L, &mx, &my);
        ftyp = get_table_mapchr_opt(L, "typ", ROOM);
        fstocked = get_table_boolean_opt(L, "stocked", 1);
        dir = mwdirs2i[get_table_option(L, "dir", "random", mwdirs)];
    }

    mcoord = SP_COORD_PACK(mx, my);
    x = mx;
    y = my;

    get_location_coord(&x, &y, ANY_LOC, gc.coder->croom, mcoord);

    if (!isok(x, y)) {
        nhl_error(L, "mazewalk coord not ok");
        /*NOTREACHED*/
        return 0;
    }

    if (ftyp < 1) {
        ftyp = ROOM;
    }

    if (dir == W_RANDOM)
        dir = random_wdir();

    /* don't use move() - it doesn't use W_NORTH, etc. */
    switch (dir) {
    case W_NORTH:
        --y;
        break;
    case W_SOUTH:
        y++;
        break;
    case W_EAST:
        x++;
        break;
    case W_WEST:
        --x;
        break;
    default:
        impossible("mazewalk: Bad direction");
    }

    if (!IS_DOOR(levl[x][y].typ)) {
        levl[x][y].typ = ftyp;
        levl[x][y].flags = 0;
    }

    /*
     * We must be sure that the parity of the coordinates for
     * walkfrom() is odd.  But we must also take into account
     * what direction was chosen.
     */
    if (!(x % 2)) {
        if (dir == W_EAST)
            x++;
        else
            x--;

        /* no need for IS_DOOR check; out of map bounds */
        levl[x][y].typ = ftyp;
        levl[x][y].flags = 0;
    }

    if (!(y % 2)) {
        if (dir == W_SOUTH)
            y++;
        else
            y--;
    }

    walkfrom(x, y, ftyp);
    if (fstocked)
        fill_empty_maze();

    return 0;
}

RESTORE_WARNING_UNREACHABLE_CODE

/* wall_property({ x1=0, y1=0, x2=78, y2=20, property="nondiggable" }); */
/* wall_property({ region = {1,0, 78,20}, property="nonpasswall" }); */
int
lspo_wall_property(lua_State *L)
{
    static const char *const wprops[] = {
        "nondiggable", "nonpasswall", NULL
    };
    static const int wprop2i[] = { W_NONDIGGABLE, W_NONPASSWALL, -1 };
    coordxy dx1 = -1, dy1 = -1, dx2 = -1, dy2 = -1;
    int wprop;

    create_des_coder();

    lcheck_param_table(L);

    get_table_coords_or_region(L, &dx1, &dy1, &dx2, &dy2);

    wprop = wprop2i[get_table_option(L, "property", "nondiggable", wprops)];

    if (dx1 == -1)
        dx1 = gx.xstart - 1;
    if (dy1 == -1)
        dy1 = gy.ystart - 1;
    if (dx2 == -1)
        dx2 = gx.xstart + gx.xsize + 1;
    if (dy2 == -1)
        dy2 = gy.ystart + gy.ysize + 1;

    get_location(&dx1, &dy1, ANY_LOC, (struct mkroom *) 0);
    get_location(&dx2, &dy2, ANY_LOC, (struct mkroom *) 0);

    set_wall_property(dx1, dy1, dx2, dy2, wprop);

    return 0;
}

staticfn void
set_wallprop_in_selection(lua_State *L, int prop)
{
    int argc = lua_gettop(L);
    boolean freesel = FALSE;
    struct selectionvar *sel = (struct selectionvar *) 0;

    create_des_coder();

    if (argc == 1) {
        sel = l_selection_check(L, -1);
    } else if (argc == 0) {
        freesel = TRUE;
        sel = selection_new();
        selection_clear(sel, 1);
    }

    if (sel) {
        selection_iterate(sel, sel_set_wall_property, (genericptr_t) &prop);
        if (freesel)
            selection_free(sel, TRUE);
    }
}

/* non_diggable(selection); */
/* non_diggable(); */
int
lspo_non_diggable(lua_State *L)
{
    set_wallprop_in_selection(L, W_NONDIGGABLE);
    return 0;
}

/* non_passwall(selection); */
/* non_passwall(); */
int
lspo_non_passwall(lua_State *L)
{
    set_wallprop_in_selection(L, W_NONPASSWALL);
    return 0;
}

#if 0
/*ARGSUSED*/
staticfn void
sel_set_wallify(coordxy x, coordxy y, genericptr_t arg UNUSED)
{
    wallify_map(x, y, x, y);
}
#endif

/* TODO: wallify(selection) */
/* wallify({ x1=NN,y1=NN, x2=NN,y2=NN }); */
/* wallify(); */
int
lspo_wallify(lua_State *L)
{
    int dx1 = -1, dy1 = -1, dx2 = -1, dy2 = -1;

    /* TODO: clamp coord values */
    /* TODO: maybe allow wallify({x1,y1}, {x2,y2}) */
    /* TODO: is_table_coord(), is_table_area(),
             get_table_coord(), get_table_area() */

    create_des_coder();

    if (lua_gettop(L) == 1) {
        dx1 = get_table_int(L, "x1");
        dy1 = get_table_int(L, "y1");
        dx2 = get_table_int(L, "x2");
        dy2 = get_table_int(L, "y2");
    }

    wallify_map(dx1 < 0 ? (gx.xstart - 1) : dx1,
                dy1 < 0 ? (gy.ystart - 1) : dy1,
                dx2 < 0 ? (gx.xstart + gx.xsize + 1) : dx2,
                dy2 < 0 ? (gy.ystart + gy.ysize + 1) : dy2);

    return 0;
}

/* reset_level is only needed for testing purposes */
int
lspo_reset_level(lua_State *L)
{
    iflags.lua_testing = TRUE;
    if (L)
        create_des_coder();
    makemap_prepost(TRUE);
    gi.in_mklev = TRUE;
    oinit(); /* assign level dependent obj probabilities */
    clear_level_structures();
    return 0;
}

/* finalize_level is only needed for testing purposes */
int
lspo_finalize_level(lua_State *L)
{
    int i;

    if (L)
        create_des_coder();

    link_doors_rooms();
    remove_boundary_syms();

    /* TODO: ensure_way_out() needs rewrite */
    if (L && gc.coder->check_inaccessibles)
        ensure_way_out();

    map_cleanup();
    wallification(1, 0, COLNO - 1, ROWNO - 1);

    if (L)
        flip_level_rnd(gc.coder->allow_flips, FALSE);

    count_level_features();

    if (L && gc.coder->solidify)
        solidify_map();

    /* This must be done before premap_detect(),
     * otherwise branch stairs won't be premapped. */
    fixup_special();

    if (L && gc.coder->premapped)
        premap_detect();

    level_finalize_topology();

    for (i = 0; i < svn.nroom; ++i) {
        fill_special_room(&svr.rooms[i]);
    }

    makemap_prepost(FALSE);
    iflags.lua_testing = FALSE;
    return 0;
}

DISABLE_WARNING_UNREACHABLE_CODE

/* map({ x = 10, y = 10, map = [[...]] }); */
/* map({ coord = {10, 10}, map = [[...]] }); */
/* map({ halign = "center", valign = "center", map = [[...]] }); */
/* map({ map = [[...]], contents = function(map) ... end }); */
/* map([[...]]) */
/* local selection = map( ... ); */
int
lspo_map(lua_State *L)
{
    /*
TODO: allow passing an array of strings as map data
TODO: handle if map lines aren't same length
TODO: gc.coder->croom needs to be updated
     */

    static const char *const left_or_right[] = {
        "left", "half-left", "center", "half-right", "right", "none", NULL
    };
    static const int l_or_r2i[] = {
        SPLEV_LEFT, SPLEV_H_LEFT, SPLEV_CENTER, SPLEV_H_RIGHT,
        SPLEV_RIGHT, -1, -1
    };
    static const char *const top_or_bot[] = {
        "top", "center", "bottom", "none", NULL
    };
    static const int t_or_b2i[] = { SPLEV_TOP, SPLEV_CENTER, SPLEV_BOTTOM, -1, -1 };
    int lr, tb;
    lua_Integer x = -1, y = -1;
    struct mapfragment *mf;
    char *tmpstr;
    int argc = lua_gettop(L);
    boolean has_contents = FALSE;
    int tryct = 0;
    int ox, oy;
    boolean lit = FALSE;
    struct selectionvar *sel;

    create_des_coder();

    if (gi.in_mk_themerooms && gt.themeroom_failed)
        return 0;

    if (argc == 1 && lua_type(L, 1) == LUA_TSTRING) {
        tmpstr = dupstr(luaL_checkstring(L, 1));
        lr = tb = SPLEV_CENTER;
        mf = mapfrag_fromstr(tmpstr);
        free(tmpstr);
    } else {
        lcheck_param_table(L);
        lr = l_or_r2i[get_table_option(L, "halign", "none", left_or_right)];
        tb = t_or_b2i[get_table_option(L, "valign", "none", top_or_bot)];
        get_table_xy_or_coord(L, &x, &y);
        tmpstr = get_table_str(L, "map");
        lit = (boolean) get_table_boolean_opt(L, "lit", FALSE);
        lua_getfield(L, 1, "contents");
        if (lua_type(L, -1) == LUA_TFUNCTION) {
            lua_remove(L, -2);
            has_contents = TRUE;
        } else {
            lua_pop(L, 1);
        }
        mf = mapfrag_fromstr(tmpstr);
        free(tmpstr);
    }

    if (!mf) {
        nhl_error(L, "Map data error");
        /*NOTREACHED*/
        return 0;
    }

    sel = selection_new();
    ox = x;
    oy = y;
 redo_maploc:
    gx.xsize = mf->wid;
    gy.ysize = mf->hei;

    if (lr == -1 && tb == -1) {
        if (gi.in_mk_themerooms && (ox == -1 || oy == -1)) {
            if (ox == -1) {
                if (gc.coder->croom) {
                    x = somex(gc.coder->croom) - mf->wid;
                    if (x < 1)
                        x = 1;
                } else {
                    x = 1 + rn2(COLNO - 1 - mf->wid);
                }
            }

            if (oy == -1) {
                if (gc.coder->croom) {
                    y = somey(gc.coder->croom) - mf->hei;
                    if (y < 1)
                        y = 1;
                } else {
                    y = rn2(ROWNO - mf->hei);
                }
            }
        }

        if (isok(x, y)) {
            /* x,y is given, place map starting at x,y */
            if (gc.coder->croom) {
                /* in a room? adjust to room relative coords */
                gx.xstart = x + gc.coder->croom->lx;
                gy.ystart = y + gc.coder->croom->ly;
                gx.xsize = min(mf->wid,
                              (gc.coder->croom->hx - gc.coder->croom->lx));
                gy.ysize = min(mf->hei,
                              (gc.coder->croom->hy - gc.coder->croom->ly));
            } else {
                gx.xsize = mf->wid;
                gy.ysize = mf->hei;
                gx.xstart = x;
                gy.ystart = y;
            }
        } else {
            mapfrag_free(&mf);
            nhl_error(L, "Map requires either x,y or halign,valign params");
            selection_free(sel, TRUE);
            return 0;
        }
    } else {
        /* place map starting at halign,valign */
        switch (lr) {
        case SPLEV_LEFT:
            gx.xstart = splev_init_present ? 1 : 3;
            break;
        case SPLEV_H_LEFT:
            gx.xstart = 2 + ((gx.x_maze_max - 2 - gx.xsize) / 4);
            break;
        case SPLEV_CENTER:
            gx.xstart = 2 + ((gx.x_maze_max - 2 - gx.xsize) / 2);
            break;
        case SPLEV_H_RIGHT:
            gx.xstart = 2 + ((gx.x_maze_max - 2 - gx.xsize) * 3 / 4);
            break;
        case SPLEV_RIGHT:
            gx.xstart = gx.x_maze_max - gx.xsize - 1;
            break;
        }
        switch (tb) {
        case SPLEV_TOP:
            gy.ystart = 3;
            break;
        case SPLEV_CENTER:
            gy.ystart = 2 + ((gy.y_maze_max - 2 - gy.ysize) / 2);
            break;
        case SPLEV_BOTTOM:
            gy.ystart = gy.y_maze_max - gy.ysize - 1;
            break;
        }
        if (!(gx.xstart % 2))
            gx.xstart++;
        if (!(gy.ystart % 2))
            gy.ystart++;
    }

    if (gy.ystart < 0 || gy.ystart + gy.ysize > ROWNO) {
        if (gi.in_mk_themerooms) {
            gt.themeroom_failed = TRUE;
            goto skipmap;
        }
        /* try to move the start a bit */
        gy.ystart += (gy.ystart > 0) ? -2 : 2;
        if (gy.ysize == ROWNO)
            gy.ystart = 0;
        if (gy.ystart < 0 || gy.ystart + gy.ysize > ROWNO)
            gy.ystart = 0;
    }
    if (gx.xsize <= 1 && gy.ysize <= 1) {
        reset_xystart_size();
    } else {
        coordxy mptyp;
        coordxy xx, yy;
        terrain terr;

        /* Themed rooms should never overwrite anything */
        if (gi.in_mk_themerooms) {
            boolean isokp = TRUE;
            for (yy = gy.ystart - 1; yy < min(ROWNO, gy.ystart + gy.ysize) + 1;
                 yy++)
                for (xx = gx.xstart - 1;
                     xx < min(COLNO, gx.xstart + gx.xsize) + 1; xx++) {
                    if (!isok(xx, yy)) {
                        isokp = FALSE;
                    } else if (yy < gy.ystart || yy >= (gy.ystart + gy.ysize)
                               || xx < gx.xstart
                               || xx >= (gx.xstart + gx.xsize)) {
                        if (levl[xx][yy].typ != STONE
                            || levl[xx][yy].roomno != NO_ROOM)
                            isokp = FALSE;
                    } else {
                        mptyp = mapfrag_get(mf, xx - gx.xstart, yy - gy.ystart);
                        if (mptyp >= MAX_TYPE)
                            continue;
                        if ((levl[xx][yy].typ != STONE
                             && levl[xx][yy].typ != mptyp)
                            || levl[xx][yy].roomno != NO_ROOM)
                            isokp = FALSE;
                    }
                    if (!isokp) {
                        if (tryct++ < 100 && (lr == -1 || tb == -1))
                            goto redo_maploc;
                        gt.themeroom_failed = TRUE;
                        goto skipmap;
                    }
                }
        }

        /* Load the map */
        for (y = gy.ystart; y < min(ROWNO, gy.ystart + gy.ysize); y++)
            for (x = gx.xstart; x < min(COLNO, gx.xstart + gx.xsize); x++) {
                mptyp = mapfrag_get(mf, (x - gx.xstart), (y - gy.ystart));
                if (mptyp == INVALID_TYPE) {
                    /* TODO: warn about illegal map char */
                    continue;
                }
                if (mptyp >= MAX_TYPE)
                    continue;
                /* clear out levl: load_common_data may set them */
                levl[x][y].flags = 0;
                levl[x][y].horizontal = 0;
                levl[x][y].roomno = 0;
                levl[x][y].edge = 0;
                SpLev_Map[x][y] = 1;
                selection_setpoint(x, y, sel, 1);
                terr.ter = mptyp;
                terr.tlit = lit;
                sel_set_ter(x, y, &terr);
            }
    }

 skipmap:
    mapfrag_free(&mf);

    if (gi.in_mk_themerooms && gt.themeroom_failed) {
        /* this mutated xstart and ystart in the process of trying to make a
         * themed room, so undo them */
        reset_xystart_size();
    }
    else if (has_contents) {
        l_push_wid_hei_table(L, gx.xsize, gy.ysize);
        nhl_pcall_handle(L, 1, 0, "lspo_map", NHLpa_panic);
        reset_xystart_size();
    }

    /* return selection where map locations were put */
    l_selection_push_copy(L, sel);
    selection_free(sel, TRUE);

    return 1;
}

RESTORE_WARNING_UNREACHABLE_CODE

void
update_croom(void)
{
    if (!gc.coder)
        return;

    if (gc.coder->n_subroom)
        gc.coder->croom = gc.coder->tmproomlist[gc.coder->n_subroom - 1];
    else
        gc.coder->croom = NULL;
}

staticfn struct sp_coder *
sp_level_coder_init(void)
{
    int tmpi;
    struct sp_coder *coder = (struct sp_coder *) alloc(sizeof *coder);

    coder->premapped = FALSE;
    coder->solidify = FALSE;
    coder->check_inaccessibles = FALSE;
    coder->allow_flips = 3; /* allow flipping level horiz/vert */
    coder->croom = NULL;
    coder->n_subroom = 1;
    coder->lvl_is_joined = FALSE;
    coder->room_stack = 0;

    splev_init_present = FALSE;
    icedpools = FALSE;

    for (tmpi = 0; tmpi <= MAX_NESTED_ROOMS; tmpi++) {
        coder->tmproomlist[tmpi] = (struct mkroom *) 0;
        coder->failed_room[tmpi] = FALSE;
    }

    update_croom();

    for (tmpi = 0; tmpi < MAX_CONTAINMENT; tmpi++)
        container_obj[tmpi] = NULL;
    container_idx = 0;

    invent_carrying_monster = NULL;

    (void) memset((genericptr_t) SpLev_Map, 0, sizeof SpLev_Map);

    svl.level.flags.is_maze_lev = 0;
    svl.level.flags.temperature = In_hell(&u.uz) ? 1 : 0;
    svl.level.flags.rndmongen = 1;
    svl.level.flags.deathdrops = 1;

    reset_xystart_size();

    return coder;
}


static const struct luaL_Reg nhl_functions[] = {
    { "message", lspo_message },
    { "monster", lspo_monster },
    { "object", lspo_object },
    { "level_flags", lspo_level_flags },
    { "level_init", lspo_level_init },
    { "engraving", lspo_engraving },
    { "mineralize", lspo_mineralize },
    { "door", lspo_door },
    { "stair", lspo_stair },
    { "ladder", lspo_ladder },
    { "grave", lspo_grave },
    { "altar", lspo_altar },
    { "map", lspo_map },
    { "feature", lspo_feature },
    { "terrain", lspo_terrain },
    { "replace_terrain", lspo_replace_terrain },
    { "room", lspo_room },
    { "corridor", lspo_corridor },
    { "random_corridors", lspo_random_corridors },
    { "gold", lspo_gold },
    { "trap", lspo_trap },
    { "mazewalk", lspo_mazewalk },
    { "drawbridge", lspo_drawbridge },
    { "region", lspo_region },
    { "levregion", lspo_levregion },
    { "exclusion", lspo_exclusion },
    { "wallify", lspo_wallify },
    { "wall_property", lspo_wall_property },
    { "non_diggable", lspo_non_diggable },
    { "non_passwall", lspo_non_passwall },
    { "teleport_region", lspo_teleport_region },
    { "reset_level", lspo_reset_level },
    { "finalize_level", lspo_finalize_level },
    { "gas_cloud", lspo_gas_cloud },
    /* TODO: { "branch", lspo_branch }, */
    /* TODO: { "portal", lspo_portal }, */
    { NULL, NULL }
};

/* TODO:

 - if des-file used MAZE_ID to start a level, the level needs
   des.level_flags("mazelevel")
 - expose gc.coder->croom or gx.xstart gy.ystart and gx.xsize gy.ysize to lua.
 - detect a "subroom" automatically.
 - new function get_mapchar(x,y) to return the mapchar on map
 - many params should accept their normal type (eg, int or bool), AND "random"
 - automatically add shuffle(array)
 - automatically add align = { "law", "neutral", "chaos" } and shuffle it.
   (remove from lua files)
 - grab the header comments from des-files and add them to the lua files

*/

void
l_register_des(lua_State *L)
{
    /* register des -table, and functions for it */
    lua_newtable(L);
    luaL_setfuncs(L, nhl_functions, 0);
    lua_setglobal(L, "des");
}

void
create_des_coder(void)
{
    if (!gc.coder)
        gc.coder = sp_level_coder_init();
}

/*
 * General loader
 */
boolean
load_special(const char *name)
{
    boolean result = FALSE;
    nhl_sandbox_info sbi = {NHL_SB_SAFE, 1*1024*1024, 0, 1*1024*1024};

    create_des_coder();

    if (!load_lua(name, &sbi))
        goto give_up;

    link_doors_rooms();
    remove_boundary_syms();

    /* TODO: ensure_way_out() needs rewrite */
    if (gc.coder->check_inaccessibles)
        ensure_way_out();

    map_cleanup();

    wallification(1, 0, COLNO - 1, ROWNO - 1);

    flip_level_rnd(gc.coder->allow_flips, FALSE);

    count_level_features();

    if (gc.coder->solidify)
        solidify_map();

    /* This must be done before premap_detect(),
     * otherwise branch stairs won't be premapped. */
    fixup_special();

    if (gc.coder->premapped)
        premap_detect();

    result = TRUE;
 give_up:
    Free(gc.coder);
    gc.coder = NULL;

    return result;
}

/* Accessor for SpLev_Map for other files.
 * Always returns FALSE for non-special levels. */
boolean
in_splev_map(coordxy x, coordxy y)
{
    if (!Is_special(&u.uz))
        return FALSE;
    if (!maze_inbounds(x, y))
        return FALSE;
    return (SpLev_Map[x][y] != 0);
}

/*sp_lev.c*/

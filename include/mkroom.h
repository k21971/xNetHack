/* NetHack 3.7	mkroom.h	$NHDT-Date: 1725653011 2024/09/06 20:03:31 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.33 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Pasi Kallinen, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MKROOM_H
#define MKROOM_H

/* mkroom.h - types and structures for room and shop initialization */

struct mkroom {
    coordxy lx, hx, ly, hy; /* usually coordxy, but hx may be -1 */
    schar rtype;          /* type of room (zoo, throne, etc...) */
    schar orig_rtype;     /* same as rtype, but not zeroed later */
    schar rlit;           /* is the room lit ? */
    schar needfill;       /* sp_lev: does the room need filling? */
    boolean needjoining;  /* sp_lev: should the room connect to others? */
    schar doorct;         /* door count */
    int   fdoor;          /* index for the first door of the room */
    schar nsubrooms;      /* number of subrooms */
    boolean irregular;    /* true if room is non-rectangular */
    schar roomnoidx;
    struct mkroom *sbrooms[MAX_SUBROOMS]; /* Subrooms pointers */
    struct monst *resident; /* priest/shopkeeper/guard for this room */
};

struct shclass {
    const char *name; /* name of the shop type */
    const char *annotation; /* simpler name for #overview; Null if same */
    char symb;        /* this identifies the shop type */
    int prob;         /* the shop type probability in % */
    schar shdist;     /* object placement type */
#define D_SCATTER 0   /* normal placement */
#define D_SHOP 1      /* shop-like placement */
#define D_TEMPLE 2    /* temple-like placement */
    struct itp {
        int iprob;    /* probability of an item type */
        int itype;    /* item type: if >=0 a class, if < 0 a specific item */
    } iprobs[9];
    const char *const *shknms; /* list of shopkeeper names for this type */
};

/* the normal rooms on the current level are described in svr.rooms[0..n] for
 * some n<MAXNROFROOMS
 * the vault, if any, is described by svr.rooms[n+1]
 * the next svr.rooms entry has hx -1 as a flag
 * there is at most one non-vault special room on a level
 */

/* values for rtype in the room definition structure */
enum roomtype_types {
    OROOM      =  0, /* ordinary room */
    THEMEROOM  =  1, /* like OROOM, but never converted to special room */
    COURT      =  2, /* contains a throne */
    SWAMP      =  3, /* contains pools */
    VAULT      =  4, /* detached room usually reached via teleport trap */
    BEEHIVE    =  5, /* contains killer bees and royal jelly */
    MORGUE     =  6, /* contains corpses, undead and graves */
    BARRACKS   =  7, /* contains soldiers and their gear */
    ZOO        =  8, /* floor covered with treasure and monsters */
    DELPHI     =  9, /* contains Oracle and peripherals */
    TEMPLE     = 10, /* contains a shrine (altar attended by priest[ess]) */
    LEPREHALL  = 11, /* leprechaun hall (Tom Proudfoot) */
    COCKNEST   = 12, /* cockatrice nest (Tom Proudfoot) */
    ANTHOLE    = 13, /* ants (Tom Proudfoot) */
    DEMONDEN   = 14, /* demons */
    SUBMERGED  = 15, /* water floor, sea monsters */
    LAVAROOM   = 16, /* lava floor, fire monsters */
    ABATTOIR   = 17, /* corpses, blood, leashes */
    STATUARY   = 18, /* lined with empty statues */
    SEMINARY   = 19, /* some roaming priests */
    SHOPBASE   = 20, /* everything above this is a shop */
    ARMORSHOP  = 21, /* specific shop defines for level compiler */
    SCROLLSHOP = 22,
    POTIONSHOP = 23,
    WEAPONSHOP = 24,
    FOODSHOP   = 25,
    RINGSHOP   = 26,
    WANDSHOP   = 27,
    TOOLSHOP   = 28,
    BOOKSHOP   = 29,
    FODDERSHOP = 30, /* health food store */
    CANDLESHOP = 31
};

#define MAXRTYPE (CANDLESHOP) /* maximum valid room type */
#define UNIQUESHOP (CANDLESHOP) /* shops here & above not randomly gen'd. */

/* Special type for search_special() */
#define ANY_TYPE (-1)
#define ANY_SHOP (-2)

#define NO_ROOM     0 /* indicates lack of room-occupancy */
#define SHARED      1 /* indicates normal shared boundary */
#define SHARED_PLUS 2 /* indicates shared boundary - extra adjacent-square
                       * searching required */
#define ROOMOFFSET  3 /* (levl[x][y].roomno - ROOMOFFSET) gives svr.rooms[]
                       * index, for inside-squares and non-shared boundaries */

/* Values for needfill */
#define FILL_NONE    0 /* do not fill this room with anything */
#define FILL_NORMAL  1 /* fill the room normally (OROOM or THEMEROOM gets
                        * fill_ordinary_room; any other room type gets stocked
                        * with its usual monsters/objects/terrain) */
#define FILL_LVFLAGS 2 /* special rooms only; set the room's rtype and level
                        * flags as appropriate, but do not put anything in
                        * it */

#define IS_ROOM_PTR(x) \
    ((x) >= svr.rooms && (x) < svr.rooms + MAXNROFROOMS)
#define IS_ROOM_INDEX(x)    ((x) >= 0 && (x) < MAXNROFROOMS)
#define IS_SUBROOM_PTR(x) \
    ((x) >= gs.subrooms && (x) < gs.subrooms + MAXNROFROOMS)
#define IS_SUBROOM_INDEX(x) ((x) > MAXNROFROOMS && (x) <= (MAXNROFROOMS * 2))
#define ROOM_INDEX(x)       ((x) - svr.rooms)
#define SUBROOM_INDEX(x)    ((x) - gs.subrooms)
#define IS_LAST_ROOM_PTR(x) (ROOM_INDEX(x) == svn.nroom)
#define IS_LAST_SUBROOM_PTR(x) \
    (!gn.nsubroom || SUBROOM_INDEX(x) == gn.nsubroom)

#endif /* MKROOM_H */

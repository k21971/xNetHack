## xNetHack 2.0 Changelog

This is the first major update version of xNetHack. It is a fork off of
the vanilla NetHack 3.6.1 release, and is also based directly on
xNetHack 1.0. See doc/fixes36.1 for the devteam's changes.

The xNetHack page at the NetHackWiki, https://nethackwiki.com/wiki/XNetHack,
attempts to describe these changes in a way that's better formatted and more
friendly to players. However, it might be out of date; in case of conflicting
information, this changelog and others in this folder are more up-to-date than
the wiki page, and the commit messages are more up-to-date than this changelog.

On top of any changes made by the NetHack devteam on 3.6.1, and any changes
made in previous xNetHack versions, xNetHack 2.0 contains the following
changes:

### Gameplay Changes

- The object materials patch has been ported (mostly from GruntHack):
  - Objects may now generate being made of a material that isn't their base type.
    Affected objects are armor, weapons, weapon-tools, boxes, a few mundane
    tools, and amulets. All other objects remain the same.
  - This affects their weight, price, AC (for armor), and special effects, such
    as silver affecting demons.
  - Material-specific variants of objects such as silver daggers are all, one
    way or another, removed:
    - Sometimes only the names are changed (e.g. leather armor is now "light
      armor", iron skull cap is now "skull cap").
    - Sometimes the object is removed entirely (e.g. there's no point in silver
      daggers being a distinct item anymore).
    - In a few cases the object is replaced with another object (e.g. mithril
      coats become racial ring mails).
  - Artifacts always have one specific material. This is the same as the base
    item for all artifacts except for Sunsword (gold), Werebane (silver),
    Grayswandir (silver), and the Platinum Yendorian Express Card (platinum).
  - Materials are not wishable except in wizard mode.
  - Items in the hero's initial inventory are always their standard material.
  - Golems can drop items made of their material in addition to their standard
    drops.
  - Glass items always break when thrown, and glass weapons break 1/6 of the
    time when used in melee. Glass armor, if it is randomly targeted by an
    attack, will break 1/6 of the time.
  - Erodeproofing glass items by any of the normal methods shatterproofs it.
    Shatterproof glass items can be wished for outside of wizard mode, but only
    for items which are usually made of glass, such as crystal plate mails. (In
    wizard mode, you may wish for shatterproof not-usually-glass items.)
  - Hobbits' elven ring mails are always mithril.
  - Weapons may get damage bonuses or penalties based on their material; for
    instance, sharp glass weapons do +3 damage, and gold blunt weapons do +2
    damage.
  - Mithril body armor always grants at least MC 2, even if its base item type
    doesn't normally grant that much.
  - Leprechauns will steal any item made of gold.
- Nymphs and foocubi will introduce themselves (giving them a name) if they do
  not already have a name, when they initiate their charm or seduction attacks.
- Permanent hallucination mode: enable it with OPTIONS=hallucination in config.
  This is unbreakable unless you wield Grayswandir. It is counted as a conduct.
- Permanent deafness mode: enable it with OPTIONS=deaf in config. It is counted
  as a conduct.
- Throwing a single gold piece upwards makes you flip it and get heads or
  tails.
- Room-and-corridor level rooms have a 2/5 chance to generate items instead of
  a 1/3 chance.
- Peaceful or tame monsters that are sleeping, paralyzed, or otherwise
  immobilized can never be displaced. Sessile monsters can be displaced 1/6 of
  the time.
- Decrease chance of secret passages generating: they never generate on the
  first four levels; on deeper levels, whenever a secret passage might have
  been generated with its 1% chance, it instead uses a 1 in (level-4 / 2000)
  chance.
- Each barracks in the castle may open into either the courtyard or the throne
  room with equal probability. These doors are no longer locked.
- Worm teeth are made of bone rather than "mysterious" material 0.
- Teleportation onto the Vibrating Square is now allowed.
- You can only rub things if you are in a form with hands.
- When metallivores eat metal from the ground, assuming it has no special
  effects on them, there is a 6/7 chance the metal goes into their inventory
  rather than vanishing.
- Orcish Town now contains all the gear that the shopkeepers, watchmen, and
  priest would have carried, as well as all the shop loot from the destroyed
  shops, scattered around.
- The Astral Plane now contains six random A-class monsters instead of three
  random V and three random L. This may generate Archons, so beware!
- Hitting things with Mjollnir now wakes monsters around the monster it hit.
- Mjollnir can now be invoked for a powerful lightning bolt.
- Port FIQHack's XP curve; this primarily means that a character who
  continuously kills monsters will keep leveling up through the later game.
- Port Fourk's random psuedo-artifact weapon names for random non-artifact but
  very good (highly enchanted, erodeproof, or both) weapons.
- Downgrade the electric eel in the Gnomish Sewer to a regular giant eel.
- Add "gourmet" as a random tin preparation method, giving 350 nutrition.

### Interface changes

- Lots of new hallucinatory monsters, engravings, shirts, headstones, and
  rumors. Also more hallucinatory gods.
- Add the curses interface (OPTIONS=windowtype:curses). And there was much
  rejoicing.
- A message is always printed when monsters are created and the hero is aware
  of them (taking the general form of "foo appear[s] from nowhere!").
- A message is always printed when the hero teleports or level teleports.
- The shopkeeper sell prompt defaults to No instead of Yes, so hitting Enter or
  Space won't sell an item.
- You get a YAFM when you attempt to apply "-".
- You can now rub stones on your hand. Currently the only use for this is a
  YAFM when you are polyselfed into a glass golem and rub an identified
  touchstone on your hand.
- Some golem colors are modified so they can be better distinguished. Straw
  golems are bright blue, rope are black, clay are orange, glass are bright
  cyan.
- When a leprechaun dodges your attack, make this clear: "You miss wildly and
  stumble forwards." Ported from FIQHack.
- Add the hilite_hidden_stairs patch from 343-nao, hilighting stairs obscured
  by objects with a red background. This option defaults to true.
- You can read the label on C- and K-rations.
- Candelabrum gives a helpful tip if you try to apply it with no candles.
- Replace the awful "You feel cold" message for freezing an out of sight door
  with "a deep cracking sound".
- Add YAFM for killing a jabberwock while hallucinating.

### Architectural changes

- Refactor xlogfile bitfields. Blind and nudist are moved to conducts. The
  exact number of bones files generated now has a whole byte to itself, in the
  flags field.
- Add whereis patch. Every time the hero changes levels, a whereis file is
  updated with some information about them.
- Refactor create_critters() and bagotricks() logic so that the latter can call
  the former.
- Add a new monster status "dead" to the level parser, which makes the monster
  instantly die (without a message or any in-game effect) when the level is
  loaded. The point is so that special levels can generate realistic items that
  the monster would have been carrying without having to specify and
  reimplement the logic for that.
- In wizard mode, you can now opt to override an artifact ignoring you because
  it's tired.
- In the tty windowport, a specific background color can now be used.

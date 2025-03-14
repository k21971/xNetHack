## xNetHack 9.0 Changelog

This is a major version of xNetHack. It is based directly on xNetHack 8.0, and
is a fork off the vanilla NetHack 3.7.0 development version release.

The most recent vanilla commit incorporated into xNetHack 9.0 is 2abe156. Note
that because 3.7.0 is still in development status, xNetHack contains major
changes including new monsters, new objects, themed rooms, and other things
*not* documented in this file or other xNetHack changelogs. See doc/fixes37.0
for the DevTeam's changes.

The xNetHack page at the NetHackWiki, https://nethackwiki.com/wiki/XNetHack,
attempts to describe these changes in a way that's better formatted and more
friendly to players. However, the wiki page might be out of date; in case of
conflicting information, this changelog and others in this directory are more
up-to-date than the wiki page, and the commit messages are more up-to-date than
this changelog.

On top of any changes made by the NetHack devteam on 3.7, and any changes
made in previous xNetHack versions, xNetHack 9.0 contains the following
changes:

### Gameplay changes

- The blessed scroll of gold detection also detects gems across the level.
- Every lock in the game may be unopenable by using a credit card. Boxes have a
  40% chance, and doors have a chance that starts at 10% and increases with
  depth.
- Also, if the card is cursed or you are fumbling, the card may slip through the
  lock, ending up inside a box or on the other side of a door.
- Cockatrice nests may contain chickatrices and cockatrice eggs.
- Eating troll meat provides 2d6 turns of intrinsic regneneration.
- Drinking from a magic fountain when either HP or Pw is below 60% may give a
  healing spring effect that increases both maximums by 1 and fully restores
  both, instead of the usual gain ability effect. The fountain still becomes
  nonmagical afterward.
- Monsters will consider if their regular melee attacks might deal a lot more
  damage to the hero than their offensive items would, and may subsequently
  decide to attack in melee rather than using that offensive item.
- Rolling boulders, eating carrots, and occasionally trying to read a dusty
  spellbook (via the dust making you sneeze) causes enough noise to wake nearby
  monsters.
- Monsters stepping on a squeaky board near you can wake you up.
- Leprechauns have a one-sided grudge against gold golems and gold dragons.
- Rock moles can eat gems and rocks (and other objects made of stone, like
  marble wands). Dilithium crystals grant them extra speed. The player can eat
  boulders and (empty) statues when polymorphed into one, but other rock moles
  won't.
- Gain energy potions are guaranteed to restore more energy. A blessed one
  restores at least 40% of your energy maximum and an uncursed one restores at
  least 25%.
- The potion of gain energy can be alchemized by combining the potions of full
  healing and gain ability.
- Sitting down can partly wipe engravings on your space.
- The chance of finding a secret door or passage via searching is no longer
  dependent on Luck.
- Zombies cannot open closed doors.
- Zombies additionally are immune to being scared by any source, including the
  scroll of scare monster.
- Orcs, barbarians, and cavemen have a Wisdom-dependent chance of avoiding the
  urge to take a bath in a fountain.
- Scrolls of earth work again in all levels besides the non-Earth Planes.
- Yeenoghu no longer carries a wand of wishing; instead there is a buried chest
  on his level that may contain it.
- You will only abuse Wisdom when trying to clumsily throw weapons without the
  proper launcher when you actually perceive you're throwing them at a monster,
  but the Wisdom penalty now happens 100% of the time instead of 20%.
- It is no longer possible to levelport downwards or teleport within the same
  level in the Quest before the nemesis is killed.
- Being adjacent to a hezrou may nauseate you, with the odds increasing if there
  are multiple hezrous.
- Hill giants are now the weakest giant, taking over the statblock of the stone
  giant (equivalent to the plain giant) with its 2d8 weapon attack.
- Stone giants are slightly stronger, taking over the statblock of the hill
  giant with its 2d10 weapon attack. They are now able to rip boulders out of
  the floor when they don't already have one, which creates a pit on that
  square.
- Fire giants have a 2d4 fire touch attack.
- Frost giants have a 3d4 cold touch attack and their own encyclopedia entry.
- Storm giants have a 4d4 ranged lightning bolt attack.
- Hrymr's sword on the Valkyrie quest is now named Ice. (Its stats are otherwise
  unchanged, and it is not an artifact.)
- Levels that have been changed following some event, such as the Valkyrie
  locate level, are ineligible to leave bones.
- Luck plays a reduced factor in to-hit calculations, now adding +1 to hit for
  every 3 points of luck, rounded to the next greatest increment: +1 for 1 to 3
  luck, +2 for 4 to 6 luck, -1 for -1 to -3 luck, etc.
- Critical hits on martial arts attacks made at Expert or higher skill may
  disarm an enemy of its weapon and knock it to an adjacent space, with higher
  chance for higher levels of skill.
- Being Skilled or better in martial arts or bare-handed combat now confers an
  bonus to AC when not wearing any body armor or a shield.

### Interface changes

- Wishing for "leather armor" will give the player a leather light armor (its
  equivalent from vanilla).
- Pets are referred to with their correct gender pronoun if they have one,
  rather than "it".
- When you #chat and are next to only one observable monster, you will
  automatically attempt to chat to that monster without being prompted for a
  direction to chat in.
- When you are a few turns from temporary invisibility expiring, you receive a
  message warning you that you're becoming visible again.
- When farlooking a closed and unlocked door you are next to, it will show up as
  "unlocked door" instead of the generic "closed door".
- Produce a message when a nymph teleports away after a successful theft.
- When polymorphed into a monster with an explosion attack, you can #monster to
  intentionally explode. (This was already possible by attacking a monster or
  force-fighting thin air.)
- Escaping out of a polymorph prompt no longer causes a random polymorph; it
  instead gives instructions on how to explicitly request that and prompts
  again. (However, escaping or failing to enter a valid form 5 times still
  causes a random polymorph.)
- The legacy text at the start of the game uses different role-based opening
  lines that replace the standard "It is written in the Book of [deity]:"
- YAFMs:
  - Chatting to a troll (even though you can't see it's a troll) while
    hallucinating.
  - Being life-saved while hallucinating.
  - Monster drinking a potion where you can't see while hallucinating.
- There is now a pager message for the bridge in the Valkyrie locate level being
  restored.

### Architectural changes


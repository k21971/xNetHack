-- Vlad's Tower level 4 (bottom level before the tower proper)
-- Primarily consisting of a deep chasm in which one can fall to their death

des.level_flags("mazelevel", "noteleport", "hardfloor", "nommap", "graveyard", "solidify", "noflipx")
des.message("You arrive in an enormous rock cavern.")
des.message("A deep and perilous chasm yawns before you, and you cannot see the bottom.")
des.map([[
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA....
AAAAAAAAAAAAAAAAAAAAA.AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA......
AAAAAAAAAAAAAAAAAAAAA.AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.AAA...
AAAAAAAAAAAAAAAAAAAA..AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA..A
AAAAAAAAAAAAAAAAAAAA.....AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.AA
AAAAAAAAAAAAAAAAAAAA......AAAAAAAAAAAAAAAAAAAA...AAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAA.......AAAAAAAAAAAAAAAAAAAA..AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAA..AAA..AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.AAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA...AAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA..AAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.....AAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA....AAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAA..AAAA..AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA...AAAAAAAA
.AAAAAAAAAAAAAAAAAAAAAAAAAA.......AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.AAAAAAAAAA
..AAAAAAAAAAAAAAAAAAAAAAAAAAAA.......AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
...AAAAAAAAAAAAAAAAAAAAAAAAA.......AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
.....AAAAAAAAAAAAAAAAAAAAAAAAAA..AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
.......AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
........AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
........AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
]])

des.levregion({ type="branch", region={01,18,01,18} })
des.ladder("up", 75, 01)
des.teleport_region({ region={01,17,04,19} })
des.region({ region={00,00,75,19}, lit=0, type="morgue", filled=2 })

-- A couple demons up from Gehennom
local demonpos = { {22,05}, {32,15}, {47,05}, {67,12} }
shuffle(demonpos)
des.monster("nalfeshnee",demonpos[1])
des.monster("vrock",demonpos[2])

-- Sentries...
local islands = selection.area(17,01,70,17):filter_mapchar('.')
for i = 1, 2 + d(3) do
   des.monster("shrieker", islands:rndcoord())
end

-- Vampires
local innercircle = selection.circle(76,00,10,1)
local outercircle = selection.circle(76,00,20,1) ~ innercircle:clone()
-- guarantee one directly on the ladder
des.monster({ id = "vampire leader", x = 75, y = 01, asleep = 1 })
for i = 1,6+d(3) do
   des.monster({ id = "vampire", coord = {outercircle:rndcoord()}, asleep = percent(80) and 1 or 0 })
   if percent(70) then
      des.monster({ id = "vampire leader", coord ={innercircle:rndcoord()}, asleep = 1 })
   end
end

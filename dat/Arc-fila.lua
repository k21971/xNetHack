-- NetHack 3.7	Arch.des	$NHDT-Date: 1432512784 2015/05/25 00:13:04 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $
--	Copyright (c) 1989 by Jean-Christophe Collet
--	Copyright (c) 1991 by M. Stephenson
-- NetHack may be freely redistributed.  See license for details.
--

des.level_flags("mazelevel", "outdoors", "hardfloor", "noflipx");

des.level_init({ style = "mines", fg = ".", bg=".", lit=1, walled=false });

-- lots of trees in the jungle
alllev = selection.area(00,00,78,20)
trees = alllev:percentage(50)
des.terrain({ selection=trees, typ="T" })

-- make random slices through jungle bits
for i=1,20 do
   local lx, ly = alllev:rndcoord()
   local lx2, ly2 = lx + math.random(-10,10), ly + math.random(-5, 5)
   if alllev:get(lx2, ly2) > 0 then
      des.terrain({ selection=selection.randline(lx, ly, lx2, ly2, 40), typ='.' })
   end
end

-- "stairs" on opposite ends
local leftstair = { selection.line(0,0, 0,20):rndcoord() }
local rightstair = { selection.line(78,1, 78,20):rndcoord() }
-- guarantee a clearing around the stairs
des.terrain({ selection = selection.ellipse(leftstair[1], leftstair[2], 2, 3, 1), typ='.' })
des.terrain({ selection = selection.ellipse(rightstair[1], rightstair[2], 2, 3, 1), typ='.' })
des.stair({ dir = "up", coord = leftstair })
des.stair({ dir = "down", coord = rightstair })

-- guarantee MOST of a path through the forest
des.replace_terrain({ selection=selection.randline(leftstair[1], leftstair[2], rightstair[1], rightstair[2], 80):percentage(75),
                      fromterrain='T', toterrain='.' })

-- lake(s)
local didwater = false
for i=1,d(2) do
   if percent(60) then
      local lakex = math.random(20, 60)
      local lakey = math.random(00, 19)
      local r1 = math.random(3, 6)
      local r2 = math.random(2, 4)
      local mainlake = selection.ellipse(lakex, lakey, r1, r2, 1):percentage(50)
      for i=1,3 do
         local lakeext = mainlake:clone():percentage(15):grow("random")
         mainlake = mainlake | lakeext
      end
      des.terrain({ selection=mainlake, typ='}' })
      didwater = true
   end
end

-- streams
for i=1,d(2) do
   if percent(90) then
      local rivx1 = math.random(20,60)
      local rivx2 = rivx1 + math.random(-15,15)
      if percent(50) then
         rivx1, rivx2 = rivx2, rivx1
      end
      local riv = selection.randline(rivx1, 00, rivx2, 20, 40)
      riv = riv | riv:clone():percentage(35):grow("east")
      riv = riv | riv:clone():percentage(35):grow("west")
      des.terrain({ selection = riv, typ='}' })
      didwater = true
   end
end

-- lots of sticks lying around
for i=1,25 do
   des.object({ id="quarterstaff", material="wooden" })
end

-- monsters
-- can't just do des.monster("S") because that will put it in water, so
-- specifically pick dry spots
-- potential extension: make the lua parser capable of passing down makemon flags and
-- make "no item on top" a makemon flag
dryspots = alllev:clone():filter_mapchar(".")
for i=1,11 do
   des.monster({ class="S", coord={dryspots:rndcoord(1)} })
end
if didwater then
   des.monster("giant eel")
   des.monster("electric eel")
   for i=1,8 do
      des.monster("piranha")
   end
end
denizens = { "panther", "tiger", "jaguar", "lynx", "giant beetle", "monkey", "ape",
             "carnivorous ape", "yeti", "giant spider", "centipede", "raven",
             "forest centaur", "wood nymph", "black naga", "guardian naga",
             "Woodland-elf", "crocodile" }
for i=1,4+d(4) do
   des.monster(denizens[d(#denizens)])
end

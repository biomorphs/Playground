-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}
local Sprites = {}
local MyTexture = TextureHandle:new()
local Gravity = -80

function Playground:InitSprite(i)
	Sprites[i] = {}
	Sprites[i].position = { math.random(-0,-0), math.random(12,12), math.random(0,0) }
	Sprites[i].size = math.random(10,25) * 0.1
	Sprites[i].colour = {math.random(),math.random(),math.random(),0.5 + math.random() * 0.5}
	Sprites[i].velocity = {(-1.0 + (math.random() * 2.0)) * 20, math.random() * 40, (-1.0 + (math.random() * 2.0)) * 20}
	Sprites[i].texture = Graphics.LoadTexture("blob.png");
end

function Playground:Init()
	print("Init!")

	for i=1,3000 do
		Playground:InitSprite(i)
	 end
end

function Playground:Tick()
	local timeDelta = Playground.DeltaTime

	for i=1,#Sprites do
		-- gravity
		Sprites[i].velocity[2] = Sprites[i].velocity[2] + Gravity * timeDelta
	
		if Sprites[i].position[2] < 0 then
			Sprites[i].position[2] = 0
			Sprites[i].velocity[2] = -Sprites[i].velocity[2] * 0.8
		end
	
		if Sprites[i].position[1] < -1000 then
			Sprites[i].position[1] = -1000
			Sprites[i].velocity[1] = -Sprites[i].velocity[1] * 0.8
		end
	
		if Sprites[i].position[1] > 1000 - Sprites[i].size then
			Sprites[i].position[1] = 1000 - Sprites[i].size
			Sprites[i].velocity[1] = -Sprites[i].velocity[1] * 0.8
		end

		if Sprites[i].position[3] < -1000 then
			Sprites[i].position[3] = -1000
			Sprites[i].velocity[3] = -Sprites[i].velocity[3] * 0.8
		end
	
		if Sprites[i].position[3] > 1000 - Sprites[i].size then
			Sprites[i].position[3] = 1000 - Sprites[i].size
			Sprites[i].velocity[3] = -Sprites[i].velocity[3] * 0.8
		end
	
		Sprites[i].size = Sprites[i].size - timeDelta * 4
		if Sprites[i].size < 0 then
			Playground:InitSprite(i)
		end
	
		-- update pos
		Sprites[i].position[1] = Sprites[i].position[1] + Sprites[i].velocity[1] * timeDelta
		Sprites[i].position[2] = Sprites[i].position[2] + Sprites[i].velocity[2] * timeDelta
		Sprites[i].position[3] = Sprites[i].position[3] + Sprites[i].velocity[3] * timeDelta
	
		Graphics.DrawTexturedCube(Sprites[i].position[1], Sprites[i].position[2], Sprites[i].position[3],
		Sprites[i].size, Sprites[i].size, Sprites[i].size,
		Sprites[i].colour[1], Sprites[i].colour[2], Sprites[i].colour[3], Sprites[i].colour[4],
		Sprites[i].texture)

		Graphics.DrawTexturedQuad(
		Sprites[i].position[1], Sprites[i].position[2], 
		Sprites[i].size, Sprites[i].size, 
		Sprites[i].colour[1], Sprites[i].colour[2], Sprites[i].colour[3], Sprites[i].colour[4],
		Sprites[i].texture)
	end
end

function Playground:Shutdown()
	print("Shutting down!")
end

function Playground:new()
	newPlayground = {}
	self.__index = self
	return setmetatable(newPlayground, self)
end
g_playground = Playground:new()
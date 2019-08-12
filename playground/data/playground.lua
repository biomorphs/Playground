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
	Sprites[i].position = { math.random(-20,20), math.random(-20,20), math.random(-20,20) }
	Sprites[i].size = math.random(8,64)
	Sprites[i].colour = {math.random(),math.random(),math.random(),0.5 + math.random() * 0.5}
	Sprites[i].velocity = {(-1.0 + (math.random() * 2.0)) * 100, math.random() * 200, (-1.0 + (math.random() * 2.0)) * 100}

	local tex = math.random(0,1)
	if(tex == 0) then
		Sprites[i].texture = Graphics.LoadTexture("brumas.jpg");
	else 
		Sprites[i].texture = Graphics.LoadTexture("blob.png");
	end
	
end

function Playground:Init()
	print("Init!")

	for i=1,10000 do
		Playground:InitSprite(i)
	end

	Graphics.LookAt(200,200,500,0,0,0)
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
	
		Sprites[i].size = Sprites[i].size - timeDelta * 8
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

		--Graphics.DrawTexturedQuad(
		--Sprites[i].position[1], Sprites[i].position[2], 
		--Sprites[i].size, Sprites[i].size, 
		--Sprites[i].colour[1], Sprites[i].colour[2], Sprites[i].colour[3], Sprites[i].colour[4],
		--Sprites[i].texture)
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
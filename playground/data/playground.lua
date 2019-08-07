-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}
local Sprites = {}
local MyTexture = TextureHandle:new()
local Gravity = -50

function Playground:InitSprite(i)
	Sprites[i] = {}
	Sprites[i].position = { math.random(0,1280), math.random(0,680) }
	Sprites[i].size = math.random(8,256)
	Sprites[i].colour = {math.random(),math.random(),math.random(),math.random()}
	Sprites[i].velocity = {(-1.0 + (math.random() * 2.0)) * 100, math.random() * 200}
	Sprites[i].texture = Graphics.LoadTexture("blob.png");
end

function Playground:Init()
	print("Init!")

	for i=1,1000 do
		Playground:InitSprite(i)
	end
end

function Playground:Tick()
	for i=1,#Sprites do
		-- gravity
		Sprites[i].velocity[2] = Sprites[i].velocity[2] + Gravity * Playground.DeltaTime

		Sprites[i].size = Sprites[i].size - Playground.DeltaTime * 8
		if Sprites[i].size < 0 then
			Playground:InitSprite(i)
		end

		-- update pos
		Sprites[i].position[1] = Sprites[i].position[1] + Sprites[i].velocity[1] * Playground.DeltaTime
		Sprites[i].position[2] = Sprites[i].position[2] + Sprites[i].velocity[2] * Playground.DeltaTime

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
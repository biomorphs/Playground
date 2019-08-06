-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}
local Sprites = {}
local MyTexture
local Gravity = math.random() * -400

function Playground:Init()
	print("Init!")
	MyTexture = Graphics.LoadTexture("blob.png")

	for i=1,100 do
		Sprites[i] = {}
		Sprites[i].position = { math.random(0,1280), math.random(0,680) }
		Sprites[i].size = math.random(8,64)
		Sprites[i].colour = {math.random(),math.random(),math.random(),math.random()}
		Sprites[i].velocity = {(-1.0 + (math.random() * 2.0)) * 100, math.random() * 200}
	end
end

function Playground:Tick()
	for i=1,#Sprites do
		-- gravity
		Sprites[i].velocity[2] = Sprites[i].velocity[2] + Gravity * Playground.DeltaTime

		-- update pos
		Sprites[i].position[1] = Sprites[i].position[1] + Sprites[i].velocity[1] * Playground.DeltaTime
		Sprites[i].position[2] = Sprites[i].position[2] + Sprites[i].velocity[2] * Playground.DeltaTime

		Graphics.DrawTexturedQuad(
		Sprites[i].position[1], Sprites[i].position[2], 
		Sprites[i].size, Sprites[i].size, 
		Sprites[i].colour[1], Sprites[i].colour[2], Sprites[i].colour[3], Sprites[i].colour[4],
		MyTexture)
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
-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}
local Sprites = {}
local MyTexture = TextureHandle:new()
local Gravity = -80
local IslandModel = Graphics.LoadModel("islands.fbx")

function Playground:InitSprite(i)
	Sprites[i] = {}
	Sprites[i].position = { math.random(-0,-0), math.random(12,12), math.random(0,0) }
	Sprites[i].colour = {math.random(),math.random(),math.random(),1}
	Sprites[i].velocity = {(-1.0 + (math.random() * 2.0)) * 20, math.random() * 40, (-1.0 + (math.random() * 2.0)) * 20}
	local modelIndex = math.random(0,2)
	if(modelIndex == 0) then 
		Sprites[i].model = Graphics.LoadModel("cube.fbx")
		Sprites[i].size = math.random(1,10) * 0.2
	end
	if(modelIndex == 1) then
		Sprites[i].model = Graphics.LoadModel("container.fbx")
		Sprites[i].size = math.random(1,10) * 0.1
	end
	
	if(modelIndex == 2) then
		Sprites[i].model = Graphics.LoadModel("cottage_blender.fbx")
		Sprites[i].size = math.random(1,10) * 0.02
		Sprites[i].position[2] = 20
	end
end

function Playground:Init()
	print("Init!")

	for i=1,2000 do
		Playground:InitSprite(i)
	 end
end

function Playground:Tick()
	local timeDelta = Playground.DeltaTime * 0.25

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
	
		Sprites[i].size = Sprites[i].size - timeDelta
		if Sprites[i].size < 0 then
			Playground:InitSprite(i)
		end
	
		-- update pos
		Sprites[i].position[1] = Sprites[i].position[1] + Sprites[i].velocity[1] * timeDelta
		Sprites[i].position[2] = Sprites[i].position[2] + Sprites[i].velocity[2] * timeDelta
		Sprites[i].position[3] = Sprites[i].position[3] + Sprites[i].velocity[3] * timeDelta
	
		Graphics.DrawModel(Sprites[i].position[1], Sprites[i].position[2], Sprites[i].position[3],
		Sprites[i].colour[1], Sprites[i].colour[2], Sprites[i].colour[3], Sprites[i].colour[4],
		Sprites[i].size,Sprites[i].model)
	end

	Graphics.DrawModel(0.0,0.0,0.0,1.0,1.0,1.0,1.0,0.5,IslandModel)
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
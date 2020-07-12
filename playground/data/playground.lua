-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

local LightShader = Graphics.LoadShader("light", "basic.vs", "basic.fs")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")

local LightModel = Graphics.LoadModel("sphere.fbx")
local IslandModel = Graphics.LoadModel("islands_low.fbx")
local ContainerModel = Graphics.LoadModel("container.fbx")
local CottageModel = Graphics.LoadModel("cottage_blender.fbx")
local MonsterModel = Graphics.LoadModel("udim-monster.fbx")

local Lights = {}
local lightSpeedMulti = 100

function Playground:InitLight(i)
	Lights[i] = {}
	Lights[i].Position = {math.random(-20,20),math.random(10,30),math.random(-20,20)}
	Lights[i].Colour = {math.random(0,255)/255.0,math.random(0,255)/255.0,math.random(0,255)/255.0}
	Lights[i].Ambient = 0.02
	Lights[i].Velocity = {0.0,0.0,0.0}
	Lights[i].Cooldown = 0.0
end

function Playground:Init()
	print("Init!")

	for i=1,4 do
		Playground:InitLight(i)
	end
end

function Playground:Tick()
	local timeDelta = Playground.DeltaTime * 0.25
	for i=1,#Lights do
		Lights[i].Cooldown = Lights[i].Cooldown - timeDelta
		
		if(Lights[i].Cooldown <= 0.0) then
			Lights[i].Velocity[1] = (math.random(-100,100) / 100.0)  * lightSpeedMulti
			Lights[i].Velocity[3] = (math.random(-100,100) / 100.0)  * lightSpeedMulti
			Lights[i].Cooldown = math.random(10,100) / 100.0
		end
		Lights[i].Position[1] = Lights[i].Position[1] + Lights[i].Velocity[1] * timeDelta
		Lights[i].Position[2] = Lights[i].Position[2] + Lights[i].Velocity[2] * timeDelta
		Lights[i].Position[3] = Lights[i].Position[3] + Lights[i].Velocity[3] * timeDelta

		Graphics.SetLight(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3], Lights[i].Ambient)
		Graphics.DrawModel(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],1.0,5.0,LightModel,LightShader)
	end

	Graphics.DrawModel(0.0,0.0,0.0,1.0,1.0,1.0,1.0,1.0,IslandModel,DiffuseShader)

	for z=1,40,4 do
		for x=1,40,2 do
			Graphics.DrawModel(-80 + (x * 4),3,-80 + (z * 4),1.0,1.0,1.0,1.0,1.0,ContainerModel,DiffuseShader)
			Graphics.DrawModel(-80 + (x * 4),10,-80 + (z*4),1.0,1.0,1.0,1.0,0.2,CottageModel,DiffuseShader)
		end
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
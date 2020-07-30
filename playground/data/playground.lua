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
local SkeletonModel = Graphics.LoadModel("kenney_graveyardkit_3/skeleton.fbx")
local Coffin = Graphics.LoadModel("kenney_graveyardkit_3/coffin.fbx")

local Lights = {}
local lightSpeedMulti = 100

function Playground:InitLight(i)
	Lights[i] = {}
	Lights[i].Position = {math.random(-50,50),math.random(5,15),math.random(-50,50)}

	local colour = {0.0,0.0,0.0}
	while colour[1] == 0 and colour[2] == 0 and colour[3] == 0 do
		colour = { math.random(1,10) / 10,math.random(1,10) / 10,math.random(1,10) / 10 }
	end

	Lights[i].Colour = colour
	Lights[i].Ambient = 0.02
	Lights[i].Velocity = {0.0,0.0,0.0}
	Lights[i].Cooldown = 0.0
	Lights[i].Attenuation = {1.0,0.07,0.017}
end

function Playground:Init()
	local lightCount = 8
	local lightColourAccum = {0.0,0.0,0.0}
	for i=1,lightCount do
		Playground:InitLight(i)
		lightColourAccum[1] = lightColourAccum[1] + Lights[i].Colour[1]
		lightColourAccum[2] = lightColourAccum[2] + Lights[i].Colour[2]
		lightColourAccum[3] = lightColourAccum[3] + Lights[i].Colour[3]
	end
	lightColourAccum[1] = lightColourAccum[1] / lightCount
	lightColourAccum[2] = lightColourAccum[2] / lightCount
	lightColourAccum[3] = lightColourAccum[3] / lightCount

	Graphics.SetClearColour(lightColourAccum[1],lightColourAccum[2],lightColourAccum[3])
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

		Graphics.PointLight(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3], Lights[i].Ambient, Lights[i].Attenuation[1], Lights[i].Attenuation[2], Lights[i].Attenuation[3])
		Graphics.DrawModel(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],1.0,2.0,LightModel,LightShader)
		Graphics.DebugDrawBox(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],6, Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],1.0)
	end

	local sunMulti = 0.5
	Graphics.DirectionalLight(-0.4,-0.8,0.4,	sunMulti*0.25, sunMulti*0.611, sunMulti*1.0,0.04)

	Graphics.DrawModel(0.0,0.0,0.0,1.0,1.0,1.0,1.0,1.0,IslandModel,DiffuseShader)
	Graphics.DrawModel(0.0,2.0,0.0,1.0,1.0,1.0,1.0,1.0,SkeletonModel,DiffuseShader)
	Graphics.DrawModel(8.0,2.0,0.0,1.0,1.0,1.0,1.0,1.0,Coffin,DiffuseShader)

	local width = 180
	local numPerWidth = 32
	local scale = 0.7

	local halfWidth = width / 2.0
	local gap = width / numPerWidth
	for z=1,numPerWidth do
		for x=1,numPerWidth do
			if((x % 2) == 0) then
				Graphics.DrawModel(-halfWidth + (x * gap),1,-halfWidth + (z*gap),1.0,1.0,1.0,1.0,1.0 * scale,ContainerModel,DiffuseShader)
			else
				Graphics.DrawModel(-halfWidth + (x * gap),1,-halfWidth + (z*gap) + 8,1.0,1.0,1.0,1.0,0.2 * scale,CottageModel,DiffuseShader)
			end			
		end
	end
end

function Playground:Shutdown()
end

function Playground:new()
	newPlayground = {}
	self.__index = self
	return setmetatable(newPlayground, self)
end
g_playground = Playground:new()
-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

local LightShader = Graphics.LoadShader("light", "basic.vs", "basic.fs")
local LightModel = Graphics.LoadModel("sphere.fbx")

local LightColour = {1.0,1.0,1.0}
local LightAmbient = 0.05
local LightPosition = {10.0,30.0,0.0}

local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")

local IslandModel = Graphics.LoadModel("islands.fbx")
local ContainerModel = Graphics.LoadModel("container.fbx")

local lightSpeed = {0.0,0.0,0.0}
local lightCooldown = 0.0

function Playground:Init()
	print("Init!")
end

function Playground:Tick()
	local timeDelta = Playground.DeltaTime * 0.25

	lightCooldown = lightCooldown - timeDelta
	local lightSpeedMulti = 100
	if(lightCooldown <= 0.0) then
		lightSpeed[1] = (math.random(-100,100) / 100.0)  * lightSpeedMulti
		lightSpeed[3] = (math.random(-100,100) / 100.0)  * lightSpeedMulti
		lightCooldown = math.random(10,100) / 100.0
	end
	LightPosition[1] = LightPosition[1] + lightSpeed[1] * timeDelta
	LightPosition[2] = LightPosition[2] + lightSpeed[2] * timeDelta
	LightPosition[3] = LightPosition[3] + lightSpeed[3] * timeDelta

	Graphics.SetLight(LightPosition[1],LightPosition[2],LightPosition[3],LightColour[1],LightColour[2],LightColour[3], LightAmbient)
	Graphics.DrawModel(LightPosition[1],LightPosition[2],LightPosition[3],1.0,1.0,1.0,1.0,5.0,LightModel,LightShader)

	Graphics.DrawModel(0.0,0.0,0.0,1.0,1.0,1.0,1.0,1.0,IslandModel,DiffuseShader)
	Graphics.DrawModel(0.0,3,0.0,1.0,1.0,1.0,1.0,1.0,ContainerModel,DiffuseShader)
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
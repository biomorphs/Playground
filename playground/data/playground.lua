-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

local LightShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local MonsterModel = Graphics.LoadModel("udim-monster.fbx")
local LightModel = Graphics.LoadModel("sphere.fbx")
local IslandModel = Graphics.LoadModel("islands_low.fbx")
local ContainerModel = Graphics.LoadModel("container.fbx")
local CottageModel = Graphics.LoadModel("cottage_blender.fbx")
local Kitchen = Graphics.LoadModel("Kitchen_PUP.fbx")

local Lights = {}
local lightCount = 24
local lightBoxMin = {-180,-36,-128}
local lightBoxMax = {64,80,8}
local lightGravity = -4096.0
local lightBounceMul = 0.9
local lightFriction = 0.95
local lightHistoryMaxValues = 64
local lightHistoryMaxDistance = 1.0		-- distance between history points
local lightXZSpeed = 200
local lightYSpeed = 200
local lightBrightness = 4.0
local lightSphereSize = 2.0

function Playground:Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Playground:pushLightHistory(i,x,y,z)
	local historySize = #Lights[i].History

	if(historySize == lightHistoryMaxValues) then
		table.remove(Lights[i].History,1)
	end
	Lights[i].History[#Lights[i].History + 1] = {x,y,z}

	-- print("History After",#Lights[i].History)
	-- for h=1,#Lights[i].History do
	-- 	print(Lights[i].History[h][1],Lights[i].History[h][2],Lights[i].History[h][3])
	-- end
end

function Playground:GenerateLightCol(i)
	local colour = {0.0,0.0,0.0}
	while colour[1] == 0 and colour[2] == 0 and colour[3] == 0 do
		colour = { lightBrightness * math.random(0,10) / 10, lightBrightness * math.random(0,10) / 10, lightBrightness * math.random(0,10) / 10 }
	end
	Lights[i].Colour = colour
end

function Playground:InitLight(i)
	Lights[i] = {}
	Lights[i].Position = {math.random(lightBoxMin[1],lightBoxMax[1]),math.random(lightBoxMin[2],lightBoxMax[2]),math.random(lightBoxMin[3],lightBoxMax[3])}
	Playground:GenerateLightCol(i)
	Lights[i].Ambient = 0.0
	Lights[i].Velocity = {0.0,0.0,0.0}
	Lights[i].Attenuation = {1.0, 0.14, 0.07}
	Lights[i].History = {}
	Playground:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
end

function Playground:Init()
	local lightColourAccum = {0.0,0.0,0.0}
	for i=1,lightCount do
		Playground:InitLight(i)
	end

	Graphics.SetClearColour(0,0,0)
end

function DrawGrid(startX,endX,stepX,startZ,endZ,stepZ,yAxis)
	for x=startX,endX,stepX do
		Graphics.DebugDrawLine(x,yAxis,startZ,x,yAxis,endZ,0.2,0.2,0.2,0.2,0.2,0.2)
	end
	for z=startZ,endZ,stepZ do
		Graphics.DebugDrawLine(startX,yAxis,z,endX,yAxis,z,0.2,0.2,0.2,0.2,0.2,0.2)
	end
end

function Playground:Tick()
	local timeDelta = Playground.DeltaTime * 0.25
	for i=1,#Lights do
		if(Playground:Vec3Length(Lights[i].Velocity) < 16.0) then
			Lights[i].Velocity[1] = (math.random(-100,100) / 100.0)  * lightXZSpeed
			Lights[i].Velocity[2] = (math.random(200,400) / 100.0)  * lightYSpeed
			Lights[i].Velocity[3] = (math.random(-100,100) / 100.0)  * lightXZSpeed
			Playground:GenerateLightCol(i)
		end

		local lastHistoryPos = Lights[i].History[#Lights[i].History]
		local travelled = {}
		travelled[1] = Lights[i].Position[1] - lastHistoryPos[1]
		travelled[2] = Lights[i].Position[2] - lastHistoryPos[2]
		travelled[3] = Lights[i].Position[3] - lastHistoryPos[3]
		local lastHistoryDistance = Playground:Vec3Length(travelled)
		if(lastHistoryDistance > lightHistoryMaxDistance) then
			Playground:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
		end

		Lights[i].Velocity[2] = Lights[i].Velocity[2] + (lightGravity * timeDelta);
		Lights[i].Position[1] = Lights[i].Position[1] + Lights[i].Velocity[1] * timeDelta
		Lights[i].Position[2] = Lights[i].Position[2] + Lights[i].Velocity[2] * timeDelta
		Lights[i].Position[3] = Lights[i].Position[3] + Lights[i].Velocity[3] * timeDelta

		if(Lights[i].Position[1] < lightBoxMin[1]) then
			Lights[i].Position[1] = lightBoxMin[1]
			Lights[i].Velocity[1] = -Lights[i].Velocity[1] * lightBounceMul
		end
		if(Lights[i].Position[2] < lightBoxMin[2]) then
			Lights[i].Position[2] = lightBoxMin[2]
			Lights[i].Velocity[1] = Lights[i].Velocity[1] * lightFriction
			Lights[i].Velocity[2] = -Lights[i].Velocity[2] * lightBounceMul
			Lights[i].Velocity[3] = Lights[i].Velocity[3] * lightFriction
		end
		if(Lights[i].Position[3] < lightBoxMin[3]) then
			Lights[i].Position[3] = lightBoxMin[3]
			Lights[i].Velocity[3] = -Lights[i].Velocity[3] * lightBounceMul
		end
		if(Lights[i].Position[1] > lightBoxMax[1]) then
			Lights[i].Position[1] = lightBoxMax[1]
			Lights[i].Velocity[1] = -Lights[i].Velocity[1] * lightBounceMul
		end
		if(Lights[i].Position[2] > lightBoxMax[2]) then
			Lights[i].Position[2] = lightBoxMax[2]
			Lights[i].Velocity[2] = -Lights[i].Velocity[2] * lightBounceMul
		end
		if(Lights[i].Position[3] > lightBoxMax[3]) then
			Lights[i].Position[3] = lightBoxMax[3]
			Lights[i].Velocity[3] = -Lights[i].Velocity[3] * lightBounceMul
		end

		Graphics.PointLight(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3], Lights[i].Ambient, Lights[i].Attenuation[1], Lights[i].Attenuation[2], Lights[i].Attenuation[3])
		Graphics.DrawModel(Lights[i].Position[1],Lights[i].Position[2],Lights[i].Position[3],Lights[i].Colour[1],Lights[i].Colour[2],Lights[i].Colour[3],1.0,lightSphereSize,LightModel,LightShader)

		for h=1,#Lights[i].History-1 do
			local alpha = 1.0 - (((#Lights[i].History - h) / #Lights[i].History))
			Graphics.DebugDrawLine(Lights[i].History[h][1],Lights[i].History[h][2],Lights[i].History[h][3],
								   Lights[i].History[h+1][1],Lights[i].History[h+1][2],Lights[i].History[h+1][3],
								   alpha * Lights[i].Colour[1],alpha * Lights[i].Colour[2],alpha * Lights[i].Colour[3],
								   alpha * Lights[i].Colour[1],alpha * Lights[i].Colour[2],alpha * Lights[i].Colour[3])
		end
	end

	DrawGrid(-256,256,32,-256,256,32,-40.0)

	local sunMulti = 0.3
	Graphics.DirectionalLight(0.7,-0.4,-0.2, sunMulti*0.25, sunMulti*0.611, sunMulti*1.0, 0.01)

	Graphics.DebugDrawAxis(0.0,8.0,0.0,8.0);
	Graphics.DrawModel(0.0,1.0,0.0,1.0,1.0,1.0,1.0,0.15,IslandModel,DiffuseShader)
	Graphics.DrawModel(0.0,1.3,0.0,1.0,1.0,1.0,1.0,1.0,MonsterModel,DiffuseShader)
	Graphics.DrawModel(-96.0,-40.5,-95.0,1.0,1.0,1.0,1.0,0.5,Kitchen,DiffuseShader)

	local width = 32
	local numPerWidth = 32
	local scale = 0.1
	local halfWidth = width / 2.0
	local gap = width / numPerWidth
	for z=1,numPerWidth do
		for x=1,numPerWidth do
			if((x % 2) == 0) then
				Graphics.DrawModel(-halfWidth + (x * gap),1,-halfWidth + (z*gap),1.0,1.0,1.0,1.0,1.0 * scale,ContainerModel,DiffuseShader)
			else
				Graphics.DrawModel(-halfWidth + (x * gap),1,-halfWidth + (z*gap),1.0,1.0,1.0,1.0,0.2 * scale,CottageModel,DiffuseShader)
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
-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

local LightShader = Graphics.LoadShader("light",  "basic.vs", "basic.fs")
local DiffuseShader = Graphics.LoadShader("diffuse", "simplediffuse.vs", "simplediffuse.fs")
local Sponza = Graphics.LoadModel("sponza.obj")
local MonsterModel = Graphics.LoadModel("udim-monster.fbx")
local LightModel = Graphics.LoadModel("sphere.fbx")
local IslandModel = Graphics.LoadModel("islands_low.fbx")
local ContainerModel = Graphics.LoadModel("container.fbx")
local CottageModel = Graphics.LoadModel("cottage_blender.fbx")

local Lights = {}
local lightCount = 32
local lightBoxMin = {-284,2,-125}
local lightBoxMax = {256,228,113}
local lightRadiusRange = {32,32}
local lightGravity = -4096.0
local lightBounceMul = 0.9
local lightFriction = 0.95
local lightHistoryMaxValues = 32
local lightHistoryMaxDistance = 0.25		-- distance between history points
local lightXZSpeed = 200
local lightYSpeed = 200
local lightBrightness = 2.0
local lightSphereSize = 2.0
local SunMulti = 0.1

-- ~distance, const, linear, quad
local lightAttenuationTable = {
	{7, 1.0, 0.7,1.8},
	{13,1.0,0.35 ,0.44},
	{20,1.0,0.22 ,0.20},
	{32,1.0,0.14,0.07},
	{50,1.0,0.09,0.032},
	{65,1.0,0.07,0.017},
	{100,1.0,0.045,0.0075},
	{160,1.0,0.027,0.0028},
	{200,1.0,0.022,0.0019},
	{325,1.0,0.014,0.0007},
	{600,1.0,0.007,0.0002},
	{3250,1.0,0.0014,0.000007}
}

function Playground:Vec3Length(v)
	return math.sqrt((v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
end

function Playground:pushLightHistory(i,x,y,z)
	local historySize = #Lights[i].History
	if(historySize == lightHistoryMaxValues) then
		table.remove(Lights[i].History,1)
	end
	Lights[i].History[#Lights[i].History + 1] = {x,y,z}
end

function Playground:GetAttenuation(lightRadius)
	local closestDistance = 10000
	local closestValue = {1,1,0.1,0.1}
	for i=1,#lightAttenuationTable do
		local distanceToRadInTable = math.abs(lightRadius - lightAttenuationTable[i][1])
		if(distanceToRadInTable<closestDistance) then
			closestValue = lightAttenuationTable[i];
			closestDistance = distanceToRadInTable
		end
	end
	return { closestValue[2], closestValue[3], closestValue[4] }
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
	Lights[i].Attenuation = Playground:GetAttenuation(math.random(lightRadiusRange[1],lightRadiusRange[2]))
	Lights[i].History = {}
	Playground:pushLightHistory(i,Lights[i].Position[1], Lights[i].Position[2], Lights[i].Position[3])
end

function Playground:Init()
	local lightColourAccum = {0.0,0.0,0.0}
	for i=1,lightCount do
		Playground:InitLight(i)
	end

	Graphics.SetClearColour(0.1,0.1,0.1)
end

function DrawGrid(startX,endX,stepX,startZ,endZ,stepZ,yAxis)
	for x=startX,endX,stepX do
		Graphics.DebugDrawLine(x,yAxis,startZ,x,yAxis,endZ,0.2,0.2,0.2,0.2,0.2,0.2)
	end
	for z=startZ,endZ,stepZ do
		Graphics.DebugDrawLine(startX,yAxis,z,endX,yAxis,z,0.2,0.2,0.2,0.2,0.2,0.2)
	end
end

local testString = "Yes"
local testSelected = false
local testChecked = true

function Playground:Tick()
	local timeDelta = Playground.DeltaTime * 0.25

	Graphics.DirectionalLight(0.7,-0.4,-0.2, SunMulti*0.25, SunMulti*0.611, SunMulti*1.0, 0.05)

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
	Graphics.DebugDrawAxis(0.0,0.0,0.0,32.0)

	Graphics.DrawModel(0.0,1.0,0.0,1.0,1.0,1.0,1.0,0.15,IslandModel,DiffuseShader)
	Graphics.DrawModel(0.0,1.3,0.0,1.0,1.0,1.0,1.0,1.0,MonsterModel,DiffuseShader)
	Graphics.DrawModel(0.0,0.5,0.0,1.0,1.0,1.0,1.0,0.2,Sponza,DiffuseShader)

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

	local windowOpen = true
	DebugGui.BeginWindow(windowOpen,"Playground")
	DebugGui.Text("Testicles")
	testString = DebugGui.TextInput("More Testes?",testString,32)
	if(DebugGui.Button("A button")) then
		testString = "You only went and clicked it!"
	end
	DebugGui.Separator();
	testSelected = DebugGui.Selectable("Go on, select it",testSelected)
	testChecked = DebugGui.Checkbox("Check that shit", testChecked)
	DebugGui.EndWindow()
end

function Playground:Shutdown()
end

function Playground:new()
	newPlayground = {}
	self.__index = self
	return setmetatable(newPlayground, self)
end
g_playground = Playground:new()
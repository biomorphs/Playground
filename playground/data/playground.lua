-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}
local DragonTexture

function Playground:Init()
	print("Init!")
	DragonTexture = Graphics.LoadTexture("dragons.jpg")
end

r=0
g=0
b=0
a=1

function Playground:Tick()
	r = r + Playground.DeltaTime * 0.4;
	if r > 1.0 then
		r = 0.0
	end
	g = g + Playground.DeltaTime * 0.3;
	if g > 1.0 then
		g = 0.0
	end
	b = b + Playground.DeltaTime * 0.18;
	if b > 1.0 then
		b = 0.0
	end
	Graphics.DrawQuad(0,0,100,100,r,g,b,a)
	for i=0, 720, 100 do 
		Graphics.DrawQuad(i,200,80,80,r,g,b,a)
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
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
g=1
b=0
a=1
y = 0

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

    y = y + Playground.DeltaTime * 200
	if y > 700 then 
		y = 0
	end
	for i=0, 1280, 5 do 
		for yoff = 0, 720, 5 do 
			Graphics.DrawQuad(i,y+yoff,4,4,r,g,b,a)
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
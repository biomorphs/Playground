-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

function Playground:Init()
	print("Init!")
end

function Playground:Tick()
	Graphics.DrawQuad(0,0,100,100,1,0,0,1)
	for i=0, 720, 100 do 
		Graphics.DrawQuad(i,200,80,80,1,0,0,1)
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
-- Playground expects a script to create a global object named g_playground with the following functions:
-- Init
-- Tick
-- Shutdown

Playground = {}

function Playground:Init()
	print("Init!")
end

clr_r = 0
clr_g = 0
clr_b = 0

function Playground:Tick()
	print(Playground.DeltaTime)

	clr_r = clr_r + Playground.DeltaTime
	clr_g = clr_g + Playground.DeltaTime * 0.2
	clr_b = clr_b + Playground.DeltaTime * 0.4
	Graphics.SetClearColour(clr_r, clr_g, clr_b)
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
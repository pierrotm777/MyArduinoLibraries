local timeprev 
local running = false
local channelValue = 0

local function init()
	
end

local function run()
  if not running then
      running = true
	  timeprev = getTime()
  end
  
  local timenow = getTime() -- 10ms tick count
  
  if timenow - timeprev > 50 then -- more than 500 msec since previous run 
	timeprev = timenow
	
	channelValue = 0
	
	--Schalterauswertung
	if (getValue('sa') > 10) then
		channelValue = channelValue + 1 --0x01 00
	end
	if (getValue('sa') < -10) then
		channelValue = channelValue + 2 --0x02 00
	end
	if (getValue('sb') > 10) then
		channelValue = channelValue + 4--0x04 00
	end
	if (getValue('sb') < -10) then
		channelValue = channelValue + 8--0x08 00
	end
	if (getValue('sc') > 10) then
		channelValue = channelValue + 16--0x10 00
	end
	if (getValue('sc') < -10) then
		channelValue = channelValue + 32--0x20 00
	end
	if (getValue('sd') > 10) then
		channelValue = channelValue + 64--0x40 00
	end
	if (getValue('sd') < -10) then
		channelValue = channelValue + 128--0x80 00
	end
	if (getValue('se') > 10) then
		channelValue = channelValue + 256--0x00 01
	end
	if (getValue('se') < -10) then
		channelValue = channelValue + 512--0x00 02
	end
	if (getValue('sg') > 10) then
		channelValue = channelValue + 1024--0x00 04
	end
	if (getValue('sg') < -10) then
		channelValue = channelValue + 2048--0x00 08
	end
	
	--print(channelValue)
	sportTelemetryPush(0x0D, 0x10, 0x01, channelValue)
 
  end
end

return { init=init, run=run }
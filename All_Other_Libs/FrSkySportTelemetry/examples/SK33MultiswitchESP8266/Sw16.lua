local timeprev 
local running = false
local channelValue = 0
local lastPressed = 0
local cpos = 1
local cpress = 0
local status ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}
local lastStatus ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}
-- Kurzbezeichnungen innerhalb der Anzeigefelder:
local nm_s = 
{
 "01", "02",
 "03", "04",
 "05", "06",
 "07", "08",
 "09", "10",
 "11", "12",
 "13", "14",
 "15", "16",
}
-- Langbezeichnungen, wird als Ueberschrift angezeigt
local nm_l =
{
 "Positionslampen",
 "Schlepplicht",
 "Blaulicht",
 "Ankerlicht",
 "05",
 "06",
 "07",
 "Suchscheinwerfer",
 "09",
 "10",
 "11",
 "12",
 "13",
 "14",
 "15",
 "16",
}


local function init()

end

local function background()
   if not running then
      running = true
	  timeprev = getTime()
  end

  local timenow = getTime() -- 10ms tick count
  
  if timenow - timeprev > 20 then -- more than 200 msec since previous run 
	timeprev = timenow
	
	channelValue = 0
	
	--Schalterauswertung
	--if (getValue('sa') > 10) then
	if (status[1]==1) then
		channelValue = channelValue + 1 --0x01 00
	end
	--if (getValue('sa') < -10) then
	if (status[2]==1) then
		channelValue = channelValue + 2 --0x02 00
	end
	--if (getValue('sb') > 10) then
	if (status[3]==1) then
		channelValue = channelValue + 4--0x04 00
	end
	--if (getValue('sb') < -10) then
	if (status[4]==1) then
		channelValue = channelValue + 8--0x08 00
	end
	--if (getValue('sc') > 10) then
	if (status[5]==1) then
		channelValue = channelValue + 16--0x10 00
	end
	--if (getValue('sc') < -10) then
	if (status[6]==1) then
		channelValue = channelValue + 32--0x20 00
	end
 	--if (getValue('sd') > 10) then
	if (status[7]==1) then
		channelValue = channelValue + 64--0x40 00
	end
	--if (getValue('sd') < 10) then
	if (status[8]==1) then
		channelValue = channelValue + 128--0x80 00
	end
	--if (getValue('se') > 10) then
	if (status[9]==1) then
		channelValue = channelValue + 256--0x00 01
	end
	--if (getValue('se') < 10) then
	if (status[10]==1) then
		channelValue = channelValue + 512--0x00 02
	end
	--if (getValue('sg') > 10) then
	if (status[11]==1) then
		channelValue = channelValue + 1024--0x00 04
	end
	--if (getValue('sg') < 10) then
	if (status[12]==1) then
		channelValue = channelValue + 2048--0x00 08
	end
	if (status[13]==1) then
		channelValue = channelValue + 4096--0x00 10
	end
	if (status[14]==1) then
		channelValue = channelValue + 8192--0x00 20
	end
	if (status[15]==1) then
		channelValue = channelValue + 16384--0x00 40
	end
	if (status[16]==1) then
		channelValue = channelValue + 32768--0x00 80
	end

sportTelemetryPush(0x0D, 0x10, 0x01, channelValue)
end

end
local function run(event)
lcd.clear()

  lcd.drawText(50,52,string.format("%05d",channelValue,SMLSIZE))
  
   -- Tasten rechts neben Display - + ENT / Scrollrad bei X9Lite
   if( event == 100) then cpos = (cpos + 1)   -- 100 / EVT_PLUS_FIRST / EVT_ROT_RIGHT / Taste +
   elseif( event == 101) then cpos = (cpos -1)  -- 101 / EVT_MINUS_FIRST / EVT_ROT_LEFT / Taste -
   elseif( event ==  98) then cpress = 1 else cpress = 0 -- 98 / Taste ENT
  end

 if cpos <= 0 then
  cpos = 16
 end  
 if cpos >= 17 then
  cpos = 1
 end 
 
 if ((cpress ==1) and ( lastStatus[cpos] ==0)) then status[cpos]=1 
 end
  if ((cpress ==1) and ( lastStatus[cpos] ==1)) then status[cpos]=0 
 end
 i = 1
 for xp = 0, 7, 1 do
  for yp = 15, 35, 20 do
   lcd.drawRectangle( (xp * 27) + 0, yp, 24, 12) 
   if i == cpos then
    lcd.drawText( (xp * 27) + 3, yp + 3, nm_s[ i], SMLSIZE + INVERS)
   else
    lcd.drawText( (xp * 27) + 3, yp + 3, nm_s[ i], SMLSIZE)
   end
   lastStatus[cpos] = status[cpos]
   lcd.drawText( (xp * 27) + 17, yp + 3, status[ i], SMLSIZE)
   i = i + 1
  end
 end 

--lcd.drawText( 120, 52, cpress, SMLSIZE)
--lcd.drawText( 12, 3,string.format("%02d",cpos, 0))
lcd.drawText( 10, 3, nm_l[cpos], 0)
end

return { init=init, run=run, background=background }
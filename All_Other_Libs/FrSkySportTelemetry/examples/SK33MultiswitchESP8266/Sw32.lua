local timeprev 
local running = false
local channelValue = 0
local lastPressed = 0
local cpos = 1
local cpress = 0
local status ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}
local lastStatus ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}
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
 "17", "18",
 "19", "20",
 "21", "22",
 "23", "24",
 "25", "26",
 "27", "28",
 "29", "30",
 "31", "32",
 "33", "34",
 "35", "36",
 "37", "38",
 "39", "40",
 "41", "42",
 "43", "44",
 "45", "46",
 "47", "48",
 "49", "50",
 "51", "52",
 "53", "54",
 "55", "56",
 "57", "58",
 "59", "60",
 "61", "62",
 "63", "64",
}
-- Langbezeichnungen, wird als Ueberschrift angezeigt
local nm_l =
{
 "01: Positionslampen",
 "02: Schlepplicht",
 "03: Blaulicht",
 "04: Ankerlicht",
 "05: ",
 "06: ",
 "07: ",
 "08: ",
 "09: Suchscheinwerfer",
 "10: ",
 "11: ",
 "12: ",
 "13: ",
 "14: ",
 "15: ",
 "16: ",
 "17: ",
 "18: ",
 "19: ",
 "20: ",
 "21: ", 
 "22: ",
 "23: ",
 "24: ",
 "25: ",
 "26: ",
 "27: ",
 "28: ",
 "29: ",
 "30: ",
 "31: ",
 "32: ",
}


local function init()

end

local function background()
   if not running then
      running = true
	  timeprev = getTime()
  end

  local timenow = getTime() -- 10ms tick count
  
  if timenow - timeprev > 30 then -- more than 300 msec since previous run 
	timeprev = timenow
	
	channelValue = 0
	
	--Schalterauswertung
	--if (getValue('sa') > 10) then  -- SA oben wie in Vorlage von Meinolf
	if (status[1]==1) then			-- GUI
		channelValue = channelValue + 1 --0x01 00
	end
	--if (getValue('sa') < -10) then	--SA unten wie in Vorlage von Meinolf
	if (status[2]==1) then			-- GUI
		channelValue = channelValue + 2 --0x02 00
	end
	if (status[3]==1) then
		channelValue = channelValue + 4--0x04 00
	end
	if (status[4]==1) then
		channelValue = channelValue + 8--0x08 00
	end
	if (status[5]==1) then
		channelValue = channelValue + 16--0x10 00
	end
	if (status[6]==1) then
		channelValue = channelValue + 32--0x20 00
	end
	if (status[7]==1) then
		channelValue = channelValue + 64--0x40 00
	end
	if (status[8]==1) then
		channelValue = channelValue + 128--0x80 00
	end
	if (status[9]==1) then
		channelValue = channelValue + 256--0x00 01
	end
	if (status[10]==1) then
		channelValue = channelValue + 512--0x00 02
	end
	if (status[11]==1) then
		channelValue = channelValue + 1024--0x00 04
	end
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
	if (status[17]==1) then			-- GUI
		channelValue = channelValue + 65536 --1x01 00
	end
	--if (getValue('sa') < -10) then	--SA unten wie in Vorlage von Meinolf
	if (status[18]==1) then			-- GUI
		channelValue = channelValue + 131072 --1x02 00
	end
	if (status[19]==1) then
		channelValue = channelValue + 262144--1x04 00
	end
	if (status[20]==1) then
		channelValue = channelValue + 524288--1x08 00
	end
	if (status[21]==1) then
		channelValue = channelValue + 1048576--1x10 00
	end
	if (status[22]==1) then
		channelValue = channelValue + 2097152--1x20 00
	end
	if (status[23]==1) then
		channelValue = channelValue + 4194304--1x40 00
	end
	if (status[24]==1) then
		channelValue = channelValue + 8388608--1x80 00
	end
	if (status[25]==1) then
		channelValue = channelValue + 16777216--1x00 01
	end
	if (status[26]==1) then
		channelValue = channelValue + 33554432--1x00 02
	end
	if (status[27]==1) then
		channelValue = channelValue + 67108864--1x00 04
	end
	if (status[28]==1) then
		channelValue = channelValue + 134217728--1x00 08
	end
	if (status[29]==1) then
		channelValue = channelValue + 268435456--1x00 10
	end
	if (status[30]==1) then
		channelValue = channelValue + 536870912--1x00 20
	end
	if (status[31]==1) then
		channelValue = channelValue + 1073741824--1x00 40
	end
	if (status[32]==1) then
		channelValue = channelValue + 2147483648--1x00 80
	end
sportTelemetryPush(0x0D, 0x10, 0x01, channelValue)
end

end
local function run(event)
lcd.clear()

  --lcd.drawText(160,3,string.format("%10d",channelValue,SMLSIZE)) -- funktioniert nur bis Integer Format 
  lcd.drawText(160,3,channelValue,SMLSIZE)
   -- Tasten rechts neben Display - + ENT / Scrollrad bei X9Lite
   if( event == EVT_ROT_RIGHT) then cpos = (cpos + 1)   -- 100 / EVT_PLUS_FIRST / EVT_ROT_RIGHT / Taste +
   elseif( event == EVT_ROT_LEFT) then cpos = (cpos -1)  -- 101 / EVT_MINUS_FIRST / EVT_ROT_LEFT / Taste -
   elseif( event ==  98) then cpress = 1 else cpress = 0 -- 98 / EVT_ENTER_FIRST / Taste ENT
  end

 if cpos <= 0 then
  cpos = 32
 end  
 if cpos >= 33 then
  cpos = 1
 end 
 
 if ((cpress ==1) and ( lastStatus[cpos] ==0)) then status[cpos]=1 
 end
  if ((cpress ==1) and ( lastStatus[cpos] ==1)) then status[cpos]=0 
 end
 i = 1
 for xp = 0, 7, 1 do
  for yp = 12, 51, 13 do
   lcd.drawRectangle( (xp * 26) + 3, yp, 24, 12) 
   if i == cpos then
    lcd.drawText( (xp * 26) + 6, yp + 3, nm_s[ i], SMLSIZE + INVERS)
   else
    lcd.drawText( (xp * 26) + 6, yp + 3, nm_s[ i], SMLSIZE)
   end
   lastStatus[cpos] = status[cpos]
   lcd.drawText( (xp * 26) + 20, yp + 3, status[ i], SMLSIZE)
   i = i + 1
  end
 end 

lcd.drawText( 4, 3, nm_l[cpos], 0)
end

return { init=init, run=run, background=background }
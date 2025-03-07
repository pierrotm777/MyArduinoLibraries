 -- Script simuliert die gepulste Ansteuerung für Engel / Momo Sauerland Module
 -- GV wird als Ausgang verwendet und dabei jeweils für EKMFA auf -100%,0%, 100% 
  
	-- Definitionen
local lastAction								-- Zeitpunkt letzte Aktion
local var = {0,0,0,0,0,0,0,0,0,0}				-- Hilfsvariablen Status an Modul übertragen	
local GV = 8									-- GV9 (0=1, 1=2...8=9)
local FP = 0									-- Flugphase 
local running = false							-- Funktionsabschnitt auf 0 setzen
local cpos = 1									-- Cursor Position auf Platz 1 setzen
local cpress = 0								-- Enter auf 0 setzen
local status ={0,0,0,0,0,0,0,0,0,0,0,0}			-- Status Variable auf 0 setzen
local lastStatus ={0,0,0,0,0,0,0,0,0,0}			-- Letzten Status auf 0 setzen
local Counts ={1,2,3,4,5,6,7,8,9,10}			-- Impulse für Funktion
local Max =100									-- Shaltwert Gruppe 1..6
local Min = -100								-- Schaltwert Gruppe 7..12
local values ={Max,Max,Max,Max,Max,Max,Max,Max,Max,Max}
local short = 30								-- Zeitdauer kurze Impulse / Pausenzeit
local long = 100								-- Zeitdauer langer Impuls = Anwahl
local Count = 0									-- Anzahl Impulse / Funktion
local Pulse = 0									-- Hilfsvariable Pulse Aktiv
local value =0									-- Hilfsvariable Impulsrichtung
   -- Kurzbezeichnungen innerhalb der Anzeigefelder:
local nm_s = 
{
 "01 ", "02 ",									-- Kurzbezeichnung sind nur 3 Buchstaben innerhalb der Rechtecke möglich
 "03 ", "04 ",									-- Das 4. Zeichen wird für den Status benötigt
 "05 ", "06 ",
 "07 ", "08 ",
 "09 ", "10 "
}

   -- Langbezeichnungen, wird als Ueberschrift angezeigt
local nm_l =
{
 "1: Standlicht",
 "2: Abblendlicht",
 "3: Fernlicht",
 "4: Nebelscheinwerfer",
 "5: Dachlampen",
 "6: " ,
 "7: Rundumleuchte",
 "8: Warnblinker",
 "09: ",
 "10: "
}

local function processGV(s)							
   if status[s]==1 and var[s] == 0 and Count ==0 then																													
		var[s] = 1;
		Count =Counts[s];
		value = values[s]		
	elseif status[s]==1 and var[s] == 1 and Count ==0 and Pulse ==0 then 								
		var[s] = 2;
	elseif status[s]==0 and var[s] == 2 and Count ==0 then																													
		var[s] = 3;	
		Count =Counts[s]; 
		value = values[s]
	elseif status[s]==0 and var[s] == 3 and Count ==0 and Pulse ==0 then 
		var[s] = 0			
	end	
return nil	
   end

local function init()
model.setGlobalVariable(GV,FP,0);
lastAction = getTime();		
end

local function background()
   if not running then
      running = true								-- Funktionsablauf im Hintergrund starten und laufen lassen
   	timeprev = getTime() 
end


	local timenow = getTime() 						-- 10ms tick count
	local timenowsys = getTime()
	
	if timenowsys - timeprev > 20 then 					-- more than 100 msec since previous run 
	timeprev = timenowsys
	
-- Auswahlauswertung und Zeitliches setzen von GV9
processGV(1);
processGV(2);
processGV(3);	
processGV(4);
processGV(5);
processGV(6);
processGV(7);
processGV(8);
processGV(9);	
processGV(10);

-- Pulse ausgeben
if Count > 0 and Pulse ==0 and (timenow - lastAction > short) then
		model.setGlobalVariable(GV,FP,value);
		lastAction = getTime();	
		Pulse =1
end
-- Pulse zurücksetzen
if Count > 1 and Pulse == 1 and (timenow - lastAction > short) then
		model.setGlobalVariable(GV,FP,0);
		lastAction = getTime();	
		Count = Count-1;
		Pulse = 0
end
-- langer Pulse als Anwahl
if Count == 1 and Pulse == 1 and (timenow - lastAction > long) then
		model.setGlobalVariable(GV,FP,0);
		lastAction = getTime();	
		Count = 0;
		Pulse = 0
end
end
end

local function run(event)
lcd.clear()
-- Tasten rechts neben Display - + ENT / Scrollrad bei X9Lite
   if( event == EVT_ROT_RIGHT) then cpos = (cpos + 1)   					-- 100 / EVT_PLUS_FIRST / EVT_ROT_RIGHT / EVT_VIRTUAL_NEXT / Taste +
   elseif( event == EVT_ROT_LEFT) then cpos = (cpos -1)  					-- 101 / EVT_MINUS_FIRST / EVT_ROT_LEFT / EVT_VIRTUAL_PREV / Taste -
   elseif( event ==  98) and Count ==0 and Pulse ==0 and (model.getGlobalVariable(GV,FP)== 0) then cpress = 1 else cpress = 0 	-- 98 / Taste ENT
  end

-- Cursor Position begrenzen
 if cpos <= 0 then
  cpos = 10										
 end  
 if cpos >= 11 then
  cpos = 1
 end 
 
--Toggeln mit Enter
 if ((cpress ==1) and ( lastStatus[cpos] ==0)) then status[cpos]=1 
 end
  if ((cpress ==1) and ( lastStatus[cpos] ==1)) then status[cpos]=0 
 end

-- Statusanzeige im jeweiligen Feld
 i = 1
 for xp = 0, 4, 1 do										-- Anzahl der anzuzeigenden Felder in X - Pos
  for yp = 15, 30, 14 do
   lcd.drawRectangle( (xp * 25) + 2, yp, 24, 12) 
   if i == cpos then
    lcd.drawText( (xp * 25) + 5, yp + 3, nm_s[ i], SMLSIZE + INVERS)
   else
    lcd.drawText( (xp * 25) + 5, yp + 3, nm_s[ i], SMLSIZE)
   end
   lastStatus[cpos] = status[cpos]
   lcd.drawText( (xp * 25) + 19, yp + 3, status[ i], SMLSIZE)
   i = i + 1
  end
 end 
 
-- Anzeige Langbezeichnungen
lcd.drawText( 1, 3, nm_l[cpos], 0)
-- Anzeige Hilfsvariablen
lcd.drawText(4,58, "Count:  "..Count,  SMLSIZE)
lcd.drawText(68,58, "GV"..(GV+1)..":",  SMLSIZE)				-- ohne Anzeige FP
-- lcd.drawText(68,58, "GV"..(GV+1).."/"..(FP+1)..":",  SMLSIZE)  	-- mit Anzeige FP
lcd.drawText ( lcd.getLastPos()+12, 58, model.getGlobalVariable(GV,FP), SMLSIZE)
end

return { init=init, run=run, background=background }
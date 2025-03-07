-- LUA Script für die Anzeige und Bedienung von 16 Schaltelementen in der Telemetrieanzeige über Drehpoti (x9e, x7 lite), oder +/-Taster (x9d,usw.)
   
   -- Script derzeit für x9e, für andere Sender muss in den Zeile event == für "EVT_ROT_RIGHT" bzw "EVT_ROT_LEFT" ein anderer Wert eingetragen werden
   -- Mögliche Werte stehen neben der Zeile rechts im Kommentar
   
   -- GV1 und GV2 werden als Ausgänge verwendet und dabei jeweils durch -100%, -50%, 50%, 100% geschrieben da es für ein Beier-Soundmodul verwendet wird.
   -- Schaltelement 1 - 4 werden die Werte am GV1 jeweils für ca. 1 Sec. geschaltet
   -- Schaltelement 5 - 8 werden die Werte am GV1 jeweils für ca. 3 Sec. geschaltet
   -- Schaltelement 9 - 12 werden die Werte am GV2 jeweils für ca. 1 Sec. geschaltet
   -- Schaltelement 13 - 16 werden die Werte am GV2 jeweils für ca. 3 Sec. geschaltet
   -- Hintergrund: Das Beier-Modul kann aufgrund der Dauer der jeweiligen Eingänge, pro Prop-Kanal 8 Funktionen schalten
   
   -- Ablage des Scripts auf der SD-Karte unter /Scripts/Telemetry
   -- In Modelleinstellungen auf den Reiter Telemtrie wechseln und am Ende der Seite für Telemtrie Seite 1 das LUA-Script auswählen
   -- Auf dem Sender lange "Page" drücken, dadurch wird das Script aufgerufen


local timer2 = {}								-- Timer Aufruf
local var1 = 0									-- Hilfsvariablen auf 0 setzen
local var2 = 0
local var3 = 0
local var4 = 0
local var5 = 0
local var6 = 0
local var7 = 0
local var8 = 0
local var9 = 0
local var10 = 0
local var11 = 0
local var12 = 0
local var13 = 0
local var14 = 0
local var15 = 0
local var16 = 0								
local running = false								-- Funktionsabschnitt auf 0 setzen
local lastPressed = 0								-- Enter-Zähler auf 0 setzen
local cpos = 1									-- Cursor Position auf Platz 1 setzen
local cpress = 0								-- Enter auf 0 setzen
local status ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}				-- Status Variable auf 0 setzen
local lastStatus ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}				-- Letzten Status auf 0 setzen

   -- Kurzbezeichnungen innerhalb der Anzeigefelder:

local nm_s = 
{
 "Sta", "Abl",									-- Kurzbezeichnung sind nur 3 Buchstaben möglich
 "Fer", "Neb",									-- Das 4. Zeichen wird für den Status benötigt
 "Dac", "Arb",
 "Run", "War",
 "09", "10",
 "11", "12",
 "13", "14",
 "15", "16",
}

   -- Langbezeichnungen, wird als Ueberschrift angezeigt

local nm_l =
{
 "Standlicht",
 "Abblendlicht",
 "Fernlicht",
 "Nebelscheinwerfer",
 "Dachlampen",
 "Arbeitsscheinwerfer",
 "Rundumleuchte",
 "Warnblinker",
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
	
   -- Timerdefinition

	timer2.start = 0							-- Timer Start wenn 0 aufwärts zählen, wenn >0 abwärts zählen
	timer2.value = 0							-- Startwert
	timer2.countdownBeep = 0						-- Countdown Zeichen auf 0 (0=Silent, 1=beep, 2=Voice)
	timer2.minuteBeep = false						-- Minuten Zeichen auf 0
	timer2.persistent = 0							-- Rücksetzen bei Sender aus
	timer2.mode = 1								-- Modus (0=Aus, 1=Zählen)
	

end

local function background()
   if not running then
      running = true								-- Funktionsablauf im Hintergrund starten und laufen lassen
   
	  
  end
	
   -- Auswahlauswertung und Zeitliches setzen von GV1 bzw. GV2

	-- Display Feld 1
	
	if status[1]==1 and var1 == 0 then								-- Status Feld 1 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,-100);							-- GV1 auf -100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var1 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[1]==1 and var1 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var1 = 2		

	elseif status[1]==0 and var1 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,-100);							-- GV1 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var1 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[1]==0 and var1 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var1 = 0										-- Variable 0 setzen
	
	end	

	-- Display Feld 2
	
	if status[2]==1 and var2 == 0 then								-- Status Feld 2 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,-50);							-- GV1 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var2 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[2]==1 and var2 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var2 = 2		

	elseif status[2]==0 and var2 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,-50);							-- GV1 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var2 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[2]==0 and var2 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var2 = 0										-- Variable 0 setzen 
		
	end
	
	-- Display Feld 3
	
	if status[3]==1 and var3 == 0 then								-- Status Feld 3 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,40);							-- GV1 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var3 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[3]==1 and var3 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var3 = 2		

	elseif status[3]==0 and var3 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,40);							-- GV1 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var3 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[3]==0 and var3 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var3 = 0										-- Variable 0 setzen		
	end
	
	-- Display Feld 4
	
	if status[4]==1 and var4 == 0 then								-- Status Feld 4 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,100);							-- GV1 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var4 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[4]==1 and var4 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var4 = 2		

	elseif status[4]==0 and var4 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,100);							-- GV1 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var4 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[4]==0 and var4 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var4 = 0										-- Variable 0 setzen
		
	end
	
	-- Display Feld 5
	
	if status[5]==1 and var5 == 0 then								-- Status Feld 5 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,-100);							-- GV1 auf -100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var5 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[5]==1 and var5 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var5 = 2		

	elseif status[5]==0 and var5 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,-100);							-- GV1 auf -100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var5 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[5]==0 and var5 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var5 = 0										-- Variable 0 setzen
		
	end
	
	-- Display Feld 6
	
	if status[6]==1 and var6 == 0 then								-- Status Feld 6 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,-40);							-- GV1 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var6 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[6]==1 and var6 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var6 = 2		

	elseif status[6]==0 and var6 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,-40);							-- GV1 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var6 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[6]==0 and var6 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var6 = 0										-- Variable 0 setzen	
	end
 	
	-- Display Feld 7
	
	if status[7]==1 and var7 == 0 then								-- Status Feld 7 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,40);							-- GV1 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var7 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[7]==1 and var7 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var7 = 2		

	elseif status[7]==0 and var7 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,40);							-- GV1 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var7 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[7]==0 and var7 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var7 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 8
	
	if status[8]==1 and var8 == 0 then								-- Status Feld 8 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(0,0,100);							-- GV1 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var8 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[8]==1 and var8 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var8 = 2		

	elseif status[8]==0 and var8 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(0,0,100);							-- GV1 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var8 = 2;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[8]==0 and var8 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(0,0,0);								-- GV1 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var8 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 9
	
	if status[9]==1 and var9 == 0 then								-- Status Feld 9 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,-100);							-- GV2 auf -100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var9 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[9]==1 and var9 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var9 = 2		

	elseif status[9]==0 and var9 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,-100);							-- GV2 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var9 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[9]==0 and var9 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var9 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 10
	
	if status[10]==1 and var10 == 0 then								-- Status Feld 10 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,-40);							-- GV2 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var10 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[10]==1 and var10 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var10 = 2		

	elseif status[10]==0 and var10 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,-40);							-- GV2 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var10 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[10]==0 and var10 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var10 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 11
	
	if status[11]==1 and var11 == 0 then								-- Status Feld 11 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,40);							-- GV2 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var11 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[11]==1 and var11 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var11 = 2		

	elseif status[11]==0 and var11 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,40);							-- GV2 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var11 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[11]==0 and var11 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var11 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 12
	
	if status[12]==1 and var12 == 0 then								-- Status Feld 12 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,100);							-- GV2 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var12 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[12]==1 and var12 == 1 and (model.getTimer(2).value == 1) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var12 = 2		

	elseif status[12]==0 and var12 == 2 then								-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,100);							-- GV2 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var12 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[12]==0 and var12 == 3 and (model.getTimer(2).value == 1) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 1 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var12 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 13
	
	if status[13]==1 and var13 == 0 then								-- Status Feld 13 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,-100);							-- GV2 auf -100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var13 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[13]==1 and var13 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var13 = 2		

	elseif status[13]==0 and var13 == 2 then							-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,-100);							-- GV2 auf -100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var13 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[13]==0 and var13 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var13 = 0										-- Variable 0 setzen
	
	end
	
	
	-- Display Feld 14
	
	if status[14]==1 and var14 == 0 then								-- Status Feld 14 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,-40);							-- GV2 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var14 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[14]==1 and var14 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var14 = 2		

	elseif status[14]==0 and var14 == 2 then							-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,-40);							-- GV2 auf -40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var14 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[14]==0 and var14 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var14 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 15
	
	if status[15]==1 and var15 == 0 then								-- Status Feld 15 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,40);							-- GV2 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var15 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[15]==1 and var15 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var15 = 2		

	elseif status[15]==0 and var15 == 2 then							-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,40);							-- GV2 auf 40% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var15 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[15]==0 and var15 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var15 = 0										-- Variable 0 setzen
	
	end
	
	-- Display Feld 16
	
	if status[16]==1 and var16 == 0 then								-- Status Feld 16 auswerten, Hilfsvariable auf 0 abfragen
		model.setGlobalVariable(1,0,100);							-- GV2 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer auswählen und Werte setzen
		timer2.mode = 1;									-- Timer starten
		var16 = 1										-- Hilfsvariable für weitere Bearbeitung setzen
	
	elseif status[16]==1 and var16 == 1 and (model.getTimer(2).value == 3) then 			-- Auswertung Status, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen (model.resetTimer2 machte immer wieder Probleme)
		var16 = 2		

	elseif status[16]==0 and var16 == 2 then							-- Auswertung Status auf 0, Variable
		model.setGlobalVariable(1,0,100);							-- GV2 auf 100% setzen
		model.setTimer(2, timer2);								-- Timer Werte setzen
		timer2.mode = 1;									-- Timer starten
		var16 = 3;										-- Hilfsvariable für weitere Bearbeitung setzen

	elseif status[16]==0 and var16 == 3 and (model.getTimer(2).value == 3) then 			-- Auswertung Status auf0, Variable, Abgelaufene Zeit
		model.setGlobalVariable(1,0,0);								-- GV2 nach 3 Sec. Wieder auf 0% setzen
		model.setTimer(2,{mode=0, start=0, value=0, minuteBeep=false, persistent=0});		-- Timer Rücksetzen
		var16 = 0										-- Variable 0 setzen
	
	end

	

end
local function run(event)
lcd.clear()

  
   -- Tasten rechts neben Display - + ENT / Scrollrad bei X9Lite
   if( event == EVT_ROT_RIGHT) then cpos = (cpos + 1)   					-- 100 / EVT_PLUS_FIRST / EVT_ROT_RIGHT / Taste +
   elseif( event == EVT_ROT_LEFT) then cpos = (cpos -1)  					-- 101 / EVT_MINUS_FIRST / EVT_ROT_LEFT / Taste -
   elseif( event ==  98) then cpress = 1 else cpress = 0 					-- 98 / Taste ENT
  end

   -- Cursor Position

 if cpos <= 0 then
  cpos = 8											-- beschränkt die mögliche Auswahl (in dem Fall 8)
 end  
 if cpos >= 9 then
  cpos = 1
 end 
 
  -- Auswahl mit Enter

 if ((cpress ==1) and ( lastStatus[cpos] ==0)) then status[cpos]=1 
 end
  if ((cpress ==1) and ( lastStatus[cpos] ==1)) then status[cpos]=0 
 end

   -- Statusanzeige im jeweiligen Feld

 i = 1
 for xp = 0, 3, 1 do										-- Anzahl der anzuzeigenden Felder in X - Pos
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

lcd.drawText( 10, 3, nm_l[cpos], 0)
end

return { init=init, run=run, background=background }
-- Lua SwitchBox for Beier SFR-1 widget V0.1 - LIGHT
--
--
-- A Radiomaster TX16S widget for the EdgeTX OS to simulate Light on a USM-RC 3
--
-- Original Author: Dieter Bruse http://bruse-it.com/
-- Modifications by Thomas RUDOLF
--
--
-- This file is part of a free Widgetlibrary.
--
-- Smart Switch is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY, without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, see <http://www.gnu.org/licenses>.
local log_filename = "/LOGS/RCBUTTONRXWidget.txt"
local name = "RCBUTTONRX"
local touchedButton = -1
local BitmapButton = Bitmap.open("/WIDGETS/RCBUTTONRX/PNG/Btn0.png")
local BitmapWidth, BitmapHeight = Bitmap.getSize(BitmapButton)

local options = {
  { "Border", COLOR, WHITE},
  { "SelectBorder", COLOR, RED},
  { "ButtonColor", COLOR, DARKGREEN},
  { "PressedColor", COLOR, GREEN},
  { "TwinChannel", BOOL, 0}
}
local ActiveChannel = 1
local ActiveButtonPage = 1
local LastActiveButton = 0

--[	####################################################################
--[	Valeur de la variable globale de transmission des clés
--[ pour le panneau de commande du Beier SFR-1/SFR-1-HL
--[	####################################################################
local KeyValues = {
Neutral = -20,
  Neutral = -20,
  Taste1 = 860,
  Taste2 = 560,
  Taste3 = 260,
  Taste4 = -320,
  Taste5 = -580,
  Taste6 = -880,
  Taste7 = -1020,
  Taste8 = -760,
  Taste9 = -460,
  Taste10 = 420,
  Taste11 = 700,
Umschaltung = 1020
}

--[	####################################################################
--[	Affectation de la variable globale pour le contrôle actif dans le widget
--[	####################################################################
local GlobalVarialble = {
  {GVarName="GV1",--"GV2",
  Index=0,
  Phase=0},
  {GVarName="GV2",--"GV2",
  Index=1,
  Phase=0}
}

local buttonType = {
  pushbutton = "pushbutton",
  togglebutton = "togglebutton",
  switch = "switch"
}

--[	####################################################################
--[ Configuration des boutons
--[ Contrôle dans le widget (max. 2 pcs.)
--[ -> Pages (max. 2 pièces)
--[ -> Boutons maximum 6 pièces/page
--[	####################################################################
local configTable = {
  {{
{ Name="Radar", FileName="radar.png", Mode=buttonType.pushbutton,ROW=1,COL=1,Value=KeyValues.Valeur01 },
{ Name="Lumière Mât/Rouge/Vert", FileName="mat2.png", Mode=buttonType.pushbutton,ROW=1,COL=2,Value=KeyValues.Valeur02 },
{ Name="Lumière Cabine", FileName="lum1.png", Mode=buttonType.pushbutton,ROW=1,COL=3,Value=KeyValues.Valeur03 },
{ Name="Lumière Extérieur", FileName="lum2.png", Mode=buttonType.pushbutton,ROW=1,COL=4,Value=KeyValues.Valeur04 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=1,COL=5,Value=KeyValues.Valeur05 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=1,COL=6,Value=KeyValues.Valeur06 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=2,COL=1,Value=KeyValues.Valeur07 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=2,COL=2,Value=KeyValues.Valeur08 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=2,COL=3,Value=KeyValues.Valeur09 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=2,COL=4,Value=KeyValues.Valeur10 },
{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=2,COL=5,Value=KeyValues.Valeur11 }
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=2,COL=6,Value=KeyValues.Valeur12 },
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=3,COL=1,Value=KeyValues.Valeur13 },
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=3,COL=2,Value=KeyValues.Valeur14 },
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=3,COL=3,Value=KeyValues.Valeur15 },
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=3,COL=4,Value=KeyValues.Valeur16 },
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=3,COL=5,Value=KeyValues.Valeur17 },
--{ Name="RESERVE", FileName="000.png", Mode=buttonType.pushbutton,ROW=3,COL=6,Value=KeyValues.Valeur18 }
 },
 }}

local buttons = {}

--[	####################################################################
--[	État du bouton. Pressé ou pas.
--[	####################################################################
local buttonState = {
  active = "active",
  inactive = "inactive"
}


--[	####################################################################
--[	write logfile
--[	####################################################################
local function write_log(message, create)
--    local write_mode = "a"
--      if create ~= true then
--        write_mode = "a"
--      else
--        write_mode = "w"
--      end
--			local file = io.open(log_filename, write_mode)
--			io.write(file, message, "\r\n")
--			io.close(file)
end

--[#####################################################################################################
--[ La fonction CallBack est appelée lorsqu'un bouton a été enfoncé
--[#####################################################################################################
local function CallBackFunktion(ImageButton, widget)
  local Value = ImageButton.globalVarValue
  --local t = {}
  --t.startTime = -1;
  --t.durationMili = -1;  
  --local elapsed = getTime() - t.startTime;
  --local elapsedMili = elapsed * 10;
	--log('elapsed: %0.1f/%0.1f sec', elapsed/100, t.durationMili/1000)
  --lcd.drawText(230,250,elapsed/100)
  --if elapsed < 2000 then
  --  lcd.drawText(100,250,"Bouton Pressé:")
  --  lcd.drawText(230,250,ImageButton.ButtonName)
  --end
  if ImageButton.buttonState == buttonState.inactive then
    Value = KeyValues.Neutral
    if ActiveButtonPage == 2 and ImageButton.buttonType ~= buttonType.switch then
      ActiveButtonPage = 1
      LastActiveButton = 0
    end
  end
  write_log("Callback " .. ImageButton.ButtonName .. " State:" .. ImageButton.buttonState .. " Value:" .. Value .. " AktivBtnPg:" .. ActiveButtonPage, false)

  -- Commutation de ligne et Gestion de la minuterie
  if ImageButton.buttonType == buttonType.switch and Value ~= KeyValues.Neutral then
    if ActiveButtonPage == 2 then
      ActiveButtonPage = 1
      LastActiveButton = 0
    else
      ActiveButtonPage = 2
      LastActiveButton = getGlobalTimer()["session"]
    end
    ImageButton.buttonState = buttonState.inactive
  end

  model.setGlobalVariable(GlobalVarialble[ActiveChannel].Index, GlobalVarialble[ActiveChannel].Phase, Value)
  if Value ~= KeyValues.Neutral then
    if ImageButton.buttonType == buttonType.switch then
      playHaptic(20, 10 )
      playHaptic(20, 0 )
    else
      playHaptic(15, 0 )
    end
  end
end

local function doNothing()
end
--[#####################################################################################################
--[ CreateButton crée un Pushbutton ou un ToggleButton
--[#####################################################################################################
local function CreateButton(Position, WidgetPosition, W, H, ButtonName, Image, ButtonType, globalVarValue, CallBack, flags)
  local self = {
    ButtonName = ButtonName,
    Image = Image,
    position = Position or { x = 0, y = 0, center = {}, bitmapScale = 100, radius = 0 },
    widgetposition = WidgetPosition or { x = 0, y = 0, center = {}, bitmapScale = 100, radius = 0 },
    xmax = Position.x + W,
    ymax = Position.y + H,
    w = W,
    h = H,
    callBack = CallBack or doNothing,
    buttonState = buttonState.inactive,
    buttonType = ButtonType or buttonType.pushbutton,
    buttonSelected = false,
    globalVarValue = globalVarValue or KeyValues.Neutral
  }

  function self.draw( event, widget)
    local pos = self.position
    if event == nil then
      pos = self.widgetposition
      if widget.zone.h < 84 then
        lcd.drawText(0,0,"Das Wideget benötigt min. 50% Ansicht in der Hoehe")
        lcd.drawText(0,15,"the widget need min. 50% view in hight")
        lcd.drawText(0,30,"Size " .. widget.zone.w .. "/" .. widget.zone.h)
        return
      end
    end

    if self.buttonSelected then
      lcd.drawFilledCircle(pos.center.x, pos.center.y, pos.radius, widget.options.SelectBorder)
    else
      lcd.drawFilledCircle(pos.center.x, pos.center.y, pos.radius, widget.options.Border)
    end

-- Chiffre = épaisseur de l'anneau des cercles en blanc

    if self.buttonState == buttonState.inactive then
      lcd.drawFilledCircle(pos.center.x, pos.center.y, pos.radius - 2, widget.options.ButtonColor)
    else
      lcd.drawFilledCircle(pos.center.x, pos.center.y, pos.radius - 2, widget.options.PressedColor)
    end
    lcd.drawBitmap(self.Image, pos.x, pos.y, pos.bitmapScale)
  end

  --[#####################################################################################################
  --[ Gestionnaire d'événements pour un bouton
  --[#####################################################################################################
  function self.onEvent(event, touchState, widget)
    write_log("self.OnEvent is entered at " .. self.ButtonName ,false)
    if event == nil then -- Widget mode
      -- Draw in widget mode. The size equals zone.w by zone.h
    else -- Full screen mode
      -- Draw in full screen mode. The size equals LCD_W by 
      if event ~= 0 then -- Do we have an event?
        write_log("self.OnEvent in event id:" .. event,false)
        if touchState then -- Only touch events come with a touchState
          write_log("self.OnEvent in touchState.",false)
          if event == EVT_TOUCH_FIRST then
            if self.buttonType == buttonType.pushbutton or self.buttonType == buttonType.switch then
              self.buttonState = buttonState.active
            elseif self.buttonType == buttonType.togglebutton then
              if self.buttonState == buttonState.active then
                self.buttonState = buttonState.inactive
              else
                self.buttonState = buttonState.active
              end
            end
            self.buttonSelected = true
            return self.callBack(self, widget)
            -- When the finger first hits the screen
          elseif event == EVT_TOUCH_BREAK then
            if self.buttonType == buttonType.pushbutton or self.buttonType == buttonType.switch then
              self.buttonState = buttonState.inactive
            end
            self.buttonSelected = false
            return self.callBack(self, widget)
            -- When the finger leaves the screen and did not slide on it
          elseif event == EVT_TOUCH_TAP then
            if self.buttonType == buttonType.pushbutton or self.buttonType == buttonType.switch then
              self.buttonState = buttonState.inactive
            end
            self.buttonSelected = false
            return self.callBack(self, widget)

            -- A short tap gives TAP instead of BREAK
            -- touchState.tapCount shows number of taps
          end
        else -- event ~= 0 and touchState == nil: key event
          write_log("self.OnEvent in KeyEvents.",false)
          if event == EVT_ENTER_FIRST then
            write_log("self.OnEvent EVT_ENTER_FIRST.",false)
            if self.buttonType == buttonType.pushbutton or self.buttonType == buttonType.switch then
              self.buttonState = buttonState.active
            elseif self.buttonType == buttonType.togglebutton then
              if self.buttonState == buttonState.active then
                self.buttonState = buttonState.inactive
              else
                self.buttonState = buttonState.active
              end
            end
            return self.callBack(self, widget)
          elseif event == EVT_VIRTUAL_ENTER then
            write_log("self.OnEvent EVT_VIRTUAL_ENTER.",false)
            if self.buttonType == buttonType.pushbutton or self.buttonType == buttonType.switch then
              self.buttonState = buttonState.inactive
            end
            return self.callBack(self, widget)
          end
        end
        write_log("End of Eventcheck at self.OnEnvent", false)
      end
      write_log("End of self.OnEvent.", false)
    end
  end

  return self
end -- CreateButton(...)

local function CalculateFullscreen(row, col, widget)
  local Position = { x = 0, y = 0, center = {}, bitmapScale = 100, radius = 0 }
  local WidgetPosition = { x = 0, y = 0, center = {}, bitmapScale = 100, radius = 0 }

  local Seitenrand = 4
  local WidgetSeitenrandX = 6
  local WidgetSeitenrandY = 1

  Position.x = ((Seitenrand + BitmapWidth) * col) - BitmapWidth 
  Position.y = ((Seitenrand + BitmapHeight) * row) - BitmapHeight 
  Position.center = {x = (Position.x + BitmapWidth) - (BitmapWidth / 2), y = (Position.y + BitmapHeight) - (BitmapHeight / 2)}
  Position.radius = BitmapWidth / 2

  WidgetPosition.bitmapScale = math.abs(((widget.zone.h / 2) / BitmapWidth ) * 100)
  local ScaledBitmapHight = BitmapWidth / 100 * WidgetPosition.bitmapScale
  local ScaledBitmapWidht = BitmapHeight / 100 *  WidgetPosition.bitmapScale
  WidgetPosition.x = (WidgetSeitenrandX + ScaledBitmapHight) * col - ScaledBitmapHight +4
  WidgetPosition.y = (WidgetSeitenrandY + ScaledBitmapWidht) * row - ScaledBitmapWidht -2
  WidgetPosition.center = {x = WidgetPosition.x + ScaledBitmapHight - (ScaledBitmapHight / 2)
                         , y = WidgetPosition.y + ScaledBitmapWidht - (ScaledBitmapWidht / 2)}
  WidgetPosition.radius = (BitmapWidth / 100 * WidgetPosition.bitmapScale) / 2

  return Position, WidgetPosition
end

--[#####################################################################################################
--[ LoadConfig crée un bouton à partir d'une structure de données
--[ Le Radiomaster TX16s a une résolution de 480px * 227px,
--[ la somme des images n'est pas vérifiée pour voir si elles tiennent à l'écran. Alors merci de calculer à l'avance :)
--[ Si la hauteur de l'image change, la variable ImageHight doit être définie ci-dessus.
--[ ====================================================================================================
local function LoadConfig(widget)
  write_log("LoadConfig: Bitmap Width:" .. BitmapWidth .. " BitmapHeight:" .. BitmapHeight,true)

  -- Inititalisieren der Globalenn Variable
  model.setGlobalVariable(GlobalVarialble[ActiveChannel].Index, GlobalVarialble[ActiveChannel].Phase, KeyValues.Neutral)

  buttons = {}
  for channel=1, #configTable do
    buttons[channel] = {}
    write_log("LoadConfig: Channel:" .. channel,false)
    for page=1, #configTable[channel] do
      write_log("LoadConfig: Page:" .. page,false)
      buttons[channel][page] = {}
      for idx=1, #configTable[channel][page] do
        local Position,WidgetPosition = CalculateFullscreen(configTable[channel][page][idx].ROW, configTable[channel][page][idx].COL, widget)
        write_log("LoadConfig: Row:" .. configTable[channel][page][idx].ROW .. "Col:" .. configTable[channel][page][idx].COL .. " x:" .. Position.x .. " y:" .. Position.y, false)
        buttons[channel][page][idx] = CreateButton(Position
                                ,WidgetPosition
                                ,BitmapWidth
                                ,BitmapHeight
                                ,configTable[channel][page][idx].Name
                                ,Bitmap.open("/WIDGETS/RCBUTTONRX/PNG/" .. configTable[channel][page][idx].FileName)
                                ,configTable[channel][page][idx].Mode
                                ,configTable[channel][page][idx].Value
                                , CallBackFunktion)
      end
    end
  end
end

--[#####################################################################################################
--[ findTouch Vérifie si un bouton a été enfoncé via le bouton Toucher ou Entrée
--[#####################################################################################################
local function findTouched(event, touchState, widget)
  local result = false

  if event == nil then -- Widget mode
    -- Draw in widget mode. The size equals zone.w by zone.h
  else -- Full screen mode
    if event ~= 0 then
      if touchState then
        for i=1, #buttons[ActiveChannel][ActiveButtonPage] do
          if touchState.x >= buttons[ActiveChannel][ActiveButtonPage][i].position.x
            and touchState.x <= buttons[ActiveChannel][ActiveButtonPage][i].xmax
            and touchState.y >= buttons[ActiveChannel][ActiveButtonPage][i].position.y
            and touchState.y <= buttons[ActiveChannel][ActiveButtonPage][i].ymax then
            result = true
            write_log("findTouched found at [" .. buttons[ActiveChannel][ActiveButtonPage][i].ButtonName .. " Type:" ..buttons[ActiveChannel][ActiveButtonPage][i].buttonType, false)
            buttons[ActiveChannel][ActiveButtonPage][i].onEvent(event, touchState, widget);
            --local timeMs = getTime()
            --lcd.drawText(230,250,timeMs)
            --while (timeMs + 3000 > getTime()) do end
			--lcd.drawText(100,250,"Bouton Pressé:")
            --lcd.drawText(230,250,buttons[ActiveChannel][ActiveButtonPage][i].ButtonName)

          end
        end
      else
        if event == EVT_VIRTUAL_NEXT_PAGE or event == EVT_VIRTUAL_PREV_PAGE then
          if widget.options.TwinChannel == 1 then
            if ActiveChannel == 1 then
              ActiveChannel = 2
            else
              ActiveChannel = 1
            end
            playHaptic(10, 5 )
            playHaptic(10, 0 )
          end
        elseif event == EVT_VIRTUAL_NEXT then
          -- Tout d’abord, annulez le TouchState
          if touchedButton ~= -1 then
            buttons[ActiveChannel][ActiveButtonPage][touchedButton].buttonSelected = false
          end

          if touchedButton == -1 or touchedButton == #buttons[ActiveChannel][ActiveButtonPage] then
            touchedButton = 1
          else
            touchedButton = touchedButton + 1
          end
          buttons[ActiveChannel][ActiveButtonPage][touchedButton].buttonSelected = true
        elseif event == EVT_VIRTUAL_PREV then
          if touchedButton ~= -1 then
            buttons[ActiveChannel][ActiveButtonPage][touchedButton].buttonSelected = false
          end

          if touchedButton == -1 or touchedButton == 1 then
            touchedButton = #buttons[ActiveChannel][ActiveButtonPage]
          else
            touchedButton = touchedButton - 1
          end
          buttons[ActiveChannel][ActiveButtonPage][touchedButton].buttonSelected = true
        elseif event == EVT_VIRTUAL_ENTER or event == EVT_ENTER_FIRST then
          result = true
          if buttons[ActiveChannel][ActiveButtonPage][touchedButton].buttonType == buttonType.switch then
            ActiveButtonPage = 2
            LastActiveButton = getGlobalTimer()["session"]
            write_log("Umschaltung erkannt um:" .. LastActiveButton)
          end
          buttons[ActiveChannel][ActiveButtonPage][touchedButton].onEvent(event, touchState, widget)
        elseif event == EVT_EXIT_FIRST or event == 1540 then
          if touchedButton ~= -1 then
            buttons[ActiveChannel][ActiveButtonPage][touchedButton].buttonSelected = false
          end
        end
      end
    end
    return result
  end
end

local function create(zone, options)
  -- Runs one time when the widget instance is registered
  -- Store zone and options in the widget table for later use
  local widget = {
    zone = zone,
    options = options
  }
  -- Add local variables to the widget table,
  -- unless you want to share with other instances!
  widget.someVariable = 3
  -- Return widget table to EdgeTX
  LoadConfig(widget)
  return widget
end

local function update(widget, options)
  -- Runs if options are changed from the Widget Settings menu
    widget.options = options

end

local function background(widget)
  -- Runs periodically only when widget instance is not visible
end

local function refresh(widget, event, touchState)
  -- Runs periodically only when widget instance is visible
  -- If full screen, then event is 0 or event value, otherwise nil
  findTouched(event, touchState, widget)

  if ActiveButtonPage == 2 and LastActiveButton > 0 then
    if getGlobalTimer()["session"] - LastActiveButton > 10 then
      -- Reset ausführen
      write_log("Automatic Switch-Reset! Inactive Time:" ..  getGlobalTimer()["session"] - LastActiveButton .. "s",false)
      LastActiveButton = 0
      ActiveButtonPage = 1
      playHaptic(20, 10 )
      playHaptic(20, 0 )
    end

  end
  if #buttons > 0 and  #buttons[ActiveChannel] > 0 then
    for i=1, #buttons[ActiveChannel][ActiveButtonPage] do
      buttons[ActiveChannel][ActiveButtonPage][i].draw(event, widget);
    end
  end
end


return {
  name = name,
  options = options,
  create = create,
  update = update,
  refresh = refresh,
  background = background
}
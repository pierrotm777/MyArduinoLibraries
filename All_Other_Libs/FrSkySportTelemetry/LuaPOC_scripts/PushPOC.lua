---- #########################################################################
---- #                                                                       #
---- # Copyright (C) OpenTX                                                  #
-----#                                                                       #
---- # License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html               #
---- #                                                                       #
---- # This program is free software; you can redistribute it and/or modify  #
---- # it under the terms of the GNU General Public License version 2 as     #
---- # published by the Free Software Foundation.                            #
---- #                                                                       #
---- # This program is distributed in the hope that it will be useful        #
---- # but WITHOUT ANY WARRANTY; without even the implied warranty of        #
---- # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
---- # GNU General Public License for more details.                          #
---- #                                                                       #
---- #########################################################################
local readCount = 0
local notNilReads = 0
local lastPhysicalID = 0
local successPush = 0
local loopCount = 0
local lastPushResult = false
local timeOfLastPush = 0

local function updateDisplay()
  lcd.clear()
  lcd.drawNumber(0, 0, loopCount)
  if (lastPushResult) then
    lcd.drawNumber(0, 20, 1)
  else
    lcd.drawNumber(0, 20, 0)
  end
end

local function init()
  timeOfLastPush = getTime()
  updateDisplay()
end

local function run(event)
  if (getTime() - timeOfLastPush > 100) then
    lastPushResult = sportTelemetryPush(2, 0x10, 0x5000, loopCount)
    updateDisplay()
    loopCount = loopCount + 1
    timeOfLastPush = getTime()
  end
  return 0
end

return {init = init, run = run}

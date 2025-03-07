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
local notNilReads = 0
local physicalId, primId, dataId, value = 0
local lastGoodValue = 0

local function updateDisplay()
  lcd.clear()
  lcd.drawNumber(0, 0, lastGoodValue, PREC1)
  lcd.drawNumber(0, 20, notNilReads)
end

local function init()
end

local function run(event)
  physicalId, primId, dataId, value = sportTelemetryPop()
  if physicalId ~= nil then
    notNilReads = notNilReads + 1
    lastGoodValue = value
  end
  updateDisplay()
  return 0
end

return {init = init, run = run}

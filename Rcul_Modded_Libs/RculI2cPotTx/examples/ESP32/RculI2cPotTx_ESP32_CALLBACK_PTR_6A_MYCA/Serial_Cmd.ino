void usbHelp(void)
{
  Serial.println(F("\r\nH    Help"));
  Serial.println(F("?        Read settings"));
  Serial.println(F("d        Debug On/Off"));
  Serial.println(F("scan     Scanner I2C On/Off"));
  Serial.println(F("cal      Start Calibration"));
  Serial.println(F("t        Display Table Calibration"));
  Serial.println(F("r        Use Default Calibration Table"));
  Serial.println(F("e        Erase Table Calibration Stored"));
  Serial.println(F("rn <val> Define Number of Repeats"));
  Serial.println(F("cv <hex> Set contacts manually (0000-FFFF)"));
  Serial.println(F("pv <val> Set proportionnal manually (0-255)"));
  Serial.println(F("q        Quit console mode\r\n"));
}

void readSettings(void)
{
  Serial.println(F("\r\noO Settings Oo\r\n"));
  RculI2cPotTx.printInfo(Serial);
  Serial.printf("Channel      : Use CH%d\r\n", RCUL_CHANNEL);
  Serial.printf("MCP23017     : %s (Add: 0x%02X)\r\n", Mcp23017Connected ? "ON" : "OFF", Mcp23017Connected ? MCP23017_ADDRESS : 0);
  Serial.printf("Proportionnal: %s (Pin: %d)\r\n", ProportionnalConnected ? "ON" : "OFF", PROP_PIN);
  Serial.printf("ContactNb    : %u\r\n", 16);
  Serial.printf("Repeats      : %d\r\n", RCUL_REPEAT);
  Serial.println(F("----------------------------------------"));
  Serial.println(F("Payload      : PROP + SW16 high byte + SW16 low byte"));
  Serial.println(F("Nibbles  send: 6; MLEN=8"));
  Serial.println(F("----------------------------------------"));
}

static String usbLine;

static void processUsbConsole(void)
{
  while (Serial.available()) {
    const char c = (char)Serial.read();

    if (!consoleMode) {
      if (c == '\n') enterConsoleMode();
      continue;
    }

    if (c == '\r') continue;

    if (c == '\n') {
      usbHandleCmd(usbLine);
      usbLine = "";
    } else if (usbLine.length() < 120) {
      usbLine += c;
    }
  }
}

static void usbHandleCmd(String cmd)
{
  cmd.trim();
  if (!cmd.length()) return;

  const int separator = cmd.indexOf(' ');
  String op = separator < 0 ? cmd : cmd.substring(0, separator);
  String rest = separator < 0 ? "" : cmd.substring(separator + 1);
  op.toLowerCase();
  rest.trim();

  if (op == "q") { exitConsoleMode(); return; }
  if (op == "h") { usbHelp(); return; }
  if (op == "?") { readSettings(); return; }

  if (op == "d") {
      DebugIsOn = !DebugIsOn;
      Serial.printf("Debug is %s\r\n", DebugIsOn ? "ON":"OFF");
    return;
  }

  if (op == "scan") {
      ScannerIsOn = !ScannerIsOn;
      if (ScannerIsOn){
        Mcp23017Connected = false;
        ProportionnalConnected = false;
      }
      else {
        Mcp23017Connected = true;
        ProportionnalConnected = true;
      }
      Serial.printf("Scanner I2C is %s\r\n", ScannerIsOn ? "ON":"OFF");
    return;
  }

	if(op == "cal")
	{
	  if(!RculI2cPotTx.isRculTableCalibrationActive())
	  {
      Serial.printf("LEARN: connect receiver PWM to SYNC_LEARN_PIN:%d", SYNC_LEARN_PIN);

      if(ReceiverPwm.attach(PWM_RX_PIN, 700, 2300) < 0)
      {
        Serial.println(F("PWM input ERROR"));
      }
      else
      {
        ReceiverPwmAttached = true;

        RculI2cPotTx.startRculTableCalibration(
          ReceiverPwm,
          &Serial,
          6,
          2,
          4);

        if(!RculI2cPotTx.isRculTableCalibrationActive())
        {
        ReceiverPwm.detach();
        ReceiverPwmAttached = false;
        pinMode(SYNC_LEARN_PIN, INPUT);
        Serial.println(F("LEARN start ERROR"));
        }
      }
	  }
    return;
	}
  else if(op == "t")
  {
    RculI2cPotTx.displayRculTable(Serial);
    return;
  }

  else if(op == "r")
  {
    RculI2cPotTx.useDefaultRculTable();
    Serial.println("Embedded default table selected");
    return;
  }

  else if(op == "e")
  {
    RculI2cPotTx.eraseStoredRculTable();
    Serial.println("Stored table erased");
    return;
  }
	
  if (op == "cv") {
    if (Mcp23017Connected) {
      Serial.print(F("Manual SW unavailable while expander is connected"));
      return;
    }

    Contacts = (uint16_t)strtoul(rest.c_str(), nullptr, 16);
    return;
  }

  if (op == "pv") {
    if (ProportionnalConnected) {
      Serial.print(F("Manual PROP unavailable while expander is connected"));
      return;
    }

    Prop = (uint8_t)atoi(rest.c_str());
    return;
  }

  if (op == "rn") {
    const uint8_t value = (uint8_t)rest.toInt();
    if (value > 6) {
      Serial.print(F("Repeat must be between 0 and 6"));
      return;
    }

    if (RCUL_REPEAT != value)
    {
      RCUL_REPEAT = value;
      preferences.begin("cfgPtr6a", false);
      preferences.putUInt("repeats", RCUL_REPEAT);
      preferences.end();

      MyRcTxSerial.setRepeatNb(RCUL_REPEAT);
      Serial.printf("Repeats=%d\r\n", RCUL_REPEAT);
    }
    return;
  }

  Serial.print(F("[USB] Unknown command. Type H."));
}

static void enterConsoleMode(void)
{
  consoleMode = true;
  Serial.println(F("\r\nConsole mode is Ready (H Help)"));
}

static void exitConsoleMode(void)
{
  consoleMode = false;
  Serial.print(F("\r\nQuit console mode\r\n"));
}


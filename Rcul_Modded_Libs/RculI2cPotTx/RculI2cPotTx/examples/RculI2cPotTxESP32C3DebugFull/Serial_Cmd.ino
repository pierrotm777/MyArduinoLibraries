void usbHelp(void)
{
  Serial.println(F("\r\nH        Help"));
  Serial.println(F("S?       Read settings"));
  Serial.println(F("d0       Debug OFF"));
  Serial.println(F("d1       POT wiper every second"));
  Serial.println(F("d2       Switch/Angl/Prop inputs every second"));
  Serial.println(F("CN <8|16> Set contact count (manual mode only)"));
  Serial.println(F("C  <hex>  Set contacts manually"));
  Serial.println(F("RN <0..6> Set repeat number"));
  Serial.println(F("Q        Quit console mode\r\n"));
}

void readSettings(void)
{
  Serial.print(F("oO Settings Oo"));
  Serial.print(F("SW        : 0x"));
  if (PayloadDesc.ContactNb == 16) {
    if (Contacts < 0x1000) Serial.print('0');
    if (Contacts < 0x0100) Serial.print('0');
  }
  if (Contacts < 0x0010) Serial.print('0');
  Serial.print(Contacts, HEX);
  PRINTF("ContactNb: %u\r\n", PayloadDesc.ContactNb);
  PRINTF("Channel  : %u\r\n", RCUL_CHANNEL);
  PRINTF("Repeat   : %u\r\n", RCUL_REPEAT);
  PRINTF("Nibbles  : %u\r\n\r\n", NibbleNbToSend());
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
  if (op == "s?") { readSettings(); return; }

  if (op == "d" ||
      (op.length() == 2 && op.charAt(0) == 'd' && isDigit(op.charAt(1)))) {
    uint8_t version = op.length() == 2 ? (uint8_t)(op.charAt(1) - '0')
                                       : (uint8_t)rest.toInt();
    if (version > 2) {
      Serial.println(F("Debug accepts d0, d1 or d2"));
      return;
    }

    DbgVersion = version;
    if (DbgVersion > 0) 
      DebugMode = true;
    else
      DebugMode = false;

    if (!DebugMode && oled_ok) {
      u8g2.setFont(u8g2_font_lubBI12_tf);
      u8g2.clearBuffer();
      u8g2.drawStr(0, 30, "Rcul Ok");
      u8g2.sendBuffer();
    }

    PRINTF("Debug is d%u %s\r\n", DbgVersion, DebugMode ? "ON" : "OFF");
    return;
  }

  if (op == "cn") {
    const uint8_t value = (uint8_t)rest.toInt();
    if (value != 8 && value != 16) {
      Serial.println(F("Contact number accepts only 8 or 16"));
      return;
    }
    if (inputExpanderReady) {
      Serial.println(F("Contact number is fixed by the connected expander"));
      return;
    }

    PayloadDesc.ContactNb = value;
    PRINTF("ContactNb=%u\r\n", PayloadDesc.ContactNb);
    return;
  }

  if (op == "c") {
    if (inputExpanderReady) {
      Serial.print(F("Manual contacts unavailable while expander is connected"));
      return;
    }

    Contacts = (uint16_t)strtoul(rest.c_str(), nullptr, 16);
    if (PayloadDesc.ContactNb == 8) Contacts &= 0x00FFU;
    return;
  }

  if (op == "rn") {
    const uint8_t value = (uint8_t)rest.toInt();
    if (value > 6) {
      Serial.print(F("Repeat must be between 0 and 6"));
      return;
    }

    RCUL_REPEAT = value;
    MyRcTxSerial.setRepeatNb(RCUL_REPEAT);
    PRINTF("Repeat=%u\r\n", RCUL_REPEAT);
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

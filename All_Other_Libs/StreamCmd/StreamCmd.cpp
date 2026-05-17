/*
 English: by RC Navy (2025)
 =======
 <StreamCmd> is a library to tokenize and parse commands received over a channel inheriting of the Stream class.
 It can be a channel of serial port type, IP socket type, I2C type, or any channel inheriting of the Stream class.
 This library is based on https://github.com/kroimon/Arduino-SerialCommand, but offering more features.

 http://p.loussouarn.free.fr
 V1.0: initial release

 Francais: par RC Navy (2025)
 ========
 <StreamCmd> est une bibliotheque pour diviser et anyser des commandes recues via un canal heritant de la classe Stream.
 Cela peut être un canal de type port serie, de type socket IP, de type I2C, ou tout autre canal heritant de la classe Stream.
 Cette bibliotheque est basee sur https://github.com/kroimon/Arduino-SerialCommand, mais offre davantage de fonctionnalites.

 http://p.loussouarn.free.fr
 V1.0: release initiale
*/
#include "StreamCmd.h"

/**
 * Constructor makes sure some things are set.
 */
StreamCmd::StreamCmd(Stream *stream, char sep, const char *term)
  : cmdNb(0),
    #if (STREAM_CMD_MODE == STREAM_CMD_ADD_MODE)
    cmdTbl(NULL),
    #endif
    defaultHandler(NULL),
    last(NULL)
{
  myStream = stream;
  lineTerm[0] = term[0];
  lineTerm[1] = term[1];
  lineTerm[2] = 0;     // End of string
  strcpy(sepStr, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}

#if (STREAM_CMD_MODE == STREAM_CMD_TBL_MODE)
/**
 * Pass pointer to the Command Table and the table item number (Use the STREAM_CMD_TBL_AND_ITEM_NB() macro).
 */
void StreamCmd::addCmd(const CmdSt_t *CmdTbl, uint8_t CmdNb)
{
  cmdTbl = CmdTbl;
  cmdNb  = CmdNb;
  #ifdef STREAM_CMD_DEBUG
  myStream->print("cmdNb=");myStream->print(cmdNb);myStream->print(lineTerm);
  #endif
}
#endif

#if (STREAM_CMD_MODE == STREAM_CMD_ADD_MODE)
/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void StreamCmd::addCmd(const char *command, void (*function)())
{
  #ifdef STREAM_CMD_DEBUG
  myStream->print("Adding command (");myStream->print(cmdNb);myStream->print("): ");myStream->print(command);myStream->print(lineTerm);
  #endif
  cmdTbl = (StreamCmdCallback *) realloc(cmdTbl, (cmdNb + 1) * sizeof(StreamCmdCallback));
  strncpy(cmdTbl[cmdNb].Cmd, command, STREAM_CMD_MAX_CMD_LEN);
  cmdTbl[cmdNb].CmdFunction = function;
  cmdNb++;
}
#endif

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void StreamCmd::setDefaultHandler(void (*function)(const char *))
{
  defaultHandler = function;
}

/**
 * This checks the stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void StreamCmd::loop()
{
  while (myStream->available() > 0)
  {
    char inChar = myStream->read();   // Read single available character, there may be more waiting
    #ifdef STREAM_CMD_DEBUG
      myStream->print(inChar);   // Echo back to serial stream
    #endif
    if (inChar == lineTerm[1]) continue;
    if (inChar == lineTerm[0]) // Check for the terminator (default '\r') meaning end of command
    {
      #ifdef STREAM_CMD_DEBUG
      myStream->print(lineTerm);myStream->print("Received: ");myStream->print(buffer);myStream->print(lineTerm);
      #endif

      char *command = strtok_r(buffer, sepStr, &last);   // Search for command at start of buffer
      if (command != NULL)
      {
        boolean matched = false;
        for (int i = 0; i < cmdNb; i++)
        {
          #ifdef STREAM_CMD_DEBUG
          myStream->print("Comparing [");myStream->print(command);myStream->print("] to [");myStream->print(cmdTbl[i].Cmd);myStream->print("]");myStream->print(lineTerm);
          #endif
          if (!strncmp(command, cmdTbl[i].Cmd, STREAM_CMD_MAX_CMD_LEN)) // Compare the found command against the list of known commands for a match
          {
            #ifdef STREAM_CMD_DEBUG
            myStream->print("Matched Command: ");myStream->print(command);myStream->print(lineTerm);
            #endif
            (*cmdTbl[i].CmdFunction)(); // Execute the stored handler function for the command
            matched = true;
            break;
          }
        }
        if (!matched && (defaultHandler != NULL))
        {
          (*defaultHandler)(command);
        }
      }
      clearBuffer();
    }
    else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (bufPos < STREAM_CMD_BUFFER_LEN)
      {
        buffer[bufPos++] = inChar;  // Put character into buffer
        buffer[bufPos] = '\0';      // Null terminate
      }
      else
      {
        #ifdef STREAM_CMD_DEBUG
        myStream->print("Line buffer is full - increase STREAM_CMD_BUFFER_LEN");myStream->print(lineTerm);
        #endif
      }
    }
  }
}

#if (STREAM_CMD_MODE == STREAM_CMD_TBL_MODE)
/*
 * Help display (only available in TBL mode.
 */
void StreamCmd::displayHelp(uint8_t ColonPos)
{
  uint8_t    BufLen = ColonPos + 4;
  char       Buf[BufLen];
  const void (*DescriptionFunction)(void);
  uint8_t    Len, Display = 0;

  for(uint8_t Idx = 0; Idx < cmdNb; Idx++)
  {
    if(!cmdTbl[Idx].DisplayCond) Display = 1;
    else                         Display = cmdTbl[Idx].DisplayCond();
    if(Display)
    {
      memset(Buf, ' ', BufLen);
      snprintf(Buf, BufLen - 2, "%s %s", cmdTbl[Idx].Cmd, cmdTbl[Idx].Args);
      Len = strlen(Buf);
      if(Len < BufLen) memset(Buf + Len, ' ', BufLen - Len);
      Buf[ColonPos + 0] = ':';
      Buf[ColonPos + 2] = 0;
      myStream->print(Buf);
      if(IS_DFP(cmdTbl[Idx].Description))
      {
        DescriptionFunction = GET_DFP(cmdTbl[Idx].Description);
        DescriptionFunction();
      }else myStream->print(cmdTbl[Idx].Description);
      myStream->print(lineTerm);
    } 
  }
}
#endif

/*
 * Clear the input buffer.
 */
void StreamCmd::clearBuffer()
{
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *StreamCmd::next()
{
  return strtok_r(NULL, sepStr, &last);
}

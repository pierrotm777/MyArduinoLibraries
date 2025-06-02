/*
 English: by RC Navy (2025)
 =======
 <StreamCmd> is a library to tokenize and parse commands received over a channel inheriting of the Stream class.
 It can be a channel of serial port type, IP socket type, I2C type, or any channel inheriting of the Stream class.
 This library is based on https://github.com/kroimon/Arduino-SerialCommand but offering more features.

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

#ifndef STREAM_CMD_H
#define STREAM_CMD_H

#include <Arduino.h>
#include <string.h>
#include <Stream.h>

/* Library Version/Revision */
#define STREAM_CMD_VERSION      1
#define STREAM_CMD_REVISION     0

/* vvv Do NOT change lines below vvv */
#define STREAM_CMD_ADD_MODE     0 // In this mode, you have to use addCmd() to add commands and associated callback
#define STREAM_CMD_TBL_MODE     1 // In this mode, you have to declare all the commands/callbacks in the STREAM_CMD_DELARE_TBL() table
/* ^^^ Do NOT change lines above ^^^ */

/* vvv Library configuration vvv */
#define STREAM_CMD_MODE         STREAM_CMD_TBL_MODE // <-- Choose here between STREAM_CMD_ADD_MODE and STREAM_CMD_TBL_MODE
#define STREAM_CMD_BUFFER_LEN   32 // Size of the input buffer in bytes (maximum length of one command plus arguments)
#define STREAM_CMD_MAX_CMD_LEN  20 // Maximum length of a command excluding the terminating null
//#define STREAM_CMD_DEBUG           // Uncomment the next line to run the library in debug mode (verbose messages)
/* ^^^ Library configuration ^^^ */

#if (STREAM_CMD_MODE == STREAM_CMD_TBL_MODE)
/* Tip: in the following CmdSt_t structure, *Decription may be either a pointer on a Description string or a pointer on a Decription Function */
#ifdef __AVR__
#warning STREAM_CMD_TBL_MODE not advised in StreamCmd.h for AVR targets since the RAM is very limited!
#define FPM                   0x8000       /* Function Pointer Marker : when the 31th bit is set, it means that its a pointer on a function rather than in a pointer on char */
#define DFP(DescFunct)        (const char*)(FPM | (uint16_t)(DescFunct)) /* Define Function Pointer on a Description Function */
#define IS_DFP(Description)   (FPM & (uint16_t)(Description))            /* Check if *Description is a Pointer on Decription Function */
#define GET_DFP(Description)  (const void (*)(void))((uint16_t)(Description) & (~FPM))
#else
#define FPM                   0x80000000UL /* Function Pointer Marker : when the 31th bit is set, it means that its a pointer on a function rather than in a pointer on char */
#define DFP(DescFunct)        (const char*)(FPM | (uint32_t)(DescFunct)) /* Define Function Pointer on a Description Function */
#define IS_DFP(Description)   (FPM & (uint32_t)(Description))            /* Check if *Description is a Pointer on Decription Function */
#define GET_DFP(Description)  (const void (*)(void))((uint32_t)(Description) & (~FPM))
#endif

#define STREAM_CMD_DELARE_TBL(Tbl)       const CmdSt_t  Tbl[]              // Use this macro to declare your command table
#define STREAM_CMD_TBL_AND_ITEM_NB(Tbl)  Tbl, sizeof(Tbl) / sizeof(Tbl[0]) // Use this macro to pass arguments to addCommand()

typedef struct{
  const char    *Cmd;
  const char    *Args;
  void         (*CmdFunction)(void);
  const char    *Description; /* Pointer on String or Pointer on Description Function */
  uint8_t      (*DisplayCond)(void);
}CmdSt_t;
#endif

class StreamCmd {
  public:
    StreamCmd(Stream *stream, char sep, const char *term);  // Constructor
    #if (STREAM_CMD_MODE == STREAM_CMD_TBL_MODE)
    void addCmd(const CmdSt_t *CmdTbl, uint8_t CmdNb);
    void displayHelp(uint8_t ColonPos);
    #endif
    #if (STREAM_CMD_MODE == STREAM_CMD_ADD_MODE)
    void addCmd(const char *command, void(*function)());    // Add a command to the processing dictionary.
    #endif
    void setDefaultHandler(void (*function)(const char *)); // A handler to call when no valid command received.

    void loop();          // Main entry point.
    void clearBuffer();   // Clears the input buffer.
    char *next();         // Returns pointer to next token found in command buffer (for getting arguments to commands).

  private:
    uint8_t cmdNb;
    #if (STREAM_CMD_MODE == STREAM_CMD_ADD_MODE)
    // Command/handler dictionary
    struct StreamCmdCallback {
      char Cmd[STREAM_CMD_MAX_CMD_LEN + 1];
      void (*CmdFunction)();
    };                            // Data structure to hold Command/Handler function key-value pairs
    StreamCmdCallback *cmdTbl;   // Actual definition for command/handler array
    #endif

    // Pointer to the default handler function
    void (*defaultHandler)(const char *);

    char sepStr[2]; // null-terminated list of character to be used as delimeters for tokenizing (default " ")
    char term[3];   // Character that signals end of command (default '\n')

    char buffer[STREAM_CMD_BUFFER_LEN + 1]; // Buffer of stored characters while waiting for terminator character
    uint8_t bufPos;                         // Current position in the buffer
    char *last;                             // State variable used by strtok_r during processing
    #if (STREAM_CMD_MODE == STREAM_CMD_TBL_MODE)
    const CmdSt_t *cmdTbl;
    #endif
    Stream *myStream;
    char lineTerm[3];
};

#endif //STREAM_CMD_H

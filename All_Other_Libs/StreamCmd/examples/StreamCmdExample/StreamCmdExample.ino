// Demo Code for StreamCmd Library
// RC Navy
// May 2025

#include <StreamCmd.h>

/* StreamCmd library configuration check */
#if (STREAM_CMD_MODE != STREAM_CMD_ADD_MODE)
#error Set '#define STREAM_CMD_MODE  STREAM_CMD_ADD_MODE' in StreamCmd.h!
#endif

#define arduinoLED 13     // Arduino LED on board

#define myStream   Serial // Define here what stream you want to use (here: Serial), you can change it to Serial1, etc

static StreamCmd sCmd(&myStream, ' ', (char *)"\r\n"); // The demo StreamCmd object, Args: Stream, Separator characyer, Line Terminator String

void setup()
{
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // Default to LED off

  myStream.begin(115200); // A decent baud rate...

  // Setup callbacks for StreamCmd commands
  sCmd.addCmd("ON",    LED_on);          // Turns LED on
  sCmd.addCmd("OFF",   LED_off);         // Turns LED off
  sCmd.addCmd("HELLO", sayHello);        // Echos the string argument back
  sCmd.addCmd("P",     processCommand);  // Converts two arguments to integers and echos them back
  sCmd.setDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?")
  myStream.println("Ready");
}

void loop()
{
  sCmd.loop();     // We don't do much, just process stream commands
}


void LED_on()
{
  myStream.println("LED on");
  digitalWrite(arduinoLED, HIGH);
}

void LED_off()
{
  myStream.println("LED off");
  digitalWrite(arduinoLED, LOW);
}

void sayHello()
{
  char *arg;
  arg = sCmd.next();    // Get the next argument from the StreamCmd object buffer
  if (arg != NULL)
  {    // As long as it existed, take it
    myStream.print("Hello ");
    myStream.println(arg);
  }
  else
  {
    myStream.println("Hello, whoever you are");
  }
}


void processCommand()
{
  int aNumber;
  char *arg;

  myStream.println("We're in processCommand");
  arg = sCmd.next();
  if (arg != NULL)
  {
    aNumber = atoi(arg);    // Converts a char string to an integer
    myStream.print("First argument was: ");
    myStream.println(aNumber);
  }
  else {
    myStream.println("No arguments");
  }

  arg = sCmd.next();
  if (arg != NULL)
  {
    aNumber = atol(arg);
    myStream.print("Second argument was: ");
    myStream.println(aNumber);
  }
  else
  {
    myStream.println("No second argument");
  }
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command)
{
  myStream.println("What?");
}

/*Program to send Latitude and Logitute Information from SIM800 to Phone via SMS on call request
 * Code by: B.Aswinth Raj
 * For: www.circuitdigest.com
 * Dated:23-04-2019
 * Sample Output for AT+CIPGSMLOC=1,1   ==> +CIPGSMLOC: 0,75.802460,26.848892,2019/04/23,08:32:35 //where 26.8488832 is Lattitude and 75.802460 is longitute 
 * Link to send: https://www.google.com/maps/place/26.8488892,75.802460&nbsp; //where 26.8488832 is Lattitude and 75.802460 is longitute 
 */

#include <SoftwareSerial.h> //Software Serial header to communicate with GSM module 
SoftwareSerial SIM800(10, 11); // RX, TX 

String Link = "The current Location is https://www.google.com/maps/place/"; //we will append the Lattitude and longitude value later int the program 
String responce = "";
String Longitude = "";
String Latitude = ""; 

String SIM800_send(String incoming) //Function to communicate with SIM800 module 
{
    SIM800.println(incoming); delay(100); //Print what is being sent to GSM module 
    String result = "";
    while (SIM800.available()) //Wait for result 
    {
    char letter = SIM800.read();
    result = result + String(letter); //combine char to string to get result 
    }

return result; //return the result 
}

void setup() {
//PWRKY pin of GSM module has to be pulled low for 1sec to enable the module 
  pinMode(12,OUTPUT);
  digitalWrite(12, LOW);   //Pull-down
  delay(1000);                      
  digitalWrite(12, HIGH); //Release 

  Serial.begin(9600); //Serial COM for debugging 
  SIM800.begin(9600); //Software serial called SIM800 to speak with SIM800 Module 
  delay(1000); //wait for serial COM to get ready 
  responce = SIM800_send("ATE1"); //Enable Echo if not enabled by default 
  Serial.print ("Responce:"); Serial.println(responce); 
  delay(1000);
  responce = SIM800_send("AT+CGATT=1"); //Set the SIM800 in GPRS mode 
  Serial.print ("Responce:"); Serial.println(responce); 
  delay(1000);
  responce = SIM800_send("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\" "); //Activate Bearer profile 
  Serial.print ("Responce:"); Serial.println(responce); 
  delay(1000);
  responce = SIM800_send("AT+SAPBR=3,1,\"APN\",\"RCMNET\" "); //Set VPN options => 'RCMNET' 'www'
  Serial.print ("Responce:"); Serial.println(responce); 
  delay(2000);
  
  responce = SIM800_send("AT+SAPBR=1,1"); //Open bearer Profile
  Serial.print ("Responce:"); Serial.println(responce); //Open bearer Profile 
  delay(2000);
  responce = SIM800_send("AT+SAPBR=2,1"); //Get the IP address of the bearer profile 
  Serial.print ("Responce:"); Serial.println(responce); 
  delay(1000);
}

void prepare_message()
{
  //Sample Output for AT+CIPGSMLOC=1,1   ==> +CIPGSMLOC: 0,75.802460,26.848892,2019/04/23,08:32:35 //where 26.8488832 is Lattitude and 75.802460 is longitute 
  int first_comma = responce.indexOf(','); //Find the position of 1st comma 
  int second_comma = responce.indexOf(',', first_comma+1); //Find the position of 2nd comma 
  int third_comma = responce.indexOf(',', second_comma+1); //Find the position of 3rd comma 
  for(int i=first_comma+1; i<second_comma; i++) //Values form 1st comma to 2nd comma is Longitude 
    Longitude = Longitude + responce.charAt(i); 
  for(int i=second_comma+1; i<third_comma; i++) //Values form 2nd comma to 3rd comma is Latitude 
    Latitude = Latitude + responce.charAt(i); 
  Serial.println(Latitude); Serial.println(Longitude); 
  Link = Link + Latitude + "," + Longitude; //Update the Link with latitude and Logitude values 
  Serial.println(Link);
}

String incoming = "";
void loop() {
  if (SIM800.available()) { //Check if the SIM800 Module is telling anything 
    char a = SIM800.read();
    Serial.write(a); //print what the module tells on serial monitor 
    incoming = incoming + String(a);
    if (a == 13) //check for new line 
    incoming =""; //clear the string if new line is detected 
    incoming.trim(); //Remove /n or /r from the incomind data 
  
    if (incoming=="RING") //If an incoming call is detected the SIM800 module will say "RING" check for it 
    {
     Serial.println ("Sending sms"); delay(1000);
     responce = SIM800_send("ATH"); //Hand up the incoming call using ATH
     delay (1000);
     responce = SIM800_send("ATE0"); //Disable Echo
     delay (1000);
     responce = ""; Latitude=""; Longitude=""; //initialise all string to null
     SIM800.println("AT+CIPGSMLOC=1,1"); delay(5000); //Request for location data 
      while (SIM800.available()) 
      {
       char letter = SIM800.read();
       responce = responce + String(letter); //Store the location information in string responce 
      }
      Serial.print("Result Obtained as:");   Serial.print(responce); Serial.println("*******");
     prepare_message(); delay(1000); //use prepare_message funtion to prepare the link with the obtained LAT and LONG co-ordinates 
     SIM800.println("AT+CMGF=1"); //Set the module in SMS mode
     delay(1000);
     
     SIM800.println("AT+CMGS=\"9612345678\""); //Send SMS to this number 
     delay(1000);
     SIM800.println(Link); // we have send the string in variable Link 
     delay(1000);
     SIM800.println((char)26);// ASCII code of CTRL+Z - used to terminate the text message 
     delay(1000);
    }
  }

  if (Serial.available()) { //For debugging 
    SIM800.write(Serial.read());
  }

}

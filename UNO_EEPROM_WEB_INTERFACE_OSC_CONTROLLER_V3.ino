

/*
This an Embedded Network Setup Webpage.

When you use the default ip-address in a webbrowser
You'll see a simple text like:
IT WORKS: go to "default-IP"/setup

This default webpage can be replaced by your own

if you go to "default-IP"/setup in a webbrowser
You'll get a network setup page.
It will pressent you the current setup filled out

When you press SUBMIT, the new values will be
saved to EEPROM (and thus still available on RESET).
Now you'll have to RESET your Arduino your self
and it will be available at your submitted IP.


You will need the arduino ethernet shield
and some libraries
 
made by JO3RI www.JO3RI.be/arduino
Modified by sydney, reads state of buttonPin, if brought high,
sends OSC command to Companion setup. You can set Page/Bank using
setup as mentioned above.
*/
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <EEPROM.h>
#include <TextFinder.h>
#include <SPI.h>    
#include <OSCMessage.h>

byte          mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte          ip[] = {192,168,1,117};
byte          subnet[] = {255,255,255,0};
byte          gateway[] = {192,168,1,1};
byte          companionip[] = {192,168,1,88};
byte          oscpath[] = {1,3};
char*         oscaddr[] = {"/press/bank/","/"};
const int     companionPort = 12321;
char          oscCommand[30];
const int     buttonPin = 2;
int           buttonState = 0;
unsigned long CurrentMillis;
unsigned long LastMillis;
int           debounceTime = 800;

EthernetServer server(80);
EthernetUDP Udp;

// This is our buffer through which we will will "flow" our HTML code.
// It has to be as big as the longest character chain +1 including the "
char buffer[100];

// This is the HTML code all chopped up. The best way to do this is, is by typing
// your HTML code in an editor, counting your characters and divide them by 8.
// you can chop your HTML on every place, but not on the \" parts. So remember,
// you have to use \" instead of simple " within the HTML, or it will not work.

const char htmlx0[] PROGMEM = "<!DOCTYPE html> <title>Arduino Configuration Page</title>";
const char htmlx1[] PROGMEM = " <body style=\"margin: 5px; padding: 5px; background-color:lightgrey;\"><table ";
const char htmlx2[] PROGMEM = "style=\"font-family:Verdana;color:lightgrey; background-color:grey; color:white;";
const char htmlx3[] PROGMEM = "font-size:20px;padding:10px;min-width:50%\"><tr><td>Arduino Configuration Page</td></tr></table><br>";
PROGMEM const char * const string_table0[] = {htmlx0, htmlx1, htmlx2, htmlx3};

const char htmla0[] PROGMEM = "<script>function hex2num (s_hex) {eval(\"var n_num=0X\" + s_hex);return n_num;}";
const char htmla1[] PROGMEM = "</script><table><form><input type=\"hidden\" name=\"SBM\" value=\"1\"><tr><td>MAC:";
const char htmla2[] PROGMEM = "<input id=\"T1\" type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT1\" value=\"";
const char htmla3[] PROGMEM = "\">.<input id=\"T3\" type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT2\" value=\"";
const char htmla4[] PROGMEM = "\">.<input id=\"T5\" type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT3\" value=\"";
const char htmla5[] PROGMEM = "\">.<input id=\"T7\" type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT4\" value=\"";
const char htmla6[] PROGMEM = "\">.<input id=\"T9\" type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT5\" value=\"";
const char htmla7[] PROGMEM = "\">.<input id=\"T11\" type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT6\" value=\"";
PROGMEM const char * const string_table1[] = {htmla0, htmla1, htmla2, htmla3, htmla4, htmla5, htmla6, htmla7};

const char htmlb0[] PROGMEM = "\"><input id=\"T2\" type=\"hidden\" name=\"DT1\"><input id=\"T4\" type=\"hidden\" name=\"DT2";
const char htmlb1[] PROGMEM = "\"><input id=\"T6\" type=\"hidden\" name=\"DT3\"><input id=\"T8\" type=\"hidden\" name=\"DT4";
const char htmlb2[] PROGMEM = "\"><input id=\"T10\" type=\"hidden\" name=\"DT5\"><input id=\"T12\" type=\"hidden\" name=\"D";
const char htmlb3[] PROGMEM = "T6\"></td></tr><tr><td>IP: <input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT7\" value=\"";
const char htmlb4[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT8\" value=\"";
const char htmlb5[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT9\" value=\"";
const char htmlb6[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT10\" value=\"";
PROGMEM const char * const string_table2[] = {htmlb0, htmlb1, htmlb2, htmlb3, htmlb4, htmlb5, htmlb6};

const char htmlc0[] PROGMEM = "\"></td></tr><tr><td>Subnet Mask: <input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT11\" value=\"";
const char htmlc1[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT12\" value=\"";
const char htmlc2[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT13\" value=\"";
const char htmlc3[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT14\" value=\"";
PROGMEM const char * const string_table3[] = {htmlc0, htmlc1, htmlc2, htmlc3};

const char htmld0[] PROGMEM = "\"></td></tr><tr><td>Gateway: <input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT15\" value=\"";
const char htmld1[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT16\" value=\"";
const char htmld2[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT17\" value=\"";
const char htmld3[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT18\" value=\"";
const char htmld4[] PROGMEM = "\"></td></tr><tr><td><br></td></tr><tr><td><input id=\"button1\" type=\"submit\" value=\"SUBMIT\" ";
const char htmld5[] PROGMEM = "></td></tr></form></table></body></html>";
PROGMEM const char * const string_table4[] = {htmld0, htmld1, htmld2, htmld3, htmld4, htmld5};

const char htmle0[] PROGMEM = "Onclick=\"document.getElementById('T2').value ";
const char htmle1[] PROGMEM = "= hex2num(document.getElementById('T1').value);";
const char htmle2[] PROGMEM = "document.getElementById('T4').value = hex2num(document.getElementById('T3').value);";
const char htmle3[] PROGMEM = "document.getElementById('T6').value = hex2num(document.getElementById('T5').value);";
const char htmle4[] PROGMEM = "document.getElementById('T8').value = hex2num(document.getElementById('T7').value);";
const char htmle5[] PROGMEM = "document.getElementById('T10').value = hex2num(document.getElementById('T9').value);";
const char htmle6[] PROGMEM = "document.getElementById('T12').value = hex2num(document.getElementById('T11').value);\"";
PROGMEM const char * const string_table5[] = {htmle0, htmle1, htmle2, htmle3, htmle4, htmle5, htmle6};

const char htmlf0[] PROGMEM = "\"><br></td></tr><tr><td> Companion Page: <input type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT19\" value=\"";
const char htmlf1[] PROGMEM = "\"><br> Companion Button: <input type=\"text\" size=\"2\" maxlength=\"2\" name=\"DT20\" value=\"";
const char htmlf2[] PROGMEM = "\"><br> Companion IP: <input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT21\" value=\"";
const char htmlf3[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT22\" value=\"";
const char htmlf4[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT23\" value=\"";
const char htmlf5[] PROGMEM = "\">.<input type=\"text\" size=\"3\" maxlength=\"3\" name=\"DT24\" value=\"";
PROGMEM const char * const string_table6[] = {htmlf0, htmlf1, htmlf2, htmlf3, htmlf4, htmlf5};

const byte ID = 0x92; //used to identify if valid data in EEPROM the "know" bit, 
// if this is written in EEPROM the sketch has ran before
// We use this, so that the very first time you'll run this sketch it will use
// the values written above. 
// defining which EEPROM address we are using for what data

void setup()
{
  pinMode(buttonPin, INPUT_PULLUP);
  ShieldSetup (); //Setup the Ethernet shield
  server.begin(); //starting the server
  Serial.begin(9600);
}

void ShieldSetup()
{
  int idcheck = EEPROM.read(0);
  if (idcheck != ID){
    //ifcheck id is not the value as const byte ID,
    //it means this sketch has NOT been used to setup the shield before
    //just use the values written in the beginning of the sketch
  }
  if (idcheck == ID){
    //if id is the same value as const byte ID,
    //it means this sketch has been used to setup the shield.
    //So we will read the values out of EERPOM ans use them
    //to setup the shield.
    for (int i = 0; i < 6; i++){
      mac[i] = EEPROM.read(i+1);
    }
    for (int i = 0; i < 4; i++){
      ip[i] = EEPROM.read(i+7);
    }
    for (int i = 0; i < 4; i++){
      subnet[i] = EEPROM.read(i+11);
    }
    for (int i = 0; i < 4; i++){
      gateway[i] = EEPROM.read(i+15);
    }
    for (int i = 0; i < 2; i++){
      oscpath[i] = EEPROM.read(i+19);
    }    
    for (int i = 0; i < 4; i++){
      companionip[i] = EEPROM.read(i+21);
    }
  }
  Ethernet.begin(mac, ip, gateway, subnet);
  Udp.begin(companionPort);
}

void loop()
{
  CurrentMillis  = millis();
  buttonState = digitalRead(buttonPin);
 if(buttonState == 1 && CurrentMillis - LastMillis > debounceTime)
 {
    oscCall();
    LastMillis = CurrentMillis;
 }
 Serial.println(buttonState);
  EthernetClient client = server.available();
  if (client) {
    TextFinder  finder(client );
    while (client.connected()) {      
      if (client.available()) {
        //This part does all the text searching
        if( finder.find("GET /") ) {
          // if you find the word "setup" continue looking for more
          // if you don't find that word, stop looking and go further
          // This way you can put your own webpage later on in the sketch
          if (finder.findUntil("setup", "\n\r")){
            // if you find the word "SBM" continue looking for more
            // if you don't find that word, stop looking and go further
            // it means the SUBMIT button hasn't been pressed an nothing has
            // been submitted. Just go to the place where the setup page is
            // been build and show it in the client's browser.
          if (finder.findUntil("SBM", "\n\r")){
            byte SET = finder.getValue();
            // Now while you are looking for the letters "DT", you'll have to remember
            // every number behind "DT" and put them in "val" and while doing so, check
            // for the according values and put those in mac, ip, subnet and gateway.
              while(finder.findUntil("DT", "\n\r")){
                int val = finder.getValue();
                // if val from "DT" is between 1 and 6 the according value must be a MAC value.
                if(val >= 1 && val <= 6) {
                  mac[val - 1] = finder.getValue();
                }
                // if val from "DT" is between 7 and 10 the according value must be a IP value.
                if(val >= 7 && val <= 10) {
                  ip[val - 7] = finder.getValue();
                }
                // if val from "DT" is between 11 and 14 the according value must be a MASK value.
                if(val >= 11 && val <= 14) {
                  subnet[val - 11] = finder.getValue();
                }
                // if val from "DT" is between 15 and 18 the according value must be a GW value.
                if(val >= 15 && val <= 18) {
                  gateway[val - 15] = finder.getValue();
                }
                if(val >= 19 && val <= 20) {
                  oscpath[val - 19] = finder.getValue();
                }
                if(val >= 21 && val <= 24) {
                  companionip[val - 21] = finder.getValue();
                }
              }
            // Now that we got all the data, we can save it to EEPROM
            for (int i = 0 ; i < 6; i++){
              EEPROM.write(i + 1,mac[i]);
            }
            for (int i = 0 ; i < 4; i++){
              EEPROM.write(i + 7, ip[i]);
            }
            for (int i = 0 ; i < 4; i++){
              EEPROM.write(i + 11, subnet[i]);
            }
            for (int i = 0 ; i < 4; i++){
              EEPROM.write(i + 15, gateway[i]);
            }
            for (int i = 0 ; i < 2; i++){
              EEPROM.write(i + 19, oscpath[i]);
            }
            for (int i = 0 ; i < 4; i++){
              EEPROM.write(i + 21, companionip[i]);
            }
            // set ID to the known bit, so when you reset the Arduino is will use the EEPROM values
            EEPROM.write(0, 0x92); 
            // if al the data has been written to EEPROM we should reset the arduino
            // for now you'll have to use the hardware reset button
          }
          // and from this point on, we can start building our setup page
          // and show it in the client's browser.
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          for (int i = 0; i < 4; i++)
            {
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table0[i])));
            client.print( buffer );
            }
          for (int i = 0; i < 3; i++)
            {
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[i])));
            client.print( buffer );
            }
          client.print(mac[0],HEX);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[3])));
          client.print( buffer );
          client.print(mac[1],HEX);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[4])));
          client.print( buffer );
          client.print(mac[2],HEX);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[5])));
          client.print( buffer );
          client.print(mac[3],HEX);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[6])));
          client.print( buffer );
          client.print(mac[4],HEX);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[7])));
          client.print( buffer );
          client.print(mac[5],HEX);
          for (int i = 0; i < 4; i++)
            {
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[i])));
            client.print( buffer );
            }
          client.print(ip[0],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[4])));
          client.print( buffer );
          client.print(ip[1],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[5])));
          client.print( buffer );
          client.print(ip[2],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[6])));
          client.print( buffer );
          client.print(ip[3],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[0])));
          client.print( buffer );
          client.print(subnet[0],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[1])));
          client.print( buffer );
          client.print(subnet[1],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[2])));
          client.print( buffer );
          client.print(subnet[2],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[3])));
          client.print( buffer );
          client.print(subnet[3],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[0])));
          client.print( buffer );
          client.print(gateway[0],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[1])));
          client.print( buffer );
          client.print(gateway[1],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[2])));
          client.print( buffer );
          client.print(gateway[2],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[3])));
          client.print( buffer );
          client.print(gateway[3],DEC);

          strcpy_P(buffer, (char*)pgm_read_word(&(string_table6[0])));
          client.print( buffer );
          client.print(oscpath[0],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table6[1])));
          client.print( buffer );
          client.print(oscpath[1],DEC);

          strcpy_P(buffer, (char*)pgm_read_word(&(string_table6[2])));
          client.print( buffer );
          client.print(companionip[0],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table6[3])));
          client.print( buffer );
          client.print(companionip[1],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table6[4])));
          client.print( buffer );
          client.print(companionip[2],DEC);
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table6[5])));
          client.print( buffer );
          client.print(companionip[3],DEC);
          
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[4])));
          client.print( buffer );
          for (int i = 0; i < 7; i++)
            {
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table5[i])));
            client.print( buffer );
            }
          strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[5])));
          client.print( buffer );
          break;
      }}
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println();
        // put your own html from here on
        client.print("IT WORKS: go to ");
        client.print(ip[0],DEC);
        for (int i= 1; i < 4; i++){
          client.print(".");
          client.print(ip[i],DEC);
          }
        client.print("/setup");
        // put your own html until here 
        break;  
        }
      }
    delay(1);
    client.stop();
  }
}
void oscCall()
{
  sprintf(oscCommand,"%s%d%s%d",oscaddr[0],oscpath[0],oscaddr[1],oscpath[1]);
   OSCMessage msg(oscCommand);
   Udp.beginPacket(companionip, companionPort);
   msg.send(Udp); // send the bytes to the SLIP stream
  Udp.endPacket(); // mark the end of the OSC Packet
  msg.empty(); // free space occupied by message
}

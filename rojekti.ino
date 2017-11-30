#include <WiFi.h>
#include <SoftwareSerial.h>
#include "SPI.h"

#define ARDUINO_RX 8//should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 9//connect to RX of the module
SoftwareSerial mySerial(ARDUINO_RX, ARDUINO_TX);//init the serial protocol, tell to myserial wich pins are TX and RX

////////////////////////////////////////////////////////////////////////////////////
//all the commands needed in the datasheet(http://geekmatic.in.ua/pdf/Catalex_MP3_board.pdf)
static byte Send_buf[8] = {0} ;//The MP3 player undestands orders in a 8 int string
                                 //0X7E FF 06 command 00 00 00 EF;(if command =01 next song order) 
#define NEXT_SONG 0X01  
#define PREV_SONG 0X02  
#define CMD_PLAY_W_INDEX 0X03 //Laulun numero
#define VOLUME_UP_ONE 0X04 
#define VOLUME_DOWN_ONE 0X05 
#define CMD_SEL_DEV 0X09 //SELECT STORAGE DEVICE, DATA IS REQUIRED 
#define DEV_TF 0X02 //HELLO,IM THE DATA REQUIRED  
#define CMD_PLAY 0X0D //RESUME PLAYBACK 
#define CMD_PAUSE 0X0E //PLAYBACK IS PAUSED 
#define CMD_WAKE_UP 0X0B


/////////////////////////////////////////////////////////////////////////
const char* ssid = "OnePlus";
const char* pass = "arduino0";

WiFiServer server(80);
 
void setup()
{
  moduuliSetup();
  langatonSetup();
}
 
void loop() 
{
kayttoliittyma(); 
}



void langatonSetup()
{
  Serial.begin(9600);   
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.print("WiFi-korttia ei loydy");    // Langattoman verkon setup ei jatku, jos  
    // don't continue:
    while(true);
  }
  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, pass);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  IPAddress ip = WiFi.localIP();
  Serial.print("Käyttöliittymän osoite verkossa: ");
  Serial.print(ip);
  
}

void moduuliSetup()
{
mySerial.begin(9600);                               // Avaa kommunikaation Arduinon ja Mp3-moduulin kanssa.
delay(500);
sendCommand(CMD_SEL_DEV, DEV_TF);                   // Lähettää ensimmäisen byten moduulille, tässä se valitsee laitteen ja tunnistaa
delay(200);                                         // Moduulin TF-slotin, jossa sijaitsee SD-kortti.
}


void kayttoliittyma()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  sendCommand(CMD_WAKE_UP, 0x0000);
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }




  
  // Read the first line of the request
  String request = client.readStringUntil('\r'); // Näppäimen luomat viittaukset muuttavat URL-koodia, jolla soittimen käyttöliittymä näkyy
  Serial.println(request);                       // Vertaa viittauksen luomia muutoksia URL-koodissa alhaalla oleviin ehtoihin.
  client.flush();                                
    
                                                 // Seuraavien ehtojen täyttyessä "sendCommand"-funkio lähettää 1 byten sarjakommunikaatiolla
                                                 // MP3-moduulille, jonka perusteella moduuli käsittelee SD-kortilla olevia äänitiedostoja
    if (request.indexOf("/MP=PLAY") != -1) {
    Serial.println("Toista mp3");
    sendCommand(CMD_PLAY_W_INDEX, 0x0001);
    
                                          // PLAY-komento, jolla toistetaan äänitiedosto.
    
  }
    if (request.indexOf("/MP=PAUSE") != -1){
    Serial.println("Mp3 pysäytetty");
    sendCommand(CMD_PAUSE, 0x0000);
                                          // PAUSE-komento jolla keskeytään audio lähetys AUX:lle
    
  }
    if (request.indexOf("/MP=NEXT") != -1){
    Serial.println("Seuraava kappale");
    sendCommand(NEXT_SONG, 0x0000);
                                          // NEXT-komento, jolla voidaan navigoida musiikkitiedostosta toiseen  
  }
    if (request.indexOf("/MP=PREV") != -1){
    Serial.println("Edellinen kappale");
    sendCommand(PREV_SONG, 0x0000);
                                          // PAUSE-komento mp3-moduulille 
  }
    if (request.indexOf("/MP=UP") != -1){
    Serial.println("Volume Up");
    sendCommand(VOLUME_UP_ONE, 0x0000);
    
                                          // ÄÄnitasoa ylös "Volume up"
  }
    if (request.indexOf("/MP=DOWN") != -1){
    Serial.println("Volume Down");
    sendCommand(VOLUME_DOWN_ONE, 0x0000); // Äänitasoa alas
  }
    //HTTP_request toiminto
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.print("Musasoitin: ");
  client.print("<style>button {text-decoration: none} body { background-color:#d0e4fe; text-align: center }</style>"); // Sivun tausta väri vaihdettu
  
      client.println("<br><br>"); //Rivivaihtoja..?
      client.println("<button><a href=\"/MP=PLAY\">PLAY</a></button>"); //
      client.println("<button><a href=\"/MP=PAUSE\">PAUSE</a></button><br>");
      client.println("<button><a href=\"/MP=NEXT\">NEXT</a></button>"); //
      client.println("<button><a href=\"/MP=PREV\">PREV</a></button><br>");
      client.println("<button><a href=\"/MP=UP\"> + </a></button>"); //
      client.println("<button><a href=\"/MP=DOWN\"> - </a></button>");
  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}

void sendCommand(byte command, byte dat)       // Tämä funktio generoi moduulille lähtevän bittijonon.
{
 delay(20);
 Send_buf[0] = 0x7E;                                // Starttibyte
 Send_buf[1] = 0xFF;                                // Versiobyte
 Send_buf[2] = 0x06;                                // Lähetettävien bytejen määrä pois lukien startti- ja lopetusbyte (6)
 Send_buf[3] = command;                             // 
 Send_buf[4] = 0x00;                                // 0x00 = no feedback, 0x01 = feedback
 Send_buf[5] = (byte)(dat >> 8);                  // datah
 Send_buf[6] = (byte)(dat);                       // datal
 Send_buf[7] = 0xEF;                                // ending byte
 
 for(byte i=0; i<8; i++)                            //
 {
   mySerial.write(Send_buf[i]) ;                    // Lähettää funktion arvot mp3-moduulille
   Serial.print(Send_buf[i], HEX);                  // Tulostaa lähetettävät arvot monitorille
 }
 Serial.println();
}

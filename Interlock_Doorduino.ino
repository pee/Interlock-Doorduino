/*
  Interlock Doorduino
 
 This sketch reads iButtons, twiddles an electronic
 strike plate, and sends door status to a web site
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * iButton reader on pin 9
 * Door open sensor on pin 4
 
 iButton ID:
 (8-bit family code + 48-bit serial number + 8-bit CRC tester) 
 I'm going to send it to the LDAP server with all those bytes in tact, and recomend storing it that
 way for the fuck of it
   
 created 26 Feb 2011
 by Beardicus brian@interlockroc.org
 
  modified 2011.10.09 -pee
  -- ethernet connectivity
  -- LDAP connection
  -- switched uint8_t to byte because apparently that's the way arduino does it
 
 Port 24 switch 1
 
 */

#include <stdio.h>
#include <OneWire.h>
#include <EtherShield.h>


// Define this to have the code use DHCP, comment out to hard code settings
#define USE_DHCP
#ifndef USE_DHCP
#define DEF_IP { 10, 10, 2, 14 }
#define DEF_GW { 10, 10, 2, 254 }
#define DEF_SN { 255, 255, 255, 0 }  
#define DEF_DNS { 10, 10, 2, 254 }
#endif

// Define this to read the mac from the chip on the back of the nanode
// comment out and fille in the following array to use a static one
#define DYNAMIC_MAC
#ifndef DYNAMIC_MAC
//#define DEF_MAC {  0x0, 0x4, 0xA3, 0x3, 0x8D, 0x98 };
#define DEF_MAC {  0x0, 0x4, 0xA3, 0x3, 0x93, 0xE };
#endif


// the LDAP server to send to
//static uint8_t ldapServer[4] = { 10, 1, 1, 7 };
static byte ldapServer[4] = { 192,168,2,1 };
static uint16_t serverPort = 9999;
//static byte server[4] = { 208, 113, 133, 131 }; // ben.woodruff.ws



/////////////////////////////////////////////////////////////////////
//
//           *** **** ***** W A R N I N G ***** **** ***
//
//              Below here there be dragons and shit
//
// Well except in the LocalButton file, there is a nice spot there 
// with an array to edit
/////////////////////////////////////////////////////////////////////



// if you make this too small DHCP will fail
// since it uses a static memcopy of 400
#define BUFFER_SIZE 550
#define UDP_BUFFER_SIZE 100

#define ID_LENGTH 8
#define ANSWER_LENGTH 4

#define GOOD_DELAY 5000
#define BAD_DELAY 10000

#define SERIAL_SPEED 19200

#ifdef DYNAMIC_MAC
static byte ethMAC[6] = { 0, 0, 0, 0, 0, 0};
#else
static byte ethMAC[6] = DEF_MAC;
#endif

#ifdef USE_DHCP
static byte ethIP[4] = { 0, 0, 0, 0 };
static byte ethGW[4] = { 0, 0, 0, 0 };
static byte ethSN[4] = { 0, 0, 0, 0 };
static byte ethDNS[4] = { 0, 0, 0, 0 };
static byte dhcpIP[4] = { 0, 0, 0, 0 };
#else
static byte ethIP[4] = DEF_IP;
static byte ethGW[4] = DEF_GW;
static byte ethSN[4] = DEF_SN;
static byte ethDNS[4] = DEF_DNS;
#endif

// "TRUE" the hard way, used to match response from LDAP
static byte validButton[ANSWER_LENGTH] = { 0x54, 0x52, 0x55, 0x45 };


// set up pins
OneWire ibutton(9);  // iButton reader

int readerLED = 5;   // iButton reader LED 
int greenLED = 7;    // good button
int redLED = 6;      // bad button
int strikePin = 8;   // strikeplate coil
int doorPin = 4;     // door reed switch
int doorDelay = 60;   // update after x seconds

static byte buf[BUFFER_SIZE+1];
static byte udpBuf[UDP_BUFFER_SIZE];

EtherShield es=EtherShield();

void setup() {

  // start the serial library
  Serial.begin(SERIAL_SPEED);
  
  Serial.println("Ethernet Doorduinobotdroid now with more stuff");

#ifdef DYNAMIC_MAC

  noInterrupts();
  
  unio_standby();
  
  unio_start_header(); // start header, wakes up the chip and syncs clock
  
  unio_sendByte(0xA0); // Address A0 (the chip address)
  unio_sendByte(0x03); // Read instruction
  unio_sendByte(0x00); // word address MSB 0x00
  unio_sendByte(0xFA); // word addres LSB 0xFA
  unio_readBytes( ethMAC , 6); // read 6 bytes
  
  // back to standby
  unio_standby();
  
  interrupts();
  
#endif

  printMAC( ethMAC );

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize ENC28J60
  es.ES_enc28j60Init( ethMAC, 8);

#ifdef USE_DHCP

  Serial.println("Starting DHCP");
  int dh  = es.allocateIPAddress(buf, BUFFER_SIZE, ethMAC, 80, ethIP, ethSN, ethGW, ethDNS, dhcpIP);
  if ( dh == 0 ) {
    Serial.println("Explode here");
  }
  
#endif

  printNetworkParameters();

#ifndef USE_DHCP
  es.ES_init_ip_arp_udp_tcp( ethMAC , ethIP, 80);
  es.ES_client_set_gwip( ethGW );
#endif

  // prime the arp cache
  es.ES_client_arp_whohas( buf, ethGW );

  // set pin modes
  pinMode(readerLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(strikePin, OUTPUT);
  pinMode(doorPin, INPUT);

  // turn the reader LED on
  digitalWrite(readerLED, HIGH);

}

void loop() {

  byte id[ID_LENGTH];
  byte checkBuffer[ID_LENGTH];
  uint16_t datp;
  int8_t plen;
  byte answer[ANSWER_LENGTH];
  
  boolean netAnswer = false;
  boolean ldapOK = false;
  
  boolean localOK = false;


  // Allow for processing outside of 1wire/ibutton processing
  plen = es.ES_enc28j60PacketReceive( BUFFER_SIZE , buf );
  datp = es.ES_packetloop_icmp_tcp( buf , plen );

  // if no ibutton is found, return
  if ( !ibutton.search( id ) ) {
    ibutton.reset_search();
    return;
  }

  // if checksum is wrong, return
  if ( OneWire::crc8( id, 7) != id[7]) {
    Serial.println("read error: bad read");
    return;
  }

  printButton( id );
   
  copyIdToBuffer( udpBuf, id );
  copyButton( checkBuffer, id );
  
  es.ES_send_udp_data(udpBuf, ID_LENGTH, serverPort, ldapServer, serverPort);
  
  //
  // FIXME: this should eventually be a cycle/retry loop
  //
  delay(250);
  
  plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
  datp = es.ES_packetloop_icmp_tcp(buf,plen);

  if ( plen != 0 ) {
    
    if ( buf[IP_PROTO_P] == IP_PROTO_UDP_V ) {
            
      answer[0] = buf[UDP_DATA_P];
      answer[1] = buf[UDP_DATA_P+1];
      answer[2] = buf[UDP_DATA_P+2];
      answer[3] = buf[UDP_DATA_P+3];
      
      netAnswer = true;
      
    }
    
  }
  
  if ( netAnswer = true ) {
    if ( checkAnswer( answer, validButton) == 1 ) {
      Serial.println("LDAP button OK");
      ldapOK = true;
    } 
  }
    
  if ( isAuthorised( checkBuffer ) ) {
    Serial.println("Local button OK");
    localOK = true;
  }
  
  if ( ( ldapOK==true ) || (localOK == true ) )  {
      
    Serial.println("authorized iButton");

    // twiddle green light and strike plate for five seconds
    digitalWrite(greenLED, HIGH);
    digitalWrite(strikePin, HIGH);
    delay( GOOD_DELAY );
    digitalWrite(greenLED, LOW);
    digitalWrite(strikePin, LOW);
      
  } else {
    Serial.println("unauthorized iButton");

    // twiddle red light and create ten second delay
    digitalWrite(redLED, HIGH);
    delay( BAD_DELAY );
    digitalWrite(redLED, LOW);
  }


} // end of main event loop




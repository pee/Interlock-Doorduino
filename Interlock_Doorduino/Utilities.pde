//
//
int checkAnswer( byte *answer, byte *validButton ) {
  
  int i;
  for ( i = 0; i < ANSWER_LENGTH; i++ ) {
    if ( answer[i] != validButton[i] ) {
      return 0;
    }
  }
  return 1;
}

//
//
void printButton( byte *button ) {
  // print out the id
  Serial.print("iButton: ");
  for( int i = 0; i <= 7; i++) {

    Serial.print(button[i], HEX);
    if ( i < 7 ) {
      Serial.print(".");
    }

  }
  Serial.println();
}

//
// Output a ip address from buffer from startByte
void printIP( byte *buf ) {
  for( int i = 0; i < 4; i++ ) {
    Serial.print( buf[i], DEC );
    if( i<3 )
      Serial.print( "." );
  }
}

//
//
void printNetworkParameters(){
  
  Serial.print( "My IP: " );
  printIP( ethIP );
  Serial.println();

  Serial.print( "Netmask: " );
  printIP( ethSN );
  Serial.println();

  Serial.print( "GW IP: " );
  printIP( ethGW );
  Serial.println();  
  
  Serial.print( "DNS IP: " );
  printIP( ethDNS );
  Serial.println();  
  
  Serial.print( "DHCP IP: " );
  printIP( dhcpIP );
  Serial.println();

}

//
//
void copyIdToBuffer( byte *buf, byte *id ) {
  
  buf[UDP_DATA_P]   = id[0];
  buf[UDP_DATA_P+1] = id[6];
  buf[UDP_DATA_P+2] = id[5];
  buf[UDP_DATA_P+3] = id[4];
  buf[UDP_DATA_P+4] = id[3];
  buf[UDP_DATA_P+5] = id[2];
  buf[UDP_DATA_P+6] = id[1];
  buf[UDP_DATA_P+7] = id[7];
  
}

//
//
void copyButton( byte *buf, byte *id ) {
  buf[0]   = id[0];
  buf[1] = id[6];
  buf[2] = id[5];
  buf[3] = id[4];
  buf[4] = id[3];
  buf[5] = id[2];
  buf[6] = id[1];
  buf[7] = id[7];
}

//
//
//
void printMAC( byte *buf ) {

  Serial.print("MAC:");

  for ( int i = 0; i < 6; i++ ) {
    Serial.print( buf[i], HEX );
    if ( i < 5 ) {
      Serial.print( ":" );
    }
  }
  Serial.println("");

}


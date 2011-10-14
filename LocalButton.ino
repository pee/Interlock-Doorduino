//
//
//
//
//
//
byte localIds[][8] = {
  { 0x01, 0x00, 0x00, 0x12, 0xfd, 0xa5, 0xb3, 0x6b }, // pee good ldap
  { 0x01, 0x00, 0x00, 0x12, 0xff, 0x51, 0x61, 0x7f },  // peetest2 - bad in ldap, good here
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };

//
//b
int isAuthorised(byte *id) {
  
  int i = 0;
  
  // this check is only valid until ID buttons start with 0x0 ;p
  while ( localIds[i][0] != 0x0 ) {
    if ( checkButton( id, localIds[i] ) == 1 ) {
      return 1;
    }
    i++;
  }
  
  return 0;
}

//
//
int checkButton( byte *answer, byte *validButton ) {
  
  int i;
  for ( i = 0; i < ID_LENGTH; i++ ) {    
    if ( answer[i] != validButton[i] ) {
      return 0;
    }
  }
  return 1;
}

#ifndef _KITELIB_PSK_H
#define _KITELIB_PSK_H

#include "TypeDef.h"
#include "PhysicalLayer.h"

// Varicode character table: - position in table corresponds to ASCII code
//                           - value in table corresponds to Varicode code
//                           - leading zeros are not shown
const uint16_t VaricodeTable[128] = {0b1010101011,  //  NUL
                                     0b1011011011,  //  SOH
                                     0b1011101101,  //  STX
                                     0b1101110111,  //  ETX
                                     0b1011101011,  //  EOT
                                     0b1101011111,  //  ENQ
                                     0b1011101111,  //  ACK
                                     0b1011111101,  //  BEL
                                     0b1011111111,  //  BS
                                     0b11101111,    //  HT
                                     0b11101,       //  LF
                                     0b1101101111,  //  VT
                                     0b1011011101,  //  FF
                                     0b11111,       //  CR
                                     0b1101110101,  //  SO
                                     0b1110101011,  //  SI
                                     0b1011110111,  //  DLE
                                     0b1011110101,  //  DC1
                                     0b1110101101,  //  DC2
                                     0b1110101111,  //  DC3
                                     0b1101011011,  //  DC4
                                     0b1101101011,  //  NAK
                                     0b1101101101,  //  SYN
                                     0b1101010111,  //  ETB
                                     0b1101111011,  //  CAN
                                     0b1101111101,  //  EM
                                     0b1110110111,  //  SUB
                                     0b1101010101,  //  ESC
                                     0b1101011101,  //  FS
                                     0b1101011101,  //  GS
                                     0b1011111011,  //  RS
                                     0b1101111111,  //  US
                                     0b1,           //  SP
                                     0b111111111,   //  !
                                     0b101011111,   //  "
                                     0b111110101,   //  #
                                     0b111011011,   //  $
                                     0b1011010101,  //  %
                                     0b1010111011,  //  &
                                     0b101111111,   //  '
                                     0b11111011,    //  (
                                     0b11110111,    //  )
                                     0b101101111,   //  *
                                     0b111011111,   //  +
                                     0b1110101,     //  ,
                                     0b110101,      //  -
                                     0b1010111,     //  .
                                     0b110101111,   //  /
                                     0b10110111,    //  0
                                     0b10111101,    //  1
                                     0b11101101,    //  2
                                     0b11111111,    //  3
                                     0b101110111,   //  4
                                     0b101011011,   //  5
                                     0b101101011,   //  6
                                     0b110101101,   //  7
                                     0b110101011,   //  8
                                     0b110110111,   //  9
                                     0b11110101,    //  :
                                     0b110111101,   //  ;
                                     0b111101101,   //  <
                                     0b1010101,     //  =
                                     0b111010111,   //  >
                                     0b1010101111,  //  ?
                                     0b1010111101,  //  @
                                     0b1111101,     //  A
                                     0b11101011,    //  B
                                     0b10101101,    //  C
                                     0b10110101,    //  D
                                     0b1110111,     //  E
                                     0b11011011,    //  F
                                     0b11111101,    //  G
                                     0b101010101,   //  H
                                     0b1111111,     //  I
                                     0b111111101,   //  J
                                     0b101111101,   //  K
                                     0b11010111,    //  L
                                     0b10111011,    //  M
                                     0b11011101,    //  N
                                     0b10101011,    //  O
                                     0b11010101,    //  P
                                     0b111011101,   //  Q
                                     0b10101111,    //  R
                                     0b1101111,     //  S
                                     0b1101101,     //  T
                                     0b101010111,   //  U
                                     0b110110101,   //  V
                                     0b101011101,   //  W
                                     0b101110101,   //  X
                                     0b101111011,   //  Y
                                     0b1010101101,  //  Z
                                     0b111110111,   //  [
                                     0b111101111,   //  backslash
                                     0b111111011,   //  ]
                                     0b1010111111,  //  ^
                                     0b101101101,   //  _
                                     0b1011011111,  //  `
                                     0b1011,        //  a
                                     0b1011111,     //  b
                                     0b101111,      //  c
                                     0b101101,      //  d
                                     0b11,          //  e
                                     0b111101,      //  f
                                     0b1011011,     //  g
                                     0b101011,      //  h
                                     0b1101,        //  i
                                     0b111101011,   //  j
                                     0b10111111,    //  k
                                     0b11011,       //  l
                                     0b111011,      //  m
                                     0b1111,        //  n
                                     0b111,         //  o
                                     0b111111,      //  p
                                     0b110111111,   //  q
                                     0b10101,       //  r
                                     0b10111,       //  s
                                     0b101,         //  t
                                     0b110111,      //  u
                                     0b1111011,     //  v
                                     0b1101011,     //  w
                                     0b11011111,    //  x
                                     0b1011101,     //  y
                                     0b111010101,   //  z
                                     0b1010110111,  //  {
                                     0b110111011,   //  |
                                     0b1010110101,  //  }
                                     0b1011010111,  //  ~
                                     0b1110110101,  //  DEL
                                     };

class VaricodeString {
  public:
    VaricodeString(char c);
    VaricodeString(const char* str);
    ~VaricodeString();
  
    size_t length();
    uint16_t* byteArr();
  
  private:
    char* _str;
    size_t _len;
    
    uint16_t getBits(char c);
};

class PSKClient {
  public:
    PSKClient(PhysicalLayer* phy);
    
    // basic methods
    int16_t begin(int pin, float base, float rate = 31.25, uint16_t audioFreq = 400);
    size_t write(uint16_t* buff, size_t len);
    size_t write(uint16_t code);
    
    size_t print(VaricodeString &);
    size_t print(const char[]);
    
    size_t println(void);
    size_t println(VaricodeString &);
    size_t println(const char[]);
    
  private:
    PhysicalLayer* _phy;
    
    int _pin;
    uint16_t _audioFreq;
    uint32_t _carrier;
    uint16_t _bitDuration;
    
    void mark();
    void space();
};

#endif

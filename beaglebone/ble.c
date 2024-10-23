
/**********************************************
* BeagleBone PRU Backscatter
* BLE advertisements backscatter
* Authors : Ambuj Varshney < ambuj_varshney@it.uu.se >
* (C) 2016 Uppsala Networked Objects (UNO)
************************************************/


#include "blebackscatter.h"

// Computes the CRC for the bluetooth LE payload 
void btLeCrc(const uint8_t* data, uint8_t len, uint8_t* dst){
     uint8_t v, t, d;
    
    while(len--){ 
        d = *data++;
        for(v = 0; v < 8; v++, d >>= 1){
            t = dst[0] >> 7;
            dst[0] <<= 1;
            if(dst[1] & 0x80) dst[0] |= 1;
            dst[1] <<= 1;
            if(dst[2] & 0x80) dst[1] |= 1;
            dst[2] <<= 1;
            
            if(t != (d & 1)){
              dst[2] ^= 0x5B;
              dst[1] ^= 0x06;
            }
        }   
    }
}

// Swap bits function
uint8_t  swapbits(uint8_t a){
  // reverse the bit order in a single byte
    uint8_t v = 0;
    if(a & 0x80) v |= 0x01;
    if(a & 0x40) v |= 0x02;
    if(a & 0x20) v |= 0x04;
    if(a & 0x10) v |= 0x08;
    if(a & 0x08) v |= 0x10;
    if(a & 0x04) v |= 0x20;
    if(a & 0x02) v |= 0x40;
    if(a & 0x01) v |= 0x80;
    return v;
}

// Whiten the payload for BLE 

void btLeWhiten(uint8_t* data, uint8_t len, uint8_t whitenCoeff){
// Implementing whitening with LFSR
    uint8_t  m;
    while(len--){
        for(m = 1; m; m <<= 1){
            if(whitenCoeff & 0x80){
                whitenCoeff ^= 0x11;
                (*data) ^= m;
            }
            whitenCoeff <<= 1;
        }
        data++;
    }
}

// Generate the payload for BLE advertisement packet  
// @Argument - Structure the holds the BLE packet
// @Argument - Payload - actual content of the PDU to be backscattered
// @Argument - Channel to be used to be used to whiten the payload

void generate_ble_adv_payload(struct blePacket *bPacket, char payload[], char mac_address[], uint8_t whiten_channel)
{

   unsigned char len;
   unsigned char ctr=0;
   uint8_t crc[3] = {0x55, 0x55,0x55};
   unsigned char i;
   // Find the length of the string to be transmitted
   len = strlen(payload);

   // Preamble byte
   bPacket-> u8preamble = 0xAA;

   // BLE access address, for advertisements is fixed
   bPacket-> access_address[0] = swapbits(0xD6);
   bPacket-> access_address[1] = swapbits(0xbe);
   bPacket-> access_address[2] = swapbits(0x89);
   bPacket-> access_address[3] = swapbits(0x8e);
   
   // This keeps track of the PDU type
   bPacket-> blePDU[ctr++] = 0x42;
   // this keeps track of the length of the PDU
   bPacket-> blePDU[ctr++] = (BLE_PAYLOAD_PREDEF + len);

   // MAC address
   bPacket-> blePDU[ctr++] = mac_address[0];
   bPacket-> blePDU[ctr++] = mac_address[1];
   bPacket-> blePDU[ctr++] = mac_address[2];
   bPacket-> blePDU[ctr++] = mac_address[3];
   bPacket-> blePDU[ctr++] = mac_address[4];
   bPacket-> blePDU[ctr++] = mac_address[5];
   
   // Some BLE flags
   bPacket-> blePDU[ctr++]  = 0x02;
   bPacket-> blePDU[ctr++]  = 0x01;
   bPacket-> blePDU[ctr++]  = 0x05;
   bPacket-> blePDU[ctr++]  = 0x0a;
   bPacket-> blePDU[ctr++]  = 0x08;

    // Copy the payload text to he payload
   for(i =0; i < len; i++)
      bPacket-> blePDU[ctr++] = payload[i];
  
   // also calculate the CRC for the payload
   btLeCrc(bPacket-> blePDU, ctr,  crc );

   for(i=0; i < 3; i++)
          bPacket-> blePDU[ctr++] = swapbits(crc[i]);

    // Perform data whitening on the payload
    btLeWhiten(bPacket-> blePDU, ctr, swapbits(whiten_channel)  | 2);

    // Swap the entire payload, including CRC
    for(i=0; i < ctr; i++)
      bPacket-> blePDU[i] = swapbits(bPacket-> blePDU[i]);

   // We finally store the length of the payload in the PDU
   bPacket->u8PayloadLen = ctr;


}


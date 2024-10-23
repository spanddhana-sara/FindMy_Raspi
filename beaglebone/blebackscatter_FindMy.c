
/**********************************************
* BeagleBone PRU Backscatter
* BLE advertisements backscatter
* Authors : Ambuj Varshney < ambuj_varshney@it.uu.se >
* (C) 2016 Uppsala Networked Objects (UNO)
************************************************/

#include "blebackscatter.h"
#define BEAGLEBONE


// Control variable
static unsigned char count=0;
// The structure to hold the MAC address of the BLE backscatter device
static uint8_t arrMacAddress[BLE_MAC_ADDRESS];
// Bluetooth packet instantiation variable 
struct blePacket sblePacketInstance;


#ifdef BEAGLEBONE
#include "pru.h"
// Pointer to PRU memory bank
void *pru0DataMem;
void *pru1DataMem;
// BeagleBone PRU pin toggle calculator macro
#define DELAY(ns) ((ns/5-6)/2)
// Freerunning time to calulate the start and end of the symbols for bluetooth
unsigned long  time,endtime;
#endif


/**** Convert byte to bit
 * Argument 1 : byte - Character to be converted to binary
 * Argument 2 : arr  - Binary representation of the character MSB first
**/
void bytetobit(uint8_t byt,uint8_t arr[8])
{

  int i;

  for(i = 0; i < 8; i++) 
    arr[i] = (((byt << i) & 0x80)) >> 7;
      
}

int main(int argc, char **argv) {

  unsigned int i,j;
  int prumemCnt=0;
  uint8_t barr[8];
  char data_to_send[] = "Varshney!";
  int data_len = sizeof(data_to_send) - 1;  // Exclude null terminator

  if(argc == 2)
  {
      strcpy(payload, argv[1]);
      printf("Payload is : %s\n", payload);

  }

  // Variables used to keep track of rolling time and the last bit
  // used for different stages of BLE packet backscatter process.
  time = 0; 

#ifdef BEAGLEBONE
    // PRU setup and other configuration
   if(geteuid()) {
      fprintf(stderr, "%s must be run as root to use prussdrv\n", argv[0]);
      return -1;
   }

  // initialize the library, PRU and interrupt; launch our PRU program 
   if(pru_setup()) {
      pru_cleanup();
      return -1;
   }

   uint8_t *pruMem8 = (uint8_t *) pru0DataMem;
   int rtn;

#endif

  /// Example MAC address and whitening channel
    char mac_address[] = {0xBA, 0xBE, 0xDE, 0xAD, 0xBE, 0xEF};  // Example MAC address
    uint8_t whiten_channel = 39;  // Example BLE channel (usually 37, 38, or 39)

    // Example message ID
    uint32_t msg_id = 0x00000000;  // Initial message ID

    // Send the data in chunks, increasing the msg_id for each chunk
    send_data_chunked(data_to_send, data_len, msg_id, mac_address, whiten_channel);


  // Populate the BLE payload array and structures
  //generate_ble_adv_payload(&sblePacketInstance, payload, arrMacAddress,39);

  // If beaglebone is not defined, return immediately after printing the BLE payload

#ifndef BEAGLEBONE

  //printf("\n BLE payload is: ");
 // printf("%x,", sblePacketInstance.u8preamble);
  //printf("%x,", sblePacketInstance.access_address[0])


  return 0;
#endif

#ifdef DEBUG
  printf("BLE packet generated, length %d\n",sblePacketInstance.u8PayloadLen );
#endif

#ifdef BEAGLEBONE
    // Populate PRU memory ban to toggle now!!!
    bytetobit(sblePacketInstance.u8preamble, barr);
    POPULATE_PRU;

#ifdef DEBUG
    printf ("len %d\n",sblePacketInstance.u8PayloadLen, sblePacketInstance.u8PayloadLen );
#endif

#ifdef DEBUG
    printf("BLE: Preamble processed is : %x\n", sblePacketInstance.u8preamble);
#endif

    bytetobit(sblePacketInstance.access_address[0], barr);
    POPULATE_PRU;
    bytetobit(sblePacketInstance.access_address[1], barr);
    POPULATE_PRU;
    bytetobit(sblePacketInstance.access_address[2], barr);
    POPULATE_PRU;
    bytetobit(sblePacketInstance.access_address[3], barr);
    POPULATE_PRU;

#ifdef DEBUG
    printf("BLE: Access address processed is %x,%x,%x,%x\n", sblePacketInstance.access_address[0],\
            sblePacketInstance.access_address[1],sblePacketInstance.access_address[2],\
            sblePacketInstance.access_address[3]);
    printf("BLE Payload is:");
#endif

    for(j=0; j < sblePacketInstance.u8PayloadLen; j++)
    {

#ifdef DEBUG
      printf("%x,",sblePacketInstance.blePDU[j] );
#endif

      bytetobit(sblePacketInstance.blePDU[j], barr);
      POPULATE_PRU;

    }

    // Indicate that bits to be transmitted are done now.
    pruMem8[prumemCnt ] = 0; 


#ifdef DEBUG
    printf("\nBLE: Payload processed: %d\n", prumemCnt);
    printf("Endtime: %f\n", endtime);

#endif

    printf("Executing PRU program\n");

    for(i=0; i< 100; i++)
    {


    if((rtn = prussdrv_exec_program(PRU_NUM, "fsk.bin")) < 0) {
      fprintf(stderr, "prussdrv_exec_program() failed\n");
      return rtn;
   }

   printf("Loop %d\n", i);
   usleep(1000*300);
  }
   printf("waiting for interrupt from PRU0...\n");

   rtn = prussdrv_pru_wait_event(PRU_EVTOUT_0);

   printf("PRU program completed, event number %d\n", rtn);

   return pru_cleanup();

#endif

}

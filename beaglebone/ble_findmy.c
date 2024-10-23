
/**********************************************
* BeagleBone PRU Backscatter
* BLE advertisements backscatter
* Authors : Ambuj Varshney < ambuj_varshney@it.uu.se >
* (C) 2016 Uppsala Networked Objects (UNO)
************************************************/


#include "blebackscatter.h"
#include "uECC.h"


// BLE Advertisement Packet Structure
struct blePacket {
    uint8_t u8preamble;
    uint8_t access_address[4];
    uint8_t blePDU[42];  // Payload data, includes MAC and payload
    uint8_t u8PayloadLen;
};


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


// Function prototypes
uint8_t* set_addr_and_payload_for_byte(char mac_address[], int index, uint32_t msg_id, uint8_t val);
void send_data_chunked(uint8_t* data_to_send, int data_len, uint32_t msg_id, char* mac_address, uint8_t whiten_channel);
void generate_ble_adv_payload(struct blePacket *bPacket, uint8_t payload[], char mac_address[], uint8_t whiten_channel);
void set_payload_from_key(uint8_t *payload, uint8_t *public_key);

// Global variables
uint8_t start_addr[16] = {0};  // Example start address
uint8_t curr_addr[16];         // Current address
 
//Function to validate public key
int is_valid_pubkey(uint8_t *pub_key_compressed) {
   uint8_t with_sign_byte[29];
   uint8_t pub_key_uncompressed[128];
   const struct uECC_Curve_t * curve = uECC_secp224r1();
   with_sign_byte[0] = 0x02;
   memcpy(&with_sign_byte[1], pub_key_compressed, 28);
   uECC_decompress(with_sign_byte, pub_key_uncompressed, curve);
   if(!uECC_valid_public_key(pub_key_uncompressed, curve)) {
       //ESP_LOGW(LOG_TAG, "Generated public key tested as invalid");
       return 0;
   }
   return 1;
}

void copy_2b_big_endian(uint8_t *dst, uint8_t *src) {
    dst[0] = src[1]; dst[1] = src[0];
}

// Function to set address and payload based on index, msg_id, and value
uint8_t* set_addr_and_payload_for_byte(char mac_address[],int index, uint32_t msg_id, uint8_t val) {
    uint8_t* public_key = (uint8_t*)malloc(28 * sizeof(uint8_t));  // Dynamically allocate memory
    memset(public_key, 0, 28 * sizeof(uint8_t));

    // Initialize public_key with modem_bytearray and other values
    public_key[0] = mac_address[0];  // BA Magic value
    public_key[1] = mac_address[1]; //BE
    public_key[2] = mac_address[2];
    public_key[3] = mac_address[3];
    public_key[4] = mac_address[4];
    public_key[5] = mac_address[5];
    public_key[6] = 0x00;
    public_key[7] = 0x00;

    // Copy msg_id to public_key[8:12] in big-endian format
    public_key[8] = (msg_id >> 24) & 0xFF;
    public_key[9] = (msg_id >> 16) & 0xFF;
    public_key[10] = (msg_id >> 8) & 0xFF;
    public_key[11] = msg_id & 0xFF;

    // Copy the address depending on index
    if (index) {
        memcpy(&public_key[12], curr_addr, 16);
    } else {
        memcpy(&public_key[12], start_addr, 16);
    }

    // Modify the public_key based on the input index and value
    int bit_index = index * 8;  // Each byte has 8 bits
    int byte_index = bit_index / 8;

    int start_byte = byte_index % 16;
    int next_byte = (start_byte + 1) % 16;

    int start_offset = bit_index % 8;
    int chunk_len = 8;  // We are handling data one byte at a time

    if ((8 - start_offset) >= chunk_len) {
        public_key[27 - start_byte] ^= val << start_offset;
    } else {
        public_key[27 - start_byte] ^= val << start_offset;
        public_key[27 - next_byte] ^= val >> (8 - start_offset);
    }

    // Update curr_addr to reflect the changes made in public_key
    memcpy(curr_addr, &public_key[12], 16);
    
    do {
         copy_2b_big_endian(&public_key[6], &valid_key_counter);
           valid_key_counter++;
       } while (!is_valid_pubkey(public_key));

    return public_key;
}

// Function to generate the BLE advertisement payload
void generate_ble_adv_payload(struct blePacket *bPacket, uint8_t payload[], char mac_address[], uint8_t whiten_channel) {
    unsigned char ctr = 0;
    uint8_t crc[3] = {0x55, 0x55, 0x55};
    unsigned char i;
    

    // Preamble byte
    bPacket->u8preamble = 0xAA;

    // BLE access address, for advertisements is fixed
    bPacket->access_address[0] = 0xD6;
    bPacket->access_address[1] = 0xBE;
    bPacket->access_address[2] = 0x89;
    bPacket->access_address[3] = 0x8E;

    // PDU Header
    bPacket->blePDU[0] = 0x42;  // PDU Type
    bPacket->blePDU[1] = 0x25;  // Payload Length MAC Addres to CRC (37 Bytes)

    // MAC address
    bPacket->blePDU[2] = mac_address[0] | 0b11000000;
    bPacket->blePDU[3] = mac_address[1];
    bPacket->blePDU[4] = mac_address[2];
    bPacket->blePDU[5] = mac_address[3];
    bPacket->blePDU[6] = mac_address[4];
    bPacket->blePDU[7] = mac_address[5];
    // Print the BLE Packet
        printf("BLE Address (6 Bytes): ");
    for (i = 2; i <= 7; i++) {
        printf("%02X ", bPacket->blePDU[i]);
    }
    printf("\n");
    // BLE advertisement payload
    bPacket->blePDU[8]  = 0x1e;  // Length (30)
    bPacket->blePDU[9]  = 0xff;  // Manufacturer Specific Data (type 0xff)
    bPacket->blePDU[10] = 0x4c;  // Company ID (Apple)
    bPacket->blePDU[11] = 0x00;  // Company ID (Apple)
    bPacket->blePDU[12] = 0x12;  // Offline Finding type and length
    bPacket->blePDU[13] = 0x19;  // Offline Finding type and length
    bPacket->blePDU[14] = 0x00;  // State
    printf("Apples FindMy (7 Bytes): ");
    for (i = 8; i <= 14; i++) {
        printf("%02X ", bPacket->blePDU[i]);
    }
    printf("\n");
    
    // Set payload from public key
    memcpy(&bPacket->blePDU[15], &payload[6], 22);
 
  //Payload copy to BLE Packet
   // printf("Payload from Key (22 Bytes): ");
    //for (i = 15; i <= 36; i++) {
     //   printf("%02X ", bPacket->blePDU[i]);
    //}
    //printf("\n");
    

    bPacket->blePDU[37] = 0X00; /* First two bits */
    bPacket->blePDU[38] = 0x00; /* Hint (0x00) */
    bPacket->blePDU[37] = payload[0] >> 6;
    printf("BLE Adv Data (31 Bytes): ");
    for (i = 8; i <= 38; i++) {
        printf("%02X ", bPacket->blePDU[i]);
    }
    printf("\n");
    
    // Also calculate the CRC for the payload
    btLeCrc(bPacket->blePDU, 42, crc);
    
    for (i = 0; i < 3; i++) {
        bPacket->blePDU[39+i] = swapbits(crc[i]);
    }
    
    // Perform data whitening on the payload
    btLeWhiten(bPacket->blePDU, 42, swapbits(whiten_channel) | 2);
    
    // Swap the entire payload, including CRC
    for (i = 0; i < 42; i++) {
        bPacket->blePDU[i] = swapbits(bPacket->blePDU[i]);
    }
    
    // We finally store the length of the payload in the PDU
    bPacket->u8PayloadLen = 42;
    
}

// Function to send data in chunks (increments msg_id after every 16 bytes)
void send_data_chunked(uint8_t* data_to_send, int data_len, uint32_t msg_id, char* mac_address, uint8_t whiten_channel) {
    void send_data_once_blocking(uint8_t byte, int index, uint32_t msg_id) { unsigned char i;
        struct blePacket packet;
        
        uint8_t* key = set_addr_and_payload_for_byte( mac_address, index % 16, msg_id, byte);
    
        // Generate BLE advertisement with the key
        generate_ble_adv_payload(&packet, key, mac_address, whiten_channel);

        
        // Print the public_key
    printf("Public Key(28 Bytes): ");
    for (i = 0; i < 28; i++) {
        printf("%02X ", key[i]);
    }
    printf("\n");
        
        
        
        printf("BLE PDU Packet (42 Bytes): ");
        for (int i = 0; i < packet.u8PayloadLen; i++) {
            printf("%02x ", packet.blePDU[i]);
        }
        printf("\n");

        free(key);  // Free the dynamically allocated public key
    }

    int chunk_size = 16;  // Size of each chunk (16 bytes per message)
    for (int start = 0; start < data_len; start += chunk_size) {
        int end = start + chunk_size > data_len ? data_len : start + chunk_size;
        printf("Sending chunk starting at index %d with msg_id %u\n", start, msg_id);

        // Loop to send each byte in the current chunk
        for (int index = start; index < end; index++) {
            send_data_once_blocking(data_to_send[index], index, msg_id);
            sleep(1);  // Delay between bytes if needed
        }

        // Increment message ID after every chunk
        msg_id++;
    }
}



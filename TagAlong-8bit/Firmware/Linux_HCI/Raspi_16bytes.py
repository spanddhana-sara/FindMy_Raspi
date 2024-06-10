#!/usr/bin/env python3

import base64
import subprocess
import time
import struct
import argparse
import sys
from ecdsa.keys import VerifyingKey
from ecdsa.curves import NIST224p

def advertisement_template():
    adv = ""
    adv += "1e"  # length (30)
    adv += "ff"  # manufacturer specific data
    adv += "4c00"  # company ID (Apple)
    adv += "1219"  # offline finding type and length
    adv += "00"  # state
    for _ in range(22):  # key[6:28]
        adv += "00"
    adv += "00"  # first two bits of key[0]
    adv += "00"  # hint
    return bytearray.fromhex(adv)


def bytes_to_strarray(bytes_, with_prefix=False):
    if with_prefix:
        return [hex(b) for b in bytes_]
    else:
        return [format(b, "x") for b in bytes_]


def is_valid_pubkey(public_key, valid_key_counter=0):  # Check if the compressed public key is valid
    temp_pub_key = bytearray(29)
    temp_pub_key[0] = 0x02

    #valid_key_counter = 0
    #print(valid_key_counter)
    counter_bytes = valid_key_counter.to_bytes(2, 'big')
    public_key[6:8] = counter_bytes

    temp_pub_key[1:] = public_key
    temp_pub_key
    #print(public_key)
    try:
      VerifyingKey.from_string(bytes(temp_pub_key), curve=NIST224p)

    except:
      return False
    else:
      print("valid key!")
      vk = VerifyingKey.from_string(bytes(temp_pub_key), curve=NIST224p)
      pub_key = vk.to_string('uncompressed')
      #print(pub_key.hex())
      #print(public_key.hex())
      print(bytearray.fromhex(public_key.hex()))
      return True

def send_public_key(public_key):
    """Simulate sending the public key."""
    print("Sending public key:", public_key)

def copy_4b_big_endian(dst, src):
    dst[0] = src[3]
    dst[1] = src[2]
    dst[2] = src[1]
    dst[3] = src[0]

start_addr = [0] * 16  # Example start address
curr_addr = start_addr.copy()


# Constants
modem_id = 0x50135013  # Example modem ID
start_addr = [0] * 16  # Example start address
curr_addr = start_addr.copy()
modem_bytearray = bytearray(4) # Initialize modem_id as a bytearray of 4 bytes

# Convert modem_id to a byte array (little-endian to match memory layout in C)
modem_id_bytes = modem_id.to_bytes(4, byteorder='little')

# Perform the copy in big-endian order
copy_4b_big_endian(modem_bytearray, modem_id_bytes)


def set_addr_and_payload_for_byte(index,  msg_id, val, chunk_len):
    valid_key_counter = 0
    public_key = bytearray(28)
    public_key[0] = 0xBA  # magic value
    public_key[1] = 0xBE
    public_key[2:6] =  modem_bytearray
    public_key[6:8] = b'\x00\x00'
   # Convert msg to a byte array (big-endian)
    msg_bytes = msg_id.to_bytes(4, byteorder='big')
    public_key[8:12] = msg_bytes

    if index:
        public_key[12:28] = curr_addr
    else:
        public_key[12:28] = start_addr

    bit_index = index * chunk_len
    byte_index = bit_index // 8

    start_byte = byte_index % 16
    next_byte = (start_byte + 1) % 16

    start_offset = bit_index % 8

    if (8 - start_offset) >= chunk_len:
        public_key[27 - start_byte] ^= val << start_offset
    else:
        public_key[27 - start_byte] ^= val << start_offset
        public_key[27 - next_byte] ^= val >> (8 - start_offset)

    curr_addr[:] = public_key[12:28]

    while not is_valid_pubkey(public_key, valid_key_counter):
            print("-------------------------")
            valid_key_counter += 1
            print(valid_key_counter)

    return public_key
    
def send_data(data_to_send, chunk_len, msg_id):
    def send_data_once_blocking(data_segment, chunk_len, msg_id):
        len_bytes = len(data_segment)
        num_chunks = len_bytes * 8 // chunk_len
        if len_bytes * 8 % chunk_len:
            num_chunks += 1

        for chunk_i in range(num_chunks):
            chunk_value = 0
            start_bit = chunk_i * chunk_len
            end_bit = start_bit + chunk_len

            start_byte = start_bit // 8
            start_bit_offset = start_bit % 8

            bits_in_first_byte = min(8 - start_bit_offset, chunk_len)
            chunk_value = (data_segment[start_byte] >> start_bit_offset) & ((1 << bits_in_first_byte) - 1)

            remaining_bits = chunk_len - bits_in_first_byte
            while remaining_bits > 0:
                start_byte += 1
                bits_to_extract = min(remaining_bits, 8)
                chunk_value |= (data_segment[start_byte] & ((1 << bits_to_extract) - 1)) << bits_in_first_byte
                bits_in_first_byte += bits_to_extract
                remaining_bits -= bits_to_extract

            return set_addr_and_payload_for_byte(chunk_i, msg_id, chunk_value, chunk_len)

    segment_size = 16  # 16 bytes

    if len(data_to_send) <= segment_size:
        # Data is 16 bytes or less
        print("Data is 16 bytes or less")
        return send_data_once_blocking(data_to_send, chunk_len, msg_id)
    else:
        # Data exceeds 16 bytes
        print("Data exceeds 16 bytes")
        total_segments = (len(data_to_send) + segment_size - 1) // segment_size

        for segment_index in range(total_segments):
            start_index = segment_index * segment_size
            end_index = min(start_index + segment_size, len(data_to_send))
            segment = data_to_send[start_index:end_index]

            print(f"Sending segment {segment_index + 1}/{total_segments}: {segment.hex()}")
            return send_data_once_blocking(segment, chunk_len, msg_id + segment_index)

   

def run_hci_cmd(cmd, hci="hci0", wait=1):
    cmd_ = ["hcitool", "-i", hci, "cmd"]
    cmd_ += cmd
    print(cmd_)
    subprocess.run(cmd_)
    if wait > 0:
        time.sleep(wait)


def start_advertising(key, interval_ms=20):
    addr = bytearray(key[:6])
    addr[0] |= 0b11000000

    adv = advertisement_template()
    adv[7:29] = key[6:28]
    adv[29] = key[0] >> 6

    print(f"key     ({len(key):2}) {key.hex()}")
    print(f"address ({len(addr):2}) {addr.hex()}")
    print(f"payload ({len(adv):2}) {adv.hex()}")

    # Set BLE address
    run_hci_cmd(["0x3f", "0x001"] + bytes_to_strarray(addr, with_prefix=True)[::-1])
    subprocess.run(["systemctl", "restart", "bluetooth"])
    time.sleep(1)

    # Set BLE advertisement payload
    run_hci_cmd(["0x08", "0x0008"] + [format(len(adv), "x")] + bytes_to_strarray(adv))

    # Set BLE advertising mode
    interval_enc = struct.pack("<h", interval_ms)
    hci_set_adv_params = ["0x08", "0x0006"]
    hci_set_adv_params += bytes_to_strarray(interval_enc)
    hci_set_adv_params += bytes_to_strarray(interval_enc)
    hci_set_adv_params += ["03", "00", "00", "00", "00", "00", "00", "00", "00"]
    hci_set_adv_params += ["07", "00"]
    run_hci_cmd(hci_set_adv_params)

    # Start BLE advertising
    run_hci_cmd(["0x08", "0x000a"] + ["01"], wait=1)


def main(args):
    # parser = argparse.ArgumentParser()
    # parser.add_argument("--key", "-k", help="Advertisement key (base64)")
    # args = parser.parse_args(args)

    # key = base64.b64decode(args.key.encode())
        # Constants
    NUM_MESSAGES = 100
    REPEAT_MESSAGE_TIMES = 1
    MESSAGE_DELAY = 0.1  # Delay in seconds

    # Initialize current message ID and message data
    current_message_id = 1
    data_to_send = b'ABC'

    # Print message bytes
    print("Bytes:", ' '.join([f"{byte:02x}" for byte in data_to_send]))

    # Message sending loop
    for _ in range(NUM_MESSAGES):
        current_message_id += 1
        for _ in range(REPEAT_MESSAGE_TIMES):
            key = send_data(data_to_send, 8, current_message_id)
            start_advertising(key)
            
    


if __name__ == "__main__":
    main(sys.argv[1:])

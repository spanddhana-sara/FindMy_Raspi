#!/usr/bin/env python3

import base64
import subprocess
import time
import struct
import argparse
import sys
from ecdsa.keys import VerifyingKey
from ecdsa.curves import NIST224p
import pandas as pd
import numpy as np

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

    counter_bytes = valid_key_counter.to_bytes(2, 'big')
    public_key[6:8] = counter_bytes

    temp_pub_key[1:] = public_key
    try:
        VerifyingKey.from_string(bytes(temp_pub_key), curve=NIST224p)
    except:
        return False
    else:
        print("valid key!")
        vk = VerifyingKey.from_string(bytes(temp_pub_key), curve=NIST224p)
        pub_key = vk.to_string('uncompressed')
        print(bytearray.fromhex(public_key.hex()))
        return True

def copy_4b_big_endian(dst, src):
    dst[0] = src[3]
    dst[1] = src[2]
    dst[2] = src[1]
    dst[3] = src[0]

start_addr = [0] * 16  # Example start address
curr_addr = start_addr.copy()

# Constants
modem_id = 0x91909190# Example modem ID
modem_bytearray = bytearray(4) # Initialize modem_id as a bytearray of 4 bytes

# Convert modem_id to a byte array (little-endian to match memory layout in C)
modem_id_bytes = modem_id.to_bytes(4, byteorder='little')

# Perform the copy in big-endian order
copy_4b_big_endian(modem_bytearray, modem_id_bytes)

def set_addr_and_payload_for_byte(index, msg_id, val):
    valid_key_counter = 0
    public_key = bytearray(28)
    public_key[0] = 0xBA  # magic value
    public_key[1] = 0xBE
    public_key[2:6] = modem_bytearray
    public_key[6:8] = b'\x00\x00'
    # Convert msg to a byte array (big-endian)
    msg_bytes = msg_id.to_bytes(4, byteorder='big')
    public_key[8:12] = msg_bytes

    if index:
        public_key[12:28] = curr_addr
    else:
        public_key[12:28] = start_addr

    bit_index = index * 8  # Each byte has 8 bits
    byte_index = bit_index // 8

    start_byte = byte_index % 16
    next_byte = (start_byte + 1) % 16

    start_offset = bit_index % 8
    chunk_len = 8  # We are handling data one byte at a time

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

def send_data_chunked(data_to_send, msg_id):
    def send_data_once_blocking(byte, index, msg_id):
        print(f"Sending byte {index}: {byte:02x}")
        key = set_addr_and_payload_for_byte(index, msg_id, byte)

        # Start advertising for the last key sent
        for _ in range(3):
            start_advertising(key)


    chunk_size = 16  # Size of each chunk
    for start in range(0, len(data_to_send), chunk_size):
        chunk = data_to_send[start:start + chunk_size]
        print(f"Sending chunk starting at index {start}: {chunk.hex()}")
        for index, byte in enumerate(chunk):
            send_data_once_blocking(byte, index, msg_id)
            time.sleep(0.1)  # Delay between bytes if needed
        msg_id += 1  # Increment message ID for each chunk

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

def load_last_processed_timestamp():
    try:
        with open('/home/lab/Desktop/last_processed_timestamp.txt', 'r') as f:
            return float(f.read().strip())
    except FileNotFoundError:
        return None

def save_last_processed_timestamp(timestamp):
    with open('/home/lab/Desktop/last_processed_timestamp.txt', 'w') as f:
        f.write(str(timestamp))

def main(args):
    #last_processed_timestamp = load_last_processed_timestamp()
     # Record the start time
    start_time = time.time()

    # Example data to send
    #data_to_send = b'The beans were initially consumed in the form of a fruit mash before being brewed as a beverage. Coffee spread to the Arab world and became popular in the 15th century. By the 17th century, it had made its way to Europe, where it quickly became a staple in social gatherings. Coffeehouses emerged across Europe and played a crucial role in the cultural and intellectual exchanges of the time. Today, coffee is one of the most consumed beverages globally, with a rich array of varieties and preparation methods reflecting its diverse cultural significance.'
    data_to_send = b'Amidst the bustling city streets, a sense of calm prevailed as the sun dipped below the horizon.'


    current_msg_id = 0
    print("Data to send:", ' '.join([f"{byte:02x}" for byte in data_to_send]))

    send_data_chunked(data_to_send, current_msg_id)

    # Update last processed timestamp
    save_last_processed_timestamp(time.time())
    
    # Record the end time and calculate elapsed time
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Total time taken: {elapsed_time:.2f} seconds")
    data_length = len(data_to_send)
    print(f"Number of bytes in data_to_send: {data_length}")

if __name__ == "__main__":
    main(sys.argv[1:])

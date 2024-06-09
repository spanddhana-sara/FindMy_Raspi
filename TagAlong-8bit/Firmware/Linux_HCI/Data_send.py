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

def sending_data(modem_id, msg_id, data):    
    public_key = bytearray(28)
    modem_bytearray = bytearray(4) # Initialize modem_id as a bytearray of 4 bytes

    public_key[0] = 0xBA  # magic value
    public_key[1] = 0xBE

    # Convert modem_id to a byte array (little-endian to match memory layout in C)
    modem_id_bytes = modem_id.to_bytes(4, byteorder='little')

    # Perform the copy in big-endian order
    copy_4b_big_endian(modem_bytearray, modem_id_bytes)

    public_key[2:6] =  modem_bytearray


    # Convert msg to a byte array (big-endian)
    msg_bytes = msg_id.to_bytes(4, byteorder='big')
    public_key[8:12] = msg_bytes
    print(public_key)

    # Index to start appending bytes in public_key
    index = 27

    data_1 = data
    # Specify encoding (e.g., UTF-8)
    encoding = 'utf-8'

    # Convert string to bytes
    data_to_send = data_1.encode(encoding)

    #print(len(data_to_send))

    # Send bytes from the end and append the byte in public_key starting from public_key[27]
    for i in range(len(data_to_send) - 1, -1, -1):
        byte = data_to_send[i]
        # Append the byte in public_key at the specified index
        public_key[index] = byte
        # Loop until a new valid public key is generated
  
        valid_key_counter = 0
        while not is_valid_pubkey(public_key, valid_key_counter):
            print("-------------------------")
            valid_key_counter += 1
            print(valid_key_counter)

               
        # Decrement the index for next byte
        index -= 1
    print("Final Public Key",public_key)
    for i in range(3): # Sending the data multiple times
    # while(1): # infinite running
        send_public_key(public_key)

        return public_key
    


def run_hci_cmd(cmd, hci="hci0", wait=1):
    cmd_ = ["hcitool", "-i", hci, "cmd"]
    cmd_ += cmd
    print(cmd_)
    subprocess.run(cmd_)
    if wait > 0:
        time.sleep(wait)


def start_advertising(key, interval_ms=2000):
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
    run_hci_cmd(["0x08", "0x000a"] + ["01"], wait=0)


def main(args):
    # parser = argparse.ArgumentParser()
    # parser.add_argument("--key", "-k", help="Advertisement key (base64)")
    # args = parser.parse_args(args)

    # key = base64.b64decode(args.key.encode())
    data = 'ABC'
    modem_id = 0xdeadbeef
    msg_id = 2
    key = send_public_key(modem_id, msg_id, data)
    start_advertising(key)


if __name__ == "__main__":
    main(sys.argv[1:])

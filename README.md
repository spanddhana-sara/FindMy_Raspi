# TagAlong - Modified, 8 bit

The application consists of two parts:

Firmware: An raspberry pi that sends out BLE advertisment which will be picked up by Apple Find My devices
DataFetcher: A macOS application used to retrieve, decode and display the uploaded data
Both are based on OpenHaystack, an open source implementation of the Find My Offline Finding protocol.

This repo is a modified version of the original TagAlong protocol. 


The firmware can write up to 8 bits per advertistment, starting from the LSB. If the data to send is more than the number of bits per advertistment, the program will send another message with the next fragement of data in the next avaliable position in the public key. If the whole data segment of the public key is used up, the new data fragment will wrap around to the LSB and XOR with the existing data.

_**Datafetcher**_

Add logging to file
Add retrieval of multiple messages and fixed interval repeated reterival
Improved performance of data fetching
Datafetcher is fixed to allow fetching of 8 bits per message.

_**How to use**_

**ESP32C3 Firmware**

Install ESP-IDF https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
Mofidy the firmware by changing the modem_id, current_message_id and data_to_send
Connect the ESP32C3
Run "Select port to use (COM, tty, usbserial)" and select the port for the ESP32C3
Run "Set Espressif device target" and select ESP32C3 and via built-in USB-JTAG
Run "menuconfig" and change the following parameters
Enable Bluetooth, Bluetooth 4.2 and Bluetooth Low Energy
Change the partition to 'partitions.csv' and set the flash size to automatically detect
Run "Build, Flash and Start Monitor"
The DataFetcher

Install OpenHaystack including the AppleMail plugin as explained https://github.com/seemoo-lab/openhaystack#installation
Run OpenHaystack and ensure that the AppleMail plugin indicator is green
Run the DataFetcher OFFetchReport application
Insert the 4 byte modem_id previously set in the ESP firmware as hex digits
Set the number of messages, message to start from and repeat fetch timing (0s if refetch is not needed)
Select a folder to set the log file, the file name will be automatically generated
Fetch uploaded messages



**Linux Support**

The script requires a Linux machine with a Bluetooth Low Energy radio chip, a Python environment, and hcitool installed. We tested it on a Raspberry Pi running the official Raspberry Pi OS.

Usage

The Python script uses HCI calls to configure Bluetooth advertising. 
To use as openhaystack device: 
You can copy the required ADVERTISMENT_KEY from the app by right-clicking on your accessory and selecting Copy advertisement key (Base64). Then run the script:

sudo python3 HCI.py --key <ADVERTISMENT_KEY>

Tagalong 8bit implementataion:

sudo python3 HCI.py --key <ADVERTISMENT_KEY>

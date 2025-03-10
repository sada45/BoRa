|FILE|Description|
|---|---|
|BoRa_tx_1280|This folder contains the code for transmitting BoRa packets, compatible with the SX1280 radio module.(BW: 200kHz(203.125kHz), 400kHz(406.250kHz), 800kHz(812.500kHz))|
|BoRa_tx_normal|This folder contains the code for transmitting BoRa packets.(BW:250kHz, 500kHz, 1000kHz)|
|Ble2LoRa_tx|This folder contains the code for transmitting BLE2LoRa packets.(BW:250kHz, 500kHz, 1000kHz)|
|SYP_tx|This folder contains the code for transmitting Symphony packets|

### 1 Send BoRa Signal
Please follow the instructions to compile and flash the program for transmitting BoRa signals.

BoRa supports spreading factors SF 6-12 and the supported bandwidths are as follows:
* 200kHz(200.125kHz)
* 250kHz
* 400kHz(400.250kHz)
* 500kHz
* 800kHz(800.500kHz)
* 1000kHz

#### 1.1 BW: 250kHz, 500kHz, 1000kHz

The following instructions will outline the process for compiling and burning firmware with settings of **SF=7** and a bandwidth of **500kHz**; similar steps apply for other parameters.

```bash
cd BoRa_tx_normal
# -s: SF
# -b: BW, kHz
python3 build_script.py -s 7 -b 500
# Before executing the following program, restart the Ubertooth device to ensure it is in the flashing mode.
ubertooth-dfu -r -d build/BoRa_tx.dfu
```
#### 1.2 BW: 200kHz, 400kHz, 800kHz(compatible with the SX1280 radio module)

The following instructions will outline the process for compiling and burning firmware with settings of **SF=7** and a bandwidth of **400kHz**; similar steps apply for other parameters.

```bash
cd BoRa_tx_1280
# -s: SF
# -b: BW, kHz
python3 build_script.py -s 7 -b 400
# Before executing the following program, restart the Ubertooth device to ensure it is in the flashing mode.
ubertooth-dfu -r -d build/BoRa_tx.dfu
```

### 2 Send Symphony BLE
Please follow the instructions to compile and flash the program for transmitting Symphony BLE signals.

```bash
cd SYP_tx
mkdir build
make
# Before executing the following program, restart the Ubertooth device to ensure it is in the flashing mode.
ubertooth-dfu -r -d build/BLE_SYP_tx.dfu
```

### 3 Send BLE2LoRa Signal
Please follow the instructions to compile and flash the program for transmitting BLE2LoRa signals.

BLE2LoRa supports spreading factors SF 6-12 and bandwidths of 250kHz, 500kHz, and 1000kHz.

The following instructions will outline the process for compiling and burning firmware with settings of **SF=7** and a bandwidth of **500kHz**; similar steps apply for other parameters.

```bash
cd Ble2LoRa_tx
# -s: SF
# -b: BW, kHz
python3 build_script.py -s 7 -b 500
# Before executing the following program, restart the Ubertooth device to ensure it is in the flashing mode.
ubertooth-dfu -r -d build/Ble2LoRa_tx.dfu
```




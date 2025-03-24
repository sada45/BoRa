# ***BoRa:*** LoRa over BLE
This is the open source code for **[MobiCom'25] *BoRa:* LoRa over BLE**.

**Each application directory has a `README.md` to introduce the usage**. The code structure shows as follows:
<!-- ```bash
BoRa_code/
|--ubertooth
   |--firmware/
      |--BoRa_tx_1280
      |--BoRa_tx_normal
      |--BoRa_rx
|--RIOT-BoRa
   |--BoRa_example/
      |--BoRa_ADC_sampling
      |--sx1280_recv
      |--sx1280_recv_bora
      |--sx1280_send

``` -->
```text
📁 BoRa_code/
├── 📂 ubertooth/
│   └── 📂 firmware/
│       ├── 📄 BoRa_tx_1280
│       ├── 📄 BoRa_tx_normal
│       └── 📄 BoRa_rx
└── 📂 RIOT-BoRa/
    └── 📂 BoRa_example/
        ├── 📄 BoRa_ADC_sampling
        ├── 📄 sx1280_recv
        ├── 📄 sx1280_recv_bora
        └── 📄 sx1280_send
```



|FILE|Description|
|---|---|
|BoRa_tx_normal|This folder contains the code for transmitting BoRa packets.(BW:250kHz, 500kHz, 1000kHz).|
|BoRa_tx_1280|This folder contains the code for transmitting BoRa packets, compatible with the SX1280 radio module.(BW: 200kHz(203.125kHz), 400kHz(406.250kHz), 800kHz(812.500kHz)).|
|BoRa_rx|This folder contains the code to put the CC2400 into ATEST mode and output the I/Q signal to ATEST1 and ATEST2.|
|BoRa_ADC_sampling| This folder contains the code for sampling the I/Q signals output from the ATEST1/2 pins on Ubertooth ONE and store them into a TF card.|
|sx1280_recv| This folder contains the code for SX1280 to receive native LoRa signals.|
|sx1280_recv_bora| This folder contains the code for SX1280 to receive emulated LoRa signals transmitted by *BoRa*.|
|sx1280_recv| This folder contains the code for SX1280 to transmit native LoRa signal|


## Requirements
`arm-none-eabi-gcc==10.3.1 20210621 (release)`
Ubertooth firmware&tools can be find in `./ubertooth/host/`, based on [Ubertooth](https://github.com/greatscottgadgets/ubertooth) commit e0fd34d8a
|Goals|Boards|
|---|---|
|*BoRa* Tx emulated LoRa signals | 1x Ubertooth One|
|*BoRa* Rx *BoRa*/LoRa | 1x Ubertooth One, 1x NUCLEO NUCLEO-F722ZE, 1x TF card slot |
|Native LoRa Tx/Rx| 1x NUCLEO-L073RZ, 1x SX1280RF1ZHP | 

## Thanks 
This work uses [Ubertooth firmware](https://github.com/greatscottgadgets/ubertooth) and [RIOT-OS](https://github.com/RIOT-OS/RIOT). Special thanks to the Ubertooth and RIOT communities for their invaluable support!

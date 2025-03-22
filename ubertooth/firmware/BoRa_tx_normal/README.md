### Send BoRa Signal
*BoRa* supports different bandwidths as follows:
* 200kHz(203.125kHz)
* 250kHz
* 400kHz(406.250kHz)
* 500kHz
* 800kHz(812.500kHz)
* 1000kHz

#### Transmit BW: 250kHz, 500kHz, 1000kHz

The following instructions will outline the process for compiling and burning firmware with settings of **SF=7** and a bandwidth of **500kHz**; similar steps apply for other parameters.

```bash
cd firmware/BoRa_tx_normal
# -s: SF
# -b: BW, kHz
python3 build_script.py -s 7 -b 500
# Before executing the following program, restart the Ubertooth device to ensure it is in the flashing mode.
ubertooth-dfu -r -d build/BoRa_tx.dfu
```
The `build_script.py` will generate two files in the application directory:
(1) `BoRa_config.h`: contains the parameters for chirp emulation 
(2) `lora_confg.h`: contains the parameters for mapping the received SF-bit native LoRa symbols to correct BSF-bit *BoRa* symbol. It should be used after the software-defined native LoRa symbol demodulation.
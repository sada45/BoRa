# SX1280 Receive native LoRa signals
========
This is an example to use a **NUCLEO-L073RZ** and a **SX1280RF1ZHP** shield board to receive the signal. 
Use the command as follow to build and flash the code:
```bash
make BOARD=nucleo-l073rz [PROGRAMMER=cpy2remed (optional)] flash
```
After the binary is flashed, start the terminal of the RIOT-OS:
```bash
make term
```
We uses implicit header mode for native LoRa receiving.
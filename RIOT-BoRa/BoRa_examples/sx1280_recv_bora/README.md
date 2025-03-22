# SX1280 Receive *BoRa* signals
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
*BoRa* uses implicit header mode, which requires setting the packet length. However, we find we need to transmit once packet with the certain length can correctly set the receiving packet length. To transmit data:
```bash
sx1280 tx <payload length>
```
After that, we can start receiving *BoRa* signals with:
```bash
sx1280 rx start
```
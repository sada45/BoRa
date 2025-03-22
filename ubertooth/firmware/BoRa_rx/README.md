### Receive BoRa/LoRa Signal on Ubertooth
This application let the Ubertooth One starts to receiving I/Q signal once boot up. The raw signal will be output to ATEST1/2. Use `RIOT-BoRa/BoRa_examples/BoRa_ADC_sampling/` to collect the raw signal with ADCs.
```bash
cd firmware/BoRa_rx
make
ubertooth-dfu -r -d build/BoRa_rx.dfu
```
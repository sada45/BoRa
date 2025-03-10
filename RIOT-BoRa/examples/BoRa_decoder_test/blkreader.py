import numpy as np
import matplotlib.pyplot as plt

sector_size = 512
buf_size = 4096
buf_size_byte = buf_size * 4
end_part = np.zeros(sector_size // 4, dtype=np.uint32)
end_val = (15 << 12) | (15 << 28)
end_part[:] = end_val

raw_datas = []
disk = open("/dev/sdd", "rb")
disk.seek(sector_size * 512 + sector_size * 32)
while True:
    bin_data = disk.read(buf_size * 4)
    array = np.frombuffer(bin_data, dtype="<u4")
    if np.all(array[:128]==end_val):
        raw_datas = np.concatenate(raw_datas)
        np.save("/home/user/RIOT-BoRa/sd_data.npy", raw_datas)
        break
    else:
        raw_datas.append(array)
disk.close()

# raw_datas = np.load("./sd_data.npy")
iq_signal = np.zeros(len(raw_datas), dtype=np.complex64)
temp = raw_datas & 0xffff
temp = temp.astype(np.float32)
iq_signal.real = (temp - np.mean(temp)) / 2048
temp = (raw_datas >>16) & 0xffff
temp = temp.astype(np.float32)
iq_signal.imag = (temp - np.mean(temp)) / 2048
np.save("iq0.npy", iq_signal)
plt.figure()
plt.plot(iq_signal.real)
plt.plot(iq_signal.imag)

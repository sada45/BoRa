import numpy as np
import matplotlib.pyplot as plt
import argparse
import warnings
import os

LOG_LEVEL = 1
parser = argparse.ArgumentParser(description='manual to this script')
parser.add_argument("-b", "--bandwidth", type=int, help="The bandwidth of LoRa")
parser.add_argument("-s", "--sf", type=int, help="The LoRa SF")
parser.add_argument("-p", "--filepath", type=str, help="Set the output file path of the RIOT OS")
parser.add_argument("-a", "--chirpacc", type=int, default=1000000, help="The accuracy of frequency change timing")
parser.add_argument("-d", "--mindis", action="store_true", default=False)
args = parser.parse_args()

# Basic parameters
lora_bd = args.bandwidth  # in KHz
lora_sf = args.sf
if lora_bd == 1000:
    lora_symbols = 64
else:
    lora_symbols = 32

if lora_bd == 800:
    lora_bd = 812.5
elif lora_bd == 400:
    lora_bd = 406.25
elif lora_bd == 200:
    lora_bd = 203.125
elif lora_bd == 1600:
    lora_bd == 1625.00
else:
    raise ValueError(f"Wrong lora bd:{lora_bd}")
    
# chirp_time_acc = 100000000
chirp_time_acc = 1000000
# chirp_time_acc = 1000

base_val = 0x0010  # The basic value of MDMCTRL 
preamble_len = 8
pwd = os.getcwd()  
lora_filepath = pwd + "/lora_config.h"
ubertooth_filepath = pwd + "/BoRa_config.h"

# Indirect params
lora_fbn = 0 # Frequency bin numbers
lora_fbw = 0
ble_freq_bin_num = 0
frequency_change_delay = 2.8 # us
mdmctrl_all =  None
ble_bbin_num = 1

def param_init():
    global lora_bd, lora_sf, chirp_time_acc
    lora_bd = args.bandwidth
    if lora_bd == 200:
        lora_bd = 203.125
    elif lora_bd == 400:
        lora_bd = 406.25
    elif lora_bd == 800:
        lora_bd = 812.5
    else:
        raise Exception("No such a bandwidth {}".format(lora_bd))
    lora_sf = args.sf
    chirp_time_acc = args.chirpacc

def init_param_cal():
    global lora_bd, lora_sf, chirp_time_acc, lora_fbn, lora_fbw, ble_freq_bin_num, lora_filepath, mdmctrl_all, ble_bbin_num
    global MAX_BIN
    lora_fbn = int(2 ** lora_sf)  # Frequency bin numbers
    lora_fbw = lora_bd / (2 ** lora_sf)
    ble_freq_bin_num = int(lora_bd // 15.625)
    if args.filepath is not None:
        
        lora_filepath = "/home/user/RIOT-BoRa/tests/drivers/{}/lora_config.h".format(args.filepath)
    ble_bbin_num = 1
    while ble_bbin_num * 2 <= ble_freq_bin_num and (ble_bbin_num * 2) <= (lora_fbn / 2):
        ble_bbin_num *= 2
    ble_bbin_num = int(ble_bbin_num)
    MAX_BIN = ble_bbin_num
    # The actual frequency bin number should be one more than the ble_freq_bin_num,
    # Since the largest frequency bin (=bw/2) carry no information
    mdmctrl_all = np.arange(-(ble_freq_bin_num-1)//2, (ble_freq_bin_num-1)//2+1, dtype=np.int8)

# Calculate the timing for frequency changes
def frequency_change_timing():
    chirp_dur = 2 ** lora_sf * 1e6 / (lora_bd * 1000)  # in us
    max_freq_bin_num = ble_freq_bin_num
    skip_num = 1
    while chirp_dur / max_freq_bin_num < frequency_change_delay:
        skip_num += 1
        max_freq_bin_num = int(np.around((ble_freq_bin_num -1) / skip_num) + 1)
    freq_error = (max_freq_bin_num - 1) * 15.625 * skip_num / chirp_dur - lora_bd / chirp_dur
    print(max_freq_bin_num, freq_error, chirp_dur / max_freq_bin_num)
    freq_change_interval = chirp_dur / max_freq_bin_num
    freq_change_interval_001us = freq_change_interval * 100
    freq_change_interval_001us_dec = int(np.around((freq_change_interval_001us - np.floor(freq_change_interval_001us)) * chirp_time_acc))
    freq_change_interval_001us = int(freq_change_interval_001us)
    
    if LOG_LEVEL > 0:
        print("---------------------------------------------------------")
        print("max_freq_bin_num:\t\t", max_freq_bin_num)
        print("freq_error:\t\t\t", freq_error)
        print("skip_num:\t\t\t", skip_num)
        print("chirp_dur:\t\t\t", chirp_dur)
        print("chirp_time_acc:\t\t\t", chirp_time_acc)
        print("freq_change_interval:\t\t", freq_change_interval)
        print("freq_change_interval_001us:\t", freq_change_interval_001us)
        print("freq_change_interval_001us_dec:\t", freq_change_interval_001us_dec)
        # print("test_freq_change_interval_001us_dec:\t", test_freq_change_interval_001us_dec)
        print("---------------------------------------------------------")
    # exit(0)
    return max_freq_bin_num, skip_num, freq_change_interval_001us, freq_change_interval_001us_dec

# Calculate the timing for the last 0.25 down-chirp in SFD
def qua_sfd_timing():
    chirp_dur = 2 ** lora_sf * 1e6 / (lora_bd * 1000)  # in us
    chirp_dur_qua = chirp_dur / 4
    qua_freq_num = np.around(ble_freq_bin_num / 4)  # The number of frequency bin in last 0.25 SFD 
    max_freq_chg_num_qua = int(qua_freq_num)
    qua_skip_num = 1
    while chirp_dur_qua / max_freq_chg_num_qua <= frequency_change_delay:
        qua_skip_num += 1
        max_freq_chg_num_qua = int(np.around((max_freq_chg_num_qua - 1) / qua_skip_num) + 1)
    qua_freq_change_interval = chirp_dur_qua / max_freq_chg_num_qua
    qua_freq_change_interval_001us = qua_freq_change_interval * 100
    qua_freq_change_interval_dec = int(np.around(qua_freq_change_interval_001us - np.floor(qua_freq_change_interval_001us)) * chirp_time_acc)
    qua_sfd_mdmctrl = np.zeros(max_freq_chg_num_qua, dtype=np.int16)
    _mdmctrl_all = np.arange(-(ble_freq_bin_num-1)//2 + 1, (ble_freq_bin_num-1)//2+2, dtype=np.int8)
    # print(_mdmctrl_all)
    mdmctrl_all_bit6 = int_to_bit6(_mdmctrl_all)

    for i in range(max_freq_chg_num_qua):
        qua_sfd_mdmctrl[i] = _mdmctrl_all[ble_freq_bin_num - 1 - qua_skip_num * i]
    qua_sfd_mdmctrl_bit6 = int_to_bit6(qua_sfd_mdmctrl)
    qua_sfd_mdmctrl_seq = np.zeros(max_freq_chg_num_qua, dtype=np.uint16)
    for i in range(max_freq_chg_num_qua):
        qua_sfd_mdmctrl_seq[i] = base_val | (((qua_sfd_mdmctrl_bit6[i]) & 0x3f) << 7)
    qua_freq_change_interval_001us = int(qua_freq_change_interval_001us)
    return max_freq_chg_num_qua, qua_freq_change_interval_001us, qua_freq_change_interval_dec, qua_sfd_mdmctrl_seq

# Calculate the timing for transmit chirps
def frequency_change_sequence(max_freq_change_num, mdmctrl_all_bit6, best_ble_freq_idx, skip_num):
    # (mdmctrl_all[(best_freq_idx[start_freq[cur_chirp]] + SKIP_NUM * cur_freq_bin) % FREQ_BIN_NUM] & 0x3f) << 7
    mdmctrl_seq = np.zeros([ble_bbin_num+1, max_freq_change_num], dtype=np.uint16)
    for i in range(ble_bbin_num):
        for j in range(max_freq_change_num):
            mdmctrl_seq[i, j] = base_val | (((mdmctrl_all_bit6[(best_ble_freq_idx[i] + skip_num * j) % (len(mdmctrl_all_bit6) - 1)]) & 0x3f) << 7)
    # Then we calculate the sequence of the down-chirp for SFD
    sfd_start_mdmctrl_idx = int(np.around((ble_freq_bin_num - 1) / skip_num) * skip_num) + 1
    # print(sfd_start_mdmctrl_idx, len(mdmctrl_all_bit6), mdmctrl_seq.shape)
    # exit(0)
    for j in range(max_freq_change_num):
        mdmctrl_seq[-1, j] = base_val | (((mdmctrl_all_bit6[sfd_start_mdmctrl_idx -(skip_num * j)]) & 0x3f) << 7)
    return mdmctrl_seq

def int_to_bit6(num):
    val = np.zeros(len(num), dtype=np.uint8)
    for i in range(len(num)):
        if (num[i] < 0):
            val[i] |= 1 << 5
        # print(i)
        val[i] |= num[i] & 0x1f
    return val

# Map the BLE frequency to the LoRa frequency, according to the minimum frequency difference 
def frequency_mapping_mindis():
    lora_frequency_bin = np.arange(-lora_fbn//2, lora_fbn//2)
    # Ignore the last frequency, which does not carry information
    ble_frequencies = mdmctrl_all[:-1] * 15.625
    ble_frequencies += (-lora_bd / 2) - ble_frequencies[0]  # Consider the CFO compensation
    lora_frequencies = lora_frequency_bin * (lora_bd / lora_fbn)
    freq_diff = np.abs(ble_frequencies[:, np.newaxis] - lora_frequencies)
    freq_diff_sort = np.argsort(freq_diff, axis=1)
    min_diff_arg = freq_diff_sort[:, 0]
    min_diff = freq_diff[np.arange(freq_diff.shape[0]), min_diff_arg]
    for i in range(len(min_diff)):
        print(mdmctrl[i], min_diff[i])
    plt.figure()
    plt.plot(mdmctrl, min_diff)
    plt.xlabel("BLE frequency bin")
    plt.ylabel("Min. frequency difference")
    plt.show()

    # We choose the ble_bbin_num of ble frequency bins that has the minimum distance with the native LoRa bins
    best_ble_freq_idx = np.argsort(min_diff)[:ble_bbin_num] 
    ble_frequencies = ble_frequencies[best_ble_freq_idx]
    # Then, we sort the frequency bins
    ble_freq_increase_idx = np.argsort(ble_frequencies)
    ble_frequencies = ble_frequencies[ble_freq_increase_idx]
    best_ble_freq_idx = best_ble_freq_idx[ble_freq_increase_idx]
    mdmctrl = mdmctrl_all[best_ble_freq_idx]
    freq_diff = np.abs(lora_frequencies[:, np.newaxis] - ble_frequencies)
    min_diff = np.min(freq_diff, axis=1)
    min_diff_arg = np.argmin(freq_diff, axis=1)

    mapping = [[] for i in range(ble_bbin_num)]

    for i in range(len(lora_frequencies)):
        # This LoRa frequency bin has the same frequency difference to the two BLE bins
        if len(np.where(freq_diff[i, :]==min_diff[i])[0]) > 1:
            continue
        mapping[min_diff_arg[i]].append(i)
    lora_recv_map = np.zeros(lora_fbn, dtype=np.int16)
    lora_recv_map[:] = -1
    all_mapped = True
    for i in range(ble_bbin_num):
        lora_recv_map[mapping[i]] = i
        if len(mapping[i]) == 0:
            warnings.warn("{}th has no correspongding mapping bin".format(i), Warning)
            all_mapped = False
        if not all_mapped:
            raise Exception("Some has no mapping")
        print("----------------------")
        print(i, "==", mdmctrl[i], "->", mapping[i])
        print(ble_frequencies[i], "->", lora_frequencies[mapping[i]], np.abs(ble_frequencies[i]-lora_frequencies[mapping[i]]))

# Map the BLE frequency to the LoRa frequency, to make the BLE frequency averagely distributed in the frequency range 
def frequency_mapping_average():
    # Ignore the last frequency, which does not carry information
    # BoRa: ble_freq_bin_num = int(lora_bd // 15.625), 
    # best_ble_freq_idx = np.around(np.linspace(0, ble_freq_bin_num-2, ble_bbin_num)).astype(int)
    # Change to:
    # best_ble_freq_idx = np.around(np.linspace(0, ble_freq_bin_num-1, ble_bbin_num)).astype(int)
    best_ble_freq_idx = np.around(np.linspace(0, ble_freq_bin_num-1, ble_bbin_num)).astype(int)
    mdmctrl = mdmctrl_all[best_ble_freq_idx]
    lora_frequency_bin = np.arange(-lora_fbn//2, lora_fbn//2)
    ble_frequencies = mdmctrl * 15.625
    ble_frequencies += (-lora_bd / 2) - ble_frequencies[0]  # Considering the CFO compensation

    lora_frequencies = lora_frequency_bin * lora_fbw
    freq_diff = np.abs(lora_frequencies[:, np.newaxis] - ble_frequencies)
    min_diff = np.min(freq_diff, axis=1)
    min_diff_arg = np.argmin(freq_diff, axis=1)
    mapping = [[] for i in range(ble_bbin_num)]
    for i in range(len(lora_frequencies)):
        # This LoRa frequency bin has the same frequency difference to the two BLE bins
        if len(np.where(freq_diff[i, :]==min_diff[i])[0]) > 1:
            print("---------------------------------------------")
            print(len(np.where(freq_diff[i, :]==min_diff[i])[0]), i)
            print(list(freq_diff[i, :]))
            print(min_diff[i])
            print("---------------------------------------------")
            continue
        mapping[min_diff_arg[i]].append(i)
    i = 0
    for i in range(len(mapping)):
        if len(mapping[i]) == 0:
            print(i)
    while i < len(min_diff_arg):
        if i > 0 and (min_diff_arg[i] == 15 and min_diff_arg[i - 1] == 14):
            print("start:", i)
        if i < len(min_diff_arg) - 1 and (min_diff_arg[i] == 15 and min_diff_arg[i + 1] == 16):
            print("end:", i)
            break
        i += 1
            
    print(list(min_diff_arg))

    lora_recv_map = np.zeros(lora_fbn, dtype=np.int16)
    lora_recv_map[:] = -1
    all_mapped = True
    
    for i in range(ble_bbin_num):
        lora_recv_map[mapping[i]] = i
        if len(mapping[i]) == 0:
            print(i, mapping[i])
            warnings.warn("{}th has no correspongding mapping bin".format(i), Warning)
            all_mapped = False
        if not all_mapped:
            raise Exception("Some has no mapping")
        print("----------------------")
        print(i, ble_frequencies[i], mapping[i][np.argmin(np.abs(ble_frequencies[i]-lora_frequencies[mapping[i]]))])
    
        print(i, "==", mdmctrl[i], "->", mapping[i])
        print(ble_frequencies[i], "->", lora_frequencies[mapping[i]], np.abs(ble_frequencies[i]-lora_frequencies[mapping[i]]))
    return ble_bbin_num, best_ble_freq_idx, lora_recv_map

def to_headerfile(ble_bbin_num, best_ble_freq_idx, lora_recv_map):
    """Generate the Ubertooth One header"""
    ubertooth_header = "#include <stdlib.h>\n\n"
    _mdmctrl_all = np.arange(-(ble_freq_bin_num-1)//2, (ble_freq_bin_num-1)//2+2, dtype=np.int8)
    # print(_mdmctrl_all)
    mdmctrl_all_bit6 = int_to_bit6(_mdmctrl_all)
    # print(mdmctrl_all_bit6, _mdmctrl_all)
    # exit(0)
    # ubertooth_header += "uint8_t mdmctrl_all[{}] = ".format(len(mdmctrl_all_bit6))
    # ubertooth_header += "{"
    # for i in range(len(mdmctrl_all_bit6)):
    #     ubertooth_header += (str(mdmctrl_all_bit6[i]))
    #     if i != len(mdmctrl_all_bit6) - 1:
    #         ubertooth_header += ", "
    # ubertooth_header += "};\n"
    max_freq_change_num, skip_num, freq_change_interval_001us, freq_change_interval_001us_dec = frequency_change_timing()
    mdmctrl_seq = frequency_change_sequence(max_freq_change_num, mdmctrl_all_bit6, best_ble_freq_idx, skip_num)
    max_freq_chg_num_qua, qua_freq_change_interval_001us, qua_freq_change_interval_dec, qua_sfd_mdmctrl_seq = qua_sfd_timing()

    ubertooth_header += "#define MAX_BIN ({})\n".format(int(MAX_BIN))
    ubertooth_header += "#define LORA_SYMBOLS ({})\n".format(int(lora_symbols))
    ubertooth_header += "#define LORA_BD ({})\n".format(int(lora_bd))
    ubertooth_header += "#define MAX_FREQ_CHG_NUM ({})\n".format(max_freq_change_num)
    ubertooth_header += "#define SKIP_NUM ({})\n".format(skip_num)
    ubertooth_header += "#define FREQ_CHG_INT ({})\n".format(freq_change_interval_001us)
    ubertooth_header += "#define FREQ_CHG_DEC ({})\n".format(freq_change_interval_001us_dec)
    ubertooth_header += "#define CHIRP_TIME_ACC ({})\n".format(int(chirp_time_acc))
    ubertooth_header += "#define FREQ_BIN_NUM ({})\n".format(len(mdmctrl_all))
    ubertooth_header += "#define BLE_BBIN_NUM ({})\n".format(ble_bbin_num)
    ubertooth_header += "#define BASE_VAL ({})\n".format(base_val)
    ubertooth_header += "#define QUA_FREQ_CHG_INT ({})\n".format(qua_freq_change_interval_001us)
    ubertooth_header += "#define QUA_FREQ_CHG_DEC ({})\n".format(qua_freq_change_interval_dec)
    ubertooth_header += "#define QUA_MAX_FREQ_CHG_NUM ({})\n".format(max_freq_chg_num_qua)
    ubertooth_header += "#define PREAMBLE_LEN ({})\n".format(int(preamble_len))
    ubertooth_header += "#define QUA_START_LEN ({})\n".format(int(preamble_len+4))
    ubertooth_header += "#define DATA_START_LEN ({})\n".format(int(preamble_len+5))
    # Write the MDMCTRL control sequence, the last one is the down chirp in SFD
    ubertooth_header += "uint16_t mdmctrl_seq[{}][{}] = ".format(ble_bbin_num+2, max_freq_change_num)
    ubertooth_header += "{"
    for i in range(ble_bbin_num+2):
        if i >= 1:
            ubertooth_header += "\t\t\t\t\t\t\t"
        ubertooth_header += "{"
        if i < ble_bbin_num+1:
            for j in range(max_freq_change_num):
                if j != max_freq_change_num-1:
                    ubertooth_header += "{}, ".format(mdmctrl_seq[i, j])
                else:
                    ubertooth_header += "{}".format(mdmctrl_seq[i, j]) + "},\n"
        else:
            for j in range(max_freq_change_num):
                if j < max_freq_chg_num_qua:
                    ubertooth_header += "{}, ".format(qua_sfd_mdmctrl_seq[j])
                elif j != max_freq_change_num-1:
                    ubertooth_header += "0, "
                else:
                    ubertooth_header += "0}};\n"


    # Calculate the timing for the SFD
    # sfd_duration_rem = 0.25 * chirp_dur
    # sfd_freq_chg_num = 2.25 * max_freq_change_num

    # ubertooth_header += "uint16_t mdmctrl_seq_down[{}] = "


    """Start generate the LoRa node header"""
    print(lora_recv_map)
    lora_header = "int16_t lora_to_ble[{}] = ".format(len(lora_recv_map)) + "{"
    for i in range(len(lora_recv_map)):
        lora_header += str(lora_recv_map[i])
        if i != (len(lora_recv_map)-1):
            lora_header += ", "
    lora_header += "};\n"
    lora_header += "uint16_t config_lora_bd = {};\n".format(int((lora_bd // 100) * 100))
    lora_header += "uint8_t sf = {};\n".format(int(lora_sf))
    return ubertooth_header, lora_header

def write_to_file(ub_header, lora_header):
    with open(ubertooth_filepath, "w") as f:
        f.write(ub_header + "\n")
    with open(lora_filepath, "w") as f:
        f.write(lora_header + "\n")

if __name__ == "__main__":
    if args.bandwidth is not None and args.sf is not None and args.filepath is not None:
        param_init()
    init_param_cal()
    ble_bbin_num, best_ble_freq_idx, lora_recv_map = frequency_mapping_average()
    # if args.mindis:
    #     ble_bbin_num, best_ble_freq_idx, lora_recv_map = frequecy_mapping_mindis()
    # else:
    #     ble_bbin_num, best_ble_freq_idx, lora_recv_map = frequency_mapping_average()
    ub_header, lora_header = to_headerfile(ble_bbin_num, best_ble_freq_idx, lora_recv_map)
    write_to_file(ub_header, lora_header)
    a = os.popen("pwd")
    if "firmware" not in a.readlines()[0]:
        raise Exception("Not in a application directory!")
    os.makedirs("./build/", exist_ok=True)
    os.system("rm ./build/*")
    os.system("make")

##define qua_FREQ_CHG_INT 281 
#define qua_FREQ_CHG_DEC

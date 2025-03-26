
def main(sf, bw, len, path):
    header = ""
    header += "#define SPEACTOR_FACTOR ({})\n".format(int(sf))
    header += "#define BANDWIDTH ({})\n".format(int(bw))
    header += "#define PAYLOAD_LENGTH ({})\n".format(int(len))
    
    with open(path, "w") as f:
        f.write(header + "\n")
        
if __name__=="__main__":
    import argparse  
    parser = argparse.ArgumentParser(description='Process some integers.')    
    parser.add_argument('-b', '--bw', type=int, default=500, required=True, help='an integer for meter')  
    args = parser.parse_args() 
    bw = args.bw
    if bw == 800:
        sf = 8
        len = 24
    elif bw == 400:
        sf = 7    
        len = 21
    else:
        raise ValueError(f"NO:{bw}")
    main(sf=sf, bw=bw, len=len, path="default_config.h")


def main(sf, bw, len, path):
    header = ""
    header += "#define SPEACTOR_FACTOR ({})\n".format(int(sf))
    header += "#define BANDWIDTH ({})\n".format(int(bw))
    header += "#define PAYLOAD_LENGTH ({})\n".format(int(len))
    if int(sf) == 8:
        header += "#define SF_8"
    with open(path, "w") as f:
        f.write(header + "\n")
        
if __name__=="__main__":
    import argparse  
    parser = argparse.ArgumentParser(description='Process some integers.')    
    parser.add_argument('-b', '--bw', type=int, default=500, required=True, help='an integer for meter')
    parser.add_argument('-s', '--sf', type=int, default=7, help='an integer for meter')  
      
    args = parser.parse_args() 
    bw = args.bw
    if bw == 800:
        sf = 8
        len = 24
    elif bw == 400:
        sf = args.sf
    elif bw == 200:
        sf = args.sf
    if sf == 8:
        len = 24
    elif sf == 7:
        len = 21
    elif sf == 6:
        len = 18
    elif sf == 9:
        len = 27
    main(sf=sf, bw=bw, len=len, path="default_config.h")

import argparse
from telnetlib import Telnet
from time import sleep

def auto_int(x):
    return int(x, 0)

def divisible_by_64(x):
    x = int(x, 0)
    if x % 64 != 0:
        raise argparse.ArgumentTypeError("The value 0x{:X} is not divisible by 64".format(x))
    return x

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("start", type=divisible_by_64, help="start address to dump")
    parser.add_argument("size", type=divisible_by_64, help="size to dump")
    parser.add_argument("file", help="output file")
    parser.add_argument("--openocd", required=True, help="host:port of openocd telnet")

    args = parser.parse_args()
    host, port = args.openocd.split(":")
    port = int(port)

    # open input file
    f = open(args.file, "rb")

    c = 0
    tn = Telnet(host, port)
    tn.read_until(b"> ")
    tn.write(b"mww 0x40062000 0x5AFA0000\n")
    tn.read_until(b"> ")
    tn.write(b"mww 0x40062008 1\n")
    tn.read_until(b"> ")    
    cmd = "mww 0x40062010 {}\n".format(args.start);
    print("START ADDRESS", cmd)
    tn.write(cmd.encode("ascii"))
    tn.read_until(b"> ")
    
    while (c < args.size):
        # write 4-byte each time
        val = int.from_bytes(f.read(4), "little")        
        cmd = "mww 0x4006200c {}\n".format(hex(val))
        print(hex(c), cmd.rstrip())
        tn.write(cmd.encode("ascii"))
        sleep(0.1)
        
        c += 4
        
        # run the flash and change the address after each page complete
        if (c and (c % 64 == 0)):
            print("flash page ", c)        
            tn.write(b"mww 0x40062008 0x41\n")            
            sleep(0.2)
            tn.write(b"mww 0x40062008 1\n")
            tn.read_until(b"> ")
            # change address to the next page
            cmd = "mww 0x40062010 {}\n".format(c + int(args.start));
            # print("NEXT PAGE", cmd)
            tn.write(cmd.encode("ascii"))
            sleep(0.1)

    print("flashing end")
    tn.write(b"mdw 0x40062004\n")
    sleep(0.5)                    

    f.close()

if __name__ == "__main__":
    main()
import hid

while True:
    hi = hid.device()

    for dev in hid.enumerate():
        #
        # Change your device and CONSOLE interface here
        #
        if dev['product_string'] == 'GMMK FULL' and dev['interface_number'] == 2:
            path = dev['path']
            # print(dev, path)
            break

        # open again if failed (keyboard reboot or hang)
        try:
            hi.open_path(path);
        except:
            next
        
        # busy reading, open console again if failed
        while True: 
            try:
                d = hi.read(16, 0)
            except:
                break

            byte_str = ''.join(chr(n) for n in d)
            byte_str = byte_str.split('\x00', 1)[0]
            print(byte_str, end = '')

hi.close()
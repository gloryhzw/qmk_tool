import hid, time

hi = hid.device()

for dev in hid.enumerate():
    if dev['usage'] == 97:
        path = dev['path']
        print(dev, path)
        break

print(hi.open_path(path))

# 1 64-packet can set 20 leds
num_step = 20
# change total led here, in my case it is 104 FULL keys
total_leds = 104

h = [0] * 65

# switch to direct mode
h[1] = 7
h[5] = 1
hi.write(h)

# loop direct led frames
h[1] = 9

loops = 0

while (True):
    c = 0

    if loops & 1:
        r = 0
        g = 0
        b = 0
    else:
        r = 255
        g = 255
        b = 0
    
    loops += 1
    
    while (c < total_leds):
        h[2] = c
        h[3] = num_step

        # the last packet setting all white 
        if c == 100:
            b = g        

        for i in range(0, num_step):
            h[i * 3 + 4] = r
            h[i * 3 + 4 + 1] = g
            h[i * 3 + 4 + 2] = b          
        c += num_step
        if (hi.write(h) == -1):
            print("error")
            exit(1)
        # time.sleep(0.001)

    print(loops)
    time.sleep(0.5)

hi.close()

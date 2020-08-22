import struct

def excmd():
    log = open("log", "rb").read()
    # Display();
    # cmd_swap();
    # cmd_loadidentity();
    # cmd_dlstart();
    bs = struct.pack("4I", 0, 0xffffff01, 0xffffff26, 0xffffff00)
    frame = 0
    while log:
        if not bs in log:
            p = len(log)
        else:
            p = log.index(bs) + len(bs)
        if p > 20:
            with open("%04d.cmd" % frame, "wb") as f:
                f.write(log[:p])
            frame += 1
        log = log[p:]
    print(frame, 'frames dumped')

excmd()

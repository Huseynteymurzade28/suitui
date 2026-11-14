#!/usr/bin/env python3
"""tools/record.py out.cast [-s 60x18] KEYS... -- CMD...

KEYS are escaped strings like '\\x1b[A', @N to wait N ms,
or =COLSxROWS to resize the terminal.
"""

import codecs, fcntl, json, os, pty, select, struct, sys, termios, time


def main():
    argv = sys.argv[1:]
    sep = argv.index("--")
    opts, cmd = argv[:sep], argv[sep + 1:]
    out = opts.pop(0)
    cols, rows = 60, 18
    if opts and opts[0] == "-s":
        cols, rows = map(int, opts[1].split("x"))
        opts = opts[2:]

    pid, fd = pty.fork()
    if pid == 0:
        os.execvp(cmd[0], cmd)
    start = time.time()
    events = []

    def resize(c, r):
        fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", r, c, 0, 0))
        if events:
            events.append([round(time.time() - start, 4), "r", f"{c}x{r}"])

    resize(cols, rows)
    utf8 = codecs.getincrementaldecoder("utf-8")("replace")

    def pump(ms):
        end = time.time() + ms / 1000
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.02)
            if not r:
                continue
            try:
                data = os.read(fd, 65536)
            except OSError:
                return
            if not data:
                return
            events.append([round(time.time() - start, 4), "o",
                           utf8.decode(data)])

    pump(800)
    for k in opts:
        if k.startswith("@"):
            pump(int(k[1:]))
            continue
        if k.startswith("=") and "x" in k:
            resize(*map(int, k[1:].split("x")))
            pump(400)
            continue
        if k.isascii():
            k = k.encode().decode("unicode_escape")
            os.write(fd, k.encode("latin1"))
        else:
            os.write(fd, k.encode())
        pump(400)
    pump(300)

    with open(out, "w") as f:
        f.write(json.dumps({"version": 2, "width": cols, "height": rows}) + "\n")
        for e in events:
            if e[1] == "o" and "\x1b[?1049l" in e[2]:
                break
            f.write(json.dumps(e) + "\n")


if __name__ == "__main__":
    main()

#!/bin/env python3

import os
import sys
import tty
import termios

ansiiprint = lambda *args: print(*args, end="", flush=True)

def isprj(f):
    if not os.path.isdir(f):
        return False
    if not os.path.isfile(f + "/shader.glsl"):
        return False
    return True

original_settings = termios.tcgetattr(sys.stdin)
new_settings = termios.tcgetattr(sys.stdin)
new_settings[3] &= ~termios.ICANON|termios.ISIG
new_settings[3] &= ~termios.ECHO 
termios.tcsetattr(sys.stdin, termios.TCSAFLUSH, new_settings)
ansiiprint("\x1b[?1049h\x1b[?25l\x1b[2J\x1b[H")

try:
    sel = 0
    while sel != "BYE":
        prjs = os.listdir()
        prjs = tuple(filter(isprj, prjs))
        if sel < 0: sel = len(prjs) - 1
        elif sel > len(prjs) - 1: sel = 0
        os.popen("killall glsl -q;./glsl \"%s\" &" % prjs[sel], "r", 1)
        ansiiprint("\x1b[2J\x1b[H")
        for i, prj in enumerate(prjs):
            if i == sel:
                ansiiprint("\x1b[7m")
            print(f"{i + 1}: {prj}")
            ansiiprint("\x1b[27m")
        print()
        while True:
            char = sys.stdin.read(1)
            if char == "w":
                sel -= 1
            elif char == "s":
                sel += 1
            elif char == "r":
                pass
            elif char == "\x03":
                sel = "BYE"
            else:
                continue
            break
finally:
    os.system("killall glsl")
    ansiiprint("\x1b[2J\x1b[H\x1b[?25h\x1b[?1049l")
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, original_settings)
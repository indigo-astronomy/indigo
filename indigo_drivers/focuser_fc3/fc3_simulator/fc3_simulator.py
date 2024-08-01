# PegasusAstro FocusCube simulator
#
# Copyright (c) 2024 CloudMakers, s. r. o.
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# https:#pegasusastro.com/command-list-for-focuscube3/

import curses
import pty
import os
import threading
import time

version = 3
position = 0
target = 0
direction = 0
backlash = 3
speed = 400

def sim_read():
	global bottom
	global master
	global lock
	
	line = []
	while True:
		char = os.read(master, 1).decode()
		if char == "\n":
			break
		line.append(char)
	result = "".join(line)
	with lock:
		bottom.addstr(" -> " + result + "\n")
		bottom.refresh()
	return result

def sim_write(message):
	global bottom
	global master
	global lock
	
	with lock:
		bottom.addstr(" <- " + message + "\n")
		bottom.refresh()
	os.write(master, (message + "\n").encode())

def sim_protocol():
	global version
	global position
	global target
	global direction
	global backlash
	global speed
	
	while True:
		command = sim_read()
		if command == "##" or command == "P#":
			sim_write("F3C_AA000000_A")
		elif command == "FA":
			sim_write("FC3:%d:%d:23.8:%d:%d" % (position, 0 if target == position else 1, direction, backlash))
		elif command == "FH":
			target = position
			sim_write("FH:1")
		elif command == "FT":
			sim_write("FT:23.8")
		elif command == "FI":
			sim_write("FI:%d" % (0 if target == position else 1))
		elif command == "FV":
			sim_write("FV:1.4.1")
		elif command == "SP":
			sim_write("SP:%d" % (speed))
		elif command.startswith("FN:"):
			target = int(command[3:])
			position = target
			sim_write(command)
		elif command.startswith("FM:"):
			target = int(command[3:])
			sim_write(command)
		elif command.startswith("FG:"):
			target += int(command[3:])
			sim_write(command)
		elif command.startswith("FD:"):
			direction = int(command[3:])
			sim_write(command)
		elif command.startswith("SP:"):
			speed = int(command[3:])
			sim_write(command)
		elif command.startswith("BL:"):
			backlash = int(command[3:])
			sim_write(command)

def sim_hardware():
	global position
	global target
	global direction
	global backlash
	global speed
	global top

	while True:
		if target < position:
			position -= 1
		elif target > position:
			position += 1
		with lock:
			top.addstr(1, 2, "position:  %6d" % (position))
			top.addstr(2, 2, "target:    %6d" % (target))
			top.addstr(3, 2, "direction: %6d" % (direction))
			top.addstr(4, 2, "backlash:  %6d" % (backlash))
			top.addstr(5, 2, "speed:     %6d" % (speed))
			top.refresh()
		time.sleep(0.1)


def main(stdscr):
	global version
	global top
	global bottom
	global master
	global lock
	
	master, slave = pty.openpty()

	stdscr.clear()
	curses.cbreak()
	curses.curs_set(0)
	height, width = stdscr.getmaxyx()
	top = curses.newwin(7, width)
	top.border()
	top.addstr(0, 2, " PegasusAstro FocusCube v%d simulator running on %s " % (version, os.ttyname(slave)), curses.A_BOLD)
	top.addstr(6, width - 20, " Press Q to exit ")
	top.refresh()
	bottom = curses.newwin(height - 7, width, 7, 0)
	bottom.scrollok(True)
	bottom.refresh()

	lock = threading.Lock()

	threading.Thread(target = sim_hardware, daemon = True).start()
	threading.Thread(target = sim_protocol, daemon = True).start()

	while True:
		if top.getch() == ord('Q'):
			break

curses.wrapper(main)


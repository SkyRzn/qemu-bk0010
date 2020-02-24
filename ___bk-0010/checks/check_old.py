#!/usr/bin/python3


import sys, os


def save_reg(res, val, reg):
	val = int(val, 16)
	res[int(reg)] = val & 0xffff
	if int(reg) == 2:
		res[int(reg)] = 0

def get_regs(line):
	res = {}
	line = line.split()
	for field in line:
		if field.startswith('r') and len(field) > 5 and field[2] == '=':
			save_reg(res, field.split('=')[1], field[1])
		if field.endswith(':'):
			save_reg(res, field[:-1], 7)
	return res

def get_flags(line):
	res = set()
	flags = line.split(':')[0]
	for k in 'NZVC':
		if k in flags:
			res.add(k)
	return res

def parse(lines):
	res = []
	prev = None

	for line in lines:
		line = line.strip()

		if line[0].startswith("Init"):
			line.pop(0)
		if line[0].startswith("SDL"):
			line.pop(0)

		if prev and prev == line:
			continue

		regs = get_regs(line)
		flags = get_flags(line)
		res.append((regs, flags))
	return res

def load():
	with open(sys.argv[1], 'r') as f:
		my = parse(f.readlines())

	with open(sys.argv[2], 'r') as f:
		emu = parse(f.readlines())

	return (my, emu)

def main():
	my_all, emu_all = load()

	my_handled = []
	emu_handled = []

	LINES = 5
	i = 0
	for my, emu in zip(my_all, emu_all):

		myregs = get_regs(my)
		emuregs = get_regs(emu)

		myflags = get_flags((my))
		emuflags = get_flags((emu))

		if my != emu:
			print('----------- %d --- %d' % (i+1))
			for i in range(LINES):
				print('%s' % (my_handled[i-LINES]))
			print('')
			for i in range(LINES):
				print('%s' % (emu_handled[i-LINES]))

			break

		i += 1

main()

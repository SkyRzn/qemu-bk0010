#!/usr/bin/python3


from operation import Oplist
import sys, os

START = 0xccee
#START = 0xd114

def main():
	myList = Oplist('000_new', 'MY', START)
	print('MY: loaded %d lines' % myList.srcLen())
	emList = Oplist('000_yoba', 'EM', START)
	print('EM: loaded %d lines' % emList.srcLen())

	LINES = 5
	i = 0

	for my, em in zip(myList, emList):
		if not my:
			print('MY ended: %d' % len(myList))
			break
		if not em:
			print('EM ended: %d' % len(emList))
			break

		if my != em:
			if myList[i].regs(7) == 0x84a4:
				i += 1
				continue
			print('----------- my=%d' % (len(myList)+myList.skipped()))
			for j in range(LINES):
				n = i - LINES + j + 1
				print('%d: %s' % (myList.srcInd(n)+myList.skipped(), myList[n].line()))
			print('')
			print('----------- em=%d' % (len(emList)+emList.skipped()))
			for j in range(LINES):
				n = i - LINES + j + 1
				print('%d: %s' % (emList.srcInd(n)+emList.skipped(), emList[n].line()))
			break
		i += 1

main()

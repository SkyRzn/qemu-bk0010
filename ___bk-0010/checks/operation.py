#!/usr/bin/python3


import sys, os


class Operation:
	def __init__(self, line):
		self.load(line)

	def __eq__(self, other):
		if not hasattr(other, '_regs'):
			return NotImplemented
		return (self._regs == other._regs and self._flags == other._flags)

	def __ne__(self, other):
		if not hasattr(other, '_regs'):
			return NotImplemented
		return (self._regs != other._regs or self._flags != other._flags)

	def load(self, line):
		self._line = line
		self._regs = {}
		self._flags = set()
		self._line = line

		line = line.split()

		flags = line.pop(0)
		for c in 'NZVC':
			if c in flags:
				self._flags.add(c)

		for field in line:
			if field.startswith('r') and len(field) > 5 and field[2] == '=':
				self._saveReg(field.split('=')[1], field[1])
			if field.endswith(':'):
				self._saveReg(field[:-1], 7)

	def _saveReg(self, val, reg):
		val = int(val, 16)
		self._regs[int(reg)] = val & 0xffff
		if int(reg) == 2:
			self._regs[int(reg)] = 0 # TEST  incorrect reference emulator behaviour

	def regs(self, reg=None):
		if reg == None:
			return self._regs
		return self._regs[reg]

	def flags(self):
		return self._flags

	def line(self):
		return self._line

class Oplist:
	def __init__(self, fn, name, startOpcode=None, startOffset=0):
		self._name = name
		self._lines = []
		self._skipped = 0

		self.__iter__() # reset
		self.load(fn, startOpcode, startOffset)

	def load(self, fn, startOpcode=None, startOffset=0):
		with open(fn, 'r') as f:
			self._lines = f.readlines()
		if startOpcode:
			for i, line in enumerate(self._lines):
				if (' %x: ' % startOpcode) in line:
					print('%s: skipped %d lines' % (self._name, i))
					self._lines = self._lines[i:]
					self._skipped = i
					return

		elif startOffset > 0:
			print('%s: skipped %d lines' % (self._name, startOffset))
			self._lines = self._lines[startOffset:]
			self._skipped = startOffset

	def __iter__(self):
		self._offset = 0
		self._cnt = 0
		self._prev = None
		self._cache = []
		return self

	def __next__(self):
		while True:
			try:
				line = self._lines[self.srcInd(self._cnt)]
			except:
				return None

			line = line.strip()

			if line.startswith("Init") or line.startswith("SDL"):
				self._offset += 1
				continue

			if self._prev and self._prev == line:
				self._offset += 1
				continue

			self._prev = line
			self._cnt += 1

			op = Operation(line)

			self._cache.append(op)

			return op

	def __getitem__(self, ind):
		if ind >= self._cnt:
			return None
		return self._cache[ind]

	def __len__(self):
		return self._cnt

	def srcInd(self, ind):
		return ind + self._offset

	def srcLen(self):
		return len(self._lines)

	def skipped(self):
		return self._skipped + self._offset


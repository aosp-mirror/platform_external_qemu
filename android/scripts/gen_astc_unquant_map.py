def getRange(tritOrQuint, bits):
  mul = 3 if tritOrQuint else 5
  return (1 << bits) * mul

def getUnquantizedTritColor(val, numBits):
  bitRange = 1 << numBits
  trit = val // bitRange
  bits = val % bitRange
  a = 0x1FF if (bits & 1) != 0 else 0
  b = 0
  c = 0
  if numBits == 1:
    b = 0
    c = 204
  elif numBits == 2:
    x = (bits >> 1) & 0x1
    b = (x << 1) | (x << 2) | (x << 4) | (x << 8)
    c = 93
  elif numBits == 3:
    x = (bits >> 1) & 0x3
    b = x | (x << 2) | (x << 7)
    c = 44
  elif numBits == 4:
    x = (bits >> 1) & 0x7
    b = x | (x << 6)
    c = 22
  elif numBits == 5:
    x = (bits >> 1) & 0xF
    b = (x >> 2) | (x << 5)
    c = 11
  elif numBits == 6:
    x = (bits >> 1) & 0x1F
    b = (x >> 4) | (x << 4)
    c = 5

  t = trit * c + b
  t ^= a
  t = (a & 0x80) | (t >> 2)
  return t

def getUnquantizedQuintColor(val, numBits):
  bitRange = 1 << numBits
  quint = val // bitRange
  bits = val % bitRange
  a = 0x1FF if (bits & 1) != 0 else 0
  b = 0
  c = 0
  if numBits == 1:
    b = 0
    c = 113
  elif numBits == 2:
    x = (bits >> 1) & 0x1
    b = (x << 2) | (x << 3) | (x << 8)
    c = 54
  elif numBits == 3:
    x = (bits >> 1) & 0x3
    b = (x >> 1) | (x << 1) | (x << 7)
    c = 26
  elif numBits == 4:
    x = (bits >> 1) & 0x7
    b = (x >> 1) | (x << 6)
    c = 13
  elif numBits == 5:
    x = (bits >> 1) & 0xF
    b = (x >> 3) | (x << 5)
    c = 6

  t = quint * c + b
  t ^= a
  t = (a & 0x80) | (t >> 2)
  return t

def getUnquantizedTritWeight(val, numBits):
  bitRange = 1 << numBits
  trit = val // bitRange
  bits = val % bitRange
  a = 0x7F if (bits & 1) != 0 else 0
  b = 0
  c = 0
  if numBits == 0:
    if trit == 0:
      return 0
    elif trit == 1:
      return 32
    else:
      return 63
  elif numBits == 1:
    c = 50
    b = 0
  elif numBits == 2:
      c = 23
      b = (bits >> 1) & 1
      b |= (b << 2) | (b << 6)
  elif numBits == 3:
      c = 11
      b = (bits >> 1) & 0x3
      b |= (b << 5)
  else:
    raise Exception('Unsupported num of bits %d' % numBits)

  t = trit * c + b
  t ^= a
  t = (a & 0x20) | (t >> 2)
  return t

def getUnquantizedQuintWeight(val, numBits):
  bitRange = 1 << numBits
  quint = val // bitRange
  bits = val % bitRange
  a = 0x7F if (bits & 1) != 0 else 0
  b = 0
  c = 0
  if numBits == 0:
    if quint == 0:
      return 0
    elif quint == 1:
      return 16
    elif quint == 2:
      return 32
    elif quint == 3:
      return 47
    else:
      return 63
  elif numBits == 1:
    c = 28
    b = 0
  elif numBits == 2:
    c = 13
    b = (bits >> 1) & 0x1
    b = (b << 1) | (b << 6)
  else:
    raise Exception('Unsupported num of bits %d' % numBits)

  t = quint * c + b
  t ^= a
  t = (a & 0x20) | (t >> 2)
  return t

def genTable(tritOrQuint, maxBits, name):
  tritOrQuintStr = "Trit" if tritOrQuint else "Quint"
  ret = "const uint kUnquant%s%sMapBitIdx[%d] = {\n" \
      % (tritOrQuintStr, name, maxBits + 1)
  ret += " " * 3
  totalEntries = 0
  for i in range(0, maxBits + 1):
    ret += " %d," % totalEntries
    totalEntries += getRange(tritOrQuint, i)
  ret += "\n};\n\n"
  ret += "const uint kUnquant%s%sMap[%d] = {\n" \
      % (tritOrQuintStr, name, totalEntries)
  for i in range(0, maxBits + 1):
    ret += " " * 3
    for val in range(0, getRange(tritOrQuint, i)):
      if tritOrQuint:
        if name == "Weight":
          unquant = getUnquantizedTritWeight(val, i)
        else:
          unquant = getUnquantizedTritColor(val, i)
      else:
        if name == "Weight":
          unquant = getUnquantizedQuintWeight(val, i)
        else:
          unquant = getUnquantizedQuintColor(val, i)
      ret += " %d," % unquant
    ret += "\n"
  ret += "};\n\n"
  return ret

def genAllTables():
  ret = genTable(True, 3, "Weight")
  ret += genTable(False, 2, "Weight")
  ret += genTable(True, 6, "Color")
  ret += genTable(False, 5, "Color")
  return ret

def main():
  with open("Output.txt", "w") as text_file:
    text_file.write("%s" % genAllTables())

if __name__ == "__main__":
    main()
char convertToLetter(unsigned long decimal) {
  char num = decimal & 0b1111;
  switch (num) {
    case 0b1000: return 'D';
    case 0b0100: return 'C';
    case 0b0010: return 'B';
    case 0b0001: return 'A';
    default: return '?';  // return a default value for numbers not in the list
  }
}

unsigned long convertToRemote(unsigned long decimal) {
  if (decimal > 0b1111)
    return decimal - (decimal & 0b1111);
  else
    return decimal;
}

static char* dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[64];
  unsigned int i = 0;

  while (Dec > 0) {
    bin[32 + i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j < bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[31 + i - (j - (bitLength - i))];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';

  return bin;
}

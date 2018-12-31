#pragma once

char *ftoa(char *buffer, double d, int precision) 
{
  long wholePart = (long) d;

  // Deposit the whole part of the number.

  itoa(wholePart,buffer,10);

  // Now work on the faction if we need one.

  if (precision > 0) {

    // We do, so locate the end of the string and insert
    // a decimal point.

    char *endOfString = buffer;
    while (*endOfString != '\0') endOfString++;
    *endOfString++ = '.';

    // Now work on the fraction, be sure to turn any negative
    // values positive.

    if (d < 0) {
      d *= -1;
      wholePart *= -1;
    }
    
    double fraction = d - wholePart;
    while (precision > 0) {

      // Multipleby ten and pull out the digit.

      fraction *= 10;
      wholePart = (long) fraction;
      *endOfString++ = '0' + wholePart;

      // Update the fraction and move on to the
      // next digit.

      fraction -= wholePart;
      precision--;
    }

    // Terminate the string.

    *endOfString = '\0';
  }

   return buffer;
}

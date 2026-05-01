#include "num2words-en.h"
#include "string.h"
#include "config.h"

#define true 1
#define false 0


static const char* const ONES[] = {
  "o'clock",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine"
};

static const char* const TEENS[] ={
  "",
  "eleven",
  "twelve",
  "thirteen",
  "fourteen",
  "fifteen",
  "sixteen",
  "seventeen",
  "eighteen",
  "nineteen"
};

static const char* const TENS[] = {
  "oh",
  "ten",
  "twenty",
  "thirty",
  "forty",
  "fifty",
  "sixty",
  "seventy",
  "eighty",
  "ninety"
};

static size_t append_string(char* buffer, const size_t remaining, const char* str) {
  size_t prev_len = strlen(buffer);
  if (remaining > 1) {
    strncat(buffer, str, remaining - 1);
  }
  return strlen(buffer) - prev_len;
}

static size_t append_number(char* words, size_t remaining, int num, short oh) {
  int tens_val = num / 10 % 10;
  int ones_val = num % 10;

  size_t len = 0;

  if (tens_val == 1 && num != 10) {
    return append_string(words, remaining, TEENS[ones_val]);
  }
  if ((num != 0) && ((tens_val != 0) || oh)) {
    size_t written = append_string(words, remaining, TENS[tens_val]);
    len += written;
    remaining -= written;
    if (ones_val > 0) {
      written = append_string(words, remaining, " ");
      len += written;
      remaining -= written;
    }
  }

  if (ones_val > 0 || num == 0) {
    len += append_string(words, remaining, ONES[ones_val]);
  }
  return len;
}


void time_to_words(int hours, int minutes, char* words, size_t length) {

  size_t remaining = length;
  memset(words, 0, length);

  if (hours == 0 || hours == 12) {
    append_string(words, remaining, TEENS[2]);
  } else {
    append_number(words, remaining, hours % 12, false);
  }

  remaining = length - strlen(words);
  append_string(words, remaining, " ");
  remaining = length - strlen(words);
  append_number(words, remaining, minutes, TimeRenderOh);
  remaining = length - strlen(words);
  append_string(words, remaining, " ");
}

void time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length)
{
  char value[BUFFER_SIZE];
  time_to_words(hours, minutes, value, length);

  memset(line1, 0, length);
  memset(line2, 0, length);
  memset(line3, 0, length);

  char *start = value;
  char *pch = strstr (start, " ");
  while (pch != NULL) {
    if (line1[0] == 0) {
      memcpy(line1, start, pch-start);
    }  else if (line2[0] == 0) {
      memcpy(line2, start, pch-start);
    } else if (line3[0] == 0) {
      memcpy(line3, start, pch-start);
    }
    start += pch-start+1;
    pch = strstr(start, " ");
  }

  // Truncate long teen values
  #ifdef SPLIT_TEENS
  if (strstr(line2, "teen") != 0) {
    if (!((strstr(line2, "thir") != 0) ||
          (strstr(line2, "fif") != 0) ||
          (strstr(line2, "six") != 0))) {
      char *pch = strstr(line2, "teen");
      if (pch) {
        memcpy(line3, pch, 4);
        pch[0] = 0;
      }
    }
  }
  #endif
}
#include "num2words-en.h"
#include "string.h"
#include <stdbool.h>

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
  "eightteen",
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

static size_t append_number(char* words, int num, bool use_oh) {
  int tens_val = num / 10 % 10;
  int ones_val = num % 10;

  size_t len = 0;

  if (tens_val == 1 && num != 10) {
    strcat(words, TEENS[ones_val]);
    return strlen(TEENS[ones_val]);
  }
  if (num != 0 && use_oh) {
    strcat(words, TENS[tens_val]);
    len += strlen(TENS[tens_val]);    
  }
  if (ones_val > 0) {
    strcat(words, " ");
    len += 1;
  }
  
  if (ones_val > 0 || num == 0) {
    strcat(words, ONES[ones_val]);
    len += strlen(ONES[ones_val]);
  }
  return len;
}

static size_t append_string(char* buffer, const size_t length, const char* str) {
  strncat(buffer, str, length);

  size_t written = strlen(str);
  return (length > written) ? written : length;
}


void time_to_words(int hours, int minutes, char* words, size_t length) {
  size_t remaining = length;
  memset(words, 0, length);

  if (hours == 0 || hours == 12) {
    remaining -= append_string(words, remaining, TEENS[2]);
  } else {
    remaining -= append_number(words, hours % 12, false);
  }

  remaining -= append_string(words, remaining, " ");
  remaining -= append_number(words, minutes, true);
  remaining -= append_string(words, remaining, " ");
}

void time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length)
{
	char value[length];
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
	if (strlen(line2) > 7) {
		pch = strstr(line2, "teen");
		if (pch) {
			memcpy(line3, pch, 4);
			pch[0] = 0;
		}
	}
}

void date_to_words(struct tm* tm, char *line, size_t length) {
  char *postfix;
  if (tm->tm_mday == 1 || tm->tm_mday == 21 || tm->tm_mday == 31) {
    postfix = "st";
  }
  else if (tm->tm_mday == 2 || tm->tm_mday == 22) {
    postfix = "nd";
  }
  else if (tm->tm_mday == 3 || tm->tm_mday == 23) {
    postfix = "rd";
  }
  else {
    postfix = "th";
  }
  int sofar = strftime(line, length, "%A, %d", tm);
  strcat(line, postfix);
  int used = sofar + strlen(postfix);
  strftime(&line[used], length-used, " %B %Y", tm);  
}
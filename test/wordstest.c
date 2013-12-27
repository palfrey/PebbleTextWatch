#include "num2words-en.h"
#include "assert.h"
#include "stdio.h"

const int BUFFER_SIZE = 44;
char line1[BUFFER_SIZE], line2[BUFFER_SIZE], line3[BUFFER_SIZE];

void doTime(int hour, int minutes) {
	time_to_3words(hour, minutes, line1, line2, line3, BUFFER_SIZE);
	printf("'%s', '%s', '%s'\n", line1, line2, line3);
}

int main() {
	doTime(4, 30);
	assert(strcmp(line1, "four") == 0);

	doTime(1, 5);
	assert(strcmp(line2, "oh") == 0);

	doTime(10, 8);
	assert(strcmp(line1, "ten") == 0);
	assert(strcmp(line2, "oh") == 0);

	return 0;
}
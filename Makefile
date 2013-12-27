test: build/words-test
	./build/words-test

build/words-test: src/num2words-en.c test/wordstest.c
	gcc -o build/words-test src/num2words-en.c test/wordstest.c -Isrc
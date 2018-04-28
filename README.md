> ## FileIndexer

	This is a file indexer under linux platform.
	It's still in development (In another word,it's an early access).

***
## Compiling Method
* This program is recommended to be compiled with Cmake
(as you can see,there's CMakeLists.),but you can use g++
as well if you like. Just notice: if you choose g++, you
will have to link libmagic.so and libhiredis.so by yourself
(you can try -lmagic and -lhiredis, that may work.).

* How to compile:

1.open the directory of the file

2.cmake .

3.make

4.That's all.You would be able to see a file names "search",that's it.

## Usage
* ./search <rootdirectory> <keyword1> <keyword2> .... <keyword n>
* ./search --buildindex <rootdirectory>
* ./search --searchbyindex <keyword1> <keyword2> .... <keyword n>
* If you want to see more details, use ./search --help to get them.

## Features
* This edition stores the searchiing results in Redis.
so if you have not installed Redis, please install it
before using.
* It can identify the MIME type of a file using magic.h,
and only PLAIN TESTS would be tested. Maybe I would develop
a version which supports filetype-changing, but not today.
So be careful:only PLAIN TESTS.
* Output: the lineserials of keywords' appearance.

## Structure
* Mainly std::vector and std::string.

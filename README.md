> ## FileIndexer

	This is a file indexer under linux platform.
	It's still in development (In another word,it's an early access).

***
## Compiling Method
* This program is recommended to be compiled with Cmake
(as you can see,there's CMakeLists.),but you can use g++
as well if you like.

* How to compile:

1.open the directory of the file

2.cmake .

3.make

4.That's all.You would be able to see a file names "search",that's it.

## Usage
* ./search <rootdirectory> <keyword1> <keyword2> .... <keyword n>

## Structure
* Mainly std::vector and std::string.

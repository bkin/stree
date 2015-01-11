all: stree
stree: src/stree.cpp
	g++ -Os -static -o stree src/stree.cpp

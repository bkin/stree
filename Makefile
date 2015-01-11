stree: src/stree.cpp
	g++ -Os -static -o stree src/stree.cpp
test: stree
	./test_stree.sh

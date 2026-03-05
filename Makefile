all: doc-sorting 

doc-sorting: docs.c
	gcc-15 -O2 docs.c -o docs -fopenmp

clean: 
	rm docs

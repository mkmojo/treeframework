all:
	mpicxx -std=c++11 ParallelFMM.cpp Common/Options.cpp -I${HOME}/usr/mpich-3.1.3/include/ -L${HOME}/usr/mpich-3.1.3/lib/
run:
	mpirun -np 10 -machine localhost ./a.out 

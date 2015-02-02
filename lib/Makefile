all:
	mpicxx -std=c++11 ParallelFMM.cpp Common/Options.cpp -I/u/qqiu/usr/mpich-3.1.3/include/ -L/u/qqiu/usr/mpich-3.1.3/lib/
run:
	mpirun -np 10 -machine localhost ./a.out 

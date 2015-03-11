import random
import sys

def generate_input(filename, num):
    with open(filename, 'a') as f:
        for num_line in range(int(num)):
            x = str(random.uniform(1, 10))
            y = str(random.uniform(1, 10))
            z = str(random.uniform(1, 10))
            mass = str(random.uniform(1, 10))
            f.write(" ".join( [x, y, z, mass]) + '\n')

def main():
    if len(sys.argv) != 3:
        print "error, must have 2 arguments"
        print "filename, n_points"
        exit(1)
    generate_input(sys.argv[1],sys.argv[2])

if __name__ == "__main__":
    main()


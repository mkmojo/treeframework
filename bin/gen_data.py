import random
import sys

def generate_input(filename, num, levels):
    with open(filename, 'w') as f:
        f.write("NDIM 3"+'\n')
        f.write("MAX_LEVEL "+levels+'\n'+'\n')
        f.write("DATA_TABLE"+'\n')
        for num_line in range(int(num)):
            x = str(random.uniform(0, 1))
            y = str(random.uniform(0, 1))
            z = str(random.uniform(0, 1))
            mass = str(random.uniform(0, 1))
            f.write(" ".join( [x, y, z, mass]) + '\n')

def main():
    if len(sys.argv) != 4:
        print "error, must have 3 arguments"
        print "filename, n_points, m_levels"
        exit(1)
    generate_input(sys.argv[1],sys.argv[2], sys.argv[3])

if __name__ == "__main__":
    main()

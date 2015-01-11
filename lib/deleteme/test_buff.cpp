#include "../MessageBuffer.h"
#include "../Common/Options.h"

using CommLayer::MessageBuffer;

template <typename U, typename V>
class some_struct{
    public:
        some_struct(int count):some_counter(count){};
        ~some_struct(){};
        void getCounter(){
            std::cout<< some_counter << std::endl;
        };
        MessageBuffer<U, V> m_comm;
    private:
        int some_counter;
};


int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &opt::rank);
    MPI_Comm_size(MPI_COMM_WORLD, &opt::numProc);
    some_struct<double, double > s(5);
    s.getCounter();
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}

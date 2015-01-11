#include "../MessageBuffer.h"
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


int main()
{
    some_struct<double, double > s(5);
    s.getCounter();
    return 0;
}

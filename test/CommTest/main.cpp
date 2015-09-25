#include<mpi.h>
#include<memory.h>
#include"../../lib/Tree.hpp"
#include"../../lib/DataUtils.hpp"
#include<sstream>

using namespace std;

class Data
{
    public:
    double x, y, z;
    Data(int x, int y, int z): x(x), y(y), z(z){}
    Data(double x, double y, double z): x(x), y(y), z(z){}
    Data():x(0), y(0), z(0){};

    ~Data(){}
    size_t getSize(){
        return sizeof(x) * 3;
    }

    void serialize(char* dest){
        size_t offset=0;
        offset += data_utils::copyData(dest+offset, &x, sizeof(x));
        offset += data_utils::copyData(dest+offset, &y, sizeof(y));
        offset += data_utils::copyData(dest+offset, &z, sizeof(z));
    }

    size_t unserialize(const char* src){
        size_t offset = 0;
        offset += data_utils::copyData(&x, src+offset, sizeof(x));
        offset += data_utils::copyData(&y, src+offset, sizeof(y));
        offset += data_utils::copyData(&z, src+offset, sizeof(z));
        return offset;
    }
    
};

template <typename T>
class Tester:public Messager<T>{
    public:
        //qqiu why written in this way
        //why need to inherit from Messager() constructor
        CommLayer<T> *comm;
        Tester() :Messager<T>(){ 
            comm = &(this->msgBuffer.msgBufferLayer);
        }

        void receive() {
            this->_pumpNetwork();
        }

        void flush() {
            this->msgBuffer.flush();
        }

        void barrier(){
            comm->barrier();
        }

        size_t getLocalBufferSize(){
            return this->localBuffer.size();
        }

        std::string dumpLocalBuffer(){
            std::stringstream ss;
            for( size_t i=0; i < this->localBuffer.size(); i++ ){
                T data = this->localBuffer[i];
                ss << data.x << " "<< data.y << " " << data.z <<endl;
            }
            return ss.str();
        }
};

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    Data d1(0.3,0.4,5.2), d2(0.1,0.2,0.4);
    // has to have the breaces to makesure destrcution is called before the finalize
    // reference: http://www.devx.com/tips/Tip/13510 
    {
        Tester<Data> tester;

        if(procRank == 0){
            tester.addToProc(1, d1);
            tester.addToProc(1, d2);
            tester.flush(); //force buffed message be sent out
            tester.barrier();
        }else{
            tester.barrier();
            tester.receive();
            // dump the content of lcoalbuffer of the receiving side
            cout << procRank << ": " << tester.dumpLocalBuffer();
        }
        cout << procRank << ": " << tester.getLocalBufferSize() << endl;
    }
    MPI_Finalize();
    return 0;
}

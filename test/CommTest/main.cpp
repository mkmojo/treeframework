#include<mpi.h>
#include<memory.h>
#include<sstream>

#include"../../lib/Tree.hpp"
#include"../../lib/DataUtils.hpp"
#include"../../comm/Message.hpp"

using namespace std;

class Data : public Message
{
    public:
        double x, y, z;
        long cellId;
        Data(double x, double y, double z): x(x), y(y), z(z), cellId(0){}
        Data():x(0), y(0), z(0), cellId(0){};
        MessageType mtype = MT_POINT;

        MessageType getType() const{
            return mtype;
        }

        size_t getNetworkSize() const{
            return sizeof(*this) + sizeof(mtype);
        }

        size_t serialize(char* dest){
            size_t offset = 0;
            offset += data_utils::copyData(dest, &mtype, sizeof(mtype));
            offset += data_utils::copyData(dest+offset, this, sizeof(*this));
            return offset;
        }

        size_t unserialize(const char* src){
            size_t offset = 0;
            offset += data_utils::copyData(&mtype, src, sizeof(mtype));
            offset += data_utils::copyData(this, src+offset, sizeof(*this));
            return offset;
        }
};


template <typename T>
class Tester:public Messager<T>{
    public:
        CommLayer *comm;
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

        std::string dumpLocalBuffer(){
            std::stringstream ss;
            for(auto &&it: this->localBuffer){
                ss << it.x << " "<< it.y << " " << it.z <<endl;
            }
            return ss.str();
        }
};

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    // has to have the curly breaces to makesure destrcution is called before the MPI_Finalize
    // reference: http://www.devx.com/tips/Tip/13510 
    {
        Tester<Data> tester;
        if(procRank == 0){
            Data d1(0.478131038, 0.083023719, 0.543180758), 
                 d2(0.396991591, 0.538753598, 0.238951817),
                 d3(0.729413654, 0.951979972, 0.21182704),
                 d4(0.711048089, 0.782658799, 0.528640619),
                 d5(0.471475992, 0.907587745, 0.907529236),
                 d6(0.237225837, 0.112680127, 0.988787918),
                 d7(0.113852497, 0.786420657, 0.270951812),
                 d8(0.022292487, 0.44344111, 0.158594184),
                 d9(0.0, 0.0, 0.0),
                 d10(0.97, 0.0, 0.0 ),
                 d11(0.0, 0.97, 0.0 ),
                 d12(0.0, 0.0, 0.97 );
            tester.addToProc(0, &d1);
            tester.addToProc(0, &d2);
            tester.addToProc(0, &d3);
            tester.addToProc(0, &d4);
            tester.addToProc(numProc - 1, &d5);
            tester.addToProc(numProc - 1, &d6);
            tester.addToProc(numProc - 1, &d7);
            tester.addToProc(numProc - 1, &d8);
            tester.flush(); //force buffed message be sent out
            tester.barrier();
        }else{
            tester.barrier();
            tester.receive();
            // dump the content of lcoalbuffer of the receiving side
            cout << procRank + ": " + tester.dumpLocalBuffer();
        }
    }

    MPI_Finalize();
    return 0;
}

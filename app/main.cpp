#include "../lib/Tree.hpp"
#include "../lib/DataUtils.hpp"
#include "../comm/Message.hpp"
#include <sstream>

class Data : public OctreePoint, public Message{
    public:
        double mass;
        MessageType mtype = MT_POINT;

        Data():mass(0.0){}
        Data(std::istringstream& ss):OctreePoint(ss){
            ss >> mass;
        }

        MessageType getType() const {
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
            offset += data_utils::copyData(&mtype, src+offset, sizeof(mtype));
            offset += data_utils::copyData(this, src+offset, sizeof(*this));
            return offset;
        }
};

NodeSet MyGenerate(const Node<Data>& mynode){
    NodeSet myset;
    std::cout << "calling my generate" << std::endl;
    return myset;
}

void MyCombine(std::vector<Data>& dataIn){
    std::cout << "calling my combine" << std::endl;
}

void MyEvolve(std::vector<Data>& dataIn){
    std::cout << "calling my evolve" << std::endl;
}

bool MyPredicate(const std::vector<Data>& dataIn){
    std::cout << "calling my predicate" << std::endl;
    return true;
}

int MyLocate(const Data& d, int depth){
    int tmp = 1;
    //if(depth == 1) return tmp;
    //int xc, yc, zc, cc;
    //double xm, ym, zm, xl, xh, yl, yh, zl, zh;
    //xl = 0.0; xh = 1.0;
    //yl = 0.0; yh = 1.0;
    //zl = 0.0; zh = 1.0;
    //for(int i = 1; i < depth; i++){
    //    tmp <<= 3;
    //    xm = xl+0.5*(xh-xl); ym = yl+0.5*(yh-yl); zm = zl+0.5*(zh-zl);
    //    xc = (d.x > xm); yc = (d.y > ym); zc = (d.z > zm);
    //    cc = (xc<<2) | (yc<<1) | zc;
    //    tmp |= cc;
    //    if(d.x > xm) xl = xm;
    //    else xh = xm;
    //    if(d.y > ym) yl = ym;
    //    else yh = ym;
    //    if(d.z > zm) zl = zm;
    //    else zh = zm;
    //}
    return tmp;
}

int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);
    {
        Tree<Data> MyTree;
        MyTree.assign(MyGenerate, MyPredicate, MyCombine, MyEvolve, MyLocate);
        MyTree.build(argv[1]);
        MyTree.print(std::cout);
        MyTree.compute();
    }
    MPI_Finalize();
}

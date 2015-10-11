#include "../lib/Tree.hpp"
#include "../lib/DataUtils.hpp"
#include <sstream>

struct Data : OctreePoint{
    double mass;

    Data():mass(0.0){}
    Data(std::istringstream& ss):OctreePoint(ss){
        ss >> mass;
    }

    size_t getSize() const{
        return sizeof(*this);
    }

    void serialize(char* dest){
        data_utils::copyData(dest, this, sizeof(*this));
    }

    size_t unserialize(const char* src){
        data_utils::copyData(this, src, sizeof(*this));
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
    Tree<Data> *MyTree=new Tree<Data>();
    MyTree->assign(MyGenerate, MyPredicate, MyCombine, MyEvolve, MyLocate);
    MyTree->build(argv[1]);
    MyTree->print(std::cout);
    /*
    MyTree.compute();
    std::cout << "calling clear" << std::endl;
    MyTree.clear();
    std::cout << "finished clear" << std::endl;
    std::cout << MyTree.getLinearTree() << std::endl;
    std::cout << MyTree.getLocalTree() << std::endl;
    */
    MPI_Finalize();
}

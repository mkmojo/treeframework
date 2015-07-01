#include "../lib/Tree.hpp"
#include <sstream>

struct Data : OctreePoint{
    double mass;
    char m_point[64]; //sl15: this needs to be changed. there should be an intermediate data type to hold the serialized version of Data

    Data():OctreePoint(),mass(0.0){}
    Data(std::istringstream& ss){
        ss >> x >> y >> z >> mass;
    }

    size_t serialize(void* dest){ //sl15: void pointer?
        std::memcpy(dest, m_point, sizeof m_point);
        //DEBUG: copy things received to data member
        std::memcpy(&x, m_point, sizeof x);
        std::memcpy(&y, m_point + sizeof(double), sizeof y);
        std::memcpy(&z, m_point + sizeof(double)*2, sizeof z);
        std::memcpy(&mass, m_point + sizeof(double)*3, sizeof mass);
        return sizeof m_point;
    }

    //copy data from incoming buffer to Point
    //roll things into struct
    size_t unserialize(const void* src)
    {
        //This is now ugly, should be done through a function
        //I may want to change it to vector and use vector to hold 
        //things together
        //sl15: I agree with the above comment. seralized data should not reside in data itself
        memcpy(m_point, src, sizeof m_point);

        memcpy(&x, m_point, sizeof x);
        memcpy(&y, m_point + sizeof(double), sizeof y);
        memcpy(&z, m_point + sizeof(double) * 2, sizeof z);
        memcpy(&mass, m_point + sizeof(double) * 3, sizeof mass);

        return sizeof m_point;
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
    if(depth == 1) return tmp;
    int xc, yc, zc, cc;
    double xm, ym, zm, xl, xh, yl, yh, zl, zh;
    xl = 0.0; xh = 1.0;
    yl = 0.0; yh = 1.0;
    zl = 0.0; zh = 1.0;
    for(int i = 1; i < depth; i++){
        tmp <<= 3;
        xm = xl+0.5*(xh-xl); ym = yl+0.5*(yh-yl); zm = zl+0.5*(zh-zl);
        xc = (d.x > xm); yc = (d.y > ym); zc = (d.z > zm);
        cc = (xc<<2) | (yc<<1) | zc;
        tmp |= cc;
        if(d.x > xm) xl = xm;
        else xh = xm;
        if(d.y > ym) yl = ym;
        else yh = ym;
        if(d.z > zm) zl = zm;
        else zh = zm;
    }
    return tmp;
}

int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);
    Tree<Data> MyTree;
    MyTree.assign(MyGenerate, MyPredicate, MyCombine, MyEvolve, MyLocate);
    MyTree.build("mytestdata.dat");
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

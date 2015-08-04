#include "../lib/TreeDef.hpp"
#include <mpi.h>
#include "gtest/gtest.h"

struct Data{
    double x, y, z, mass;
    Data(std::istringstream& ss){
        ss >> x >> y >> z >> mass;
    }
};

NodeSet TestGenerate(const Node<Data, double>& mynode){
    NodeSet myset;
    std::cout << "calling test generate" << std::endl;
    return myset;
}

void TestCombine(double& comValIn, std::vector<Data>& dataIn){
    std::cout << "calling test combine" << std::endl;
}

void TestEvolve(std::vector<Data>& dataIn, double& comValIn){
    std::cout << "calling test evolve" << std::endl;
}

bool TestPredicate(const std::vector<Data>& dataIn){
    std::cout << "calling test predicate" << std::endl;
    return true;
}

int TestLocate(const Data& d, int depth){
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


class TreeTest : public :: testing :: Test {
    protected:
        Tree <Data, double> TestTree_L1;
        Tree <Data, double> TestTree_L2;
        Tree <Data, double> TestTree_L3;
        virtual void SetUp(){
            TestTree_L1.assign(TestGenerate, TestPredicate, TestCombine, TestEvolve, TestLocate);
            TestTree_L2.assign(TestGenerate, TestPredicate, TestCombine, TestEvolve, TestLocate);
            TestTree_L3.assign(TestGenerate, TestPredicate, TestCombine, TestEvolve, TestLocate);
            TestTree_L1.build("testdata1.dat");
            TestTree_L2.build("testdata2.dat");
            TestTree_L3.build("testdata3.dat");
        }
};

TEST_F(TreeTest, InsertionLevel1Works){
    EXPECT_EQ("0000001(8)[]", TestTree_L1.getLinearTree());
    EXPECT_EQ("0000001", TestTree_L1.getLocalTree());
}

TEST_F(TreeTest, InsertionLevel2Works){
    EXPECT_EQ("0000001(0)[0001000,0001001,0001010,0001011,0001100,0001101,0001110,0001111];0001111(1)[];0001110(1)[];0001101(1)[];0001011(1)[];0001001(1)[];0001010(1)[];0001100(1)[];0001000(1)[]", TestTree_L2.getLinearTree());
    EXPECT_EQ("0001000*0001001*0001010*0001011*0001100*0001101*0001110*0001111*0000001", TestTree_L2.getLocalTree());
}

TEST_F(TreeTest, InsertionLevel3Works){
    EXPECT_EQ("0000001(0)[0001000,0001001,0001010,0001011,0001100,0001101,0001110,0001111];0001111(0)[1111000];0001110(0)[1110000];0001101(0)[1101000];0001100(0)[1100000];0001011(0)[1011000];0001010(0)[1010000];0001001(0)[1001000];0001000(0)[1000000];1111000(1)[];1110000(1)[];1101000(1)[];1011000(1)[];1001000(1)[];1010000(1)[];1100000(1)[];1000000(1)[]", TestTree_L3.getLinearTree());
    EXPECT_EQ("1000000*0001000*1001000*0001001*1010000*0001010*1011000*0001011*1100000*0001100*1101000*0001101*1110000*0001110*1111000*0001111*0000001", TestTree_L3.getLocalTree());
}

TEST_F(TreeTest, ClearupWorks){
    TestTree_L1.clear();
    TestTree_L2.clear();
    TestTree_L3.clear();
    ASSERT_TRUE(TestTree_L1.isEmpty());
    ASSERT_TRUE(TestTree_L2.isEmpty());
    ASSERT_TRUE(TestTree_L3.isEmpty());
    ASSERT_TRUE(TestTree_L1.getLocalTree().empty());
    ASSERT_TRUE(TestTree_L2.getLinearTree().empty());
    ASSERT_TRUE(TestTree_L3.getLocalTree().empty());
    ASSERT_TRUE(TestTree_L1.getLinearTree().empty());
    ASSERT_TRUE(TestTree_L2.getLocalTree().empty());
    ASSERT_TRUE(TestTree_L3.getLinearTree().empty());
}

int main(int argc, char* argv[]) {
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}

#ifndef MODELFREE_H
#define MODELFREE_H
#include <Eigen/Dense>
#include <Eigen/Core>
#include <vector>
class ModelFreeDecoder{
private:
    Eigen::VectorXf lam0;
    Eigen::VectorXf lam1;
    Eigen::VectorXf lastCusum;
    Eigen::VectorXf temp;
    Eigen::VectorXf temp1;
    int nUnits;
    double x;
public:
    ModelFreeDecoder():lam0(),lam1(),lastCusum(),temp(),nUnits(0),x(0){}
    ModelFreeDecoder(int len):lam0(len),lam1(len),
        lastCusum(Eigen::VectorXf::Zero(len)),
        temp(Eigen::VectorXf::Zero(len)),
        temp1(Eigen::VectorXf::Zero(len)),
        nUnits(len){}

    void decode(Eigen::VectorXf y)
    {
        temp = y.cwiseProduct(temp1);
        temp = temp - (lam1-lam0);
        lastCusum = (lastCusum + temp).cwiseMax(0);
        x= lastCusum.maxCoeff();
    }


    void updateBaseLine(Eigen::MatrixXf &seq)
    {
        int len = seq.cols();
        nUnits = seq.rows();
        lastCusum = Eigen::VectorXf::Zero(nUnits);
        lam0 = seq.rowwise().mean();
        lam0 = lam0.array() + 0.1;
        lam1 = lam0 + 2*lam0.cwiseSqrt();
        temp1 = (lam1.cwiseQuotient(lam0)).array().log();
        float sumcum;
    }

    float getX() const { return x;}

};
#endif // MODELFREE_H

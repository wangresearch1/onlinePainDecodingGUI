#ifndef __PLDS_TEST__
#define __PLDS_TEST__
#include <Eigen/Dense>
#include <Eigen/Core>
#include <Eigen/SparseCore>

const int defaultBufSize = 100000;

class PLDSMStepObservationCost
{
private:
    int nU;
    int nY;
    Eigen::MatrixXf y;
    Eigen::VectorXf xsm;
    Eigen::VectorXf Vsm;
public:
    PLDSMStepObservationCost(const int nu_, const int ny_, Eigen::MatrixXf y_, Eigen::VectorXf xsm_, Eigen::VectorXf Vsm_) : nU(nu_), nY(ny_), y(y_), xsm(xsm_), Vsm(Vsm_){}
    double operator()(const Eigen::VectorXf& x, Eigen::VectorXf& grad) const
    {
        double fx = 0.0;
        Eigen::VectorXf d = x.tail(nU);
        Eigen::VectorXf C = x.head(nU);
        Eigen::VectorXf CC(nU);

        CC << C.cwiseProduct(C);

        Eigen::MatrixXf h = (C*xsm.transpose()).colwise() + d;
        Eigen::MatrixXf yhat = h + (CC*Vsm.transpose()) / 2;
        yhat = yhat.array().exp();

        fx = -(fx + (y.transpose().cwiseProduct(h) - yhat).sum());

        Eigen::VectorXf dfc = Eigen::VectorXf::Zero(nU);
        Eigen::VectorXf dfd = Eigen::VectorXf::Zero(nU);
        dfc = dfc + (y.transpose() - yhat)*xsm - C.cwiseProduct(yhat*Vsm);
        dfd = dfd + (y.transpose() - yhat).rowwise().sum();

        grad << -dfc, -dfd;

        return fx;
    }
};
class ExpFamPCACost
{
private:
    int nU;
    int nY;
    Eigen::MatrixXf y;
    double lambda;
public:
    ExpFamPCACost(const int nu_, const int ny_, Eigen::MatrixXf y_, double lambda_) : nU(nu_), nY(ny_), y(y_), lambda(lambda_) {}
    double operator()(const Eigen::VectorXf& x, Eigen::VectorXf& grad)
    {
        double fx = 0.0;
        Eigen::VectorXf d = x.tail(nU);
        Eigen::VectorXf C = x.head(nU);
        Eigen::VectorXf X = x.segment(nU, nY);
        Eigen::MatrixXf nu = C*X.transpose();
        nu.colwise() += d;
        Eigen::MatrixXf Yhat = nu.array().exp();
        nu = nu.cwiseProduct(-y.transpose()) + Yhat;

        fx = nu.sum() + lambda / 2 * (C.norm()*C.norm() + X.norm());

        Eigen::MatrixXf YhatmY = Yhat - y.transpose();
        Eigen::MatrixXf gX = C.transpose()*YhatmY + lambda*X.transpose();
        Eigen::MatrixXf gC = YhatmY*X + lambda*C;
        Eigen::MatrixXf gd = YhatmY.rowwise().sum();

        grad << gC, gX.transpose(), gd;

        return fx;
    }
};
//template<typename T>
class PLDSmodel {
private:
    float a;
    float x0;
    float q0;
    float q;
    Eigen::VectorXf c;
    Eigen::VectorXf d;
    Eigen::VectorXf xsm;
    Eigen::VectorXf Vsm;
    // circular buffer
    int bufSize;
    float* xsmbuf;
    float* vsmbuf;
    int idx_curbuf;
    int idx_acquisitorbuf;
public:
    //constructor
    PLDSmodel() :a(0), x0(0), q0(0), q(0), c(), d(),
        xsm(), Vsm(),bufSize(defaultBufSize),
        xsmbuf(nullptr),vsmbuf(nullptr),idx_curbuf(0),idx_acquisitorbuf(0)
    {
        xsmbuf = new float[bufSize];
        vsmbuf = new float[bufSize];
    }
    //PLDSmodel(PLDSmodel& rhs) {}//todo
    PLDSmodel& operator=(PLDSmodel& rhs)
    {
        if (this == &rhs) return *this;
        a = rhs.getA();
        x0 = rhs.getX0();
        q0 = rhs.getQ0();
        q = rhs.getQ();
        c = rhs.getC();
        d = rhs.getD();
        xsm = rhs.getXsm();
        Vsm = rhs.getVsm();
        bufSize = rhs.getBufSize();
        idx_curbuf = rhs.getIdx_curbuf();
        idx_acquisitorbuf = rhs.idx_acquisitorbuf;

        float* p_xsmbuf_origin = xsmbuf;
        xsmbuf = new float[bufSize];
        memcpy(xsmbuf, rhs.xsmbuf, sizeof(float)*bufSize);
        if (nullptr != p_xsmbuf_origin)
            delete [] p_xsmbuf_origin;

        float* p_vsmbuf_origin = vsmbuf;
        vsmbuf = new float[bufSize];
        memcpy(vsmbuf, rhs.vsmbuf, sizeof(float)*bufSize);
        if (nullptr != p_vsmbuf_origin)
            delete [] p_vsmbuf_origin;

        return *this;
    }//todo

    //destructor
    ~PLDSmodel()
    {
        delete [] xsmbuf;
        delete [] vsmbuf;
    }

    //model update
    void updateModelParams(PLDSmodel & rhs)
    {
        a = rhs.getA();
        x0 = rhs.getX0();
        q0 = rhs.getQ0();
        q = rhs.getQ();
        c = rhs.getC();
        d = rhs.getD();
        xsm = rhs.getXsm();
        Vsm = rhs.getVsm();
        bufSize = rhs.getBufSize();
        idx_curbuf = 0;
        idx_acquisitorbuf = rhs.idx_acquisitorbuf;
    }

    //sets,gets and inline methods
    float getA() const { return a; }
    float getX0() const { return x0; }
    float getQ0() const { return q0; }
    float getQ() const { return q; }
    Eigen::VectorXf getC() const  { return c; }
    Eigen::VectorXf getD() const { return d; }
    Eigen::VectorXf getXsm() const { return xsm; }
    Eigen::VectorXf getVsm() const { return Vsm; }
    int getIdx_curbuf() const {return idx_curbuf;}
    int getIdx_acquisitorbuf() const {return idx_acquisitorbuf;}
    int getBufSize() const {return bufSize;}
    float getXsmBufValue(const int idx) const
    {
        if(idx>=0 && idx<bufSize)
        {
            return xsmbuf[idx];
        }
        else
        {
            std::cout << "invalid index = "<<idx<<std::endl;
            return 0;
        }
    }
    float getVsmBufValue(const int idx) const
    {
        if(idx>=0 && idx<bufSize)
        {
            return vsmbuf[idx];
        }
        else
        {
            std::cout << "invalid index = "<<idx<<std::endl;
            return 0;
        }
    }

    float getLastXsmBufValue() const {return xsmbuf[idx_curbuf];}
    float getLastVsmBufValue() const {return vsmbuf[idx_curbuf];}
    Eigen::VectorXf getBaselineX(const int startIdx, const int endIdx);

    void setA(const float a_in) { a = a_in; }
    void setX0(const float x0_in) { x0 = x0_in; }
    void setQ0(const float q0_in) { q0 = q0_in; }
    void setQ(const float q_in) { q = q_in; }
    void setC(const Eigen::VectorXf& c_in) { c = c_in; }
    void setD(const Eigen::VectorXf& d_in) { d = d_in; }
    void setXsm(const Eigen::VectorXf& xsm_in) { xsm = xsm_in; }
    void setVsm(const Eigen::VectorXf& vsm_in) { Vsm = vsm_in; }
    void setIdx_curbuf(const int idx_cubuf_){idx_curbuf=idx_cubuf_;}
    void setIdx_acquisitorbuf(const int idx_acquisitorbuf_){idx_acquisitorbuf=idx_acquisitorbuf_;}
    void setBufSize(const int bufSize_){bufSize = bufSize_;}
    void addNewXnV(const double x, const double v)
    {
        idx_curbuf = (idx_curbuf+1+bufSize)%bufSize;
        xsmbuf[idx_curbuf] = x;
        vsmbuf[idx_curbuf] = v;
    }

    //PLDS methods
    void expFamPCAInit(const Eigen::MatrixXf& seq, const int units, const int seq_l);
    void trainEmLaplace_old(const Eigen::MatrixXf& seq, const int units, const int seq_l, const float binSize, const int maxIter, const int maxCpuTime_s);
    void trainEmLaplace(const Eigen::MatrixXf& seq, const int units, const int seq_l, const float binSize, const int maxIter, const int maxCpuTime_s);
    void PLDSOnlineFilterTrial(const Eigen::MatrixXf& seq, const int units, const int seq_l, const float binSize, Eigen::VectorXf &x, Eigen::VectorXf &V);
    void PLDSOnlineFilterBin(const Eigen::VectorXf& seq, const float binSize, const float x0, const float q0, float &x, float &V);
};
void writeZscore(const std::string filename, Eigen::VectorXf & zscore);
void readSeqModelState(const std::string filename, std::vector<PLDSmodel> &model, std::vector<Eigen::MatrixXf> &seq, std::vector<Eigen::VectorXf> & smoothing_x, std::vector<Eigen::VectorXf> & smoothing_v, std::vector<Eigen::VectorXf> & filter_x, std::vector<Eigen::VectorXf> & filter_v, int &units, int &seq_l);
#endif

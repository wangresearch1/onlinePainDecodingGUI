#ifndef _ALGORITHMS__
#define _ALGORITHMS__

//#define debug

#include <iostream>
#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include "bufferManager.h"
#include "configManager.h"

namespace painBMI
{
// ++++++++++++++ decoder related classes +++++++++++++++++++++++++++++++++++++++++++++++++
class DecoderBase
{
protected:
    std::string name;
    std::string type;
public:
    DecoderBase(std::string name_, std::string type_) :
        name(name_), type(type_)
    {}
    std::string get_type() const { return type; }
    std::string get_name() const { return name; }
    virtual float decode(const Eigen::VectorXf& data) = 0;
    virtual void updateBaseline(const Eigen::MatrixXf& data) = 0;
    virtual ~DecoderBase(){}
};

/************************************************************************
*               Model based decoder: PLDS and TLDS                      *
*************************************************************************/
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

class PLDSMStepObservationCost_fixd
{
private:
    int nU;
    int nY;
    Eigen::MatrixXf y;
    Eigen::VectorXf xsm;
    Eigen::VectorXf Vsm;
    Eigen::VectorXf fix_d;
public:
    PLDSMStepObservationCost_fixd(const int nu_, const int ny_, Eigen::MatrixXf y_, Eigen::VectorXf xsm_, Eigen::VectorXf Vsm_, Eigen::VectorXf fix_d_) : nU(nu_), nY(ny_), y(y_), xsm(xsm_), Vsm(Vsm_), fix_d(fix_d_){}
    double operator()(const Eigen::VectorXf& x, Eigen::VectorXf& grad) const
    {
        double fx = 0.0;
        Eigen::VectorXf CC(nU);
        CC << x.cwiseProduct(x);
        Eigen::MatrixXf h = (x*xsm.transpose()).colwise() + fix_d;
        Eigen::MatrixXf yhat = h + (CC*Vsm.transpose()) / 2;
        yhat = yhat.array().exp();
        fx = -(fx + (y.transpose().cwiseProduct(h) - yhat).sum());
        Eigen::VectorXf dfc = Eigen::VectorXf::Zero(nU);
        dfc = dfc + (y.transpose() - yhat)*xsm - x.cwiseProduct(yhat*Vsm);
        grad << -dfc;

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

class ExpFamPCACost_fixd
{
private:
    int nU;
    int nY;
    Eigen::MatrixXf y;
    double lambda;
    Eigen::VectorXf fix_d;
public:
    ExpFamPCACost_fixd(const int nu_, const int ny_, Eigen::MatrixXf y_, double lambda_, Eigen::VectorXf fix_d_) : nU(nu_), nY(ny_), y(y_), lambda(lambda_), fix_d(fix_d_) {}
    double operator()(const Eigen::VectorXf& x, Eigen::VectorXf& grad)
    {
        double fx = 0.0;
        Eigen::VectorXf C = x.head(nU);
        Eigen::VectorXf X = x.segment(nU, nY);
        Eigen::MatrixXf nu = C*X.transpose();
        nu.colwise() += fix_d;
        Eigen::MatrixXf Yhat = nu.array().exp();
        nu = nu.cwiseProduct(-y.transpose()) + Yhat;

        fx = nu.sum() + lambda / 2 * (C.norm()*C.norm() + X.norm());

        Eigen::MatrixXf YhatmY = Yhat - y.transpose();
        Eigen::MatrixXf gX = C.transpose()*YhatmY + lambda*X.transpose();
        Eigen::MatrixXf gC = YhatmY*X + lambda*C;
        grad << gC, gX.transpose();

        return fx;
    }
};

class Modeldecoder : public DecoderBase
{
protected:
    bool init;
    // data param
    float binSize; // in second
    int units;
    int seq_l; // train seq length, will be set during training
    // model params
    float a;
    float q;
    Eigen::VectorXf c;
    Eigen::VectorXf d;
    Eigen::VectorXf xsm;
    Eigen::VectorXf Vsm;
    // last state, updated after training and every decoding call
    float x0;
    float q0;
    // baseline
    float meanBase;
    float stdBase;
public:
    // constructor
    Modeldecoder() :
        DecoderBase("unnamed", "Model"),init(false),
        binSize(0.05)
    {}

    Modeldecoder(std::string name_, std::string type_) :
        DecoderBase(name_, type_),init(false),
        binSize(0.05)
    {}

    // sets,gets and inline methods
    bool isInit() const {return init;}
    float get_binSize() const { return binSize; }
    float getA() const { return a; }
    float getX0() const { return x0; }
    float getQ0() const { return q0; }
    float getQ() const { return q; }
    Eigen::VectorXf getC() const  { return c; }
    Eigen::VectorXf getD() const { return d; }
    Eigen::VectorXf getXsm() const { return xsm; }
    Eigen::VectorXf getVsm() const { return Vsm; }
    float get_meanBase() const { return meanBase; }
    float get_stdBase() const { return stdBase; }

    float getZscore() const
    {
        if (abs(stdBase) < 1e-6)
        {
            return 0;
        }
        else
        {
            return (x0 - meanBase) / stdBase;
        }
    }

    float getZscore(const float x_) const
    {
        if (abs(stdBase) < 1e-6)
        {
            return 0;
        }
        else
        {
            return (x_ - meanBase) / stdBase;
        }
    }

    float getConf(const float nStd) const
    {
        if (abs(stdBase) < 1e-6 || !isInit())
        {
            return 0;
        }
        else
        {
            return sqrt(q0)*nStd / stdBase;
        }
    }

    float getConf(const float q_,const float nStd) const
    {
        if (abs(stdBase) < 1e-6 || !isInit())
        {
            return 0;
        }
        else
        {
            return sqrt(q_)*nStd / stdBase;
        }
    }

    void set_binSize(float binSize_){ binSize = binSize_; }

    // methods
    virtual void train(const Eigen::MatrixXf& data) = 0;
    virtual float decode(const Eigen::VectorXf& data) = 0;
    virtual void updateBaseline(const Eigen::MatrixXf& data);
    virtual ~Modeldecoder(){}
};

class PLDSdecoder :public Modeldecoder
{
private:
    //bool init;
    // EM param
    int maxIter;
    int maxCpuTime_s;
    // model param
    void expFamPCAInit(const Eigen::MatrixXf & data);
public:
    // constructor
    PLDSdecoder() :
        Modeldecoder("unnamed", "PLDS"),
        maxIter(200), maxCpuTime_s(30)
    {}
    PLDSdecoder(std::string name_) :
        Modeldecoder(name_, "PLDS"),
        maxIter(200), maxCpuTime_s(30)
    {}

    // sets,gets and inline methods
    int get_maxIter() const { return maxIter; }
    int get_maxCpuTime() const { return maxCpuTime_s; }

    void set_maxIter(int maxIter_) { maxIter = maxIter_; }
    void set_maxCpuTime(int time_){ maxCpuTime_s = time_; }

    // methods
    virtual void train(const Eigen::MatrixXf& data);
    virtual float decode(const Eigen::VectorXf& data);
    virtual ~PLDSdecoder(){}
};

enum TLDSTYPE{ LOGTLDS, SQROOTLDS };

class TLDSdecoder :public Modeldecoder
{
private:
    // em param
    int emMaxIter;
    int faMaxIter;
    float emTol;
    float faTol;
    // tlds type
    TLDSTYPE tldsType;
    Eigen::MatrixXf tSeq;
    // model param
    Eigen::VectorXf r; //observation noise covariance
    Eigen::VectorXf ll; //log likelihood curve
    void faInit(const Eigen::MatrixXf & data);
public:
    // constructor
    TLDSdecoder() :
        Modeldecoder("unnamed", "TLDS"),
        emMaxIter(300), faMaxIter(100), emTol(1e-6), faTol(0.001),
        tldsType(LOGTLDS)
    {}
    TLDSdecoder(std::string name_) :
        Modeldecoder(name_, "TLDS"),
        emMaxIter(300), faMaxIter(100), emTol(1e-6), faTol(0.001),
        tldsType(LOGTLDS)
    {}

    // sets,gets and inline methods
    Eigen::VectorXf getR() const{ return r; }
    Eigen::VectorXf getLL() const { return ll; }
    int getEmMaxIter() const { return emMaxIter; }
    int getFaMaxIter() const { return faMaxIter; }
    float getEmTol() const { return emTol; }
    float getFaTol() const { return faTol; }
    TLDSTYPE getTldsType() const { return tldsType; }

    void setEmMaxIter(const int emMaxIter_){ emMaxIter = emMaxIter_; }
    void setFaMaxIter(const int faMaxIter_){ faMaxIter = faMaxIter_; }
    void setEmTol(const float emTol_){ emTol = emTol_; }
    void setFaTol(const float faTol_){ faTol = faTol_; }
    void setTldsType(const TLDSTYPE tldsType_){ tldsType = tldsType_; }

    virtual void train(const Eigen::MatrixXf& data);
    virtual float decode(const Eigen::VectorXf& data);
    virtual ~TLDSdecoder(){}
};

/************************************************************************
*               Model free decoder: CUSUM                               *
*************************************************************************/
class ModelFreedecoder : public DecoderBase
{
protected:
    bool init;
    // data param
    int units;
    // baseline params
    Eigen::VectorXf lam0;
    Eigen::VectorXf lam1;
    // temporary params for timing saving
    Eigen::VectorXf temp;
    Eigen::VectorXf temp1;
    // last cusum
    Eigen::VectorXf lastCusum;
public:
    // constructor
    ModelFreedecoder() :
        DecoderBase("unnamed", "ModelFree"),init(false)
    {}

    ModelFreedecoder(std::string name_) :
        DecoderBase(name_, "ModelFree"),init(false)
    {}

    // sets,gets and inline methods
    Eigen::VectorXf getLam0() const  { return lam0; }
    Eigen::VectorXf getLam1() const { return lam1; }
    Eigen::VectorXf getLastCusum() const { return lastCusum; }

    // methods
    virtual float decode(const Eigen::VectorXf& data);
    virtual void updateBaseline(const Eigen::MatrixXf& data);
    virtual ~ModelFreedecoder(){}
};

// ++++++++++++++ detector related classes +++++++++++++++++++++++++++++++++++++++++++++++++
enum MODEL_DECODER_TYPE{ PLDS, TLDS };

const std::vector<std::string> DecoderType = {"PLDS","TLDS","ModelFree"};

typedef std::vector<DecoderBase*> dec_vec;
class DetectorBase
{
protected:
    std::string name;
    std::string type;
    float thresh;
    std::string region;
    int last_detection_cnt;
    int min_interval;//in number of bins, the minimum bins between detections
    bool cur_bin_done;
public:
    DetectorBase(std::string name_, std::string type_, float thresh_=0, std::string region_="noregion") :
        name(name_), type(type_), thresh(thresh_), region(region_), last_detection_cnt(0), min_interval(100),cur_bin_done(false)
    {}

    void setThresh(float thresh_){ thresh = thresh_; }
    float get_thresh() const { return thresh; }
    std::string get_type() const { return type; }
    std::string get_name() const { return name; }
    std::string get_region() const { return region; }
    int get_last_detect_cnt() const {return last_detection_cnt;}

    void clearDoneFlag() {cur_bin_done = false;}

    virtual std::string get_decoder_type() const = 0;
    virtual bool detect(const Eigen::VectorXf& data) = 0;
    virtual bool detect(const Eigen::VectorXf& data, const float thresh,const bool self = true) = 0;
    virtual ~DetectorBase(){}
};

enum model_detector_status{model_training,update_required,normal};

class ModelDetector :public DetectorBase
{
private:
    bool enabled;
    MODEL_DECODER_TYPE decoder_type;
    Modeldecoder* p_decoder;
    float nStd;
    float interval;
    UPDATE_MODE update_mode;
    circular_buffer<float> z_buf;
    circular_buffer<float> q_buf;
    uint32_t l_base;
    float binSize;
    Modeldecoder* p_decoder_tmp;
    std::vector<Modeldecoder*> trainedDecoders;
    int train_bin_cnt = 0;// for catching up decoding
    circular_buffer<float> z_buf_tmp;// from catching up decoding
    uint32_t l_train; // training seq length
    circular_buffer2d<float>* catching_up_buf;
    model_detector_status status;
    Eigen::VectorXf zscore_base;
    Eigen::VectorXf conf_base;
public:
    // constructor
    /*
        ModelDetector() :
            DetectorBase("unnamed", "Single"), enabled(true), p_decoder(nullptr)
        {}

        ModelDetector(std::string name_) :
            DetectorBase(name_, "Single"), enabled(true), p_decoder(nullptr)
        {}
        */
    ModelDetector(std::string name_ = "unnamed", MODEL_DECODER_TYPE type_ = PLDS, std::string region_ = "ACC",
                  float thresh_ = 1.65, float nStd_ = 2, float interval_ = 2, UPDATE_MODE update_ = manual, size_t size_=10000,
                  uint32_t l_base_ = 80)
        :DetectorBase(name_, "Single", thresh_, region_),
          enabled(true),decoder_type(type_), p_decoder(nullptr),
          nStd(nStd_), interval(interval_), update_mode(update_), z_buf(size_), q_buf(size_), l_base(l_base_),binSize(0.05),
          p_decoder_tmp(nullptr),trainedDecoders(),z_buf_tmp(size_),l_train(200),catching_up_buf(nullptr),status(normal),zscore_base(),conf_base()
    {
        switch (type_)
        {
        case PLDS:
            p_decoder = new PLDSdecoder;
            p_decoder_tmp = new PLDSdecoder;
            *p_decoder_tmp = *p_decoder;
            break;
        case TLDS:
            p_decoder = new TLDSdecoder;
            p_decoder_tmp = new TLDSdecoder;
            *p_decoder_tmp = *p_decoder;
            break;
        default:
            p_decoder = new PLDSdecoder;
            p_decoder_tmp = new PLDSdecoder;
            *p_decoder_tmp = *p_decoder;
        }
    }

    // inline methods
    bool isEnabled() const { return enabled; }
    void set_nStd(float nStd_){ nStd = nStd_; }
    void set_interval(float interval_){ interval = interval_; }
    void set_update(UPDATE_MODE update_){ update_mode = update_; }
    // set baseline in seconds
    void set_l_base(float base_){ l_base = round(base_/binSize); }

    void enable() { enabled = true; }
    void disable() { enabled = false; }
    float get_nstd() const { return nStd; }
    float get_interval() const { return interval; }
    UPDATE_MODE get_update() const { return update_mode; }
    size_t get_bufSize() const { return z_buf.size(); }
    uint32_t get_l_base() const { return l_base; }
    uint32_t get_l_train() const { return l_train; }
    void set_l_train(uint32_t l_train_){l_train = l_train_; }
    float getZscore() const {return p_decoder->getZscore();}
    float getConf() const {return p_decoder->getConf(nStd);}
    Eigen::VectorXf getZscore_b() const {return zscore_base;}
    Eigen::VectorXf getConf_b() const {return conf_base;}
    Eigen::VectorXf getDecoderC() const {return p_decoder->getC(); }
    virtual std::string get_decoder_type() const {return p_decoder->get_type();}
    float get_meanBase() const { return p_decoder->get_meanBase(); }
    float get_stdBase() const { return p_decoder->get_stdBase(); }
    int n_trained_decoders() const {return trainedDecoders.size();}

    // methods
    void update();
    virtual bool detect(const Eigen::VectorXf& data);
    virtual bool detect(const Eigen::VectorXf& data, const float thresh, const bool self = true);
    // in case called from other higher level decoder
    void train(const Eigen::MatrixXf& data);

    void push_buf(const Eigen::VectorXf& data)
    {
        if (catching_up_buf == nullptr)
        {
            catching_up_buf = new circular_buffer2d<float>(10000,data.rows());
        }
        catching_up_buf->putEigenVec(data);
    }

    void use_last_model()
    {
        if (status==normal && trainedDecoders.size()>1)
        {
            trainedDecoders.pop_back();
            *p_decoder_tmp = *(trainedDecoders.back());
            trainedDecoders.pop_back();
            status = update_required;
        }
    }

    void update_model()
    {
        switch (decoder_type)
        {
        case PLDS:
            trainedDecoders.push_back(new PLDSdecoder);
            break;
        case TLDS:
            trainedDecoders.push_back(new TLDSdecoder);
            break;
        default:
            trainedDecoders.push_back(new PLDSdecoder);
        }
        *(trainedDecoders.back()) = *p_decoder_tmp;
        std::cout << "n_decoder="<<trainedDecoders.size()<<"last_a="<<trainedDecoders.back()->getA()<<std::endl;

        *p_decoder = *p_decoder_tmp;
        z_buf.reset();
        while(!z_buf_tmp.empty())
        {
            z_buf.put(z_buf_tmp.get());
        }
        status = normal;
    }

    model_detector_status getStatus() const {return status;}
    void process_based_on_status(const Eigen::VectorXf& data)
    {
        switch(status)
        {
        case model_training:
            push_buf(data);
            std::cout<<name<<" push buf "<<train_bin_cnt++<<std::endl;
            break;
        case update_required:
            update_model();
            status = normal;
            std::cout<<name<<" update model done"<<std::endl;
            break;
        default:
            break;
        }
    }

//    bool isUpdateRequired()
//    {
//        if (train_bin_cnt<10000-1)
//            train_bin_cnt++;
//        return require_update;
//    }

    virtual ~ModelDetector()
    {
        if (p_decoder != nullptr)
        {
            delete p_decoder;
        }
        if (p_decoder_tmp != nullptr)
        {
            delete p_decoder_tmp;
        }
        if (catching_up_buf != nullptr)
        {
            delete catching_up_buf;
        }

        for (int i=0;i<trainedDecoders.size();i++)
        {
            if (trainedDecoders[i]!=nullptr)
            {
                delete trainedDecoders[i];
                trainedDecoders[i]=nullptr;
            }
        }
    }
};

class ModelFreeDetector : public DetectorBase
{
private:
    bool enabled;
    ModelFreedecoder* p_decoder;
    float interval;
    UPDATE_MODE update_mode;
    circular_buffer<float> x_buf;
    uint32_t l_base;
    float binSize;
    circular_buffer2d<float> *p_spk_buf;
    int nSpikes;
public:
    ModelFreeDetector(std::string name_ = "unnamed", std::string region_ = "ACC",
                      float thresh_ = 3.38, float interval_ = 2, UPDATE_MODE update_ = manual, size_t size_ = 10000,
                      uint32_t l_base_ = 80)
        :DetectorBase(name_, "Single", thresh_, region_),
          enabled(true), p_decoder(nullptr),
          interval(interval_), update_mode(update_), x_buf(size_), l_base(l_base_),
          binSize(0.05), p_spk_buf(nullptr),nSpikes(0)
    {
        p_decoder = new ModelFreedecoder;
    }

    // inline methods
    bool isEnabled() const { return enabled; }
    void set_interval(float interval_){ interval = interval_; }
    void set_update(UPDATE_MODE update_){ update_mode = update_; }
    // set baseline in seconds
    void set_l_base(float base_){ l_base = round(base_ / binSize); }

    void enable() { enabled = true; }
    void disable() { enabled = false; }
    float get_interval() const { return interval; }
    UPDATE_MODE get_update() const { return update_mode; }
    size_t get_bufSize() const { return x_buf.size(); }
    uint32_t get_l_base() const { return l_base; }
    float getX() {return x_buf.read();}

    // must set spk buffer before using
    void set_p_spk_buf(circular_buffer2d<float> * p_spk_buf_) { p_spk_buf = p_spk_buf_; }

    // methods
    void update();
    virtual std::string get_decoder_type() const {return p_decoder->get_type();}
    virtual bool detect(const Eigen::VectorXf& data);
    virtual bool detect(const Eigen::VectorXf& data, const float thresh, const bool self = true);

    virtual ~ModelFreeDetector()
    {
        if (p_decoder != nullptr)
        {
            delete p_decoder;
        }
    }

};

class GreedyDetector :public DetectorBase
{
private:
    bool enabled;
    std::vector<DetectorBase*> detector_list;
    std::vector<float> thresh_list;
    std::vector<float> higher_thresh_list;
public:
    GreedyDetector(std::string name_ = "unnamed")
        :DetectorBase(name_, "Greedy"),enabled(true)
    {}

    // must add other decoders
    void addDetector(DetectorBase* detector, float thresh = 0, float higer_thresh = 0)
    {
        if (thresh == 0)
        {
            thresh_list.push_back(detector->get_thresh());
        }
        else
        {
            thresh_list.push_back(thresh);
        }

        if (higer_thresh == 0)
        {
            higher_thresh_list.push_back(detector->get_thresh());
        }
        else
        {
            higher_thresh_list.push_back(higer_thresh);
        }

        detector_list.push_back(detector);
    }

    virtual std::string get_decoder_type() const
    {
        std::string str = "";
        for (int i=0;i<detector_list.size();i++)
        {
            str +=" " + detector_list[i]->get_name();
        }
        return str;
    }

    const std::vector<DetectorBase*>& get_detector_list()const {return detector_list;}

    virtual bool detect(const Eigen::VectorXf& data);
    virtual bool detect(const Eigen::VectorXf& data, const float thresh,const bool self = true);
    bool detect(const std::vector<Eigen::VectorXf>& data, const std::vector<std::string>& region,const bool self = true);

    virtual ~GreedyDetector(){}
};

class CCFDetector :public DetectorBase
{
private:
    bool enabled;
    std::vector<ModelDetector*> detector_list;
    float CCF_Value, CCF_Area;
    float mean_base_ccf;
    float std_base_ccf;
    float nStd;// confidence interval is nStd*baseline_std
    std::vector<float> CCF_Thresh;
    float CCF_Area_Thresh;
    float alpha;
    float beta;
    float ffactor;  // forgeting factor
public:
    CCFDetector(std::string name_ = "unnamed", bool enabled_= true, float alpha_= 0.5, float beta_= 0.5
                , float ffactor_= 0.6, float CCF_Area_Thresh_= 1.6, float nStd = 3)
        :DetectorBase(name_, "CCF"), enabled(enabled_), CCF_Value(0.0), CCF_Area(0.0), mean_base_ccf(0.0),
          std_base_ccf(0.0), nStd(nStd), CCF_Thresh({0,0}), CCF_Area_Thresh(CCF_Area_Thresh_),
        alpha(alpha_),beta(beta_),ffactor(ffactor_)
    {}

    // must add other decoders
    void addDetector(ModelDetector* detector)
    {
        detector_list.push_back(detector);
    }

    void update_ccf_threshold()
    {
        // update ccf baseline based on the baseline data of related detectors
        //compute_ccf_base();
        // update threshold
        CCF_Thresh[0] = mean_base_ccf - nStd * std_base_ccf;
        CCF_Thresh[1] = mean_base_ccf + nStd * std_base_ccf;
        //std::cout << "CCF_Thresh="<<CCF_Thresh[0]<<","<<CCF_Thresh[1]<<std::endl;
        //std::cout << "base_ccf="<<mean_base_ccf<<","<<std_base_ccf<<std::endl;
    }

    virtual std::string get_decoder_type() const
    {
        std::string str = "";
        for (int i=0;i<detector_list.size();i++)
        {
            str +=" " + detector_list[i]->get_name();
        }
        return str;
    }

    void compute_ccf_base();
    float get_ccf_value() const { return CCF_Value; }
    float get_ccf_area() const { return CCF_Area; }
    float get_ccf_area_thresh() const { return CCF_Area_Thresh; }
    std::vector<float> get_ccf_thresh(){ return CCF_Thresh; }
    const std::vector<ModelDetector*>& get_detector_list() {return detector_list;}

    void set_alpha(float alpha_){alpha = alpha_;std::cout<<"alpha="<<alpha<<std::endl;}
    void set_beta(float beta_){beta = beta_;std::cout<<"beta="<<beta<<std::endl;}
    void set_ffactor(float ffactor_){ffactor = ffactor_;std::cout<<"ffactor="<<ffactor<<std::endl;}
    void set_area_thresh(float aThresh_){CCF_Area_Thresh = aThresh_;std::cout<<"CCF_Area_Thresh="<<CCF_Area_Thresh<<std::endl;}

    virtual bool detect(const Eigen::VectorXf& data);
    virtual bool detect(const Eigen::VectorXf& data, const float thresh ,const bool self = true);
    virtual ~CCFDetector(){}
};

class EMGDetector :public DetectorBase
{
private:
    bool enabled;
    //float thresh;
    float factor;
    std::vector<ModelDetector*> detector_list;
public:
    EMGDetector(std::string name_ = "unnamed", float thresh_=0 ,bool enabled_= true):DetectorBase(name_, "EMG", thresh_), enabled(enabled_),factor(0.00307767464582315583781143136297*100)//factor(1.9226660969878231147190771202734e-1)//factor(1.94875977809706e-01)
    {}

    virtual std::string get_decoder_type() const
    {
        std::string str = "EMG";
        return str;
    }

    float get_emg_mv(const float data) const { return data*factor; }
    bool detect(const float data) const {return data>thresh;}

    virtual bool detect(const Eigen::VectorXf& data);
    virtual bool detect(const Eigen::VectorXf& data, const float thresh ,const bool self = true);
    virtual ~EMGDetector(){}
};
/************************************************************************
*               Algorithm Manager                                       *
*************************************************************************/
class AlgorithmManager
{
private:
    std::vector<DecoderBase*> decoder_list;
    std::vector<DetectorBase*> detector_list;

public:
    //flags for online
    bool ready;

    AlgorithmManager(AlgorithmConfig* algCofig):ready(false)
    {
        //construct decoders
        for (auto iter = algCofig->get_decoder_config_list().begin();iter < algCofig->get_decoder_config_list().end();iter++)
        {
            if ((*iter)->get_type().compare("PLDS")==0 )
            {
                PLDSDecConfig* mdc_p = static_cast<PLDSDecConfig*>(*iter);
                PLDSdecoder* tmp_dec_p;
                tmp_dec_p = new PLDSdecoder(mdc_p->get_name());
                tmp_dec_p->set_maxCpuTime(mdc_p->get_maxCpuTimeInSec());
                tmp_dec_p->set_maxIter(mdc_p->get_maxIter());
                decoder_list.push_back(static_cast<DecoderBase*>(tmp_dec_p));
            }

            if ((*iter)->get_type().compare("TLDS")==0)
            {
                TLDSDecConfig* mdc_p = static_cast<TLDSDecConfig*>(*iter);
                TLDSdecoder* tmp_dec_p;
                tmp_dec_p = new TLDSdecoder(mdc_p->get_name());
                tmp_dec_p->setEmMaxIter(mdc_p->get_EmMaxIter());
                tmp_dec_p->setEmTol(mdc_p->get_EmTol());
                tmp_dec_p->setFaMaxIter(mdc_p->get_FaMaxIter());
                tmp_dec_p->setFaTol(mdc_p->get_FaTol());

                if (mdc_p->get_type().compare("LOGTLDS"))
                {
                    tmp_dec_p->setTldsType(LOGTLDS);
                }
                else if (mdc_p->get_type().compare("SQROOTLDS"))
                {
                    tmp_dec_p->setTldsType(SQROOTLDS);
                }
                else
                {
                    std::cout << "invalid TLDS type: "<< mdc_p->get_type()<<std::endl;
                }
                decoder_list.push_back(static_cast<DecoderBase*>(tmp_dec_p));
            }

            if ((*iter)->get_type().compare("ModelFree")==0)
            {
                ModelFreeDecConfig* mdc_p = static_cast<ModelFreeDecConfig*>(*iter);
                ModelFreedecoder* tmp_dec_p;
                tmp_dec_p = new ModelFreedecoder(mdc_p->get_name());
                decoder_list.push_back(static_cast<DecoderBase*>(tmp_dec_p));
            }
        }
        //construct detectors
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        //***** to be done: single decoder params not configurable yet ********
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        std::cout << algCofig->get_detector_config_list().size();
        for (auto iterc = algCofig->get_detector_config_list().begin();iterc < algCofig->get_detector_config_list().end();iterc++)
        {
            std::cout <<"detector type="<< (*iterc)->get_type()<<std::endl;
            // construct single detector
            if ((*iterc)->get_type().compare("single")==0 )
            {
                SingleDetectorConfig* sdc_p = static_cast<SingleDetectorConfig*>(*iterc);
                std::string dec_name =  sdc_p->get_decoder();
                // find the correlated decoder
                for (std::vector<DecoderBase*>::iterator iter= decoder_list.begin();iter<decoder_list.end();iter++)
                {
                    if ((*iter)->get_name().compare(dec_name)==0)
                    {
                        if ((*iter)->get_type().compare("PLDS")==0 )
                        {
                            ModelDetector* tmp_det_p;
                            tmp_det_p = new ModelDetector(sdc_p->get_name(),
                                                          PLDS,
                                                          sdc_p->get_region(),
                                                          sdc_p->get_thresh(),
                                                          sdc_p->get_nStd(),
                                                          sdc_p->get_base_interval(),
                                                          sdc_p->get_updateMode());
                            detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
                        }
                        else if ((*iter)->get_type().compare("TLDS")==0 )
                        {
                            ModelDetector* tmp_det_p;
                            tmp_det_p = new ModelDetector(sdc_p->get_name(),
                                                          TLDS,
                                                          sdc_p->get_region(),
                                                          sdc_p->get_thresh(),
                                                          sdc_p->get_nStd(),
                                                          sdc_p->get_base_interval(),
                                                          sdc_p->get_updateMode());
                            detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
                        }
                        else if ((*iter)->get_type().compare("ModelFree")==0 )
                        {
                            ModelFreeDetector* tmp_det_p;
                            tmp_det_p = new ModelFreeDetector(sdc_p->get_name(),
                                                              sdc_p->get_region(),
                                                              sdc_p->get_thresh(),
                                                              sdc_p->get_base_interval(),
                                                              sdc_p->get_updateMode());
                            detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
                        }
                        break;
                    }
                }
            }
            // construct greedy detector
            else if ((*iterc)->get_type().compare("greedy")==0 )
            {
                GreedyDetectorConfig* gdc_p = static_cast<GreedyDetectorConfig*>(*iterc);
                GreedyDetector* tmp_det_p;
                tmp_det_p = new GreedyDetector(gdc_p->get_name());
                // find the related detector
                for (int i=0;i<gdc_p->get_detector_list().size();i++)
                    for (std::vector<DetectorBase*>::iterator iter= detector_list.begin();iter<detector_list.end();iter++)
                    {
                        if ((*iter)->get_name().compare(gdc_p->get_detector_list()[i])==0)
                        {
                            tmp_det_p->addDetector(*iter,gdc_p->get_lower_thresh_list()[i],gdc_p->get_higher_thresh_list()[i]);
                            break;
                        }
                    }
                detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
            }
            // construct CCF detector
            else if ((*iterc)->get_type().compare("ccf")==0 )
            {
                CCFDetectorConfig* gdc_p = static_cast<CCFDetectorConfig*>(*iterc);
                CCFDetector* tmp_det_p;
                tmp_det_p = new CCFDetector(gdc_p->get_name(),true, gdc_p->get_alpha(),
                                            gdc_p->get_beta(),gdc_p->get_ffactor(),gdc_p->get_aThresh(),gdc_p->get_nStd());
                // find the related detector
                for (int i=0;i<gdc_p->get_detector_list().size();i++)
                    for (std::vector<DetectorBase*>::iterator iter= detector_list.begin();iter<detector_list.end();iter++)
                    {
                        if ((*iter)->get_name().compare(gdc_p->get_detector_list()[i])==0)
                        {
                            tmp_det_p->addDetector(static_cast<ModelDetector*>(*iter));
                            break;
                        }
                    }
                detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
            }
            else if ((*iterc)->get_type().compare("emg")==0 )
            {
                EMGDetectorConfig* gdc_p = static_cast<EMGDetectorConfig*>(*iterc);
                EMGDetector* tmp_det_p = new EMGDetector(gdc_p->get_name(),gdc_p->get_thresh());
                detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
                std::cout << "hit here1" << std::endl;
            }
        }
        //EMGDetector* tmp_det_p = new EMGDetector;
        //detector_list.push_back(static_cast<DetectorBase*>(tmp_det_p));
        //std::cout << "hit here" << std::endl;
    }

    const std::vector<DecoderBase*>& get_decoder_list() const {return decoder_list;}
    const std::vector<DetectorBase*>& get_detector_list() const {return detector_list;}

    ~AlgorithmManager()
    {
        for (std::vector<DecoderBase*>::iterator iter=decoder_list.begin();iter<decoder_list.end();iter++)
        {
            if (*iter != nullptr)
                delete (*iter);
            *iter = nullptr;
        }
        decoder_list.clear();
        for (std::vector<DetectorBase*>::iterator iter=detector_list.begin();iter<detector_list.end();iter++)
        {
            if (*iter != nullptr)
                delete (*iter);
            *iter = nullptr;
        }
        detector_list.clear();
    }
};
}

#endif

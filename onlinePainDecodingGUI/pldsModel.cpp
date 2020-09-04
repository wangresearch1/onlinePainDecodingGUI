#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "pldsmodel.h"
//#include <Eigen/Dense>
//#include <Eigen/Core>
#include "LBFGS_hsl.h"
#include <cmath>
#include <numeric>
using namespace LBFGSpp;
void writeZscore(const std::string filename, Eigen::VectorXf & zscore)
{
    std::ofstream file;
    file.open(filename);
    if (file.is_open())
    {
        file << zscore;
        file.close();
    }
}
void readSeqModelState(const std::string filename, std::vector<PLDSmodel> &model, std::vector<Eigen::MatrixXf> &seq, std::vector<Eigen::VectorXf> & smoothing_x, std::vector<Eigen::VectorXf> & smoothing_v, std::vector<Eigen::VectorXf> & filter_x, std::vector<Eigen::VectorXf> & filter_v, int &units, int &seq_l)
{
    int trials;
    std::ifstream file;
    file.open(filename);
    if(file.is_open())
    {
        file >> trials;
        file >> seq_l;
        file >> units;
        Eigen::MatrixXf seqTrial(seq_l, units);
        PLDSmodel tmpModel;
        for (int t = 0; t < trials; t++)
        {
            // read seq
            for (int s = 0; s < seq_l; s++)
            {
                for (int u = 0; u < units; u++)
                {
                    file >> seqTrial(s, u);
                }
                //printf("\n");
            }
            seq.push_back(seqTrial);
            // read model
            float temp;
            file >> temp;
            tmpModel.setA(temp);
            file >> temp;
            tmpModel.setQ(temp);
            file >> temp;
            tmpModel.setQ0(temp);
            file >> temp;
            tmpModel.setX0(temp);

            Eigen::VectorXf tmpVector(units);
            for (int u = 0; u < units; u++)
            {
                file >> tmpVector(u);
            }
            tmpModel.setC(tmpVector);
            for (int u = 0; u < units; u++)
            {
                file >> tmpVector(u);
            }
            tmpModel.setD(tmpVector);

            model.push_back(tmpModel);
            // read smoothing result
            tmpVector.resize(seq_l);
            for (int s = 0; s < seq_l; s++)
            {
                file >> tmpVector(s);
            }
            smoothing_x.push_back(tmpVector);
            for (int s = 0; s < seq_l; s++)
            {
                file >> tmpVector(s);
            }
            smoothing_v.push_back(tmpVector);
            //read filter result
            tmpVector.resize(seq_l-1);
            for (int s = 0; s < seq_l-1; s++)
            {
                file >> tmpVector(s);
            }
            filter_x.push_back(tmpVector);
            for (int s = 0; s < seq_l-1; s++)
            {
                file >> tmpVector(s);
            }
            filter_v.push_back(tmpVector);
        }
    }
    file.close();
}

void PLDSmodel::expFamPCAInit(const Eigen::MatrixXf & seq, const int units, const int seq_l)
{
    //PLDSmodel tmpModel;
    Eigen::VectorXf default_c = Eigen::VectorXf::Random(units);
    Eigen::VectorXf default_d = Eigen::VectorXf::Ones(units)*(-2);
    //default_c << 1.5992, -0.3923, 0.2208, -0.5937, -2.0504, 0.00042709, 1.6937;
    //default_d << -2, -2, -2, -2, -2, -2, -2;
    setA(0.9);
    setQ(0.19);
    setQ0(1);
    setX0(0);
    setC(default_c);
    setD(default_d);

    //expFamPCA param
    const int dt = 10; // rebin factor
    const float lam = 1; // penalizer
    const int maxIter = 10000;
    const int maxFunEvals = 50000;
    const float progTol = 1e-9;
    const float optTol = 1e-5;
    //rebinRaster
    const int seq_l_new = floor(seq_l / dt);
    Eigen::MatrixXf seq_new(seq_l_new, units);
    for (int i = 0; i < seq_l_new; i++)
    {
        for (int u = 0; u < units; u++)
        {
            seq_new(i, u) = 0;
            for (int j = 0; j < dt; j++)
            {
                seq_new(i, u) += seq(i*dt + j, u);
            }
            //printf("%d ", seq_new(i, u));
        }
        //printf("\n");
    }
    //rough initialization for ExpFamPCA based on SVD
    //prepare zero-mean y
    Eigen::VectorXf mean_y = seq_new.colwise().mean();

    //svd
    Eigen::MatrixXf y_zero_mean = seq_new.rowwise() - mean_y.transpose();
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(y_zero_mean.transpose(), Eigen::ComputeThinU | Eigen::ComputeThinV);
    //Eigen::VectorXf cxdInit = Eigen::VectorXf::Random(units * 2 + seq_l_new) * 0.01;
    //Eigen::VectorXf xInit(seq_l_new);
    //xInit << 0.0101868528212858, -0.00133217479507735, -0.00714530163787158, 0.0135138576842666, -0.00224771056052584, -0.00589029030720801, -0.00293753597735416, -0.00847926243637934, -0.0112012830124373, 0.0252599969211831, 0.0165549759288735, 0.00307535159238252, -0.0125711835935205, -0.00865468030554804, -0.00176534114231451, 0.00791416061628634, -0.0133200442131525, -0.0232986715580508, -0.0144909729283874, 0.00333510833065806, 0.00391353604432901, 0.00451679418928238, -0.00130284653145721, 0.00183689095861942, -0.00476153016619074, 0.00862021611556922, -0.0136169447087075, 0.00455029556444334, -0.00848709379933659, -0.00334886938964048, 0.00552783345944550, 0.0103909065350496, -0.0111763868326521, 0.0126065870912090, 0.00660143141046978, -0.000678655535426873, -0.00195221197898754, -0.00217606350143192, -0.00303107621351741, 0.000230456244251053;
    Eigen::VectorXf xInit = Eigen::VectorXf::Random(seq_l_new) * 0.01;
    Eigen::VectorXf cInit = svd.matrixU().col(0);
    Eigen::VectorXf dInit = mean_y.array().cwiseMax(0.1).log();

    Eigen::VectorXf cxdInit(units * 2 + seq_l_new);
    cxdInit << cInit, xInit, dInit;
    //std::cout << " cd="<<cxdInit.transpose()<<std::endl;

    // LBFGS optimization
    LBFGSParam<float> param;
    param.epsilon = optTol;
    param.max_iterations = maxIter;
    param.linesearch = LBFGS_LINESEARCH_BACKTRACKING_WOLFE;
    param.m = 100;
    const double lambda = 1;
    LBFGSSolver<float> solver(param);
    ExpFamPCACost fun(units, seq_l_new, seq_new, lambda);
    float fx;
    const int niter = solver.minimize(fun, cxdInit, fx);

    if (std::isinf(cxdInit.array().sum()) || std::isinf(fx))
    {
        std::cout << "expPCAInit: infinite result" << std::endl;
        std::cout <<"niter="<<niter<<" fx="<<fx<< " cd="<<cxdInit.transpose()<<std::endl;
        //break;
    }

    //Function returns all parameters lumped together as one vector, so need to disentangle:
    Eigen::VectorXf c = cxdInit.head(units);
    Eigen::VectorXf x = cxdInit.segment(units, seq_l_new);
    Eigen::VectorXf d = cxdInit.tail(units);
    const float mean_x = x.mean();
    x = x.array() - mean_x;
    d = d.array() - log(dt);
    d += c * mean_x;
    // normalization
    const float scale = c.norm();
    c /= scale;
    x *= scale;
    setC(c);
    setD(d);

    //estimate LDS observation
    float pi = x.transpose() * x;
    pi /= seq_l_new;

    Eigen::MatrixXf A = x.head(seq_l_new - 1).colPivHouseholderQr().solve(x.tail(seq_l_new - 1));
    //std::cout << A(0,0) <<std::endl;
    const float a = std::min(std::max(A(0, 0), float(0.1)), float(1));
    setA(a);
    float q = pi - a*pi*a;
    setQ(q);
    setQ0(q / (1 - a*a));
    setX0(0);
}

void PLDSmodel::trainEmLaplace_old(const Eigen::MatrixXf & seq, const int units, const int seq_l, const float binSize, const int maxIter, const int maxCpuTime_s)
{
    float a;// = (*this).getA();
    float q;// = (*this).getQ();
    float x0 = getX0();
    float q0 = getQ0();
    Eigen::VectorXf c;// = (*this).getC();
    Eigen::VectorXf cdInit(units * 2);
    Eigen::VectorXf d;// = (*this).getD();
    //temporal variables
    float x_pred, q_pred, q_filt, x_filt,b;
    Eigen::VectorXf rate(units);
    Eigen::VectorXf y_pred(units);
    Eigen::VectorXf x_pred_v(seq_l);
    Eigen::VectorXf x_filt_v(seq_l);
    Eigen::VectorXf q_pred_v(seq_l);
    Eigen::VectorXf q_filt_v(seq_l);
    Eigen::VectorXf x_smooth_v(seq_l);//xsm
    Eigen::VectorXf q_smooth_v(seq_l);//Vsm
    Eigen::VectorXf vv_smooth_v(seq_l-1);//VVsm

    for (int iter = 0; iter < maxIter; iter++)
    {
        a = getA();
        q = getQ();
        //x0 = (*this).getX0();
        //q0 = (*this).getQ0();
        c = getC();
        d = getD();
        //--- E-step ---
        //forward filter
        for (int t = 0; t < seq_l; t++)
        {
            //prediction
            x_pred = a*x0;
            q_pred = a*q0*a + q;
            rate = (c*x_pred + d).array().exp();
            y_pred = rate*binSize;
            //filtering
            q_filt = 1 / (1/q_pred + c.cwiseProduct(y_pred).transpose()*c);
            //std::cout << q_filt * (c.transpose()*(seq.row(t).transpose() - y_pred)) << std::endl;
            x_filt = x_pred + (q_filt * (c.transpose()*(seq.row(t).transpose()-y_pred)))(0,0);
            x_pred_v(t) = x_pred;
            x_filt_v(t) = x_filt;
            q_pred_v(t) = q_pred;
            q_filt_v(t) = q_filt;
            //update
            x0 = x_filt;
            q0 = q_filt;
        }
        x_smooth_v(seq_l - 1) = x_filt_v(seq_l - 1);
        q_smooth_v(seq_l - 1) = q_filt_v(seq_l - 1);
        //backward smoothing
        for (int t = seq_l - 2; t >= 0; t--)
        {
            b = a * q_filt_v(t) / q_pred_v(t + 1);
            x_smooth_v(t) = x_filt_v(t) + b*(x_smooth_v(t + 1) - x_pred_v(t + 1));//xsm
            q_smooth_v(t) = q_filt_v(t) + b*(q_smooth_v(t + 1) - q_pred_v(t + 1))*b;//Vsm
        }
        //compute VVsm -- posterior time-lag 1 covariance
        for (int t = 0; t < seq_l-1; t++)
        {
            vv_smooth_v(t) = sqrt(q_smooth_v(t)*q_smooth_v(t + 1));
            if (!(vv_smooth_v(t) == vv_smooth_v(t)))
            {
                std::cout << t << " " << vv_smooth_v(t) << " " << q_smooth_v(t) << " " << q_smooth_v(t+1) << std::endl;
            }
        }

        //--- M-step ---
        //LDS
        Eigen::VectorXf MUsm0 = x_smooth_v.head(seq_l - 1);
        Eigen::VectorXf MUsm1 = x_smooth_v.tail(seq_l - 1);
        float s11 = q_smooth_v.tail(seq_l - 1).sum() + MUsm1.transpose()*MUsm1;
        double vv_sum = vv_smooth_v.sum();
        //std::cout << vv_smooth_v.transpose() << std::endl;
        double MUsm01 = MUsm0.transpose()*MUsm1;
        float s01 = vv_sum + MUsm01;
        float s00 = q_smooth_v.head(seq_l - 1).sum() + MUsm0.transpose()*MUsm0;
        if (!(s01 == s01))
        {
            s01 = std::max(vv_sum, MUsm01);
        }
        //std::cout << "q_smooth_v.tail(seq_l - 1).sum()" << std::endl;
        //std::cout << q_smooth_v.tail(seq_l - 1).sum() << std::endl;
        //std::cout << "MUsm0" << std::endl;
        //std::cout << MUsm0 << std::endl;
        //std::cout << "MUsm1" << std::endl;
        //std::cout << MUsm1 << std::endl;
        //std::cout << "MUsm0.transpose()*MUsm1" << std::endl;
        //std::cout << MUsm0.transpose()*MUsm1 << std::endl;
        //std::cout << "vv_smooth_v.sum() + MUsm0.transpose()*MUsm1" << std::endl;
        //std::cout << vv_smooth_v.sum() + MUsm0.transpose()*MUsm1 << std::endl;
        //double a1 = vv_smooth_v.sum();
        //double a2 = MUsm0.transpose()*MUsm1;
        //std::cout << log(exp(a1)*exp(a2)) << std::endl;

        setA(s01 / s00);
        setQ((s11+(*this).getA()*s00*(*this).getA()-2*s01*(*this).getA())/400.0);
        setX0(x_smooth_v(0));
        setQ0(q_smooth_v(0));
        //std::cout << (*this).getA() << ',' << (*this).getQ() << ',' << (*this).getX0() << ',' << (*this).getQ0() << std::endl;
        //---- note ---- matlab code considered situation of q<0 or q0<0, but no solution provided

        //Observation
        cdInit << c, d;
        // LBFGS optimization
        float progTol = 1e-9;
        float optTol = 1e-5;
        LBFGSParam<float> param;
        param.epsilon = optTol;
        param.max_iterations = 500;
        param.linesearch = LBFGS_LINESEARCH_BACKTRACKING_WOLFE;
        param.m = 100;
        double lambda = 1;
        LBFGSSolver<float> solver(param);
        PLDSMStepObservationCost fun(units, seq_l, seq, x_smooth_v, q_smooth_v);
        float fx;
        //std::cout << cdInit << std::endl;
        int niter = solver.minimize(fun, cdInit, fx);
        //std::cout << cdInit << std::endl;
        c = cdInit.head(units);
        d = cdInit.tail(units);
        setC(c);
        setD(d);

        //std::cout << iter << std::endl;
        //std::cout << (*this).getA() << std::endl;
        //std::cout << (*this).getQ() << std::endl;
        //std::cout << (*this).getX0() << std::endl;
        //std::cout << (*this).getQ0() << std::endl;
        //std::cout << std::endl;
        //std::cout << (*this).getC() << std::endl;
        //std::cout << std::endl;
        //std::cout << (*this).getD() << std::endl;
        //std::cout << std::endl;
    }
    setXsm(x_smooth_v);
    setVsm(q_smooth_v);
    //std::cout << (*this).getA() << std::endl;
    //std::cout << (*this).getQ() << std::endl;
    //std::cout << (*this).getX0() << std::endl;
    //std::cout << (*this).getQ0() << std::endl;
    //std::cout << std::endl;
    //std::cout << (*this).getC() << std::endl;
    //std::cout << std::endl;
    //std::cout << (*this).getD() << std::endl;
}
void PLDSmodel::trainEmLaplace(const Eigen::MatrixXf & seq, const int units, const int seq_l, const float binSize, const int maxIter, const int maxCpuTime_s)
{
    float a;// = (*this).getA();
    float q;// = (*this).getQ();
    float x0;// = (*this).getX0();
    float q0;// = (*this).getQ0();
    Eigen::VectorXf c(units);// = (*this).getC();
    Eigen::VectorXf cdInit(units * 2);
    Eigen::VectorXf d(units);// = (*this).getD();
    //temporal variables
    float x_pred, q_pred, q_filt, x_filt, b;
    Eigen::VectorXf rate(units);
    Eigen::VectorXf y_pred(units);
    Eigen::VectorXf x_pred_v(seq_l);
    Eigen::VectorXf x_filt_v(seq_l);
    Eigen::VectorXf q_pred_v(seq_l);
    Eigen::VectorXf q_filt_v(seq_l);
    Eigen::VectorXf x_smooth_v(seq_l);//xsm
    Eigen::VectorXf q_smooth_v(seq_l);//Vsm
    Eigen::VectorXf vv_smooth_v(seq_l - 1);//VVsm

    //direct optimization method variables/params
    Eigen::VectorXf xx(seq_l);
    Eigen::VectorXf xxOld(seq_l);
    Eigen::VectorXf XX(seq_l-1);
    Eigen::VectorXf QiXX(seq_l);
    Eigen::MatrixXf y_pred_m;
    Eigen::MatrixXf lambda;
    Eigen::MatrixXf YC;
    Eigen::VectorXf lat_grad(seq_l);
    Eigen::VectorXf grad(seq_l);
    Eigen::VectorXf lat_hess_diag(seq_l);
    Eigen::VectorXf poiss_hess(seq_l);
    Eigen::VectorXf update(seq_l);
    Eigen::MatrixXf hess = Eigen::MatrixXf::Zero(seq_l, seq_l);

    //sym_blk_tridiag_inv_v1_sp variables
    Eigen::VectorXf S(seq_l - 1);
    Eigen::VectorXf V(seq_l);
    Eigen::VectorXf VV(seq_l - 1);


    float ii = 0;
    float logdet = 0;
    float dx;
    float X0;
    float pll;
    float gnll;

    //for test
    /*setA(0.675356109903864);
        setQ(0.867660983384669);
        setQ0(1.595275521093311);
        c << -0.471314293584756,
                0.161899648186566,
                0.090236436122432,
                0.362438433828862,
                0.224942408985201,
                0.746838254130283,
                0.061486930862886;
        d << -0.651522174420826,
                - 1.407892811997443,
                - 0.766932083053808,
                - 0.749813380849120,
                - 0.909291408653864,
                - 0.921122041933229,
                - 0.595446049399480;
        setC(c);
        setD(d);*/

    for (int iter = 0; iter < maxIter; iter++)
    {
        a = getA();
        q = getQ();
        c = getC();
        d = getD();
        if (q < 1e-6)
        {
            //for debugging
            break;
        }
        x0 = getX0();
        q0 = getQ0();

        //E-step - direct optimization method
        // getPriorMeanLDS
        xx(0) = x0;
        for (int t = 1; t < seq_l; t++)
        {
            xx(t) = a*xx(t - 1);
        }

        //PLDSLaplaceInferenceCore
        float q_inv = 1 / q;
        float q0_inv = 1 / q0;
        float a_qi_a = a*a / q;
        float ll = -INFINITY;
        float ll_prev = -INFINITY;
        int e_iter=0;
        while (1)
        {
            if (e_iter++ > 10)
            {
                break;
            }
            //break when converge
            if (ll - ll_prev < 1e-10)
            {
                break;
            }

            XX = xx.tail(seq_l - 1) - xx.head(seq_l - 1)*a;
            QiXX << -q0_inv*(xx(0) - x0), -q_inv*XX;

            y_pred_m = (c*xx.transpose()).colwise() + d;

            lat_grad << QiXX.head(seq_l - 1) - a*QiXX.tail(seq_l - 1), QiXX(seq_l - 1);

            lat_hess_diag << q0_inv + a_qi_a, Eigen::VectorXf::Ones(seq_l - 2)*(q_inv + a_qi_a), q_inv;
            lat_hess_diag = -lat_hess_diag;
            lambda = y_pred_m.array().exp();
            poiss_hess = -c.transpose()*lambda.cwiseProduct(c.replicate(1, lambda.cols()));

            hess.diagonal() = lat_hess_diag + poiss_hess;
            // set off-diagonal values
            for (int i = 0; i < seq_l - 1; i++)
            {
                hess(i, i + 1) = a*q_inv;
                hess(i + 1, i) = a*q_inv;
            }
            YC = (seq.transpose() - lambda).cwiseProduct(c.replicate(1, lambda.cols()));
            grad = YC.colwise().sum() + lat_grad.transpose();

            //update = hess.inverse()*grad;
            update = hess.ldlt().solve(grad);

            ll_prev = ll;
            xxOld = xx;

            ii = 0;
            logdet = log(q0) + (seq_l - 1)*log(q);
            //Newton'method line scan loop
            while (1)
            {
                dx = pow(2,-ii);
                //step size too small
                if (dx < 0.001)
                {
                    break;
                }
                ii += 0.1;
                xx = xxOld - dx*update;
                //Recompute just the likelihood @ dx step
                y_pred_m = (c*xx.transpose()).colwise() + d;
                lambda = y_pred_m.array().exp();

                XX = xx.tail(seq_l - 1) - xx.head(seq_l - 1)*a;
                X0 = xx(0) - x0;
                pll = (y_pred_m.cwiseProduct(seq.transpose())-lambda).array().sum();
                gnll = logdet + (XX.cwiseProduct(XX)*q_inv).array().sum() + X0*q0_inv*X0;
                ll = pll - gnll / 2;
                //found a larger likelihood
                if (ll > ll_prev)
                {
                    break;
                }
            }
        }
        //sym_blk_tridiag_inv_v1_sp
        Eigen::VectorXf diagBlk = -hess.diagonal();
        float offDiagBlk = a*q_inv;
        S(seq_l - 2) = offDiagBlk / diagBlk(seq_l - 1);
        for (int i = seq_l - 3; i >= 0; i--)
        {
            S(i) = offDiagBlk / (diagBlk(i + 1) - S(i + 1)*offDiagBlk);
        }
        V(0) = 1 / (diagBlk(0) - offDiagBlk*S(0));
        VV(0) = S(0)*V(0);
        for (int i = 1; i < seq_l - 1; i++)
        {
            V(i) = (1+offDiagBlk*V(i-1)*S(i-1))/(diagBlk(i) - offDiagBlk*S(i));
            VV(i) = S(i)*V(i);
        }
        V(seq_l - 1) = (1 + offDiagBlk*V(seq_l - 2)*S(seq_l - 2)) / diagBlk(seq_l - 1);
        x_smooth_v = xx;
        q_smooth_v = V;
        vv_smooth_v = VV;

        //--- M-step ---
        //LDS
        Eigen::VectorXf MUsm0 = x_smooth_v.head(seq_l - 1);
        Eigen::VectorXf MUsm1 = x_smooth_v.tail(seq_l - 1);
        float s11 = q_smooth_v.tail(seq_l - 1).sum() + MUsm1.transpose()*MUsm1;
        double vv_sum = vv_smooth_v.sum();
        //std::cout << vv_smooth_v.transpose() << std::endl;
        double MUsm01 = MUsm0.transpose()*MUsm1;
        float s01 = vv_sum + MUsm01;
        float s00 = q_smooth_v.head(seq_l - 1).sum() + MUsm0.transpose()*MUsm0;
        if (!(s01 == s01))
        {
            s01 = std::max(vv_sum, MUsm01);
        }
        //std::cout << "q_smooth_v.tail(seq_l - 1).sum()" << std::endl;
        //std::cout << q_smooth_v.tail(seq_l - 1).sum() << std::endl;
        //std::cout << "MUsm0" << std::endl;
        //std::cout << MUsm0 << std::endl;
        //std::cout << "MUsm1" << std::endl;
        //std::cout << MUsm1 << std::endl;
        //std::cout << "MUsm0.transpose()*MUsm1" << std::endl;
        //std::cout << MUsm0.transpose()*MUsm1 << std::endl;
        //std::cout << "vv_smooth_v.sum() + MUsm0.transpose()*MUsm1" << std::endl;
        //std::cout << vv_smooth_v.sum() + MUsm0.transpose()*MUsm1 << std::endl;
        //double a1 = vv_smooth_v.sum();
        //double a2 = MUsm0.transpose()*MUsm1;
        //std::cout << log(exp(a1)*exp(a2)) << std::endl;

        setA(s01 / s00);
        setQ((s11 + getA()*s00*getA() - 2 * s01*getA()) / (seq_l - 1));
        setX0(x_smooth_v(0));
        setQ0(q_smooth_v(0));

        //---- note ---- matlab code considered situation of q<0 or q0<0, but no solution provided

        //Observation
        cdInit << c, d;
        // LBFGS optimization
        float progTol = 1e-9;
        float optTol = 1e-5;
        LBFGSParam<float> param;
        param.epsilon = optTol;
        param.max_iterations = 500;
        param.linesearch = LBFGS_LINESEARCH_BACKTRACKING_WOLFE;
        param.m = 100;
        param.max_linesearch = 25;
        double lambda = 1;
        LBFGSSolver<float> solver(param);
        PLDSMStepObservationCost fun(units, seq_l, seq, x_smooth_v, q_smooth_v);
        float fx;

        //std::cout <<" iter="<<iter<<" fx="<<fx<< " cd="<<cdInit.transpose()<<std::endl;
        int niter = solver.minimize(fun, cdInit, fx);

        //if (std::isinf<float>(fx))
        if (std::isinf(cdInit.array().sum()) || std::isinf(fx))
        {
            std::cout << "infinite result, abandon and break" << std::endl;
            std::cout <<"niter="<<niter<<" iter="<<iter<<" fx="<<fx<< " cd="<<cdInit.transpose()<<std::endl;
            break;
        }

        c = cdInit.head(units);
        d = cdInit.tail(units);
        setC(c);
        setD(d);

        //std::cout << iter << std::endl;
        //std::cout << getA() << std::endl;
        //std::cout << getQ() << std::endl;
        //std::cout << getX0() << std::endl;
        //std::cout << getQ0() << std::endl;
        //std::cout << std::endl;
        //std::cout << getC() << std::endl;
        //std::cout << std::endl;
        //std::cout << getD() << std::endl;
        //std::cout << std::endl;
    }
    setXsm(x_smooth_v);
    setVsm(q_smooth_v);
    //std::cout << (*this).getA() << std::endl;
    //std::cout << (*this).getQ() << std::endl;
    //std::cout << (*this).getX0() << std::endl;
    //std::cout << (*this).getQ0() << std::endl;
    //std::cout << std::endl;
    //std::cout << (*this).getC() << std::endl;
    //std::cout << std::endl;
    //std::cout << (*this).getD() << std::endl;
}

void PLDSmodel::PLDSOnlineFilterTrial(const Eigen::MatrixXf& seq, const int units, const int seq_l, const float binSize, Eigen::VectorXf &x, Eigen::VectorXf &V)
{
    float x0 = getXsm()(0);
    float q0 = getVsm()(0);
    //float a = getA();
    //float q = getQ();
    //Eigen::VectorXf c = getC();
    //Eigen::VectorXf d = getD();

    float x_pred, q_pred, q_filt, x_filt, b;
    Eigen::VectorXf rate(units);
    Eigen::VectorXf y_pred(units);
    //Eigen::VectorXf x_pred_v(seq_l-1);
    //Eigen::VectorXf x_filt_v(seq_l-1);
    //Eigen::VectorXf q_pred_v(seq_l-1);
    //Eigen::VectorXf q_filt_v(seq_l-1);

    for (int t = 1; t < seq_l; t++)
    {
        //prediction
        x_pred = a*x0;
        q_pred = a*q0*a + q;
        rate = (c*x_pred + d).array().exp();
        y_pred = rate *binSize;
        //filtering
        q_filt = 1 / (1 / q_pred + c.cwiseProduct(y_pred).transpose()*c);
        //std::cout << q_filt * (c.transpose()*(seq.row(t).transpose() - y_pred)) << std::endl;
        x_filt = x_pred + (q_filt * (c.transpose()*(seq.row(t).transpose() - y_pred)))(0, 0);
        //x_pred_v(t) = x_pred;
        x(t-1) = x_filt;
        //q_pred_v(t) = q_pred;
        V(t-1) = q_filt;
        //update
        //std::cout << t<<':'<<x_filt << std::endl;
        x0 = x_filt;
        q0 = q_filt;
    }
    //std::cout << x << std::endl;
    //std::cout << std::endl;
    //std::cout << V << std::endl;
}
void PLDSmodel::PLDSOnlineFilterBin(const Eigen::VectorXf& seq, const float binSize, const float x0, const float q0, float &x, float &V)
{
    //float a = getA();
    //float q = getQ();
    //Eigen::VectorXf c = getC();
    //Eigen::VectorXf d = getD();
    int units = seq.rows();

    float x_pred, q_pred, q_filt, x_filt, b;
    Eigen::VectorXf rate(units);
    Eigen::VectorXf y_pred(units);
    //prediction
    x_pred = a*x0;
    q_pred = a*q0*a + q;
    rate = (c*x_pred + d).array().exp();
    y_pred = rate *binSize;
    //filtering
    q_filt = 1 / (1 / q_pred + c.cwiseProduct(y_pred).transpose()*c);
    //std::cout << q_filt * (c.transpose()*(seq.row(t).transpose() - y_pred)) << std::endl;
    x_filt = x_pred + (q_filt * (c.transpose()*(seq - y_pred)))(0, 0);
    //std::cout<<"seq - y_pred="<<(seq - y_pred).transpose()<<std::endl;
    //std::cout<<"(c.transpose()*(seq - y_pred))="<<(c.transpose()*(seq - y_pred))<<" x0="<<x0<<" x_pred="<<x_pred<<" x_filt="<<x_filt<<" q_filt="<<q_filt<<std::endl;
    //x_pred_v(t) = x_pred;
    x = x_filt;
    //q_pred_v(t) = q_pred;
    V = q_filt;
    //update
    //std::cout << t<<':'<<x_filt << std::endl;
}
Eigen::VectorXf PLDSmodel::getBaselineX(const int startIdx, const int endIdx)
{
    int length;
    if (startIdx>endIdx)
    {
        length = bufSize-startIdx + endIdx +1;
    }
    else
    {
        length = endIdx-startIdx+1;
    }
    Eigen::VectorXf tmpX(length);
    int idx = startIdx;
    int idxV = 0;
    while(idx!=endIdx)
    {
        tmpX(idxV) = getXsmBufValue(idx);
        idx = (idx+1)%bufSize;
        idxV++;
    }
    return tmpX;
}

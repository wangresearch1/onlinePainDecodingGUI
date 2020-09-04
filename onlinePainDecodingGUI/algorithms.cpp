/**************************************************************************
---------------------------------------------------------------------------
The algorithms for decoding and detecting pain onset from neuronal ensemble
activities.

More details can be found in following papers and websites:
1.Chen Z, Wang J (2016). Statistical analysis of neuronal population codes
for encoding acute pain. Proc. ICASSP.
2.Chen Z, et al. (2017). Deciphering neuronal population codes for acute
thermal pain. J. Neural Eng. 14(3): 036023.
3.Koepcke L, Ashida G, Kretzberg K (2016). Single and multiple change point
detection in spike trains: Comparison of different CUSUM methods. Front.
Syst. Neurosci., 10: 51.
4.Hu S, Zhang Q, Wang J, Chen Z (2017). A real-time rodent neural interface
for deciphering acute pain signals from neuronal ensemble spike activity.
Proc. 51st Asilomar Conf.
5.Hu S, Zhang Q, Wang J, Chen Z. Real-time particle filtering and smoothing
algorithms for detecting abrupt changes in neural ensemble spike activity.
J. Neurophysiol. (revision)
6.Chen Z, Hu S, Zhang Q, Wang J (2017). Quickest Detection for Abrupt Changes
in Neuronal Ensemble Spiking Activity Using Model-based and Model-free
Approaches. IEEE EMBS Conf. on Neural Eng.
7.Xiao Z, Hu S, Zhang Q, Wang J, Chen Z. Ensembles of change-point detectors:
Implications fo real-time BMI applications. J. Comput. Neurosci. (submitted)

author: Sile Hu, yuehusile@gmail.com
version: v1.0
date: 2017-12-12
___________________________________________________________________________
***************************************************************************/


#include "algorithms.h"
#include "LBFGS_hsl.h"
#include "ui_paindetectclient.h"
#include "paindetectclient.h"
#include <cmath>
#define fix_d
namespace painBMI
{
// ++++++++++++++decoder related classes+++++++++++++++++++++++++++++++++++++++++++++++++
/************************************************************************
*               Model based decoder                                     *
*************************************************************************/
/*
 * update baseline mean & std of model-based decoders.
 * input: data - baseline latent variables, for PLDS/TLDS data is vector
 * ouput: void
 */
void Modeldecoder::updateBaseline(const Eigen::MatrixXf& data)
{
    if (data.rows()==0)
    {
        std::cout << "empty data seq"<<std::endl;
        return;
    }
    meanBase = data.mean();
    stdBase = sqrt(data.array().pow(2).mean() - meanBase*meanBase);
}
/************************************************************************
*               Model based decoder: PLDS                               *
*************************************************************************/

/*
 * initialization for PLDS model, using FamPCA.
 * input: data - training data spikes, cols-units/rows-bins
 * ouput: void
 */
void PLDSdecoder::expFamPCAInit(const Eigen::MatrixXf & data)
{
    c = Eigen::VectorXf::Random(units);
    d = Eigen::VectorXf::Ones(units)*(-2);
    a = 0.9;
    q = 0.19;
    q0 = 1;
    x0 = 0;

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
                seq_new(i, u) += data(i*dt + j, u);
            }
        }
    }
    //rough initialization for ExpFamPCA based on SVD
    //prepare zero-mean y
    Eigen::VectorXf mean_y = seq_new.colwise().mean();

    //svd
    Eigen::MatrixXf y_zero_mean = seq_new.rowwise() - mean_y.transpose();
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(y_zero_mean.transpose(), Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::VectorXf xInit = Eigen::VectorXf::Random(seq_l_new) * 0.01;
    Eigen::VectorXf cInit = svd.matrixU().col(0);
    Eigen::VectorXf dInit = mean_y.array().cwiseMax(0.1).log();

#ifndef fix_d
    Eigen::VectorXf cxdInit(units * 2 + seq_l_new);
    cxdInit << cInit, xInit, dInit;
#else
    Eigen::VectorXf cxdInit(units + seq_l_new);
    cxdInit << cInit, xInit;
#endif

    //std::cout << " cd=" << cxdInit.transpose() << std::endl;

    // LBFGS optimization
    LBFGSpp::LBFGSParam<float> param;
    param.epsilon = optTol;
    param.max_iterations = maxIter;
    param.linesearch = LBFGSpp::LBFGS_LINESEARCH_BACKTRACKING_WOLFE;
    param.m = 100;
    const double lambda = 1;
    LBFGSpp::LBFGSSolver<float> solver(param);
#ifndef fix_d
    ExpFamPCACost fun(units, seq_l_new, seq_new, lambda);
#else
    ExpFamPCACost_fixd fun(units, seq_l_new, seq_new, lambda, dInit);
#endif
    float fx;
    const int niter = solver.minimize(fun, cxdInit, fx);

    if (std::isinf(cxdInit.array().sum()) || std::isinf(fx))
    {
        std::cout << "expPCAInit: infinite result" << std::endl;
        //std::cout << "niter=" << niter << " fx=" << fx << " cd=" << cxdInit.transpose() << std::endl;
        //break;
    }

    //std::cout << "niter=" << niter << " fx=" << fx << " cd=" << cxdInit.transpose() << std::endl;

    //Function returns all parameters lumped together as one vector, so need to disentangle:
    c = cxdInit.head(units);
    Eigen::VectorXf x = cxdInit.segment(units, seq_l_new);
#ifndef fix_d
    d = cxdInit.tail(units);
#else
    d = dInit;
#endif
    const float mean_x = x.mean();
    x = x.array() - mean_x;
    d = d.array() - log(dt);
    d += c * mean_x;

    // normalization
    const float scale = c.norm();
    c /= scale;
    x *= scale;

    //estimate LDS observation
    float pi = x.transpose() * x;
    pi /= seq_l_new;

    Eigen::MatrixXf A = x.head(seq_l_new - 1).colPivHouseholderQr().solve(x.tail(seq_l_new - 1));
    //std::cout << A(0,0) <<std::endl;
    a = std::min(std::max(A(0, 0), float(0.1)), float(1));
    q = pi - a*pi*a;
    q0 = q / (1 - a*a);
    x0 = 0;
}

/*
 * train PLDS model, using Laplacian EM.
 * input: data - training data spikes, cols-units/rows-bins
 * ouput: void
 */
void PLDSdecoder::train(const Eigen::MatrixXf& data)
{
    units = data.cols();
    seq_l = data.rows();
    //        units = data.rows();
    //        seq_l = data.cols();
    std::cout <<"units"<< units << std::endl;
    std::cout <<"seq_l"<< seq_l << std::endl;

    expFamPCAInit(data);
    std::cout << "a=" << a << std::endl;
    std::cout << "q=" << q << std::endl;
    //a = 0.179;
    //q = 1.84;

    //c = Eigen::VectorXf(units);
    //d = Eigen::VectorXf(units);
    xsm = Eigen::VectorXf(seq_l);
    Vsm = Eigen::VectorXf(seq_l);

    //+++++++++++++++++++ init tmp variables ++++++++++++++++
#ifndef fix_d
    Eigen::VectorXf cdInit(units * 2);
#else
    Eigen::VectorXf cdInit(units);
#endif
    //temporal variables
    float x_pred, q_pred, q_filt, x_filt, b;
    Eigen::VectorXf rate(units);
    Eigen::VectorXf y_pred(units);
    Eigen::VectorXf x_pred_v(seq_l);
    Eigen::VectorXf x_filt_v(seq_l);
    Eigen::VectorXf q_pred_v(seq_l);
    Eigen::VectorXf q_filt_v(seq_l);
    Eigen::VectorXf vv_smooth_v(seq_l - 1);//VVsm

    //direct optimization method variables/params
    Eigen::VectorXf xx(seq_l);
    Eigen::VectorXf xxOld(seq_l);
    Eigen::VectorXf XX(seq_l - 1);
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


    for (int iter = 0; iter < maxIter; iter++)
    {
        std::cout << iter << std::endl;
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
        int e_iter = 0;
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
            YC = (data.transpose() - lambda).cwiseProduct(c.replicate(1, lambda.cols()));
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
                dx = pow(2, -ii);
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
                pll = (y_pred_m.cwiseProduct(data.transpose()) - lambda).array().sum();
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
            V(i) = (1 + offDiagBlk*V(i - 1)*S(i - 1)) / (diagBlk(i) - offDiagBlk*S(i));
            VV(i) = S(i)*V(i);
        }
        V(seq_l - 1) = (1 + offDiagBlk*V(seq_l - 2)*S(seq_l - 2)) / diagBlk(seq_l - 1);
        xsm = xx;
        Vsm = V;
        vv_smooth_v = VV;

        //--- M-step ---
        //LDS
        Eigen::VectorXf MUsm0 = xsm.head(seq_l - 1);
        Eigen::VectorXf MUsm1 = xsm.tail(seq_l - 1);
        float s11 = Vsm.tail(seq_l - 1).sum() + MUsm1.transpose()*MUsm1;
        double vv_sum = vv_smooth_v.sum();
        //std::cout << vv_smooth_v.transpose() << std::endl;
        double MUsm01 = MUsm0.transpose()*MUsm1;
        float s01 = vv_sum + MUsm01;
        float s00 = Vsm.head(seq_l - 1).sum() + MUsm0.transpose()*MUsm0;
        if (!(s01 == s01))
        {
            s01 = std::max(vv_sum, MUsm01);
        }

        a = s01 / s00;
        q = (s11 + getA()*s00*getA() - 2 * s01*getA()) / (seq_l - 1);
        //x0 = xsm(0);
        //q0 = Vsm(0);
        x0 = xsm(seq_l - 1);
        q0 = Vsm(seq_l - 1);

        //---- note ---- matlab code considered situation of q<0 or q0<0, but no solution provided

        //Observation
#ifndef fix_d
        cdInit << c, d;
#else
        cdInit << c;
#endif
        // LBFGS optimization
        float progTol = 1e-9;
        float optTol = 1e-5;
        LBFGSpp::LBFGSParam<float> param;
        param.epsilon = optTol;
        param.max_iterations = 500;
        param.linesearch = LBFGSpp::LBFGS_LINESEARCH_BACKTRACKING_WOLFE;
        param.m = 100;
        param.max_linesearch = 25;
        double lambda = 1;
        LBFGSpp::LBFGSSolver<float> solver(param);
#ifndef fix_d
        PLDSMStepObservationCost fun(units, seq_l, data, xsm, Vsm);
#else
        PLDSMStepObservationCost_fixd fun(units, seq_l, data, xsm, Vsm, d);
#endif
        float fx;

        int niter = solver.minimize(fun, cdInit, fx);

        if (std::isinf(cdInit.array().sum()) || std::isinf(fx))
        {
            std::cout << "infinite result, abandon and break" << std::endl;
            //std::cout << "niter=" << niter << " iter=" << iter << " fx=" << fx << " cd=" << cdInit.transpose() << std::endl;
            break;
        }

        c = cdInit.head(units);
#ifndef fix_d
        d = cdInit.tail(units);
#endif
    }
    init = true;
}

/*
 * decode with PLDS model.
 * input: data - data spikes to be decode, rows-units
 * ouput: decoded latent variable
 * +++++++++++++++++++++++++++++++++++++++++++++++
 * NOTE: x0 and q0 will be updated after each call
 * +++++++++++++++++++++++++++++++++++++++++++++++
 */
float PLDSdecoder::decode(const Eigen::VectorXf& data)
{
    if (!init) return 0;
    int v_units = data.rows();
    if (v_units != units)
    {
        std::cout << "PLDSdecoder:vector length different from model"
                  << "v_units=" << v_units << "model units=" << units << std::endl;
        return 0;
    }

    float x_pred, q_pred, q_filt, x_filt;
    Eigen::VectorXf rate(units);
    Eigen::VectorXf y_pred(units);
    //prediction
    x_pred = a*x0;
    q_pred = a*q0*a + q;
    rate = (c*x_pred + d).array().exp();
    y_pred = rate *binSize;

    //filtering
    q_filt = 1 / (1 / q_pred + c.cwiseProduct(y_pred).transpose()*c);
    x_filt = x_pred + (q_filt * (c.transpose()*(data - y_pred)))(0, 0);
    x0 = x_filt;
    q0 = q_filt;

    //std::cout<<"x0="<<x0<<" q0="<<q0<<std::endl;
    return x0;
}

/************************************************************************
*               Model based decoder: TLDS                               *
*************************************************************************/

/*
 * initialization for TLDS model, using Factor analysis.
 * input: data - training data spikes, cols-units/rows-bins
 * ouput: void
 */
//TO BE FIXED: numerical problem occured for low firing rate data (a lot of zeros...)
void TLDSdecoder::faInit(const Eigen::MatrixXf & data)
{
    Eigen::MatrixXf XX(seq_l, units);
    Eigen::MatrixXf cX(units, units);
    Eigen::VectorXf L(units);
    Eigen::MatrixXf epsi(seq_l, units);

    epsi = (Eigen::MatrixXf::Random(seq_l, units)*0.1).array().abs();

    if (tldsType == LOGTLDS)
    {
        // +0.1 to avoid the all-zero data case
        //tSeq = ((data + epsi).array()+1).log();
        tSeq = (data.array() + 1).log();
    }
    else
    {
        // +0.1 to avoid the all-zero data case
        //tSeq = (data + epsi).cwiseSqrt();
        tSeq = data.cwiseSqrt();
    }


    auto tmp = tSeq;

    d = tSeq.colwise().mean();
    tSeq = tSeq.rowwise() - d.transpose();
    XX = tSeq.transpose()*tSeq;
    cX = XX.array() / (seq_l - 1);
    XX = XX.array() / seq_l;

    auto tt  = cX.determinant();

    float scale = pow(cX.determinant(), 1.0 / units);
    //L = Eigen::VectorXf::Random(units)*sqrt(std::fmax(scale,1));
    L = Eigen::VectorXf::Random(units)*sqrt(scale);
    /*
        if (units == 6) L << -0.2290, 1.2478, 0.1201, -1.2045, -1.5084, 0.8386;
        if (units == 7) L << -1.6999, -1.6525, -0.4268, -2.0909, -0.7219, 0.5004, 2.2210;
        if (units == 8) L << 0.3189, 0.4629, -1.6422, -0.3616, 0.8706, -0.5206, 0.0686, 0.5539;
        if (units == 10) L << 0.2216, -0.1376, 0.1761, -0.3911, -0.4380, -2.0001, -0.3125, 0.3393, 0.3206, 0.7681;
        if (units == 12) L << -1.1817, 0.4817, 0.1661, 1.3291, 1.4354, -0.7408, 1.5254, -0.8764, 1.1599, -1.2547, -1.8879, -0.5333;
        if (units == 14) L << 0.3675, -0.0104, -0.5901, 1.2398, -0.1081, -1.4640, -0.0675, 1.3366, 0.2676, -0.2184, -0.9733, -0.8022, 0.2143, 1.7681;
        L = L*sqrt(scale);
        */

    Eigen::VectorXf Ph(units);
    Eigen::VectorXf LP(units);
    Eigen::VectorXf tmpL(units);
    Eigen::MatrixXf MM(units, units);
    Eigen::VectorXf beta(units);
    Eigen::MatrixXf XM(seq_l, units);
    Eigen::VectorXf EZ(seq_l);
    Eigen::VectorXf H(seq_l);

    Ph = cX.diagonal();
    float con = -units / 2.0 * log(2 * EIGEN_PI);
    float dM, EZZ;
    float lik = 0;
    float oldlik = 0;
    float likbase;
    for (int i = 0; i < faMaxIter; i++)
    {
        //E step
        LP = L.cwiseQuotient(Ph);
        MM = Ph.cwiseInverse().asDiagonal();
        MM = MM - LP*(1.0 / (L.dot(LP) + 1))*LP.transpose();
        dM = sqrt(MM.determinant());
        beta = L.transpose()*MM;
        XM = tSeq*MM;
        EZ = XM*L;
        EZZ = 1 - beta.dot(L) + (beta.transpose()*XX*beta);

        //compute log likelihood
        oldlik = lik;
        H = -0.5*(XM.cwiseProduct(tSeq)).rowwise().sum();
        H = H.array() + con + log(dM);
        lik = H.sum();

        //M step
        L = tSeq.transpose()*EZ*(1.0 / (EZZ*seq_l));
        Ph = (XX - L*EZ.transpose()*tSeq*(1.0 / seq_l)).diagonal();

        if (i <= 2)
        {
            likbase = lik;
        }
        else if (lik < oldlik)
        {
            std::cout << "init violation" << std::endl;
        }
        else if (lik - likbase < (1 + faTol)*(oldlik - likbase) || isinf<float>(lik))
        {
            break;
        }
        std::cout << "TLDS fa iter="<<i<<std::endl;
    }


    Eigen::MatrixXf Phi(units, units);
    Eigen::VectorXf temp1(units);
    Eigen::MatrixXf temp2(units, units);
    Eigen::VectorXf t1(seq_l - 1);
    Eigen::VectorXf t2(seq_l - 1);
    c = L;
    r = Ph;
    Phi = r.cwiseInverse().asDiagonal();
    temp1 = Phi*L;
    temp2 = Phi - temp1*(1.0 / (1 + L.dot(temp1)))*temp1.transpose();
    temp1 = tSeq*temp2*L;
    x0 = temp1.mean();

    t1 = temp1.head(seq_l - 1);
    t2 = temp1.tail(seq_l - 1);
    temp1 = temp1.array() - x0;
    q = temp1.dot(temp1)*(1.0 / (seq_l - 1));
    q0 = q;
    a = t1.dot(t2) / (t1.dot(t1) + q);
}

/*
 * train TLDS model, using Laplacian EM.
 * input: data - training data spikes, cols-units/rows-bins
 * ouput: void
 */
void TLDSdecoder::train(const Eigen::MatrixXf& data)
{
    units = data.cols();
    seq_l = data.rows();
    std::cout <<"units"<< units << std::endl;
    std::cout <<"seq_l"<< seq_l << std::endl;

    faInit(data);

    float con = pow(2 * EIGEN_PI, -units / 2.0);
    int tailIdx = seq_l - 1;

    Eigen::VectorXf temp1(units);
    Eigen::VectorXf temp2(units);
    float temp3 = 0;
    Eigen::VectorXf temp4(units);
    Eigen::MatrixXf invP(units, units);
    Eigen::VectorXf CP(units);
    Eigen::VectorXf Kcur(units);
    float KC(units);
    Eigen::VectorXf yDiff(units);
    Eigen::VectorXf xCur(seq_l);
    Eigen::VectorXf pCur(seq_l);

    Eigen::VectorXf pPre(seq_l);
    float detiP;
    float xPre = x0;

    float A1 = 0;
    float A2 = 0;
    float A3 = 0;
    float Ptsum = 0;
    float pt = 0;
    Eigen::VectorXf YX = Eigen::VectorXf::Zero(units);
    Eigen::VectorXf Xfin(seq_l);
    Eigen::VectorXf Pfin(seq_l);
    Eigen::VectorXf J = Eigen::VectorXf::Zero(seq_l);
    float Pcov;
    float T1;
    Eigen::VectorXf YY = tSeq.cwiseProduct(tSeq).colwise().sum() / seq_l;

    float lik = 0;
    float oldlik;
    float likbase;
    for (int i = 0; i < emMaxIter; i++)
    {
        //-----E step-----
        oldlik = lik;
        //---kalmansmooth
        lik = 0;
        pPre(0) = q0;
        xPre = x0;
        //forward pass
        for (int t = 0; t < seq_l; t++)
        {
            temp1 = c.cwiseQuotient(r);
            temp2 = temp1*pPre(t);
            temp3 = c.dot(temp2);
            temp4 = temp1*(1.0 / (temp3 + 1));
            invP = r.cwiseInverse().asDiagonal();
            invP = invP - temp2*temp4.transpose();
            CP = temp1 - temp3*temp4;
            Kcur = pPre(t)*CP;
            KC = Kcur.transpose()*c;
            yDiff = tSeq.row(t) - xPre*c.transpose();
            xCur(t) = xPre + yDiff.transpose()*Kcur;
            pCur(t) = pPre(t) - KC*pPre(t);
            if (t < seq_l - 1)
            {
                xPre = xCur(t)*a;
                pPre(t + 1) = a*pCur(t)*a + q;
            }

            //calculate likelihood
            detiP = sqrt(invP.determinant());

            lik = lik + log(detiP) - 0.5*(yDiff.transpose().cwiseProduct(yDiff.transpose()*invP)).sum();

            /* matlab exception handle code
                if (isreal(detiP) & detiP>0)
                lik=lik+N*log(detiP)-0.5*sum(sum(Ydiff.*(Ydiff*invP)));
                else
                problem=1;
                end;
                */
        }
        lik = lik + seq_l*log(con);

        //backward pass
        A1 = 0;
        A2 = 0;
        A3 = 0;
        Ptsum = 0;
        YX.setZero();
        Xfin(tailIdx) = xCur(tailIdx);
        Pfin(tailIdx) = pCur(tailIdx);
        pt = Pfin(tailIdx) + Xfin(tailIdx)*Xfin(tailIdx);
        A2 = -pt;
        Ptsum = pt;
        YX = tSeq.row(tailIdx)*Xfin(tailIdx);
        for (int t = tailIdx - 1; t >= 0; t--)
        {
            J(t) = pCur(t)*a / pPre(t + 1);
            Xfin(t) = xCur(t) + (Xfin(t + 1) - xCur(t)*a)*J(t);
            Pfin(t) = pCur(t) + J(t)*(Pfin(t + 1) - pPre(t + 1))*J(t);
            pt = Pfin(t) + Xfin(t)*Xfin(t);
            Ptsum = Ptsum + pt;
            YX = YX + tSeq.row(t).transpose()*Xfin(t);
        }
        A3 = Ptsum - pt;
        A2 = Ptsum + A2;
        Pcov = (1 - KC)*a*pCur(tailIdx - 1);
        A1 = A1 + Pcov + Xfin(tailIdx)*Xfin(tailIdx - 1);
        for (int t = tailIdx - 1; t > 0; t--)
        {
            Pcov = (pCur(t) + J(t)*(Pcov - a*pCur(t)))*J(t - 1);
            A1 = A1 + Pcov + Xfin(t)*Xfin(t - 1);
        }
        if (i <= 2)
        {
            likbase = lik;
        }
        else if (lik < oldlik)
        {
            std::cout << "violation" << " lik=" << lik << " oldlik="<<oldlik << std::endl;
        }
        else if (lik - likbase < (1 + emTol)*(oldlik - likbase) || isinf<float>(lik))
        {
            break;
        }

        //----M step-----
        //Re-estimate A,C,Q,R,x0,P0;
        //x0 = Xfin(0);
        x0 = Xfin(seq_l-1);
        //T1 = Xfin(0) - x0;
        //q0 = Pfin(0);
        q0 = Pfin(seq_l-1);
        c = YX / Ptsum;
        r = YY - (c*YX.transpose()).diagonal() / seq_l;
        a = A1 / A2;
        q = (1.0 / (seq_l - 1))*(A3 - a*A1);
        /*
            if (det(Q)<0)
            fprintf('Q problem\n');
            end;
            */
        xsm = Xfin;
        Vsm = Pfin;
        std::cout << "TLDS EM iter="<<i<<std::endl;
    }
    init =true;
}
#ifdef debug
float TLDSdecoder::decode(const Eigen::VectorXf& data)
{
    units = 14;

    int v_units = data.rows();
    if (v_units != units)
    {
        std::cout << "TLDSdecoder:vector length different from model"
                  << "v_units=" << v_units << "model units=" << units << std::endl;
        return 0;
    }

    //units = 14;

    float x_pred, q_pred, q_filt, x_filt;
    Eigen::VectorXf K(units);
    Eigen::VectorXf y_pred(units);

    d = Eigen::VectorXf(14);
    c = Eigen::VectorXf(14);
    r = Eigen::VectorXf(14);
    x0 = -0.352905050834708;
    q0 = 0.00355884685509721;
    a = 0.828427968083645;
    q = 0.274737662858320;
    d << 0.196389335969059,	0.0717785569203146,	0.207471472707350,	0.196389335969059,	0.228765001528662,	0.196389335969059,	0.260481227478969,	0.489253727435403,	0.235887858880215,	0.0230281455335530,	0.104409266433589,	0.196389335969059,	0.317487142685624,	0.0253309600869083;
    c << 4.67060539750119e-05,	0.0338760294252767,	0.0635529786474569,	4.67060539750119e-05, - 0.0388455698912908,	4.67060539750119e-05,	0.00467060539750119, - 0.315638018512723,	0.0183323795862765,	0.0228986067037092,	0.0475170990293294,	4.67060539750119e-05,	0.0382236379419094, - 0.0174687062590038;
    r << 0.0986945803148053,	0.0465606463505761,	0.107603266511482,	0.0986945803148053,	0.130769874855311,	0.0986945803148053,	0.150377524284418,	0.154649116999482,	0.130232663784873,	0.0149746447536490,	0.0749987392028757,	0.0986945803148053,	0.160128835542604,	0.0166504908627824;
    //prediction
    x_pred = a*x0;
    q_pred = a*q0*a + q;
    y_pred = c*x_pred + d;
    std::cout << "y_pred=" << y_pred << std::endl;

    //std::cout << "c=" << c << std::endl;
    //std::cout << "c*q_pred*c.transpose()=" << c*q_pred*c.transpose() << std::endl;
    //std::cout << "(c*q_pred*c.transpose()).colwise() + r=" << (c*q_pred*c.transpose()).colwise() + r << std::endl;
    //std::cout << "((c*q_pred*c.transpose()).colwise() + r).inverse()=" << ((c*q_pred*c.transpose()).colwise() + r).inverse() << std::endl;
    Eigen::MatrixXf rr = r.asDiagonal();
    //std::cout << " r.diagonal() = " << rr << std::endl;
    //std::cout << " (c*q_pred*c.transpose()) + rr = " << (c*q_pred*c.transpose()) + rr << std::endl;

    K = q_pred*c.transpose()*((c*q_pred*c.transpose()) + rr).inverse();
    std::cout << " K = " << K << std::endl;

    //filtering
    //std::cout << "K=" << K << std::endl;
    //std::cout << "(data - y_pred)=" << (data - y_pred) << std::endl;
    //std::cout << "K.dot(data - y_pred)=" << K.dot(data - y_pred) << std::endl;
    x_filt = (K.dot(data - y_pred) + x_pred);
    q_filt = (1 - K.dot(c))*q_pred;
    x0 = x_filt;
    q0 = q_filt;

    return x_filt;

    return 0;
}
#else

/*
 * decode with TLDS model.
 * input: data - data spikes to be decoded, rows-units
 * ouput: decoded latent variable
 * +++++++++++++++++++++++++++++++++++++++++++++++
 * NOTE: x0 and q0 will be updated after each call
 * +++++++++++++++++++++++++++++++++++++++++++++++
 */
float TLDSdecoder::decode(const Eigen::VectorXf& data)
{
    if (!init) return 0;
    int v_units = data.rows();
    if (v_units != units)
    {
        std::cout << "TLDSdecoder:vector length different from model"
                  << "v_units=" << v_units << "model units=" << units << std::endl;
        return 0;
    }

    //units = 14;

    float x_pred, q_pred, q_filt, x_filt;
    Eigen::VectorXf K(units);
    Eigen::VectorXf y_pred(units);
    //prediction
    x_pred = a*x0;
    q_pred = a*q0*a + q;
    y_pred = c*x_pred + d;
    Eigen::MatrixXf rr = r.asDiagonal();

    K = q_pred*c.transpose()*((c*q_pred*c.transpose()) + rr).inverse();

    //filtering
    x_filt = (K.dot(data - y_pred) + x_pred);
    q_filt = (1 - K.dot(c))*q_pred;
    x0 = x_filt;
    q0 = q_filt;

    return x_filt;
}
#endif
/************************************************************************
    *               Model free decoder: CUSUM                               *
    *************************************************************************/
#ifdef debug
float ModelFreedecoder::decode(const Eigen::VectorXf& data)
{

    units = 14;

    int v_units = data.rows();
    if (v_units != units)
    {
        std::cout << "TLDSdecoder:vector length different from model"
                  << "v_units=" << v_units << "model units=" << units << std::endl;
        return 0;
    }

    lam0 = Eigen::VectorXf(14);
    lam1 = Eigen::VectorXf(14);
    temp = Eigen::VectorXf(14);
    temp1 = Eigen::VectorXf(14);
    lastCusum = Eigen::VectorXf::Zero(14);
    lam0 << 0.100000000000000,	0.263934426229508,	0.460655737704918,	0.100000000000000,	0.411475409836066,	0.116393442622951,	0.460655737704918,	0.788524590163934,	0.509836065573771,	0.100000000000000,	0.165573770491803,	0.149180327868852,	0.559016393442623,	0.116393442622951;
    lam1 << 0.732455532033676,	1.29142540181909,	1.81808822251574,	0.732455532033676,	1.69440239113479,	0.798723106709017,	1.81808822251574,	2.56450273210593,	1.93789217872027,	0.732455532033676,	0.979388910414386,	0.921657713606073,	2.05436437092830, 0.798723106709017;
    temp1 << 1.99123244593912,	1.58780116318503,	1.37288980964787,	1.99123244593912,	1.41533612509811,	1.92603813704694,	1.37288980964787	,1.17935628026440	,1.33526692157374	,1.99123244593912,	1.77751197771204,	1.82101808291972,	1.30154297143557,	1.92603813704694;

    temp = data.cwiseProduct(temp1);
    temp = temp - (lam1 - lam0);
    lastCusum = (lastCusum + temp).cwiseMax(0);
    return lastCusum.maxCoeff();
}
#else

/*
 * modelfree decode.
 * input: data - data spikes to be decoded, rows-units
 * ouput: decoded latent variable
 * +++++++++++++++++++++++++++++++++++++++++++++++
 * NOTE: lastCusum will be updated after each call
 * +++++++++++++++++++++++++++++++++++++++++++++++
 */
float ModelFreedecoder::decode(const Eigen::VectorXf& data)
{
    if (!init) return 0;

    int v_units = data.rows();
    if (v_units != units)
    {
        std::cout << "ModelFreedecoder:vector length different from baseline"
                  << "v_units=" << v_units << "baseline units=" << units << std::endl;
        return 0;
    }

    temp = data.cwiseProduct(temp1);
    temp = temp - (lam1 - lam0);
    lastCusum = (lastCusum + temp).cwiseMax(0);
    return lastCusum.maxCoeff();
}
#endif

/*
 * update baseline params of model-free decoders.
 * input: data - baseline spikes, cols-units/rows-bins
 * ouput: void
 */
void ModelFreedecoder::updateBaseline(const Eigen::MatrixXf& data)
{
    init = true;
    units = data.rows();
    lastCusum = Eigen::VectorXf::Zero(units);
    lam0 = data.rowwise().mean();
    lam0 = lam0.array() + 0.1;
    lam1 = lam0 + 2 * lam0.cwiseSqrt();
    temp1 = (lam1.cwiseQuotient(lam0)).array().log();
    //std::cout << "lam0="<<lam0<<" lam1="<<lam1<<" temp1="<<temp1<<std::endl;
    //std::cout << "lastCusum="<<lastCusum<<std::endl;
}


// ++++++++++++++ detector related classes +++++++++++++++++++++++++++++++++++++++++++++++++
/************************************************************************
*               Model detector: PLDS/TLDS                               *
*************************************************************************/

/*
 * detect with model-based decoders.
 * call the detect(const Eigen::VectorXf&, const float, const bool)
 * reloaded function with default threshold and self mode
 *
 * input: data - data spikes to be decode/detect, rows-units
 * ouput: detect result, true-detected/false-not detected
 */
bool ModelDetector::detect(const Eigen::VectorXf& data)
{
    return detect(data,thresh);
}

/*
 * detect with model-based decoders.
 * input: data - data spikes to be decode/detect, rows-units
 *        thresh_ - detecting threshold
 *        self_ - flag indicates called
 *                     by current detecor(self_==true, call decoder) or
 *                     by another detector(self_==false, not call decoder,
 *                                         just read last decoding result)
 * ouput: detect result, true-detected/false-not detected
 */
bool ModelDetector::detect(const Eigen::VectorXf& data, const float thresh_, const bool self_)
{
    if (last_detection_cnt<10000 && self_)
        last_detection_cnt++;
    //std::cout << "item="<<z_buf.items()<<" self="<<self_<<std::endl;
    if (!p_decoder->isInit()) return false;
    float z, zscore, conf;
    if (!cur_bin_done)
    {
        z = p_decoder->decode(data);
        cur_bin_done = true;
    }
    if (self_)
        z_buf.put(z);
    if (self_)
        q_buf.put(p_decoder->getQ0());

    zscore = p_decoder->getZscore();
    conf = p_decoder->getConf(nStd);

//    std::cout<<region<<" zs="<<zscore<<" cf="<<conf
//            <<" sf="<<self_<<" th="<<thresh_
//           <<" cnt="<<last_detection_cnt<<std::endl;

    if (zscore - conf > thresh_)
    {
        //std::cout << "higher! last_detection_cnt="<<last_detection_cnt<<std::endl;
    }
    if (zscore + conf < -thresh_)
    {
        //std::cout << "lower! last_detection_cnt="<<last_detection_cnt<<std::endl;
    }

    bool detected = (zscore - conf > thresh_ || zscore + conf < -thresh_);
    if (detected)
    {

        //std::cout <<region<< " model, self="<<self_<<" last_detection_cnt="<<last_detection_cnt<<std::endl;
        if (last_detection_cnt<min_interval && self_)
        {
            detected = false;
        }
        if (self_)
            last_detection_cnt = 0;
    }

    // higher bound lower || lower bound higher the -/+ thresh
    return detected;
}

/*
 * update baseline of the related decoder.
 * input: none
 * ouput: void
 */
void ModelDetector::update()
{
    uint32_t shift = round(interval / binSize);
    std::cout << "in update"<<std::endl;

    if (z_buf.items()>=shift + l_base)
    {
        std::cout << "in if"<<std::endl;
        Eigen::VectorXf base_data = z_buf.read(shift + l_base, shift);
        std::cout << "z_buf"<<std::endl;
        Eigen::VectorXf base_cov = q_buf.read(shift + l_base, shift);
        std::cout << "q_buf"<<std::endl;
        p_decoder->updateBaseline(base_data);
        std::cout << "updateBaseline"<<std::endl;
        // add z-score & conf of baseline
        Eigen::VectorXf tmp1(base_data.rows());
        Eigen::VectorXf tmp2(base_data.rows());
        for (int i=0;i<base_data.rows();i++)
        {
           tmp1[i] = p_decoder->getZscore(base_data[i]);
           //std::cout << p_decoder->getZscore(base_data[i])<<",";
        }
        std::cout <<std::endl;
        for (int i=0;i<base_cov.rows();i++)
        {
            tmp2[i] = p_decoder->getConf(base_cov[i],nStd);
            //std::cout << p_decoder->getConf(base_cov[i],nStd)<<",";
        }
        std::cout <<std::endl;
        zscore_base = tmp1;
        conf_base = tmp2;
        //std::cout << "zscore_base"<<std::endl;
        //std::cout << zscore_base << std::endl;
        //std::cout << "conf_base" << std::endl;
        //std::cout << getConf() << std::endl;
        //std::cout << conf_base << std::endl;
    }
    else
    {
        std::cout << "not enough data in buffer! required="<<shift + l_base
                  << "current="<<z_buf.items()<<std::endl;
    }
}

/*
 * train related decoder, and do catching up decoding/updating baseline
 * after training
 * input: data - training data spikes, cols-units/rows-bins
 * ouput: void
 */
void ModelDetector::train(const Eigen::MatrixXf& data)
{
    train_bin_cnt = 0;
    status = model_training;
    p_decoder_tmp->train(data);
    std::cout <<"A:"<<std::endl;
    std::cout<<p_decoder_tmp->getA()<<std::endl;
    std::cout <<"C:"<<std::endl;
    std::cout<<p_decoder_tmp->getC()<<std::endl;
    std::cout <<"d:"<<std::endl;
    std::cout<<p_decoder_tmp->getD()<<std::endl;
    std::cout << "train bin cnt="<<train_bin_cnt<<std::endl;
    // catching up decoding
    int cnt = 0;
    while(catching_up_buf!=nullptr && !catching_up_buf->empty())
    {
        z_buf_tmp.put(p_decoder_tmp->decode(catching_up_buf->get()));
    }
    // update baseline
    // use the 60% in the middle of the first half of training seq
    //std::cout << "Debug :  Row: " << data.rows() << " Col: "<< data.cols() << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    int start = data.rows()*0.1;
    int len = data.rows()*0.3;
    p_decoder_tmp->updateBaseline(p_decoder_tmp->getXsm().segment(start,len));
    status = update_required;
}

/************************************************************************
*               Model Free detector: PLDS/TLDS                          *
*************************************************************************/
/*
 * detect with modelfree decoders.
 * call the detect(const Eigen::VectorXf&, const float, const bool)
 * reloaded function with default threshold and self mode
 *
 * input: data - data spikes to be decode/detect, rows-units
 * ouput: detect result, true-detected/false-not detected
 */
bool ModelFreeDetector::detect(const Eigen::VectorXf& data)
{
    return detect(data,thresh);
}

/*
 * detect with model-free decoders.
 * input: data - data spikes to be decode/detect, rows-units
 *        thresh_ - detecting threshold
 *        self_ - flag indicates called
 *                     by current detecor(self_==true, call decoder) or
 *                     by another detector(self_==false, not call decoder,
 *                                         just read last decoding result)
 * ouput: detect result, true-detected/false-not detected
 */
bool ModelFreeDetector::detect(const Eigen::VectorXf& data, const float thresh_, const bool self_)
{
    if (last_detection_cnt<10000 && self_)
        last_detection_cnt++;

    if(0==nSpikes && data.rows()>0)
    {
        nSpikes = data.rows();
        p_spk_buf = new circular_buffer2d<float>(10000,nSpikes);
    }
    if (self_)
        p_spk_buf->putEigenVec(data);
    float x;
    if (!cur_bin_done)
    {
        x = p_decoder->decode(data);
        cur_bin_done = true;
    }
    else
    {
        x = x_buf.read();
    }
    if (self_)
        x_buf.put(x);

    //std::cout << "self="<<self_<<" item="<<x_buf.items();

    if (!x_buf.full() && x_buf.items()<1) return false;

    // at least two consecutive points
    bool detected = (x > thresh_ && x_buf.read(1, 1)(0)>thresh_);
    if (detected)
    {
        if (last_detection_cnt<min_interval && self_)
        {
            detected = false;
            //last_detection_cnt = 0;
        }
        if (self_)
            last_detection_cnt = 0;
    }

    return detected;
}

/*
 * update baseline of the related decoder.
 * input: none
 * ouput: void
 */
void ModelFreeDetector::update()
{
    if(0==nSpikes) return;

    uint32_t shift = round(interval / binSize);

    if (p_spk_buf->items()>=shift + l_base)
    {
        p_decoder->updateBaseline(p_spk_buf->read(shift + l_base, shift));
    }
    else
    {
        std::cout << "not enough data in buffer! required="<<shift + l_base
                  << "current="<<p_spk_buf->items()<<std::endl;
    }
}

/************************************************************************
*               Greedy detector: PLDS/TLDS                          *
*************************************************************************/
/*
 * RULE: ALL of the listed detectors detected with a lower threshold,
 *       or ONE of them detected with a higher threshold
 */

/*
 * detect with modelfree decoders.
 * call the detect(const Eigen::VectorXf&, const float, const bool)
 * reloaded function with default threshold and self mode
 *
 * input: data - data spikes to be decode/detect, rows-units
 * ouput: detect result, true-detected/false-not detected
 */
bool GreedyDetector::detect(const Eigen::VectorXf& data)
{
    return detect(data,0,true);
}

/*
 * detect with greedy detector.
 * input: data - data spikes to be decode/detect, rows-units
 *               NOTE: this is a vector, each element is the data from one region,
 *        region - vector of names of regions
 *        self_ - flag indicates called
 *                     by current detecor(self_==true, call decoder) or
 *                     by another detector(self_==false, not call decoder,
 *                                         just read last decoding result)
 * ouput: detect result, true-detected/false-not detected
 */
bool GreedyDetector::detect(const std::vector<Eigen::VectorXf>& data, const std::vector<std::string>& region, const bool self_)
{
    if (self_)
        last_detection_cnt++;
    //std::cout <<"inside GreedyDetector"<<std::endl;
    int cnt = 0;
    bool detected = false;
    for (int i = 0; i < detector_list.size() && !detected; i++)
    {
        // as a nested detector, the call of sub detectors must pass flag=false,
        // to avoid redundent result/data buffering in the related single detector
        for (int j=0;j<region.size() && !detected;j++)
        {
            if (region[j].compare(detector_list[i]->get_region())==0)
            {
                //std::cout <<"inside GreedyDetector region="<<region[j]<<std::endl;
                if (detector_list[i]->detect(data[j], higher_thresh_list[i],false))
                {
                    detected = true;
                    continue;
                }

                if (detector_list[i]->detect(data[j], thresh_list[i],false))
                {
                    cnt++;
                }
            }
        }

        // for greedy
        if (detector_list[i]->get_region().compare("noregion")==0 && detector_list[i]->get_type().compare("Greedy")==0)
        {
            if (static_cast<GreedyDetector*>(detector_list[i])->detect(data,region,false))
            {
                detected = true;
                continue;
            }

            if (static_cast<GreedyDetector*>(detector_list[i])->detect(data,region,false))
            {
                cnt++;
            }
        }
    }

    if (cnt == detector_list.size())
    {
        detected = true;
    }

    if (detected)
    {
         //std::cout << "greedy last_detection_cnt="<<last_detection_cnt<<std::endl;
        if (last_detection_cnt<min_interval && self_)
        {
            //if (detected)
            //    std::cout << "greedy last_detection_cnt="<<last_detection_cnt<<std::endl;
            detected = false;
        }
        last_detection_cnt = 0;
    }

    if (detected)
    {
        //std::cout<<"greedy cnt="<<cnt<<std::endl;
        //std::cout<<"thresh1="<<thresh_list[0]<<"det="<<static_cast<GreedyDetector*>(detector_list[0])->get_name()<<std::endl;
        //std::cout<<"thresh2="<<thresh_list[1]<<"det="<<static_cast<GreedyDetector*>(detector_list[1])->get_name()<<std::endl;
    }

    return detected;
}

/*
 * detect with related decoders.
 * input: data - data spikes to be decode/detect, rows-units
 *        thresh_ - detecting threshold
 *        self_ - flag indicates called
 *                     by current detecor(self_==true, call decoder) or
 *                     by another detector(self_==false, not call decoder,
 *                                         just read last decoding result)
 * ouput: detect result, true-detected/false-not detected
 */
bool GreedyDetector::detect(const Eigen::VectorXf& data, const float thresh, const bool self_)
{
    if (self_)
        last_detection_cnt++;
    int cnt = 0;
    bool detected=false;
    for (int i = 0; i < detector_list.size() && !detected; i++)
    {
        // as a nested detector, the call of sub detectors must pass flag=false,
        // to avoid redundent result/data buffering in the related single detector
        if (detector_list[i]->detect(data, higher_thresh_list[i]),false)
        {
            detected=true;
            continue;
        }

        if (detector_list[i]->detect(data, thresh_list[i]),false)
        {
            cnt++;
        }
    }

    if (cnt == detector_list.size())
    {
        detected = true;
    }

    if (detected)
    {
        if (last_detection_cnt<min_interval && self_)
        {
            //if (detected)
              //  std::cout << "last_detection_cnt="<<last_detection_cnt<<std::endl;
            detected = false;
        }
        last_detection_cnt = 0;
    }
    return detected;
}

bool CCFDetector::detect(const Eigen::VectorXf& data)
{
    last_detection_cnt++;
    if (detector_list.size() != 2)
    {
        std::cout << "Two dectectors are requried !! " << std::endl;
        return false;
    }
    compute_ccf_base();
    std::vector <float> CCF_tmp;
    float up_bound, low_bound;
    float eps = 1e-6;
    if (fabs(std_base_ccf)<eps)
    {
        //std::cout << "invalid baseline" <<std::endl;
        return false;
    }

    for (int i = 0; i < 2; i++)
    {
        up_bound = detector_list[i]->getZscore() + detector_list[i]->getConf();
        low_bound = detector_list[i]->getZscore() - detector_list[i]->getConf();
        if (abs(up_bound) <= abs(low_bound))
            //CCF_tmp.push_back(abs(up_bound));
            CCF_tmp.push_back(up_bound);
        else
            //CCF_tmp.push_back(abs(low_bound));
            CCF_tmp.push_back(low_bound);
    }
    int sign0 = (CCF_tmp[0]>0)?1:-1;
    int sign1 = (CCF_tmp[1]>0)?1:-1;
    CCF_Value = (1 - ffactor) * CCF_Value + ffactor * sign0*std::pow(abs(CCF_tmp[0]), alpha) * sign1*std::pow(abs(CCF_tmp[1]), beta);

    //std::cout << "ccf_value="<<CCF_Value<<" ff="<<ffactor<<" ccf_tmp="<<CCF_tmp[0]<<","<<CCF_tmp[1]<<std::endl;
    //std::cout << "std::pow(CCF_tmp[0], alpha)="<<std::pow(CCF_tmp[0], alpha)<<std::endl;
    //std::cout << "std::pow(CCF_tmp[1], beta)="<<std::pow(CCF_tmp[1], beta)<<std::endl;
    // make sure the thresholds are up-to-date
    update_ccf_threshold();
    //std::cout << "CCF_Thresh="<<CCF_Thresh[0]<<","<<CCF_Thresh[1]<<std::endl;

    if (CCF_Value < CCF_Thresh[0] || CCF_Value > CCF_Thresh[1])
        CCF_Area += std::max((CCF_Thresh[0] - CCF_Value), (CCF_Value - CCF_Thresh[1]));
    else
        CCF_Area = 0;

    bool detected;
    if (CCF_Area >= CCF_Area_Thresh)
        detected= true;
    else
        detected= false;

    if (detected)
    {
        //std::cout <<"ccf last_detection_cnt="<<last_detection_cnt<<" min_interval="<<min_interval<<std::endl;
        if (last_detection_cnt<min_interval)
        {
            detected = false;
        }
        last_detection_cnt = 0;
    }
    return detected;
}

bool CCFDetector::detect(const Eigen::VectorXf& data, const float thresh ,const bool self)
{
    return detect(data);
}

void CCFDetector::compute_ccf_base()
{
    if (detector_list.size() != 2)
    {
        std::cout << "Two dectectors are requried !! " << std::endl;
        return;
    }
    float alpha = 0.5;
    float beta = 0.5;
    float ffactor = 0.6;  // forgeting factor
    Eigen::VectorXf zscore_base1, zscore_base2;
    Eigen::VectorXf conf_base1, conf_base2;
    float up_b[2], low_b[2], ccf_tmp[2];

    zscore_base1 = detector_list[0]->getZscore_b();
    conf_base1 = detector_list[0]->getConf_b();
    zscore_base2 = detector_list[1]->getZscore_b();
    conf_base2 = detector_list[1]->getConf_b();
    if (zscore_base1.rows()<1 || zscore_base2.rows()<1)
    {
        mean_base_ccf = 0;
        std_base_ccf = 0;
        return;
    }
    //std::cout << "got model" <<std::endl;

    Eigen::VectorXf CCF_V(zscore_base1.size());

    for (int i = 0; i < zscore_base1.size(); i++)
    {
        up_b[0] = zscore_base1(i) + conf_base1(i);
        low_b[0] = zscore_base1(i) - conf_base1(i);
        up_b[1] = zscore_base2(i) + conf_base2(i);
        low_b[1] = zscore_base2(i) - conf_base2(i);
        for (int j = 0; j < 2; j++)
        {
            if (abs(up_b[j]) <= abs(low_b[j]))
                //ccf_tmp[j] = abs(up_b[j]);
                ccf_tmp[j] = up_b[j];
            else
                //ccf_tmp[j] = abs(low_b[j]);
                ccf_tmp[j] = low_b[j];
        }

        int sign0 = (ccf_tmp[0]>0)?1:-1;
        int sign1 = (ccf_tmp[1]>0)?1:-1;
        if (i != 0)
            CCF_V(i) = (1 - ffactor) * CCF_V(i - 1) + ffactor * sign0*std::pow(abs(ccf_tmp[0]), alpha) * sign1*std::pow(abs(ccf_tmp[1]), beta);
        else
            CCF_V(i) = sign0*std::pow(abs(ccf_tmp[0]), alpha) * sign1*std::pow(abs(ccf_tmp[1]), beta);
    }
    mean_base_ccf = CCF_V.mean();
    std_base_ccf = sqrt(CCF_V.array().pow(2).mean() - mean_base_ccf*mean_base_ccf);
    //update_ccf_threshold();
}

bool EMGDetector::detect(const Eigen::VectorXf& data, const float thresh ,const bool self)
{
    return detect(data);
}

bool EMGDetector::detect(const Eigen::VectorXf& data)
{
    return true;
}

}



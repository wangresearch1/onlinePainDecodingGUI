// Copyright (C) 2016 Yixuan Qiu <yixuan.qiu@cos.name>
// Under MIT license

/*
* modified by hsl to get the same result as matlab minFunc LBFGS does
* by Sile Hu
* date: 2017-3-24
*/

#ifndef LBFGS_H
#define LBFGS_H

#include <Eigen/Core>
#include "LBFGS/Param.h"
#include "LBFGS/LineSearch_hsl.h"
#include <numeric>
#include <cmath>

namespace LBFGSpp {


///
/// LBFGS solver for unconstrained numerical optimization
///
template <typename Scalar>
class LBFGSSolver
{
private:
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> Vector;
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> Matrix;
    typedef Eigen::Map<Vector> MapVec;

    const LBFGSParam<Scalar>& m_param;  // Parameters to control the LBFGS algorithm
    Matrix                    m_s;      // History of the s vectors
    Matrix                    m_y;      // History of the y vectors
    Vector                    m_ys;     // History of the s'y values
    Vector                    m_alpha;  // History of the step lengths
    Vector                    m_fx;     // History of the objective function values
    Vector                    m_xp;     // Old x
    Vector                    m_grad;   // New gradient
    Vector                    m_gradp;  // Old gradient
    Vector                    m_drt;    // Moving direction

    inline void reset(int n)
    {
        const int m = m_param.m;
        m_s.resize(n, m);
        m_y.resize(n, m);
        m_ys.resize(m);
        m_alpha.resize(m);
        m_xp.resize(n);
        m_grad.resize(n);
        m_gradp.resize(n);
        m_drt.resize(n);
        if(m_param.past > 0)
            m_fx.resize(m_param.past);
    }

public:
    ///
    /// Constructor for LBFGS solver.
    ///
    /// \param param An object of \ref LBFGSParam to store parameters for the
    ///        algorithm
    ///
    LBFGSSolver(const LBFGSParam<Scalar>& param) :
        m_param(param)
    {
        m_param.check_param();
    }

    ///
    /// Minimizing a multivariate function using LBFGS algorithm.
    /// Exceptions will be thrown if error occurs.
    ///
    /// \param f  A function object such that `f(x, grad)` returns the
    ///           objective function value at `x`, and overwrites `grad` with
    ///           the gradient.
    /// \param x  In: An initial guess of the optimal point. Out: The best point
    ///           found.
    /// \param fx Out: The objective function value at `x`.
    ///
    /// \return Number of iterations used.
    ///
    template <typename Foo>
    inline int minimize(Foo& f, Vector& x, Scalar& fx)
    {
        std::cout<< "inside minimize"<<std::endl;
        const int n = x.size();
        const int fpast = m_param.past;
        reset(n);

        // Evaluate function and compute gradient
        //std::cout << x << std::endl;
        fx = f(x, m_grad);
        //std::cout << m_grad << std::endl;
        //Scalar xnorm = x.norm();
        //Scalar gnorm = m_grad.norm();
        //if(fpast > 0)
        //    m_fx[0] = fx;

        //// Early exit if the initial x is already a minimizer
        //if(gnorm <= m_param.epsilon * std::max(xnorm, Scalar(1.0)))
        //{
        //    return 1;
        //}

        //Exit if initial point is optimal
        Scalar optCond = m_grad.array().abs().maxCoeff();
        if (optCond <= m_param.epsilon)
        {
            printf("Optimality Condition below optTol\n");
            return 0;
        }

        // Initial direction
        m_drt.noalias() = -m_grad;
        // Initial step
        // ----- change this step size as minFunc does ----
        //Scalar step = Scalar(1.0) / m_drt.norm();
        Scalar step = std::min(Scalar(1.0) / m_grad.array().abs().sum(), Scalar(1.0));
        Scalar fx_old;
        Vector m_grad_old;
        int k = 1;
        int end = 0;

        // ---- matlab minFunc lbfgs variables
        int lbfgs_start = 1;
        int lbfgs_end = 0;
        float Hdiag = 1;
        Matrix S(n,m_param.m);
        Matrix Y(n,m_param.m);
        Vector YS(m_param.m);

        for( ; ; )
        {
            // Save the curent x and gradient
            m_xp.noalias() = x;
            m_gradp.noalias() = m_grad;
            fx_old = fx;
            m_grad_old = m_grad;
            // Line search to update x, fx and gradient
            //std::cout << "k=" << k << ", fx=" << fx << ", step=" << step << std::endl;
            //std::cout << "x="<<x.transpose() << std::endl;
            //std::cout << std::endl;
            //std::cout << "m_grad=" << m_grad.transpose() << std::endl;
            //std::cout << std::endl;
            //std::cout << "m_drt=" << m_drt.transpose() << std::endl;
            //std::cout << std::endl;
            LineSearch<Scalar>::Backtracking(f, fx, x, m_grad, step, m_drt, m_xp, m_param);

            if (std::isinf(x.array().sum()) || std::isinf(m_grad.array().sum()) || std::isinf(fx))
            {
                std::cout << "LBFGS: infinite result, return" << std::endl;
                fx = std::numeric_limits<float>::infinity();
                return k;
            }

            optCond = m_grad.array().abs().maxCoeff();
            if (optCond <= m_param.epsilon)
            {
                printf("Optimality Condition below optTol\n");
                break;
            }

            if ((m_drt*step).array().abs().maxCoeff() <= 1.0e-9)
            {
                printf("Step Size below progTol\n");
                break;
            }

            if (abs(fx - fx_old) < 1.0e-9)
            {
                printf("Function Value changing by less than progTol\n");
                break;
            }

            // New x norm and gradient norm
            //xnorm = x.norm();
            //gnorm = m_grad.norm();

            // Convergence test -- gradient
            //if(gnorm <= m_param.epsilon * std::max(xnorm, Scalar(1.0)))
            //{
            //    return k;
            //}
            //// Convergence test -- objective function value
            //if(fpast > 0)
            //{
            //    if(k >= fpast && (m_fx[k % fpast] - fx) / fx < m_param.delta)
            //        return k;

            //    m_fx[k % fpast] = fx;
            //}
            // Maximum number of iterations
            if(m_param.max_iterations != 0 && k >= m_param.max_iterations)
            {
                return k;
            }

            //printf("k=%d g(0)=%f\n",k,m_grad[0]);

            // Update s and y
            // s_{k+1} = x_{k+1} - x_k
            // y_{k+1} = g_{k+1} - g_k
            //----- old lfbgls -----
            //MapVec svec(&m_s(0, end), n);
            //MapVec yvec(&m_y(0, end), n);
            //svec.noalias() = x - m_xp;
            //yvec.noalias() = m_grad - m_gradp;

            //// ys = y's = 1/rho
            //// yy = y'y
            //Scalar ys = yvec.dot(svec);
            //Scalar yy = yvec.squaredNorm();
            //m_ys[end] = ys;

            //// Recursive formula to compute d = -H * g
            //m_drt.noalias() = -m_grad;
            //int bound = std::min(m_param.m, k);
            //end = (end + 1) % m_param.m;
            //int j = end;
            //for(int i = 0; i < bound; i++)
            //{
            //    j = (j + m_param.m - 1) % m_param.m;
            //    MapVec sj(&m_s(0, j), n);
            //    MapVec yj(&m_y(0, j), n);
            //    m_alpha[j] = sj.dot(m_drt) / m_ys[j];
            //    m_drt.noalias() -= m_alpha[j] * yj;
            //}

            //m_drt *= (ys / yy);

            //for(int i = 0; i < bound; i++)
            //{
            //    MapVec sj(&m_s(0, j), n);
            //    MapVec yj(&m_y(0, j), n);
            //    Scalar beta = yj.dot(m_drt) / m_ys[j];
            //    m_drt.noalias() += (m_alpha[j] - beta) * sj;
            //    j = (j + 1) % m_param.m;
            //}

            // ---- matlab minFunc lbfgs
            //lbfgsAdd
            //g - g_old, t*d, S, Y, YS, lbfgs_start, lbfgs_end, Hdiag
            Vector y = m_grad - m_grad_old;
            Scalar ys = y.transpose()*step*m_drt;
            if (ys > 1e-10)
            {
                if (lbfgs_end < m_param.m)
                {
                    lbfgs_end += 1;
                    if (1 != lbfgs_start)
                    {
                        if (lbfgs_start == m_param.m)
                        {
                            lbfgs_start = 1;
                        }
                        else
                        {
                            lbfgs_start += 1;
                        }
                    }
                }
                else
                {
                    lbfgs_start = std::min(2, m_param.m);
                    lbfgs_end = 1;
                }

                S.col(lbfgs_end - 1) = step*m_drt;
                Y.col(lbfgs_end - 1) = y;
                //std::cout << S.col(lbfgs_end - 1) << std::endl;
                //std::cout << std::endl;
                //std::cout << Y.col(lbfgs_end - 1) << std::endl;
                YS(lbfgs_end-1) = ys;
                Hdiag = ys / (y.transpose()*y);
            }
            //lbfgsProd
            m_drt = -m_grad;
            int len = 0;
            Vector al(m_param.m);
            Vector be(m_param.m);
            if (1 == lbfgs_start)
            {
                len = lbfgs_end - lbfgs_start + 1;
            }
            else
            {
                len = m_param.m;
            }
            int idx = lbfgs_end - 1;
            for (int i = 0; i < len; i++)
            {
                //std::cout << "m_drt="<<m_drt.transpose() << std::endl;
                //std::cout << "S.col("<<idx<<")=" << S.col(idx).transpose() << std::endl;
                Scalar tmp = m_drt.transpose() * S.col(idx);
                al(idx) = tmp / YS(idx);
                //std::cout << "al(" << idx << ")=" << al(idx) << std::endl;
                m_drt = m_drt - al(idx)*Y.col(idx);
                //std::cout << Y.col(idx) << std::endl;
                //std::cout << "m_drt=" << m_drt.transpose() << std::endl;
                idx = (idx - 1) % m_param.m;
            }
            m_drt = m_drt*Hdiag;
            //std::cout << m_drt << std::endl;
            idx = lbfgs_start - 1;
            for (int i = 0; i < len; i++)
            {
                Scalar tmp = m_drt.transpose() * Y.col(idx);
                be(idx) = tmp / YS(idx);
                //std::cout << m_drt << std::endl;
                //std::cout << al(idx) - be(idx) << std::endl;
                //std::cout << S.col(idx) << std::endl;
                m_drt = m_drt + (al(idx)-be(idx))*S.col(idx);
                //std::cout << m_drt << std::endl;
                idx = (idx + 1) % m_param.m;
            }
            //std::cout << "m_drt=" << m_drt.transpose() << std::endl;
            // step = 1.0 as initial guess
            step = Scalar(1.0);
            k++;
        }

        return k;
    }
};


} // namespace LBFGSpp

#endif // LBFGS_H

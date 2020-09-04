// Copyright (C) 2016 Yixuan Qiu <yixuan.qiu@cos.name>
// Under MIT license

/*
    edit backtracking to get same result as matlab minFunc does
*/

#ifndef LINE_SEARCH_H
#define LINE_SEARCH_H

#include <Eigen/Core>
#include <stdexcept>  // std::runtime_error


namespace LBFGSpp {


///
/// Line search algorithms for LBFGS. Mainly for internal use.
///
template <typename Scalar>
class LineSearch
{
private:
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1> Vector;

public:
    ///
    /// Line search by backtracking.
    ///
    /// \param f      A function object such that `f(x, grad)` returns the
    ///               objective function value at `x`, and overwrites `grad` with
    ///               the gradient.
    /// \param fx     In: The objective function value at the current point.
    ///               Out: The function value at the new point.
    /// \param x      Out: The new point moved to.
    /// \param grad   In: The current gradient vector. Out: The gradient at the
    ///               new point.
    /// \param step   In: The initial step length. Out: The calculated step length.
    /// \param drt    The current moving direction.
    /// \param xp     The current point.
    /// \param param  Parameters for the LBFGS algorithm
    ///
    template <typename Foo>
    static void Backtracking(Foo& f, Scalar& fx, Vector& x, Vector& grad,
                             Scalar& step,
                             const Vector& drt, const Vector& xp,
                             const LBFGSParam<Scalar>& param)
    {
        // Decreasing and increasing factors
        const Scalar dec = 0.5;
        const Scalar inc = 2.1;

        // Check the value of step
        if(step <= Scalar(0))
            std::invalid_argument("'step' must be positive");

        // Save the function value at the current x
        Scalar fx_prev = fx;
		Scalar fx_init = fx;
        // Projection of gradient on the search direction
        Scalar dg_prev = grad.dot(drt);
        // Make sure d points to a descent direction
		if (dg_prev > 0)
            std::logic_error("the moving direction increases the objective function value");

		const Scalar dg_test = param.ftol * dg_prev;
        Scalar width;
		Scalar dg_new;
		Scalar dg;
		Vector grad_prev = grad;

		Scalar step_prev = 0;
		bool done = false;
        int iter;
		Scalar bracket[2];
		Scalar bracketFval[2];
		Scalar bracketGval[2];
        for(iter = 0; iter < param.max_linesearch; iter++)
        {
            // x_{k+1} = x_k + step * d_k
            x.noalias() = xp + step * drt;
			//if (0 == iter)
			//{
				//std::cout <<"xp="<< xp.transpose() << std::endl;
				//std::cout << std::endl;
				//std::cout <<"dtr="<< drt.transpose() << std::endl;
				//std::cout << std::endl;
				//std::cout <<"x="<< x.transpose() << std::endl;
				//std::cout << std::endl;
			//}
            // Evaluate this candidate
            fx = f(x, grad);
			dg_new = grad.dot(drt);
			//int a = 0;
			if (fx > fx_prev + step * dg_test || (iter>1 && fx>=fx_prev))
            {
				bracket[0] = step_prev;
				bracket[1] = step;
				bracketFval[0] = fx_prev;
				bracketFval[1] = fx;
				bracketGval[0] = grad_prev.dot(drt);
				bracketGval[1] = grad.dot(drt);
				break;
            } else{
                // Armijo condition is met
                if(param.linesearch == LBFGS_LINESEARCH_BACKTRACKING_ARMIJO)
                    break;

                dg = grad.dot(drt);
				if (abs(dg) <= -param.wolfe * dg_prev)
                {             
					done = true;
					break;
                } 
				else 
				{
					if (dg >= 0)
					{
						bracket[0] = step_prev;
						bracket[1] = step;
						bracketFval[0] = fx_prev;
						bracketFval[1] = fx;
						bracketGval[0] = grad_prev.dot(drt);
						bracketGval[1] = grad.dot(drt);
						break;
					}
                }
            }

			Scalar temp = step_prev;
			Scalar step_prev = step;
			Scalar minstep = step + 0.01*(step - temp);
			Scalar maxstep = step * 10;

			//polyinterp to get new step size
			if (step > temp)
			{
				Scalar d1 = dg_prev + dg - 3 * (fx - fx_prev) / (step - temp);
				Scalar d2_2 = d1*d1 - dg_prev*dg;
				if (d2_2 >= 0)
				{
					Scalar t = temp - (step - temp)*(dg + sqrt(d2_2) - d1) / (dg - dg_prev + 2 * sqrt(d2_2));
					step = std::min(std::max(t, minstep), maxstep);
				}
				else
				{
					step = (maxstep + minstep) / 2;
				}
			}
			fx_prev = fx;
			dg_prev = dg;
			grad_prev = grad;


            //if(iter >= param.max_linesearch)
            //    throw std::runtime_error("the line search routine reached the maximum number of iterations");

            //if(step < param.min_step)
            //    throw std::runtime_error("the line search step became smaller than the minimum value allowed");

            //if(step > param.max_step)
            //    throw std::runtime_error("the line search step became larger than the maximum value allowed");

            //step *= width;
        }
		int hIdx, lIdx, hIdx1, lIdx1;
		Scalar minstep = std::min(bracket[0], bracket[1]);
		Scalar maxstep = std::max(bracket[0], bracket[1]);
		while (!done && iter<param.max_linesearch)
		{
			if (bracketFval[0] > bracketFval[1])
			{
				hIdx = 0;
				lIdx = 1;
			}
			else
			{
				hIdx = 1;
				lIdx = 0;
			}
			if (bracket[0] > bracket[1])
			{
				hIdx1 = 0;
				lIdx1 = 1;
			}
			else
			{
				hIdx1 = 1;
				lIdx1 = 0;
			}
			//polyinterp to get new step size
			Scalar d1 = bracketGval[lIdx1] + bracketGval[hIdx1] - 3 * (bracketFval[lIdx1] - bracketFval[hIdx1]) / (bracket[lIdx1] - bracket[hIdx1]);
			Scalar d2 = sqrt(d1*d1 - bracketGval[lIdx1] * bracketGval[hIdx1]);
			if (d2 >= 0)
			{
				Scalar t = bracket[hIdx1] - (bracket[hIdx1] - bracket[lIdx1])*(bracketGval[hIdx1] + d2 - d1) / (bracketGval[hIdx1] - bracketGval[lIdx1] + 2 * d2);
				step = std::min(std::max(t, minstep), maxstep);
			}
			else
			{
				step = (minstep + maxstep) / 2;
			}

			//evaluate new point
			x.noalias() = xp + step * drt;
			fx = f(x, grad);
			dg = grad.dot(drt);
			/*std::cout << "fx=" << fx << ", dg=" << dg << std::endl;
			std::cout << "x=" << x.transpose() << std::endl;*/
			iter++;

			if (fx > fx_init + dg*step*1e-4 || fx > bracketFval[lIdx])
			{
				bracket[hIdx] = step;
				bracketFval[hIdx] = fx;
				bracketGval[hIdx] = dg;
			}
			else
			{
				if (abs(dg) <= -param.wolfe * dg_prev)
				{
					done = true;
				}
				else if (dg*(bracket[hIdx] - bracket[lIdx])>=0)
				{
					bracket[hIdx] = bracket[lIdx];
					bracketFval[hIdx] = bracketFval[lIdx];
					bracketGval[hIdx] = bracketGval[lIdx];
				}
				bracket[lIdx] = step;
				bracketFval[lIdx] = fx;
				bracketGval[lIdx] = dg;
			}
		}
    }
};


} // namespace LBFGSpp

#endif // LINE_SEARCH_H

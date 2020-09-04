#ifndef __DETECTOR_H__
#define __DETECTOR_H__
#include <Eigen/Dense>
#include <Eigen/Core>
#include <iostream>

class PLDSPainDetector
{
private:
	float mean_base;
	float std_base;
	float thresh;
	int nStd;// confident interval is nstd*std
public:
	//constructor
	PLDSPainDetector() : mean_base(0), std_base(0), thresh(0), nStd(0){}
	PLDSPainDetector(float thr_, int nStd_) : mean_base(0), std_base(0), thresh(thr_), nStd(nStd_){}
	PLDSPainDetector(float mb_,float stdb_, float thr_, int nStd_): mean_base(mb_),std_base(stdb_), thresh(thr_),nStd(nStd_){}

	//inlines
    float getMeanBase() const { return mean_base; }
    float getStdBase() const { return std_base; }
    float getThresh() const { return thresh; }
    int getNStd() const { return nStd; }

	void setMeanBase(float mb_){ mean_base = mb_; }
	void setStdBase(float stdb_){ std_base = stdb_; }
	void setThresh(float thr_){ thresh = thr_; }
	void setNStd(int nStd_){ nStd = nStd_; }

    float getZscoreBin(const float x) const { return (x - mean_base) / std_base; }

    void getBaselineMeanStd(const Eigen::VectorXf &x, const int idx_start, const int idx_end)
	{
        int segmentL = std::min(idx_end - idx_start + 1,x.rows()-idx_start);
        mean_base = x.segment(idx_start, segmentL).mean();
        std_base = sqrt(x.segment(idx_start, segmentL).array().pow(2).mean() - mean_base*mean_base);
	}

    void getBaselineMeanStd(const float* px, const int length)
	{
		Eigen::Map<Eigen::VectorXf> x(const_cast<float*>(px), length);
		mean_base = x.mean();
		std_base = sqrt(x.array().pow(2).mean() - mean_base*mean_base);
	}

    float getConfInterval(const float var) const
    {
         return sqrt(var)*nStd / std_base;
    }

    bool isDetected(const float zscore, const float var) const
	{
        float confInterval = sqrt(var)*nStd / std_base;
		if (zscore - confInterval > thresh || zscore + confInterval < -thresh)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

    void getZscoreTrial(const Eigen::VectorXf &x, Eigen::VectorXf &zscore) const
	{
		zscore = (x.array() - mean_base) / std_base;
	}
};
#endif

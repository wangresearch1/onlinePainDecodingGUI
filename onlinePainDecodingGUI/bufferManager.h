#pragma once
#ifndef _BUFFER_MANAGER__
#define _BUFFER_MANAGER__
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <Eigen/SparseCore>

/**
* Important Usage Note: This library reserves one spare entry for queue-full detection
* Otherwise, corner cases and detecting difference between full/empty is hard.
* You are not seeing an accidental off-by-one.
*/
namespace painBMI
{
	template <class T>
	class circular_buffer {
	protected:
		std::mutex mutex_;
		std::unique_ptr<T[]> buf_;
		size_t head_ = 0;
		size_t tail_ = 0;
		size_t size_;
	public:
		circular_buffer(size_t size) :
			buf_(std::unique_ptr<T[]>(new T[size])),
			size_(size)
		{
			//empty constructor
		}

        /* for debug */
        //size_t get_head() const {return head_;}
        /* for debug end */

        int items()
        {
            if (full())
            {
                return size_;
            }
            else if (empty())
            {
                return 0;
            }
            else
            {
                return head_-tail_;
            }
        }

		void put(T item)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			buf_[head_] = item;
			head_ = (head_ + 1) % size_;
			//std::cout << buf_[head_-1] << std::endl;

			if (head_ == tail_)
			{
				tail_ = (tail_ + 1) % size_;
			}
		}

		T get(void)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty())
			{
				return T();
			}

			//Read data and advance the tail (we now have a free space)
			auto val = buf_[tail_];
			tail_ = (tail_ + 1) % size_;

			return val;
		}

		// read from head
		T read()
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty())
			{
				return T();
			}

			auto val = buf_[(head_ -1 + size_) % size_];

			return val;
		}

		void read(T*& reVal, size_t start, size_t end)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty() || start < end)
			{
				return;
			}

			//auto val = buf_[head_];
			int cnt = 0;
			for (int i = start+1; i > end; i--)
			{
				size_t idx = (head_ - i + size_) % size_;
				reVal[cnt] = buf_[idx];
				cnt++;
			}
			return;
		}

		// Eigen::VectorXf
		 Eigen::Matrix<T,-1,1,0,-1,1> read(const size_t start, const size_t end)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty() || start < end)
			{
				std::cout << "empty or invalid idx" << std::endl;
				return Eigen::Matrix<T, -1, 1, 0, -1, 1>();
			}
			Eigen::Matrix<T, -1, 1, 0, -1, 1> vec(start-end+1);
			int cnt = 0;
			for (int i = start + 1; i > end; i--)
			{
				size_t idx = (head_ - i + size_) % size_;
				vec(cnt) = buf_[idx];
				cnt++;
			}
			
			return vec;
		}

		void reset(void)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			head_ = tail_;
		}

		bool empty(void)
		{
			//if head and tail are equal, we are empty
			return head_ == tail_;
		}

		bool full(void)
		{
			//If tail is ahead the head by 1, we are full
			return ((head_ + 1) % size_) == tail_;
		}

		size_t size(void) const
		{
			return size_ - 1;
		}
	};

	template <class T>
	class circular_buffer2d : public circular_buffer<T>
	{
	private:
		size_t width_;
	public:
        circular_buffer2d(size_t size, size_t width) :circular_buffer<T>(size*width),  // size= how many bins; width= number of units
			width_(width)
		{
			//circular_buffer<T>(size*width);
			size_ = size;
			//empty constructor
		}

        int items()
        {
            if (full())
            {
                return size_;
            }
            else if (empty())
            {
                return 0;
            }
            else
            {
                return (head_-tail_)/width_;
            }
        }

		void put(T* &p_item)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			for (int i = 0; i < width_; i++)
			{
				buf_[head_] = p_item[i];
				//std::cout << p_item[i] << std::endl;
				//if (head_>0)
				//std::cout << buf_[head_-1] << std::endl;

				head_ = (head_ + 1) % (size_*width_);

				if (head_ == tail_)
				{
					tail_ = (tail_ + 1) % (size_*width_);
				}
			}
		}

        void putEigenVec(const Eigen::VectorXf &p_item)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (int i = 0; i < width_; i++)
            {
                buf_[head_] = p_item(i);
                //std::cout << p_item[i] << std::endl;
                //if (head_>0)
                //std::cout << buf_[head_-1] << std::endl;

                head_ = (head_ + 1) % (size_*width_);

                if (head_ == tail_)
                {
                    tail_ = (tail_ + 1) % (size_*width_);
                }
            }
        }
        Eigen::Matrix<T, -1, 1, 0, -1, 1> get(void)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (empty())
            {
                return Eigen::Matrix<T, -1, 1, 0, -1, 1>();
            }
            Eigen::Matrix<T, -1, 1, 0, -1, 1> vec(width_);
            //Read data and advance the tail (we now have a free space)
            for (int i = 0; i < width_; i++)
            {
                vec(i) = buf_[tail_];
                tail_ = (tail_ + 1) % (size_*width_);
            }

            return vec;
        }

		// read from head
		void read(T* &reVal)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty())
			{
				return;
			}
			for (int i = 0; i < width_; i++)
			{
				size_t idx = (head_ - width_ + i + size_*width_) % (size_*width_);
				reVal[i] = buf_[idx];
			}
			return;
		}
		
		Eigen::Matrix<T, -1, 1, 0, -1, 1> read()
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty())
			{
				return Eigen::Matrix<T, -1, 1, 0, -1, 1>();
			}
			Eigen::Matrix<T, -1, 1, 0, -1, 1> vec(width_);
			for (int i = 0; i < width_; i++)
			{
				size_t idx = (head_ - width_ + i + size_*width_) % (size_*width_);
				vec(i) = buf_[idx];
			}
			return vec;
			
		}
		
		void read(T*& reVal, size_t start, size_t end)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty() || start < end)
			{
				return;
			}

			int cnt = 0;
			for (int i = (start+1)*width_; i > end*width_; i--)
			{
				size_t idx = (head_ - i + size_*width_) % (size_*width_);
				reVal[cnt] = buf_[idx];
				//std::cout << buf_[idx] << std::endl;
				cnt++;
			}
			return;
		}

		//Eigen::MatrixXf
		Eigen::Matrix<T, -1, -1> read(const size_t start, const size_t end)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty() || start < end)
			{
				return Eigen::Matrix<T, -1, -1>();
			}
			Eigen::Matrix<T, -1, -1> mat(width_,start-end+1);
			int cnt = 0;
			for (int i = (start + 1)*width_; i > end*width_; i--)
			{
				size_t idx = (head_ - i + size_*width_) % (size_*width_);
				mat(cnt%width_,cnt/ width_) = buf_[idx];
				cnt++;
			}
			return mat;
		}

		Eigen::Matrix<T, -1, -1> read(const size_t start, const size_t end, std::vector<int> idx_list)
		{
			std::lock_guard<std::mutex> lock(mutex_);

			if (empty() || start < end || idx_list.size() == 0 || idx_list.size()>width_)
			{
				return Eigen::Matrix<T, -1, -1>();
			}
			Eigen::Matrix<T, -1, -1> mat(idx_list.size(), start - end + 1);
			int cnt = 0;
			size_t idx0 = (head_ - (start + 1)*width_ + size_*width_) % (size_*width_);

			size_t idx;
			for (int i = 0; i <= start - end; i++)
			{
				for (int j = 0; j < idx_list.size(); j++)
				{
					idx = idx0 + idx_list[j];
					mat(cnt%idx_list.size(), cnt / idx_list.size()) = buf_[idx];
					cnt++;
					//std::cout << cnt << std::endl;
				}
				idx0 = (idx0 + width_) % (size_*width_);
			}
			return mat;
		}

		size_t width(void)
		{
			return width_ - 1;
		}
	};

	typedef std::vector<circular_buffer<float>*> ccbuf_vec;
	typedef std::vector<circular_buffer2d<float>*> ccbuf2d_vec;

	class BufferManager
	{
	private:
		ccbuf2d_vec spikebuffer_list;
		std::vector<std::string> detector_list;
		std::vector<std::string> region_list;
		ccbuf_vec z_buff_list;
		size_t size;
		std::vector<size_t> units_list;
		//size_t n_decoders;

	public:
		BufferManager(size_t size_, std::vector<size_t> units_, std::vector<std::string> detector_list_, std::vector<std::string> region_list_): 
			spikebuffer_list(), detector_list(detector_list_), region_list(region_list_), z_buff_list(), size(size_), units_list(units_)
		{
			spikebuffer_list.clear();
			for (std::vector<size_t>::iterator iter = units_list.begin(); iter < units_list.end(); iter++)
			{
				spikebuffer_list.push_back(new circular_buffer2d<float>(size_,(*iter)));
			}
			z_buff_list.clear();
			for (std::vector<std::string>::iterator iter = detector_list.begin(); iter < detector_list.end(); iter++)
			{
				z_buff_list.push_back(new circular_buffer<float>(size_));
			}
		}

		std::vector<std::string> get_region_list() const { return region_list; }
		std::vector<std::string> get_detector_list() const { return detector_list; }
		std::vector<size_t> get_units_list() const { return units_list; }
		size_t get_size() const { return size; }

		int getIdx(std::string region)
		{
			int idx = -1;
			for (std::vector<std::string>::iterator iter = region_list.begin(); iter < region_list.end(); iter++)
			{
				if (region.compare((*iter)) == 0)
				{
					idx = iter - region_list.begin();
					break;
				}
			}
			if (idx == -1)
			{
				std::cout << "invalid region: " << region << std::endl;
				return -1;
			}
			return idx;
		}

		int getDetectorIdx(std::string detector)
		{
			int idx = -1;
			for (std::vector<std::string>::iterator iter = detector_list.begin(); iter < detector_list.end(); iter++)
			{
				if (detector.compare((*iter)) == 0)
				{
					idx = iter - detector_list.begin();
					break;
				}
			}
			if (idx == -1)
			{
				std::cout << "invalid detector: " << detector << std::endl;
				return -1;
			}
			return idx;
		}

		void putSpike(float* spikes, std::string region)
		{
			int idx = getIdx(region);
			if (idx == -1)
			{
				std::cout << "putSpike: invalid region=" << region << std::endl;
				return;
			}
			spikebuffer_list[idx]->put(spikes);
		}
		
		Eigen::VectorXf readSpike(std::string region)
		{
			int idx = getIdx(region);
			if (idx == -1)
			{
				std::cout << "readSpike: invalid region="<<region << std::endl;
				return Eigen::VectorXf();
			}
			return spikebuffer_list[idx]->read();
		}
		

		void readSpike(float* &spikes, std::string region, size_t start, size_t end)
		{
			int idx = getIdx(region);
			if (idx == -1)
			{
				std::cout << "readSpike: invalid region=" << region << std::endl;
				return;
			}
			spikebuffer_list[idx]->read(spikes, start, end);
		}
		
		Eigen::MatrixXf readSpike(std::string region, size_t start, size_t end)
		{
			int idx = getIdx(region);
			if (idx == -1)
			{
				std::cout << "readSpike: invalid region=" << region << std::endl;
				return Eigen::MatrixXf();
			}
			return spikebuffer_list[idx]->read(start, end);
		}
		

		size_t getUnit(std::string region)
		{
			int idx = getIdx(region);
			if (idx == -1)
			{
				std::cout << "getUnit: invalid region=" << region << std::endl;
				return 0;
			}
			return units_list[idx];
		}

		void putZ(float z, std::string detector)
		{
			int idx = getDetectorIdx(detector);
			if (idx == -1)
			{
				std::cout << "putZ: invalid detector=" << detector << std::endl;
				return;
			}
			z_buff_list[idx]->put(z);
		}

		float readZ(std::string detector)
		{
			int idx = getDetectorIdx(detector);
			if (idx == -1)
			{
				std::cout << "readZ: invalid detector=" << detector << std::endl;
				return 0;
			}
			return z_buff_list[idx]->read();
		}

		void readZ(float*& reVal,std::string detector, size_t start, size_t end)
		{
			int idx = getDetectorIdx(detector);
			if (idx == -1)
			{
				std::cout << "readZ: invalid detector=" << detector << std::endl;
				return;
			}
			z_buff_list[idx]->read(reVal,start,end);
		}

		Eigen::VectorXf readZ(std::string detector, size_t start, size_t end)
		{
			int idx = getDetectorIdx(detector);
			if (idx == -1)
			{
				std::cout << "readZ: invalid detector=" << detector << std::endl;
				return Eigen::VectorXf();
			}
			return z_buff_list[idx]->read(start, end);
		}

		~BufferManager()
		{
			for (ccbuf2d_vec::iterator iter = spikebuffer_list.begin(); iter < spikebuffer_list.end(); iter++)
			{
				delete (*iter);
			}

			for (ccbuf_vec::iterator iter = z_buff_list.begin(); iter < z_buff_list.end(); iter++)
			{
				delete (*iter);
			}
		}
	};
}
#endif

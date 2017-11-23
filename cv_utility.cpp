#include "cv_utility.h"

#include <iostream>

cv::Mat cvutil::grayscale(const cv::Mat& original)
{
	// Check for invalid images
	if(original.depth() != CV_8U)
	{
		std::cout << "ERROR: Image does not have 8 bit depth. Grayscaling not supported!" << std::endl;
		throw std::invalid_argument{"Grayscaling of image with invalid depth"};
	}

	switch(original.channels())
	{
	case 3:	// 3 channels <=> color
	{
		auto size = original.size();
		auto gray = cv::Mat(size, CV_8UC1, cv::Scalar::all(0));

		for(int i = 0; i < size.height; ++i)
			for(int j = 0; j < size.width; ++j)
				for(int c = 0; c < original.channels(); ++c)
					gray.at<uchar>(i, j) += original.at<cv::Vec<uchar, 3>>(i, j)[c] / original.channels();
		return gray;
	}

		// Check for invalid images
	case 1:	// 1 channel <=> grayscale
		std::cout << "WARNING: Image is already grayscale!" << std::endl;
		return original.clone();

	default:	// 2 or >3 channels, not supported
		std::cout << "ERROR: Image has " << original.channels() << " channels. Grayscaling not supported!" << std::endl;
		throw std::invalid_argument{"Grayscaling of image with invalid number of channels"};
	}
}

cv::Mat cvutil::energy(const cv::Mat& original)
{
	// Check for invalid images
	if(original.type() != CV_8UC1)
	{
		std::cout << "ERROR: Image has more than one channel or a depth >8 bits. Energy function not supported!" << std::endl;
		throw std::invalid_argument{"Energy function applied to image with invalid type"};
	}

	// Sobel masks
	constexpr int maskx[3][3] = {{-1,0,1},
								 {-1,0,1},
								 {-1,0,1}};
	constexpr int masky[3][3] = {{-1,-1,-1},
								 { 0, 0, 0},
								 { 1, 1, 1}};

	// Correlation
	auto size = original.size();
	auto energy = original.clone();

	int grad_x = 0;
	int grad_y = 0;

	for(int i = 0; i < size.height; ++i)
	{
		for(int j = 0; j < size.width; ++j)
		{
			grad_x = 0;
			grad_y = 0;
			for(int offi = 0; offi < 3; ++offi)
			{
				for(int offj = 0; offj < 3; ++offj)
				{
					grad_x += maskx[offi][offj] * repeat_at<uchar>(original, i + offi - 1, j + offj - 1);
					grad_y += masky[offi][offj] * repeat_at<uchar>(original, i + offi - 1, j + offj - 1);
				}
			}
			energy.at<uchar>(i, j) = static_cast<uchar>(std::clamp(std::sqrt(grad_x*grad_x/9 + grad_y*grad_y/9) / std::sqrt(2.0), 0.0, static_cast<double>(std::numeric_limits<uchar>::max())));
		}
	}
	return energy;
}

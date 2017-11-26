#include "cv_utility.h"

#include <iostream>

cv::Mat cvutil::grayscale(const cv::Mat& image)
{
	// Check for invalid images
	if(image.depth() != CV_8U)
	{
		std::cout << "ERROR: Image does not have 8 bit depth. Grayscaling not supported!" << std::endl;
		throw std::invalid_argument{"Grayscaling of image with invalid depth"};
	}

	switch(image.channels())
	{
	case 3:	// 3 channels <=> color
	{
		auto gray = cv::Mat(image.size(), CV_8UC1, cv::Scalar::all(0));

		for(int r = 0; r < image.rows; ++r)
			for(int c = 0; c < image.cols; ++c)
				for(int ch = 0; ch < image.channels(); ++ch)
					gray.at<uchar>(r, c) += image.at<cv::Vec<uchar, 3>>(r, c)[ch] / image.channels();
		return gray;
	}

		// Check for invalid images
	case 1:	// 1 channel <=> grayscale
		std::cout << "WARNING: Image is already grayscale!" << std::endl;
		return image.clone();

	default:	// 2 or >3 channels, not supported
		std::cout << "ERROR: Image has " << image.channels() << " channels. Grayscaling not supported!" << std::endl;
		throw std::invalid_argument{"Grayscaling of image with invalid number of channels"};
	}
}

cv::Mat cvutil::energy(const cv::Mat& image)
{
	// Check for invalid images
	if(image.type() != CV_8UC1)
	{
		std::cout << "ERROR: Image has more than one channel or a depth >8 bits. Energy function not supported!" << std::endl;
		throw std::invalid_argument{"Energy function applied to image with invalid type"};
	}

	// Sobel masks
	constexpr int mask_h[3][3] = {{-1,0,1},
								 {-1,0,1},
								 {-1,0,1}};
	constexpr int mask_v[3][3] = {{-1,-1,-1},
								 { 0, 0, 0},
								 { 1, 1, 1}};

	// Correlation
	auto energy = image.clone();
	int grad_h = 0;
	int grad_v = 0;

	for(int r = 0; r < image.rows; ++r)
	{
		for(int c = 0; c < image.cols; ++c)
		{
			grad_h = 0;
			grad_v = 0;
			for(int off_r = 0; off_r < 3; ++off_r)
			{
				for(int off_c = 0; off_c < 3; ++off_c)
				{
					grad_h += mask_h[off_r][off_c] * clamp_at<uchar>(image, r + off_r - 1, c + off_c - 1);
					grad_v += mask_v[off_r][off_c] * clamp_at<uchar>(image, r + off_r - 1, c + off_c - 1);
				}
			}
			// Calculate gradient lengths using the euclidean norm.
			// Scale down to [0, max(uchar)] by dividing the gradients by 3 and the length by sqrt(2).
			// Clamp to [0, max(uchar)] to prevent possible overflows due to floating point arithmetic.
//			energy.at<uchar>(i, j) = static_cast<uchar>(std::clamp(std::sqrt(grad_h*grad_h/9 + grad_v*grad_v/9) / std::sqrt(2.0), 0.0, static_cast<double>(std::numeric_limits<uchar>::max())));

			// Calculate sum of absolute gradients.
			// Scale down to [0, max(uchar)] by dividing the sum by 2.
			energy.at<uchar>(r, c) = static_cast<uchar>((std::abs(grad_h) + std::abs(grad_v)) / 2);
		}
	}
	return energy;
}

std::vector<int> cvutil::vertical_seam(const cv::Mat& image, std::function<bool(int, int)> compare)
{
	if(image.type() != CV_8UC1)
	{
		std::cout << "ERROR: Image has more than one channel or a depth >8 bits. Seam finding not supported!" << std::endl;
		throw std::invalid_argument{"Vertical seam finding applied to image with invalid type"};
	}

	// Init
	// Route matrix
	auto routes = cv::Mat(image.size(), CV_8SC1);

	// Energy of the currently calculated row
	auto current = std::vector<int>(static_cast<size_t>(image.cols), 0);
	// Energy of the row before the current one
	auto last = std::vector<int>(static_cast<size_t>(image.cols));
	for(int c = 0; c < image.cols; ++c)	// Initialize with first row of the image
		last[static_cast<size_t>(c)] = image.at<uchar>(0, c);

	for(int r = 1; r < image.rows; ++r)
	{
		for(int c = 0; c < image.cols; ++c)
		{
			// Find max neighbour
			current[static_cast<size_t>(c)] = last[static_cast<size_t>(c)];
			routes.at<signed char>(r, c) = 0;

			if(c-1 >= 0 && compare(last[static_cast<size_t>(c-1)], current[static_cast<size_t>(c)]))
			{
				current[static_cast<size_t>(c)] = last[static_cast<size_t>(c-1)];
				routes.at<signed char>(r, c) = -1;
			}

			if(c+1 < image.cols && compare(last[static_cast<size_t>(c+1)], current[static_cast<size_t>(c)]))
			{
				current[static_cast<size_t>(c)] = last[static_cast<size_t>(c+1)];
				routes.at<signed char>(r, c) = 1;
			}
			current[static_cast<size_t>(c)] += image.at<uchar>(r, c);
		}
		current.swap(last);
	}

	auto seam = std::vector<int>(static_cast<size_t>(image.rows), 0);
	auto col = static_cast<int>(std::max_element(last.begin(), last.end(), [&compare](const auto& a, const auto& b) {return !compare(a,b);}) - last.begin());

	for(int r = routes.rows-1; r >= 0; --r)
	{
		seam[static_cast<size_t>(r)] = col;
		col += static_cast<int>(routes.at<signed char>(r, col));
	}
	return seam;
}

//std::vector<int> cvutil::horizontal_seam(const cv::Mat& image, std::function<bool(int, int)> compare)
//{

//}

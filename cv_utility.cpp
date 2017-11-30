#include "cv_utility.h"

#include <iostream>
#include <thread>

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

		// Multithreading
		auto thread_count = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, image.rows);
		auto threads = std::vector<std::thread>{};

		for(int t = 0; t < thread_count; ++t)
		{
			// Calculate the start and end of the working interval for the next thread
			int start = image.rows * t / thread_count;
			int end = image.rows * (t+1) / thread_count;

			// Start thread
			threads.emplace_back([start, end, &image, &gray] () {
				for(int r = start; r < end; ++r)
					for(int c = 0; c < image.cols; ++c)
						for(int ch = 0; ch < image.channels(); ++ch)
							gray.at<uchar>(r, c) += image.at<cv::Vec<uchar, 3>>(r, c)[ch] / image.channels();
			});
		}
		for(auto& t : threads)
			t.join();

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

	// Multithreading
	auto thread_count = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, image.rows);
	auto threads = std::vector<std::thread>{};

	for(int t = 0; t < thread_count; ++t)
	{
		// Calculate the start and end of the working interval for the next thread
		int start = image.rows * t / thread_count;
		int end = image.rows * (t+1) / thread_count;

		// Start thread
		threads.emplace_back([start, end, &image, &energy, &mask_h, &mask_v] () {
			int grad_h = 0;
			int grad_v = 0;
			for(int r = start; r < end; ++r)
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
					// Calculate gradient length using the euclidean norm.
					// Scale down to [0, max(uchar)] by dividing the gradients by 3 and the length by sqrt(2).
					// Clamp to [0, max(uchar)] to prevent possible overflows due to floating point arithmetic.
					energy.at<uchar>(r, c) = static_cast<uchar>(std::clamp(std::sqrt(grad_h*grad_h/9 + grad_v*grad_v/9) / std::sqrt(2.0), 0.0, static_cast<double>(std::numeric_limits<uchar>::max())));

					// Calculate sum of absolute gradients.
					// Scale down to [0, max(uchar)] by dividing the sum by 2.
					energy.at<uchar>(r, c) = static_cast<uchar>((std::abs(grad_h) + std::abs(grad_v))/6);
				}
			}
		});
	}
	for(auto& t : threads)
		t.join();

	return energy;
}

std::vector<int> cvutil::vertical_seam(const cv::Mat& image, std::function<bool(int, int)> compare)
{
	if(image.type() != CV_8UC1)
	{
		std::cout << "ERROR: Image has more than one channel or a depth >8 bits. Seam finding not supported!" << std::endl;
		throw std::invalid_argument{"Vertical seam finding applied to image with invalid type"};
	}
	if(image.cols <= 1)
	{
		std::cout << "ERROR: Image has only one or less columns. Seam finding not supported!" << std::endl;
		throw std::invalid_argument{"Vertical seam finding applied to image with too few columns"};
	}

	// Init
	// Route matrix
	auto routes = cv::Mat(image.size(), CV_8SC1);

	// Energy of the currently calculated row
	auto current = std::vector<int>(static_cast<size_t>(image.cols), 0);
	// Energy of the row before the current one
	auto last = std::vector<int>(static_cast<size_t>(image.cols));
	// Initialize with first row of the image
	for(int c = 0; c < image.cols; ++c)
		last[static_cast<size_t>(c)] = image.at<uchar>(0, c);

	// Multithreading
	auto thread_count = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, image.cols);
	auto threads = std::vector<std::thread>{};

	for(int r = 1; r < image.rows; ++r)
	{
		for(int t = 0; t < thread_count; ++t)
		{
			// Start thread
			threads.emplace_back([t, &thread_count, &image, &current, &last, &routes, &compare, &r] () {
				for(int c = t; c < image.cols; c+=thread_count)
				{
					current[static_cast<size_t>(c)] = last[static_cast<size_t>(c)];
					routes.at<signed char>(r, c) = 0;
					// Find max neighbour
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
					// Set value of this column to max(neighbours) + local
					current[static_cast<size_t>(c)] += image.at<uchar>(r, c);
				}
			});
		}
		for(auto& t : threads)
			t.join();
		threads.clear();

		// Last = current, current will be overwritten during the next iteration
		current.swap(last);
	}

	auto seam = std::vector<int>(static_cast<size_t>(image.rows), 0);
	auto col = static_cast<int>(std::max_element(last.begin(), last.end(), [&compare] (const auto& a, const auto& b) { return !compare(a,b); }) - last.begin());

	for(int r = routes.rows-1; r >= 0; --r)
	{
		seam[static_cast<size_t>(r)] = col;
		col += static_cast<int>(routes.at<signed char>(r, col));
	}
	return seam;
}

std::vector<int> cvutil::horizontal_seam(const cv::Mat& image, std::function<bool(int, int)> compare)
{
	if(image.type() != CV_8UC1)
	{
		std::cout << "ERROR: Image has more than one channel or a depth >8 bits. Seam finding not supported!" << std::endl;
		throw std::invalid_argument{"Vertical seam finding applied to image with invalid type"};
	}
	if(image.rows <= 1)
	{
		std::cout << "ERROR: Image has only one or less rows. Seam finding not supported!" << std::endl;
		throw std::invalid_argument{"Horizontal seam finding applied to image with too few rows"};
	}

	// Init
	// Route matrix
	auto routes = cv::Mat(image.size(), CV_8SC1);

	// Energy of the currently calculated column
	auto current = std::vector<int>(static_cast<size_t>(image.rows), 0);
	// Energy of the column before the current one
	auto last = std::vector<int>(static_cast<size_t>(image.rows));
	for(int r = 0; r < image.rows; ++r)	// Initialize with first column of the image
		last[static_cast<size_t>(r)] = image.at<uchar>(r, 0);

	// Multithreading
	auto thread_count = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, image.rows);
	auto threads = std::vector<std::thread>{};

	for(int c = 1; c < image.cols; ++c)
	{
		for(int t = 0; t < thread_count; ++t)
		{
			// Start thread
			threads.emplace_back([t, &thread_count, &image, &current, &last, &routes, &compare, &c] () {
				for(int r = t; r < image.rows; r+=thread_count)
				{
					// Find max neighbour
					current[static_cast<size_t>(r)] = last[static_cast<size_t>(r)];
					routes.at<signed char>(r, c) = 0;

					if(r-1 >= 0 && compare(last[static_cast<size_t>(r-1)], current[static_cast<size_t>(r)]))
					{
						current[static_cast<size_t>(r)] = last[static_cast<size_t>(r-1)];
						routes.at<signed char>(r, c) = -1;
					}

					if(r+1 < image.rows && compare(last[static_cast<size_t>(r+1)], current[static_cast<size_t>(r)]))
					{
						current[static_cast<size_t>(r)] = last[static_cast<size_t>(r+1)];
						routes.at<signed char>(r, c) = 1;
					}
					// Set value of this row to max(neighbours) + local
					current[static_cast<size_t>(r)] += image.at<uchar>(r, c);
				}
			});
		}
		for(auto& t : threads)
			t.join();
		threads.clear();

		// Last = current, current will be overwritten during the next iteration
		current.swap(last);
	}

	auto seam = std::vector<int>(static_cast<size_t>(image.cols), 0);
	auto row = static_cast<int>(std::max_element(last.begin(), last.end(), [&compare] (const auto& a, const auto& b) { return !compare(a,b); }) - last.begin());

	for(int c = routes.cols-1; c >= 0; --c)
	{
		seam[static_cast<size_t>(c)] = row;
		row += static_cast<int>(routes.at<signed char>(row, c));
	}
	return seam;
}

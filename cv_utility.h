#ifndef CV_UTILITY_H
#define CV_UTILITY_H

#include "opencv2/core/core.hpp"
#include <iostream>

namespace cvutil
{
	/**
	 * @brief grayscale Converts 8UC3 images to 8UC1 by averaging the channels per pixel.
	 * @param original The original 8UC3 image.
	 * @return The averaged 8UC1 image.
	 */
	cv::Mat grayscale(const cv::Mat& original);

	/**
	 * @brief energy Converts a single channel image to the result of its energy function per pixel.
	 * To achieve this, a sobel edge detection is applied to the original and the gradient lengths are in a new image of the same type.
	 * @param original The original one channel image.
	 * @return The new image of equal type containing the gradient lengths.
	 */
	cv::Mat energy(const cv::Mat& original);

	template<typename T>
	/**
	 * @brief clamp_at Mat::at(i0, i1) but with edge-clamped out-of-range coordinates.
	 * @param image The image that is accessed.
	 * @param row The row index.
	 * @param col The column index.
	 * @return The element at image[cow, col]
	 */
	const T& clamp_at(const cv::Mat& image, int row, int col)
	{
		auto size = image.size();
		row = std::clamp(row, 0, size.height-1);
		col = std::clamp(col, 0, size.width-1);
		/*DEBUG REMOVE THIS*/if(row < 0 || col < 0 || row >= size.height || col >= size.width) std::cout << "FAIL(" << row << ',' << col << ')' << std::endl;
		return image.at<T>(row, col);
	}

	template<typename T>
	/**
	 * @brief mirror_at Mat::at(i0, i1) but with edge-mirrored out-of-range coordinates.
	 * @param image The image that is accessed.
	 * @param row The row index.
	 * @param col The column index.
	 * @return The element at image[cow, col]
	 */
	const T& mirror_at(const cv::Mat& image, int row, int col)
	{
		auto size = image.size();
		if(row < 0)
			row = (std::abs(row) % size.height) - 1;
		if(row >= size.height)
			row = size.height - (row % size.height) - 1;

		if(col < 0)
			col = (std::abs(col) % size.width) - 1;
		if(col >= size.width)
			col = size.width - (col % size.width) - 1;

		/*DEBUG REMOVE THIS*/if(row < 0 || col < 0 || row >= size.height || col >= size.width) std::cout << "FAIL(" << row << ',' << col << ')' << std::endl;
		return image.at<T>(row, col);
	}

	template<typename T>
	/**
	 * @brief repeat_at Mat::at(i0, i1) but with repeated out-of-range coordinates.
	 * @param image The image that is accessed.
	 * @param row The row index.
	 * @param col The column index.
	 * @return The element at image[cow, col]
	 */
	const T& repeat_at(const cv::Mat& image, int row, int col)
	{
		auto size = image.size();
		row = (row+size.height) % size.height;
		col = (col+size.width) % size.width;
		/*DEBUG REMOVE THIS*/if(row < 0 || col < 0 || row >= size.height || col >= size.width) std::cout << "FAIL(" << row << ',' << col << ')' << std::endl;
		return image.at<T>(row, col);
	}
}

#endif // CV_UTILITY_H
#ifndef CV_UTILITY_H
#define CV_UTILITY_H

#include "opencv2/core/core.hpp"
#include <iostream>

namespace cvutil
{
	/**
	 * @brief grayscale Converts 8UC3 images to 8UC1 by averaging the channels per pixel.
	 * @param image The original 8UC3 image.
	 * @return The averaged 8UC1 image.
	 */
	cv::Mat grayscale(const cv::Mat& image);

	/**
	 * @brief energy Converts a single channel image to the result of its energy function per pixel.
	 * To achieve this, a sobel edge detection is applied to the original and the gradient lengths are in a new image of the same type.
	 * @param image The original one channel image.
	 * @return The new image of equal type containing the gradient lengths.
	 */
	cv::Mat energy(const cv::Mat& image);

	std::vector<int> vertical_seam(const cv::Mat& image, std::function<bool(int, int)> compare = std::less<int>());

	std::vector<int> horizontal_seam(const cv::Mat& image, std::function<bool(int, int)> compare = std::less<int>());

	template<typename T>
	/**
	 * @brief remove_vertical_seam Removes one pixel per row by moving all pixels after that one to the left and reducing the matrix header by one column.
	 * @param image The image that is modified.
	 * @param seam The vector that contains the column coordinate for each row. seam.size() == image.rows
	 */
	void remove_vertical_seam(cv::Mat& image, const std::vector<int>& seam)
	{
		if(image.rows != static_cast<int>(seam.size()))
		{
			std::cout << "ERROR: Seam size does not match up with image width. Seam removal not supported!" << std::endl;
			throw std::invalid_argument{"Vertical seam removal applied to mismatching image and seam"};
		}

		for(int r = 0; r < image.rows; ++r)
			for(int c = seam[static_cast<size_t>(r)]+1; c < image.cols; ++c)
				image.at<T>(r, c-1) = image.at<T>(r, c);

		image = cv::Mat(image, cv::Range(0, image.rows), cv::Range(0, image.cols-1));
	}

	template<typename T>
	/**
	 * @brief remove_horizontal_seam Removes one pixel per column by moving all pixels after that one upwards and reducing the matrix header by one row.
	 * @param image The image that is modified.
	 * @param seam The vector that contains the row coordinate for each column. seam.size() == image.cols
	 */
	void remove_horizontal_seam(cv::Mat& image, const std::vector<int>& seam)
	{
		if(image.cols != static_cast<int>(seam.size()))
		{
			std::cout << "ERROR: Seam size does not match up with image height. Seam removal not supported!" << std::endl;
			throw std::invalid_argument{"Horizontal seam removal applied to mismatching image and seam"};
		}

		for(int c = 0; c < image.cols; ++c)
			for(int r = seam[static_cast<size_t>(c)]+1; r < image.rows; ++r)
				image.at<T>(r-1, c) = image.at<T>(r, c);

		image = cv::Mat(image, cv::Range(0, image.rows-1), cv::Range(0, image.cols));
	}

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
		return image.at<T>(row, col);
	}
}

#endif // CV_UTILITY_H

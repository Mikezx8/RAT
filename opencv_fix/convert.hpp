#pragma once
#include <opencv2/core.hpp>

// Safe conversion functions that don't use the problematic cvtColor
namespace safe_opencv {
    inline cv::Mat convertBGRAtoBGR(const cv::Mat& bgra) {
        cv::Mat bgr(bgra.rows, bgra.cols, CV_8UC3);
        
        for (int y = 0; y < bgra.rows; y++) {
            for (int x = 0; x < bgra.cols; x++) {
                cv::Vec4b bgra_pixel = bgra.at<cv::Vec4b>(y, x);
                cv::Vec3b& bgr_pixel = bgr.at<cv::Vec3b>(y, x);
                
                bgr_pixel[0] = bgra_pixel[0]; // B
                bgr_pixel[1] = bgra_pixel[1]; // G
                bgr_pixel[2] = bgra_pixel[2]; // R
            }
        }
        
        return bgr;
    }
    
    inline cv::Mat convertBGRtoBGRA(const cv::Mat& bgr) {
        cv::Mat bgra(bgr.rows, bgr.cols, CV_8UC4);
        
        for (int y = 0; y < bgr.rows; y++) {
            for (int x = 0; x < bgr.cols; x++) {
                cv::Vec3b bgr_pixel = bgr.at<cv::Vec3b>(y, x);
                cv::Vec4b& bgra_pixel = bgra.at<cv::Vec4b>(y, x);
                
                bgra_pixel[0] = bgr_pixel[0]; // B
                bgra_pixel[1] = bgr_pixel[1]; // G
                bgra_pixel[2] = bgr_pixel[2]; // R
                bgra_pixel[3] = 255;        // A
            }
        }
        
        return bgra;
    }
}

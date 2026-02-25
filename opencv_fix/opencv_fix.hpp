#pragma once

// Define our own cvtColor function without AlgorithmHint
#include <opencv2/core.hpp>

namespace cv {
    // Forward declare the 4-parameter version that actually exists in the library
    CV_EXPORTS void cvtColor(InputArray src, OutputArray dst, int code, int dstCn = 0);
}

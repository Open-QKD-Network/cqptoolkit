/*!
* @file
* @brief CQP Toolkit - OpenCL helper class
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 16 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#if defined(OPENCL_FOUND)
#include "Platform.h"
#include <vector>
#include <memory>

#define CL_HPP_TARGET_OPENCL_VERSION 200
//#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY

#if defined(__APPLE__) || defined(__MACOSX)
    #include <OpenCL/cl2.hpp>
#else
    #include <CL/cl2.hpp>
#endif

namespace cl
{
    class Platform;
    class Device;
} // namespace cl

namespace cqp
{
    /// Provide a simple way of creating and managing an OpenCL context
    /// @details This uses the https://www.khronos.org/opencl/ {OpenCL} library developed by the Khronos group
    /// From the opencl website:
    ///     OpenCL (Open Computing Language) is the open, royalty-free standard for cross-platform, parallel programming of diverse processors found in personal computers, servers, mobile devices and embedded platforms. OpenCL greatly improves the speed and responsiveness of a wide spectrum of applications in numerous market categories from gaming and entertainment to scientific and medical software.
    ///
    /// for Intel: https://wiki.tiker.net/OpenCLHowTo
    class CQPTOOLKIT_EXPORT OpenCLHelper
    {
    public:
        /// Check the return value of cl calls and report any error
        /// @param result The return parameter of an OpenCL call.
        /// @return The value of result
        static int32_t LogCL(int32_t result);

        static bool FindBestDevice(cl::Platform& myPlatform, cl::Device& myDevice);

        /// Get the error string for a return value
        /// @param[in] error The return code of an OpenCL call
        /// @return The descriptive string for the error code.
        static const std::string ClErrorString(int32_t error);

    }; // class OpenCLHelper
} // namespace cqp
#endif // defined(OPENCL_FOUND)

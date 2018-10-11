/*!
* @file
* @brief SecFileBuff
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 20/10/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "CQPToolkit/Util/Platform.h"
#include <streambuf>
#include <cryptopp/secblock.h>

namespace cqp
{

    /**
     * @brief The SecFileBuff class
     * A streambuf (for use in iostreams) which stores it's data in a secure buffer.
     * Data is read/writen to a basic file handle
     */
    class CQPTOOLKIT_EXPORT SecFileBuff : public std::streambuf
    {
    public:
        /**
         * @brief SecFileBuff
         * constructor
         * @param fd file handle to access
         * @param bufferSize size of the buffer
         */
        SecFileBuff(FILE_HANDLE fd, size_t bufferSize);

    protected:
        ///@{
        /// @name basic_streambuf interface

        int_type underflow() override;
        int_type overflow(int_type __c) override;

        /// @}

        ///@{
        /// @name internal
        int fileDescriptor = -1;
        const size_t bufferSize;
        CryptoPP::SecBlock<char> underflowBuffer;
        CryptoPP::SecBlock<char> overflowBuffer;
        ///@}
    };

} // namespace cqp



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

#include <streambuf>
#include <cryptopp/secblock.h>
#include "Algorithms/Util/FileIO.h"
#include "CQPToolkit/cqptoolkit_export.h"

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

        /** React to buffer underflow
         * @return The last character retrieved
         */
        int_type underflow() override;

        /** React to buffer underflow
         * @return The last character retrieved
         */
        int_type overflow(int_type __c) override;

        /// @}

        ///@{
        /// @name internal

        /// The open file handle
        int fileDescriptor = -1;
        /// The size of underflowBuffer and overflowBuffer
        const size_t bufferSize;
        /// storage when underflow is called
        CryptoPP::SecBlock<char> underflowBuffer;
        /// storage when overflow is called
        CryptoPP::SecBlock<char> overflowBuffer;
        ///@}
    };

} // namespace cqp



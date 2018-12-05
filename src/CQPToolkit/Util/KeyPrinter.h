/*!
* @file
* @brief CQP Toolkit - Random Number Generator
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/

#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include <mutex>
#pragma once

namespace cqp
{
    /// Outputs any string it receives as hex to the logger
    class CQPTOOLKIT_EXPORT KeyPrinter : public virtual IKeyCallback
    {
    public:
        /// Default constructor
        KeyPrinter() {}

        /**
         * @brief KeyPrinter
         * Constructor
         * @param prefix Prepend this string to every output
         */
        explicit KeyPrinter(const std::string& prefix);

        /**
         * @brief SetOutputPrefix
         * Change the string prefixed to the output
         * @param newPrefix the new prefix
         */
        void SetOutputPrefix(const std::string& newPrefix);

        ///@{
        /// @name IKeyCallback interface

        /**
         * @brief OnKeyGeneration
         * Called with newly produced key
         * Will output the key as a hex string to the standard logger
         * @param keyData
         */
        virtual void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;

        /// @}
    private:
        /// prevents multiple threads from outputting to the logger at the same time.
        static std::mutex outputGuard;
        /// prepend this to each output
        std::string outputPrefix = "";
    };
}

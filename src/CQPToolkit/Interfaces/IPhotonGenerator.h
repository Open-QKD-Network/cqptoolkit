/*!
* @file
* @brief CQP Toolkit - Photon
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 08 Feb 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include "CQPToolkit/cqptoolkit_export.h"
#include "Algorithms/Datatypes/Qubits.h"

namespace cqp
{

    // possible parameters
    // pulse width
    // avg. photons per pulse (<1.0)
    // filter bias of bases
    // wavelength + tolerance
    // photon state purity

    /// @brief  A device which produces photons.
    /// @details Calls the associated callback interface when new data is available.
    class CQPTOOLKIT_EXPORT IPhotonGenerator
    {
    public:
        /// Cause the generator to output the specified Qubit
        /// @details This must block until the transmission has been completed
        virtual void Fire() = 0;

        /// Notify the receiver that the frame has started
        virtual void StartFrame() = 0;

        /// Notify the receiver that the frame has ended
        virtual void EndFrame() = 0;

        /// pure virtual destructor
        virtual ~IPhotonGenerator() = default;
    };

}

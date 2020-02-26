/*!
* @file
* @brief ISessionCallback
* @copyright Copyright (C) KETS Quantum Ltd 2020.
*    Contact: https://kets-quantum.com/
*    See LICENCE for details
* @date 25/2/2020
* @author Richard Collins <richard.collins@kets-quantum.com>
*/
#pragma once
#include "CQPToolkit/cqptoolkit_export.h"
#include "QKDInterfaces/Device.pb.h"

namespace cqp
{

    /**
     * ISessionCallback class allows classes to be notified when the session changes
     */
    class CQPTOOLKIT_EXPORT ISessionCallback
    {
    public:
        /**
         * @brief NewSessionDetails
         * Called when a new session is starting
         * @param sessionDetails The new settings
         */
        virtual void NewSessionDetails(const remote::SessionDetailsFrom& sessionDetails) = 0;
        /**
         * @brief SessionHasEnded
         * Called when a session had ended
         */
        virtual void SessionHasEnded() = 0;
        /// Class destructor
        virtual ~ISessionCallback() = default;
    }; // class ISessionCallback

} // namespace cqp


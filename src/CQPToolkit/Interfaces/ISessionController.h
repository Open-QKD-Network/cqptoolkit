/*!
* @file
* @brief ISessionController
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 6/2/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include "Algorithms/Datatypes/Base.h"
#include "Algorithms/Datatypes/URI.h"
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/codegen/sync_stream.h>
#include "CQPToolkit/cqptoolkit_export.h"

namespace grpc
{
    class Status;
}

namespace cqp
{
    namespace remote
    {
        class LinkStatus;
        class SessionDetailsFrom;
    }

    /**
     * @brief The ISessionController class
     * Interface for managing key generation between two points
     */
    class CQPTOOLKIT_EXPORT ISessionController
    {
    public:

        /**
         * @brief RegisterServices
         * @param builder builder to register with
         */
        virtual void RegisterServices(grpc::ServerBuilder& builder) = 0;

        /**
         * @brief Connect
         * Connect to a running service, requires that StartServer has previously been called
         * @param otherController The other side to connect to
         * @return status of the connection
         */
        virtual grpc::Status Connect(URI otherController) = 0;

        /**
         * @brief Disconnect
         */
        virtual void Disconnect() = 0;

        /// @copydoc remote::IDevice::GetLinkStatus
        /// @param context server details
        virtual grpc::Status GetLinkStatus(grpc::ServerContext* context, ::grpc::ServerWriter<remote::LinkStatus>* writer) = 0;

        /**
         * @brief StartSession
         * Instruct both sides to start generating key
         * From this point on, GetKeyPublisher can be called
         * @details
         * @startuml StartSessionBehaviour
         *    control "SessionController : A" as SeshA
         *    boundary "ISessionController : B" as SeshB
         *
         *    [-> SeshA : StartSession
         *    activate SeshA
         *        note over SeshA
         *            Connect to the other session controller
         *        end note
         *        SeshA -> SeshB : SessionStarting()
         *    deactivate SeshA
         * @enduml
         * @param sessionDetails settings for the session
         * @return result of command
         */
        virtual grpc::Status StartSession(const remote::SessionDetailsFrom& sessionDetails) = 0;

        /**
         * @brief EndSession
         * Stop generating key
         */
        virtual void EndSession() = 0;

        /**
         * @brief ~ISessionController
         * Destructor
         */
        virtual ~ISessionController() = default;
    };

} // namespace cqp



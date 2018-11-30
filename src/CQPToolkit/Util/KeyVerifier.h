/*!
* @file
* @brief KeyVerifier
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 14/8/2017
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once

#include "CQPToolkit/Interfaces/IKeyPublisher.h"
#include "CQPAlgorithms/Util/Event.h"
#include <mutex>


namespace cqp
{

    /**
     * @brief The IKeyVerificationFailure class
     * Interface for notification of a key failure.
     */
    class CQPTOOLKIT_EXPORT IKeyVerificationFailure
    {
    public:
        /**
         * @brief OnKeyVerifyFailure
         * Callback is issued when alice and bobs keys do not match
         * @param id The key id which didn't match
         * @param first The value of the first key to be received
         * @param second The value of the first key to be received
         */
        virtual void OnKeyVerifyFailure(const KeyID& id, const PSK& first, const PSK& second) = 0;

        /// Virtual destructor
        virtual ~IKeyVerificationFailure() = default;
    };

    /**
     * @brief The KeyVerifier class
     * This class must be attached to both alice and bobs key publisher
     * Obviously this class is for testing only.
     */
    class CQPTOOLKIT_EXPORT KeyVerifier :
        public Event<void (IKeyVerificationFailure::*)(const KeyID&, const PSK&, const PSK&), &IKeyVerificationFailure::OnKeyVerifyFailure>
    {
    public:
        KeyVerifier() = default;
        ~KeyVerifier() override;

        /**
         * @brief The Receiver class
         * Receive keys from a publisher and compare them to existing keys
         */
        class Receiver : public virtual IKeyCallback
        {
        public:
            /// @{
            /// @name IKeyCallback interface

            /**
             * @brief OnKeyGeneration
             * Stores the incoming key until another instance is received, when they will be compared, if they do not match
             * OnKeyVerifyFailure is called on all listeners
             * @param keyData
             */
            void OnKeyGeneration(std::unique_ptr<KeyList> keyData) override;

            /// @}

            /**
             * @brief Receiver
             * Constructor
             * @param kv parent class which holds the received keys
             * @param side Which of the pair of keys is this receiving
             */
            Receiver(KeyVerifier* kv, bool side): parent(kv), isLeft(side) {}
        protected:
            /// The holder of the keys
            KeyVerifier* parent;
            /// which side is this receiving
            bool isLeft = false;
            /// key id counter
            KeyID nextId = 0;
        };
        friend Receiver;

        /// The receiver for one side of the key pair
        Receiver receiverA {this, true};
        /// The receiver for one side of the key pair
        Receiver receiverB {this, false};
    protected:
        /// Protect access to the received keys
        std::mutex storagelock;
        /// storage for keys while we wait for the other copy to come in.
        std::unordered_map<KeyID, PSK> receivedKeys;
    };

} // namespace cqp



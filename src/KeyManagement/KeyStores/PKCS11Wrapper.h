/*!
* @file
* @brief PKCS11Wrapper
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/7/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>

// pkcs11t.h include is non-trivial
// The following prepares for the include

#if defined(_MSC_VER)
    #pragma pack(push, cryptoki, 1)
#endif

/// A pointer
#define CK_PTR *

#if defined(_MSC_VER)
/// Function declatation macro
#define CK_DECLARE_FUNCTION(returnType, name) \
    returnType __declspec(dllimport) name
#else
/// Function declaration macro
#define CK_DECLARE_FUNCTION(returnType, name) \
    returnType name
#endif

#if defined(_MSC_VER)
/// Function pointer declatation macro
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) \
    returnType __declspec(dllimport) (* name)
#else
/// Function pointer declaration macro
#define CK_DECLARE_FUNCTION_POINTER(returnType, name) \
    returnType (* name)
#endif

/// Callback Function declaration macro
#define CK_CALLBACK_FUNCTION(returnType, name) \
    returnType (* name)

#if !defined(NULL_PTR)
    /// Null pointer definition
    #define NULL_PTR nullptr
#endif

#include <pkcs11/pkcs11t.h>

#if defined(_MSC_VER)
    #pragma pack(pop, cryptoki)
#endif

// end of pkcs11t.h include
#include <vector>
#include <memory>
#include <map>
#include "Algorithms/Logging/Logger.h"
#include "Algorithms/Util/Strings.h"
#include "Algorithms/Datatypes/Keys.h"
#include <chrono>
#include "KeyManagement/keymanagement_export.h"

// the global namespace prefix (::) is used here just to clarify that a type is
// a pure C definition.

namespace cqp
{

    namespace p11
    {
        /**
         * @brief CheckP11
         * Report an error if the return code is not CK_OK
         * @param retVal Return value to check
         * @return Always returns retVal
         */
        KEYMANAGEMENT_EXPORT ::CK_RV CheckP11(::CK_RV retVal);

        class DataObject;
        class AttributeList;

        /// A list of DataObjects
        using ObjectList = std::vector<DataObject>;
        /// A list of slots
        using SlotList = std::vector<::CK_SLOT_ID>;
        /// A list of mechanisms
        using MechanismList = std::vector<::CK_MECHANISM_TYPE>;

        /// The number of characters a label is padded/truncated to
        const int LabelSize = 32;

        /**
         * @brief Convert a white space padded string to a normal string
         * @tparam N Length of fixed length string
         * @param str String to trim
         * @return str trimmed from the right of any white space
         */
        template<unsigned int N>
        std::string FromPKCSString(const unsigned char (& str)[N]);

        /**
         * @brief The Module class
         * Manages access to the PKCS library by importing dynamic functions and
         * managing cleanup.
         */
        class KEYMANAGEMENT_EXPORT Module
        {
        public:
            /**
             * @brief Create
             * Open a PKCS library, if the initialisation fails, it will return a null pointer
             * @param libName filename of the library to load, in Linux this would be like "libmypkcs.so"
             * Full paths /can/ be included
             * @param reserved The value to pass in the reserved field of the initialisation data.
             * @return A Module instance if the loading was successful
             */
            static std::shared_ptr<Module> Create(const std::string& libName, const void* reserved = nullptr);

            /// destructor
            ~Module();

            /**
             * @brief GetInfo
             * @param[out] info Destination for module info
             * @return CK_OK on success
             */
            ::CK_RV GetInfo(::CK_INFO& info) const;

            /**
             * @brief GetSlotList
             * Get a list of slot ids
             * @param[in] tokenPresent If true, only return slots with tokens (ie useful slots)
             * @param[out] slots The slots found
             * @return CK_OK on success
             */
            ::CK_RV GetSlotList(bool tokenPresent, SlotList& slots);

            /**
             * @brief P11Lib
             * Get the function list which provides access to the library
             * @return PKCS11 function pointers
             */
            inline const CK_FUNCTION_LIST* P11Lib() const;
        protected: // methods
            /**
             * @brief Module
             * Constructor
             */
            Module() {}

        protected: // members
            /// handle provided by dlopen, et al
            void* libHandle = nullptr;
            /// A list of function pointers to access the library
            ::CK_FUNCTION_LIST_PTR functions = nullptr;
            /// Parameters to initialise the library with
            ::CK_C_INITIALIZE_ARGS initArgs {};

            /// modules can only loaded once
            static std::map<std::string, std::weak_ptr<Module>> loadedModules;
        }; // class Module

        const CK_FUNCTION_LIST*Module::P11Lib() const
        {
            return functions;
        } // Module

        /**
         * @brief The Slot class
         * From (PKCS#11 Docs)[http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html]
         *
         * > A logical reader that potentially contains a token.
         */
        class KEYMANAGEMENT_EXPORT Slot
        {
        public:
            /**
             * @brief Slot
             * Create a slot
             * @param module The module which produced the slot id
             * @param slotId The ID of the slot
             */
            Slot(std::shared_ptr<Module> module, ::CK_SLOT_ID slotId);
            /// Destructor
            ~Slot();

            /**
             * @brief InitToken
             * Initialise the token, pre-pare it for first use. Only needs to be performed once per token
             * Not needed to use the token - this is a step usually performed by the owner of the token
             * @param[in] pin passphrase for the accessing the token
             * If re-initialising the token, this must match the previous pin
             * @param[in] label Human readable label for the token
             * @return CK_OK on success
             */
            ::CK_RV InitToken(const std::string& pin, const std::string& label);

            /**
             * @brief GetMechanismList
             * Get a list of mechanisms (A process for implementing a cryptographic operation.)
             * @param[out] mechanismList Destination for the list
             * @return CK_OK on success
             */
            ::CK_RV GetMechanismList(MechanismList& mechanismList);

            /**
             * @brief GetMechanismInfo
             * Get details of a specific mechanism
             * @param[in] mechType Which mechanism to get
             * @param[out] info Destination for the info
             * @return CK_OK on success
             */
            ::CK_RV GetMechanismInfo(::CK_MECHANISM_TYPE mechType, ::CK_MECHANISM_INFO& info);

            /**
             * @brief GetTokenInfo
             * Get details about the token
             * @param[out] tokenInfo Destination for the info
             * @return CK_OK on success
             */
            ::CK_RV GetTokenInfo(::CK_TOKEN_INFO& tokenInfo);

            /**
             * @brief GetModule
             * @return The module this slot is linked to
             */
            inline std::shared_ptr<Module> GetModule();

            /**
             * @brief GetID
             * @return The id for this slot
            */
            inline ::CK_SLOT_ID GetID() const;
        protected:
            /// The module for this slot
            std::shared_ptr<Module> myModule;
            /// This slots id
            ::CK_SLOT_ID id {};
            /// PKCS functions
            const ::CK_FUNCTION_LIST * const functions;
        }; // class Slot

        /**
         * @brief The Session class
         * From [PKCS#11 Docs](http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html)
         *
         * > A logical connection between an application and a token.
         */
        class KEYMANAGEMENT_EXPORT Session : public std::enable_shared_from_this<Session>
        {
        public:

            /// Default open flags
            static const CK_FLAGS DefaultFlags = CKF_RW_SESSION | CKF_SERIAL_SESSION;

            /**
             * @brief Create
             * Instantiates a session
             * @details This is required due to the need for shared pointers.
             * @param slot The slot to attach this session to
             * @param flags Initialisation flags
             * @param callbackData Data passed to the callback
             * @param callback Provide a notification callback function to receive details about
             * cryptographic functions. see section 5.16 of the [PKCS11 documentation](http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html#_Toc416959752)
             * @return A pointer to the new session object
             */
            inline static std::shared_ptr<Session> Create(
                std::shared_ptr<Slot> slot,
                CK_FLAGS flags = DefaultFlags,
                void* callbackData = nullptr,
                CK_NOTIFY callback = nullptr
            );

            // destructor
            ~Session();

            /**
             * @brief Login
             * Authenticate to the device, this must be called before accessing keys
             * @param userType Which user to log in as
             * @param pin Password for user
             * @return CK_OK on success
             */
            ::CK_RV Login(::CK_USER_TYPE userType, const std::string& pin);

            /**
             * @brief Logout
             * @return CK_OK on success
             */
            ::CK_RV Logout();

            /**
             * @brief GetSessionInfo
             * Get the session info
             * @param[out] info destination for the info
             * @return CK_OK on success
             */
            ::CK_RV GetSessionInfo(::CK_SESSION_INFO& info);

            /**
             * @brief CloseSession
             * @return CK_OK on success
             */
            ::CK_RV CloseSession();

            /**
             * @brief FindObjects
             * Create DataObject instances based on search parameters
             * @param[in] searchParams a list of attributes to search on
             * @param[in] maxResults The maximum number of results to return
             * @param[out] results DataObjects representing the results
             * @return CK_OK on success
             * @details
             * Example usage:
             * @code
             * ObjectList found;
             * AttributeList attrList;
             * attrList.Set(CKA_CLASS, keyClass);
             * attrList.Set(CKA_KEY_TYPE, keyType);
             * attrList.Set(CKA_ID, htobe64(keyId));
             * auto result = session->FindObjects(attrList, 1, found);
             * @endcode
             */
            ::CK_RV FindObjects(const AttributeList& searchParams, unsigned long maxResults, ObjectList& results);

            /**
             * @brief WrapKey
             * @param mechanism The wrapping mechanism
             * @param wrappingKey The wrapping key
             * @param key The key to be wrapped
             * @param wrappedKey the result
             * @return CK_OK on success
             */
            CK_RV WrapKey(CK_MECHANISM_PTR mechanism, const DataObject& wrappingKey,
                          const DataObject& key, std::vector<uint8_t>& wrappedKey);

            /**
             * @brief UnwrapKey
             * @param mechanism The wrapping mechanism
             * @param unwrappingKey The unwrapping key
             * @param wrappedkey The key to be unwrapped
             * @param keyTemplate Template for the new key
             * @param key the result
             * @return CK_OK on success
             */
            CK_RV UnwrapKey(CK_MECHANISM_PTR mechanism, const DataObject& unwrappingKey,
                            const std::vector<uint8_t>& wrappedkey, const AttributeList& keyTemplate, DataObject& key);

            /**
             * @brief GetSlot
             * @return The slot for this session
             */
            inline std::shared_ptr<Slot> GetSlot();

            /**
             * @brief GetSessionHandle
             * @return The session handle
             */
            inline CK_SESSION_HANDLE GetSessionHandle() const;

            /**
             * @brief IsLoggedIn
             * @return true if the token is logged in
             */
            inline bool IsLoggedIn()
            {
                return loggedIn;
            }

        protected: // methods
            /**
             * @brief Session
             * @details This is protected so that it can never be constructed on the stack
             * The class assumes is is always managed by a shared_ptr. The shared_from_this call is
             * used to pass a reference down to DataObject classes
             * @param slot Slot to connect to this session
             * @param flags initialisation flags
             * @param callbackData Data passed to the callback
             * @param callback Provide a notification callback function to receive details about
             * cryptographic functions. see section 5.16 of the [PKCS11 documentation](http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html#_Toc416959752)
             */
            Session(std::shared_ptr<Slot> slot, ::CK_FLAGS flags = CKF_RW_SESSION, void* callbackData = nullptr, const ::CK_NOTIFY callback = nullptr);

        protected: // members
            /// The slot which this session is connected to
            std::shared_ptr<Slot> mySlot;
            /// handle from library
            ::CK_SESSION_HANDLE handle {};
            /// PKCS functions
            const ::CK_FUNCTION_LIST * const functions;
            /// whether we're logged in
            bool loggedIn = false;
        }; // class Session

        /**
         * @brief The AttributeList class
         */
        class KEYMANAGEMENT_EXPORT AttributeList
        {
        public:
            /**
             * @brief AttributeList
             * Construct an empty list
             */
            AttributeList() {}

            /**
             * @brief AttributeList
             * Construct a partially populated list
             * @param types Which types to pre-populate,
             * each will have a null value
             */
            inline AttributeList(const std::vector<::CK_ATTRIBUTE_TYPE>& types);

            /**
             * @brief Set
             * Create a slot for type
             * @param type The name of the attribute
             */
            void Set(::CK_ATTRIBUTE_TYPE type);

            /**
             * @brief Set
             * Create/set the value
             * @param type The name of the attribute
             * @param value Value to set
             */
            void Set(::CK_ATTRIBUTE_TYPE type, const std::string& value);

            /// @copydoc Set
            inline void Set(::CK_ATTRIBUTE_TYPE type, const char* value);

            /// @copydoc Set
            /// @param time Value to set
            void Set(::CK_ATTRIBUTE_TYPE type, std::chrono::system_clock::time_point time);

            /// @copydoc Set
            void Set(::CK_ATTRIBUTE_TYPE type, const PSK& value);

            /// @copydoc Set
            /// @tparam T Type of value to set
            /// @param value Value to set
            template<typename T, typename std::enable_if<std::is_integral<T> {}, int>::type = 0>
                    void Set(::CK_ATTRIBUTE_TYPE type, const T& value);

            /**
             * @brief Get
             * Get a value from the list
             * @param type Which element to get
             * @param output destination for the value
             * @return true if the value was found
             */
            bool Get(::CK_ATTRIBUTE_TYPE type, std::vector<uint8_t>& output);

            /// @copydoc Get
            bool Get(::CK_ATTRIBUTE_TYPE type, PSK& output);

            /// @copydoc Get
            bool Get(::CK_ATTRIBUTE_TYPE type, std::string& output);

            /// @copydoc Get
            /// @tparam T Type of value to get
            template<typename T, typename std::enable_if<std::is_integral<T> {}, int>::type = 0>
                    bool Get(::CK_ATTRIBUTE_TYPE type, T& output);

            /**
             * @brief GetAttributes
             * Get a pointer to all values
             * @return C pointer to value array
             */
            inline ::CK_ATTRIBUTE* GetAttributes() const;

            /**
             * @brief GetCount
             * @return The number of elements returned by GetAttributes
             */
            inline unsigned long GetCount() const;

            /**
             * @brief ReserveStorage
             * Create value storage for each type currently defined.
             * This allows the pointer returned by GetAttributes to be filled in.
             */
            void ReserveStorage();

        protected:
            /// internal storage type, compatible with C API calls
            using Value = std::vector<uint8_t>;
            /// additional info for each value
            struct MappedValue
            {
                /// The attribute structure this storage item relates to
                ::CK_ATTRIBUTE* attribute = nullptr;
                /// the storage for the data
                Value value;
            };
            /// dictionary of values to attribute types
            using Storage = std::map<::CK_ATTRIBUTE_TYPE, MappedValue>;
            /// a list of attributes for compatibility with C API
            using Attributes = std::vector<::CK_ATTRIBUTE>;
            /// allocated memory for values
            Storage valueStorage;
            /// holds attributes details and pointer to valueStorage
            Attributes attributes;


        }; // class AttributeList

        /**
         * @brief The DataObject class
         */
        class KEYMANAGEMENT_EXPORT DataObject
        {
        public:
            /**
             * @brief DataObject
             * Initialise an instance with no values, create it on the device with CreateObject()
             * @param session The session this object belongs to
             */
            DataObject(std::shared_ptr<Session> session);
            /**
             * @brief DataObject
             * Initialise from an existing object from the device
             * @param session The session this object belongs to
             * @param handle Handle for a preexisting object
             */
            DataObject(std::shared_ptr<Session> session, ::CK_OBJECT_HANDLE handle);

            /**
             * @brief CreateObject Create a new object on the device with the given values.
             * The handle will be valid if this is successful
             * @param values Initial values for the object
             * @return CK_OK on success
             */
            ::CK_RV CreateObject(const AttributeList& values);

            /**
             * @brief Remove the object from the device using the current handle
             * @return CK_OK on success
             */
            ::CK_RV DestroyObject();

            /**
             * @brief Request the attribute values from the device using the current handle
             * @param[in,out] value The attribute(s) to query. The results will be stored in the placeholders
             * @return CK_OK on success
             * @details
             * Example usage:
             * @code
             * AttributeList attrList;
             * attrList.Set(CKA_VALUE) // create an empty attribute to be populated
             * if(dataObject.GetAttributeValue(attrList)) == CK_OK)
             * {
             *    attrList.Get(CKA_VALUE, retrievedValue);
             * }
             * @endcode
             */
            ::CK_RV GetAttributeValue(AttributeList& value);

            /** @copybrief GetAttributeValue
             * @details shortcut version which puts the resulting single value into value
             * @param type Which attribute to get
             * @param[out] value Destination for the value
             * @tparam T the type for the output value
             * @return CK_OK on success
             */
            template<typename T>
            ::CK_RV GetAttributeValue(::CK_ATTRIBUTE_TYPE type, T& value);


            /**
             * @brief SetAttributeValue
             * Change a value on the device directly
             * @param value
             * @return CK_OK on success
             */
            ::CK_RV SetAttributeValue(const AttributeList& value);

            /**
             * @brief SetAttributeValue
             * Change/set an attribute value
             * @tparam T Type of value to set
             * @param type identifier for the value
             * @param value new value to set
             * @return CK_OK on success
             */
            template<typename T>
            ::CK_RV SetAttributeValue(::CK_ATTRIBUTE_TYPE type, const T& value);

            /**
             * @brief Handle
             * @return The data object handle
             */
            inline ::CK_OBJECT_HANDLE Handle() const;

            inline void SetHandle(::CK_OBJECT_HANDLE handle);
        protected:
            /// The session for this object
            std::shared_ptr<Session> mySession;
            /// object handle from the api
            ::CK_OBJECT_HANDLE handle = 0;
            /// PKCS functions
            const ::CK_FUNCTION_LIST * const functions;
        }; // class DataObject

        // ============ Public implementations ================
#if !defined(DOXYGEN)
        template<unsigned int N>
        std::string FromPKCSString(const unsigned char (& str)[N])
        {
            std::string result(str, str + N);
            rtrim(result);
            return result;
        } // FromPKCSString

        // Slot Public implementations

        std::shared_ptr<Module> Slot::GetModule()
        {
            return myModule;
        } // Slot::GetModule

        CK_SLOT_ID Slot::GetID() const
        {
            return id;
        } // Slot::GetID

        // session public implementations

        std::shared_ptr<Session> Session::Create(std::shared_ptr<Slot> slot, CK_FLAGS flags, void* callbackData, CK_NOTIFY callback)
        {
            return std::shared_ptr<Session>(new Session(slot, flags, callbackData, callback));
        } // Session::Create

        std::shared_ptr<Slot> Session::GetSlot()
        {
            return mySlot;
        } // Session::GetSlot

        CK_SESSION_HANDLE Session::GetSessionHandle() const
        {
            return handle;
        } // Session::GetSessionHandle

        // AttributeList implementations

        template<typename T, typename std::enable_if<std::is_integral<T> {}, int>::type>
        void AttributeList::Set(::CK_ATTRIBUTE_TYPE type, const T& value)
        {
            __try
            {
                // prepare the storage
                Set(type);
                // map the data to a byte array
                auto* inputOverlay = reinterpret_cast<const uint8_t*>(&value);
                // allocate/find the storage slot
                auto& currentStorage = valueStorage[type];
                // copy the data into the storage
                currentStorage.value.assign(inputOverlay, inputOverlay + sizeof(value));
                // setup the attribute fields, this will exists because of calling Set(type) above
                currentStorage.attribute->type = type;
                currentStorage.attribute->pValue = currentStorage.value.data();
                currentStorage.attribute->ulValueLen = currentStorage.value.size();
            } // try
            CATCHLOGERROR
        } // AttributeList::Set

        template<typename T, typename std::enable_if<std::is_integral<T> {}, int>::type>
        bool AttributeList::Get(::CK_ATTRIBUTE_TYPE type, T& output)
        {
            bool result = false;
            __try
            {
                auto it = valueStorage.find(type);
                if(it != valueStorage.end() && it->second.attribute)
                {
                    output = *reinterpret_cast<T*>(it->second.attribute->pValue);
                    result = true;
                }
                return result;
            } // try
            CATCHLOGERROR
            return result;
        } // AttributeList::Get

        AttributeList::AttributeList(const std::vector<CK_ATTRIBUTE_TYPE>& types)
        {
            for(auto type : types)
            {
                Set(type);
            }
        } // AttributeList::AttributeList

        void AttributeList::Set(CK_ATTRIBUTE_TYPE type, const char* value)
        {
            Set(type, std::string(value));
        } // AttributeList::Set

        CK_ATTRIBUTE* AttributeList::GetAttributes() const
        {
            return const_cast<::CK_ATTRIBUTE*>(attributes.data());
        } // AttributeList::GetAttributes

        unsigned long AttributeList::GetCount() const
        {
            return attributes.size();
        } // AttributeList::GetCount

        // DataObject implementations

        template<typename T>
        ::CK_RV DataObject::GetAttributeValue(::CK_ATTRIBUTE_TYPE type, T& value)
        {
            AttributeList params({type});
            CK_RV result = GetAttributeValue(params);
            if(result == CKR_OK)
            {
                params.Get(type, value);
            }
            return result;
        } // DataObject::GetAttributeValue

        CK_OBJECT_HANDLE DataObject::Handle() const
        {
            return handle;
        } // DataObject::Handle

        void DataObject::SetHandle(CK_OBJECT_HANDLE newHandle)
        {
            handle = newHandle;
        } // DataObject::SetHandle

        template<typename T>
        ::CK_RV DataObject::SetAttributeValue(::CK_ATTRIBUTE_TYPE type, const T& value)
        {
            AttributeList params;
            params.Set(type, value);
            return SetAttributeValue(params);
        } // DataObject::SetAttributeValue
#endif
    } // namespace p11
} // namespace cqp



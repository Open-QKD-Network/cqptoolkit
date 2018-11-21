/*!
* @file
* @brief CommandArgs
*
* @copyright Copyright (C) University of Bristol 2018
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 8/3/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <unordered_map>

namespace cqp
{
    class URI;

    /**
     * @brief The CommandArgs class
     * Provides convienient access to an applicatons command line arguments
     */
    class CQPTOOLKIT_EXPORT CommandArgs
    {
    public:
        /**
         * @brief The Option class
         * Defines a command line option any associated value
         */
        class CQPTOOLKIT_EXPORT Option
        {
        public:
            /// Callback function used to process incomming values
            /// used with the Callback method
            using Func = std::function<void(const Option& option)>;

            /**
             * @brief Callback
             * Assign a function to call when the option is found on the command line
             * @param cb Function to call
             * @return This option object
             */
            Option& Callback(Func cb)
            {
                callback = cb;
                return *this;
            }

            /**
             * @brief HasArgument
             * specifies that the option will be followed by a parameter
             * @return this object
             */
            Option& HasArgument()
            {
                hasArgument = true;
                return *this;
            }

            /**
             * @brief Required
             *  Specify that the option must be provided on the command line
             * @return this object
             */
            Option& Required()
            {
                required = true;
                return *this;
            }

            /**
             * @brief Bind
             * Bind to a parameter with the same name as the long name.
             * This implies that the argument requires a parameter
             * @return This option
             */
            Option& Bind()
            {
                hasArgument = true;
                boundTo = longName;
                return *this;
            }

            /// @copydoc Bind
            /// @param key The name of the property to store the value in
            Option& Bind(const std::string& key)
            {
                hasArgument = true;
                boundTo = key;
                return *this;
            }

            /**
             * @brief ToString
             * @return The option and it's value as a "key = value" pair
             */
            std::string ToString() const
            {
                return longName + " = " + value;
            }

            /// The full length name of the option used with --
            std::string longName;
            /// The single character option name use with -
            std::string shortName;
            /// User readable description shown when printing the help message
            std::string description;
            /// Any value which has been parsed
            /// This is only parsed if HasArgument or Bind has been called
            std::string value;
            /// If speciifed, a parsed value will be stored in the application parameter dictionary
            /// Implies HasArgument
            std::string boundTo;
            /// Should the next value be interpreted as a value for this option
            bool hasArgument = false;
            /// function to call when the option is found on the command line
            Func callback;
            /// set to true if the option was specified on the command line
            bool set = false;
            /// If it is an error for this option to not be specified on the command line
            bool required = false;
        };

        /// constructor
        CommandArgs() = default;

        /**
         * @brief AddOption
         * Create an option to parse.
         * long (--name) and optionally short (-n) options can be created
         * @param longName The string for long (--) options, must be unique within a collection of options
         * @param shortName The single character for the short option (-). If this is blank, there is no
         * short option
         * @param description Description to display in the help message
         * @return The newly created option
         */
        Option& AddOption(const std::string& longName, const std::string& shortName, const std::string& description);

        /**
         * @brief Parse
         * Process the list of strings, assigning values to the defined options
         * @param args
         * @return true on successfull parsing
         */
        bool Parse(const std::vector<std::string>& args);
        /**
         * @brief StopOptionsProcessing
         * Breaks out of parsing, can be called from within a callback
         */
        void StopOptionsProcessing();

        /**
         * @brief PrintHelp
         * Format the defined options as a help message describing what can be passed to the application
         * @param output The destination for the message
         * @param header Sent to the output first as a banner
         * @param footer Sent to the output last
         */
        void PrintHelp(std::ostream& output, const std::string& header = "", const std::string& footer = "");

        /**
         * @brief PropertiesToString
         * Only relevent for options which have been bound with Bind()
         * @return All the values collected as properties formated as "key = value"
         */
        std::string PropertiesToString() const;
        /**
         * @brief LoadProperties
         * Read properties from a file created with PropertiesToString()
         * @param filename
         * @return true on success
         */
        bool LoadProperties(const std::string& filename);

        /**
         * @brief HasProp
         * @param key property name specified for Bind()
         * @return true if the option was specified on the command
         */
        bool HasProp(const std::string& key) const;

        /**
         * @brief GetProp
         * Get the value associated with the option name
         * @param key The long name of the option
         * @param out The value provided with the option
         * @return true if out was changed (the command was specified)
         */
        bool GetProp(const std::string& key, bool& out) const;
        /// @copydoc GetProp
        bool GetProp(const std::string& key, size_t& out) const;
        /// @copydoc GetProp
        bool GetProp(const std::string& key, int& out) const;
        /// @copydoc GetProp
        bool GetProp(const std::string& key, double& out) const;
        /// @copydoc GetProp
        bool GetProp(const std::string& key, uint16_t& out) const;
        /// @copydoc GetProp
        bool GetProp(const std::string& key, std::string& out) const;
        /// @copydoc GetProp
        bool GetProp(const std::string& key, URI& out) const;

        /**
         * @brief GetStringProp
         * Get a property as a string
         * @param key The id of the property
         * @return The value of key or an empty string
         */
        std::string GetStringProp(const std::string& key) const;
        /**
         * @brief IsSet
         * @param longName parameter name
         * @return true of the option has been specified on the command line
         */
        bool IsSet(const std::string& longName) const;

        /**
         * @brief operator []
         * @param key long name of the option
         * @return The option if it exists
         */
        Option* operator[](const std::string& key) const noexcept
        {
            Option* result = nullptr;
            auto it = longOptions.find(key);
            if(it != longOptions.end())
            {
                result = it->second;
            }
            return result;
        }

        /**
         * @brief GetCommandName
         * @return The name of the application
         */
        std::string GetCommandName() const
        {
            return cmdName;
        }
    protected:
        /// should parameter parsing end
        bool stopProcessing = false;
        /// defined options
        std::vector<Option*> options;
        /// dictionary based on short name
        std::map<std::string, Option*> shortOptions;
        /// dictionary based on long name
        std::map<std::string, Option*> longOptions;

        /// properties set from options which have been bound with Bind()
        std::unordered_map<std::string, std::string> properties;
        /// The name of the command from the system
        /// this does not nessacarilly equal the compiled application name
        std::string cmdName;
    };

} // namespace cqp



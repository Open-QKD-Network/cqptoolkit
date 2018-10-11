/*!
* @file
* @brief CQP Toolkit - Interface example
*
* @copyright Copyright (C) University of Bristol 2016
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 18 April 2016
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "Interfaces.h"
#include "CQPToolkit/cqptoolkit_export.h"
/// @include Interfaces.h
namespace cqp
{
    /// Interfaces in C++ are abstract classes, we add certain keywords to allow the compiler to perform some checks
    /// Interfaces must always have the prefix I
    /// the CQPTOOLKIT_EXPORT macro evaluates to different things depending on the build being performed, on windows
    ///   it means that the class can be seen by those using the dll, on other systems it has no effect.
    class CQPTOOLKIT_EXPORT IThisIsAnInterface
    {
        // Remember to make the functions public
    public:
        /// All interface operations are pure (=0) virtual, they cannot be called without an implementing object to fulfill them.
        virtual void Foo() = 0;
    };

    /// This class implements the IThisIsAnInterface by inheriting from it
    /// The inheritence is public so that others know we provide the Foo function
    /// It has also been made virtual - so there is no storage associated with the IThisIsAnInterface.
    ///   This is logical for an interface as an object of IThisIsAnInterface should never exist.
    ///   This also solves some issues with the (diamond inheritence problem)[http://www.cprogramming.com/tutorial/virtual_inheritance.html]
    /// Note that other than interfaces, non-virtual multiple inheritence should be avoided at all costs.
    class CQPTOOLKIT_EXPORT ImplementsInterface : public virtual IThisIsAnInterface
    {
    public:
        /// The non-pure virtual function we are implementing from IThisIsAnInterface;
        virtual void Foo() override;
    };

    /// This class doesn't inherit from anything but is exported from the dll
    class CQPTOOLKIT_EXPORT UsesInterface
    {
    public:
        /// The virtual keyword here allows sub classes to override the funtion.
        virtual void Go(IThisIsAnInterface* iface);
    };

    /// An example of normal class inheritence
    /// No virtual keyword is used as the UsesInterface is a real class
    class CQPTOOLKIT_EXPORT Child : public UsesInterface
    {
    public:
        /// The virtual keyword here is not needed but included for clarity
        /// The override keyword should always be used to allow the compiler to verify that
        /// the signitures match.
        virtual void Go(IThisIsAnInterface* iface) override;

        /// A normal function.
        virtual void DoSomethingElse();
    };

    // There is no implementation for IThisIsAnInterface

    /// The non-pure virtual function we are implementing from IThisIsAnInterface;
    void ImplementsInterface::Foo()
    {
        // Do something amazing
    }

    /// The virtual keyword here allows sub classes to override the funtion.
    void UsesInterface::Go(IThisIsAnInterface* iface)
    {
        // Never assume that a pointer is valid
        if(iface != nullptr)
        {
            // Use the interface
            iface->Foo();
        }
    }

    /// The override keyword should always be used to allow the compiler to verify that
    /// the signitures match.
    void Child::Go(IThisIsAnInterface* iface)
    {
        // Do the normal behaviour from the parent
        UsesInterface::Go(iface);
        // Do something else...
        DoSomethingElse();
    }

    /// A normal function.
    void Child::DoSomethingElse()
    {
        // ...
    }
}

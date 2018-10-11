### @file
### @brief CQP Toolkit - Tools
### 
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 04 April 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 

project(UI)

# Perform some standard setup steps like detecting the platform.
include(CommonSetup)

# projects below here will be put into this package

SET_COMPONENT_NAME("UI" CQP_INSTALL_COMPONENT)

ADD_ALL_SUBDIRS()

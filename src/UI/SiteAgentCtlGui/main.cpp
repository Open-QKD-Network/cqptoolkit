/*!
* @file
* @brief
*
* @copyright Copyright (C) University of Bristol 2017
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 13/02/2018
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#include "SiteAgentCtlGui.h"
#include <QApplication>

/**
 * @brief main
 * Launches KeyViewer::MainWindow
 * @param argc
 * @param argv
 * @return return value of qkdtunnel::MainWindow
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SiteAgentCtlGui w;
    w.show();

    return a.exec();
}

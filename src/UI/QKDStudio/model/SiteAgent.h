/*!
* @file
* @brief SiteAgent
*
* @copyright Copyright (C) University of Bristol 2019
*    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
*    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*    See LICENSE file for details.
* @date 29/3/2019
* @author Richard Collins <richard.collins@bristol.ac.uk>
*/
#pragma once
#include <nodes/NodeDataModel>
#include "QKDInterfaces/Site.pb.h"

class QString;
class QScrollArea;
class QToolButton;

namespace grpc
{
    class Channel;
    class ChannelCredentials;
}

namespace cqp
{
    class SiteEditor;

    namespace remote
    {
        class Site;

    }

    namespace model
    {
        class SiteAgentData;

        class SiteAgent : public QtNodes::NodeDataModel
        {
            Q_OBJECT

        public:

            SiteAgent();

            ~SiteAgent() override;

            QString caption() const override;
            QString name() const override
            {
                return QStringLiteral("siteagent");
            }
            unsigned int nPorts(QtNodes::PortType portType) const override;

            QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

            QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

            void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

            std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

            QWidget* embeddedWidget() override;

            void SetDetails(const remote::Site& details);
            void SetAddress(const std::string& address);
        protected:
            std::shared_ptr<SiteAgentData> siteData;
            QScrollArea* topWidget;
            std::shared_ptr<grpc::Channel> channel;
            std::shared_ptr<grpc::ChannelCredentials> creds;

            std::unique_ptr<SiteEditor> siteEditor;
            QToolButton* disconnect;


            remote::Site details;
            remote::SiteAgentConfig config;

        protected slots:
            void OnConnect();
            void OnDisconnect();
            void OnEdit();
            void OnEditFinished(int result);
        };

    } // namespace model
} // namespace cqp



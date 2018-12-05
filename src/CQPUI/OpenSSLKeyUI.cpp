#include "OpenSSLKeyUI.h"
#include "ui_OpenSSLKeyUI.h"
#include "KeyManagement/KeyStores/HSMStore.h"
#include "KeyManagement/KeyStores/YubiHSM.h"
#include "Algorithms/Logging/ConsoleLogger.h"
#include "CQPUI/HSMPinDialog.h"
#include "Algorithms/Util/Strings.h"
#include <QInputDialog>
#include <QPushButton>
#include "Algorithms/Util/Process.h"
#if defined(__unix__)
    #include <unistd.h>
#endif

using cqp::keygen::HSMStore;
using cqp::keygen::YubiHSM;
using cqp::ui::HSMPinDialog;

namespace cqp {
    namespace ui {

        static std::vector<std::string> knownModules = {"libsofthsm2.so", "yubihsm_pkcs11.so"};

        OpenSSLKeyUI::OpenSSLKeyUI(QWidget *parent) :
            QDialog(parent),
            ui(new Ui::OpenSSLKeyUI)
        {
            ui->setupUi(this);
            ui->foundModules->setSizeAdjustPolicy(QTreeWidget::SizeAdjustPolicy::AdjustToContents);
        }

        OpenSSLKeyUI::~OpenSSLKeyUI()
        {
            delete ui;
        }

        void OpenSSLKeyUI::FindTokens()
        {
            ui->foundModules->clear();
            // build a list of HSMs
            auto foundTokens = cqp::keygen::HSMStore::FindTokens(knownModules);
            for(const auto& token : foundTokens)
            {
                std::map<std::string, std::string> dictionary;
                token.ToDictionary(dictionary);
                QTreeWidgetItem *newItem = new QTreeWidgetItem(ui->foundModules);
                newItem->setText(0, QString::fromStdString(dictionary["token"]));
                newItem->setText(1, QString::fromStdString(dictionary["serial"]));
                newItem->setText(2, QString::fromStdString(dictionary["module-name"]));
                newItem->setText(3, QString::fromStdString(token));
            }

            if(!foundTokens.empty())
            {
                ui->foundModules->resizeColumnToContents(0);
                ui->foundModules->topLevelItem(0)->setSelected(true);
                ui->hsmUrl->setText(ui->foundModules->topLevelItem(0)->text(3));
                ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
            } else {
                ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
            }
        }

        int OpenSSLKeyUI::exec()
        {
            FindTokens();
            return QDialog::exec();
        }

        bool OpenSSLKeyUI::RememberChoice() const
        {
            return ui->remember->checkState() == Qt::CheckState::Checked;
        }

        std::string OpenSSLKeyUI::GetStoreUrl() const
        {
            return ui->hsmUrl->text().toStdString();
        }

        static std::shared_ptr<HSMPinDialog> pinDialog = nullptr;
        static std::shared_ptr<HSMStore> activeStore = nullptr;

        void OpenSSLKeyUI::Register(SSL_CTX* ctx)
        {
            if(!pinDialog)
            {
                pinDialog.reset(new HSMPinDialog(nullptr));
            }
            //myUserDataIndex = SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
            //SSL_CTX_set_ex_data(ctx, myUserDataIndex, this);
            SSL_CTX_set_psk_client_callback(ctx, &OpenSSLKeyUI::ClientCallback);
        }

        unsigned int OpenSSLKeyUI::ClientCallback(SSL*, const char* hint, char* identity,
                                                  unsigned int max_identity_len, unsigned char* psk,
                                                  unsigned int max_psk_len)
        {
            unsigned int result = 0;
            ConsoleLogger::Enable();
            DefaultLogger().SetOutputLevel(LogLevel::Trace);
            LOGTRACE("Started");

            //const auto ctx = SSL_get_SSL_CTX(ssl);
            //OpenSSLKeyUI* self = static_cast<OpenSSLKeyUI*>(SSL_CTX_get_ex_data(ctx, myUserDataIndex));

            std::shared_ptr<HSMStore> myStore = activeStore;
            if(!myStore)
            {
                using namespace std;
                std::string storeUrl;
                bool rememberChoice = false;
                // hack for running inside an application with it's own gui,
                // qt doesn't play nice with others
                if(QApplication::instance() == nullptr)
                {
                    LOGTRACE("Running Chooser program");
                    int stdOut = 0;
                    std::vector<std::string> args;
                    Process chooser;
                    std::vector<std::string> lines;

                    chooser.Start("ChooseHSM", args, nullptr, &stdOut);
                    while(chooser.Running())
                    {
                        string line;
                        char lastChar = {};

                        do
                        {
                            if(::read(stdOut, &lastChar, 1) > 0 && lastChar != '\n')
                            {
                                line.append(1, lastChar);
                            }
                        }
                        while (lastChar != 0 && lastChar != '\n');
                        lines.push_back(line);
                    }

                    if(chooser.WaitForExit() == 0)
                    {
                        if(!lines.empty())
                        {
                            storeUrl = lines[0];
                        }
                        if(lines.size() >= 2 && lines[1] == "1")
                        {
                            rememberChoice = true;
                        }
                    }
                } else {

                    LOGTRACE("Creating UI to choose module");
                    // ask the user for the module to use
                    OpenSSLKeyUI ui;
                    if(ui.exec() == DialogCode::Accepted)
                    {
                        LOGTRACE("HSM selection made");
                        // create the class to manage the module
                        storeUrl = ui.GetStoreUrl();

                        rememberChoice = ui.RememberChoice();

                    }
                }

                if(QString::fromStdString(storeUrl).contains("yubi"))
                {
                    LOGTRACE("Creating Yubi HSM manager");
                    myStore.reset(new YubiHSM(storeUrl, pinDialog.get()));
                } else {
                    LOGTRACE("Creating PKCS11 HSM manager");
                    myStore.reset(new HSMStore(storeUrl, pinDialog.get()));
                }

                if(rememberChoice)
                {
                    LOGTRACE("Saving manager for next time");
                    activeStore = myStore;
                }
            }

            if(myStore)
            {
                uint64_t keyId = 0;

                PSK keyValue;
                if(myStore->FindKey(hint, keyId, keyValue) && keyValue.size() <= max_psk_len)
                {
                    std::copy(keyValue.begin(), keyValue.end(), psk);
                    std::string keyIdString = "pkcs:object=" + myStore->GetSource() + "?id=" + std::to_string(keyId);
                    keyIdString.copy(identity, max_identity_len);
                    result = keyValue.size();
                } // if key found

            }

            LOGTRACE("Leaving");
            return result;
        }

        void OpenSSLKeyUI::on_addModule_clicked()
        {
            const auto newMod = QInputDialog::getText(this, "Add Module","Module to add:").toStdString();
            if(!newMod.empty())
            {
                knownModules.push_back(newMod);
                FindTokens();
            }
        }

        void OpenSSLKeyUI::on_foundModules_itemClicked(QTreeWidgetItem *item, int)
        {
            if(item)
            {
                ui->hsmUrl->setText(item->text(3));
                ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
            }
        }

        void OpenSSLKeyUI::on_reloadModules_clicked()
        {
            FindTokens();
        }

    } // namespace ui
} // namespace cqp


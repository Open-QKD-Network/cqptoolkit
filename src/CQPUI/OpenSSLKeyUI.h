#pragma once
#include <QDialog>
#include <QTreeWidgetItem>
#include "CQPUI/cqpui_export.h"

#include "openssl/ssl.h"

namespace cqp {
    namespace ui {

        namespace Ui {
            class OpenSSLKeyUI;
        }

        class CQPUI_EXPORT OpenSSLKeyUI : public QDialog
        {
            Q_OBJECT

        public:
            explicit OpenSSLKeyUI(QWidget *parent = nullptr);
            virtual ~OpenSSLKeyUI() override;

            void FindTokens();
            bool RememberChoice() const;
            std::string GetStoreUrl() const;
            static void Register(SSL_CTX *ctx);

        protected:

            /**
             * @brief OpenSSLHandler_ClientCallback
             * Supplies OpenSSL with the correct PSK on request. For TLS <= 1.2. Attach with
             * ``SSL_CTX_set_psk_client_callback`` or ``SSL_set_psk_client_callback``
             * @see https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_set_psk_client_callback.html
             * @param ssl
             * @param identity
             * @param psk
             * @param max_psk_len
             * @return 0 on failure, otherwise the length of the PSK
             */
            static unsigned int ClientCallback(SSL *ssl, const char *hint,
                    char* identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len);

        private slots:
            void on_addModule_clicked();

            void on_foundModules_itemClicked(QTreeWidgetItem *item, int column);

            void on_reloadModules_clicked();

        private:
            Ui::OpenSSLKeyUI *ui;
            static int myUserDataIndex;

            // QDialog interface
        public slots:
            int exec() override;
        };


    } // namespace ui
} // namespace cqp


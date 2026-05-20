#include "ProxyCredentialsHandler.h"
#include "Settings.h"
#include <QInputDialog>
#include "Helper.h"

ProxyCredentialsHandler::ProxyCredentialsHandler()
	: QObject()
	, credentials_()
	, enable_credentials_dialog_(false)
{
}

void ProxyCredentialsHandler::proxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator)
{
	QString host = proxy.hostName().toLower().trimmed();

	if (credentials_.contains(host))
	{
		//qDebug() << "ProxyCredentialsHandler::proxyAuthenticationRequired - CACHED";
		const QPair<QString, QString>& credentials = credentials_[host];
		authenticator->setUser(credentials.first);
		authenticator->setPassword(credentials.second);
	}
	else
	{
		QString user = Settings::string("proxy_user", true);
		QString password = Settings::string("proxy_password", true);
		if (!user.isEmpty() && !password.isEmpty())
		{
			//qDebug() << "ProxyCredentialsHandler::proxyAuthenticationRequired - FROM SETTINGS" << user << password;
			authenticator->setUser(user);
			authenticator->setPassword(password);
		}
		else if (enable_credentials_dialog_)
		{
			//qDebug() << "ProxyCredentialsHandler::proxyAuthenticationRequired - ASKING USER";
			QString user = QInputDialog::getText(nullptr, "Proxy user required", "Proxy user", QLineEdit::Normal, Helper::userName());
			QString password = QInputDialog::getText(nullptr, "Proxy password required", "Proxy password", QLineEdit::Password);
			authenticator->setUser(user);
			authenticator->setPassword(password);
		}
	}
}

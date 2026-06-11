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
	QString host_and_port = proxy.hostName().toLower().trimmed() + ":" + QString::number(proxy.port());
	bool debug = false;
	if (debug) qDebug() << "Getting credentials for proxy:" << host_and_port;

	//not in cache > try to determine user/password
	if (!credentials_.contains(host_and_port))
	{
		QString user = Settings::string("proxy_user", true);
		QString password = Settings::string("proxy_password", true);
		if (!user.isEmpty() && !password.isEmpty())
		{
			if (debug) qDebug() << "  filling cache from settings:" << user << password;
			credentials_[host_and_port] = qMakePair(user, password);
		}
		else if (enable_credentials_dialog_)
		{
			if (debug) qDebug() << "ProxyCredentialsHandler::proxyAuthenticationRequired - ASKING USER";
			QString user = QInputDialog::getText(nullptr, "Proxy user required", "Proxy user", QLineEdit::Normal, Helper::userName());
			QString password = QInputDialog::getText(nullptr, "Proxy password required", "Proxy password", QLineEdit::Password);
			if (!user.isEmpty() && !password.isEmpty())
			{
				if (debug) qDebug() << "  filling cache from user input:" << user << password;
				credentials_[host_and_port] = qMakePair(user, password);
			}
		}
	}

	if (credentials_.contains(host_and_port))
	{
		const QPair<QString, QString>& credentials = credentials_[host_and_port];
		if (debug) qDebug() << "  using cached credentials:" << credentials.first << credentials.second;
		authenticator->setUser(credentials.first);
		authenticator->setPassword(credentials.second);
	}
	else
	{
		if (debug) qDebug() << "  no credentials found!";
	}
}

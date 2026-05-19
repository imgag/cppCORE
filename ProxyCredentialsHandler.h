#ifndef PROXYCREDENTIALSHANDLER_H
#define PROXYCREDENTIALSHANDLER_H

#include <QObject>
#include <QNetworkProxy>
#include <QAuthenticator>

//Singleton that determines/caches proxy credentials
class ProxyCredentialsHandler
	: public QObject
{
	Q_OBJECT

public:
	static ProxyCredentialsHandler& instance()
	{
		static ProxyCredentialsHandler instance;
		return instance;
	}

	//Sets credentialy manually (not necessary in most cases)
	void setCredentials(QString host, QString user, QString password)
	{
		host = host.toLower().trimmed();
		credentials_[host] = qMakePair(user, password);
	}

	//Clears all cached credentials
	void clearCredentials()
	{
		credentials_.clear();
	}

	//Enable user dialog
	void enableUserDialog(bool enabled) { enable_credentials_dialog_ = enabled; }

public slots:
	void proxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator);

private:
	ProxyCredentialsHandler();

	Q_DISABLE_COPY_MOVE(ProxyCredentialsHandler)

	QHash<QString, QPair<QString, QString>> credentials_;
	bool enable_credentials_dialog_;
};

#endif // PROXYCREDENTIALSHANDLER_H

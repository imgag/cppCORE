#include "BasicServer.h"
#include "Log.h"
#include <QJsonObject>
#include <QJsonDocument>

BasicServer::BasicServer(quint16 port, QObject* parent)
	: QObject(parent)
{
	setupDefaultRoutes();
	if (tcp_server.listen(QHostAddress::Any, port)) http_server.bind(&tcp_server);
}

bool BasicServer::isRunning() const
{
	return tcp_server.isListening();
}

void BasicServer::setupDefaultRoutes()
{
	http_server.route("/version", []()
	{
		QJsonObject obj {{"name", "HTTP server"}, {"version", "1.0.0"}};
		return QHttpServerResponse("application/json", QJsonDocument(obj).toJson(QJsonDocument::Compact));
	});

	#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	http_server.setMissingHandler(this, [] (const QHttpServerRequest &req, QHttpServerResponder & resp)
	{
		Log::info("Failed to access '" + req.url().path() + "', the path does not exist");
		resp.sendResponse(QHttpServerResponse("text/plain", "You are trying to open the path '" + req.url().path().toLatin1() + "' which does not exist", QHttpServerResponder::StatusCode::NotFound));
	});
	#endif
}
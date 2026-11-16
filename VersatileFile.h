#ifndef VERSATILEFILE_H
#define VERSATILEFILE_H

#include "cppCORE_global.h"
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QSharedPointer>
#include <QByteArray>
#include <QObject>
#include <QTimer>
#include <zlib.h> //TODO Marc try libdeflate instead of zlib. Also for BAM/CRAM via htslib?
#include "GzipStreamDecompressor.h"
#include "Helper.h"

//File class that can handle plain text files, gzipped text files and URLs.
//If you need QString output with proper handling of the encoding, use VersatileTextStream.
class CPPCORESHARED_EXPORT VersatileFile
    : public QObject
{
    Q_OBJECT

public:
	//Constructor for local/remote plain/gzipped files.
	VersatileFile(QString file_name, bool stdin_if_empty = false);
	//Destructor. Calls close() to frees all resources.
	~VersatileFile();

	//Open file. Returns false if file could not be opened. Open mode is only used in LOCAL mode.
	//Note: Use QIODevice::Text in addition to the read/write mode for text files to replace '\r\n' by '\n' when reading.
	bool open(QIODevice::OpenMode mode = QFile::ReadOnly, bool throw_on_error = true);
	//Returns the proxy used for the current URL.
	QNetworkProxy proxy() const;
	//Disables automatic proxy determination and by that uses no proxy.
	void disableProxy();

	//Returns the open mode (for local files).
	QIODevice::OpenMode openMode() const;
	//File mode.
	enum Mode { LOCAL, LOCAL_GZ, URL, URL_GZ};
	//Returns the file mode.
	Mode mode() const
	{
		return mode_;
	}

	//set output buffer size, i.e. the maximum line size. Call before opening the file!
	void setGzBufferSize(int bytes);
	//set internal buffer size, i.e. the buffer that is internally used by zlib. Increasing this buffer should improve reading speed. Call before opening the file!
	void setGzBufferSizeInternal(int bytes);

	bool isOpen() const { return is_open_; }
	bool isReadable() const;

	QByteArray read(qint64 maxlen = 0);
	QByteArray readAll();
    QByteArray readLine(bool trim_line_endings = false);

	bool atEnd() const;
	bool exists();
	void close();

	qint64 pos() const;
	bool seek(qint64 pos);
	qint64 size();

	QString fileName() const;

	//checks if a file is GZ or BGZ.
	bool isGzipped();

private:
    QNetworkAccessManager net_mgr_;
	QString file_name_;
	FILE* file_stream_pointer_;
	Mode mode_ = LOCAL;
	bool is_open_;

	//members for LOCAL mode
	QSharedPointer<QFile> local_source_;

	//members for LOCAL_GZ mode
	int gz_buffer_size_ = 1048576; //1MB buffer
	int gz_buffer_size_internal_ = 16*1048576; //16MB buffer
	char* gz_buffer_ = nullptr;
	gzFile gz_stream_ = nullptr;

    GzipStreamDecompressor decompressor_;

    //members for URL mode
    QByteArray buffer_;
    qint64 cursor_position_ = 0; // position in a VersatileFile, as if we are reading QFile
    qint64 remote_position_ = 0; // where we are in the remote file while we read and save its content into a buffer)
    qint64 buffer_read_pos_ = 0; // position within the read buffer
    static constexpr qint64 chunkSize() { return 200 * 1024 * 1024; } // 200Mb

    //members for GZ_URL mode
    qint64 max_compressed_chunk_size_ = qint64(50)*1024*1024; // 50MB chunks will be sent for decompression
    qint64 uncompressed_size_limit_ = qint64(5)*1024*1024*1024; // 5GB limit to avoid bad allocation (when reading large portions of data at once)

    //members for all modes
    qint64 file_size_ = -1;
    bool file_exists_ = false;

    //members for remote decompression
    bool remote_gz_finished_ = false;
    QByteArray decompressed_buffer_;
    qint64 decompressed_buffer_pos_ = 0;

    //gets a chunk from the remote file
    QByteArray httpRangeRequest(qint64 start, qint64 end);

	//sends a GET or HEAD request and waits until it is finished.
	//'operation' must be QNetworkAccessManager::GetOperation or QNetworkAccessManager::HeadOperation,
	//retryable network errors are retried up to 5 times with an increasing delay,
	//on success, calls 'on_success' with the reply and returns its result.
	template <typename OnSuccess>
	auto runRequest(QNetworkAccessManager::Operation operation, const QNetworkRequest& request, const QString& action, OnSuccess&& on_success)
	{
		const int max_retry_attempts = 5;
		const long retry_delay_ms = 1000;

		QString last_error;
		for (int attempt=1; attempt<=max_retry_attempts; ++attempt)
		{
			QScopedPointer<QNetworkReply> reply(operation==QNetworkAccessManager::HeadOperation ? net_mgr_.head(request) : net_mgr_.get(request));

			if (!reply->isFinished())
			{
				QEventLoop loop;
				QObject::connect(reply.data(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
				loop.exec();
			}

			last_error = reply->errorString();

			if (reply->error()!=QNetworkReply::NoError)
			{
				if (attempt==max_retry_attempts)
				{
					if (!Helper::isTransportError(reply->error()))
					{
						int status_code = 0;
						QByteArray body = reply.data()->readAll();
						QMap<QByteArray, QByteArray> headers;
						QVariant status_attribute = reply.data()->attribute(QNetworkRequest::HttpStatusCodeAttribute);
						if (status_attribute.isValid()) status_code = status_attribute.toInt();
						const auto pairs = reply.data()->rawHeaderPairs();
						for (const auto &h : pairs)
						{
							headers.insert(h.first, h.second);
						}

						THROW_HTTP(HttpException, "HTTP Error: Could not " + action + " " + file_name_ + ": " + last_error + "\nReply: " + body, status_code, headers, body);
					}
					else
					{
						THROW(NetworkException, "Network Error: " + last_error);
					}
				}

				//wait between attempts
				QEventLoop wait_loop;
				QTimer::singleShot(retry_delay_ms * attempt, &wait_loop, &QEventLoop::quit);
				wait_loop.exec();
				continue;
			}

			return on_success(*reply);
		}

		THROW(FileAccessException, "Could not " + action + " " + file_name_ + ": " + last_error);
	}

};


#endif // VERSATILEFILE_H

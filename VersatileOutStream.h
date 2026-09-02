#ifndef VERSATILEOUTSTREAM_H
#define VERSATILEOUTSTREAM_H

#include "cppCORE_global.h"
#include <QFile>
#include <QIODevice>
#include <zlib.h>

///Streaming output device for plain and gzip-compressed files.
class CPPCORESHARED_EXPORT VersatileOutStream
	: public QIODevice
{
public:
	///Compression level 10 selects plain output; levels 0-9 select gzip output.
	static constexpr int NO_COMPRESSION = 10;

	VersatileOutStream(QString filename, bool stdout_if_empty=false, int compression_level=NO_COMPRESSION, int compression_strategy=Z_DEFAULT_STRATEGY);
	~VersatileOutStream() override;

	///Flushes and closes the output. Throws FileAccessException on failure.
	void close() override;

	QString filename() const
	{
		return filename_;
	}

	bool isCompressed() const
	{
		return compression_level_!=NO_COMPRESSION;
	}

protected:
	qint64 readData(char* data, qint64 max_size) override;
	qint64 writeData(const char* data, qint64 size) override;

private:
	void closeInternal(bool throw_on_error);

	QString filename_;
	int compression_level_;
	QFile file_;
	gzFile gz_file_ = nullptr;

	VersatileOutStream(const VersatileOutStream&) = delete;
	VersatileOutStream& operator=(const VersatileOutStream&) = delete;
};

#endif // VERSATILEOUTSTREAM_H

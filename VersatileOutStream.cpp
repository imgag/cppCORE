#include "VersatileOutStream.h"
#include "Exceptions.h"
#include <limits>

VersatileOutStream::VersatileOutStream(QString filename, bool stdout_if_empty, int compression_level, int compression_strategy)
	: filename_(filename)
	, compression_level_(compression_level)
	, file_(filename)
{
	//check input
	if (compression_level_<0 || compression_level_>NO_COMPRESSION) THROW(ArgumentException, "Invalid compression level '" + QString::number(compression_level_) + "' given for output file '" + filename_ + "'!");
	if (compression_strategy<0 || compression_strategy>4) THROW(ArgumentException, "Invalid gzip compression strategy '" + QString::number(compression_strategy) + "' given for output file '" + filename_ + "'!");
	if (isCompressed() && filename_.isEmpty()) THROW(ArgumentException, "Cannot write gzip-compressed output to stdout!");
	if (isCompressed() && !filename.toLower().endsWith(".gz")) THROW(ArgumentException, "Compression requested, but filename does not end with '.gz'!");
	if (!isCompressed() && filename.toLower().endsWith(".gz")) THROW(ArgumentException, "No compression requested, but filename ends with '.gz'!");

	if (isCompressed())
	{

		QByteArray open_mode = "wb" + QByteArray::number(compression_level_);
		gz_file_ = gzopen(filename_.toUtf8().constData(), open_mode.constData());
		if (gz_file_==nullptr) THROW(FileAccessException, "Could not open file '" + filename_ + "' for writing!");
		gzbuffer(gz_file_, 1024*1024);
		if (gzsetparams(gz_file_, compression_level_, compression_strategy)!=Z_OK)
		{
			gzclose(gz_file_);
			gz_file_ = nullptr;
			THROW(FileAccessException, "Could not configure gzip compression for file '" + filename_ + "'!");
		}
	}
	else
	{
		bool opened = false;
		if (filename_.isEmpty() && stdout_if_empty)
		{
			opened = file_.open(stdout, QIODevice::WriteOnly);
		}
		else
		{
			opened = file_.open(QIODevice::WriteOnly | QIODevice::Truncate);
		}
		if (!opened) THROW(FileAccessException, "Could not open file '" + filename_ + "' for writing!");
	}

	QIODevice::open(QIODevice::WriteOnly);
}

VersatileOutStream::~VersatileOutStream()
{
	closeInternal(false);
}

void VersatileOutStream::close()
{
	closeInternal(true);
}

void VersatileOutStream::closeInternal(bool throw_on_error)
{
	if (!isOpen()) return;

	bool close_failed = false;
	if (isCompressed())
	{
		close_failed = gzclose(gz_file_)!=Z_OK;
		gz_file_ = nullptr;
	}
	else
	{
		close_failed = !file_.flush();
		file_.close();
	}

	QIODevice::close();
	if (close_failed && throw_on_error) THROW(FileAccessException, "Could not finish writing file '" + filename_ + "'!");
}

qint64 VersatileOutStream::readData(char*, qint64)
{
	THROW(NotImplementedException, "Reading data with VersatileOutStream is not implemented!");
}

qint64 VersatileOutStream::writeData(const char* data, qint64 size)
{
	if (size<=0) return 0;

	if (!isCompressed())
	{
		return file_.write(data, size);
	}

	qint64 written_total = 0;
	while (written_total<size)
	{
		const qint64 remaining = size-written_total;
		const unsigned int chunk_size = static_cast<unsigned int>(qMin(remaining, static_cast<qint64>(std::numeric_limits<int>::max())));
		const int written = gzwrite(gz_file_, data+written_total, chunk_size);
		if (written<=0) return -1;
		written_total += written;
	}

	return written_total;
}

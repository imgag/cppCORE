#ifndef GZIPSTREAMDECOMPRESSOR_H
#define GZIPSTREAMDECOMPRESSOR_H

#include "Exceptions.h"
#include "Log.h"
#include <limits>

// This class handles the decompression of GZ data, it accepts compressed input data
// in chunks and produces QByteArray uncompressed data
class GzipStreamDecompressor
{
public:
    GzipStreamDecompressor()
    {
        memset(&s_, 0, sizeof(s_));
        int ret = inflateInit2(&s_, 16 + MAX_WBITS);
        if (ret != Z_OK) THROW(ProgrammingException, "inflateInit2 failed");
    }

    ~GzipStreamDecompressor()
    {
        inflateEnd(&s_);
    }

    bool feed(const char* chunk, qsizetype chunk_size, QByteArray& out)
    {
        if (chunk_size < 0 || (chunk_size > 0 && chunk == nullptr) || static_cast<quint64>(chunk_size) > std::numeric_limits<uInt>::max())
        {
            Log::error("Invalid gzip input chunk size: " + QString::number(chunk_size));
            return false;
        }

        s_.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(chunk));
        s_.avail_in = static_cast<uInt>(chunk_size);

        uint8_t temp[64 * 1024];

        while (s_.avail_in > 0)
        {
            s_.next_out = temp;
            s_.avail_out = sizeof(temp);

            int ret = inflate(&s_, Z_NO_FLUSH);

            if (ret == Z_STREAM_END)
            {
                size_t produced = sizeof(temp) - s_.avail_out;
                if (produced)
                    out.append(reinterpret_cast<char*>(temp), produced);

                // Allow multi-member gzip
                inflateReset(&s_);
                continue;
            }

            if (ret != Z_OK && ret != Z_BUF_ERROR)
            {
                Log::error("inflate error: " + QString::number(ret) + ", error: " + s_.msg);
                return false;
            }

            size_t produced = sizeof(temp) - s_.avail_out;
            if (produced)
            {
                out.append(reinterpret_cast<char*>(temp), produced);
            }

            if (ret == Z_BUF_ERROR && produced == 0)
            {
                break;
            }
        }

        return true;
    }

private:
    z_stream s_;
};

#endif // GZIPSTREAMDECOMPRESSOR_H

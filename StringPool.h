#ifndef STRINGPOOL_H
#define STRINGPOOL_H

#include "cppCORE_global.h"
#include <QByteArray>
#include <QString>

///Application-wide pool for sharing the storage of equal strings across data structures.
///All methods are thread-safe. Contents are preserved without trimming or normalization.
class CPPCORESHARED_EXPORT StringPool
{
public:
	///Returns an owning string sharing storage with other equal pooled strings.
	static QString get(const QString& value);
	///Returns an owning byte array sharing storage with other equal pooled byte arrays.
	static QByteArray get(const QByteArray& value);
	///Clears both pools. Previously returned values remain valid through Qt implicit sharing.
	///Subsequent calls may use different storage for the same contents.
	static void clean();

private:
	StringPool() = delete;
};

#endif // STRINGPOOL_H

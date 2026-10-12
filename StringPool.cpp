#include "StringPool.h"
#include <QReadWriteLock>
#include <QSet>

namespace
{
	struct Pool
	{
		QSet<QString> strings;
		QSet<QByteArray> byte_arrays;
		QReadWriteLock lock;
	};

	//Defined in cppCORE so all libraries in the process access the same pool.
	Pool& pool()
	{
		static Pool instance;
		return instance;
	}

	template<typename T>
	T get(const T& value, QSet<T>& values, QReadWriteLock& lock)
	{
		//Preserve the distinction between null and non-null empty values.
		if (value.isEmpty()) return value;
		{
			QReadLocker locker(&lock);
			auto found = values.constFind(value);
			if (found != values.cend()) return *found;
		}

		QWriteLocker locker(&lock);
		//Recheck after acquiring the write lock, including after concurrent clean().
		auto found = values.constFind(value);
		if (found != values.cend()) return *found;

		//Own the buffer even if the caller supplied a string made with fromRawData().
		return *values.insert(T(value.constData(), value.size()));
	}
}

QString StringPool::get(const QString& value)
{
	Pool& instance = pool();
	return ::get(value, instance.strings, instance.lock);
}

QByteArray StringPool::get(const QByteArray& value)
{
	Pool& instance = pool();
	return ::get(value, instance.byte_arrays, instance.lock);
}

void StringPool::clean()
{
	Pool& instance = pool();
	QWriteLocker locker(&instance.lock);
	instance.strings.clear();
	instance.byte_arrays.clear();
}

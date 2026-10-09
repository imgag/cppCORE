#ifndef PARAMETERLIST_H
#define PARAMETERLIST_H

#include "cppCORE_global.h"
#include <QHash>
#include <QVariant>
#include <QByteArrayList>
#include <QObject>
#include <QString>

//Parameter type
enum class ParameterType
{
	INT,
	FLOAT,
	BOOL,
	STRING,
	STRINGLIST
};

//Constraint types
enum class ConstraintType
{
	MIN, //INT/FLOAT: min value
	MAX, //INT/FLOAT: max value
	ALLOWED_VALUES, //STRINGLIST: allowed values
	NON_EMPTY //STRING/STRINGLIST: forbids empty strings
};

//Metadata and value of one parameter. Values are initialized and updated by ParameterList.
class CPPCORESHARED_EXPORT Parameter
{
public:
	//constructor
	Parameter(QByteArray name, QString description, ParameterType type, QHash<ConstraintType, QVariant> constraints);

	//members
	const QByteArray& name() const { return name_; }
	const QString& description() const { return description_; }
	ParameterType type() const { return type_; }
	const QHash<ConstraintType, QVariant>& constraints() const { return constraints_; }
	const QVariant& value() const { return value_; }

	//Converts parameter type to string
	static QByteArray toString(ParameterType type);
	//Converts constraint type to string
	static QByteArray toString(ConstraintType type);

private:
	friend class ParameterList;

	QByteArray name_;
	QString description_;
	ParameterType type_;
	QHash<ConstraintType, QVariant> constraints_;
	QVariant value_;
};

//TODO Marc: replace FilterParameter and ParameterData with this class

//Ordered parameters with indexed lookup by name.
class CPPCORESHARED_EXPORT ParameterList
	: public QObject
{
	Q_OBJECT

public:
	ParameterList(QByteArray name, const QList<Parameter>& parameters, const QHash<QByteArray, QVariant>& defaults);
	const QByteArray& name() const { return name_; }
	const Parameter& parameter(const QByteArray& name) const { return entries_.at(parameterIndex(name)); }
	const QList<Parameter>& parameters() const { return entries_; }

	int getInt(const QByteArray& name) const { return checkedValue(name, ParameterType::INT).toInt(); }
	void setInt(const QByteArray& name, int value) { setValue(name, ParameterType::INT, value); }
	double getFloat(const QByteArray& name) const { return checkedValue(name, ParameterType::FLOAT).toDouble(); }
	void setFloat(const QByteArray& name, double value) { setValue(name, ParameterType::FLOAT, value); }
	bool getBool(const QByteArray& name) const { return checkedValue(name, ParameterType::BOOL).toBool(); }
	void setBool(const QByteArray& name, bool value) { setValue(name, ParameterType::BOOL, value); }
	QByteArray getString(const QByteArray& name) const { return checkedValue(name, ParameterType::STRING).toByteArray(); }
	void setString(const QByteArray& name, const QByteArray& value) { setValue(name, ParameterType::STRING, value); }
	QByteArrayList getStringList(const QByteArray& name) const { return checkedValue(name, ParameterType::STRINGLIST).value<QByteArrayList>(); }
	void setStringList(const QByteArray& name, const QByteArrayList& value) { setValue(name, ParameterType::STRINGLIST, QVariant::fromValue(value)); }

signals:
	void parameterChanged();

private:
	QByteArray name_;
	QList<Parameter> entries_;
	QHash<QByteArray, int> indices_;

	int parameterIndex(const QByteArray& name) const;
	int checkedIndex(const QByteArray& name, ParameterType type) const;
	const QVariant& checkedValue(const QByteArray& name, ParameterType type) const { return entries_.at(checkedIndex(name, type)).value(); }
	void setValue(const QByteArray& name, ParameterType type, const QVariant& value, bool emit_changed_signal = true);
};



#endif // PARAMETERLIST_H

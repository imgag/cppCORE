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
	MIN,
	MAX,
	ALLOWED_VALUES,
	NON_EMPTY //Presence forbids empty strings, including entries in a string list.
};

//Parameter description
class CPPCORESHARED_EXPORT ParameterDescription
{
public:
	//constructor
	ParameterDescription(QByteArray name, QString description, ParameterType type, QHash<ConstraintType, QVariant> constraints);

	//members
	const QByteArray& name() const { return name_; }
	const QString& description() const { return description_; }
	ParameterType type() const { return type_; }
	const QHash<ConstraintType, QVariant>& constraints() const { return constraints_; }

	//Converts parameter type to string
	static QByteArray toString(ParameterType type);
	//Converts constraint type to string
	static QByteArray toString(ConstraintType type);

private:
	QByteArray name_;
	QString description_;
	ParameterType type_;
	QHash<ConstraintType, QVariant> constraints_;
};

//TODO Marc: replace FilterParameter and ParameterData with this class

//Parameter configuration that defines the order of parameters in UI and provides meta data about parameters
class CPPCORESHARED_EXPORT Parameter
{
public:
	Parameter(QByteArray track_name);
	const QByteArray& trackName() const {return track_name_;}

	//Clears all entries
	void clear();
	//Appends a entry
	void append(const ParameterDescription& param);
	//Returns the metadata for a parameter by name.
	const ParameterDescription& parameter(const QByteArray& name) const;
	const QList<ParameterDescription>& parameters() const { return entries_; }

protected:
	QByteArray track_name_;
	QList<ParameterDescription> entries_;
};

//Parameter
class CPPCORESHARED_EXPORT ParameterList
	: public QObject
{
	Q_OBJECT

public:
	ParameterList(const Parameter& config, const QHash<QByteArray, QVariant>& defaults);

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

protected:
	const Parameter config_;

private:
	QHash<QByteArray, QVariant> values_;

	const ParameterDescription& checkParameter(const QByteArray& name, ParameterType type) const;
	const QVariant& checkedValue(const QByteArray& name, ParameterType type) const;
	void setValue(const QByteArray& name, ParameterType type, const QVariant& value, bool emit_changed_signal = true);
};



#endif // PARAMETERLIST_H

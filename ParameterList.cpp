#include "ParameterList.h"
#include "Exceptions.h"
#include "Helper.h"
#include <cmath>
#include <limits>
#include <utility>



QByteArray Parameter::toString(ParameterType type)
{
	switch (type)
	{
		case ParameterType::INT: return "INT";
		case ParameterType::DOUBLE: return "DOUBLE";
		case ParameterType::BOOL: return "BOOL";
		case ParameterType::STRING: return "STRING";
		case ParameterType::STRINGLIST: return "STRINGLIST";
	}
	THROW(ProgrammingException, "Unhandled parameter type: " + QString::number(static_cast<int>(type)));
}

QByteArray Parameter::toString(ConstraintType type)
{
	switch (type)
	{
		case ConstraintType::MIN: return "MIN";
		case ConstraintType::MAX: return "MAX";
		case ConstraintType::ALLOWED_VALUES: return "ALLOWED_VALUES";
		case ConstraintType::NON_EMPTY: return "NON_EMPTY";
	}
	THROW(ProgrammingException, "Unhandled constraint type: " + QString::number(static_cast<int>(type)));
}

Parameter::Parameter(QByteArray name, QString description, ParameterType type, QHash<ConstraintType, QVariant> constraints)
	: name_(name)
	, description_(description)
	, type_(type)
	, constraints_(constraints)
{
	QString parameter_name = QString::fromUtf8(name_);
	if (type_ == ParameterType::INT || type_ == ParameterType::DOUBLE)
	{
		for(auto it=constraints.cbegin(); it!=constraints.cend(); ++it)
		{
			if (it.key()==ConstraintType::MIN)
			{
				QString value_as_str = it.value().toString();
				if (!Helper::isNumeric(value_as_str)) THROW(ArgumentException, "Int/double parameter '"+name_+"' has non-numeric 'MIN' constraint '" + value_as_str + "'!");
			}
			else if (it.key()==ConstraintType::MAX)
			{
				QString value_as_str = it.value().toString();
				if (!Helper::isNumeric(value_as_str)) THROW(ArgumentException, "Int/double parameter '"+name_+"' has non-numeric 'MAX' constraint '" + value_as_str + "'!");
			}
			else THROW(ArgumentException, "Int/double parameter '"+name_+"' cannot have constraint '" + Parameter::toString(it.key()) + "'!");

			bool ok = false;
			double limit = it.value().toDouble(&ok);
			if (!ok || !std::isfinite(limit))
			{
				THROW(ArgumentException, "Constraint '" + Parameter::toString(it.key()) + "' for parameter '" + name_ + "' is not a representable finite number: '" + it.value().toString() + "'.");
			}
		}
		if (constraints_.contains(ConstraintType::MIN) && constraints_.contains(ConstraintType::MAX)
			&& constraints_.value(ConstraintType::MIN).toDouble() > constraints_.value(ConstraintType::MAX).toDouble())
		{
			THROW(ArgumentException, "MIN constraint exceeds MAX constraint for parameter '" + name_ + "'.");
		}
	}
	else if (type_ == ParameterType::STRING || type_ == ParameterType::STRINGLIST)
	{
		for(auto it=constraints.cbegin(); it!=constraints.cend(); ++it)
		{
			if (it.key()==ConstraintType::ALLOWED_VALUES)
			{
				if (it.value().userType() != QMetaType::QString && it.value().userType() != QMetaType::QByteArray)
				{
					THROW(ArgumentException, "String/string-list parameter '"+name_+"' has non-string ALLOWED_VALUES constraint!");
				}
			}
			else if (it.key() != ConstraintType::NON_EMPTY) THROW(ArgumentException, "String/string-list parameter '"+name_+"' cannot have constraint '" + Parameter::toString(it.key()) + "'!");
		}
	}
	else
	{
		for(auto it=constraints.cbegin(); it!=constraints.cend(); ++it)
		{
			THROW(ArgumentException, "String parameter '"+name_+"' cannot have constraint '" + Parameter::toString(it.key()) + "'!");
		}
	}
}

ParameterList::ParameterList(QByteArray name, const QList<Parameter>& parameters, const QHash<QByteArray, QVariant>& defaults)
	: name_(name)
	, entries_(parameters)
{
	indices_.reserve(entries_.size());
	for (int i = 0; i < entries_.size(); ++i)
	{
		const QByteArray& name = std::as_const(entries_).at(i).name();
		if (indices_.contains(name)) THROW(ArgumentException, "Parameter '" + name + "' added twice to parameter list '" + name_ + "'.");
		indices_.insert(name, i);
	}
	//Reject undefined parameter names in defaults
	for (auto it = defaults.cbegin(); it != defaults.cend(); ++it)
	{
		parameterIndex(it.key());
	}

	//add default for each parameter
	for (int i = 0; i < entries_.size(); ++i)
	{
		const Parameter& parameter = std::as_const(entries_).at(i);
		auto it = defaults.constFind(parameter.name());
		if (it == defaults.cend()) THROW(ArgumentException, "Missing default for parameter '" + parameter.name() + "' in parameter list '" + name_ + "'.");

		setValue(parameter.name(), parameter.type(), it.value(), false);
	}
}

int ParameterList::parameterIndex(const QByteArray& name) const
{
	auto it = indices_.constFind(name);
	if (it == indices_.cend()) THROW(ArgumentException, "Unknown parameter '" + name + "' in parameter list '" + name_ + "'.");
	return it.value();
}

int ParameterList::checkedIndex(const QByteArray& name, ParameterType type) const
{
	int index = parameterIndex(name);
	const Parameter& parameter = entries_.at(index);
	if (parameter.type() != type) THROW(ArgumentException, "Requested type '"+Parameter::toString(type)+"' for parameter '" + name + "' with type '"+Parameter::toString(parameter.type())+"' in parameter list '"+name_ +"'.");
	return index;
}

void ParameterList::setValue(const QByteArray& name, ParameterType type, const QVariant& value, bool emit_changed_signal)
{
	const int index = checkedIndex(name, type);
	const QVariant normalized_value = validatedValue(std::as_const(entries_).at(index), value);
	if (entries_.at(index).value() == normalized_value) return;
	entries_[index].value_ = normalized_value;
	if (emit_changed_signal) emit parameterChanged();
}

void ParameterList::overrideConstraint(const QByteArray& name, ConstraintType type, const QVariant& value)
{
	const int index = parameterIndex(name);
	const Parameter& current = std::as_const(entries_).at(index);
	auto constraints = current.constraints();
	constraints.insert(type, value);
	Parameter updated(current.name(), current.description(), current.type(), constraints);
	updated.value_ = validatedValue(updated, current.value());
	if (current.constraints() == updated.constraints()) return;
	entries_[index] = updated;
	emit constraintsChanged();
}

QVariant ParameterList::validatedValue(const Parameter& parameter, const QVariant& value) const
{
	const QByteArray& name = parameter.name();
	const ParameterType type = parameter.type();
	const auto& constraints = parameter.constraints();
	if (!value.isValid())
	{
		THROW(ArgumentException, "Invalid value for parameter '" + name + "' in parameter list '" + name_ + "'.");
	}
	QVariant normalized_value;
	if (type == ParameterType::INT || type == ParameterType::DOUBLE)
	{
		bool ok = false;
		double numeric_value = value.toDouble(&ok);
		if (!ok || value.isNull())
		{
			THROW(ArgumentException, "Non-numeric value for parameter '" + name + "' in parameter list '" + name_ + "'.");
		}
		if (!std::isfinite(numeric_value))
		{
			THROW(ArgumentException, "Non-finite value for parameter '" + name + "' in parameter list '"+name_+"'.");
		}
		if (type == ParameterType::INT)
		{
			if (std::floor(numeric_value) != numeric_value || numeric_value < std::numeric_limits<int>::min() || numeric_value > std::numeric_limits<int>::max())
			{
				THROW(ArgumentException, "Value is not a representable integer for parameter '" + name + "' in parameter list '" + name_ + "'.");
			}
			normalized_value = static_cast<int>(numeric_value);
		}
		else normalized_value = numeric_value;
		for (auto it = constraints.cbegin(); it != constraints.cend(); ++it)
		{
			double limit = it.value().toDouble();
			if (it.key() == ConstraintType::MIN && numeric_value < limit)
			{
				THROW(ArgumentException, "Value '"+value.toString()+"' of parameter '"+name+"' smaller than MIN constraint ("+QString::number(limit)+") for parameter '" + name + "' in parameter list '"+name_+"'.");
			}
			if (it.key() == ConstraintType::MAX && numeric_value > limit)
			{
				THROW(ArgumentException, "Value '"+value.toString()+"' of parameter '"+name+"' larger than MAX constraint ("+QString::number(limit)+") for parameter '" + name + "' in parameter list '"+name_+"'.");
			}
		}
	}
	else if (type == ParameterType::BOOL)
	{
		if (value.userType() != QMetaType::Bool || value.isNull())
		{
			THROW(ArgumentException, "Non-boolean value for parameter '" + name + "' in parameter list '" + name_ + "'.");
		}
		normalized_value = value.toBool();
	}
	else if (type == ParameterType::STRING)
	{
		if (value.userType() != QMetaType::QString && value.userType() != QMetaType::QByteArray)
		{
			THROW(ArgumentException, "Non-string value for parameter '" + name + "' in parameter list '" + name_ + "'.");
		}
		normalized_value = value.toByteArray();
		if (constraints.contains(ConstraintType::NON_EMPTY) && normalized_value.toByteArray().isEmpty())
		{
			THROW(ArgumentException, "Empty string is not allowed for parameter '" + name + "' in parameter list '" + name_ + "'.");
		}
		if (constraints.contains(ConstraintType::ALLOWED_VALUES))
		{
			QByteArrayList allowed_values = constraints.value(ConstraintType::ALLOWED_VALUES).toByteArray().split('\t');
			if (!allowed_values.contains(normalized_value.toByteArray()))
			{
				THROW(ArgumentException, "Value '"+value.toByteArray()+"' is not allowed for parameter '" + name + "' in parameter list '"+name_+"'.");
			}
		}
	}
	else if (type == ParameterType::STRINGLIST)
	{
		if (value.userType() != qMetaTypeId<QByteArrayList>())
		{
			THROW(ArgumentException, "Non-string-list value for parameter '" + name + "' in parameter list '" + name_ + "'.");
		}
		const QByteArrayList entries = value.value<QByteArrayList>();
		if (constraints.contains(ConstraintType::NON_EMPTY) && entries.contains(QByteArray()))
		{
			THROW(ArgumentException, "Empty string is not allowed in parameter '" + name + "' in parameter list '" + name_ + "'.");
		}
		if (constraints.contains(ConstraintType::ALLOWED_VALUES))
		{
			const QByteArrayList allowed_values = constraints.value(ConstraintType::ALLOWED_VALUES).toByteArray().split('\t');
			for (const QByteArray& entry : entries)
			{
				if (!allowed_values.contains(entry))
				{
					THROW(ArgumentException, "Value '" + entry + "' is not allowed for parameter '" + name + "' in parameter list '" + name_ + "'.");
				}
			}
		}
		normalized_value = QVariant::fromValue(entries);
	}
	else
	{
		THROW(ProgrammingException, "Unhandled parameter type: " + QString::number(static_cast<int>(type)));
	}

	return normalized_value;
}

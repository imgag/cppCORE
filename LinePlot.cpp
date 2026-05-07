#include "LinePlot.h"

#include <limits>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QLegend>

#include "Exceptions.h"
#include "Log.h"
#include "BasicStatistics.h"
#include "PlotUtils.h"

LinePlot::LinePlot()
	: yrange_set_(false)
{
}

void LinePlot::addLine(const QVector<double>& values, QString label)
{
	if (xvalues_.count()!=0 && values.count()!=xvalues_.count())
	{
		THROW(ArgumentException, "Plot '" + label + "' has " + QString::number(values.count()) + " values, but " + QString::number(xvalues_.count()) + " are expected because x axis values are set!");
	}

	lines_.append(PlotLine(values, label));
}

void LinePlot::setXValues(const QVector<double>& xvalues)
{
	if (lines_.count()!=0)
	{
		THROW(ProgrammingException, "You have to set x axis values of LinePlot before adding any lines!");
	}

	xvalues_ = xvalues;
}

void LinePlot::store(QString filename)
{
	if (lines_.isEmpty())
	{
		Log::warn("LinePlot does not have any lines to plot");
		return;
	}

	if (!yrange_set_)
	{
		double min_val = std::numeric_limits<double>::max();
		double max_val = -std::numeric_limits<double>::max();

		for (const PlotLine& line : lines_)
		{
			for (double value : line.values)
			{
				min_val = std::min(value, min_val);
				max_val = std::max(value, max_val);
			}
		}

		if (max_val > min_val)
		{
			ymin_ = min_val - 0.01 * (max_val - min_val);
			ymax_ = max_val + 0.01 * (max_val - min_val);
		}
	}

	PlotUtils* plot_utils = new PlotUtils();
	QChart* chart = plot_utils->getChart();

	// create axes
	QValueAxis* axis_x = new QValueAxis();
	if (!xlabel_.isEmpty()) axis_x->setTitleText(xlabel_);
	double xmin = std::numeric_limits<double>::max();
	double xmax = -std::numeric_limits<double>::max();
	for (double value : xvalues_)
	{
		xmin = std::min(value, xmin);
		xmax = std::max(value, xmax);
	}
	if (xvalues_.isEmpty())
	{
		xmin = ymin_;
		xmax = ymax_;
	}

	axis_x->setRange(xmin, xmax);
	axis_x->applyNiceNumbers();
	chart->addAxis(axis_x, Qt::AlignBottom);

	QValueAxis* axis_y = new QValueAxis();
	if (!ylabel_.isEmpty()) axis_y->setTitleText(ylabel_);
	if (BasicStatistics::isValidFloat(ymin_) && BasicStatistics::isValidFloat(ymax_))
	{
		axis_y->setRange(ymin_, ymax_);
	}
	axis_y->applyNiceNumbers();
	chart->addAxis(axis_y, Qt::AlignLeft);

	// series of data points
	for (const PlotLine& line : lines_)
	{
		QLineSeries* series = new QLineSeries();
		series->setName(line.label);

		int n = line.values.size();
		for (int i = 0; i < n; ++i)
		{
			double x = (i < xvalues_.size()) ? xvalues_[i] : i;
			series->append(x, line.values[i]);
		}

		chart->addSeries(series);
		series->attachAxis(axis_x);
		series->attachAxis(axis_y);

		// improves line smoothness (optional) to look very close to matplotlib
		QPen pen = series->pen();
		pen.setWidthF(1.5);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		series->setPen(pen);
	}

	// title and legend
	if (lines_.count() == 1)
	{
		if (!lines_[0].label.isEmpty())
		{
			chart->setTitle(lines_[0].label);
		}
		chart->legend()->setVisible(false);
	}
	else
	{
		chart->legend()->setVisible(true);
		chart->legend()->setAlignment(Qt::AlignBottom);

		QFont legendFont = chart->legend()->font();
		legendFont.setPointSize(8);
		chart->legend()->setFont(legendFont);
	}

	plot_utils->applyFontSettings();
	plot_utils->saveAsPng(filename, 600, 400);
}


LinePlot::PlotLine::PlotLine()
{
}

LinePlot::PlotLine::PlotLine(const QVector<double>& v, QString l)
	: values(v)
	, label(l)
{
}

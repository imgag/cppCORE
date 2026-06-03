#include "ScatterPlot.h"

#include <limits>
#include <QChart>
#include <QScatterSeries>
#include <QLineSeries>
#include <QValueAxis>
#include <QLogValueAxis>
#include <QChartView>
#include <QStringList>

#include "Exceptions.h"
#include "Log.h"
#include "BasicStatistics.h"
#include "PlotUtils.h"

ScatterPlot::ScatterPlot()
	: yrange_set_(false)
	, xrange_set_(false)
	, yscale_log_(false)
{
}

void ScatterPlot::store(QString filename)
{
	if (points_.isEmpty())
	{
		Log::warn("ScatterPlot does not have any points to plot");
		return;
	}

	if (!xrange_set_ || !yrange_set_)
	{
		double xmin = std::numeric_limits<double>::max();
		double xmax = -std::numeric_limits<double>::max();
		double ymin = std::numeric_limits<double>::max();
		double ymax = -std::numeric_limits<double>::max();

		for (const QPointF& p : points_)
		{
			xmin = std::min(xmin, p.y());
			xmax = std::max(xmax, p.y());
			ymin = std::min(ymin, p.x());
			ymax = std::max(ymax, p.x());
		}

		if (!xrange_set_)
		{
			xmin_ = xmin-0.01*(xmax-xmin);
			xmax_ = xmax+0.01*(xmax-xmin);
		}
		if (!yrange_set_)
		{
			ymin_ = ymin-0.01*(ymax - ymin);
			ymax_ = ymax+0.01*(ymax - ymin);
		}
	}

	PlotUtils* plot_utils = new PlotUtils();
	QChart* chart = plot_utils->getChart();
	chart->legend()->setVisible(color_legend_.count() > 0);

	// group by color
	QHash<QString, QList<QPointF>> grouped;
	grouped.reserve(color_legend_.size());
	for (int i=0; i<points_.size(); ++i)
	{
		QString color = (colors_.size() > i) ? colors_[i] : "black";
		grouped[color].append(points_[i]);
	}

	for (auto it = grouped.begin(); it != grouped.end(); ++it)
	{
		auto* s = new QScatterSeries();
		s->setMarkerSize(6.0);
		s->setColor(QColor::fromString(it.key()));

		if (color_legend_.contains(it.key()))
		{
			s->setName(color_legend_[it.key()]);
		}
		else
		{
			s->setName("");
		}

		s->append(it.value());
		chart->addSeries(s);
	}

	// vertical lines
	for (double x : vlines_)
	{
		auto* line = new QLineSeries();
		line->append(x, ymin_);
		line->append(x, ymax_);

		QPen pen(Qt::black);
		pen.setStyle(Qt::DashLine);
		line->setPen(pen);
		line->setName("");

		chart->addSeries(line);
	}

	QAbstractAxis* axis_x = new QValueAxis();
	axis_x->setTitleText(xlabel_);

	QAbstractAxis* axis_y = nullptr;

	if (yscale_log_)
	{
		auto* log_axis = new QLogValueAxis();
		log_axis->setTitleText(ylabel_);
		axis_y = log_axis;
	}
	else
	{
		auto* val_axis = new QValueAxis();
		val_axis->setTitleText(ylabel_);
		axis_y = val_axis;
	}

	chart->addAxis(axis_x, Qt::AlignBottom);
	chart->addAxis(axis_y, Qt::AlignLeft);

	// attach axes
	for (auto* s : chart->series())
	{
		s->attachAxis(axis_x);
		s->attachAxis(axis_y);
	}

	if (BasicStatistics::isValidFloat(xmin_) && BasicStatistics::isValidFloat(xmax_))
	{
		static_cast<QValueAxis*>(axis_x)->setRange(xmin_, xmax_);
	}

	if (BasicStatistics::isValidFloat(ymin_) && BasicStatistics::isValidFloat(ymax_))
	{
		if (yscale_log_)
		{
			static_cast<QLogValueAxis*>(axis_y)->setRange(ymin_, ymax_);
		}
		else
		{
			static_cast<QValueAxis*>(axis_y)->setRange(ymin_, ymax_);
		}
	}

	plot_utils->applyFontSettings();
	plot_utils->saveAsPng(filename, 600, 400);
}

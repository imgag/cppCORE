#include "PlotUtils.h"
#include "Exceptions.h"
#include <QApplication>
#include <QFontDatabase>
#include <QLineSeries>
#include <QAreaSeries>
#include <QLegendMarker>
#include <QLegend>

PlotUtils::PlotUtils()
	: chart_view_(new QChart())
{
	// QChart needs an instance of a GUI app and a screen to be rendered, here we make sure it will work properly on the server in a headless mode.
	// QT_QPA_PLATFORM=offscreen environment variables has to be set for the headless mode, otherwise an exception will be thrown
	QCoreApplication* app = QCoreApplication::instance();
	if (!qobject_cast<QApplication*>(app)) THROW(ProgrammingException, "The code needs a running GUI application to be able to render plots");
}

QChart& PlotUtils::chart()
{
	return *chart_view_.chart();
}

void PlotUtils::applyFontSettings()
{
	int font_id = QFontDatabase::addApplicationFont(":/fonts/Arimo-Regular.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Arimo-Bold.ttf");
	QFontDatabase::addApplicationFont(":/fonts/Arimo-Medium.ttf");
	QString font_family = QFontDatabase::applicationFontFamilies(font_id).at(0);

	QFont regular_font = QFont(font_family);
	regular_font.setPixelSize(14);
	regular_font.setWeight(QFont::Normal);

	QFont bold_font = QFont(font_family);
	bold_font.setPixelSize(14);
	bold_font.setWeight(QFont::Bold);

	chart_view_.chart()->setTitleFont(bold_font);
	for (auto axis : chart_view_.chart()->axes())
	{
		axis->setLabelsFont(regular_font);
		axis->setTitleFont(bold_font);
	}
	chart_view_.chart()->legend()->setFont(regular_font);
}

QFont PlotUtils::getLabelFont()
{
	int font_id = QFontDatabase::addApplicationFont(":/fonts/Arimo-Regular.ttf");
	QString font_family = QFontDatabase::applicationFontFamilies(font_id).at(0);
	QFont label_font = QFont(font_family);
	label_font.setPixelSize(9);
	return label_font;
}

void PlotUtils::overpaintAxisX(QValueAxis* axis_x, QValueAxis* axis_y, double max)
{

	// a hack to hide zero-height bars, which are drawn on the top of the x axis
	QLineSeries *upper_x = new QLineSeries();
	QLineSeries *lower_x = new QLineSeries();
	// double max = bars_.size() - 0.5;
	lower_x->append(0, 0);
	upper_x->append(max, 0);
	lower_x->append(max, 0);
	upper_x->append(max, 0);

	QAreaSeries *area_x = new QAreaSeries(upper_x, lower_x);
	upper_x->setParent(area_x);
	lower_x->setParent(area_x);
	area_x->setName("x_axis");

	QColor x_color(axis_x->gridLineColor());
	area_x->setColor(x_color);
	area_x->setBorderColor(x_color);
	chart_view_.chart()->addSeries(area_x);
	area_x->attachAxis(axis_x);
	area_x->attachAxis(axis_y);


	for (QAbstractSeries* s : chart_view_.chart()->series())
	{
		QAreaSeries* area = qobject_cast<QAreaSeries*>(s);
		if (!area) continue;

		if (area->name() == "x_axis")
		{
			auto markers = chart_view_.chart()->legend()->markers(area);
			for (auto m : std::as_const(markers)) m->setVisible(false);
		}
	}
}

void PlotUtils::saveAsPng(QString filename, int width, int height)
{
	// image rendering
	// QChartView chartView(chart_);
	chart_view_.resize(width, height);
	chart_view_.setMinimumSize(width, height);
	chart_view_.setMaximumSize(width, height);

	// antialiasing for smoother lines and text
	chart_view_.setRenderHint(QPainter::Antialiasing, true);
	chart_view_.setRenderHint(QPainter::TextAntialiasing, true);
	chart_view_.setRenderHint(QPainter::SmoothPixmapTransform, true);

	QApplication::processEvents();
	QPixmap pixmap = chart_view_.grab();
	pixmap.setDevicePixelRatio(1.0);
	pixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	if (!pixmap.save(filename.replace("\\", "/"), "PNG"))
	{
		THROW(ProgrammingException, "Could not save bar plot to the file: " + filename);
	}	
}

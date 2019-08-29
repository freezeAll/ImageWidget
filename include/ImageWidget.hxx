#pragma once

#include <QtWidgets/QWidget>
#include "opencv2/opencv.hpp"
class ImageWidgetPrivate;
class ImageWidgetBasePrivate;
struct ImageBox
{
	ImageBox(const std::string& name = "default",const QRect& rect = QRect(0,0,0,0),const QColor& color = QColor(0,0,255),const int& id = -1,const bool& display = true) :
		id(id),
		x(rect.x()),
		y(rect.y()),
		width(rect.width()),
		height(rect.height()),
		name(name),
		display(display),
		editing(false),
		red(color.red()),
		blue(color.blue()),
		green(color.green()),
		isEnv(false)
	{}
	std::string name;
	bool display;
	bool isEnv;
	int id;
	int x;
	int y;
	int width;
	int height;
	bool editing;
	int red;
	int green;
	int blue;
	cv::Rect toCvRect()
	{
		return cv::Rect(x,y,width,height);
	}
	QRect toQRect()
	{
		return QRect(x, y, width, height);
	}
	void fromQRect(const QRect& r)
	{
		x = r.x();
		y = r.y();
		width = r.width();
		height = r.height();
	}
	void fromCvRect(const cv::Rect& r)
	{
		x = r.x;
		y = r.y;
		width = r.width;
		height = r.height;
	}
	bool isInBox(const QPoint& p)
	{
		return p.x() >= x && p.x() <= (x + width) && p.y() >= y && p.y() <= (y + height);
	}
	QColor toQColor()
	{
		return QColor(red,green,blue);
	}
	void fromQColor(const QColor& c)
	{
		red = c.red();
		blue = c.blue();
		green = c.green();
	}
};

struct PaintData
{
	std::vector<std::tuple<cv::Point2d, cv::Point2d, int, cv::Scalar>> lines;
	std::vector<std::tuple<cv::Rect2d, int, cv::Scalar>> rects;
	std::vector<std::tuple<cv::Rect2d, int, cv::Scalar>> circles;
	void drawDatas(cv::Mat& mat);
	void paintDatas(QPainter*, ImageWidgetBasePrivate*);
};

class ImageWidgetBase : public QWidget
{
	Q_OBJECT

public:
	ImageWidgetBase(QWidget *parent = Q_NULLPTR);
	~ImageWidgetBase();
public slots:
	;
	void displayCVMat(const cv::Mat&);
	void displayQImage(const QImage&);
	void displayCVMatWithData(const cv::Mat&, const PaintData&);
	void displayQImageWithData(const QImage&, const PaintData&);
	void displayDoneCVMat(const cv::Mat&);
	void displayDoneQImage(const QImage&);
	void displayDoneCVMatWithData(const cv::Mat&,const PaintData&);
	void displayDoneQImageWithData(const QImage&, const PaintData&);
signals:
	void clickedPosition(QPoint);
protected:
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void paintEvent(QPaintEvent*);
	void wheelEvent(QWheelEvent*);
	ImageWidgetBasePrivate *d;
};

class ImageWidget : public ImageWidgetBase
{
	Q_OBJECT

public:
	ImageWidget(QWidget *parent = Q_NULLPTR);
	~ImageWidget();
public slots:
	bool addImageBox(const ImageBox&);
	bool paintNewImageBox(const std::string& name = "default", const QColor& color = QColor(0, 0, 255), const int& id = -1, const bool& display = true);
	bool addImageBox(const std::string& name = "default", const QRect& rect = QRect(0, 0, 0, 0), const QColor& color = QColor(0, 0, 255), const int& id = -1, const bool& display = true);
	bool getImageBoxFromId(const int&,ImageBox&);
	std::vector<ImageBox> getImageBoxsFromName(const std::string&);
	void clearAllBoxs();
protected:
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void paintEvent(QPaintEvent*);
	void wheelEvent(QWheelEvent*);
	void contextMenuEvent(QContextMenuEvent*);
private:
	ImageWidgetPrivate *d_ptr;
};

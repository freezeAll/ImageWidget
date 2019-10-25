#pragma once

#include <QGlobal.h>
#ifdef IMAGEWIDGET_QML
#include <QtQuick/QQuickPaintedItem>
#include <QtGui/QPainter>
#else
#include <QtWidgets/QWidget>
#endif // IMAGEWIDGET_QML
#include "opencv2/opencv.hpp"
Q_DECLARE_METATYPE(cv::Mat)
class ImageWidgetPrivate;
class ImageWidgetBasePrivate;
class ImageBox : public QObject
{
	Q_OBJECT
public:
	enum ShapeType
	{
		Point,
		Line,
		Rect,
		Circle
	};
	Q_ENUM(ShapeType)
	ImageBox(
		const ImageBox::ShapeType& itype, 
		const QString& name = "default", 
		const int& ix1 = 0, const int& iy1 = 0, 
		const int& ix2 = 0, const int& iy2 = 0, 
		const QColor& color = QColor(0, 0, 255), 
		const int& id = -1, 
		const bool& display = true) :
		id(id),
		x(ix1),
		y(iy1),
		name(name),
		display(display),
		editing(false),
		red(color.red()),
		blue(color.blue()),
		green(color.green()),
		env(false),
		type(itype)
	{
		switch (type)
		{
		case Point:
			break;
		case Circle:
			width = ix2;
			height = iy2;
			break;
		case Rect:
			width = ix2;
			height = iy2;
			break;
		case Line:
			ex = ix2;
			ey = iy2;
			break;
		default:
			break;
		}
	}
	ImageBox() = default;
	ImageBox(const ImageBox& o)
	{
		name = o.name;
		display = o.display;
		env = o.env;
		id = o.id;
		x = o.x;
		y = o.y;
		width = o.width;
		height = o.height;
		ex = o.ex;
		ey = o.ey;
		editing = o.editing;
		red = o.red;
		green = o.green;
		blue = o.blue;
		type = o.type;
	}
	void operator=(const ImageBox& o)
	{
		name = o.name;
		display = o.display;
		env = o.env;
		id = o.id;
		x = o.x;
		y = o.y;
		width = o.width;
		height = o.height;
		ex = o.ex;
		ey = o.ey;
		editing = o.editing;
		red = o.red;
		green = o.green;
		blue = o.blue;
		type = o.type;
	}
	QString name;
	bool display;
	bool env;
	int id;
	double x;
	double y;
	double width;
	double height;
	double ex;
	double ey;
	bool editing;
	int red;
	int green;
	int blue;
	ShapeType type;
	Q_PROPERTY(QString name READ getName WRITE setName)
	QString getName()
	{
		return name;
	}
	void setName(const QString& n)
	{
		name = n;
	}
	Q_PROPERTY(bool display READ isDisplay WRITE setIsDisplayed)
	bool isDisplay()
	{
		return display;
	}
	void setIsDisplayed(const bool& d)
	{
		display = d;
	}
	Q_PROPERTY(bool env READ isEnv WRITE setIsEnv)
	bool isEnv()
	{
		return env;
	}
	void setIsEnv(const bool& e)
	{
		env = e;
	}
	Q_PROPERTY(int id READ getId WRITE setId)
	int getId()
	{
		return id;
	}
	void setId(const int& i)
	{
		id = i;
	}
	Q_PROPERTY(double x READ getX WRITE setX)
	double getX()
	{
		return x;
	}
	void setX(const double& i)
	{
		x = i;
	}
	Q_PROPERTY(double y READ getY WRITE setY)
	double getY()
	{
		return y;
	}
	void setY(const double& i)
	{
		y = i;
	}
	Q_PROPERTY(double ex READ getEx WRITE setEx)
		double getEx()
	{
		return ex;
	}
	void setEx(const double& i)
	{
		ex = i;
	}
	Q_PROPERTY(double ey READ getEy WRITE setEy)
	double getEy()
	{
		return ey;
	}
	void setEy(const double& i)
	{
		ey = i;
	}
	Q_PROPERTY(double width READ getWidth WRITE setWidth)
		double getWidth()
	{
		return width;
	}
	void setWidth(const double& i)
	{
		width = i;
	}
	Q_PROPERTY(double height READ getHeight WRITE setHeight)
		double getHeight()
	{
		return height;
	}
	void setHeight(const double& i)
	{
		height = i;
	}
	Q_PROPERTY(bool editing READ isEditing WRITE setIsEditing)
		bool isEditing()
	{
		return editing;
	}
	void setIsEditing(const bool& e)
	{
		editing = e;
	}
	Q_PROPERTY(int red READ getRed WRITE setRed)
		int getRed()
	{
		return red;
	}
	void setRed(const int& i)
	{
		red = i;
	}
	Q_PROPERTY(int green READ getGreen WRITE setGreen)
	int getGreen()
	{
		return green;
	}
	void setGreen(const int& i)
	{
		green = i;
	}
	Q_PROPERTY(int blue READ getBlue WRITE setBlue)
	int getBlue()
	{
		return blue;
	}
	void setBlue(const int& i)
	{
		blue = i;
	}
	Q_PROPERTY(ShapeType type READ getType WRITE setType)
	ShapeType getType()
	{
		return type;
	}
	void setType(const ShapeType& i)
	{
		type = i;
	}
	cv::Rect toCvRect()
	{
		return cv::Rect(std::floor(x), std::floor(y), std::floor(width), std::floor(height));
	}
	QRect toQRect()
	{
		return QRect(std::floor(x), std::floor(y), std::floor(width), std::floor(height));
	}
	void fromQRect(const QRect & r)
	{
		x = r.x();
		y = r.y();
		width = r.width();
		height = r.height();
	}
	void fromCvRect(const cv::Rect & r)
	{
		x = r.x;
		y = r.y;
		width = r.width;
		height = r.height;
	}
	bool isInBox(const QPoint & p)
	{
		bool out(false);
		if (type == Rect || type == Circle)
		{
			out = p.x() >= x && p.x() <= (x + width) && p.y() >= y && p.y() <= (y + height);
		}
		else if (type == Line)
		{
			double k = (double(ey) - double(y)) / (double(ex) - double(x));
			out = ((std::abs(k * double(p.x()) - double(p.y()) + (y - k * x)) / sqrt(k * k + 1.)) < 5.) && p.x() - 5 >= x&& p.x() + 5. <= ex;
		}
		else
		{
			out = std::abs(p.x() - x) < 5 && std::abs(p.x() - x) < 5;
		}
		return out;
	}
	QColor toQColor()
	{
		return QColor(red, green, blue);
	}
	void fromQColor(const QColor & c)
	{
		red = c.red();
		blue = c.blue();
		green = c.green();
	}
	void fromQLine(const QLine & line)
	{
		x = line.x1();
		y = line.y1();
		ex = line.x2();
		ey = line.y2();
	}
	QLine toQLine()
	{
		return QLine(std::floor(x), std::floor(y), std::floor(ex), std::floor(ey));
	}
	void fromQPoint(const QPoint & pnt)
	{
		x = pnt.x();
		y = pnt.y();
	}
	QPoint toQPoint()
	{
		return QPoint(std::floor(x), std::floor(y));
	}
};
Q_DECLARE_METATYPE(ImageBox)

class PaintData
{
public:
	std::vector<std::tuple<cv::Point2d, cv::Point2d, int, cv::Scalar>> lines;
	std::vector<std::tuple<cv::Rect2d, int, cv::Scalar>> rects;
	std::vector<std::tuple<cv::Rect2d, int, cv::Scalar>> circles;
	void drawDatas(cv::Mat& mat);
	void paintDatas(QPainter*, ImageWidgetBasePrivate*);
};
Q_DECLARE_METATYPE(PaintData)

class ImageWidgetBase : public 
#ifdef IMAGEWIDGET_QML
	QQuickPaintedItem
#else
	QWidget
#endif // IMAGEWIDGET_QML
{
	Q_OBJECT
	Q_DISABLE_COPY(ImageWidgetBase)
public:
#ifdef IMAGEWIDGET_QML
	ImageWidgetBase(QQuickItem* parent = Q_NULLPTR);
#else
	ImageWidgetBase(QWidget *parent = Q_NULLPTR);
#endif
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
	void displayCVMat(const QVariant& img);
	void displayQImage(const QVariant& img);
	void displayCVMatWithData(const QVariant& img, const QVariant& paint_data);
	void displayQImageWithData(const QVariant& img, const QVariant& paint_data);
	void displayDoneCVMat(const QVariant& img);
	void displayDoneQImage(const QVariant& img);
	void displayDoneCVMatWithData(const QVariant& img, const QVariant& paint_data);
	void displayDoneQImageWithData(const QVariant& img, const QVariant& paint_data);
signals:
	void clickedPosition(QPoint);
	void underMouseSourcePosition(QPoint);
	void underMouseTargetPosition(QPoint);
protected:
	virtual void mousePressEvent(QMouseEvent*) override;
	virtual void mouseMoveEvent(QMouseEvent*) override;
	virtual void mouseReleaseEvent(QMouseEvent*) override;
	virtual void mouseDoubleClickEvent(QMouseEvent*) override;
#ifdef IMAGEWIDGET_QML
	virtual void paint(QPainter* painter) override;
#else
	virtual void paintEvent(QPaintEvent*) override;
#endif
	virtual void wheelEvent(QWheelEvent*) override;
	ImageWidgetBasePrivate *d;
};

class ImageWidget : public ImageWidgetBase
{
	Q_OBJECT
	Q_DISABLE_COPY(ImageWidget)
public:
#ifdef IMAGEWIDGET_QML
	ImageWidget(QQuickItem* parent = Q_NULLPTR);
#else
	ImageWidget(QWidget *parent = Q_NULLPTR);
#endif

	~ImageWidget();
public slots:
	bool addImageBox(const ImageBox&);
	bool paintNewImageBox(const ImageBox::ShapeType& type = ImageBox::Rect,const QString& name = "default",const QColor& color = QColor(0, 0, 255), const int& id = -1, const bool& display = true);
	bool addImageBox(const ImageBox::ShapeType& type = ImageBox::Rect,const QString& name = "default", const int& = 0, const int& = 0, const int& = 0, const int& = 0, const QColor& color = QColor(0, 0, 255), const int& id = -1, const bool& display = true);
	bool getImageBoxFromId(const int&,ImageBox&);
	std::vector<ImageBox> getImageBoxsFromName(const QString&);
	void clearAllBoxs();
protected:
	virtual void mousePressEvent(QMouseEvent*) override;
	virtual void mouseMoveEvent(QMouseEvent*) override;
	virtual void mouseReleaseEvent(QMouseEvent*) override;
	virtual void mouseDoubleClickEvent(QMouseEvent*) override;
#ifdef IMAGEWIDGET_QML
	virtual void paint(QPainter* painter) override;
#else
	virtual void paintEvent(QPaintEvent*) override;
#endif
	virtual void wheelEvent(QWheelEvent*) override;
#ifndef IMAGEWIDGET_QML
	virtual void contextMenuEvent(QContextMenuEvent*) override;
#endif
private:
	ImageWidgetPrivate *d_ptr;
};

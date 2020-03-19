#pragma once

#include <QGlobal.h>
#ifdef IMAGEWIDGET_QML
#include <QtQuick/QQuickPaintedItem>
#include <QtGui/QPainter>
#else
#include <QtWidgets/QWidget>
#include <QString>
#include <QPen>
#include <QBrush>
//#include <QObject>
#endif // IMAGEWIDGET_QML
#include "opencv2/opencv.hpp"
#include <optional>
#include <QVariant>
#pragma once
#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(IMAGEWIDGET_LIB)
#  define IMAGEWIDGET_EXPORT Q_DECL_EXPORT
# else
#  define IMAGEWIDGET_EXPORT Q_DECL_IMPORT
# endif
#else
# define IMAGEWIDGET_EXPORT
#endif


class ImageWidgetPrivate;
class ImageWidgetBasePrivate;

class IMAGEWIDGET_EXPORT ImageBox : public QObject
{
	Q_OBJECT

public:
	enum GrabedEdgeType
	{
		None,
		Top,
		Bottom,
		Left,
		Right,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight
	};
	ImageBox(const QString& name = "default",const int& id = -1, const bool& display = true, const bool& env = false, QObject* parent = nullptr);
	ImageBox(const ImageBox& other);
	~ImageBox();
	virtual void operator=(const ImageBox& box);
	Q_PROPERTY(bool display READ isDisplay WRITE setIsDisplay);
	Q_PROPERTY(int boxID READ getBoxID WRITE setBoxID);
	Q_PROPERTY(QString name READ getName WRITE setName);
	Q_PROPERTY(bool env READ isEnv WRITE setIsEnv);
	Q_PROPERTY(bool editing READ getEditing WRITE setEditing);

	bool isDisplay();
	bool isEnv();
	void setIsEnv(const bool&);
	void setIsDisplay(const bool&);
	int getBoxID();
	void setBoxID(const int&);
	QString getName();
	void setName(const QString&);
	bool getEditing();
	void setEditing(const bool&);
	Q_INVOKABLE virtual void resetData() {};
	virtual cv::Mat getMask(const QSize&) { return cv::Mat(); };
	Q_INVOKABLE QVariant getMaskVar(const int& width,const int& height);
protected:
	virtual void paintShape(QPainter*, ImageWidgetBasePrivate*) {};
	virtual bool isInBox(const QPoint&) { return false; };
	virtual void moveBox(const QPointF&,const QSize&) {};
	virtual std::optional<ImageBox::GrabedEdgeType> checkPress(const QPoint&, const double&) { return std::optional<ImageBox::GrabedEdgeType>(); };
	virtual std::optional<ImageBox::GrabedEdgeType> checkMove(const QPoint&, const double&) { return std::optional<ImageBox::GrabedEdgeType>(); };
	virtual void normalize() {};
	virtual void startPaint(const QPointF&) {};
	virtual void endPaint(const QPointF&) {};
	virtual void editEdge(const ImageBox::GrabedEdgeType& type, const QPointF& pos) {};
	virtual void fixShape(const QSize& size) {};
protected:
	friend class ImageWidgetPrivate;
	friend class ImageWidget;
	friend class ImageWidgetBasePrivate;
	friend class ImageWidgetBase;
	int boxID;
	bool display;
	QString name;
	bool env;
	bool editing;
};
Q_DECLARE_METATYPE(ImageBox)

class IMAGEWIDGET_EXPORT RectImageBox : public ImageBox
{
	Q_OBJECT
signals:
public:
	Q_INVOKABLE RectImageBox(QObject* parent = nullptr);
	Q_INVOKABLE RectImageBox(
		const double& x,
		const double& y,
		const double& width,
		const double& height,
		const QPen & pen = QPen(QColor(0,0,255)),
		const QBrush& brush= QBrush(QColor(0, 0, 255, 0),Qt::BrushStyle::SolidPattern),
		const QString& name = "default",
		const int& id = -1,
		const bool& display = true, 
		const bool& env = false,
		QObject* parent = nullptr);
	Q_INVOKABLE RectImageBox(const RectImageBox& other);
	virtual void operator=(const ImageBox& ib) override;
	void operator=(const RectImageBox& rb);
	~RectImageBox();
	Q_PROPERTY(double x READ getX WRITE setX);
	Q_PROPERTY(double y READ getY WRITE setY);
	Q_PROPERTY(double width READ getWidth WRITE setWidth);
	Q_PROPERTY(double height READ getHeight WRITE setHeight);
	Q_PROPERTY(QPen pen READ getPen WRITE setPen);
	Q_PROPERTY(QBrush brush READ getBrush WRITE setBrush);
	Q_PROPERTY(QPen editingPen READ getEditingPen WRITE setEditingPen);
	Q_PROPERTY(QBrush editingBrush READ getEditingBrush WRITE setEditingBrush);

	QPen getPen();
	QBrush getBrush();
	QPen getEditingPen();
	QBrush getEditingBrush();

	void setPen(const QPen& pen);
	void setEditingPen(const QPen& pen);
	void setBrush(const QBrush& brush);
	void setEditingBrush(const QBrush& brush);
	void setX(const double& x);
	double getX();
	void setY(const double& y);
	double getY();
	void setWidth(const double& w);
	double getWidth();
	void setHeight(const double& h);
	double getHeight();
	Q_INVOKABLE QRect getQRect();
	Q_INVOKABLE void fromQRect(const QRect& rect);
	Q_INVOKABLE QRectF getQRectF();
	Q_INVOKABLE void fromQRectF(const QRectF& rect);

	Q_INVOKABLE cv::Rect getCVRect();
	Q_INVOKABLE void fromCVRect(const cv::Rect& rect);
	Q_INVOKABLE virtual void resetData() override;
	Q_INVOKABLE virtual cv::Mat getMask(const QSize&) override;
protected:
	virtual void paintShape(QPainter*, ImageWidgetBasePrivate*) override;
	virtual bool isInBox(const QPoint&) override;
	virtual void moveBox(const QPointF& ,const QSize&) override;
	virtual std::optional<ImageBox::GrabedEdgeType> checkPress(const QPoint&,const double&) override;
	virtual std::optional<ImageBox::GrabedEdgeType> checkMove(const QPoint&, const double&) override;
	virtual void normalize() override;
	virtual void startPaint(const QPointF&) override;
	virtual void endPaint(const QPointF&) override;
	virtual void editEdge(const ImageBox::GrabedEdgeType& type, const QPointF& pos) override;
	virtual void fixShape(const QSize& size) override;
	double x;
	double y;
	double width;
	double height;
	QPen pen;
	QBrush brush;
	QPen editingPen;
	QBrush editingBrush;
};
Q_DECLARE_METATYPE(RectImageBox)

class IMAGEWIDGET_EXPORT EllipseImageBox : public RectImageBox
{
	Q_OBJECT
signals:
public:
	Q_INVOKABLE EllipseImageBox(QObject* parent = nullptr);
	Q_INVOKABLE EllipseImageBox(
		const double& x,
		const double& y,
		const double& width,
		const double& height,
		const QPen& pen = QPen(QColor(0, 0, 255)),
		const QBrush& brush = QBrush(QColor(0, 0, 255, 0.2), Qt::BrushStyle::SolidPattern),
		const QString& name = "default",
		const int& id = -1,
		const bool& display = true,
		const bool& env = false,
		QObject* parent = nullptr);
	Q_INVOKABLE EllipseImageBox(const EllipseImageBox& other);
	void operator=(const EllipseImageBox& rb);
	~EllipseImageBox();
	Q_INVOKABLE virtual cv::Mat getMask(const QSize&) override;
protected:
	virtual void paintShape(QPainter*, ImageWidgetBasePrivate*) override;
};
Q_DECLARE_METATYPE(EllipseImageBox)

class IMAGEWIDGET_EXPORT PaintData
{
public:
	std::vector<std::tuple<cv::Point2d, cv::Point2d, int, cv::Scalar>> lines;
	std::vector<std::tuple<cv::Rect2d, int, cv::Scalar>> rects;
	std::vector<std::tuple<cv::Rect2d, int, cv::Scalar>> circles;
	std::vector<std::tuple<std::string,cv::Point2d,int,std::string,cv::Scalar>> texts;
	std::vector<std::tuple<cv::Point2d, int,int, cv::Scalar>> corss_lines;
	void drawDatas(cv::Mat& mat) const;
	void paintDatas(QPainter*, ImageWidgetBasePrivate*);
	PaintData& operator<<(PaintData&);
};
Q_DECLARE_METATYPE(PaintData)
Q_DECLARE_METATYPE(cv::Mat)
class IMAGEWIDGET_EXPORT ImageWidgetBase : public
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
	ImageWidgetBase(QWidget* parent = Q_NULLPTR);
#endif
	virtual ~ImageWidgetBase();
public slots:
	;
	void displayCVMat(cv::Mat);
	void displayQImage(const QImage&);
	void displayCVMatWithData(const cv::Mat&, const PaintData&);
	void displayQImageWithData(const QImage&, const PaintData&);
	void displayDoneCVMat(const cv::Mat&);
	void displayDoneQImage(const QImage&);
	void displayDoneCVMatWithData(const cv::Mat&, const PaintData&);
	void displayDoneQImageWithData(const QImage&, const PaintData&);
	void displayCVMat(QVariant);
	void displayQImage(const QVariant& img);
	void displayCVMatWithData(const QVariant& img, const QVariant& paint_data);
	void displayQImageWithData(const QVariant& img, const QVariant& paint_data);
	void displayDoneCVMat(const QVariant& img);
	void displayDoneQImage(const QVariant& img);
	void displayDoneCVMatWithData(const QVariant& img, const QVariant& paint_data);
	void displayDoneQImageWithData(const QVariant& img, const QVariant& paint_data);
	void resetScale();
	void setBackgroudColor(const QColor&);
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
	void onWidthChanged();
	void onHeightChanged();
#else
	virtual void resizeEvent(QResizeEvent*) override;
	virtual void paintEvent(QPaintEvent*) override;
#endif
	virtual void wheelEvent(QWheelEvent*) override;
	ImageWidgetBasePrivate* d;
};

class IMAGEWIDGET_EXPORT ImageWidget : public ImageWidgetBase
{
	Q_OBJECT
		Q_DISABLE_COPY(ImageWidget)
public:
#ifdef IMAGEWIDGET_QML
	ImageWidget(QQuickItem* parent = Q_NULLPTR);
#else
	ImageWidget(QWidget* parent = Q_NULLPTR);
#endif

	virtual ~ImageWidget();
public slots:
	void paintNewImageBox(QVariant box);
	void addImageBox(QVariant box);
	void paintNewImageBox(ImageBox* box);
	void addImageBox(ImageBox* box);
	void removeImageBoxById(const int& id);
	void removeImageBoxByName(const QString& name);
	ImageBox* getImageBoxFromId(const int& id);
	QVariant getImageBoxVarFromId(const int& id);
	QList<ImageBox*> ImageWidget::getImageBoxsFromName(const QString& name);
	QVariantList ImageWidget::getImageBoxVarlistFromName(const QString& name);
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
	ImageWidgetPrivate* d_ptr;
};

#ifdef IMAGEWIDGET_QML
QML_DECLARE_TYPE(ImageWidget)
QML_DECLARE_TYPE(ImageWidgetBase)
QML_DECLARE_TYPE(RectImageBox)
QML_DECLARE_TYPE(EllipseImageBox)
#endif
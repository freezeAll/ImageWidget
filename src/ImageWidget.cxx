#include "ImageWidget.hxx"
#include <QTimer>
#include <QMouseEvent>
#include <QLinkedList>
#include <QPainter>
#include <QMenu>

class ImageWidgetBasePrivate : public QObject
{
	Q_OBJECT
public:
	ImageWidgetBasePrivate(ImageWidgetBase* parent) :
		q_ptr(parent),
		done_flag(false),
		log_zoom(1.),
		moving(false)
	{
		connect(&done_timer, &QTimer::timeout, this, &ImageWidgetBasePrivate::doneImageTimerTimeout);
	}
	~ImageWidgetBasePrivate() {}
private:
	friend ImageWidgetBase;
	friend ImageWidget;
	ImageWidgetBase* q_ptr;
	cv::Mat rgb;
	cv::Mat rgb_done;
	QPixmap display_img;
	QPixmap display_img_done;
	QTimer done_timer;
	bool done_flag;
	double log_zoom;
	QPoint start_point;
	QPointF source_position;
	QSizeF source_size;
	bool moving;
	PaintData done_paint_data;
	PaintData paint_data;
public:
	QSize getImageSize()
	{
		if (done_flag)
		{
			return display_img_done.size();
		}
		else
		{
			return display_img.size();
		}
	}

	QImage cvMatToQImage(const cv::Mat &m, bool is_done = false)
	{
		if (is_done)
		{
			switch (m.channels())
			{
			case 1:
				cv::cvtColor(m, rgb_done, cv::COLOR_GRAY2RGB);
				break;
			case 3:
				cv::cvtColor(m, rgb_done, cv::COLOR_BGR2RGB);
				break;
			case 4:
				cv::cvtColor(m, rgb_done, cv::COLOR_BGRA2RGB);
				break;
			default:
				return QImage();
			}
			return QImage(rgb_done.data, rgb_done.cols, rgb_done.rows, rgb_done.step, QImage::Format::Format_RGB888);
		}
		else {
			switch (m.channels())
			{
			case 1:
				cv::cvtColor(m, rgb, cv::COLOR_GRAY2RGB);
				break;
			case 3:
				cv::cvtColor(m, rgb, cv::COLOR_BGR2RGB);
				break;
			case 4:
				cv::cvtColor(m, rgb, cv::COLOR_BGRA2RGB);
				break;
			default:
				return QImage();
			}
			return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format::Format_RGB888);
		}
	}

	void startDoneImageTimer(const int& ms = 2000)
	{
		done_timer.start(ms);
		done_flag = true;
	}

	void doneImageTimerTimeout()
	{
		done_flag = false;
		done_timer.stop();
		done_paint_data = PaintData();
		paint_data = PaintData();
		q_ptr->update();
	}
	double getAveragePower()
	{
		return (getXPower() + getYPower()) / 2.;
	}
	void zoomIn(QWheelEvent *e)
	{
		auto m = e->posF();

		float power;
		if (source_size.width() > source_size.height())
		{
			power = float(source_size.width()) / float(q_ptr->width());
		}
		else
		{
			power = float(source_size.height()) / float(q_ptr->height());
		}


		m.setX(m.x() * power); m.setY(m.y() * power);
		m *= 0.05;
		source_position += m;
		source_size *= 0.95;
		q_ptr->update();
	}
	void zoomOut(QWheelEvent *e)
	{
		auto m = e->posF();

		float power;
		if (source_size.width() > source_size.height())
		{
			power = float(source_size.width()) / float(q_ptr->width());
		}
		else
		{
			power = float(source_size.height()) / float(q_ptr->height());
		}

		m.setX(m.x() * power); m.setY(m.y() * power);
		m *= 0.05;

		source_position -= m;
		source_size /= 0.95;

		q_ptr->update();
	}

	double getXPower()
	{
		return source_size.width() / q_ptr->width();
	}

	double getYPower()
	{
		return source_size.height() / q_ptr->height();
	}

	void getMovedBox(ImageBox& box, const QPoint& start_point, const QPoint& end_point)
	{
		QPointF vec = end_point - start_point;
		vec.setX(vec.x() * (source_size.width() / q_ptr->width()));
		vec.setY(vec.y() * (source_size.height() / q_ptr->height()));
		if (box.type == ImageBox::Rect || box.type == ImageBox::Circle)
		{
			if ((box.x + vec.x()) <= (display_img.width() - box.width) && (box.x + vec.x() >= 0))
			{
				box.x += vec.x();
			}
			if ((box.y + vec.y()) <= (display_img.height() - box.height) && (box.y + vec.y() >= 0))
			{
				box.y += vec.y();
			}
		}
		else if(box.type == ImageBox::Line)
		{
			if ((box.x + vec.x()) <= display_img.width() && (box.x + vec.x() >= 0) && 
				(box.ex + vec.x()) <= display_img.width() && (box.ex + vec.x() >= 0))
			{
				box.x += vec.x();
				box.ex += vec.x();
			}
			if ((box.y + vec.y()) <= display_img.height()  && (box.y + vec.y() >= 0) && 
				(box.ey + vec.y()) <= display_img.height() && (box.ey + vec.y() >= 0))
			{
				box.y += vec.y();
				box.ey += vec.y();
			}
		}
		else if (box.type == ImageBox::Point)
		{
			if ((box.x + vec.x()) <= display_img.width() && (box.x + vec.x() >= 0))
			{
				box.x += vec.x();
			}
			if ((box.y + vec.y()) <= display_img.height() && (box.y + vec.y() >= 0))
			{
				box.y += vec.y();
			}
		}
	}

	template <typename T, typename Y>
	T getImageLine(const Y& rt)
	{
		auto p1 = getImagePosition<decltype(rt.p1())>(rt.p1());
		auto p2 = getImagePosition<decltype(rt.p2())>(rt.p2());
		T rtn;
		rtn.setP1(p1);
		rtn.setP2(p2);
		return rtn;
	}

	template <typename T, typename Y>
	T getPaintLine(const Y& rt)
	{
		auto p1 = getPaintPosition<decltype(rt.p1())>(rt.p1());
		auto p2 = getPaintPosition<decltype(rt.p2())>(rt.p2());
		T rtn;
		rtn.setP1(p1);
		rtn.setP2(p2);
		return rtn;
	}

	template <typename T,typename Y>
	T getImageRect(const Y& rt)
	{
		auto tl = getImagePosition<decltype(rt.topLeft())>( rt.topLeft());
		auto br = getImagePosition<decltype(rt.bottomRight())>(rt.bottomRight());
		T rtn;
		rtn.setTopLeft(tl);
		rtn.setBottomRight(br);
		return rtn;
	}

	template <typename T, typename Y>
	T getPaintRect(const Y& rt)
	{
		auto tl = getPaintPosition<decltype(rt.topLeft())>(rt.topLeft());
		auto br = getPaintPosition<decltype(rt.bottomRight())>(rt.bottomRight());
		T rtn;
		rtn.setTopLeft(tl);
		rtn.setBottomRight(br);
		return rtn;
	}

	template<typename T,typename Y>
	T getPaintPosition(const Y& rt)
	{
		auto tmp_img = &(done_flag ? display_img_done : display_img);
		float power(1.f);
		if (tmp_img->width() > tmp_img->height())
		{
			power = float(q_ptr->width()) / float(source_size.width());
		}
		else
		{
			power = float(q_ptr->height()) / float(source_size.height());
		}
		return T((float(rt.x())  - float(source_position.x())) * power,(float(rt.y())  - float(source_position.y())) * power);
	}

	template <typename T, typename Y>
	T getImagePosition(const Y& rt)
	{
		auto tmp_img = &(done_flag ? display_img_done : display_img);
		float power(1.f);
		if (tmp_img->width() > tmp_img->height())
		{
			power = float(source_size.width()) / float(q_ptr->width());
		}
		else
		{
			power = float(source_size.height()) / float(q_ptr->height());
		}
		return T(float(rt.x()) * power + source_position.x(), float(rt.y()) * power + source_position.y());
	}
};

ImageWidgetBase::ImageWidgetBase(
#ifdef IMAGEWIDGET_QML
	QQuickItem* parent)
	: QQuickPaintedItem(parent),
#else
	QWidget *parent)
	: QWidget(parent),
#endif // QIMAGEWIDGET_QML
	d(new ImageWidgetBasePrivate(this))
{
#ifdef IMAGEWIDGET_QML
	setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
#endif // IMAGEWIDGET_QML

}

ImageWidgetBase::~ImageWidgetBase()
{
	delete d;
}

void ImageWidgetBase::displayCVMat(const cv::Mat & img)
{
	if (d->display_img.size() != QSize(img.cols, img.rows))
	{
		QPoint src_pnt;
		QSize src_size;
		if (img.cols > img.rows)
		{
			float power = float(img.cols) / float(width());
			auto w = float(height()) * power;
			src_pnt.setY(-(w - float(img.rows)) / 2.);
			src_pnt.setX(0);
			src_size.setWidth(img.cols);
			src_size.setHeight(w);
		}
		else
		{
			float power = float(img.rows) / float(height());
			auto h = float(width()) * power;
			src_pnt.setY(0);
			src_pnt.setX(-(h - float(img.cols)) / 2.);
			src_size.setWidth(h);
			src_size.setHeight(img.rows);

		}
		d->source_position = src_pnt;
		d->source_size = src_size;
	}
	d->display_img = QPixmap::fromImage(d->cvMatToQImage(img));
	update();
}

void ImageWidgetBase::displayQImage(const QImage & img)
{
	if (d->display_img.size() != img.size())
	{
		QPoint src_pnt;
		QSize src_size;
		if (img.width() > img.height())
		{
			float power = float(img.width()) / float(width());
			auto w = float(height()) * power;
			src_pnt.setY(-(w - float(img.height())) / 2.);
			src_pnt.setX(0);
			src_size.setWidth(img.width());
			src_size.setHeight(w);
		}
		else
		{
			float power = float(img.height()) / float(height());
			auto h = float(width()) * power;
			src_pnt.setY(0);
			src_pnt.setX(-(h - float(img.width())) / 2.);
			src_size.setWidth(h);
			src_size.setHeight(img.height());

		}
		d->source_position = src_pnt;
		d->source_size = src_size;
	}

	d->display_img = QPixmap::fromImage(img.copy(QRect(0,0,img.width(),img.height())));
	update();
}

void ImageWidgetBase::displayCVMatWithData(const cv::Mat& img, const PaintData& data)
{
	d->paint_data = data;
	displayCVMat(img);
}

void ImageWidgetBase::displayQImageWithData(const QImage& img, const PaintData& data)
{
	d->paint_data = data;
	displayQImage(img);
}

void ImageWidgetBase::displayDoneCVMat(const cv::Mat & img)
{
	if (d->display_img_done.size() != QSize(img.cols, img.rows))
	{
		d->source_position = QPoint(0, 0);
		d->source_size = QSize(img.cols, img.rows);
	}
	d->display_img_done = QPixmap::fromImage(d->cvMatToQImage(img,true));
	d->startDoneImageTimer();
	update();
}

void ImageWidgetBase::displayDoneQImage(const QImage & img)
{
	if (d->display_img_done.size() != img.size())
	{
		d->source_position = QPoint(0, 0);
		d->source_size = img.size();
	}
	d->display_img_done = QPixmap::fromImage(img.copy(QRect(0, 0, img.width(), img.height())));
	d->startDoneImageTimer();
	update();
}

void ImageWidgetBase::displayDoneCVMatWithData(const cv::Mat& img, const PaintData& data)
{
	d->done_paint_data = data;
	displayCVMat(img);
}

void ImageWidgetBase::displayDoneQImageWithData(const QImage& img, const PaintData& data)
{
	d->done_paint_data = data;
	displayDoneQImage(img);
}

void ImageWidgetBase::displayCVMat(const QVariant& img)
{
	if (img.canConvert<cv::Mat>())
	{
		displayCVMat(img.value<cv::Mat>());
	}
}

void ImageWidgetBase::displayQImage(const QVariant& img)
{
	if (img.canConvert<QImage>())
	{
		displayQImage(img.value<QImage>());
	}
}

void ImageWidgetBase::displayCVMatWithData(const QVariant& img, const QVariant& paint_data)
{
	if (img.canConvert<cv::Mat>() && paint_data.canConvert<PaintData>())
	{
		displayCVMatWithData(img.value<cv::Mat>(), paint_data.value<PaintData>());
	}
}

void ImageWidgetBase::displayQImageWithData(const QVariant& img, const QVariant& paint_data)
{
	if (img.canConvert<QImage>() && paint_data.canConvert<PaintData>())
	{
		displayQImageWithData(img.value<QImage>(), paint_data.value<PaintData>());
	}
}

void ImageWidgetBase::displayDoneCVMat(const QVariant& img)
{
	if (img.canConvert<cv::Mat>())
	{
		displayDoneCVMat(img.value<cv::Mat>());
	}
}

void ImageWidgetBase::displayDoneQImage(const QVariant& img)
{
	if (img.canConvert<QImage>())
	{
		displayDoneQImage(img.value<QImage>());
	}
}

void ImageWidgetBase::displayDoneCVMatWithData(const QVariant& img, const QVariant& paint_data)
{
	if (img.canConvert<cv::Mat>() && paint_data.canConvert<PaintData>())
	{
		displayDoneCVMatWithData(img.value<cv::Mat>(), paint_data.value<PaintData>());
	}
}

void ImageWidgetBase::displayDoneQImageWithData(const QVariant& img, const QVariant& paint_data)
{
	if (img.canConvert<QImage>() && paint_data.canConvert<PaintData>())
	{
		displayDoneQImageWithData(img.value<QImage>(),paint_data.value<PaintData>());
	}
}

void ImageWidgetBase::mousePressEvent(QMouseEvent *e)
{
	d->moving = true;
	d->start_point = e->pos();
	update();
}

void ImageWidgetBase::mouseMoveEvent(QMouseEvent *e)
{
	if (!d->moving)
		return;
	auto m = e->pos();
	auto vec = m - d->start_point;
	vec.setX(vec.x() * (d->source_size.width() / width()));
	vec.setY(vec.y() * (d->source_size.height() / height()));
	d->source_position.setX(d->source_position.x() - vec.x());
	d->source_position.setY(d->source_position.y() - vec.y());
	d->start_point = m;
	update();
}

void ImageWidgetBase::mouseReleaseEvent(QMouseEvent *e)
{
	d->moving = false;
	emit clickedPosition(d->getImagePosition<QPoint>(e->pos()));
}

void ImageWidgetBase::mouseDoubleClickEvent(QMouseEvent* e)
{
	QPoint src_pnt;
	QSize src_size;

	auto tmp_img = &(d->done_flag ? d->display_img_done : d->display_img);
	if (tmp_img->width() > tmp_img->height())
	{
		float power = float(tmp_img->width()) / float(width());
		auto w = float(height()) * power;
		src_pnt.setY(-(w - float(tmp_img->height())) / 2.);
		src_pnt.setX(0);
		src_size.setWidth(tmp_img->width());
		src_size.setHeight(w);
	}
	else
	{
		float power = float(tmp_img->height()) / float(height());
		auto h = float(width()) * power;
		src_pnt.setY(0);
		src_pnt.setX(-(h - float(tmp_img->width())) / 2.);
		src_size.setWidth(h);
		src_size.setHeight(tmp_img->height());
	}
	d->source_position = src_pnt;
	d->source_size = src_size;
	update();
}

#ifdef IMAGEWIDGET_QML
void ImageWidgetBase::paint(QPainter* painter)
#else
void ImageWidgetBase::paintEvent(QPaintEvent* e)
#endif
{
#ifdef IMAGEWIDGET_QML
	QPainter* painter_ptr = painter;
#else
	QPainter painter_obj;
	QPainter* painter_ptr = &painter_obj;
	painter_ptr->begin(this);
#endif
	painter_ptr->setBrush(QBrush(QColor(125, 125, 125)));
	painter_ptr->setPen(QPen(QColor(125, 125, 125)));
	painter_ptr->drawRect(QRect(0, 0, width(),height()));
	auto tmp_img = &(d->done_flag ? d->display_img_done : d->display_img);

	painter_ptr->drawPixmap(QRectF(0, 0, width(), height()), *tmp_img, QRectF(d->source_position, d->source_size));
	(d->done_flag ? d->done_paint_data : d->paint_data).paintDatas(painter_ptr,d);
#ifndef IMAGEWIDGET_QML
	painter_ptr->end();
#endif
}

void ImageWidgetBase::wheelEvent(QWheelEvent * e)
{
	if (e->delta() > 0)
	{
		d->zoomIn(e);
	}
	else if(e->delta() < 0)
	{
		d->zoomOut(e);
	}
}

class ImageWidgetPrivate : public QObject
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
	ImageWidgetPrivate(ImageWidget* parent) :
		q_ptr(parent),
		grabed_box_ptr(nullptr),
		is_painting(false),
		grabed_obj(false),
		grabed_edge(false),
		grabedge_thresh(3)
	{

	}
	~ImageWidgetPrivate()
	{

	}
	void normalizeAllBox()
	{
		for (auto &a : box_list)
		{
			if (a.type == ImageBox::Point)
			{
				continue;
			}
			else if(a.type == ImageBox::Line)
			{
				if (a.ex < a.x)
				{
					std::swap(a.ex, a.x);
					std::swap(a.ey, a.y);
				}
			}
			else
			{
				a.fromQRect(a.toQRect().normalized());
			}
		}
	}
	void resetBoxEditing()
	{
		for (auto &a : box_list)
		{
			a.editing = false;
		}
	}

	void checkPress(const QPoint& p, const double& power)
	{
		if (box_list.empty())
			return;
		if (!box_list.begin()->editing)
			return;
		double thresh = grabedge_thresh * power;
		//点和线
		if (box_list.begin()->type == ImageBox::Point || box_list.begin()->type == ImageBox::Line)
		{
			QPointF judge_pnt1, judge_pnt2;
			switch (box_list.begin()->type)
			{
			case ImageBox::Point:
				judge_pnt1 = box_list.begin()->toQPoint();
				judge_pnt2 = judge_pnt1;
				break;
			case ImageBox::Line:
				judge_pnt1 = box_list.begin()->toQLine().p1();
				judge_pnt2 = box_list.begin()->toQLine().p2();
				break;
			default:
				break;
			}
			if (std::abs(judge_pnt1.x() - p.x()) <= thresh && std::abs(judge_pnt1.y() - p.y()) <= thresh)
			{
				grabed_type = TopLeft;
				grabed_edge = true;
			}
			if (std::abs(judge_pnt2.x() - p.x()) <= thresh && std::abs(judge_pnt2.y() - p.y()) <= thresh)
			{
				grabed_type = BottomRight;
				grabed_edge = true;
			}


			return;
		}
		//圆和矩形
		auto tmp_rt = box_list.begin()->toQRect();
		if (std::abs(tmp_rt.left() - p.x()) < thresh)
		{
			grabed_type = Left;
			grabed_edge = true;
		}
		if (std::abs(tmp_rt.right() - p.x()) < thresh)
		{
			grabed_type = Right;
			grabed_edge = true;
		}
		if (std::abs(tmp_rt.top() - p.y()) < thresh)
		{
			grabed_type = Top;
			grabed_edge = true;
		}
		if (std::abs(tmp_rt.bottom() - p.y()) < thresh)
		{
			grabed_type = Bottom;
			grabed_edge = true;
		}
		if (grabed_edge)
		{
			QPoint p1;
			QPoint p2;
			switch (grabed_type)
			{
			case ImageWidgetPrivate::Top:
				p1 = tmp_rt.topLeft() - p;
				p2 = tmp_rt.topRight() - p;
				if (std::abs(p1.x()) <= thresh && std::abs(p1.y()) <= thresh)
				{
					grabed_type = TopLeft;
				}
				if (std::abs(p2.x()) <= thresh && std::abs(p2.y()) <= thresh)
				{
					grabed_type = TopRight;
				}
				break;
			case ImageWidgetPrivate::Bottom:
				p1 = tmp_rt.topLeft() - p;
				p2 = tmp_rt.topRight() - p;
				if (std::abs(p1.x()) <= thresh && p1.y() <= thresh)
				{
					grabed_type = BottomLeft;
				}
				if (std::abs(p2.x()) <= thresh && p2.y() <= thresh)
				{
					grabed_type = BottomRight;
				}
				break;
			case ImageWidgetPrivate::Left:
				p1 = tmp_rt.topLeft() - p;
				p2 = tmp_rt.bottomLeft() - p;
				if (std::abs(p1.x()) <= thresh && std::abs(p1.y()) <= thresh)
				{
					grabed_type = TopLeft;
				}
				if (std::abs(p2.x()) <= thresh && std::abs(p2.y()) <= thresh)
				{
					grabed_type = BottomLeft;
				}
				break;
			case ImageWidgetPrivate::Right:
				p1 = tmp_rt.topLeft() - p;
				p2 = tmp_rt.bottomLeft() - p;
				if (std::abs(p1.x()) <= thresh && std::abs(p1.y()) <= thresh)
				{
					grabed_type = TopRight;
				}
				if (std::abs(p2.x()) <= thresh && std::abs(p2.y()) <= thresh)
				{
					grabed_type = BottomRight;
				}
				break;
			default:
				break;
			}
		}
	}
	bool checkMove(const QPoint& p, const double& power)
	{
		if (box_list.empty())
			return false;
		if (!box_list.begin()->editing)
			return false;
		bool undermouse(false);
		double thresh = grabedge_thresh * power;
		auto tmp_rt = box_list.begin()->toQRect();
		if (box_list.begin()->type == ImageBox::Point || box_list.begin()->type == ImageBox::Line)
		{
			QPoint judge_pnt1, judge_pnt2;
			switch (box_list.begin()->type)
			{
			case ImageBox::Point:
				judge_pnt1 = box_list.begin()->toQPoint();
				judge_pnt2 = judge_pnt1;
				break;
			case ImageBox::Line:
				judge_pnt1 = box_list.begin()->toQLine().p1();
				judge_pnt2 = box_list.begin()->toQLine().p2();
				break;
			default:
				break;
			}
			if (std::abs(judge_pnt1.x() - p.x()) <= thresh && std::abs(judge_pnt1.y() - p.y()) <= thresh)
			{
				grabed_type = TopLeft;
				undermouse = true;
			}
			if (std::abs(judge_pnt2.x() - p.x()) <= thresh && std::abs(judge_pnt2.y() - p.y()) <= thresh)
			{
				grabed_type = BottomRight;
				undermouse = true;
			}
			return undermouse;
		}

		if (abs(tmp_rt.left() - p.x()) < thresh)
		{
			undermouse = true;
			grabed_type = Left;
		}
		if (abs(tmp_rt.right() - p.x()) < thresh)
		{
			undermouse = true;
			grabed_type = Right;
		}
		if (abs(tmp_rt.top() - p.y()) < thresh)
		{
			undermouse = true;
			grabed_type = Top;
		}
		if (abs(tmp_rt.bottom() - p.y()) < thresh)
		{
			undermouse = true;
			grabed_type = Bottom;
		}
		QPoint p1;
		QPoint p2;
		if (undermouse)
		{
			switch (grabed_type)
			{
			case ImageWidgetPrivate::Top:
				/*auto tmp_tl*/p1 = tmp_rt.topLeft() - p;
				/*auto tmp_tr*/p2 = tmp_rt.topRight() - p;
				if (abs(p1.x()) <= thresh && abs(p1.y()) <= thresh)
				{
					grabed_type = TopLeft;
				}
				if (abs(p2.x()) <= thresh && abs(p2.y()) <= thresh)
				{
					grabed_type = TopRight;
				}
				break;
			case ImageWidgetPrivate::Bottom:
				/*auto tmp_bl*/p1 = tmp_rt.topLeft() - p;
				/*auto tmp_br*/p2 = tmp_rt.topRight() - p;
				if (abs(p1.x()) <= thresh && p1.y() <= thresh)
				{
					grabed_type = BottomLeft;
				}
				if (abs(p2.x()) <= thresh && p2.y() <= thresh)
				{
					grabed_type = BottomRight;
				}
				break;
			case ImageWidgetPrivate::Left:
				/*auto tmp_tl*/p1 = tmp_rt.topLeft() - p;
				/*auto tmp_bl*/p2 = tmp_rt.bottomLeft() - p;
				if (abs(p1.x()) <= thresh && abs(p1.y()) <= thresh)
				{
					grabed_type = TopLeft;
				}
				if (abs(p2.x()) <= thresh && abs(p2.y()) <= thresh)
				{
					grabed_type = BottomLeft;
				}
				break;
			case ImageWidgetPrivate::Right:
				/*auto tmp_tr*/p1 = tmp_rt.topLeft() - p;
				/*auto tmp_br*/p2 = tmp_rt.bottomLeft() - p;
				if (abs(p1.x()) <= thresh && abs(p1.y()) <= thresh)
				{
					grabed_type = TopRight;
				}
				if (abs(p2.x()) <= thresh && abs(p2.y()) <= thresh)
				{
					grabed_type = BottomRight;
				}
				break;
			default:
				break;
			}
		}
		return undermouse;
	}

private:
	friend ImageWidget;
	QLinkedList<ImageBox> box_list;
	ImageWidget* q_ptr;
	bool is_painting;
	ImageBox* grabed_box_ptr;
	bool grabed_obj;
	QPoint start_point;
	bool grabed_edge;
	const int grabedge_thresh;
	GrabedEdgeType grabed_type;
	ImageBox new_box_tmp;
};

#ifdef IMAGEWIDGET_QML
ImageWidget::ImageWidget(QQuickItem* parent)
#else
ImageWidget::ImageWidget(QWidget *parent)
#endif // IMAGEWIDGET_QML
	: ImageWidgetBase(parent),
	d_ptr(new ImageWidgetPrivate(this))
{
#ifdef IMAGEWIDGET_QML
	setKeepMouseGrab(false);
#else
	setMouseTracking(true);
#endif
}


ImageWidget::~ImageWidget()
{
	delete d_ptr;
}

void ImageWidget::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::MouseButton::LeftButton)
	{
		//正在画图
		if (d_ptr->is_painting)
		{
			d_ptr->box_list.push_front(d_ptr->new_box_tmp);
			d_ptr->grabed_box_ptr = &(*d_ptr->box_list.begin());
			d_ptr->grabed_box_ptr->editing = true;
			auto start_point = d->getImagePosition<QPointF>(e->pos());
			d_ptr->grabed_box_ptr->x = start_point.x();
			d_ptr->grabed_box_ptr->y = start_point.y();
			update();
			return;
		}
		auto pos = d->getImagePosition<QPoint>(e->pos());

		d_ptr->checkPress(pos,d->getAveragePower());
		if (d_ptr->grabed_edge)
		{
			update();
			return;
		}
		bool flag(false);
		decltype(d_ptr->box_list.begin()) iter(d_ptr->box_list.begin());
		for (auto &a : d_ptr->box_list)
		{
			if (!a.display)
			{
				iter++;
				continue;
			}
			//auto pos = d->getImagePosition(e->pos());
			if (a.isInBox(pos))
			{
				auto tmp = *iter;
				d_ptr->box_list.erase(iter);
				d_ptr->box_list.push_front(tmp);
				d_ptr->grabed_box_ptr = &(*d_ptr->box_list.begin());
				d_ptr->resetBoxEditing();
				d_ptr->grabed_box_ptr->editing = true;
				d_ptr->start_point = e->pos();
				flag = true;
				break;
			}
			iter++;
		}
		if (d_ptr->grabed_box_ptr == nullptr)
		{
			ImageWidgetBase::mousePressEvent(e);
			return;
		}
	}
}

void ImageWidget::mouseMoveEvent(QMouseEvent *e)
{
	if (d_ptr->grabed_edge)
	{
		auto m = d->getImagePosition<QPointF>(e->pos());
		if (m.x() < 0)
		{
			m.setX(0);
		}
		if (m.y() < 0)
		{
			m.setY(0);
		}
		if (m.x() > d->getImageSize().width() || m.x() > d->getImageSize().width())
		{
			m.setX(d->getImageSize().width());
		}
		if (m.y() > d->getImageSize().height() || m.y() > d->getImageSize().height())
		{
			m.setY(d->getImageSize().height());
		}
		auto box_ptr = d_ptr->box_list.begin();
		QRect new_rt;
		switch (d_ptr->grabed_type)
		{
		case ImageWidgetPrivate::Left:
			new_rt = box_ptr->toQRect();
			new_rt.setLeft(m.x());
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::Right:
			new_rt = box_ptr->toQRect();
			new_rt.setRight(m.x());
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::Top:
			new_rt = box_ptr->toQRect();
			new_rt.setTop(m.y());
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::Bottom:
			new_rt = box_ptr->toQRect();
			new_rt.setBottom(m.y());
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::TopLeft:
			if (box_ptr->type == ImageBox::Line || box_ptr->type == ImageBox::Point)
			{
				box_ptr->x = m.x();
				box_ptr->y = m.y();
			}
			else
			{
				new_rt = box_ptr->toQRect();
				new_rt.setTopLeft(QPoint(m.x(),m.y()));
				box_ptr->fromQRect(new_rt);
			}
			break;
		case ImageWidgetPrivate::TopRight:
			new_rt = box_ptr->toQRect();
			new_rt.setTopRight(QPoint(m.x(), m.y()));
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::BottomLeft:
			new_rt = box_ptr->toQRect();
			new_rt.setBottomLeft(QPoint(m.x(), m.y()));
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::BottomRight:
			if (box_ptr->type == ImageBox::Line )
			{
				box_ptr->ex = m.x();
				box_ptr->ey = m.y();
			}
			else if (box_ptr->type == ImageBox::Point)
			{
				box_ptr->x = m.x();
				box_ptr->y = m.y();
			}
			else
			{
				new_rt = box_ptr->toQRect();
				new_rt.setBottomRight(QPoint(m.x(), m.y()));
				box_ptr->fromQRect(new_rt);
			}
			break;
		default:
			break;
		}

		update();
	}
#ifndef IMAGEWIDGET_QML


	if (d_ptr->checkMove(d->getImagePosition<QPoint>(e->pos()), d->getAveragePower()))
	{
		switch (d_ptr->grabed_type)
		{
		case ImageWidgetPrivate::Left:
			setCursor(Qt::SizeHorCursor);
			break;
		case ImageWidgetPrivate::Right:
			setCursor(Qt::SizeHorCursor);
			break;
		case ImageWidgetPrivate::Top:
			setCursor(Qt::SizeVerCursor);
			break;
		case ImageWidgetPrivate::Bottom:
			setCursor(Qt::SizeVerCursor);
			break;
		case ImageWidgetPrivate::TopLeft:
			setCursor(Qt::SizeFDiagCursor);
			break;
		case ImageWidgetPrivate::TopRight:
			setCursor(Qt::SizeBDiagCursor);
			break;
		case ImageWidgetPrivate::BottomLeft:
			setCursor(Qt::SizeBDiagCursor);
			break;
		case ImageWidgetPrivate::BottomRight:
			setCursor(Qt::SizeFDiagCursor);
			break;
		default:
			break;
		}
	}
	else if(d_ptr->box_list.begin() != d_ptr->box_list.end())
	{
		if (d_ptr->box_list.begin()->isInBox(d->getImagePosition<QPoint>(e->pos())))
		{
			setCursor(Qt::SizeAllCursor);
		}
		else
		{
			setCursor(Qt::ArrowCursor);
		}
	}
	else
	{
		setCursor(Qt::ArrowCursor);
	}
#endif // IMAGEWIDGET_QML

	if (!d_ptr->grabed_box_ptr)
	{
		ImageWidgetBase::mouseMoveEvent(e);
		return;
	}

	if (d_ptr->is_painting)
	{
		auto m = d->getImagePosition<QPointF>(e->pos());
		switch (d_ptr->grabed_box_ptr->type)
		{
		case ImageBox::Rect:
			d_ptr->grabed_box_ptr->width = m.x() - d_ptr->grabed_box_ptr->x;
			d_ptr->grabed_box_ptr->height = m.y() - d_ptr->grabed_box_ptr->y;

			break;
		case ImageBox::Point:
			d_ptr->grabed_box_ptr->x = m.x();
			d_ptr->grabed_box_ptr->y = m.y();
			break;
		case ImageBox::Circle:
			d_ptr->grabed_box_ptr->width = m.x() - d_ptr->grabed_box_ptr->x;
			d_ptr->grabed_box_ptr->height = m.y() - d_ptr->grabed_box_ptr->y;
			break;
		case ImageBox::Line:
			d_ptr->grabed_box_ptr->ex = m.x();
			d_ptr->grabed_box_ptr->ey = m.y();
			break;
		default:
			break;
		}
		if (d_ptr->grabed_box_ptr->type == ImageBox::Circle || d_ptr->grabed_box_ptr->type == ImageBox::Rect)
		{
			if ((d_ptr->grabed_box_ptr->x + d_ptr->grabed_box_ptr->width) > d->getImageSize().width())
			{
				d_ptr->grabed_box_ptr->width = d->getImageSize().width() - d_ptr->grabed_box_ptr->x;
			}
			if ((d_ptr->grabed_box_ptr->x + d_ptr->grabed_box_ptr->width) < 0)
			{
				d_ptr->grabed_box_ptr->width = -d_ptr->grabed_box_ptr->x;
			}
			if ((d_ptr->grabed_box_ptr->y + d_ptr->grabed_box_ptr->height) > d->getImageSize().height())
			{
				d_ptr->grabed_box_ptr->height = d->getImageSize().height() - d_ptr->grabed_box_ptr->y;
			}
			if ((d_ptr->grabed_box_ptr->y + d_ptr->grabed_box_ptr->height) < 0)
			{
				d_ptr->grabed_box_ptr->height = -d_ptr->grabed_box_ptr->y;
			}
		}
		else if(d_ptr->grabed_box_ptr->type == ImageBox::Point || d_ptr->grabed_box_ptr->type == ImageBox::Line)
		{
			if (m.x() > d->getImageSize().width())
			{
				switch (d_ptr->grabed_box_ptr->type)
				{
				case ImageBox::Point:
					d_ptr->grabed_box_ptr->x = d->getImageSize().width();
					break;		
				case ImageBox::Line:
					d_ptr->grabed_box_ptr->ex = d->getImageSize().width();
					break;
				default:
					break;
				}
			}
			if (m.x() < 0)
			{
				switch (d_ptr->grabed_box_ptr->type)
				{
				case ImageBox::Point:
					d_ptr->grabed_box_ptr->x = 0;
					break;
				case ImageBox::Line:
					d_ptr->grabed_box_ptr->ex = 0;
					break;
				default:
					break;
				}
			}
			if (m.x() > d->getImageSize().height())
			{
				switch (d_ptr->grabed_box_ptr->type)
				{
				case ImageBox::Point:
					d_ptr->grabed_box_ptr->y = d->getImageSize().height();
					break;
				case ImageBox::Line:
					d_ptr->grabed_box_ptr->ey = d->getImageSize().height();
					break;
				default:
					break;
				}
			}
			if (m.y() < 0)
			{
				switch (d_ptr->grabed_box_ptr->type)
				{
				case ImageBox::Point:
					d_ptr->grabed_box_ptr->y = 0;
					break;
				case ImageBox::Line:
					d_ptr->grabed_box_ptr->ey = 0;
					break;
				default:
					break;
				}
			}
		}
	}
	else
	{
		auto m = e->pos();
		d->getMovedBox(*d_ptr->grabed_box_ptr, d_ptr->start_point, m);
		d_ptr->start_point = m;
	}
	update();
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *e)
{
	if (d->moving)
	{
		d_ptr->resetBoxEditing();
	}
	ImageWidgetBase::mouseReleaseEvent(e);
	d_ptr->normalizeAllBox();
	if (d_ptr->is_painting)
	{
		for (auto iter = d_ptr->box_list.begin() + 1; iter != d_ptr->box_list.end(); iter++)
		{
			iter->editing = false;
		}
	}
	d_ptr->is_painting = false;
	d_ptr->grabed_box_ptr = nullptr;
	d_ptr->grabed_edge = false;
	update();
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
	ImageWidgetBase::mouseDoubleClickEvent(e);
}

#ifdef IMAGEWIDGET_QML
void ImageWidget::paint(QPainter* painter)
#else
void ImageWidget::paintEvent(QPaintEvent* e)
#endif
{
#ifdef IMAGEWIDGET_QML
	ImageWidgetBase::paint(painter);
	QPainter* painter_ptr = painter;
#else
	ImageWidgetBase::paintEvent(e);
	QPainter painter_obj;
	QPainter* painter_ptr = &painter_obj;
	painter_ptr->begin(this);
#endif
	for (auto &a : d_ptr->box_list)
	{
		if (!a.display)
			continue;
		QPen pen;
		QBrush brush;
		brush.setStyle(a.env ? Qt::BrushStyle::FDiagPattern : Qt::BrushStyle::NoBrush);
		brush.setColor(a.toQColor());
		pen.setStyle(a.editing ? Qt::PenStyle::DashLine : Qt::PenStyle::SolidLine);
		pen.setColor(a.toQColor());
		painter_ptr->setBrush(brush);
		painter_ptr->setPen(pen);
		switch (a.type)
		{
		case ImageBox::Rect:
			painter_ptr->drawRect(d->getPaintRect<QRect>(a.toQRect()));
			break;
		case ImageBox::Circle:
		{
			auto c = d->getPaintRect<QRectF>(a.toQRect());
			painter_ptr->drawEllipse(c);
			painter_ptr->drawRect(c);
		}
			break;
		case ImageBox::Point:
		{
			brush.setStyle(Qt::SolidPattern);
			painter_ptr->setBrush(brush);
			auto pnt = d->getPaintPosition<QPoint>(a.toQPoint());
			painter_ptr->drawEllipse(pnt.x() - 3,pnt.y() - 3,6,6);
		}
		break;
		case ImageBox::Line:
		{
			brush.setStyle(Qt::SolidPattern);
			painter_ptr->setBrush(brush);
			auto line = a.toQLine();
			auto p1 = d->getPaintPosition<QPointF>(line.p1());
			auto p2 = d->getPaintPosition<QPointF>(line.p2());
			painter_ptr->drawEllipse(p1.x() - 3, p1.y() - 3, 6, 6);
			painter_ptr->drawEllipse(p2.x() - 3, p2.y() - 3, 6, 6);
			painter_ptr->drawLine(d->getPaintLine<QLine>(a.toQLine()));
		}
		break;
		default:
			break;
		}
	}
#ifndef IMAGEWIDGET_QML
	painter_ptr->end();
#endif
}

void ImageWidget::wheelEvent(QWheelEvent *e)
{
	ImageWidgetBase::wheelEvent(e);
	update();
}
#ifndef IMAGEWIDGET_QML
void ImageWidget::contextMenuEvent(QContextMenuEvent *e)
{

	QMenu menu;

	decltype(d_ptr->box_list.begin()) selected_iter(d_ptr->box_list.end());
	for (auto iter = d_ptr->box_list.begin();iter != d_ptr->box_list.end();iter++)
	{
		auto pos = d->getImagePosition<QPoint>(e->pos());
		if (iter->isInBox(pos))
		{
			selected_iter = iter;
			break;
		}
	}
	if (selected_iter != d_ptr->box_list.end())
	{
		auto new_box = *selected_iter;
		new_box.editing = true;
		d_ptr->box_list.erase(selected_iter);
		d_ptr->box_list.push_front(new_box);
		for (auto iter = d_ptr->box_list.begin() + 1; iter != d_ptr->box_list.end(); iter++)
		{
			iter->editing = false;
		}
		update();
		selected_iter = d_ptr->box_list.begin();
		QAction env_action(u8"反向");
		env_action.setCheckable(true);
		env_action.setChecked(selected_iter->env);
		connect(&env_action, &QAction::triggered, [this, &selected_iter,&env_action]() {
			selected_iter->env = env_action.isChecked();
			update();
		});
		menu.addAction(&env_action);
		menu.exec(e->globalPos());
		return;
	}
#ifdef TEST
	QAction paint_action(u8"创建选框");
	connect(&paint_action, &QAction::triggered, [this]() {
		d_ptr->new_box_tmp = ImageBox();
		d_ptr->is_painting = true;
	});
	menu.addAction(&paint_action);
#else
#endif // TEST
	menu.exec(e->globalPos());
	return;
}
#endif
bool ImageWidget::addImageBox(const ImageBox & box)
{
	bool flag(false);
	for (auto &a : d_ptr->box_list)
	{
		if (a.id == -1)
			continue;
		if (a.id == box.id)
		{
			flag = true;
			break;
		}
	}
	if (flag)
	{
		return false;
	}
	d_ptr->box_list.push_front(box);
	update();
	return true;
}

bool ImageWidget::paintNewImageBox(const ImageBox::ShapeType& type, const QString & name, const QColor & color, const int & id, const bool & display)
{
	bool flag(false);
	for (auto &a : d_ptr->box_list)
	{
		if (a.id == -1)
			continue;
		if (a.id == id)
		{
			flag = true;
			break;
		}
	}
	if (flag)
	{
		return false;
	}
	d_ptr->is_painting = true;
	d_ptr->new_box_tmp = ImageBox(type,name, 0,0,0,0, color, id, display);
	return true;
}

bool ImageWidget::addImageBox(const ImageBox::ShapeType& type,const QString & name, const int& ix1, const int& iy1, const int& ix2, const int& iy2, const QColor & color, const int & id, const bool & display)
{
	bool flag(false);
	for (auto &a : d_ptr->box_list)
	{
		if (a.id == -1)
			continue;
		if (a.id == id)
		{
			flag = true;
			break;
		}
	}
	if (flag)
	{
		return false;
	}
	d_ptr->box_list.push_front(ImageBox(type,name, ix1,iy1,ix2,iy2, color, id, display));
	update();
	return true;
}

bool ImageWidget::getImageBoxFromId(const int & id, ImageBox & rtn)
{
	ImageBox* found_ptr(nullptr);
	for (auto &a : d_ptr->box_list)
	{
		if (a.id == -1)
			continue;
		if (a.id == id)
		{
			found_ptr = &a;
			break;
		}
	}
	if (found_ptr)
	{
		rtn = *found_ptr;
		rtn.editing = false;
		return true;
	}
	return false;
}

std::vector<ImageBox> ImageWidget::getImageBoxsFromName(const QString & name)
{
	std::vector<ImageBox> out;
	for (auto &a : d_ptr->box_list)
	{
		if (a.name == name)
		{
			if (a.editing = true)
			{
				ImageBox b = a;
				b.editing = false;
				out.push_back(b);
			}
			else
				out.push_back(a);
		}

	}
	return out;
}

void ImageWidget::clearAllBoxs()
{
	d_ptr->box_list.clear();
	update();
}

#include "ImageWidget.moc"

void PaintData::paintDatas(QPainter* painter, ImageWidgetBasePrivate* d_ptr)
{
	for (const auto& line : lines)
	{
		QLineF paint_line;
		const auto& [p1, p2, thinkness, color] = line;
		auto pen_color = QColor(color[2], color[1], color[0]);

		paint_line.setP1(d_ptr->getPaintPosition<QPoint>(QPoint(p1.x,p1.y)));
		paint_line.setP2(d_ptr->getPaintPosition<QPoint>(QPoint(p2.x, p2.y)));

		QPen pen(pen_color);
		pen.setWidth(thinkness);
		painter->setPen(pen);
		painter->drawLine(paint_line);
	}
	for (const auto& rect : rects)
	{
		QRectF paint_rect;
		const auto& [cvr, thinkness, color] = rect;
		auto pen_color = QColor(color[2], color[1], color[0]);
		paint_rect.setTopLeft(d_ptr->getPaintPosition<QPoint>(QPoint(cvr.x, cvr.y)));
		paint_rect.setBottomRight(d_ptr->getPaintPosition<QPoint>(QPoint(cvr.br().x, cvr.br().y)));
		QPen pen(pen_color);
		QBrush brush(pen_color);
		brush.setStyle(thinkness < 0 ? Qt::SolidPattern : Qt::NoBrush);
		if(thinkness > 0)
			pen.setWidth(thinkness);
		painter->setPen(pen);
		painter->setBrush(brush);
		painter->drawRect(paint_rect);
	}
	for (const auto& rect : circles)
	{
		QRectF paint_rect;
		const auto& [cvr, thinkness, color] = rect;
		auto pen_color = QColor(color[2], color[1], color[0]);
		paint_rect.setTopLeft(d_ptr->getPaintPosition<QPoint>(QPoint(cvr.x, cvr.y)));
		paint_rect.setBottomRight(d_ptr->getPaintPosition<QPoint>(QPoint(cvr.br().x, cvr.br().y)));
		QPen pen(pen_color);
		pen.setStyle(Qt::PenStyle::SolidLine);
		QBrush brush(pen_color);
		brush.setStyle(thinkness < 0 ? Qt::SolidPattern : Qt::NoBrush);
		if (brush.style() == Qt::NoBrush)
			pen.setWidth(thinkness);

		painter->setPen(pen);
		painter->setBrush(brush);
		painter->drawEllipse(paint_rect);
	}
}

void PaintData::drawDatas(cv::Mat& mat)
{
	for (const auto& line : lines)
	{
		cv::line(mat, std::get<0>(line), std::get<1>(line), std::get<3>(line), std::get<2>(line));
	}
	for (const auto& rect : rects)
	{
		cv::rectangle(mat, std::get<0>(rect), std::get<2>(rect), std::get<1>(rect));
	}
	for (const auto& circle : circles)
	{
		cv::RotatedRect rect(std::get<0>(circle).tl(), std::get<0>(circle).tl() + cv::Point2d(std::get<0>(circle).width), std::get<0>(circle).br());
		cv::ellipse(mat, rect, std::get<2>(circle), std::get<1>(circle));
	}

}
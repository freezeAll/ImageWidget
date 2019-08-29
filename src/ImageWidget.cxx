#include "ImageWidget.hxx"
#include <QTimer>
#include <QMouseEvent>
#include <QLinkedList>
#include <QPainter>
#include <QMenu>

enum ImageWidgetMode : qint8
{
	Normal
};

class ImageWidgetBasePrivate : public QObject
{
	Q_OBJECT
public:
	ImageWidgetBasePrivate(ImageWidgetBase* parent) :
		q_ptr(parent),
		mode(Normal),
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
	ImageWidgetMode mode;
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

	void switchMode(const ImageWidgetMode&)
	{

	}
	double getAveragePower()
	{
		return (getXPower() + getYPower()) / 2.;
	}
	void zoomIn(QWheelEvent *e)
	{
		auto m = e->posF();

		float vx = source_size.width() / q_ptr->width();
		float vy = source_size.height() / q_ptr->height();

		m.setX(m.x() * vx); m.setY(m.y() * vy);
		m *= 0.05;
		source_position += m;
		source_size *= 0.95;
		log_zoom *= 0.95;
		q_ptr->update();
	}
	void zoomOut(QWheelEvent *e)
	{
		auto m = e->posF();

		float vx = source_size.width() / q_ptr->width();
		float vy = source_size.height() / q_ptr->height();

		m.setX(m.x() * vx); m.setY(m.y() * vy);
		m *= 0.05;

		source_position -= m;

		if (log_zoom < 1.0)
		{
			source_size /= 0.95;
			log_zoom /= 0.95;
		}
		if (log_zoom >= 1.0)
		{
			source_position.setX(0);
			source_position.setY(0);
		}
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
		auto vec = end_point - start_point;
		vec.setX(vec.x() * (source_size.width() / q_ptr->width()));
		vec.setY(vec.y() * (source_size.height() / q_ptr->height()));

		if ((box.x + vec.x()) <= (display_img.width() - box.width) && (box.x + vec.x() >= 0.0))
		{
			box.x += vec.x();
		}
		if ((box.y + vec.y()) <= (display_img.height() - box.height) && (box.y + vec.y() >= 0.0))
		{
			box.y += vec.y();
		}
	}

	QRectF getPaintRect(const QRect& rt)
	{
		float x_power, y_power;
		QPointF new_pos;
		x_power = float(q_ptr->width()) / float(source_size.width());
		y_power = float(q_ptr->height()) / float(source_size.height());
		new_pos = QPointF(rt.x(), rt.y()) - source_position;
		return QRectF(new_pos.x() * x_power, new_pos.y() * y_power, rt.width() * x_power, rt.height() * y_power);
	}

	QRect getImageRect(const QRect& rt)
	{
		float x_power, y_power;
		QPointF new_pos;
		x_power = float(source_size.width()) / float(q_ptr->width());
		y_power = float(source_size.height()) / float(q_ptr->height());
		new_pos = QPointF(float(rt.x()) * x_power, float(rt.y()) * y_power) - source_position;
		return QRect(new_pos.x() * x_power, new_pos.y() * y_power, rt.width() * x_power, rt.height() * y_power);
	}
	QPointF getPaintPosition(const QPoint& rt)
	{
		float x_power, y_power;
		QPointF new_pos;
		x_power = float(q_ptr->width()) / float(source_size.width());
		y_power = float(q_ptr->height()) / float(source_size.height()) ;
		new_pos = QPointF(rt.x(), rt.y()) - source_position;
		return QPointF(new_pos.x() * x_power, new_pos.y() * y_power);
	}

	QPoint getImagePosition(const QPoint& rt)
	{
		float x_power, y_power;
		QPointF new_pos;
 		x_power = float(source_size.width()) / float(q_ptr->width());
		y_power = float(source_size.height()) / float(q_ptr->height());
		new_pos = QPointF(float(rt.x()) * x_power, float(rt.y()) * y_power)  + source_position;
		return QPoint(new_pos.x(), new_pos.y());
	}
};

ImageWidgetBase::ImageWidgetBase(QWidget *parent)
	: QWidget(parent),
	d(new ImageWidgetBasePrivate(this))
{
}

ImageWidgetBase::~ImageWidgetBase()
{
	delete d;
}

void ImageWidgetBase::displayCVMat(const cv::Mat & img)
{
	if (d->display_img.size() != QSize(img.cols, img.rows))
	{
		d->source_position = QPoint(0, 0);
		d->source_size = QSize(img.cols, img.rows);
	}
	d->display_img = QPixmap::fromImage(d->cvMatToQImage(img));
	update();
}

void ImageWidgetBase::displayQImage(const QImage & img)
{
	if (d->display_img.size() != img.size())
	{
		d->source_position = QPoint(0, 0);
		d->source_size = img.size();
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
	if ((d->source_position.x() - vec.x()) <= (d->display_img.width() - d->source_size.width()) && (d->source_position.x() - vec.x() >= 0.0))
	{
		d->source_position.setX(d->source_position.x() - vec.x());
	}
	if ((d->source_position.y() - vec.y()) <= (d->display_img.height() - d->source_size.height()) && (d->source_position.y() - vec.y() >= 0.0))
	{
		d->source_position.setY(d->source_position.y() - vec.y());
	}
	d->start_point = m;
	update();
}

void ImageWidgetBase::mouseReleaseEvent(QMouseEvent *e)
{
	d->moving = false;
	emit clickedPosition(d->getImagePosition(e->pos()));
}

void ImageWidgetBase::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
	if (d->done_flag)
	{
		painter.drawPixmap(QRectF(0, 0, width(), height()), d->display_img_done, QRectF(d->source_position, d->source_size));
		d->done_paint_data.paintDatas(&painter,d);
	}
	else
	{
		painter.drawPixmap(QRectF(0, 0, width(), height()), d->display_img, QRectF(d->source_position, d->source_size));
		d->paint_data.paintDatas(&painter, d);
	}
	painter.end();
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
			a.fromQRect(a.toQRect().normalized());
		}
	}
	void resetBoxEditing()
	{
		for (auto &a : box_list)
		{
			a.editing = false;
		}
	}

	void checkPress(const QPoint& p,const double& power)
	{
		if (box_list.empty())
			return;
		if (!box_list.begin()->editing)
			return;
		double thresh = grabedge_thresh * power;
		auto tmp_rt = box_list.begin()->toQRect();
		if (abs(tmp_rt.left() - p.x()) < thresh)
		{
			grabed_type = Left;
			grabed_edge = true;
		}
		if (abs(tmp_rt.right() - p.x()) < thresh)
		{
			grabed_type = Right;
			grabed_edge = true;
		}
		if (abs(tmp_rt.top() - p.y()) < thresh)
		{
			grabed_type = Top;
			grabed_edge = true;
		}
		if (abs(tmp_rt.bottom() - p.y()) < thresh)
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

ImageWidget::ImageWidget(QWidget *parent)
	: ImageWidgetBase(parent),
	d_ptr(new ImageWidgetPrivate(this))
{
	setMouseTracking(true);
}


ImageWidget::~ImageWidget()
{
	delete d_ptr;
}

void ImageWidget::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::MouseButton::LeftButton)
	{
		if (d_ptr->is_painting)
		{
			d_ptr->box_list.push_front(d_ptr->new_box_tmp);
			d_ptr->grabed_box_ptr = &(*d_ptr->box_list.begin());
			d_ptr->grabed_box_ptr->editing = true;
			auto start_point = d->getImagePosition(e->pos());
			d_ptr->grabed_box_ptr->x = start_point.x();
			d_ptr->grabed_box_ptr->y = start_point.y();
			update();
			return;
		}
		auto pos = d->getImagePosition(e->pos());
		d_ptr->checkPress(pos,d->getAveragePower());
		if (d_ptr->grabed_edge)
		{
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
		auto m = d->getImagePosition(e->pos());
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
			new_rt = box_ptr->toQRect();
			new_rt.setTopLeft(m);
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::TopRight:
			new_rt = box_ptr->toQRect();
			new_rt.setTopRight(m);
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::BottomLeft:
			new_rt = box_ptr->toQRect();
			new_rt.setBottomLeft(m);
			box_ptr->fromQRect(new_rt);
			break;
		case ImageWidgetPrivate::BottomRight:
			new_rt = box_ptr->toQRect();
			new_rt.setBottomRight(m);
			box_ptr->fromQRect(new_rt);
			break;
		default:
			break;
		}
		update();
	}
	if (d_ptr->checkMove(d->getImagePosition(e->pos()), d->getAveragePower()))
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
		if (d_ptr->box_list.begin()->isInBox(d->getImagePosition(e->pos())))
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


	if (d_ptr->grabed_box_ptr == nullptr )
	{
		ImageWidgetBase::mouseMoveEvent(e);
		return;
	}

	if (d_ptr->is_painting)
	{
		auto m = d->getImagePosition(e->pos());
		d_ptr->grabed_box_ptr->width = m.x() - d_ptr->grabed_box_ptr->x;
		d_ptr->grabed_box_ptr->height = m.y() - d_ptr->grabed_box_ptr->y;
		if ( (d_ptr->grabed_box_ptr->x + d_ptr->grabed_box_ptr->width)  > d->getImageSize().width())
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

void ImageWidget::paintEvent(QPaintEvent *e)
{
	ImageWidgetBase::paintEvent(e);
	QPainter painter;
	painter.begin(this);
	for (auto &a : d_ptr->box_list)
	{
		if (!a.display)
			continue;
		QPen pen;
		QBrush brush;
		brush.setStyle(a.isEnv ? Qt::BrushStyle::FDiagPattern : Qt::BrushStyle::NoBrush);
		brush.setColor(a.toQColor());
		pen.setStyle(a.editing ? Qt::PenStyle::DashLine : Qt::PenStyle::SolidLine);
		pen.setColor(a.toQColor());
		painter.setBrush(brush);
		painter.setPen(pen);
		painter.drawRect(d->getPaintRect(a.toQRect()));
	}
	painter.end();
}

void ImageWidget::wheelEvent(QWheelEvent *e)
{
	ImageWidgetBase::wheelEvent(e);
	update();
}

void ImageWidget::contextMenuEvent(QContextMenuEvent *e)
{

	QMenu menu;

	decltype(d_ptr->box_list.begin()) selected_iter(d_ptr->box_list.end());
	for (auto iter = d_ptr->box_list.begin();iter != d_ptr->box_list.end();iter++)
	{
		auto pos = d->getImagePosition(e->pos());
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
		env_action.setChecked(selected_iter->isEnv);
		connect(&env_action, &QAction::triggered, [this, &selected_iter,&env_action]() {
			selected_iter->isEnv = env_action.isChecked();
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

bool ImageWidget::paintNewImageBox(const std::string & name, const QColor & color, const int & id, const bool & display)
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
	d_ptr->new_box_tmp = ImageBox(name, QRect(0,0,0,0), color, id, display);
	return true;
}

bool ImageWidget::addImageBox(const std::string & name, const QRect & rect, const QColor & color, const int & id, const bool & display)
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
	d_ptr->box_list.push_front(ImageBox(name, rect, color, id, display));
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
	if (found_ptr != nullptr)
	{
		rtn = *found_ptr;
		rtn.editing = false;
		return true;
	}
	return false;
}

std::vector<ImageBox> ImageWidget::getImageBoxsFromName(const std::string & name)
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

		paint_line.setP1(d_ptr->getPaintPosition(QPoint(p1.x,p1.y)));
		paint_line.setP2(d_ptr->getPaintPosition(QPoint(p2.x, p2.y)));

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
		paint_rect.setTopLeft(d_ptr->getPaintPosition(QPoint(cvr.x, cvr.y)));
		paint_rect.setBottomRight(d_ptr->getPaintPosition(QPoint(cvr.br().x, cvr.br().y)));
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
		paint_rect.setTopLeft(d_ptr->getPaintPosition(QPoint(cvr.x, cvr.y)));
		paint_rect.setBottomRight(d_ptr->getPaintPosition(QPoint(cvr.br().x, cvr.br().y)));
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
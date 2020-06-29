#include "ImageWidget.hxx"
#include <QTimer>
#include <QMouseEvent>
#include <QLinkedList>
#include <QPainter>
#ifndef IMAGEWIDGET_QML
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#endif
const int grabedge_thresh = 3;

class ImageWidgetBasePrivate : public QObject
{
	Q_OBJECT
public:
	ImageWidgetBasePrivate(ImageWidgetBase* parent) :
		q_ptr(parent),
		done_flag(false),
		log_zoom(1.),
		moving(false),
		backgroudcolor(125,125,125)
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
	QPointF start_point;
	QPointF source_position;
	QSizeF source_size;
	bool moving;
	PaintData done_paint_data;
	PaintData paint_data;
	QColor backgroudcolor;
public:
	double getLogZoom()
	{
		return log_zoom;
	}

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

	QImage cvMatToQImage(const cv::Mat& m, bool is_done = false)
	{
		if (is_done)
		{
			rgb_done.release();
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
			rgb.release();
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
	void zoomIn(QWheelEvent* e)
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
	void zoomOut(QWheelEvent* e)
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
		box.moveBox(vec,getImageSize());
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

	template <typename T, typename Y>
	T getImageRect(const Y& rt)
	{
		auto tl = getImagePosition<decltype(rt.topLeft())>(rt.topLeft());
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

	template<typename T, typename Y>
	T getPaintPosition(const Y& rt)
	{
		auto tmp_img = &(done_flag ? display_img_done : display_img);
		double power(1.);
		if (tmp_img->width() > tmp_img->height())
		{
			power = double(q_ptr->width()) / double(source_size.width());
		}
		else
		{
			power = double(q_ptr->height()) / double(source_size.height());
		}
		return T((double(rt.x()) - double(source_position.x())) * power, (double(rt.y()) - double(source_position.y())) * power);
	}
	double getPower()
	{
		auto tmp_img = &(done_flag ? display_img_done : display_img);
		double power(1.);
		if (tmp_img->width() > tmp_img->height())
		{
			power = double(q_ptr->width()) / double(source_size.width());
		}
		else
		{
			power = double(q_ptr->height()) / double(source_size.height());
		}
		return power;
	}

	template <typename T, typename Y>
	T getImagePosition(const Y& rt)
	{
		auto tmp_img = &(done_flag ? display_img_done : display_img);
		double power(1.);
		if (tmp_img->width() > tmp_img->height())
		{
			power = double(source_size.width()) / double(q_ptr->width());
		}
		else
		{
			power = double(source_size.height()) / double(q_ptr->height());
		}
		return T(double(rt.x()) * power + source_position.x(), double(rt.y()) * power + source_position.y());
	}
};

ImageWidgetBase::ImageWidgetBase(
#ifdef IMAGEWIDGET_QML
	QQuickItem* parent)
	: QQuickPaintedItem(parent),
#else
QWidget* parent)
	: QWidget(parent),
#endif // QIMAGEWIDGET_QML
	d(new ImageWidgetBasePrivate(this))
{
#ifdef IMAGEWIDGET_QML
	setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
	connect(this, &ImageWidgetBase::widthChanged, this, &ImageWidgetBase::onWidthChanged);
	connect(this, &ImageWidgetBase::heightChanged, this, &ImageWidgetBase::onHeightChanged);
#endif // IMAGEWIDGET_QML

}

ImageWidgetBase::~ImageWidgetBase()
{
	delete d;
}
#include <fstream>
void ImageWidgetBase::displayCVMat(cv::Mat img)
{
	if (img.empty())
	{
		return;
	}

	if (d->display_img.size() != QSize(img.cols, img.rows))
	{
		QPoint src_pnt;
		QSize src_size;
		if (float(img.cols) / float(img.rows) > float(width()) / float(height()))
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
	d->display_img.detach();
	auto qimg = d->cvMatToQImage(img);
	d->display_img = QPixmap::fromImage(qimg);
	qimg.detach();
	update();
}

void ImageWidgetBase::displayQImage(const QImage& img)
{
	if (img.byteCount() == 0)
	{
		return;
	}
	if (d->display_img.size() != img.size())
	{
		QPoint src_pnt;
		QSize src_size;
		if (float(img.width()) / float(img.height()) > float(width()) / float(height()))
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

	d->display_img = QPixmap::fromImage(img.copy(QRect(0, 0, img.width(), img.height())));
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

void ImageWidgetBase::displayDoneCVMat(const cv::Mat& img)
{
	if (img.empty())
	{
		return;
	}
	if (d->display_img_done.size() != QSize(img.cols, img.rows))
	{
		QPoint src_pnt;
		QSize src_size;
		if (float(img.cols) / float(img.rows) > float(width()) / float(height()))
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
	d->display_img_done = QPixmap::fromImage(d->cvMatToQImage(img, true));
	d->startDoneImageTimer();
	update();
}

void ImageWidgetBase::displayDoneQImage(const QImage& img)
{
	if (img.byteCount() == 0)
	{
		return;
	}
	if (d->display_img_done.size() != img.size())
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

void ImageWidgetBase::displayCVMat(QVariant img)
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
		displayDoneQImageWithData(img.value<QImage>(), paint_data.value<PaintData>());
	}
}

void ImageWidgetBase::resetScale()
{
	QPoint src_pnt;
	QSize src_size;

	auto tmp_img = &(d->done_flag ? d->display_img_done : d->display_img);
	if (!tmp_img->isNull())
	{
		if (float(tmp_img->width()) / float(tmp_img->height()) > float(width()) / float(height()))
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
	}
	d->source_position = src_pnt;
	d->source_size = src_size;
	update();

}

void ImageWidgetBase::setBackgroudColor(const QColor& c)
{
	d->backgroudcolor = c;
	update();
}

void ImageWidgetBase::mousePressEvent(QMouseEvent* e)
{
	d->moving = true;
	d->start_point = e->pos();
	update();
}

void ImageWidgetBase::mouseMoveEvent(QMouseEvent* e)
{
	if (!d->moving)
		return;
	auto m = QPointF(e->pos().x(),e->pos().y());
	auto vec = m - d->start_point;
	vec.setX(vec.x() * (d->source_size.width() / width()));
	vec.setY(vec.y() * (d->source_size.height() / height()));
	d->source_position.setX(d->source_position.x() - vec.x());
	d->source_position.setY(d->source_position.y() - vec.y());
	d->start_point = m;
	update();
}

void ImageWidgetBase::mouseReleaseEvent(QMouseEvent* e)
{
	d->moving = false;
	emit clickedPosition(d->getImagePosition<QPoint>(e->pos()));
}

void ImageWidgetBase::mouseDoubleClickEvent(QMouseEvent* e)
{
	resetScale();
}
#ifdef IMAGEWIDGET_QML
void ImageWidgetBase::onWidthChanged()
{
	resetScale();
}

void ImageWidgetBase::onHeightChanged()
{
	resetScale();
}
#endif
#ifndef IMAGEWIDGET_QML
void ImageWidgetBase::resizeEvent(QResizeEvent* e)
{
	QWidget::resizeEvent(e);
	resetScale();
}
#endif

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
	painter_ptr->setBrush(QBrush(d->backgroudcolor));
	painter_ptr->setPen(QPen(d->backgroudcolor));
	painter_ptr->drawRect(QRect(0, 0, width(), height()));
	auto tmp_img = &(d->done_flag ? d->display_img_done : d->display_img);

	painter_ptr->drawPixmap(QRectF(0, 0, width(), height()), *tmp_img, QRectF(d->source_position, d->source_size));
	(d->done_flag ? d->done_paint_data : d->paint_data).paintDatas(painter_ptr, d);
#ifndef IMAGEWIDGET_QML
	painter_ptr->end();
#endif
}

void ImageWidgetBase::wheelEvent(QWheelEvent* e)
{
	if (e->delta() > 0)
	{
		d->zoomIn(e);
	}
	else if (e->delta() < 0)
	{
		d->zoomOut(e);
	}
}

class ImageWidgetPrivate : public QObject
{
	Q_OBJECT
public:
	ImageWidgetPrivate(ImageWidget* parent) :
		q_ptr(parent),
		grabed_box_ptr(nullptr),
		is_painting(false),
		grabed_obj(false),
		grabed_edge(false)
	{

	}
	~ImageWidgetPrivate()
	{

	}
	void normalizeAllBox()
	{
		for (auto& a : box_list)
		{
			a->normalize();
		}
	}
	void resetBoxEditing()
	{
		for (auto& a : box_list)
		{
			a->setEditing(false);
		}
	}

	void checkPress(const QPoint& p, const double& power)
	{
		if (box_list.empty())
			return;
		if (!(*box_list.begin())->getEditing())
			return;

		auto rtn = (*box_list.begin())->checkPress(p, power);
		if (rtn.has_value())
		{
			grabed_type = rtn.value();
			grabed_edge = true;
		}
		else
		{
			grabed_edge = false;
		}
	}
	bool checkMove(const QPoint& p, const double& power)
	{
		if (box_list.empty())
			return false;
		if (!(*box_list.begin())->getEditing())
			return false;
		auto val = (*box_list.begin())->checkMove(p, power);
		if (val.has_value())
		{
			grabed_type = val.value();
			return true;
		}
		else
		{
			return false;
		}
	}

private:
	friend ImageWidget;
	QList<ImageBox*> box_list;
	ImageWidget* q_ptr;
	bool is_painting;
	ImageBox* grabed_box_ptr;
	bool grabed_obj;
	QPoint start_point;
	bool grabed_edge;
	ImageBox::GrabedEdgeType grabed_type;
	ImageBox* new_box_tmp;
};

#ifdef IMAGEWIDGET_QML
ImageWidget::ImageWidget(QQuickItem* parent)
#else
ImageWidget::ImageWidget(QWidget* parent)
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

void ImageWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::MouseButton::LeftButton)
	{
		//正在画图
		if (d_ptr->is_painting)
		{
			d_ptr->box_list.push_front(d_ptr->new_box_tmp);
			d_ptr->new_box_tmp = nullptr;
			d_ptr->grabed_box_ptr = *(d_ptr->box_list.begin());
			d_ptr->grabed_box_ptr->setEditing(true);
			auto start_point = d->getImagePosition<QPointF>(e->pos());
			d_ptr->grabed_box_ptr->startPaint(start_point);
			update();
			return;
		}
		auto pos = d->getImagePosition<QPoint>(e->pos());

		d_ptr->checkPress(pos, d->getAveragePower());
		if (d_ptr->grabed_edge)
		{
			update();
			return;
		}
		bool flag(false);
		decltype(d_ptr->box_list.begin()) iter(d_ptr->box_list.begin());
		for (auto& a : d_ptr->box_list)
		{
			if (a->isInBox(pos))
			{
				auto tmp = *iter;
				d_ptr->box_list.erase(iter);
				d_ptr->box_list.push_front(tmp);
				d_ptr->grabed_box_ptr = *d_ptr->box_list.begin();
				d_ptr->resetBoxEditing();
				d_ptr->grabed_box_ptr->setEditing(true);
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

void ImageWidget::mouseMoveEvent(QMouseEvent* e)
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
		(*box_ptr)->editEdge(d_ptr->grabed_type, m);
		update();
	}
#ifndef IMAGEWIDGET_QML
	if (d_ptr->checkMove(d->getImagePosition<QPoint>(e->pos()), d->getAveragePower()))
	{
		switch (d_ptr->grabed_type)
		{
		case ImageBox::GrabedEdgeType::Left:
			setCursor(Qt::SizeHorCursor);
			break;
		case ImageBox::GrabedEdgeType::Right:
			setCursor(Qt::SizeHorCursor);
			break;
		case ImageBox::GrabedEdgeType::Top:
			setCursor(Qt::SizeVerCursor);
			break;
		case ImageBox::GrabedEdgeType::Bottom:
			setCursor(Qt::SizeVerCursor);
			break;
		case ImageBox::GrabedEdgeType::TopLeft:
			setCursor(Qt::SizeFDiagCursor);
			break;
		case ImageBox::GrabedEdgeType::TopRight:
			setCursor(Qt::SizeBDiagCursor);
			break;
		case ImageBox::GrabedEdgeType::BottomLeft:
			setCursor(Qt::SizeBDiagCursor);
			break;
		case ImageBox::GrabedEdgeType::BottomRight:
			setCursor(Qt::SizeFDiagCursor);
			break;
		default:
			setCursor(Qt::ArrowCursor);
			break;
		}
	}
	else if (d_ptr->box_list.begin() != d_ptr->box_list.end())
	{
		if ((*d_ptr->box_list.begin())->isInBox(d->getImagePosition<QPoint>(e->pos())))
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
		d_ptr->grabed_box_ptr->endPaint(m);
		d_ptr->grabed_box_ptr->fixShape(d->getImageSize());
	}
	else
	{
		auto m = e->pos();
		d->getMovedBox(*d_ptr->grabed_box_ptr, d_ptr->start_point, m);
		d_ptr->start_point = m;
	}
	update();
}

void ImageWidget::mouseReleaseEvent(QMouseEvent* e)
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
			(*iter)->setEditing(false);
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
	for (auto& a : d_ptr->box_list)
	{
		a->paintShape(painter_ptr, d);
	}
#ifndef IMAGEWIDGET_QML
	painter_ptr->end();
#endif
}

void ImageWidget::wheelEvent(QWheelEvent* e)
{
	ImageWidgetBase::wheelEvent(e);
	update();
}
#ifndef IMAGEWIDGET_QML
void ImageWidget::contextMenuEvent(QContextMenuEvent* e)
{

	QMenu menu;
	decltype(d_ptr->box_list.begin()) selected_iter(d_ptr->box_list.end());
	for (auto iter = d_ptr->box_list.begin(); iter != d_ptr->box_list.end(); iter++)
	{
		auto pos = d->getImagePosition<QPoint>(e->pos());
		if ((*iter)->isInBox(pos))
		{
			selected_iter = iter;
			break;
		}
	}
	if (selected_iter != d_ptr->box_list.end())
	{
		auto new_box = *selected_iter;
		new_box->setEditing(true);
		d_ptr->box_list.erase(selected_iter);
		d_ptr->box_list.push_front(new_box);
		for (auto iter = d_ptr->box_list.begin() + 1; iter != d_ptr->box_list.end(); iter++)
		{
			(*iter)->setEditing(false);
		}
		update();
		selected_iter = d_ptr->box_list.begin();
		QAction env_action("反向");
		env_action.setCheckable(true);
		env_action.setChecked((*selected_iter)->isEnv());
		connect(&env_action, &QAction::triggered, [this, &selected_iter, &env_action]() {
			(*selected_iter)->setIsEnv(env_action.isChecked());
			update();
			});
		QAction outmask_action("输出Mask图片");
		connect(&outmask_action, &QAction::triggered, [this, &selected_iter, &outmask_action]() {
			auto fn = QFileDialog::getSaveFileName(this, "选择文件", "./img.png", "Image (*.png *.bmp *.jpg)");
			try
			{
				cv::imwrite(fn.toStdString(), (*selected_iter)->getMask(d->getImageSize()));
			}
			catch (const cv::Exception & e)
			{
				QMessageBox::warning(nullptr, "警告", "输出失败");

			}
		});
		menu.addAction(&env_action);
		menu.addAction(&outmask_action);
		menu.exec(e->globalPos());
		return;
	}
#ifdef TEST
	QAction paint_action(u8"创建选框");
	connect(&paint_action, &QAction::triggered, [this]() {
		paintNewImageBox(new RectImageBox(0, 0, 0, 0));
		});
	menu.addAction(&paint_action);
#else
#endif // TEST
	QAction save_img("输出原图");
	connect(&save_img, &QAction::triggered,this, [this]() {
		auto fn = QFileDialog::getSaveFileName(this, "选择文件", "./img.png", "Image (*.png *.bmp *.jpg)");
		try
		{
			cv::Mat tmp;
			cv::cvtColor(d->rgb,tmp,cv::COLOR_RGB2BGR);
			cv::imwrite(fn.toStdString(), tmp);
		}
		catch (const cv::Exception& e)
		{
			QMessageBox::warning(nullptr, "警告", "输出失败");

		}
		});

	QAction save_pimg("输出绘制图片");

	connect(&save_pimg, &QAction::triggered,this, [this]() {
		auto fn = QFileDialog::getSaveFileName(this, "选择文件", "./img.png", "Image (*.png *.bmp *.jpg)");

		try
		{
			cv::Mat tmp;
			cv::cvtColor(d->rgb,tmp,cv::COLOR_RGB2BGR);
			d->paint_data.drawDatas(tmp);
			cv::imwrite(fn.toStdString(), tmp);
		}
		catch (const cv::Exception & e)
		{
			QMessageBox::warning(nullptr,"警告", "输出失败");

		}
		});
	menu.addAction(&save_img);
	menu.addAction(&save_pimg);
	menu.exec(e->globalPos());
	return;
}
#endif

void ImageWidget::paintNewImageBox(ImageBox* box)
{
	for (auto iter = d_ptr->box_list.begin(); iter != d_ptr->box_list.end(); iter++)
	{
		if ((*iter)->getBoxID() == -1)
			continue;
		if ((*iter)->getBoxID() == box->getBoxID())
		{
			auto tmp = *iter;
			d_ptr->box_list.erase(iter);
			tmp->deleteLater();
			d_ptr->box_list.push_front(box);
			break;
		}
	}
	box->setParent(this);
	d_ptr->is_painting = true;
	d_ptr->new_box_tmp = box;
}

void ImageWidget::addImageBox(ImageBox* box)
{
	for (auto iter = d_ptr->box_list.begin(); iter != d_ptr->box_list.end(); iter++)
	{
		if ((*iter)->getBoxID() == -1)
			continue;
		if ((*iter)->getBoxID() == box->getBoxID())
		{
			auto tmp = *iter;
			d_ptr->box_list.erase(iter);
			tmp->deleteLater();
			break;
		}
	}
	box->setParent(this);
	d_ptr->box_list.push_front(box);
	update();
}

void ImageWidget::removeImageBoxById(const int& id)
{
	for (auto iter = d_ptr->box_list.begin(); iter != d_ptr->box_list.end();iter++)
	{
		if ((*iter)->getBoxID() == id)
		{
			auto tmp = *iter;
			d_ptr->box_list.erase(iter);
			tmp->deleteLater();
			break;
		}
	}
}

void ImageWidget::removeImageBoxByName(const QString& name)
{
	while (true)
	{
		bool flag = false;
		for (auto iter = d_ptr->box_list.begin(); iter != d_ptr->box_list.end(); iter++)
		{
			if ((*iter)->getName() == name)
			{
				auto tmp = *iter;
				d_ptr->box_list.erase(iter);
				tmp->deleteLater();
				flag = true;
				break;
			}
		}
		if (!flag)
			break;
	}
}

void ImageWidget::paintNewImageBox(QVariant box)
{
	return paintNewImageBox(static_cast<ImageBox*>(box.value<QObject*>()));
}
void ImageWidget::addImageBox(QVariant box)
{
	auto val = static_cast<ImageBox*>(box.value<QObject*>());
	addImageBox(val);
}

ImageBox* ImageWidget::getImageBoxFromId(const int& id)
{
	ImageBox* found_ptr(nullptr);
	for (auto& a : d_ptr->box_list)
	{
		if (a->getBoxID() == -1)
			continue;
		if (a->getBoxID() == id)
		{
			found_ptr = a;
			break;
		}
	}
	return found_ptr;
}

QVariant ImageWidget::getImageBoxVarFromId(const int& id)
{
	QVariant var;
	var.setValue( static_cast<QObject*>(getImageBoxFromId(id)));
	return var;
}

QList<ImageBox*> ImageWidget::getImageBoxsFromName(const QString& name)
{
	QList<ImageBox*> out;
	for (auto& a : d_ptr->box_list)
	{
		if (a->getName() == name)
		{
			out.push_back(a);
		}
	}
	return out;
}

QVariantList ImageWidget::getImageBoxVarlistFromName(const QString& name)
{
	QList<QVariant> out;
	auto rtn = getImageBoxsFromName(name);
	for (auto& node: rtn)
	{
		QVariant var;
		var.setValue(static_cast<QObject*>(node));
		out.push_back(var);
	}
	return out;
}

void ImageWidget::clearAllBoxs()
{
	for (auto& a : d_ptr->box_list)
	{
		a->deleteLater();
	}
	d_ptr->box_list.clear();
	update();
}

void ImageBox::operator=(const ImageBox& box)
{
	display = box.display;
	name = box.name;
	boxID = box.boxID;
	env = box.env;
	editing = box.editing;
}

ImageBox::ImageBox(const ImageBox& other)
{
	*this = other;
}


void PaintData::paintDatas(QPainter* painter, ImageWidgetBasePrivate* d_ptr)
{
	for (const auto& line : lines)
	{
		QLineF paint_line;
		const auto& [p1, p2, thinkness, color] = line;
		auto pen_color = QColor(color[2], color[1], color[0]);

		paint_line.setP1(d_ptr->getPaintPosition<QPoint>(QPoint(p1.x, p1.y)));
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
		if (thinkness > 0)
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
	for (const auto& center : corss_lines)
	{
		const auto& [center_pos, wh,thinkness,color] = center;
		QLineF line1(d_ptr->getPaintPosition<QPointF>(QPointF(center_pos.x - wh / 2., center_pos.y)), d_ptr->getPaintPosition<QPointF>(QPointF(center_pos.x + wh / 2., center_pos.y)));
		QLineF line2(d_ptr->getPaintPosition<QPointF>(QPointF(center_pos.x,center_pos.y - wh / 2.)), d_ptr->getPaintPosition<QPointF>(QPointF(center_pos.x, center_pos.y + wh / 2.)));
		auto pen_color = QColor(color[2], color[1], color[0]);
		QPen pen(pen_color);
		pen.setStyle(Qt::PenStyle::SolidLine);
		QBrush brush(pen_color);
		brush.setStyle(thinkness < 0 ? Qt::SolidPattern : Qt::NoBrush);
		if (brush.style() == Qt::NoBrush)
			pen.setWidth(thinkness);
		painter->setPen(pen);
		painter->setBrush(brush);
		painter->drawLine(line1);
		painter->drawLine(line2);
	}
	for (const auto& text_tuple : texts)
	{
		const auto& [text, pos, pixel_size, font_style, color] = text_tuple;

		auto pen_color = QColor(color[2], color[1], color[0]);
		QPen pen(pen_color);
		QFont font;
		font.setFamily(font_style.c_str());
		auto text_scale = pixel_size * d_ptr->getPower();
		font.setPixelSize(std::floor(text_scale < 1 ? 1 : text_scale));
		painter->setPen(pen);
		painter->setFont(font);
		painter->drawText(d_ptr->getPaintPosition<QPointF>(QPoint(pos.x,pos.y)),text.c_str());
	}
}

PaintData& PaintData::operator<<(PaintData& inp)
{
	for (auto& a : inp.lines)
	{
		lines.push_back(std::move(a));
	}
	for (auto& a : inp.circles)
	{
		circles.push_back(std::move(a));
	}	
	for (auto& a : inp.rects)
	{
		rects.push_back(std::move(a));
	}	
	for (auto& a : inp.texts)
	{
		texts.push_back(std::move(a));
	}
	for (auto& a : inp.corss_lines)
	{
		corss_lines.push_back(std::move(a));
	}
	return *this;
}

void PaintData::drawDatas(cv::Mat& mat) const
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
	for (const auto& center : corss_lines)
	{
		cv::line(mat, std::get<0>(center) - cv::Point2d(std::get<1>(center) / 2., 0.), std::get<0>(center) + cv::Point2d(std::get<1>(center) / 2., 0.), std::get<3>(center),std::get<2>(center));
		cv::line(mat, std::get<0>(center) - cv::Point2d(0., std::get<1>(center) / 2.), std::get<0>(center) + cv::Point2d(0., std::get<1>(center) / 2.), std::get<3>(center), std::get<2>(center));
	}
	for (const auto& text : texts)
	{
		cv::putText(mat,std::get<0>(text), std::get<1>(text),cv::FONT_HERSHEY_COMPLEX, cv::getFontScaleFromHeight(cv::FONT_HERSHEY_COMPLEX,std::get<2>(text)),std::get<4>(text));
	}
}

//ImageBox(QObject* parent):

//{
//
//}
//ImageBox(/*const QString& name,*/ const int& id, const bool& display, const bool& env, QObject* parent) :


ImageBox::ImageBox(
	const QString& name,
	const int& id,
	const bool& display,
	const bool& env,
	QObject* parent) 
	:
	QObject(parent),
	boxID(-1),
	name(name),
	display(display),
	editing(false),
	env(env)
{

}

RectImageBox::RectImageBox(QObject* parent):
	RectImageBox(
		0,
		0,
		0,
		0, 
		QPen(QColor(0, 0, 255)),
		QBrush(QColor(0, 0, 255, 0.2),
		Qt::BrushStyle::SolidPattern), 
		"default",
		-1,
		true,
		false,
		parent)
{
}


RectImageBox::RectImageBox(const double& x,
	const double& y,
	const double& width,
	const double& height,
	const QPen& pen,
	const QBrush& brush,
	const QString& name,
	const int& id,
	const bool& display, const bool& env,
	QObject* parent) :
	x(x),
	y(y),
	width(width),
	height(height),
	pen(pen),
	brush(brush),
	editingPen(pen),
	editingBrush(brush), 
	ImageBox(name,id,display,env,parent)
{
	editingPen.setStyle(Qt::DashLine);
	auto tmp = brush.color();
	tmp.setAlpha(100);
	editingBrush.setColor(tmp);
}

ImageBox::~ImageBox()
{
}

bool ImageBox::isDisplay()
{
	return display;
}

bool ImageBox::isEnv()
{
	return env;
}

void ImageBox::setIsEnv(const bool& e)
{
	env = e;
}

void ImageBox::setIsDisplay(const bool& d)
{
	display = d;
}

int ImageBox::getBoxID()
{
	return boxID;
}

void ImageBox::setBoxID(const int& i)
{
	boxID = i;
}

QString ImageBox::getName()
{
	return name;
}

void ImageBox::setName(const QString& n)
{
	name = n;
}

bool ImageBox::getEditing()
{
	return editing;
}

void ImageBox::setEditing(const bool& e)
{
	editing = e;
}

QVariant ImageBox::getMaskVar(const int& width, const int&height)
{
	QVariant var;
	var.setValue(getMask(QSize(width,height)));
	return var;
}

RectImageBox::RectImageBox(const RectImageBox& other) :
	ImageBox(other)
{
	*this = other;
}

void RectImageBox::operator=(const ImageBox& rb)
{
	const RectImageBox& pm = static_cast<const RectImageBox&>(rb);
	operator=(pm);
}

void RectImageBox::operator=(const RectImageBox& rb)
{
	pen = rb.pen;
	brush = rb.brush;
	editingBrush = rb.editingBrush;
	editingPen = rb.editingPen;
	x = rb.x;
	y = rb.y;
	width = rb.width;
	height = rb.height;

	display = rb.display;
	name = rb.name;
	boxID = rb.boxID;
	env = rb.env;
	editing = rb.editing;
}

RectImageBox::~RectImageBox()
{
}

QRect RectImageBox::getQRect()
{
	return QRect(x,y,width,height);
}

void RectImageBox::fromQRect(const QRect& rect)
{
	x = rect.x();
	y = rect.y();
	width = rect.width();
	height = rect.height();
}

QRectF RectImageBox::getQRectF()
{
	return QRectF(x,y,width,height);
}

void RectImageBox::fromQRectF(const QRectF& rect)
{
	x = rect.x();
	y = rect.y();
	width = rect.width();
	height = rect.height();
}

cv::Rect RectImageBox::getCVRect()
{
	return cv::Rect(x,y,width,height);
}

Q_INVOKABLE void RectImageBox::fromCVRect(const cv::Rect& rect)
{
	x = rect.x;
	y = rect.y;
	width = rect.width;
	height = rect.height;
}

void RectImageBox::paintShape(QPainter* painter, ImageWidgetBasePrivate* d)
{
	if (!isDisplay())
		return;
	if (getEditing())
	{
		painter->setPen(editingPen);
		painter->setBrush(editingBrush);
	}
	else
	{
		painter->setPen(pen);
		painter->setBrush(brush);
	}
	painter->drawRect(d->getPaintRect<QRectF>(QRectF(x,y,width,height)));
	QFont font;
	font.setFamily("Microsoft YaHei");
	double text_scale = 16 /** d->getPower()*/;
	font.setPixelSize(std::floor(text_scale < 1 ? 1 : text_scale));
	painter->setPen(QPen(pen.color()));
	painter->setFont(font);
	painter->drawText(d->getPaintPosition<QPointF>(QPoint(x, y)), "Name:" + getName() + QString(" (%1,%2,%3,%4)").arg(QString::number(int(x)), QString::number(int(y)), QString::number(int(width)), QString::number(int(height))));
}

bool RectImageBox::isInBox(const QPoint& p)
{
	if (!isDisplay())
		return false;
	return p.x() >= x && p.x() <= (x + width) && p.y() >= y && p.y() <= (y + height);
}

void RectImageBox::moveBox(const QPointF& vec,const QSize& is)
{
	if(x + vec.x() >= 0 && (x + vec.x() + width) < is.width())
		x += vec.x();
	if(y + vec.y() >= 0 && (y + vec.y() + height) < is.height())
		y += vec.y();
}

std::optional<ImageBox::GrabedEdgeType> RectImageBox::checkPress(const QPoint& p, const double& power)
{
	std::optional<ImageBox::GrabedEdgeType> opt;
	double thresh = grabedge_thresh * power;
	auto tmp_rt = getQRect();
	if (std::abs(tmp_rt.left() - p.x()) < thresh)
	{
		opt = Left;
	}
	if (std::abs(tmp_rt.right() - p.x()) < thresh)
	{
		opt = Right;
	}
	if (std::abs(tmp_rt.top() - p.y()) < thresh)
	{
		opt = Top;
	}
	if (std::abs(tmp_rt.bottom() - p.y()) < thresh)
	{
		opt = Bottom;
	}
	if (opt.has_value())
	{
		QPoint p1;
		QPoint p2;
		switch (opt.value())
		{
		case ImageBox::GrabedEdgeType::Top:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.topRight() - p;
			if (std::abs(p1.x()) <= thresh && std::abs(p1.y()) <= thresh)
			{
				opt = TopLeft;
			}
			if (std::abs(p2.x()) <= thresh && std::abs(p2.y()) <= thresh)
			{
				opt = TopRight;
			}
			break;
		case ImageBox::GrabedEdgeType::Bottom:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.topRight() - p;
			if (std::abs(p1.x()) <= thresh && p1.y() <= thresh)
			{
				opt = BottomLeft;
			}
			if (std::abs(p2.x()) <= thresh && p2.y() <= thresh)
			{
				opt = BottomRight;
			}
			break;
		case ImageBox::GrabedEdgeType::Left:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.bottomLeft() - p;
			if (std::abs(p1.x()) <= thresh && std::abs(p1.y()) <= thresh)
			{
				opt = TopLeft;
			}
			if (std::abs(p2.x()) <= thresh && std::abs(p2.y()) <= thresh)
			{
				opt = BottomLeft;
			}
			break;
		case ImageBox::GrabedEdgeType::Right:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.bottomLeft() - p;
			if (std::abs(p1.x()) <= thresh && std::abs(p1.y()) <= thresh)
			{
				opt = TopRight;
			}
			if (std::abs(p2.x()) <= thresh && std::abs(p2.y()) <= thresh)
			{
				opt = BottomRight;
			}
			break;
		default:
			break;
		}
	}
	return opt;
}

std::optional<ImageBox::GrabedEdgeType> RectImageBox::checkMove(const QPoint& p, const double& power)
{
	std::optional<ImageBox::GrabedEdgeType> opt;
	double thresh = grabedge_thresh * power;
	auto tmp_rt = getQRect();
	if (abs(tmp_rt.left() - p.x()) < thresh)
	{
		opt = Left;
	}
	if (abs(tmp_rt.right() - p.x()) < thresh)
	{
		opt = Right;
	}
	if (abs(tmp_rt.top() - p.y()) < thresh)
	{
		opt = Top;
	}
	if (abs(tmp_rt.bottom() - p.y()) < thresh)
	{
		opt = Bottom;
	}
	QPoint p1;
	QPoint p2;
	if (opt.has_value())
	{
		switch (opt.value())
		{
		case ImageBox::GrabedEdgeType::Top:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.topRight() - p;
			if (abs(p1.x()) <= thresh && abs(p1.y()) <= thresh)
			{
				opt = TopLeft;
			}
			if (abs(p2.x()) <= thresh && abs(p2.y()) <= thresh)
			{
				opt = TopRight;
			}
			break;
		case ImageBox::GrabedEdgeType::Bottom:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.topRight() - p;
			if (abs(p1.x()) <= thresh && p1.y() <= thresh)
			{
				opt = BottomLeft;
			}
			if (abs(p2.x()) <= thresh && p2.y() <= thresh)
			{
				opt = BottomRight;
			}
			break;
		case ImageBox::GrabedEdgeType::Left:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.bottomLeft() - p;
			if (abs(p1.x()) <= thresh && abs(p1.y()) <= thresh)
			{
				opt = TopLeft;
			}
			if (abs(p2.x()) <= thresh && abs(p2.y()) <= thresh)
			{
				opt = BottomLeft;
			}
			break;
		case ImageBox::GrabedEdgeType::Right:
			p1 = tmp_rt.topLeft() - p;
			p2 = tmp_rt.bottomLeft() - p;
			if (abs(p1.x()) <= thresh && abs(p1.y()) <= thresh)
			{
				opt = TopRight;
			}
			if (abs(p2.x()) <= thresh && abs(p2.y()) <= thresh)
			{
				opt = BottomRight;
			}
			break;
		default:
			break;
		}
	}
	return opt;

}

cv::Mat RectImageBox::getMask(const QSize& sz)
{
	cv::Mat out(sz.height(), sz.width(), CV_8UC1, cv::Scalar(env ? 255 : 0));
	cv::rectangle(out, cv::Rect(x, y, width, height), cv::Scalar(env ? 0 : 255), -1);
	return out;
}


void RectImageBox::normalize()
{
	auto nor = QRectF(x, y, width, height).normalized();
	x = nor.x();
	y = nor.y();
	width = nor.width();
	height = nor.height();
}

void RectImageBox::startPaint(const QPointF& pnt)
{
	x = pnt.x();
	y = pnt.y();
}

void RectImageBox::endPaint(const QPointF& pnt)
{
	width = pnt.x() - x;
	height = pnt.y() - y;
}

void RectImageBox::editEdge(const ImageBox::GrabedEdgeType& type, const QPointF& pos)
{
	QRectF tmp_rt;
	switch (type)
	{
	case ImageBox::GrabedEdgeType::Left:
		tmp_rt = getQRectF();
		tmp_rt.setLeft(pos.x());
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::Right:
		tmp_rt = getQRectF();
		tmp_rt.setRight(pos.x());
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::Top:
		tmp_rt = getQRectF();
		tmp_rt.setTop(pos.y());
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::Bottom:
		tmp_rt = getQRectF();
		tmp_rt.setBottom(pos.y());
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::TopLeft:
		tmp_rt = getQRectF();
		tmp_rt.setTopLeft(pos);
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::TopRight:
		tmp_rt = getQRectF();
		tmp_rt.setTopRight(pos);
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::BottomLeft:
		tmp_rt = getQRectF();
		tmp_rt.setBottomLeft(pos);
		fromQRectF(tmp_rt);
		break;
	case ImageBox::GrabedEdgeType::BottomRight:
		tmp_rt = getQRectF();
		tmp_rt.setBottomRight(pos);
		fromQRectF(tmp_rt);
		break;
	default:
		break;
	}

}

void RectImageBox::fixShape(const QSize& size)
{
	if (x + width > size.width())
	{
		width = size.width() - x;
	}
	if (x + width < 0)
	{
		width = -x;
	}
	if (y + height > size.height())
	{
		height = size.height() - y;
	}
	if (y + height < 0)
	{
		height = -y;
	}
}

void RectImageBox::resetData()
{
	x = 0;
	y = 0;
	width = 0;
	height = 0;
}

QPen RectImageBox::getPen()
{
	return pen;
}

QBrush RectImageBox::getBrush()
{
	return brush;
}

QPen RectImageBox::getEditingPen()
{
	return editingPen;
}

QBrush RectImageBox::getEditingBrush()
{
	return editingBrush;
}

void RectImageBox::setPen(const QPen& p)
{
	pen = p;
}

void RectImageBox::setEditingPen(const QPen& p)
{
	editingPen = p;
}

void RectImageBox::setBrush(const QBrush& b)
{
	brush = b;
}

void RectImageBox::setEditingBrush(const QBrush& b)
{
	editingBrush = b;
}

void RectImageBox::setX(const double& x)
{
	this->x = x;
}

double RectImageBox::getX()
{
	return x;
}

void RectImageBox::setY(const double& y)
{
	this->y = y;
}

double RectImageBox::getY()
{
	return y;
}

void RectImageBox::setWidth(const double& w)
{
	this->width = w;
}

double RectImageBox::getWidth()
{
	return width;
}

void RectImageBox::setHeight(const double& h)
{
	this->height = h;
}

double RectImageBox::getHeight()
{
	return height;
}


EllipseImageBox::EllipseImageBox(QObject* parent) :
	RectImageBox(parent)
{
}
EllipseImageBox::EllipseImageBox(
	const double& p1,
	const double& p2,
	const double& p3,
	const double& p4,
	const QPen& p5,
	const QBrush& p6,
	const QString& p7,
	const int& p8,
	const bool& p9,
	const bool& p10,
	QObject* p11) 
	:
	RectImageBox(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)
{

}
EllipseImageBox::EllipseImageBox(const EllipseImageBox& other) :
	RectImageBox(other)
{
}

void EllipseImageBox::operator=(const EllipseImageBox& rb)
{
	RectImageBox::operator=(rb);
}

cv::Mat EllipseImageBox::getMask(const QSize& sz)
{
	cv::Mat out(sz.height(), sz.width(), CV_8UC1, cv::Scalar(env ? 255 : 0));
	cv::RotatedRect rect(cv::Point2f(x + width / 2., y + height / 2.),cv::Size2f( width, height), 0);
	cv::ellipse(out, rect, cv::Scalar(env ? 0 : 255), -1);
	return out;
}

EllipseImageBox::~EllipseImageBox()
{
}

void EllipseImageBox::paintShape(QPainter* painter, ImageWidgetBasePrivate* d)
{
	if (!isDisplay())
		return;
	if (getEditing())
	{
		painter->setPen(editingPen);
		painter->setBrush(editingBrush);
	}
	else
	{
		painter->setPen(pen);
		painter->setBrush(brush);
	}
	auto rect_tmp = d->getPaintRect<QRectF>(QRectF(x, y, width, height));
	painter->drawRect(rect_tmp);
	painter->drawEllipse(rect_tmp);
	QFont font;
	font.setFamily("Microsoft YaHei");
	double text_scale = 16 /** d->getPower()*/;
	font.setPixelSize(std::floor(text_scale < 1 ? 1 : text_scale));
	painter->setPen(QPen(pen.color()));
	painter->setFont(font);
	painter->drawText(d->getPaintPosition<QPointF>(QPoint(x, y)), "Name:" + getName() + QString(" (%1,%2,%3,%4)").arg(QString::number(int(x)), QString::number(int(y)), QString::number(int(width)), QString::number(int(height))));
}
#ifdef IMAGEWIDGET_QML
#include <QQmlExtensionPlugin>

class ImageWidgetQMLPlugin : public QQmlExtensionPlugin     // 继承QQmlExtensionPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)  // 为这个插件定义了一个唯一的接口，并注册至元对象系统

public:
	ImageWidgetQMLPlugin(QObject* parent = 0) : QQmlExtensionPlugin(parent) { }
	void registerTypes(const char* uri) 
	{
		qmlRegisterType<ImageWidget>(uri, 1, 0, "ImageWidget");
		qmlRegisterType<ImageWidgetBase>(uri, 1, 0, "ImageWidgetBase");
		qmlRegisterType<RectImageBox>(uri, 1, 0, "RectImageBox");
		qmlRegisterType<EllipseImageBox>(uri, 1, 0, "EllipseImageBox");
	}
};

#endif
#include "ImageWidget.moc"
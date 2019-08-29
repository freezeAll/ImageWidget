#include "ROIDialog.hxx"
#include <QPushButton>
#include <QScreen>
#include <QApplication>
class ROIDialogPrivate : public QObject
{
	Q_OBJECT
public:
	ROIDialogPrivate(cv::Mat s,ImageBox& ib,ROIDialog * parent) :
		q(parent),
		src(s),
		rtn_box(ib)
	{
		old_id = ib.id;
	}
private:
	friend class ROIDialog;
	cv::Mat src;
	ImageBox& rtn_box;
	ROIDialog* q;
	int old_id;
	ImageWidget* iw_ptr;
};


ROIDialog::ROIDialog(cv::Mat src, ImageBox& ib, const double& sc, QWidget *parent)
	: QDialog(parent),
	d(new ROIDialogPrivate(src,ib,this))
{
	setWindowTitle("ROIDialog");
	double roidialog_scale;
	auto screens = QApplication::screens();
	if (screens.size() == 1)
	{
		auto screen_size = screens[0]->size();
		auto img_size = QSize(src.cols, src.rows);
		double width_power, height_power;
		width_power = double(img_size.width()) / double(screen_size.width());
		height_power = double(img_size.height()) / double(screen_size.height());
		if (height_power > width_power)
		{
			auto tmp_height = screen_size.height() * (sc);
			roidialog_scale = tmp_height / img_size.height();
		}
		else
		{
			auto tmp_width = screen_size.width() * (sc);
			roidialog_scale = tmp_width / img_size.width();
		}
	}

	auto ptr = new ImageWidget(this);
	d->iw_ptr = ptr;
	ptr->setGeometry(0, 0, src.cols * roidialog_scale, src.rows * roidialog_scale);
	this->setFixedSize(QSize(src.cols * roidialog_scale + 80, src.rows * roidialog_scale));
	ptr->displayCVMat(src);
	if (ib.width == 0 || ib.height == 0)
	{
		ptr->paintNewImageBox(ib.name, ib.toQColor(), 1, true);
	}
	else
	{
		ib.id = 1;
		ptr->addImageBox(ib);
	}
	auto okbtn_ptr = new QPushButton("OK", this);
	okbtn_ptr->setGeometry(src.cols * roidialog_scale, 0, 80, 40);
	auto cancel_ptr = new QPushButton("Cancel", this);
	cancel_ptr->setGeometry(src.cols * roidialog_scale, 50, 80, 40);
	auto reset_ptr = new QPushButton("Reset", this);
	reset_ptr->setGeometry(src.cols * roidialog_scale, 100, 80, 40);

	connect(okbtn_ptr, &QPushButton::clicked, this, [this]() {
		ImageBox out;
		d->iw_ptr->getImageBoxFromId(1,out);
		out.id = d->old_id;
		d->rtn_box = out;
		accept();
	});
	connect(cancel_ptr, &QPushButton::clicked, this, [this]() {
		reject();
	});
	connect(reset_ptr, &QPushButton::clicked, this, [this]() {
		d->iw_ptr->clearAllBoxs();
		d->iw_ptr->paintNewImageBox(d->rtn_box.name, d->rtn_box.toQColor(), 1, true);
		});
}

ROIDialog::~ROIDialog()
{
	delete d;
}

#include "ROIDialog.moc"
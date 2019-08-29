#pragma once

#include <QDialog>
#include "ImageWidget.h"

class ROIDialogPrivate;
class ROIDialog : public QDialog
{
	Q_OBJECT

public:
	ROIDialog(cv::Mat,ImageBox&,const double&,QWidget *parent = nullptr);
	~ROIDialog();
private:
	ROIDialogPrivate * d;
};

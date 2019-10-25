#pragma once
#ifndef IMAGEWIDGET_QML
#include <QDialog>
#include "ImageWidget.hxx"

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
#endif
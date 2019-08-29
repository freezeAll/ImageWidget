#include "ImageWidget.hxx"
#include <QtWidgets/QApplication>
#include "ROIDialog.hxx"
#ifdef TEST
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
	auto img = cv::imread("1.bmp");
	ImageBox box;
	box.id = 123;
	box.fromQRect(QRect(0, 0, 500, 500));
	box.isEnv = true;
	ROIDialog w(img,box,0.3);
	w.show();
	auto res = a.exec();
	return res;
}
#endif // !TEST
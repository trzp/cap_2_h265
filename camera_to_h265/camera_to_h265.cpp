#include "H265EncoderTang.h"
#include <thread>

int main(int argc, char** argv)
{
	H265EncoderTang coder265(640, 480, 20);

	thread th(&H265EncoderTang::mainLoop, std::ref(coder265));
	th.detach();

	VideoCapture cap;
	Mat frame;
	int key;

	cap.open(0);
	if (!cap.isOpened())
		return 0;

	while (1)
	{
		cap >> frame;
		if (frame.empty())
			break;

		coder265.encodeNewFrame(frame);

		imshow("video", frame);
		key = waitKey(100);//每帧延时20毫秒
		if (key == 27)
		{
			break;
		}
	}
	cap.release();
	coder265.release();
}
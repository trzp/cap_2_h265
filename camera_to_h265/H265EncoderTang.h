#pragma once
#ifndef H265_ENCODER_H
#define H265_ENCODER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <mutex>
#include <queue>
#include <memory>

#include<opencv2/opencv.hpp>

#if defined ( __cplusplus)
extern "C"
{
#include "x265.h"
};
#else
#include "x265.h"
#endif

using namespace cv;
using namespace std;

class H265EncoderTang
{
public:
	int yuvFormat;
	int width;
	int height;
	int fps;

private:
	int yuvLen;
	Mat yuvImg;
	bool bStop;  //子线程结束
	x265_param* pParam;
	x265_encoder* pHandle;
	x265_picture* pPic_in;
	shared_ptr<unsigned char> yuvBuffer;

	mutex g_i_mutex;
	queue <shared_ptr<unsigned char>> frameQue;	//线程通信的队列
	queue <bool> stopQue;


public:
	H265EncoderTang();
	~H265EncoderTang();
	H265EncoderTang(int width, int height, int fps);
	void mainLoop();
	void encodeNewFrame(Mat frame);
	void release();

private:
	shared_ptr<unsigned char> getYuv();
};

#endif


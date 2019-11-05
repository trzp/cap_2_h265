#pragma once
#ifndef H265_ENCODER_H
#define H265_ENCODER_H

#include "UdpSocket.h"
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

//申明了一个较大的pNal_buf用于接收一帧码流，此设置可能需要具体的图片大小来决定
//如果程序提示stack overflow，可将 项目属性-->链接器-->系统-->堆栈保留大小”设大一点


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
	UdpSocket udp;
	bool upd_on = false;
	char pNal_buf[921600];  //分配一个比较大的内存接受pNal码流

	FILE* fp_dst;
	bool file_on = false;

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
	void coderInit(int width, int height, int fps, string filename);
	void coderInit(int width, int height, int fps, string ip, short port);
	void mainLoop();
	void encodeNewFrame(Mat frame);
	void release();

private:
	void init_encoder(int width, int height, int fps);
	shared_ptr<unsigned char> getYuv();
};

#endif


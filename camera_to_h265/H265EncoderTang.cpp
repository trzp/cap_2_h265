#include "H265EncoderTang.h"

//在类外启动线程执行mainLoop
//thread th(&H265EncoderTang::mainLoop, std::ref(coder265)); //coder265为类的实例
//th.detach();

//h265编码相同的内容，为h264的大约1/4
//h265效率更高，画质更好

H265EncoderTang::H265EncoderTang()
{
}

H265EncoderTang::~H265EncoderTang()
{
}

void H265EncoderTang::init_encoder(int width, int height, int fps)
{
	this->width = width;
	this->height = height;
	this->yuvFormat = X265_CSP_I420;
	this->fps = fps;

	this->bStop = false;

	this->pParam = x265_param_alloc();
	x265_param_default(this->pParam);
	this->pParam->bRepeatHeaders = 1;
	this->pParam->internalCsp = this->yuvFormat;
	this->pParam->sourceWidth = this->width;
	this->pParam->sourceHeight = this->height;
	this->pParam->fpsNum = this->fps;
	this->pParam->fpsDenom = 1;

	this->pHandle = x265_encoder_open(this->pParam);
	if (this->pHandle == NULL) {
		printf("x265_encoder_open err\n");
		throw;
	}
	int y_size = pParam->sourceWidth * pParam->sourceHeight;

	this->pPic_in = x265_picture_alloc();
	x265_picture_init(this->pParam, this->pPic_in);
	char* buff = NULL;  //用于初始化

	switch (yuvFormat) {
	case X265_CSP_I444: {
		buff = (char*)malloc(y_size * 3);
		this->pPic_in->planes[0] = buff;
		this->pPic_in->planes[1] = buff + y_size;
		this->pPic_in->planes[2] = buff + y_size * 2;
		this->pPic_in->stride[0] = this->width;
		this->pPic_in->stride[1] = this->width;
		this->pPic_in->stride[2] = this->width;
		break;
	}
	case X265_CSP_I420: {
		buff = (char*)malloc(y_size * 3 / 2);
		this->pPic_in->planes[0] = buff;
		this->pPic_in->planes[1] = buff + y_size;
		this->pPic_in->planes[2] = buff + y_size * 5 / 4;
		this->pPic_in->stride[0] = this->width;
		this->pPic_in->stride[1] = this->width / 2;
		this->pPic_in->stride[2] = this->width / 2;
		break;
	}
	default: {
		printf("Colorspace Not Support.\n");
		throw;
	}
	}

	this->yuvLen = this->width * this->height * 3 / 2;
	this->yuvBuffer = shared_ptr<unsigned char>(new unsigned char[this->yuvLen], [](unsigned char* p) {delete[] p; });
}

void H265EncoderTang::coderInit(int width, int height, int fps,string filename)
{
	char* file = (char*)filename.data();
	this->fp_dst = fopen(file, "wb");
	this->file_on = true;
	this->init_encoder(width,height,fps);
}

void H265EncoderTang::coderInit(int width, int height, int fps, string ip, short port)
{
	this->udp.set_remote_addr(ip,port);
	this->upd_on = true;
	this->init_encoder(width, height, fps);
}

void H265EncoderTang::encodeNewFrame(Mat frame)
{
	cvtColor(frame, this->yuvImg, CV_BGR2YUV_I420);
	memcpy(this->yuvBuffer.get(), this->yuvImg.data, this->yuvLen * sizeof(unsigned char));
	lock_guard<mutex> lock(this->g_i_mutex);
	this->frameQue.push(this->yuvBuffer);
}


void H265EncoderTang::mainLoop() 
{
	shared_ptr<unsigned char> yuvBufTransfer;
	x265_nal* pNals = NULL;
	uint32_t iNal = 0;
	int ret;

	int count = 0;
	bool w = false;

	while (!this->bStop)
	{
		count++;
		yuvBufTransfer = getYuv();

		if (yuvBufTransfer)
		{
			switch (yuvFormat) {
			case X265_CSP_I444: {
				this->pPic_in->planes[0] = yuvBufTransfer.get();                        //Y
				this->pPic_in->planes[1] = yuvBufTransfer.get() + this->height * this->width;            //U
				this->pPic_in->planes[2] = yuvBufTransfer.get() + this->height * this->width * 2;        //V
				break; }
			case X265_CSP_I420: {
				this->pPic_in->planes[0] = yuvBufTransfer.get();
				this->pPic_in->planes[1] = yuvBufTransfer.get() + this->height * this->width;
				this->pPic_in->planes[2] = yuvBufTransfer.get() + this->height * this->width * 5 / 4;
				break; }
			default: {
				printf("Colorspace Not Support.\n");
			}
			}

			ret = x265_encoder_encode(this->pHandle, &pNals, &iNal, this->pPic_in, NULL);
			
			//写文件
			if (this->file_on)
			{
				for (int j = 0; j < iNal; j++) {
					fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
				}
			}

			//udp发送
			if (this->upd_on)
			{
				for (int j = 0; j < iNal; j++) {
					memcpy(this->pNal_buf, pNals[j].payload, pNals[j].sizeBytes); //将码流数据复制到buffer中
					this->udp.send(this->pNal_buf,pNals[j].sizeBytes);
				}
			}
		}
		else
		{
			this_thread::sleep_for(chrono::milliseconds(5));
		}
	}
	
	//flush残余码流
	if (this->file_on)
	{
		
		while (1) {
			ret = x265_encoder_encode(pHandle, &pNals, &iNal, NULL, NULL);
			if (ret == 0) {
				break;
			}
			for (int j = 0; j < iNal; ++j) {
				fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_dst);
			}
		}
	}

	//释放资源
	x265_encoder_close(this->pHandle);
	x265_picture_free(this->pPic_in);
	x265_param_free(this->pParam);

	if (this->file_on)
		fclose(fp_dst);

	this->stopQue.push(true);
}

void H265EncoderTang::release()
{
	this->bStop = true;
	while (this->stopQue.empty())
	{
		this_thread::sleep_for(chrono::milliseconds(100));//等待线程结束
	}
	
}

shared_ptr<unsigned char> H265EncoderTang::getYuv()
	{
		shared_ptr<unsigned char> yuvBuf(nullptr);
		{
			lock_guard<mutex> lock(this->g_i_mutex);

			while (!this->frameQue.empty())	//取最新帧，且清空。避免帧速过高时处理不过来
			{
				yuvBuf = this->frameQue.back();
				this->frameQue.pop();
			}

		}
		return yuvBuf;
	}

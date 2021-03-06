//all the include and pragma information
#include "stdafx.h"
#include "javaJniTest.h"
#include "jni.h"
#pragma  warning(disable:4996)
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <iostream>
#include <windows.h>
#include <time.h>
#define HAVE_STRUCT_TIMESPEC
#include "pthread.h"
extern "C" {
#include <libavutil/opt.h>  
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"pthreadVC2.lib")
#pragma comment(lib,"swscale.lib")
#define __STDC_CONSTANT_MACROS
using namespace std;

//define global variables
LPDIRECT3D9 g_pD3D = NULL;
D3DDISPLAYMODE ddm;
D3DPRESENT_PARAMETERS d3dpp;
IDirect3DDevice9 * g_pd3dDevice;
IDirect3DSurface9 * pSurface;
JavaVM *g_VM;
jobject g_obj;
//buffer is a data structure that holds the data of exactly one pixel
//struct buffer {
//	byte *frame;
//	buffer() {
//		frame = new byte[1920 * 1080 * 3];
//	}
//};
int in_w = 1920, in_h = 1080;
struct buffer {
	AVFrame *frame;
	uint8_t* frame_buffer;
	int num_bytes = avpicture_get_size(AV_PIX_FMT_YUV420P, in_w, in_h);
	buffer() {
		//cout << "constructor" << endl;
		frame = av_frame_alloc();
	     frame_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
		avpicture_fill((AVPicture*)frame, frame_buffer, AV_PIX_FMT_YUV420P, in_w, in_h);
	}
	buffer(const buffer& tempbuffer)
	{
		//cout << "copy constructor" << endl;
		frame = av_frame_alloc();
		frame_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
		avpicture_fill((AVPicture*)frame, frame_buffer, AV_PIX_FMT_YUV420P, in_w, in_h);
		memcpy(frame, tempbuffer.frame, sizeof(frame));
		memcpy(frame_buffer, tempbuffer.frame_buffer, num_bytes);
	}
	buffer& operator =(const buffer& tempbuffer)
	{
		//cout << "operator" << endl;
		memcpy(this->frame, tempbuffer.frame, sizeof(frame));
		memcpy(this->frame_buffer, tempbuffer.frame_buffer, num_bytes);
		return *this;
	}
	~buffer() {
		//cout << "deconstructed" << endl;
		av_frame_free(&frame);
	}
};





//queue is a generic loop queue, which means the head and rear will wrapback when they hit the end
template <typename T>
class queue {
public:
	T * tmp;
	T * test;
	int head;
	int rear;
	int size;

	queue(int size) {
		test = new T[size];
		tmp = new T();
		head = 0;
		rear = 0;
		this->size = size;
	}
	~queue() {
		//cout << "deleting tmp" << endl;
		delete tmp;
		//cout << "deleted tmp" << endl;
		//cout << "deleting test" << endl;
		delete []test;
		//cout << "deleted test" << endl;
	}

	void push(T* target) {
		//cout << "producer is at"<<rear << endl;
		test[rear] = *target;
		rear = (rear + 1) % size;
	}

	T* pop() {
		//cout << "consumer is at" << head << endl;
		//cout << "is poping" << endl;
		*tmp = test[head];
		head = (head + 1) % size;
		//cout << "is returning" << endl;
		return tmp;
	}
	bool isEmpty() {
		return head == rear;
	}
	bool isFull() {
		return (rear + 1) % size == head;
	}
};


//initialize the configuration of d3d related 
void d3dini() {

	ZeroMemory(&d3dpp, sizeof(D3DPRESENT_PARAMETERS));
	//ZeroMemory(&d3dpp, sizeof(d3dpp));
	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm);
	d3dpp.Windowed = TRUE;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.BackBufferFormat = ddm.Format;
	d3dpp.BackBufferHeight = ddm.Height;
	d3dpp.BackBufferWidth = ddm.Width;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.hDeviceWindow = GetDesktopWindow();
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);
	if (hr == D3D_OK) {
		cout << "Created Device" << endl;
		g_pd3dDevice->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &pSurface, NULL);
	}
	else if (hr == D3DERR_DEVICELOST)
	{
		cout << "D3DERR_DEVICELOST" << endl;
	}
	else if (hr == D3DERR_INVALIDCALL)
	{
		cout << "D3DERR_INVALIDCALL" << endl;
	}
	else if (hr == D3DERR_NOTAVAILABLE)
	{
		cout << "D3DERR_NOTAVAILABLE" << endl;
	}
	else if (hr == D3DERR_OUTOFVIDEOMEMORY)
	{
		cout << "D3DERR_OUTOFVIDEOMEMORY" << endl;
	}
	else
		cout << hr << endl;
}
//int Capture()
//{
	//string address ="d:/1.png";
	//LPCSTR address_pointer = NULL;
	//address_pointer = address.c_str();
	//g_pd3dDevice->GetFrontBufferData(0, pSurface);
		//LPD3DXBUFFER bufferedImage = NULL;
		//D3DXSaveSurfaceToFileInMemory(&bufferedImage, D3DXIFF_PNG, pSurface, NULL, NULL);
		//bufferedImage->Release();
	//hr = D3DXSaveSurfaceToFile(address_pointer, D3DXIFF_PNG, pSurface, NULL, NULL);
//}

//flush out the remaining data in the encoder
int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;
	while (1) {
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame) {
			ret = 0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}



int webSocketCall(void *opaque, uint8_t *buf, int buf_size) {
	JNIEnv *env;
	bool attached;
	int getEnvStat = g_VM->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		char threadName[] = "consumer";
		JavaVMAttachArgs jvmargs_consumer = { JNI_VERSION_1_6,threadName,NULL };
		if (g_VM->AttachCurrentThread((void **)&env, (void *)&jvmargs_consumer) != 0) {
			return 0;
		}
		cout << "Consumer thread successfully attached" << endl;
		attached = true;
	}
	jbyteArray arr = env->NewByteArray(buf_size);
	env->SetByteArrayRegion(arr, 0, buf_size, (jbyte *)buf);
	jclass javaClass = env->GetObjectClass(g_obj);
	if (javaClass == 0) {
		cout << ("can not find the class") << endl;
		g_VM->DetachCurrentThread();
		return 0;
	}
	jmethodID javaCallbackId = env->GetMethodID(javaClass, "webSocketTransmit", "(I[B)I");
	if (javaCallbackId == NULL) {
		cout << "can not find the method" << endl;
		return 0;
	}
	env->CallIntMethod(g_obj, javaCallbackId, buf_size,arr);
	return 1;
}


//retrieve the frame data from the loop queue and encode it
void* frameConsumer(void * loopQueue) {
	JNIEnv *env;
	jboolean running;
	int getEnvStat = g_VM->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		char threadName[] = "consumer";
		JavaVMAttachArgs jvmargs_consumer = { JNI_VERSION_1_6,threadName,NULL };
		if (g_VM->AttachCurrentThread((void **)&env, (void *)&jvmargs_consumer) != 0) {
			return 0;
		}
		cout << "Consumer thread successfully attached" << endl;
	}
	jclass javaClass = env->GetObjectClass(g_obj);
	if (javaClass == 0) {
		cout << ("can not find the class") << endl;
		g_VM->DetachCurrentThread();
		return 0;
	}
	jfieldID runningFlag= env->GetFieldID(javaClass, "runningFlag", "Z");
	int delayed_frame = 200;
	int frame_skipped = 0;
	//int num_adjusting_frame = 20;
	queue<buffer> * tempQueue= (queue<buffer> *)loopQueue;
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_st;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVPacket pkt;
	uint8_t* picture_buf;
	int frameIndex = 0;
	//AVFrame* pFrame;
	int picture_size;
	//int y_size;
	const char* out_file = "D://ds.mp4";
	
	av_register_all();
	pFormatCtx = avformat_alloc_context();
	fmt = av_guess_format(NULL, out_file, NULL);
	pFormatCtx->oformat = fmt;
	/*if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
		printf("Failed to open output file! \n");
		return 0;
	}*/
	unsigned char* outbuffer = (unsigned char*)av_malloc(4000);
	AVIOContext *avio_out = avio_alloc_context(outbuffer, 4000, 1, NULL, NULL, webSocketCall, NULL);
	pFormatCtx->pb = avio_out;
	pFormatCtx->flags|= AVFMT_FLAG_CUSTOM_IO;
	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st == NULL) {
		return 0;
	}
	pCodecCtx = video_st->codec;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_w;
	pCodecCtx->height = in_h;
	pCodecCtx->bit_rate = 8000;
	pCodecCtx->gop_size = 5;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 30;
	video_st->time_base.num = 1;
	video_st->time_base.den = 30;
	//video_st->codec->extradata = pCodecCtx->extradata;
	//video_st->avg_frame_rate.den = 1;
	//video_st->avg_frame_rate.num = 15;
	pCodecCtx->qmin = 15;
	pCodecCtx->qmax = 35;
	pCodecCtx->flags = pCodecCtx->flags|CODEC_FLAG_GLOBAL_HEADER;
	//pCodecCtx->max_b_frames =2;
	AVDictionary *param = 0;
	avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);
	//H.264  
	if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "superfast", 0);
		av_dict_set(&param, "tune", "zerolatency+fastdecode", 0);
		av_dict_set(&param, "profile", "baseline", 0);
		av_dict_set(&param, "movflags", "empty_moov+default_base_moof+frag_keyframe", 0);
	}
	//H.265  
	if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}
	pCodec = avcodec_find_encoder_by_name("libx264");
	if (!pCodec) {
		printf("Can not find encoder! \n");
		return 0;
	}
	if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
		printf("Failed to open encoder! \n");
		return 0;
	}
	if (pCodecCtx->extradata_size > 0) {
		int extra_size = (uint64_t)pCodecCtx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE;
		video_st->codecpar->extradata = (uint8_t*)av_mallocz(extra_size);
		memcpy(video_st->codecpar->extradata, pCodecCtx->extradata, pCodecCtx->extradata_size);
		video_st->codecpar->extradata_size = pCodecCtx->extradata_size;
	}
	//pFrame = av_frame_alloc();
	picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	//picture_buf = (uint8_t *)av_malloc(picture_size);
	//avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	//Write File Header  
	avformat_write_header(pFormatCtx, &param);
	av_new_packet(&pkt, picture_size);
	//y_size = pCodecCtx->width * pCodecCtx->height;
	CoInitialize(NULL);
	buffer* temp = new buffer();
	while (1) {
		running = env->GetBooleanField(g_obj,runningFlag);
		if (!running) {
			cout << "Stopping consumer thread" << endl;
			break;
		}
		while (tempQueue->isEmpty()) {
			//cout << "is empty now!!" << endl;
			Sleep(1);
		}
		frameIndex++;
		if (frameIndex <= delayed_frame) {
			tempQueue->pop();
			continue;
		}
		/*if (frameIndex % 1000 == 0) {
				tempQueue->pop();
				frame_skipped++;
				continue;
		}*/
		//byte* tempFrame = (tempQueue->pop()).frame;
		//pFrame->data[0] = tempFrame;
		//pFrame->pts = av_rescale_q(i, pCodecCtx->time_base, video_st->codec->time_base);
		*temp = *(tempQueue->pop());
		AVFrame* framedata = temp->frame;
		picture_buf = temp->frame_buffer;
		framedata->pts = frameIndex-delayed_frame-frame_skipped;
		framedata->width = in_w;
		framedata->height = in_h;
		framedata->format = AV_PIX_FMT_YUV420P;
		//pFrame->pts =i;
		//pFrame->width = 1920;
		//pFrame->height = 1080;
		//pFrame->format = 2;
		int got_picture = 0;
		int ret = avcodec_encode_video2(pCodecCtx, &pkt, framedata, &got_picture);
		/*if (pkt.pts != AV_NOPTS_VALUE) {
			pkt.pts = av_rescale_q(pCodecCtx->coded_frame->pts, pCodecCtx->time_base, video_st->time_base);
			cout << "pts ok " << pCodecCtx->coded_frame->pts<<endl;
		}
		if (pkt.dts != AV_NOPTS_VALUE) {
			pkt.dts = av_rescale_q(i, pCodecCtx->time_base, video_st->time_base);
			cout << "dts ok" << endl;
		}*/
		//pkt.pts = av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base);
		//pkt.dts = av_rescale_q(pkt.dts, video_st->codec->time_base, video_st->time_base);
		/*if (i == 11) {
			cout << "wozaizheli" << endl;
			cout <<unsigned( *(pkt.data+5) )<< endl;
			cout << pkt.size << endl;
		}*/
		if (ret < 0) {
			printf("Failed to encode! \n");
			return 0;
		}
		if (got_picture == 1) {
			//printf("Succeed to encode frame: %5d\tsize:%5d\n", frameIndex, pkt.size);
			pkt.pts = av_rescale_q(pkt.pts, pCodecCtx->time_base, video_st->time_base);
			pkt.dts = av_rescale_q(pkt.dts, pCodecCtx->time_base, video_st->time_base);
			//pkt.duration = 1;
			//pkt.pos = -1;
			//av_packet_rescale_ts(&pkt, pCodecCtx->time_base, video_st->time_base);
			//pkt.stream_index = video_st->index;
			//ret = av_write_frame(pFormatCtx, &pkt);
			ret = av_interleaved_write_frame(pFormatCtx, &pkt);
			av_free_packet(&pkt);
			//av_free(framedata);
			//av_free(picture_buf);
		}
	}
	int ret = flush_encoder(pFormatCtx, 0);
	if (ret < 0) {
		printf("Flushing encoder failed\n");
		return 0;
	}
	cout << "flush succeed" << endl;
	av_write_trailer(pFormatCtx);
	if (video_st) {
		avcodec_close(video_st->codec);
		//av_free(framedata);
		//av_free(picture_buf);
	}
	//avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
	av_freep(&avio_out->buffer);
	av_freep(&avio_out);
	CoUninitialize();
	//cout << ("deleting temp") << endl;
	delete temp;
	//cout << ("deleted temp") << endl;
	g_VM->DetachCurrentThread();
	return 0;
}

//extract the RGB data from the ARGB surface and feed it to the loop queue
SwsContext * ctx = sws_getContext(in_w, in_h, AV_PIX_FMT_BGRA, in_w, in_h, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
const int linesize[1] = { 4 * in_w };
void* frameProducer(void* loopQueue) {
	JNIEnv *env;
	jboolean running;
	int getEnvStat = g_VM->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		char threadName[] = "producer";
		JavaVMAttachArgs jvmargs_consumer = { JNI_VERSION_1_6,threadName,NULL };
		if (g_VM->AttachCurrentThread((void **)&env, (void *)&jvmargs_consumer) != 0) {
			return 0;
		}
		cout << "Producer thread successfully attached" << endl;
	}
	jclass javaClass = env->GetObjectClass(g_obj);
	if (javaClass == 0) {
		cout << ("can not find the class") << endl;
		g_VM->DetachCurrentThread();
		return 0;
	}
	jfieldID runningFlag = env->GetFieldID(javaClass, "runningFlag", "Z");
	queue<buffer> * tempQueue = (queue<buffer> *)loopQueue;
	//DWORD begin, end, time_ms, initial;
	//DWORD test1, test2;
	//begin = ::GetTickCount();
	//initial = begin;
	//while (begin-initial<40000) {
		//end = ::GetTickCount();
		//time_ms = end - begin;
		//if (time_ms >= 30) {
	int i = 0;
	buffer *frameData = new buffer();
	while (1) {
		running = env->GetBooleanField(g_obj, runningFlag);
		if (!running) {
			cout << "Stopping producer thread" << endl;
			break;
		}
		while (tempQueue->isFull()) {
			//cout << "is full now!!" << endl;
			Sleep(1);
		}
		//LPDIRECT3DSURFACE9 back = NULL;
		//test1 = ::GetTickCount();
		g_pd3dDevice->GetFrontBufferData(0, pSurface);
		//g_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface);
		//test2 = ::GetTickCount();
		//cout << test2 - test1 << endl;

		D3DLOCKED_RECT d3d_rect;
		pSurface->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
		//byte *processedFrame = new byte[1920 * 1080 * 3];
		byte* source = (byte *)d3d_rect.pBits;
		sws_scale(ctx,(const uint8_t * const *)&source,linesize,0,in_h,frameData->frame->data, frameData->frame->linesize);
		//memcpy(&processedFrame, frameData.frame, 1920 * 1080 * 3 * sizeof(byte));
		tempQueue->push(frameData);
		pSurface->UnlockRect();
		i++;
	}
			//begin = end;
		//}
	//}
	g_VM->DetachCurrentThread();
	//cout<<("deleting framedata") << endl;
	delete frameData;
	//cout << ("deleted framedata") << endl;
	return 0;
}

//void* frameProducer(void* loopQueue) {
//	cout << "haha" << endl;
//	queue<buffer> * tempQueue = (queue<buffer> *)loopQueue;
//	buffer frameData;
//	cout << 1 << endl;
//	return 0;
//}



//int _tmain(int argc, _TCHAR* argv[]) {
//	queue<buffer>* test = new queue<buffer>(100);
//	pthread_t producer;
//	pthread_t consumer;
//	pthread_create(&producer, NULL, frameProducer, (void*)test);
//	pthread_create(&consumer, NULL, frameConsumer, (void*)test);
//	pthread_exit(NULL);
//}


//main function
//int main() {
//	queue<buffer>* test = new queue<buffer>(500);
//	pthread_t producer;
//	pthread_t consumer;
//	d3dini();
//	pthread_create(&producer, NULL, frameProducer, (void*)test);
//	//cout << "haha" << endl;
//	pthread_create(&consumer, NULL, frameConsumer, (void*)test);
//	pthread_exit(NULL);
//}

JNIEXPORT void JNICALL Java_javaJniTest_streamingStart(JNIEnv * env, jobject thiz) {
		env->GetJavaVM(&g_VM);
		g_obj = env->NewGlobalRef(thiz);
		queue<buffer>* test = new queue<buffer>(5);
		pthread_t producer;
		pthread_t consumer;
		d3dini();
		pthread_create(&producer, NULL, frameProducer, (void*)test);
		pthread_create(&consumer, NULL, frameConsumer, (void*)test);
		pthread_join(producer, NULL);
		pthread_join(consumer, NULL);
		//pthread_exit(NULL);
		env->DeleteGlobalRef(g_obj);
		cout << "deleting loopqueue" << endl;
		delete test;
		cout << "deleted loopqueue" << endl;
		cout << "Streaming ended" << endl;
}

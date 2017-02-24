// 2015-07-02 <brion@pobox.com>

#include <memory>
#include <emscripten/bind.h>
#include "codec_api.h"

using namespace emscripten;

struct H264Plane {
	unsigned char *data;
	int stride;
	int height;
	int width;
	int offset;
	int skip;
	
	H264Plane();
	H264Plane(unsigned char *_data, int _stride, int _height, int _width, int _offset, int _skip);
	val data_getter() const;
	void data_setter(val v);
};

struct H264Frame {
	int width;
	int height;
	H264Plane y;
	H264Plane cb;
	H264Plane cr;

	H264Frame();
	H264Frame(int _width, int _height, H264Plane _y, H264Plane _cb, H264Plane _cr);
};

class H264Decoder {
	ISVCDecoder *mDecoder;
	int mWidth;
	int mHeight;

public:
	H264Decoder();
	~H264Decoder();

	void initVideo(int frameWidth, int frameHeight);
	H264Frame input(int offset, int time, int timecode, int duration, bool keyframe, bool discontinuity, std::string buffer);
};

// ----

H264Plane::H264Plane(unsigned char *_data, int _stride, int _height, int _width, int _offset, int _skip) :
	data(_data),
	stride(_stride),
	height(_height),
	width(_width),
	offset(_offset),
	skip(_skip)
{}

H264Plane::H264Plane() :
	data(NULL),
	stride(0),
	height(0),
	width(0),
	offset(0),
	skip(0)
{}
	
val H264Plane::data_getter() const
{
	return val(memory_view<unsigned char>(stride * height, (unsigned char *)data));
}

void H264Plane::data_setter(val v)
{
	// stub, needed to compile bindings
}

// ----

H264Frame::H264Frame(int _width, int _height, H264Plane _y, H264Plane _cb, H264Plane _cr) :
	width(_width),
	height(_height),
	y(_y),
	cb(_cb),
	cr(_cr)
{}

H264Frame::H264Frame() :
	width(0),
	height(0),
	y(),
	cb(),
	cr()
{}

H264Decoder::H264Decoder()
{
	WelsCreateDecoder(&mDecoder);
}

H264Decoder::~H264Decoder()
{
	if (mDecoder) {
		mDecoder->Uninitialize();
		delete mDecoder;
	}
}

void H264Decoder::initVideo(int frameWidth, int frameHeight)
{
	mWidth = frameWidth;
	mHeight = frameHeight;

    SDecodingParam s = {0};
    //s.eOutputColorFormat = videoFormatI420;
    s.uiTargetDqLayer = (uint8_t) - 1;
    s.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_SVC;

	int ret = mDecoder->Initialize(&s);
	// zero is success
	//printf("decoder->Initialize returns %d\n", ret);
}

H264Frame H264Decoder::input(int offset, int time, int timecode, int duration, bool keyframe, bool discontinuity, std::string buffer)
{
	unsigned const char *pSrc = (unsigned const char *)buffer.data();
	int iSrcLen = buffer.length();
	//printf("input src %ld len %d\n", pSrc, iSrcLen);

	unsigned char *pDst[3] = {0};
	SBufferInfo sDstInfo = {0};
	int ret = mDecoder->DecodeFrame2(pSrc, iSrcLen, &pDst[0], &sDstInfo);
	// zero is success
	//printf("decoder->DecodeFrame2 returns %d; status %d\n", ret, sDstInfo.iBufferStatus);

	// @todo are the crop params needed?
	H264Plane y(pDst[0], sDstInfo.UsrData.sSystemBuffer.iStride[0], sDstInfo.UsrData.sSystemBuffer.iHeight, sDstInfo.UsrData.sSystemBuffer.iWidth, 0, 0);
	H264Plane cb(pDst[1], sDstInfo.UsrData.sSystemBuffer.iStride[1], sDstInfo.UsrData.sSystemBuffer.iHeight / 2, sDstInfo.UsrData.sSystemBuffer.iWidth / 2, 0, 0);
	H264Plane cr(pDst[2], sDstInfo.UsrData.sSystemBuffer.iStride[1], sDstInfo.UsrData.sSystemBuffer.iHeight / 2, sDstInfo.UsrData.sSystemBuffer.iWidth / 2, 0, 0);
	H264Frame frame(mWidth, mHeight, y, cb, cr);

	return frame;
}

// ----

EMSCRIPTEN_BINDINGS(h264_decoder) {

	value_object<H264Plane>("H264Plane")
		.field("data", &H264Plane::data_getter, &H264Plane::data_setter)
		.field("stride", &H264Plane::stride)
		.field("height", &H264Plane::height)
		.field("width", &H264Plane::width)
		.field("offset", &H264Plane::offset)
		.field("skip", &H264Plane::skip)
		;

	value_object<H264Frame>("H264Frame")
		.field("width", &H264Frame::width)
		.field("height", &H264Frame::height)
		.field("y", &H264Frame::y)
		.field("cb", &H264Frame::cb)
		.field("cr", &H264Frame::cr)
		;

	class_<H264Decoder>("H264Decoder")
		.smart_ptr_constructor("H264DecoderPtr", &std::make_shared<H264Decoder>)
		.function("initVideo", &H264Decoder::initVideo)
		.function("input", &H264Decoder::input)
		;

}

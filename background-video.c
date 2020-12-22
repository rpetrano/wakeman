#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <cairo.h>
#include "log.h"

cairo_surface_t *load_background_video(const char *path) {
	cairo_surface_t *image;

	AVFormatContext *pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0) {
		wakeman_log(LOG_ERROR, "Failed to read background video.");
		return NULL;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		wakeman_log(LOG_ERROR, "Failed to load video streams.");
		return NULL;
	}

	av_dump_format(pFormatCtx, 0, path, 0);

	int videoStream = -1;
	for (unsigned int i  = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		wakeman_log(LOG_ERROR, "Failed to find a video stream.");
		return NULL;
	}

	AVCodecParameters *pCodecPar = pFormatCtx->streams[videoStream]->codecpar;
	AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
	if (pCodec == NULL) {
		wakeman_log(LOG_ERROR, "Unsupported video codec.");
		return NULL;
	}

	AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_parameters_to_context(pCodecCtx, pCodecPar) < 0) {
		wakeman_log(LOG_ERROR, "Can't convert codec parameters.");
		return NULL;
	}

	AVDictionary *optionsDict = NULL;
	if (avcodec_open2(pCodecCtx, pCodec, &optionsDict) < 0) {
		wakeman_log(LOG_ERROR, "Could not open video codec.");
		return NULL;
	}

	AVFrame *pFrame    = av_frame_alloc(),
			*pFrameRGB = av_frame_alloc();

	if (pFrame == NULL || pFrameRGB == NULL) {
		wakeman_log(LOG_ERROR, "Can't allocate AV frame.");
		return NULL;
	}

	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_ARGB, pCodecCtx->width, pCodecCtx->height, 1); // try 32, 16 for SMD
	uint8_t *buffer = (uint8_t *) av_malloc(numBytes*sizeof(uint8_t));

	struct SwsContext *sws_ctx = sws_getContext(
		pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		pCodecCtx->width,
		pCodecCtx->height,
		AV_PIX_FMT_ARGB,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	/*
	av_image_fill_arrays(
		pFrameRGB->data,
		pFrameRGB->linesize,
		buffer,
		AV_PIX_FMT_ARGB,
		pCodecCtx->width,
		pCodecCtx->height,
		1
	); // try 32, 16 for SMD
	*/

	AVPacket packet;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			avcodec_send_packet(pCodecCtx, &packet);
			while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
				int x = sws_scale(
					sws_ctx,
					pFrame->data,
					pFrame->linesize,
					0,
					pFrame->height,
					pFrameRGB->data,
					pFrameRGB->linesize
				);
				int stride = cairo_format_stride_for_width(
						CAIRO_FORMAT_RGB24, pFrameRGB->width);
				image = cairo_image_surface_create_for_data(
					(unsigned char *) pFrameRGB->data,
					CAIRO_FORMAT_RGB24,
					pFrameRGB->width,
					pFrameRGB->height,
					stride
				);
			}
		}
		av_packet_unref(&packet);
		break;
	}

	av_free(buffer);
	av_free(pFrameRGB);
	av_free(pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	wakeman_log(LOG_ERROR, "testkovic");

	if (!image) {
		wakeman_log(LOG_ERROR, "Failed to process video.");
		return NULL;
	}

	if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
		wakeman_log(LOG_ERROR, "Failed to read background video: %s."
				, cairo_status_to_string(cairo_surface_status(image)));
		return NULL;
	}
	return image;
}

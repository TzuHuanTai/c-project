#include <iostream>
#include <cstdlib>
#include <sstream>
#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

// g++ -g streaming_v4l2.cpp -o streaming_v4l2 -lavformat -lavcodec -lavutil -lswscale -lavdevice -pthread
// ffmpeg -f v4l2 -list_formats all -i /dev/video0
// v4l2-ctl --list-devices

using namespace std;
uint64_t frame_count = 0;

void initialize_avformat_context(AVFormatContext *&fctx, const char *format_name)
{
  if (avformat_alloc_output_context2(&fctx, nullptr, format_name, nullptr) < 0)
  {
    cout << "Could not allocate output format context!" << endl;
    exit(1);
  }
}

void initialize_io_context(AVFormatContext *&fctx, const char *output)
{
  if (!(fctx->oformat->flags & AVFMT_NOFILE))
  {
    if (avio_open2(&fctx->pb, output, AVIO_FLAG_WRITE, nullptr, nullptr) < 0)
    {
      cout << "Could not open output IO context!" << endl;
      exit(1);
    }
  }
}

void set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx, double width, double height, int fps)
{
  const AVRational dst_fps = {fps, 1};

  codec_ctx->codec_tag = 0;
  codec_ctx->codec_id = AV_CODEC_ID_H264;
  codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_ctx->width = width;
  codec_ctx->height = height;
  codec_ctx->bit_rate = 6000000;
  codec_ctx->gop_size = 12;
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx->framerate = dst_fps;
  codec_ctx->time_base = av_inv_q(dst_fps);
  if (fctx->oformat->flags & AVFMT_GLOBALHEADER)
  {
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }
}

void initialize_codec_stream(AVStream *&stream, AVCodecContext *&codec_ctx, AVCodec *&codec, int fps)
{
  const AVRational dst_fps = {fps, 1};
  stream->avg_frame_rate = dst_fps;
  stream->r_frame_rate = dst_fps;

  AVDictionary *codec_options = nullptr;
  av_dict_set(&codec_options, "profile", "baseline", 0);

  // open video encoder
  if (avcodec_open2(codec_ctx, codec, &codec_options) < 0)
  {
    cout << "Could not open video encoder!" << endl;
    exit(1);
  }

  if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0)
  {
    cout << "Could not initialize stream codec parameters!" << endl;
    exit(1);
  }
}

SwsContext *initialize_sample_scaler(AVCodecContext *in_codec_ctx, AVCodecContext *out_codec_ctx, double width, double height)
{
  cout << "v4l2 decoder id: " << in_codec_ctx->codec_id << endl;
  cout << "v4l2 decoder fmt: " << in_codec_ctx->pix_fmt << endl;
  cout << "v4l2 decoder framerate: " << in_codec_ctx->framerate.num << endl;
  cout << "v4l2 decoder time_base: " << in_codec_ctx->time_base.num << "/" << in_codec_ctx->time_base.den << endl;
  cout << "x264 encoder id: " << out_codec_ctx->codec_id << endl;
  cout << "x264 encoder fmt: " << out_codec_ctx->pix_fmt << endl;
  cout << "x264 encoder framerate: " << out_codec_ctx->framerate.num << endl;
  cout << "x264 encoder time_base: " << out_codec_ctx->time_base.num << "/" << out_codec_ctx->time_base.den << endl;

  SwsContext *swsctx = sws_getContext(width, height, in_codec_ctx->pix_fmt, width, height, out_codec_ctx->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);

  if (!swsctx)
  {
    cout << "Could not initialize sample scaler!" << endl;
    exit(1);
  }

  return swsctx;
}

AVFrame *allocate_frame_buffer(AVCodecContext *codec_ctx, double width, double height)
{
  AVFrame *frame = av_frame_alloc();

  int buffer_size = av_image_get_buffer_size(codec_ctx->pix_fmt, width, height, 1);
  cout << "av frame buffer_size: " << buffer_size << endl;
  uint8_t *framebuf = (uint8_t *)av_malloc(buffer_size);
  av_image_fill_arrays(frame->data, frame->linesize, framebuf, codec_ctx->pix_fmt, width, height, 1);

  frame->width = width;
  frame->height = height;
  frame->format = static_cast<int>(codec_ctx->pix_fmt);
  frame->pts = 0;

  return frame;
}

void write_frame(AVCodecContext *encoder, AVFormatContext *fmt_ctx, AVFrame *frame, AVPacket *pkt)
{
  int ret = avcodec_send_frame(encoder, frame);
  while (ret >= 0)
  {
    ret = avcodec_receive_packet(encoder, pkt);
    if (ret == AVERROR(EAGAIN))
    {
      break;
    }
    else if (ret < 0)
    {
      cout << "Error encoding frame from codec context!" << endl;
      break;
    }

    pkt->pts = pkt->dts = av_rescale_q(frame_count, encoder->time_base, fmt_ctx->streams[0]->time_base);

    av_interleaved_write_frame(fmt_ctx, pkt);
    av_packet_unref(pkt);
  }
}

void read_frame(AVFormatContext *&fmt_ctx, AVCodecContext *&decoder, AVFrame *&frame, AVPacket *pkt)
{
  int ret = av_read_frame(fmt_ctx, pkt);

  ret = avcodec_send_packet(decoder, pkt);
  if (ret < 0)
  {
    cout << "Error decoding packet!" << endl;
  }

  ret = avcodec_receive_frame(decoder, frame);
  if (ret < 0 && ret != AVERROR(EAGAIN))
  {
    cout << "Error decoding frame from codec context!" << endl;
  }

  av_packet_unref(pkt);
}

void initialize_v4l2_context(AVFormatContext *&ifmt_ctx, AVCodecContext *&decoder_ctx, const char *input_format,
                             const char *dev_name, int fps, int width, int height)
{
  ifmt_ctx = avformat_alloc_context();
  if (!ifmt_ctx)
  {
    av_log(0, AV_LOG_ERROR, "Cannot allocate input format (Out of memory?)\n");
    exit(1);
  }
  ifmt_ctx->flags |= AVFMT_FLAG_NONBLOCK; // Enable non-blocking mode

  AVDictionary *options = NULL;
  stringstream framesize;
  framesize << width << "x" << height;
  av_dict_set(&options, "framerate", to_string(fps).c_str(), 0);
  av_dict_set(&options, "video_size", framesize.str().c_str(), 0);

  AVInputFormat *ifmt = av_find_input_format(input_format);
  if (!ifmt)
  {
    av_log(0, AV_LOG_ERROR, "Cannot find input format\n");
    exit(1);
  }

  // open input file, and allocate format context
  if (avformat_open_input(&ifmt_ctx, dev_name, ifmt, &options) < 0)
  {
    av_log(0, AV_LOG_ERROR, "Could not open source %s\n", dev_name);
    exit(1);
  }

  AVCodec *decoder = NULL;

  cout << "v4l2 number of streams: " << ifmt_ctx->nb_streams << endl;
  for (int i = 0; i < ifmt_ctx->nb_streams; i++)
  {
    cout << "v4l2 type: " << ifmt_ctx->streams[i]->codecpar->codec_type << endl;
    cout << "v4l2 id: " << ifmt_ctx->streams[i]->codecpar->codec_id << endl;
  }

  int video_stream_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
  cout << "v4l2 find best video stream: " << video_stream_index << endl;

  auto codec_par = ifmt_ctx->streams[video_stream_index]->codecpar;
  decoder_ctx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(decoder_ctx, codec_par);
  avcodec_open2(decoder_ctx, decoder, NULL);
}

void stream_video(double width, double height, int fps, const char *dev_name, const char *output, const char *encoder_name)
{
  avformat_network_init();
  avdevice_register_all();

  AVFormatContext *ifmt_ctx = nullptr;
  AVFormatContext *ofmt_ctx = nullptr;
  AVCodec *out_codec = nullptr;
  AVStream *out_stream = nullptr;
  AVCodecContext *in_codec_ctx = nullptr;
  AVCodecContext *out_codec_ctx = nullptr;

  cout << " ==== initialize v4l2 input... ====" << endl;
  initialize_v4l2_context(ifmt_ctx, in_codec_ctx, "video4linux2", dev_name, fps, width, height);

  cout << "==== initailize flv output... ====" << endl;
  initialize_avformat_context(ofmt_ctx, "flv");
  initialize_io_context(ofmt_ctx, output);

  out_codec = avcodec_find_encoder_by_name(encoder_name);
  out_stream = avformat_new_stream(ofmt_ctx, out_codec);
  out_codec_ctx = avcodec_alloc_context3(out_codec);

  set_codec_params(ofmt_ctx, out_codec_ctx, width, height, fps);
  initialize_codec_stream(out_stream, out_codec_ctx, out_codec, fps);

  cout << "==== show info ====" << endl;
  av_dump_format(ifmt_ctx, 0, dev_name, 0);
  av_dump_format(ofmt_ctx, 0, output, 1);

  cout << "==== initailize others ====" << endl;
  auto in_pkt = av_packet_alloc();
  auto out_pkt = av_packet_alloc();
  auto *in_frame = allocate_frame_buffer(in_codec_ctx, width, height);

  // ==== start push streaming ====
  cout << "Begin push streaming!" << endl;

  if (avformat_write_header(ofmt_ctx, nullptr) < 0)
  {
    cout << "Could not write header!" << endl;
    exit(1);
  }

  bool end_of_stream = false;
  int64_t video_duration = 0;
  int64_t init_video_time = av_gettime();
  const int sleep_time = av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base) * 1000 / 3;

  cout << "sleep_time: " << sleep_time << endl;

  do
  {
    if (av_gettime() >= video_duration + init_video_time)
    {
      read_frame(ifmt_ctx, in_codec_ctx, in_frame, in_pkt);

      write_frame(out_codec_ctx, ofmt_ctx, in_frame, out_pkt);

      video_duration = av_rescale_q(++frame_count, out_codec_ctx->time_base, out_stream->time_base) * 1000;
    }
    else
    {
      av_usleep(sleep_time);
    }
  } while (!end_of_stream);

  av_write_trailer(ofmt_ctx);

  av_packet_free(&in_pkt);
  av_packet_free(&out_pkt);
  av_frame_free(&in_frame);
  avcodec_close(in_codec_ctx);
  avcodec_close(out_codec_ctx);
  avformat_free_context(ifmt_ctx);
  avformat_free_context(ofmt_ctx);
}

int main(int argc, char *argv[])
{
  // av_log_set_level(AV_LOG_DEBUG);
  // double width = atoi(argv[2]), height = atof(argv[3]);
  // const char *dev_name = argv[1],
  // int fps = atoi(argv[4]);
  // const char *output = argv[5];

  double width = 1280, height = 720;
  int fps = 30;
  const char *dev_name = "/dev/video0";
  const char *output = "rtmp://192.168.42.25/live/zero"; //"udp://192.168.37.25:12345"; //"rtmp://localhost/live/test";
  const char *encoder_name = "h264_omx";                 //"libx264", "h264_omx";

  try
  {
    stream_video(width, height, fps, dev_name, output, encoder_name);
  }
  catch (exception &e)
  {
    cout << "exception: " << e.what() << "\n";
  }
  return 0;
}

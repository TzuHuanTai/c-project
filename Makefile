CC = g++
CFLAGS = -g -Wall

FFMPEG_LIBS = -lavdevice-58 \
            -lavcodec-58 \
            -lavfilter-7 \
        	-lavformat-58 \
            -lavutil-56 \
            -lswresample-3 \
            -lswscale-5
FFMPEG_INCS = -I "C:\ffmpeg\include"
FFMPEG_LIBS_PATH = -L "C:\ffmpeg\bin"

OPENCV_LINK_LIBS = -llibopencv_calib3d451 \
	-llibopencv_core451 \
	-llibopencv_dnn451 \
	-llibopencv_features2d451 \
	-llibopencv_flann451 \
	-llibopencv_highgui451 \
	-llibopencv_imgcodecs451 \
	-llibopencv_imgproc451 \
	-llibopencv_imgproc451 \
	-llibopencv_ml451 \
	-llibopencv_objdetect451 \
	-llibopencv_photo451 \
	-llibopencv_stitching451 \
	-llibopencv_video451 \
	-llibopencv_videoio451 \
	-llibopencv_xfeatures2d451

OPENCV_INC = -I "D:\lib\opencv-build\install\include"
OPENCV_LIB_PATH =  -L "D:\lib\opencv-build\install\x64\mingw\bin"


all: stereo_match stereo_depth2

streaming_v4l2:
	g++ -g streaming_v4l2.cpp -o streaming_v4l2 -lavformat -lavcodec -lavutil -lswscale -lavdevice -pthread

hello: hello.cpp
	$(CC) hello.cpp -o hello.exe $(OPENCV_INC) $(OPENCV_LIB_PATH) $(OPENCV_LINK_LIBS)

kmeans: kmeans.cpp
	$(CC) kmeans.cpp -o kmeans.exe $(OPENCV_INC) $(OPENCV_LIB_PATH) $(OPENCV_LINK_LIBS)

camera: camera.cpp
    $(CC) $(CFLAGS) "D:\work\projects\c-project\camera.cpp" -o "D:\work\projects\c-project\camera.exe" \
	$(ALL_INCS) $(LIB_PATH) $(OPENCV_LINK_LIBS)

camera_async: camera_async.cpp
    $(CC) $(CFLAGS) "D:\work\projects\c-project\camera_async.cpp" -o "D:\work\projects\c-project\camera_async.exe" \
	$(OPENCV_INC) $(OPENCV_LIB_PATH) $(OPENCV_LINK_LIBS)

encode_video: encode_video.cpp
	$(CC) $(CFLAGS) encode_video.cpp -o encode_video.exe $(FFMPEG_INCS) $(FFMPEG_LIBS_PATH) $(FFMPEG_LIBS)

encode_audio: encode_audio.c
	gcc $(CFLAGS) encode_audio.c -o encode_audio.exe $(FFMPEG_INCS) $(FFMPEG_LIBS_PATH) $(FFMPEG_LIBS)

dshow_list:
	$(CC) $(CFLAGS) dshow_list.cpp -o dshow_list.exe $(FFMPEG_INCS) $(FFMPEG_LIBS_PATH) $(FFMPEG_LIBS)

dshow_capture_video:
	$(CC) $(CFLAGS) dshow_capture_video.cpp -o dshow_capture_video.exe $(FFMPEG_INCS) $(FFMPEG_LIBS_PATH) $(FFMPEG_LIBS)

stereo_match:
	$(CC) $(CFLAGS) "D:\work\projects\c-project\stereo_match.cpp" -o "D:\work\projects\c-project\stereo_match.exe" \
	$(OPENCV_INC) $(OPENCV_LIB_PATH) $(OPENCV_LINK_LIBS)

stereo_depth:
	$(CC) $(CFLAGS) "D:\work\projects\c-project\stereo_depth.cpp" -o "D:\work\projects\c-project\stereo_depth.exe" \
	$(OPENCV_INC) $(OPENCV_LIB_PATH) $(OPENCV_LINK_LIBS)

stereo_depth2:
	$(CC) $(CFLAGS) "D:\work\projects\c-project\stereo_depth2.cpp" -o "D:\work\projects\c-project\stereo_depth2.exe" \
	$(OPENCV_INC) $(OPENCV_LIB_PATH) $(OPENCV_LINK_LIBS)

clean:
	del camera.exe \
	camera_async.exe \
	hello.exe \
	encode_video.exe \
	encode_audio.exe \
	dshow_list.exe \
	dshow_capture_video.exe
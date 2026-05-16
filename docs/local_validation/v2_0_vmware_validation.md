# V2.0 VMware Ubuntu Local Validation

## git
489dd8e (HEAD -> v2.0-video-encode, origin/v1.9-middleware-featurepack, v1.9-middleware-featurepack) v1.9: add middleware feature pack and validation scripts
f5c46ac (origin/v1.8-vm-visual-perf-framelease, v1.8-vm-visual-perf-framelease) v1.8: VM benchmark and FrameLeasePool validation
ea46028 (origin/v1.6-yuyv-fused-preprocess, v1.6-yuyv-fused-preprocess) v1.6-step7: add performance report template for YUYV fused preprocess
347dffc v1.6-step6: add YUYV fused preprocess task document
1cd5309 v1.6-step5: add camera YUYV pipeline benchmark scripts

## final_status
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"

## h264 ffprobe
codec_name=h264
width=640
height=480

## h265 ffprobe
codec_name=hevc
width=640
height=480

## encode benchmark
backend,codec,frames,mean_ms,p95_ms,bitrate_kbps
soft_stub,H264,10,0.0045662,0.0094617,49.056
soft_stub,H265,10,0.0039283,0.0049574,49.056
mpp_stub,H264,10,0.0015338,0.00241525,2.16
v4l2m2m,H264,10,0.0015862,0.0027167,2.16

## camera rgb/yuyv
status=NORMAL fps=30 media_frames=120 preprocess_frames=120 npu_frames=120 error_code=0 text="NORMAL"

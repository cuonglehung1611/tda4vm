# 1. Build
make                            # build defalut app: ./out/cam_capture
make cam_capture                # capture frame from /dev/videoX

# 2. Run
./cam_capture /dev/videoX 10    # capture 10 frame
./cam_capture /dev/videoX 0     # run forever

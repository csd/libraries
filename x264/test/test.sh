#!/bin/sh
#
# Simple script to compile the client to test video_codec with x264 with slice support
# Remember you can change slice's number in x264API/video_codec.c in line 27

echo ""
echo "*******************************************************************************************"
echo "Make clean and make in x264API and x264apiclient"
echo "*******************************************************************************************"
echo ""

cd x264API
make clean ; make
cd ..

cd x264apiClient
make clean ;make
cd ..

echo ""
echo "*******************************************************************************************"
echo "This script doesn't check errors, to launch example \$cd x264apliclient ; ./x264apliclient "
echo "The example takes input.yuv as input (not included) and returns output.264"
echo "*******************************************************************************************"
echo ""

echo ""
echo "*******************************************************************************************"
echo "Testing example"
echo "*******************************************************************************************"
echo ""

cd x264apiClient
./x264client
du -sh output.264
mv output.264 ../

echo ""
echo "*******************************************************************************************"
echo "If all was ok, you will have output.264 generated in this directory"
echo "*******************************************************************************************"
echo ""


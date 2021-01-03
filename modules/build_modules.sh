# Copyright 2020 Dan Williams. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# build rpi_ws281x
cd rpi_ws281x
scons -j 2
cd ..

# build NE10 (assuming for Raspbian - armv7)
cd Ne10
mkdir build
cd build
cmake -DGNULINUX_PLATFORM=ON -DNE10_LINUX_TARGET_ARCH=armv7 ..
make -j 2
cd ../..

# build jsoncpp
cd jsoncpp
mkdir build
cd build
cmake ..
make -j 2
cd ../..

# plotperfectclient
# No build, just add the 2 .c files you the top level build.


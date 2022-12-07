# Copyright 2020, 2022 Dan Williams. All Rights Reserved.
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

env = Environment(CC = 'gcc', CCFLAGS = '-O2 -g -Wall -Werror -fdiagnostics-color=always')

src = [ 'main.cpp',
        'AudioDisplayBase.cpp',
        'AudioDisplayAmplitude.cpp',
        'AudioDisplayFft.cpp',
        'AudioLeds.cpp',
        'alsaMic.cpp',
        'DisplayGradient.cpp',
        'specAnFft.cpp',
        'fftModifier.cpp',
        'fftRunRate.cpp',
        'ledStrip.cpp',
        'colorScale.cpp',
        'colorGradient.cpp',
        'gradientToScale.cpp',
        'GradientUserCues.cpp',
        'hsvrgb.cpp',
        'gradientChangeThread.cpp',
        'RemoteControl.cpp',
        'rotaryEncoder.cpp',
        'SaveRestore.cpp',
        'TCPThreads.c',
        'modules/plotperfectclient/sendMemoryToPlot.c', 
        'modules/plotperfectclient/smartPlotMessage.c' ]

defines = []

inc = [ './modules/plotperfectclient', 
        './modules/Ne10/inc', 
        './modules/rpi_ws281x', 
        './modules/jsoncpp/include' ]

lib = ['rt', 'asound', 'pthread', 'NE10', 'ws2811', 'wiringPi', 'jsoncpp_static']

libpath = ['./modules/Ne10/build/modules', './modules/rpi_ws281x', './modules/jsoncpp/build/lib']

env.Program( source=src,
             CPPDEFINES=defines,
             CPPPATH=inc,
             LIBS=lib,
             LIBPATH=libpath,
             target="SpecAnLedPi" )

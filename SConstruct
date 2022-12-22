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

AdcHatAttached = True # Specifies whether the ADC hat card is attached or not.
Ne10Compatible = True # NE10 is used to do FFTs. NE10 isn't comptible with ARM6 (Pi W Zeros).

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
        'seeed_adc_8chan_12bit.cpp',
        'TCPThreads.c',
        'modules/plotperfectclient/sendMemoryToPlot.cpp', 
        'modules/plotperfectclient/smartPlotMessage.cpp' ]

defines = []

inc = [ './modules/plotperfectclient', 
        './modules/Ne10/inc', 
        './modules/rpi_ws281x', 
        './modules/jsoncpp/include', 
        './modules/WiringPi/wiringPi' ]

lib = ['rt', 'asound', 'pthread', 'NE10', 'ws2811', 'wiringPi', 'jsoncpp_static']

libpath = ['./modules/Ne10/build/modules', './modules/rpi_ws281x', './modules/jsoncpp/build/lib', './modules/WiringPi/wiringPi']

# Update build based on some global settings.
if not AdcHatAttached:
   defines.append('NO_ADCS')

if not Ne10Compatible:
   defines.append('NO_FFTS')


env.Program( source=src,
             CPPDEFINES=defines,
             CPPPATH=inc,
             LIBS=lib,
             LIBPATH=libpath,
             target="SpecAnLedPi" )

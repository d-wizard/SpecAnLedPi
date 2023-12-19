# Copyright 2020, 2022 - 2023 Dan Williams. All Rights Reserved.
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

AdcHatAttached = False # Specifies whether the ADC hat card is attached or not.
Ne10Compatible = False # NE10 is used to do FFTs. NE10 isn't comptible with ARM6 (Pi W Zeros).

# Cross Compile Parameters. 
crossCompilePrefix = None # Example: '/path/to/bin/armv6-rpi-linux-gnueabihf-'
preCompiledPortableLibDirectory = None # If specified the 'Portable Code Library' won't be built.


################################################################################

env = Environment(CC = 'gcc', CCFLAGS = '-O2 -g -Wall -Werror -fdiagnostics-color=always')

################################################################################
if crossCompilePrefix != None:
   env.Replace(CC=crossCompilePrefix+'gcc')
   env.Replace(CXX=crossCompilePrefix+'g++')
   env.Replace(AR=crossCompilePrefix+'ar')
   env.Replace(RANDLIB=crossCompilePrefix+'randlib')

################################################################################
# Common 
################################################################################
defines = ['']
# defines += ['PLOTTER_FORCE_BACKGROUND_THREAD']

# Update build based on some global settings.
if not AdcHatAttached:
   defines.append('NO_ADCS')

if not Ne10Compatible:
   defines.append('NO_FFTS')

inc = [ './modules/plotperfectclient', 
        './modules/Ne10/inc', 
        './modules/rpi_ws281x', 
        './modules/jsoncpp/include', 
        './modules/WiringPi/wiringPi' ]


################################################################################
# Portable Code Library Build
################################################################################
if preCompiledPortableLibDirectory == None: # Don't compile if a directory where the pre-compiled library exists is specified.
   src = [ 'SpecAnLed.cpp',
           'AudioDisplayBase.cpp',
           'AudioDisplayAmplitude.cpp',
           'AudioDisplayFft.cpp',
           'AudioLeds.cpp',
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

   libPortableCode = env.StaticLibrary(
      CPPDEFINES=defines,
      CPPPATH=inc,
      target='SpecAnLedPiLib',
      source=src
   )

################################################################################
# The SpecAnLedPi Binary
################################################################################
if crossCompilePrefix == None: # If not cross compiling, build the final binary
   srcNonPortable = [
      'alsaMic.cpp',
      'SpecAnLedMain.cpp'
      ]

   lib = ['SpecAnLedPiLib', 'rt', 'asound', 'pthread', 'NE10', 'ws2811', 'wiringPi', 'jsoncpp_static']

   libpath = ['.', './modules/Ne10/build/modules', './modules/rpi_ws281x', './modules/jsoncpp/build/lib', './modules/WiringPi/wiringPi']

   if preCompiledPortableLibDirectory != None:
      libpath += [preCompiledPortableLibDirectory]

   env.Program( source=srcNonPortable,
               CPPDEFINES=defines,
               CPPPATH=inc,
               LIBS=lib,
               LIBPATH=libpath,
               target="SpecAnLedPi" )

################################################################################
# The AmbientDisplay Binary
################################################################################
if crossCompilePrefix == None: # If not cross compiling, build the final binary
   srcNonPortable = [
      'ambient/AmbientDisplayMain.cpp',
      'ambient/AmbientDisplay.cpp',
      'ambient/displays/AmbDisp3SpotLights.cpp'
      ]

   extraIncludes = [
      '.',
      'ambient',
      'ambient/displays'
   ]

   lib = ['SpecAnLedPiLib', 'rt', 'asound', 'pthread', 'NE10', 'ws2811', 'wiringPi', 'jsoncpp_static']

   libpath = ['.', './modules/Ne10/build/modules', './modules/rpi_ws281x', './modules/jsoncpp/build/lib', './modules/WiringPi/wiringPi']

   if preCompiledPortableLibDirectory != None:
      libpath += [preCompiledPortableLibDirectory]

   env.Program( source=srcNonPortable,
               CPPDEFINES=defines,
               CPPPATH=inc+extraIncludes,
               LIBS=lib,
               LIBPATH=libpath,
               target="AmbientDisplay" )

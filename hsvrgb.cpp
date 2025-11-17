/* Copyright 2020, 2025 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include "hsvrgb.h"

/**
 * Calculates a value along a quarter circle curve.
 * Bounds the input value between 0.0 and 1.0.
 * 
 * @param val The input value (0.0 to 1.0).
 * @return The value mapped along the quarter circle curve.
 */
static double quarterCircle(double val) {
    // Bound the value
    val = std::max(0.0, std::min(1.0, val));
    
    // The equation for a quarter circle: y=(1 - (1-x)^2)^0.5
    // http://www.wolframalpha.com/input/?i=y%3D%281+-+%281-x%29%5E2%29%5E0.5+from+x%3D0+to+1
    return std::sqrt(1.0 - std::pow(1.0 - val, 2));
}

/**
 * Adjusts a hue value (assumed to be between 0.0 and 1.0) using a more perceptually
 * uniform scaling based on a quarter circle function and power curve.
 *
 * @param hue The input hue value (0.0 to 1.0).
 * @return The adjusted hue value.
 */
static double betterHue(double hue) {
    const double oneSixth = 1.0 / 6.0;

    // Determine which sixth of the hue we are in.
    int whichSixth = static_cast<int>(hue / oneSixth);
    double remainderInSixth = hue - static_cast<double>(whichSixth) * oneSixth;
    // Direction is 0 for even sixths, 1 for odd sixths
    int direction = whichSixth & 1; 

    // Setup for scaling
    double scaledRemainder = remainderInSixth * 6.0; // Convert from 0 to 1/6th to 0 to 1
    double scaledPower = 2.0; // 2 = square the value, 3 = cube the value, 0.5 = square root, etc

    // Do the math to determine the scaling.
    if (direction) { scaledRemainder = 1.0 - scaledRemainder; } // Mirror on odd sixths
    scaledRemainder = quarterCircle(scaledRemainder);
    scaledRemainder = std::pow(scaledRemainder, scaledPower); // Use power to better scale
    if (direction) { scaledRemainder = 1.0 - scaledRemainder; } // Mirror back on odd sixths

    scaledRemainder *= oneSixth; // Scale back down to 1/6th
    
    // Return the final adjusted hue
    return static_cast<double>(whichSixth) * oneSixth + scaledRemainder;
}

RgbColor HsvToRgb(HsvColor hsv, bool applyBetterHue)
{
    RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    if(applyBetterHue)
    {
        hsv.h = (unsigned char)(betterHue(double(hsv.h)/256.0) * 256.0); // Convert from 0 to 255 and back when using the 'betterHue' function.
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6; 

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
}

HsvColor RgbToHsv(RgbColor rgb)
{
    HsvColor hsv;
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = rgbMax;
    if (hsv.v == 0)
    {
        hsv.h = 0;
        hsv.s = 0;
        return hsv;
    }

    hsv.s = 255 * (long)(rgbMax - rgbMin) / hsv.v;
    if (hsv.s == 0)
    {
        hsv.h = 0;
        return hsv;
    }

    if (rgbMax == rgb.r)
        hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if (rgbMax == rgb.g)
        hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

    return hsv;
}


long RgbToLong(RgbColor rgb)
{
   return ( (long)rgb.r << 16 | (long)rgb.g << 8 | (long)rgb.b);
}


RgbColor LongToRgb(long rgb)
{
   RgbColor retVal;
   retVal.r = (rgb >> 16) & 0xFF;
   retVal.g = (rgb >>  8) & 0xFF;
   retVal.b = (rgb >>  0) & 0xFF;
   return retVal;
}

/* Copyright 2020 Dan Williams. All Rights Reserved.
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
#pragma once

#include <vector>
#include <string>
#include "colorGradient.h"

namespace SaveRestore
{

class Gradient
{
public:
   Gradient();

   void save(ColorGradient::tGradient& gradToSave);

   ColorGradient::tGradient restore();
   ColorGradient::tGradient restoreNext();
   ColorGradient::tGradient restorePrev();

   ColorGradient::tGradient deleteCurrent();

private:
   const std::string LATEST_NAME = "latest";
   const std::string USER_SAVE_PREFIX = "colors";
   std::string m_saveRestoreDir;
   std::string m_latestFileSavePath;

   std::vector<std::string> getAllFiles();
   static void splitNumFromName(std::string& fileName, std::string& namePart, int& numPart);
   static bool sortPathFunc(std::string path0, std::string path1);

   ColorGradient::tGradient restore(int index);

   bool match(ColorGradient::tGradient& comp1, ColorGradient::tGradient& comp2);

   std::string getLatestPath();
   void setLatestPath(std::string latest);

   int indexFromName(std::vector<std::string> filePaths, std::string fileName);

   ColorGradient::tGradient restore(int index, std::vector<std::string> filePaths);

   ColorGradient::tGradient read(std::string filePath);
   void write(std::string filePath, ColorGradient::tGradient toWrite);
   
};

}

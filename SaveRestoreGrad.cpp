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
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <filesystem>
#include "SaveRestoreGrad.h"

SaveRestoreGrad::SaveRestoreGrad()
{
   m_saveRestoreDir = std::filesystem::current_path().native() + "/.specanledpi";
   m_latestFileSavePath = m_saveRestoreDir + "/" + LATEST_NAME;
   if(!std::filesystem::exists(m_saveRestoreDir))
   {
      std::filesystem::create_directory(m_saveRestoreDir);
   }
}


std::vector<std::string> SaveRestoreGrad::getAllFiles()
{
   std::vector<std::string> retVal;
   for(const auto& entry : std::filesystem::directory_iterator(m_saveRestoreDir))
   {
      if(!std::filesystem::is_directory(entry.path()))
      {
         if(entry.path().filename() != LATEST_NAME)
         {
            retVal.push_back(entry.path().native());
         }
      }
   }
   return retVal;
}


void SaveRestoreGrad::save(std::vector<ColorGradient::tGradientPoint>& gradToSave)
{
   int saveFileNum = 0;
   bool saved = false;

   do
   {
      std::stringstream saveFileName;
      saveFileName << m_saveRestoreDir << "/" << "colors" << saveFileNum;
      if(!std::filesystem::exists(saveFileName.str()))
      {
         write(saveFileName.str(), gradToSave);
         saved = true;
      }
      else
      {
         // Check for match.
         auto readGrad = read(saveFileName.str());
         if(match(readGrad, gradToSave))
         {
            saved = true; // Already matchs a saved file. Exit loop
         }
         saveFileNum++;
      }

      if(saved)
      {
         setLatestPath(saveFileName.str());
      }
   } while(!saved);

}

std::vector<ColorGradient::tGradientPoint> SaveRestoreGrad::restore(int index)
{
   return restore(index, getAllFiles());
}

std::vector<ColorGradient::tGradientPoint> SaveRestoreGrad::restore()
{
   auto allFiles = getAllFiles();
   auto latest = getLatestPath();
   return restore(indexFromName(allFiles, latest), allFiles);
}

std::vector<ColorGradient::tGradientPoint> SaveRestoreGrad::restoreNext()
{
   auto allFiles = getAllFiles();
   auto latest = getLatestPath();
   return restore(indexFromName(allFiles, latest)+1, allFiles);
}

std::vector<ColorGradient::tGradientPoint> SaveRestoreGrad::restorePrev()
{
   auto allFiles = getAllFiles();
   auto latest = getLatestPath();
   return restore(indexFromName(allFiles, latest)-1, allFiles);
}

bool SaveRestoreGrad::match(std::vector<ColorGradient::tGradientPoint>& comp1, std::vector<ColorGradient::tGradientPoint>& comp2)
{
   bool retVal = false;
   if(comp1.size() == comp2.size())
   {
      retVal = true;
      for(size_t i = 0; i < comp1.size(); ++i)
      {
         if(comp1[i] != comp2[i])
         {
            retVal = false;
            break;
         }
      }
   }
   return retVal;
}

std::string SaveRestoreGrad::getLatestPath()
{
   std::string line = "";
   if(std::filesystem::exists(m_latestFileSavePath))
   {
      std::ifstream readFile;
      readFile.open(m_latestFileSavePath);
      std::getline(readFile, line);
      readFile.close();
   }
   return line;
}

void SaveRestoreGrad::setLatestPath(std::string latest)
{
   std::ofstream writeFile;
   writeFile.open(m_latestFileSavePath);
   writeFile << latest;
   writeFile.close();
}

int SaveRestoreGrad::indexFromName(std::vector<std::string> filePaths, std::string fileName)
{
   int matchingIndex = -1;
   for(size_t i = 0; i < filePaths.size(); ++i)
   {
      if(filePaths[i] == fileName)
      {
         matchingIndex = i;
         return matchingIndex;
      }
   }
   return matchingIndex;
}

std::vector<ColorGradient::tGradientPoint> SaveRestoreGrad::restore(int index, std::vector<std::string> filePaths)
{
   std::vector<ColorGradient::tGradientPoint> retVal;
   int numPaths = filePaths.size();
   if(numPaths > 0)
   {
      index = index % numPaths;
      if(index < 0)
         index += numPaths;
      assert(index >= 0 && index < numPaths);

      if(index >= 0 && index < numPaths)
      {
         setLatestPath(filePaths[index]);
         retVal = read(filePaths[index]);
      }
   }
   return retVal;
}

std::vector<ColorGradient::tGradientPoint> SaveRestoreGrad::read(std::string filePath)
{
   std::vector<ColorGradient::tGradientPoint> retVal;
   ColorGradient::tGradientPoint newPoint;

   std::ifstream readFile;
   std::string line;
   readFile.open(filePath.c_str());
   int numIndex = 0;
   while(std::getline(readFile, line))
   {
      std::istringstream iss(line);
      float numVal;
      if (!(iss >> numVal)) { break; } // error

      switch(numIndex)
      {
         case 0:
            newPoint.hue = numVal;
         break;
         case 1:
            newPoint.saturation = numVal;
         break;
         case 2:
            newPoint.lightness = numVal;
         break;
         case 3:
            newPoint.reach = numVal;
         break;
         case 4:
            newPoint.position = numVal;
         break;
      }
      numIndex++;
      if(numIndex >= 5)
      {
         retVal.push_back(newPoint);
         numIndex = 0;
      }

   }

   readFile.close();
   
   return retVal;
}

void SaveRestoreGrad::write(std::string filePath, std::vector<ColorGradient::tGradientPoint> toWrite)
{
   std::stringstream ss;
   for(const auto& point : toWrite)
   {
      ss << point.hue        << std::endl;
      ss << point.saturation << std::endl;
      ss << point.lightness  << std::endl;
      ss << point.reach      << std::endl;
      ss << point.position   << std::endl;
   }
   std::ofstream writeFile;
   writeFile.open(filePath.c_str());
   writeFile << ss.str();
   writeFile.close();
}

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
#include <algorithm>
#include "SaveRestore.h"


static ColorGradient::tGradient jsonToGrad(Json::Value& jsonIn)
{
   ColorGradient::tGradient retVal;
   ColorGradient::tGradientPoint point;

   for(auto pointItr = jsonIn.begin(); pointItr != jsonIn.end(); ++pointItr)
   {
      point.hue = (*pointItr)["hue"].asFloat();
      point.saturation = (*pointItr)["saturation"].asFloat();
      point.lightness = (*pointItr)["lightness"].asFloat();
      point.position = (*pointItr)["position"].asFloat();
      point.reach = (*pointItr)["reach"].asFloat();
      retVal.push_back(point);
   }

   return retVal;
}

static std::vector<ColorGradient::tGradient> jsonToGradVect(Json::Value& jsonIn)
{
   std::vector<ColorGradient::tGradient> retVal;

   for(auto gradItr = jsonIn.begin(); gradItr != jsonIn.end(); ++gradItr)
   {
      retVal.push_back(jsonToGrad(*gradItr));
   }

   return retVal;
}


static Json::Value gradToJson(ColorGradient::tGradient& gradIn)
{
   Json::Value retVal;
   for(size_t i = 0; i < gradIn.size(); ++i)
   {
      Json::Value point;
      point["hue"] = gradIn[i].hue;
      point["saturation"] = gradIn[i].saturation;
      point["lightness"] = gradIn[i].lightness;
      point["position"] = gradIn[i].position;
      point["reach"] = gradIn[i].reach;

      retVal[std::to_string(i)] = point;
   }
   return retVal;
}

static Json::Value gradVectToJson(std::vector<ColorGradient::tGradient>& gradsIn)
{
   Json::Value retVal;
   for(size_t i = 0; i < gradsIn.size(); ++i)
   {
      retVal[std::to_string(i)] = gradToJson(gradsIn[i]);
   }
   return retVal;
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

SaveRestoreJson::SaveRestoreJson()
{
}

void SaveRestoreJson::save_gradient(ColorGradient::tGradient& gradToSave)
{
   std::unique_lock<std::mutex> lock(m_mutex); // Lock around all public functions (they will never call each other).

   int currentIndex;
   auto existingGrads = getAllGradients(currentIndex);

   // Check if this gradient already exists (i.e. we don't want duplicates).
   bool gradExists = false;
   for(size_t i = 0; i < existingGrads.size(); ++i)
   {
      if(match(existingGrads[i], gradToSave))
      {
         gradExists = true;
         currentIndex = i;
         break;
      }
   }

   // Read the settings json file.
   Json::Value settingsJson;
   getJson(SETTINGS_JSON, settingsJson);

   if(!gradExists)
   {
      // Add the new gradient to the end.
      auto userGrads = getUserGradients(settingsJson); // Just the user specified gradients.
      userGrads.push_back(gradToSave);

      // Update settings json with the new gradient.
      settingsJson["user"] = gradVectToJson(userGrads);
      settingsJson["grad_index"] = existingGrads.size(); // New one is being added to the end, so old size is the correct index.
   }
   else
   {
      settingsJson["grad_index"] = currentIndex; // Make sure the index matches the gradient that matches the one passed in.
   }

   saveSettings(settingsJson); // Save off the changes.
}

bool SaveRestoreJson::match(ColorGradient::tGradient& comp1, ColorGradient::tGradient& comp2)
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

ColorGradient::tGradient SaveRestoreJson::restore_gradient()
{
   return restoreGradient(0);
}

ColorGradient::tGradient SaveRestoreJson::restore_gradientNext()
{
   return restoreGradient(1);
}

ColorGradient::tGradient SaveRestoreJson::restore_gradientPrev()
{
   return restoreGradient(-1);
}

ColorGradient::tGradient SaveRestoreJson::restoreGradient(int indexDelta)
{
   std::unique_lock<std::mutex> lock(m_mutex); // Lock around all public functions (they will never call each other).

   ColorGradient::tGradient retVal;
   int currentIndex = 0;
   auto existingGrads = getAllGradients(currentIndex);
   int existingGradsSize = existingGrads.size();

   if(existingGradsSize > 0)
   {
      // Update currentIndex and bound.
      currentIndex += indexDelta;
      if(currentIndex >= existingGradsSize)
         currentIndex = 0;
      else if(currentIndex < 0)
         currentIndex = existingGradsSize-1;

      // Return the gradient.
      retVal = existingGrads[currentIndex];

      // Save the index to this gradient.
      Json::Value settingsJson;
      getJson(SETTINGS_JSON, settingsJson);
      saveGradientIndex(settingsJson, currentIndex);
   }
   return retVal;
}

ColorGradient::tGradient SaveRestoreJson::delete_gradient()
{
   std::unique_lock<std::mutex> lock(m_mutex); // Lock around all public functions (they will never call each other).

   int currentIndex = 0;
   int numPresetGrads = 0;
   auto existingGrads = getAllGradients(currentIndex, numPresetGrads);
   int numExistings = existingGrads.size();

   // Will be modifing the settings Json. Read it out here.
   Json::Value settingsJson;
   getJson(SETTINGS_JSON, settingsJson);

   int toDeleteIndex = currentIndex - numPresetGrads; // Convert to index in the user specified gradients.
   if(toDeleteIndex >= 0)
   {
      auto userGrads = getUserGradients(settingsJson); // Just the user specified gradients.

      if(int(userGrads.size()) > toDeleteIndex)
      {
         userGrads.erase(userGrads.begin() + toDeleteIndex);
      }

      // Update to account for removed user gradient.
      numExistings--;
      
      settingsJson["user"] = gradVectToJson(userGrads);
   }

   // Save off the new index and the updated user gradients (if one was actually removed).
   currentIndex--;
   if(currentIndex < 0)
      currentIndex = numExistings-1;
   saveGradientIndex(settingsJson, currentIndex); // Will save the index and the updated user gradients.

   // Return the previous gradient.
   ColorGradient::tGradient retVal;
   if(numExistings > 0 && currentIndex >= 0 && currentIndex < numExistings)
      retVal = existingGrads[currentIndex];
   return retVal;
}

void SaveRestoreJson::saveGradientIndex(Json::Value& settingsJson, int index)
{
   settingsJson["grad_index"] = index;
   saveSettings(settingsJson);
}

void SaveRestoreJson::saveSettings(Json::Value& settingsJson)
{
   std::ofstream writeFile;
   writeFile.open(SETTINGS_JSON);
   writeFile << settingsJson << std::endl;
   writeFile.close();
}


void SaveRestoreJson::getJson(std::string pathToJson, Json::Value& jsonRetVal)
{
   std::ifstream inFile(pathToJson, std::ifstream::binary);
   Json::Value presetJson;
   try
   {
      inFile >> jsonRetVal;
   }
   catch(const std::exception& e)
   {
      //std::cerr << e.what() << '\n';
   }
   
}

std::vector<ColorGradient::tGradient> SaveRestoreJson::getAllGradients(int& currentIndex)
{
   int numPresetGrads = 0; // dummy
   return getAllGradients(currentIndex, numPresetGrads);
}

std::vector<ColorGradient::tGradient> SaveRestoreJson::getAllGradients(int& currentIndex, int& numPresetGrads)
{
   Json::Value presetJson;
   getJson(PRESET_GRADIENT_JSON, presetJson);
   auto presetGrads = jsonToGradVect(presetJson);
   numPresetGrads = presetGrads.size();

   Json::Value settingsJson;
   getJson(SETTINGS_JSON, settingsJson);
   auto userGrads = getUserGradients(settingsJson);

   currentIndex = settingsJson["grad_index"].asInt();

   std::vector<ColorGradient::tGradient> allGrads;
   allGrads.insert(allGrads.end(), presetGrads.begin(), presetGrads.end());
   allGrads.insert(allGrads.end(), userGrads.begin(), userGrads.end());

   return allGrads;
}


std::vector<ColorGradient::tGradient> SaveRestoreJson::getUserGradients(Json::Value& settingsJson)
{
   return jsonToGradVect(settingsJson["user"]);;
}

void SaveRestoreJson::save_displayIndex(int index)
{
   std::unique_lock<std::mutex> lock(m_mutex); // Lock around all public functions (they will never call each other).

   Json::Value settingsJson;
   getJson(SETTINGS_JSON, settingsJson);
   settingsJson["display_index"] = index;
   saveSettings(settingsJson);
}

int SaveRestoreJson::restore_displayIndex()
{
   std::unique_lock<std::mutex> lock(m_mutex); // Lock around all public functions (they will never call each other).

   Json::Value settingsJson;
   getJson(SETTINGS_JSON, settingsJson);

   int retVal = settingsJson["display_index"].asInt();
   return retVal;
}


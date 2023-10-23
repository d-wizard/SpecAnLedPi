/* Copyright 2020, 2022 - 2023 Dan Williams. All Rights Reserved.
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
#include <mutex>
#include "colorGradient.h"
#include "json/json.h"

class SaveRestoreJson
{
public:
   typedef enum
   {
      E_DEFAULT,
      E_LOCAL,
      E_REMOTE
   }eRemoteLocalOptions;

   SaveRestoreJson(std::string settingsJsonPath = "settings.json", std::string presetGradJsonPath = "presets.json");

   unsigned restore_numLeds();
   bool restore_mirrorLedMode();

   void save_gradient(ColorGradient::tGradient& gradToSave);

   ColorGradient::tGradient restore_gradient();
   ColorGradient::tGradient restore_gradientNext();
   ColorGradient::tGradient restore_gradientPrev();

   ColorGradient::tGradient delete_gradient();

   void save_displayIndex(int index);
   int restore_displayIndex();

   void save_gradientReverse(bool reversed);
   bool restore_gradientReverse();

   void save_remoteLocal(eRemoteLocalOptions remoteLocal);
   eRemoteLocalOptions restore_remoteLocal();

   void save_gain(float gain);
   float restore_gain();
   void save_brightness(float brightness);
   float restore_brightness();

   std::string restore_microphoneName();

private:
   // Make uncopyable
   SaveRestoreJson(SaveRestoreJson const&);
   void operator=(SaveRestoreJson const&);

   const std::string SETTINGS_JSON = "settings.json";
   const std::string PRESET_GRADIENT_JSON = "presets.json";

   std::mutex m_mutex;

   float m_lastGainVal = -1;   // initialize to invalid value.
   float m_lastBrightVal = -1; // initialize to invalid value.

   void getJson(std::string pathToJson, Json::Value& jsonRetVal);

   std::vector<ColorGradient::tGradient> getAllGradients(int& currentIndex);
   std::vector<ColorGradient::tGradient> getAllGradients(int& currentIndex, int& numPresetGrads);
   std::vector<ColorGradient::tGradient> getUserGradients(Json::Value& settingsJson);

   ColorGradient::tGradient restoreGradient(int indexDelta);

   bool match(ColorGradient::tGradient& comp1, ColorGradient::tGradient& comp2);

   void saveGradientIndex(Json::Value& settingsJson, int index);
   void saveSettings(Json::Value& settingsJson);
};

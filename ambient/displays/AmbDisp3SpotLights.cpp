/* Copyright 2023 Dan Williams. All Rights Reserved.
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
#include "AmbDisp3SpotLights.h"
#include "WaveformGen.h"

// Brightness Constants.
#define BRIGHTNESS_PATTERN_NUM_POINTS (51)
#define BRIGHTNESS_PATTERN_HI_LEVEL (.35)
#define BRIGHTNESS_PATTERN_LO_LEVEL (0.0)

#define GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO (3.0)

#define NUM_SPOT_LIGHTS (3)

AmbDisp3SpotLights::AmbDisp3SpotLights(std::shared_ptr<LedStrip> ledStrip):
   AmbientLedStripBase(ledStrip)
{
   init();
   startThread();
}

AmbDisp3SpotLights::AmbDisp3SpotLights(std::shared_ptr<LedStrip> ledStrip, const ColorGradient::tGradient& gradient, unsigned numGradDuplicates):
   AmbientLedStripBase(ledStrip, gradient, numGradDuplicates)
{
   init();
   startThread();
}

AmbDisp3SpotLights::~AmbDisp3SpotLights()
{
   stopThread();
}

void AmbDisp3SpotLights::init()
{
   /////////////////////////////////////////////////////////////////////////////
   // Define Brightness Scale
   /////////////////////////////////////////////////////////////////////////////
   WaveformGen<AmbDispFltType> brightValGen(BRIGHTNESS_PATTERN_NUM_POINTS);
   brightValGen.Sinc(-100, 100);
   brightValGen.absoluteValue();
   brightValGen.scale(BRIGHTNESS_PATTERN_HI_LEVEL - BRIGHTNESS_PATTERN_LO_LEVEL);
   brightValGen.shift(BRIGHTNESS_PATTERN_LO_LEVEL);

   WaveformGen<AmbDispFltType> brightPosGen(BRIGHTNESS_PATTERN_NUM_POINTS);
   brightPosGen.Linear(0, 1);

   ColorScale::tBrightnessScale spotLightBrightness(BRIGHTNESS_PATTERN_NUM_POINTS);
   for(int i = 0; i < BRIGHTNESS_PATTERN_NUM_POINTS; ++i)
   {
      spotLightBrightness[i].brightness = brightValGen.getPoints()[i];
      spotLightBrightness[i].startPoint = brightPosGen.getPoints()[i];
      // smartPlot_2D(&spotLightBrightness[i].startPoint, E_FLOAT_32, &spotLightBrightness[i].brightness, E_FLOAT_32, 1, 100, -1, "2D", "val");
   }

   /////////////////////////////////////////////////////////////////////////////
   // Setup the Display
   /////////////////////////////////////////////////////////////////////////////
   auto gradPoints = m_gradient;
   unsigned numDuplicates = (m_numGradDuplicates > 0) ? m_numGradDuplicates : 2; // If the number of gradient duplicates is specified, use that value. Otherwise use the default value.
   ColorGradient::DuplicateGradient(gradPoints, numDuplicates, true);
   ColorScale::DuplicateBrightness(spotLightBrightness, int(GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO), false);

   std::vector<ColorScale::tBrightnessScale> spotLights;
   for(int i = 0; i < NUM_SPOT_LIGHTS; ++i)
      spotLights.push_back(spotLightBrightness);
   m_ambDisp = std::make_unique<AmbientDisplay>(gradPoints, spotLights);

   /////////////////////////////////////////////////////////////////////////////
   // Setup the Brightness Movement
   /////////////////////////////////////////////////////////////////////////////
   m_movementSources.push_back(std::make_shared<AmbientMovement::LinearSource<AmbDispFltType>>(0.001, 0.1));
   m_movementSources.push_back(std::make_shared<AmbientMovement::LinearSource<AmbDispFltType>>(0.0008381984, 0.4));
   m_movementSources.push_back(std::make_shared<AmbientMovement::LinearSource<AmbDispFltType>>(0.0003984116, 0.7));
   auto brightTransforms_saw    = std::make_shared<AmbientMovement::SawTransform<AmbDispFltType>>();
   auto brightTransforms_scale  = std::make_shared<AmbientMovement::LinearTransform<AmbDispFltType>>(0.4/GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO);
   std::vector<AmbientMovement::TransformPtr<AmbDispFltType>> brightTransformsSawScale = {brightTransforms_saw, brightTransforms_scale};
   std::vector<AmbientMovement::TransformPtr<AmbDispFltType>> brightTransformsLinScale = {brightTransforms_scale};

   m_movementGenerators.emplace_back(std::make_unique<AmbMoveGen>(m_movementSources[0], brightTransformsSawScale));
   m_movementGenerators.emplace_back(std::make_unique<AmbMoveGen>(m_movementSources[1], brightTransformsLinScale));
   m_movementGenerators.emplace_back(std::make_unique<AmbMoveGen>(m_movementSources[2], brightTransformsLinScale));

   /////////////////////////////////////////////////////////////////////////////
   // Add some randomness to the brightness movement speed
   /////////////////////////////////////////////////////////////////////////////
   std::vector<AmbientMovement::TransformPtr<AmbDispFltType>> brightMoveTransforms;
   brightMoveTransforms.emplace_back(std::make_shared<AmbientMovement::RandNegateTransform<AmbDispFltType>>()); // Randomly change the direction of the movement.
   brightMoveTransforms.emplace_back(std::make_shared<AmbientMovement::BlockFastSignChanges<AmbDispFltType>>(5.0)); // Keep direction of movement from change too quickly.
   m_brightMoveSpeedModGen = std::make_unique<AmbMoveGen>(
      std::make_shared<AmbientMovement::RandUniformSource<AmbDispFltType>>(0.5, 1.25), // Generator
      brightMoveTransforms); // Transforms.
   m_brightMoveSpeedModRandNumGen = std::make_unique<AmbMoveGen>(
      std::make_shared<AmbientMovement::RandUniformSource<AmbDispFltType>>(0.0, 1.0));
}

void AmbDisp3SpotLights::updateLedStrip()
{
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
   m_ambDisp->toRgbVect(GRADIENT_TO_BRIGHTNESS_PATTERN_RATIO*m_numLeds, m_ledColorPattern, m_numLeds);
   m_ledStrip->set(m_ledColorPattern);
   m_ambDisp->gradient_shift(-0.002);
   for(size_t i = 0; i < m_movementGenerators.size(); ++i)
   {
      m_ambDisp->brightness_shift(m_movementGenerators[i]->getNextDelta(), i);
   }

   for(size_t i = 0; i < m_movementSources.size(); ++i)
   {
      if(m_brightMoveSpeedModRandNumGen->getNext() < 0.02) // 2% chance of hitting
      {
         m_movementSources[i]->scaleIncr(m_brightMoveSpeedModGen->getNext());
      }
   }
}
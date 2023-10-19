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
#pragma once
#include <memory>
#include <vector>
#include "AmbientLedStripBase.h"
#include "AmbientDisplay.h"
#include "AmbientMovement.h"

class AmbDisp3SpotLights : public AmbientLedStripBase
{
public:
   AmbDisp3SpotLights(std::shared_ptr<LedStrip> ledStrip);
   AmbDisp3SpotLights(std::shared_ptr<LedStrip> ledStrip, const ColorGradient::tGradient& gradient);
   virtual ~AmbDisp3SpotLights();

private:
   virtual void updateLedStrip() override;
   SpecAnLedTypes::tRgbVector m_ledColorPattern;

   typedef AmbientMovement::Generator<AmbDispFltType> AmbMoveGen;
   typedef std::unique_ptr<AmbMoveGen> AmbMoveGenPtr;

   std::unique_ptr<AmbientDisplay> m_ambDisp;
   std::vector<std::shared_ptr<AmbientMovement::LinearSource<AmbDispFltType>>> m_movementSources;
   std::vector<AmbMoveGenPtr> m_movementGenerators;

   AmbMoveGenPtr m_brightMoveSpeedModGen;

   int m_brightMoveSpeedModGenCount = 0;

   void init();
};

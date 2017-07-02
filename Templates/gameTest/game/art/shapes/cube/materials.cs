//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

//--- cube.dae MATERIALS BEGIN ---
singleton Material(GridMaterial)
{
   diffuseMap = "core/art/white";
	diffuseColor[0] = "1 1 1 1";
	specular[0] = "0.9 0.9 0.9 1";
	specularPower[0] = 0.415939;
	pixelSpecular[0] = false;
	emissive[0] = false;

	doubleSided = false;
	translucent = false;
	translucentBlendOp = "None";
   materialTag0 = "Miscellaneous";
   smoothness[0] = "1";
   metalness[0] = "1";
   specularPower0 = "0.415939";
   pixelSpecular0 = "0";
   specular0 = "0.9 0.9 0.9 1";
   
   glow = false;
   mapTo = "unmapped_mat";
};

//--- cube.dae MATERIALS END ---


singleton Material(SimpleConeMat)
{
   mapTo = "GridMaterial";
   diffuseMap[0] = "core/art/shapes/blue";
   emissive[0] = "1";
   castShadows = "0";
};
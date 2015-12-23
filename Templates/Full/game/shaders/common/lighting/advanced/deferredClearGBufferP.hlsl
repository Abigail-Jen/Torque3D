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

struct Fragout
{
   float4 col : COLOR0;
   float4 col1 : COLOR1;
   float4 col2 : COLOR2;
   float4 col3 : COLOR3;
};

//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
Fragout main( )
{
   Fragout OUT;
   
   // Clear Prepass Buffer ( Normals/Depth );
   OUT.col =  float4(1.0, 1.0, 1.0, 1.0);

   // Clear Color Buffer.
   OUT.col1 = float4(0.0, 0.0, 0.0, 0.0001);

   // Clear Material Info Buffer.
   OUT.col2 = float4(0.0, 0.0, 0.0, 0.0);
   
   // Clear Light Info Buffer.
   OUT.col3 = float4(0.0, 0.0, 0.0, 0.0);

   return OUT;
}

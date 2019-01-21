#include "../../shaderModelAutoGen.hlsl"

#include "farFrustumQuad.hlsl"
#include "../../lighting.hlsl"
#include "../../torque.hlsl"

/*struct ConvexConnectP
{
   float4 pos : TORQUE_POSITION;
   float4 uv0 : TEXCOORD1;
   float4 vsEyeDir : TEXCOORD2;
};*/

struct ConvexConnectP
{
   float4 pos : TORQUE_POSITION;
   float4 wsEyeDir : TEXCOORD0;
   float4 ssPos : TEXCOORD1;
   float4 vsEyeDir : TEXCOORD2;
};

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 2);
TORQUE_UNIFORM_SAMPLER2D(BRDFTexture, 5);

uniform float4 rtParams0;
uniform float4 vsFarPlane;
uniform float4x4 cameraToWorld;
uniform float3 eyePosWorld;

//cubemap arrays require all the same size. so shared mips# value
uniform float cubeMips;
#define MAX_PROBES 50

uniform float numProbes;
TORQUE_UNIFORM_SAMPLERCUBEARRAY(cubeMap, 3);
TORQUE_UNIFORM_SAMPLERCUBEARRAY(irradianceCubemap, 4);
uniform float3    inProbePosArray[MAX_PROBES];
uniform float4x4  worldToObjArray[MAX_PROBES];
uniform float3    bbMinArray[MAX_PROBES];
uniform float3    bbMaxArray[MAX_PROBES];
uniform float     useSphereMode[MAX_PROBES];
uniform float     radius[MAX_PROBES];
uniform float2    attenuation[MAX_PROBES];

// Box Projected IBL Lighting
// Based on: http://www.gamedev.net/topic/568829-box-projected-cubemap-environment-mapping/
// and https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
float3 boxProject(float3 wsPosition, float3 reflectDir, float3 boxWSPos, float3 boxMin, float3 boxMax)
{ 
   float3 nrdir = reflectDir;
	float3 offset = wsPosition;
   float3 plane1vec = (boxMax - offset) / nrdir;
   float3 plane2vec = (boxMin - offset) / nrdir;
	
	float3 furthestPlane = max(plane1vec, plane2vec);
	float dist = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);
   float3 posonbox = offset + nrdir * dist;

   return posonbox - boxWSPos;
}

float3 iblBoxDiffuse( Surface surface, int id)
{
   float3 cubeN = boxProject(surface.P, surface.N, inProbePosArray[id], bbMinArray[id], bbMaxArray[id]);
   cubeN.z *=-1;
   //return TORQUE_TEXCUBEARRAYLOD(irradianceCubemap,cubeN,id,0).xyz;
   return float3(1,1,1);
}

float3 iblBoxSpecular(Surface surface, float3 surfToEye, TORQUE_SAMPLER2D(brdfTexture), int id)
{
   float ndotv = clamp(dot(surface.N, surfToEye), 0.0, 1.0);

    // BRDF
   float2 brdf = TORQUE_TEX2DLOD(brdfTexture, float4(surface.roughness, ndotv,0.0,0.0)).xy;

    // Radiance (Specular)
   float lod = surface.roughness*cubeMips;
   float3 r = reflect(surfToEye, surface.N);
   float3 cubeR = normalize(r);
   cubeR = boxProject(surface.P, surface.N, inProbePosArray[id], bbMinArray[id], bbMaxArray[id]);
	
   //float3 radiance = TORQUE_TEXCUBEARRAYLOD(cubeMap,cubeR,id,lod).xyz * (brdf.x + brdf.y);
   float3 radiance = float3(1,1,1);
    
   return radiance;
}

float defineBoxSpaceInfluence(Surface surface, int id)
{
    float tempAttenVal = 3.5; //replace with per probe atten
    float3 surfPosLS = mul( worldToObjArray[id], float4(surface.P,1.0)).xyz;

    float3 boxMinLS = inProbePosArray[id]-(float3(1,1,1)*radius[id]);
    float3 boxMaxLS = inProbePosArray[id]+(float3(1,1,1)*radius[id]);

    float boxOuterRange = length(boxMaxLS - boxMinLS);
    float boxInnerRange = boxOuterRange / tempAttenVal;

    float3 localDir = float3(abs(surfPosLS.x), abs(surfPosLS.y), abs(surfPosLS.z));
    localDir = (localDir - boxInnerRange) / (boxOuterRange - boxInnerRange);

    return max(localDir.x, max(localDir.y, localDir.z)) * -1;
}

float4 main( ConvexConnectP IN ) : SV_TARGET
{
    // Compute scene UV
   float3 ssPos = IN.ssPos.xyz / IN.ssPos.w; 
   float2 uvScene = getUVFromSSPos( ssPos, rtParams0 );

   //eye ray WS/LS
   float3 vsEyeRay = getDistanceVectorToPlane( -vsFarPlane.w, IN.vsEyeDir.xyz, vsFarPlane );
   float3 wsEyeRay = mul(cameraToWorld, float4(vsEyeRay, 0)).xyz;
   
   //unpack normal and linear depth 
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, uvScene);
   
   //create surface
   Surface surface = createSurface( normDepth, TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
                                    uvScene, eyePosWorld, wsEyeRay, cameraToWorld);	
   //early out if emissive
   if (getFlag(surface.matFlag, 0))
   {   
      discard;
   }                                    
	float blendVal[MAX_PROBES];
   float3 surfToEye = normalize(surface.P - eyePosWorld);

   int i;
	float blendSum = 0;
	float invBlendSum = 0;
      
   for(i=0; i < numProbes; i++)
   {
        float3 probeWS = inProbePosArray[i];
        float3 L = probeWS - surface.P;
      
        if(useSphereMode[i])
        {
            float3 L = inProbePosArray[i] - surface.P;
            blendVal[i] = 1.0-length(L)/radius[i];
            blendVal[i] = max(0,blendVal[i]);
        }
        else
        {
            blendVal[i] = defineBoxSpaceInfluence(surface, i);
            blendVal[i] = max(0,blendVal[i]);		
        }
		blendSum += blendVal[i];
        invBlendSum +=(1.0f - blendVal[i]);
   }
	
   // Weight0 = normalized NDF, inverted to have 1 at center, 0 at boundary.
   // And as we invert, we need to divide by Num-1 to stay normalized (else sum is > 1). 
   // respect constraint B.
   // Weight1 = normalized inverted NDF, so we have 1 at center, 0 at boundary
   // and respect constraint A.
   for(i=0; i < numProbes; i++)
   {
      blendVal[i] = (1.0f - ( blendVal[i] / blendSum)) / (numProbes - 1);
      blendVal[i] *= ((1.0f - blendVal[i]) / invBlendSum);
      blendSum += blendVal[i];
   }

    // Normalize blendVal
   if (blendSum == 0.0f) // Possible with custom weight
   {
      blendSum = 1.0f;
   }

   float invBlendSumWeighted = 1.0f / blendSum;
   for (i = 0; i < numProbes; ++i)
   {
      blendVal[i] *= invBlendSumWeighted;
   }
    
   float3 irradiance = float3(0,0,0);
   float3 specular = float3(0,0,0);
   float3 F = FresnelSchlickRoughness(surface.NdotV, surface.f0, surface.roughness);
   
   //energy conservation
   float3 kD = 1.0.xxx - F;
   kD *= 1.0 - surface.metalness;
   for (i = 0; i < numProbes; ++i)
   {
      irradiance += blendVal[i]*iblBoxDiffuse(surface,i);
      
      specular += blendVal[i]*F*iblBoxSpecular(surface, surfToEye, TORQUE_SAMPLER2D_MAKEARG(BRDFTexture),i);
   }
   //final diffuse color
   float3 diffuse = kD * irradiance * surface.baseColor.rgb;
	return float4(diffuse + specular * surface.ao, blendSum);
}
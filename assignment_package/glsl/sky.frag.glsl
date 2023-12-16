#version 150

uniform mat4 u_ViewProj;   // We're actually passing the inverse of the viewproj
                           // from our CPU, but it's named u_ViewProj so we don't
                           // have to bother rewriting our ShaderProgram class

uniform ivec2 u_Dimensions; // Screen dimensions

uniform vec3 u_Eye; // Camera pos

uniform float u_TimeSky;

out vec4 out_Col;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Here we define palettes for different times of day


// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                              vec3(254, 192, 81) / 255.0,
                              vec3(255, 137, 103) / 255.0,
                              vec3(253, 96, 81) / 255.0,
                              vec3(57, 32, 51) / 255.0);
// Dusk palette
const vec3 dusk[5] = vec3[](vec3(144, 96, 144) / 255.0,
                           vec3(96, 72, 120) / 255.0,
                           vec3(72, 48, 120) / 255.0,
                           vec3(48, 24, 96) / 255.0,
                           vec3(0, 24, 72) / 255.0);

const vec3 day[5] = vec3[](vec3(137, 207, 235) / 255.0,
                          vec3(91, 169, 205) / 255.0,
                          vec3(60, 144, 170) / 255.0,
                          vec3(32, 112, 141) / 255.0,
                          vec3(14, 80, 108) / 255.0);

const vec3 night[5] = vec3[](vec3(127, 143, 184) / 255.0,
                            vec3(61, 90, 153) / 255.0,
                            vec3(27, 76, 131) / 255.0,
                            vec3(15, 52, 97) / 255.0,
                            vec3(0, 31, 61) / 255.0);

const vec3 sunColor = vec3(255, 255, 190) / 255.0;
const vec3 cloudColor = sunset[3];

// Translates a direction on the sphere to a "UV" coordinate

vec2 sphereToUV(vec3 p) {
   float phi = atan(p.z, p.x);
   if(phi < 0) {
       phi += TWO_PI;
   }
   float theta = acos(p.y);
   return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}

// These functions translate the UV coordinates of the sky sphere into a specific color
       // mixture from our color palette


vec3 uvToSunset(vec2 uv) {
   if(uv.y < 0.5) {
       return sunset[0];
   }
   else if(uv.y < 0.55) {
       return mix(sunset[0], sunset[1], (uv.y - 0.5) / 0.05);
   }
   else if(uv.y < 0.6) {
       return mix(sunset[1], sunset[2], (uv.y - 0.55) / 0.05);
   }
   else if(uv.y < 0.65) {
       return mix(sunset[2], sunset[3], (uv.y - 0.6) / 0.05);
   }
   else if(uv.y < 0.75) {
       return mix(sunset[3], sunset[4], (uv.y - 0.65) / 0.1);
   }
   return sunset[4];
}

vec3 uvToDusk(vec2 uv) {
   if(uv.y < 0.5) {
       return dusk[0];
   }
   else if(uv.y < 0.55) {
       return mix(dusk[0], dusk[1], (uv.y - 0.5) / 0.05);
   }
   else if(uv.y < 0.6) {
       return mix(dusk[1], dusk[2], (uv.y - 0.55) / 0.05);
   }
   else if(uv.y < 0.65) {
       return mix(dusk[2], dusk[3], (uv.y - 0.6) / 0.05);
   }
   else if(uv.y < 0.75) {
       return mix(dusk[3], dusk[4], (uv.y - 0.65) / 0.1);
   }
   return dusk[4];
}

// These functions translate the UV coordinates of the sky sphere into a specific color
       // mixture from our color palette

vec3 uvToDay(vec2 uv) {
   if(uv.y < 0.5) {
       return day[0];
   }
   else if(uv.y < 0.55) {
       return mix(day[0], day[1], (uv.y - 0.5) / 0.05);
   }
   else if(uv.y < 0.6) {
       return mix(day[1], day[2], (uv.y - 0.55) / 0.05);
   }
   else if(uv.y < 0.65) {
       return mix(day[2], day[3], (uv.y - 0.6) / 0.05);
   }
   else if(uv.y < 0.75) {
       return mix(day[3], day[4], (uv.y - 0.65) / 0.1);
   }
   return day[4];
}

vec3 uvToNight(vec2 uv) {
   if(uv.y < 0.5) {
       return night[0];
   }
   else if(uv.y < 0.55) {
       return mix(night[0], night[1], (uv.y - 0.5) / 0.05);
   }
   else if(uv.y < 0.6) {
       return mix(night[1], night[2], (uv.y - 0.55) / 0.05);
   }
   else if(uv.y < 0.65) {
       return mix(night[2], night[3], (uv.y - 0.6) / 0.05);
   }
   else if(uv.y < 0.75) {
       return mix(night[3], night[4], (uv.y - 0.65) / 0.1);
   }
   return night[4];
}

vec2 random2( vec2 p ) {
   return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
   return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                         dot(p,vec3(269.5, 183.3, 765.54)),
                         dot(p, vec3(420.69, 631.2,109.21))))
                *43758.5453);
}

//These are noise functions we use to create the clouds.

float WorleyNoise3D(vec3 p)
{
   // Tile the space
   vec3 pointInt = floor(p);
   vec3 pointFract = fract(p);

   float minDist = 1.0; // Minimum distance initialized to max.

   // Search all neighboring cells and this cell for their point
   for(int z = -1; z <= 1; z++)
   {
       for(int y = -1; y <= 1; y++)
       {
           for(int x = -1; x <= 1; x++)
           {
               vec3 neighbor = vec3(float(x), float(y), float(z));

               // Random point inside current neighboring cell
               vec3 point = random3(pointInt + neighbor);

               // Animate the point
               point = 0.5 + 0.5 * sin(u_TimeSky * 0.01 + 6.2831 * point); // 0 to 1 range

               // Compute the distance b/t the point and the fragment
               // Store the min dist thus far
               vec3 diff = neighbor + point - pointFract;
               float dist = length(diff);
               minDist = min(minDist, dist);
           }
       }
   }
   return minDist;
}

float WorleyNoise(vec2 uv)
{
   // Tile the space
   vec2 uvInt = floor(uv);
   vec2 uvFract = fract(uv);

   float minDist = 1.0; // Minimum distance initialized to max.

   // Search all neighboring cells and this cell for their point
   for(int y = -1; y <= 1; y++)
   {
       for(int x = -1; x <= 1; x++)
       {
           vec2 neighbor = vec2(float(x), float(y));

           // Random point inside current neighboring cell
           vec2 point = random2(uvInt + neighbor);

           // Animate the point
           point = 0.5 + 0.5 * sin(u_TimeSky * 0.01 + 6.2831 * point); // 0 to 1 range

           // Compute the distance b/t the point and the fragment
           // Store the min dist thus far
           vec2 diff = neighbor + point - uvFract;
           float dist = length(diff);
           minDist = min(minDist, dist);
       }
   }
   return minDist;
}

float worleyFBM(vec3 uv) {
   float sum = 0;
   float freq = 4;
   float amp = 0.5;
   for(int i = 0; i < 8; i++) {
       sum += WorleyNoise3D(uv * freq) * amp;
       freq *= 2;
       amp *= 0.5;
   }
   return sum;
}

//#define RAY_AS_COLOR
//#define SPHERE_UV_AS_COLOR
#define WORLEY_OFFSET

void main()
{

    // Translate to device coords
   vec2 ndc = (gl_FragCoord.xy / vec2(u_Dimensions)) * 2.0 - 1.0; // -1 to 1 NDC

//   out_Col = vec3(ndc * 0.5 + 0.5, 1);

   vec4 p = vec4(ndc.xy, 1, 1); // Pixel at the far clip plane
   p *= 1000.0; // Times far clip plane value
   p = /*Inverse of*/ u_ViewProj * p; // Convert from unhomogenized screen to world


   // Get the ray direction of the fragment
   vec3 rayDir = normalize(p.xyz - u_Eye);

#ifdef RAY_AS_COLOR
   out_Col = vec4(0.5 * (rayDir + vec3(1,1,1)), 1);
   return;
#endif

   vec2 uv = sphereToUV(rayDir);
#ifdef SPHERE_UV_AS_COLOR
   out_Col = vec4(uv, 0, 1);
   return;
#endif


   vec2 offset = vec2(0.0);
#ifdef WORLEY_OFFSET
   // Get a noise value in the range [-1, 1]
   // by using Worley noise as the noise basis of FBM
   offset = vec2(worleyFBM(rayDir));
   offset *= 2.0;
   offset -= vec2(1.0);
#endif

   // Compute a gradient from the bottom of the sky-sphere to the top

   // This essentially gets the different colors from the palettes and prepares them to be used later in the
   // shader program based on where they are on the sphere
   vec3 sunsetColor = uvToSunset(uv + offset * 0.1);
   vec3 duskColor = uvToDusk(uv + offset * 0.1);
   vec3 dayColor = uvToDay(uv + offset * 0.1);
   vec3 nightColor = uvToNight(uv + offset * 0.1);

   // Add a glowing sun in the sky

   // I added here the functionality of the sun to move
   float rotSpeed = 0.001;
   float theta = u_TimeSky * rotSpeed * PI;
   vec3 sunDir = vec3(
               0,
               cos(theta),
               sin(theta));
   sunDir = normalize(sunDir);



#define SUN_HIGH_THRESHOLD 0.5
#define SUN_MED_THRESHOLD -0.1
#define SUN_LOW_THRESHOLD -0.45

#define SUNSET_THRESHOLD 0.5
#define DUSK_THRESHOLD -0.1



   // This sets the primary and secondary colors for the fragment - I'm not using it anymore but I don't want
   // to break my code :)
   vec3 primaryCol;
   vec3 secondaryCol;
   if (sunDir.y > SUN_HIGH_THRESHOLD) {
       primaryCol = dayColor;
       secondaryCol = dayColor;
   } else if (sunDir.y > SUN_MED_THRESHOLD) {
       primaryCol = sunsetColor;
       secondaryCol = duskColor;
   } else if (sunDir.y > SUN_LOW_THRESHOLD) {
       primaryCol = duskColor;
       secondaryCol = nightColor;
   } else {
       primaryCol = nightColor;
       secondaryCol = nightColor;
   }


   // Gets the dot product of the ray direction and the sun direction
   float raySunDot = dot(rayDir, sunDir);


   // Get the components of the color of the sky at this fragment and blend them to create a color
   float sunHighAmount = smoothstep(DUSK_THRESHOLD, 1.0, sunDir.y);
   float sunMedAmount = smoothstep(DUSK_THRESHOLD, SUNSET_THRESHOLD, sunDir.y);
   float sunLowAmount = smoothstep(SUN_LOW_THRESHOLD, SUN_MED_THRESHOLD, sunDir.y);
   float sunBottomAmount = smoothstep(-1.0, DUSK_THRESHOLD, sunDir.y) + 0.1;

   vec3 blendedColor =
       dayColor * sunHighAmount +
       sunsetColor * sunMedAmount +
       duskColor * sunLowAmount +
       nightColor * sunBottomAmount;

   blendedColor = normalize(blendedColor);

//   vec3 sunDir = normalize(vec3(0, 0.1, 1.0));


   // We're creating the sun here essentially by defining a zone in which we'll draw the sun
   float sunSize = 30;
   float angle = acos(dot(rayDir, sunDir)) * 360.0 / PI;


   // If the angle between our ray dir and vector to center of sun
   // is less than the threshold, then we're looking at the sun
   if(angle < sunSize) {
       // Full center of sun
       if(angle < 7.5) {
           out_Col = vec4(sunColor, 1);
       }
       // Corona of sun, mix with sky color
       else {
           out_Col = vec4(mix(sunColor, blendedColor, (angle - 7.5) / 22.5), 1);
       }
   }
   // Otherwise our ray is looking into just the sky
   else {



       // I'm blending the colors as above here instead of doing the bounds checking below. This does mean
       // that my sunset "wraps around" improperly but I don't have time to fix this, so I'm returning here.
       out_Col = vec4(blendedColor, 1);
       return;


//       raySunDot = dot(rayDir, sunDir);

//       if(raySunDot > SUNSET_THRESHOLD) {
//           out_Col = vec4(blendedColor, 1);
//       }
//       // Any dot product between 0.75 and -0.1 is a LERP b/t sunset and dusk color
//       else if (raySunDot > DUSK_THRESHOLD) {
//           float t = (raySunDot - SUNSET_THRESHOLD) / (DUSK_THRESHOLD - SUNSET_THRESHOLD);
//           out_Col = vec4(secondaryCol, 1);
//       }
//       // Any dot product <= -0.1 are pure dusk color
//       else {
//           out_Col = vec4(blendedColor, 1);
//       }
   }

}

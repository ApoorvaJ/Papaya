
#version 120                                                  
                                                              
uniform float Cursor; // Uniforms[1]
                                                              
varying  vec2 Frag_UV;                                        
                                                              
// Source: Fast branchless RGB to HSV conversion in GLSL
// http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl

vec3 hsv2rgb(vec3 c)                                          
{                                                             
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);            
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);         
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y); 
}                                                             
                                                              
void main()                                                   
{                                                             
    vec4 Hue = vec4(hsv2rgb(vec3(1.0-Frag_UV.y, 1.0, 1.0))    
               .xyz,1.0);                                     
    if (abs(0.5 - Frag_UV.x) > 0.3333)                        
    {                                                         
        gl_FragColor = vec4(0.36,0.36,0.37,                   
                    float(abs(1.0-Frag_UV.y-Cursor) < 0.0039));
    }                                                         
    else                                                      
        gl_FragColor = Hue;                                   
}                                                             

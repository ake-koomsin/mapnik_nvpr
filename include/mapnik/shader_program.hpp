#ifndef MAPNIK_SHADER_PROGRAM_HPP
#define MAPNIK_SHADER_PROGRAM_HPP

#define MULTILINE(...) #__VA_ARGS__

namespace mapnik {

static const char clearShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {
    gl_FragColor = vec4(0.0);
}

);
static const char srcShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {
	vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
	
    gl_FragColor = src;
}

);
static const char dstShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {
	vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);
	
    gl_FragColor = dst;
}

);
static const char srcOverShader[] = MULTILINE(

uniform sampler2DMS srcTexture;
uniform sampler2DMS dstTexture;

void main (void) {

    // vec2 iTmp = textureSize(srcTexture);
    // vec2 tmp = iTmp * gl_TexCoord[1].st;

    vec2 iTmp2 = textureSize(dstTexture);
    vec2 tmp2 = iTmp2 * gl_TexCoord[2].st;

    //vec4 src = vec4(0.0); //= texelFetch(srcTexture, ivec2(tmp), gl_SampleID);//texture(srcTexture, interpolateAtSample(gl_TexCoord[1].st, gl_SampleID));//texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = vec4(0.0); //= texelFetch(dstTexture, ivec2(tmp), gl_SampleID);//texture(dstTexture, interpolateAtSample(gl_TexCoord[2].st, gl_SampleID));//texture2D(dstTexture, gl_TexCoord[2].st);

    for (int i = 0; i < 4; i++) {
        //src = src + texelFetch(srcTexture, ivec2(tmp), i);
        dst = dst + texelFetch(dstTexture, ivec2(tmp2), i);
    }

    //src = src * 0.25;
    dst = dst * 0.25;

    vec3  premultipledSrc    = gl_Color.rgb;
    vec3  premultipledDst    = dst.rgb;

    vec4 result = vec4(0.0);

    // Dca' = Sca × Da + Sca × (1 - Da) + Dca × (1 - Sa)
    //      = Sca + Dca × (1 - Sa)
    // Da'  = Sa × Da + Sa × (1 - Da) + Da × (1 - Sa)
    //      = Sa + Da - Sa × Da

    result.a   = gl_Color.a + dst.a - gl_Color.a * dst.a;
    result.rgb = (premultipledSrc + premultipledDst * (1.0 - gl_Color.a));

    gl_FragColor = result;
}

);
static const char dstOverShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Dca × Sa + Sca × (1 - Da) + Dca × (1 - Sa)
    //      = Dca + Sca × (1 - Da)
    // Da'  = Da × Sa + Sa × (1 - Da) + Da × (1 - Sa)
    //      = Sa + Da - Sa × Da

    result.a   = src.a + dst.a - (src.a * dst.a);
    result.rgb = (premultipledDst + premultipledSrc * (1.0 - dst.a)) / result.a;

    gl_FragColor = result;
}

);
static const char srcInShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Sca × Da
    // Da'  = Sa × Da

    result.a   = src.a * dst.a;
    result.rgb = (premultipledSrc * dst.a) / result.a;

    gl_FragColor = result;
}

);
static const char dstInShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Dca × Sa
    // Da'  = Sa × Da

    result.a   = src.a * dst.a;
    result.rgb = (premultipledDst * src.a) / result.a;

    gl_FragColor = result;
}

);
static const char srcOutShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Sca × (1 - Da)
    // Da'  = Sa × (1 - Da)

    result.a   = src.a * (1.0 - dst.a);
    result.rgb = (premultipledSrc * (1 - dst.a)) / result.a;

    gl_FragColor = result;
}

);
static const char dstOutShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Dca × (1 - Sa) 
    // Da'  = Da × (1 - Sa)

    result.a   = dst.a * (1.0 - src.a);
    result.rgb = (premultipledDst * (1 - src.a)) / result.a;

    gl_FragColor = result;
}

);
static const char srcAtopShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Sca × Da + Dca × (1 - Sa)
    // Da'  = Sa × Da + Da × (1 - Sa)
    //      = Da

    result.a   = dst.a;
    result.rgb = (premultipledSrc * dst.a + premultipledDst * (1.0 - src.a)) / result.a;

    gl_FragColor = result;
}

);
static const char dstAtopShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Dca × Sa + Sca × (1 - Da)
    // Da'  = Da × Sa + Sa × (1 - Da)
    //      = Sa

    result.a   = src.a;
    result.rgb = (premultipledDst * src.a + premultipledSrc * (1.0 - dst.a)) / result.a;

    gl_FragColor = result;
}

);
static const char xorShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Sca × (1 - Da) + Dca × (1 - Sa)
    // Da'  = Sa × (1 - Da) + Da × (1 - Sa)
    //      = Sa + Da - 2 × Sa × Da

    result.a   = src.a + dst.a - 2 * src.a * dst.a;
    result.rgb = (premultipledSrc * (1 - dst.a) + premultipledDst * (1.0 - src.a)) / result.a;

    gl_FragColor = result;
}

);
static const char plusShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Sca × Da + Dca × Sa + Sca × (1 - Da) + Dca × (1 - Sa)
    //      = Sca + Dca
    // Da'  = Sa × Da + Da × Sa + Sa × (1 - Da) + Da × (1 - Sa)
    //      = Sa + Da

    result.a   = src.a + dst.a;
    result.rgb = (premultipledSrc + premultipledDst) / result.a;

    gl_FragColor = result;
}

);
static const char minusShader[] = "";
static const char multiplyShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = Sca × Dca + Sca × (1 - Da) + Dca × (1 - Sa)
    // Da'  = Sa × Da + Sa × (1 - Da) + Da × (1 - Sa)
    //      = Sa + Da - Sa × Da

    result.a   = src.a + dst.a - (src.a * dst.a);
    result.rgb = (premultipledSrc * premultipledDst + premultipledSrc * (1.0 - dst.a) + premultipledDst * (1.0 - src.a)) / result.a;

    gl_FragColor = result;
}

);
static const char screenShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = (Sca × Da + Dca × Sa - Sca × Dca) + Sca × (1 - Da) + Dca × (1 - Sa)
    //      = Sca + Dca - Sca × Dca
    // Da'  = Sa + Da - Sa × Da

    result.a   = src.a + dst.a - (src.a * dst.a);
    result.rgb = (premultipledSrc + premultipledDst - (premultipledSrc * premultipledDst)) / result.a;

    gl_FragColor = result;
}

);
static const char overlayShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // if 2 × Dca <= Da
    //   Dca' = 2 × Sca × Dca + Sca × (1 - Da) + Dca × (1 - Sa)
    // otherwise
    //   Dca' = Sa × Da - 2 × (Da - Dca) × (Sa - Sca) + Sca × (1 - Da) + Dca × (1 - Sa)
    //        = Sca × (1 + Da) + Dca × (1 + Sa) - 2 × Dca × Sca - Da × Sa
    
    // Da' = Sa + Da - Sa × Da
    
    result.a   = src.a + dst.a - (src.a * dst.a);
    
    //if (2 * premultipledDst <= dst.a) {
	//	result.rgb = 2 * premultipledSrc * premultipledDst + premultipledSrc * (1 - dst.a) + premultipledDst * (1 - src.a);
	//} else {
	//	result.rgb = premultipledSrc * (1 + dst.a) + premultipledDst * (1 + src.a) - 2 * premultipledDst * premultipledSrc - (dst.a * src.a);
	//}
    
    //result.rgb = (premultipledSrc + premultipledDst - (premultipledSrc * premultipledDst)) / result.a;

    gl_FragColor = result;
}

);
static const char darkenShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = min(Sca × Da, Dca × Sa) + Sca × (1 - Da) + Dca × (1 - Sa)
    // Da'  = Sa + Da - Sa × Da 

    result.a   = src.a + dst.a - (src.a * dst.a);
    result.rgb = (min(premultipledSrc * dst.a, premultipledDst * src.a) + premultipledSrc * (1 - dst.a) + premultipledDst * (1 - src.a)) / result.a;

    gl_FragColor = result;
}

);
static const char lightenShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // Dca' = max(Sca × Da, Dca × Sa) + Sca × (1 - Da) + Dca × (1 - Sa)
    // Da'  = Sa + Da - Sa × Da

    result.a   = src.a + dst.a - (src.a * dst.a);
    result.rgb = (max(premultipledSrc * dst.a, premultipledDst * src.a) + premultipledSrc * (1 - dst.a) + premultipledDst * (1 - src.a)) / result.a;

    gl_FragColor = result;
}

);
static const char colorDodgeShader[] = MULTILINE(

uniform sampler2D srcTexture;
uniform sampler2D dstTexture;

void main (void) {

    vec4 src = texture2D(srcTexture, gl_TexCoord[1].st);
    vec4 dst = texture2D(dstTexture, gl_TexCoord[2].st);

    vec3  premultipledSrc    = src.rgb * src.a;
    vec3  premultipledDst    = dst.rgb * dst.a;

    vec4 result = vec4(0.0);

    // if Sca == Sa and Dca == 0
    //   Dca' = Sca × (1 - Da) + Dca × (1 - Sa)
    //        = Sca × (1 - Da)
    // otherwise if Sca == Sa
    //   Dca' = Sa × Da + Sca × (1 - Da) + Dca × (1 - Sa)
    // otherwise if Sca < Sa
    //   Dca' = Sa × Da × min(1, Dca/Da × Sa/(Sa - Sca)) + Sca × (1 - Da) + Dca × (1 - Sa)

    // Da'  = Sa + Da - Sa × Da

    result.a   = src.a + dst.a - (src.a * dst.a);
    
    // TODO
    gl_FragColor = result;
}

);
static const char colorBurnShader[] = "";
static const char hardLightShader[] = "";
static const char softLightShader[] = "";
static const char differenceShader[] = "";
static const char exclusionShader[] = "";
static const char contrastShader[] = "";
static const char invertShader[] = "";
static const char invertRGBShader[] = "";
static const char grainMergeShader[] = "";
static const char grainExtractShader[] = "";
static const char hueShader[] = "";
static const char saturationShader[] = "";
static const char colorShader[] = "";
static const char valueShader[] = "";

static const char *shaderTable[35] = {
    clearShader,
    srcShader,
    dstShader,
    srcOverShader,
    dstOverShader,
    srcInShader,
    dstInShader,
    srcOutShader,
    dstOutShader,
    srcAtopShader,
    dstAtopShader,
    xorShader,
    plusShader,
    minusShader,
    multiplyShader,
    screenShader,
    overlayShader,
    darkenShader,
    lightenShader,
    colorDodgeShader,
    colorBurnShader,
    hardLightShader,
    softLightShader,
    differenceShader,
    exclusionShader,
    contrastShader,
    invertShader,
    invertRGBShader,
    grainMergeShader,
    grainExtractShader,
    hueShader,
    saturationShader,
    colorShader,
    valueShader
};

}

#endif // MAPNIK_SHADER_PROGRAM_HPP


// uniform sampler2D srcTexture;
// uniform sampler2D dstTexture;
// // uniform float opacity;

// void main (void)
// {
//     vec4 src  = texture2D(srcTexture, gl_TexCoord[0].st);
//     vec4 dst = texture2D(dstTexture, gl_TexCoord[1].st);

//     float normalizedSrcAlpha = src.a / 255;
//     float normalizedDstAlpha = dst.a / 255;
//     vec3  premultipledSrc    = src.rgb * normalizedSrcAlpha;
//     vec3  premultipledDst    = dst.rgb * normalizedDstAlpha;

//     vec4 result;

//     result.rgb = premultipledSrc + premultipledDst * (1 - normalizedSrcAlpha);
//     result.a   = (normalizedSrcAlpha + normalizedDstAlpha - normalizedSrcAlpha * normalizedDstAlpha) * 255;

//     gl_FragColor = result;
// }

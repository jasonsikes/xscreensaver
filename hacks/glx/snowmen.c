/* snowmen, Copyright (c) 2023 Jason Sikes <sikes@acm.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */


#define DEFAULTS    "*delay:    30000       \n"

# define release_snow 0

#include "xlockmore.h"
#include <ctype.h>

/* because I sometimes use printf() for debugging. */
#include <stdio.h>

#include "ximage-loader.h"
#include "snowmen_textures.h"

#ifdef USE_GL /* whole file */


typedef struct {
    GLfloat x,y,z;
} Vert3d;

typedef struct {
    GLfloat x,y,z,w;
} Vert4d;

typedef struct {
    GLfloat x,y;
} Vert2d;



/* Because M_PI might not be defined. */
const GLfloat kPi = 3.141592653589;
const GLfloat kTau = 2 * kPi;

const GLdouble kCameraPerspectiveAngle = 36;
const GLdouble kCameraRadius = 33;
const GLdouble kCameraHeight = 7;

/* Near and far view planes. */
const GLdouble kHither = 10;
const GLdouble kYon    = 200;

/* 'D' means 'Delta'. On every frame,
 * move the camera and snowmen by this amount.
 */
const GLdouble kDCameraRho = 0.001;
const GLfloat kDSnowmanRho = 0.007;

const GLfloat kCarrotScaleDivide = 1.9;
const Vert3d kCarrotOutlineInverseScale = {0.6, 0.6, 0.7};

const GLfloat kSkateLateralDistance = 0.7;
const GLfloat kSkateHeight          = 0.01;
const GLfloat kSkateVentralDistance = 0;
const GLfloat kSkateScale           = 1.0 / 30;

const GLfloat kSnowmanTravelRadiusVariation    = 2;
const GLfloat kSnowmanTravelRadius             = 10;
const GLfloat kSnowmanMinorLoopMinimumCount    = 3;
const GLfloat kSnowmanMinorLoopCountRange      = 2.0;
const GLfloat kSnowmanTiltAngleMultiplier      = -10;
const GLfloat kSnowballRadiusStart             = 1;
const GLfloat kSnowballOverlap                 = 1.2;
const GLfloat kSnowballOutlineRadiusIncrement  = 0.06;
const GLfloat kSnowballSizeRelativeToPrevious  = 0.75;



const GLfloat kUniverseEdgeRadius = 150;
const GLfloat kShoreHeight = 1;
const GLfloat kHillsHeight = 20;


const unsigned int kCountOfSnowmen = 9;

/* IMPORTANT! See comment under 'setupTrees()' for how to set this value. */
const unsigned int kCountOfTrees = 27;

const unsigned int kCountOfHatSlices = 16;
const unsigned int kCountOfTreeSkirts = 5;

const unsigned int kCountOfPondExteriorVertices = 160;
const unsigned int kCountOfPondWeights = 11;

// Caution: texture has outline that is expecting exactly this number
const unsigned kCountOfTreeSkirtVertices = 16;

const unsigned kCountOfTreeTrunkSlices = 8;

// some colors
const Vert4d kColorSky = {0.53, 0.81, 0.92, 1};
const GLfloat kColorSnow[] = {1, 0.98, 0.98, 1};
const GLfloat kColorCarrot[] = {1, 0.35, 0, 1};
const GLfloat kColorSkate[] = {0.5, 0.5, 0.5, 1.0};
const GLfloat kColorSnowmanArm[] = {0.55, 0.37, 0.17, 1};
const GLfloat kColorIce[] = {0.6, 0.8, 0.9, 0.92};
const GLfloat kColorTreeTrunk[] = {0.55, 0.27, 0.07, 1};
const GLfloat kColorInkOutline[] = {0, 0, 0, 1};
const GLfloat kColorShadow[] = {0, 0, 0, 0.1};

Vert4d snowmanHatColors[kCountOfSnowmen] = {
    {
        .65, .16, .16, 1 // brown
    }, {
        1, .97, .86, 1     // Cornsilk
    }, {
        .6, .2, .8, 1  // Dark Orchid
    }, {
        .87, .63, .87 , 1     // Plum
    }, {
        .86, .08, .24, 1 // Crimson
    }, {
        .98, .5, .54, 1 // Salmon
    }, {
        .96, .64, .38, 1 // Sandy Brown
    }, {
        .5, .5, .5, 1    // Gray
    }, {
        1.0, 0.35, 0, 1  // Orange added because Rachael likes orange
    }
};


// For the snowball, pond, shore, etc.
typedef struct {
    unsigned countOfVertices;
    Vert3d *vertAry;
    Vert3d *outlineVertAry;
    Vert2d *texAry;
} VertexTextureObject_t;

// For the hat
typedef struct {
    Vert3d *vertAry;
    Vert3d *outlineVertAry;
    GLuint *indexAry;
    void *brimBottomTriangleFanIndices;
    GLuint countOfBrimBottomTriangleFanIndices;
    void *brimTopTriangleFanIndices;
    GLuint countOfBrimTopTriangleFanIndices;
    void *brimTriangleStripIndices;
    GLuint countOfBrimTriangleStripIndices;
    void *stemTopTriangleFanIndices;
    GLuint countOfStemTopTriangleFanIndices;
    void *stemTriangleStripIndices;
    GLuint countOfStemTriangleStripIndices;
} HatStruct_t;

// The IDs of all of the vertex buffer objects.
typedef enum {
    // Snowball (A snowman is made from a snowball drawn three times: head, torso, and base.)
    snowballCoordID,
    snowballTexID,
    
    // the skate
    skateCoordID,
    skateIndicesID,
    
    // the pond
    pondCoordID,
    pondTexID,
    
    // the shore
    shoreCoordID,
    shoreTexID,
    
    // the hills
    hillsCoordID,
    hillsTexID,
    
    // the trees
    treesCoordID,
    treesOutlineCoordID,
    treesTexID,
    
    // The carrot nose
    carrotCoordID,
    carrotIndicesID,
    
    // Hat
    hatCoordID,
    hatOutlineCoordID,
    hatIndicesID,
    
    // Arm
    armCoordID,
    armCoordOutlineID,
    armIndicesID,
    
    countOfVboIDs
} VertexBufferID_t;

// The IDs of the texture buffers.
typedef enum {
    kTextureIDSnowmanBase,
    kTextureIDSnowmanHead,
    kTextureIDSnowmanTorso,
    kTextureIDHills,
    kTextureIDIce,
    kTextureIDShore,
    kTextureIDTrees,
    kCountOfTextureIDs
} TextureBufferID_t;

// The scene is drawn in stages, but some elements are only drawn at certain stages
typedef enum {
    drawingStageIDShadow,
    drawingStageIDReflection,
    drawingStageIDReal
} DrawingStageID_t;

// Everything about each snowman.
typedef struct {
    Vert4d hatColor;
    GLfloat baseSnowballRotation;
    GLfloat startingRho;
    GLfloat startingMinorRho;
    GLfloat countOfMinorLoops;
    GLfloat currentRho;
    GLfloat minorRho;
    GLfloat positionX;
    GLfloat positionY;
    GLfloat direction;
    GLfloat tilt;
} SnowmanState_t;


// Everything about each tree
typedef struct {
    Vert3d location;
    GLfloat height;
    GLfloat bottomSkirtRadius;
    GLfloat rotation;
} TreeState_t;

typedef struct {
    GLXContext *glx_context;
    
    GLuint bufferID[countOfVboIDs];
    GLuint textureID[kCountOfTextureIDs];
    
    VertexTextureObject_t snowballAry;
    VertexTextureObject_t pondInfo;
    VertexTextureObject_t treesInfo;
    VertexTextureObject_t shoreInfo;
    VertexTextureObject_t hillsInfo;
    
    HatStruct_t hatInfo;
    
    GLdouble cameraRho;
    GLfloat snowmanRho;
    
    // In normal circumstances, we would simply use GL_FRONT and GL_BACK for face culling,
    // but since we are sometimes drawing reflections and making an outline, it's better to use state variables.
    GLenum cullingFaceFront;
    GLenum cullingFaceBack;
    
    // Set this to True when we are drawing shadows.
    Bool isDrawingShadows;

    SnowmanState_t snowmanIndividual[kCountOfSnowmen];
    TreeState_t treeIndividual[kCountOfTrees];
    
} snow_configuration;

// prototypes.
static void normalizeVertex(Vert3d *v);
static void divideTriangle(snow_configuration *bp,
                           Vert3d *va, Vert3d *vb, Vert3d *vc,
                           Vert2d *ta, Vert2d *tb, Vert2d *tc,
                           int iteration, unsigned int *snowballAssemblyIndex);
static void createBufferObjects(snow_configuration *bp);
static void createAllTextures(ModeInfo *mi);
static void createTexture(ModeInfo *mi,
                          GLuint textureID,
                          const unsigned char *data,
                          unsigned int dataLen);
static void initTree(TreeState_t *state,
              Vert3d *location,
              GLfloat height,
              GLfloat bottomSkirtRadius);
static void setupTrees(snow_configuration *bp);
static void setupSnowmen(snow_configuration *bp);
static void initSnowman(SnowmanState_t *state,
                 GLfloat startingRho,
                 Vert4d *hatColor);
static void snowmanUpdateState(SnowmanState_t *state, GLfloat rho);
static void drawSnowball(snow_configuration *bp,
                  GLfloat radius,
                  GLfloat outlineRadius,
                  GLuint texture);
static void drawArms(snow_configuration *bp);
static void drawSkate(snow_configuration *bp, Bool isOnLeft);
static void drawCarrot(snow_configuration *bp, GLfloat radius);
static void drawSnowman(snow_configuration *bp, SnowmanState_t *state);
static void drawObjectsForStage(snow_configuration *bp, DrawingStageID_t stage);
static void drawHat(snow_configuration *bp, Vert4d *hatColor, GLfloat radius);
static void drawIce(snow_configuration *bp);
static void drawShore(snow_configuration *bp);
static void drawHills(snow_configuration *bp);
static void drawTrees(snow_configuration *bp);
static void drawSnowmen(snow_configuration *bp);
static void drawTree(snow_configuration *bp, TreeState_t *state);
static void setShadowMatrix(Bool isDrawingShore);



static snow_configuration *bps = NULL;


/*************************************************
 *
 *     SNOWBALL
 *
 *************************************************/

// Normalize a vertex so that it remains on the surface of a sphere.
static void normalizeVertex(Vert3d *v)
{
    GLfloat d = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x /= d;
    v->y /= d;
    v->z /= d;
}

// Given triangle vertex and texture coordinates and an iteration value, either:
// * emit the given triangle to the snowball array being generated, or
// * subdivide the triangle into four by creating midpoints of the three vertices.
static void divideTriangle(snow_configuration *bp,
                           Vert3d *va, Vert3d *vb, Vert3d *vc,
                           Vert2d *ta, Vert2d *tb, Vert2d *tc,
                           int iteration, unsigned int *snowballAssemblyIndex)
{
    if (iteration > 0) {
        Vert3d v1, v2, v3;
        Vert2d t1, t2, t3;
        
        v1.x = va->x + vb->x;
        v1.y = va->y + vb->y;
        v1.z = va->z + vb->z;
        t1.x = (ta->x + tb->x) / 2;
        t1.y = (ta->y + tb->y) / 2;
        normalizeVertex(&v1);
        
        v2.x = va->x + vc->x;
        v2.y = va->y + vc->y;
        v2.z = va->z + vc->z;
        t2.x = (ta->x + tc->x) / 2;
        t2.y = (ta->y + tc->y) / 2;
        normalizeVertex(&v2);
        
        v3.x = vc->x + vb->x;
        v3.y = vc->y + vb->y;
        v3.z = vc->z + vb->z;
        t3.x = (tc->x + tb->x) / 2;
        t3.y = (tc->y + tb->y) / 2;
        normalizeVertex(&v3);
        
        divideTriangle(bp, va, &v1, &v2, ta, &t1, &t2, iteration - 1, snowballAssemblyIndex);
        divideTriangle(bp, vc, &v2, &v3, tc, &t2, &t3, iteration - 1, snowballAssemblyIndex);
        divideTriangle(bp, vb, &v3, &v1, tb, &t3, &t1, iteration - 1, snowballAssemblyIndex);
        divideTriangle(bp, &v1, &v3, &v2, &t1, &t3, &t2, iteration - 1, snowballAssemblyIndex);
    } else {
        Vert3d *vertAry = bp->snowballAry.vertAry;
        Vert2d *texAry = bp->snowballAry.texAry;
        
        vertAry[*snowballAssemblyIndex] = *va;
        texAry[(*snowballAssemblyIndex)++] = *ta;
        
        vertAry[*snowballAssemblyIndex] = *vb;
        texAry[(*snowballAssemblyIndex)++] = *tb;
        
        vertAry[*snowballAssemblyIndex] = *vc;
        texAry[(*snowballAssemblyIndex)++] = *tc;
    }
}

static void createSnowballBufferObjects(snow_configuration *bp)
{
    // Three iterations seems the perfect number to make what appears to be a lumpy snowball.
    const int countOfIterations = 3;
    unsigned int snowballAssemblyIndex = 0;
    VertexTextureObject_t *ary = &(bp->snowballAry);
    
    int countOfVertices = (int) powf(4, (countOfIterations + 1)) * 3;
    ary->countOfVertices = countOfVertices;
    ary->vertAry = malloc(sizeof(Vert3d) * countOfVertices);
    ary->texAry = malloc(sizeof(Vert2d) * countOfVertices);
    
    // The texture is made from four equilateral triangles each starting at the center
    // out to each of the four edges.
    // We don't use the four corners of the texture image.
    const GLfloat a = 0.211325;
    const GLfloat b = 0.788675;
    
    // Our snowball is a rough sphere, starting with a tetrahedron and subdividing the
    // surface vertices and texture coordinates.
    Vert3d vertSeed[] = {
        {
            0.0, 1.0, 0.0
        }, {
            0.816497, -0.333333, 0.471405
        }, {
            0.0, -0.333333, -0.942809
        }, {
            -0.816497, -0.333333, 0.471405
        }
    };
    Vert2d texSeed[] = {
        {
            0.5, 0.5 // 0 center
        }, {
            b, 1     // 1 face a0b0 - left
        }, {
            a, 1
        }, {
            1, a     // 3 bottom
        }, {
            1, b
        }, {
            0, b     // 5 left b1a1 - right
        }, {
            0, a
        }, {
            a, 0     // 7 right 0b0a - face
        }, {
            b, 0
        }
    };
    
    // face
    divideTriangle(bp,
                   vertSeed + 0, vertSeed + 1, vertSeed + 2,
                   texSeed + 0,  texSeed + 1,  texSeed + 2,
                   countOfIterations, &snowballAssemblyIndex);
    
    // bottom
    divideTriangle(bp,
                   vertSeed + 3, vertSeed + 2, vertSeed + 1,
                   texSeed + 0,  texSeed + 3,  texSeed + 4,
                   countOfIterations, &snowballAssemblyIndex);
    
    // left
    divideTriangle(bp,
                   vertSeed + 0, vertSeed + 2, vertSeed + 3,
                   texSeed + 0,  texSeed + 5,  texSeed + 6,
                   countOfIterations, &snowballAssemblyIndex);
    
    // right
    divideTriangle(bp,
                   vertSeed + 0, vertSeed + 3, vertSeed + 1,
                   texSeed + 0,  texSeed + 7,  texSeed + 8,
                   countOfIterations, &snowballAssemblyIndex);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + snowballCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 bp->snowballAry.countOfVertices * sizeof(Vert3d),
                 bp->snowballAry.vertAry,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + snowballTexID) );
    glBufferData(GL_ARRAY_BUFFER,
                 bp->snowballAry.countOfVertices * sizeof(Vert2d),
                 bp->snowballAry.texAry,
                 GL_STATIC_DRAW);
}


/*************************************************
 *
 *     POND
 *
 *************************************************/


// Given a value and a range, normalize the value on [0..1]
static GLfloat normalizeOnRange(GLfloat value, GLfloat min, GLfloat max)
{
    return (value - min) / (max - min);
}

static void createPondBufferObjects(snow_configuration *bp)
{
    /* DON'T CHANGE THESE VALUES!
     * After setting these, other elements have become dependent on them.
     */
    const float weights[kCountOfPondWeights] = {
        // 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
        12, 2, 0, 3, 4, 5, 0, 0, 0, 0
    };
    const float kPondWeightModerator = 0.7;
    
    VertexTextureObject_t *ps = &(bp->pondInfo);
    
    // The pond is drawn as a triangle fan. The first vertex (index 0) is the center.
    // The second vertex (index 1) is the start of the outer edge. The last vertex is
    // a repeat of the first vertex to complete the fan.
    ps->countOfVertices = kCountOfPondExteriorVertices + 2;
    ps->vertAry = (Vert3d*)malloc((ps->countOfVertices) * sizeof(Vert3d));
    ps->texAry = (Vert2d*)malloc((ps->countOfVertices) * sizeof(Vert2d));
    
    Vert3d *vertPtr = ps->vertAry;
    Vert2d *texPtr = ps->texAry;
    unsigned i, j;
    GLfloat minX = 0;
    GLfloat minZ = 0;
    GLfloat maxX = 0;
    GLfloat maxZ = 0;
    float r;
    GLfloat x, z;
    
    // First, emit the center of the fan
    vertPtr->x = 0;
    vertPtr->y = 0;
    vertPtr->z = 0;
    ++vertPtr;
    
    // computer the pond vertices as a polar function of the sum of sines
    for (i = 0; i < kCountOfPondExteriorVertices; ++i) {
        r = 0;
        for (j = 0; j < kCountOfPondWeights; ++j) {
            r += weights[j] * (2 + sinf(3.0 * i * j * kTau/kCountOfPondExteriorVertices));
        }
        x = kPondWeightModerator * r * sinf(kTau * i / kCountOfPondExteriorVertices);
        z = kPondWeightModerator * r * cosf(kTau * i / kCountOfPondExteriorVertices);
        vertPtr->x = x;
        vertPtr->y = 0;
        vertPtr->z = z;
        ++vertPtr;
        
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }
    
    // Complete the triangle fan
    *vertPtr = (ps->vertAry)[1];
    
    // Build the texture coordinates
    vertPtr = ps->vertAry;
    for (i = 0; i < ps->countOfVertices; ++i) {
        texPtr->x = normalizeOnRange(vertPtr->x, minX, maxX);
        texPtr->y = normalizeOnRange(vertPtr->z, minZ, maxZ);
        ++vertPtr;
        ++texPtr;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + pondCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 ps->countOfVertices * sizeof(Vert3d),
                 ps->vertAry,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + pondTexID) );
    glBufferData(GL_ARRAY_BUFFER,
                 ps->countOfVertices * sizeof(Vert2d),
                 ps->texAry,
                 GL_STATIC_DRAW);
}

/*************************************************
 *
 *     TREES
 *
 *************************************************/

static void createTreeBufferObjects(snow_configuration *bp)
{
    const GLfloat heightOfTrunk = 0.3;
    const GLfloat upperRadiusOfTrunk = 0.11;
    const GLfloat lowerRadiusOfTrunk = 0.15;
    const GLfloat treeHeightCenterAdd = 0.26;
    const GLfloat treeHeightAlternateEdgeAdd = 0.06;
    const GLfloat outlineCenterAdd = 0.04;
    const GLfloat outlineEdgeAdd = 0.05;
    const GLfloat outlineEdgeMult = 1.03;

    int i;
    int j;
    unsigned countOfTreeFanVertices = (kCountOfTreeSkirtVertices + 2) * kCountOfTreeSkirts;
    unsigned countOfTrunkVertices = (kCountOfTreeTrunkSlices + 1) * 2;
    Vert3d *vertPtr = (Vert3d*) malloc(( countOfTreeFanVertices + countOfTrunkVertices)  * sizeof(Vert3d));
    Vert3d *outlineVertPtr = (Vert3d*) malloc(countOfTreeFanVertices * sizeof(Vert3d));
    Vert2d *texPtr = (Vert2d*) malloc(countOfTreeFanVertices * sizeof(Vert2d));
    
    bp->treesInfo.vertAry = vertPtr;
    bp->treesInfo.outlineVertAry = outlineVertPtr;
    bp->treesInfo.texAry = texPtr;
    
    Vert3d *startVert;
    Vert3d *outlineStartVert;
    Vert2d *startTex;
    
    
    for (j = 0; j < kCountOfTreeSkirts; ++j) {
        GLfloat normalizedHeight  = (j + 1.0) / kCountOfTreeSkirts; // 0 = base, 1 = top
        GLfloat rotation = random() * kTau / RAND_MAX;
        
        // emit the center vertex
        vertPtr->x = 0;
        vertPtr->y = normalizedHeight + treeHeightCenterAdd * (2 - normalizedHeight);
        vertPtr->z = 0;
        ++vertPtr;
        outlineVertPtr->x = 0;
        outlineVertPtr->y = normalizedHeight + treeHeightCenterAdd * (2 - normalizedHeight) + outlineCenterAdd;
        outlineVertPtr->z = 0;
        ++outlineVertPtr;
        texPtr->x = 0.5;
        texPtr->y = 0.5;
        ++texPtr;
        
        startVert = vertPtr;
        outlineStartVert = outlineVertPtr;
        startTex = texPtr;
        
        GLfloat edgeRadius = 1.0 - j/(1.0*kCountOfTreeSkirts);
        for (i = 0; i < kCountOfTreeSkirtVertices; ++i) {
            vertPtr->x = edgeRadius * sinf( i * kTau / kCountOfTreeSkirtVertices + rotation);
            vertPtr->y = normalizedHeight + (i & 1) * treeHeightAlternateEdgeAdd * (1.2-normalizedHeight);
            vertPtr->z = edgeRadius * cosf( i * kTau / kCountOfTreeSkirtVertices + rotation);
            ++vertPtr;
            outlineVertPtr->x = (edgeRadius * outlineEdgeMult + outlineEdgeAdd) * sinf( i * kTau / kCountOfTreeSkirtVertices + rotation);
            outlineVertPtr->y = normalizedHeight + (i & 1) * treeHeightAlternateEdgeAdd * (1-normalizedHeight);
            outlineVertPtr->z = (edgeRadius * outlineEdgeMult + outlineEdgeAdd) * cosf( i * kTau / kCountOfTreeSkirtVertices + rotation);
            ++outlineVertPtr;
            texPtr->x = 0.5 + sinf(i * kTau / kCountOfTreeSkirtVertices) / 2;
            texPtr->y = 0.5 + cosf(i * kTau / kCountOfTreeSkirtVertices) / 2;
            ++texPtr;
        } // /for i
        
        // Complete the circle
        *vertPtr = *startVert;
        ++vertPtr;
        *outlineVertPtr = *outlineStartVert;
        ++outlineVertPtr;
        *texPtr = *startTex;
        ++texPtr;
    } // /for j
    
    startVert = vertPtr;
    for (i = 0; i < kCountOfTreeTrunkSlices; ++i) {
        vertPtr->x = lowerRadiusOfTrunk * cosf(i * kTau / kCountOfTreeTrunkSlices);
        vertPtr->y = 0;
        vertPtr->z = lowerRadiusOfTrunk * sinf(i * kTau / kCountOfTreeTrunkSlices);
        ++vertPtr;
        vertPtr->x = upperRadiusOfTrunk * cosf(i * kTau / kCountOfTreeTrunkSlices);
        vertPtr->y = heightOfTrunk;
        vertPtr->z = upperRadiusOfTrunk * sinf(i * kTau / kCountOfTreeTrunkSlices);
        ++vertPtr;
    }
    
    // Complete the circle
    *vertPtr = *startVert;
    *(vertPtr + 1) = *(startVert + 1);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + treesCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 ( countOfTreeFanVertices + countOfTrunkVertices)  * sizeof(Vert3d),
                 bp->treesInfo.vertAry,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + treesOutlineCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfTreeFanVertices  * sizeof(Vert3d),
                 bp->treesInfo.outlineVertAry,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + treesTexID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfTreeFanVertices * sizeof(Vert2d),
                 bp->treesInfo.texAry,
                 GL_STATIC_DRAW);
}


/*************************************************
 *
 *     HILLS
 *
 *************************************************/

static void createHillsBufferObjects(snow_configuration *bp)
{
    unsigned countOfShoreVertices = bp->pondInfo.countOfVertices - 1; // skip the first one
    unsigned int countOfVertices = countOfShoreVertices * 2;
    GLfloat textureRepeatCount = 3; // number of times texture repeats as it goes around

    GLfloat textureYMin = 0.002;
    GLfloat textureYMax = 0.998;
    Vert3d *vertPtr = malloc(countOfVertices * sizeof(Vert3d));
    Vert2d *texPtr = malloc(countOfVertices * sizeof(Vert2d));
    bp->hillsInfo.countOfVertices = countOfVertices;
    bp->hillsInfo.vertAry = vertPtr;
    bp->hillsInfo.texAry = texPtr;
    int i;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat d;
    Vert3d *shorePtr = bp->pondInfo.vertAry + 1; // Skip the pond center vertex
    
    // Calculate the coordinates
    for (i = 0; i < countOfShoreVertices; ++i) {
        x = shorePtr->x;
        y = shorePtr->y;
        z = shorePtr->z;
        ++shorePtr;
        d = sqrtf(x*x + z*z);
        x = x/d * kUniverseEdgeRadius;
        z = z/d * kUniverseEdgeRadius;
        
        vertPtr->x = x;
        vertPtr->y = y;
        vertPtr->z = z;
        ++vertPtr;
        
        vertPtr->x = x;
        vertPtr->y = kHillsHeight;
        vertPtr->z = z;
        ++vertPtr;
    }
    
    // Calculate the texture coordinates
    for (i = 0; i < countOfShoreVertices; ++i) {
        // The '2' is because the repeat count must be even
        x = i * textureRepeatCount * 2/ (countOfShoreVertices - 1);
        
        texPtr->x = x;
        texPtr->y = textureYMin;
        ++texPtr;
        
        texPtr->x = x;
        texPtr->y = textureYMax;
        ++texPtr;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + hillsCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 bp->hillsInfo.countOfVertices * sizeof(Vert3d),
                 bp->hillsInfo.vertAry,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + hillsTexID) );
    glBufferData(GL_ARRAY_BUFFER,
                 bp->hillsInfo.countOfVertices * sizeof(Vert2d),
                 bp->hillsInfo.texAry,
                 GL_STATIC_DRAW);
}

/*************************************************
 *
 *     SHORE
 *
 *************************************************/

static void createShoreBufferObjects(snow_configuration *bp)
{
    int countOfSlopedCircleSteps = 7;
    GLfloat radiusOfSlopedCircle = 2;   // distance of smaller steps from the pond edge
    GLfloat radiusOfFlatCircle = kUniverseEdgeRadius;
    GLfloat textureRepeatCount = 3;     // number of times texture repeats as it goes around the pond
    GLfloat portionOfTexture = 0.25;    // only use this amount of the texture on the sloped part
    GLfloat curveValue = 1.0 / 3;       // sharpness of the shore's curvature
    GLfloat heightOfShore = kShoreHeight;
    unsigned countOfShoreVertices = bp->pondInfo.countOfVertices - 2; // skip the first and last
    unsigned countOfVertices = countOfShoreVertices * 2 * (countOfSlopedCircleSteps + 1);
    // The pond source information we are interested in.
    Vert3d *shorePtr2 = bp->pondInfo.vertAry + 1; // Skip the pond center vertex
    GLfloat d2 = sqrtf(shorePtr2->x * shorePtr2->x + shorePtr2->z * shorePtr2->z);
    GLfloat d1;
    Vert3d *shorePtr1;
    Vert3d *destCoordPtr = malloc(countOfVertices * sizeof(Vert3d));
    Vert2d *destTexPtr = malloc(countOfVertices * sizeof(Vert2d));
    GLfloat texEdgeX1;
    GLfloat texEdgeX2;
    
    bp->shoreInfo.countOfVertices = countOfVertices;
    bp->shoreInfo.vertAry = destCoordPtr;
    bp->shoreInfo.texAry = destTexPtr;
    
    
    // The shore is drawn as curved rings connected by spokes
    int circle;
    int spoke;
    for (spoke = 0; spoke < countOfShoreVertices ; ++spoke) {
        shorePtr1 = shorePtr2;
        ++shorePtr2;
        d1 = d2;
        d2 = sqrtf(shorePtr2->x * shorePtr2->x + shorePtr2->z * shorePtr2->z);
        texEdgeX1 = textureRepeatCount * 2 * spoke       / countOfShoreVertices;
        texEdgeX2 = textureRepeatCount * 2 * (spoke + 1) / countOfShoreVertices;
        for (circle = 0; circle < countOfSlopedCircleSteps; ++circle) {
            if (circle == countOfSlopedCircleSteps - 1) {
                // Last circle is much further away
                destCoordPtr->x = shorePtr1->x / d1 * radiusOfFlatCircle;
                destCoordPtr->y = heightOfShore;
                destCoordPtr->z = shorePtr1->z / d1 * radiusOfFlatCircle;
                ++destCoordPtr;
                destTexPtr->x = texEdgeX1;
                destTexPtr->y = 0.01;
                ++destTexPtr;
                
                for (int i = 0; i < 2; ++i) {
                    destCoordPtr->x = shorePtr2->x / d2 * radiusOfFlatCircle;
                    destCoordPtr->y = heightOfShore;
                    destCoordPtr->z = shorePtr2->z / d2 * radiusOfFlatCircle;
                    ++destCoordPtr;
                    destTexPtr->x = texEdgeX2;
                    destTexPtr->y = 0.01;
                    ++destTexPtr;
                }
            } else {
                destCoordPtr->x = shorePtr1->x + shorePtr1->x / d1 * circle / countOfSlopedCircleSteps * radiusOfSlopedCircle;
                destCoordPtr->y = heightOfShore * powf(circle * radiusOfSlopedCircle / countOfSlopedCircleSteps, curveValue);
                destCoordPtr->z = shorePtr1->z + shorePtr1->z / d1 * circle / countOfSlopedCircleSteps * radiusOfSlopedCircle;
                ++destCoordPtr;
                destTexPtr->x = texEdgeX1;
                destTexPtr->y = 1-portionOfTexture * powf(circle / (countOfSlopedCircleSteps - 2.0), curveValue);
                ++destTexPtr;
                if (circle == 0) {
                    destCoordPtr->x = shorePtr1->x + shorePtr1->x / d1 * circle / countOfSlopedCircleSteps * radiusOfSlopedCircle;
                    destCoordPtr->y = heightOfShore * powf(circle * radiusOfSlopedCircle / countOfSlopedCircleSteps, curveValue);
                    destCoordPtr->z = shorePtr1->z + shorePtr1->z / d1 * circle / countOfSlopedCircleSteps * radiusOfSlopedCircle;
                    ++destCoordPtr;
                    destTexPtr->x = texEdgeX1;
                    destTexPtr->y = 1-portionOfTexture * powf(circle / (countOfSlopedCircleSteps - 2.0), curveValue);
                    ++destTexPtr;
                }
                
                destCoordPtr->x = shorePtr2->x + shorePtr2->x / d2 * circle / countOfSlopedCircleSteps * radiusOfSlopedCircle;
                destCoordPtr->y = heightOfShore * powf(circle * radiusOfSlopedCircle / countOfSlopedCircleSteps, curveValue);
                destCoordPtr->z = shorePtr2->z + shorePtr2->z / d2 * circle / countOfSlopedCircleSteps * radiusOfSlopedCircle;
                ++destCoordPtr;
                destTexPtr->x = texEdgeX2;
                destTexPtr->y = 1-portionOfTexture * powf(circle / (countOfSlopedCircleSteps - 2.0), curveValue);
                ++destTexPtr;
                
            }
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + shoreCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 bp->shoreInfo.countOfVertices * sizeof(Vert3d),
                 bp->shoreInfo.vertAry,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + shoreTexID) );
    glBufferData(GL_ARRAY_BUFFER,
                 bp->shoreInfo.countOfVertices * sizeof(Vert2d),
                 bp->shoreInfo.texAry,
                 GL_STATIC_DRAW);
}

/*************************************************
 *
 *     HAT
 *
 *************************************************/

static void createHatBufferObjects(snow_configuration *bp)
{
    const GLfloat BRIM_RADIUS = 1;
    const GLfloat BRIM_BOTTOM_Y = -0.05;
    const GLfloat BRIM_TOP_Y = 0.05;
    const GLfloat STEM_TOP_RADIUS = 0.6;
    const GLfloat STEM_BOTTOM_RADIUS = 0.5;
    const GLfloat STEM_BOTTOM_Y = 0.05;
    const GLfloat STEM_TOP_Y = 1.2;

    const GLfloat OUTLINE = 0.05;
    const GLfloat OUTLINE_BRIM_RADIUS = BRIM_RADIUS + OUTLINE;
    const GLfloat OUTLINE_BRIM_BOTTOM_Y = BRIM_BOTTOM_Y - OUTLINE;
    const GLfloat OUTLINE_BRIM_TOP_Y = BRIM_TOP_Y + OUTLINE;
    const GLfloat OUTLINE_STEM_TOP_RADIUS = STEM_TOP_RADIUS + OUTLINE;
    const GLfloat OUTLINE_STEM_BOTTOM_RADIUS = STEM_BOTTOM_RADIUS + OUTLINE;
    const GLfloat OUTLINE_STEM_BOTTOM_Y = STEM_BOTTOM_Y + OUTLINE;
    const GLfloat OUTLINE_STEM_TOP_Y = STEM_TOP_Y + OUTLINE;

    int i;
    unsigned vertexIter = 0;
    unsigned indexIter = 0;
    unsigned countOfVertices = kCountOfHatSlices * 4;
    unsigned countOfIndices = kCountOfHatSlices * 7 + 4;
    
    bp->hatInfo.vertAry =        (Vert3d*) malloc( countOfVertices * sizeof(Vert3d) );
    bp->hatInfo.outlineVertAry = (Vert3d*) malloc( countOfVertices * sizeof(Vert3d) );
    bp->hatInfo.indexAry =       (GLuint*) malloc( countOfIndices * sizeof(GLuint) );
    
    Vert3d *vertAry =        bp->hatInfo.vertAry;
    Vert3d *outlineVertAry = bp->hatInfo.outlineVertAry;
    GLuint *indexAry =       bp->hatInfo.indexAry;
    
    
    // build the brim bottom triangle fan
    bp->hatInfo.brimBottomTriangleFanIndices = (void*) (indexIter * sizeof(GLuint));
    bp->hatInfo.countOfBrimBottomTriangleFanIndices = kCountOfHatSlices;
    GLuint brimBottomIndices = vertexIter;
    for ( i = 0; i < kCountOfHatSlices; ++i ) {
        ( vertAry + vertexIter ) -> x = -sinf( kTau * i / kCountOfHatSlices ) * BRIM_RADIUS;
        ( vertAry + vertexIter ) -> y = BRIM_BOTTOM_Y;
        ( vertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * BRIM_RADIUS;

        ( outlineVertAry + vertexIter ) -> x = -sinf( kTau * i / kCountOfHatSlices ) * OUTLINE_BRIM_RADIUS;
        ( outlineVertAry + vertexIter ) -> y = OUTLINE_BRIM_BOTTOM_Y;
        ( outlineVertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * OUTLINE_BRIM_RADIUS;

        *( indexAry + indexIter ) = vertexIter;
        ++vertexIter;
        ++indexIter;
    }
    
    // build the brim top triangle fan
    bp->hatInfo.brimTopTriangleFanIndices = (void*) (indexIter * sizeof(GLuint));
    bp->hatInfo.countOfBrimTopTriangleFanIndices = kCountOfHatSlices;
    GLuint brimTopIndices = vertexIter;
    for ( i = 0; i < kCountOfHatSlices; ++i ) {
        ( vertAry + vertexIter ) -> x = sinf( kTau * i / kCountOfHatSlices ) * BRIM_RADIUS;
        ( vertAry + vertexIter ) -> y = BRIM_TOP_Y;
        ( vertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * BRIM_RADIUS;

        ( outlineVertAry + vertexIter ) -> x = sinf( kTau * i / kCountOfHatSlices ) * OUTLINE_BRIM_RADIUS;
        ( outlineVertAry + vertexIter ) -> y = OUTLINE_BRIM_TOP_Y;
        ( outlineVertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * OUTLINE_BRIM_RADIUS;

        *( indexAry + indexIter ) = vertexIter;
        ++vertexIter;
        ++indexIter;
    }
    
    // build the brim triangle strips between the bottom and top
    bp->hatInfo.countOfBrimTriangleStripIndices = kCountOfHatSlices * 2 + 2;
    bp->hatInfo.brimTriangleStripIndices = (void*) (indexIter * sizeof(GLuint));
    for ( i = 0; i < kCountOfHatSlices; ++i ) {
        *( indexAry + indexIter ) = brimTopIndices + i;
        ++indexIter;
        *( indexAry + indexIter ) = brimBottomIndices + kCountOfHatSlices - 1 - i;
        ++indexIter;
    }
    // finish the loop
    *( indexAry + indexIter ) = brimTopIndices;
    ++indexIter;
    *( indexAry + indexIter ) = brimBottomIndices + kCountOfHatSlices - 1;
    ++indexIter;
    
    // build the stem top triangle fan
    bp->hatInfo.countOfStemTopTriangleFanIndices = kCountOfHatSlices;
    bp->hatInfo.stemTopTriangleFanIndices = (void*) (indexIter * sizeof(GLuint));
    GLuint stemTopIndices = vertexIter;
    for ( i = 0; i < kCountOfHatSlices; ++i ) {
        ( vertAry + vertexIter ) -> x = sinf( kTau * i / kCountOfHatSlices ) * STEM_TOP_RADIUS;
        ( vertAry + vertexIter ) -> y = STEM_TOP_Y;
        ( vertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * STEM_TOP_RADIUS;

        ( outlineVertAry + vertexIter ) -> x = sinf( kTau * i / kCountOfHatSlices ) * OUTLINE_STEM_TOP_RADIUS;
        ( outlineVertAry + vertexIter ) -> y = OUTLINE_STEM_TOP_Y;
        ( outlineVertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * OUTLINE_STEM_TOP_RADIUS;

        *( indexAry + indexIter ) = vertexIter;
        ++vertexIter;
        ++indexIter;
    }
    GLuint stemBottomIndices = vertexIter;
    for ( i = 0; i < kCountOfHatSlices; ++i ) {
        ( vertAry + vertexIter ) -> x = sinf( kTau * i / kCountOfHatSlices ) * STEM_BOTTOM_RADIUS;
        ( vertAry + vertexIter ) -> y = STEM_BOTTOM_Y;
        ( vertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * STEM_BOTTOM_RADIUS;

        ( outlineVertAry + vertexIter ) -> x = sinf( kTau * i / kCountOfHatSlices ) * OUTLINE_STEM_BOTTOM_RADIUS;
        ( outlineVertAry + vertexIter ) -> y = OUTLINE_STEM_BOTTOM_Y;
        ( outlineVertAry + vertexIter ) -> z = cosf( kTau * i / kCountOfHatSlices ) * OUTLINE_STEM_BOTTOM_RADIUS;

        ++vertexIter;
    }
    
    // build the stem triangle strips between the bottom and the top
    bp->hatInfo.countOfStemTriangleStripIndices = kCountOfHatSlices * 2 + 2;
    bp->hatInfo.stemTriangleStripIndices = (void*) (indexIter * sizeof(GLuint));
    for ( i = 0; i < kCountOfHatSlices; ++i ) {
        *( indexAry + indexIter ) = stemTopIndices + i;
        ++indexIter;
        *( indexAry + indexIter ) = stemBottomIndices + i;
        ++indexIter;
    }
    // finish the loop
    *( indexAry + indexIter ) = stemTopIndices;
    ++indexIter;
    *( indexAry + indexIter ) = stemBottomIndices;
    ++indexIter;
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + hatCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfVertices * sizeof(Vert3d),
                 vertAry,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, * ( (bp->bufferID) + hatIndicesID) );
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 countOfIndices * sizeof(GLuint),
                 indexAry,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + hatOutlineCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfVertices * sizeof(Vert3d),
                 outlineVertAry,
                 GL_STATIC_DRAW);
}


/*************************************************
 *
 *     ARM
 *
 *************************************************/


// Create a circle of eight vertices for a part of the arm.
// Output will be written to memory pointed to by dest.
// A total of [ 24 * sizeof(GLfloat) ] bytes will be written.
static Vert3d * createArmJoint(Vert3d *dest,
                               Vert3d origin,
                               Vert3d range)
{
    const GLfloat sqrt2 = 1 / sqrtf(2);
    Vert3d range45 = {
        range.x * sqrt2,
        range.y * sqrt2,
        range.z * sqrt2
    };
    
    *dest = (Vert3d) {
        origin.x - range.x,
        origin.y + range.y,
        origin.z
    };
    ++dest;
    *dest = (Vert3d) {
        origin.x - range45.x,
        origin.y + range45.y,
        origin.z + range45.z
    },
    ++dest;
    *dest = (Vert3d) {
        origin.x,
        origin.y,
        origin.z + range.z
    },
    ++dest;
    *dest = (Vert3d) {
        origin.x + range45.x,
        origin.y - range45.y,
        origin.z + range45.z
    },
    ++dest;
    *dest = (Vert3d) {
        origin.x + range.x,
        origin.y - range.y,
        origin.z
    },
    ++dest;
    *dest = (Vert3d) {
        origin.x + range45.x,
        origin.y - range45.y,
        origin.z - range45.z
    },
    ++dest;
    *dest = (Vert3d) {
        origin.x,
        origin.y,
        origin.z - range.z
    },
    ++dest;
    *dest = (Vert3d) {
        origin.x - range45.x,
        origin.y + range45.y,
        origin.z - range45.z
    };
    ++dest;
    return dest;
}

static void createArmBufferObjects(snow_configuration *bp)
{
    const int countOfVertices = 8 * 6;
    const GLfloat outlineWidth = 0.03;

    static Vert3d verts[countOfVertices];
    static Vert3d outlineVerts[countOfVertices];

    // Proximal joint [0-7]
    Vert3d pos = (Vert3d) {0.76, 0, 0};
    Vert3d range = (Vert3d) {0, 0.06, 0.06};
    Vert3d *vertPtr = createArmJoint(verts, pos, range);
    
    // Medial1 joint [8-15]
    pos = (Vert3d) {1.6, 0.2, 0};
    range = (Vert3d) {0, 0.05, 0.05};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Medial2 joint [16-23]
    pos = (Vert3d) {1.9, 0.1, 0};
    range = (Vert3d) {0, 0.04, 0.04};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Distal1 terminus [24-31]
    pos = (Vert3d) {2.3, 0.2, 0};
    range = (Vert3d) {0.01, 0.02, 0.02};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Distal2 terminus [32-39]
    pos = (Vert3d) {2, 0.5, 0.3};
    range = (Vert3d) {0.01, 0.02, 0.02};
    vertPtr = createArmJoint(vertPtr, pos, range);

    // Distal3 terminus [40-47]
    pos = (Vert3d) {2.3, 0, 0.1};
    range = (Vert3d) {-0.01, 0.02, 0.02};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Now we need to repeat the whole thing for the outline.

    // Proximal joint [0-7]
    pos = (Vert3d) {0.793, 0, 0};
    range = (Vert3d) {0, 0.06 + outlineWidth, 0.06 + outlineWidth};
    vertPtr = createArmJoint(outlineVerts, pos, range);
    
    // Medial1 joint [8-15]
    pos = (Vert3d) {1.6, 0.2, 0};
    range = (Vert3d) {0, 0.05 + outlineWidth, 0.05 + outlineWidth};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Medial2 joint [16-23]
    pos = (Vert3d) {1.9, 0.1, 0};
    range = (Vert3d) {0, 0.04 + outlineWidth, 0.04 + outlineWidth};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Distal1 terminus [24-31]
    pos = (Vert3d) {2.3 + outlineWidth, 0.2, 0};
    range = (Vert3d) {0.01, 0.02 + outlineWidth, 0.02 + outlineWidth};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    // Distal2 terminus [32-39]
    pos = (Vert3d) {2 + outlineWidth, 0.5, 0.3};
    range = (Vert3d) {0.03, 0.02 + outlineWidth, 0.02 + outlineWidth};
    vertPtr = createArmJoint(vertPtr, pos, range);

    // Distal3 terminus [40-47]
    pos = (Vert3d) {2.3 + outlineWidth, 0, 0.1};
    range = (Vert3d) {-0.01, 0.02 + outlineWidth, 0.02 + outlineWidth};
    vertPtr = createArmJoint(vertPtr, pos, range);
    
    const GLuint countOfIndices = 18 * 5 + 4 + 8;

    static const GLuint indices[countOfIndices] = {
        24,16, 25,17, 26,18, 27,19, 28,20, 29,21, 30,22, 31,23, 24,16,     // Segment 3
        16,8, 17,9, 18,10, 19,11, 20,12, 21,13, 22,14, 23,15, 16,8,        // Segment 2
        8,0, 9,1, 10,2, 11,3, 12,4, 13,5, 14,6, 15,7, 8,0, 0,              // Segment 1
        32, 32,8, 33,9, 34,10, 35,11, 36,12, 37,13, 38,14, 39,15, 32,8, 8, // Segment 4
        40, 40,16, 41,17, 42,18, 43,19, 44,20, 45,21, 46,22, 47,23, 40,16, // Segment 5
        7, 6, 5, 4, 3, 2, 1, 0                                             // Outline at Proximal joint
    };
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + armCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfVertices * sizeof(Vert3d),
                 verts,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + armCoordOutlineID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfVertices * sizeof(Vert3d),
                 outlineVerts,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, * ( (bp->bufferID) + armIndicesID) );
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 countOfIndices * sizeof(GLuint),
                 indices,
                 GL_STATIC_DRAW);
}

/*************************************************
 *
 *     SKATE
 *
 *************************************************/

static void createSkateBufferObjects(snow_configuration *bp)
{
    const GLfloat SKATE_HALF_WIDTH = 1;
    static const Vert3d verts[] = {
        {
            SKATE_HALF_WIDTH, 0, -15
        }, {
            SKATE_HALF_WIDTH, 5, -12
        }, {
            SKATE_HALF_WIDTH, 0, 13
        }, {
            SKATE_HALF_WIDTH, 5, 12
        }, {
            SKATE_HALF_WIDTH, 2, 18
        }, {
            SKATE_HALF_WIDTH, 6, 14
        }, {
            SKATE_HALF_WIDTH, 5, 20
        }, {
            SKATE_HALF_WIDTH, 8, 15
        }, {
            SKATE_HALF_WIDTH, 8, 20
        }, {
            SKATE_HALF_WIDTH, 10, 14
        }, {
            SKATE_HALF_WIDTH, 10, 19
        }, {
            SKATE_HALF_WIDTH, 12, 12
        }, {
            SKATE_HALF_WIDTH, 12, 15
        }, {
            -SKATE_HALF_WIDTH, 0, -15
        }, {
            -SKATE_HALF_WIDTH, 5, -12
        }, {
            -SKATE_HALF_WIDTH, 0, 13
        }, {
            -SKATE_HALF_WIDTH, 5, 12
        }, {
            -SKATE_HALF_WIDTH, 2, 18
        }, {
            -SKATE_HALF_WIDTH, 6, 14
        }, {
            -SKATE_HALF_WIDTH, 5, 20
        }, {
            -SKATE_HALF_WIDTH, 8, 15
        }, {
            -SKATE_HALF_WIDTH, 8, 20
        }, {
            -SKATE_HALF_WIDTH, 10, 14
        }, {
            -SKATE_HALF_WIDTH, 10, 19
        }, {
            -SKATE_HALF_WIDTH, 12, 12
        }, {
            -SKATE_HALF_WIDTH, 12, 15
        }
    };
    int countOfVertices = 26;
    
    static const GLuint indices[] = {
        25,24,23,22,21,20,19,18,17,16,15,14,13, // rstsi 13
        0,1,2,3,4,5,6,7,8,9,10,11,12, // lstsi 13
        0,13, 1,14, 3,16, 5,18, 7,20, 9,22, 11,24, 12,25, 10,23, 8,21, 6,19, 4,17, 2,15, 0,13 // ctsi 28
    };
    int countOfIndices = 13+13+28;
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + skateCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfVertices * sizeof(Vert3d),
                 verts,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, * ( (bp->bufferID) + skateIndicesID) );
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 countOfIndices * sizeof(GLuint),
                 indices,
                 GL_STATIC_DRAW);
}

/*************************************************
 *
 *     CARROT
 *
 *************************************************/

static void createCarrotBufferObjects(snow_configuration *bp)
{
    const GLfloat DORSAL_DIAMETER = 0.15;
    const GLfloat DIAG_DD         = 0.11;
    const GLfloat MEDIAL_DIAMETER = 0.1;
    const GLfloat DIAG_MD         = 0.07;
    const GLfloat DORSAL_Z        = 0.0;
    const GLfloat MEDIAL_Z        = 0.4;
    
    static const Vert3d verts[] = {
        // dorsal
        {
            0, DORSAL_DIAMETER, DORSAL_Z
        }, {
            -DIAG_DD, DIAG_DD, DORSAL_Z
        }, {
            -DORSAL_DIAMETER, 0, DORSAL_Z
        }, {
            -DIAG_DD, -DIAG_DD, DORSAL_Z
        }, {
            0, -DORSAL_DIAMETER, DORSAL_Z
        }, {
            DIAG_DD, -DIAG_DD, DORSAL_Z
        }, {
            DORSAL_DIAMETER, 0, DORSAL_Z
        }, {
            DIAG_DD, DIAG_DD, DORSAL_Z
        },
        
        // medial
        {
            0, MEDIAL_DIAMETER, MEDIAL_Z
        }, {
            -DIAG_MD, DIAG_MD, MEDIAL_Z
        }, {
            -MEDIAL_DIAMETER, 0, MEDIAL_Z
        }, {
            -DIAG_MD, -DIAG_MD, MEDIAL_Z
        }, {
            0, -MEDIAL_DIAMETER, MEDIAL_Z
        }, {
            DIAG_MD, -DIAG_MD, MEDIAL_Z
        }, {
            MEDIAL_DIAMETER, 0, MEDIAL_Z
        }, {
            DIAG_MD, DIAG_MD, MEDIAL_Z
        },
        
        // ventral point
        {
            0, -0.1, 0.8
        }
    };
    int countOfVertices = 17;
    
    static const GLuint indices[] = {
        8,0,9,1,10,2,11,3,12,4,13,5,14,6,15,7,8,0, // triangle strip    (18)
        16,8,9,10,11,12,13,14,15,8,                // triangle fan      (10)
        7,6,5,4,3,2,1,0                            // base triangle fan ( 8)
    };
    
    int countOfIndices = 18+10+8;
    
    glBindBuffer(GL_ARRAY_BUFFER, * ( (bp->bufferID) + carrotCoordID) );
    glBufferData(GL_ARRAY_BUFFER,
                 countOfVertices * sizeof(Vert3d),
                 verts,
                 GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, * ( (bp->bufferID) + carrotIndicesID) );
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 countOfIndices * sizeof(GLuint),
                 indices,
                 GL_STATIC_DRAW);
}

/*************************************************
 *
 *     TREES
 *
 *************************************************/

static void initTree(TreeState_t *state,
              Vert3d *location,
              GLfloat height,
              GLfloat bottomSkirtRadius)
{
    state->location = *location;
    state->height = height;
    state->bottomSkirtRadius = bottomSkirtRadius;
    state->rotation = random() * 360.0 / RAND_MAX;
}

static void setupTrees(snow_configuration *bp)
{
    /* The trees are assembled in little groups of one, two, or three, in this order:
     * [1,3,2,3,...]. This is the result of the "if (i % #...)" logic below.
     * IMPORTANT! If that logic changes or the count of tree groups changes, then the
     * kCountOfTrees value needs to be updated accordingly.
     */
    const unsigned countOfTreeGroups = 12;
    unsigned countOfPondVertices = bp->pondInfo.countOfVertices - 2; // skipping center and wrap vertices
    TreeState_t *tree = bp->treeIndividual;
    
    for (int i = 0; i < countOfTreeGroups; ++i) {
        Vert3d *pondVertex = bp->pondInfo.vertAry + i * countOfPondVertices / countOfTreeGroups + 1;
        GLfloat d = sqrtf(pondVertex->x * pondVertex->x + pondVertex->z * pondVertex->z);
        GLfloat treeDistance = d * 0.85 + kUniverseEdgeRadius * 0.15;
        Vert3d location;
        Vert3d tempLocation;
        
        location.x = pondVertex->x / d * treeDistance;
        location.y = kShoreHeight;
        location.z = pondVertex->z / d * treeDistance;
        initTree(tree, &location, 8, 3);
        ++tree;
        
        if (i % 2) {
            tempLocation = location;
            tempLocation.x += 0.5 * location.x + 0.2 * location.z;
            tempLocation.z += 0.5 * location.z - 0.1 * location.x;
            initTree(tree, &tempLocation, 12, 5);
            ++tree;
        }
        
        if (i % 4 > 1) {
            tempLocation = location;
            tempLocation.x += 0.5 * location.x - 0.1 * location.z;
            tempLocation.z += 0.7 * location.z + 0.3 * location.x;
            initTree(tree, &tempLocation, 14, 5);
            ++tree;
        }
        
    }
}


/*************************************************
 *
 *     SNOWMAN
 *
 *************************************************/

static void initSnowman(SnowmanState_t *state,
                 GLfloat startingRho,
                 Vert4d *hatColor)
{
    state->hatColor = *hatColor;
    state->startingRho = startingRho;
    state->startingMinorRho = random() * kTau / RAND_MAX;
    state->countOfMinorLoops = kSnowmanMinorLoopMinimumCount + floorf( kSnowmanMinorLoopCountRange / RAND_MAX * random() );
    state->baseSnowballRotation = random() * 360.0 / RAND_MAX;
}

static void setupSnowmen(snow_configuration *bp)
{
    for (int i = 0; i < kCountOfSnowmen; ++i) {
        initSnowman(bp->snowmanIndividual + i,
                    kTau * i / kCountOfSnowmen,
                    snowmanHatColors + i);
    }
}

static void snowmanUpdateState(SnowmanState_t *state, GLfloat rho)
{
    state->currentRho =0;
    rho = rho + state->startingRho;
    state->minorRho = rho * state->countOfMinorLoops + state->startingMinorRho;
    state->positionX = (kSnowmanTravelRadius + kSnowmanTravelRadiusVariation * sinf(state->minorRho)) * sinf(rho);
    state->positionY = (kSnowmanTravelRadius + kSnowmanTravelRadiusVariation * sinf(state->minorRho)) * cosf(rho);
    state->direction = rho * 180 / kPi + 90 - cosf(state->minorRho) * 30;
    state->tilt = kSnowmanTiltAngleMultiplier * sinf(state->minorRho);
}

/*************************************************
 *
 *     GENERAL INITIALIZATION
 *
 *************************************************/

static void createBufferObjects(snow_configuration *bp)
{
    glGenBuffers(countOfVboIDs, bp->bufferID);
    
    createPondBufferObjects(bp);
    createShoreBufferObjects(bp);
    createHillsBufferObjects(bp);
    createSkateBufferObjects(bp);
    createSnowballBufferObjects(bp);
    createCarrotBufferObjects(bp);
    createTreeBufferObjects(bp);
    createArmBufferObjects(bp);
    createHatBufferObjects(bp);
}

static void createTexture(ModeInfo *mi,
                          GLuint textureID,
                          const unsigned char *data,
                          unsigned int dataLen)
{
    glBindTexture (GL_TEXTURE_2D, textureID );
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    XImage *image = image_data_to_ximage (MI_DISPLAY (mi), MI_VISUAL (mi),
                                          data, dataLen);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 image->width, image->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image->data);
    XDestroyImage (image);
    
}

static void createAllTextures(ModeInfo *mi)
{
    snow_configuration *bp = &bps[MI_SCREEN(mi)];
    glGenTextures(kCountOfTextureIDs, bp->textureID);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDSnowmanBase),
                  base_png,
                  base_png_len);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDSnowmanHead),
                  head_png,
                  head_png_len);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDSnowmanTorso),
                  torso_png,
                  torso_png_len);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDHills),
                  hillTexture_png,
                  hillTexture_png_len);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDIce),
                  iceTexture_png,
                  iceTexture_png_len);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDShore),
                  shoreTexture_png,
                  shoreTexture_png_len);
    
    createTexture(mi,
                  * ( (bp->textureID) + kTextureIDTrees),
                  treeTexture_png,
                  treeTexture_png_len);
}

/* Window management, etc
 */
ENTRYPOINT ModeSpecOpt snow_opts = {0, NULL, 0, NULL, NULL};

ENTRYPOINT void
reshape_snow (ModeInfo *mi, int width, int height)
{
    double proportionWidthToHeight = (double) width / (double) height;
    glViewport( 0, 0, (GLsizei) width, (GLsizei) height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( kCameraPerspectiveAngle, proportionWidthToHeight,
                   kHither, kYon );
}


ENTRYPOINT Bool
snow_handle_event (ModeInfo *mi, XEvent *event)
{
    return False;
}

ENTRYPOINT void 
init_snow (ModeInfo *mi)
{
    snow_configuration *bp;
    
    MI_INIT (mi, bps);
    bp = &bps[MI_SCREEN(mi)];
    
    bp->glx_context = init_GL(mi);
    
    reshape_snow (mi, MI_WIDTH(mi), MI_HEIGHT(mi));
    
    bp->cameraRho = kPi/2;
    bp->snowmanRho = 0;

    setupSnowmen(bp);
    
    createBufferObjects(bp);
    createAllTextures(mi);
    setupTrees(bp);
}



/*************************************************
 *
 *     UPDATE and DRAW
 *
 *************************************************/

/* setShadowMatrix()
 * From Improving Shadows and Reflections via the Stencil Buffer
 * Mark J. Kilgard
 * NVIDIA Corporation
 */
// The following routine shadowMatrix constructs a 4x4 matrix and passes it to
// glMultMatrixf() which projects coordinates onto either
// isDrawingShore=False: the pond surface plane or
// isDrawingShore=True: the shore surface plane.
static void setShadowMatrix(Bool isDrawingShore)
{
    static GLfloat light[4] = { 0, 1, 1.1, 0 };
    static GLfloat shorePlane[4] = { 0, 1, 0, -kShoreHeight };
    static GLfloat pondPlane[4] = { 0, 1, 0, 0 };

    GLfloat m[4][4];
    
    GLfloat *plane = isDrawingShore ? shorePlane : pondPlane;

    GLfloat dot = plane[0]*light[0] + plane[1]*light[1] + plane[2]*light[2] + plane[3]*light[3];
    m[0][0] = dot - light[0]*plane[0];
    m[1][0] =     - light[0]*plane[1];
    m[2][0] =     - light[0]*plane[2];
    m[3][0] =     - light[0]*plane[3];
    m[0][1] =     - light[1]*plane[0];
    m[1][1] = dot - light[1]*plane[1];
    m[2][1] =     - light[1]*plane[2];
    m[3][1] =     - light[1]*plane[3];
    m[0][2] =     - light[2]*plane[0];
    m[1][2] =     - light[2]*plane[1];
    m[2][2] = dot - light[2]*plane[2];
    m[3][2] =     - light[2]*plane[3];
    m[0][3] =     - light[3]*plane[0];
    m[1][3] =     - light[3]*plane[1];
    m[2][3] =     - light[3]*plane[2];
    m[3][3] = dot - light[3]*plane[3];
    glMultMatrixf( &m[0][0] );
}


// draw a skate.
// isOnLeft = YES: draw on the left.
// isOnLeft = NO: draw on right
static void drawSkate(snow_configuration *bp, Bool isOnLeft)
{
    glPushMatrix();
    
    glCullFace(bp->cullingFaceBack);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    
    glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[skateCoordID]);
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bp->bufferID[skateIndicesID]);

    if (bp->isDrawingShadows) {
        glColor4fv(kColorShadow);
    } else {
        glColor4fv(kColorSkate);
    }
    if ( isOnLeft ) {
        glTranslatef( kSkateLateralDistance, kSkateHeight, kSkateVentralDistance );
    } else {
        glTranslatef( -kSkateLateralDistance, kSkateHeight, kSkateVentralDistance );
    }
    glScalef( kSkateScale, kSkateScale, kSkateScale );
    
    glDrawElements(GL_TRIANGLE_STRIP, 13, GL_UNSIGNED_INT, (void*) 0); // Right side
    glDrawElements(GL_TRIANGLE_STRIP, 13, GL_UNSIGNED_INT, (void*) (13 * sizeof(GLuint))); // Left side
    glDrawElements(GL_TRIANGLE_STRIP, 28, GL_UNSIGNED_INT, (void*) (26 * sizeof(GLuint))); // Center
    
    glPopMatrix();
}

static void drawArms(snow_configuration *bp)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glCullFace(bp->cullingFaceBack);

    glBindBuffer( GL_ARRAY_BUFFER,  bp->bufferID[armCoordID] );
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bp->bufferID[armIndicesID]);

    if (bp->isDrawingShadows) {
        glColor4fv(kColorShadow);
    } else {
        glColor4fv(kColorSnowmanArm);
    }

    for ( int i = 0; i < 2; ++i ) {
        glDrawElements(GL_TRIANGLE_STRIP, 18 * 5 + 4, GL_UNSIGNED_INT, (void*) 0);

        glDrawArrays(GL_TRIANGLE_FAN, 24, 8);
        glDrawArrays(GL_TRIANGLE_FAN, 32, 8);
        glDrawArrays(GL_TRIANGLE_FAN, 40, 8);

        glCullFace(bp->cullingFaceFront);
        glScalef( -1, 1, 1 );
    }
    
    if ( ! bp->isDrawingShadows) {
        glColor4fv(kColorInkOutline);
        glBindBuffer( GL_ARRAY_BUFFER,  bp->bufferID[armCoordOutlineID] );
        glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );

        for ( int i = 0; i < 2; ++i ) {
            glDrawElements(GL_TRIANGLE_STRIP, 18 * 5 + 4, GL_UNSIGNED_INT, (void*) 0);

            glDrawElements(GL_TRIANGLE_FAN, 8, GL_UNSIGNED_INT, (void*) ((18 * 5 + 4) * sizeof(GLuint)));
            glDrawArrays(GL_TRIANGLE_FAN, 24, 8);
            glDrawArrays(GL_TRIANGLE_FAN, 32, 8);
            glDrawArrays(GL_TRIANGLE_FAN, 40, 8);

            glCullFace(bp->cullingFaceBack);
            glScalef( -1, 1, 1 );
        }
    }
}

static void drawCarrot(snow_configuration *bp, GLfloat radius)
{
    glPushMatrix();
    
    glCullFace(bp->cullingFaceFront);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    
    glBindBuffer(GL_ARRAY_BUFFER, bp->bufferID[carrotCoordID]);
    glVertexPointer(3, GL_FLOAT, 0, (GLvoid*) 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bp->bufferID[carrotIndicesID]);

    glScalef( radius, radius, radius );
    glTranslatef( 0, 0, radius * kCarrotScaleDivide );

    if (bp->isDrawingShadows) {
        glColor4fv(kColorShadow);
    } else {
        glColor4fv(kColorInkOutline);
    }
    glDrawElements(GL_TRIANGLE_STRIP, 18, GL_UNSIGNED_INT, (void*) 0);
    glDrawElements(GL_TRIANGLE_FAN, 10, GL_UNSIGNED_INT, (void*) (18 * sizeof(GLuint)));
    glDrawElements(GL_TRIANGLE_FAN, 8, GL_UNSIGNED_INT, (void*)  (28 * sizeof(GLuint)));

    if ( ! bp->isDrawingShadows) {
        glColor4fv( kColorCarrot );
        glScalef( kCarrotOutlineInverseScale.x, kCarrotOutlineInverseScale.y, kCarrotOutlineInverseScale.z );
        glCullFace(bp->cullingFaceBack);
        
        glDrawElements(GL_TRIANGLE_STRIP, 18, GL_UNSIGNED_INT, (void*) 0);
        glDrawElements(GL_TRIANGLE_FAN, 10, GL_UNSIGNED_INT, (void*) (18 * sizeof(GLuint)));
    }

    glPopMatrix();
}

static void drawSnowball(snow_configuration *bp,
                  GLfloat radius,
                  GLfloat outlineRadius,
                  GLuint texture)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glEnable(GL_CULL_FACE);
    glCullFace(bp->cullingFaceBack);
    glPushMatrix();
    glScalef(radius, radius, radius);

    glBindBuffer(GL_ARRAY_BUFFER, bp->bufferID[snowballCoordID]);
    glVertexPointer(3, GL_FLOAT, 0, (GLvoid*) 0);

    glColor4fv(kColorShadow);
    if ( ! bp->isDrawingShadows) {
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, texture );
        glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[snowballTexID] );
        glTexCoordPointer( 2, GL_FLOAT, 0, (GLvoid*) 0 );

        glColor4fv( kColorSnow );
        glDrawArrays(GL_TRIANGLES, 0, bp->snowballAry.countOfVertices);
        glColor4fv(kColorInkOutline);
    }
    
    glDisable( GL_TEXTURE_2D );
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glScalef(outlineRadius / radius, outlineRadius / radius, outlineRadius / radius);
    glCullFace(bp->cullingFaceFront);
    glDrawArrays(GL_TRIANGLES, 0, bp->snowballAry.countOfVertices);

    glPopMatrix();
}

static void drawHat(snow_configuration *bp, Vert4d *hatColor, GLfloat radius)
{
    const GLfloat HAT_TRANSLATE_HEIGHT = 1.09;
    HatStruct_t *hatInfo  = &(bp->hatInfo);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    
    glCullFace(bp->cullingFaceBack);
    glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[hatCoordID] );
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bp->bufferID[hatIndicesID]);

    if (bp->isDrawingShadows) {
        glColor4fv(kColorShadow);
    } else {
        glColor4f( hatColor->x, hatColor->y, hatColor->z, hatColor->w );
    }

    glTranslatef( 0.0, radius * HAT_TRANSLATE_HEIGHT, 0.0 );
    glScalef( radius, radius, radius );
    
    glDrawElements(GL_TRIANGLE_FAN,
                   hatInfo->countOfBrimBottomTriangleFanIndices,
                   GL_UNSIGNED_INT,
                   hatInfo->brimBottomTriangleFanIndices);
    glDrawElements(GL_TRIANGLE_FAN,
                   hatInfo->countOfBrimTopTriangleFanIndices,
                   GL_UNSIGNED_INT,
                   hatInfo->brimTopTriangleFanIndices);
    glDrawElements(GL_TRIANGLE_STRIP,
                   hatInfo->countOfBrimTriangleStripIndices,
                   GL_UNSIGNED_INT,
                   hatInfo->brimTriangleStripIndices);
    glDrawElements(GL_TRIANGLE_FAN,
                   hatInfo->countOfStemTopTriangleFanIndices,
                   GL_UNSIGNED_INT,
                   hatInfo->stemTopTriangleFanIndices);
    glDrawElements(GL_TRIANGLE_STRIP,
                   hatInfo->countOfStemTriangleStripIndices,
                   GL_UNSIGNED_INT,
                   hatInfo->stemTriangleStripIndices);
    
    if ( ! bp->isDrawingShadows) {
        glColor4fv(kColorInkOutline);
        
        glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[hatOutlineCoordID] );
        glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );

        glCullFace(bp->cullingFaceFront);

        glDrawElements(GL_TRIANGLE_FAN,
                       hatInfo->countOfBrimBottomTriangleFanIndices,
                       GL_UNSIGNED_INT,
                       hatInfo->brimBottomTriangleFanIndices);
        glDrawElements(GL_TRIANGLE_FAN,
                       hatInfo->countOfBrimTopTriangleFanIndices,
                       GL_UNSIGNED_INT,
                       hatInfo->brimTopTriangleFanIndices);
        glDrawElements(GL_TRIANGLE_STRIP,
                       hatInfo->countOfBrimTriangleStripIndices,
                       GL_UNSIGNED_INT,
                       hatInfo->brimTriangleStripIndices);
        glDrawElements(GL_TRIANGLE_FAN,
                       hatInfo->countOfStemTopTriangleFanIndices,
                       GL_UNSIGNED_INT,
                       hatInfo->stemTopTriangleFanIndices);
        glDrawElements(GL_TRIANGLE_STRIP,
                       hatInfo->countOfStemTriangleStripIndices,
                       GL_UNSIGNED_INT,
                       hatInfo->stemTriangleStripIndices);
    }
}

static void drawIce(snow_configuration *bp)
{
    glColor4fv( kColorIce );
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDepthMask(GL_FALSE);
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, bp->textureID[kTextureIDIce] );
    glBindBuffer( GL_ARRAY_BUFFER,   bp->bufferID[pondTexID] );
    glTexCoordPointer( 2, GL_FLOAT, 0, (GLvoid*) 0 );
    glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[pondCoordID] );
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    glDrawArrays( GL_TRIANGLE_FAN, 0, bp->pondInfo.countOfVertices );
    glDepthMask(GL_TRUE);
}


static void drawShore(snow_configuration *bp)
{
    glColor4fv( kColorSnow );
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glCullFace(bp->cullingFaceBack);
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, bp->textureID[kTextureIDShore] );
    glBindBuffer( GL_ARRAY_BUFFER,   bp->bufferID[shoreTexID] );
    glTexCoordPointer( 2, GL_FLOAT, 0, (GLvoid*) 0 );
    glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[shoreCoordID] );
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, bp->shoreInfo.countOfVertices );
}


static void drawHills(snow_configuration *bp)
{
    glColor4fv( kColorSnow );
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glCullFace(bp->cullingFaceBack);
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, bp->textureID[kTextureIDHills] );
    glBindBuffer( GL_ARRAY_BUFFER,   bp->bufferID[hillsTexID] );
    glTexCoordPointer( 2, GL_FLOAT, 0, (GLvoid*) 0 );
    glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[hillsCoordID] );
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, bp->hillsInfo.countOfVertices );
}


static void drawTree(snow_configuration *bp, TreeState_t *state)
{
    GLfloat radius = state->bottomSkirtRadius;
    Vert3d location = state->location;
    GLfloat height = state->height;
    GLfloat rotation = state->rotation;

    glEnableClientState(GL_VERTEX_ARRAY);

    if ( ! bp->isDrawingShadows) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, bp->textureID[kTextureIDTrees]);
        glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[treesTexID]);
        glTexCoordPointer( 2, GL_FLOAT, 0, (GLvoid*) 0 );
        glColor4f(1, 1, 1, 1);
    } else {
        glColor4fv(kColorShadow);
    }

    glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[treesCoordID]);
    glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
    
    glPushMatrix();

    glTranslatef( location.x, location.y, location.z );
    glScalef( radius, height, radius );
    glRotatef( rotation, 0, 1, 0 );
    
    glDisable(GL_CULL_FACE);
    for (int i = 0; i < kCountOfTreeSkirts; ++i) {
        glDrawArrays( GL_TRIANGLE_FAN,
                     i * (kCountOfTreeSkirtVertices + 2),
                     kCountOfTreeSkirtVertices + 2 );
    }
    glEnable(GL_CULL_FACE);

    // The tree skirt shadows cover the tree trunk shadows.
    // So don't bother rendering the tree trunk shadows
    if ( ! bp->isDrawingShadows) {
        
        // draw the tree trunk
        glDisable( GL_TEXTURE_2D );
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        
        glColor4fv( kColorTreeTrunk );
        
        glDrawArrays( GL_TRIANGLE_STRIP,
                     kCountOfTreeSkirts * (kCountOfTreeSkirtVertices + 2),
                     (kCountOfTreeTrunkSlices + 1) * 2);
        
        // Tree skirts outline
        glColor4fv(kColorInkOutline);
        glCullFace(bp->cullingFaceFront);
        
        glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[treesOutlineCoordID]);
        glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
        
        for (int i = 0; i < kCountOfTreeSkirts; ++i) {
            glDrawArrays( GL_TRIANGLE_FAN,
                         i * (kCountOfTreeSkirtVertices + 2),
                         kCountOfTreeSkirtVertices + 2 );
        }
        
        // tree trunk outline
        glBindBuffer( GL_ARRAY_BUFFER, bp->bufferID[treesCoordID]);
        glVertexPointer( 3, GL_FLOAT, 0, (GLvoid*) 0 );
        glScalef(1.3, 1.1, 1.3);
        glDrawArrays( GL_TRIANGLE_STRIP,
                     kCountOfTreeSkirts * (kCountOfTreeSkirtVertices + 2),
                     (kCountOfTreeTrunkSlices + 1) * 2);
    }

    glPopMatrix();
    
}


static void drawSnowman(snow_configuration *bp, SnowmanState_t *state)
{
    glPushMatrix();
    GLfloat radius = kSnowballRadiusStart;
    
    glTranslatef(state->positionX, 0, state->positionY);
    glRotatef(state->direction, 0, 1, 0);
    
    drawSkate(bp, True);
    drawSkate(bp, False);
    
    glRotatef(state->tilt, 0, 0, 1);
    
    // Base
    glTranslatef( 0,  kSnowballOverlap,  0.0 ); // move up so that the base touches the ground
    glRotatef(state->baseSnowballRotation, 0, 1, 0 ); // rotate the base snowball to vary the markings a little
    drawSnowball(bp,
                 radius,
                 radius + kSnowballOutlineRadiusIncrement,
                 bp->textureID[kTextureIDSnowmanBase]);
    glRotated( 0 - state->baseSnowballRotation, 0, 1, 0 ); // restore rotation
    
    // Torso
    radius *= kSnowballSizeRelativeToPrevious;
    glTranslatef( 0.0, radius + kSnowballSizeRelativeToPrevious / kSnowballOverlap, 0.0 );
    drawSnowball(bp,
                 radius,
                 radius + kSnowballOutlineRadiusIncrement,
                 bp->textureID[kTextureIDSnowmanTorso]);
    
    drawArms(bp);
    
    // Head
    radius *= kSnowballSizeRelativeToPrevious;
    glTranslatef( 0, radius + kSnowballSizeRelativeToPrevious / kSnowballOverlap, 0.0 );
    drawSnowball(bp,
                 radius,
                 radius + kSnowballOutlineRadiusIncrement,
                 bp->textureID[kTextureIDSnowmanHead]);
    
    drawCarrot(bp, radius);
    
    // draw a tophat
    drawHat(bp, &(state->hatColor), radius);
    
    glPopMatrix();
    
}


static void drawTrees(snow_configuration *bp)
{
    for (int i = 0; i < kCountOfTrees; ++i) {
        drawTree(bp, bp->treeIndividual + i);
    }
}


static void drawSnowmen(snow_configuration *bp)
{
    for (int i = 0; i < kCountOfSnowmen; ++i) {
        drawSnowman(bp, bp->snowmanIndividual + i);
    }
}


ENTRYPOINT void
draw_snow (ModeInfo *mi)
{
    snow_configuration *bp = &bps[MI_SCREEN(mi)];
    Display *dpy = MI_DISPLAY(mi);
    Window window = MI_WINDOW(mi);
    
    if (!bp->glx_context)
        return;

    // Update camera and snowmen
    bp->cameraRho += kDCameraRho;
    if (bp->cameraRho > kTau) bp->cameraRho -= kTau;
    bp->snowmanRho += kDSnowmanRho;
    if (bp->snowmanRho > kTau) bp->snowmanRho -= kTau;
    for (int i = 0; i < kCountOfSnowmen; ++i) {
        snowmanUpdateState(bp->snowmanIndividual + i,
                           bp->snowmanRho);
    }
    
    // Draw
    glXMakeCurrent(MI_DISPLAY(mi), MI_WINDOW(mi), *bp->glx_context);
    
    glShadeModel(GL_FLAT);
    glClearColor( kColorSky.x, kColorSky.y, kColorSky.z, kColorSky.w );
    glClearStencil(1);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    gluLookAt(kCameraRadius * sin(0 - bp->cameraRho), kCameraHeight, kCameraRadius * cos(bp->cameraRho),
              0, 0, 0,
              0, 1, 0);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bp->cullingFaceFront = GL_FRONT;
    bp->cullingFaceBack = GL_BACK;
    bp->isDrawingShadows = False;

    drawIce(bp);

    // Draw the reflections on the pond surface
    bp->cullingFaceFront = GL_BACK;
    bp->cullingFaceBack = GL_FRONT;
    glScalef(1, -1, 1); // Inverse for reflection

    drawShore(bp);
    drawTrees(bp);
    drawSnowmen(bp);

    glScalef(1, -1, 1); // Reorient upward
    bp->cullingFaceFront = GL_FRONT;
    bp->cullingFaceBack = GL_BACK;

    // Draw the ice again. I know it seems redundant, but this looks better to me when doing reflections.
    drawIce(bp);
    drawShore(bp);

    // Draw shadows
    bp->isDrawingShadows = True;
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_ALWAYS);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc( GL_EQUAL, 1, 1 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );

    glPushMatrix();
    setShadowMatrix(False);
    drawSnowmen(bp);
    glPopMatrix();

    glPushMatrix();
    setShadowMatrix(True);
    drawTrees(bp);
    glPopMatrix();

    glDisable(GL_STENCIL_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // Draw everything that is real
    bp->isDrawingShadows = False;

    drawHills(bp);
    drawTrees(bp);
    drawSnowmen(bp);

    glXSwapBuffers(dpy, window);
}


ENTRYPOINT void
free_snow (ModeInfo *mi)
{
    snow_configuration *bp = &bps[MI_SCREEN(mi)];
    if (!bp->glx_context) return;
    glXMakeCurrent(MI_DISPLAY(mi), MI_WINDOW(mi), *bp->glx_context);
    
    free(bp->snowballAry.vertAry);
    free(bp->snowballAry.texAry);
    
    free(bp->pondInfo.vertAry);
    free(bp->pondInfo.texAry);
    
    free(bp->shoreInfo.vertAry);
    free(bp->shoreInfo.texAry);
    
    free(bp->hillsInfo.vertAry);
    free(bp->hillsInfo.texAry);
    
    free(bp->hatInfo.vertAry);
    free(bp->hatInfo.outlineVertAry);
    free(bp->hatInfo.indexAry);
    
    free(bp->treesInfo.vertAry);
    free(bp->treesInfo.outlineVertAry);
    free(bp->treesInfo.texAry);
    
    glDeleteBuffers(countOfVboIDs, bp->bufferID);
    glDeleteTextures(kCountOfTextureIDs, bp->textureID);
}

XSCREENSAVER_MODULE_2 ("Snowmen", snowmen, snow)

#endif /* USE_GL */

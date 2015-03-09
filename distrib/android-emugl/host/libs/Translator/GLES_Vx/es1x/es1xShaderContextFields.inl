/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 
GLboolean						drawingPoints;
GLboolean						texturingEnabled;
es1xTextureFormat				textureFormat[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
GLboolean						lightingEnabled;
GLboolean						colorMaterialEnabled;
GLboolean						fogEnabled;
es1xFogMode						fogMode;
GLboolean						normalizeEnabled;
GLboolean						alphaTestEnabled;
es1xCompareFunc					alphaTestFunc;
GLboolean						textureUnitEnabled[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvMode					texEnvMode[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xLightType					lightType[ES1X_NUM_SUPPORTED_LIGHTS];
GLboolean						lightEnabled[ES1X_NUM_SUPPORTED_LIGHTS];
GLboolean						rescaleNormalEnabled;
GLboolean						vertexArrayEnabled;
GLboolean						normalArrayEnabled;
GLboolean						colorArrayEnabled;
GLboolean						textureCoordArrayEnabled[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvCombineRGB			texEnvCombineRGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvCombineAlpha			texEnvCombineAlpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvSrc					texEnvSrc0RGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvSrc					texEnvSrc1RGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvSrc					texEnvSrc2RGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvSrc					texEnvSrc0Alpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvSrc					texEnvSrc1Alpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvSrc					texEnvSrc2Alpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvOperandRGB			texEnvOperand0RGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvOperandRGB			texEnvOperand1RGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvOperandRGB			texEnvOperand2RGB[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvOperandAlpha			texEnvOperand0Alpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvOperandAlpha			texEnvOperand1Alpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
es1xTexEnvOperandAlpha			texEnvOperand2Alpha[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
GLboolean						texEnvCoordReplaceOES[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
GLboolean						pointSizeArrayOESEnabled;

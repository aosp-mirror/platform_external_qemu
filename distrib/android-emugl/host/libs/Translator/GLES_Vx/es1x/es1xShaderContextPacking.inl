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

 
ES1X_ASSERT((context->drawingPoints & ~((1 << 1) - 1)) == 0);					*dst	|= (context->drawingPoints << 0);								/*		32 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texturingEnabled & ~((1 << 1) - 1)) == 0);				*dst	|= (context->texturingEnabled << 1);							/*		31 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightingEnabled & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightingEnabled << 2);							/*		30 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->colorMaterialEnabled & ~((1 << 1) - 1)) == 0);			*dst	|= (context->colorMaterialEnabled << 3);						/*		29 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->fogEnabled & ~((1 << 1) - 1)) == 0);						*dst	|= (context->fogEnabled << 4);								/*		28 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->fogMode & ~((1 << 2) - 1)) == 0);							*dst	|= (context->fogMode << 5);									/*		27 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->normalizeEnabled & ~((1 << 1) - 1)) == 0);				*dst	|= (context->normalizeEnabled << 7);							/*		25 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->alphaTestEnabled & ~((1 << 1) - 1)) == 0);				*dst	|= (context->alphaTestEnabled << 8);							/*		24 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->alphaTestFunc & ~((1 << 3) - 1)) == 0);					*dst	|= (context->alphaTestFunc << 9);								/*		23 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->rescaleNormalEnabled & ~((1 << 1) - 1)) == 0);			*dst	|= (context->rescaleNormalEnabled << 12);						/*		20 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->vertexArrayEnabled & ~((1 << 1) - 1)) == 0);				*dst	|= (context->vertexArrayEnabled << 13);						/*		19 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->normalArrayEnabled & ~((1 << 1) - 1)) == 0);				*dst	|= (context->normalArrayEnabled << 14);						/*		18 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->colorArrayEnabled & ~((1 << 1) - 1)) == 0);				*dst	|= (context->colorArrayEnabled << 15);							/*		17 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->pointSizeArrayOESEnabled & ~((1 << 1) - 1)) == 0);		*dst	|= (context->pointSizeArrayOESEnabled << 16);					/*		16 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->textureFormat[0] & ~((1 << 3) - 1)) == 0);				*dst	|= (context->textureFormat[0] << 17);							/*		15 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->textureFormat[1] & ~((1 << 3) - 1)) == 0);				*dst	|= (context->textureFormat[1] << 20);							/*		12 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->textureUnitEnabled[0] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->textureUnitEnabled[0] << 23);						/*		9 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->textureUnitEnabled[1] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->textureUnitEnabled[1] << 24);						/*		8 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvMode[0] & ~((1 << 3) - 1)) == 0);					*dst	|= (context->texEnvMode[0] << 25);								/*		7 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->texEnvMode[1] & ~((1 << 3) - 1)) == 0);					*dst	|= (context->texEnvMode[1] << 28);								/*		4 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->textureCoordArrayEnabled[0] & ~((1 << 1) - 1)) == 0);		*dst	|= (context->textureCoordArrayEnabled[0] << 31); dst++;		/*		1 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->textureCoordArrayEnabled[1] & ~((1 << 1) - 1)) == 0);		*dst	|= (context->textureCoordArrayEnabled[1] << 0);				/*		32 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvCombineRGB[0] & ~((1 << 3) - 1)) == 0);				*dst	|= (context->texEnvCombineRGB[0] << 1);						/*		31 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->texEnvCombineRGB[1] & ~((1 << 3) - 1)) == 0);				*dst	|= (context->texEnvCombineRGB[1] << 4);						/*		28 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->texEnvCombineAlpha[0] & ~((1 << 3) - 1)) == 0);			*dst	|= (context->texEnvCombineAlpha[0] << 7);						/*		25 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->texEnvCombineAlpha[1] & ~((1 << 3) - 1)) == 0);			*dst	|= (context->texEnvCombineAlpha[1] << 10);						/*		22 bit(s) available, 3 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc0RGB[0] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc0RGB[0] << 13);							/*		19 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc0RGB[1] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc0RGB[1] << 15);							/*		17 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc1RGB[0] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc1RGB[0] << 17);							/*		15 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc1RGB[1] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc1RGB[1] << 19);							/*		13 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc2RGB[0] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc2RGB[0] << 21);							/*		11 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc2RGB[1] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc2RGB[1] << 23);							/*		9 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc0Alpha[0] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc0Alpha[0] << 25);						/*		7 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc0Alpha[1] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc0Alpha[1] << 27);						/*		5 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc1Alpha[0] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc1Alpha[0] << 29);						/*		3 bit(s) available, 2 bit(s) added	*/
																				*dst	|= ((context->texEnvSrc1Alpha[1] & 0x00000001) << 31); dst++;	/*		1 bit(s) available, 1 bit(s) added */
ES1X_ASSERT((context->texEnvSrc1Alpha[1] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc1Alpha[1] >> 1);						/*		32 bit(s) available, 1 bit(s) added */
ES1X_ASSERT((context->texEnvSrc2Alpha[0] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc2Alpha[0] << 1);						/*		31 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvSrc2Alpha[1] & ~((1 << 2) - 1)) == 0);				*dst	|= (context->texEnvSrc2Alpha[1] << 3);						/*		29 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand0RGB[0] & ~((1 << 2) - 1)) == 0);			*dst	|= (context->texEnvOperand0RGB[0] << 5);						/*		27 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand0RGB[1] & ~((1 << 2) - 1)) == 0);			*dst	|= (context->texEnvOperand0RGB[1] << 7);						/*		25 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand1RGB[0] & ~((1 << 2) - 1)) == 0);			*dst	|= (context->texEnvOperand1RGB[0] << 9);						/*		23 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand1RGB[1] & ~((1 << 2) - 1)) == 0);			*dst	|= (context->texEnvOperand1RGB[1] << 11);						/*		21 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand2RGB[0] & ~((1 << 2) - 1)) == 0);			*dst	|= (context->texEnvOperand2RGB[0] << 13);						/*		19 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand2RGB[1] & ~((1 << 2) - 1)) == 0);			*dst	|= (context->texEnvOperand2RGB[1] << 15);						/*		17 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand0Alpha[0] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->texEnvOperand0Alpha[0] << 17);					/*		15 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand0Alpha[1] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->texEnvOperand0Alpha[1] << 18);					/*		14 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand1Alpha[0] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->texEnvOperand1Alpha[0] << 19);					/*		13 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand1Alpha[1] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->texEnvOperand1Alpha[1] << 20);					/*		12 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand2Alpha[0] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->texEnvOperand2Alpha[0] << 21);					/*		11 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvOperand2Alpha[1] & ~((1 << 1) - 1)) == 0);			*dst	|= (context->texEnvOperand2Alpha[1] << 22);					/*		10 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvCoordReplaceOES[0] & ~((1 << 1) - 1)) == 0);		*dst	|= (context->texEnvCoordReplaceOES[0] << 23);					/*		9 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->texEnvCoordReplaceOES[1] & ~((1 << 1) - 1)) == 0);		*dst	|= (context->texEnvCoordReplaceOES[1] << 24);					/*		8 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightType[0] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[0] << 25);								/*		7 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->lightType[1] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[1] << 27);								/*		5 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->lightType[2] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[2] << 29);								/*		3 bit(s) available, 2 bit(s) added	*/
																				*dst	|= ((context->lightType[3] & 0x00000001) << 31); dst++;			/*		1 bit(s) available, 1 bit(s) added */
ES1X_ASSERT((context->lightType[3] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[3] >> 1);								/*		32 bit(s) available, 1 bit(s) added */
ES1X_ASSERT((context->lightType[4] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[4] << 1);								/*		31 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->lightType[5] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[5] << 3);								/*		29 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->lightType[6] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[6] << 5);								/*		27 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->lightType[7] & ~((1 << 2) - 1)) == 0);					*dst	|= (context->lightType[7] << 7);								/*		25 bit(s) available, 2 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[0] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[0] << 9);							/*		23 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[1] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[1] << 10);							/*		22 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[2] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[2] << 11);							/*		21 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[3] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[3] << 12);							/*		20 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[4] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[4] << 13);							/*		19 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[5] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[5] << 14);							/*		18 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[6] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[6] << 15);							/*		17 bit(s) available, 1 bit(s) added	*/
ES1X_ASSERT((context->lightEnabled[7] & ~((1 << 1) - 1)) == 0);					*dst	|= (context->lightEnabled[7] << 16);							/*		16 bit(s) available, 1 bit(s) added	*/

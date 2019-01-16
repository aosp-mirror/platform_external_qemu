// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

// Table converting ASCII to GL_LINES coordinates

#define STROKES_PER_GLYPH 5

struct Stroke {
    float x0;
    float y0;
    float x1;
    float y1;
};

struct VectorGlyph {
    Stroke strokes[STROKES_PER_GLYPH];
};

static constexpr VectorGlyph kNotSupportedGlyph = {
    {
        { -0.5f, -0.5f, 0.5f, 0.5f },
        { -0.5f, -0.5f, 0.5f, -0.5f },
        { -0.5f, -0.5f, -0.5f, 0.5f },
        { 0.5f, -0.5f, 0.5f, 0.5f },
        { -0.5f, 0.5f, 0.5f, 0.5f },
    },
};

static constexpr VectorGlyph kSpaceGlyph = {
    { { }, { }, { }, { }, { }, },
};

static constexpr VectorGlyph kPercentGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, -0.8f },
        { -0.8f, 0.8f, -0.6f, 0.6f },
        { 0.8f, -0.8f, 0.6f, -0.6f },
        {}, {},
    },
};

static constexpr VectorGlyph kPlusGlyph = {
    {
        { 0.0f, 0.8f, 0.0f, -0.8f },
        { 0.8f, 0.0f, -0.8f, 0.0f },
        {}, {}, {},
    },
};

static constexpr VectorGlyph kPeriodGlyph = {
    {
        { -0.2f, -0.8f, 0.2f, -0.8f },
        { 0.2f, -0.8f, 0.2f, -0.6f },
        { 0.2f, -0.6f, -0.2f, -0.6f },
        { -0.2f, -0.6f, -0.2f, -0.8f },
        {},
    },
};

static constexpr VectorGlyph kSlashGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, -0.8f },
        {}, {}, {}, {},
    },
};

static constexpr VectorGlyph k0Glyph = {
    {
        { -0.8f, -0.8f, 0.8f, -0.8f },
        { 0.8f, -0.8f, 0.8f, 0.8f },
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f },
        { -0.8f, -0.8f, 0.8f, 0.8f },
    },
};

static constexpr VectorGlyph k1Glyph = {
    {
        { 0.0f, -0.8f, 0.0f, 0.8f },
        { 0.0f, 0.8f, -0.6f, 0.3f },
        { }, { }, { },
    },
};

static constexpr VectorGlyph k2Glyph = {
    {
        { -0.8f, 0.3f, -0.3f, 0.8f, },
        { -0.3f, 0.8f, 0.3f, 0.8f, },
        { 0.3f, 0.8f, 0.8f, 0.3f, },
        { 0.8f, 0.3f, -0.8f, -0.8f },
        { -0.8f, -0.8f, 0.8f, -0.8f },
    },
};

static constexpr VectorGlyph k3Glyph = {
    {
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, 0.8f, 0.0f, },
        { 0.8f, 0.0f, -0.7f, 0.0f },
        { 0.8f, 0.0f, 0.8f, 0.8f },
        { 0.8f, 0.8f, -0.8f, 0.8f },
    },
};

static constexpr VectorGlyph k4Glyph = {
    {
        { 0.0f, -0.8f, 0.0f, 0.2f, },
        { -0.8f, -0.1f, 0.8f, -0.1f, },
        { -0.8f, -0.1f, 0.0f, 0.8f, },
        { }, { },
    },
};

static constexpr VectorGlyph k5Glyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, -0.8f, 0.0f, },
        { -0.8f, 0.0f, 0.8f, 0.0f, },
        { 0.8f, 0.0f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, -0.8f, -0.8f, },
    },
};

static constexpr VectorGlyph k6Glyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, 0.8f, 0.0f, },
        { 0.8f, 0.0f, -0.8f, 0.0f, },
    },
};

static constexpr VectorGlyph k7Glyph = {
    {
        { -0.8f, 0.8f, 0.8f, 0.8f, },
        { 0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.6f, 0.0f, 0.6f, 0.0f, },
        { }, { },
    },
};

static constexpr VectorGlyph k8Glyph = {
    {
        { -0.8f, -0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, 0.8f, 0.8f, },
        { 0.8f, 0.8f, 0.8f, -0.8f, },
        { -0.8f, 0.0f, 0.8f, 0.0f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
    },
};

static constexpr VectorGlyph k9Glyph = {
    {
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, 0.8f, 0.8f, },
        { 0.8f, 0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, -0.8f, 0.0f, },
        { -0.8f, 0.0f, 0.8f, 0.0f, },
    },
};

static constexpr VectorGlyph kAGlyph = {
    {
        { -0.8f, -0.8f, 0.0f, 0.8f, },
        { 0.0f, 0.8f, 0.8f, -0.8f, },
        { -0.6f, 0.0f, 0.6f, 0.0f, },
        {}, {},
    },
};

static constexpr VectorGlyph kBGlyph = {
    {
        { -0.8f, 0.8f, 0.8f, 0.4f, },
        { 0.8f, 0.4f, -0.8f, 0.0f, },
        { -0.8f, 0.0f, 0.8f, -0.4f, },
        { 0.8f, -0.4f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, -0.8f, 0.8f, },
    },
};

static constexpr VectorGlyph kCGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        {}, {},
    },
};

static constexpr VectorGlyph kDGlyph = {
    {
        { -0.8f, 0.8f, 0.8f, 0.4f, },
        { 0.8f, 0.4f, 0.8f, -0.4f, },
        { 0.8f, -0.4f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, -0.8f, 0.8f, },
        {},
    },
};

static constexpr VectorGlyph kEGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        { -0.8f, 0.0f, 0.4f, 0.0f, },
        {},
    },
};

static constexpr VectorGlyph kFGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, 0.0f, 0.4f, 0.0f, },
        {}, {},
    },
};

static constexpr VectorGlyph kGGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, 0.8f, 0.0f, },
        { 0.8f, 0.0f, 0.0f, 0.0f, },
    },
};

static constexpr VectorGlyph kHGlyph = {
    {
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { 0.8f, 0.8f, 0.8f, -0.8f, },
        { -0.8f, 0.0f, 0.8f, 0.0f, },
        {}, {},
    },
};

static constexpr VectorGlyph kIGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { 0.8f, -0.8f, -0.8f, -0.8f },
        { 0.0f, 0.8f, 0.0f, -0.8f },
        {}, {},
    },
};

static constexpr VectorGlyph kJGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { 0.4f, 0.8f, 0.4f, -0.4f },
        { 0.4f, -0.4f, -0.4f, -0.8f },
        {}, {},
    },
};

static constexpr VectorGlyph kKGlyph = {
    {
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, 0.0f, 0.8f, 0.8f, },
        { -0.4f, 0.2f, 0.6f, -0.8f, },
        {}, {},
    },
};

static constexpr VectorGlyph kLGlyph = {
    {
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        {}, {}, {},
    },
};

static constexpr VectorGlyph kMGlyph = {
    {
        { -0.8f, -0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, 0.0f, 0.0f, },
        { 0.0f, 0.0f, 0.8f, 0.8f, },
        { 0.8f, 0.8f, 0.8f, -0.8f, },
        {},
    },
};

static constexpr VectorGlyph kNGlyph = {
    {
        { -0.8f, -0.8f, -0.8f, 0.8f, },
        { -0.8f, 0.8f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, 0.8f, 0.8f, },
        {}, {},
    },
};

static constexpr VectorGlyph kOGlyph = {
    {
        { -0.8f, -0.8f, 0.8f, -0.8f },
        { 0.8f, -0.8f, 0.8f, 0.8f },
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f },
        {},
    },
};

static constexpr VectorGlyph kPGlyph = {
    {
        { -0.8f, 0.0f, 0.8f, 0.0f },
        { 0.8f, 0.0f, 0.8f, 0.8f },
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f },
        {},
    },
};

static constexpr VectorGlyph kQGlyph = {
    {
        { -0.8f, -0.8f, 0.8f, -0.8f },
        { 0.8f, -0.8f, 0.8f, 0.8f },
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f },
        { 0.4f, -0.4f, 0.8f, -0.8f },
    },
};

static constexpr VectorGlyph kRGlyph = {
    {
        { -0.8f, 0.0f, 0.8f, 0.0f },
        { 0.8f, 0.0f, 0.8f, 0.8f },
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f },
        { -0.8f, 0.0f, 0.8f, -0.8f },
    },
};

static constexpr VectorGlyph kSGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.4f },
        { -0.8f, 0.4f, 0.8f, -0.4f },
        { 0.8f, -0.4f, -0.8f, -0.8f },
        {}, {},
    },
};

static constexpr VectorGlyph kTGlyph = {
    {
        { 0.8f, 0.8f, -0.8f, 0.8f },
        { 0.0f, 0.8f, 0.0f, -0.8f },
        {}, {}, {},
    },
};

static constexpr VectorGlyph kUGlyph = {
    {
        { -0.8f, -0.8f, 0.8f, -0.8f },
        { 0.8f, -0.8f, 0.8f, 0.8f },
        { -0.8f, 0.8f, -0.8f, -0.8f },
        {}, {},
    },
};

static constexpr VectorGlyph kVGlyph = {
    {
        { -0.8f, 0.8f, 0.0f, -0.8f, },
        { 0.0f, -0.8f, 0.8f, 0.8f, },
        {}, {}, {},
    },
};

static constexpr VectorGlyph kWGlyph = {
    {
        { -0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.0f, 0.0f, },
        { 0.0f, 0.0f, 0.8f, -0.8f, },
        { 0.8f, -0.8f, 0.8f, 0.8f, },
        {},
    },
};

static constexpr VectorGlyph kXGlyph = {
    {
        { -0.8f, -0.8f, 0.8f, 0.8f, },
        { -0.8f, 0.8f, 0.8f, -0.8f, },
        {}, {}, {},
    },
};

static constexpr VectorGlyph kYGlyph = {
    {
        { -0.8f, 0.8f, 0.0f, 0.0f, },
        { 0.0f, 0.0f, 0.8f, 0.8f, },
        { 0.0f, 0.0f, 0.0f, -0.8f, },
        {}, {},
    },
};

static constexpr VectorGlyph kZGlyph = {
    {
        { -0.8f, 0.8f, 0.8f, 0.8f, },
        { 0.8f, 0.8f, -0.8f, -0.8f, },
        { -0.8f, -0.8f, 0.8f, -0.8f, },
        {}, {},
    },
};

static constexpr VectorGlyph kVectorFontGlyphs[255] = {
// 000
    kNotSupportedGlyph,
// 001
    kNotSupportedGlyph,
// 002
    kNotSupportedGlyph,
// 003
    kNotSupportedGlyph,
// 004
    kNotSupportedGlyph,
// 005
    kNotSupportedGlyph,
// 006
    kNotSupportedGlyph,
// 007
    kNotSupportedGlyph,
// 008
    kNotSupportedGlyph,
// 009
    kNotSupportedGlyph,
// 010
    kNotSupportedGlyph,
// 011
    kNotSupportedGlyph,
// 012
    kNotSupportedGlyph,
// 013
    kNotSupportedGlyph,
// 014
    kNotSupportedGlyph,
// 015
    kNotSupportedGlyph,
// 016
    kNotSupportedGlyph,
// 017
    kNotSupportedGlyph,
// 018
    kNotSupportedGlyph,
// 019
    kNotSupportedGlyph,
// 020
    kNotSupportedGlyph,
// 021
    kNotSupportedGlyph,
// 022
    kNotSupportedGlyph,
// 023
    kNotSupportedGlyph,
// 024
    kNotSupportedGlyph,
// 025
    kNotSupportedGlyph,
// 026
    kNotSupportedGlyph,
// 027
    kNotSupportedGlyph,
// 028
    kNotSupportedGlyph,
// 029
    kNotSupportedGlyph,
// 030
    kNotSupportedGlyph,
// 031
    kNotSupportedGlyph,
// 032
    kSpaceGlyph,
// 033 !
    kNotSupportedGlyph,
// 034 "
    kNotSupportedGlyph,
// 035 #
    kNotSupportedGlyph,
// 036 $
    kNotSupportedGlyph,
// 037 %
    kPercentGlyph,
// 038 &
    kNotSupportedGlyph,
// 039 '
    kNotSupportedGlyph,
// 040 (
    kNotSupportedGlyph,
// 041 )
    kNotSupportedGlyph,
// 042 *
    kNotSupportedGlyph,
// 043 +
    kPlusGlyph,
// 044 ,
    kNotSupportedGlyph,
// 045 -
    kNotSupportedGlyph,
// 046 .
    kPeriodGlyph,
// 047 /
    kSlashGlyph,
// 048: 0
    k0Glyph,
// 049: 1
    k1Glyph,
// 050: 2
    k2Glyph,
// 051: 3
    k3Glyph,
// 052: 4
    k4Glyph,
// 053: 5
    k5Glyph,
// 054: 6
    k6Glyph,
// 055: 7
    k7Glyph,
// 056: 8
    k8Glyph,
// 057: 9
    k9Glyph,
// 058
    kNotSupportedGlyph,
// 059
    kNotSupportedGlyph,
// 060
    kNotSupportedGlyph,
// 061
    kNotSupportedGlyph,
// 062
    kNotSupportedGlyph,
// 063
    kNotSupportedGlyph,
// 064
    kNotSupportedGlyph,
// 065: A
    kAGlyph,
// 066: B
    kBGlyph,
// 067: C
    kCGlyph,
// 068: D
    kDGlyph,
// 069: E
    kEGlyph,
// 070: F
    kFGlyph,
// 071: G
    kGGlyph,
// 072: H
    kHGlyph,
// 073: I
    kIGlyph,
// 074: J
    kJGlyph,
// 075: K
    kKGlyph,
// 076: L
    kLGlyph,
// 077: M
    kMGlyph,
// 078: N
    kNGlyph,
// 079: O
    kOGlyph,
// 080: P
    kPGlyph,
// 081: Q
    kQGlyph,
// 082: R
    kRGlyph,
// 083: S
    kSGlyph,
// 084: T
    kTGlyph,
// 085: U
    kUGlyph,
// 086: V
    kVGlyph,
// 087: W
    kWGlyph,
// 088: X
    kXGlyph,
// 089: Y
    kYGlyph,
// 090: Z
    kZGlyph,
// 091
// 092
// 093
// 094
// 095
// 096
// 097
// 098
// 099
// 100
// 101
// 102
// 103
// 104
// 105
// 106
// 107
// 108
// 109
// 110
// 111
// 112
// 113
// 114
// 115
// 116
// 117
// 118
// 119
// 120
// 121
// 122
// 123
// 124
// 125
// 126
// 127
// 128
// 129
// 130
// 131
// 132
// 133
// 134
// 135
// 136
// 137
// 138
// 139
// 140
// 141
// 142
// 143
// 144
// 145
// 146
// 147
// 148
// 149
// 150
// 151
// 152
// 153
// 154
// 155
// 156
// 157
// 158
// 159
// 160
// 161
// 162
// 163
// 164
// 165
// 166
// 167
// 168
// 169
// 170
// 171
// 172
// 173
// 174
// 175
// 176
// 177
// 178
// 179
// 180
// 181
// 182
// 183
// 184
// 185
// 186
// 187
// 188
// 189
// 190
// 191
// 192
// 193
// 194
// 195
// 196
// 197
// 198
// 199
// 200
// 201
// 202
// 203
// 204
// 205
// 206
// 207
// 208
// 209
// 210
// 211
// 212
// 213
// 214
// 215
// 216
// 217
// 218
// 219
// 220
// 221
// 222
// 223
// 224
// 225
// 226
// 227
// 228
// 229
// 230
// 231
// 232
// 233
// 234
// 235
// 236
// 237
// 238
// 239
// 240
// 241
// 242
// 243
// 244
// 245
// 246
// 247
// 248
// 249
// 250
// 251
// 252
// 253
// 254
// 255
};

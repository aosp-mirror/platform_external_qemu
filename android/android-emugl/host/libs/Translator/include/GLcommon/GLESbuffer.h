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
#ifndef GLES_BUFFER_H
#define GLES_BUFFER_H

#include <aemu/base/files/Stream.h>
#include <stdio.h>
#include <GLES/gl.h>
#include <GLcommon/ObjectData.h>
#include <GLcommon/RangeManip.h>

class GLESbuffer: public ObjectData {
public:
   GLESbuffer():ObjectData(BUFFER_DATA) {}
   GLESbuffer(android::base::Stream* stream);
   void onSave(android::base::Stream* stream,
               unsigned int globalName) const override;
   void restore(ObjectLocalName localName,
                const getGlobalName_t& getGlobalName) override;
   GLuint getSize(){return m_size;};
   GLuint getUsage(){return m_usage;};
   GLvoid* getData(){ return m_data;}
   bool  setBuffer(GLuint size,GLuint usage,const GLvoid* data);
   bool  setSubBuffer(GLuint offset,GLuint size,const GLvoid* data);
   void  getConversions(const RangeList& rIn,RangeList& rOut);
   bool  fullyConverted(){return m_conversionManager.size() == 0;};
   void  setBinded(){m_wasBound = true;};
   bool  wasBinded(){return m_wasBound;};
   ~GLESbuffer();

private:
    GLuint         m_size = 0;
    GLuint         m_usage = GL_STATIC_DRAW;
    unsigned char* m_data = nullptr;
    RangeList      m_conversionManager;
    bool           m_wasBound = false;
};

#endif

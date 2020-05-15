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
#ifndef __VAR__H__
#define __VAR__H__

#include "VarType.h"
#include <string>
#include <stdio.h>

class Var {
public:
    // pointer data direction - from the client point of view.
    typedef enum { POINTER_OUT = 0x1, POINTER_IN = 0x2, POINTER_INOUT = 0x3 } PointerDir;
    Var() = default;

    Var(const std::string & name,
        const VarType * vartype,
        const std::string & lenExpression,
        PointerDir dir,
        const std::string &packExpression,
        const std::string &unpackExpression,
        const std::string &writeExpression) :
        m_name(name),
        m_type(const_cast<VarType *>(vartype)),
        m_lenExpression(lenExpression),
        m_pointerDir(dir),
        m_packExpression(packExpression),
        m_unpackExpression(unpackExpression),
        m_host_packTmpAllocExpression(""),
        m_host_packExpression(""),
        m_guest_unpackExpression(""),
        m_guest_packExpression(""),
        m_writeExpression(writeExpression)
    {
    }

    void init(const std::string name, const VarType * vartype,
              std::string lenExpression,
              PointerDir dir,
              std::string packExpression,
              std::string unpackExpression,
              std::string writeExpression) {
        m_name = name;
        m_type = vartype;
        m_lenExpression = lenExpression;
        m_packExpression = packExpression;
        m_unpackExpression = unpackExpression;
        m_host_packTmpAllocExpression = "";
        m_host_packExpression = "";
        m_guest_unpackExpression = "";
        m_guest_packExpression = "";
        m_writeExpression = writeExpression;
        m_pointerDir = dir;
        m_nullAllowed = false;
        m_isLarge = false;

    }

    const std::string & name() const { return m_name; }
    const VarType * type() const { return m_type; }
    bool isPointer() const { return m_type->isPointer(); }
    bool isVoid() const { return ((m_type->bytes() == 0) && (!m_type->isPointer())); }
    const std::string & lenExpression() const { return m_lenExpression; }
    const std::string & packExpression() const { return(m_packExpression); }
    const std::string & unpackExpression() const { return(m_unpackExpression); }
    const std::string & hostPackTmpAllocExpression() const { return(m_host_packTmpAllocExpression); }
    const std::string & hostPackExpression() const { return(m_host_packExpression); }
    const std::string & guestUnpackExpression() const { return(m_guest_unpackExpression); }
    const std::string & guestPackExpression() const { return(m_guest_packExpression); }
    const std::string & writeExpression() const { return(m_writeExpression); }
    const std::string & paramCheckExpression() const { return m_paramCheckExpression; }
    void setLenExpression(const std::string & lenExpression) { m_lenExpression = lenExpression; }
    void setPackExpression(const std::string & packExpression) { m_packExpression = packExpression; }
    void setUnpackExpression(const std::string & unpackExpression) { m_unpackExpression = unpackExpression; }
    void setHostPackTmpAllocExpression(const std::string & expr) { m_host_packTmpAllocExpression = expr; }
    void setHostPackExpression(const std::string & expr) { m_host_packExpression = expr; }
    void setGuestUnpackExpression(const std::string & expr) { m_guest_unpackExpression = expr; }
    void setGuestPackExpression(const std::string & expr) { m_guest_packExpression = expr; }
    void setWriteExpression(const std::string & writeExpression) { m_writeExpression = writeExpression; }
    void setParamCheckExpression(const std::string & paramCheckExpression) { m_paramCheckExpression = paramCheckExpression; }
    void setPointerDir(PointerDir dir) { m_pointerDir = dir; }
    PointerDir pointerDir() { return m_pointerDir; }
    void setNullAllowed(bool state) { m_nullAllowed = state; }
    void setIsLarge(bool state) { m_isLarge = state; }
    void setDMA(bool state) { m_isDMA = state; }
    bool nullAllowed() const { return m_nullAllowed; }
    bool isLarge() const { return m_isLarge; }
    bool isDMA() const { return m_isDMA; }
    void printType(FILE *fp) { fprintf(fp, "%s", m_type->name().c_str()); }
    void printTypeName(FILE *fp) { printType(fp); fprintf(fp, " %s", m_name.c_str()); }

private:
    std::string m_name;
    const VarType * m_type = nullptr;
    std::string m_lenExpression; // an expression to calcualte a pointer data size
    PointerDir m_pointerDir = POINTER_IN;
    bool m_nullAllowed = false;
    bool m_isLarge = false;
    bool m_isDMA = false;
    std::string m_packExpression; // an expression to pack data into the stream
    std::string m_unpackExpression; // an expression to unpack data that has arrived from the stream
    std::string m_host_packTmpAllocExpression; // an expression to create temporaries for getting into packed form for readbacks
    std::string m_host_packExpression; // an expression to pack data into the stream but for readbacks
    std::string m_guest_unpackExpression; // an expression to unpack readbacks on the guest
    std::string m_guest_packExpression; // an expression to pack large buffer writes on the guest
    std::string m_writeExpression; // an expression to write data into the stream
    std::string m_paramCheckExpression; //an expression to check parameter value
};

#endif

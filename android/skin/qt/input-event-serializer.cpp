/* Copyright (C) 2016 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

/* Copyright 2011 Stanislaw Adaszewski. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY STANISLAW ADASZEWSKI ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Stanislaw Adaszewski. */

#include "input-event-serializer.h"

/******************************************************************************/

QDataStream& operator<<(QDataStream &ds, const QMouseEvent &ev)
{
	ds
		<< (qint32) EventClass::MouseEvent
		<< (qint32) ev.type()
		<< ev.pos()
		<< ev.globalPos()
		<< (qint32) ev.button()
		<< (qint32) ev.buttons()
		<< (qint32) ev.modifiers();
	return ds;
}

QDataStream& operator>>(QDataStream &ds, QMouseEvent* &ev)
{
	qint32 type;
	QPoint pos, globalPos;
	qint32 button, buttons, modifiers;
	ds >> type >> pos >> globalPos >> button >> buttons >> modifiers;
        ev = new QMouseEvent((QEvent::Type)type, pos, globalPos,
                             (Qt::MouseButton)button, (Qt::MouseButtons)buttons,
                             (Qt::KeyboardModifiers)modifiers);
        return ds;
}

/******************************************************************************/

QDataStream& operator<<(QDataStream &ds, const QKeyEvent &ev)
{
	ds
		<< (qint32) EventClass::KeyEvent
		<< (qint32) ev.type()
		<< (qint32) ev.key()
		<< (qint32) ev.modifiers()
		<< ev.text()
		<< ev.isAutoRepeat()
		<< (qint32) ev.count();
	return ds;
}

QDataStream& operator>>(QDataStream &ds, QKeyEvent* &ev)
{
	qint32 type, key, modifiers;
	QString text;
	bool autorep;
	qint32 count;
	ds >> type >> key >> modifiers >> text >> autorep >> count;
        ev = new QKeyEvent((QEvent::Type)type, key,
                           (Qt::KeyboardModifiers)modifiers, text, autorep,
                           count);
        return ds;
}

/******************************************************************************/

QDataStream& operator<<(QDataStream &ds, const QInputEvent *ev)
{
    if (dynamic_cast<const QKeyEvent*>(ev))
		ds << *static_cast<const QKeyEvent*>(ev);
	else if (dynamic_cast<const QMouseEvent*>(ev))
		ds << *static_cast<const QMouseEvent*>(ev);

	return ds;
}

QDataStream& operator>>(QDataStream &ds, QInputEvent*& ev)
{
	qint32 cls;
	ds >> cls;

	switch(cls)
	{
        case EventClass::MouseEvent:
		{
			QMouseEvent *tmp;
			ds >> tmp;
			ev = tmp;
		}
		break;
        case EventClass::KeyEvent:
		{
			QKeyEvent *tmp;
			ds >> tmp;
			ev = tmp;
		}
		break;
    }

	return ds;
}

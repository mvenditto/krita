/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.g
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *

 The X11-specific event handling code is based upon code from 
 src/kernel/qapplication_x11.cpp from the Qt GUI Toolkit and is subject
 to the following license and copyright:

 ****************************************************************************
** 
**
** Implementation of X11 startup routines and event handling
**
** Created : 931029
**
** Copyright (C) 1992-2003 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "kis_canvas.h"
#include "kis_cursor.h"
#include "kis_move_event.h"
#include "kis_button_press_event.h"
#include "kis_button_release_event.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>

bool KisCanvas::X11SupportInitialised = false;
long KisCanvas::X11AltMask = 0;
long KisCanvas::X11MetaMask = 0;

#endif // Q_WS_X11

KisCanvas::KisCanvas(QWidget *parent, const char *name) : super(parent, name)
{
	setBackgroundMode(QWidget::NoBackground);
	setMouseTracking(true);
	m_enableMoveEventCompressionHint = true;
	m_lastPressure = 0;

#ifdef Q_WS_X11
	if (!X11SupportInitialised) {
		initX11Support();
	}

	m_lastRootX = -1;
	m_lastRootY = -1;
#endif
}

KisCanvas::~KisCanvas()
{
}

void KisCanvas::showScrollBars()
{
	Q_INT32 w = width();
	Q_INT32 h = height();

	resize(w - 1, h - 1);
	resize(w, h);
}

void KisCanvas::paintEvent(QPaintEvent *e)
{
	emit gotPaintEvent(e);
}

void KisCanvas::mousePressEvent(QMouseEvent *e)
{
	KisButtonPressEvent ke(INPUT_DEVICE_MOUSE, e -> pos(), e -> globalPos(), PRESSURE_DEFAULT, 0, 0, e -> button(), e -> state());
	buttonPressEvent(&ke);
}

void KisCanvas::mouseReleaseEvent(QMouseEvent *e)
{
	KisButtonReleaseEvent ke(INPUT_DEVICE_MOUSE, e -> pos(), e -> globalPos(), PRESSURE_DEFAULT, 0, 0, e -> button(), e -> state());
	buttonReleaseEvent(&ke);
}

void KisCanvas::mouseMoveEvent(QMouseEvent *e)
{
	KisMoveEvent ke(INPUT_DEVICE_MOUSE, e -> pos(), e -> globalPos(), PRESSURE_DEFAULT, 0, 0, e -> state());
	moveEvent(&ke);
}

void KisCanvas::tabletEvent(QTabletEvent *e)
{
	enumInputDevice device;

	switch (e -> device()) {
	default:
	case QTabletEvent::NoDevice:
	case QTabletEvent::Stylus:
		device = INPUT_DEVICE_STYLUS;
		break;
	case QTabletEvent::Puck:
		device = INPUT_DEVICE_PUCK;
		break;
	case QTabletEvent::Eraser:
		device = INPUT_DEVICE_ERASER;
		break;
	}

	double pressure = e -> pressure() / 255.0;
#if 1
	// Use pressure threshold to detect 'left button' press/release
	if (pressure >= PRESSURE_THRESHOLD && m_lastPressure < PRESSURE_THRESHOLD) {
		KisButtonPressEvent ke(device, e -> pos(), e -> globalPos(), pressure, e -> xTilt(), e -> yTilt(), Qt::LeftButton, Qt::NoButton);
		buttonPressEvent(&ke);
	} else if (pressure < PRESSURE_THRESHOLD && m_lastPressure >= PRESSURE_THRESHOLD) {
		KisButtonReleaseEvent ke(device, e -> pos(), e -> globalPos(), pressure, e -> xTilt(), e -> yTilt(), Qt::LeftButton, Qt::NoButton);
		buttonReleaseEvent(&ke);
	}
#else
	// Note: Qt doesn't provide button or state for tablet events.

	if (e -> type() == QEvent::TabletPress) {
		KisButtonPressEvent ke(device, e -> pos(), e -> globalPos(), e -> pressure() / 255.0, e -> xTilt(), e -> yTilt(), Qt::LeftButton, Qt::NoButton);
		buttonPressEvent(&ke);
	}
	else
	if (e -> type() == QEvent::TabletRelease) {
		KisButtonReleaseEvent ke(device, e -> pos(), e -> globalPos(), e -> pressure() / 255.0, e -> xTilt(), e -> yTilt(), Qt::LeftButton, Qt::NoButton);
		buttonReleaseEvent(&ke);
	}
#endif
	else {
		KisMoveEvent ke(device, e -> pos(), e -> globalPos(), pressure, e -> xTilt(), e -> yTilt(), Qt::NoButton);
		moveEvent(&ke);
#ifdef Q_WS_X11
		// Fix the problem that when you change from using a tablet device to the mouse,
		// the first mouse button event is not recognised. This is because we handle 
		// X11 core mouse move events directly so Qt does not get to see them. This breaks
		// the tablet event accept/ignore mechanism, causing Qt to consume the first
		// mouse button event it sees, instead of a mouse move. 'Ignoring' tablet move events
		// stops Qt from stealing the next mouse button event. This does not affect the 
		// tablet aware tools as they do not care about mouse moves while the tablet device is
		// drawing.
		if (e -> type() == QEvent::TabletMove) {
			e -> ignore();
		}
#endif
	}

	m_lastPressure = pressure;
}
  
void KisCanvas::enterEvent(QEvent *e)
{
	emit gotEnterEvent(e);
}

void KisCanvas::leaveEvent(QEvent *e)
{
	emit gotLeaveEvent(e);
}

void KisCanvas::wheelEvent(QWheelEvent *e)
{
	emit mouseWheelEvent(e);
}

void KisCanvas::keyPressEvent(QKeyEvent *e)
{
	emit gotKeyPressEvent(e);
}

void KisCanvas::keyReleaseEvent(QKeyEvent *e)
{
	emit gotKeyReleaseEvent(e);
}

void KisCanvas::moveEvent(KisMoveEvent *e)
{
	emit gotMoveEvent(e);
}

void KisCanvas::buttonPressEvent(KisButtonPressEvent *e)
{
	emit gotButtonPressEvent(e);
}

void KisCanvas::buttonReleaseEvent(KisButtonReleaseEvent *e)
{
	emit gotButtonReleaseEvent(e);
}

#ifdef Q_WS_X11

void KisCanvas::initX11Support()
{
	Q_ASSERT(!X11SupportInitialised);
	X11SupportInitialised = true;

	// Look at the modifier mapping and get the correct masks for alt/meta
	XModifierKeymap *map = XGetModifierMapping(x11Display());

	if (map) {
		int mapIndex = 0;

		for (int maskIndex = 0; maskIndex < 8; maskIndex++) {
			for (int i = 0; i < map -> max_keypermod; i++) {
				if (map -> modifiermap[mapIndex]) {

					KeySym sym = XKeycodeToKeysym(x11Display(), map -> modifiermap[mapIndex], 0);

					if (X11AltMask == 0 && (sym == XK_Alt_L || sym == XK_Alt_R)) {
						X11AltMask = 1 << maskIndex;
					}
					if (X11MetaMask == 0 && (sym == XK_Meta_L || sym == XK_Meta_R)) {
						X11MetaMask = 1 << maskIndex;
					}
				}

				mapIndex++;
			}
		}

		XFreeModifiermap(map);
	}
	else {
		// Assume defaults
		X11AltMask = Mod1Mask;
		X11MetaMask = Mod4Mask;
	}
}

int KisCanvas::translateX11ButtonState(int state)
{
	int buttonState = 0;

	if (state & Button1Mask)
		buttonState |= Qt::LeftButton;
	if (state & Button2Mask)
		buttonState |= Qt::MidButton;
	if (state & Button3Mask)
		buttonState |= Qt::RightButton;
	if (state & ShiftMask)
		buttonState |= Qt::ShiftButton;
	if (state & ControlMask)
		buttonState |= Qt::ControlButton;
	if (state & X11AltMask)
		buttonState |= Qt::AltButton;
	if (state & X11MetaMask)
		buttonState |= Qt::MetaButton;

	return buttonState;
}

bool KisCanvas::x11Event(XEvent *event)
{
	if (event -> type == MotionNotify) {
		// Mouse move
		if (!m_enableMoveEventCompressionHint) {

			XMotionEvent motion = event -> xmotion;
			QPoint globalPos(motion.x_root, motion.y_root);

			if (globalPos.x() != m_lastRootX || globalPos.y() != m_lastRootY) {

				int state = translateX11ButtonState(motion.state);
				QPoint pos(motion.x, motion.y);
				QMouseEvent e(QEvent::MouseMove, pos, globalPos, Qt::NoButton, state);

				mouseMoveEvent(&e);
			}

			m_lastRootX = globalPos.x();
			m_lastRootY = globalPos.y();

			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

#endif // Q_WS_X11

#include "kis_canvas.moc"


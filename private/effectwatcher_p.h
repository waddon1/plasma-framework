/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef BLURWATCHER_H
#define BLURWATCHER_H

#include <QWidget>

#include <plasma/windoweffects.h>

namespace Plasma
{

class EffectWatcher: public QWidget
{
    Q_OBJECT

public:
    EffectWatcher(QString property, QWidget *parent = 0);

protected:
#ifdef Q_WS_X11
    bool x11Event(XEvent *event);
#endif

Q_SIGNALS:
    void blurBehindChanged(bool blur);

private:
    QString m_property;
    bool m_effectActive;
};

}

#endif 
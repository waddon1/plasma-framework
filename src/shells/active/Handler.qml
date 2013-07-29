/*   vim:set foldmethod=marker:
 *
 *   Copyright (C) 2013 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2
import org.kde.plasma.shells.active 0.1 as Active
import org.kde.solidx 0.1 as SolidX
import QtQuick.Window 2.0

Item {
    id: main
    state: "Active"

    property bool loaded   : false
    property bool willing  : solidx.touchscreenPresent
    property int  priority : 0

    signal updated()

    onWillingChanged:  {
        console.log("This is the new status - is active shell willing? " + willing)
        main.updated()
    }

    onPriorityChanged: main.updated()

    onLoadedChanged:
        if (loaded) {
            handler.load()
        } else {
            handler.unload()
        }

    Active.HandlerObject {
        id: handler
    }

    SolidX.Interface {
        id: solidx
    }

    Window {
        id: activeDialog

        visible: main.loaded

        width: 500
        height: 500

        Rectangle {
            anchors.fill: parent
            color: "white"
        }

        Text {
            anchors.fill: parent
            font.pointSize: 32
            text: "Active"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter
        }
    }
}


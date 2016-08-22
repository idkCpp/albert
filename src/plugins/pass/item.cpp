// albert - a simple application launcher for linux
// Copyright (C) 2016 Martin Bürgmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "item.h"
#include <QProcess>

/** ***************************************************************************/
Pass::Item::Item(QString title) : title_(title){
}



/** ***************************************************************************/
Pass::Item::~Item(){
}



/** ***************************************************************************/
QString Pass::Item::text() const {
    return title_;
}



/** ***************************************************************************/
QString Pass::Item::subtext() const {
    return "Copy this password";
}



/** ***************************************************************************/
QString Pass::Item::iconPath() const {
    // Icon of item
    return QString();
}



/** ***************************************************************************/
void Pass::Item::activate(ExecutionFlags *) {
    QProcess::startDetached("pass", {"-c", title_});
}

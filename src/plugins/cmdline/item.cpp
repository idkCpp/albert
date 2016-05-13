// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
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

QString cmdline::Item::iconPath_;

/** ***************************************************************************/
cmdline::Item::Item(const QString &title, const QString &text, const QString &cmd) : title_(title), text_(text), cmd_(cmd)
{

}



/** ***************************************************************************/
cmdline::Item::~Item(){

}



/** ***************************************************************************/
QString cmdline::Item::text() const {
    return title_;
}



/** ***************************************************************************/
QString cmdline::Item::subtext() const {
    return text_;
}



/** ***************************************************************************/
QString cmdline::Item::iconPath() const {
    return iconPath_;
}



/** ***************************************************************************/
void cmdline::Item::activate(ExecutionFlags *) {
    QProcess::startDetached(cmd_);
}



/** ***************************************************************************/
bool cmdline::Item::hasChildren() const {
    // Performance measure.
    return false;
}



/** ***************************************************************************/
vector<shared_ptr<AlbertItem>> cmdline::Item::children() {
    // Return the children.
    // Did not want to have children? Subclass A2leaf instead.
    return vector<shared_ptr<AlbertItem>>();
}


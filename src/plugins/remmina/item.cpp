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
#include <QSettings>
#include <QProcess>


/** ***************************************************************************/
Remmina::Item::Item(QString configFilePath){
    configFile_ = configFilePath;
    QSettings set(configFilePath, QSettings::IniFormat);
    set.beginGroup("remmina");
    name_ = set.value("name").toString();
}



/** ***************************************************************************/
Remmina::Item::~Item(){

}



/** ***************************************************************************/
Remmina::Item *Remmina::Item::copy()
{
    return new Item(configFile_);
}



/** ***************************************************************************/
QString Remmina::Item::text() const {
    return name_;
}



/** ***************************************************************************/
QString Remmina::Item::subtext() const {
    return "Connect with remmina";
}



/** ***************************************************************************/
QString Remmina::Item::iconPath() const {
    // Icon of item
    return QString();
}



/** ***************************************************************************/
void Remmina::Item::activate(ExecutionFlags *) {
    // Do sth cool...
    //qDebug("Connecting to %s", name_.toStdString().c_str());
    QProcess::startDetached("remmina", {"-c", configFile_});
}

// albert - a simple application launcher for linux
// Copyright (C) 2014-2016 Manuel Schneider
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

#include "query.h"
#include "query_p.hpp"

/** ***************************************************************************/
Query::Query(const QString &term, const QString &originalTerm) {
    impl = new QueryPrivate(term, originalTerm);
}



/** ***************************************************************************/
Query::~Query() {
    delete impl;
}



/** ***************************************************************************/
void Query::addMatch(shared_ptr<AlbertItem> item, short score) {
    impl->addMatch(item, score);
}



/** ***************************************************************************/
void Query::reset() {
    impl->reset();
}



/** ***************************************************************************/
void Query::setValid(bool b) {
    impl->setValid(b);
}



/** ***************************************************************************/
bool Query::isValid() {
    return impl->isValid();
}



/** ***************************************************************************/
const QString &Query::searchTerm() const {
    return impl->searchTerm();
}



/** ***************************************************************************/
const QString &Query::originalSearchTerm() const {
    return impl->originalSearchTerm();
}

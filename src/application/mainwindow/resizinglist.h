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

#pragma once
#include <QListView>

class ResizingList : public QListView
{
    Q_OBJECT
    Q_PROPERTY(int maxItems READ maxItems WRITE setMaxItems MEMBER maxItems_ NOTIFY maxItemsChanged)

public:

    ResizingList(QWidget *parent = 0) : QListView(parent), maxItems_(5) {}
    virtual ~ResizingList() {}

    uint8_t maxItems() const;
    void setMaxItems(uint8_t maxItems);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    void reset() override;

private:

    uint8_t maxItems_;

signals:

    void maxItemsChanged();

};

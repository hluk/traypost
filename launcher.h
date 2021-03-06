/*
    Copyright (c) 2013, Lukas Holecek <hluk@email.cz>

    This file is part of TrayPost.

    TrayPost is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    TrayPost is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TrayPost.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>

class QThread;

namespace traypost {

class Tray;
class ConsoleReader;

class Launcher : public QObject
{
    Q_OBJECT
public:
    Launcher();

    ~Launcher();

public slots:
    void start();

private:
    Tray *tray_;
    ConsoleReader *reader_;
    QThread *readerThread_;
};

} // namespace traypost

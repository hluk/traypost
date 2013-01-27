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

#include <QMainWindow>

#include <memory>

namespace traypost {

class TrayPrivate;

class Tray : public QObject
{
    Q_OBJECT

public:
    explicit Tray(QObject *parent = nullptr);

    /**
     * Set tray tooltip.
     */
    void setToolTip(const QString &text);

    /**
     * Set tray icon.
     */
    void setIcon(const QIcon &icon);

    /**
     * Set tray icon text.
     */
    void setIconText(const QString &text);

    /**
     * Set icon text font, text color and text outline color.
     */
    void setIconTextStyle(const QFont &font, const QColor &color, const QColor &outlineColor);

    /**
     * Set format of time.
     */
    void setTimeFormat(const QString &format);

    /**
     * Set format of each message/record.
     */
    void setMessageFormat(const QString &format);

    /**
     * Show tray icon.
     */
    void show();

public slots:
    void onInputLine(const QString &line);

    void onInputEnd();

    void exit();

    void resetMessages();

    void showLog();

signals:
    void readLine();

private:
    TrayPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(Tray)
};

} // namespace traypost

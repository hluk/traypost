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

#include "tray.h"
#include "log_dialog.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QTimer>

#include <iostream>

#if QT_VERSION < 0x050000
#   include <QTextDocument> // Qt::escape()
#endif

#include <stdio.h>

namespace traypost {

namespace {

QString escapeHtml(const QString &str)
{
#if QT_VERSION < 0x050000
    return Qt::escape(str);
#else
    return str.toHtmlEscaped();
#endif
}

} // namespace

constexpr int maxMessageLines = 10;

class TrayPrivate : public QObject {
    Q_OBJECT
public:
    TrayPrivate(Tray *parent)
        : QObject(parent)
        , q_ptr(parent)
        , lines_(0)
        , inputRead_(false)
        , endOfInput_(false)
        , timeout_(8000)
    {
        tray_.setToolTip( tr("No messages available.") );
        createMenu();
        connectSignals();
    }

    void createMenu()
    {
        Q_Q(Tray);

        menu_.clear();

        // Reset
        actionReset_ = menu_.addAction( QIcon::fromTheme("edit-clear"), tr("&Reset"),
                                        q, SLOT(resetMessages()) );
        actionReset_->setVisible(true);

        actionShowLog_ = menu_.addAction( QIcon::fromTheme("document-open"), tr("&Show Log"),
                                          q, SLOT(showLog()) );
        actionShowLog_->setVisible(true);

        // Exit
        menu_.addAction( QIcon::fromTheme("application-exit"), tr("E&xit"),
                         q, SLOT(exit()) );

        tray_.setContextMenu(&menu_);
    }

    void connectSignals()
    {
        connect(&tray_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(onTrayActivated(QSystemTrayIcon::ActivationReason)));

        timerMessage_.setInterval(1000);
        timerMessage_.setSingleShot(true);
        connect( &timerMessage_, SIGNAL(timeout()), SLOT(showMessage()) );
    }

    void show()
    {
        tray_.show();
        setIconText(iconText_);
    }

    void setIcon(const QIcon &icon)
    {
        icon_ = icon;
        if ( tray_.isVisible() )
            setIconText(iconText_);
        else
            tray_.setIcon(icon_);
    }

    void setIconText(const QString &text)
    {
        iconText_ = text;

        if ( !tray_.isVisible() )
            return;

        QIcon icon;
        auto sizes = icon_.availableSizes();

        Q_ASSERT( !sizes.empty() );


        // Current tray icon size.
        auto currentSize = tray_.geometry().size();
        if ( !sizes.contains(currentSize) ) {
            sizes.push_back(currentSize);
            QSize fromSize = sizes.first();
            for (const QSize &size : sizes) {
                if ( size.width() >= currentSize.width() && size.height() >= currentSize.height() ) {
                    fromSize = size;
                    break;
                }
            }

            icon.addPixmap( icon_.pixmap(fromSize).scaled(currentSize) );
        }

        // Render text in icon pixmaps.
        for (const QSize &size : sizes) {
            QPixmap pix( icon_.pixmap(size) );
            if ( pix.isNull())
                continue;
            QPainter p(&pix);

            // text position
            QFontMetrics fm(iconTextFont_);
            auto textSize = fm.size(Qt::TextSingleLine, text);
            const int x = ( size.width() - textSize.width() ) / 2;
            const int y = ( size.height() + textSize.height() ) / 2 - fm.descent();

            // Draw text outline.
            QPainterPath path;
            path.addText(x, y, iconTextFont_, text);
            p.setPen(QPen(iconTextOutlineColor_, 2));
            p.setBrush(iconTextOutlineColor_);
            p.drawPath(path);

            // Draw text.
            p.setFont(iconTextFont_);
            p.setPen(iconTextColor_);
            p.drawText(x, y, text);

            icon.addPixmap(pix);
        }

        tray_.setIcon(icon);

        bool showReset = !iconText_.isEmpty();

        actionReset_->setVisible(showReset);
        menu_.setDefaultAction(showReset ? actionReset_ : actionShowLog_);
    }

    void setIconTextStyle(const QFont &font, const QColor &color, const QColor &outlineColor)
    {
        iconTextFont_ = font;
        iconTextColor_ = color;
        iconTextOutlineColor_ = outlineColor;
        setIconText(iconText_);
    }

    void resetMessages()
    {
        lines_ = 0;
        setIconText( QString() );
        tray_.setToolTip( QString() );
    }

    void showLog()
    {
        if (dialogLog_ != nullptr) {
            dialogLog_->show();
            dialogLog_->activateWindow();
            dialogLog_->raise();
            dialogLog_->setFocus();
            return;
        }

        dialogLog_ = new LogDialog(records_);
        dialogLog_->setWindowIcon(icon_);
        dialogLog_->resize(480, 480);
        dialogLog_->show();

        connect( dialogLog_, SIGNAL(finished(int)), dialogLog_, SLOT(deleteLater()) );
        connect( dialogLog_, SIGNAL(itemActivated(int)), this, SLOT(onItemActivated(int)) );
    }

    void onInputLine(const QString &line)
    {
        inputRead_ = true;
        setToolTip(line);
    }

    void onInputEnd()
    {
        if (inputRead_)
            setToolTip( tr("-- END OF INPUT --"), true );
    }

public slots:
    void setToolTip(const QString &text, bool endOfInput = false)
    {
        if (endOfInput_)
            return;

        endOfInput_ = endOfInput;

        auto record = recordFormat_
                .arg( QDateTime::currentDateTime().toString(timeFormat_) )
                .arg( endOfInput ? QString("<b><u>%1</u></b>").arg(text) : escapeHtml(text) );
        records_.append(record);

        QString msg = records_.size() > maxMessageLines ? QString("<p>...</p>") : QString();
        for (int i = qMax(0, records_.size() - maxMessageLines); i < records_.size(); ++i)
            msg.append(records_[i]);
        tray_.setToolTip(msg);

        // TODO: Set message after an interval.
        message_ = text;
        timerMessage_.start();

        setIconText( QString::number(++lines_) );

        if (dialogLog_ != nullptr)
            dialogLog_->addRecord(record);
    }

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason)
    {
        Q_Q(Tray);
        if (reason == QSystemTrayIcon::DoubleClick) {
            showLog();
        } else if (reason == QSystemTrayIcon::Trigger) {
            auto act = menu_.defaultAction();
            if (act != nullptr)
                act->trigger();
            if ( dialogLog_ != nullptr && dialogLog_->hasFocus() )
                dialogLog_->hide();
        } else if (reason == QSystemTrayIcon::MiddleClick) {
            q->exit();
        }
    }

    void onItemActivated(int row)
    {
        if ( (row + (endOfInput_ ? 1 : 0)) < records_.size() )
            std::cout << records_[row].toStdString() << std::endl;
    }

    void showMessage()
    {
        tray_.showMessage(QString("TrayPost"), message_, QSystemTrayIcon::NoIcon, timeout_);
        message_.clear();
    }

protected:
    Tray * const q_ptr;
    Q_DECLARE_PUBLIC(Tray)

    QSystemTrayIcon tray_;
    QMenu menu_;
    QPointer<QAction> actionReset_;
    QPointer<QAction> actionShowLog_;
    QPointer<LogDialog> dialogLog_;
    QIcon icon_;

    QString iconText_;
    QFont iconTextFont_;
    QColor iconTextColor_;
    QColor iconTextOutlineColor_;

    int lines_;

    QStringList records_;

    bool inputRead_;

    QString timeFormat_;
    QString recordFormat_;

    bool endOfInput_;

    int timeout_;
    QString message_;
    QTimer timerMessage_;
};

Tray::Tray(QObject *parent)
    : QObject(parent)
    , d_ptr(new TrayPrivate(this))
{
}

void Tray::setToolTip(const QString &text)
{
    Q_D(Tray);
    d->setToolTip(text);
}

void Tray::setMessageTimeout(int ms)
{
    Q_D(Tray);
    d->timeout_ = ms;
}

void Tray::setIcon(const QIcon &icon)
{
    Q_D(Tray);
    d->setIcon(icon);
}

void Tray::setIconText(const QString &text)
{
    Q_D(Tray);
    d->setIconText(text);
}

void Tray::setIconTextStyle(const QFont &font, const QColor &color, const QColor &outlineColor)
{
    Q_D(Tray);
    d->setIconTextStyle(font, color, outlineColor);
}

void Tray::setTimeFormat(const QString &format)
{
    Q_D(Tray);
    d->timeFormat_ = format;
}

void Tray::setMessageFormat(const QString &format)
{
    Q_D(Tray);
    d->recordFormat_ = format;
}

void Tray::show()
{
    Q_D(Tray);
    d->show();
}

void Tray::onInputLine(const QString &line)
{
    Q_D(Tray);
    d->onInputLine(line);
    emit readLine();
}

void Tray::onInputEnd()
{
    Q_D(Tray);
    d->onInputEnd();
}

void Tray::exit()
{
    QApplication::quit();
}

void Tray::resetMessages()
{
    Q_D(Tray);
    d->resetMessages();
}

void Tray::showLog()
{
    Q_D(Tray);
    d->showLog();
}

} // namespace traypost

#include "tray.moc"

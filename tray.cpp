/*
    Copyright (c) 2013, Lukas Holecek <hluk@email.cz>

    This file is part of TrayPost.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tray.h"

#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QSocketNotifier>
#include <QSystemTrayIcon>
#include <QTextEdit>

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

const QString timeFormat("dd.MM.yyyy hh:mm:ss.zzz");
const QString lineFormat("<p><b>%1</b>: %2</p>");
constexpr int stdinBatchSize = 256;

class TrayPrivate : QObject {
    Q_OBJECT
public:
    TrayPrivate(Tray *parent)
        : QObject(parent)
        , q_ptr(parent)
        , socketNotifier_( new QSocketNotifier(0, QSocketNotifier::Read) )
        , lines_(0)
    {
        createMenu();
        connectSignals();

        connect(socketNotifier_, SIGNAL(activated(int)), this, SLOT(readLine()));
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
        bool showLog = !records_.isEmpty();

        actionReset_->setVisible(showReset);
        actionShowLog_->setVisible(showLog);
        menu_.setDefaultAction(showReset ? actionReset_ : showLog ? actionShowLog_ : nullptr);
    }

    void setIconTextStyle(const QFont &font, const QColor &color, const QColor &outlineColor)
    {
        iconTextFont_ = font;
        iconTextColor_ = color;
        iconTextOutlineColor_ = outlineColor;
        setIconText(iconText_);
    }

    void setToolTip(const QString &text)
    {
        auto record = lineFormat
                .arg( QDateTime::currentDateTime().toString(timeFormat) )
                .arg( escapeHtml(text) );
        records_.append(record);

        tray_.setToolTip( tray_.toolTip() + record);
        setIconText( QString::number(++lines_) );

        tray_.showMessage(QString("TrayPost"), text);
    }

    void resetMessages()
    {
        lines_ = 0;
        setIconText( QString() );
        tray_.setToolTip( QString() );
    }

    void showLog()
    {
        if ( records_.isEmpty() )
            return;

        auto dialog = new QDialog();
        dialog->setWindowTitle( tr("TrayPost - Log") );
        dialog->setWindowIcon(icon_);

        auto layout = new QVBoxLayout(dialog);

        auto recordCount = records_.split('\n').size();
        auto label = new QLabel( tr("Number of records: %1").arg(recordCount), dialog );
        layout->addWidget(label);

        auto edit = new QTextEdit(dialog);
        edit->setHtml(records_);
        edit->setReadOnly(true);
        layout->addWidget(edit);

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
        layout->addWidget(buttons);

        connect( dialog, SIGNAL(finished(int)), dialog, SLOT(deleteLater()) );
        connect( buttons, SIGNAL(accepted()), dialog, SLOT(accept()) );

        dialog->adjustSize();
        dialog->show();
    }

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason)
    {
        Q_Q(Tray);
        if (reason == QSystemTrayIcon::DoubleClick) {
            showLog();
        } else if (reason == QSystemTrayIcon::Trigger) {
            auto act = menu_.defaultAction();
            if (act != nullptr)
                act->trigger();
        } else if (reason == QSystemTrayIcon::MiddleClick) {
            q->exit();
        }
    }

    void readLine()
    {
        if (socketNotifier_ == nullptr)
            return;

        static char buffer[BUFSIZ];
        static QByteArray line;
        static struct timeval stdin_tv = {0,0};
        fd_set stdin_fds;

        /* disable stdin buffering (otherwise select waits on new line) */
        setbuf(stdin, NULL);

        /*
         * interrupt after reading at most N lines and
         * resume after processing pending events in event loop
         */
        for( int i = 0; i < stdinBatchSize; ++i ) {
            /* set stdin */
            FD_ZERO(&stdin_fds);
            FD_SET(0, &stdin_fds);

            /* check if data available */
            if ( select(1, &stdin_fds, NULL, NULL, &stdin_tv) <= 0 )
                break;

            /* read data */
            if ( fgets(buffer, BUFSIZ, stdin) ) {
                line.append(buffer);
                /* each line is one item */
                if ( line.endsWith('\n') ) {
                    line.resize( line.size()-1 );
                    setToolTip( QString::fromLocal8Bit(line.constData()) );
                    line.clear();
                }
            } else {
                break;
            }
        }

        if ( ferror(stdin) ) {
            perror( tr("Error reading stdin!").toLocal8Bit().constData() );
        } else if ( feof(stdin) ) {
            if ( !line.isNull() )
                setToolTip( QString::fromLocal8Bit(line.constData()) );
            socketNotifier_->deleteLater();
            socketNotifier_ = nullptr;
            return;
        }
    }

protected:
    Tray * const q_ptr;
    Q_DECLARE_PUBLIC(Tray)

    QSystemTrayIcon tray_;
    QMenu menu_;
    QPointer<QAction> actionReset_;
    QPointer<QAction> actionShowLog_;
    QIcon icon_;

    QString iconText_;
    QFont iconTextFont_;
    QColor iconTextColor_;
    QColor iconTextOutlineColor_;

    QSocketNotifier *socketNotifier_;
    int lines_;

    QString records_;
};

Tray::Tray(QObject *parent)
    : QObject(parent)
    , d_ptr(new TrayPrivate(this))
{
}

void Tray::setToolTip(const QString &text)
{
    Q_D(Tray);
    d->tray_.setToolTip(text);
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

void Tray::show()
{
    Q_D(Tray);
    d->show();
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

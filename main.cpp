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
#include <iostream>

#define VERSION "0.0.1"

class Arguments {
public:
    Arguments(const QStringList &arguments)
        : arguments_(std::move(arguments))
        , index_(1)
        , argIndex_(0)
    {
    }

    bool hasNext() const
    {
        return index_ < arguments_.size();
    }

    bool next()
    {
        name_ = value_ = QString();

        if ( !hasNext() )
            return false;

        const QString &arg = arguments_[index_];

        if ( arg.startsWith("--") ) {
            argIndex_ = 0;
            const int i = arg.indexOf(QChar('='));
            if (i != -1) {
                name_ = arg.left(i);
                value_ = arg.mid(i + 1);
            } else {
                name_ = arg;
            }

            ++index_;
        } else if ( arg.startsWith("-") ) {
            ++argIndex_;
            name_ = QString(QChar('-')) + arg.mid(argIndex_, 1);
            if ( argIndex_ + 1 == arg.size() ) {
                argIndex_ = 0;
                ++index_;
            }
        } else {
            ++index_;
        }

        return true;
    }

    const QString getName() const { return name_; }

    const QString fetchValue()
    {
        if (value_.isNull()) {
            if (argIndex_ > 0) {
                value_ = arguments_[index_].mid(argIndex_ + 1);
                argIndex_ = 0;
            }

            if (value_.isNull() && hasNext())
                value_ = arguments_[index_];

            ++index_;
        }

        return value_;
    }

    const QString &getArgument(int index) const
    {
        return arguments_[index];
    }

private:
    QStringList arguments_;
    QString name_;
    QString value_;
    int index_;
    int argIndex_;
};

QFont fontFromString(const QString &fontDesc)
{
    QFont font;
    QStringList tokens = fontDesc.split( QRegExp("\\s*[,;]\\s*") );

    for (QString &token : tokens) {
        QString t = token.toLower();
        if (t == "bold") {
            font.setBold(true);
        } else if (t == "italic") {
            font.setItalic(true);
        } else if (t.startsWith("under")) {
            font.setUnderline(true);
        } else if (t.startsWith("over")) {
            font.setOverline(true);
        } else if (t.startsWith("strike")) {
            font.setStrikeOut(true);
        } else {
            bool ok;
            int px = t.toInt(&ok);
            if (ok) {
                font.setPixelSize(px);
            } else {
                font.setFamily(token);
            }
        }
    }

    return font;
}

void error(const QString &msg, int exitCode = 0)
{
    std::cerr << msg.toLocal8Bit().data() << std::endl;
    if (exitCode != 0)
        exit(exitCode);
}

void printLine(const QString &line = QString())
{
    std::cout << line.toLocal8Bit().data() << std::endl;
}

void printHelp(const QString &program)
{
    printLine( QObject::tr("Usage: %1 [Options]").arg(program) );
    printLine( QString("  -h, --help                    ") + QObject::tr("Print help.") );
    printLine( QString("  -i, --icon {file name}        ") + QObject::tr("Set icon.") );
    printLine( QString("  -t, --text {icon text}        ") + QObject::tr("Set icon text.") );
    printLine( QString("  -T, --tooltip {tooltip text}  ") + QObject::tr("Set icon tool tip.") );
    printLine( QString("  -c, --color {color=black}     ") + QObject::tr("Set icon text color.") );
    printLine( QString("  -o, --outline {color=white}   ") + QObject::tr("Set icon text outline color.") );
    printLine( QString("  -f, --font {font}             ") + QObject::tr("Set icon text font.") );
    printLine();
    printLine( QString("TrayPost Desktop Tray Notifier " VERSION " (hluk@email.cz)") );
    exit(0);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QIcon icon;
    QString toolTip( QObject::tr("No messages available.") );
    QString iconText;
    QColor textColor;
    QColor textOutlineColor;
    QFont font = QApplication::font();

    Arguments args( app.arguments() );
    while ( args.next() ) {
        const QString &name = args.getName();

        if (name == "-h" || name == "--help") {
            printHelp( args.getArgument(0) );
        } else if (name == "-i" || name == "--icon") {
            auto &value = args.fetchValue();
            if (value.isNull())
                error( QObject::tr("Option %1 needs icon path.").arg(name), 2 );
            icon = QIcon(value);
            if ( icon.availableSizes().isEmpty() )
                icon = QIcon(QPixmap(value));
            if ( icon.availableSizes().isEmpty() )
                error( QObject::tr("Cannot open icon \"%1\".").arg(value) );
        } else if (name == "-t" || name == "--text") {
            auto &value = args.fetchValue();
            if (value.isNull())
                error( QObject::tr("Option %1 needs text.").arg(name), 2 );
            iconText = value;
        } else if (name == "-T" || name == "--tooltip") {
            auto &value = args.fetchValue();
            if (value.isNull())
                error( QObject::tr("Option %1 needs text.").arg(name), 2 );
            toolTip = value;
        } else if (name == "-c" || name == "--color") {
            auto &value = args.fetchValue();
            if (value.isNull())
                error( QObject::tr("Option %1 needs text.").arg(name), 2 );
            textColor = QColor(value);
            if ( !textColor.isValid() )
                error( QObject::tr("Invalid color \"%1\".").arg(value) );
        } else if (name == "-o" || name == "--outline") {
            auto &value = args.fetchValue();
            if (value.isNull())
                error( QObject::tr("Option %1 needs text.").arg(name), 2 );
            textOutlineColor = QColor(value);
            if ( !textOutlineColor.isValid() )
                error( QObject::tr("Invalid color \"%1\".").arg(value) );
        } else if (name == "-f" || name == "--font") {
            auto &value = args.fetchValue();
            if (value.isNull())
                error( QObject::tr("Option %1 needs font name.").arg(name), 2 );
            font = fontFromString(value);
        } else {
            error( QObject::tr("Unknown option \"%1\".").arg(name), 2 );
        }
    }

    if ( icon.availableSizes().isEmpty() )
        icon = QIcon::fromTheme("mail-unread");
    if ( !textColor.isValid() )
        textColor = Qt::black;
    if ( !textOutlineColor.isValid() )
        textOutlineColor = Qt::white;

    traypost::Tray tray;
    tray.setToolTip(toolTip);
    tray.setIcon(icon);
    tray.setIconText(iconText);
    tray.setIconTextStyle(font, textColor, textOutlineColor);
    tray.show();

    return app.exec();
}

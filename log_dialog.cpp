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
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "log_dialog.h"
#include "ui_log_dialog.h"

#include <QScrollBar>

#if QT_VERSION < 0x050000
#   include <QTextDocument> // Qt::escape()
#endif

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

QString Record::toString(const QString &format, const QString &timeFormat) const
{
    return format.arg( escapeHtml(text) ).arg( time.toString(timeFormat) );
}

LogDialog::LogDialog(const QList<Record> &records, const QString &format,
                     const QString &timeFormat, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogDialog)
    , timeFormat_(timeFormat)
    , format_(format)
{
    ui->setupUi(this);
    ui->listLog->setUniformItemSizes(true);

    for (auto &record : records) {
        createRecord(record);
    }

    ui->listLog->setCurrentRow(0);
}

LogDialog::~LogDialog()
{
    delete ui;
}

void LogDialog::addRecord(const Record &record)
{
    auto scrollBar = ui->listLog->verticalScrollBar();
    bool atBottom = scrollBar->value() == scrollBar->maximum();

    auto item = createRecord(record);

    if ( isFilteredOut(item, ui->lineEditSearch->text()) )
        item->setHidden(true);
    else if (atBottom && scrollBar->value() != 0)
        ui->listLog->scrollToItem(item);
}

bool LogDialog::isFilteredOut(QListWidgetItem *item, const QString &text) const
{
    auto widget = ui->listLog->itemWidget(item);
    auto itemText = widget->property("text").toString();
    return !itemText.contains(text, Qt::CaseInsensitive);
}

void LogDialog::on_listLog_itemActivated(QListWidgetItem *item)
{
    int row = ui->listLog->row(item);
    emit itemActivated(row);
}

void LogDialog::on_buttonReset_clicked()
{
    ui->lineEditSearch->clear();
}

void LogDialog::on_lineEditSearch_textChanged(const QString &text)
{
    auto list = ui->listLog;
    for ( int row = 0; row < list->count(); ++row ) {
        auto item = list->item(row);
        item->setHidden( isFilteredOut(item, text) );
    }
}

QListWidgetItem *LogDialog::createRecord(const Record &record)
{
    auto w = new QLabel( record.toString(format_, timeFormat_), ui->listLog );
    w->setContentsMargins(4, 4, 4, 4);

    auto item = new QListWidgetItem(ui->listLog);
    item->setSizeHint(w->sizeHint());
    ui->listLog->addItem(item);
    ui->listLog->setItemWidget(item, w);

    return item;
}

} // namespace traypost

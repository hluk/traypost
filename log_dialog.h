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

#include <QDialog>
#include <QDateTime>

class QListWidgetItem;

namespace Ui {
class LogDialog;
}

namespace traypost {

struct Record {
    Record() : text(), time() {}
    Record(const QString &text) : text(text), time(QDateTime::currentDateTime()) {}
    QString toString(const QString &format, const QString &timeFormat) const;

    QString text;
    QDateTime time;
};

class LogDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LogDialog(const QList<Record> &records, const QString &format,
                       const QString &timeFormat, QWidget *parent = nullptr);

    ~LogDialog();

    void addRecord(const Record &record);

    bool isFilteredOut(QListWidgetItem *item, const QString &text) const;

signals:
    void itemActivated(int row);

private slots:
    void on_listLog_itemActivated(QListWidgetItem *item);
    void on_buttonReset_clicked();
    void on_lineEditSearch_textChanged(const QString &text);

private:
    QListWidgetItem *createRecord(const Record &record);

    Ui::LogDialog *ui;
    QString timeFormat_;
    QString format_;
};

} // namespace traypost

#ifndef MAILCLIENT_H
#define MAILCLIENT_H

#include <QWidget>
#include <QtCore/QTextCodec>
#include <QTextStream>
#include <iostream>
#include <QCoreApplication>

class MailClient : public QWidget
{
    Q_OBJECT

public:
    explicit MailClient(QWidget* parent = 0);
    ~MailClient();
    bool HandleAccount();
    QString str;


};

#endif

#include <QtGui>
#include "mailclient.h"

#include "./receivemailapi/Pop3Client.h"

#include <iostream>
#include <QCoreApplication>
#include <QtGlobal>
#include <QtDebug>
#include "./receivemailapi/mail.h"
using namespace std;


#define _ERROR(s) {cerr << (s) << endl; return false;}
bool MailClient::HandleAccount()
{
    Pop3Client client(false);
    client.setConnectionType(Pop3Client::TcpConnection);
    if (!client.Connect("pop.163.com",110))
        _ERROR("could not connect")
    if (!client.Login("123sunyong123@163.com","sunyongyong0311"))
        _ERROR("could not login")

    QVector< QPair<QString,QString> > uIds; //list with <msgId, uniqueId>
    if (!client.GetUniqueIdList(uIds))
        _ERROR("could not get uniqueId Listing")

    for (int i = uIds.size() - 1;i >= 0;i--)
    {
        QString msg;
        if (!client.GetMessagess(uIds[i].first,msg))
            _ERROR("could not retrieve msg " + uIds[i].first.toStdString())

        //test sunyong
        Mail mails;
        mails.separateResponse(msg);
        mails.envelopeParse(mails.envelop);
        //qDebug()<<mails.contentCode;
        mails.contentParse();
        qDebug()<<QString("-------输出内容-------").toStdString().c_str();
        mails.test();
        qDebug()<<"------------------------------";
        if (str== NULL)
            str = mails.getContent();

    }

    if (!client.Quit())
        _ERROR("could not quit")

    return true;
}
#undef _ERROR

MailClient::MailClient(QWidget *parent) :
    QWidget(parent)
 {
    str = "";
    HandleAccount();
}

MailClient::~MailClient()
{

}


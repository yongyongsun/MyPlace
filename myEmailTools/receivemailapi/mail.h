#ifndef MAIL_H
#define MAIL_H
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QVector>
#include <QDebug>
const QString HeadT[]={"From","To","Subject","Content-Type","Content-Transfer-Encoding","Date"};
enum HeadType{
    FROM,TO,SUBJECT,CONTENTTYPE,CTE,DATES,NOUSE,NOTHEAD
};
class Mail
{
public:
    Mail();
    void test();
    void envelopeParse(QString);
    void separateResponse(QString response);
    void contentParse();
    const QString& getTitle(){return title;}
    const QString& getContent();
    QString envelop;

private:
    QString mimeDecode(QString);
    QString getLine(QString &);
    HeadType getHeadType(QString);
    void handle(HeadType,QString);
    QString parseBoundry(QString);
    QByteArray& quotedprintableDecode(QString &input);

private:
    QString sender;
    QString receiver;
    QString time;
    QString title;
    QString contentCode;

    QString content;
    QString contentType;
    QString boundary;
    QString contentEncodeType;
};

#endif // MAIL_H

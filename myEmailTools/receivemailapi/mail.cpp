#include "mail.h"
#include <QTextCodec>

Mail::Mail()
{
}

void Mail::test()
{
    qDebug()<<"sender:"<<this->sender<<"\r\n";
    qDebug()<<"receiver:"<<this->receiver<<"\r\n";
    qDebug()<<"title:"<<this->title<<"\r\n";
    qDebug()<<"data:"<<this->time<<"\r\n";
    qDebug()<<"contentEncodeType"<<this->contentEncodeType<<"\r\n";
    qDebug()<<"--envelop--"<<this->envelop<<"\r\n";
    qDebug()<<"--contentCode--"<<this->contentCode<<"\r\n";
    qDebug()<<"--content--"<<QString::fromLocal8Bit(this->content.toLatin1().data())<<"\r\n";
}

QString Mail::getLine(QString &response)
{
    int idx=response.indexOf("\n");
    QString result=response.left(idx);
    response=response.mid(idx+1,-1);
    return result;
}


HeadType Mail::getHeadType(QString line){

    QString type;
    int i;
    int idx=0;
    idx=line.indexOf(":");
    if(idx==-1)
        return NOTHEAD;
    else{
        type=line.mid(0,idx);
    }
    for(i=0;i<6;i++){
        if(type!=HeadT[i]){
        }
        else
            break;
    }
    switch (i) {
    case FROM:
        return FROM;
        break;
    case TO:
        return TO;
        break;
    case SUBJECT:
        return SUBJECT;
        break;
    case CONTENTTYPE:
        return CONTENTTYPE;
        break;
    case CTE:
        return CTE;
        break;
    case DATES:
        return DATES;
    default:
        return NOUSE;
        break;
    }
}

void Mail::handle(HeadType type, QString line)
{
    int idx;
    switch(type){
    case FROM:
        this->sender=mimeDecode(line);
        break;
    case TO:
        this->receiver=mimeDecode(line);
        break;
    case SUBJECT:
        this->title=mimeDecode(line);
        break;
    case CONTENTTYPE:
        this->contentType=line;
        idx=line.indexOf("boundary");
        if(idx!=-1)
            this->boundary=line.mid(idx+9);
        break;
    case CTE:
        this->contentEncodeType=line;
        break;
    case DATES:
        idx=line.indexOf(":");
        this->time=line;
        break;
    default:
        break;
    }
}

//QString Mail::parseBoundry(QString str)
//{
//    int i=0,idx;
//    QString type,encodetype,data,boundry;
//    QStringList line=str.split("\n");
//    int count=line.length();
//    while((line[i].simplified()!="")&&(i<count)){
//        idx=line[i].indexOf("charset=");
//       // qDebug()<<idx;
//        if(idx!=-1){
//            type=line[i].mid(idx+8);
//       //     qDebug()<< type.replace("\"","");
//        //    qDebug()<<type;
//            i++;
//        }
//        if((idx=line[i].indexOf("Content-Transfer-Encoding="))!=-1){
//            type=line[i].mid(idx+25);
//       //     qDebug()<< type.replace("\"","");
//            i++;
//        }
//        i++;
//    }

//    while(i<count){
//        data.append(QByteArray::fromBase64(line[i++].toAscii()));
//        data=data+"\n";
//    }
//    return data;
//}

QString Mail::parseBoundry(QString str){
    QStringList strList;
    QString charset,data,cte;
    qDebug()<<"*********";
    strList=str.split("\n");
    int i=0,idx=0;
    int count=strList.length();
    while(i<count){
        idx=strList[i].indexOf("charset=");
        if(idx!=-1){
              charset=strList[i].mid(idx+8);
        }
        idx=strList[i].indexOf("Content-Transfer-Encoding:");
        if(idx!=-1){
                cte=strList[i].mid(idx+26);
        }
        if(strList[i].simplified()=="")
            break;
        i++;
    }

    while(i<count)
        data=data+strList[i++]+"\n";

   // QTextCodec::setCodecForCStrings(QTextCodec::codecForName(charset.toStdString().c_str()));
    qDebug()<<cte;
    if(cte.simplified()=="base64")
        data=QByteArray::fromBase64(data.toLatin1());
    qDebug()<<data;
    qDebug()<<"*********";

    return data;
}

QByteArray &Mail::quotedprintableDecode(QString &input)
{
    //                    0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  @  A   B   C   D   E   F
    const int hexVal[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};
    int lnum,rnum;
    QByteArray *output = new QByteArray();

    for (int i = 0; i < input.length(); i++)
    {
        if (input.at(i).toLatin1() == '=')
        {
            if(input.at(i+1).toLatin1()=='\r'){
                i=i+1;
                if(input.at(i+1).toLatin1()=='\n')
                        i=i+1;
            }
            else{
                lnum=(hexVal[input.at(++i).toLatin1() - '0'] << 4);
                rnum=hexVal[input.at(++i).toLatin1() - '0'];
                output->append(lnum+rnum-256);
            }
        }
        else
        {
            output->append(input.at(i).toLatin1());
        }
    }

    return *output;
}

void Mail::separateResponse(QString response)
{
    int i=0;
    QStringList responseList=response.split("$");
    while(i<responseList.length()){
        if(responseList[i].simplified()=="")
            break;
        this->envelop=this->envelop+responseList[i++]+'\n';
    }
//    while(i<responseList.length()){
//        this->contentCode=this->contentCode+responseList[i++];
//    }

    this->contentCode=response;
}

void Mail::contentParse()
{
    int i=0;
    int count;
    QString data,str;
    if(!this->contentCode.isEmpty()){
        if(this->contentEncodeType=="quoted-printable")
            this->content=this->quotedprintableDecode(this->contentCode);
        else{
            QStringList contentlist=this->contentCode.split("\n");
            count=contentlist.length();
            while(i<count){
               qDebug()<<contentlist[i].indexOf(this->boundary);
               qDebug()<<contentlist[i];
                if(contentlist[i].indexOf(this->boundary)!=-1){
                   qDebug()<<this->boundary;
                   qDebug()<<contentlist[i];
                    qDebug()<<"************";
                   qDebug()<<str;
                    data.append(parseBoundry(str));
                    qDebug()<<"************";
                    str="";
                }
                else
                    str=str+contentlist[i]+"\n";
                i++;
            }
            this->content=data;
        }
    }
}


void Mail::envelopeParse(QString response)
{
    int i=0;
    int count;
    QString line;
    QString data;
    HeadType type,oldtype;
    int idx=0;
    QStringList responseList;
    responseList=response.split("\n");
    for(i=0;i<responseList.length();i++){
        line=responseList[i];
        type=getHeadType(responseList[i]);
        if((type!=NOUSE)&&(type!=NOTHEAD)){
            data=mimeDecode(responseList[i].mid(line.indexOf(":")+1,-1));
            if(i==responseList.length()-1){
                handle(type,data);
                break;
            }
            while(getHeadType(responseList[i+1])==NOTHEAD){
                data=data+mimeDecode(responseList[i+1]);
                i++;
                if(i==responseList.length()-1)
                    break;
            }
            handle(type,data);
        }

//        if((type!=NOUSE)&&(type!=NOTHEAD)){
//            if(!data.isEmpty()){        //若data为空则说�//                handle(oldtype,data);
//                data="";
//            }
//            data=data+line.mid(line.indexOf(":")+1,-1);
//        }
//        else if(type==NOTHEAD)
//            data=data+line

    }

}

//void pccode(QString str){
//    int i=0;
//    int state=0;
//    QString output;
//    while(i<str.length()){
//        if((state==0)&&str.at(i)=='='&&(str.at(i+1)=='?')){
//            i
//            state=1;
//            //�始解�//            while(str.at(i)){
//                if(str.at(i)=='?')
//            }
//        }
//        else
//            output.append(str.at(i));
//    }
//}


QString Mail::mimeDecode(QString str)
{
    int i=0;
    QString type,encodeType,data,str_append;

    str = str.trimmed();
    str.replace("\"","");
    if(str.mid(0,2)=="=?"){
        str_append=str.mid(str.indexOf("?=")+2,-1);
        str=str.mid(1,str.indexOf("?=")-1);
        i=str.indexOf("?",2);
        type=str.mid(str.indexOf("=?")+2,i-str.indexOf("?")-1);
        encodeType=str.mid(i+1,1);
        data=str.mid(i+3,-1);//str.indexOf("?=")-str.indexOf("?",2)-2);
        //QTextCodec::setCodecForCStrings(QTextCodec::codecForName(type.toStdString().c_str()));
        str=QByteArray::fromBase64(data.toLatin1())+str_append;
        str=str.simplified();
        if(encodeType.toUpper()=="B")
            return str ;
        else if(encodeType.toUpper()=="Q")
             return quotedprintableDecode(data);
        else
            return "ERROR";
    }
    else
        return str;
}


const QString& Mail::getContent()
{
    return content;

}

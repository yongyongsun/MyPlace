#include "Pop3Client.h"
#include <iostream>
#include <QtDebug>
#include <windows.h>

#define _ERROR(s) qDebug() << s << m_psock->errorString();
Pop3Client::Pop3Client(bool readOnly)
{
    m_psock = NULL;
	this->readOnly = readOnly;
    ct = TcpConnection;
	state = NotConnected;
}

void Pop3Client::SetReadOnly(bool readOnly)
{
	this->readOnly = readOnly;
}

void Pop3Client::setConnectionType(ConnectionType cty){
    this->ct = cty;
    switch (ct)
    {
    case TcpConnection:
        this->m_psock = new QTcpSocket();
        break;
    case SslConnection:
        this->m_psock = new QSslSocket();
    }

    m_psock->blockSignals(true);
}

bool Pop3Client::Connect(QString host,short unsigned int port)
{
    switch (this->ct)
    {
    case TcpConnection:
        m_psock->connectToHost(host, port);
        break;
    case SslConnection:
        ((QSslSocket*) m_psock)->connectToHostEncrypted(host, port);
        break;

    }

    if (!m_psock->waitForConnected(CONNECT_TIMEOUT))
	{
		_ERROR("Could not connect: ");
		return false;
	}
	QString response;
	ReadResponse(false,response);
	if (response.startsWith("+OK"))
		state = Authorization;
	else
		return false;
	return true;
}

//Sends a command to the server and returns the response
QString Pop3Client::doCommand(QString command,bool isMultiline)
{
    qDebug() << "sending command: " << command;
	QString response;
    qint64 writeResult = m_psock->write(command.toLocal8Bit());
    if (writeResult != command.size())
        _ERROR("Could not write all bytes: ");
    if (writeResult > 0 && !m_psock->waitForBytesWritten(WRITE_TIMEOUT))
        _ERROR("Could not write bytes from buffer: ");
	if (!ReadResponse(isMultiline,response))
		return "";

    return response;
}

bool Pop3Client::ReadResponse(bool isMultiline,QString& response)
{
	char buff[1512+1]; // according to RFC1939 the response can only be 512 chars
    bool couldRead = m_psock->waitForReadyRead( READ_TIMEOUT ) ;
    if (!couldRead)
        _ERROR("could not receive data: ");
	bool complete=false;
	bool completeLine=false;
	unsigned int offset=0;
	while (couldRead && !complete)
	{
		if (offset >= sizeof(buff))
		{
			qDebug() << "avoiding buffer overflow, server is not RFC1939 compliant\n";
			return false;
		}
        qint64 bytesRead = m_psock->readLine(buff + offset,sizeof(buff)-offset);
		if (bytesRead == -1)
			return false;

		couldRead = bytesRead > 0;
		completeLine = buff[offset+bytesRead-1]=='\n';
		if (couldRead)
		{
			if (completeLine)
			{
				offset = 0;
				if (response.size() == 0)
				{//first line, check for error
					response.append(buff);
					if (!response.startsWith("+OK"))
						complete = true;
					else
						complete = !isMultiline;
				}
				else
                {   //NOT first line, check for byte-stuffing
					//check if the response was complete
					complete = ( strcmp(buff,".\r\n") == 0 );
					if (!complete)
					{
						if (buff[0] == '.' && buff[1]== '.')
							response.append(buff+1);	//remove the stuffed byte and add to the response
						else
							response.append(buff);	//add to the response
					}
				}
			}
			else
			{
//				qDebug() << "Line INComplete: " << buff <<"\n";
				offset += bytesRead;
			}
		}
		if (couldRead && !complete)
		{
            if (m_psock->bytesAvailable() <= 0)
			{
//		qDebug() << "waiting for data\n";
                couldRead = m_psock->waitForReadyRead( READ_TIMEOUT ) ;
//		qDebug() << "waiting for data finished, couldread: " << couldRead << "\n";
			}
		}
	}
	return couldRead && complete;
}

bool Pop3Client::Login(QString user, QString pwd)
{
	if (!SendUser(user))
		return false;
	if (!SendPasswd(pwd))
		return false;
	state = Transaction;
	return true;
}
bool Pop3Client::SendUser(QString& user)
{
	QString res = doCommand("USER "+user+"\r\n",false);
	if (res.startsWith("+OK"))
		return true;
	else
		return false;
}
bool Pop3Client::SendPasswd(QString& pwd)
{
	QString res = doCommand("PASS "+pwd+"\r\n",false);
	if (res.startsWith("+OK"))
		return true;
	else
		return false;
}
bool Pop3Client::Quit()
{
	if (readOnly)
		if (!ResetDeleted())
			return false;
	QString res = doCommand("QUIT\r\n",false);
	if (res.startsWith("+OK"))
	{
        if (!m_psock->waitForDisconnected(DISCONNECT_TIMEOUT))
		{
			_ERROR("Connection was not closed by server: ");
			return false;
		}
		else
			state = NotConnected;

        if (m_psock != NULL)
            delete m_psock;

		return true;
	}
	else
	{
		return false;
	}
}
bool Pop3Client::GetMailboxStatus(int& nbMessages, int& totalSize)
{
	QString res = doCommand("STAT\r\n",false);
	if (res.startsWith("+OK"))
	{
		QStringList sl = res.split(' ',QString::SkipEmptyParts);
		if (sl.count() < 3)
			return false;
		else
		{
			nbMessages = sl[1].toInt();
			totalSize = sl[2].toInt();
		}
		return true;
	}
	else
		return false;
}
bool Pop3Client::ResetDeleted()
{
	QString res = doCommand("RSET\r\n",false);
	if (res.startsWith("+OK"))
		return true;
	else
		return false;
}
bool Pop3Client::NoOperation()
{
	QString res = doCommand("NOOP\r\n",false);
	if (res.startsWith("+OK"))
		return true;
	else
		return false;
}
QPair<QString,QString> Pop3Client::parseUniqueIdListing(QString& line)
{
	QPair<QString,QString> p;
	QStringList sl = line.split(' ',QString::SkipEmptyParts);
	p.first = sl[0];
	p.second = sl[1];
	return p;
}
QPair<QString,int> Pop3Client::parseMsgIdListing(QString& line)
{
	QPair<QString,int> p;
	QStringList sl = line.split(' ',QString::SkipEmptyParts);
	p.first = sl[0];
	p.second = sl[1].toInt();
	return p;
}
bool Pop3Client::GetUniqueIdList(QVector< QPair<QString,QString> >& uIdList)
{
	QString res = doCommand("UIDL\r\n",true);
	if (res.startsWith("+OK"))
	{
		QStringList sl = res.split("\r\n",QString::SkipEmptyParts);
		int i;
		for (i=1;i<sl.count();i++)
			uIdList.append(parseUniqueIdListing(sl[i]));
		return true;
	}
	else
		return false;
}
bool Pop3Client::GetUniqueIdList(QString msgId, QPair<QString,QString>& uIdList)
{
	QString res = doCommand("UIDL "+msgId+"\r\n",false);
	if (res.startsWith("+OK"))
	{
		QStringList sl = res.split(' ',QString::SkipEmptyParts);
		uIdList.first = sl[1];
		uIdList.second = sl[2];
		return true;
	}
	else
		return false;
}
bool Pop3Client::Delete(QString msgId)
{
	if (readOnly)
		return false;
	QString res = doCommand("DELE "+msgId+"\r\n",false);
	if (res.startsWith("+OK"))
		return true;
	else
		return false;
}
bool Pop3Client::GetMessageTop(QString msgId, int nbLines, QString& msgTop)
{
	QString res = doCommand("TOP "+msgId+" "+QString::number(nbLines)+"\r\n",true);
	if (res.startsWith("+OK"))
	{
		msgTop = res.section("\r\n",1);
		return true;
	}
	else
		return false;
}
bool Pop3Client::GetMessagess(QString msgId, QString& msg)
{
	QString res = doCommand("RETR "+msgId+"\r\n",true);
	if (res.size() == 0)
		return false;
	if (res.startsWith("+OK"))
	{
        msg = res.section("\r\n",1);

//        //test sunyong
//        QByteArray ba = str1.toLatin1(); // must
//        const char* buff = ba.data();
//        int   wcsLen = ::MultiByteToWideChar(CP_ACP, NULL, buff, strlen(buff), NULL,0);
//        wchar_t* wszString = new wchar_t[wcsLen + 1];
//        ::MultiByteToWideChar(CP_ACP, NULL, buff, strlen(buff), wszString, wcsLen);
//         wszString[wcsLen] = '\0';
//        msg= QString::fromWCharArray(wszString);
//        delete[] wszString;




		return true;
	}
	else
	{
//		qDebug() << "ErrResponse: " << res;
		return false;
	}
}
bool Pop3Client::GetMsgList(QVector< QPair<QString,int> >& uIdList)
{
	QString res = doCommand("LIST\r\n",true);
	if (res.startsWith("+OK"))
	{
		QStringList sl = res.split("\r\n",QString::SkipEmptyParts);
		int i;
		for (i=1;i<sl.count();i++)
			uIdList.append(parseMsgIdListing(sl[i]));
		return true;
	}
	else
		return false;
}
bool Pop3Client::GetMsgList(QString msgId, QPair<QString,int>& uIdList)
{
	QString res = doCommand("LIST "+msgId+"\r\n",false);
	if (res.startsWith("+OK"))
	{
		QStringList sl = res.split(' ',QString::SkipEmptyParts);
		uIdList.first = sl[1];
		uIdList.second = sl[2].toInt();
		return true;
	}
	else
		return false;
}
bool Pop3Client::LoginWithDigest(QString user, QString digest)
{
	QString res = doCommand("APOP "+user+" "+digest+"\r\n",false);
	if (res.startsWith("+OK"))
		return true;
	else
		return false;
}


#undef _ERROR


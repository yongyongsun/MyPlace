#include "smtpclient.h"
#include "mimehtml.h"
#include "mimeattachment.h"
#include <QFileInfo>
#include <QByteArray>

SmtpClient::SmtpClient(const QString & host, int port, ConnectionType connectionType) :
    socket(NULL),
    name("localhost"),
    authMethod(AuthPlain),
    connectionTimeout(5000),
    responseTimeout(5000),
    mp_mimeMessage(NULL)
{
    setConnectionType(connectionType);

    this->host = host;
    this->port = port;

    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(socketReadyRead()));
}

SmtpClient::~SmtpClient() {}

void SmtpClient::setUser(const QString &user)
{
    this->user = user;
}

void SmtpClient::setPassword(const QString &password)
{
    this->password = password;
}

void SmtpClient::setAuthMethod(AuthMethod method)
{
    this->authMethod = method;
}

void SmtpClient::setHost(QString &host)
{
    this->host = host;
}

void SmtpClient::setPort(int port)
{
    this->port = port;
}

void SmtpClient::setConnectionType(ConnectionType ct)
{
    this->connectionType = ct;

    switch (connectionType)
    {
    case TcpConnection:
        socket = new QTcpSocket(this);
        break;
    case SslConnection:
    case TlsConnection:
        socket = new QSslSocket(this);
    }
}

const QString& SmtpClient::getHost() const
{
    return this->host;
}

const QString& SmtpClient::getUser() const
{
    return this->user;
}

const QString& SmtpClient::getPassword() const
{
    return this->password;
}

SmtpClient::AuthMethod SmtpClient::getAuthMethod() const
{
    return this->authMethod;
}

int SmtpClient::getPort() const
{
    return this->port;
}

SmtpClient::ConnectionType SmtpClient::getConnectionType() const
{
    return connectionType;
}

const QString& SmtpClient::getName() const
{
    return this->name;
}

void SmtpClient::setName(const QString &name)
{
    this->name = name;
}

const QString & SmtpClient::getResponseText() const
{
    return responseText;
}

int SmtpClient::getResponseCode() const
{
    return responseCode;
}

QTcpSocket* SmtpClient::getSocket() {
    return socket;
}

int SmtpClient::getConnectionTimeout() const
{
    return connectionTimeout;
}

void SmtpClient::setConnectionTimeout(int msec)
{
    connectionTimeout = msec;
}

int SmtpClient::getResponseTimeout() const
{
    return responseTimeout;
}

void SmtpClient::setResponseTimeout(int msec)
{
    responseTimeout = msec;
}

bool SmtpClient::connectToHost()
{
    switch (connectionType)
    {
    case TlsConnection:
    case TcpConnection:
        socket->connectToHost(host, port);
        break;
    case SslConnection:
        ((QSslSocket*) socket)->connectToHostEncrypted(host, port);
        break;

    }

    if (!socket->waitForConnected(connectionTimeout))
    {
        emit smtpError(ConnectionTimeoutError);
        return false;
    }

    try
    {
        // Wait for the server's response
        waitForResponse();

        // If the response code is not 220 (Service ready)
        // means that is something wrong with the server
        if (responseCode != 220)
        {
            emit smtpError(ServerError);
            return false;
        }

        // Send a EHLO/HELO message to the server
        // The client's first command must be EHLO/HELO
        sendMessage("EHLO " + name);

        // Wait for the server's response
        waitForResponse();

        // The response code needs to be 250.
        if (responseCode != 250) {
            emit smtpError(ServerError);
            return false;
        }

        if (connectionType == TlsConnection) {
            // send a request to start TLS handshake
            sendMessage("STARTTLS");

            // Wait for the server's response
            waitForResponse();

            // The response code needs to be 220.
            if (responseCode != 220) {
                emit smtpError(ServerError);
                return false;
            };

            ((QSslSocket*) socket)->startClientEncryption();

            if (!((QSslSocket*) socket)->waitForEncrypted(connectionTimeout)) {
                qDebug() << ((QSslSocket*) socket)->errorString();
                emit smtpError(ConnectionTimeoutError);
                return false;
            }

            // Send ELHO one more time
            sendMessage("EHLO " + name);

            // Wait for the server's response
            waitForResponse();

            // The response code needs to be 250.
            if (responseCode != 250) {
                emit smtpError(ServerError);
                return false;
            }
        }
    }
    catch (ResponseTimeoutException)
    {
        return false;
    }

    return true;
}

bool SmtpClient::login()
{
    return login(user, password, authMethod);
}

bool SmtpClient::login(const QString &user, const QString &password, AuthMethod method)
{
    try {
        if (method == AuthPlain)
        {
            // Sending command: AUTH PLAIN base64('\0' + username + '\0' + password)
            sendMessage("AUTH PLAIN " + QByteArray().append((char) 0).append(user).append((char) 0).append(password).toBase64());

            // Wait for the server's response
            waitForResponse();

            // If the response is not 235 then the authentication was faild
            if (responseCode != 235)
            {
                emit smtpError(AuthenticationFailedError);
                return false;
            }
        }
        else if (method == AuthLogin)
        {
            // Sending command: AUTH LOGIN
            sendMessage("AUTH LOGIN");

            // Wait for 334 response code
            waitForResponse();
            if (responseCode != 334) { emit smtpError(AuthenticationFailedError); return false; }

            // Send the username in base64
            sendMessage(QByteArray().append(user).toBase64());

            // Wait for 334
            waitForResponse();
            if (responseCode != 334) { emit smtpError(AuthenticationFailedError); return false; }

            // Send the password in base64
            sendMessage(QByteArray().append(password).toBase64());

            // Wait for the server's responce
            waitForResponse();

            // If the response is not 235 then the authentication was faild
            if (responseCode != 235)
            {
                emit smtpError(AuthenticationFailedError);
                return false;
            }
        }
    }
    catch (ResponseTimeoutException e)
    {
        // Responce Timeout exceeded
        emit smtpError(AuthenticationFailedError);
        return false;
    }

    return true;
}

bool SmtpClient::sendMail()
{
    if (mp_mimeMessage == NULL)
        return false;

    try
    {
        // Send the MAIL command with the sender
        sendMessage("MAIL FROM: <" + mp_mimeMessage->getSender().getAddress() + ">");

        waitForResponse();

        if (responseCode != 250) return false;

        // Send RCPT command for each recipient
        QList<EmailAddress*>::const_iterator it, itEnd;
        // To (primary recipients)
        for (it = mp_mimeMessage->getRecipients().begin(), itEnd = mp_mimeMessage->getRecipients().end();
             it != itEnd; ++it)
        {
            sendMessage("RCPT TO: <" + (*it)->getAddress() + ">");
            waitForResponse();

            if (responseCode != 250) return false;
        }

        // Cc (carbon copy)
        for (it = mp_mimeMessage->getRecipients(MimeMessage::Cc).begin(), itEnd = mp_mimeMessage->getRecipients(MimeMessage::Cc).end();
             it != itEnd; ++it)
        {
            sendMessage("RCPT TO: <" + (*it)->getAddress() + ">");
            waitForResponse();

            if (responseCode != 250) return false;
        }

        // Bcc (blind carbon copy)
        for (it = mp_mimeMessage->getRecipients(MimeMessage::Bcc).begin(), itEnd = mp_mimeMessage->getRecipients(MimeMessage::Bcc).end();
             it != itEnd; ++it)
        {
            sendMessage("RCPT TO: <" + (*it)->getAddress() + ">");
            waitForResponse();

            if (responseCode != 250) return false;
        }

        // Send DATA command
        sendMessage("DATA");
        waitForResponse();

        if (responseCode != 354) return false;

        sendMessage(mp_mimeMessage->toString());

        // Send \r\n.\r\n to end the mail data
        sendMessage(".");

        waitForResponse();

        if (responseCode != 250) return false;
    }
    catch (ResponseTimeoutException)
    {
        return false;
    }

    return true;
}

void SmtpClient::quit()
{
    sendMessage("QUIT");

    if (mp_mimeMessage != NULL)
    {
        delete mp_mimeMessage;
        mp_mimeMessage = NULL;
    }
}

void SmtpClient::waitForResponse() throw (ResponseTimeoutException)
{
    do {
        if (!socket->waitForReadyRead(responseTimeout))
        {
            emit smtpError(ResponseTimeoutError);
            throw ResponseTimeoutException();
        }

        while (socket->canReadLine()) {
            // Save the server's response
            responseText = socket->readLine();

            // Extract the respose code from the server's responce (first 3 digits)
            responseCode = responseText.left(3).toInt();

            if (responseCode / 100 == 4)
                emit smtpError(ServerError);

            if (responseCode / 100 == 5)
                emit smtpError(ClientError);

            if (responseText[3] == ' ') { return; }
        }
    } while (true);
}

void SmtpClient::sendMessage(const QString &text)
{
    socket->write(text.toUtf8() + "\r\n");
}

void SmtpClient::socketStateChanged(QAbstractSocket::SocketState state)
{
}

void SmtpClient::socketError(QAbstractSocket::SocketError socketError)
{
}

void SmtpClient::socketReadyRead()
{
}

void SmtpClient::setMailSender(const QString & stradd) //mail sender address
{
    if (mp_mimeMessage == NULL)
        mp_mimeMessage = new MimeMessage();

    mp_mimeMessage->setSender(new EmailAddress(stradd));
}

void SmtpClient::setAddMailRevicer(const QStringList & list,MimeMessage::RecipientType type)
{
    if (mp_mimeMessage == NULL)
        mp_mimeMessage = new MimeMessage();

    for (int i = 0; i < list.size(); i++)
    {
        EmailAddress * address = new EmailAddress(list.at(i));
        mp_mimeMessage->addRecipient(address);
    }
}

void SmtpClient::setMailTitle(const QString& strTitle)// 邮件标题
{
    if (mp_mimeMessage == NULL)
        mp_mimeMessage = new MimeMessage();

    mp_mimeMessage->setSubject(strTitle);
}

void SmtpClient::setMailContent(const QString str)//邮件内容
{
    if (mp_mimeMessage == NULL)
        mp_mimeMessage = new MimeMessage();

//    MimeHtml text;
//    text.setHtml(str);
    mp_mimeMessage->addPart(new MimeHtml(str));
}

void SmtpClient::setAddAttachment(const QStringList lists)//附件
{
    if (mp_mimeMessage == NULL)
        mp_mimeMessage = new MimeMessage();

    foreach (QString tempAtta, lists) {
        QFile *file=new QFile(tempAtta);
        if (file->exists()){
            mp_mimeMessage->addPart(new MimeAttachment(file));
        }
    }
}

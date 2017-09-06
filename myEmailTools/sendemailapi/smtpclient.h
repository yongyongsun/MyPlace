#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QtNetwork/QSslSocket>

#include "mimemessage.h"


class SmtpClient : public QObject
{
    Q_OBJECT
public:    

    enum AuthMethod
    {
        AuthPlain,
        AuthLogin
    };

    enum SmtpError
    {
        ConnectionTimeoutError,
        ResponseTimeoutError,
        AuthenticationFailedError,
        ServerError,    // 4xx smtp error
        ClientError     // 5xx smtp error
    };

    enum ConnectionType
    {
        TcpConnection,
        SslConnection,
        TlsConnection       // STARTTLS
    };   

    SmtpClient(const QString & host = "locahost", int port = 25, ConnectionType ct = TcpConnection);

    ~SmtpClient();   

    const QString& getHost() const;
    void setHost(QString &host);

    int getPort() const;
    void setPort(int port);

    const QString& getName() const;
    void setName(const QString &name);

    ConnectionType getConnectionType() const;
    void setConnectionType(ConnectionType ct);

    const QString & getUser() const;
    void setUser(const QString &host);

    const QString & getPassword() const;
    void setPassword(const QString &password);

    SmtpClient::AuthMethod getAuthMethod() const;
    void setAuthMethod(AuthMethod method);

    const QString & getResponseText() const;
    int getResponseCode() const;

    int getConnectionTimeout() const;
    void setConnectionTimeout(int msec);

    int getResponseTimeout() const;
    void setResponseTimeout(int msec);

    QTcpSocket* getSocket();

    bool connectToHost();
    bool login();
    bool login(const QString &user, const QString &password, AuthMethod method = AuthLogin);

    bool sendMail();
    void quit();

    void setMailSender(const QString & stradd); //发件人列表
    void setAddMailRevicer(const QStringList & list,MimeMessage::RecipientType type = MimeMessage::To);//收件人列表
    void setMailTitle(const QString& strTitle);// 邮件标题
    void setMailContent(const QString str);//邮件内容
    void setAddAttachment(const QStringList lists);//附件
private:
    QTcpSocket *socket;
    QString host;
    int port;
    ConnectionType connectionType;
    QString name;

    QString user;
    QString password;
    AuthMethod authMethod;

    int connectionTimeout;
    int responseTimeout;

    QString responseText;
    int responseCode;

    MimeMessage *mp_mimeMessage;

private:
    class ResponseTimeoutException {};
    void waitForResponse() throw (ResponseTimeoutException);
    void sendMessage(const QString &text);    

protected slots:
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketError(QAbstractSocket::SocketError error);
    void socketReadyRead();

signals:
    void smtpError(SmtpError e);    

};

#endif // SMTPCLIENT_H

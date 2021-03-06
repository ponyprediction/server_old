#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QVector>
#include "socket-thread.hpp"
#include "core/databasemanager.hpp"

class Server : public QTcpServer
{
    Q_OBJECT
public:
    Server();
    ~Server();
protected:
    void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE;
    QVector<SocketThread*> listSocketThread;
private:
    DatabaseManager databaseManager;
public slots:
    void onDisconnection(SocketThread *);
};

#endif // SERVER_H

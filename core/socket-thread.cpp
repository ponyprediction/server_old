#include "socket-thread.hpp"
#include "qendian.h"
#include "core/util.hpp"
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QFile>

SocketThread::SocketThread(int socketDescriptor,
                           DatabaseManager * pDatabaseManager):
    socketClient(new QTcpSocket()),logged(false),downloadManager()
{
    socketClient->setSocketDescriptor(socketDescriptor);
    databaseManager = pDatabaseManager;
    write("hi");
}

SocketThread::~SocketThread()
{
    socketClient->deleteLater();
    emit disconnection(this);
}

void SocketThread::run()
{
    connect(socketClient, SIGNAL(readyRead()), this, SLOT(readyRead()),
            Qt::DirectConnection);
    connect(socketClient, SIGNAL(disconnected()), this, SLOT(disconnect()));
    exec();
}

void SocketThread::readyRead()
{
    QString request = read();
    Util::log(request);
    if (request == "exit")
    {
        write("bye");
        disconnect();
    }
    else if(logged)
    {
        if(request == "HEY")
            write("Coucou");
        else if(request.startsWith("getbrain "))
        {
            int idJob = request.remove(0,9).toInt();
            int idBestBrain = databaseManager->getIdBestBrain(idJob);
            QFile file(Util::getLineFromConf("pathToSavedBrain") +
                       QString::number(idBestBrain) + ".brain");
            if(file.exists())
            {
                file.open(QIODevice::ReadOnly);
                write("brain " + file.readAll());
            }
        }
        else if(request.startsWith("sendbrain "))
        {
            QString brainXML = request.remove(0,10);
            QFile brain(Util::getLineFromConf("pathToSavedBrain") +
                        "uploadedbrain.brain");
            brain.open(QIODevice::WriteOnly);
            brain.write(brainXML.toUtf8());
            brain.close();
            QString fileName = Util::getLineFromConf("pathToSavedBrain") +
                    "uploadedbrain.brain";
            databaseManager->saveBrain(
                        Util::getLineFromFile(fileName,"ratio").toDouble(),
                        Util::getLineFromFile(fileName,"jobId").toInt(),
                        user.getUserid());
            write("brainreceived");
        }
        else
        {
            write("wtf");
        }
    }
    else
    {
        if(request.startsWith("log"))
        {
            delete downloadManager;
            downloadManager = new QNetworkAccessManager();
            QStringList split = request.split(" ");
            if(split.length() >= 3)
            {
                user.setUsername(split[1]);
                QUrl url("http://"+Util::getLineFromConf("ip")
                         + "/php/scripts/check-password.php");
                QUrlQuery postData;
                postData.addQueryItem("hash",
                                      databaseManager->getUserHash(user.getUsername()));
                postData.addQueryItem("password", split[2]);
                connect(downloadManager, SIGNAL(finished(QNetworkReply*)),
                        this, SLOT(onPasswordCheckReply(QNetworkReply*)));
                downloadManager->post(QNetworkRequest(url),
                                      postData.toString().toUtf8());
            }
            else
            {
                write("wtf");
            }
        }
        else
        {
            write("wtf");
        }
    }
}

void SocketThread::disconnect()
{
    quit();
}

void SocketThread::onPasswordCheckReply(QNetworkReply * reply)
{
    QString answer = reply->readAll();
    if(answer == "true" &&
            databaseManager->getUserConfirmation(user.getUsername()))
    {
        logged=true;
        write("welcome");
        user.setUserid(databaseManager->getUserId(user.getUsername()));
    }
    else if(answer == "false" ||
            !databaseManager->getUserConfirmation(user.getUsername()))
    {
        user.setUsername("");
        write("unicorn");
    }
    else
    {
        write("Unknown error");
    }
}

QString SocketThread::read()
{
    QString request(socketClient->readAll());
    request = request.left(request.size()-2);
    return request;
}

bool SocketThread::write(QString answer)
{
    answer += "\r\n";
    if(socketClient->write(answer.toUtf8())!= -1)
    {
        return true;
    }
    else
        return false;
}

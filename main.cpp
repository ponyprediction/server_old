#include <QCoreApplication>
#include "core/server.hpp"
#include <iostream>
#include "core/util.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Server server;
    Util::init(&a,&server);
    Util::catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP});
    if (!server.listen(QHostAddress::Any,50000)) {
        Util::log("Unable to start the server: " + server.errorString());
        return 0;
    }
    Util::log("Server started ! ");
    return a.exec();
}

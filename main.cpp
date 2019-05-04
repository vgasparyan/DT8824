#include <QCoreApplication>
#include "dt8824client.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    DT8824Client  dt8824;
    if (!dt8824.connect("localhost", 8080)) {
        qWarning("Failed to connect.");
        return 1;
    }

    if (dt8824.enableProtectedCommands("admin") != DeviceError::OK) {
        qWarning() << "Failed to enable protected commands\n";
        return 1;
    }

    auto result = dt8824.scpi_query("*IDN?");
    qInfo() << "DT8824 version  : " << result;

    auto r = dt8824.getAinEnable(1);
    qInfo() << "DT8824 channel  : " << r;

    dt8824.setAinEnable(1,1);

    return a.exec();
}

#ifndef DT8824CLIENT_H
#define DT8824CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>

//error codes returned from set configuration operations
enum DeviceError {
    OK,             //No errors
    PARAM,          //Parameter errors
    COMMUNICATION,  //Communication errors
    SYSTEM,         //Error in device
};

enum GAIN {
    GAIN_1=1,
    GAIN_8=8,
    GAIN_16=16,
    GAIN_32=32,
    GAIN_INVALID = 0xFF
};

/**
 * @brief The DT8824Client class
 *
 * Simple client for communication with DT8824 DAQ device
 * https://www.mccdaq.com/Products/24-Bit-DAQ-Modules/DT8824
 *
 * @todo support fetch command
 */
class DT8824Client : public QObject
{
    Q_OBJECT
public:

public:
    explicit DT8824Client(QObject *parent = nullptr);
    ~DT8824Client();

    DT8824Client(const DT8824Client&) = delete;
    DT8824Client& operator = (const DT8824Client&) = delete;

public:
    bool connect(QString host, int port);
    int scpi_command(const QString& cmd_str);
    int scpi_command_test();
    QString scpi_query(const QString &query);

    //supported scpi commands
    bool getAinEnable(int channel);
    void setAinEnable(int channel, bool enable);

    int getBufferSize();
    float getAdClockRate();
    DeviceError setAdClockRate(float rate);
    DeviceError setAdTriggerSource();

    GAIN getAinGain(int channel);
    DeviceError setAinGain(int channel, GAIN gain);
    int getScanLastIndex();
    int scpiErrorCount();
    void clearSystemErr();
    QString getSystemErr();
    DeviceError enableProtectedCommands(const QString& password);
    bool isAdArmed();
    bool isAdActive();
    DeviceError adArmTrigger();
    DeviceError abort();

signals:

public slots:
    void connected();
    void disconnected();

private:
    QTcpSocket* m_socket = nullptr;
    QMutex m_mutex;
    int timeout = 1000;
};

#endif // DT8824CLIENT_H

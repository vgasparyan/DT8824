#include "dt8824client.h"
#include <iostream>

#include <QDebug>
#include <QMutexLocker>

/**
 * @brief Convert voltage value to raw value.
 *
 * @param volt : flating point number between +10.0 and -10.0
 * @return The raw 24-bit DA values in offset binary where
 *          +10V = 0xffffff, -10V = 0x00, 0V = 0x800000
 **/
static uint32_t Volt2RawData(float volt)
{
    constexpr int   RESOLUTION    = (0xffffff + 1);  //2^24
    constexpr float VOLT_MIN   = -10.0f;
    constexpr float VOLT_MAX   = 10.0f;
    constexpr float PERCENT_FS    = RESOLUTION/(VOLT_MAX - VOLT_MIN);

    if (volt < VOLT_MIN) {
        volt = VOLT_MIN;
    } else if (volt >= VOLT_MAX) {
        volt = VOLT_MAX;
    }

    uint32_t raw = (int)((volt - VOLT_MIN)*(PERCENT_FS));
    return (raw >= RESOLUTION) ? (RESOLUTION - 1) : raw;
}

bool DT8824Client::connect(QString host, int port)
{
    m_socket->abort();
    m_socket->connectToHost(host, port);
    return m_socket->waitForConnected();
}

/**
 * @brief Send SCPI command to DT88224 device
 * @param command_str - SCPI command to be sent
 **/
int DT8824Client::scpi_command(const QString& cmd_str)
{
    QString cmd(cmd_str);
    cmd += "\n";
    auto len =  m_socket->write(cmd.toUtf8());
    if (!m_socket->waitForBytesWritten(timeout)) {
        qDebug() << m_socket->errorString();
    }
    return len;
}

QString DT8824Client::scpi_query(const QString& query)
{
    Q_ASSERT(!query.isEmpty());
    QMutexLocker locker(&m_mutex);
    scpi_command(query);
    m_socket->waitForReadyRead(timeout);
    auto data = m_socket->readAll();
    return QString(data);
}

bool DT8824Client::getAinEnable(int channel)
{
    bool enable = false;
    if ((channel >=1) && (channel <=4)) {
        QString cmd = QString("AD:ENAB? (@%1)").arg(channel);
        enable = scpi_query(cmd) == "1";
    }
    return enable;
}

void DT8824Client::setAinEnable(int channel, bool enable)
{
    QString cmd;
    cmd.sprintf("AD:ENAB %d,(@%d)", enable, channel);
    scpi_command(cmd);
}

int DT8824Client::getBufferSize()
{
    const auto result = scpi_query("AD:BUFF:SIZe?");
    return result.isEmpty() ? 0 : result.toInt();
}

float DT8824Client::getAdClockRate()
{
    auto response = scpi_query("AD:CLOCK:FREQ?");
    return response.isEmpty() ? 0.0 : response.toFloat();
}

DeviceError DT8824Client::setAdClockRate(float rate)
{
    auto err = DeviceError::OK;
    int errCount = scpiErrorCount();

    QString cmd;
    cmd.sprintf(":AD:CLOCK:SOURCE INT; :AD:CLOCK:FREQ %.3f", rate);

    if (scpi_command(cmd)) {
        if (scpiErrorCount() > errCount) {
            err = DeviceError::SYSTEM;
        }
    } else {
        err =  DeviceError::COMMUNICATION;
    }
    return err;
}

DeviceError DT8824Client::setAdTriggerSource()
{
    auto err = DeviceError::OK;
    int errCount = scpiErrorCount();

    if (scpi_command("AD:TRIG IMM")) {
        if (scpiErrorCount() > errCount) {
            err = DeviceError::SYSTEM;
        }
    } else {
        err =  DeviceError::COMMUNICATION;
    }
    return err;
}


GAIN DT8824Client::getAinGain(int channel)
{
    auto gain = GAIN::GAIN_INVALID;

    if ((channel >=1) && (channel <=4)) {
        QString cmd;
        cmd.sprintf("AD:GAIN? (@%d)", channel);
        const auto response = scpi_query(cmd);
        if (!response.isEmpty()) {
            gain = (GAIN)response.toInt();
            switch (gain) {
            case GAIN_1:
            case GAIN_8:
            case GAIN_16:
            case GAIN_32:
                break;
            default:
                break;
            }
        }
    }
    return gain;
}

DeviceError DT8824Client::setAinGain(int channel, GAIN gain)
{
    auto err = DeviceError::OK;

    if ((channel >=1) && (channel <=4)) {
        int errCount = scpiErrorCount();
        QString cmd;
        cmd.sprintf("AD:GAIN %d, (@%d)", (int)gain, channel);
        if (scpi_command(cmd)) {
            if (scpiErrorCount() > errCount) {
                err = DeviceError::SYSTEM;
            }
        } else {
            err =  DeviceError::COMMUNICATION;
        }
    } else {
        err = DeviceError::PARAM;
    }
    return err;
}

int DT8824Client::getScanLastIndex()
{
    int ret = -1;
    auto response = scpi_query(":AD:STAT:SCA?");
    if (!response.isEmpty()) {
        auto vec = response.split(',');
        if (vec.size() != 2) return  ret;
        auto str = vec.at(1);
        ret = str.toInt();
    }
    return ret;
}

int DT8824Client::scpiErrorCount()
{
    auto response = scpi_query(":SYST:ERR:COUNT?");
    return (!response.isEmpty()) ? response.toInt() : 0;
}

void DT8824Client::clearSystemErr()
{
    scpi_command("*CLS");
}

QString DT8824Client::getSystemErr()
{
    QString response = scpi_query("SYST:ERR?");
    if (!response.isEmpty() && response[0] == '0') {
        response.clear(); //return empty string
    }
    return response;
}

DeviceError DT8824Client::enableProtectedCommands(const QString& password)
{
    auto err = DeviceError::OK;
    int err_cnt = scpiErrorCount();
    QString cmd = "SYST:PASS:CEN " + password;
    if (scpi_command(cmd)) {
        if (scpiErrorCount() > err_cnt) {
            err = DeviceError::SYSTEM;
        }
    } else {
        err =  DeviceError::COMMUNICATION;
    }
    return err;
}

bool DT8824Client::isAdArmed()
{
    bool ret = false;
    QString response = scpi_query("AD:STAT?");
    if (!response.isEmpty()) {
        int status = response.toInt() ;
        ret = ((status & 0x02) == 0x02);
    }
    return ret;
}

bool DT8824Client::isAdActive()
{
    bool ret = false;
    QString response = scpi_query("AD:STAT?");
    if (!response.isEmpty()) {
        int status = response.toInt() ;
        ret = ((status & 0x01) == 0x01);
    }
    return ret;
}

DeviceError DT8824Client::adArmTrigger()
{
    auto err = DeviceError::OK;
    int err_cnt = scpiErrorCount();
    if (scpi_command("AD:TRIG:SOUR IMM;:AD:ARM;:AD:INIT")) {
        if (scpiErrorCount() > err_cnt) {
            err = DeviceError::SYSTEM;
        }
    } else {
        err = DeviceError::COMMUNICATION;
    }
    return err;
}

DeviceError DT8824Client::abort()
{
    return scpi_command(":AD:ABORt")  ? DeviceError::OK
                                      : DeviceError::COMMUNICATION;
}

//TODO: Implement FETCH command

void DT8824Client::connected()
{
    qInfo() << "Connected to DT8824 device";
}

void DT8824Client::disconnected()
{
    qInfo() << "Disconnected from DT8824 device";
}

DT8824Client::DT8824Client(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    QObject::connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
    QObject::connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

DT8824Client::~DT8824Client()
{
}

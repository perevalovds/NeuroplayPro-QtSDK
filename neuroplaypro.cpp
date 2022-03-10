#include "neuroplaypro.h"

// ===================== NeuroplayDevice ====================== //

NeuroplayDevice::NeuroplayDevice(const QJsonObject &json) :
    m_id(-1),
    m_isConnected(false),
    m_isStarted(false),
    m_grabFilteredData(false), m_grabRawData(false), m_grabRhythms(false), m_grabMeditation(false), m_grabConcentration(false),
    m_meditation(0), m_concentration(0)
{
    m_name = json["name"].toString();
    m_model = json["model"].toString();
    m_serialNumber = json["serialNumber"].toString();
    m_maxChannels = json["maxChannels"].toInt();
    m_preferredChannelCount = json["preferredChannelCount"].toInt();
    QJsonArray arr = json["channelModes"].toArray();
    for (QJsonValueRef value: arr)
    {
        QJsonObject o = value.toObject();
        int channels = o["channels"].toInt();
        int frequency = o["frequency"].toInt();
        m_channelModes << QPair<int, int>(channels, frequency);
    }
    m_grabTimer = new QTimer(this);
    connect(m_grabTimer, &QTimer::timeout, this, &NeuroplayDevice::grabRequest);

    connect(this, &QObject::destroyed, this, &NeuroplayDevice::stop);
}

QStringList NeuroplayDevice::channelModes() const
{
    QStringList list;
    for (auto mode: m_channelModes)
        list << QString("%1ch@%2Hz").arg(mode.first).arg(mode.second);
    return list;
}

void NeuroplayDevice::makeFavorite()
{
    request({{"command", "makefavorite"}, {"value", m_name}});
    request("getfavoritedevicename");
}

void NeuroplayDevice::grabFilteredData(bool enable)
{
    m_grabFilteredData = enable;
    switchGrabMode();
}

void NeuroplayDevice::grabRawData(bool enable)
{
    m_grabRawData = enable;
    switchGrabMode();
}

void NeuroplayDevice::grabRhythmsHistory(bool enable)
{
    m_grabRhythms = enable;
    switchGrabMode();
}

void NeuroplayDevice::grabMeditationHistory(bool enable)
{
    m_grabMeditation = enable;
    switchGrabMode();
}

void NeuroplayDevice::grabConcentrationHistory(bool enable)
{
    m_grabConcentration = enable;
    switchGrabMode();
}

NeuroplayDevice::ChannelsData NeuroplayDevice::readFilteredDataHistory()
{
    ChannelsData result;
    while (!m_filteredDataBuffer.isEmpty())
    {
        QVector<double> entry = m_filteredDataBuffer.dequeue();
        int chnum = entry.size();
        if (result.size() < chnum)
            result.resize(chnum);
        for (int i=0; i<chnum; i++)
            result[i] << entry[i];
    }
    return result;
}

NeuroplayDevice::ChannelsData NeuroplayDevice::readRawDataHistory()
{
    ChannelsData result;
    while (!m_rawDataBuffer.isEmpty())
    {
        QVector<double> entry = m_rawDataBuffer.dequeue();
        int chnum = entry.size();
        if (result.size() < chnum)
            result.resize(chnum);
        for (int i=0; i<chnum; i++)
            result[i] << entry[i];
    }
    return result;
}

QVector<NeuroplayDevice::ChannelsRhythms> NeuroplayDevice::readRhythmsHistory()
{
    QVector<ChannelsRhythms> result;
    while (!m_rhythmsBuffer.isEmpty())
    {
        result << m_rhythmsBuffer.dequeue();
    }
    return result;
}

QVector<NeuroplayDevice::TimedValue> NeuroplayDevice::readMeditationHistory()
{
    QVector<TimedValue> result;
    while (!m_meditationBuffer.isEmpty())
    {
        result << m_meditationBuffer.dequeue();
    }
    return result;
}

QVector<NeuroplayDevice::TimedValue> NeuroplayDevice::readConcentrationHistory()
{
    QVector<TimedValue> result;
    while (!m_concentrationBuffer.isEmpty())
    {
        result << m_concentrationBuffer.dequeue();
    }
    return result;
}

void NeuroplayDevice::setGrabInterval(int value_ms)
{
    m_grabIntervalMs = value_ms;
    m_grabTimer->setInterval(m_grabIntervalMs);
}

void NeuroplayDevice::start()
{
    m_isStarted = false;
    request({{"command", "startdevice"}, {"sn", m_serialNumber}});
}

void NeuroplayDevice::start(int channelNumber)
{
    m_isStarted = false;
    request({{"command", "startdevice"}, {"sn", m_serialNumber}, {"channels", channelNumber}});
}

void NeuroplayDevice::stop()
{
    m_isStarted = false;
    m_grabFilteredData = false;
    m_grabRawData = false;
    m_grabRhythms = false;
    m_grabMeditation = false;
    m_grabConcentration = false;
    switchGrabMode();
    request("stopdevice");
}

void NeuroplayDevice::setStarted(bool started)
{
    bool need_emit = !m_isStarted && started;
    m_isStarted = started;
    if (started)
        request("spectrumfrequencies");
    if (need_emit)
        emit ready();
}

void NeuroplayDevice::request(QString text)
{
    emit doRequest(text);
}

void NeuroplayDevice::request(QJsonObject json)
{
    request(QJsonDocument(json).toJson());
}

void NeuroplayDevice::onResponse(QJsonObject resp)
{
    QString cmd = resp["command"].toString();
    qDebug() << "received " << cmd;

    if (cmd == "lastspectrum")
    {
        m_spectrum.clear();
        QJsonArray arr = resp["spectrum"].toArray();
        for (QJsonValueRef ch: arr)
        {
            QVector<double> array;
            for (QJsonValueRef val: ch.toArray())
                array << val.toDouble();
            m_spectrum << array;
        }
        emit spectrumReady();
    }
    else if (cmd == "spectrumfrequencies")
    {
        m_spectrumFrequencies.clear();
        for (QJsonValueRef val: resp["spectrum"].toArray())
            m_spectrumFrequencies << val.toDouble();
    }
    else if (cmd == "rhythms")
    {
        m_rhythms.clear();
        QJsonArray arr = resp["rhythms"].toArray();
        for (QJsonValueRef ch: arr)
        {
            QJsonObject o = ch.toObject();
            Rhythms r;
            r.delta = o["delta"].toDouble();
            r.theta = o["theta"].toDouble();
            r.alpha = o["alpha"].toDouble();
            r.beta = o["beta"].toDouble();
            r.gamma = o["gamma"].toDouble();
            r.timestamp = o["t"].toInt();
            m_rhythms << r;
        }
        emit rhythmsReady();
    }
    else if (cmd == "meditation")
    {
        m_meditation = resp["meditation"].toDouble();
        emit meditationReady();
    }
    else if (cmd == "concentration")
    {
        m_concentration = resp["concentration"].toDouble();
        emit concentrationReady();
    }
    else if (cmd == "bci")
    {
        m_meditation = resp["meditation"].toDouble();
        m_concentration = resp["concentration"].toDouble();
        emit bciReady();
    }
    else if (cmd == "enabledatagrabmode")
    {
        m_grabTimer->start(m_grabIntervalMs);
    }
    else if (cmd == "disabledatagrabmode")
    {
        m_grabTimer->stop();
    }
    else if (cmd == "filtereddata")
    {
        ChannelsData data;
        QJsonArray arr = resp["data"].toArray();
        for (QJsonValueRef ch: arr)
        {
            QVector<double> array;
            for (QJsonValueRef val: ch.toArray())
                array << val.toDouble();
            data << array;
        }
        emit filteredDataReceived(data);
    }
    else if (cmd == "rawdata")
    {
        ChannelsData data;
        QJsonArray arr = resp["data"].toArray();
        for (QJsonValueRef ch: arr)
        {
            QVector<double> array;
            for (QJsonValueRef val: ch.toArray())
                array << val.toDouble();
            data << array;
        }
        emit rawDataReceived(data);
    }
    else if (cmd == "grabfiltereddata")
    {
        QJsonArray arr = resp["data"].toArray();
        int chnum = arr.size();
        int count = arr[0].toArray().size();
        for (int i=0; i<count; i++)
        {
            QVector<double> entry;
            for (int j=0; j<chnum; j++)
            {
                entry << arr[j].toArray()[i].toDouble();
            }
            m_filteredDataBuffer.enqueue(entry);
        }
    }
    else if (cmd == "grabrawdata")
    {
        QJsonArray arr = resp["data"].toArray();
        int chnum = arr.size();
        int count = arr[0].toArray().size();
        for (int i=0; i<count; i++)
        {
            QVector<double> entry;
            for (int j=0; j<chnum; j++)
            {
                entry << arr[j].toArray()[i].toDouble();
            }
            m_rawDataBuffer.enqueue(entry);
        }
    }
    else if (cmd == "rhythmshistory")
    {
        QJsonArray history = resp["history"].toArray();
        for (QJsonValueRef entry: history)
        {
            ChannelsRhythms chr;
            QJsonArray arr = entry.toArray();
            for (QJsonValueRef ch: arr)
            {
                QJsonObject o = ch.toObject();
                Rhythms r;
                r.delta = o["delta"].toDouble();
                r.theta = o["theta"].toDouble();
                r.alpha = o["alpha"].toDouble();
                r.beta = o["beta"].toDouble();
                r.gamma = o["gamma"].toDouble();
                r.timestamp = o["t"].toInt();
                chr << r;
            }
            m_rhythms = chr;
            m_rhythmsBuffer.enqueue(chr);
        }
    }
    else if (cmd == "meditationhistory")
    {
        QJsonArray history = resp["history"].toArray();
        for (QJsonValueRef entry: history)
        {
            QJsonObject o = entry.toObject();
            TimedValue tv;
            tv.value = o["v"].toDouble();
            tv.timestamp = o["t"].toInt();
            m_meditation = tv.value;
            m_meditationBuffer.enqueue(tv);
        }
    }
    else if (cmd == "concentrationhistory")
    {
        QJsonArray history = resp["history"].toArray();
        for (QJsonValueRef entry: history)
        {
            QJsonObject o = entry.toObject();
            TimedValue tv;
            tv.value = o["v"].toDouble();
            tv.timestamp = o["t"].toInt();
            m_concentration = tv.value;
            m_concentrationBuffer.enqueue(tv);
        }
    }
    else if (cmd == "stoprecord" && resp["result"].toBool())
    {
        QByteArray edf, npd;
        QJsonArray files = resp["files"].toArray();
        for (QJsonValueRef file: files)
        {
            QJsonObject o = file.toObject();
            if (o["type"] == "edf")
                edf = QByteArray::fromBase64(o["data"].toString().toLocal8Bit());
            if (o["type"] == "npd")
                npd = QByteArray::fromBase64(o["data"].toString().toLocal8Bit());
        }
        emit recordedData(edf, npd);
    }
}

void NeuroplayDevice::switchGrabMode()
{
    bool enable = false;
    if (m_grabFilteredData || m_grabRawData || m_grabRhythms || m_grabMeditation || m_grabConcentration)
        enable = true;

    if (enable && !m_grabTimer->isActive())
        request("enabledatagrabmode");
    else if (!enable)
    {
        request("disabledatagrabmode");
        m_grabTimer->stop();
    }
}

void NeuroplayDevice::grabRequest()
{
    if (m_grabFilteredData)
        request("grabrawdata");
    if (m_grabRawData)
        request("graboriginaldata");
    if (m_grabRhythms)
        request("rhythmsHistory");
    if (m_grabMeditation)
        request("meditationHistory");
    if (m_grabConcentration)
        request("concentrationHistory");
}

// ====================== NeuroplayPro ========================= //

NeuroplayPro::NeuroplayPro(QObject *parent) : QObject(parent),
    m_state(Disconnected),
    m_isDataGrab(false),
    m_currentDevice(nullptr),
    m_LPF(0), m_HPF(0), m_BSF(0),
    m_dataStorageTime(0)
{
    socket = new QWebSocket();
    connect(socket, &QWebSocket::connected, [=]()
    {
        m_state = Connected;
        send("help");
    });
    connect(socket, &QWebSocket::disconnected, [=]()
    {
        m_state = Disconnected;
        emit disconnected();
    });
    connect(socket, &QWebSocket::textMessageReceived, this, &NeuroplayPro::onSocketResponse);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(200);
    connect(m_searchTimer, &QTimer::timeout, [=]()
    {
        send("listdevices");
    });

    m_devStartTimer = new QTimer(this);
    m_devStartTimer->setInterval(200);
    connect(m_devStartTimer, &QTimer::timeout, [=]()
    {
        send("currentdeviceinfo");
    });
}

NeuroplayPro::~NeuroplayPro()
{
    close();
}

void NeuroplayPro::open()
{
    socket->open(QUrl("ws://localhost:1336"));
}

void NeuroplayPro::close()
{
    for (NeuroplayDevice *dev: m_deviceList)
        dev->deleteLater();
    m_deviceMap.clear();
    m_deviceList.clear();
    socket->close();
}

void NeuroplayPro::send(QString cmd)
{
    qDebug() << "> " + cmd;
    socket->sendTextMessage(cmd);
//    emit response("> " + cmd);
}

void NeuroplayPro::send(QJsonObject obj)
{
    send(QJsonDocument(obj).toJson());
}

NeuroplayDevice *NeuroplayPro::createDevice(const QJsonObject &o)
{
    NeuroplayDevice *dev = new NeuroplayDevice(o);
    qDebug() << "device created" << dev->name();
    QObject::connect(dev, SIGNAL(doRequest(QString)), this, SLOT(send(QString)));//, Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(responseJson(QJsonObject)), dev, SLOT(onResponse(QJsonObject)));//, Qt::QueuedConnection);
    dev->m_id = m_deviceList.size();
    m_deviceMap[dev->name()] = dev;
    m_deviceList << dev;
    emit deviceConnected(dev);
    return dev;
}

void NeuroplayPro::onSocketResponse(const QString &text)
{
    QJsonDocument json = QJsonDocument::fromJson(text.toUtf8());
    QJsonObject resp = json.object();
    QString cmd = resp["command"].toString();
    bool result = resp["result"].toBool();

    emit responseJson(resp);

    if (resp.contains("error") && m_state == Ready)
    {
        emit error(resp["error"].toString());
        return;
    }

    if (cmd == "help")
    {
        QString help;
        QJsonArray arr = resp["commands"].toArray();
        for (QJsonValueRef value: arr)
        {
            QJsonObject o = value.toObject();
            QString c = o["command"].toString();
            QString d = o["description"].toString();
            m_commands[c] = d;
            if (d.isEmpty())
                help += c + "\n";
            else
                help += c + " \t - " + m_commands[c] + "\n";
        }

        // aqcuire current settings:
        send("version");
        send("getfavoritedevicename");
        send("getfilters");
        send("getdatastoragetime");

        // aqcuire current connected device.
        // the search will started later if device is not connected
        send("currentdeviceinfo");

        bool need_emit = (m_state < Searching);
        m_state = Searching;
        if (need_emit)
        {
            emit connected();
            return;
        }

//        emit response(help);
        return;
    }
    else if (cmd == "version" && result)
    {
        m_version = resp["version"].toString();
    }
    else if (cmd == "getfavoritedevicename")
    {
        m_favoriteDeviceName = resp["device"].toString();
    }
    else if (cmd == "getfilters" || cmd == "setdefaultfilters")
    {
        m_LPF = resp["LPF"].toDouble(0);
        m_HPF = resp["HPF"].toDouble(0);
        m_BSF = resp["BSF"].toDouble(0);
    }
    else if (cmd == "getdatastoragetime")
    {
        m_dataStorageTime = resp["storagetime"].toInt();
    }
    else if (cmd == "startsearch")
    {
        for (NeuroplayDevice *dev: m_deviceList)
            dev->m_isConnected = false;
        m_searchTimer->start();
        QTimer::singleShot(6000, [=]()
        {
            m_searchTimer->stop();
            m_state = Ready;
        });
    }
    else if (cmd == "listdevices")
    {
        QJsonArray arr = resp["devices"].toArray();
        for (QJsonValueRef devjson: arr)
        {
            QJsonObject o = devjson.toObject();
            QString name = o["name"].toString();
            NeuroplayDevice *dev = nullptr;
            if (!m_deviceMap.contains(name))
            {
                dev = createDevice(o);
            }
            else
            {
                dev  = m_deviceMap[name];
            }
            dev->m_isConnected = true;
        }
    }
    else if (cmd == "startdevice")
    {
        m_searchTimer->stop();
        m_devStartTimer->start();
        QTimer::singleShot(6000, [=](){m_devStartTimer->stop();});
    }
    else if (cmd == "currentdeviceinfo")
    {
        if (resp["result"].toBool())
        {
            m_devStartTimer->stop();
            QJsonObject o = resp["device"].toObject();
            QString name = o["name"].toString();
            if (!m_deviceMap.contains(name))
            {
                createDevice(o);
            }
            m_currentDevice = m_deviceMap[name];
            m_currentDevice->setStarted();
            emit deviceReady(m_currentDevice);
        }
        else if (m_state < Ready)
        {
            send("startsearch");
        }
    }
    else if (cmd == "enabledatagrabmode")
    {
        m_isDataGrab = true;
    }
    else if (cmd == "disabledatagrabmode")
    {
        m_isDataGrab = false;
    }

//    emit response(text);
}

void NeuroplayPro::setLPF(double value)
{
    m_LPF = value;
    send({{"command", "setLPF"}, {"value", m_LPF}});
}

void NeuroplayPro::setHPF(double value)
{
    m_HPF = value;
    send({{"command", "setHPF"}, {"value", m_HPF}});
}

void NeuroplayPro::setBSF(double value)
{
    m_BSF = value;
    send({{"command", "setBSF"}, {"value", m_BSF}});
}

void NeuroplayPro::setFilters(double LPF, double HPF, double BSF)
{
    setLPF(LPF);
    setHPF(HPF);
    setBSF(BSF);
}

void NeuroplayPro::setDefaultFilters()
{
    send("setdefaultfilters");
}

void NeuroplayPro::setDataStorageTime(int seconds)
{
    //! @bug "value" value expected as string instead of number!
    send({{"command", "setdatastoragetime"}, {"value", QString::number(seconds)}});
}

void NeuroplayPro::enableDataGrabMode()
{
    send("enabledatagrabmode");
}

void NeuroplayPro::disableDataGrabMode()
{
    send("disabledatagrabmode");
}

void NeuroplayPro::setDataGrabMode(bool enabled)
{
    if (enabled)
        enableDataGrabMode();
    else
        disableDataGrabMode();
}

#include "neuroplaypro.h"

// ===================== NeuroplayDevice ====================== //

NeuroplayDevice::NeuroplayDevice(const QJsonObject &json) :
    m_id(-1),
    m_isConnected(false),
    m_isStarted(false),
    m_grabRawData(false), m_grabRhythms(false), m_grabMeditation(false), m_grabConcentration(false),
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

NeuroplayDevice::ChannelsData NeuroplayDevice::rawData()
{
    m_grabRawData = true;
    enableGrabMode();

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

QVector<NeuroplayDevice::ChannelsRhythms> NeuroplayDevice::rhythmsHistory()
{
    m_grabRhythms = true;
    enableGrabMode();
    QVector<ChannelsRhythms> result;
    while (!m_rhythmsBuffer.isEmpty())
    {
        result << m_rhythmsBuffer.dequeue();
    }
    return result;
}

QVector<NeuroplayDevice::TimedValue> NeuroplayDevice::meditationHistory()
{
    m_grabMeditation = true;
    enableGrabMode();
    QVector<TimedValue> result;
    while (!m_meditationBuffer.isEmpty())
    {
        result << m_meditationBuffer.dequeue();
    }
    return result;
}

QVector<NeuroplayDevice::TimedValue> NeuroplayDevice::concentrationHistory()
{
    m_grabConcentration = true;
    enableGrabMode();
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
    disableGrabMode();
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
//    m_busy = true;
    emit doRequest(text);
}

void NeuroplayDevice::request(QJsonObject json)
{
    request(QJsonDocument(json).toJson());
}

void NeuroplayDevice::onResponse(QJsonObject resp)
{
    QString cmd = resp["command"].toString();
//    m_busy = false;
    if (cmd == "spectrum")
    {
        m_spectrum.clear();
        QJsonArray arr = resp["spectrum"].toArray();
        for (QJsonValueRef ch: arr)
        {
            QVector<double> array;
            for (QJsonValueRef val: ch.toArray())
                array << val.toString().toDouble();
            m_spectrum << array;
        }
        emit spectrumReady();
    }
    else if (cmd == "spectrumfrequencies")
    {
        m_spectrumFrequencies.clear();
        for (QJsonValueRef val: resp["spectrum"].toArray())
            m_spectrumFrequencies << val.toString().toDouble();
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
    else if (cmd == "rawdata")
    {
        QJsonArray arr = resp["rawData"].toArray();
        int chnum = arr.size();
        int count = arr[0].toArray().size();
        for (int i=0; i<count; i++)
        {
            QVector<double> entry;
            for (int j=0; j<chnum; j++)
            {
                entry << arr[j].toArray()[i].toString().toDouble();
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
}

void NeuroplayDevice::enableGrabMode()
{
    if (!m_grabTimer->isActive())
        request("enabledatagrabmode");
}

void NeuroplayDevice::disableGrabMode()
{
    request("disabledatagrabmode");
    m_grabTimer->stop();
    m_grabRawData = false;
    m_grabRhythms = false;
    m_grabMeditation = false;
    m_grabConcentration = false;
}

void NeuroplayDevice::grabRequest()
{
    if (m_grabRawData)
        request("rawdata");
    if (m_grabRhythms)
        request("rhythmsHistory");
    if (m_grabMeditation)
        request("meditationHistory");
    if (m_grabConcentration)
        request("concentrationHistory");
}




// ====================== NeuroplayPro ========================= //

NeuroplayPro::NeuroplayPro(QObject *parent) : QObject(parent),
    m_isConnected(false),
    m_isDataGrab(false),
    m_currentDevice(nullptr),
    m_LPF(0), m_HPF(0), m_BSF(0)
{
    socket = new QWebSocket();
    connect(socket, &QWebSocket::connected, [=]()
    {
        send("help");
    });
    connect(socket, &QWebSocket::disconnected, [=]()
    {
        m_isConnected = false;
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
//    m_busy = true;
    socket->sendTextMessage(cmd);
    emit response("> " + cmd);
}

void NeuroplayPro::send(QJsonObject obj)
{
    send(QJsonDocument(obj).toJson());
}

void NeuroplayPro::onSocketResponse(const QString &text)
{
    QJsonDocument json = QJsonDocument::fromJson(text.toUtf8());
    QJsonObject resp = json.object();
    QString cmd = resp["command"].toString();

    emit responseJson(resp);

    if (resp.contains("error") && !m_devStartTimer->isActive())
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

        send("getfavoritedevicename");
        send("getfilters");

        send("startsearch");

        bool need_emit = !m_isConnected;
        m_isConnected = true;
        if (need_emit)
        {
            emit connected();
            return;
        }

        emit response(help);
        return;
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
    else if (cmd == "startsearch")
    {
        for (NeuroplayDevice *dev: m_deviceList)
            dev->m_isConnected = false;
        m_searchTimer->start();
        QTimer::singleShot(3000, [=](){m_searchTimer->stop();});
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
                dev = new NeuroplayDevice(o);
                QObject::connect(dev, SIGNAL(doRequest(QString)), this, SLOT(send(QString)), Qt::QueuedConnection);
                QObject::connect(this, SIGNAL(responseJson(QJsonObject)), dev, SLOT(onResponse(QJsonObject)), Qt::QueuedConnection);
                dev->m_id = m_deviceList.size();
                m_deviceMap[name] = dev;
                m_deviceList << dev;
                emit deviceConnected(dev);
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
        QTimer::singleShot(3000, [=](){m_devStartTimer->stop();});
    }
    else if (cmd == "currentdeviceinfo")
    {
        if (resp["result"].toBool())
        {
            m_devStartTimer->stop();
            QString name = resp["device"].toObject()["name"].toString();
            if (m_deviceMap.contains(name))
            {
                m_currentDevice = m_deviceMap[name];
                m_currentDevice->setStarted();
                emit deviceReady(m_currentDevice);
            }
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

//    m_busy = false;

    emit response(text);
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

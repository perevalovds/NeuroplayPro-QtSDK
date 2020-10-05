#ifndef NEUROPLAYPRO_H
#define NEUROPLAYPRO_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QVector>
#include <QQueue>

class NeuroplayDevice : public QObject
{
    Q_OBJECT
public:
    int id() const {return m_id;}
    const QString &name() const {return m_name;}
    const QString &model() const {return m_model;}
    const QString &serialNumber() const {return m_serialNumber;}
    int maxChannels() const {return m_maxChannels;}
    int preferredChannelCount() const {return m_preferredChannelCount;}
    const QVector< QPair<int, int> > &channelModesValues() const {return m_channelModes;}
    QStringList channelModes() const;

    void makeFavorite();

    typedef struct
    {
        double delta, theta, alpha, beta, gamma;
        int timestamp;
    } Rhythms;
    typedef struct
    {
        double value;
        int timestamp;
    } TimedValue;
    typedef QVector< QVector<double> > ChannelsData;
    typedef QVector<Rhythms> ChannelsRhythms;

    const ChannelsData &spectrum() const {return m_spectrum;}
    const QVector<double> &spectrumFrequencies() const {return m_spectrumFrequencies;}
    const ChannelsRhythms &rhythms() const {return m_rhythms;}
    double meditation() const {return m_meditation;}
    double concentration() const {return m_concentration;}

    bool isConnected() const {return m_isConnected;}
    bool isStarted() const {return m_isStarted;}

    void grabFilteredData(bool enable = true);
    void grabRawData(bool enable = true);
    void grabRhythmsHistory(bool enable = true);
    void grabMeditationHistory(bool enable = true);
    void grabConcentrationHistory(bool enable = true);

    ChannelsData readFilteredDataHistory();
    ChannelsData readRawDataHistory();
    QVector<ChannelsRhythms> readRhythmsHistory();
    QVector<TimedValue> readMeditationHistory();
    QVector<TimedValue> readConcentrationHistory();

    void setGrabInterval(int value_ms);
    int grabInterval() const {return m_grabIntervalMs;}

public slots:
    void start();
    void start(int channelNumber);
    void stop();

    void startRecord() {request("startrecord");}
    void stopRecord() {request("stoprecord");}

    void requestFilteredData()  {request("filtereddata");}
    void requestRawData()       {request("rawdata");}
    void requestSpectrum()      {request("spectrum");}
    void requestRhythms()       {request("rhythms");}
    void requestMeditation()    {request("meditation");}
    void requestConcentration() {request("concentration");}
    void requestBCI()           {request("BCI");}

signals:
    void ready();

    void filteredDataReceived(ChannelsData data);
    void rawDataReceived(ChannelsData data);
    void spectrumReady();
    void rhythmsReady();
    void meditationReady();
    void concentrationReady();
    void bciReady();

    void recordedData(QByteArray edf, QByteArray npd);

private:
    int m_id;
    QString m_name;
    QString m_model;
    QString m_serialNumber;
    int m_maxChannels;
    int m_preferredChannelCount;
    QVector< QPair<int, int> > m_channelModes;
    friend class NeuroplayPro;
    NeuroplayDevice(const QJsonObject &json);

    bool m_isConnected;
    bool m_isStarted;

    bool m_grabFilteredData;
    bool m_grabRawData;
    bool m_grabRhythms;
    bool m_grabMeditation;
    bool m_grabConcentration;

    QVector<double> m_spectrumFrequencies;
    ChannelsData m_spectrum;
    ChannelsRhythms m_rhythms;
    double m_meditation;
    double m_concentration;

    QQueue< QVector<double> > m_filteredDataBuffer;
    QQueue< QVector<double> > m_rawDataBuffer;
    QQueue<ChannelsRhythms> m_rhythmsBuffer;
    QQueue<TimedValue> m_meditationBuffer;
    QQueue<TimedValue> m_concentrationBuffer;
    QTimer *m_grabTimer;
    int m_grabIntervalMs = 50;

    void setStarted(bool started = true);

    void request(QString text);
    void request(QJsonObject json);

    void switchGrabMode();

signals: // private
    void doRequest(QString text);

private slots:
    void onResponse(QJsonObject resp);
    void grabRequest();
};



// ============================================================== //

class NeuroplayPro : public QObject
{
    Q_OBJECT
public:
    enum State {Disconnected, Connected, Searching, Ready};

    explicit NeuroplayPro(QObject *parent = nullptr);
    virtual ~NeuroplayPro();
    State state() const {return m_state;}
    bool isConnected() const {return m_state >= Connected;}
    bool isDataGrabMode() const {return m_isDataGrab;}
    int deviceCount() const {return m_deviceList.size();}
    NeuroplayDevice *device(int id) {return (id >= 0 && id < deviceCount())? m_deviceList[id]: nullptr;}
    NeuroplayDevice *device(QString sn) {return m_deviceMap.contains(sn)? m_deviceMap[sn]: nullptr;}
    NeuroplayDevice *currentDevice() {return m_currentDevice;}
    QString favoriteDeviceName() const {return m_favoriteDeviceName;}
    QString version() {return m_version;}

    double LPF() const {return m_LPF;}
    void setLPF(double value);
    double HPF() const {return m_HPF;}
    void setHPF(double value);
    double BSF() const {return m_BSF;}
    void setBSF(double value);
    void setFilters(double LPF, double HPF, double BSF);
    void setDefaultFilters();

    int dataStorageTime() const {return m_dataStorageTime;}
    void setDataStorageTime(int seconds);

public slots:
    void open();
    void close();
    void enableDataGrabMode();
    void disableDataGrabMode();
    void setDataGrabMode(bool enabled);

signals:
    void connected();
    void disconnected();
    void error(QString text);
    void deviceConnected(NeuroplayDevice *device);
    void deviceReady(NeuroplayDevice *device);

protected slots:
public slots:
    void send(QString cmd);

private:
    QWebSocket *socket;
    State m_state;
    bool m_isDataGrab;
    QMap<QString, QString> m_commands;
    QVector<NeuroplayDevice*> m_deviceList;
    QMap<QString, NeuroplayDevice*> m_deviceMap;
    QTimer *m_searchTimer;
    QTimer *m_devStartTimer;
    NeuroplayDevice *m_currentDevice;
    QString m_favoriteDeviceName;
    double m_LPF, m_HPF, m_BSF;
    int m_dataStorageTime;
    QString m_version;

    void send(QJsonObject obj);
    friend class NeuroplayDevice;

    NeuroplayDevice *createDevice(const QJsonObject &o);

private slots:
    void onSocketResponse(const QString &text);

signals:
    void responseJson(QJsonObject json);

};

#endif // NEUROPLAYPRO_H

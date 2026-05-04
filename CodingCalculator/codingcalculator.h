#ifndef CODINGCALCULATOR_H
#define CODINGCALCULATOR_H

#include <QObject>
#include <QByteArray>
#include <QVariantList>
#include <QStringList>
#include <QJsonObject>

struct CodingField;

struct Choice {
    quint32 value = 0;
    QString name;
    QList<CodingField*> children;
};

struct CodingField {
    QString id;
    QString name;
    QString type;
    quint32 bitOffset = 0;
    quint32 bitLength = 0;
    QList<Choice> options;
    QList<CodingField*> children;
};

struct DeviceInfo {
    QString id;
    QString name;
    quint32 codingBytes = 0;
    QString configPath;
    QString sampleFile;
};

struct SampleItem {
    QString id;
    QString name;
    QString codingHex;
};

class CodingCalculator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList deviceNames READ deviceNames NOTIFY devicesChanged)
    Q_PROPERTY(int currentDeviceIndex READ currentDeviceIndex NOTIFY currentDeviceChanged)
    Q_PROPERTY(QString currentCodingHex READ currentCodingHex NOTIFY currentCodingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QVariantList visibleFields READ visibleFields NOTIFY visibleFieldsChanged)
    Q_PROPERTY(QString currentSampleFile READ currentSampleFile NOTIFY currentDeviceChanged)
    Q_PROPERTY(QStringList sampleNames READ sampleNames NOTIFY samplesChanged)
    Q_PROPERTY(int currentSampleIndex READ currentSampleIndex NOTIFY currentSampleChanged)

public:
    explicit CodingCalculator(QObject* parent = nullptr);
    ~CodingCalculator() override;

    QStringList deviceNames() const;
    int currentDeviceIndex() const;
    QString currentCodingHex() const;
    QString statusMessage() const;
    QVariantList visibleFields() const;
    QString currentSampleFile() const;
    QStringList sampleNames() const;
    int currentSampleIndex() const;

    Q_INVOKABLE bool loadDevices();
    Q_INVOKABLE bool selectDevice(int index);
    Q_INVOKABLE bool setCodingHex(const QString& hex);
    Q_INVOKABLE bool updateFieldValue(const QString& fieldId, const QVariant &value);
    Q_INVOKABLE bool selectSample(int index);
    Q_INVOKABLE bool loadCurrentSample();
    Q_INVOKABLE bool saveCurrentSample();

signals:
    void devicesChanged();
    void currentDeviceChanged();
    void currentCodingChanged();
    void statusMessageChanged();
    void visibleFieldsChanged();
    void samplesChanged();
    void currentSampleChanged();

private:
    //Read configuration JSONs
    CodingField readFieldFromJson(const QJsonObject& obj) const;
    Choice readOptionFromJson(const QJsonObject& obj) const;

    bool loadCurrentConfig();
    bool loadSamples();

    //Updates which fields should be visible in the UI
    void refreshVisibleFields();
    void appendVisibleFields(QVariantList &list, const QList<CodingField*>& fields) const;
    QVariantMap buildTreeFromJson(const CodingField& field) const;

    CodingField* findField(const QString& fieldId);

    void resetFields(const QList<CodingField*>& fields);
    bool checkFieldTree(const QList<CodingField*>& fields, quint8 totalBits) const;
    void freeFieldTree();

    //Bit logic for decoding
    quint32 readBits(const QByteArray& coding, quint8 bitOffset, quint8 bitLength) const;
    void writeBits(QByteArray &coding, quint8 bitOffset, quint8 bitLength, quint32 value);

    QString cleanHex(const QString &hex) const;
    QString findProjectFolder() const;
    QString selectedSamplePath() const;
    void setStatus(const QString& text);

    QString m_projectRoot;
    QList<DeviceInfo> m_devices;
    int m_currentDeviceIndex = -1;
    QList<CodingField*> m_fields;
    QByteArray m_coding;
    QVariantList m_visibleFields;
    QString m_status;
    QList<SampleItem> m_samples;
    int m_currentSampleIndex = -1;
    bool m_hasLoadedCoding = false;
};


#endif // CODINGCALCULATOR_H

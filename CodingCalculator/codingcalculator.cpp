#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "codingcalculator.h"

CodingCalculator::CodingCalculator(QObject* parent)
    : QObject(parent)
{
}

CodingCalculator::~CodingCalculator()
{
    freeFieldTree();
}

QStringList CodingCalculator::deviceNames() const
{
    QStringList names;
    for (const auto& device : m_devices)
        names.append(device.name);
    return names;
}

int CodingCalculator::currentDeviceIndex() const
{
    return m_currentDeviceIndex;
}

QString CodingCalculator::currentCodingHex() const
{
    return QString::fromLatin1(m_coding.toHex().toUpper());
}

QString CodingCalculator::statusMessage() const
{
    return m_status;
}

QVariantList CodingCalculator::visibleFields() const
{
    return m_visibleFields;
}

QString CodingCalculator::currentSampleFile() const
{
    if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size())
        return {};

    return m_devices[m_currentDeviceIndex].sampleFile;
}

QStringList CodingCalculator::sampleNames() const
{
    QStringList names;
    for (const auto& sample : m_samples)
        names.append(sample.name);
    return names;
}

int CodingCalculator::currentSampleIndex() const
{
    return m_currentSampleIndex;
}

bool CodingCalculator::loadDevices()
{
    m_projectRoot = findProjectFolder();
    if (m_projectRoot.isEmpty()) {
        setStatus("Project folder not found");
        return false;
    }

    QFile file(QDir(m_projectRoot).filePath("configs/devices.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus("Could not open devices.json");
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        setStatus("devices.json is not valid");
        return false;
    }

    m_devices.clear();

    const QJsonArray devicesArray = doc.object().value("devices").toArray();
    for (const QJsonValue& value : devicesArray) {
        const QJsonObject obj = value.toObject();

        DeviceInfo device;
        device.id = obj.value("id").toString();
        device.name = obj.value("name").toString();
        device.codingBytes = obj.value("codingBytes").toInt();
        device.configPath = QDir(m_projectRoot).filePath("configs/" + obj.value("configFile").toString());
        device.sampleFile = obj.value("sampleFile").toString();

        if (!device.id.isEmpty() && !device.name.isEmpty() && device.codingBytes > 0 && !device.configPath.isEmpty())
            m_devices.append(device);
    }

    if (m_devices.isEmpty()) {
        setStatus("No devices found");
        return false;
    }

    emit devicesChanged();
    return true;
}

bool CodingCalculator::selectDevice(int index)
{
    if (index < 0 || index >= m_devices.size()) {
        setStatus("Wrong device index");
        return false;
    }

    m_currentDeviceIndex = index;
    if (!loadCurrentConfig())
        return false;

    m_coding = QByteArray(m_devices[m_currentDeviceIndex].codingBytes, char(0));
    resetFields(m_fields);
    m_hasLoadedCoding = false;
    m_visibleFields.clear();
    loadSamples();
    emit currentDeviceChanged();
    emit currentCodingChanged();
    emit visibleFieldsChanged();
    setStatus("Device selected");
    return true;
}

bool CodingCalculator::setCodingHex(const QString& hex)
{
    if (m_currentDeviceIndex < 0) {
        setStatus("Pick a device first");
        return false;
    }

    const QString value = cleanHex(hex);
    const int expected = m_devices[m_currentDeviceIndex].codingBytes * 2;
    if (value.size() != expected) {
        setStatus(QString("Expected %1 hex characters").arg(expected));
        return false;
    }

    const QByteArray bytes = QByteArray::fromHex(value.toLatin1());
    if (bytes.size() != m_devices[m_currentDeviceIndex].codingBytes) {
        setStatus("Invalid hex value");
        return false;
    }

    m_coding = bytes;
    m_hasLoadedCoding = true;
    refreshVisibleFields();
    emit currentCodingChanged();
    setStatus("Coding updated");
    return true;
}

bool CodingCalculator::updateFieldValue(const QString& fieldId, const QVariant& value)
{
    if (m_currentDeviceIndex < 0) {
        setStatus("Pick a device first");
        return false;
    }

    if (!m_hasLoadedCoding) {
        setStatus("Decode or load coding first");
        return false;
    }

    CodingField* field = findField(fieldId);
    if (!field) {
        setStatus("Unknown field");
        return false;
    }

    if (field->type == "bool") {
        const int boolValue = value.toInt() ? 1 : 0;
        writeBits(m_coding, field->bitOffset, field->bitLength, boolValue);
        if (boolValue == 0)
            resetFields(field->children);
    } else if (field->type == "enum") {
        const int selectedValue = value.toInt();
        writeBits(m_coding, field->bitOffset, field->bitLength, quint32(selectedValue));
        for (const Choice& option : field->options) {
            if (option.value != selectedValue)
                resetFields(option.children);
        }
    } else if (field->type == "uint") {
        quint32 val = value.toUInt();
        quint32 max = (1 << field->bitLength) - 1;

        if (val > max)
            return false;

        writeBits(m_coding, field->bitOffset, field->bitLength, val);

        if (val == 0)
            resetFields(field->children);
    } else {
        setStatus("Field type is not editable");
        return false;
    }

    refreshVisibleFields();
    emit currentCodingChanged();
    setStatus("Field updated");
    return true;
}

bool CodingCalculator::selectSample(int index)
{
    if (index < 0 || index >= m_samples.size())
        return false;

    if (m_currentSampleIndex == index)
        return true;

    m_currentSampleIndex = index;
    emit currentSampleChanged();
    return true;
}

bool CodingCalculator::loadCurrentSample()
{
    if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size()) {
        setStatus("Pick a device first");
        return false;
    }

    if (m_currentSampleIndex < 0 || m_currentSampleIndex >= m_samples.size()) {
        setStatus("Pick a sample first");
        return false;
    }

    const bool ok = setCodingHex(m_samples[m_currentSampleIndex].codingHex);
    if (ok)
        setStatus("Sample loaded");
    return ok;
}

bool CodingCalculator::saveCurrentSample()
{
    if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size()) {
        setStatus("Pick a device first");
        return false;
    }

    if (m_currentSampleIndex < 0 || m_currentSampleIndex >= m_samples.size()) {
        setStatus("Pick a sample first");
        return false;
    }

    if (!m_hasLoadedCoding) {
        setStatus("Nothing to save yet");
        return false;
    }

    const QString filePath = selectedSamplePath();
    if (filePath.isEmpty()) {
        setStatus("Sample file is missing");
        return false;
    }

    m_samples[m_currentSampleIndex].codingHex = currentCodingHex();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setStatus("Could not save sample file");
        return false;
    }

    QJsonArray items;
    for (const auto& sample : m_samples) {
        QJsonObject item;
        item.insert("id", sample.id);
        item.insert("name", sample.name);
        item.insert("codingHex", sample.codingHex);
        items.append(item);
    }

    QJsonObject root;
    root.insert("deviceId", m_devices[m_currentDeviceIndex].id);
    root.insert("samples", items);

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.flush();
    file.close();

    setStatus("Sample saved");
    return true;
}

CodingField CodingCalculator::readFieldFromJson(const QJsonObject& obj) const
{
    CodingField field;
    field.id = obj.value("id").toString();
    field.name = obj.value("name").toString();
    field.type = obj.value("type").toString();
    field.bitOffset = obj.value("bitOffset").toInt();
    field.bitLength = obj.value("bitLength").toInt();

    const QJsonArray optionsArray = obj.value("options").toArray();
    for (const QJsonValue& optionValue : optionsArray)
        field.options.append(readOptionFromJson(optionValue.toObject()));

    const QJsonArray childrenArray = obj.value("children").toArray();
    for (const QJsonValue& childValue : childrenArray)
        field.children.append(new CodingField(readFieldFromJson(childValue.toObject())));

    return field;
}

Choice CodingCalculator::readOptionFromJson(const QJsonObject& obj) const
{
    Choice option;
    option.value = obj.value("value").toInt();
    option.name = obj.value("name").toString();

    const QJsonArray childrenArray = obj.value("children").toArray();
    for (const QJsonValue& childValue : childrenArray)
        option.children.append(new CodingField(readFieldFromJson(childValue.toObject())));

    return option;
}

bool CodingCalculator::loadCurrentConfig()
{
    if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size())
        return false;

    QFile file(m_devices[m_currentDeviceIndex].configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus("Could not open config file");
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        setStatus("Config file is not valid");
        return false;
    }

    freeFieldTree();
    const QJsonArray fieldsArray = doc.object().value("fields").toArray();
    for (const QJsonValue& fieldValue : fieldsArray)
        m_fields.append(new CodingField(readFieldFromJson(fieldValue.toObject())));

    const int totalBits = m_devices[m_currentDeviceIndex].codingBytes * 8;
    if (!checkFieldTree(m_fields, totalBits)) {
        freeFieldTree();
        setStatus("Config has fields outside coding length");
        return false;
    }

    return true;
}


void CodingCalculator::refreshVisibleFields()
{
    QVariantList items;
    appendVisibleFields(items, m_fields);
    m_visibleFields = items;
    emit visibleFieldsChanged();
}

void CodingCalculator::appendVisibleFields(QVariantList& list, const QList<CodingField*>& fields) const
{
    for (CodingField* field : fields) {
        if (!field)
            continue;

        list.append(buildTreeFromJson(*field));

        if (field->type == "bool") {
            const quint32 isSwitchOn = readBits(m_coding, field->bitOffset, field->bitLength);
            if (isSwitchOn != 0)
                appendVisibleFields(list, field->children);
        }

        if (field->type == "enum") {
            const quint32 selectedValue = readBits(m_coding, field->bitOffset, field->bitLength);
            for (const Choice& option : field->options) {
                if (option.value == selectedValue) {
                    appendVisibleFields(list, option.children);
                    break;
                }
            }
        }

        if (field->type == "uint") {
            const quint32 number = readBits(m_coding, field->bitOffset, field->bitLength);
            if (number != 0)
                appendVisibleFields(list, field->children);
        }
    }
}

QVariantMap CodingCalculator::buildTreeFromJson(const CodingField& field) const
{
    QVariantMap data;
    data.insert("id", field.id);
    data.insert("name", field.name);
    data.insert("type", field.type);
    data.insert("bitLength", field.bitLength);
    data.insert("currentValue", readBits(m_coding, field.bitOffset, field.bitLength));

    quint32 maxValue = (1 << field.bitLength) - 1;

    data.insert("maxValue", QString::number(maxValue));

    QVariantList options;
    for (const Choice& option : field.options) {
        QVariantMap item;
        item.insert("value", option.value);
        item.insert("name", option.name);
        options.append(item);
    }

    data.insert("options", options);
    return data;
}

CodingField* CodingCalculator::findField(const QString& fieldId)
{
    QList<CodingField*> stack = m_fields;

    while (!stack.isEmpty()) {
        CodingField* field = stack.takeLast();
        if (!field)
        {
            continue;
        }
        if (field->id == fieldId)
            return field;

        if (field->type == "bool") {
            const quint32 enabled = readBits(m_coding, field->bitOffset, field->bitLength);
            if (enabled != 0) {
                for (int i = field->children.size() - 1; i >= 0; --i)
                    stack.append(field->children[i]);
            }
        }

        if (field->type == "enum") {
            const quint32 selectedValue = readBits(m_coding, field->bitOffset, field->bitLength);
            for (const Choice& option : field->options) {
                if (option.value == selectedValue) {
                    for (int i = option.children.size() - 1; i >= 0; --i)
                        stack.append(option.children[i]);
                    break;
                }
            }
        }

        if (field->type == "uint") {
            const quint32 numValue = readBits(m_coding, field->bitOffset, field->bitLength);
            if (numValue != 0) {
                for (int i = field->children.size() - 1; i >= 0; --i)
                    stack.append(field->children[i]);
            }
        }
    }

    return nullptr;
}

void CodingCalculator::resetFields(const QList<CodingField*>& fields)
{
    for (CodingField* field : fields) {
        if (!field)
            continue;

        if (field->type == "bool") {
            writeBits(m_coding, field->bitOffset, field->bitLength, 0);
            resetFields(field->children);
        }

        if (field->type == "enum") {
            quint32 defaultValue = 0;
            if (!field->options.isEmpty())
                defaultValue = field->options.first().value;

            writeBits(m_coding, field->bitOffset, field->bitLength, quint32(defaultValue));
            for (const Choice& option : field->options)
                resetFields(option.children);
        }

        if (field->type == "uint") {
            writeBits(m_coding, field->bitOffset, field->bitLength, 0);
            resetFields(field->children);
        }
    }
}

bool CodingCalculator::checkFieldTree(const QList<CodingField*>& fields, quint8 totalBits) const
{
    QList<CodingField*> stack = fields;

    while (!stack.isEmpty()) {
        CodingField* field = stack.takeLast();
        if (!field)
            return false;

        if (field->bitOffset < 0 || field->bitLength <= 0 || field->bitOffset + field->bitLength > totalBits)
            return false;

        if (field->type == "enum") {
            const quint32 valueLimit = 1 << field->bitLength;
            for (const Choice& choice : field->options) {
                if (choice.value < 0 || choice.value >= valueLimit)
                    return false;

                for (CodingField* child : choice.children)
                    stack.append(child);
            }
            continue;
        }

        for (CodingField* child : field->children)
            stack.append(child);
    }

    return true;
}

void CodingCalculator::freeFieldTree()
{
    QList<CodingField*> stack = m_fields;

    while (!stack.isEmpty()) {
        CodingField* field = stack.takeLast();
        if (!field)
            continue;

        for (CodingField* child : field->children)
            stack.append(child);

        for (const Choice& choice : field->options) {
            for (CodingField* child : choice.children)
                stack.append(child);
        }

        delete field;
    }

    m_fields.clear();
}

quint32 CodingCalculator::readBits(const QByteArray& coding, quint8 bitOffset, quint8 bitLength) const
{
    quint32 res = 0;

    for (int i = 0; i < bitLength; ++i) {
        const quint8 bitPos = bitOffset + i;
        const quint32 byteIndex = coding.size() - 1 - (bitPos / 8);

        if (byteIndex >= 0 && byteIndex < coding.size()) {
            const quint32 bitIndex = bitPos % 8;
            const quint8 byte = static_cast<quint8>(coding[byteIndex]);
            const quint32 bit = (byte >> bitIndex) & 1;
            res |= (bit << i);
        }
    }

    return res;
}

void CodingCalculator::writeBits(QByteArray& coding, quint8 bitOffset, quint8 bitLength, quint32 value)
{
    for (int i = 0; i < bitLength; ++i) {
        const qint8 bitPos = bitOffset + i;
        const qint32 byteIndex = coding.size() - 1 - (bitPos / 8);

        if (byteIndex >= 0 && byteIndex < coding.size()) {
            const qint32 bitIndex = bitPos % 8;
            quint8 byte = static_cast<quint8>(coding[byteIndex]);
            const quint8 mask = static_cast<quint8>(1 << bitIndex);

            if ((value >> i) & 1)
                byte |= mask;
            else
                byte &= static_cast<quint8>(~mask);

            coding[byteIndex] = static_cast<char>(byte);
        }
    }
}

QString CodingCalculator::cleanHex(const QString& hex) const
{
    QString value = hex.trimmed().toUpper();
    value.remove(' ');

    for (const QChar ch : value) {
        const bool valid = (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
        if (!valid)
            return {};
    }

    return value;
}

QString CodingCalculator::findProjectFolder() const
{
    QDir dir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < 8; ++i) {
        const bool hasDevices = QFileInfo::exists(dir.filePath("configs/devices.json"));
        const bool hasSamples = QFileInfo(dir.filePath("samples")).isDir();
        const bool hasCmake = QFileInfo::exists(dir.filePath("CMakeLists.txt"));

        if (hasDevices && hasSamples && hasCmake)
            return dir.absolutePath();

        if (!dir.cdUp())
            break;
    }

    return {};
}

QString CodingCalculator::selectedSamplePath() const
{
    if (m_projectRoot.isEmpty() || m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size())
        return {};

    const QString fileName = QFileInfo(m_devices[m_currentDeviceIndex].sampleFile).fileName();
    if (fileName.isEmpty())
        return {};

    return QDir(m_projectRoot).filePath("samples/" + fileName);
}

bool CodingCalculator::loadSamples()
{
    m_samples.clear();
    m_currentSampleIndex = -1;

    const QString filePath = selectedSamplePath();
    if (filePath.isEmpty()) {
        emit samplesChanged();
        emit currentSampleChanged();
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatus("Could not open sample file");
        emit samplesChanged();
        emit currentSampleChanged();
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        setStatus("Sample file is not valid");
        emit samplesChanged();
        emit currentSampleChanged();
        return false;
    }

    const QJsonObject root = doc.object();
    const QString deviceId = root.value("deviceId").toString();
    if (!deviceId.isEmpty() && deviceId != m_devices[m_currentDeviceIndex].id) {
        setStatus("Sample file belongs to another device");
        emit samplesChanged();
        emit currentSampleChanged();
        return false;
    }

    const QJsonArray samplesArray = root.value("samples").toArray();
    for (const QJsonValue& value : samplesArray) {
        const QJsonObject obj = value.toObject();
        SampleItem sample;
        sample.id = obj.value("id").toString();
        sample.name = obj.value("name").toString();
        sample.codingHex = obj.value("codingHex").toString();
        if (!sample.name.isEmpty() && !sample.codingHex.isEmpty())
            m_samples.append(sample);
    }

    emit samplesChanged();
    emit currentSampleChanged();
    return true;
}

void CodingCalculator::setStatus(const QString& text)
{
    if (m_status != text)
    {
        m_status = text;
        emit statusMessageChanged();
    }
}


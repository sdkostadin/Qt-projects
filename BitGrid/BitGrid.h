#ifndef BITGRID_H
#define BITGRID_H
#include <QObject>
class BitGrid : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int rawValue READ rawValue WRITE setRawValue NOTIFY valueChanged)
    Q_PROPERTY(QString hex READ hex NOTIFY valueChanged)
    Q_PROPERTY(QString binary READ binary NOTIFY valueChanged)

public:
    explicit BitGrid(QObject *parent = nullptr)
        : QObject(parent), m_value(0)
    {}

    int rawValue() const
    {
        return m_value;
    }

    void setRawValue(int value)
    {
        value &= 0xFFFF;

        if (m_value == value)
            return;

        m_value = value;
        emit valueChanged();
    }

    QString hex() const
    {
        QString h = QString::number(m_value, 16).toUpper();
        return "0x" + h.rightJustified(4, '0');
    }

    QString binary() const
    {
        QString result;

        for (int i = 15; i >= 0; --i) {
            result += ((m_value >> i) & 1) ? '1' : '0';

            //if (i % 4 == 0 && i != 0)
            //    result += ' ';
        }

        return result;
    }

    Q_INVOKABLE void toggleBit(int index)
    {
        if (index < 0 || index >= 16)
            return;

        setRawValue(m_value ^ (1 << index));
        emit valueChanged();
    }

    Q_INVOKABLE void reset()
    {
        setRawValue(0);
    }

signals:
    void valueChanged();

private:
    int m_value;
};
#endif // BITGRID_H

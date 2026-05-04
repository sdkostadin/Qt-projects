import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 620
    height: 360
    title: "Bit Grid Editor"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        Label {
            text: "16-bit Bit Grid Editor"
            font.pixelSize: 22
            font.bold: true
        }

        GridLayout {
            columns: 8
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: 16

                delegate: Button {
                    property int bitIndex: 15 - index

                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 50

                    //text: bitIndex + "\n" + ((BitGrid.rawValue >> bitIndex) & 1)
                    text:bitIndex +"\n" + BitGrid.binary[index]

                    onClicked: {
                        BitGrid.toggleBit(bitIndex)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#cccccc"
        }

        RowLayout {
            spacing: 12

            Label {
                text: "Hex:"
                Layout.preferredWidth: 90
            }

            Label {
                text: BitGrid.hex
                font.bold: true
            }
        }

        RowLayout {
            spacing: 12

            Label {
                text: "Decimal:"
                Layout.preferredWidth: 90
            }

            Label {
                text: BitGrid.rawValue
                font.bold: true
            }
        }

        RowLayout {
            spacing: 12

            Label {
                text: "Binary:"
                Layout.preferredWidth: 90
            }

            Label {
                text: BitGrid.binary
                font.family: "monospace"
                font.bold: true
            }
        }

        Button {
            text: "Reset"
            contentItem: Text
            {
                text: parent.text
                color:"black"
            }
            onClicked: BitGrid.reset()
        }
    }
}
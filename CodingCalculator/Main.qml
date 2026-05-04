import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1000
    height: 700
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: "Coding Calculator"
    color: "black"

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: "Device"
                color: "white"
                font.pixelSize: 15
                Layout.preferredWidth: 100
            }

            ComboBox {
                id: deviceCombo
                Layout.preferredWidth: 250
                model: codingCalculator.deviceNames
                currentIndex: -1
                onActivated: codingCalculator.selectDevice(currentIndex)
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: codingCalculator.statusMessage
                color: "white"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                Layout.preferredWidth: 350
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: "Coding"
                color: "white"
                font.pixelSize: 15
                Layout.preferredWidth: 100
            }

            TextField {
                id: hexField
                Layout.preferredWidth: 200
                text: codingCalculator.currentCodingHex
            }

            Button {
                text: "Decode"
                onClicked: codingCalculator.setCodingHex(hexField.text)
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: "Samples file"
                color: "white"
                font.pixelSize: 15
                Layout.preferredWidth: 100
            }

            TextField {
                id: sampleFileField
                Layout.preferredWidth: 250
                text: codingCalculator.currentSampleFile
                readOnly: true
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                text: "Sample name"
                color: "white"
                font.pixelSize: 15
                Layout.preferredWidth: 100
            }

            ComboBox {
                id: sampleBox
                Layout.preferredWidth: 300
                model: codingCalculator.sampleNames
                currentIndex: -1
                Component.onCompleted: currentIndex = -1
                onActivated: codingCalculator.selectSample(currentIndex)
            }

            Button {
                text: "Load"
                enabled: sampleBox.currentIndex >= 0
                onClicked: codingCalculator.loadCurrentSample()
            }

            Button {
                text: "Save"
                enabled: sampleBox.currentIndex >= 0 && hexField.text.length > 0
                onClicked: codingCalculator.saveCurrentSample()
            }

            Item {
                Layout.fillWidth: true
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                id: fieldsColumn
                width: Math.max(parent.width - 8, 750)
                spacing: 10

                Repeater {
                    model: codingCalculator.visibleFields

                    delegate: Item {
                        required property var modelData

                        width: fieldsColumn.width
                        height: 42

                        RowLayout {
                            anchors.fill: parent
                            spacing: 12

                            Label {
                                text: modelData.name || ""
                                color: "white"
                                font.pixelSize: 15
                                Layout.preferredWidth: 200
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }

                            ComboBox {
                                id: combo
                                visible: modelData.type === "enum"
                                Layout.preferredWidth: 300
                                model: modelData.options || []
                                textRole: "name"

                                Component.onCompleted: updateIndex()
                                onModelChanged: updateIndex()

                                function updateIndex() {
                                    currentIndex = -1
                                    if (!model)
                                        return
                                    for (let i = 0; i < model.length; ++i) {
                                        if (Number(model[i].value) === Number(modelData.currentValue)) {
                                            currentIndex = i
                                            return
                                        }
                                    }
                                }

                                onActivated: {
                                    if (currentIndex >= 0 && currentIndex < model.length) {
                                        codingCalculator.updateFieldValue(modelData.id, model[currentIndex].value)
                                    }
                                }
                            }

                            Switch {
                                visible: modelData.type === "bool"
                                checked: Number(modelData.currentValue) === 1
                                onToggled: codingCalculator.updateFieldValue(modelData.id, checked ? 1 : 0)
                            }

                            TextField {
                                visible: modelData.type === "uint"
                                Layout.preferredWidth: 180
                                text: String(modelData.currentValue ?? "")
                                horizontalAlignment: Text.AlignHCenter
                                validator: IntValidator {
                                    bottom: 0
                                    top: Number(modelData.maxValue)
                                }
                                onEditingFinished: {
                                    if (text.length > 0)
                                        codingCalculator.updateFieldValue(modelData.id, text)
                                }
                            }

                            Label {
                                visible: modelData.type === "uint"
                                text: "0-" + (modelData.maxValue || "")
                                color: "white"
                                font.pixelSize: 12
                            }

                            Item {
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: codingCalculator

        function onCurrentCodingChanged() {
            hexField.text = codingCalculator.currentCodingHex
        }

        function onCurrentDeviceChanged() {
            if (deviceCombo.currentIndex !== codingCalculator.currentDeviceIndex)
                deviceCombo.currentIndex = codingCalculator.currentDeviceIndex
            hexField.text = codingCalculator.currentCodingHex
        }

        function onSamplesChanged() {
            sampleBox.currentIndex = codingCalculator.currentSampleIndex
        }

        function onCurrentSampleChanged() {
            sampleBox.currentIndex = codingCalculator.currentSampleIndex
        }
    }
}